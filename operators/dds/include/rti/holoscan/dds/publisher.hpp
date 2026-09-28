/* *******************************************************************************
 * (c) 2026 Copyright, Real-Time Innovations, Inc. All rights reserved.
 * RTI grants Licensee a license to use, modify, compile, and create derivative
 * works of the Software. Licensee has the right to distribute object form only
 * for use with RTI products. The Software is provided "as is", with no warranty
 * of any type, including any warranty for fitness for any purpose. RTI is under no
 * obligation to maintain or support the Software. RTI shall not be liable for any
 * incidental or consequential damages arising out of the use or inability to use
 * the software.
 *******************************************************************************/

#pragma once

#include <concepts>
#include <chrono>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>

#include <dds/dds.hpp>

#include <holoscan/core/execution_context.hpp>
#include <holoscan/core/operator.hpp>
#include <holoscan/core/operator_spec.hpp>
#include <holoscan/core/temporal_contract.hpp>
#include <holoscan/core/domain/tensor.hpp>

#include "rti/holoscan/dds/config.hpp"

namespace rti::holoscan::dds {

template <typename Adapter, typename DdsType>
concept PublisherAdapter = requires(const typename Adapter::holoscan_type& value) {
  { Adapter::to_dds(value) } -> std::same_as<DdsType>;
};

template <typename DdsType, typename Adapter>
  requires PublisherAdapter<Adapter, DdsType>
class PublisherOp final : public ::holoscan::Operator<> {
 public:
  using PortType = typename Adapter::holoscan_type;

  explicit PublisherOp(const EndpointConfig& config) : config_(config) {}

  void setup(::holoscan::OperatorSpec& spec) override {
    if constexpr (std::same_as<PortType, ::holoscan::Tensor>) {
      spec.input(input, "input")
          .queue_depth(32U)
          .expects_tensor(Adapter::tensor_input_spec());
    } else {
      spec.input(input, "input").queue_depth(32U);
    }
  }

  [[nodiscard]] ::holoscan::Contract contract() const override {
    ::holoscan::Contract result;
    result.trigger(::holoscan::OnEach{input});
    return result;
  }

  void start() override {
    if (config_.topic_name.empty()) {
      throw std::invalid_argument{"DDS topic name must not be empty"};
    }

    const auto qos = detail::endpoint_qos(config_.qos_file, config_.qos_profile);
    participant_.emplace(config_.domain_id, qos.participant);
    publisher_.emplace(*participant_, qos.publisher);
    topic_.emplace(*participant_, config_.topic_name, qos.topic);
    writer_.emplace(*publisher_, *topic_, qos.writer);
    detail::register_process_local_publication(config_, writer_->instance_handle());

    if (!config_.wait_for_reader) {
      return;
    }
    if (config_.reader_match_timeout.count() <= 0 ||
        config_.reader_match_poll_interval.count() <= 0) {
      throw std::invalid_argument{"DDS reader match timeouts must be positive"};
    }
    const auto deadline = std::chrono::steady_clock::now() + config_.reader_match_timeout;
    while (std::chrono::steady_clock::now() < deadline) {
      if (writer_->publication_matched_status().current_count() > 0) {
        return;
      }
      std::this_thread::sleep_for(config_.reader_match_poll_interval);
    }
    throw std::runtime_error{"Timed out waiting for a compatible DDS reader"};
  }

  void stop() override {
    if (writer_) {
      try {
        writer_->wait_for_acknowledgments(
            ::dds::core::Duration::from_secs(config_.acknowledgment_timeout.count() / 1000.0));
      } catch (...) {
        // Teardown must still release the remaining DDS entities. End-to-end
        // delivery is asserted by the receiving application, not hidden here.
      }
    }
    if (writer_) {
      detail::unregister_process_local_publication(config_, writer_->instance_handle());
    }
    writer_.reset();
    topic_.reset();
    publisher_.reset();
    participant_.reset();
  }

  [[nodiscard]] ::holoscan::expected<void, ::holoscan::Error> compute(
      ::holoscan::ExecutionContext&) override {
    auto value = input.receive_data();
    if (!value) {
      return ::holoscan::make_unexpected(std::move(value).error());
    }
    if (!writer_) {
      return ::holoscan::make_unexpected(
          ::holoscan::Error{::holoscan::ErrorCode::kFailure, "DDS writer is not started"});
    }

    try {
      writer_->write(Adapter::to_dds(*value));
      return {};
    } catch (const std::exception& error) {
      return ::holoscan::make_unexpected(::holoscan::Error{
          ::holoscan::ErrorCode::kFailure, std::string{"DDS publish failed: "} + error.what()});
    } catch (...) {
      return ::holoscan::make_unexpected(
          ::holoscan::Error{::holoscan::ErrorCode::kFailure, "DDS publish failed"});
    }
  }

  ::holoscan::Input<PortType> input;

 private:
  EndpointConfig config_;
  std::optional<::dds::domain::DomainParticipant> participant_;
  std::optional<::dds::pub::Publisher> publisher_;
  std::optional<::dds::topic::Topic<DdsType>> topic_;
  std::optional<::dds::pub::DataWriter<DdsType>> writer_;
};

}  // namespace rti::holoscan::dds
