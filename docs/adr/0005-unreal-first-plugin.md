# ADR 0005: Unreal-first plugin with a portable math module

- Status: Supersedes 0001
- Date: 2026-09-29
- Deciders: Felipe Carvajal Brown

## Context

ADR 0001 made Ogiva an engine-agnostic C++20 core reached through a C ABI, with Unreal Engine as one thin adapter among several. The product is aimed at Unreal Engine 5.5 and later, not at every engine, and the engine-agnostic framing adds a C ABI, a separate CMake-built core and an adapter layer that do not serve that target.

ADR 0002 still requires the analysis code to compile to WebAssembly for the dashboard, so that the sim and the dashboard compute statistics from one implementation.

## Decision

- Ogiva ships as one Unreal Engine plugin supporting UE 5.5 and later, built by UnrealBuildTool.
- The solver and analysis live in a plugin module written in standard C++ only: no UObject, no engine math types, no engine containers, no engine headers. The rest of the plugin (subsystem, DataAssets, Blueprint nodes, debug draw, replay) is ordinary Unreal code that depends on that module.
- The C ABI and support for other engines are dropped.
- Code follows Unreal Engine coding conventions, including inside the portable module.
- A side CMake build compiles the portable module's sources outside Unreal for native Catch2 tests and for the Emscripten WASM build. Physics and analysis tests run there. UE Automation tests cover only the Unreal-facing layer, including unit and axis conversion.

## Consequences

- ADR 0002 stays valid: the dashboard's WASM module is built from the same sources the plugin compiles.
- Physics tests run in seconds without launching the editor, and core CI does not need an Unreal install.
- The portable module has two build definitions (Build.cs and CMake) that must list the same sources and the same determinism flags.
- Unity, Godot, Bevy and native hosts are no longer supported; the Rust `ogiva-sys` crate and other-engine adapters leave the v2 plan.
- The sync agent and backend depend on the telemetry schema, not on a C ABI, so ADR 0004 is unaffected.

## Alternatives considered

- **Fully Unreal-native code** (UObject, engine math types, UE Automation for all tests). Simplest to write inside Unreal, but the analysis could no longer compile to WASM, forcing a second implementation for the dashboard and superseding ADR 0002.
- **Keep the engine-agnostic CMake core and only adopt Unreal formatting.** Keeps ADR 0001, but retains the C ABI and adapter layer that the Unreal-only target does not need.
- **UE Automation as the only test framework.** One framework, but every physics test would need the editor and CI would need an Unreal install.
