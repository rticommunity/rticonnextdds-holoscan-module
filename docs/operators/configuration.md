# DDS endpoint configuration

Both reusable operators receive an `rti::holoscan::dds::EndpointConfig`:

```cpp
struct EndpointConfig {
  std::uint32_t domain_id{0U};
  std::string topic_name;
  std::string qos_file{"HoloscanConnextQos.xml"};
  std::string qos_profile{"HoloscanConnext::ReliableKeepAll"};
  std::uint32_t max_samples_per_activation{32U};
  bool wait_for_reader{true};
  std::chrono::milliseconds reader_match_timeout{10000};
  std::chrono::milliseconds reader_match_poll_interval{100};
  std::chrono::milliseconds acknowledgment_timeout{5000};
  std::chrono::milliseconds notification_retry_interval{1};
};
```

## Fields

| Field | Default | Required action |
|---|---|---|
| `domain_id` | `0` | Change it if the deployment uses another DDS domain |
| `topic_name` | Empty | Always set it to the application topic |
| `qos_file` | `HoloscanConnextQos.xml` | Ensure the file exists in the process working directory or provide a path |
| `qos_profile` | `HoloscanConnext::ReliableKeepAll` | Ensure the XML library and profile exist |
| `max_samples_per_activation` | `32` | Maximum samples a subscriber emits per compute; must be positive |
| `wait_for_reader` | `true` | Make a publisher wait for a matched reader during startup |
| `reader_match_timeout` | `10 s` | Publisher startup match deadline |
| `reader_match_poll_interval` | `100 ms` | Publisher match polling interval |
| `acknowledgment_timeout` | `5 s` | Publisher shutdown acknowledgment deadline |
| `notification_retry_interval` | `1 ms` | Retry interval when the Holoscan notification queue is full |

The subscriber uses `max_samples_per_activation` both for its Holoscan
`max_emits_per_compute` contract and for the Connext `Selector`, so a single
activation never exceeds the declared output bound.

For parallel integration tests, `HOLOSCAN_DDS_DOMAIN_ID` can override the
domain and `HOLOSCAN_DDS_TOPIC_PREFIX` can prefix the configured topic. These
environment overrides are intended for test isolation; production applications
should set the values explicitly in `EndpointConfig`.

## Matching rules

A DDS writer and reader communicate only when all of these are compatible:

1. Same DDS domain ID.
2. Same topic name.
3. Compatible DDS type definitions.
4. Compatible requested/offered QoS.
5. A network path that permits DDS discovery and user data.

The C++ type name in the source is not sufficient by itself. Both endpoints
must generate compatible DDS type metadata from compatible IDL.

## XML lookup

The module constructs a Connext `QosProvider` from `qos_file`, then loads the
participant, publisher, subscriber, topic, writer, and reader QoS objects from
`qos_profile`.

For example:

```cpp
const EndpointConfig config{
    .domain_id = 42,
    .topic_name = "RobotState",
    .qos_file = "RobotQos.xml",
    .qos_profile = "RobotQosLibrary::ReliableState",
};
```

```xml
<dds>
  <qos_library name="RobotQosLibrary">
    <qos_profile name="ReliableState">
      <datawriter_qos>
        <reliability>
          <kind>RELIABLE_RELIABILITY_QOS</kind>
        </reliability>
      </datawriter_qos>
      <datareader_qos>
        <reliability>
          <kind>RELIABLE_RELIABILITY_QOS</kind>
        </reliability>
      </datareader_qos>
    </qos_profile>
  </qos_library>
</dds>
```

Run the executable from a directory containing `RobotQos.xml`, or supply an
absolute container path in `qos_file`.

## Profiles supplied by the examples

| Example | Profile | Behavior |
|---|---|---|
| Two-IDL | `HoloscanConnext::ReliableKeepAll` | Reliable, keep-all, transient-local |
| Typed Shapes | `HoloscanConnext::ShapesInterop` | Best-effort, keep-last depth 32 |

The Shapes profile matches the normal RTI Shapes Demo delivery model. The
two-IDL profile is intentionally strict so the finite test receives all 20
samples of both types. It is not automatically appropriate for an unbounded
production stream.

Production QoS must define resource limits, history, durability, blocking,
failure, and discovery behavior for the actual data rate and deployment.
