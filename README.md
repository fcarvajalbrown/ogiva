# Ogiva

Engine-agnostic ballistics and shooting-analytics SDK in C++20. Validated external-ballistics physics, structured per-shot telemetry, and trainer-grade analysis, usable from Unreal Engine 5.5, any engine with a C FFI, or the browser via WebAssembly.

> Status: pre-alpha, in development. See [ROADMAP.md](ROADMAP.md).

## Why

Training simulators often export results as CSV and leave the analysis to the customer. Ogiva makes the analysis part of the product: every shot is recorded with its conditions, and every group is explained, including whether a miss comes from the shooter, the sights, or the rifle and ammunition.

## Features

- **Physics:** 3-DOF point-mass model, G1/G7 drag, RK4 and adaptive RK45, zeroing, spin drift, Coriolis, air density from real conditions, seeded gusting wind.
- **Deterministic:** same inputs and seed give bit-identical results on every platform, WASM included.
- **Impact scoring:** hit zones, obliquity and energy per hit, pluggable resolver for armored zones using customer-supplied protection tables.
- **Telemetry:** versioned session, shot and aim-trace records, offline-first local storage.
- **Analysis:** MPI, CEP, confidence intervals, significance-tested sight corrections, aim-trace stability metrics, Monte Carlo equipment vs shooter split.
- **Engine-agnostic:** C++ API, stable C ABI, UE5.5 plugin, WASM build.
- **Dashboard demo:** multi-tenant SaaS (Rust Axum, Postgres, SvelteKit) running the same analysis code in the browser.

## Architecture

```text
Host engine (UE5.5 / any) ─► Ogiva core (C++20 + C ABI) ─► local SQLite
                                                              │
                                                     sync agent (Rust)
                                                              ▼
                     dashboard (SvelteKit + WASM core) ◄─ API (Axum) ─► Postgres
```

Full diagrams and decisions: [docs/PRD.md](docs/PRD.md), [docs/adr/](docs/adr/).

## Repository layout

| Path | Contents |
| --- | --- |
| `core/` | C++20 SDK, tests, WASM bindings |
| `adapters/unreal/Ogiva/` | UE5.5 plugin |
| `demo/unreal/OgivaRange/` | Range demo project |
| `sync-agent/` | Offline-first uploader |
| `backend/` | API and database migrations |
| `dashboard/` | Web dashboard |
| `docs/` | PRD, ADRs, glossary, reports |

## Quick start

Requires CMake 3.28+, a C++20 compiler, Rust stable, Node LTS with pnpm, and UE5.5 for the demo.

```bash
git clone <repo-url> ogiva && cd ogiva
cmake --preset dev && cmake --build --preset dev
ctest --preset dev
```

Engine, backend and dashboard setup: see each folder's README (added per milestone).

## Using the core from C

```c
#include <ogiva/ogiva.h>

ogiva_context* ctx = ogiva_create(NULL);
// load a projectile profile, set environment, fire, read the result
ogiva_destroy(ctx);
```

The C ABI is versioned and only grows; see `ogiva_version()`.

## Scope note

Ogiva computes impact kinematics and scores hits on zones. Outcomes on armored zones come from protection tables supplied and certified by the deploying organization; the repo ships only a fictional demo table.

## License

To be decided (see open questions in the PRD).

## Author

Felipe Carvajal Brown
