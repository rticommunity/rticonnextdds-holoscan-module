# Two-IDL reusable operator example

This is the smallest example showing that the publisher and subscriber
operators are not tied to one DDS type. The same C++ operator templates are
instantiated with two unrelated application-owned IDLs.

Use this example first when learning the module API. Use the Shapes examples
afterward to compare richer Holoscan payload boundaries.

## Data flows

The publisher application contains two independent paths:

```text
SequenceSource<uint32_t> -> TelemetryAdapter -> DataWriter<Telemetry>
SequenceSource<uint32_t> -> CommandAdapter   -> DataWriter<Command>
```

The subscriber application reverses those paths:

```text
DataReader<Telemetry> -> TelemetryAdapter -> SequenceSink<uint32_t>
DataReader<Command>   -> CommandAdapter   -> SequenceSink<uint32_t>
```

The graph payload is a sample index (`uint32_t`). The complete DDS samples
contain additional fields constructed and validated by their adapters.

## DDS endpoints

| DDS topic | DDS type | Key | Other fields |
|---|---|---|---|
| `HoloscanTelemetry` | `Telemetry` | `sensor_id` | `sample_index`, `value` |
| `HoloscanCommand` | `Command` | `target_id` | `sample_index`, `instruction` |

Both use domain 0 and `HoloscanConnext::ReliableKeepAll`. The profile is
reliable, keep-all, and transient-local so the finite test can validate all 20
samples of both types.

## Files

| File | Purpose |
|---|---|
| `idl/Telemetry.idl` | Application-owned telemetry data contract |
| `idl/Command.idl` | Application-owned command data contract |
| `example_adapters.hpp` | Maps the scalar Holoscan payload to and from each DDS type |
| `example_graph.hpp` | Common source, sink, sample counting, and validation |
| `publisher.cpp` | The `connext_two_idl_publisher` application |
| `subscriber.cpp` | The `connext_two_idl_subscriber` application |
| `HoloscanConnextQos.xml` | Matching DDS QoS |

The root `CMakeLists.txt` calls `rti_holoscan_add_idl()` for both IDLs. Their
generated `.hpp`, `.cxx`, and plugin files are created under `build-ea2`.

## Publisher application

`connext_two_idl_publisher`:

1. Creates two `SequenceSource` operators.
2. Generates sequence values 1 through 20 on each path.
3. Converts each value into either `Telemetry` or `Command`.
4. Publishes on the matching DDS topic.
5. Emits the original graph value on each publisher's `published` port.
6. Stops after both local paths have processed 20 samples.

The publisher waits for compatible readers during startup. Start the
subscriber first.

## Subscriber application

`connext_two_idl_subscriber`:

1. Creates one typed DDS subscriber operator per topic.
2. Uses a Connext `WaitSet` to activate on available DDS data.
3. Validates every generated DDS field in the adapters.
4. Emits each `sample_index` into its Holoscan path.
5. Verifies the ordered sequence 1 through 20 for both topics.
6. Stops after receiving all 40 samples.

## Run the applications manually

Complete the [root container quick start](../../README.md#container-quick-start)
and retain its `MODULE_ROOT` and `HOLOSCAN_INSTALL` variables.

Open two Bash terminals. Start the subscriber in terminal 1:

```bash
docker run --rm --runtime=nvidia --network=host \
  --user "$(id -u):$(id -g)" \
  -e RTI_LICENSE_FILE=/workspace/rticonnextdds-holoscan-module/build-ea2/rti_license.dat \
  -v "${HOLOSCAN_INSTALL}:/opt/holoscan:ro" \
  -v "${MODULE_ROOT}:/workspace/rticonnextdds-holoscan-module" \
  -w /workspace/rticonnextdds-holoscan-module/build-ea2/examples/two_idl \
  rticonnextdds-holoscan-module:ea2 \
  ../../connext_two_idl_subscriber
```

Then start the publisher in terminal 2:

```bash
docker run --rm --runtime=nvidia --network=host \
  --user "$(id -u):$(id -g)" \
  -e RTI_LICENSE_FILE=/workspace/rticonnextdds-holoscan-module/build-ea2/rti_license.dat \
  -v "${HOLOSCAN_INSTALL}:/opt/holoscan:ro" \
  -v "${MODULE_ROOT}:/workspace/rticonnextdds-holoscan-module" \
  -w /workspace/rticonnextdds-holoscan-module/build-ea2/examples/two_idl \
  rticonnextdds-holoscan-module:ea2 \
  ../../connext_two_idl_publisher
```

Success messages are:

```text
DDS publisher sent Telemetry=20, Command=20 samples
DDS subscriber received and validated Telemetry=20, Command=20 samples
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
  ctest --test-dir build-ea2 -R connext_two_idl_integration --output-on-failure
```

## What to copy for a new IDL

Use this example as the minimal pattern:

1. Add the IDL and a `rti_holoscan_add_idl()` target.
2. Define an adapter with `holoscan_type`, `to_dds()`, and `from_dds()`.
3. Instantiate `PublisherOp<GeneratedType, Adapter>` and/or
   `SubscriberOp<GeneratedType, Adapter>`.
4. Link the executable with the generated type target.
5. Configure matching endpoints and XML QoS.
6. Add an integration test that checks field contents and counts.

Continue with [Using your own IDL](../../docs/using-your-own-idl.md) for a
complete walkthrough.
