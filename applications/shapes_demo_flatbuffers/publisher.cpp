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

#include <exception>
#include <iostream>

#include <holoscan/core/compile.hpp>
#include <holoscan/core/graph.hpp>
#include <holoscan/core/run.hpp>
#include <holoscan/time/realtime_clock.hpp>

#include "ShapeType.hpp"
#include "rti/holoscan/dds/config.hpp"
#include "rti/holoscan/dds/publisher.hpp"
#include "shape_adapter.hpp"
#include "testing/shape_test_operators.hpp"

int run_publisher() {
  using Publisher = rti::holoscan::dds::PublisherOp<
      ShapeTypeExtended, rti::holoscan::example::ShapeAdapter>;
  using rti::holoscan::example::testing::DeterministicShapeSourceOp;

  holoscan::Graph graph{"connext-shapes-publisher"};
  const rti::holoscan::dds::EndpointConfig base_config{
      .topic_name = "Square",
      .qos_profile = "HoloscanConnext::ShapesInterop",
  };
  const auto config = rti::holoscan::dds::with_environment_overrides(base_config);
  const auto source = graph.op<DeterministicShapeSourceOp>("deterministic-shape-source");
  const auto publisher = graph.op<Publisher>("dds-shape-publisher", config);
  graph.add_flow(source->output, publisher->input,
                 holoscan::ConnectionOptions{.queue_depth = 32U});
  graph.partition("shapes-publisher").add(source).add(publisher);
  graph.set_default_clock(graph.add_clock<holoscan::RealtimeClock>("clock"));

  const holoscan::ExecutionPlan plan = holoscan::compile(graph);
  if (!plan.ok()) {
    std::cerr << plan.json() << '\n';
    return 1;
  }
  holoscan::RunSession session = holoscan::run_async(plan);
  session.wait();
  if (session.termination() != holoscan::RunTermination::kCompleted ||
      !session.diagnostics().empty()) {
    std::cerr << "DDS Shapes publisher did not complete cleanly: termination="
              << static_cast<int>(session.termination())
              << " diagnostics=" << session.diagnostics().size() << '\n';
    return 1;
  }
  std::cout << "DDS Shapes publisher sent 20 ShapeTypeExtended samples\n";
  return 0;
}

int main() {
  try {
    return run_publisher();
  } catch (const std::exception& error) {
    std::cerr << "DDS Shapes publisher failed: " << error.what() << '\n';
    return 2;
  }
}
