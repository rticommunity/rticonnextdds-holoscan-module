# DDS endpoint configuration

Both reusable operators receive an `rti::holoscan::dds::EndpointConfig`:

```cpp
struct EndpointConfig {
  std::uint32_t domain_id{0U};
  std::string topic_name;
  std::string qos_file{"HoloscanConnextQos.xml"};
  std::string qos_profile{"HoloscanConnext::ReliableKeepAll"};
};
```

## Fields

| Field | Default | Required action |
|---|---|---|
| `domain_id` | `0` | Change it if the deployment uses another DDS domain |
| `topic_name` | Empty | Always set it to the application topic |
| `qos_file` | `HoloscanConnextQos.xml` | Ensure the file exists in the process working directory or provide a path |
| `qos_profile` | `HoloscanConnext::ReliableKeepAll` | Ensure the XML library and profile exist |

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
| XCDR Shapes | `HoloscanConnext::ShapesInterop` | Best-effort, keep-last depth 32 |

The Shapes profile matches the normal RTI Shapes Demo delivery model. The
two-IDL profile is intentionally strict so the finite test receives all 20
samples of both types. It is not automatically appropriate for an unbounded
production stream.

Production QoS must define resource limits, history, durability, blocking,
failure, and discovery behavior for the actual data rate and deployment.
