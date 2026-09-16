/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Real-Time Innovations, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <string>

#include <dds/dds.hpp>

namespace rti::holoscan::dds {

struct EndpointConfig {
  std::uint32_t domain_id{0U};
  std::string topic_name;
  std::string qos_file{"HoloscanConnextQos.xml"};
  std::string qos_profile{"HoloscanConnext::ReliableKeepAll"};
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

}  // namespace rti::holoscan::dds
