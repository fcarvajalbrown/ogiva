# ADR 0006: Double precision throughout OgivaCore

- Status: Accepted
- Date: 2026-09-30
- Deciders: Felipe Carvajal Brown

## Context

M1 introduces the state and vector types that every later module builds on: integrators, target-plane crossing, zeroing, telemetry and analysis. The scalar type has to be fixed before any of them lands, because changing it later touches every public signature, every golden file and the WASM build.

Three facts shaped the choice. Since Large World Coordinates, Unreal Engine 5's `FVector` is the double-precision `FVector3d`, and Epic recommends the float variant only where float is required. An adaptive Dormand-Prince integrator in single precision stops meeting tolerances below about 1e-6 and its local error plateaus near 1e-7, which would also cut the range of the M1 RK4 convergence-order test to one or two decades. WebAssembly has a native `f64` type whose arithmetic results are deterministic across implementations, NaN payloads aside.

## Decision

- Every scalar in `OgivaCore` is `double`: vectors, projectile state, environment values, integrator tolerances, telemetry and analysis.
- SoA batches store `double` components.
- No float variant and no scalar template parameter.

## Consequences

- The `Ogiva` module converts units and axes to Unreal's `FVector` without any precision change.
- RK45 tolerances and the convergence tests can use the full range double allows.
- Golden files and determinism checks cover one scalar type on every target, WASM included.
- SoA batches take twice the memory of float. At thousands of rounds in flight this is a few hundred kilobytes, which is not a constraint.
- If a future consumer needs float, it gets a new ADR.

## Alternatives considered

- **float everywhere.** Halves batch memory and doubles SIMD width, but the adaptive integrator floors near 1e-7 relative error, the convergence-order test loses most of its range, and every Unreal boundary would narrow and widen values.
- **Templated on the scalar type.** Supports both, but doubles the determinism and golden-file surface with no current consumer for float.
