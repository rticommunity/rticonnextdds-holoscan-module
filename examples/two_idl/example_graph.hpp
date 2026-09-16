/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Real-Time Innovations, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <utility>

#include <holoscan/core/execution_context.hpp>
#include <holoscan/core/operator.hpp>
#include <holoscan/core/operator_spec.hpp>
#include <holoscan/core/run.hpp>
#include <holoscan/core/temporal_contract.hpp>

namespace rti::holoscan::example {

using namespace std::chrono_literals;

constexpr std::uint32_t kSampleCount = 20U;

enum class Stream : std::size_t { kTelemetry = 0U, kCommand = 1U };

class Observation {
 public:
  static void reset() noexcept {
    const std::lock_guard lock{mutex_};
    counts_ = {0U, 0U};
    valid_ = true;
  }

  static void record(Stream stream, std::uint32_t value) noexcept {
    {
      const std::lock_guard lock{mutex_};
      const std::size_t index = static_cast<std::size_t>(stream);
      const std::uint32_t expected = counts_[index] + 1U;
      if (value != expected || counts_[index] >= kSampleCount) {
        valid_ = false;
      } else {
        counts_[index] = value;
      }
    }
    ready_.notify_all();
  }

  [[nodiscard]] static bool wait_for_both(std::chrono::seconds timeout) {
    std::unique_lock lock{mutex_};
    const bool complete = ready_.wait_for(lock, timeout, [] {
      return !valid_ ||
             (counts_[0] == kSampleCount && counts_[1] == kSampleCount);
    });
    return complete && valid_ && counts_[0] == kSampleCount && counts_[1] == kSampleCount;
  }

 private:
  static inline std::mutex mutex_;
  static inline std::condition_variable ready_;
  static inline std::array<std::uint32_t, 2U> counts_{};
  static inline bool valid_{true};
};

class SequenceSource final : public ::holoscan::Operator<> {
 public:
  void setup(::holoscan::OperatorSpec& spec) override { spec.output(output, "output"); }

  [[nodiscard]] ::holoscan::Contract contract() const override {
    ::holoscan::Contract result;
    result.trigger(::holoscan::OnClock{.period = 10ms});
    return result;
  }

  [[nodiscard]] ::holoscan::expected<void, ::holoscan::Error> compute(
      ::holoscan::ExecutionContext&) override {
    if (next_ >= kSampleCount) {
      return {};
    }
    return output.emit(++next_);
  }

  ::holoscan::Output<std::uint32_t> output;

 private:
  std::uint32_t next_{0U};
};

class SequenceSink final : public ::holoscan::Operator<> {
 public:
  explicit SequenceSink(Stream stream) noexcept : stream_(stream) {}

  void setup(::holoscan::OperatorSpec& spec) override {
    spec.input(input, "input").queue_depth(32U);
  }

  [[nodiscard]] ::holoscan::Contract contract() const override {
    ::holoscan::Contract result;
    result.trigger(::holoscan::OnEach{input});
    return result;
  }

  [[nodiscard]] ::holoscan::expected<void, ::holoscan::Error> compute(
      ::holoscan::ExecutionContext&) override {
    auto value = input.receive_data();
    if (!value) {
      return ::holoscan::make_unexpected(std::move(value).error());
    }
    Observation::record(stream_, *value);
    return {};
  }

  ::holoscan::Input<std::uint32_t> input;

 private:
  Stream stream_;
};

inline int finish_run(::holoscan::RunSession& session, bool complete, const char* success_message) {
  session.request_stop();
  session.wait();

  if (!complete || session.termination() != ::holoscan::RunTermination::kStopped ||
      !session.diagnostics().empty()) {
    std::cerr << "Example did not complete cleanly: complete=" << complete
              << " termination=" << static_cast<int>(session.termination())
              << " diagnostics=" << session.diagnostics().size() << '\n';
    for (const auto& diagnostic : session.diagnostics()) {
      std::cerr << "  diagnostic=" << diagnostic.token << '\n';
    }
    return 1;
  }

  std::cout << success_message << '\n';
  return 0;
}

}  // namespace rti::holoscan::example
