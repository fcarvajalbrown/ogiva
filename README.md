<p align="center"><img src="assets/logo.svg" alt="Ogiva logo, an upright bullet inside a target ring" width="128"></p>
<h1 align="center">Ogiva</h1>
<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20-1E4E8C.svg?logo=cplusplus" alt="C++20">
  <img src="https://img.shields.io/badge/Unreal%20Engine-5.5%2B-1E4E8C.svg?logo=unrealengine" alt="Unreal Engine 5.5+">
  <img src="https://img.shields.io/badge/Rust-stable-F26B1D.svg?logo=rust" alt="Rust stable">
  <img src="https://img.shields.io/badge/WebAssembly-analysis-F26B1D.svg?logo=webassembly" alt="WebAssembly analysis">
  <img src="https://img.shields.io/badge/status-pre--alpha-C4500F.svg" alt="Status: pre-alpha">
</p>

Ballistics and shooting-analytics plugin for Unreal Engine 5.5 and later. Validated external-ballistics physics, structured per-shot telemetry, and trainer-grade analysis, with the same analysis code running in the browser via WebAssembly.

> Status: pre-alpha, in development. See [ROADMAP.md](ROADMAP.md).

## Why

Training simulators often export results as CSV and leave the analysis to the customer. Ogiva makes the analysis part of the product: every shot is recorded with its conditions, and every group is explained, including whether a miss comes from the shooter, the sights, or the rifle and ammunition.

## Features

- **Physics:** 3-DOF point-mass model, G1/G7 drag, RK4 and adaptive RK45, zeroing, spin drift, Coriolis, air density from real conditions, seeded gusting wind.
- **Deterministic:** same inputs and seed give bit-identical results on every platform, WASM included.
- **Impact scoring:** hit zones, obliquity and energy per hit, pluggable resolver for armored zones using customer-supplied protection tables.
- **Telemetry:** versioned session, shot and aim-trace records, offline-first local storage.
- **Analysis:** MPI, CEP, confidence intervals, significance-tested sight corrections, aim-trace stability metrics, Monte Carlo equipment vs shooter split.
- **Unreal plugin:** UE 5.5 and later, Blueprint nodes and DataAssets, with the physics in a portable standard-C++ module that also builds to WASM.
- **Dashboard demo:** multi-tenant SaaS (Rust Axum, Postgres, SvelteKit) running the same analysis code in the browser.

## Architecture

```text
Unreal layer (Ogiva module) ─► OgivaCore (standard C++) ─► local SQLite
                                                              │
                                                     sync agent (Rust)
                                                              ▼
          dashboard (SvelteKit + OgivaCore as WASM) ◄─ API (Axum) ─► Postgres
```

Full diagrams and decisions: [docs/PRD.md](docs/PRD.md), [docs/adr/](docs/adr/).

## Repository layout

| Path | Contents |
| --- | --- |
| `plugin/Ogiva/` | UE 5.5+ plugin: `OgivaCore` (standard C++) and `Ogiva` (Unreal layer) |
| `tests/core/` | Catch2 tests for `OgivaCore` |
| `demo/OgivaRange/` | Range demo project |
| `sync-agent/` | Offline-first uploader |
| `backend/` | API and database migrations |
| `dashboard/` | Web dashboard |
| `docs/` | PRD, ADRs, glossary, reports |

## Quick start

Requires CMake 3.25+, Ninja, a C++20 compiler, Rust stable, Node LTS with pnpm, and Unreal Engine 5.5 or later for the plugin and demo.

```bash
git clone https://github.com/fcarvajalbrown/ogiva.git && cd ogiva
cmake --preset dev && cmake --build --preset dev
ctest --preset dev
```

On Windows, run the CMake commands from a Visual Studio Developer PowerShell so Ninja finds MSVC.

Engine, backend and dashboard setup: see each folder's README (added per milestone).

## Scope note

Ogiva computes impact kinematics and scores hits on zones. Outcomes on armored zones come from protection tables supplied and certified by the deploying organization; the repo ships only a fictional demo table.

## License

To be decided (see open questions in the PRD).

## Author

Felipe Carvajal Brown
