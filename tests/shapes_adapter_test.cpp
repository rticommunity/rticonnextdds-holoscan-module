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

#include <iostream>

#include "shape_adapter.hpp"

int main() {
  rti::holoscan::shapes::ShapeT input;
  input.color = "ORANGE";
  input.x = 41;
  input.y = 73;
  input.shape_size = 29;
  input.fill_kind = rti::holoscan::shapes::ShapeFillKind_HorizontalHatch;
  input.angle = 17.5F;

  const ShapeTypeExtended dds_sample = rti::holoscan::example::ShapeAdapter::to_dds(input);
  const auto output = rti::holoscan::example::ShapeAdapter::from_dds(dds_sample);

  if (output.color != input.color || output.x != input.x || output.y != input.y ||
      output.shape_size != input.shape_size || output.fill_kind != input.fill_kind ||
      output.angle != input.angle) {
    std::cerr << "Shapes adapter changed one or more fields\n";
    return 1;
  }

  std::cout << "Shapes adapter preserved all ShapeTypeExtended fields\n";
  return 0;
}
