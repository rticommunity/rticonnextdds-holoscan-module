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
#include "rti/holoscan/dds/publisher.hpp"
#include "shape_adapter.hpp"

int run_publisher() {
  using Publisher = rti::holoscan::dds::PublisherOp<
      ShapeTypeExtended, rti::holoscan::example::ShapeAdapter>;
  using rti::holoscan::example::ShapeObservation;
  using rti::holoscan::example::ShapeSink;
  using rti::holoscan::example::ShapeSource;

  ShapeObservation::reset();
  holoscan::Graph graph{"connext-shapes-publisher"};
  const rti::holoscan::dds::EndpointConfig config{
      .topic_name = "Square",
      .qos_profile = "HoloscanConnext::ShapesInterop",
  };
  const auto source = graph.op<ShapeSource>("shape-source");
  const auto publisher = graph.op<Publisher>("dds-shape-publisher", config);
  const auto sink = graph.op<ShapeSink>("published-shape-observer");
  graph.add_flow(source->output, publisher->input,
                 holoscan::ConnectionOptions{.queue_depth = 32U});
  graph.add_flow(publisher->published, sink->input,
                 holoscan::ConnectionOptions{.queue_depth = 32U});
  graph.partition("shapes-publisher").add(source).add(publisher).add(sink);
  graph.set_default_clock(graph.add_clock<holoscan::RealtimeClock>("clock"));

  const holoscan::ExecutionPlan plan = holoscan::compile(graph);
  if (!plan.ok()) {
    std::cerr << plan.json() << '\n';
    return 1;
  }
  holoscan::RunSession session = holoscan::run_async(plan);
  const bool complete = ShapeObservation::wait_for_all(std::chrono::seconds{10});
  return rti::holoscan::example::finish_shapes_run(
      session, complete, "DDS Shapes publisher sent 20 ShapeTypeExtended samples");
}

int main() {
  try {
    return run_publisher();
  } catch (const std::exception& error) {
    std::cerr << "DDS Shapes publisher failed: " << error.what() << '\n';
    return 2;
  }
}
