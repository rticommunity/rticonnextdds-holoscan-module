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
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <mutex>

#include <holoscan/core/run.hpp>

#include "shape_schema_traits.hpp"

namespace rti::holoscan::example::testing {

inline constexpr std::uint32_t kShapeSampleCount = 20U;

// Checks the fixed sample sequence used by this example, not arbitrary Shapes Demo input.
class ShapeSampleVerifier {
 public:
  using ShapePayload = ::rti::holoscan::shapes::ShapeT;

  static void reset() noexcept {
    const std::lock_guard lock{mutex_};
    received_ = 0U;
    valid_ = true;
    seen_.fill(false);
  }

  static void record(const ShapePayload& shape) {
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

inline int finish_verified_shapes_run(::holoscan::RunSession& session,
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

}  // namespace rti::holoscan::example::testing