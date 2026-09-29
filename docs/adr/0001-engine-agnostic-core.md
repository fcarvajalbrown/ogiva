# ADR 0001: Engine-agnostic core with a C ABI

- Status: Accepted
- Date: 2026-09-29

## Context

Ogiva targets UE5.5 first, but the value is the physics and analysis, not the engine integration. Training-sim vendors use different engines, custom native apps, or web front ends, and a portfolio piece should show clean separation of concerns.

## Decision

- All physics, telemetry and analysis live in `core/`, pure C++20 with no engine, network or UI dependencies.
- Hosts reach the core through a C++ API or a stable C ABI (`ogiva.h`): opaque handles, POD structs, error codes, no exceptions across the boundary.
- Engine specifics (units, axes, collision, rendering) live in thin adapters. The host supplies collision through a raycast callback.
- The core is SI, right-handed, Z-up; each adapter owns its conversion.

## Consequences

- One tested core serves UE, other engines, the backend (via Rust bindings later) and the browser (via WASM).
- Adapters stay small and are easy to add.
- The C ABI needs discipline: additive changes only, versioned, documented.
- Some UE conveniences (UObjects in the core, engine math types) are deliberately unavailable in the core.
