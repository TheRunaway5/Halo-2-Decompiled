# Source scene physics in multiplayer

Completed TU1.5 scene roots and WORLD hierarchies now travel with the native
multiplayer snapshot. Each root retains its full source object datum, authored
placement/definition, visibility and exact transform. Objects with no model
retain an actual empty hierarchy. Modeled roots use the existing bounded WORLD
codec and its original signed descriptor extent; all scene owners participate
in pose admission alongside players, corpses and vehicles.

The client validates each placement and generation before applying the received
root and nodes to its existing scene presentation. It captures accepted ticks
for render interpolation and synchronizes prop/scenery collision geometry.
Complete snapshot omissions retire corresponding native scene presentation.
The bridge does not simulate a second physical world, reconstruct rotations
from Euler angles, or supply an impulse/solver result. Initial generation
mismatch and unsupported recreation remain explicit errors; arbitrary late
join/recreation behavior still needs dedicated coverage.

A read-only rendering observer checks every adopted root, visibility value and
WORLD matrix against the received source snapshot before model interpolation.
The multiplayer harness also compares those same roots and all node banks
against the authority's actual same-tick state.

## Evidence

| Run | Result |
| --- | --- |
| `live-integration-v147/lan-objects` | PASS: 19 Lockout roots and181 exact authority comparisons. |
| `live-integration-v149/lan-lift` | PASS: 181 comparisons and518 adopted-scene render frames. |
| `live-integration-v149/lan-movement` | PASS with scene adoption; source-authority comparisons retained. |
| `live-integration-v150/lan-objects` | PASS: 181 comparisons and301 adopted-scene render frames. |
| `live-integration-v150/lan-vehicle` | PASS: 302 comparisons,472 adopted-scene frames,65 forward and25 steering commands, enter/drive/exit and actual seated camera. |

V149's vehicle run is RED because the desktop lost focus, clearing its keyboard
state after46 forward commands. The harness now reacquires the real window and
completes the observed input budget. V149's grenade authority reached a missing
1CD7F0 physical-body velocity/query owner; the complete original body and its
reached dependencies were lowered before V150. Neither failure is suppressed.

The current checkout's production and acceptance targets build successfully.
`live-root-v151` passes all seven selected visible scenarios with protocol66:
local vehicle/lift/objects and UDP vehicle/lift/movement/objects. Each UDP run
retains exact authority comparisons (301 for vehicle,181 for the other cases),
with493/892/805/404 adopted-scene render frames for vehicle/lift/movement/objects,
respectively. Every scenario retains a nonempty, probed X11 window recording.
This current-root evidence includes the production wire fields absent from the
older private pair. Installed-launcher delivery remains a separate open step.

Full TU1.5 physics parity remains incomplete. The next visible fixture throws a
real grenade beside an authored Lockout power module to exercise damage,
physical response and retirement. It currently reaches further original damage
and animation owners; no prop impulse/destruction pass is claimed yet.
