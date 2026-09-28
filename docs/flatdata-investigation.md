# FlatData investigation

This document records the first feasibility checks for a native Connext FlatData
path on the `flatdata-native` branch.

## Initial findings

- Connext 7.7.0 includes the C++11 FlatData API and code generator support.
- A FlatData sample is an `rti::flat::Sample<...>` backed by a loaned buffer.
- A reader receives it through `dds::sub::LoanedSamples` and must keep the loan
  alive while the application accesses the sample.
- A writer can use `get_loan()` for fixed-size types or `build_data()` for
  mutable types.

## IDL experiment

The standard Shapes IDL cannot be annotated as FlatData without changing its
definition. With a string member, the generator rejected the default type:

```text
@language_binding(FLAT_DATA) in an appendable type requires fixed-size types;
'color' cannot be a string
```

Adding `@mutable` allowed code generation for a type containing the Shapes
fields. The generated type is a FlatData sample and includes the expected
`rti/topic/flat/FlatData.hpp` support.

The generated code also compiled successfully when using the RTI platform
definitions and include paths from the module CMake configuration.

## Interoperability experiment

Two container-only tests were run on domain 0 with the same `Square` topic:

- Standard `ShapeTypeExtended` writer -> experimental FlatData reader:
  `FLAT_READER_NO_SAMPLE`.
- Experimental FlatData writer -> standard `ShapeTypeExtended` reader:
  `PLAIN_READER_NO_SAMPLE`.

Both applications created their DDS entities and the FlatData writer built a
sample successfully, but no sample crossed the type boundary in either
direction. The current experimental type is therefore not interoperable with
the regular Shapes Demo type as generated.

## Current blockers

1. The standard `ShapeTypeExtended` is extensible and inherited. The first
   mutable FlatData definition generated from the same fields did not exchange
   samples with the regular Shapes type in either direction.
2. A Holoscan Tensor cannot expose a reader loan and immediately release it.
   The Tensor wrapper must retain the `LoanedSamples` owner until downstream
   operators finish consuming the buffer.
3. The existing generic DDS operators assume copyable C++ values. FlatData needs
   specialized reader and writer operators with explicit loan ownership.
4. A generic user-facing "provide any IDL and QoS" adapter is not realistic for
   this path. The generated FlatData type and its lifetime adapter are type
   specific.

## Next experiment

Build a container-only FlatData publisher/subscriber pair using a Shapes-shaped
mutable IDL, then test it against the regular Shapes type. Only after that
boundary is proven should the Tensor wrapper and Holoscan graph be added.
