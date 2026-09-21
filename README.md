# RTI Connext DDS module for Holoscan 4.6

This repository contains one C++ reference application:
`applications/connext_shapes_holoviz`. It generates black RTI Shapes samples,
publishes them to Connext DDS, subscribes to `Square`, `Circle`, and `Triangle`
samples, and renders local and received shapes with Holoviz.

The supported environment is NVIDIA Holoscan 4.6.0 with CUDA 12 and RTI
Connext DDS 7.7.0. The source is intended to be consumed by HoloHub through
the `source_location` value `holohub/4.6` in `metadata.json`.

## Prerequisites

- aarch64 or x86_64 NVIDIA system with a working NVIDIA Container Runtime;
- Docker and the Holoscan 4.6 CUDA 12 image;
- an RTI Connext DDS 7.7.0 license file;
- network access to the NVIDIA and RTI package repositories during the image build.

Do not install Holoscan, CUDA, Connext, or project dependencies on the host.

## RTI license

Obtain an RTI Connext evaluation or commercial license and save it as
`rti_license.dat` in this repository. Do not commit the file. At runtime mount
it read-only at `${NDDSHOME}/rti_license.dat` and set `RTI_LICENSE_FILE` to that
path.

## Container-only build

The default Docker base is
`nvcr.io/nvidia/clara-holoscan/holoscan:v4.6.0-cuda12-dgpu`.

```bash
docker build --build-arg BASE_IMAGE=nvcr.io/nvidia/clara-holoscan/holoscan:v4.6.0-cuda12-dgpu \
  --tag rticonnextdds-holoscan:holohub-4.6 .
docker run --rm -it --network host \
  -v "$PWD:/workspace/rticonnextdds-holoscan-module" \
  -v "$PWD/rti_license.dat:/opt/rti.com/rti_connext_dds-7.7.0/rti_license.dat:ro" \
  -w /workspace/rticonnextdds-holoscan-module \
  rticonnextdds-holoscan:holohub-4.6 bash
```

Inside the container:

```bash
cmake -S . -B build-holohub-4.6 -DBUILD_TESTING=ON
cmake --build build-holohub-4.6 --parallel
ctest --test-dir build-holohub-4.6 --output-on-failure
```

The build generates the standard RTI `ShapeTypeExtended` type from the
Connext-installed `resource/idl/ShapeType.idl`.

## Headless validation

Run without a graphical display. In this mode the application keeps the
Holoscan source, Connext publisher, and Connext subscriber active, and uses a
validation sink instead of initializing Vulkan; the visual path is covered by
the command below.

```bash
./build-holohub-4.6/applications/connext_shapes_holoviz/connext_shapes_holoviz \
  --headless --samples 40 --domain-id 42
```

The process publishes 40 black `Square` samples and polls all three DDS input
topics. Use a second containerized RTI Shapes-compatible reader on the same
domain to verify external DDS interoperability.

## Holoviz visual run

When graphical access is available, pass the X11 display through to the
container and omit `--headless`:

```bash
docker run --rm -it --network host --gpus all \
  -e NVIDIA_DRIVER_CAPABILITIES=all --cap-add CAP_SYS_PTRACE --ipc=host \
  --ulimit memlock=-1 --ulimit stack=33554432 \
  -e DISPLAY="$DISPLAY" -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v "$PWD:/workspace/rticonnextdds-holoscan-module" \
  -v "$PWD/rti_license.dat:/opt/rti.com/rti_connext_dds-7.7.0/rti_license.dat:ro" \
  -w /workspace/rticonnextdds-holoscan-module \
  rticonnextdds-holoscan:holohub-4.6 \
  ./build-holohub-4.6/applications/connext_shapes_holoviz/connext_shapes_holoviz \
  --domain-id 0 --samples 300
```

## DDS configuration

The application uses the RTI Shapes Demo contract:

| Setting | Value |
|---|---|
| Type | `ShapeTypeExtended` |
| Topics | `Square`, `Circle`, `Triangle` |
| Domain | `--domain-id`, default `0` |
| QoS | `HoloscanShapes::BestEffort` |
| XML | `HoloscanConnextQos.xml` |

`--publish-topic` selects the topic written by this process. The generated
local sample is black; external samples retain their DDS color and geometry.

The Holoscan graph is:

```text
ShapeSource ──┬── ShapesPublisher ── Connext DDS
              └── ShapesRenderer ── Holoviz
Connext DDS ─── ShapesSubscriber ──┘
```

All code is C++ and all build/test/run actions are designed to happen inside
the pinned container.
