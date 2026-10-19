# Testing

Complete the [quick start prerequisites](../README.md#requirements), including
SDK image, CLI, and license setup. Run the following Bash commands from the
repository root. The tests use the same container workflow as the examples.

## Build the test targets

Build both applications with testing enabled. A fixed image name is used here
so that CTest and the visual test use the same dependencies:

```bash
export HOLOSCAN_CLI="$PWD/.venv-holoscan-cli/bin/holoscan"
export HOST_WORKSPACE="$PWD"
export HOST_BUILD_DIR="$PWD/build/connext_shapes_holoviz"
export IMAGE=rticonnextdds-holoscan-module:ea3
test -s rti_license.dat

$HOLOSCAN_CLI build connext_shapes_holoviz --img "$IMAGE" --parallel 2 \
  --configure-args=-DBUILD_TESTING=ON \
  --configure-args=-DAPP_shapes_demo_flatbuffers=ON \
  --configure-args=-DAPP_connext_shapes_holoviz=ON
```

## Run CTest

```bash
docker run --rm --runtime=nvidia --network=host \
  --user "$(id -u):$(id -g)" \
  -e RTI_LICENSE_FILE=/workspace/rticonnextdds_holoscan_module/rti_license.dat \
  -v "$HOST_WORKSPACE:/workspace/rticonnextdds_holoscan_module" \
  -w /workspace/rticonnextdds_holoscan_module \
  "$IMAGE" \
  ctest --test-dir build/connext_shapes_holoviz --output-on-failure
```

The default suite checks:

- Holoscan and Connext runtime initialization.
- Adapter conversion in both directions, preserving all Shapes fields.
- DDS exchange between separate publisher and subscriber processes: 20 samples,
  with checks for missing, duplicate, reordered, or incorrectly reconstructed data.

To select only the DDS exchange test, append
`-R connext_shapes_demo_flatbuffers_integration` to the CTest command.
The two processes run inside one container. Use the visual tests below to
check communication between separate containers.

## Headless visual tests

Run these tests from a host shell with Docker CLI and daemon access. The host
paths set above must be visible to the daemon. No physical display is required:
each application container starts its own Xvfb display.

```bash
bash tests/run_connext_shapes_holoviz_visual.sh \
  "$IMAGE" build/test-artifacts/connext_shapes_holoviz
```

The test starts a black-square publisher and a red-circle publisher in separate
containers. It checks that both windows contain the local and received shapes,
including the dark-blue outline on external samples, and rejects application
exits or runtime callback errors. Captures are saved as `local.png` and
`external.png` in the specified artifact directory.

Repeat with UDP disabled to verify Connext SHMEM communication:

```bash
bash tests/run_connext_shapes_holoviz_visual.sh \
  "$IMAGE" build/test-artifacts/connext_shapes_holoviz/shmem shmem
```

Success prints `connext_shapes_holoviz visual DDS test passed`. Repeat the
SHMEM test to check communication after application restarts.

The scripts are also registered as CTest tests when configured with
`-DRTI_HOLOSCAN_ENABLE_VISUAL_TESTS=ON`. They are disabled by default because
they require Docker orchestration access and host workspace paths.

## Container shared memory

The CLI profile and visual tests use:

```text
--ipc=host --pid=host --tmpfs=/dev/shm:rw,size=256m
```

On the validated Linux setup, Connext uses System V shared memory. Sharing IPC
and PID namespaces preserves SHMEM communication between containers and with
the host. The private `/dev/shm` isolates HoloMQ POSIX resources and is discarded
when a container is removed.

The examples exchange data between containers through DDS. This configuration
does not support HoloMQ POSIX shared memory between containers. Larger
applications may need a larger tmpfs. IPC and PID namespaces remain shared
with the host; deployments requiring stronger isolation need a different
transport or container configuration.

## Troubleshooting

- **`libcuda.so.1` cannot be loaded:** confirm that the NVIDIA Container Runtime
  works on the host. The CTest command needs `--runtime=nvidia` even for the
  CPU-only DDS test because the SDK runtime links to the CUDA driver.
- **License error:** verify the root `rti_license.dat` is valid and readable,
  and that `RTI_LICENSE_FILE` points to its path inside the container.
- **Publisher times out waiting for a reader:** start the subscriber first for
  manual runs. Check domain, topic, type, compatible QoS, and network access.
- **Visual test cannot launch containers:** check Docker daemon access and that
  `HOST_WORKSPACE` and `HOST_BUILD_DIR` are absolute host paths, not paths from
  inside another container.
