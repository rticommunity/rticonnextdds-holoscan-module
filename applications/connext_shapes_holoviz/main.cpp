/* *******************************************************************************
 * (c) 2026 Copyright, Real-Time Innovations, Inc. All rights reserved.
 * RTI grants Licensee a license to use, modify, compile, and create derivative
 * works of the Software. The Software is provided "as is", with no warranty.
 *******************************************************************************/

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include <holoscan/core/compile.hpp>
#include <holoscan/core/connection_options.hpp>
#include <holoscan/core/domain/tensor.hpp>
#include <holoscan/core/graph.hpp>
#include <holoscan/core/operator.hpp>
#include <holoscan/core/operator_spec.hpp>
#include <holoscan/core/run.hpp>
#include <holoscan/core/tensor_contract.hpp>
#include <holoscan/core/tensor_output_loan.hpp>
#include <holoscan/core/temporal_contract.hpp>
#include <holoscan/operators/holoviz/holoviz_op.hpp>
#include <holoscan/time/realtime_clock.hpp>
#include <holoviz/layer_desc.hpp>
#include <holoviz/sink_desc.hpp>

#include "ShapeType.hpp"
#include "rti/holoscan/dds/config.hpp"
#include "rti/holoscan/dds/publisher.hpp"
#include "rti/holoscan/dds/subscriber.hpp"
#include "shape_adapter.hpp"

namespace {

using ShapesPayload = rti::holoscan::shapes::ShapeT;
using Publisher = rti::holoscan::dds::PublisherOp<ShapeTypeExtended, rti::holoscan::example::ShapeAdapter>;
using Subscriber = rti::holoscan::dds::SubscriberOp<ShapeTypeExtended, rti::holoscan::example::ShapeAdapter>;
using namespace std::chrono_literals;

constexpr std::int32_t kLogicalCanvas = 256;
constexpr std::int32_t kRenderScale = 3;
constexpr std::int32_t kCanvas = kLogicalCanvas * kRenderScale;
constexpr std::int32_t kShapeSize = 30;
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

class BouncingShapeSource final : public holoscan::Operator<> {
 public:
  BouncingShapeSource(std::string color, std::string_view topic) : color_(std::move(color)) {
    if (topic == "Circle") {
      x_ = 180.F; y_ = 180.F; vx_ = 0.8F; vy_ = 0.6F;
    } else if (topic == "Triangle") {
      x_ = 64.F; y_ = 190.F; vx_ = 2.2F; vy_ = -1.2F;
    }
  }
  void setup(holoscan::OperatorSpec& spec) override { spec.output(output, "output").max_emits_per_compute(1U); }
  [[nodiscard]] holoscan::Contract contract() const override {
    holoscan::Contract result;
    result.trigger(holoscan::OnClock{.period = 33ms});
    return result;
  }
  [[nodiscard]] holoscan::expected<void, holoscan::Error> compute(holoscan::ExecutionContext&) override {
    constexpr float lower = static_cast<float>(kShapeSize / 2);
    constexpr float upper = static_cast<float>(kLogicalCanvas - kShapeSize / 2);
    x_ += vx_;
    y_ += vy_;
    if (x_ <= lower || x_ >= upper) { vx_ = -vx_; x_ = std::clamp(x_, lower, upper); }
    if (y_ <= lower || y_ >= upper) { vy_ = -vy_; y_ = std::clamp(y_, lower, upper); }
    ShapesPayload shape;
    shape.color = color_;
    shape.x = std::lround(x_);
    shape.y = std::lround(y_);
    shape.shape_size = kShapeSize;
    shape.fill_kind = rti::holoscan::shapes::ShapeFillKind_Solid;
    shape.angle = angle_ += 2.F;
    return output.emit(std::move(shape));
  }
  holoscan::Output<ShapesPayload> output;
 private:
  std::string color_;
  float x_{80.F}, y_{110.F}, vx_{2.F}, vy_{1.5F}, angle_{0.F};
};

class ShapeDiscard final : public holoscan::Operator<> {
 public:
  void setup(holoscan::OperatorSpec& spec) override { spec.input(input, "input").queue_depth(32U); }
  [[nodiscard]] holoscan::Contract contract() const override {
    holoscan::Contract result; result.trigger(holoscan::OnEach{input}); return result;
  }
  [[nodiscard]] holoscan::expected<void, holoscan::Error> compute(holoscan::ExecutionContext&) override {
    auto sample = input.receive_data();
    if (!sample) return holoscan::make_unexpected(std::move(sample).error());
    return {};
  }
  holoscan::Input<ShapesPayload> input;
};

class ShapesCanvas final : public holoscan::Operator<> {
 public:
  explicit ShapesCanvas(std::string local_topic) : local_topic_(std::move(local_topic)) {}
  void setup(holoscan::OperatorSpec& spec) override {
    spec.input(local, "local").queue_depth(32U);
    spec.input(square, "square").queue_depth(32U);
    spec.input(circle, "circle").queue_depth(32U);
    spec.input(triangle, "triangle").queue_depth(32U);
    holoscan::TensorOutputSpec image{};
    image.representation.memory_kind = holoscan::MemoryKind::kHost;
    image.representation.dtype = DLDataType{kDLUInt, 8U, 1U};
    image.representation.rank = 3U;
    image.bounds = holoscan::tensor_bounds(static_cast<std::size_t>(kCanvas) * kCanvas * 4U);
    image.storage = holoscan::TensorOutputStorage::kRuntimePool;
    spec.output(frame, "frame").produces_tensor(image);
  }
  [[nodiscard]] holoscan::expected<void, holoscan::Error> compute(holoscan::ExecutionContext& context) override {
    if (local.selection().state == holoscan::InputState::kSelected) update(local, local_shape_);
    else if (square.selection().state == holoscan::InputState::kSelected) update(square, square_shape_);
    else if (circle.selection().state == holoscan::InputState::kSelected) update(circle, circle_shape_);
    else if (triangle.selection().state == holoscan::InputState::kSelected) update(triangle, triangle_shape_);
    const std::array<std::int64_t, 3> shape{kCanvas, kCanvas, 4};
    auto loan = frame.allocate_tensor(holoscan::TensorLoanRequest{.shape = shape, .dtype = DLDataType{kDLUInt, 8U, 1U}});
    if (!loan) {
      if (loan.error().code == holoscan::ErrorCode::kResourceExhausted) return {};
      return holoscan::make_unexpected(std::move(loan).error());
    }
    auto writer = loan->write_host();
    if (!writer) {
      if (writer.error().code == holoscan::ErrorCode::kResourceExhausted) return {};
      return holoscan::make_unexpected(std::move(writer).error());
    }
    auto* pixels = writer->data_as<std::uint8_t>();
    if (!pixels) return holoscan::make_unexpected(holoscan::Error{holoscan::ErrorCode::kFailure, "Expected uint8 canvas"});
    std::fill_n(pixels, writer->byte_size(), std::uint8_t{255});
    draw(pixels, local_shape_, local_topic_, false);
    draw(pixels, square_shape_, "Square", true);
    draw(pixels, circle_shape_, "Circle", true);
    draw(pixels, triangle_shape_, "Triangle", true);
    auto committed = std::move(*writer).commit();
    if (!committed) {
      if (committed.error().code == holoscan::ErrorCode::kResourceExhausted) return {};
      return holoscan::make_unexpected(std::move(committed).error());
    }
    auto emitted = frame.emit(std::move(*loan), holoscan::EmitOptions{.capture_time = context.activation_time()});
    if (!emitted && emitted.error().code == holoscan::ErrorCode::kResourceExhausted) return {};
    return emitted;
  }
  holoscan::Input<ShapesPayload> local, square, circle, triangle;
  holoscan::Output<holoscan::Tensor> frame;
 private:
  static void update(holoscan::Input<ShapesPayload>& input, std::optional<ShapesPayload>& target) {
    auto sample = input.receive_data();
    if (sample) target = std::move(*sample);
  }
  static std::array<std::uint8_t, 3> color(std::string_view name) {
    if (name == "BLACK") return {0, 0, 0};
    if (name == "RED") return {255, 0, 0};
    if (name == "GREEN") return {0, 180, 0};
    if (name == "BLUE") return {0, 0, 255};
    if (name == "YELLOW") return {255, 220, 0};
    if (name == "ORANGE") return {255, 128, 0};
    if (name == "PURPLE") return {128, 0, 180};
    return {90, 90, 90};
  }
  static void put(std::uint8_t* pixels, int x, int y, const std::array<std::uint8_t, 3>& rgb) {
    if (x < 0 || x >= kCanvas || y < 0 || y >= kCanvas) return;
    const auto offset = (static_cast<std::size_t>(y) * kCanvas + x) * 4U;
    pixels[offset] = rgb[0]; pixels[offset + 1] = rgb[1]; pixels[offset + 2] = rgb[2]; pixels[offset + 3] = 255;
  }
  static bool contains(std::string_view topic, int dx, int dy, int radius) {
    if (radius < 1) return false;
    if (topic == "Square") return std::abs(dx) <= radius && std::abs(dy) <= radius;
    if (topic == "Circle") return dx * dx + dy * dy <= radius * radius;
    return dy >= -radius && std::abs(dx) <= radius - dy / 2;
  }
  static void draw(std::uint8_t* pixels, const std::optional<ShapesPayload>& shape,
                   std::string_view topic, bool external) {
    if (!shape) return;
    const auto rgb = color(shape->color);
    constexpr std::array<std::uint8_t, 3> external_border{0, 0, 128};
    const int center_x = shape->x * kRenderScale;
    const int center_y = shape->y * kRenderScale;
    const int radius = std::max(2, shape->shape_size / 2) * kRenderScale;
    for (int dy = -radius; dy <= radius; ++dy) for (int dx = -radius; dx <= radius; ++dx) {
      if (!contains(topic, dx, dy, radius)) continue;
      const bool inner = contains(topic, dx, dy, radius - 2 * kRenderScale);
      put(pixels, center_x + dx, center_y + dy, external && !inner ? external_border : rgb);
    }
  }
  std::string local_topic_;
  std::optional<ShapesPayload> local_shape_, square_shape_, circle_shape_, triangle_shape_;
};

int run(const Options& options) {
  auto publisher_config = endpoint(options.domain_id, options.publish_topic);
  publisher_config.wait_for_reader = false;
  holoscan::Graph graph{"connext-shapes-holoviz"};
  const auto source = graph.op<BouncingShapeSource>("local-shape-source", options.publish_color, options.publish_topic);
  const auto publisher = graph.op<Publisher>("dds-shape-publisher", publisher_config);
  const auto published = graph.op<ShapeDiscard>("published-shape-discard");
  const auto square = graph.op<Subscriber>("dds-square-subscriber", endpoint(options.domain_id, "Square"));
  const auto circle = graph.op<Subscriber>("dds-circle-subscriber", endpoint(options.domain_id, "Circle"));
  const auto triangle = graph.op<Subscriber>("dds-triangle-subscriber", endpoint(options.domain_id, "Triangle"));
  const auto canvas = graph.op<ShapesCanvas>("shapes-canvas", options.publish_topic);
  ::holoviz::LayerStack layers;
  layers.add_image_layer("frame", ::holoviz::ImageLayerDesc{.format = ::holoviz::ImageFormat::kR8G8B8A8Unorm, .max_width = kCanvas, .max_height = kCanvas});
  const auto holoviz = graph.op<holoscan::holoviz::HolovizOp>("holoviz", std::move(layers), ::holoviz::WindowSinkDesc{.title = "Connext Shapes + Holoviz", .width = options.width, .height = options.height}, 16ms, 100ms);
  const holoscan::ConnectionOptions flow{.queue_depth = 32U};
  graph.add_flow(source->output, publisher->input, flow);
  graph.add_flow(publisher->published, published->input, flow);
  graph.add_flow(source->output, canvas->local, flow);
  graph.add_flow(square->output, canvas->square, flow);
  graph.add_flow(circle->output, canvas->circle, flow);
  graph.add_flow(triangle->output, canvas->triangle, flow);
  graph.add_flow(canvas->frame, holoviz->layer_input(0), flow);
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
