# Public DDS operator headers

This directory contains the prototype public C++ API:

| Header | API | Documentation |
|---|---|---|
| `publisher.hpp` | `PublisherOp<DdsType, Adapter>` | [Publisher operator](../../../../docs/operators/publisher.md) |
| `subscriber.hpp` | `SubscriberOp<DdsType, Adapter>` | [Subscriber operator](../../../../docs/operators/subscriber.md) |
| `config.hpp` | `EndpointConfig` and XML QoS loading | [Endpoint configuration](../../../../docs/operators/configuration.md) |

Start with the repository [README](../../../../README.md) for the container
build and terminology, then use
[Using your own IDL](../../../../docs/using-your-own-idl.md) for an
application walkthrough.
