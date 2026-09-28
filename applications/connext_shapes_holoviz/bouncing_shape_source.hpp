/* *******************************************************************************
 * (c) 2026 Copyright, Real-Time Innovations, Inc. All rights reserved.
 * RTI grants Licensee a license to use, modify, compile, and create derivative
 * works of the Software. The Software is provided "as is", with no warranty.
 *******************************************************************************/

#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>
#include <string_view>
#include <utility>

#include <holoscan/core/execution_context.hpp>
#include <holoscan/core/operator.hpp>
#include <holoscan/core/operator_spec.hpp>
#include <holoscan/core/temporal_contract.hpp>

#include "shape_display_config.hpp"
#include "shape_schema_traits.hpp"

namespace rti::holoscan::example::shapes_holoviz {

class BouncingShapeSourceOp final : public ::holoscan::Operator<> {
 public:
  using ShapePayload = ::rti::holoscan::shapes::ShapeT;

  BouncingShapeSourceOp(std::string color, std::string_view topic) : color_(std::move(color)) {
    if (topic == "Circle") {
      x_ = 180.F; y_ = 180.F; vx_ = 0.8F; vy_ = 0.6F;
    } else if (topic == "Triangle") {
      x_ = 64.F; y_ = 190.F; vx_ = 2.2F; vy_ = -1.2F;
    }
  }

  void setup(::holoscan::OperatorSpec& spec) override {
    spec.output(output, "output").max_emits_per_compute(1U);
  }

  [[nodiscard]] ::holoscan::Contract contract() const override {
    ::holoscan::Contract result;
    result.trigger(::holoscan::OnClock{.period = std::chrono::milliseconds{33}});
    return result;
  }

  [[nodiscard]] ::holoscan::expected<void, ::holoscan::Error> compute(
      ::holoscan::ExecutionContext&) override {
    constexpr float lower = static_cast<float>(kShapeSize / 2);
    constexpr float upper = static_cast<float>(kLogicalCanvas - kShapeSize / 2);
    x_ += vx_;
    y_ += vy_;
    if (x_ <= lower || x_ >= upper) { vx_ = -vx_; x_ = std::clamp(x_, lower, upper); }
    if (y_ <= lower || y_ >= upper) { vy_ = -vy_; y_ = std::clamp(y_, lower, upper); }
    ShapePayload shape;
    shape.color = color_;
    shape.x = std::lround(x_);
    shape.y = std::lround(y_);
    shape.shape_size = kShapeSize;
    shape.fill_kind = ::rti::holoscan::shapes::ShapeFillKind_Solid;
    shape.angle = angle_ += 2.F;
    return output.emit(std::move(shape));
  }

  ::holoscan::Output<ShapePayload> output;

 private:
  std::string color_;
  float x_{80.F}, y_{110.F}, vx_{2.F}, vy_{1.5F}, angle_{0.F};
};

}  // namespace rti::holoscan::example::shapes_holoviz
