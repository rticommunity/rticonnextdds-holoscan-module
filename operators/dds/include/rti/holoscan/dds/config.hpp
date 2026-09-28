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
#include <vector>

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
  bool ignore_process_local_publications{false};
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

struct LocalPublication {
  std::uint32_t domain_id;
  std::string topic_name;
  ::dds::core::InstanceHandle handle;
};

inline std::mutex local_publications_mutex;
inline std::vector<LocalPublication> local_publications;

inline void register_process_local_publication(const EndpointConfig& config, const ::dds::core::InstanceHandle& handle) {
  if (!config.ignore_process_local_publications) return;
  const std::lock_guard lock{local_publications_mutex};
  local_publications.push_back({config.domain_id, config.topic_name, handle});
}

inline void unregister_process_local_publication(const EndpointConfig& config, const ::dds::core::InstanceHandle& handle) {
  if (!config.ignore_process_local_publications) return;
  const std::lock_guard lock{local_publications_mutex};
  std::erase_if(local_publications, [&](const LocalPublication& publication) {
    return publication.domain_id == config.domain_id && publication.topic_name == config.topic_name && publication.handle == handle;
  });
}

inline void ignore_process_local_publications(const EndpointConfig& config, ::dds::domain::DomainParticipant& participant) {
  if (!config.ignore_process_local_publications) return;
  std::vector<::dds::core::InstanceHandle> handles;
  {
    const std::lock_guard lock{local_publications_mutex};
    for (const auto& publication : local_publications) {
      if (publication.domain_id == config.domain_id && publication.topic_name == config.topic_name) handles.push_back(publication.handle);
    }
  }
  for (const auto& handle : handles) { try { ::dds::pub::ignore(participant, handle); } catch (...) {} }
}

inline bool is_process_local_publication(const EndpointConfig& config,
                                         const ::dds::core::InstanceHandle& handle) {
  if (!config.ignore_process_local_publications) return false;
  const std::lock_guard lock{local_publications_mutex};
  for (const auto& publication : local_publications) {
    if (publication.domain_id == config.domain_id && publication.topic_name == config.topic_name &&
        publication.handle == handle) {
      return true;
    }
  }
  return false;
}
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
