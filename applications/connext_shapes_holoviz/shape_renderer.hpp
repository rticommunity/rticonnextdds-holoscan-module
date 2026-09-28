/* *******************************************************************************
 * (c) 2026 Copyright, Real-Time Innovations, Inc. All rights reserved.
 * RTI grants Licensee a license to use, modify, compile, and create derivative
 * works of the Software. The Software is provided "as is", with no warranty.
 *******************************************************************************/

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <holoscan/core/domain/tensor.hpp>
#include <holoscan/core/execution_context.hpp>
#include <holoscan/core/operator.hpp>
#include <holoscan/core/operator_spec.hpp>
#include <holoscan/core/tensor_contract.hpp>
#include <holoscan/core/tensor_output_loan.hpp>

#include "shape_display_config.hpp"
#include "shape_schema_traits.hpp"

namespace rti::holoscan::example::shapes_holoviz {

class ShapeRendererOp final : public ::holoscan::Operator<> {
 public:
  using ShapePayload = ::rti::holoscan::shapes::ShapeT;

  explicit ShapeRendererOp(std::string local_topic) : local_topic_(std::move(local_topic)) {}

  void setup(::holoscan::OperatorSpec& spec) override {
    spec.input(local, "local").queue_depth(32U);
    spec.input(square, "square").queue_depth(32U);
    spec.input(circle, "circle").queue_depth(32U);
    spec.input(triangle, "triangle").queue_depth(32U);
    ::holoscan::TensorOutputSpec image{};
    image.representation.memory_kind = ::holoscan::MemoryKind::kHost;
    image.representation.dtype = DLDataType{kDLUInt, 8U, 1U};
    image.representation.rank = 3U;
    image.bounds = ::holoscan::tensor_bounds(static_cast<std::size_t>(kCanvas) * kCanvas * 4U);
    image.storage = ::holoscan::TensorOutputStorage::kRuntimePool;
    spec.output(frame, "frame").produces_tensor(image);
  }

  [[nodiscard]] ::holoscan::expected<void, ::holoscan::Error> compute(
      ::holoscan::ExecutionContext& context) override {
    if (local.selection().state == ::holoscan::InputState::kSelected) update(local, local_shape_);
    else if (square.selection().state == ::holoscan::InputState::kSelected) update(square, square_shape_);
    else if (circle.selection().state == ::holoscan::InputState::kSelected) update(circle, circle_shape_);
    else if (triangle.selection().state == ::holoscan::InputState::kSelected) update(triangle, triangle_shape_);
    const std::array<std::int64_t, 3> shape{kCanvas, kCanvas, 4};
    auto loan = frame.allocate_tensor(::holoscan::TensorLoanRequest{.shape = shape, .dtype = DLDataType{kDLUInt, 8U, 1U}});
    if (!loan) {
      if (loan.error().code == ::holoscan::ErrorCode::kResourceExhausted) return {};
      return ::holoscan::make_unexpected(std::move(loan).error());
    }
    auto writer = loan->write_host();
    if (!writer) {
      if (writer.error().code == ::holoscan::ErrorCode::kResourceExhausted) return {};
      return ::holoscan::make_unexpected(std::move(writer).error());
    }
    auto* pixels = writer->data_as<std::uint8_t>();
    if (!pixels) return ::holoscan::make_unexpected(::holoscan::Error{::holoscan::ErrorCode::kFailure, "Expected uint8 canvas"});
    std::fill_n(pixels, writer->byte_size(), std::uint8_t{255});
    draw(pixels, local_shape_, local_topic_, false);
    draw(pixels, square_shape_, "Square", true);
    draw(pixels, circle_shape_, "Circle", true);
    draw(pixels, triangle_shape_, "Triangle", true);
    auto committed = std::move(*writer).commit();
    if (!committed) {
      if (committed.error().code == ::holoscan::ErrorCode::kResourceExhausted) return {};
      return ::holoscan::make_unexpected(std::move(committed).error());
    }
    auto emitted = frame.emit(std::move(*loan), ::holoscan::EmitOptions{.capture_time = context.activation_time()});
    if (!emitted && emitted.error().code == ::holoscan::ErrorCode::kResourceExhausted) return {};
    return emitted;
  }

  ::holoscan::Input<ShapePayload> local, square, circle, triangle;
  ::holoscan::Output<::holoscan::Tensor> frame;

 private:
  static void update(::holoscan::Input<ShapePayload>& input, std::optional<ShapePayload>& target) {
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

  static void draw(std::uint8_t* pixels, const std::optional<ShapePayload>& shape,
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
  std::optional<ShapePayload> local_shape_, square_shape_, circle_shape_, triangle_shape_;
};

}  // namespace rti::holoscan::example::shapes_holoviz
