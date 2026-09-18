# ADR 0001: EA1 DDS and Holoscan payload boundary

- Status: provisional
- Date: 2026-09-06
- Applies to: Holoscan 5 EA1 proof of concept

## Context

RTI Code Generator produces strongly typed C++ classes from application-owned
IDL. Holoscan 5 EA1 does not admit arbitrary generated owning C++ classes as
`Input<T>` or `Output<T>` graph payloads.

The proof of concept must demonstrate reusable Connext publisher and subscriber
operators with more than one IDL type without claiming an unsupported direct
graph-port integration.

## Decision

The operators are templates over two application-provided types:

- The generated Connext DDS type used to create the typed topic, reader, and
  writer.
- An adapter that declares an EA1-admitted `holoscan_type` and converts between
  that graph payload and the generated DDS type.

The examples use Holoscan schema payloads as their graph boundary. Their Shapes
adapters construct and validate the complete generated DDS samples while the
XCDR adapter exposes the serialized sample as a Tensor.
The generated DDS types never cross the EA1 graph-port boundary.

The subscriber uses a Connext `WaitSet` on `DATA_AVAILABLE` and posts a bounded
Holoscan `NotificationSource`. DDS waiting therefore occurs on a dedicated
thread and does not block a Holoscan execution lane or require periodic graph
polling.

## Consequences

- The same operator implementation is exercised with two unrelated IDL types.
- Applications retain ownership of their IDL, generated code, and mapping.
- The conversion can copy or transform data. This proof of concept makes no
  zero-copy claim.
- The public API must not be frozen around this adapter until EA2 is evaluated.
- A user whose graph payload differs from the DDS model must provide a typed
  adapter; they do not need to reimplement DDS entity or scheduler handling.

## Rejected EA1 alternatives

### Direct `Input<DdsType>` and `Output<DdsType>`

Rejected because generated Connext owning types are outside EA1's admitted
payload families.

### Serialize every graph edge as opaque XCDR bytes

Rejected for the initial strongly typed example. It would hide the application
data contract, introduce ownership and memory-placement questions, and would
not by itself provide GPU-safe or zero-copy semantics.

### Poll the reader from `compute()`

Rejected because it would spend scheduler activations checking for external
data and would not demonstrate the EA1 readiness integration required for a
production-oriented subscriber.

## EA2 review

Evaluate EA2 custom FlatBuffers payload and codec registration. Determine
whether the adapter can be generated from the IDL, whether a companion Holoscan
schema is required, and whether the graph can expose a richer admitted payload
without changing the internal DDS endpoint lifecycle.
