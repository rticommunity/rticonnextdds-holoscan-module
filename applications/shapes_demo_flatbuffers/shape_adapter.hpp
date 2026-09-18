/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Real-Time Innovations, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdexcept>

#include "ShapeType.hpp"
#include "shape_schema_traits.hpp"

namespace rti::holoscan::example {

struct ShapeAdapter {
  using holoscan_type = ::rti::holoscan::shapes::ShapeT;

  static ::ShapeTypeExtended to_dds(const holoscan_type& value) {
    return ::ShapeTypeExtended{value.color,
                               value.x,
                               value.y,
                               value.shape_size,
                               to_dds_fill(value.fill_kind),
                               value.angle};
  }

  static holoscan_type from_dds(const ::ShapeTypeExtended& value) {
    holoscan_type result;
    result.color = value.color;
    result.x = value.x;
    result.y = value.y;
    result.shape_size = value.shapesize;
    result.fill_kind = from_dds_fill(value.fillKind);
    result.angle = value.angle;
    return result;
  }

 private:
  static ::ShapeFillKind to_dds_fill(::rti::holoscan::shapes::ShapeFillKind value) {
    switch (value) {
      case ::rti::holoscan::shapes::ShapeFillKind_Solid:
        return ::ShapeFillKind::SOLID_FILL;
      case ::rti::holoscan::shapes::ShapeFillKind_Transparent:
        return ::ShapeFillKind::TRANSPARENT_FILL;
      case ::rti::holoscan::shapes::ShapeFillKind_HorizontalHatch:
        return ::ShapeFillKind::HORIZONTAL_HATCH_FILL;
      case ::rti::holoscan::shapes::ShapeFillKind_VerticalHatch:
        return ::ShapeFillKind::VERTICAL_HATCH_FILL;
    }
    throw std::invalid_argument{"Unknown Holoscan shape fill kind"};
  }

  static ::rti::holoscan::shapes::ShapeFillKind from_dds_fill(::ShapeFillKind value) {
    switch (value) {
      case ::ShapeFillKind::SOLID_FILL:
        return ::rti::holoscan::shapes::ShapeFillKind_Solid;
      case ::ShapeFillKind::TRANSPARENT_FILL:
        return ::rti::holoscan::shapes::ShapeFillKind_Transparent;
      case ::ShapeFillKind::HORIZONTAL_HATCH_FILL:
        return ::rti::holoscan::shapes::ShapeFillKind_HorizontalHatch;
      case ::ShapeFillKind::VERTICAL_HATCH_FILL:
        return ::rti::holoscan::shapes::ShapeFillKind_VerticalHatch;
    }
    throw std::invalid_argument{"Unknown DDS shape fill kind"};
  }
};

}  // namespace rti::holoscan::example
