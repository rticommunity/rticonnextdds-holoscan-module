# Using your own IDL

This guide adds one generated Connext type to a Holoscan application. Complete
the [container quick start](../README.md#container-quick-start) before using
it.

The current repository is an in-tree prototype. The CMake helper is consumed
from this source tree; it is not yet available through an installed package.

## 1. Add the IDL to the application

For example, create `idl/RobotState.idl`:

```idl
module robot {
    struct RobotState {
        @key unsigned long robot_id;
        unsigned long sequence;
        double x;
        double y;
        string<64> mode;
    };
};
```

The application owns this file. Do not place generated `.hpp` or `.cxx`
files in source control.

## 2. Generate the C++ type from CMake

Add the IDL target to `CMakeLists.txt`:

```cmake
rti_holoscan_add_idl(
    TARGET robot_state_types
    IDL path/to/idl/RobotState.idl
)
```

The helper invokes `rtiddsgen -language C++11`, creates generated sources
under the build directory, and creates a static CMake target named
`robot_state_types`. Link that target to every executable that includes the
generated type:

```cmake
target_link_libraries(robot_application
    PRIVATE
        RTIConnextDDSHoloscan::dds
        robot_state_types
)
```

Then include the generated header normally:

```cpp
#include "RobotState.hpp"
```

If the IDL includes other IDL files, list their directories:

```cmake
rti_holoscan_add_idl(
    TARGET robot_state_types
    IDL path/to/idl/RobotState.idl
    INCLUDE_DIRS
        path/to/common_idl
        path/to/vendor_idl
)
```

CMake tracks the IDL inputs and regenerates the C++ sources after a change.

## 3. Choose the Holoscan payload boundary

The generated `robot::RobotState` is the DDS type. You must still decide what
the Holoscan graph carries.

### Option A: an existing Holoscan payload

Use this when only selected DDS fields belong in the graph. The two-IDL
example maps a scalar graph value to a larger DDS sample:

```cpp
struct RobotStateAdapter {
  using holoscan_type = std::uint32_t;

  static robot::RobotState to_dds(holoscan_type sequence) {
    robot::RobotState sample;
    sample.robot_id = 1;
    sample.sequence = sequence;
    sample.x = 0.0;
    sample.y = 0.0;
    sample.mode = "RUNNING";
    return sample;
  }

  static holoscan_type from_dds(const robot::RobotState& sample) {
    return sample.sequence;
  }
};
```

This is simple but intentionally exposes only `sequence` to the graph.

### Option B: a typed FlatBuffers Holoscan payload

Use this when Holoscan operators must access `robot_id`, `x`, `y`, `mode`, or
other fields directly. Define a companion `.fbs` schema, generate its EA2
schema traits with `holoscan_add_flatbuffer_schema()`, and map every field in
the adapter.

The complete reference is the
[typed Shapes example](../applications/shapes_demo_flatbuffers/README.md).

This provides the best typed Holoscan experience but requires maintaining or
generating the companion schema and mapping.

### Option C: an XCDR byte Tensor

Use this when existing Connext-oriented application code should construct and
read `robot::RobotState`, while intermediate Holoscan operators only route or
record opaque data.

The graph carries a bounded host `Tensor<uint8_t>`. The application serializes
the generated sample at the graph boundary and deserializes it before reading
fields. No companion FlatBuffers field model is required.

The complete reference is the
[XCDR Shapes example](../applications/shapes_demo_xcdr/README.md).

The current `ShapeXcdrAdapter` directly names the generated Shapes serializer
functions. For a new type, provide the equivalent adapter for the functions
generated in `RobotStatePlugin.hpp`. A future module API should remove most of
this boilerplate.

## 4. Instantiate publisher and subscriber types

```cpp
using RobotPublisher = rti::holoscan::dds::PublisherOp<
    robot::RobotState,
    RobotStateAdapter>;

using RobotSubscriber = rti::holoscan::dds::SubscriberOp<
    robot::RobotState,
    RobotStateAdapter>;
```

The same adapter normally provides both `to_dds()` and `from_dds()`. An
application that only publishes or only subscribes needs only the direction
it instantiates.

## 5. Configure matching endpoints

Publisher:

```cpp
const auto publisher = graph.op<RobotPublisher>(
    "robot-publisher",
    rti::holoscan::dds::EndpointConfig{
        .domain_id = 0,
        .topic_name = "RobotState",
        .qos_file = "RobotQos.xml",
        .qos_profile = "RobotQosLibrary::RobotStateProfile",
    });
```

Subscriber:

```cpp
const auto subscriber = graph.op<RobotSubscriber>(
    "robot-subscriber",
    rti::holoscan::dds::EndpointConfig{
        .domain_id = 0,
        .topic_name = "RobotState",
        .qos_file = "RobotQos.xml",
        .qos_profile = "RobotQosLibrary::RobotStateProfile",
    });
```

Both processes must have access to `RobotQos.xml`. See
[endpoint configuration](operators/configuration.md).

## 6. Connect the graph ports

Publisher graph:

```cpp
graph.add_flow(robot_source->output, publisher->input);
```

Subscriber graph:

```cpp
graph.add_flow(subscriber->output, robot_sink->input);
```

The connected source and sink ports must use `RobotStateAdapter::holoscan_type`.

## 7. Build and validate

Re-run the container build command from the root README. Then start the
subscriber application before the publisher. Confirm:

- Both processes use the same domain, topic, type, and QoS profile.
- The subscriber receives the expected number of samples.
- Every important field is checked, not only the sample count.
- Both applications stop without Holoscan diagnostics or Connext errors.

For complex IDL, add tests covering nested members, variable-length members,
enums, optional members, and maximum expected serialized sizes.
