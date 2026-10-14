# TU1.5 live prop blast integration

The current retained world executes the original grenade, radial damage,
point-impulse, object destruction/retirement and effect update paths. The native
renderer reads completed source bounds, velocity, runtime flags and frozen
state. Protocol69 carries those prop inputs to independent client views.
Cold construction bounds are used only before a source-authoritative frame
exists; a missing live bounds owner remains an error.

English TU1.5 executable SHA-256:
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The fixture declares three starting positions beside the authored Lockout
power module before tick zero, throws one grenade through ordinary SDL input,
and supplies no physics, damage, retirement or camera results.

## Current-root visible evidence

`live-root-prop-v217` completes both181-tick sequences:

| Mode | Source prop movement | Retirement | Exact UDP snapshots | Original impulse comparison |
| --- | --- | --- | --- | --- |
| Local, two views | 0.433688 units | Observed | Not applicable | Two events; all512 motion bytes match unchanged324F80 |
| UDP, three participants/two client views | 0.495380 units | Observed | 181/181;274 adopted scene frames | Two events; all512 motion bytes match unchanged324F80 |

Both views submit actual grenade geometry and explosion particles. The local
recording contains215 frames; the UDP recording contains199. Both execution
receipts verify the same unchanged native ELF:
`85f1287be4c2e55985c372dc8fe8854cc711bb76909d49bd968d677e9a6ba947`.
The native X11 surface is mirrored unaltered to the desktop while test input
is isolated. Reports, impulse records, original comparisons, screenshots and
window videos are retained under
`analysis/multiplayer/physics-live-tu15-20261012/live-root-prop-v217/`.

## Source owners and validation

The read-only impulse hook observes actual1D94CE ->324F80, validates the real
component/body-list/motion owner and saves the actual pre/post256-byte motion
record plus impulse and point. It runs the complete original callee before
publishing the event. `tools/oracle_live_body_impulses_tu15.py` independently
executes unchanged TU1.5 instructions using those captured inputs.

Literal1281A0..1281DC selects the resident sound gestalt; native delivery checks
its full datum, group and resident range. Actual2131B0/213260 construct/reset
the original availability bitmap. These are metadata/storage owners, not a
claim of audio-device activation. The original object sound cleanup117560,
218190,218070 and2180F0 is wired into the same retained bank after the
three-player blast reached that additional path.

The native virtual-memory binding now accepts the authenticated actual
33B3BF/33B3FB call sites as well as their retained correspondence IDs, requiring
the same TU1.5 return frames and real owned allocations. Original pool startup
and host-resource continuation tests pass. The recovered13E6E0 linear packed
color branch compares all GP/SSE outputs, defined flags and empty x87 depth
against unchanged TU1.5 across1160 cases and all four rounding modes.

## Remaining scope

These are bounded live prop movement/destruction and transport checks. They do
not establish all object types, spawned debris presentation, generation
recreation/late join, every model-region transition, complete effect/audio
presentation, all contact/solver behavior, physical Xbox floating-point bits
or complete TU1.5 physics parity. The current packaged launcher checks predate
this prop update; its production delivery still needs verification.
