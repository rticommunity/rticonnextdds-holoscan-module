/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Real-Time Innovations, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <chrono>
#include <concepts>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>

#include <dds/dds.hpp>

#include <holoscan/core/execution_context.hpp>
#include <holoscan/core/lifecycle.hpp>
#include <holoscan/core/operator.hpp>
#include <holoscan/core/operator_spec.hpp>
#include <holoscan/core/readiness_source.hpp>
#include <holoscan/core/temporal_contract.hpp>

#include "rti/holoscan/dds/config.hpp"

namespace rti::holoscan::dds {

template <typename Adapter, typename DdsType>
concept SubscriberAdapter = requires(const DdsType& value) {
  { Adapter::from_dds(value) } -> std::same_as<typename Adapter::holoscan_type>;
};

template <typename DdsType, typename Adapter>
  requires SubscriberAdapter<Adapter, DdsType>
class SubscriberOp final : public ::holoscan::Operator<> {
 public:
  using PortType = typename Adapter::holoscan_type;

  explicit SubscriberOp(const EndpointConfig& config) : config_(config) {}

  void setup(::holoscan::OperatorSpec& spec) override {
    spec.output(output, "output").max_emits_per_compute(32U);
    spec.notification_source(notification_, "dds-data")
        .capacity(1U)
        .sender_reference_capacity(1U);
    spec.lifecycle().stage(::holoscan::LifecycleStage::kArm, &SubscriberOp::arm);
  }

  [[nodiscard]] ::holoscan::Contract contract() const override {
    ::holoscan::Contract result;
    result.trigger(::holoscan::OnNotified{notification_});
    return result;
  }

  void start() override {
    if (config_.topic_name.empty()) {
      throw std::invalid_argument{"DDS topic name must not be empty"};
    }
    if (!notification_sender_) {
      throw std::runtime_error{"Holoscan notification sender was not armed"};
    }

    const auto qos = detail::endpoint_qos(config_.qos_file, config_.qos_profile);
    participant_.emplace(config_.domain_id, qos.participant);
    subscriber_.emplace(*participant_, qos.subscriber);
    topic_.emplace(*participant_, config_.topic_name, qos.topic);
    reader_.emplace(*subscriber_, *topic_, qos.reader);

    status_condition_.emplace(*reader_);
    status_condition_->enabled_statuses(::dds::core::status::StatusMask::data_available());
    waitset_ += *status_condition_;
    status_attached_ = true;
    waitset_ += stop_condition_;
    stop_attached_ = true;

    stop_requested_.store(false, std::memory_order_release);
    notification_outstanding_ = false;
    worker_failed_.store(false, std::memory_order_release);
    worker_ = std::thread([this] { wait_for_data(); });
  }

  void stop() override {
    stop_requested_.store(true, std::memory_order_release);
    stop_condition_.trigger_value(true);
    drained_.notify_all();
    if (worker_.joinable()) {
      worker_.join();
    }

    if (status_attached_ && status_condition_) {
      waitset_ -= *status_condition_;
      status_attached_ = false;
    }
    if (stop_attached_) {
      waitset_ -= stop_condition_;
      stop_attached_ = false;
    }
    status_condition_.reset();
    reader_.reset();
    topic_.reset();
    subscriber_.reset();
    participant_.reset();
    notification_sender_.reset();
    stop_condition_.trigger_value(false);
  }

  [[nodiscard]] ::holoscan::expected<void, ::holoscan::Error> compute(
      ::holoscan::ExecutionContext&) override {
    const auto retire_notification = [this] {
      {
        const std::lock_guard lock{notification_mutex_};
        notification_outstanding_ = false;
      }
      drained_.notify_one();
    };

    if (worker_failed_.load(std::memory_order_acquire)) {
      retire_notification();
      return ::holoscan::make_unexpected(
          ::holoscan::Error{::holoscan::ErrorCode::kFailure, "DDS WaitSet worker failed"});
    }
    if (!reader_) {
      retire_notification();
      return ::holoscan::make_unexpected(
          ::holoscan::Error{::holoscan::ErrorCode::kFailure, "DDS reader is not started"});
    }

    auto samples = reader_->take();
    for (const auto& sample : samples) {
      if (!sample.info().valid()) {
        continue;
      }
      auto emitted = output.emit(Adapter::from_dds(sample.data()));
      if (!emitted) {
        retire_notification();
        return ::holoscan::make_unexpected(std::move(emitted).error());
      }
    }

    retire_notification();
    return {};
  }

  ::holoscan::Output<PortType> output;

 private:
  ::holoscan::LifecycleStatus arm(::holoscan::LifecycleContext& context) noexcept {
    auto sender = context.notification_sender(notification_);
    if (!sender) {
      return ::holoscan::LifecycleStatus::kFatalFailure;
    }
    notification_sender_.emplace(std::move(*sender));
    return ::holoscan::LifecycleStatus::kOk;
  }

  void wait_for_data() noexcept {
    using namespace std::chrono_literals;
    try {
      while (!stop_requested_.load(std::memory_order_acquire)) {
        const auto active_conditions = waitset_.wait();
        if (stop_requested_.load(std::memory_order_acquire)) {
          break;
        }

        bool data_available = false;
        for (const auto& condition : active_conditions) {
          if (status_condition_ && condition == *status_condition_) {
            data_available = true;
            break;
          }
        }
        if (!data_available) {
          continue;
        }

        {
          const std::lock_guard lock{notification_mutex_};
          notification_outstanding_ = true;
        }

        while (!stop_requested_.load(std::memory_order_acquire)) {
          auto posted = notification_sender_->post();
          if (posted) {
            break;
          }
          if (posted.error().code != ::holoscan::ErrorCode::kNotReady &&
              posted.error().code != ::holoscan::ErrorCode::kBackpressured) {
            throw std::runtime_error{"Holoscan notification source closed unexpectedly"};
          }
          std::this_thread::sleep_for(1ms);
        }

        std::unique_lock lock{notification_mutex_};
        drained_.wait(lock, [this] {
          return !notification_outstanding_ ||
                 stop_requested_.load(std::memory_order_acquire);
        });
      }
    } catch (...) {
      worker_failed_.store(true, std::memory_order_release);
      if (notification_sender_) {
        (void)notification_sender_->post();
      }
    }
  }

  EndpointConfig config_;
  ::holoscan::NotificationSource notification_;
  std::optional<::holoscan::NotificationSender> notification_sender_;
  std::optional<::dds::domain::DomainParticipant> participant_;
  std::optional<::dds::sub::Subscriber> subscriber_;
  std::optional<::dds::topic::Topic<DdsType>> topic_;
  std::optional<::dds::sub::DataReader<DdsType>> reader_;
  std::optional<::dds::core::cond::StatusCondition> status_condition_;
  ::dds::core::cond::GuardCondition stop_condition_;
  ::dds::core::cond::WaitSet waitset_;
  std::thread worker_;
  std::atomic<bool> stop_requested_{false};
  std::atomic<bool> worker_failed_{false};
  std::mutex notification_mutex_;
  std::condition_variable drained_;
  bool notification_outstanding_{false};
  bool status_attached_{false};
  bool stop_attached_{false};
};

}  // namespace rti::holoscan::dds
