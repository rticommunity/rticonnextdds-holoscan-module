/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Real-Time Innovations, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>

#include "shape_adapter.hpp"
#include "shape_xcdr.hpp"

namespace {

using Clock = std::chrono::steady_clock;

template <typename T>
void materialize(const T& value) {
#if defined(__GNUC__) || defined(__clang__)
  asm volatile("" : : "g"(&value) : "memory");
#else
  (void)value;
#endif
}

template <typename Function>
double measure_ns_per_sample(std::uint32_t iterations, Function&& function) {
  const auto start = Clock::now();
  for (std::uint32_t index = 0U; index < iterations; ++index) {
    function(index);
  }
  const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start);
  return static_cast<double>(elapsed.count()) / static_cast<double>(iterations);
}

}  // namespace

int main(int argc, char** argv) {
  std::uint32_t iterations = 100000U;
  if (argc == 3 && std::string_view{argv[1]} == "--iterations") {
    const auto parsed = std::strtoul(argv[2], nullptr, 10);
    if (parsed == 0UL || parsed > 100000000UL) {
      throw std::invalid_argument{"iterations must be between 1 and 100000000"};
    }
    iterations = static_cast<std::uint32_t>(parsed);
  } else if (argc != 1) {
    throw std::invalid_argument{"usage: connext_shapes_conversion_benchmark [--iterations N]"};
  }

  rti::holoscan::shapes::ShapeT shape;
  shape.color = "ORANGE";
  shape.x = 41;
  shape.y = 73;
  shape.shape_size = 29;
  shape.fill_kind = rti::holoscan::shapes::ShapeFillKind_HorizontalHatch;
  shape.angle = 17.5F;

  std::uint64_t typed_checksum = 0U;
  const double typed_ns = measure_ns_per_sample(iterations, [&](std::uint32_t index) {
    shape.x = static_cast<std::int32_t>(index);
    const auto dds_sample = rti::holoscan::example::ShapeAdapter::to_dds(shape);
    materialize(dds_sample);
    const auto output = rti::holoscan::example::ShapeAdapter::from_dds(dds_sample);
    materialize(output);
    typed_checksum += static_cast<std::uint64_t>(output.x) + output.color.size();
  });

  std::uint64_t xcdr_checksum = 0U;
  std::int64_t serialized_bytes = 0;
  const auto fixed_dds_sample = rti::holoscan::example::ShapeAdapter::to_dds(shape);
  const auto fixed_tensor = rti::holoscan::example::ShapeXcdrAdapter::encode(fixed_dds_sample);
  std::uint64_t encode_checksum = 0U;
  const double encode_ns = measure_ns_per_sample(iterations, [&](std::uint32_t index) {
    auto dds_input = fixed_dds_sample;
    dds_input.x = static_cast<std::int32_t>(index);
    const auto tensor = rti::holoscan::example::ShapeXcdrAdapter::encode(dds_input);
    materialize(tensor);
    encode_checksum += static_cast<std::uint64_t>(tensor.nbytes());
  });

  std::uint64_t decode_checksum = 0U;
  const double decode_ns = measure_ns_per_sample(iterations, [&](std::uint32_t) {
    const auto dds_output = rti::holoscan::example::ShapeXcdrAdapter::decode(fixed_tensor);
    materialize(dds_output);
    decode_checksum += static_cast<std::uint64_t>(dds_output.x) + dds_output.color.size();
  });

  const double xcdr_ns = measure_ns_per_sample(iterations, [&](std::uint32_t index) {
    shape.x = static_cast<std::int32_t>(index);
    const auto dds_input = rti::holoscan::example::ShapeAdapter::to_dds(shape);
    materialize(dds_input);
    const auto tensor = rti::holoscan::example::ShapeXcdrAdapter::encode(dds_input);
    materialize(tensor);
    const auto dds_output = rti::holoscan::example::ShapeXcdrAdapter::decode(tensor);
    materialize(dds_output);
    const auto output = rti::holoscan::example::ShapeAdapter::from_dds(dds_output);
    materialize(output);
    xcdr_checksum += static_cast<std::uint64_t>(output.x) + output.color.size();
    serialized_bytes = tensor.nbytes();
  });

  if (typed_checksum != xcdr_checksum || encode_checksum == 0U || decode_checksum == 0U ||
      serialized_bytes <= 0) {
    std::cerr << "Conversion benchmark validation failed\n";
    return 1;
  }

  std::cout << "Shapes conversion benchmark\n"
            << "iterations=" << iterations << '\n'
            << "typed_ns_per_sample=" << typed_ns << '\n'
            << "xcdr_encode_tensor_ns_per_sample=" << encode_ns << '\n'
            << "xcdr_decode_tensor_ns_per_sample=" << decode_ns << '\n'
            << "xcdr_tensor_ns_per_sample=" << xcdr_ns << '\n'
            << "xcdr_overhead_ratio=" << xcdr_ns / typed_ns << '\n'
            << "xcdr_serialized_bytes=" << serialized_bytes << '\n';
  return 0;
}
