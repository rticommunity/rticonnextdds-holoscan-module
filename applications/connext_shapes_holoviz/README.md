# Connext Shapes with Holoviz

This reference application displays moving Shapes samples in Holoviz while exchanging
the standard RTI Shapes Demo `ShapeTypeExtended` DDS type.

```text
local Holoscan shape -> Holoviz
                      -> DDS writer -> external DDS readers

external DDS Shapes -> DDS readers -> Holoviz
```

The application owns its FlatBuffers schema, adapter, and QoS XML. It uses the
standard `ShapeType.idl` provided by the installed Connext distribution, so it
remains wire-compatible with RTI Shapes Demo.

## Read the code

Start with `main.cpp` for CLI options and graph connections. Then read
`bouncing_shape_source.hpp` for local generation, `shape_adapter.hpp` for the
FlatBuffers to DDS mapping, and `shape_renderer.hpp` for the RGBA frame sent to
Holoviz. `shape_display_config.hpp` holds the example's logical canvas and render scale.

## Behavior

- By default it publishes a black `Square` on domain `0`.
- The local shape moves and bounces in a logical 256 by 256 Shapes Demo area.
- Separate DDS readers subscribe to `Square`, `Circle`, and `Triangle`.
- Local DDS publications are ignored by those readers, so the local shape is
  drawn once and external shapes receive a dark-blue outline.
- It runs until `Ctrl+C` or `SIGTERM`, then requests a clean Holoscan stop.

## Run in a container

## Holoscan CLI workflow

`metadata.json` exposes `square`, `circle`, `triangle`, `headless_square`, and `headless_circle` as first-class Holoscan CLI modes:

```bash
export HOLOSCAN_CLI=./.venv-holoscan-cli/bin/holoscan
export HOLOSCAN_SDK_ROOT=/path/to/holoscan-sdk/install-aarch64
$HOLOSCAN_CLI build-container connext_shapes_holoviz
$HOLOSCAN_CLI build connext_shapes_holoviz --no-docker-build --local-sdk-root "$HOLOSCAN_SDK_ROOT"
$HOLOSCAN_CLI run connext_shapes_holoviz headless_square --no-docker-build --no-local-build --local-sdk-root "$HOLOSCAN_SDK_ROOT"
```

Use `square`, `circle`, or `triangle` instead of the headless modes when a real display is available. The normal modes open a Holoviz window on that display; the headless modes create a virtual Xvfb display inside the container. To demonstrate DDS exchange, run `square` and `circle` in separate terminals or run one side against RTI Shapes Demo. The CLI mounts the ignored project-root `rti_license.dat` into the container.

Complete the root [container quick start](../../README.md#quick-start-shapes-with-holoviz),
then run the application from the module build directory inside the module container:

```bash
./connext_shapes_holoviz --domain-id 0
```

Useful options:

```text
--publish-topic Square|Circle|Triangle
--publish-color BLACK|RED|GREEN|BLUE|YELLOW|ORANGE|PURPLE
--domain-id ID
--width PX --height PX
```

A second instance can publish an external red circle:

```bash
./connext_shapes_holoviz --domain-id 0 --publish-topic Circle --publish-color RED
```

It may instead be paired with RTI Shapes Demo on the same domain. External
`Square`, `Circle`, and `Triangle` samples appear with a dark-blue perimeter.

## Headless visual test

This is an opt-in integration test, separate from the normal CTest suite. It
starts two application containers, creates one Xvfb display in each container,
captures both Holoviz windows, and checks that each image contains the local
black square, the external red circle, and the dark-blue external outline. No
physical monitor or `DISPLAY` variable is required.

The test does require Docker orchestration access from the shell that launches
it. In particular, the shell must have the Docker CLI, access to the Docker
daemon, and the host paths used below must be visible to that daemon. A failure
to meet those requirements is an infrastructure failure, not a Holoviz image
validation failure.

Captures are kept in:

```text
build/rticonnextdds-holoscan-module/test-artifacts/connext_shapes_holoviz/
```

Prepare the CLI image and the host paths first:

```bash
export MODULE_ROOT="$PWD"
export HOLOSCAN_SDK_ROOT="$MODULE_ROOT/holoscan-sdk/install-aarch64"
export HOST_WORKSPACE="$MODULE_ROOT"
export HOST_SDK_ROOT="$HOLOSCAN_SDK_ROOT"
test -s "$HOST_WORKSPACE/rti_license.dat"
test -f "$HOST_SDK_ROOT/lib/cmake/holoscan/holoscan-full-config.cmake"
$HOLOSCAN_CLI build connext_shapes_holoviz \
  --local-sdk-root "$HOLOSCAN_SDK_ROOT"
export HOST_BUILD_DIR="$MODULE_ROOT/build/connext_shapes_holoviz"
test -x "$HOST_BUILD_DIR/applications/connext_shapes_holoviz/connext_shapes_holoviz"
```

Select the EA2 image explicitly and run the visual test directly:

```bash
IMAGE=rticonnextdds-holoscan-module-connext_shapes_holoviz:ea-2
docker image inspect "$IMAGE" >/dev/null

HOST_BUILD_DIR="$HOST_BUILD_DIR" bash tests/run_connext_shapes_holoviz_visual.sh \
  "$IMAGE" \
  build/rticonnextdds-holoscan-module/test-artifacts/connext_shapes_holoviz
```

Success prints `connext_shapes_holoviz visual DDS test passed` and produces
`local.png` and `external.png` in the artifact directory. The script also
checks the expected colors and fails if either application exits or reports a
Holoscan runtime callback error. It does not open a window on the host.

The CTest target `connext_shapes_holoviz_visual_integration` invokes the same
script, but is intentionally not enabled by default because it launches nested
Docker containers. Use the direct command above for the predictable user
workflow.
