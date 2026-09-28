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
#include <string>
#include <utility>

#include <holoscan/core/execution_context.hpp>
#include <holoscan/core/operator.hpp>
#include <holoscan/core/operator_spec.hpp>
#include <holoscan/core/temporal_contract.hpp>

#include "shape_sample_verifier.hpp"

namespace rti::holoscan::example::testing {

using ShapePayload = ::rti::holoscan::shapes::ShapeT;
using namespace std::chrono_literals;

class DeterministicShapeSourceOp final : public ::holoscan::Operator<> {
 public:
  void setup(::holoscan::OperatorSpec& spec) override {
    spec.output(output, "output").max_emits_per_compute(1U);
  }

  [[nodiscard]] ::holoscan::Contract contract() const override {
    ::holoscan::Contract result;
    result.trigger(::holoscan::OnClock{.period = 20ms})
        .complete_after(::holoscan::ComputeCalls{.limit = kShapeSampleCount});
    return result;
  }

  [[nodiscard]] ::holoscan::expected<void, ::holoscan::Error> compute(
      ::holoscan::ExecutionContext&) override {
    static constexpr std::array<const char*, 3U> colors{"BLUE", "ORANGE", "PURPLE"};
    ShapePayload shape;
    shape.color = colors[emitted_ % colors.size()];
    shape.x = 20 + static_cast<std::int32_t>(emitted_ * 5U);
    shape.y = 30 + static_cast<std::int32_t>(emitted_ * 3U);
    shape.shape_size = 24 + static_cast<std::int32_t>(emitted_ % 8U);
    shape.fill_kind = ::rti::holoscan::shapes::ShapeFillKind_Solid;
    shape.angle = static_cast<float>(emitted_) * 2.5F;
    ++emitted_;
    return output.emit(std::move(shape));
  }

  ::holoscan::Output<ShapePayload> output;

 private:
  std::uint32_t emitted_{0U};
};

class ValidatingShapeSinkOp final : public ::holoscan::Operator<> {
 public:
  void setup(::holoscan::OperatorSpec& spec) override {
    spec.input(input, "input").queue_depth(32U);
  }

  [[nodiscard]] ::holoscan::Contract contract() const override {
    ::holoscan::Contract result;
    result.trigger(::holoscan::OnEach{input});
    return result;
  }

  [[nodiscard]] ::holoscan::expected<void, ::holoscan::Error> compute(
      ::holoscan::ExecutionContext&) override {
    auto shape = input.receive_data();
    if (!shape) {
      return ::holoscan::make_unexpected(std::move(shape).error());
    }
    std::cout << "ShapeTypeExtended: color=" << shape->color << " x=" << shape->x
              << " y=" << shape->y << " size=" << shape->shape_size
              << " angle=" << shape->angle << '\n';
    ShapeSampleVerifier::record(*shape);
    return {};
  }

  ::holoscan::Input<ShapePayload> input;
};

}  // namespace rti::holoscan::example::testing