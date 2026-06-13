# Lease.Server.Grpc

`PiSubmarine.Lease.Server.Grpc` adapts `Lease.Api::ILeaseIssuer` to a generated
gRPC `Service`.

## Security

This module assumes it is hosted inside a mutually authenticated gRPC server:

- the shared host terminates TLS and verifies client certificates
- unauthenticated peers are rejected at the RPC boundary

This module does not own bind addresses, certificates, or server lifecycle. No
lease business logic lives here; the adapter delegates to `Lease.Api`.
