# Connext Shapes with Holoviz

This is the sole reference application in the Holoscan 4.6 module. It uses
the classic Holoscan 4.x `Application`, `Operator`, `CountCondition`, and
`HolovizOp` APIs. A local operator generates black shapes, the publisher sends
them to RTI Connext DDS, and the subscriber maps external `Square`, `Circle`,
and `Triangle` samples back into Holoviz primitives.

Build and run instructions are in the repository [README](../../README.md).
