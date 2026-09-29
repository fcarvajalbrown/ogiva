# ADR 0004: Offline-first telemetry with a sync agent

- Status: Accepted
- Date: 2026-09-29

## Context

Shooting ranges often have poor or no connectivity, and some defense customers run air-gapped. A session must never be lost or block the sim because the network is down.

## Decision

- The core writes telemetry to a local SQLite file through a non-blocking ring-buffer recorder; the host thread never waits on I/O.
- The local file is the source of truth until the server acknowledges the upload.
- A separate Rust sync agent uploads signed, idempotent session bundles, retries with exponential backoff, and marks rows synced only after a server ack.
- The backend re-validates schema version on ingest and rejects bundles it cannot read, without losing them locally.
- Air-gapped deployments run the same backend image on premises; the agent points at it instead of the cloud.

## Consequences

- No data loss on network failure; the sim runs identically online and offline.
- Idempotent ingest makes retries and duplicate uploads safe.
- Schema evolution needs care: the backend must accept older schema versions still sitting on range PCs.
- One more process (the agent) to install and monitor on each range PC.
