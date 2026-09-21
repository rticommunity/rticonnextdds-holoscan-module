# RTI Connext DDS for NVIDIA Holoscan

Bring the power of RTI Connext to NVIDIA Holoscan applications. This module
makes it easy for Holoscan applications to publish and subscribe to strongly
typed data using the DDS publish/subscribe communication model, without
requiring applications to build and maintain custom point-to-point
connectivity.

Use it to:

- Connect independent Holoscan applications and non-Holoscan systems using a
  standard publish/subscribe communication model.
- Reuse existing DDS data models or define application-owned data types using
  IDL.
- Decouple producers and consumers, allowing applications to evolve
  independently.
- Control how each data flow is delivered using configurable DDS QoS policies.
- Scale from local processes to distributed systems while using the same
  data-centric communication model.

## Why RTI Connext?

[RTI Connext](https://www.rti.com/products/third-party-integrations/nvidia) is
a real-time data-streaming platform for intelligent distributed systems. It
enables applications to share the right data at the right time, wherever they
run.

Connext is designed for systems that require reliable, low-latency, and
scalable data exchange. Built on the Object Management Group (OMG) Data
Distribution Service (DDS) standard, Connext provides:

- Automatic discovery of compatible publishers and subscribers.
- Direct, data-centric communication without an application-level message
  broker.
- Fine-grained QoS control for each data flow.
- Strongly typed interfaces defined with OMG IDL 4 and DDS-XTypes, including
  support for mutable and extensible types.
- A modular architecture that keeps applications decoupled and easier to
  evolve.

## Quick start: Shapes with Holoviz

The fastest way to verify the integration is the `connext_shapes_holoviz`
reference application. It generates a moving black square locally, displays it
with Holoviz, and publishes it with the standard RTI Shapes Demo DDS type. The
same application subscribes to external `Square`, `Circle`, and `Triangle`
topics and displays those samples as they arrive.

All commands below build and run inside Docker. No Holoscan or Connext
installation is required on the host.

Before running the example, prepare the EA2 SDK and the host CLI. EA2 does not
use a public GAR image, so build the SDK from the private NVIDIA EA2 source tree
using NVIDIA's container workflow:

```bash
git clone --branch v5.0.0-ea2 --depth 1 \
  https://github.com/nvidia-holoscan/holoscan-sdk.git \
  holoscan-sdk

cd holoscan-sdk
./run build --arch arm64 --cudaarchs <cuda-architecture> --sccache false
```

This produces the `holoscan-sdk-build-aarch64:v5.0.0-ea2` image and an
`install-aarch64` SDK directory. Keep both outside this repository.

Install the EA2 Holoscan CLI in a Python 3.11+ virtual environment:

```bash
cd /path/to/rticonnextdds-holoscan-module
python3.11 -m venv .venv-holoscan-cli
.venv-holoscan-cli/bin/python -m pip install --upgrade pip
.venv-holoscan-cli/bin/python -m pip install --pre \
  --extra-index-url https://pypi.nvidia.com \
  'holoscan-cli==5.0.0a1'
```

Set the repository, SDK, CLI, and license paths for the current shell:

```bash
export MODULE_ROOT=/path/to/rticonnextdds-holoscan-module
export HOLOSCAN_SDK_ROOT=/path/to/holoscan-sdk/install-aarch64
export HOLOSCAN_CLI="${MODULE_ROOT}/.venv-holoscan-cli/bin/holoscan"
cd "${MODULE_ROOT}"
test -s rti_license.dat
```


Build the application image and compile the application in the EA2 container:

```bash
$HOLOSCAN_CLI build-container connext_shapes_holoviz
$HOLOSCAN_CLI build connext_shapes_holoviz \
  --no-docker-build --local-sdk-root "$HOLOSCAN_SDK_ROOT"
```

Run two copies of the same application in separate terminals. The modes select
the local shape and color while both applications also subscribe to the other
DDS samples:

**Terminal 1 — black square:**

```bash
$HOLOSCAN_CLI run connext_shapes_holoviz square \
  --no-docker-build --no-local-build \
  --local-sdk-root "$HOLOSCAN_SDK_ROOT"
```

**Terminal 2 — red circle:**

```bash
$HOLOSCAN_CLI run connext_shapes_holoviz circle \
  --no-docker-build --no-local-build \
  --local-sdk-root "$HOLOSCAN_SDK_ROOT"
```

The two instances exchange Shapes samples through Connext DDS and display both
the locally generated and received shapes in Holoviz.

## What can you do with this module?

The module provides reusable C++ publish and subscribe operators for
application-owned DDS types, CMake-driven Connext type generation from IDL,
XML QoS configuration, and ready-to-run applications.

The examples demonstrate two complementary integrations: typed FlatBuffers
payloads for Holoscan-oriented applications and opaque XCDR byte Tensors for
Connext-oriented applications. Everything is built, tested, and run in Docker,
with no Connext or Holoscan installation required on the host.

The repository is currently an engineering prototype for Holoscan 5 EA2. It
is not yet an installed or packaged production module. All builds and tests
run in Docker; the host does not need a Holoscan or Connext installation.

## Start here

Choose the section that matches what you want to do:

| Goal | Read this |
| --- | --- |
| Build the repository and run a first test | [Shapes + Holoviz quick start](#quick-start-shapes-with-holoviz) |
| Understand the DDS and Holoscan terminology | [Concepts and terminology](#concepts-and-terminology) |
| Understand the publisher operator | [Publisher operator](docs/operators/publisher.md) |
| Understand the subscriber operator | [Subscriber operator](docs/operators/subscriber.md) |
| Configure domain, topic, and QoS | [Endpoint configuration](docs/operators/configuration.md) |
| Verify a generated DDS type with a Holoscan payload | [Typed Shapes example](applications/shapes_demo_flatbuffers/README.md) |
| Use typed fields inside a Holoscan graph | [Typed Shapes example](applications/shapes_demo_flatbuffers/README.md) |
| Keep using generated DDS samples and transport XCDR bytes | [XCDR Tensor example](applications/shapes_demo_xcdr/README.md) |
| Add an application-owned IDL | [Using your own IDL](docs/using-your-own-idl.md) |
| Review the engineering scope | [Implementation plan](PLAN.md) |

## Components

### Reusable operators

- `PublisherOp<DdsType, Adapter>` receives a Holoscan payload, converts it to
  a generated DDS type, and writes it with a Connext `DataWriter`.
- `SubscriberOp<DdsType, Adapter>` receives a generated DDS type with a
  Connext `DataReader`, converts it to a Holoscan payload, and emits it into
  the graph.

Both operators own the Connext participant, topic, reader or writer, QoS, and
lifecycle. Applications provide their data type and the conversion at the
Holoscan/DDS boundary.

### Runnable examples

| Example | Holoscan graph payload | DDS type | Primary purpose |
| --- | --- | --- | --- |
| `shapes_demo_flatbuffers` | FlatBuffers `ShapeT` | `ShapeTypeExtended` | Demonstrate typed DDS conversion |
| `shapes_demo_xcdr` | XCDR byte Tensor | `ShapeTypeExtended` | Demonstrate DDS-native XCDR transport |

Each example consists of two independent Holoscan applications: a publisher
and a subscriber. They communicate through DDS, not through an in-process
Holoscan connection.

| Component | Purpose |
| --- | --- |
| [Publisher operator](docs/operators/publisher.md) | Publish an application-owned DDS type from a Holoscan graph |
| [Subscriber operator](docs/operators/subscriber.md) | Receive an application-owned DDS type into a Holoscan graph |
| [Endpoint configuration](docs/operators/configuration.md) | Configure DDS domain, topic, XML QoS, and matching behavior |
| [Typed Shapes application](applications/shapes_demo_flatbuffers/README.md) | Use generated DDS types with a FlatBuffers payload |
| [XCDR Shapes application](applications/shapes_demo_xcdr/README.md) | Use generated DDS types with an XCDR Tensor payload |
| [Own-IDL guide](docs/using-your-own-idl.md) | Generate a new Connext type and integrate it into a graph |
| [Module Dockerfile](Dockerfile) | Holoscan EA2, Connext, Code Generator, and build environment |

Each operator and example documents its own ports, graph, QoS behavior,
memory model, commands, validation, and limitations. This README contains the
shared setup and recommended starting workflow.

## Supported versions

| Component | Version |
| --- | --- |
| Prototype module version | `0.1.0` |
| RTI Connext DDS | `7.7.0` |
| NVIDIA Holoscan SDK | `5.0.0 EA2` |
| CUDA | `13` |
| Validated architecture | `aarch64` |
| Connext architecture | `armv8Linux4gcc8.5.0` |

The module version describes this integration's own API and implementation.
The Connext version is pinned separately by the Docker build argument
`RTI_CONNEXT_VERSION`, which defaults to `7.7.0`. Changing it requires updating
the Debian package, `NDDSHOME`, architecture, generated types, and validation
together.

## Concepts and terminology

### DDS terminology

- **IDL** defines the shared data contract. RTI Code Generator turns the IDL
  into C++ classes and Connext serialization support.
- A **domain** is an isolated DDS communication space identified by a numeric
  domain ID. Only applications using the same domain can discover each other.
- A **topic** combines a topic name and a DDS type. A writer and reader must
  use compatible topic names and types.
- A **DataWriter** publishes samples. A **DataReader** receives samples.
- **Discovery** allows compatible DDS endpoints to find each other without an
  application-level broker.
- **QoS** controls delivery behavior such as reliability, history, durability,
  and resource limits. This repository loads QoS from XML.
- **XCDR** is a DDS wire encoding. The XCDR example stores one encapsulated
  serialized DDS sample in a byte tensor.

### Holoscan terminology

- A **graph** connects processing stages.
- An **operator** is one processing stage in the graph.
- An **input port** receives graph payloads; an **output port** emits them.
- A **payload** is the value carried on a graph edge. Holoscan 5 validates its
  type and, for tensors, its memory and shape contract.
- A **partition** is a group of operators scheduled and deployed together.
- A **NotificationSource** wakes the scheduler when an asynchronous source has
  data. The DDS subscriber uses one instead of polling from `compute()`.

### How the systems meet

Connext-generated C++ classes are not automatically Holoscan graph payloads.
The module therefore uses an adapter:

```text
Holoscan payload <-> application adapter <-> generated DDS type
```

The examples demonstrate two useful boundaries:

```text
Typed: Holoscan ShapeT <-> ShapeAdapter <-> DDS ShapeTypeExtended
Opaque: DDS ShapeTypeExtended <-> XCDR Tensor<uint8_t>
```

See [Choosing a payload boundary](#choosing-a-payload-boundary) before adding
your own data model.

## Requirements

The validated ARM64 development setup uses:

- NVIDIA Holoscan SDK `5.0.0 EA2`.
- CUDA `13` and an NVIDIA driver compatible with the EA2 SDK.
- RTI Connext DDS `7.7.0`.
- Docker with the NVIDIA Container Runtime.
- A valid `rti_license.dat` for runtime tests.
- An NVIDIA platform supported by Holoscan 5. IGX Orin is not supported
  because it does not provide the required CUDA 13 platform.

The default Dockerfile expects these ARM64 values:

```text
Holoscan image: holoscan-sdk-build-aarch64:v5.0.0-ea2
Connext architecture: armv8Linux4gcc8.5.0
```

An x86_64 environment needs a matching Holoscan base image and Connext
architecture passed as Docker build arguments. That path has not yet been
validated by this prototype.

Do not install Holoscan or Connext on the host for this workflow. All build,
test, and runtime dependencies are provided inside Docker.

## RTI Connext license

The source code in this repository is distributed under the RTI license in
`LICENSE` and does not include an RTI Connext runtime license. Request and
download an activation key from the
[RTI Connext license page](https://content.rti.com/l/983311/2025-07-25/q6729c).

Keep the resulting `rti_license.dat` outside the container image and source
control. The quick start downloads it from inside the development container
into the ignored `build-ea2` directory and passes its path through
`RTI_LICENSE_FILE` at runtime.

## Advanced container workflow

## Holoscan CLI workflow

The supported user workflow is to let the Holoscan CLI build and run each application in Docker. The CLI profile in `pyproject.toml` selects the validated EA2 base image.

From the repository root:

```bash
export MODULE_ROOT=/path/to/rticonnextdds-holoscan-module
export HOLOSCAN_SDK_ROOT=/path/to/holoscan-sdk/install-aarch64
export HOLOSCAN_CLI="${MODULE_ROOT}/.venv-holoscan-cli/bin/holoscan"
test -s rti_license.dat
$HOLOSCAN_CLI build-container shapes_demo_flatbuffers
$HOLOSCAN_CLI build shapes_demo_flatbuffers --no-docker-build --local-sdk-root "$HOLOSCAN_SDK_ROOT"
$HOLOSCAN_CLI modes shapes_demo_flatbuffers
```

Run the subscriber and publisher in separate terminals:

```bash
$HOLOSCAN_CLI run shapes_demo_flatbuffers subscriber --no-docker-build --no-local-build --local-sdk-root "$HOLOSCAN_SDK_ROOT"
$HOLOSCAN_CLI run shapes_demo_flatbuffers publisher --no-docker-build --no-local-build --local-sdk-root "$HOLOSCAN_SDK_ROOT"
```

Use the same sequence with `shapes_demo_xcdr`. For the visual reference application, build its CLI image and select `square`, `circle`, `triangle`, `headless_square`, or `headless_circle` with `holoscan run`. The CLI mounts the ignored `rti_license.dat` into the container; the license is never copied into the image or committed.

The following commands assume Bash. Start Bash first if the current shell is
`tcsh` or another shell with different variable syntax:

```bash
bash
```

### 1. Build the Holoscan EA2 SDK

From the private NVIDIA EA2 `holoscan-sdk` source directory, use NVIDIA's
container workflow:

```bash
./run build --arch arm64 --cudaarchs 87 --sccache false
```

The validated build produces:

- Docker image `holoscan-sdk-build-aarch64:v5.0.0-ea2`.
- Installed SDK directory `install-aarch64`.

Do not copy EA2 source or binaries into this repository.

### 2. Define the two repository paths

Set absolute paths for the shell session. Replace the example paths if the
repositories live elsewhere:

```bash
export MODULE_ROOT=/path/to/rticonnextdds-holoscan-module
export HOLOSCAN_INSTALL=/path/to/holoscan-sdk/install-aarch64
cd "${MODULE_ROOT}"
```

Confirm that the SDK installation is present:

```bash
test -f "${HOLOSCAN_INSTALL}/lib/cmake/holoscan/holoscan-config.cmake" \
  || test -f "${HOLOSCAN_INSTALL}/lib/cmake/holoscan/holoscan-full-config.cmake"
```

### 3. Build the module development image

```bash
docker build -t rticonnextdds-holoscan-module:ea2 "${MODULE_ROOT}"
```

The image installs Connext 7.7.0, RTI Code Generator, the pinned Holoscan CLI
(`5.0.0a1` for EA2), CMake, and the build tools. It does not contain a
Connext license or private Holoscan artifacts.

### 4. Configure and compile the module

```bash
docker run --rm --network=host \
  --user "$(id -u):$(id -g)" \
  -v "${HOLOSCAN_INSTALL}:/opt/holoscan:ro" \
  -v "${MODULE_ROOT}:/workspace/rticonnextdds-holoscan-module" \
  -w /workspace/rticonnextdds-holoscan-module \
  rticonnextdds-holoscan-module:ea2 \
  bash -lc 'cmake -S . -B build-ea2 -G Ninja \
    -DCMAKE_PREFIX_PATH=/opt/holoscan && cmake --build build-ea2'
```

Generated DDS and FlatBuffers files remain under the ignored `build-ea2`
directory. Nothing is installed on the host.

### 5. Obtain a Connext license

The examples require a valid Connext license. The following command downloads
the Holoscan evaluation license from RTI from inside the module container:

```bash
docker run --rm --network=host \
  --user "$(id -u):$(id -g)" \
  -v "${MODULE_ROOT}:/workspace/rticonnextdds-holoscan-module" \
  -w /workspace/rticonnextdds-holoscan-module \
  rticonnextdds-holoscan-module:ea2 \
  bash -lc 'mkdir -p build-ea2 && \
    curl -fL https://content.rti.com/l/983311/2025-07-25/q6729c \
      -o build-ea2/rti_license.dat'
```

The file is ignored by Git. Never add it to the image or commit it. If the
download is unavailable, obtain a valid license from the
[RTI Connext license page](https://content.rti.com/l/983311/2025-07-25/q6729c)
and place it at `${MODULE_ROOT}/build-ea2/rti_license.dat`.

### 6. Run the first DDS round trip

Run the typed Shapes integration test:

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

Success ends with:

```text
100% tests passed, 0 tests failed out of 1
```

This test launches a subscriber and publisher as separate processes, sends 20
`ShapeTypeExtended` samples, and validates every field after reception.

`--runtime=nvidia` is required because Holoscan EA2 runtime binaries link to
the CUDA driver even though these examples are headless and CPU-only.

### 7. Run the complete suite

```bash
docker run --rm --runtime=nvidia --network=host \
  --user "$(id -u):$(id -g)" \
  -e RTI_LICENSE_FILE=/workspace/rticonnextdds-holoscan-module/build-ea2/rti_license.dat \
  -v "${HOLOSCAN_INSTALL}:/opt/holoscan:ro" \
  -v "${MODULE_ROOT}:/workspace/rticonnextdds-holoscan-module" \
  -w /workspace/rticonnextdds-holoscan-module \
  rticonnextdds-holoscan-module:ea2 \
  ctest --test-dir build-ea2 --output-on-failure
```

The suite contains six tests: SDK/runtime smoke tests, generated-type tests,
typed and XCDR adapter tests, and three real DDS integrations.

## Choosing a payload boundary

The typed Shapes and XCDR Shapes examples use the same DDS type and topic but
optimize for different users:

| Question | Companion FlatBuffers payload | XCDR byte Tensor |
| --- | --- | --- |
| Most natural for | Holoscan developer | Existing Connext developer |
| Application API | Holoscan `ShapeT` | Generated DDS `ShapeTypeExtended` |
| Graph port | Typed structured payload | `Tensor<uint8_t>` |
| Access fields in graph | Directly | Deserialize first |
| Per-IDL Holoscan schema | Required | Not required for opaque stages |
| Nested or evolving IDL | Matching schema and adapter must evolve | Encoded without duplicating the field model |
| Current conversion cost | Field mapping and copy | XCDR serialization, allocation, and deserialization |
| GPU use | Can expose GPU-oriented fields or tensors | Opaque XCDR is not GPU-native |

Choose FlatBuffers when Holoscan operators need to inspect or transform the
fields. Choose XCDR when existing Connext code should keep using its generated
sample and intermediate graph operators only need to route opaque data.

The XCDR graph boundary is more IDL-agnostic, but the current DDS endpoint is
not type-erased. It still instantiates a generated `DataWriter<DdsType>` or
`DataReader<DdsType>` and uses the serializer for that type.

## Troubleshooting

### `libcuda.so.1` cannot be loaded

Add `--runtime=nvidia` to the `docker run` command and confirm that the NVIDIA
Container Runtime works on the host.

### Connext reports a license error

Confirm that the file exists and is not expired:

```bash
test -s "${MODULE_ROOT}/build-ea2/rti_license.dat"
grep -i FEATURE "${MODULE_ROOT}/build-ea2/rti_license.dat"
```

Confirm that the container command sets:

```text
RTI_LICENSE_FILE=/workspace/rticonnextdds-holoscan-module/build-ea2/rti_license.dat
```

### The publisher times out waiting for a reader

Start the subscriber first. The publisher waits up to ten seconds for a
compatible DDS reader before reporting an error.

If both applications are running, verify that they use the same domain ID,
topic name, DDS type, and compatible QoS. Host networking must also permit DDS
discovery and user-data traffic.

### CMake cannot find Holoscan

Verify the `HOLOSCAN_INSTALL` host path and its read-only mount at
`/opt/holoscan`. The configure command must include:

```text
-DCMAKE_PREFIX_PATH=/opt/holoscan
```

### CMake cannot find Connext or RTI Code Generator

Rebuild the module image. The Dockerfile installs Connext 7.7.0 and sets
`NDDSHOME`; no host Connext installation is used.

## Current limitations

- This is EA2 prototype source, not an installed CMake or Debian package.
- Only the ARM64 container workflow described above has been validated.
- Python bindings are not included.
- The FlatBuffers adapter is handwritten.
- The XCDR path allocates a host buffer and tensor ownership metadata for each
  sample; it is not zero-copy.
- No GPU or network performance claim is made by the conversion benchmark.
- A native DDS External Topics provider is not implemented.

Architecture decisions are recorded in
[ADR 0001](docs/adr/0001-ea1-payload-boundary.md) and
[ADR 0002](docs/adr/0002-ea2-shapes-payload-boundary.md).

## Ownership and contact

Vendor: RTI Real-Time Innovations

Contact: `holoscan@rti.com`

## Learn more

- [Connext Developer's Guide](https://community.rti.com/static/documentation/developers/)
- [RTI Connext Professional](https://www.rti.com/products/connext-professional)
- [RTI and NVIDIA](https://www.rti.com/products/third-party-integrations/nvidia)
- [The Connext databus](https://www.rti.com/products/what-is-a-databus)
- [The DDS standard](https://www.rti.com/products/dds-standard)
