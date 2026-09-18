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
  const rti::holoscan::dds::EndpointConfig base_config{
      .topic_name = "Square",
      .qos_profile = "HoloscanConnext::ShapesInterop",
  };
  const auto config = rti::holoscan::dds::with_environment_overrides(base_config);
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
