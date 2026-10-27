# Enhanced netcode goal

Requested 2026-10-25: improve hit detection and physics-object responsiveness
beyond the 2004 flow without a large bandwidth increase. The host chooses the
mode, and the lobby shows it:

- **Original**: the exact 2004 behaviour (still gains its own movement
  prediction, which the 2004 client had).
- **Balanced** (default): lag-compensated hitscan capped at 100 ms, with a
  cover veto, plus the presentation improvements below.
- **Full**: a higher rewind cap with no veto, for low-latency groups.

The current flow and its bandwidth are described in
[NETCODE_BANDWIDTH.md](NETCODE_BANDWIDTH.md).

## Items

| # | Item | Modes | Cost | State |
|---|---|---|---|---|
| 1 | Non-host movement prediction (own biped stepped locally; host adopts motion within TU1.5 limits, as the 2004 client) | All | 17 B/input per view (motion block) | Done (protocol 113), see [item 1](#item-1-non-host-movement-prediction); native B95D0 teleport adoption is its own project |
| 2 | Lag-compensated hitscan: the host rewinds target hitboxes to the shooter's viewed tick, capped at 100 ms (higher latency leads the remainder); cover veto (a target behind solid geometry for more than 60–80 ms cannot be hit by a rewound shot); hitscan precision weapons only (battle rifle, sniper rifle, magnum, carbine); not projectiles or melee | Balanced/Full | 3 B/input | Implemented (protocol 113), see [item 2](#item-2-lag-compensated-hitscan) |
| 3a | Adaptive interpolation buffer sized from measured jitter | Balanced/Full | 0 | Done: `AdaptiveInterpolationClock`, `native_adaptive_interpolation`, see [item 3a](#item-3a-adaptive-interpolation-buffer) |
| 3b | Dead reckoning of physics objects and projectiles between snapshots (not players) | All | 0 | Done: `ReplicaMotion`, `native_replica_motion` |
| 4a | Physics impulse events sent immediately, outside the budget, and applied to the replica's motion at once | All | about 10–20 B/event | Done: about 20 B for a lone event, 14 B in bursts (random contents); `native_lan_impulse_events` |
| 4b | Locally predicted response of loose props to the local player's shots and pushes, blended into the authoritative state | Balanced/Full, only if 4a is not enough | small | |
| 5 | Redundant input: each input packet repeats the two inputs before it | All | 1 B per view and repeat plus changed held fields | Done (protocol 113), see [item 5](#item-5-redundant-input) |
| 6 | Predicted impact sparks and decals for the local player's own shots (never predicted player-hit feedback) | Balanced/Full | 0 | Done for precision weapons: `native_predicted_impacts`, `predicted_impact_tu15`, see [item 6](#item-6-predicted-impact-sparks) |

## Rules

- Original must stay exactly the 2004 behaviour apart from item 1, and items 3b,
  4a and 5 (which only change presentation or loss tolerance, not a result).
- Native code only: no new TU1.5 emulation (`run_actual`).
- Each item comes with a test. Latency-sensitive items are tuned with injected
  delay (the slow-client harness plus about 100–150 ms), because two clients on
  one PC show no difference.

## Item 1: non-host movement prediction

**Model.** The 2004 distributed client (simulation mode 4) stepped its own
biped at once and sent its motion; the host applied it through retail
`A9F70`/`AA260` (TU1.5 `B41D0` classify, `B44C0` apply: `B4DA0` teleport,
`B4F00` velocity nudge) and the client's own unit took host corrections only
above 2.5 wu (TU1.5; retail 3.0). Background, the TU1.5 movement chain and
the setter analysis are in [NETCODE_BANDWIDTH.md](NETCODE_BANDWIDTH.md#non-host-responsiveness).

**Client** ([nonhost_movement_client.hpp](../include/halo2/nonhost_movement_client.hpp),
[nonhost_movement_prediction.hpp](../include/halo2/nonhost_movement_prediction.hpp)).
Each local on-foot, living view runs a native predictor: `Simulation::move_unit`'s
mover (now one shared implementation, `step_native_unit_motion`) plus the
TU1.5 control chain ahead of it (152E80 basis and facing, DF880 dead zone) and
the measured TU1.5 origin height (bipeds rest 0.0099 wu above the native
capsule). Every input tick it predicts the input before sending it, so the
own position answers on the input tick, and attaches the predicted motion.
Every received snapshot names the input the host consumed; the predictor
compares the host's motion with its own prediction of that input and, beyond
the trust distance, restarts from the host's motion and replays the newer
inputs (all inputs sent so far are kept, so a restart never loses the ones in
flight). The shown difference decays by the 2004 object blend (45% per tick;
35% orientation) instead of jumping; errors above 2.5 wu, a teleport serial
or a new unit cut. A host-forced move (see below) is always replayed. The
runtime draws the camera/HUD/shown position from the prediction; seats keep
host motion. `H2_NET_PREDICTION=0` disables it.

**Own body.** The third-person body (seen by the other local views, its
shadow casters and its lighting/coverage sphere) is drawn at the same
predicted origin. Each frame `predicted_body_roots`
([multiplayer_presentation.hpp](../include/halo2/multiplayer_presentation.hpp))
selects every view whose shown position is exactly its prediction, on a
living, unseated unit; `SourceWorldInterpolation::predict` then translates
that body, and every attachment its current bank hangs from it (held
weapons, stuck grenades), by the predicted origin minus the authoritative
origin the same pose sample shows: the endpoint blend at the sample fraction
when the body's bank pairs, else the current endpoint (a lost snapshot or a
cut). The replicated animation pose and facing are kept, so the body sits
where the predicted camera is at any playout blend and through the 45%
correction decay. Seated, dead and unpredicted views, other players, the
host and the wire are unchanged; with no roots sampling is byte-identical.

**Trust 0.05 wu.** The host can only nudge (B44C0 decision 2) until a native
`B95D0` placement exists, which moves the host's biped by at most 0.05 wu/s
toward the report, so a wide trust band lets host and client drift apart.
Measured over real UDP on Lockout at 100 ms: trust 0.2 never corrected and
left 0.29 wu after an impulse; trust 0 replayed every snapshot (largest shown
ground correction 0.052 wu/tick); 0.05 replays about 3% of snapshots, shows
at most 0.0012 wu/tick on the ground and leaves a 0.012 wu rest offset. The
2004 rule (trust and snap 2.5 wu) needs the native teleport.

**Host** ([nonhost_motion_adoption.hpp](../include/halo2/nonhost_motion_adoption.hpp)).
After the step and before each snapshot (`LanHost::broadcast` and candidate
broadcasts) the adopter records forced moves for every remote view: a new
current unit, a completed teleporter (1A6830) serial, a damage event on the
unit, an impulse-journal event on the unit, or a velocity increase no input
can produce. A forced move is stamped at the consumed input + 1 and published
per view; reports predicted from an older baseline are refused as stale. The
report of the consumed input is then decided by the TU1.5 rules (reject past
3.0 wu; teleport past 0.2 wu; else nudge) plus anti-cheat bounds in every
mode (horizontal speed within the authored bound + 0.25 wu/s; a correction
budget of 0.3 wu/s, burst 0.6 wu, less whole mover steps explained by held or
skipped inputs) and written through the native TU1.5 setters
([native_tu15_object_motion.hpp](../include/halo2/native_tu15_object_motion.hpp):
`B44C0` decision 2 with `B4BD0`, `BC200` dead bands, `B4F00` and `B9800`,
byte-checked against the translated route on Lockout). Without a native
placement a teleport decision takes `B41D0`'s own fallback, the nudge. A
deactivated Havok island (unported listener `282870`) is skipped; the host's
own step wakes a biped its input moves.

**Modes.** Original uses the TU1.5 `B41D0`/`B44C0` limits; Balanced and Full
ask for exact adoption (teleport distance 0). Both reach the nudge until the
native teleport exists, so the three modes behave alike today. The client
settings (trust 0.05 wu, snap 2.5 wu) are the same in every mode.

**Wire (protocol 113).** After the displayed snapshot time, one motion block
per view: `u8` flags (bit 0 present, bit 1 grounded, bit 2 crouching), then
when present `u8` baseline age (input sequence minus the acknowledged input
the prediction restarted from; 255 unknown), 3 × i24 position (1/1024 wu) and
3 × i16 velocity (1/256 wu/s): 17 bytes, or one zero byte when absent (the
only absent encoding; reserved bits must be zero). Snapshot bodies add, after
the consumed look, `u8` known + `u32` consumed input sequence per peer and
`u8` + `u32` forced-motion sequence per view (strict markers).

**Trajectory parity** (`original_tu15_nonhost_trajectory`, Lockout, 5821
on-foot ticks, error of the native predictor against the TU1.5 host after
replaying 1 or 5 inputs): median 0.0003 / 0.0042 wu, p90 0.010 / 0.034 wu.
Flat running, idle, small stick input and stopping are exact or nearly so;
the remaining errors are crouch transitions (5-tick p90 0.19 wu), wall
contacts, slopes, steps and ledges, and airborne collisions.

**Test** (`original_tu15_nonhost_prediction_lan`, Lockout TU1.5 host,
LanHost/LanClient through a delaying, lossy loopback proxy): the own position
moves on the input tick while snapshots show it 3–5 ticks later; 100 ms and
133 ms round trips (the second with 10% loss each way). A teleport (21 wu) is
cut and then exact; an impulse (6 wu/s) is reported as forced and converges
within 0.03–0.12 wu standing; the rest error is 0.012–0.014 wu. Shown ground
corrections stay at or below 0.0012 wu/tick without loss; jumps and crouch
transitions show up to 0.07 wu/tick.

**Body test** (`native_nonhost_body_presentation`): the production movement
client against a host three inputs late, with a 0.4 wu correction, a lost
snapshot and a 21 wu teleport, sampled at independent playout and animation
blends. The own body's root equals the predicted camera origin every frame,
its held weapon and a grenade stuck to that weapon move with it (model-order
and independent sampling), its sphere follows; a seated local view, its
vehicle and a remote player are byte-identical, and so is everything with
prediction off.

**Left for later.**
- Native `B95D0` placement (701 functions): enables teleport adoption, exact
  adoption in Balanced/Full and the 2004 own-unit rule in Original.
- Mover gaps: jump/landing, crouch transitions, wall/step/ledge contacts.
- Own body: effects anchored to it (third-person muzzle flashes, attached
  particles) still follow host ticks, and its facing is the host's.
- Vehicles: see [vehicles](#vehicles-phase-1). The host-side `B4C40` is now
  native; seats and the driven vehicle are still not predicted.
- A biped impulse journal: the impulse journal publishes loose objects, and
  `NonHostMotionAdopter::observe` has the hook for biped events.

### Vehicles (phase 1)

Research and a host-side prototype from 2026-10-27. Nothing here is wired into
the runtime, and the input schema is unchanged.

**What the 2004 game did.** Retail `828B0` reported the motion of the
vehicle the player drove: the parent of the player's unit, when that unit is
its driver (vehicle `+248`). The report held position, forward, up, linear
velocity and angular velocity. TU1.5 `865B0` (retail `82980`) adopts that
motion for a biped or vehicle with no parent, when the player controls it
directly or through its `+248` driver. It does this in two steps:

1. `B41D0` mode 2 classifies the report. It rejects a vehicle beyond 9.0 wu
   and teleports one beyond 0.2 wu.
2. `B44C0` decision 2 applies a nudge:
   - the 0.06 linear and 0.01 angular dead bands;
   - then `B4F00`, using the reported forward, up and angular velocity;
   - then `B4C40`;
   - then `B9800`.

The TU1.5 distances are set at `68D5F`: biped 2.5/3.0 wu (`0x4DBD0C`/`10`)
and vehicle 7.5/9.0 wu (`0x4DBD14`/`18`).

**What `B4C40` does.** It updates the vehicle's riders and does nothing for
other objects. `B44C0` passes it the corrected linear velocity. `B4DA0`
passes the corrected velocity and the new position.
- The rider iterator `D2B60`/`D2BD0`:
  - climbs while the object's `+4` bit 26 is set and its parent is a unit;
  - yields the root's unit children;
  - then yields the unit children of each child unit that has bit 26.
- With a position, the vehicle's `+270` gains the old minus the new position.
- With a velocity, each rider's `+270` gains (new velocity − vehicle `+88`) ×
  seconds per tick.
- Every rider gets `+134` bit 28.

Unit `+270` appears to be the previous point from which `D06B6` derives a
rider's velocity. The native port is `native_tu15_vehicle_child_update` and
`native_tu15_vehicle_riders`. `native_tu15_nudge_object` now takes vehicles.
It refuses an unported `B9800` before `B4C40` writes anything.

**Evidence.**
- `original_tu15_native_vehicle_child_update` replays 240 cases. They come from
  running the untouched TU1.5 bytes in Unicorn over declared object trees
  (`tools/oracle_vehicle_child_update_tu15.py`). The cases reach all 270
  instruction sites of `B4C40`/`D2B60`/`D2BD0` and match 7,716 written bytes.
  Six mutations are rejected:
  - flag bit;
  - ride bit;
  - delta operand order;
  - header type instead of data `+AA`;
  - no 5-bit shift mask;
  - a single iterator level.
- `original_tu15_native_vehicle_motion` drives every TU1.5 driver choice on
  prepared MCC Coagulation: Warthog, Ghost, Banshee, Scorpion, Wraith and
  Spectre. There is no Mongoose: TU1.5's vehicle slot 6 is NONE.
  - Each seated tick (2,511 in total) compares the vehicle against the
    translated route: `BC190`/`BC200`, the rider order (4,182 riders) and
    `B4F00`.
  - It also compares three `B4C40` forms against the translated iterator plus
    the listing glue: 70,272 bytes.
  - `B9800`: 109,186 bytes.
  - `B44C0` decision 2 with every reported operand: 143,284 bytes, including
    419 linear and 419 angular dead-band cases.
  - Two `B9800` calls were refused on a deactivated island.

**No native vehicle physics a client could run.** The driven-vehicle chain
is generated code over the host's source bank:
- `C9010`→`C6E90` copies the driver's controls;
- `F2340` is the update (583 sites);
- `F1A50` is the control;
- `20DEA0` is the state builder;
- the class bodies on vehicle `+1F0`: tank `F4230` (Scorpion), wheeled `F44E0`
  (Warthog), hover `F5570` (Ghost, Wraith, Spectre) and flight `F6D60`
  (Banshee), with 178, 248, 1,092 and 1,179 sites;
- `20D9A0` delivers forces, `20E350` handles force and contact (1,452 sites),
  and `F8BD0` casts the ground rays;
- the world solve `1CC8D0`.

Only the rate kernels `20D680`/`20D770` are native. That is about 1.4% of the
roughly 6,900 vehicle-specific sites, and the native world-solve kernels are
bound to the bank. Seat look `18A300` (439 sites) is also generated. A faithful
client predictor therefore needs that port, or a client-side TU1.5 world.

**Prototype predictor.** `NonHostVehiclePredictor`
([nonhost_vehicle_prediction.hpp](../include/halo2/nonhost_vehicle_prediction.hpp))
is the vehicle form of the biped predictor:
- replay from exact authority;
- position and angle trust bands;
- the 2004 blend: 45% for position, and a normalized quaternion blend of 35%
  for orientation (retail `A9B40`);
- a cut beyond 7.5 wu.

The stepper is pluggable, and the only native one is force-free extrapolation
(`step_nonhost_vehicle_extrapolation`). It is tested by
`native_nonhost_vehicle_prediction`.

Measured against the TU1.5 host on the same drives, with k inputs in flight,
as median / p90 / max position error in wu:

| Vehicle | No prediction, k=5 | Extrapolated, k=1 | Extrapolated, k=5 | Extrapolated orientation max, k=5 (rad) |
|---|---|---|---|---|
| Warthog | 0.48 / 0.99 / 1.04 | 0.004 / 0.010 / 0.18 | 0.045 / 0.10 / 0.59 | 0.52 |
| Ghost | 0.57 / 0.88 / 0.96 | 0.007 / 0.015 / 0.024 | 0.057 / 0.11 / 0.16 | 0.44 |
| Banshee | 0.33 / 0.65 / 0.73 | 0.007 / 0.030 / 0.13 | 0.057 / 0.16 / 0.56 | 0.84 |
| Scorpion | 0.28 / 0.40 / 0.42 | 0.004 / 0.009 / 0.031 | 0.031 / 0.083 / 0.23 | 0.08 |
| Wraith | 0.42 / 0.61 / 0.63 | 0.010 / 0.025 / 0.043 | 0.072 / 0.16 / 0.25 | 0.15 |
| Spectre | 0.33 / 0.74 / 0.82 | 0.011 / 0.042 / 0.17 | 0.078 / 0.27 / 0.72 | 0.24 |

Extrapolation removes most of the momentum lag. It cannot answer input, so
its errors come at throttle, steering and braking changes, where prediction
matters. On foot, by comparison, the 5-tick p90 is 0.034 wu. The host must
not adopt extrapolated motion: `B44C0` would replace the vehicle's velocity
with the client's.

**Design (phase 2).**
- *Wire, next protocol.* Motion flags bit 3 marks a driven-vehicle block.
  Bits 1 and 2 must then be zero. The block is the same age, 3 × i24 position
  and 3 × i16 linear velocity, followed by:
  - the orientation as a smallest-three quaternion: 2-bit index and 3 × 15-bit
    components scaled by 1/√2, 6 bytes;
  - the angular velocity: 3 × i16 at 1/1024 rad/s (±32 rad/s), 6 bytes.

  That is 29 bytes per driving view, against 17 on foot. It changes the
  encoder (`multiplayer_network.cpp` `encode_motion`), the Rust decoder
  (`native_requests.rs` `motion`), the bridge and the differential test.
- *Host.* `865B0` admission: the player's unit is the `+248` driver of a vehicle
  with no parent. Then:
  - the `B41D0` vehicle rules: reject beyond 9.0; past 0.2, teleport, falling
    back to the nudge until `B95D0`;
  - native `B44C0` decision 2 with every operand;
  - anti-cheat: a speed bound from the vehicle tag, plus the correction budget.

  Forced motion gets a driven-vehicle impulse. The journal now excludes units.
  It also covers damage, seat changes, the `EF870` flip and an unexplained
  velocity bound for each class.
- *Client.* Predict the driven vehicle only with a TU1.5-faithful stepper,
  qualified by a trajectory oracle like `original_tu15_nonhost_trajectory`.
  Present the vehicle and its riders at the predicted pose.

## Item 2: lag-compensated hitscan

**Mode.** `MultiplayerOptions::netcode_mode` (default Balanced; `H2_NET_MODE`
overrides) is applied once by `MultiplayerPreparedWorld` through
`MultiplayerMatch::configure_netcode`. Original installs nothing. Source
shadows (replay worlds and possible successors) are configured like the host;
joining clients record the mode only. Non-hosts
read the host's choice from `MultiplayerState::netcode_mode` (every protocol
113 snapshot) and, in a System Link lobby, from `SystemLinkState::netcode_mode`.
Names and constants are in [netcode_mode.hpp](../include/halo2/netcode_mode.hpp):
caps 100 ms (Balanced) and 250 ms (Full), veto window 70 ms.

**Input.** Every input carries the displayed snapshot time: the low 16 bits of
the older interpolation endpoint's host tick and the blend toward the next
tick in 1/255 steps (3 bytes; tick 0 with fraction 255 means none, the only
absent encoding). The presentation reports it with `LanClient::set_view`; the
host resolves the full tick against its present tick.

**Where the original resolves a hitscan shot.** A fired barrel (TU1.5
`0x109220`) creates the projectile and calls `0xFF100`, which, for projectiles
whose definition has `proj+BC` bit 5 (travels instantaneously), moves it once
through `FF15C → 0xFDEB0` (retail `f8eb0`) in the same tick. Its path test
`0x101030` (retail `fc030`) calls `collision_test_vector` `0x16C620` (retail
`1697c0`) with flags `2480000F` at `1010AB`, plus two `04800008` side rays for
projectiles with a radius. The hit result then goes to the material response
and damage owners. A remote player's fire is the delivered command record
(`deliver_command`), applied by the player scheduler inside `step_objects`.
The projectile's `+C4` is its owner player and `+140` (the query's excluded
object) the firing unit.

**What the test reads for a biped.** The collidable cluster lists (TU1.5 heads
`[4EABB0]`, entries `[4EABB4]`, 32-byte records: owner, next, type/flags,
sphere center and radius) give the coarse segment-sphere test; the object's
WORLD node bank (`object+114` size, `+116` offset, 52-byte nodes) feeds the
posed collision model; `object+40/+4C` is the sphere for attached children
(held weapons, `+10` first child, `+0C` sibling, `+14` parent).

**Rewind.** `NativeLagCompensation`
([source](../src/gameplay/multiplayer/native_lag_compensation.cpp)) retains,
after each host tick while any remote view exists, every on-foot player's
node banks, `+30..+4F` spheres and `+64` position (biped and attachment tree)
and its cluster-entry sphere. The live world hands each `16C620` call made
from `0x101030` during an `FF15C` move to it. For a precision projectile of a
remote shooter it picks the retained tick nearest the displayed time, capped,
writes those bytes over every other player's body under a write journal,
runs the unchanged original query, and restores every replaced byte from the
journal (counted conflicts and mismatches stay zero in the test). Precision
is tag data: an instantaneous projectile whose weapon's primary trigger is
not spew (fully automatic). On the MCC maps this selects the battle rifle,
magnum, carbine, sniper rifle and beam rifle; the sentinel beam and Spectre
turret stay at host time. Projectiles and melee keep host time.

**Cover veto (Balanced).** A rewound hit stands only if one of the target's
WORLD node positions is visible from the shot origin, by the native structure
query `original_map_vector_contact` (flags `04800005`, BSP and instanced
structure), at present or at a retained tick no older than 70 ms. Otherwise
the query's writes and the replaced state are rolled back and the query runs
at present.

**Replay and migration.** The rewind is ordinary work of the step that fires.
A host step with a rewind owner and at least one remote displayed view
records those views with its controls: execution call `MatchStepViews` (kind
32: the controls, the rewind mode and the 16 optional displayed times; a
numeric network replay capability next to `MatchStep`). Every other step,
and every Original step, keeps the original `MatchStep`. The Match authority
checkpoint carries `netcode_mode` and, when the producer has a rewind owner,
its retained history (`NativeLagCompensationHistory`,
[header](../include/halo2/native_lag_compensation_history.hpp): each frame's
tick and every on-foot body's node banks, spheres, position, cluster-entry
primitive and clusters; at most cap + 2 frames, 5 under Balanced and 9 under
Full). A world importing a cut mirrors the producer: it keeps its owner when
the mode matches, builds a detached one when it differs or is missing (the
replacement is registered by the commit), and removes it for a cut without
one; the history is validated (same mode, window, ordered ticks no newer than
the cut, node-bank extents, slots and cluster indices) and staged before the
commit. A replay world sets the recorded views before each replayed step
(`MatchStep` clears them), so it runs the identical rewind from the same
history: replay-mode migration records rewound ticks like any other tick and
forces no complete capture. Source shadows are configured like the host
(owner from their local mode, then the producer's from the first cut), so a
host promoted from a shadow keeps Balanced/Full rewind with the history it
replayed, extended as it continues.

**Limits.** Cluster membership is not re-linked: a rewound target is found
through the clusters it occupies now (mismatches are counted; 100 ms of
motion rarely leaves a biped's membership). Seated players and vehicles are
not rewound: that needs the vehicle as the retained root (its node bank,
spheres and collidable entries, with every occupant and turret as attached
children), a slot map for several players per vehicle and a shooter seated
in the target vehicle, and a veto over the vehicle's nodes. The structure
query does not see broken glass.

**Test.** `lag_compensation_tu15` (coagulation, MCC cache, TU1.5 XBE) runs
an Original, a Balanced and a Full world, each in its own process, with a
battle-rifle shooter reporting a displayed time 3 or 6 ticks behind. Balanced
and Full hit where the target was 100 ms ago and Original misses; a 200 ms
view is capped at 3 ticks under Balanced (misses the 200 ms position, hits
the cap position) and reaches 6 ticks under Full; a target hidden for three
ticks is vetoed under Balanced and hit under Full, and one exposed inside the
window is hit. After a rewound shot, every bank page matches an Original
world's, apart from 131 bytes (two records at `4ECC40` and three flag bytes)
that also differ between two Original worlds.

## Item 3a: adaptive interpolation buffer

**Where.** Non-hosts, Balanced and Full
([adaptive_interpolation.hpp](../include/halo2/adaptive_interpolation.hpp)).
Original keeps the fixed delay: at a receipt the previous snapshot is shown,
blended to the newest over one tick (the existing formula, unchanged).
Sessions qualifying peer host migration (retained effect history, paced
snapshot candidates) keep it too, because their cut proofs pair the presented
receipt with the client's newest one.

**Clock.** Each accepted snapshot's poll time feeds an arrival line (the 3 s
sliding minimum of arrival time minus tick time), the RFC 3550 inter-arrival
jitter (EWMA, gain 1/16) and the largest lateness behind the line in the same
window. Lateness counts from the tick after the previous newest one, so lost
or stale-dropped ticks count as waiting. The target delay is one tick + 4 ms +
max(3 × jitter, held lateness) − the frame interval (a snapshot polled at a
frame was not needed by the frame before), within [one tick + 4 ms, 250 ms].
An underrun hold also records the delay that would have avoided it, held over
the window. Playback slews toward the target at 1 ± 8 % (time constant
0.5 s), so a delay change never moves the presented time discontinuously.
With no newer snapshot it holds at the newest tick (an underrun) instead of
extrapolating. Errors over 0.5 s (the first snapshot, long stalls) re-place it.

**Runtime.** Accepted states wait in a playout queue. The next one is released
into the unchanged projection (effects, poses, audio, journals) once playback
reaches the current endpoint; the blend is playback's position within the
pair. Inputs report this frame's presented time through `LanClient::set_view`
(previously the previous frame's blend), so Balanced rewinds use what was
drawn. The own look and movement prediction read the newest accepted state,
not the presented one; movement reconciliation runs at receipt, in delivery
order. `H2_NET_STATS=1` adds an `H2 net view:` line every 5 s: current and
target delay, jitter, held lateness, underruns, snaps and buffered states
(Original prints the fixed delay). `H2_NET_INTERP=fixed` keeps the fixed delay
in every mode, for comparisons.

**Lag compensation.** The displayed time keeps its encoding (older endpoint
and 1/255 blend). A jittery link's larger delay counts against the rewind cap
(100 ms Balanced): older views are capped as before. `lag_compensation_tu15`
supplies displayed times directly and is unaffected.

**Test.** `native_adaptive_interpolation` models 30 Hz host ticks, a 40 ms path
plus injected jitter, LanClient's stale-tick drop and 144 Hz and 60 Hz client
frames for 60 s, applying the runtime's release rule and blend to every frame.
Steady delay (144 Hz): clean 37.9 ms (one tick + 4.6 ms), 20 ms uniform jitter
54.5 ms, 60 ms jitter 91.7 ms, recurring 150 ms bursts 185 ms. Underrun frames:
0 %, 0.012 %, 0.081 %, 0.27 % (the first bursts). No presented time steps
backwards or jumps, the rate stays within 1 ± 8 % outside holds and nothing
snaps, including a link that turns jittery for 20 s (delay 37 → 88 → 37 ms).
Original keeps exactly one tick behind the newest snapshot at every receipt
(and shows 13–817 forward jumps per minute under the same profiles).

## Item 5: redundant input

**Wire (protocol 113).** After the motion blocks, `u8` repeat count (0–2,
never below sequence 1), then for each earlier input (sequence − 1, − 2) one
mask byte per view (forward, right, yaw, pitch, flags, zoom; bits 6–7
reserved and zero) followed by each masked value, which must differ bitwise
from the next newer input's. Repeats carry no motion block (the host adopts
only the newest report); press serials, completions and intents travel only
with the newest input.

**Host input policy.** Every authenticated packet queues its newest input and
its repeats when they are newer than the consumed one and not queued yet
(older packets can still fill a gap). The host consumes one input per tick in
sequence order; a backlog above three drops the oldest, so buffering stays at
most two ticks, and an empty queue holds the last input. Press pulses (jump,
fire, grenade, melee...) and completions are delivered with the input they
came with, so they stay aligned with that input's aim. Lag compensation's
displayed time travels with the consumed input (a repeat has none).

**Bandwidth.** An input request was about 143 bytes for one view (114 bytes
plus 29 bytes of LNA2 authentication). Over the LAN test's course it averages
163.9 bytes: the 17-byte motion block, the repeat count and two repeats of one
mask byte plus the changed held fields (+21 bytes, about +5 kbit/s per view at
30 Hz). Running while turning adds up to 17 + 1 + 2 × 9 = 36 bytes; the
largest possible addition (every repeated field changed) is 60 bytes.

**Test.** `native_request_protocol` (Rust/C++ reference differential with motion
and repeats, strictness cases), the Rust `native_requests` tests, and
`native_request_ingress` (rejected motion/repeat encodings change nothing; a
lost input is recovered from the next packet and consumed in order). The LAN
test above drops 10% of datagrams each way: the host skipped no input.

## Item 6: predicted impact sparks

**Where.** Non-hosts, Balanced and Full, the local player's own presses of a
precision weapon ([predicted_impacts.hpp](../include/halo2/predicted_impacts.hpp)):
the lag-compensation classification (instantaneous projectile, latching
primary trigger) with magazine rounds; energy weapons are excluded (heat is
not replicated). Presentation only: no hit, damage or result is predicted,
and nothing on a player.

**Gate.** A press is predicted only when it certainly fires: on foot, single
wielding, idle magazine, no melee/grenade/unit action, enough unconfirmed
rounds, the weapon current for its authored ready time + 0.1 s, no reload,
melee, grenade, switch or pickup input in the last second, and the previous
press, the previous predicted press and the host's last shot all a full
weapon cycle (barrel shots per fire at the barrel rate, plus fire recovery)
+ 0.1 s back. Each authored shot per fire (the battle rifle's three) is due at
the barrel rate; the first on the firing input.

**Trace.** On the frame a round is due it is traced from that frame's camera
(predicted look and position, ordinary unit eye) against the structure BSP
with the cover veto's native query (`original_map_vector_contact`, 04800005).
Other players (capsules) and vehicles on the line of fire cancel it. The
surface's global material and plane come from the collision source identity;
the response row, approach angles and scale are the native terrain branch;
`NativeWorldEffects::impact` spawns the projectile's own material effects
(primary, sweetener, secondary) and its material-contact effect.

**Host copy.** Each predicted round is kept for 1 s. A replicated contact is
the copy, and is not spawned, when it names the same shooter (TU1.5 contacts
in the detonation journal carry player and unit; legacy native bullet impacts
match by weapon, terrain flag and material), falls between 1 tick before and
30 ticks after the press's newest known host tick, lies within 0.35 wu + 3 %
of the shot distance, and selected one of the predicted effects. A host
contact on a player selects that body's effect, so blood, shield and hit
feedback are never suppressed. If the host's round hit a player the predicted
world spark has already shown; that is accepted.

**What the host replicates.** In TU1.5 matches the host's own wall sparks are
created by the world's material-effect path, which is not replicated: on
Coagulation the battle rifle's TU1.5 contact request (`100C1C`) carries no
effect for walls of materials 48, 69, 86 and 103, so no contact reaches the
detonation journal. Non-hosts therefore showed no spark for any bullet on a
wall; the predicted one is the first, and has no host copy to suppress there.
The ledger still covers every replicated contact form: a TU1.5 contact that
does carry an effect, and the native legacy world's bullet impacts.

**Limits.** In TU1.5 matches decals come from the host's replicated decal frame
(the source cache owns decal parts), so only the spark, its lights and its
sounds are predicted there; native legacy worlds also predict the decal part.
Objects other than players and vehicles (scenery, props) are not in the trace:
a round the host stops on one shows a predicted wall spark and the host's own
spark. Particles are born at the next presented tick (at most one tick later),
the clock every world effect uses.

**Test.** `native_predicted_impacts` covers the gate and ledger rules (other
shooters, player-hit effects, tick window, tolerance, expiry, legacy terrain
and material, burst order). `predicted_impact_tu15` runs on the MCC
Coagulation cache. In a TU1.5 host world, each predicted battle-rifle press
spawns its effects (for example `E9BA4AAD`, `EEBA4FAD` on material 48) on the
firing frame, and the host fires exactly the predicted three rounds with the
predicted two-tick spacing. In a native legacy host world, the host's three
terrain impacts lie within 0.07 wu of the predicted ones and are all
suppressed, while the three impacts of an unpredicted press spawn. A TU1.5
trace of the same shot (projectile origin about 0.3 wu ahead of the eye,
aimed at the same point) reaches the wall 0.007 wu from the predicted point.

