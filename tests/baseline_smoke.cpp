/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Real-Time Innovations, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <iostream>
#include <mutex>
#include <utility>

#include <dds/dds.hpp>

#include <holoscan/core/compile.hpp>
#include <holoscan/core/connection_options.hpp>
#include <holoscan/core/execution_context.hpp>
#include <holoscan/core/graph.hpp>
#include <holoscan/core/operator.hpp>
#include <holoscan/core/operator_spec.hpp>
#include <holoscan/core/run.hpp>
#include <holoscan/core/temporal_contract.hpp>
#include <holoscan/time/realtime_clock.hpp>

namespace {

using namespace std::chrono_literals;

struct BaselineObservation {
  static void reset() noexcept {
    const std::lock_guard lock(mutex);
    received = 0U;
  }

  static void record(std::uint32_t value) {
    {
      const std::lock_guard lock(mutex);
      received = value;
    }
    ready.notify_all();
  }

  [[nodiscard]] static bool wait_for_sample() {
    std::unique_lock lock(mutex);
    return ready.wait_for(lock, 10s, [] { return received != 0U; });
  }

  static inline std::mutex mutex;
  static inline std::condition_variable ready;
  static inline std::uint32_t received{};
};

class BaselineSource final : public holoscan::Operator<> {
 public:
  void setup(holoscan::OperatorSpec& spec) override { spec.output(output_, "output"); }

  [[nodiscard]] holoscan::Contract contract() const override {
    holoscan::Contract result;
    result.trigger(holoscan::OnClock{.period = 1ms});
    return result;
  }

  [[nodiscard]] holoscan::expected<void, holoscan::Error> compute(
      holoscan::ExecutionContext&) override {
    return output_.emit(++next_);
  }

  holoscan::Output<std::uint32_t> output_;

 private:
  std::uint32_t next_{};
};

class BaselineSink final : public holoscan::Operator<> {
 public:
  void setup(holoscan::OperatorSpec& spec) override {
    spec.input(input_, "input").queue_depth(1U);
  }

  [[nodiscard]] holoscan::expected<void, holoscan::Error> compute(
      holoscan::ExecutionContext&) override {
    auto value = input_.receive_data();
    if (!value) {
      return holoscan::make_unexpected(std::move(value).error());
    }
    BaselineObservation::record(*value);
    return {};
  }

  holoscan::Input<std::uint32_t> input_;
};

}  // namespace

int main() {
  try {
    BaselineObservation::reset();

    // Keeping the participant alive while the graph runs proves that both
    // runtimes can coexist in one process. No user data is published yet.
    dds::domain::DomainParticipant participant{0};

    holoscan::Graph graph{"holoscan-connext-baseline"};
    const auto source = graph.op<BaselineSource>("source");
    const auto sink = graph.op<BaselineSink>("sink");
    graph.add_flow(source->output_, sink->input_, holoscan::ConnectionOptions{.queue_depth = 1U});
    graph.partition("baseline-partition").add(source).add(sink);
    graph.set_default_clock(graph.add_clock<holoscan::RealtimeClock>("clock"));

    const holoscan::ExecutionPlan plan = holoscan::compile(graph);
    if (!plan.ok()) {
      std::cerr << plan.json() << '\n';
      return 1;
    }

    holoscan::RunSession session = holoscan::run_async(plan);
    const bool received = BaselineObservation::wait_for_sample();
    session.request_stop();
    session.wait();

    if (!received || session.termination() != holoscan::RunTermination::kStopped ||
        !session.diagnostics().empty()) {
      std::cerr << "Holoscan baseline graph did not stop cleanly"
                << " received=" << received
                << " termination=" << static_cast<int>(session.termination())
                << " diagnostics=" << session.diagnostics().size() << '\n';
      for (const holoscan::Diagnostic& diagnostic : session.diagnostics()) {
        std::cerr << "  diagnostic=" << diagnostic.token << '\n';
      }
      return 2;
    }

    participant.close();
    std::cout << "Holoscan 5 and RTI Connext DDS baseline passed\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "Baseline failed: " << error.what() << '\n';
    return 3;
  }
}
