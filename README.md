# RTI Connext DDS Holoscan Module

This repository is the development home for an RTI-owned Holoscan 5 module
that integrates RTI Connext DDS with NVIDIA Holoscan applications.

The current EA1 baseline is intentionally small. It verifies that a standalone
C++ project can consume an installed Holoscan 5 SDK and RTI Connext DDS 7.7.0,
create a DDS participant, execute a minimal Holoscan graph, and generate two
application-owned C++ data models from IDL. Generic DDS publisher and subscriber
operator templates are included as an EA1 proof of concept; their adapter
boundary remains provisional until it is evaluated against EA2.

All development, builds, and tests run in containers. Nothing from the private
Holoscan Engineering Release is copied into this repository or its container
image.

See [PLAN.md](PLAN.md) for the proposed implementation phases and scope.
The provisional EA1 payload decision is recorded in
[ADR 0001](docs/adr/0001-ea1-payload-boundary.md).

## EA1 development build

Build NVIDIA's Holoscan 5 EA1 SDK image and installation first. Then build this
development image from the repository root:

```bash
docker build -t rticonnextdds-holoscan-module:ea1 .
```

The default image targets the Connext ARM64 architecture installed by the
7.7.0 Debian packages (`armv8Linux4gcc8.5.0`). A future x86 build must override
both `BASE_IMAGE` and `RTI_CONNEXT_ARCH` with the matching values.

Configure and compile the baseline using only container-provided tools and a
read-only mount of the installed SDK:

```bash
docker run --rm --runtime=nvidia \
  --network=host \
  -v /path/to/holoscan-sdk/install-aarch64:/opt/holoscan:ro \
  -v "${PWD}":/workspace/rticonnextdds-holoscan-module \
  rticonnextdds-holoscan-module:ea1 \
  bash -lc 'cmake -S . -B build -G Ninja \
    -DCMAKE_PREFIX_PATH=/opt/holoscan && cmake --build build'
```

## Application-owned IDL

The example declares two independent data models in
`examples/two_idl/idl`. They are generated during the normal CMake build; no
manual `rtiddsgen` command or checked-in generated source is required:

```cmake
rti_holoscan_add_idl(
    TARGET my_application_types
    IDL path/to/MyApplicationType.idl
)

target_link_libraries(my_application PRIVATE my_application_types)
```

At this stage the helper is an in-tree prototype. It proves the intended user
workflow, but it is not yet an installed public API.

## Two-IDL example

The example contains two independent Holoscan applications:

- `connext_two_idl_publisher` maps Holoscan `uint32_t` samples to the generated
  `Telemetry` and `Command` DDS types and publishes 20 samples of each.
- `connext_two_idl_subscriber` receives both generated DDS types, validates all
  fields, and emits the sample indices into its Holoscan graph.

The applications use the same reusable templates:

```cpp
using TelemetryPublisher =
    rti::holoscan::dds::PublisherOp<Telemetry, TelemetryAdapter>;
using TelemetrySubscriber =
    rti::holoscan::dds::SubscriberOp<Telemetry, TelemetryAdapter>;
```

The adapter is deliberate in EA1. Connext-generated owning C++ types are used
inside the DDS operators, while the graph ports use payloads admitted by
Holoscan 5 EA1. EA2's custom FlatBuffers payload and codec support must be
evaluated before this becomes the final public data-boundary API.

Run the subscriber first from a second terminal, mounting the same source tree,
SDK installation, and license in both containers. From the repository root:

```bash
docker run --rm --runtime=nvidia --network=host \
  -v /path/to/holoscan-sdk/install-aarch64:/opt/holoscan:ro \
  -v "${PWD}":/workspace/rticonnextdds-holoscan-module \
  -v /path/to/rti_license.dat:/opt/rti.com/rti_connext_dds-7.7.0/rti_license.dat:ro \
  -w /workspace/rticonnextdds-holoscan-module/build/examples/two_idl \
  rticonnextdds-holoscan-module:ea1 \
  ../../connext_two_idl_subscriber
```

Then run the publisher:

```bash
docker run --rm --runtime=nvidia --network=host \
  -v /path/to/holoscan-sdk/install-aarch64:/opt/holoscan:ro \
  -v "${PWD}":/workspace/rticonnextdds-holoscan-module \
  -v /path/to/rti_license.dat:/opt/rti.com/rti_connext_dds-7.7.0/rti_license.dat:ro \
  -w /workspace/rticonnextdds-holoscan-module/build/examples/two_idl \
  rticonnextdds-holoscan-module:ea1 \
  ../../connext_two_idl_publisher
```

The automated integration test starts those two executables as separate
processes and validates all 40 samples:

```bash
docker run --rm --runtime=nvidia --network=host \
  -v /path/to/holoscan-sdk/install-aarch64:/opt/holoscan:ro \
  -v "${PWD}":/workspace/rticonnextdds-holoscan-module \
  -v /path/to/rti_license.dat:/opt/rti.com/rti_connext_dds-7.7.0/rti_license.dat:ro \
  rticonnextdds-holoscan-module:ea1 \
  ctest --test-dir build -R connext_two_idl_integration --output-on-failure
```

The runtime smoke test requires a valid `rti_license.dat`. Mount it read-only;
never copy it into the image or commit it:

```bash
docker run --rm --runtime=nvidia \
  --network=host \
  -v /path/to/holoscan-sdk/install-aarch64:/opt/holoscan:ro \
  -v "${PWD}":/workspace/rticonnextdds-holoscan-module \
  -v /path/to/rti_license.dat:/opt/rti.com/rti_connext_dds-7.7.0/rti_license.dat:ro \
  rticonnextdds-holoscan-module:ea1 \
bash -lc 'ctest --test-dir build --output-on-failure'
```
