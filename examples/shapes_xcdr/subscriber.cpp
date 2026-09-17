/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Real-Time Innovations, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>
#include <exception>
#include <iostream>

#include <holoscan/core/compile.hpp>
#include <holoscan/core/connection_options.hpp>
#include <holoscan/core/graph.hpp>
#include <holoscan/core/run.hpp>
#include <holoscan/time/realtime_clock.hpp>

#include "ShapeType.hpp"
#include "example_graph.hpp"
#include "rti/holoscan/dds/config.hpp"
#include "rti/holoscan/dds/subscriber.hpp"
#include "xcdr_graph.hpp"

int run_subscriber() {
  using Subscriber = rti::holoscan::dds::SubscriberOp<
      ShapeTypeExtended, rti::holoscan::example::ShapeXcdrAdapter>;
  using rti::holoscan::example::DdsShapeXcdrSink;
  using rti::holoscan::example::ShapeObservation;

  ShapeObservation::reset();
  holoscan::Graph graph{"connext-shapes-xcdr-subscriber"};
  const rti::holoscan::dds::EndpointConfig config{
      .topic_name = "Square",
      .qos_profile = "HoloscanConnext::ShapesInterop",
  };
  const auto subscriber = graph.op<Subscriber>("dds-shape-xcdr-subscriber", config);
  const auto sink = graph.op<DdsShapeXcdrSink>("received-dds-shape-observer");
  graph.add_flow(subscriber->output, sink->input,
                 holoscan::ConnectionOptions{.queue_depth = 32U});
  graph.partition("shapes-xcdr-subscriber").add(subscriber).add(sink);
  graph.set_default_clock(graph.add_clock<holoscan::RealtimeClock>("clock"));

  const holoscan::ExecutionPlan plan = holoscan::compile(graph);
  if (!plan.ok()) {
    std::cerr << plan.json() << '\n';
    return 1;
  }
  holoscan::RunSession session = holoscan::run_async(plan);
  const bool complete = ShapeObservation::wait_for_all(std::chrono::seconds{20});
  return rti::holoscan::example::finish_shapes_run(
      session, complete, "DDS XCDR subscriber received 20 ShapeTypeExtended samples");
}

int main() {
  try {
    return run_subscriber();
  } catch (const std::exception& error) {
    std::cerr << "DDS XCDR subscriber failed: " << error.what() << '\n';
    return 2;
  }
}
