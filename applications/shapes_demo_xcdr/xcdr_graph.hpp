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

#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <utility>

#include <holoscan/core/domain/tensor.hpp>
#include <holoscan/core/execution_context.hpp>
#include <holoscan/core/operator.hpp>
#include <holoscan/core/operator_spec.hpp>
#include <holoscan/core/temporal_contract.hpp>

#include "example_graph.hpp"
#include "shape_adapter.hpp"
#include "shape_xcdr.hpp"

namespace rti::holoscan::example {

using namespace std::chrono_literals;

class DdsShapeXcdrSource final : public ::holoscan::Operator<> {
 public:
  void setup(::holoscan::OperatorSpec& spec) override {
    spec.output(output, "output").produces_tensor(ShapeXcdrAdapter::tensor_output_spec());
  }

  [[nodiscard]] ::holoscan::Contract contract() const override {
    ::holoscan::Contract result;
    result.trigger(::holoscan::OnClock{.period = 20ms});
    return result;
  }

  [[nodiscard]] ::holoscan::expected<void, ::holoscan::Error> compute(
      ::holoscan::ExecutionContext&) override {
    if (emitted_ >= kShapeSampleCount) {
      return {};
    }

    // The application works with its normal generated Connext type. Only the
    // graph boundary is converted to an opaque XCDR byte Tensor.
    static constexpr std::array<const char*, 3U> colors{"BLUE", "ORANGE", "PURPLE"};
    ::ShapeTypeExtended shape;
    shape.color = colors[emitted_ % colors.size()];
    shape.x = 20 + static_cast<std::int32_t>(emitted_ * 5U);
    shape.y = 30 + static_cast<std::int32_t>(emitted_ * 3U);
    shape.shapesize = 24 + static_cast<std::int32_t>(emitted_ % 8U);
    shape.fillKind = ::ShapeFillKind::SOLID_FILL;
    shape.angle = static_cast<float>(emitted_) * 2.5F;
    ++emitted_;
    return output.emit(ShapeXcdrAdapter::encode(shape));
  }

  ::holoscan::Output<::holoscan::Tensor> output;

 private:
  std::uint32_t emitted_{0U};
};

class DdsShapeXcdrSink final : public ::holoscan::Operator<> {
 public:
  void setup(::holoscan::OperatorSpec& spec) override {
    spec.input(input, "input")
        .queue_depth(32U)
        .expects_tensor(ShapeXcdrAdapter::tensor_input_spec());
  }

  [[nodiscard]] ::holoscan::Contract contract() const override {
    ::holoscan::Contract result;
    result.trigger(::holoscan::OnEach{input});
    return result;
  }

  [[nodiscard]] ::holoscan::expected<void, ::holoscan::Error> compute(
      ::holoscan::ExecutionContext&) override {
    auto tensor = input.receive_data();
    if (!tensor) {
      return ::holoscan::make_unexpected(std::move(tensor).error());
    }

    // The application recovers its generated Connext type and accesses its
    // fields normally. Operators that only route the Tensor do not need this.
    const ::ShapeTypeExtended shape = ShapeXcdrAdapter::decode(*tensor);
    std::cout << "XCDR ShapeTypeExtended: color=" << shape.color << " x=" << shape.x
              << " y=" << shape.y << " size=" << shape.shapesize
              << " angle=" << shape.angle << '\n';
    ShapeObservation::record(ShapeAdapter::from_dds(shape));
    return {};
  }

  ::holoscan::Input<::holoscan::Tensor> input;
};

}  // namespace rti::holoscan::example
