# CLAUDE.md

Context and rules for working in the Ogiva repo. Read `docs/PRD.md` before any non-trivial task; it is the source of truth for scope and math.

## What Ogiva is

A ballistics and shooting-analytics plugin for Unreal Engine 5.5 and later (ADR 0005). A portable standard-C++ module (`OgivaCore`) plus an Unreal layer (`Ogiva`), a Rust sync agent, a Rust Axum backend and a SvelteKit dashboard that runs the core's analysis compiled to WASM. Portfolio project aimed at tactical training-simulation vendors that sell worldwide.

## Repo map

| Path | What lives there |
| --- | --- |
| `plugin/Ogiva/` | UE 5.5+ plugin, `Ogiva.uplugin` |
| `plugin/Ogiva/Source/OgivaCore/` | Standard C++ only: ballistics, environment, projectiles, impact, telemetry, analysis; `Build.cs` plus side `CMakeLists.txt` for tests and WASM |
| `plugin/Ogiva/Source/Ogiva/` | Unreal layer: subsystem, DataAssets, Blueprint nodes, debug draw, replay |
| `tests/core/` | Catch2 tests, reference tables, golden files |
| `demo/OgivaRange/` | Range demo project, loads the plugin via `AdditionalPluginDirectories` |
| `sync-agent/` | Rust, local SQLite to API upload |
| `backend/` | Rust Axum + sqlx + Postgres, `migrations/` |
| `dashboard/` | SvelteKit |
| `docs/` | PRD, ADRs, glossary, C4, error-analysis report |

## Architecture rules

- Dependencies point inward. `OgivaCore` never includes engine, network or UI headers: no UObject, no engine math types, no engine containers. The `Ogiva` module, agent and backend depend on its schema, never the reverse.
- `OgivaCore` is SI units, right-handed, Z-up. Unit and axis conversion to Unreal happens only in the `Ogiva` module, in one place, with a UE Automation test.
- `OgivaCore.Build.cs` and its side `CMakeLists.txt` list the same sources and the same determinism flags.
- Physics and analysis are tested in the side CMake build with Catch2, never only inside Unreal.
- Determinism is a feature: no `-ffast-math`, FMA contraction off, seeded RNG only, no wall-clock time inside the solver. Same seed must give bit-identical telemetry on all targets.
- Integrators sit behind the `Integrator` interface. Physics never depends on frame rate.
- Hot paths take SoA batches; no per-projectile virtual calls in the step loop.
- New architectural decisions get an ADR in `docs/adr/` before the code lands.

## Impact and armor boundary

- Ogiva scores hits on zones and resolves armored-zone hits through `ImpactResolver` using a protection table supplied, signed and versioned by the customer.
- Do not add penetration equations, armor-defeat models, or real-world armor or caliber-vs-armor data to this repo.
- The demo table is fictional ("Protection A/B/C" x demo presets) and must stay labeled demo-only.

## Code conventions

- Comments: one line only. No multi-line or block comments anywhere; informal wording is fine if it keeps it to one line.
- C++20 in Unreal Engine coding conventions, `OgivaCore` included. `clang-format` and `clang-tidy` configs at repo root are authoritative.
- Rust: `cargo fmt`, `cargo clippy -D warnings`.
- JavaScript/TypeScript: pnpm only, never npm or yarn. Non-negotiable.
- Python (tooling scripts only): remind Felipe to create and activate a venv before installing anything.
- Authors and branding: "Felipe Carvajal Brown" in `Cargo.toml` authors, footers, reports and plugin metadata.

## How to work here

- Scaffold first: agree the full structure of a milestone, then build it file by file.
- **Do not stop to ask after each file or step.** Build, verify, commit and push through the milestone on your own. Stop and ask only for a critical design decision: public API or ABI shape, a new dependency, an architectural choice that needs an ADR, or a result that contradicts the PRD or ROADMAP.
- For fixes, give diffs or snippets, not full files, unless asked.
- Fix bugs at the root cause. Never loosen a tolerance, change test parameters or add a workaround to make a test pass. If a physics test fails, the physics is wrong until proven otherwise.
- Decision questions go as 2 to 4 multiple-choice options, recommended one marked "(rec)" with a short reason.
- Mermaid C4 diagrams: 2 to 4 word arrow labels, `UpdateRelStyle` with `$offsetX`/`$offsetY` on every relationship.

## Commands

Created during M1; update this section when they change.

```bash
cmake --preset dev && cmake --build --preset dev
ctest --preset dev
cmake --preset wasm && cmake --build --preset wasm
cargo test --workspace
pnpm --dir dashboard install && pnpm --dir dashboard test
```

## Definition of done

- Tests for the change exist and pass locally and in CI.
- Validation gates from `ROADMAP.md` for the current milestone still pass.
- Public API or ABI changes are reflected in `docs/` and the version is bumped.
- No new warnings from the formatters or linters.
