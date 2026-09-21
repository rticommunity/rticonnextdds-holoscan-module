/* ******************************************************************************
 *  (c) 2026 Copyright, Real-Time Innovations, Inc. All rights reserved.
 *  RTI Examples License.
 * *******************************************************************************/

#include <dds/dds.hpp>
#include <holoscan/holoscan.hpp>
#include <holoscan/operators/holoviz/holoviz.hpp>

#include <array>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "ShapeType.hpp"

namespace {
using ShapeType = ShapeTypeExtended;

struct Shape {
  enum class Kind { Square, Circle, Triangle };
  Kind kind;
  std::string color;
  float x, y, width, height;
};

std::vector<float> color_rgba(const std::string& color) {
  if (color == "RED") return {1.f, 0.f, 0.f, 1.f};
  if (color == "GREEN") return {0.f, 1.f, 0.f, 1.f};
  if (color == "BLUE") return {0.f, 0.f, 1.f, 1.f};
  if (color == "YELLOW") return {1.f, 1.f, 0.f, 1.f};
  if (color == "CYAN") return {0.f, 1.f, 1.f, 1.f};
  if (color == "MAGENTA") return {1.f, 0.f, 1.f, 1.f};
  if (color == "ORANGE") return {1.f, .5f, 0.f, 1.f};
  return {0.f, 0.f, 0.f, 1.f};
}

class ShapeSource : public holoscan::Operator {
 public:
  HOLOSCAN_OPERATOR_FORWARD_ARGS(ShapeSource)
  void setup(holoscan::OperatorSpec& spec) override {
    spec.output<std::vector<Shape>>("output");
    spec.param(topic_, "topic", "Topic", "Shapes Demo topic", std::string("Square"));
  }
  void compute(holoscan::InputContext&, holoscan::OutputContext& output,
               holoscan::ExecutionContext&) override {
    constexpr float canvas = 256.f, size = 40.f;
    x_ += vx_; y_ += vy_;
    if (x_ < size / 2 || x_ > canvas - size / 2) vx_ = -vx_;
    if (y_ < size / 2 || y_ > canvas - size / 2) vy_ = -vy_;
    const auto kind = topic_.get() == "Circle" ? Shape::Kind::Circle
                     : topic_.get() == "Triangle" ? Shape::Kind::Triangle
                     : Shape::Kind::Square;
    output.emit(std::vector<Shape>{{kind, "BLACK", x_ / canvas, y_ / canvas,
                                   size / canvas, size / canvas}}, "output");
  }
 private:
  holoscan::Parameter<std::string> topic_;
  float x_{64.f}, y_{96.f}, vx_{2.f}, vy_{1.5f};
};

class ShapesPublisher : public holoscan::Operator {
 public:
  HOLOSCAN_OPERATOR_FORWARD_ARGS(ShapesPublisher)
  void setup(holoscan::OperatorSpec& spec) override {
    spec.input<std::vector<Shape>>("input");
    spec.param(domain_id_, "domain_id", "Domain ID", "DDS domain", 0u);
    spec.param(topic_name_, "topic", "Topic", "Shapes Demo topic", std::string("Square"));
    spec.param(qos_file_, "qos_file", "QoS file", "RTI QoS XML", std::string("HoloscanConnextQos.xml"));
  }
  void initialize() override {
    holoscan::Operator::initialize();
    qos_provider_ = dds::core::QosProvider(qos_file_.get());
    participant_ = dds::domain::DomainParticipant(domain_id_.get());
    publisher_ = dds::pub::Publisher(participant_);
    topic_ = dds::topic::Topic<ShapeType>(participant_, topic_name_.get());
    writer_ = dds::pub::DataWriter<ShapeType>(publisher_, topic_,
        qos_provider_.datawriter_qos("HoloscanShapes::BestEffort"));
  }
  void compute(holoscan::InputContext& input, holoscan::OutputContext&,
               holoscan::ExecutionContext&) override {
    for (const auto& shape : input.receive<std::vector<Shape>>("input").value()) {
      ShapeType sample;
      sample.color = shape.color;
      sample.x = static_cast<int32_t>(shape.x * 235.f);
      sample.y = static_cast<int32_t>(shape.y * 265.f);
      sample.shapesize = static_cast<int32_t>(shape.width * 235.f);
      writer_.write(sample);
      ++published_;
    }
  }
  size_t published() const { return published_; }
 private:
  holoscan::Parameter<uint32_t> domain_id_;
  holoscan::Parameter<std::string> topic_name_, qos_file_;
  dds::core::QosProvider qos_provider_{dds::core::null};
  dds::domain::DomainParticipant participant_{dds::core::null};
  dds::pub::Publisher publisher_{dds::core::null};
  dds::topic::Topic<ShapeType> topic_{dds::core::null};
  dds::pub::DataWriter<ShapeType> writer_{dds::core::null};
  size_t published_{0};
};

class ShapesSubscriber : public holoscan::Operator {
 public:
  HOLOSCAN_OPERATOR_FORWARD_ARGS(ShapesSubscriber)
  void setup(holoscan::OperatorSpec& spec) override {
    spec.output<std::vector<Shape>>("output");
    spec.param(domain_id_, "domain_id", "Domain ID", "DDS domain", 0u);
    spec.param(qos_file_, "qos_file", "QoS file", "RTI QoS XML", std::string("HoloscanConnextQos.xml"));
  }
  void initialize() override {
    holoscan::Operator::initialize();
    qos_provider_ = dds::core::QosProvider(qos_file_.get());
    participant_ = dds::domain::DomainParticipant(domain_id_.get());
    subscriber_ = dds::sub::Subscriber(participant_);
    const auto qos = qos_provider_.datareader_qos("HoloscanShapes::BestEffort");
    square_ = dds::sub::DataReader<ShapeType>(subscriber_,
        dds::topic::Topic<ShapeType>(participant_, "Square"), qos);
    circle_ = dds::sub::DataReader<ShapeType>(subscriber_,
        dds::topic::Topic<ShapeType>(participant_, "Circle"), qos);
    triangle_ = dds::sub::DataReader<ShapeType>(subscriber_,
        dds::topic::Topic<ShapeType>(participant_, "Triangle"), qos);
  }
  void compute(holoscan::InputContext&, holoscan::OutputContext& output,
               holoscan::ExecutionContext&) override {
    std::vector<Shape> shapes;
    take(square_, Shape::Kind::Square, shapes);
    take(circle_, Shape::Kind::Circle, shapes);
    take(triangle_, Shape::Kind::Triangle, shapes);
    if (!shapes.empty()) output.emit(std::move(shapes), "output");
  }
 private:
  void take(dds::sub::DataReader<ShapeType>& reader, Shape::Kind kind,
            std::vector<Shape>& output) {
    for (const auto& sample : reader.take()) {
      if (!sample.info().valid()) continue;
      output.push_back({kind, sample.data().color, sample.data().x / 235.f,
                        sample.data().y / 265.f, sample.data().shapesize / 235.f,
                        sample.data().shapesize / 265.f});
    }
  }
  holoscan::Parameter<uint32_t> domain_id_;
  holoscan::Parameter<std::string> qos_file_;
  dds::core::QosProvider qos_provider_{dds::core::null};
  dds::domain::DomainParticipant participant_{dds::core::null};
  dds::sub::Subscriber subscriber_{dds::core::null};
  dds::sub::DataReader<ShapeType> square_{dds::core::null}, circle_{dds::core::null}, triangle_{dds::core::null};
};

class ShapesRenderer : public holoscan::Operator {
 public:
  HOLOSCAN_OPERATOR_FORWARD_ARGS(ShapesRenderer)
  void setup(holoscan::OperatorSpec& spec) override {
    spec.input<std::vector<Shape>>("local");
    // External DDS traffic is optional: the local source must still drive
    // Holoviz when no RTI Shapes Demo reader/writer is present.
    spec.input<std::vector<Shape>>("received").condition(holoscan::ConditionType::kNone);
    spec.output<holoscan::gxf::Entity>("outputs");
    spec.output<std::vector<holoscan::ops::HolovizOp::InputSpec>>("output_specs");
    spec.param(allocator_, "allocator", "Allocator", "Host tensor allocator");
  }
  void compute(holoscan::InputContext& input, holoscan::OutputContext& output,
               holoscan::ExecutionContext& context) override {
    auto entity = holoscan::gxf::Entity::New(&context);
    auto specs = std::vector<holoscan::ops::HolovizOp::InputSpec>();
    append(input, "local", entity, specs, context);
    append(input, "received", entity, specs, context);
    output.emit(entity, "outputs");
    output.emit(specs, "output_specs");
  }
 private:
  template <size_t N, size_t C>
  void tensor(holoscan::gxf::Entity& entity, const char* name,
              const std::array<std::array<float, C>, N>& values,
              holoscan::ExecutionContext& context) {
    auto allocator = nvidia::gxf::Handle<nvidia::gxf::Allocator>::Create(
        context.context(), allocator_->gxf_cid());
    auto value = static_cast<nvidia::gxf::Entity&>(entity).add<nvidia::gxf::Tensor>(name).value();
    value->reshape<float>(nvidia::gxf::Shape({N, C}), nvidia::gxf::MemoryStorageType::kHost,
                          allocator.value());
    std::memcpy(value->pointer(), values.data(), N * C * sizeof(float));
  }
  void append(holoscan::InputContext& input, const char* port,
              holoscan::gxf::Entity& entity,
              std::vector<holoscan::ops::HolovizOp::InputSpec>& specs,
              holoscan::ExecutionContext& context) {
    auto received = input.receive<std::vector<Shape>>(port);
    if (!received) return;
    for (const auto& shape : *received) {
      const auto name = std::to_string(specs.size());
      holoscan::ops::HolovizOp::InputSpec spec;
      spec.tensor_name_ = name;
      spec.color_ = color_rgba(shape.color);
      spec.priority_ = static_cast<int32_t>(specs.size());
      spec.line_width_ = 5.f;
      if (shape.kind == Shape::Kind::Square) {
        spec.type_ = holoscan::ops::HolovizOp::InputType::RECTANGLES;
        tensor<2, 2>(entity, name.c_str(), {{{shape.x - shape.width / 2, shape.y - shape.height / 2},
                                             {shape.x + shape.width / 2, shape.y + shape.height / 2}}}, context);
      } else if (shape.kind == Shape::Kind::Circle) {
        spec.type_ = holoscan::ops::HolovizOp::InputType::OVALS;
        tensor<1, 4>(entity, name.c_str(), {{{shape.x, shape.y, shape.width, shape.height}}}, context);
      } else {
        spec.type_ = holoscan::ops::HolovizOp::InputType::LINE_STRIP;
        tensor<4, 2>(entity, name.c_str(), {{{shape.x - shape.width / 2, shape.y + shape.height / 2},
                                             {shape.x + shape.width / 2, shape.y + shape.height / 2},
                                             {shape.x, shape.y - shape.height / 2},
                                             {shape.x - shape.width / 2, shape.y + shape.height / 2}}}, context);
      }
      specs.push_back(std::move(spec));
    }
  }
  holoscan::Parameter<std::shared_ptr<holoscan::Allocator>> allocator_;
};

class HeadlessShapeSink : public holoscan::Operator {
 public:
  HOLOSCAN_OPERATOR_FORWARD_ARGS(HeadlessShapeSink)
  void setup(holoscan::OperatorSpec& spec) override {
    spec.input<std::vector<Shape>>("local");
    spec.input<std::vector<Shape>>("received").condition(holoscan::ConditionType::kNone);
  }
  void compute(holoscan::InputContext& input, holoscan::OutputContext&,
               holoscan::ExecutionContext&) override {
    // Consume both branches so headless CI validates graph scheduling and DDS
    // setup without requiring Vulkan or a display server.
    (void)input.receive<std::vector<Shape>>("local");
    (void)input.receive<std::vector<Shape>>("received");
  }
};

struct Options { uint32_t domain{0}, samples{0}; bool headless{false}; std::string topic{"Square"}; };

void usage() {
  std::cout << "Holoscan 4.6 RTI Shapes + Holoviz\n"
            << "  --headless              run without a display\n"
            << "  --samples N             finite run (default 300)\n"
            << "  --domain-id N           DDS domain (default 0)\n"
            << "  --publish-topic TOPIC   Square, Circle, or Triangle\n";
}

Options parse_options(int argc, char** argv) {
  Options options;
  static option flags[] = {{"help", no_argument, nullptr, 'h'}, {"headless", no_argument, nullptr, 'H'},
    {"samples", required_argument, nullptr, 's'}, {"domain-id", required_argument, nullptr, 'd'},
    {"publish-topic", required_argument, nullptr, 't'}, {nullptr, 0, nullptr, 0}};
  while (true) {
    const int c = getopt_long(argc, argv, "hHs:d:t:", flags, nullptr);
    if (c == -1) break;
    if (c == 'h') { usage(); std::exit(0); }
    if (c == 'H') options.headless = true;
    if (c == 's') options.samples = std::stoul(optarg);
    if (c == 'd') options.domain = std::stoul(optarg);
    if (c == 't') options.topic = optarg;
  }
  if (options.samples == 0) options.samples = options.headless ? 40 : 300;
  return options;
}

class ShapesApplication : public holoscan::Application {
 public:
  explicit ShapesApplication(Options options) : options_(std::move(options)) {}
  void compose() override {
    using namespace holoscan;
    auto allocator = make_resource<UnboundedAllocator>("shape_allocator");
    auto source = make_operator<ShapeSource>("source", make_condition<CountCondition>(options_.samples), Arg("topic", options_.topic));
    auto publisher = make_operator<ShapesPublisher>("publisher", Arg("domain_id", options_.domain), Arg("topic", options_.topic));
    auto subscriber = make_operator<ShapesSubscriber>("subscriber", make_condition<CountCondition>(options_.samples), Arg("domain_id", options_.domain));
    if (options_.headless) {
      auto sink = make_operator<HeadlessShapeSink>(
          "headless_sink", make_condition<CountCondition>(options_.samples));
      add_flow(source, sink, {{"output", "local"}});
      add_flow(subscriber, sink, {{"output", "received"}});
      return;
    }
    auto renderer = make_operator<ShapesRenderer>("renderer", Arg("allocator", allocator));
    std::vector<ops::HolovizOp::InputSpec> tensors;
    auto holoviz = make_operator<ops::HolovizOp>("holoviz", Arg("tensors", tensors), Arg("headless", options_.headless),
        Arg("width", 800), Arg("height", 800), Arg("window_title", std::string("Connext Shapes + Holoviz")));
    add_flow(source, publisher, {{"output", "input"}});
    add_flow(source, renderer, {{"output", "local"}});
    add_flow(subscriber, renderer, {{"output", "received"}});
    add_flow(renderer, holoviz, {{"outputs", "receivers"}, {"output_specs", "input_specs"}});
  }
 private: Options options_;
};
}  // namespace

int main(int argc, char** argv) {
  try {
    auto options = parse_options(argc, argv);
    const auto sample_count = options.samples;
    const auto topic = options.topic;
    auto app = holoscan::make_application<ShapesApplication>(std::move(options));
    app->run();
    std::cout << "Published " << sample_count << " black " << topic << " samples\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "connext_shapes_holoviz failed: " << error.what() << '\n';
    return 2;
  }
}
