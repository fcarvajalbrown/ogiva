# ADR 0002: Analysis compiled to WASM for the dashboard

- Status: Accepted
- Date: 2026-09-29

## Context

The dashboard must show the same group statistics, sight corrections and trace metrics the sim shows. Reimplementing the analysis in TypeScript or in the backend would create a second source of truth that drifts over time, and trainers would see different numbers in different places.

## Decision

- The `analysis/` module (and optionally the solver) is compiled to WebAssembly with Emscripten from the same sources as the native build.
- The dashboard loads the WASM module and computes every displayed statistic in the browser from raw telemetry.
- The backend stores raw telemetry and does not recompute analysis; it may cache WASM-computed summaries for listing pages, tagged with the SDK version.
- Golden-file tests compare native and WASM outputs bit for bit.

## Consequences

- One implementation, one set of tests, identical numbers in sim and dashboard.
- Determinism rules (no fast-math, FMA off, own transcendentals where needed) now also bind the WASM build.
- The dashboard bundle carries a WASM payload; it must be lazy-loaded and kept small.
- Changing analysis means rebuilding and redeploying the dashboard's WASM asset in lockstep with the SDK version.
