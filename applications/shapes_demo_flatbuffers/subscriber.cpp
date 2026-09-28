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
#include "rti/holoscan/dds/config.hpp"
#include "rti/holoscan/dds/subscriber.hpp"
#include "shape_adapter.hpp"
#include "testing/shape_test_operators.hpp"

int run_subscriber() {
  using Subscriber = rti::holoscan::dds::SubscriberOp<
      ShapeTypeExtended, rti::holoscan::example::ShapeAdapter>;
  using rti::holoscan::example::testing::ShapeSampleVerifier;
  using rti::holoscan::example::testing::ValidatingShapeSinkOp;

  ShapeSampleVerifier::reset();
  holoscan::Graph graph{"connext-shapes-subscriber"};
  const rti::holoscan::dds::EndpointConfig base_config{
      .topic_name = "Square",
      .qos_profile = "HoloscanConnext::ShapesInterop",
  };
  const auto config = rti::holoscan::dds::with_environment_overrides(base_config);
  const auto subscriber = graph.op<Subscriber>("dds-shape-subscriber", config);
  const auto sink = graph.op<ValidatingShapeSinkOp>("received-shape-validator");
  graph.add_flow(subscriber->output, sink->input,
                 holoscan::ConnectionOptions{.queue_depth = 32U});
  graph.partition("shapes-subscriber").add(subscriber).add(sink);
  graph.set_default_clock(graph.add_clock<holoscan::RealtimeClock>("clock"));

  const holoscan::ExecutionPlan plan = holoscan::compile(graph);
  if (!plan.ok()) {
    std::cerr << plan.json() << '\n';
    return 1;
  }
  holoscan::RunSession session = holoscan::run_async(plan);
  const bool complete = ShapeSampleVerifier::wait_for_all(std::chrono::seconds{20});
  return rti::holoscan::example::testing::finish_verified_shapes_run(
      session, complete, "DDS Shapes subscriber received 20 ShapeTypeExtended samples");
}

int main() {
  try {
    return run_subscriber();
  } catch (const std::exception& error) {
    std::cerr << "DDS Shapes subscriber failed: " << error.what() << '\n';
    return 2;
  }
}
