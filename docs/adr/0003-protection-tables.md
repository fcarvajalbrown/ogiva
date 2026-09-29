# ADR 0003: Armored-zone outcomes from customer-supplied protection tables

- Status: Accepted
- Date: 2026-09-29

## Context

Force-on-force and adversary scenarios need hits on armored zones (targets, adversaries, trainees' own gear) to be scored and counted. Organizations that buy training simulators certify protection outcomes with their own ballistic-lab test data and require traceability of every result to that data.

## Decision

- The core computes impact kinematics (point, velocity, energy, obliquity) and the zone hit, through the host's raycast callback.
- Outcomes on armored zones come from an `ImpactResolver`. The default resolver is a lookup table (protection profile x projectile profile to stopped, not stopped, or a probability) supplied, signed and versioned by the deploying organization.
- Every resolved hit records the resolver table version in telemetry.
- The repo ships no penetration equations, armor-defeat models, or real-world armor data. It ships one fictional demo table ("Protection A/B/C" x demo presets), labeled demo-only.

## Consequences

- Results are auditable: any outcome traces to a specific certified table version.
- Customers keep ownership and liability of their protection data.
- The demo works end to end with fictional data; real deployments swap the table without code changes.
- Signature verification and table-version handling become part of the core's test suite.
