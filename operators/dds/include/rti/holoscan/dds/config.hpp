/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Real-Time Innovations, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <dds/dds.hpp>

namespace rti::holoscan::dds {

struct EndpointConfig {
  static constexpr std::uint32_t kDefaultMaxSamplesPerActivation = 32U;
  std::uint32_t domain_id{0U};
  std::string topic_name;
  std::string qos_file{"HoloscanConnextQos.xml"};
  std::string qos_profile{"HoloscanConnext::ReliableKeepAll"};
  std::uint32_t max_samples_per_activation{kDefaultMaxSamplesPerActivation};
  bool wait_for_reader{true};
  std::chrono::milliseconds reader_match_timeout{10000};
  std::chrono::milliseconds reader_match_poll_interval{100};
  std::chrono::milliseconds acknowledgment_timeout{5000};
  std::chrono::milliseconds notification_retry_interval{1};
};

namespace detail {

struct EndpointQos {
  ::dds::domain::qos::DomainParticipantQos participant;
  ::dds::pub::qos::PublisherQos publisher;
  ::dds::sub::qos::SubscriberQos subscriber;
  ::dds::topic::qos::TopicQos topic;
  ::dds::pub::qos::DataWriterQos writer;
  ::dds::sub::qos::DataReaderQos reader;
};

inline EndpointConfig environment_overrides(EndpointConfig config) {
  if (const char* value = std::getenv("HOLOSCAN_DDS_DOMAIN_ID"); value && *value) {
    const std::string_view text{value};
    std::uint32_t domain = 0U;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), domain);
    if (error != std::errc{} || end != text.data() + text.size()) {
      throw std::invalid_argument{"HOLOSCAN_DDS_DOMAIN_ID must be an unsigned integer"};
    }
    config.domain_id = domain;
  }
  if (const char* prefix = std::getenv("HOLOSCAN_DDS_TOPIC_PREFIX")) {
    config.topic_name = std::string{prefix} + config.topic_name;
  }
  return config;
}

inline EndpointQos endpoint_qos(const std::string& qos_file, const std::string& qos_profile) {
  static std::mutex mutex;
  static std::map<std::string, ::dds::core::QosProvider> providers;

  const std::lock_guard lock{mutex};
  auto provider = providers.find(qos_file);
  if (provider == providers.end()) {
    provider = providers.try_emplace(qos_file, qos_file).first;
  }
  auto& value = provider->second;
  return EndpointQos{
      .participant = value.participant_qos(qos_profile),
      .publisher = value.publisher_qos(qos_profile),
      .subscriber = value.subscriber_qos(qos_profile),
      .topic = value.topic_qos(qos_profile),
      .writer = value.datawriter_qos(qos_profile),
      .reader = value.datareader_qos(qos_profile),
  };
}

}  // namespace detail

inline EndpointConfig with_environment_overrides(EndpointConfig config) {
  return detail::environment_overrides(std::move(config));
}

}  // namespace rti::holoscan::dds
