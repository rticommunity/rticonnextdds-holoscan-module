/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Real-Time Innovations, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

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
