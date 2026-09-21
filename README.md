# RTI Connext DDS for NVIDIA Holoscan

![EXPERIMENTAL](assets/experimental-stamp.png)

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
export HOLOSCAN_ARCH=arm64  # use amd64 on x86_64
export CUDA_ARCH=87       # set to the target GPU compute capability
./run build --arch "$HOLOSCAN_ARCH" --cudaarchs "$CUDA_ARCH" --sccache false
```

This produces an architecture-specific SDK image and install directory (for
example, `holoscan-sdk-build-aarch64:v5.0.0-ea2` and `install-aarch64`). Keep
both outside this repository.

Install the EA2 Holoscan CLI in a Python 3.11+ virtual environment. Then obtain the Connext license described in [RTI Connext license](#rti-connext-license) before continuing.

```bash
cd /path/to/rticonnextdds-holoscan-module
python3 --version  # must be Python 3.11 or newer
python3 -m venv .venv-holoscan-cli
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


Run two copies of the same application in separate terminals. The modes select
the local shape and color while both applications also subscribe to the other
DDS samples:

**Terminal 1 — black square:**

```bash
$HOLOSCAN_CLI run connext_shapes_holoviz square --local-sdk-root "$HOLOSCAN_SDK_ROOT"
```

**Terminal 2 — red circle:**

```bash
$HOLOSCAN_CLI run connext_shapes_holoviz circle --local-sdk-root "$HOLOSCAN_SDK_ROOT"
```

The two instances exchange Shapes samples through Connext DDS and display both
the locally generated and received shapes in Holoviz.

## What can you do with this module?

The module provides reusable C++ publish and subscribe operators for
application-owned DDS types, CMake-driven Connext type generation from IDL,
XML QoS configuration, and ready-to-run applications.


- The FlatBuffers example demonstrates typed payloads for Holoscan-oriented applications.
- The XCDR example demonstrates opaque byte Tensor payloads for Connext-oriented applications.
- Everything is built, tested, and run in Docker, with no Connext or Holoscan installation required on the host.

The repository is currently an engineering prototype for Holoscan 5 EA2. It
is not yet an installed or packaged production module. All builds and tests
run in Docker; the host does not need a Holoscan or Connext installation.

## Start here

Choose the section that matches what you want to do:

| Goal | Read this |
| --- | --- |
| Build the repository and run a first test | [Shapes + Holoviz quick start](#quick-start-shapes-with-holoviz) |
| Build manually, run CTest, or troubleshoot containers | [Advanced container workflow](docs/advanced-container-workflow.md) |
| Understand the DDS and Holoscan terminology | [Concepts and terminology](#concepts-and-terminology) |
| Understand the publisher operator | [Publisher operator](docs/operators/publisher.md) |
| Understand the subscriber operator | [Subscriber operator](docs/operators/subscriber.md) |
| Configure domain, topic, and QoS | [Endpoint configuration](docs/operators/configuration.md) |
| Verify a generated DDS type with a Holoscan payload | [Typed Shapes example](applications/shapes_demo_flatbuffers/README.md) |
| Use typed fields inside a Holoscan graph | [Typed Shapes example](applications/shapes_demo_flatbuffers/README.md) |
| Keep using generated DDS samples and transport XCDR bytes | [XCDR Tensor example](applications/shapes_demo_xcdr/README.md) |
| Add an application-owned IDL | [Using your own IDL](docs/using-your-own-idl.md) |

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
| `connext_shapes_holoviz` | FlatBuffers `ShapeT` plus Holoviz frame | `ShapeTypeExtended` | Demonstrate local generation, DDS exchange, and Holoviz visualization |

The FlatBuffers and XCDR examples each provide independent publisher and
subscriber applications. They communicate through DDS, not through an
in-process Holoscan connection. The Holoviz example is one application that
generates a local shape, publishes it, subscribes to the three Shapes topics,
and visualizes both local and received samples.

| Component | Purpose |
| --- | --- |
| [Publisher operator](docs/operators/publisher.md) | Publish an application-owned DDS type from a Holoscan graph |
| [Subscriber operator](docs/operators/subscriber.md) | Receive an application-owned DDS type into a Holoscan graph |
| [Endpoint configuration](docs/operators/configuration.md) | Configure DDS domain, topic, XML QoS, and matching behavior |
| [Typed Shapes application](applications/shapes_demo_flatbuffers/README.md) | Use generated DDS types with a FlatBuffers payload |
| [XCDR Shapes application](applications/shapes_demo_xcdr/README.md) | Use generated DDS types with an XCDR Tensor payload |
| [Shapes + Holoviz application](applications/connext_shapes_holoviz/README.md) | Combine local Shapes generation, DDS pub/sub, and Holoviz |
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
control. The Quick Start expects this ignored file in the module root. The
manual workflow uses `build-ea2/rti_license.dat` and passes its path through
`RTI_LICENSE_FILE` at runtime.

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

## Current limitations

- This is EA2 prototype source, not an installed CMake or Debian package.
- Only the ARM64 container workflow described above has been validated.
- Python bindings are not included.
- The FlatBuffers adapter is handwritten.
- The XCDR path allocates a host buffer and tensor ownership metadata for each
  sample; it is not zero-copy.
- No GPU or network performance claim is made by the conversion benchmark.
- A native DDS External Topics provider is not implemented.


## Ownership and contact

Vendor: RTI Real-Time Innovations

Contact: `holoscan@rti.com`

## Learn more

- [Connext Developer's Guide](https://community.rti.com/static/documentation/developers/)
- [RTI Connext Professional](https://www.rti.com/products/connext-professional)
- [RTI and NVIDIA](https://www.rti.com/products/third-party-integrations/nvidia)
- [The Connext databus](https://www.rti.com/products/what-is-a-databus)
- [The DDS standard](https://www.rti.com/products/dds-standard)
