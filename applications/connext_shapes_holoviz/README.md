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
FlatBuffers to DDS mapping, and `shape_renderer.hpp` for the RGBA8 `ImageT` sent to
Holoviz. `shape_display_config.hpp` holds the example's logical canvas and render scale.

## Behavior

- By default it publishes a black `Square` on domain `0`.
- The local shape moves and bounces in a logical 256 by 256 Shapes Demo area.
- Separate DDS readers subscribe to `Square`, `Circle`, and `Triangle`.
- Local DDS publications are ignored by those readers, so the local shape is
  drawn once and external shapes receive a dark-blue outline.
- It runs until `Ctrl+C` or `SIGTERM`, then requests a clean Holoscan stop.

## Holoscan CLI workflow

`metadata.json` exposes `square`, `circle`, `triangle`, `headless_square`, and `headless_circle` as first-class Holoscan CLI modes:

```bash
export HOLOSCAN_CLI=./.venv-holoscan-cli/bin/holoscan
$HOLOSCAN_CLI build-container connext_shapes_holoviz
$HOLOSCAN_CLI build connext_shapes_holoviz --no-docker-build
$HOLOSCAN_CLI run connext_shapes_holoviz headless_square --no-docker-build --no-local-build
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

## Tests

See the [testing guide](../../tests/README.md#headless-visual-tests) for the
two-container visual test, SHMEM-only verification, and saved captures.
