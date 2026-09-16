/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Real-Time Innovations, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include "Command.hpp"
#include "Telemetry.hpp"

namespace rti::holoscan::example {

struct TelemetryAdapter {
  using holoscan_type = std::uint32_t;

  static Telemetry to_dds(holoscan_type sample_index) {
    Telemetry sample;
    sample.sensor_id = 7U;
    sample.sample_index = sample_index;
    sample.value = 1000.0 + static_cast<double>(sample_index) * 0.5;
    return sample;
  }

  static holoscan_type from_dds(const Telemetry& sample) {
    const double expected_value = 1000.0 + static_cast<double>(sample.sample_index) * 0.5;
    if (sample.sensor_id != 7U || sample.value != expected_value) {
      throw std::runtime_error{"Invalid Telemetry sample contents"};
    }
    return sample.sample_index;
  }
};

struct CommandAdapter {
  using holoscan_type = std::uint32_t;

  static Command to_dds(holoscan_type sample_index) {
    Command sample;
    sample.target_id = 11U;
    sample.sample_index = sample_index;
    sample.instruction = "command-" + std::to_string(sample_index);
    return sample;
  }

  static holoscan_type from_dds(const Command& sample) {
    if (sample.target_id != 11U ||
        sample.instruction != "command-" + std::to_string(sample.sample_index)) {
      throw std::runtime_error{"Invalid Command sample contents"};
    }
    return sample.sample_index;
  }
};

}  // namespace rti::holoscan::example
