/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Real-Time Innovations, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>

#include <holoscan/core/compile.hpp>
#include <holoscan/core/connection_options.hpp>
#include <holoscan/core/graph.hpp>
#include <holoscan/core/run.hpp>
#include <holoscan/time/realtime_clock.hpp>

#include "Command.hpp"
#include "Telemetry.hpp"
#include "example_adapters.hpp"
#include "example_graph.hpp"
#include "rti/holoscan/dds/config.hpp"
#include "rti/holoscan/dds/subscriber.hpp"

int run_subscriber() {
  using Command = rti::holoscan::example::Command;
  using CommandAdapter = rti::holoscan::example::CommandAdapter;
  using Telemetry = rti::holoscan::example::Telemetry;
  using TelemetryAdapter = rti::holoscan::example::TelemetryAdapter;
  using SubscriberConfig = rti::holoscan::dds::EndpointConfig;
  using CommandSubscriber = rti::holoscan::dds::SubscriberOp<Command, CommandAdapter>;
  using TelemetrySubscriber = rti::holoscan::dds::SubscriberOp<Telemetry, TelemetryAdapter>;
  using rti::holoscan::example::Observation;
  using rti::holoscan::example::SequenceSink;
  using rti::holoscan::example::Stream;

  Observation::reset();
  holoscan::Graph graph{"connext-two-idl-subscriber"};

  const SubscriberConfig telemetry_config{.topic_name = "HoloscanTelemetry"};
  const SubscriberConfig command_config{.topic_name = "HoloscanCommand"};
  const auto telemetry_subscriber =
      graph.op<TelemetrySubscriber>("telemetry-subscriber", telemetry_config);
  const auto command_subscriber =
      graph.op<CommandSubscriber>("command-subscriber", command_config);
  const auto telemetry_sink = graph.op<SequenceSink>("telemetry-check", Stream::kTelemetry);
  const auto command_sink = graph.op<SequenceSink>("command-check", Stream::kCommand);

  graph.add_flow(telemetry_subscriber->output,
                 telemetry_sink->input,
                 holoscan::ConnectionOptions{.queue_depth = 32U});
  graph.add_flow(command_subscriber->output,
                 command_sink->input,
                 holoscan::ConnectionOptions{.queue_depth = 32U});
  graph.partition("telemetry-subscriber-partition")
      .add(telemetry_subscriber)
      .add(telemetry_sink);
  graph.partition("command-subscriber-partition")
      .add(command_subscriber)
      .add(command_sink);
  graph.set_default_clock(graph.add_clock<holoscan::RealtimeClock>("clock"));

  const holoscan::ExecutionPlan plan = holoscan::compile(graph);
  if (!plan.ok()) {
    std::cerr << plan.json() << '\n';
    return 1;
  }

  holoscan::RunSession session = holoscan::run_async(plan);
  const bool complete = Observation::wait_for_both(std::chrono::seconds{20});
  return rti::holoscan::example::finish_run(
      session, complete, "DDS subscriber received and validated Telemetry=20, Command=20 samples");
}

int main() {
  try {
    return run_subscriber();
  } catch (const std::exception& error) {
    std::cerr << "DDS subscriber failed: " << error.what() << '\n';
    return 2;
  }
}
