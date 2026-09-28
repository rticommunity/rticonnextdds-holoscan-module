# RTI Connext DDS Holoscan 5 Module Plan

## Status

This document defines the initial engineering plan for an RTI-owned Holoscan
5 module. It was created from EA1 and has now been reviewed against the
Holoscan 5 EA2 source, documentation, examples, and the integration direction
provided by NVIDIA.

EA2 is a moving pre-release baseline. The plan must be reviewed again against
the Holoscan 5 GA SDK before a public release.
No source, binary, documentation, or other artifact from the private Early
Access distribution will be copied into this repository.

## Objective

Deliver a standalone, installable Holoscan 5 module that connects Holoscan
graphs to RTI Connext DDS using reusable C++ publisher and subscriber
operators.

Applications must own their data model. A user should be able to add an IDL,
generate its C++ types as part of the CMake build, and use the module's
publisher and subscriber without modifying or rebuilding the module itself.

The intended consumer experience is approximately:

```cmake
find_package(holoscan REQUIRED CONFIG)
find_package(RTIConnextDDSHoloscan REQUIRED CONFIG)

rti_holoscan_add_idl(
  TARGET robot_types
  IDL RobotState.idl
)

target_link_libraries(robot_application
  PRIVATE
    RTIConnextDDSHoloscan::dds
    robot_types
)
```

```cpp
using RobotPublisher =
    rti::holoscan::dds::PublisherOp<RobotState>;
using RobotSubscriber =
    rti::holoscan::dds::SubscriberOp<RobotState>;
```

The exact API depends on the Holoscan 5 payload investigation described
below. The example above expresses the desired experience, not a committed
interface.

## Expected first deliverable

The first deliverable is a C++ proof of concept, not a production package. It
will contain:

- A Holoscan 5 module built as a standalone CMake project.
- Reusable Connext publisher and subscriber operators.
- CMake-driven Connext C++ type generation with `rtiddsgen`.
- DDS QoS configured through XML.
- A publisher and subscriber running as independent Holoscan applications.
- An end-to-end check that receives exactly 20 samples of each type.
- A fully containerized build and test workflow.

Completing this proof of concept will provide the evidence needed to define
the production architecture and estimate the remaining work.

## EA2 implementation status

The EA2 prototype now adds a richer interoperability path:

- `ShapeTypeExtended` is generated from the standard Connext
  `ShapeType.idl` and used by the DDS reader and writer.
- An EA2 custom FlatBuffers `ShapeT` is used on Holoscan graph ports.
- A typed adapter maps every Shapes field between both owning types.
- The subscriber uses a Connext `WaitSet` and Holoscan
  `NotificationSource`, rather than polling from `compute()`.
- Two independent Holoscan applications exchange and validate 20 Shapes
  samples through DDS.

This resolves the EA2 feasibility question for structured payloads. It also
confirms that the companion-schema approach involves a conversion/copy and
that automating schema and adapter generation remains an important production
design question.

## Scope

### Initial scope

- RTI Connext DDS 7.7.0.
- Holoscan 5 C++ API.
- Strongly typed C++ generated from application-owned IDL.
- Generic publish and subscribe behavior.
- Domain ID, topic name, and QoS profile configuration.
- Reliable, keep-all example QoS with bounded resource limits.
- Clean participant, topic, writer, and reader lifecycle management.
- Standalone consumption through installed CMake package targets.
- Container-based development, builds, tests, and examples.

### Deferred scope

- Python bindings, until the supported Holoscan 5 Python operator surface is
  available.
- Debian and Python package publication.
- Broad IDL feature coverage beyond the two proof-of-concept types.
- Performance claims until measurements exist.
- A `DynamicData` API. Generated, strongly typed C++ is the primary design.
- Production support for every Connext transport, security configuration, or
  discovery topology.

## Holoscan 5 findings that shape the design

### New operator model

Holoscan 5 is not source compatible with Holoscan 4.x. Operators use typed
`Input<T>` and `Output<T>` members and separate their behavior into:

- `setup()` for ports and structural configuration.
- `contract()` for activation and temporal behavior.
- `compute()` for the steady-state data path.
- `start()` and `stop()` for resource acquisition and release.

The DDS entities should be created at lifecycle start and released at stop.
The per-sample `compute()` path should remain bounded, avoid unnecessary
allocation, and always propagate Holoscan errors explicitly.

### Closed payload type system

EA1 does not admit arbitrary C++ classes as graph-port payloads. Its supported
families are fixed-width scalars, Holoscan schema types, tensors, and a
restricted experimental POD mechanism. A C++ class generated directly by
`rtiddsgen` therefore cannot be assumed to work as `Input<T>` or `Output<T>`.

This is the primary architecture question for the proof of concept. Before
the generic operator API is finalized, we must determine the supported way to
bridge:

```text
Holoscan 5 admitted payload <-> Connext generated C++ type
```

The investigation will evaluate, in this order:

1. Whether EA2 exposes a supported schema/codec extension that can admit a
   Connext-generated owning type directly.
2. Whether a build-time generator can derive the required Holoscan schema and
   adapter from the same application IDL.
3. A documented adapter contract between an application-owned Holoscan
   FlatBuffers type and its Connext-generated type.

The accepted design must not require users to implement new publisher or
subscriber operators. If an application-level adapter is unavoidable, its
surface must be small, typed, testable, and preferably generated.

### Subscriber activation

A DDS subscriber is an asynchronous source, so its activation must be aligned
with the Holoscan 5 scheduler. The proof of concept may use bounded periodic
polling to establish correctness, but that is not automatically acceptable
for the production module.

The production investigation must prefer an event-driven design based on DDS
readiness and a supported Holoscan readiness/notification mechanism. It must
not block a Holoscan worker indefinitely in a DDS wait and should avoid a
high-frequency polling loop. Scheduler integration, syscall rate, bounded
queues, backpressure, and shutdown behavior must be measured and documented.

### External Topics

EA1 External Topics declare typed HoloMQ boundaries using `import_topic<T>()`
and `export_topic<T>()`. They are relevant to the overall integration, but
they do not directly make DDS a Holoscan transport and they follow the same
closed payload rules.

During the architecture phase we will evaluate whether a Connext bridge at an
External Topic boundary provides a cleaner long-term integration than graph
operators. This is a parallel design evaluation, not a replacement for the
initial publisher/subscriber proof of concept.

### Module and packaging maturity

EA1 describes domain operator modules as separately installable components
but does not yet provide a final public contract for every module metadata and
distribution detail. We will use a normal standalone CMake package now and
adopt the official Holoscan 5 module manifest, layout, and packaging helpers
from EA2 or GA when they are available.

The project will consume an installed Holoscan SDK with
`find_package(holoscan CONFIG REQUIRED)`. It will not depend on being placed
inside either the Holoscan SDK source tree or HoloHub.

## Proposed repository structure

```text
rticonnextdds-holoscan-module/
├── CMakeLists.txt
├── cmake/
│   ├── RTIConnextDDSHoloscanConfig.cmake.in
│   └── RTIConnextDDSGenerateIdl.cmake
├── include/rti/holoscan/dds/
│   ├── publisher.hpp
│   ├── subscriber.hpp
│   └── type_adapter.hpp
├── src/
├── applications/
│   └── shapes_demo_flatbuffers/
│       ├── idl/
│       ├── publisher/
│       ├── subscriber/
│       └── USER_QOS_PROFILES.xml
├── tests/
├── docker/
├── packaging/
├── docs/
├── LICENSE
└── README.md
```

The structure may be adjusted to the official Holoscan 5 module template once
NVIDIA publishes it.

## Implementation phases and gates

### Phase 0: establish the development baseline

- Record the exact Holoscan EA and Connext versions used for validation.
- Create development and runtime containers; install nothing on the host.
- Consume Holoscan as an installed SDK dependency.
- Install Connext development packages and Code Generator only in the build
  container.
- Inject `rti_license.dat` at runtime and keep it out of images and Git.
- Add formatting, static analysis, CTest, and test presets.

**Gate:** a minimal standalone Holoscan 5 graph configures, builds, and runs
from this repository in a container.

### Phase 1: resolve payload and scheduling feasibility

- Generate two C++ types from IDL with Connext 7.7.0.
- Confirm the EA2 port-admission behavior for generated Connext types.
- Prototype and compare the supported payload-adapter options.
- Validate publisher input and subscriber output composition.
- Prototype bounded subscriber activation and investigate event-driven wakeup.
- Evaluate External Topics as an alternative boundary architecture.
- Record the decision and rejected alternatives in an ADR.

**Gate:** one supported, reusable path carries both example types across a
Holoscan graph/DDS boundary without modifying the operators per type.

No full implementation estimate will be committed before this gate.

### Phase 2: build the two-IDL proof of concept

- Implement the C++ publisher and subscriber lifecycle.
- Integrate `rtiddsgen -language C++11` into CMake.
- Load domain, topic, and QoS profile configuration.
- Run independent publisher and subscriber processes.
- Publish and validate 20 `ShapeTypeExtended` samples through each path.
- Validate exact counts, identifiers, values, and clean shutdown.
- Add negative tests for incompatible topics/types or missing configuration.

**Gate:** the two-process container test passes repeatedly without sample
loss, hangs, unbounded growth, or teardown errors.

### Phase 3: turn the proof of concept into a reusable module

- Separate public API, private implementation, and examples.
- Define stable namespaces and installed headers.
- Export versioned CMake package targets.
- Provide `rti_holoscan_add_idl()` and hide Code Generator boilerplate.
- Verify that an out-of-tree consumer can use only `find_package()` and the
  installed module.
- Document IDL ownership, generated files, QoS, discovery, licensing, and
  lifecycle behavior.

**Gate:** a clean external consumer project builds and runs without accessing
the module source tree.

### Phase 4: quality and performance validation

- Unit-test type-independent logic as ordinary C++.
- Add graph compile/admission tests and retain useful plan diagnostics.
- Add deterministic graph tests where a synthetic clock is applicable.
- Add real Connext end-to-end tests for both IDL types.
- Measure latency, throughput, CPU use, allocations, syscalls, and queue
  behavior before making performance claims.
- Validate failure and restart paths, including partial startup.
- Test supported `x86_64` and `aarch64` Holoscan 5 targets.

**Gate:** functional, lifecycle, and performance acceptance criteria are
documented and met on NVIDIA-supported Holoscan 5 platforms.

### Phase 5: packaging and publication

- Adopt the final Holoscan 5 module metadata and packaging conventions.
- Produce an installable Debian package for the C++ module.
- Define how the package locates or depends on Connext 7.7.0 without
  redistributing unapproved proprietary artifacts.
- Encode module version and Connext compatibility separately.
- Add reproducible package smoke tests in a clean runtime container.
- Prepare public documentation and NVIDIA listing material.

**Gate:** a user can install the module and build the external example without
cloning this repository or HoloHub.

### Phase 6: Python bindings

- Revisit the Holoscan 5 Python operator API at GA.
- Bind the C++ operator API rather than reimplementing DDS behavior in Python.
- Decide how application-owned generated types cross the Python binding
  boundary.
- Add Python examples and parity tests without weakening the C++ API.

**Gate:** supported Python applications use the same C++ implementation and
pass the same two-IDL communication contract.

## Dependencies and licensing assumptions

- Holoscan SDK 5 is a required external dependency.
- RTI Connext DDS 7.7.0 C++ libraries and `rtiddsgen` are required for the
  initial implementation.
- The module source uses the RTI examples license in `LICENSE`.
- Connext binaries, Debian packages, and licenses are not part of this source
  repository.
- A valid Connext license is supplied to development and test containers as a
  runtime secret or read-only mount; it is never committed or baked into an
  image.
- Redistribution rights and the final Debian dependency model must be agreed
  before public package publication.

## Versioning

The module and its Connext dependency have independent versions:

- Module API/implementation version: semantic versioning, initially `1.0.0`
  for the first supported public release.
- Connext compatibility: initially pinned and tested with `7.7.0`.
- Holoscan compatibility: declared as an explicit supported version range
  after EA2/GA validation.

A module source change increments the module version. A change to the tested
or required Connext release updates compatibility metadata but does not replace
the module's own semantic version.

## Development workflow

- Work in this standalone repository; HoloHub 4.5 remains a reference only.
- Build, test, lint, package, and run exclusively in containers.
- Do not install Holoscan, Connext, code generators, or Python dependencies on
  the host.
- Do not commit Early Access artifacts, RTI credentials, package-repository
  credentials, or `rti_license.dat`.
- Keep changes reviewable with focused commits: infrastructure, architecture,
  operators, examples/tests, documentation, and packaging.
- Require a successful clean container build and test run before merging.
- Treat EA APIs as provisional and isolate them behind the smallest practical
  compatibility layer.

## Current constraints and risks

1. **Payload boundary:** EA2 supports custom FlatBuffers payloads, but generated
   Connext owning types are not used directly as graph payloads. Applications
   use a typed companion schema and adapter.
2. **Generated integration:** producing companion Holoscan schemas and adapters
   automatically for arbitrary application IDLs remains future work.
3. **API stability:** Holoscan 5 EA APIs and packaging conventions may change
   before GA.
4. **Platform access:** validation must run on hardware supported by the
   selected Holoscan 5 release. The available IGX Orin cannot run the CUDA 13
   HoloViz path required by EA2.
5. **Packaging Connext:** public module packages cannot assume that RTI
   proprietary binaries or licenses may be redistributed.
6. **Python timing:** production Python bindings depend on the GA binding
   surface and cannot be designed reliably from EA1.

## Definition of done for the production module

The module is complete when:

- It follows the published Holoscan 5 module format.
- Its C++ operators work with application-owned IDL through a documented,
  supported generated-type workflow.
- A consumer uses it through installed CMake targets without HoloHub or the
  module source tree.
- Publisher and subscriber examples exchange both IDL types reliably between
  independent processes.
- Lifecycle, failure, graph-admission, and end-to-end tests pass in containers
  on supported targets.
- The subscriber integration meets agreed latency, syscall, and boundedness
  criteria.
- Version compatibility, Connext installation, QoS, licensing, and IDL
  integration are documented.
- Installable packages pass clean-environment smoke tests.
- NVIDIA and RTI agree on publication and listing requirements.

## Immediate next step

The EA2 prototype builds and passes its seven containerized tests on ARM64. It
now includes reusable publisher and subscriber operators, CMake-driven Connext
type generation, event-driven subscriber activation, two application-owned IDL
types, and independent publisher/subscriber applications.

Two Shapes integrations exercise the same generated `ShapeTypeExtended` DDS
type. The typed path maps every field to an EA2 custom FlatBuffers payload; the
opaque path carries an encapsulated sample in a bounded host byte tensor. The
decisions and tradeoffs are recorded in
`docs/adr/0002-ea2-shapes-payload-boundary.md`.

The next step is to add a Shapes-to-HoloViz reference application and validate
the complete visual path on a supported CUDA 13 system. That validation cannot
be completed on the available IGX Orin and is waiting for temporary access to
compatible hardware. The reusable DDS operators and current headless examples
do not depend on that remaining visualization work.
