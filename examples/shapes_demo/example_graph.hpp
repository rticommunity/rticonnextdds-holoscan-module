/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Real-Time Innovations, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <string>
#include <utility>

#include <holoscan/core/execution_context.hpp>
#include <holoscan/core/operator.hpp>
#include <holoscan/core/operator_spec.hpp>
#include <holoscan/core/run.hpp>
#include <holoscan/core/temporal_contract.hpp>

#include "shape_schema_traits.hpp"

namespace rti::holoscan::example {

using ShapesPayload = ::rti::holoscan::shapes::ShapeT;
using namespace std::chrono_literals;

inline constexpr std::uint32_t kShapeSampleCount = 20U;

class ShapeObservation {
 public:
  static void reset() noexcept {
    const std::lock_guard lock{mutex_};
    received_ = 0U;
    valid_ = true;
    seen_.fill(false);
  }

  static void record(const ShapesPayload& shape) {
    {
      const std::lock_guard lock{mutex_};
      static constexpr std::array<const char*, 3U> colors{"BLUE", "ORANGE", "PURPLE"};
      const auto offset = shape.x - 20;
      if (offset < 0 || offset % 5 != 0) {
        valid_ = false;
      } else {
        const auto index = static_cast<std::uint32_t>(offset / 5);
        if (index >= kShapeSampleCount || seen_[index] ||
            shape.color != colors[index % colors.size()] ||
            shape.y != 30 + static_cast<std::int32_t>(index * 3U) ||
            shape.shape_size != 24 + static_cast<std::int32_t>(index % 8U) ||
            shape.fill_kind != ::rti::holoscan::shapes::ShapeFillKind_Solid ||
            shape.angle != static_cast<float>(index) * 2.5F) {
          valid_ = false;
        } else {
          seen_[index] = true;
          ++received_;
        }
      }
    }
    ready_.notify_all();
  }

  [[nodiscard]] static bool wait_for_all(std::chrono::seconds timeout) {
    std::unique_lock lock{mutex_};
    const bool complete = ready_.wait_for(lock, timeout, [] {
      return !valid_ || received_ >= kShapeSampleCount;
    });
    return complete && valid_ && received_ == kShapeSampleCount;
  }

 private:
  static inline std::mutex mutex_;
  static inline std::condition_variable ready_;
  static inline std::array<bool, kShapeSampleCount> seen_{};
  static inline std::uint32_t received_{0U};
  static inline bool valid_{true};
};

class ShapeSource final : public ::holoscan::Operator<> {
 public:
  void setup(::holoscan::OperatorSpec& spec) override {
    spec.output(output, "output").max_emits_per_compute(1U);
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

    static constexpr std::array<const char*, 3U> colors{"BLUE", "ORANGE", "PURPLE"};
    ShapesPayload shape;
    shape.color = colors[emitted_ % colors.size()];
    shape.x = 20 + static_cast<std::int32_t>(emitted_ * 5U);
    shape.y = 30 + static_cast<std::int32_t>(emitted_ * 3U);
    shape.shape_size = 24 + static_cast<std::int32_t>(emitted_ % 8U);
    shape.fill_kind = ::rti::holoscan::shapes::ShapeFillKind_Solid;
    shape.angle = static_cast<float>(emitted_) * 2.5F;
    ++emitted_;
    return output.emit(std::move(shape));
  }

  ::holoscan::Output<ShapesPayload> output;

 private:
  std::uint32_t emitted_{0U};
};

class ShapeSink final : public ::holoscan::Operator<> {
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
    ShapeObservation::record(*shape);
    return {};
  }

  ::holoscan::Input<ShapesPayload> input;
};

inline int finish_shapes_run(::holoscan::RunSession& session,
                             bool complete,
                             const char* success_message) {
  session.request_stop();
  session.wait();
  if (!complete || session.termination() != ::holoscan::RunTermination::kStopped ||
      !session.diagnostics().empty()) {
    std::cerr << "Shapes example did not complete cleanly: complete=" << complete
              << " termination=" << static_cast<int>(session.termination())
              << " diagnostics=" << session.diagnostics().size() << '\n';
    return 1;
  }
  std::cout << success_message << '\n';
  return 0;
}

}  // namespace rti::holoscan::example
