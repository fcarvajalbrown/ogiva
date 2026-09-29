# Glossary

Terms used across Ogiva code and docs. Core units are SI unless stated.

| Term | Meaning |
| --- | --- |
| BC | Ballistic coefficient: how well a projectile keeps velocity, relative to a reference shape (G1 or G7) |
| G1 / G7 | Standard reference projectiles with published drag tables; G7 fits modern long boat-tail bullets better |
| Cd(M) | Drag coefficient as a function of Mach number |
| Form factor (i) | Ratio between a projectile's drag and the reference projectile's drag |
| Mach (M) | Air-relative speed divided by local speed of sound |
| Point-mass (3-DOF) | Trajectory model treating the projectile as a point with position and velocity only |
| RK4 / RK45 | Runge–Kutta integrators: fixed-step 4th order, and adaptive Dormand–Prince 4(5) |
| Dense output | Interpolating the solution between integrator steps to find exact events |
| TOF | Time of flight |
| MV | Muzzle velocity |
| Zeroing | Setting the sight so the trajectory crosses the line of sight at a chosen range |
| Holdover / windage | Vertical and horizontal aim corrections for a given range and wind |
| Sg | Gyroscopic stability factor; above about 1.5 is considered stable |
| Twist rate | Barrel rifling pitch, length per full turn, right or left hand |
| Spin drift | Lateral drift caused by projectile spin |
| Aerodynamic jump | Vertical deflection caused by crosswind at the muzzle |
| Coriolis | Deflection from Earth's rotation; depends on latitude and firing azimuth |
| ISA | International Standard Atmosphere, the default environment preset |
| OU process | Ornstein–Uhlenbeck process: seeded, mean-reverting noise used for wind gusts |
| MIL (mrad) | Milliradian: 1 unit = 1 m at 1000 m |
| MOA | Minute of angle: 1/60 degree, about 29.1 mm at 100 m |
| Click | One turret adjustment step, e.g. 0.1 MIL or 1/4 MOA, set per optic profile |
| String | A series of shots fired as one group |
| MPI | Mean point of impact; its offset from the aim point is bias (accuracy) |
| ES | Extreme spread: largest distance between any two impacts in a group |
| Mean radius | Average distance of impacts from the MPI |
| CEP | Circular error probable: radius containing 50% of impacts |
| Hotelling T² | Multivariate test used to decide if the MPI offset is significant before suggesting a correction |
| Aim trace | Fixed-rate samples of aim position before and after the shot break |
| Break | The moment the trigger releases the shot |
| Follow-through | Aim movement right after the break |
| Hit zone | Named region of a target or avatar, tagged protected or unprotected |
| Protection profile | Identifier for the protection on a zone, resolved through a customer-supplied table |
| ImpactResolver | Core interface that decides the outcome of a hit on a protected zone |
| Obliquity | Angle between the impact direction and the surface normal |
| Blue-on-blue | A hit on a friendly participant in force-on-force drills |
| Sync agent | Process that uploads local sessions to the backend when online |
| RLS | Postgres row-level security, used to isolate tenants by `org_id` |
