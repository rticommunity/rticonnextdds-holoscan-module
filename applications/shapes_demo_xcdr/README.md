# Connext Shapes over an XCDR Tensor

This example is designed for an existing Connext developer who wants to keep
working with generated DDS samples while Holoscan transports the serialized
sample as opaque data.

The application accesses normal generated fields:

```cpp
ShapeTypeExtended shape;
shape.color = "ORANGE";
shape.x = 120;
shape.y = 80;

holoscan::Tensor encoded = ShapeXcdrAdapter::encode(shape);
ShapeTypeExtended decoded = ShapeXcdrAdapter::decode(encoded);
```

The graph payload is a contiguous host `Tensor<uint8_t>` containing one
encapsulated XCDR sample:

```text
Application ShapeTypeExtended -> XCDR Tensor -> ShapeTypeExtended -> DDS
DDS -> ShapeTypeExtended -> XCDR Tensor -> Application ShapeTypeExtended
```

This is the opaque alternative to the
[typed Holoscan Shapes example](../shapes_demo_flatbuffers/README.md).

## When to use this approach

Use XCDR when Holoscan operators only need to route, queue, record, replay, or
otherwise treat DDS samples as opaque data. Such operators can use the same
byte-tensor port without duplicating every IDL field in a FlatBuffers schema.

An operator that needs `shape.x`, a nested member, or any other typed field
must deserialize the tensor first. XCDR is not a GPU-native representation and
a CUDA kernel cannot directly interpret an arbitrary DDS object graph.

## What is and is not type-agnostic

The Holoscan graph boundary is independent of the fields in the IDL:

```cpp
holoscan::Input<holoscan::Tensor> input;
holoscan::Output<holoscan::Tensor> output;
```

The current DDS endpoint is still typed. It creates either:

```cpp
dds::pub::DataWriter<ShapeTypeExtended>
dds::sub::DataReader<ShapeTypeExtended>
```

and `ShapeXcdrAdapter` calls the serializer functions generated for
`ShapeTypeExtended`. This prototype does not provide a type-erased serialized
DDS writer or reader. A new IDL needs a corresponding XCDR adapter, although
intermediate opaque Tensor operators remain unchanged.

## Tensor ownership and memory

The tensor contains bytes, not a numeric pointer to a DDS object.

`ShapeXcdrAdapter::encode()`:

1. Asks the generated Connext plugin for the serialized size.
2. Allocates a `std::vector<uint8_t>`.
3. Serializes one XCDR1 sample with its encapsulation header.
4. Wraps the buffer in a host, rank-one `holoscan::Tensor`.
5. Stores the vector in a `TensorMemoryReference` so copied Tensor handles keep
   the buffer alive.

The current Shapes contract accepts at most 512 bytes. A different IDL must
choose and validate an appropriate bound.

The output declares an inline/copy transfer policy. Inside the current
partition, Tensor handles share the backing buffer. Across an interface,
Holoscan may materialize the bounded host bytes. This is safe but not
zero-copy.

## DDS configuration

Both applications use:

| Setting | Value |
|---|---|
| Domain ID | `0` |
| Topic | `Square` |
| DDS type | `ShapeTypeExtended` |
| QoS file | `HoloscanConnextQos.xml` included with this example |
| QoS profile | `HoloscanConnext::ShapesInterop` |
| Reliability | Best effort |
| History | Keep last, depth 32 |

The DDS wire endpoint remains compatible with RTI Shapes Demo. The extra XCDR
conversion is at the Holoscan graph boundary.

## Files

| File | Purpose |
|---|---|
| `example_graph.hpp` | Local deterministic Shapes source, sink, and validation helpers |
| `HoloscanConnextQos.xml` | Local Shapes-compatible DDS QoS |
| `shape_xcdr.hpp` | XCDR serialization, deserialization, Tensor ownership, and Tensor contracts |
| `xcdr_graph.hpp` | Application source and sink that work with `ShapeTypeExtended` |
| `publisher.cpp` | `connext_shapes_demo_xcdr_publisher` application |
| `subscriber.cpp` | `connext_shapes_demo_xcdr_subscriber` application |
| `tests/shapes_conversion_benchmark.cpp` | Typed versus XCDR CPU conversion benchmark |

This example generates its own `ShapeTypeExtended` support and includes its own
XML QoS file, so it builds independently of the typed Shapes example.

## Publisher application

`connext_shapes_demo_xcdr_publisher` builds this graph:

```text
DdsShapeXcdrSource -> Tensor<uint8_t> -> PublisherOp -> Tensor<uint8_t>
                                                        |
                                                        v
                                                 DdsShapeXcdrSink
```

The application:

1. Constructs 20 ordinary generated `ShapeTypeExtended` objects.
2. Sets fields such as `shape.x`, `shape.color`, and `shape.fillKind`.
3. Serializes each object into an XCDR Tensor.
4. Passes that Tensor to the reusable publisher.
5. Deserializes it inside the endpoint and writes a typed DDS sample.
6. Forwards and decodes the original Tensor locally to validate progress.

This deliberately demonstrates that Connext-oriented application code can
remain centered on generated DDS samples.

## Subscriber application

`connext_shapes_demo_xcdr_subscriber` builds this graph:

```text
DDS Square -> SubscriberOp -> Tensor<uint8_t> -> DdsShapeXcdrSink
```

The application:

1. Receives typed `ShapeTypeExtended` samples from DDS.
2. Serializes each received object into a Tensor before graph emission.
3. Passes the opaque Tensor through the Holoscan graph.
4. Deserializes it at the application sink.
5. Accesses normal generated DDS fields and validates all 20 samples.

## Run the applications manually

## Holoscan CLI workflow

`metadata.json` exposes `publisher` and `subscriber` modes as first-class Holoscan CLI applications. The CLI builds and runs them in Docker:

```bash
export HOLOSCAN_CLI=./.venv-holoscan-cli/bin/holoscan
export HOLOSCAN_SDK_ROOT=/path/to/holoscan-sdk/install-aarch64
$HOLOSCAN_CLI build-container shapes_demo_xcdr
$HOLOSCAN_CLI build shapes_demo_xcdr --no-docker-build --local-sdk-root "$HOLOSCAN_SDK_ROOT"
```

Run `subscriber` first and `publisher` in a second terminal:

```bash
$HOLOSCAN_CLI run shapes_demo_xcdr subscriber --no-docker-build --no-local-build --local-sdk-root "$HOLOSCAN_SDK_ROOT"
$HOLOSCAN_CLI run shapes_demo_xcdr publisher --no-docker-build --no-local-build --local-sdk-root "$HOLOSCAN_SDK_ROOT"
```

The CLI mounts the ignored project-root `rti_license.dat` into the application container.

Complete the [root container quick start](../../README.md#quick-start-shapes-with-holoviz)
and retain its `MODULE_ROOT` and `HOLOSCAN_INSTALL` variables.

Open two Bash terminals. Start the subscriber in terminal 1:

```bash
docker run --rm --runtime=nvidia --network=host \
  --user "$(id -u):$(id -g)" \
  -e RTI_LICENSE_FILE=/workspace/rticonnextdds-holoscan-module/build-ea2/rti_license.dat \
  -v "${HOLOSCAN_INSTALL}:/opt/holoscan:ro" \
  -v "${MODULE_ROOT}:/workspace/rticonnextdds-holoscan-module" \
  -w /workspace/rticonnextdds-holoscan-module/build-ea2/applications/shapes_demo_xcdr \
  rticonnextdds-holoscan-module:ea2 \
  ./connext_shapes_demo_xcdr_subscriber
```

Then start the publisher in terminal 2:

```bash
docker run --rm --runtime=nvidia --network=host \
  --user "$(id -u):$(id -g)" \
  -e RTI_LICENSE_FILE=/workspace/rticonnextdds-holoscan-module/build-ea2/rti_license.dat \
  -v "${HOLOSCAN_INSTALL}:/opt/holoscan:ro" \
  -v "${MODULE_ROOT}:/workspace/rticonnextdds-holoscan-module" \
  -w /workspace/rticonnextdds-holoscan-module/build-ea2/applications/shapes_demo_xcdr \
  rticonnextdds-holoscan-module:ea2 \
  ./connext_shapes_demo_xcdr_publisher
```

Success ends with:

```text
DDS XCDR publisher sent 20 ShapeTypeExtended samples
DDS XCDR subscriber received 20 ShapeTypeExtended samples
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
  ctest --test-dir build-ea2 -R connext_shapes_demo_xcdr_integration --output-on-failure
```

The test fails if either application cannot compile its graph, communicate
through DDS, decode the Tensor, or validate every field in all 20 samples.

## Run the conversion benchmark

Use a separate Release build. The commands still run entirely inside Docker:

```bash
docker run --rm --runtime=nvidia --network=host \
  --user "$(id -u):$(id -g)" \
  -v "${HOLOSCAN_INSTALL}:/opt/holoscan:ro" \
  -v "${MODULE_ROOT}:/workspace/rticonnextdds-holoscan-module" \
  -w /workspace/rticonnextdds-holoscan-module \
  rticonnextdds-holoscan-module:ea2 \
  bash -lc 'cmake -S . -B build-ea2-release -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=/opt/holoscan && \
    cmake --build build-ea2-release \
      --target connext_shapes_conversion_benchmark && \
    ./build-ea2-release/connext_shapes_conversion_benchmark \
      --iterations 100000'
```

The output reports:

- Typed field-adapter round-trip time.
- XCDR encode and Tensor creation time.
- XCDR decode time.
- Complete XCDR Tensor round-trip time.
- Serialized sample size.

This is a CPU conversion microbenchmark. It does not measure DDS discovery,
network latency, Holoscan scheduling, GPU transfer, or a sustained streaming
pipeline. Do not use its ratio as a system-level performance claim.

## Adapt this pattern to another IDL

1. Generate the C++ type and plugin with `rti_holoscan_add_idl()`.
2. Define a bounded host byte-Tensor contract appropriate for the maximum XCDR
   size.
3. Call the generated type's serialize and deserialize functions.
4. Retain the byte buffer through `TensorMemoryReference` or a runtime-managed
   pool.
5. Instantiate the publisher and subscriber with the generated type and XCDR
   adapter.
6. Test nested members, maximum-length strings and sequences, malformed
   buffers, and oversize samples.

See [Using your own IDL](../../docs/using-your-own-idl.md) for the common
generation and endpoint steps.
