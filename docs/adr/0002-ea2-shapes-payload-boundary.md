# ADR 0002: EA2 structured DDS and Holoscan payload boundary

- Status: accepted for the EA2 prototype
- Date: 2026-09-16
- Applies to: Holoscan 5 EA2 Shapes Demo example

## Context

Connext generates `ShapeTypeExtended` from OMG IDL. Holoscan EA2 admits
custom graph payloads through its FlatBuffers schema and codec mechanism, but
that mechanism does not make an arbitrary Connext-generated owning C++ class
directly usable as `Input<T>` or `Output<T>`.

The prototype needs to exercise a realistic structured DDS type and remain
compatible with RTI Shapes Demo.

## Decision

The DDS endpoints use `ShapeTypeExtended`, generated at build time from the
`ShapeType.idl` installed with Connext. Graph ports use a companion EA2
FlatBuffers payload, `rti::holoscan::shapes::ShapeT`. A typed adapter maps all
fields between the two representations.

The reusable publisher and subscriber remain templates over the DDS type and
adapter. They own DDS entity management and scheduler integration; an
application supplies its data model and mapping.

## Consequences

- RTI Shapes Demo can replace either Holoscan endpoint on the `Square` topic.
- Strings, enums, integers, and floating-point fields are validated, so this
  is materially richer than the EA1 scalar boundary.
- Conversion is explicit and testable, but it copies values between two
  independently generated owning types.
- Complex or nested IDL is feasible when a corresponding Holoscan schema and
  mapping exist. Hand-maintaining both becomes less attractive as the model
  grows, so generation from a common source should be evaluated before the
  API is declared production-ready.

## Alternatives considered

### Use `Input<ShapeTypeExtended>` directly

Not selected. The Connext-generated class does not provide the Holoscan EA2
schema package and codec evidence required for a graph payload.

### Keep the DDS type entirely inside an application-specific operator

Valid for narrow integrations and avoids a second public graph type, but it
makes each data model require its own operator behavior. The current adapter
contract better separates reusable DDS endpoint mechanics from application
mapping.

### Carry serialized XCDR as an opaque byte payload

Implemented as a parallel experiment using a bounded host
`Tensor<uint8_t>`. It removes the duplicate field model from the opaque graph
boundary, but graph operators lose typed field access. The tensor owns its
XCDR buffer through `TensorMemoryReference`, so it does not pass a raw pointer.

The experiment confirms that this is functionally viable and works with the
same `ShapeTypeExtended` DDS topic. It also confirms that it is not inherently
zero-copy or GPU-safe: the current path serializes into a new host buffer,
allocates tensor ownership metadata, and reconstructs a DDS object before
typed access. Cross-interface delivery may additionally materialize the
bounded tensor according to its declared inline/copy policy.

### Implement DDS as an External Topics provider

Deferred. EA2 does not expose a public third-party transport-provider API for
External Topics. NVIDIA has suggested bridge code for the current release and
is considering the broader extension point.
