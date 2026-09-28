/* *******************************************************************************
 * (c) 2026 Copyright, Real-Time Innovations, Inc. All rights reserved.
 * RTI grants Licensee a license to use, modify, compile, and create derivative
 * works of the Software. The Software is provided "as is", with no warranty.
 *******************************************************************************/

#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include <holoscan/core/compile.hpp>
#include <holoscan/core/connection_options.hpp>
#include <holoscan/core/graph.hpp>
#include <holoscan/core/run.hpp>
#include <holoscan/operators/holoviz/holoviz_op.hpp>
#include <holoscan/time/realtime_clock.hpp>
#include <holoviz/layer_desc.hpp>
#include <holoviz/sink_desc.hpp>

#include "ShapeType.hpp"
#include "bouncing_shape_source.hpp"
#include "rti/holoscan/dds/config.hpp"
#include "rti/holoscan/dds/publisher.hpp"
#include "rti/holoscan/dds/subscriber.hpp"
#include "shape_adapter.hpp"
#include "shape_display_config.hpp"
#include "shape_renderer.hpp"

namespace {

using Publisher = rti::holoscan::dds::PublisherOp<ShapeTypeExtended, rti::holoscan::example::ShapeAdapter>;
using Subscriber = rti::holoscan::dds::SubscriberOp<ShapeTypeExtended, rti::holoscan::example::ShapeAdapter>;
using rti::holoscan::example::shapes_holoviz::BouncingShapeSourceOp;
using rti::holoscan::example::shapes_holoviz::ShapeRendererOp;
using rti::holoscan::example::shapes_holoviz::kCanvas;
using namespace std::chrono_literals;

constexpr std::array<std::string_view, 3> kTopics{"Square", "Circle", "Triangle"};
std::atomic_bool stop_requested{false};

void stop_on_signal(int) { stop_requested.store(true, std::memory_order_release); }

struct Options {
  std::string publish_topic{"Square"};
  std::string publish_color{"BLACK"};
  std::uint32_t domain_id{0};
  std::uint32_t width{768};
  std::uint32_t height{768};
};

Options parse_options(int argc, char** argv) {
  Options result;
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg{argv[i]};
    const auto value = [&]() -> std::string {
      if (++i == argc) throw std::invalid_argument{"Missing value for " + std::string{arg}};
      return argv[i];
    };
    if (arg == "--publish-topic") result.publish_topic = value();
    else if (arg == "--publish-color") result.publish_color = value();
    else if (arg == "--domain-id") result.domain_id = std::stoul(value());
    else if (arg == "--width") result.width = std::stoul(value());
    else if (arg == "--height") result.height = std::stoul(value());
    else if (arg == "--help") {
      std::cout << "Usage: connext_shapes_holoviz [--publish-topic Square|Circle|Triangle] "
                   "[--publish-color COLOR] [--domain-id ID] [--width PX] [--height PX]\n";
      std::exit(0);
    } else {
      throw std::invalid_argument{"Unknown option " + std::string{arg}};
    }
  }
  if (std::ranges::find(kTopics, std::string_view{result.publish_topic}) == kTopics.end()) {
    throw std::invalid_argument{"--publish-topic must be Square, Circle, or Triangle"};
  }
  return result;
}

rti::holoscan::dds::EndpointConfig endpoint(std::uint32_t domain, std::string topic) {
  rti::holoscan::dds::EndpointConfig config;
  config.domain_id = domain;
  config.topic_name = std::move(topic);
  config.qos_profile = "HoloscanConnext::ShapesInterop";
  config.ignore_process_local_publications = true;
  return config;
}

int run(const Options& options) {
  auto publisher_config = endpoint(options.domain_id, options.publish_topic);
  publisher_config.wait_for_reader = false;
  holoscan::Graph graph{"connext-shapes-holoviz"};
  const auto source = graph.op<BouncingShapeSourceOp>("local-shape-source", options.publish_color, options.publish_topic);
  const auto publisher = graph.op<Publisher>("dds-shape-publisher", publisher_config);
  const auto square = graph.op<Subscriber>("dds-square-subscriber", endpoint(options.domain_id, "Square"));
  const auto circle = graph.op<Subscriber>("dds-circle-subscriber", endpoint(options.domain_id, "Circle"));
  const auto triangle = graph.op<Subscriber>("dds-triangle-subscriber", endpoint(options.domain_id, "Triangle"));
  const auto renderer = graph.op<ShapeRendererOp>("shape-renderer", options.publish_topic);
  ::holoviz::LayerStack layers;
  layers.add_image_layer("frame", ::holoviz::ImageLayerDesc{.format = ::holoviz::ImageFormat::kR8G8B8A8Unorm, .max_width = kCanvas, .max_height = kCanvas});
  const auto holoviz = graph.op<holoscan::holoviz::HolovizOp>("holoviz", std::move(layers), ::holoviz::WindowSinkDesc{.title = "Connext Shapes + Holoviz", .width = options.width, .height = options.height}, 16ms, 100ms);
  const holoscan::ConnectionOptions flow{.queue_depth = 32U};
  graph.add_flow(source->output, publisher->input, flow);
  graph.add_flow(source->output, renderer->local, flow);
  graph.add_flow(square->output, renderer->square, flow);
  graph.add_flow(circle->output, renderer->circle, flow);
  graph.add_flow(triangle->output, renderer->triangle, flow);
  graph.add_flow(renderer->frame, holoviz->layer_input(0), flow);
  graph.partition("connext-shapes-holoviz").include_all();
  graph.set_default_clock(graph.add_clock<holoscan::RealtimeClock>("clock"));
  const auto plan = holoscan::compile(graph);
  if (!plan.ok()) { std::cerr << plan.json() << '\n'; return 1; }
  std::cout << "Connext Shapes Holoviz running: publishing " << options.publish_color << ' ' << options.publish_topic << " on domain " << options.domain_id << ". Press Ctrl+C to stop.\n";
  auto session = holoscan::run_async(plan);
  while (!stop_requested.load(std::memory_order_acquire) && session.termination() == holoscan::RunTermination::kRunning) std::this_thread::sleep_for(100ms);
  session.request_stop();
  session.wait();
  return session.diagnostics().empty() ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    std::signal(SIGINT, stop_on_signal);
    std::signal(SIGTERM, stop_on_signal);
    return run(parse_options(argc, argv));
  } catch (const std::exception& error) {
    std::cerr << "Connext Shapes Holoviz failed: " << error.what() << '\n';
    return 2;
  }
}
