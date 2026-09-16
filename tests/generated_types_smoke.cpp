/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Real-Time Innovations, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstdint>
#include <iostream>
#include <string>

#include "Command.hpp"
#include "Telemetry.hpp"

int main() {
  rti::holoscan::example::Telemetry telemetry;
  telemetry.sensor_id = 7U;
  telemetry.sample_index = 19U;
  telemetry.value = 42.5;

  rti::holoscan::example::Command command;
  command.target_id = 7U;
  command.sample_index = 20U;
  command.instruction = "capture";

  if (telemetry.sensor_id != command.target_id || telemetry.sample_index != 19U ||
      telemetry.value != 42.5 || command.sample_index != 20U ||
      command.instruction != std::string{"capture"}) {
    std::cerr << "Generated type contents did not round-trip through their accessors\n";
    return 1;
  }

  std::cout << "Two application-owned IDL types generated and usable\n";
  return 0;
}
