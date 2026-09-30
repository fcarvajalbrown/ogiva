# ADR 0007: Local firing frame, X downrange, Y left, Z up

- Status: Accepted
- Date: 2026-09-30
- Deciders: Felipe Carvajal Brown

## Context

`OgivaCore` is SI, right-handed and Z-up, but nothing fixed which horizontal axis points where. The point-mass model needs that fixed before it lands: the Coriolis term takes Earth's rotation vector expressed in the local frame, drop and windage are read straight from the state, and the `Ogiva` module converts every position and velocity to Unreal's X-forward, Y-right, Z-up, left-handed frame in centimeters.

## Decision

- The local frame has X along the firing azimuth (downrange), Y to the shooter's left, and Z up. It is right-handed.
- Latitude and firing azimuth enter only through a helper that expresses Earth's rotation vector in this frame.
- The conversion to Unreal negates Y and scales metres to centimeters, and lives only in the `Ogiva` module.

## Consequences

- Drop is the Z component and windage the Y component of the state, with no rotation.
- The Unreal conversion is a sign flip and a scale, simple to cover with one UE Automation test.
- Positions are not geographic. Anything that needs east and north, such as a map overlay, rotates by the firing azimuth outside the solver.

## Alternatives considered

- **East-North-Up.** Earth's rotation vector is trivial in ENU, but every drop and windage readout and the Unreal conversion would have to rotate by the firing azimuth.
