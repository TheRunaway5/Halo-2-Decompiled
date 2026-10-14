# Complete original Xbox TU1.5 sandbox parity

Status: **active, incomplete**. Requested on 2026-10-13. This goal covers every
sandbox gap identified in the accompanying chat; completing a component does
not reduce its scope. Existing weapon, physics, damage, world, game-mode and
shader goals supply evidence and implementation, not blanket completion.

## Authority and acceptance

The reference is original Xbox Halo 2 English TU1.5, executable SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`,
with owned original Xbox cache assets. Original retail evidence can establish
unchanged behavior only with verified correspondence to TU1.5. Vista/MCC
gameplay, invented tuning, simplified host behavior and guessed service returns
cannot establish parity. Unknown owners remain explicit.

Each element requires all applicable gates:

1. **Reference:** authenticated original owners, complete relevant branches,
   authored operands and original update/ownership order are identified.
2. **Logic:** native state, requests, random consumption and retained memory
   agree with independently executed unchanged original instructions. Declared
   external boundaries and unexecuted branches remain visible.
3. **Composition:** real inputs reach the original owners in the live match;
   actual contacts, objects, targets, allocations and lifecycle results feed
   downstream consumers without parallel substitute gameplay.
4. **Publication:** local, split-screen and LAN preserve identity, events,
   independent views/hands and lifecycle through loss, duplication, reordering,
   join, death, reset and recreation where applicable.
5. **Presentation:** matched original TU1.5 poses, frames and PCM substantiate
   applicable models, effects, lighting, HUD and sound claims. GPU validation
   and finite native captures alone do not pass this gate.
6. **Delivery:** a coherent build and its actual installed launcher selections
   contain the tested work, with appropriate regressions passing.

No row below is certified complete. Existing successes retain their narrower
scope in the linked reports. The weapon census additionally requires separate
coverage for every definition and its applicable contracts, including AI,
vehicle/turret, objective and unused authored variants.

## Coverage matrix

| Area | Required elements | Current starting point / outstanding work |
| --- | --- | --- |
| Weapon controls and ownership | All triggers/barrels; primary/offhand; charge/hold/release; reload/chamber/interrupt; inventory, pickup/switch/drop; world clocks; aim/error/recoil and random order | [Weapon goal](WEAPONS_TU15_PARITY_GOAL.md). Source SMG fire/manual reload now compose in a headless match; the other weapon, hand, event and presentation contracts remain open. |
| Weapon special behavior | Plasma charge; rocket tracking; Needler acquisition/guidance/stick/fuse/supercombine; sword target selection/lunge/magnetism; AI and mounted weapon owners | [Weapon goal](WEAPONS_TU15_PARITY_GOAL.md), [live combat](TU15_LIVE_COMBAT_EFFECTS_20261013.md). One source-owned distant lunge/movement/contact/damage sequence now passes; full sword coverage remains open. |
| Powerups and abilities | Authored equipment pickup/respawn; overshield activation/decay/damage; camouflage pickup/expiry/disruption; intrinsic suit control; trait and shadow/render consumers | [Camouflage integration](MP_CAMOUFLAGE_NATIVE_BINDING.md), [material integration](MP_CAMOUFLAGE_MATERIAL_NATIVE_BINDING.md). Explicit timer grants are not pickup acceptance. |
| Vehicles | Every authored controller/motion class; wheels/suspension/hover/flight; all seats, boarding, entry/exit; mounted weapons; collisions/impulses/destruction and riders | [Physics goal](PHYSICS_PLAYERS_VEHICLES_OBJECTS_GOAL.md). Warthog/Ghost selected checks pass; Banshee exit and full flight coverage remain open. |
| Projectiles and grenades | Actual weapon creation; trajectories, guidance, contacts/materials; bounce/penetration/attachment; fuse/arming; direct/radial/super/boarding damage; detonation/retirement | [Grenade integration](MP_GRENADE_LIVE_TU15_20261013.md), [projectile/melee effects](PROJECTILE_MELEE_EFFECTS_20261013.md). Live frag acceptance does not complete plasma sticking or every projectile family. |
| Damage and vitality | Original posed node/region admission; shield/body/section transitions; all producer families; attribution/reaction; fatal cleanup, corpses and respawn | [Damage coverage](DAMAGE_SHIELD_COVERAGE_GOAL.md), [live combat](TU15_LIVE_COMBAT_EFFECTS_20261013.md). Preserve source-produced vitality and requests; do not infer full coverage from scalar comparisons. |
| Dynamic world | Props/items, breakage/chained blasts, debris; supports/carriers/conveyors; devices, machinery, doors, lifts/trains, riding/crushing; source lifetimes and replica recreation | [Physics goal](PHYSICS_PLAYERS_VEHICLES_OBJECTS_GOAL.md), [world features](MAP_WORLD_FEATURES.md). Selected lift/gate/script checks do not complete destruction or all maps. |
| Enemy/allied AI | Original perception/awareness/targets; tactical state/orders/cover; paths/steering; weapon/vehicle use; species and movement modes; animation and encounter/difficulty owners | Current native actor loop remains a provisional brain. Recover original owners before replacing it; retain all authored campaign/AI weapon definitions in scope. |
| Objective objects and rules | Flag/ball/bomb pickup/carry/melee/drop/death/return/reset; capture/arming/detonation; role/hill/territory interactions and complete applicable match rules | [Game-mode goal](GAME_MODE_PROGRESS.md). Bounded scoring and object helpers do not establish full normal matches. |
| Animation and feedback | Both species and hands; unit/own-model arbitration; current/saved poses, overlays/blends/IK/root motion; firing/reload/melee/lunge/hit/death and attacker feedback | [Weapon goal](WEAPONS_TU15_PARITY_GOAL.md), [live combat](TU15_LIVE_COMBAT_EFFECTS_20261013.md). Finite pose checks retain their limits. |
| Combat rendering and audio | FP/world muzzles, beams/forces/casings, contrails; impacts/blood; blade/lens attachments; color/light receivers; HUD/scopes/heat/battery; effects and PCM lifetimes | [Live combat](TU15_LIVE_COMBAT_EFFECTS_20261013.md), [shader goal](SHADER_PARITY_COMPLETION_GOAL.md). Full Xbox frame/PCM comparisons remain required. |
| Composed acceptance and delivery | Authored campaign and multiplayer contexts, normal runtime, split-screen and LAN; coherent tested snapshots and installed launchers | Required for every applicable row above. Preserve dirty/concurrent work and record exact source/runtime/reference hashes. |

## Work record

- Initial audit refreshed newer October 13 weapon, vehicle, grenade, combat,
  world and game-mode reports before choosing the first implementation seam.
- First investigation: original weapon targeting and delivered command
  ownership, including the deliberately failing distant-sword acceptance path.
  No target/confidence or source body state will be assigned to force success.

### First accepted implementation checkpoint

`source_sandbox_special_tu15` contains **37 complete authenticated original
bodies**, including lunge movement, contact/history, recording, Slayer
notifications, sound-status/metadata, bit-bank construction and physical
action completion. Reached collision collector bodies also use the shared
combat module. Generators retain exact original listings and authenticate
direct call sites and literal callback slots. No supplied success return,
forced target/confidence, source-body position write or replacement lunge
motion is used.

The unchanged-original movement oracle passes **336** finite raw-ABI cases.
Native comparison checks complete executed instruction sequences, GP/SSE
registers, defined flags and retained memory, including indirect constants.
This is bounded evidence for four movement bodies and their actual callees,
not all 37 bodies' branch coverage, a full original world replay or Xbox pixels.

`lunge-coherent-motion-v1` now **passes all 180 source-owned match ticks**.
Actual delivered movement establishes target distance `2.40331`, with original
targeting confidence `1`. Source activities `0A0005BB` and `0B0005B2` produce
**1.90626 units of attacker movement**, versus zero measured idle movement,
and close the target distance to `0.496893`. The source chooses impact counter
`3`, publishes the authored shield-depleted request `E95507D0` and material-56
contact at tick `89`, applies damage (minimum retained vitality `-0.777778`),
and completes its action. The strengthened fixture requires real closing
movement as well as special activity, changing poses, contact and damage.

The original sound bit-bank constructor/reset now run with real host storage.
The narrowly authenticated raw allocation keeps original arena/CRC/pointer
stores; other XAPI callers retain their existing platform owner. A separately
supplied native resident-cache binding selects the actual current-map `ugh!`
using original `1281A0..1281DC` operands and validates its salt/group/payload.
It is immutable resource delivery, not an invented voice status or execution
claim for the rest of `1281A0`. The complete sound bit-bank consumer executes
against original-produced state. Audio device/voice/DSP activation and full
original map/audio scheduling remain open. Earlier NULL-ugh failures are
retained as historical evidence, not the current lunge result.

The coherent full native runtime and seven focused executables build in
`build-sandbox-tu15-20261013-v1`, from the isolated
`.local/sandbox-tu15-source-20261013-v1` source. Six focused CTests pass with
no skips: special movement, source world/startup (including original sound
storage), combat action, bitmap caller, particle colors and grenade gate.
The same build passes ordinary close sword damage and frag detonation/count
debit plus its real loopback UDP location/color/control round trip. These are
headless Linux integration checks, not rendered split-screen/LAN lunge,
audible hardware, matched Xbox framebuffer/PCM or full sword parity.

The original-weapon primary-input regression remains **RED** in that coherent
build: the actual source SMG stays at `60` rounds and records `0` trigger frames
over twelve delivered fire ticks. Command `+14/+18` remain neutral; original
`153070` and `C92C0` carry them to unit `+1C0/+1C4`. Next work must compose
original weapon input, state/publication and event ownership while retiring
parallel native firing. Merely adding trigger stores would create duplicate
shots and does not satisfy this goal. See `primary-coherent-baseline.log`.

Evidence: `analysis/sandbox-tu15-goal-20261013/`. `coverage.json` retains all
63 weapon definitions plus 39 shared requirements, with six gates per element.
No whole element is certified complete. Early diagnostic overlays using old
archives are retained separately; shared header drift made a coherent rebuild
necessary, and accepted results above come from that fresh build. Installed
launcher delivery and every broader acceptance gate remain required.

### Source primary input and manual reload checkpoint, 2026-10-14

The ordinary SMG regression is now **green** in a second coherent build.
The native delivered-command boundary supplies original held-fire flags and
both trigger pressures. Authenticated `1889D0` produces the held flags at
`1895B7/1895CA/1895D2`; `153070 -> C92C0 -> C7DD0 -> 105500` retains the
pressures and weapon input. Manual reload uses bit28, consumed by original
`C7F7A` through action0/action10. Source movement plus multiplayer weapon
ownership now bypasses `Simulation::update_weapon` before any native refill,
ready clock, firing, ammunition, damage or projectile mutation. Other native
simulation contexts keep their existing owner.

Inventory publication reads the original two52-byte barrels, two12-byte
triggers and two16-byte magazines, plus ready/heat/battery/own-state fields.
The authored primary trigger/barrel selects the displayed magazine. No round
count or reload phase is assigned back into source storage. The acceptance
checks loaded and reserve publication against the actual source bytes on
every live tick, and requires the parallel native shot counter to stay zero.
This does not complete all shared-reserve HUD queries or event publication.

`source_sandbox_special_tu15` now contains **89 complete original bodies**
and **9,749 original instructions**. Its52 added bodies retain complete
barrel admission/rate, prepared shot, attribution, unit notification, battery,
firing stimulus, error, projectile postpass/aim/placement/contact and reload
consumers. Direct call sites and complete original bytes are authenticated by
the pinned TU1.5 executable. Reached descendants still require real owners;
none receive a fabricated successful return. The original AI admission gate
remains its current constructed value; these firing checks do not establish
the unfinished tactical AI subsystem.

The new raw-ABI oracle passes **294 original/native cases**, comparing exact
instruction paths, GP/SSE registers, defined flags and retained memory for
the complete rate owner and six denied admission paths. Its unparented ABI
inputs are explicit fixtures; positive world admission is exercised by the
native match, not independently replayed as an entire original world. The
previous336 movement comparisons also pass. These630 cases do not cover
every branch in all89 compiled bodies.

The normal120-tick SMG match retains12 actual unit trigger-pressure frames
and consumes source rounds `60 -> 53`. The235-tick manual reload match then
observes51 non-idle original magazine frames, refills to60 and spends seven
actual reserve rounds. The coherent build passes **nine focused CTests with
no skips**, including both live matches and both original/native oracles.
The180-tick lunge regression still moves1.90626 units, closes to0.496893 and
applies source damage; frag detonation/count debit and the existing loopback
UDP effect-publication acceptance also still pass.

Source: `.local/sandbox-tu15-source-20261014-v2`. Build:
`build-sandbox-tu15-20261014-v2`. Evidence and exact hashes:
`analysis/sandbox-tu15-goal-20261013/accepted-checkpoint-v2.json`.
The v1 source/results remain preserved. A failed regeneration that imported
an unrelated new test into the frozen CMake file is retained as a diagnostic;
the final accepted build includes only tests whose sources are present.

The next required work is original weapon event/first-person publication,
semi-automatic press/release and dual-hand ownership, then the remaining
weapon-specific contracts. Native `last_shot`/reload/effect presentation cannot
be inferred from end-of-tick ammo differences. Matched Xbox frames/PCM,
split-screen/LAN weapon composition and installed launcher delivery remain
unverified. No weapon or shared sandbox element is certified complete.

### Original first-person ownership checkpoint, 2026-10-14

The source-local view was previously absent: player `+28` and the actual
`4F54F0+0C` map remained NONE after native `configure_local_views`. Native
view selection now calls complete original `151C90`, which owns association,
release, input registration and `16B16A` unit transition. Original `168CA1`
now runs at its authenticated `13AB80` phase, after effects and before the
source tick increment. It observes actual local units/hands and advances
complete `169C46`. Native source-owned views and replicated clock endpoints
read the actual original registration, channel, event/delay, node-map and
clock fields. The parallel native FP activity/clock owner is bypassed.

A clock interval belongs to one source unit/hand registration. A birth,
replacement or graph change rebases its interpolation origin to the first
actual new source sample; the previous identity cannot supply an endpoint.
Unbound remote source players have no fabricated local FP clock. This is
explicit snapshot availability, not completion of remote prediction/events.

Three missing original storage owners now run their actual constructors and
map resets: input/view action cache `1EECF0/1EED30`, feedback request bank
`2293E0/229420`, and local effect/impulse bank `1563A0/1563E0`. Literal startup
callback records authenticate each pair. Complete input-history and both
FP animation-event callback bodies are compiled. No source state, animation
result, sound handle, target or platform-success result is supplied to force
these paths. The current explicit native English locale is a platform input;
original language conversion and localized string lookup/formatting retain
their own owners. The frozen map-support module incorporates the shared
checkout's authenticated message descendants for early local-player birth.

The special module now contains **110 complete original bodies /11,428
instructions**. Map-support contains **171 complete bodies /13,889
instructions**. These are compiled body counts, not unique newly recovered
owners across all modules or branch-coverage totals.

Both late view binding and binding before original player birth pass the
235-tick SMG fire/reload cycle. Each requires original pressure input, rounds
`60 ->53 ->60`, seven reserve rounds spent,51 original reload frames and
zero parallel native shots. Actual original FP channels change154 times.
Every live tick checks published registration, channel/event/delay state,
node counts and all256 node-map entries against source bytes. Source clock
origins are independently checked against the previous stable sample or the
first actual sample of the new identity. Original release/rebind and an
actual UDP host/client round trip also pass, including a joined unbound
remote player with a real unit and no local FP-clock advertisement.

Three real composed original FP phases (firing tick84, reload tick130, idle
tick220) have independent unchanged-XBE execution. Unicorn matches all
60,084,310 committed bytes, instruction paths, GP/SSE registers and defined
flags in each phase. Its MXCSR precision flag differs and remains reported.
An independent Linux i386 hardware single-step replay matches all **5,359
original instructions**, retained read/write bytes, GP/SSE registers,
defined flags, x87 state and the complete MXCSR including precision status.
No original code is patched, no gameplay callee return is supplied, no
expected native output feeds original execution, and unretained data reads
are rejected. This is host i386 evidence over actual native-composed entry
banks, not Xbox hardware, a whole original frame, original Xbox boot, pixels
or PCM. The read-only phase diagnostic is inactive in ordinary execution.

**Critical remaining gate:** the original camera director callback in
`4F5514+0C` is still zero. `158290` therefore returns its original non-first-
person mode and `16B1BC` disables hand bit0. These three phases execute the
real FP manager/clock but do **not** execute `16A4DD` or the FP sound-event
callbacks. Merely compiling those bodies does not establish pose/event or
audio parity. The next source seam is original camera/director binding and
update, followed by original pose/event publication into rendering/audio.
Do not assign the director callback or force the mode to bypass this owner.

A coherent full native build and **eleven focused CTests pass with no skips**.
The distant sword/lunge, close sword and frag regressions also pass: lunge
moves1.90626 units, reaches0.496893 separation and source vitality-0.777778;
frag detonates at140 with two actual effect locations and retained count
debit/UDP publication. Source: `.local/sandbox-tu15-source-20261014-v3`;
build: `build-sandbox-tu15-20261014-v3`. Exact source, executable, reference,
map and evidence hashes are in
`analysis/sandbox-tu15-goal-20261013/accepted-checkpoint-v3.json`.
The v1/v2 accepted sources/results remain preserved. Neither installed
launcher selection nor matched Xbox frames/PCM is verified. All102 whole
sandbox elements remain incomplete; species/hands, split-screen/remote
prediction, all other weapons/vehicles, AI, world and rule gates remain open.

### Original positive first-person pose checkpoint, 2026-10-14

The v3 camera gate is now resolved through its original lifecycle. Literal
callback record `44B3BC` identifies constructor `157E10` and new-map reset
`157EB0`. Both complete bodies run on the retained arena/CRC/storage. The
reset owns the original default `245400` director callback, and `158290 /
16B1BC` now admit the actual first-person pose. No director callback, camera
mode, hand bit or pose count is assigned to bypass these routines. Full
per-frame director update, look input, other camera modes and transitions
remain separate acceptance work.

The positive path recovered complete packed-node count/output, ammunition,
root matrix and hierarchy consumers, plus the literal `48C36C` codec
callback `296220`. Normal source firing then reached the original FP
muzzle/node lookup `1692AB ->1E1360`, including its complete map consumers.
The special module contains **124 complete original bodies /11,968
instructions**. These counts describe compiled code, not full branch coverage.

The normal235-tick cycle passes with the original hand pose gate and node
count required on every live FP tick. The independent captures now positively
execute `16A4DD`: firing tick84, reload tick130 and idle tick220. Unchanged
original i386 execution matches **52,066 reported instruction events**
(**51,937 CPU single steps**, with REP trace conventions), GP/SSE registers,
defined flags, x87 controls/status/tags, MXCSR, retained source matrices and
all source writes. It independently observes and matches152/156/224 scalar
lanes of packed reciprocal-sqrt estimates. Touched input extents contain
188,416/188,416/200,704 bytes; all remaining bytes in each60,084,310-byte
committed bank are checked unchanged. Original instructions are unpatched,
unretained reads are rejected, and no expected native output or gameplay
return is fed to hardware execution.

Unicorn has small floating-point/value and status differences in these
larger pose phases. They remain fully visible in the report. Acceptance uses
the independent exact i386 hardware comparison, which checks the entire
state and CPU inputs. The dedicated packed-estimate hardware harness leaves
the earlier pinned frame/hardware oracle files unchanged. This establishes
three bounded host-CPU pose phases, not Xbox hardware estimate bits, original
whole-game boot/frame ordering, GPU pixels or PCM.

The coherent full native build and eleven focused CTests pass without skips.
The tests now require the original positive pose gate; disabling it cannot
pass. The distant lunge, close sword and frag/UDP regressions also pass with
the same measured source behavior as v3. Source:
`.local/sandbox-tu15-source-20261014-v4`; build:
`build-sandbox-tu15-20261014-v4`; exact hashes and boundaries:
`analysis/sandbox-tu15-goal-20261013/accepted-checkpoint-v4.json`.
All three earlier accepted sources, executables and reports remain intact.

**Next required work:** publish actual original current/saved FP poses and
original event admissions into the renderer/audio pipeline. The renderer
still evaluates decoded clips and native IK using the published source
clock. The three selected positive pose phases do not execute FP sound
callbacks; their compiled presence and successful whole cycle do not prove
actual sound-event playback. Both hands/species, varying look/camera modes,
all weapon contracts, split-screen and full LAN behavior, original Xbox
frame/PCM comparisons and installed launcher delivery remain open. All102
whole sandbox elements remain incomplete.

### Original FP render-record handoff checkpoint, 2026-10-14

The local source-owned foreground and FP marker consumers now use completed
original draw matrices. Native submits its actual view/unit and camera at the
authenticated `3D35A ->168DED` request boundary (four caller records). Full
`168DED`, `168D76`, `16B240` and `16B4E8` retain original camera-control
composition, model-node remapping, combined arms ownership and marker IK.
Three complete direct helpers were also recovered. The special module now
contains **131 complete original bodies /12,509 instructions**.

Native camera inputs follow original `2BA58` and `2BAA6..2BAAB` publication.
The output is caller scratch in the existing retained native ABI bank; no
persistent allocation, model identity, node matrix, IK result or source
success return is invented. The adapter reads actual model/owner/mode and
model-order node matrices. The original director mode remains a real gate.
This is a native request to the complete original owner, not execution proof
for the enclosing original rasterizer startup or whole view scheduler.

The renderer skins its actual resident arms/weapon meshes from those matrices.
Source-owned local paths bypass independent decoded animation, ammunition
pose overlay, fractional FP clock sampling and native hand IK. Dual arms are
one original combined draw bank; the old native triangle split remains only
in the other presentation path. FP muzzle markers consume the same original
weapon-node owner. Source game clocks/events are not advanced by rendering.
Remote/client prediction and captured-reference diagnostic paths retain their
separate owners and remain outside this local acceptance.

The235-tick headless fire/reload cycle now requests original render records on
every actual live local frame. It requires exactly the authored37-node arms
and4-node SMG, source unit/weapon identities and mode1, and skins the real
meshes directly from the returned matrices. Existing trigger/ammunition,
reload, clock-origin, release/rebind and UDP checks continue to pass.

Independent unchanged-XBE i386 replay now covers both composed phase families:
**52,066** clock/pose instruction events and **19,395** render-preparation
instruction events, across firing/reload/idle. Every case compares GP/SSE,
defined flags, x87/MXCSR, observed CPU estimates, exact touched bytes and
unchanged remaining bytes of the60,084,310-byte committed source bank. The
render captures positively execute original `1E1390` hand attachment at84
and220; the actual reload130 gate skips it. No original code is patched,
no native expected output feeds original execution, and unretained reads
are rejected. Unicorn differences remain reported separately; exact host
hardware is the acceptance backend. Xbox CPU estimates and full frame/PCM
comparisons are still not proved.

The coherent full native build and **twelve focused CTests pass with no skips**.
The real hidden Vulkan runtime additionally renders four640x480 frames on
**NVIDIA GeForce RTX5080**. Its foreground observer requires every weapon
node bit to equal the actual original draw-record bank, nonempty skinned
geometry and no second FP interpolation owner; all four draws pass. The
captured PPM is visually inspected, with a lossless PNG preview verified at
zero changed pixels. Arms/weapon are present without an obvious rig or
placement break. This is native GPU delivery evidence, not Xbox visual parity.
The native capture still reports dynamic object-light/lens-flare and particle
external-texture boundaries. The distant lunge, close sword and frag/UDP
regressions retain their previous measured source results.

Source: `.local/sandbox-tu15-source-20261014-v5`; build:
`build-sandbox-tu15-20261014-v5`; exact source, executable, reference, map and
evidence hashes: `analysis/sandbox-tu15-goal-20261013/accepted-checkpoint-v5.json`.
The previous four accepted sources, executables and evidence remain intact.

Next work is original FP event/audio admission and native PCM delivery, plus
full camera/view scheduling, varying input, both hands/species, inventory and
all remaining weapon contracts. Actual source draw records also require full
split-screen/client/LAN validation and matched original Xbox TU1.5 frames.
The wider power, vehicle, projectile, world, AI, objective and combat feedback
scope remains unchanged. All102 whole sandbox elements remain incomplete;
installed launcher delivery and the complete stated goal remain unverified.

### Original sound request and cold-admission checkpoint, 2026-10-14

The actual local first-person cycle now has a read-only observation boundary
around the complete `18CEFC ->223B20` sound submit. It preserves the caller's
160-byte request, source sound datum, full incoming/outgoing ABI, instruction
trace and actual result. The complete original routine still decides admission.
No source availability flag, voice identity, selected sound or platform-success
return is supplied. The observer adds no serialized state. The source-owned
local renderer no longer registers the separate native FP clock/event callback;
the original clock/event owner remains the producer.

The235-tick ordinary SMG fire/reload cycle reaches **18 actual submissions**.
One is the real `169C21 ->169BA2 ->18BE50` first-person reload callback at126;
the others are actual source calls outside that FP callback phase. All18 pass
through complete `223B20`. Their original result is `FFFFFFFF`: the authored
`snd!+0D` custom-playback index is `FF`, and the ordinary fallback branches at
`223C6E` because the actual system availability bytes at`+78..+7A` are
`00 01 00`. The driver pointer remains0. No ordinary `128B10` voice creation is
reached. This is a measured cold-driver rejection, not successful playback.
Original marker/world-position consumers execute in the reload phase; an
assumed missing first-person marker registry is not the cause of this rejection.

Independent unchanged-XBE i386 replay covers **25 cases /93,215 instruction
events**:18 submits (1,638), the positive reload event phase (20,116), three
clock/pose phases (52,066) and three render-preparation phases (19,395). Each
case compares exact GP/SSE state, defined flags, x87/MXCSR, source instruction
order, declared actual CPU estimates, touched bytes and every unchanged
remaining byte in the60,084,310-byte committed bank. No original code is patched
and native expected output does not feed original execution. Unicorn float/status
differences remain retained as diagnostics; exact i386 hardware is the acceptance
backend. This is host CPU evidence, not Xbox hardware/PCM evidence.

The accepted native source remains **131 complete special bodies /12,509
instructions**, with171 map-support bodies /13,889 instructions. The coherent
frozen build uses its own matching generator/header pair; the first attempted
build with the concurrently changed root generator is retained as a failed
diagnostic. It was corrected without editing an earlier accepted checkpoint or
copying an unrelated ABI change into this source.

The full native build, **thirteen focused CTests with no skips**, distant-lunge,
close-sword and frag/UDP regressions pass. Four real640x480 Vulkan frames on
NVIDIA GeForce RTX5080 still require every foreground weapon matrix bit to
match the original draw-record bank, nonempty geometry and no second FP
interpolation owner. The PPM is visually inspected; PNG conversion changes
zero pixels. The capture is muted and establishes no PCM delivery. Dynamic
object-light/lens-flare and particle external-texture boundaries remain reported.

Source:`.local/sandbox-tu15-source-20261014-v6`; build:
`build-sandbox-tu15-20261014-v6`; exact source/executable/reference/capture and
evidence hashes:`analysis/sandbox-tu15-goal-20261013/accepted-checkpoint-v6.json`.
All five earlier accepted sources, executables and evidence remain intact.

Next required audio work is complete original startup/map ownership with real
native device, saved settings, DSP residency/download, listener, stream/static
buffer and routing-plugin services, followed by positive admission, cache/voice/
completion lifetimes and native PCM delivery. Existing separate SDL playback
cannot be counted as this original sound owner. `224EC0` must not be replaced
with a chosen success or a direct write to system`+78`/`+7A`. Full camera/view
scheduling, both hands/species, all weapons/inventory, local/split/LAN and
matched Xbox TU1.5 frames/PCM remain open. All102 whole sandbox elements and
verified installed delivery remain incomplete; the stated goal remains active.

### Original sound-startup CPU ownership checkpoint, 2026-10-14

`source_sound_startup_tu15_generated.cpp` now compiles **seven complete original
TU1.5 bodies /934 instructions** and registers them with the same retained
world ABI. These are driver startup`224EC0` (420), cleanup`2254F0` (74), queue
allocation`12EB50` (24), environment construction`1972C0` (94), effect-buffer
mapping`1973E0` (63), mapped-buffer construction`2275A0` (167) and feedback
construction`227EA0` (92). All instructions and call sites come from the pinned
owned TU1.5 image; original`128050` is the driver call site. Complete CFGs are
compiled into native C++ with the frozen source's matching generator/header
pair. No original opcode interpreter, successful device result, ready flag or
recorded I/O playback is installed in production. External services remain
full-ABI requirements.

A private cold boot of the authenticated, media-unpatched TU1.5 reference disc
now observes the **whole original environment constructor and effect mapping**
returning normally. Hardware breakpoints retain actual registers, source banks
and16 real I/O operations: section load/free, DSP-image download, nine effect-data
reads, mapped-buffer create, volume, mix bins and play. No guest instruction,
memory, register or callee return is supplied. The real environment return has
AL1; the downloaded descriptor and source effect inputs come from that run.
This is original Xbox code in xemu0.8.136, not physical Xbox DSP/PCM evidence.

Independent unpatched i386 execution covers **18 CPU intervals /391 instruction
events** between those actual external calls. Native CPU composition compares
every retained byte against the host-original computed buffers, all GP/SSE and
logical extended x87 values, status, defined controls/flags and instruction order.
Its test-only external inputs are the genuinely observed I/O completions and
explicitly retained private stack residue. Two missing-provider cases stop at
the exact original first I/O request without returning a chosen false/success.
The production engine has no such fixture-return provider.

The distinction between CPU and platform state is explicit. The raw xemu x87
control/tag encodings differ from the host in36 reported fields; all logical
values, occupancy and defined control bits match. Across the intervals,1,229
bytes of unwritten/dead private stack residue differ between the actual guest
stops and pure host CPU execution. Their producer is not reconstructed here.
Complete raw buffers and differences remain retained. Persistent source banks,
source-written live stack, caller stack and meaningful ABI values agree with
the genuine run. These observations are not a claim of full guest-stack/raw-FP
encoding equality or native kernel/DSP execution. Native CPU bank comparisons
against unchanged host-original execution apply no byte masks.

The fresh full native build and **fourteen focused CTests pass with no skips**.
The25 original fire/reload/pose/render i386 cases from the prior scope are
repeated against this build and pass (93,215 instruction events; every committed
60,084,310-byte bank accounted per case). The18 cold source submits still return
NONE through actual availability`00 01 00`, with driver pointer0. No native audio
activation is claimed. Distant lunge, close sword and frag/UDP results remain
unchanged. Four real640x480 RTX5080 Vulkan frames again consume exact original
weapon matrices; the visually inspected muted PPM and lossless PNG preview have
zero changed pixels. Existing rendering boundaries remain reported.

Source:`.local/sandbox-tu15-source-20261014-v7`; build:
`build-sandbox-tu15-20261014-v7`; exact source/executable/reference/capture and
evidence hashes:`analysis/sandbox-tu15-goal-20261013/accepted-checkpoint-v7.json`.
The six earlier accepted sources, executables and evidence remain intact.

Next required audio work is genuine native section residency/lifetime, device
creation/capabilities/saved settings, DSP download/effect-data application,
stream/static-buffer and plugin services, followed by complete startup/map
activation, positive admission, cache/voice/completion lifetimes and PCM delivery.
The newly compiled CPU owners preserve those real dependencies; they do not
replace them. All weapons/powers/vehicles/projectiles/world/AI/objectives and
full camera/view, local/split/LAN, original frame/PCM and installed-delivery gates
remain in scope. All102 whole sandbox elements remain incomplete and the full
goal stays active.

### Native preloaded DSP resource ownership checkpoint, 2026-10-14

The live world now supplies a real native resource owner at the reached
`2DF851`/`2DF86C` DSPImage boundaries. `SourceRuntimeDspSection` authenticates the
whole owned TU1.5 executable with SHA-256, reads its actual section metadata and
payload, requires the declared preloaded/read-only layout, and validates that
the admitted source bank contains those exact immutable bytes. The additional
request references are real C++ shared ownership of that resident resource.
The returned source alias comes from its authenticated mapped address; it is
not a selected voice, availability flag or supplied successful device result.
Only the reached paired DSPImage domain is supported. Unknown requests and
unmatched unloads stop instead of producing an invented success/failure.

A new private cold boot of unchanged TU1.5 observes actual DSP load/free calls.
The whole guest `.text` is checked against the owned executable. The original
load changes its section reference count **1 ->2** and returns`58D9E0`; free
changes **2 ->1** and returns1. The original resident bytes match the owned
payload before/after. Hardware breakpoints perform no guest code, memory,
register or return modification. These are original Xbox-in-xemu observations;
physical Xbox PCM and full kernel VM/shared-page behavior are not claimed.

The native owner matches that paired reference/lifetime and byte contract. It
preserves preexisting references during nested failed calls, rejects an
unmatched unload, verifies immutable bindings after restore and rejects
outstanding requests at authority export/prepare/commit. Execution scopes drop
new native references when an unsupported downstream owner throws. There is
no extra serialized mutable section state: supported quiescent authority
checkpoints require zero outstanding request references. The baseline immutable
resource is reconstructed locally from the pinned executable.

The native section adapter declares its own platform ABI: genuine EAX result
and stdcall consumption, with remaining ABI values preserved. It does not
claim original kernel volatile clobbers, last-error state, guest XBE-header
counter writes or shared-page VM behavior. The literal header remains an
immutable native input. This is a native resource boundary, not an emulator
implementation of the Xbox section syscalls.

Original`1972C0` now reaches its real`38594D` download request using the genuine
native retained resource, replacing the earlier fixture-positive section result
in this diagnostic composition. The DSP provider remains required and missing;
it throws at that exact boundary, and the actual native reference returns to1.
No source device, downloaded DSP result, initialized flag or PCM is supplied.
Two independent unpatched i386 CPU intervals cover **104 instructions**, all
**5,230,544 retained bytes per interval**, GP/SSE/logical extended x87 state,
status, defined controls/flags and instruction order, with **no byte masks** or
fixture callee returns. Raw physical x87-tag classification differs from the
logical ABI and remains reported; no raw-tag encoding parity is claimed.

The coherent full native build and **sixteen focused CTests pass with no skips**,
including the original18 startup CPU intervals, native DSP resource/prefix checks
and world continuation. The25 sound/FP/render original i386 comparisons repeat
against this build and pass (93,215 events and every60,084,310-byte committed
bank accounted per case). All18 ordinary source submits remain honestly cold:
NONE, availability`00 01 00`, driver pointer0. Distant lunge, close sword and
frag/UDP results remain unchanged. Four real muted640x480 RTX5080 Vulkan frames
again consume the exact original weapon matrices; their visually inspected PPM
and lossless PNG differ by zero pixels. Existing rendering boundaries remain.

Source:`.local/sandbox-tu15-source-20261014-v8`; build:
`build-sandbox-tu15-20261014-v8`; complete source/executable/reference/capture
and evidence hashes:`analysis/sandbox-tu15-goal-20261013/accepted-checkpoint-v8.json`.
All seven earlier accepted sources, executables and evidence remain intact.

Next audio work is actual native device/capability/saved-policy ownership,
DSP download/execution/effect-data application, stream/static-buffer and plugin
services, then full startup/map activation, positive admission, cache/voice/
completion lifetimes and PCM delivery. The real section reference is one
completed native dependency, not successful audio startup. Every other stated
weapon/power/vehicle/projectile/world/AI/objective, full view/local/split/LAN,
matched Xbox frame/PCM and installed-delivery requirement stays in scope.
All102 whole sandbox elements remain incomplete; the goal remains active.

### Native output-resource and explicit native-trace checkpoint, 2026-10-14

`SourceRuntimeSoundOutput` now supplies the reached native output creation
service at original `224F26 ->387877`. The original `224EC0` body still owns
arena allocation, alignment, checksum, zeroing and driver publication. Only
when its actual null-GUID/null-outer creation request is reached does the native
owner open an SDL device. It publishes the actual SDL device ID into the
original output slot after successful creation and returns the native service's
success result. The identity is opaque native state for native service adapters;
it is not an Xbox COM object, vtable, kernel handle or emulated Xbox SDK device.
Original SDK singleton/AddRef, device/voice construction and capabilities are
not inferred from an SDL device. Unsupported/repeated requests stop.

The service declares native platform ABI preservation outside the genuine EAX
result and stdcall stack consumption; original SDK/kernel volatile clobbers are
not asserted. Production reuses the existing native callback policy of512 frames
at48kHz stereo float. This policy is not proof of Xbox scheduling, MCPX routing,
HRTF, DSP execution or physical speaker consumption. Normal world construction
remains lazy and does not activate the source sound driver.

A fresh private cold boot of unchanged OG Xbox TU1.5 captures the original
`224EC0` entry state, actual creation request/return and actual capability request/
return. Whole `.text` bytes match the pinned owned executable. Original execution
naturally allocates/clears the driver, calls the real device constructor and then
requests capabilities. No guest instruction, memory, register or callee return
is supplied. The actual original capability data is retained as reference only;
it is never copied into a native response.

From the genuine original entry bank/ABI, the compiled native body now allocates
and clears the driver and opens a real **PipeWire** SDL output. The actual SDL
callback reports playing. Source execution then reaches required **38568E**
with that real native device identity and the original capability output slot.
The missing capability provider throws; no capability data, ready flag, source
voice or PCM result is produced. Exception cleanup closes SDL and withdraws its
owned output word. Nested failure preserves a preexisting device. Actual invalid
SDL-device failure leaves the source output/result ABI untouched. If another
owner changes the output binding, cleanup still closes SDL, preserves the other
value and records a permanent fault that prevents quiescence.

World execution scopes own exception cleanup; authority export/prepare/commit
reject live or faulted output ownership. A supported quiescent checkpoint has no
open output device to omit. Full source audio release/removal/handoff and active
voice serialization remain unfinished. The normal world remains sound-cold.

Two independent unchanged original i386 intervals match **2,839 instruction
events** (the tracer separately reports2,838 hardware steps for this REP path), all
**5,201,776 retained bytes per interval**, GP/SSE/logical x87 state and defined
flags/controls, with no byte masks or fixture callee results. The second interval
starts from the genuine native factory result/ABI; the original SDK constructor
is not replaced inside original execution. Its CPU caller receives the native
resource identity as an explicit native platform input.

This checkpoint also corrects the scope of the v8 DSP-prefix trace claim. V8
compared hardware-original and Unicorn-original traces; its prefix snapshots did
not retain the compiled native trace. V9 now records native instruction events
explicitly and compares them with both original executions. The DSP prefix
passes **104 events** and every **5,230,544 retained bytes per interval** with the
native trace included. Earlier immutable v8 evidence is preserved, and does not
retroactively constitute this additional native-trace check. Raw x87 physical
classification/control encoding differences remain reported separately.

The coherent full native build and **17 focused CTests pass with no skips**.
All25 sound/FP/render original comparisons pass again (93,215 events, every
60,084,310-byte committed bank per case), alongside lunge/sword/frag and world
continuation checks. Four visually inspected muted640x480 RTX5080 Vulkan frames
still consume exact original first-person matrices. Their PPM/lossless PNG
pixel difference is zero. No matched Xbox framebuffer/PCM claim follows.

Source:`.local/sandbox-tu15-source-20261014-v9`; build:
`build-sandbox-tu15-20261014-v9`; evidence:
`analysis/sandbox-tu15-goal-20261013/accepted-checkpoint-v9.json`.
All eight earlier accepted checkpoints remain unchanged. Device capabilities,
saved policy, actual DSP execution/effect data, stream/static-buffer/plugin
services, full startup/map admission, voice/cache/completion lifetimes and PCM
remain open. All102 whole sandbox elements remain incomplete, with every other
weapon/power/vehicle/projectile/world/AI/objective, full view/local/split/LAN,
matched Xbox presentation and installed-delivery requirement retained. The goal
remains active.

### Original SDK factory path and native resources checkpoint, 2026-10-14

The live world now executes the original SDK factory path at `387877`, rather
than using the v9 whole-call native device adapter there. The v9 standalone SDL
resource component remains available and tested, but its successful open is not
equivalent to the original SDK construction sequence. The original executable
allocates SDK records, constructs the device/data objects, initializes resource
accounting, queries saved settings and initializes hardware before publishing
its interface pointer. Those CPU prerequisites must remain in the live path.

`source_sound_sdk_tu15_generated.cpp` compiles **35 complete original bodies /
905 authenticated instructions** across `.text` and `DSOUND`: the public/core
factory, construction/allocation/accounting/capability owners, saved-setting
wrapper and four CRT accounting initializers. Every compiled instruction is
checked against the pinned TU1.5 executable. These are static native bodies,
not a production opcode interpreter or successful-return fixture. Missing
settings, hardware, private-pool, virtual-method and cleanup owners still stop
at their actual requests. Compiled body count is not branch or full SDK parity.

Actual CRT rows`46D7E4..46D7F0` select whole original`387BAA`, `387BB5`,
`38A988` and`38A993`. Their source instructions initialize the SDK statistics
pointers. Native code does not assign free-voice/SGE counters, allocation totals,
speaker settings, device identities or source ready flags.

`SourceRuntimeSoundSdkResources` supplies actual owned native raw storage,
capacity queries and recursive synchronization at the reached original import
sites. The original CPU code owns block clearing, the`pool` client marker,
capacity accounting, reference counts, lists, singleton publication and object
initialization. Native service ABI preservation, source address selection and
host allocation capacity are explicit platform policies; original kernel record
writes and volatile clobbers are not claimed. Native user-thread execution owns
an explicit passive IRQL context; elevated or foreign execution contexts stop.
The source guard still reads the actual owned FS-relative byte.

A fresh unchanged TU1.5 cold boot verifies all905 compiled instruction rows and
observes the actual factory prefix: one critical-section entry, two real kernel
allocations, two actual capacity queries and the naturally reached saved-setting
helper. The emulator's actual FS base`80035BDC` is read from its segment state;
its actual IRQL byte is0 and is retained with the ABI. No guessed KPCR address,
guest memory/register/code write or supplied callee return is used. The actual
saved-setting result is retained as reference only, never native configuration.

The native prefix starts from the genuine original caller state and uses real
native resources. Original code constructs its SDK singleton and partial data
record, accounts the actual two capacities and reaches **2E101C -> the actual
saved-setting import** with setting index9, four-byte output and real stack
cells. That provider remains unavailable. No source device output, free-resource
counter, ready flag, downloaded DSP result or PCM is supplied. Exceptions release
new native raw blocks and unlock synchronization. Partial source singleton/
object/accounting writes remain diagnostic; their rollback is not invented.
The owner records a permanent fault and rejects reuse/export/prepare/commit of
that partial state. Supported quiescent checkpoints require an untouched native
allocation cursor, no live blocks/locks, no fault and the owned passive context.
Active SDK lifetime serialization and normal source Release remain unfinished.

The reference kernel returns **56 and184 bytes** for the first two requested
raw blocks. The current native provider owns actual **4,096-byte host blocks**,
so original CPU accounting reports8,192 bytes in this native prefix. This is
explicitly a remaining pool-size/resource-accounting fidelity gap. The reader
is not made to report the observed Xbox totals, and host page sizing does not
prove Xbox pool capacity or whole `GetCaps` parity.

Six independent unchanged original i386 CPU intervals match **2,284 reported
instruction events** (the tracer separately reports2,280 hardware steps), all
retained bytes in each interval, GP/SSE/logical x87 state, defined controls/flags
and the explicitly captured native trace. Retained extents are5,256,853 bytes
for intervals1/2,5,260,949 for3/4 and5,265,045 for5/6. The actual source FS base
and retained byte participate in execution. No output byte mask, fixture callee
result, expected final output or original instruction patch is used. Native
platform results are explicit interval inputs; original kernel execution and
raw x87 classification encoding parity are not claimed.

The coherent native build and **18 focused CTests pass without skips**. The
25 sound/FP/render original comparisons pass again (93,215 events, every
60,084,310-byte committed bank per case), and previous output/DSP prefix checks
remain passing. Lunge/sword/frag and authority continuation checks pass. Four
visually inspected muted640x480 RTX5080 Vulkan frames consume exact original
first-person matrices, with zero PPM/PNG pixel difference. This is native GPU
regression evidence, not a matched Xbox frame/PCM comparison. The standalone
SDL component reports device status playing; callback invocation and physical
PCM consumption are not established by that status query.

An initial build encountered full`/tmp`; compiler temporary storage moved into
this goal's workspace. After an interrupted link/test session, the retained
build was resumed and the final18-test run completed. Earlier accepted source,
executables, captures and evidence are preserved.

Source:`.local/sandbox-tu15-source-20261014-v10`; build:
`build-sandbox-tu15-20261014-v10`; evidence:
`analysis/sandbox-tu15-goal-20261013/accepted-checkpoint-v10.json`.
All nine earlier accepted checkpoints remain unchanged. Exact pool capacities,
real saved policy, hardware/DSP/effect-data application, all stream/static/plugin
services, normal startup/map admission, complete voice/cache/completion/release
lifetimes and PCM remain open. All102 whole sandbox elements remain incomplete;
every other weapon/power/vehicle/projectile/world/AI/objective, full view/local/
split/LAN, matched Xbox presentation and installed-delivery gate remains in
scope. The goal remains active.

### Recovered reference pool capacities and accounting checkpoint, 2026-10-14

The native SDK pool provider now owns backing with the recovered reference
client capacities. It no longer accounts an entire host page for every small
SDK allocation. Original SDK instructions still clear blocks, install the
`pool` client marker, account actual capacities and construct objects. No
capability total is copied from a capture or assigned to make startup succeed.

Three new unchanged TU1.5 cold boots observe real pool allocation/query calls
while the whole original SDK device factory returns normally. The allocation
and query code is read from the actual resolved imports, with code equality
checked before/after execution. The actual BIOS and emulator binaries are
hashed. This kernel evidence is explicitly from the owned **Complex4627/xemu**
platform; physical retail-kernel equivalence is not established. The original
TU1.5 executable remains pinned and byte checked, and hardware breakpoints make
no guest code, memory, register or callee-return modification.

The actual allocation prefix compares requested bytes with`FD8` (4,056). The
small path computes allocation units as`(bytes+27h)>>5`. The whole actual size
getter reads its consumed header byte and returns`units*32-8`. Larger requests
select the original memory allocator. Its captured body counts full pages plus
one nonzero low12-bit tail; this caller's guard-page argument is0. The captured
large query scans to the actual terminal page and returns that page count times
4,096. These rules come from original instructions and actual returns, not from
fitting two observed sizes.

The naturally reached constructor requests and capacities are:

| Raw request | Actual reference client capacity | Native owned client capacity |
| ---: | ---: | ---: |
|48|56|56|
|184|184|184|
|1,348|1,368|1,368|
|32|56|56|
|40,964|45,056|45,056|

`source_sound_sdk_pool_layout` implements this capacity calculation. Small
native allocations own a32-byte-unit backing with an eight-byte private header;
the size byte consumed by the original getter describes that real extent.
Large allocations own the actual page-sized extent and an aligned source alias.
The resource registry validates real ownership before a capacity query; changed
size metadata and an unrepresentable32-bit backing stop. Source address choice,
unused header bytes, native synchronization and failure reporting remain declared
native policies. Xbox previous-size/type/tag/free-list records, physical page
availability, PTEs and original kernel failure/volatile-clobber behavior are not
manufactured or claimed. Native code does not write a recorded capacity result
into an unbacked allocation.

The native component performs **4,068 real allocations**: every small requested
size0..4,056 and11 large boundary requests. All five naturally observed complete
original allocation/query pairs match. Each small size executes the unchanged
original allocation prefix independently; all actual native size bytes agree
with its calculated allocation units. The **4,057 whole original small-block
size getters** then execute unchanged on the host i386 CPU, consuming the actual
native header inputs and matching capacity, stdcall stack consumption, retained
memory and the independent original trace. They cover **28,399 original
instructions**. The11 large cases independently execute the unchanged original
MM page-count arithmetic; no physical PTEs or positive allocation return is
supplied to make that arithmetic pass. Kernel physical allocation, free-list
lifecycle, zero-size whole-kernel allocation success and the full32-bit request/
resource-failure domain remain outside these bounded contract checks.

The original CPU harness now admits explicitly authenticated additional original
code banks. Kernel code must match its captured SHA-256 and every executed byte;
it is loaded as unchanged original bytes, not an instruction patch in the game
image. The extra-code mechanism is test-only. Existing original game-image
instruction checks remain in force and all previous game CPU comparisons repeat
successfully. Production compiles the recovered capacity expression natively;
it does not interpret kernel opcodes or emulate physical PTEs.

The compiled original SDK factory prefix now accounts **240 bytes** for its
first two actual blocks (56+184), matching the observed reference prefix instead
of the prior host-page total8,192. Six independent original i386 intervals match
**296 reported instruction events** (292 hardware steps), the explicitly captured
native trace, meaningful ABI including actual FS context and every retained byte:
5,256,853 bytes for intervals1/2,5,256,917 for3/4 and5,257,109 for5/6. Original
clearing now follows actual capacities, which explains the lower instruction
count. The real saved-setting import still stops the native constructor before
its output/ready flag, hardware initialization or source PCM can be admitted.

The coherent native build and **19 focused CTests pass without skips**. The
25 sound/FP/render original comparisons pass (93,215 events, every60,084,310-byte
committed bank per case), previous output/DSP prefixes remain passing, and
lunge/sword/frag plus authority continuation pass. Four visually inspected muted
640x480 RTX5080 Vulkan frames consume exact original first-person matrices.
Their PPM/PNG pixel difference is zero. This is regression evidence, not matched
Xbox framebuffer or physical PCM proof. Ordinary source audio remains cold.

Source:`.local/sandbox-tu15-source-20261014-v11`; build:
`build-sandbox-tu15-20261014-v11`; evidence:
`analysis/sandbox-tu15-goal-20261013/accepted-checkpoint-v11.json`.
All ten earlier accepted checkpoints remain unchanged. The matched capacity
contract does not complete SDK resources or whole `GetCaps`: physical resource
availability/failure and free/reuse, real saved policy, hardware/DSP/effect-data,
stream/static/plugin services, full startup/map admission and all voice/cache/
completion/release/PCM lifetimes remain open. All102 whole sandbox elements
remain incomplete, with every other weapon/power/vehicle/projectile/world/AI/
objective, view/local/split/LAN, matched Xbox presentation and installed-delivery
gate retained. The goal remains active.

### Actual audio settings resource/query checkpoint, 2026-10-14

The native SDK resource owner now accepts an explicitly bound actual EEPROM
UserSettings resource. The existing loader reads the actual256-byte file and
retains only its immutable96-byte non-key UserSettings section. Its existing
checksum expression is preserved. `AudioFlags` is at section offset56, proved
by the pinned primary structure/lookup expression and an independent32-bit
compiler layout assertion. Native code does not choose a stereo setting, copy
a successful query result from a capture or retain encrypted/factory keys.

`source_try_tu15_audio_settings_query` owns the actual`37DA9A ->2E101C`
`XC_AUDIO` request: index9, four-byte output, caller-local type/value cells and
null ResultLength. It authenticates the complete original frame. A valid
checksum produces REG_DWORD and the actual resource's raw AudioFlags; an invalid
checksum returns its actual failure status and leaves type/value untouched.
All outputs are validated before publication. No store goes through null
ResultLength. Other query indices remain available to their existing owners;
the original startup`XC_MISC` path and its tests remain passing.

`SourceRuntimeSoundSdkResources::bind_user_settings` requires its available
quiescent native executor before retaining the immutable resource. The live
world exposes the same explicit file binding through
`bind_sound_user_settings`. This is a local platform input and initializes no
source audio device. Missing resource input stays unavailable; normal gameplay
audio is still cold. Binding after a partial/failed SDK construction is rejected.
Positive query composition is tested through the real resource owner; full
ordinary-game audio activation/delivery is not established by this API.

A new unchanged TU1.5 cold boot observes the actual query, subsequent boot-mode
reader and helper return. Its type4, status0 and actual audio word match the
owned EEPROM non-key section. The actual platform-data import is ordinal356,
`HalBootSMCVideoMode`, read by complete`33C29D`. Its observed reference value1
is recorded as diagnostic data only and is **not supplied as current native
mode**. The original helper separately forces value1 for mode3 or clears the
high16 bits for mode6 (`AND WORD [EBP-2],0`). That dependency remains distinct
from reading saved audio. The prior prose said low16; the generated original
operand was already correct. Checkpoint v13 independently exercises this branch.
No native AV/SMC mode, profile or successful reader return is invented.

The SDK compiler now keeps the actual`37DABA ->33C29D` call at a required ABI
boundary while retaining the complete original three-instruction reader. The
native configured path reaches that request after a genuine query. No boot-mode
input is provided and the diagnostic composition stops there. Unconfigured
native settings remain stopped at the earlier query. No source device output,
ready flag, DSP result or PCM is admitted by either path.

The positive case uses the actual checksum-valid resource. A separately declared
checksum-damaged copy exercises failure. The original helper's **own37DAB6
instruction** performs its zero fallback after the failed query; the provider
does not return a selected zero setting. Both cases retain and unwind the real
native pool/lock resources and permanently fault partial source construction.
Their AudioFlags/type/status/write order are compared with the complete pinned
primary query expression independently compiled to i386, including its actual
atomic snapshot lock and declared byte memcpy/memset primitives. Valid output
has nine ordered writes; invalid output has none. Every5,257,109 retained caller
bytes matches per query. This is native query-contract evidence, not original
Xbox kernel volatile-clobber/store-width or physical AV-state proof.

Fourteen independent unchanged original SDK i386 intervals match **599 reported
instruction events** (591 hardware steps), all retained memory, meaningful
GP/SSE/logical x87 state, defined controls/flags, actual source FS context and
compiled native instruction order. Seven valid intervals cover299 events;
seven failed-query intervals cover300, including the actual fallback branch.
No original instruction patch, expected final output, fixture callee result,
byte mask, boot-mode value or device return is supplied to original execution.
The two query comparisons independently consume actual resource bytes or the
declared checksum negative input.

A linker dependency cycle surfaced because the query implementation and full
process-startup composition shared one archive object. The unchanged startup
composition is now in its own translation unit; query clients can link their
actual dependency without pulling unrelated startup/platform allocation calls.
The final complete build and **21 focused CTests pass without skips**, including
the existing48-case startup-settings regression. An initial incomplete CTest run
and two oracle-harness errors remain retained as diagnostics; the final complete
run and repaired comparison are the acceptance evidence.

The4,068 native pool cases /4,057 original i386 getters, previous output/DSP/SDK
prefixes and25 sound/FP/render original comparisons pass again. The25 cases
cover93,215 events and every60,084,310-byte committed bank each. Lunge/sword/frag
and authority continuation pass. Four visually inspected muted640x480 RTX5080
Vulkan frames consume exact original first-person matrices; PPM/PNG difference
is zero. All18 ordinary source sound submissions remainNONE. These checks do
not establish matched Xbox frames, physical PCM or successful audio startup.

Source:`.local/sandbox-tu15-source-20261014-v12`; build:
`build-sandbox-tu15-20261014-v12`; evidence:
`analysis/sandbox-tu15-goal-20261013/accepted-checkpoint-v12.json`.
All eleven earlier accepted checkpoints remain unchanged. Actual boot-SMC/native
platform data, hardware/DSP/effect-data, stream/static/plugin services, full
startup/map admission, voice/cache/completion/release/PCM and pool free/reuse/
physical-failure lifecycles remain open. Every other weapon/power/vehicle/
projectile/world/AI/objective, view/local/split/LAN, matched Xbox presentation and
installed-delivery requirement remains in scope. All102 whole sandbox elements
remain incomplete and the goal remains active.

### Thirteenth accepted checkpoint: boot publication and complete audio helper

A private, unchanged TU1.5 cold boot now observes the actual producer of
ordinal356. At the owned Complex4627 kernel entry, the exported word is7. A
hardware write watchpoint observes its sole reached publication to1. Read-only
execution breakpoints capture the real SMBus request at80054F44: address20,
command4, byte read, actual caller output cell. Its real return at80054F49 is
status0 with output1. Original code, memory and registers are never written,
and no callee return is supplied. The BIOS/emulator hashes are retained. This
identified modified BIOS platform is supplemental evidence, not physical retail
kernel equivalence or a current native AV-pack value.

`source_boot_smc_video_mode_after_read` ports only the recovered publication
interval80054F49..80054F58. A negative signed status retains the explicit prior
word; a nonnegative status publishes the response masked with7. It does not
choose an initial7, translate an invented native error, read hardware or claim
the rest of the boot routine. All6,240 declared prior/status/response cases
match unchanged original instructions on the host i386 CPU (21,840 events).
The real observed transaction also matches. Native SMBus transport remains
unavailable; an absent transport is never converted into a fabricated failure.

The runtime image loader accepts an explicit named
`SourceRuntimeKernelBootSmcVideoMode` resource. Before mounting the original
read-only section, it authenticates import41B784 as encoded ordinal356 and
resolves it to retained read-only backing. Missing/unnamed input, wrong image
selection and later writes are rejected. The existing unconfigured loader
leaves the encoded import untouched. Ordinary live-world construction does
not bind a boot mode, alter its existing timer import or activate sound.

The complete original37DA9A helper and complete33C29D reader now compose in a
separate branch-coverage test with the actual native EEPROM-query owner. The
reader executes its original instructions and returns through the real source
stack; no reader return is supplied. All8 low-three-bit AV-pack modes,37 audio
flag patterns (including each single bit), and valid/damaged checksums form
592 declared cases. Original and altered inputs are labeled test-only. The
native mode resource is never inferred from these reference/coverage values.

Every query is independently recomputed using the pinned compiled primary
checksum/lookup code. All592 queries agree on status, ordered nine/zero output
writes and every4,188-byte retained caller bank. The1,184 unchanged original
CPU intervals agree with the compiled native trace, meaningful GP/SSE/FP/flags
and every retained byte across15,762 events. These are complete helper CPU
branches around a declared query ABI, not original kernel clobber equivalence,
ordinary sound-device startup or PCM. Mode3 writes1 through its original branch;
mode6 clears the **high16** audio bits with`AND WORD [EBP-2],0`. The previous
prose said low16; the generated operand was already correct and is unchanged.

The first regression run exposed an existing shutdown race: after publishing a
file-job completion, the source worker could be cancelled at its free-list mutex
before retiring the job and reaching the idle semaphore. A deterministic host
schedule holds the fourth actual job at that mutex until stop is requested. It
reproduces the rejected checkpoint on the old owner. Cancellation now applies
only at the authenticated original idle semaphore wait; intervening original
cleanup and real mutex acquisitions finish. The same test then exports a real
joined idle worker with empty queue, released mutexes and actual file owners.
No idle flag, queue completion or semaphore permit is assigned to force success.

The final isolated build passes24 focused tests, including original arena store/
return/CRC validation, the deterministic shutdown overlap, arena continuation
and whole live-world continuation. The initial failing run and targeted recheck
remain diagnostic. Refreshed original comparisons pass for saved audio, pool
capacity, SDK/output/DSP prefixes, and25 first-person/sound/render cases covering
93,215 events and all60,084,310 retained bytes each. Lunge, close sword and frag
checks pass. Four muted native Vulkan frames consume exact source node matrices;
the inspected PPM and lossless PNG differ by zero pixels. Xbox framebuffer/PCM
and installed delivery are not established. All18 ordinary submissions remainNONE.

Source:`.local/sandbox-tu15-source-20261014-v13`; build:
`build-sandbox-tu15-20261014-v13`; evidence:
`analysis/sandbox-tu15-goal-20261013/accepted-checkpoint-v13.json`.
All twelve earlier checkpoints remain immutable. Native AV/SMC ownership,
hardware/DSP/voices/streams, source audio admission/cache/completion/release/PCM,
and every remaining weapon/power/vehicle/projectile/world/AI/objective,
local/split/LAN, Xbox presentation and delivery gate remain open. All102 whole
sandbox elements remain incomplete; the goal remains active.

### Fourteenth accepted checkpoint: scheduled firing and current-surface distortion

The original weapon scheduler now publishes completed108A70 calls to the native
presentation journal. The original routine still owns effect selection, refresh,
allocation, color stores and the actual instance result. Seven whole calls in the
normal SMG input sequence create one real instance and refresh it six times;
a queued shot after release remains observable. Full salted weapon/unit/instance
identities, original scalar bits and source colors are retained. Parallel native
shot production and legacy duplicate effect creation remain zero.

Inherited motion is queried by the complete originalBC200 and reached pure
accessors using private native scratch. It cannot write the live source bank or
enter a second live gameplay call scope. Moving, stopping and stationary actual
weapon queries preserve every source byte. These three complete original CPU
comparisons cover235 instructions and60,088,406 retained bytes each. The seven
complete effect comparisons cover12,708 instructions and60,084,310 bytes each,
including original/native paths, meaningful ABI and x87 status. No original code,
callee result, allocation handle, target or success state is supplied.

The comparisons exposed two frozen-generator defects. FST/FSTP must retain the
source x87C1 store result; the existing shared lowering is now present in this
coherent combat module. Pool instruction observations must use the selected
image's addresses exactly once. Both corrections preserve original instruction
bodies. Refreshed first-person, sound and melee/grenade checks pass afterward.

Actual source-scheduled gameplay-part ownership permits the authored jpt! and
acceleration parts to accompany native visual systems without a second damage
producer. Unowned uses still reject. Presentation maps key by weapon and effect
while retaining full salt checks. The private snapshot protocol preserves the
source-scheduled bit, rejects reserved flags and rejects scheduled metadata
without the actual presentation/instance. Five real fragmented loopback UDP
snapshots retain seven events through reversed/duplicate fragments, one lost
fragment and a late older snapshot. This verifies metadata transport and late
join retention; split-screen/LAN rendered effects and PCM remain unverified.

The existing original-backed distortion consumer is narrowly integrated from
shared work. Authenticated19510/14F40 and SDK surface-rotation evidence selects
the current physical render surface for external23. The renderer copies current
color immediately before each dependent particle, preserves color/depth/stencil
across continuation passes, and retains ordered authored raw sampler writes.
The distinct classification7 vertex program and signed RGB/unsigned-alpha
filtering feed stage1 BUMPENVMAP. Missing bindings, programs or bump-state owners
remain errors. This is native resource adaptation, not full original rasterizer
scheduling or physical Xbox coherency. The reference contains120 complete binder
calls, eight surface rotations and six complete18540 sampler cases. The current
native checks reproduce all six sampler cases;32 declared-input static/borrowed
view GPU cases have at most one-byte color error and unchanged depth. The older
original particle draw/depth bank also passes.

The live native probe initially misread its observer's tick argument as a weapon
handle, invalidating its reported zero-particle counts. It now identifies real
draws by retained effect owner/definition and a completed source journal event.
The corrected18-tick native Vulkan run publishes five new source events and
submits30 matching particle records across10 ticks; all18 foreground model draws
consume the byte-exact original node bank. The captured640x480 PPM was inspected
through a lossless PNG with zero pixel difference. Audio is muted; this is not a
matched Xbox framebuffer/PCM comparison. The source particle pool's complete
render/update/retirement ownership and other unsupported effect lifecycles
remain open. A separate stale incremental object caused an initial invalid-free
failure after copied header timestamps were preserved; touching the imported
sources/headers and rebuilding produced the accepted coherent binaries.

The isolated build produces24 executables;32 focused CTests pass without
skips. Refreshed original comparisons pass for all25 prior first-person/sound
cases (93,215 events), seven effect calls, three private motion queries, saved
settings, boot publication/audio helper, pool capacity and SDK/output/DSP
prefixes. Lunge, close-sword and frag composition pass. All18 ordinary sound
requests still returnNONE through the original cold-device fallback; full audio
startup/admission, voice/stream/DSP/cache/release lifetimes and PCM remain open.

Source:`.local/sandbox-tu15-source-20261014-v14`; build:
`build-sandbox-tu15-20261014-v14`; evidence:
`analysis/sandbox-tu15-goal-20261013/accepted-checkpoint-v14.json`.
All thirteen previous checkpoints remain unchanged. Every remaining weapon,
power, vehicle, projectile, dynamic-world, tactical-AI, objective, local/split/LAN,
matched Xbox presentation and installed delivery gate remains required. All102
whole sandbox elements remain incomplete; the goal remains active.

### Fifteenth candidate: source particle phase and attribution gap

This candidate is **unaccepted**; checkpoint14 remains the latest accepted build.
It integrates53 complete original particle-frame bodies (2,661 instructions),
18 draw/storage bodies (1,528 instructions), actual380FA0 bounds construction
and22C440 frame-audio storage construction. Whole179DF0 now runs once per native
render frame. Actual12E142 calls13AA50 before12E151 calls13A9B0, whose13AA28
calls179DF0; the first early-placement diagnostic was corrected to this order.
The existing native elapsed input is an explicit adaptation. Complete149160/
13AA50 timing, early termination and full13A9B0 scheduling remain required.
Completed native portal results feed whole3F3E0, with one union reset and all
local views publishing. Complete original camera/portal production is unverified.

A235-tick normal source SMG sequence emits live particles, drains all four
particle pools and retires its actual full salted effect identity. Later slot
reuse is permitted; checking for a permanently zero salt was a probe defect.
This sequence has no original draw callbacks and follows stale-draw removal,
not ordinary rendered aging. Fifty-one retained original component cases cover
motion, visibility, draw admission and35 natural-drain frames. They retain their
explicit captured inputs and private-stack exclusions; the new live API oracle
compares every60,084,310-byte committed bank without those exclusions.

Independent scripted firing in1,2 and4 actual local views consumes source rounds
[5], [5,4] and [5,4,3,2]. Every18-frame run advances the source particle phase
exactly18 times and consumes18,36 and72 exact original-pose foreground draws.
Authored particle records per view are[30], [30,22] and[30,22,14,8]. Twelve
captured whole API calls across those runs and the headless sequence match
untouched original i386 paths, meaningful CPU exit and every retained byte.
Audio remains muted. Original176B30 callback/vertex/shader/GPU production and
live draw stamps remain open; these native frames are not Xbox pixel/PCM proof.

The first composed run exposed actual1816F1 ->181C00 instanced-impact projection.
That complete owner and1453B0/1454D0 helpers are now generated from authenticated
original bytes. A refreshed whole firing-effect comparison then exposed stale
x87C1 in an older frozen death/effect module's FST/FSTP stores. Shared source
already had the correct lowering. Regenerating only this candidate's module
fixed all seven original CPU exits without altering original instructions,
results or masks. All twelve prior oracle families refresh successfully.

The coherent28-target build passes84 focused tests, followed by lunge/sword/
frag regressions. The additional16 death tests expose nine failures: stationary,
moving, remote/spectate, lifetime and melee cases stop at their fatal-hit check;
radius, fall, camera and three cleanup branches pass. A diagnostic confirms
that source bullets kill the victim (body -0.0138891, shield0, death count1,
25 source rounds spent), while shooter kills, native hits and native shots are0.
The missing seam is original attribution/stat publication, not permission to
force a kill or native hit counter. The native motion readback currently labels
all source fatalities as environmental with last_damager=-2; its exact source
attribution/notification owner must be recovered before changing that bridge.
D8C20 is a generic object-owner resolver, not by itself the lethal credit query.
Original14EAD0 registration and genuine statistics/attribution consumers are
being traced; no guessed credit or parallel damage event is inserted.

Source:`.local/sandbox-tu15-source-20261014-v15`; build:
`build-sandbox-tu15-20261014-v15`; current evidence:
`work/sandbox-primary-v15/candidate-summary-v15.json`.
All102 whole sandbox elements and the full goal remain incomplete and active.


## Fifteenth candidate attribution continuation: still unaccepted

The previous nine fatal-hit failures are now resolved in the isolated v15
candidate. Original source stats already credited the actual shooter/victim;
the native match failed to publish them, and original phase/status remained0.
Whole15AFB0 now starts the selected engine after the retained map constructors.
Original69220 constructs the missing simulation-context backing. The original
start sets phase1/status1; neither field is assigned by a native repair.
Its complete1029-instruction call matches unchanged TU1.5 i386 execution,
meaningful CPU and all60084310 retained bytes in v41. This is the retained
cold native simulation context, not full original local/System Link startup.

The source-world damage bridge now runs wholeDBB90 with the authenticated
DA7DA..DA7E2 EDI/report and full-datum stack contract. It replaces the former
physical-only fragment and stops duplicate native attribution. Completed164AB0
statistics and actual source death timers are read back; normal source firing
spends25 rounds and publishes1 kill,1 death and1 Slayer point with native shots0.
Admitted radius damage credits the kill and the shooter's genuine self-damage:
1 kill,1 death,1 suicide and score0. Native early kill-trigger handling is
excluded from source-owned movement; the original world now produces the
walk-off fatality and its actual300-tick countdown. The death test observes
that countdown and the subsequent HUD fade instead of ending before respawn.
Its native-hit assertion is replaced by actual original credited-stat evidence;
UDP checks now verify kill, death and score publication. Temporary diagnostic
prints are removed. Canonical observer interfaces remain unchanged: direct
world/damage call traces translate through authenticated instruction maps,
while phantom/vehicle compositions retain their own translation.

The coherent29-target build passes100 selected CTests in v38, followed by
lunge/sword/frag checks. The separate campaign-camera policy test also passes.
All12 prior original-comparison families refresh in v39. Fresh muted GPU runs
in v40 independently fire1/2/4 views, consume[5], [5,4], [5,4,3,2] rounds,
and perform18 particle phases plus18/36/72 exact-node foreground observations.
Twelve refreshed whole particle API calls match original paths/CPU/every retained
byte. PNG conversion is lossless; live captures differ from older native pixels
and are not Xbox framebuffer or PCM comparisons. Full frame-delta production,
original particle draw callbacks/stamps and original sound delivery remain open.

Strict wholeDBB90 comparison is not complete. Its path matches10287 instructions
before original126139 calls1A3B70; native execution instead uses the existing
synchronous animation-resource bridge. Original execution continues into file
routing/request creation and reaches NtWaitForSingleObjectEx800000EA. The
file-bank handle words in this retained world are zero; its constructed150-node
queue has real synchronization aliases, but the boot worker was stopped/joined.
The exact original file-open/queue/worker/completion path must be integrated,
with real native file outcomes and safe cross-thread memory publication. No
kernel return, file handle, done flag or source branch is supplied to the
original comparison to force a pass. See work/sandbox-primary-v15 resource
primary evidence and credit-radius-original-stop-v37.log. True player creation
and census/activation, original pre/post-world engine scheduling, all other
sandbox elements, local/split/LAN/Xbox presentation and installed delivery gates
remain required. Latest accepted stays v14; all102 whole elements remain open.

## Fifteenth candidate resource continuation: still unaccepted

The isolated v15 candidate now runs original file-open/read/write/size/truncate/
close requests through its genuine persistent source worker. A new module
lowers14 authenticated complete roots,668 instructions. The original queue
allocates IDs/nodes, retries partial callbacks, publishes done, unlinks work
and returns leases. Native services perform actual file operations; writable
access requires an explicitly registered private namespace. The owned map is
opened read-only. No handle, done flag or gameplay return is fabricated.

The component test completes17 distinct actual source requests and restores
the original free-node set. It covers a two-chunk read, zero/EOF/short reads,
real failed open/seek, private-file extension/shrink, two-chunk write/flush,
file-size queries, an actual read-only-descriptor write failure and closes.
Six captured read callbacks match298 unchanged original title instructions;
seven write/size/close callbacks match202. Each comparison checks CPU, trace
and all57512002 retained bytes under the declared native XAPI file policy;
the latter also checks the complete resulting private-file bytes. The resize
callback executes in the existing arena module and has real-file and golden
startup checks; it is not one of those seven new CPU captures. The old arena
golden and continuation cases plus the expanded resource case pass3/3.
The earlier30-target build passed102 selected tests and lunge/sword/frag; a
fresh broad run after the writable expansion is recorded separately in v57.

This is a standalone component, not replacement of the three live synchronous
resource bridges. Original cache initialization is a writable HDD cache path,
including cache names, size/header validation and map activation. That path,
safe overlapping cache polling/buffer lifetimes, persistent live worker state,
cancel/release and resource-owned continuation remain required. The test's
condition variable observes actual source done stores; it does not establish
live-world memory publication or Xbox IO timing. Resource-configured arena
checkpoints explicitly reject rather than omitting file/worker ownership.
The1AA680 FileReference wrapper is generated but not admitted without its
remaining normalization owners. No original Xbox XAPI/kernel wrapper, pixels,
PCM or installed delivery is proved here. Accepted stays v14; all102 whole
elements and the complete goal remain open.
