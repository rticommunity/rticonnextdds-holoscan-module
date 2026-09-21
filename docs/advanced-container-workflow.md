# Advanced container workflow

This document covers the manual EA2 container workflow, CTest execution, and
container troubleshooting. For the shortest path, start with the [Shapes + Holoviz quick start](../README.md#quick-start-shapes-with-holoviz).

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
export HOLOSCAN_ARCH=arm64  # use amd64 on x86_64
export CUDA_ARCH=87       # set to the target GPU compute capability
./run build --arch "$HOLOSCAN_ARCH" --cudaarchs "$CUDA_ARCH" --sccache false
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

