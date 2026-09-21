# Typed Holoscan Shapes example

This example is designed for a Holoscan developer who wants structured fields
on graph ports while communicating with an existing DDS data model.

It uses:

- `ShapeTypeExtended`, generated from the standard `ShapeType.idl` installed
  with Connext 7.7.0, on the DDS side.
- `rti::holoscan::shapes::ShapeT`, generated from `shape.fbs`, on the Holoscan
  side.
- `ShapeAdapter` to map every field between them.

```text
Holoscan ShapeT <-> ShapeAdapter <-> DDS ShapeTypeExtended
```

This is the typed alternative to the
[XCDR Tensor example](../shapes_demo_xcdr/README.md).

## When to use this approach

Use a companion Holoscan payload when graph operators need to read or modify
individual fields:

```cpp
holoscan::Input<rti::holoscan::shapes::ShapeT> input;

auto shape = input.receive_data();
std::cout << shape->x << ' ' << shape->color << '\n';
```

The graph compiler knows the schema. This makes the payload easy to compose
with other typed Holoscan operators and allows a future schema to include
tensor fields with explicit memory contracts.

The cost is maintaining or generating a companion schema and adapter for each
DDS model. The current conversion copies values; it is not zero-copy.

## Data model

The DDS type includes:

| Field | DDS representation | Holoscan representation |
|---|---|---|
| `color` | Bounded string | FlatBuffers string |
| `x` | 32-bit integer | 32-bit integer |
| `y` | 32-bit integer | 32-bit integer |
| `shapesize` | 32-bit integer | `shape_size` 32-bit integer |
| `fillKind` | `ShapeFillKind` enum | `ShapeFillKind` enum |
| `angle` | 32-bit float | 32-bit float |

`ShapeAdapter` explicitly maps the enum values and the differently named size
field. Its unit test verifies a complete round trip.

## DDS configuration

Both applications use:

| Setting | Value |
|---|---|
| Domain ID | `0` |
| Topic | `Square` |
| DDS type | `ShapeTypeExtended` |
| QoS file | `HoloscanConnextQos.xml` |
| QoS profile | `HoloscanConnext::ShapesInterop` |
| Reliability | Best effort |
| History | Keep last, depth 32 |

These type, topic, and delivery choices are compatible with the normal RTI
Shapes Demo model.

## Files

| File | Purpose |
|---|---|
| `shape.fbs` | Holoscan EA2 FlatBuffers payload schema |
| `shape_adapter.hpp` | Complete field mapping between `ShapeT` and `ShapeTypeExtended` |
| `example_graph.hpp` | Typed source, sink, deterministic data, and validation |
| `publisher.cpp` | `connext_shapes_demo_flatbuffers_publisher` application |
| `subscriber.cpp` | `connext_shapes_demo_flatbuffers_subscriber` application |
| `HoloscanConnextQos.xml` | Shapes-compatible DDS QoS |

The root CMake build generates the DDS type from Connext's installed
`ShapeType.idl` and the Holoscan payload from `shape.fbs`.

## Publisher application

`connext_shapes_demo_flatbuffers_publisher` builds this graph:

```text
ShapeSource<ShapeT> -> PublisherOp<ShapeTypeExtended, ShapeAdapter>
                                       |
                                       v
                                ShapeSink<ShapeT>
```

The application:

1. Generates 20 deterministic typed Holoscan shapes.
2. Maps each `ShapeT` to `ShapeTypeExtended`.
3. Publishes each sample on DDS topic `Square`.
4. Emits the original `ShapeT` on the `published` output.
5. Validates local progress and exits.

The local sink confirms that `DataWriter::write()` was called. The separate
subscriber application confirms DDS delivery and field contents.

## Subscriber application

`connext_shapes_demo_flatbuffers_subscriber` builds this graph:

```text
DDS Square -> SubscriberOp<ShapeTypeExtended, ShapeAdapter> -> ShapeSink<ShapeT>
```

The application:

1. Waits for `ShapeTypeExtended` samples with a Connext `WaitSet`.
2. Wakes the Holoscan scheduler through `NotificationSource`.
3. Maps every DDS sample to `ShapeT`.
4. Emits the typed payload into the graph.
5. Validates all fields, detects duplicates, and exits after exactly 20
   expected samples.

## Run the applications manually

## Holoscan CLI workflow

`metadata.json` exposes `publisher` and `subscriber` modes as first-class Holoscan CLI applications. The CLI builds and runs them in Docker:

```bash
export HOLOSCAN_CLI=./.venv-holoscan-cli/bin/holoscan
export HOLOSCAN_SDK_ROOT=/Users/juanca/dgx-holo5/holoscan5-ea-private/ea2/holoscan-sdk/install-aarch64
$HOLOSCAN_CLI build-container shapes_demo_flatbuffers
$HOLOSCAN_CLI build shapes_demo_flatbuffers --no-docker-build --local-sdk-root "$HOLOSCAN_SDK_ROOT"
```

Run `subscriber` first and `publisher` in a second terminal:

```bash
$HOLOSCAN_CLI run shapes_demo_flatbuffers subscriber --no-docker-build --no-local-build --local-sdk-root "$HOLOSCAN_SDK_ROOT"
$HOLOSCAN_CLI run shapes_demo_flatbuffers publisher --no-docker-build --no-local-build --local-sdk-root "$HOLOSCAN_SDK_ROOT"
```

The CLI mounts the ignored project-root `rti_license.dat` into the application container.

Complete the [root container quick start](../../README.md#container-quick-start)
and retain its `MODULE_ROOT` and `HOLOSCAN_INSTALL` variables.

Open two Bash terminals. Start the subscriber in terminal 1:

```bash
docker run --rm --runtime=nvidia --network=host \
  --user "$(id -u):$(id -g)" \
  -e RTI_LICENSE_FILE=/workspace/rticonnextdds-holoscan-module/build-ea2/rti_license.dat \
  -v "${HOLOSCAN_INSTALL}:/opt/holoscan:ro" \
  -v "${MODULE_ROOT}:/workspace/rticonnextdds-holoscan-module" \
  -w /workspace/rticonnextdds-holoscan-module/build-ea2/applications/shapes_demo_flatbuffers \
  rticonnextdds-holoscan-module:ea2 \
  ./connext_shapes_demo_flatbuffers_subscriber
```

Then start the publisher in terminal 2:

```bash
docker run --rm --runtime=nvidia --network=host \
  --user "$(id -u):$(id -g)" \
  -e RTI_LICENSE_FILE=/workspace/rticonnextdds-holoscan-module/build-ea2/rti_license.dat \
  -v "${HOLOSCAN_INSTALL}:/opt/holoscan:ro" \
  -v "${MODULE_ROOT}:/workspace/rticonnextdds-holoscan-module" \
  -w /workspace/rticonnextdds-holoscan-module/build-ea2/applications/shapes_demo_flatbuffers \
  rticonnextdds-holoscan-module:ea2 \
  ./connext_shapes_demo_flatbuffers_publisher
```

The subscriber prints each reconstructed shape. Success ends with:

```text
DDS Shapes publisher sent 20 ShapeTypeExtended samples
DDS Shapes subscriber received 20 ShapeTypeExtended samples
```

## Run the automated integration

```bash
docker run --rm --runtime=nvidia --network=host \
  --user "$(id -u):$(id -g)" \
  -e RTI_LICENSE_FILE=/workspace/rticonnextdds-holoscan-module/build-ea2/rti_license.dat \
  -v "${HOLOSCAN_INSTALL}:/opt/holoscan:ro" \
  -v "${MODULE_ROOT}:/workspace/rticonnextdds-holoscan-module" \
  -w /workspace/rticonnextdds-holoscan-module \
  rticonnextdds-holoscan-module:ea2 \
  ctest --test-dir build-ea2 -R connext_shapes_demo_flatbuffers_integration --output-on-failure
```

The test launches both applications as separate processes and fails on a
missing, duplicate, reordered, or incorrectly reconstructed sample.

## Interoperate with RTI Shapes Demo

The generated DDS type, domain, and `Square` topic are wire-compatible with
RTI Shapes Demo. The easiest visual check is:

1. Start RTI Shapes Demo as a reader on domain 0.
2. Ensure it displays the `Square` topic.
3. Run `connext_shapes_demo_flatbuffers_publisher` using the command above.
4. Observe the published shapes in Shapes Demo.

The supplied Holoscan subscriber is an automated validator: it expects the
specific 20-sample sequence generated by this repository. Arbitrary shapes
drawn in the graphical Shapes Demo will not satisfy that test. For exploratory
input, replace `ShapeObservation` with application behavior that accepts
arbitrary valid samples.

## Adapt this pattern to another IDL

1. Generate the application DDS type with `rti_holoscan_add_idl()`.
2. Define a companion FlatBuffers schema representing the fields required by
   the Holoscan graph.
3. Generate EA2 schema traits with `holoscan_add_flatbuffer_schema()`.
4. Implement and test both adapter directions.
5. Instantiate the publisher and subscriber with the generated DDS type and
   adapter.
6. Configure matching DDS endpoints and validate every mapped field.

See [Using your own IDL](../../docs/using-your-own-idl.md) for the complete
workflow.
