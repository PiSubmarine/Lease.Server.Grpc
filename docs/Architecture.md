# Lease.Server.Grpc

`PiSubmarine.Lease.Server.Grpc` adapts `Lease.Api::ILeaseIssuer` to a secure
gRPC service.

## Security

This module uses mutual TLS by default:

- server certificate secures the channel
- client certificate is required and verified
- unauthenticated peers are rejected at the RPC boundary

No lease business logic lives here; the server delegates to `Lease.Api`.
