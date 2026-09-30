# Roadmap

Each milestone ends with something demoable and a gate that must pass before the next one starts. Scope and math live in `docs/PRD.md`.

## M1 Core solver

ADRs: [0002](docs/adr/0002-wasm-analysis.md), [0005](docs/adr/0005-unreal-first-plugin.md), [0006](docs/adr/0006-double-precision-core.md), [0007](docs/adr/0007-local-firing-frame.md)

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
- [x] G1/G7 drag tables with PCHIP interpolation
  - API agreed: generic `FPchipCurve` (validated `Create` from X/Y spans, `Evaluate` clamps outside the knots) plus `EDragModel { G1, G7 }`, `GetReferenceDragCurve` and `ReferenceDragCoefficient`, with G1/G7 compiled in. The generic curve leaves room for customer Doppler-derived Cd curves (custom drag models) in M2 profiles.
  - Survey of other solvers: bclibc, the C++ core of py-ballisticcalc (LGPL-3.0, algorithm only, no code copied), moved its drag curve to PCHIP with per-segment Horner coefficients and binary search, but extrapolates the end cubic past the last knot. gehtsoft's BallisticCalculator line uses piecewise 3-point quadratics, neither monotone nor C1. JBM's McCoy-derived programs use CD vs Mach tables.
  - Ogiva therefore stores per-segment Horner coefficients, uses SciPy's weighted harmonic-mean interior slopes with three-point shape-preserving endpoints, and clamps instead of extrapolating.
  - Data found: JBM hosts `mcg1.txt` (79 points) and `mcg7.txt` (84 points), Mach 0 to 5, 4 significant digits, stated as sourced from BRL and posted with McCoy's permission, under a JBM site copyright notice. Primary BRL source for G7 not located yet; McCoy's *Modern Exterior Ballistics* tabulates G1 and G7.
  - First decision was to hold the G1/G7 numbers until a primary public source turned up. None was found, so Felipe had JBM's tables checked by a lawyer and cleared them for import. Provenance, hashes and the regeneration command are in [docs/drag-tables.md](docs/drag-tables.md).
  - `FPchipCurve` landed with tests (validation, knot exactness, linear reproduction, hand-derived slopes, endpoint limiter, monotone knee, peak, clamping). CI green on all five jobs; UE 5.5 BuildPlugin green.
  - `EDragModel`, `GetReferenceDragCurve` and `ReferenceDragCoefficient` in `OgivaDrag.h`, compiled from `tests/core/data/drag/mcg1.txt` and `mcg7.txt` by `tools/drag-tables/generate_reference_drag_data.py`. Tests read the same files: every knot matches exactly, and no interval of either table overshoots its neighbouring knots.
  - Sources checked on the DTIC mirror at archive.org, none tabulating G1/G7 Cd vs Mach: BRL Report 1900 (McCoy 1976, ADB012872, since approved for public release; wind effects only), ADA171462 (BRL-MR-3523), ADA205633 (7.62 mm match bullets), ADA098110 (MC DRAG), ADA162133 (5.56 mm NATO), ADA554683 (BC comparison, public release; discusses G1/G7 BCs but prints no table). apps.dtic.mil returns 403 to scripted requests.
- [x] Point-mass model (drag + gravity + Coriolis)
  - API agreed: non-virtual `FPointMassModel` built from `FPointMassParams` (reference drag curve, BC in kg/m2 with C = m/(i d^2), air density, speed of sound, constant wind, gravity, Earth rotation in the local frame), called as `Derivative(Time, State)` so M2's time-varying wind needs no API break. Helpers `BallisticCoefficientFromImperial` (lb/in2 x 703.0696) and `EarthRotationInFrame(Latitude, Azimuth)`.
  - Drag term in SI: a = -(pi/8) rho Cd_ref(M) |v_r| v_r / C, matching the PRD equation; bclibc uses the same form in imperial units. Coriolis uses ground velocity: -2 Omega x v.
  - Frame fixed by ADR 0007: X downrange, Y left, Z up.
  - Landed in `OgivaPointMass.h` with `StandardGravity` (9.80665 m/s2) and `EarthRotationRate` (7.292115e-5 rad/s) constants. A null drag curve yields NaN rather than silently dropping drag. Tests check drag against the PRD form in m, d and i to 1e-13 relative, wind-relative drag, Coriolis sign (right in the north, left in the south, Eotvos lift eastward) and the BC conversion.
  - Determinism gap for the M2 golden-file gate: `EarthRotationInFrame` uses `std::sin`/`std::cos`, which may differ in the last ulp between libms. The PRD mitigation is Ogiva's own transcendental functions; not written yet.
  - STANAG 4355 Modified Point Mass (spin, yaw of repose) is the NATO standard, aimed at artillery; out of M1 scope per the PRD, candidate for a future ADR.
- [x] `Integrator` interface: Euler, RK4, Dormand–Prince RK45
  - API agreed: abstract `IIntegrator` with one virtual `EError StepBatch(Model, Time, States, TimeStep, OutResults)`, so the virtual cost is per batch, not per projectile; `InvalidArgument` when the spans differ in size. Implementations `FEulerIntegrator`, `FRk4Integrator`, `FDormandPrinceIntegrator`.
  - `FStepResult` holds the new state, the end-point derivative (FSAL for Dormand-Prince, needed by the Hermite target-plane crossing) and `LocalError` as a per-component `FProjectileState` (zero for Euler and RK4), so the RK45 driver can apply a scaled atol + rtol norm across metres and m/s. The accept/reject loop lives in a driver, not in the stepper.
  - Landed in `OgivaIntegrator.h`. Tests: Euler step exact, RK4 and Dormand-Prince reproduce the vacuum parabola in one step, end derivative equals f(t+h, y), batch equals single steps, and local-error order against a 1000-substep Dormand-Prince reference on a smooth linear-in-Mach drag curve (PCHIP is only C1 at knots, which would pollute the order): RK4 measured 4.94, Dormand-Prince error estimate 4.98, theory 5, tolerance +/-0.3.
  - Known cost: Euler and RK4 evaluate f once more per step for `EndDerivative`, which the next step's k1 recomputes. Taking the start derivative as an input would remove it; revisit when batching is profiled in M4.
  - Still open for later items: the adaptive RK45 driver (step control with atol/rtol) is part of target-plane crossing and table generation, not this stepper.
- [x] Target-plane crossing via Hermite dense output
  - API agreed: `FlyToPlane(Integrator, Model, Initial, Plane, Settings, OutCrossing)` in `OgivaFlight.h`. `FPlane { Normal, Offset }` crosses where Dot(Normal, P) == Offset; `FFlightSettings` picks `EStepControl::Fixed` (TimeStep) or `EStepControl::Adaptive` (TimeStep as first step, AbsoluteTolerance + RelativeTolerance on the scaled local-error norm), plus `MaxTime`. The crossing comes from a root of the cubic Hermite through both step endpoints, never the overshooting step. `TargetNotReached` past `MaxTime`.
  - `IIntegrator` gains `ErrorEstimateOrder()` (0 = no estimate: Euler, RK4; 5: Dormand-Prince). Adaptive with order 0 returns `InvalidArgument`, and the step controller uses the order as its exponent, so a future embedded pair needs no driver change.
  - Measured with the approved cubic Hermite: at rtol = atol = 1e-10, adaptive Dormand-Prince's crossing of an 800 m plane in air missed a 0.1 ms RK4 reference by 9e-9 s and 3.9e-6 m/s, and fixed-step Dormand-Prince crossing error fell as h^4, not h^5: interpolation-bound. Felipe approved a quintic Hermite from the same endpoint data (position, velocity, acceleration), velocity taken as its derivative, no API change. Same case after: 1.5e-11 s and 1.7e-8 m/s; fixed-step Dormand-Prince at 0.1 s went from 4.7e-8 s to 5.0e-10 s.
  - Landed in `OgivaFlight.h`. Step controller: scaled RMS norm over the six components with scale atol + rtol max(|y0|, |y1|), factor 0.9 err^(-1/order) clamped to [0.2, 5], last step clipped to `MaxTime`. The root solve is a bracketed Newton with bisection fallback. A state already on the plane returns itself at t = 0; a non-finite step returns `NotConverged`.
  - Tests: vacuum ground and downrange crossings match the closed form to 1e-13 relative (RK4 fixed and Dormand-Prince adaptive); an inclined-plane crossing lies on the plane to 1e-14; adaptive vs fixed agreement in air (bounds 1e-10 s, 1e-7 m, 1e-7 m/s); invalid inputs, `TargetNotReached` and NaN models. UE 5.5 BuildPlugin green.
  - Determinism gap alongside `EarthRotationInFrame`: the step controller uses `std::pow`, so adaptive step sequences can differ across libms in the last ulp. Fixed-step flights are unaffected.
- [x] Zeroing by secant method, holdover and windage solve
  - API agreed: `SolveAim(Integrator, Model, Muzzle, MuzzleSpeed, Target, Settings, OutAim)` returns elevation, azimuth and the crossing, and is the single solver. `SolveZero` (`FZeroRequest`: muzzle, muzzle speed, sight height, zero range) aims at the sight-line point, and `SolveHold` reports drop, drift, elevation and windage at any range relative to that zero.
  - Literature and reference solvers surveyed in [docs/research/aim-solving.md](docs/research/aim-solving.md): no recent paper, bclibc uses damped Newton with a golden-section plus Ridder fallback, Hornady 4DOF fixes the zero angle, and Broyden's generalised secant is the standard 2D shooting solver.
  - Solver agreed: 2D Broyden on elevation and azimuth from the line of sight plus a vacuum drop estimate, miss measured in the plane through the target perpendicular to the line of sight; fallback brackets elevation below the maximum-range angle (growing steps, golden-section peak, Illinois root) and alternates azimuth corrections. Targets past maximum range return `OutOfRange`, never the high-angle lob; a Broyden result with a negative d(vertical)/d(elevation) is rejected as the high root.
  - Landed in `OgivaAim.h` with a public `LaunchDirection(Elevation, Azimuth)` helper (not in the approved sketch; hosts need it to fire a solution). Miss tolerance is max(atol, rtol x range, 64 eps x range), so fixed-step solves converge to roundoff. Drop is positive below the sight line, drift positive toward +Y (left), holds are aim angles minus the zero's. The fallback is reachable from tests through the private `OgivaAimFallback.h`, since Broyden converged in every probed case.
  - Measured: Broyden solved vacuum targets up to 1019.5 m of the 1019.7 m maximum at 100 m/s. In air at 100 m/s the solver's `OutOfRange` boundary (645 m solves, 650 m does not) matches a brute-force elevation scan (peak crossing height +2.6 m at 645 m, -3.8 m at 650 m).
  - Tests: level and inclined vacuum aims match the closed-form low angle to 1e-10 relative, near-maximum-range low angle, out-of-range in vacuum and air, drag + wind + Coriolis aims hit the target (RK4 fixed within 1e-9 m, Dormand-Prince adaptive within 1e-7 m) and agree to 1e-9 rad, vacuum zero and holds against the closed form, crosswind drift and hold signs, fallback against closed form and against Broyden in wind and at 645 m. UE 5.5 BuildPlugin green.
  - `LaunchDirection` and `SightElevation` use `std::sin`/`std::cos`/`std::atan2`: same libm determinism gap as `EarthRotationInFrame`.
- [x] Tests: vacuum parabola, constant-Cd vs RK45, RK4 order of convergence
  - `tests/core/TrajectoryTests.cpp`: 200-step RK4 and Dormand-Prince vacuum flights stay on the closed-form parabola to rounding; Dormand-Prince matches the analytic gravity-free constant-Cd solution (x = ln(1 + k v0 t)/k) to 4e-16 in distance and 2e-15 in speed; constant-Cd RK4 at 1 ms differs from a 0.1 ms Dormand-Prince reference by 5e-12 m (bound 1e-9); global order measured RK4 4.02 (bound 4 +/- 0.2) and Euler 1.0007 (bound 1 +/- 0.1).

- [ ] First reference drop table for the gate
  - Felipe chose a published G1/G7 table and confirmed JBM calculator output is cleared for reuse. JBM's online calculators are retired: `jbmtraj-5.1.cgi` now returns a 404 page stating "the JBM Ballistics Calculators ... have been retired and are no longer available". Blocked on Felipe choosing another source.

**Gate:** vacuum and convergence tests green; first reference drop table within tolerance.

Versioning: `Ogiva.uplugin` stays at 0.1.0 through M1 and bumps to 0.2.0 when the M1 gate passes, instead of per public-API item.

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
  - Raised by Felipe: a friend's company runs a simulator projecting onto a wall, where projector lens distortion made shot-to-image accuracy a hard math problem they solved with an equation Felipe does not recall; Felipe wants Ogiva to do it better.
  - Answers so far: their detection and screen setup are proprietary, so Ogiva must support every detection method and screen type used by the leading systems, chosen by research. Their symptom and fix are unknown. Felipe believes the friend's company is the PRD's target company, pending his confirmation; if confirmed, the PRD open question on projector ranges closes and this item's priority moves up from v2.
  - Decision: stays in v2. Felipe has no projector, camera or laser hardware to build or test against.
  - Research: [docs/research/projector-calibration.md](docs/research/projector-calibration.md). Leading systems use laser plus fixed cameras, IR spot tracking, camera on weapon, or pose tracking; state of the art for calibration is dense structured-light correspondence, which handles lens distortion, curved screens and multiple projectors, versus the single homography common in low-cost systems.
- On-prem / air-gapped deployment bundle
