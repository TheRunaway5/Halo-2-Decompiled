# Player team inputs at original TU1.5 biped birth

The cooperative Spectre test exposed source units with team NONE while the
native session selected team0. These were actual source body+138 readbacks;
unit+13C held the correct full player datum and unit+12C was NONE. This was
not an AI binding failure.

Original English TU1.5 player spawn sites151FEC..15202C supply the player datum
to creation request+5C, the sign-extended player+C0 team to request+64, and
player+89 to request+A8 before B9B70. The request base is ESP+34 at152054;
152001 reads the team and15200A writes four bytes at ESP+98. The native player
biped birth now supplies these actual player-owned inputs. Generic biped
creation still uses its original initializer without a player context.

The first composed rerun (`seat-birth-live-v277`) proves that this write alone
is insufficient: the actual player configuration itself already contains
NONE. Complete14E7B0 admits its team only when original engine+0A includes
that team. Native session setup delivered the variant but omitted the actual
session-roster inputs used to build that mask.

Native team joins now publish their real slot and selected team before source
player allocation. Original15A2C0 reads active at options+2DC and team at+374,
with E4 bytes per slot. It computes engine+06; complete15A470/15A1A0 constructs
the mapping and engine+0A before14EBE0/14E9E0 sanitizes the new player's profile.
Existing player configurations remain owned by those original routines. No
completed unit team or engine mask is injected. The initial placement inside
construct_player in `v278` was too late: mask refresh iterates allocated
players, including the unfinished new record. Delivery is moved before
allocation in `v279`/`v280`.

The connected vehicle occupancy module additionally lowers complete154C70
(37 instructions, reached at EDB0F) and1AA2B0 (29 instructions, reached at
830CC during player configuration). The authored palette lookup83050 (39 instructions, reached at8318E) is
also complete. Complete19B180 (25 instructions, reached at15AC6F) retains the original
team notification; its1648E0 callee already has a registered native owner.
Original B7E50 and all other required
callees keep their ownership. The seven complete module bodies total327
instructions. Their generation and call targets are authenticated against
English TU1.5 SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

Evidence is under `analysis/multiplayer/physics-live-tu15-20261012/`:
`player-birth-source-v277/` retains spawn contracts and original routine
listings; `seat-transition-source-v282/` retains complete generated bodies.
The visible composed rerun `roster-notification-live-v282` passes with813
exact observed UDP snapshots. All three actual source unit teams remain0.
The companion enters driver0 normally, delivers300 forward commands and
travels28.907700 units with the passenger aboard. The passenger aims0.6
radians and exits normally. Original camera submissions occur698 times for
the passenger and576 for the driver; both compare position/basis/FOV to the
authority packet. The runtime ELF stays unchanged and the live desktop mirror
is verified continuously through the run. Video, raw bindings and reports
are retained. Team departure/reassignment,
all boarding transitions and full physics parity are not established here.

The native LAN input also now carries the displayed source-selected vehicle
action, full object datum and seat index to original command+28/+2C, instead
of recalculating a potentially different target on the authority. Structural
object generation/type/seat checks precede the original action admission.
Neutral displayed actions are canonicalized because original inactive action
records can retain old object/index bytes. Native protocol78 requires peers
built from the same schema. The actual UDP test covers enter, boarding,
explicit neutral and absent intent, without advancing the match clock.
The actual UDP release/coalescing regression in `release-intent-red-v284`
fails when a later neutral packet replaces the completed hold target.
Protocol78 retains the target from the release frame with the completion
serial and exact saved pickup duration. Delivery consumes that tuple once;
subsequent continuous input retains its own target. `release-intent-focused-v285`
passes all eight checks, including native multiplayer and the release regression.
Rapid multiple completed holds and other interaction classes remain separate
open input paths.
