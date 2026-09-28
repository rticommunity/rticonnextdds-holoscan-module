/* *******************************************************************************
 * (c) 2026 Copyright, Real-Time Innovations, Inc. All rights reserved.
 * RTI grants Licensee a license to use, modify, compile, and create derivative
 * works of the Software. The Software is provided "as is", with no warranty.
 *******************************************************************************/

#pragma once

#include <cstdint>

namespace rti::holoscan::example::shapes_holoviz {

inline constexpr std::int32_t kLogicalCanvas = 256;
inline constexpr std::int32_t kRenderScale = 3;
inline constexpr std::int32_t kCanvas = kLogicalCanvas * kRenderScale;
inline constexpr std::int32_t kShapeSize = 30;

}  // namespace rti::holoscan::example::shapes_holoviz
