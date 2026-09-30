# Roadmap

Each milestone ends with something demoable and a gate that must pass before the next one starts. Scope and math live in `docs/PRD.md`.

## M1 Core solver

ADRs: [0002](docs/adr/0002-wasm-analysis.md), [0005](docs/adr/0005-unreal-first-plugin.md), [0006](docs/adr/0006-double-precision-core.md)

- [x] Side CMake build for `OgivaCore` with presets (`dev`, `release`, `wasm`), `.clang-format` in Unreal conventions, clang-tidy, CI skeleton
  - `.clang-tidy` enforces Unreal naming except the `b` bool prefix, which clang-tidy cannot express; `#pragma once` allowed.
  - CI (`.github/workflows/core.yml`): Windows MSVC via `tools/build/dev.cmd`, Linux GCC, Linux Clang, macOS AppleClang, plus a lint job with LLVM 23 from apt.llvm.org (PyPI only ships clang-tidy 22, which lacks checks the config uses). clang-tidy runs only on the compile database, so the UBT-only module startup file is skipped. First run green on all five jobs.
  - Toolchain on the dev machine: CMake 4.2, MSVC 19.51 via VS 2026 (run CMake from a VS Developer shell), LLVM 23.1.2, Ninja 1.13.2, UE 5.5 (full) and UE 5.8 (binary-only install, no UBT source); Emscripten not installed yet.
- [x] `Ogiva.uplugin` and `OgivaCore` module skeleton: Build.cs with determinism flags, version query, error codes
  - `Ogiva.uplugin` `VersionName` is the single version source: root CMake and `OgivaCore.Build.cs` both read it.
  - UBT defaults MSVC to `/fp:fast`, so Build.cs forces `FPSemantics = Precise`. Known gap: under UBT with clang-cl on Windows, `/fp:precise` still allows FMA contraction and UBT exposes no per-module hook to add `-ffp-contract=off`; MSVC is the default Windows compiler, so this only bites if a target opts into clang-cl.
  - `OGIVACORE_API` expands to `DLLEXPORT` from `HAL/Platform.h`; Build.cs force-includes that header so OgivaCore sources stay free of engine includes. `Private/OgivaCoreModule.cpp` (IMPLEMENT_MODULE) is the one file in Build.cs and not in CMake.
  - Verified with `RunUAT BuildPlugin` on UE 5.5: Editor, Game Development and Game Shipping build.
  - Catch2 v3.16.0 vendored under `tests/core/vendor/catch2`, GPG-verified and byte-identical to the tag.
- [x] State and vector types, SI units, Z-up
  - Header-only `FVector3`, `FProjectileState`, `FStateDerivative` and `Advance` in `double` (ADR 0006). CI green on all five jobs.
- [ ] G1/G7 drag tables with PCHIP interpolation
  - API agreed: generic `FPchipCurve` (validated `Create` from X/Y spans, `Evaluate` clamps outside the knots) plus `EDragModel { G1, G7 }`, `GetReferenceDragCurve` and `ReferenceDragCoefficient`, with G1/G7 compiled in. The generic curve leaves room for customer Doppler-derived Cd curves (custom drag models) in M2 profiles.
  - Survey of other solvers: bclibc, the C++ core of py-ballisticcalc (LGPL-3.0, algorithm only, no code copied), moved its drag curve to PCHIP with per-segment Horner coefficients and binary search, but extrapolates the end cubic past the last knot. gehtsoft's BallisticCalculator line uses piecewise 3-point quadratics, neither monotone nor C1. JBM's McCoy-derived programs use CD vs Mach tables.
  - Ogiva therefore stores per-segment Horner coefficients, uses SciPy's weighted harmonic-mean interior slopes with three-point shape-preserving endpoints, and clamps instead of extrapolating.
  - Data found: JBM hosts `mcg1.txt` (79 points) and `mcg7.txt` (84 points), Mach 0 to 5, 4 significant digits, stated as sourced from BRL and posted with McCoy's permission, under a JBM site copyright notice. Primary BRL source for G7 not located yet; McCoy's *Modern Exterior Ballistics* tabulates G1 and G7.
- [ ] Point-mass model (drag + gravity + Coriolis)
- [ ] `Integrator` interface: Euler, RK4, Dormand–Prince RK45
- [ ] Target-plane crossing via Hermite dense output
- [ ] Zeroing by secant method, holdover and windage solve
- [ ] Tests: vacuum parabola, constant-Cd vs RK45, RK4 order of convergence

**Gate:** vacuum and convergence tests green; first reference drop table within tolerance.

## M2 Calibers and environment

- [ ] Projectile profile schema + JSON loader + validation
- [ ] Miller Sg with velocity and density corrections
- [ ] Spin drift (Litz) and aerodynamic jump
- [ ] MV temperature sensitivity
- [ ] Air density (Buck vapor pressure), speed of sound
- [ ] Wind field per range segment, OU gusts (seeded)
- [ ] Environment presets: ISA, hot/humid, high altitude, custom
- [ ] Error-analysis report: Euler vs RK4 vs RK45

**Gate:** full reference table set passes; determinism golden files match on Linux, Windows, macOS.

## M3 Impact, telemetry and analysis

- [ ] Hit-zone map and zone lookup through the host raycast callback
- [ ] Impact kinematics: point, velocity, energy, obliquity
- [ ] `ImpactResolver` interface + table resolver + signature/version checks
- [ ] Fictional demo protection table, labeled demo-only
- [ ] Telemetry schema v1, ring-buffer recorder, SQLite sink, JSON/CSV export
- [ ] Group stats: MPI, ES, mean radius, CEP, CI on σ
- [ ] Hotelling T² sight correction in MIL/MOA clicks
- [ ] Aim-trace metrics
- [ ] Monte Carlo equipment-only dispersion
- [ ] Zone accounting: armor vs exposed, blue-on-blue

**Gate:** synthetic-group CI coverage ≈ 95%; recorder never blocks the host thread under load test.

## M4 Unreal layer and range demo

- [ ] `Ogiva` runtime module depending on `OgivaCore`, tested on UE 5.5 and the latest UE 5.x
- [ ] `UOgivaSubsystem`, `UProjectileProfile` DataAsset, Blueprint nodes
- [ ] UE unit/axis conversion in the `Ogiva` module + UE Automation tests
- [ ] Debug trajectory draw, shot replay
- [ ] `OgivaRange` demo: static range, known-distance targets, one adversary scenario with armored zones
- [ ] In-game session summary panel

**Gate:** 60 fps with the solver in the loop on a mid-range PC; UE results match the side CMake build within float tolerance.

## M5 SaaS

- [ ] Emscripten build of analysis (+ solver)
- [ ] Backend: tenancy, RLS on `org_id`, auth and roles, signed idempotent ingest
- [ ] Stripe test-mode billing, plan gating middleware
- [ ] Sync agent: upload, retry with backoff, ack-based sync marks
- [ ] Dashboard: trainee, instructor and admin views
- [ ] i18n (es, en, pt), metric/imperial, MIL/MOA display
- [ ] Region-pinned tenants in config

**Gate:** a session shot in UE appears in the dashboard with identical stats computed by WASM.

## M6 Pitch polish

- [ ] C4 context and container diagrams in `docs/`
- [ ] Error-analysis report finalized
- [ ] README screenshots and GIFs
- [ ] 2-minute video: shot to dashboard
- [ ] Trademark and name availability check for "Ogiva"

**Gate:** someone new clones the repo and runs the demo from the README alone.

## Later (v2)

- Camera-calibration adapter for projector/laser ranges
- On-prem / air-gapped deployment bundle
