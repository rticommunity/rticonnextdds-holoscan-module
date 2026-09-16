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
#include "rti/holoscan/dds/publisher.hpp"

int run_publisher() {
  using Command = rti::holoscan::example::Command;
  using CommandAdapter = rti::holoscan::example::CommandAdapter;
  using Telemetry = rti::holoscan::example::Telemetry;
  using TelemetryAdapter = rti::holoscan::example::TelemetryAdapter;
  using PublisherConfig = rti::holoscan::dds::EndpointConfig;
  using CommandPublisher = rti::holoscan::dds::PublisherOp<Command, CommandAdapter>;
  using TelemetryPublisher = rti::holoscan::dds::PublisherOp<Telemetry, TelemetryAdapter>;
  using rti::holoscan::example::Observation;
  using rti::holoscan::example::SequenceSink;
  using rti::holoscan::example::SequenceSource;
  using rti::holoscan::example::Stream;

  Observation::reset();
  holoscan::Graph graph{"connext-two-idl-publisher"};

  const PublisherConfig telemetry_config{.topic_name = "HoloscanTelemetry"};
  const PublisherConfig command_config{.topic_name = "HoloscanCommand"};
  const auto telemetry_source = graph.op<SequenceSource>("telemetry-source");
  const auto command_source = graph.op<SequenceSource>("command-source");
  const auto telemetry_publisher =
      graph.op<TelemetryPublisher>("telemetry-publisher", telemetry_config);
  const auto command_publisher = graph.op<CommandPublisher>("command-publisher", command_config);
  const auto telemetry_sink = graph.op<SequenceSink>("telemetry-complete", Stream::kTelemetry);
  const auto command_sink = graph.op<SequenceSink>("command-complete", Stream::kCommand);

  graph.add_flow(telemetry_source->output,
                 telemetry_publisher->input,
                 holoscan::ConnectionOptions{.queue_depth = 32U});
  graph.add_flow(telemetry_publisher->published,
                 telemetry_sink->input,
                 holoscan::ConnectionOptions{.queue_depth = 32U});
  graph.add_flow(command_source->output,
                 command_publisher->input,
                 holoscan::ConnectionOptions{.queue_depth = 32U});
  graph.add_flow(command_publisher->published,
                 command_sink->input,
                 holoscan::ConnectionOptions{.queue_depth = 32U});
  graph.partition("telemetry-publisher-partition")
      .add(telemetry_source)
      .add(telemetry_publisher)
      .add(telemetry_sink);
  graph.partition("command-publisher-partition")
      .add(command_source)
      .add(command_publisher)
      .add(command_sink);
  graph.set_default_clock(graph.add_clock<holoscan::RealtimeClock>("clock"));

  const holoscan::ExecutionPlan plan = holoscan::compile(graph);
  if (!plan.ok()) {
    std::cerr << plan.json() << '\n';
    return 1;
  }

  holoscan::RunSession session = holoscan::run_async(plan);
  const bool complete = Observation::wait_for_both(std::chrono::seconds{10});
  return rti::holoscan::example::finish_run(
      session, complete, "DDS publisher sent Telemetry=20, Command=20 samples");
}

int main() {
  try {
    return run_publisher();
  } catch (const std::exception& error) {
    std::cerr << "DDS publisher failed: " << error.what() << '\n';
    return 2;
  }
}
