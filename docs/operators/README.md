# DDS operator reference

The module provides two reusable C++ Holoscan operators:

- [PublisherOp](publisher.md) converts a Holoscan payload into a generated DDS
  sample and publishes it.
- [SubscriberOp](subscriber.md) receives a generated DDS sample and converts
  it into a Holoscan payload.

Both use [EndpointConfig](configuration.md) for the DDS domain, topic, and XML
QoS profile.

An application supplies:

1. A C++ DDS type generated from its IDL.
2. An adapter defining the Holoscan graph payload and conversion functions.
3. An `EndpointConfig` with a topic name.
4. A matching XML QoS profile.

For a complete implementation, start with the
[typed Shapes example](../../applications/shapes_demo_flatbuffers/README.md).
