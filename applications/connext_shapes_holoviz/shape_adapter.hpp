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
