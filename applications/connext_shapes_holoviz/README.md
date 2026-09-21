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

## Behavior

- By default it publishes a black `Square` on domain `0`.
- The local shape moves and bounces in a logical 256 by 256 Shapes Demo area.
- Separate DDS readers subscribe to `Square`, `Circle`, and `Triangle`.
- Local DDS publications are ignored by those readers, so the local shape is
  drawn once and external shapes receive a dark-blue outline.
- It runs until `Ctrl+C` or `SIGTERM`, then requests a clean Holoscan stop.

## Run in a container

Complete the root [container quick start](../../README.md#container-quick-start),
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

`connext_shapes_holoviz_visual_integration` starts two containerized instances
on virtual displays, captures both Holoviz windows, and validates the local
black square plus external red circle and dark-blue outline. Captures are kept in:

```text
build/rticonnextdds-holoscan-module/test-artifacts/connext_shapes_holoviz/
```

Run it through CTest from the module container environment:

```bash
ctest --test-dir build/rticonnextdds-holoscan-module \
  -R connext_shapes_holoviz_visual_integration --output-on-failure
```
