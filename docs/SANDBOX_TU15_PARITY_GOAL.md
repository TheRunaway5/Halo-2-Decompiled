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

Final resource continuation regression: v57 completes the coherent30-target
build,102/102 selected CTests, and lunge/sword/frag checks successfully. No
whole-element coverage or accepted-checkpoint advancement follows this result.


Current v78 continuation: common retained-memory copies and registry operations
now use a per-bank recursive guard. A borrowed span needs an AccessScope or a
quiescent/immutable owner; there is no whole-frame lock. The direct done-byte
polling test and a real TSan concurrency run pass. This does not bind the live
persistent cache worker or its continuation, cancellation and buffer lifetimes.

Interactive delivery found additional original owners. Whole102C70 (74 original
instructions) now resolves the actual carried weapon and calls realE8E10/E8E50.
Two declared primary-hand action inputs compare exactly against unchanged TU1.5:
683/513 ordered instructions, fullCPU and60084310 retained bytes per case. This
is not all hand/action states or complete player birth.

The real idle channel selected authored clips26/27 outside the native startup
clock subset. Presentation now decodes the exact graph and packed selection,
including its authored inherited-graph owner. A natural idle-variant case and
its draw match unchanged original code on the host i386 backend:23215/7589
instructions and every retained byte accounted. Unicorn rounding/exception-bit
differences remain recorded; hostCPU evidence is not Xbox hardware pixels/PCM.

Four-player grenade input then reached missing particle-contact2C2D90. Its
whole body, literal helper closure and actual metadata callbacks are lowered;
the expanded frame module has71 roots/3874 instructions. The same four-view
45-second combat sequence now exits normally. Original contact-phase comparison
and a fresh coherent regression run are pending. Temporary FP debug tags were
removed; existing unsupported-model guards remain. These bounded results do
not complete any of the102 whole elements. Accepted staysv14, candidatev15
unaccepted; the installed preview is still the older known-stopping binary until
validation and package refresh. Full menus/maps/game and live source cache,
render/audio/LAN integration remain required.


v92 validation and test delivery: the coherent37-target build passed108 selected
CTests, lunge/sword/frag. A new original particle-owner golden regression passed
separately, for109 selected checks total. The live unmuted four-view source frame
compares exactly with unchanged original i386 host execution:12645instructions,
fullCPU and all60084310retainedbytes accounted. Entered2C2D90/176A40/24FFE0/
252100 branches are covered; physical-query, hit and impact branches remain open.
Unicorn exception-bit differences remain recorded. Xbox hardware/pixels/PCM and
complete particle behavior are not established by this comparison.

The fresh immutable sandbox-test-v15-20261014-r2 test candidate is now selected
by Halo2-Sandbox-Test and its shortcuts. Actual executed SHA is8be194ad2797f0b727
212cc18c56910088f4c85b936288de7718c491396e1b40. Unlimited-frame packaged checks
pass:1view idle40sec/963ticks,2view combat45sec/1093ticks,4view combat45sec/840ticks,
all clean requested closes. The real root wrapper also executes this same SHA.
These are bounded Lockout host tests with player1 keyboard/mouse input, not
playthroughs, every weapon/map/controller, LAN or framebuffer/PCM acceptance.

Concurrent launcher work replaced the earlier home test button with an MCC
content preview. The root home now retains both routes, with separate hit bounds:
Xbox TU1.5 sandbox test opens the pinned r2 selector; MCC content preview keeps
its own route and ordinary Play readiness guard.13Tk checks and a real pointer
handoff pass; both launcher screens were visually inspected. Current ordinary
launchers select .local/mcc-native-20261013/cmake/halo2_native and still report
MCC gameplay not ready; their regular Play does not select this Xbox candidate.
Old stopping preview and its evidence are preserved. Accepted staysv14; v15 is
unaccepted and0/102 whole elements are complete. Full menus/maps/game remains
unfinished. Next original resource dependency is the authenticated6-root,
338-instruction1AB480 normalization closure before1AA680 admission; live cache,
persistent worker and resource-owned continuation are still required.


v103 original resource/FileReference progress: six authenticated normalization
roots (338 instructions) are added to the resource module, now20 whole roots /
1006 instructions. Whole1AA680 is admitted on the standalone native arena and
its literal1AA6E0 yield reaches the existing genuine host scheduling owner.
No native completion, file handle or source callback return is substituted.

A red-capable composed test first failed at the unadmitted wrapper (v96), then
passes normalization, real queued open, actual map header read and source close.
The retained worker processes20 actual requests;19 external completion stores
and the wrapper's genuine stack completion are observed. Its original free-node
set is restored. The original publisher stores a slot index before the salted
ID; the test now observes the authenticated final12210F/1233AF store and proves
20 distinct original request IDs. The initial v97 identity assertion counted
both intermediate and final stores; no gameplay logic was changed for that fix.

Four declared FileReference path/flag inputs compare exactly against unchanged
TU1.5 whole1AB480:1137/1843/1444/2150 instructions, fullCPU and5869634retained
bytes per case. These records are explicit native caller inputs at the fields
read by the original function; this is not integrated original map/FileReference
construction. Eight focused memory/arena/queue/normalization regressions pass.
Seven read and seven other file callbacks also compare exactly, using independent
real native IO outcomes. Xbox XAPI/kernel wrappers and IO timing remain outside
that native policy. No live source cache or resource continuation claim follows.

The installed immutable r2 test build and its prior109-check/1-2-4-view delivery
evidence are unchanged. Private v15 source now contains this additional resource
component; no new full110-case regression or installed delivery is claimed for
it yet. World still retires its boot worker and retains three synchronous
resource bridges. Original six-bank cache startup/map activation, persistent
worker ownership, query/cancel/free lifetimes and resource continuation remain
required. Accepted staysv14; goal active and0/102 whole elements complete.


v124 resource/cache progress (2026-10-15). Goal remains active, private v15
unaccepted, accepted v14 unchanged and0/102 whole sandbox elements complete.
Authenticated whole1A3AD0 cache startup and helper closure added20 roots;
copy1AAE20/1AAD10 and flush1AB2B0/1AB290 add4 more. Current resource module:
44 whole roots,2053 instructions, generated SHA
`cb462b45d4512319c5bf66870faff18fd68162e03636f0f5c1665b869189a61e`.

Original code produces all six bank handles and sizes through the retained
worker. Native configuration maps only source-selected z/n cache000..019.map
names into private files and supplies explicit language and UTC platform inputs.
The test covers cold creation, reopening invalid headers, and writing unchanged
owned disc headers through original requests before reopening again:78 queued
requests and12 genuine native clock calls pass. Bank1 correctly rejects the
missing disc path: disc headers lack the cache-produced path at+20; whole1A4600
writes that path during copying. This is not full map copying/activation or a
successful valid bank1 disc probe. Source time, formatting and file APIs remain
an explicit native platform policy; Xbox wrappers/TLS/hardware timing are open.
Resource-configured checkpoints reject missing ownership instead of omitting it.

Fifteen original-executable helper comparisons pass for the stack probe,
signed bit-string addressing, and forward/backward byte scans, with5869634
retained bytes per case. The expanded queue test passes25 source-owned requests,
24 external completion stores plus the FileReference stack completion,25 unique
salted IDs and restored free-node leases. It exercises alternating multi-chunk
copy, separate flush, read-only destination failure, failed source seek and EOF.
Seven read and16 other callbacks match unchanged executable CPU, instruction
order, all57512002 retained bytes and full private file bytes using independent
real native IO. Nine focused memory/arena/normalization/queue/cache tests pass.

The next authenticated dependency is whole1A4600 copy control with original
resource-pool construction12EBE0 (literal caller1281D), allocation12FF10/12FE00,
and release130030. Existing standalone boot leaves4F2D24/4F2D34 zero. Do not
assign substitute pools, file handles, copied-header paths or completion flags.
The live World still stops its boot worker and retains three synchronous
resource bridges. Persistent worker lifetime, map activation, query/cancel/free,
resource continuation and all full gameplay/pixel/PCM acceptance gates remain.
Immutable installed r2 has not been repackaged for these private components.


v142 original resource allocator progress (2026-10-15):56 whole resource roots,
2595 instructions, final generated SHA
`6fee86012bd8b594410a8533e7a9e971410d155eb3ce160e4fd4dc421319f28d`.
Whole12EBE0 constructs all three metadata pools through its literal11F3F0
allocator, complete33B3AC XAPI wrapper and original16E450/13FC50 bodies.
Whole12ECF0 constructs the payload range from original boot-produced physical
boundaries; whole12FF10/12FE00 allocate, and130030 plus original residency/
16E8A0 deletion helpers release. Four whole originalCPU/trace/memory comparisons
pass:5135/5932/456/176 instructions, all57229378 retained bytes per stage.
Independent real host pages/mprotect supply native resources; one actual native
CPU timestamp is a declared input. Xbox kernel clobbers/cache policy, allocator
failures/retry/eviction and composed live ownership remain unproved.2DB4CA is
MmSetAddressProtect, not an allocation/commit owner.

General dispatch preserves native formatting/yield/last-error policy precedence
and complete source tail-call ABI. Those three overlapping native boundaries
caused and now resolve the initial regressions; queue allocation was not the
cause. Eleven focused checks and refreshed7 read/16 other IO callback original
comparisons pass at the final head. Whole1A4600 copy control, map activation,
persistent worker, query/cancel/free and continuation are next. Inspection at
12E030 was rejected as an entry (it starts JE); no code was lowered or executed
from it. Acceptedv14 and installedr2 stay unchanged. Privatev15 unaccepted,
goal active and0/102 whole sandbox elements complete; no full-suite/delivery claim.


v158 whole source map-copy progress (2026-10-15):81 resource roots /3860
instructions, final generated SHA
`4a3dce266346dfb839ef39c04bd37f48bc4a2bff11d2578514bf70e0e930c875`.
Whole1A4600 and its request/query/selection/failure/eviction/checksum helpers now
execute. Native callers supply a declared schedule using the genuine source
done byte; original code owns all20 states, bank selection, requests, headers,
checksums, retries and buffer/file cleanup. No engine flag/header/path is forced.

The owned mainmenu,shared,single_player_shared andLockout payloads compare in
full:706572800 payload bytes plus four exact source-produced cache headers,
2729 queued source requests. Each copy traverses states0..19. Original warm
startup retains all four, including the positive bank1 single-player-shared
disc probe. The original type table selects mainmenu2/shared0/single-player1/
Lockout3; bank1's special probe is not a Lockout probe. A separate cold Lockout
request without cached dependencies executes three source attempts/nine queued
requests, reports original status4, and releases its resources.

The observed MMX high-dword unpack is lowered literally. Twenty-seven original
pure helper/checksum/clock-getter comparisons pass, including11 checksum sizes
and a full40000-byte chunk. Existing loader-bound native tick data preserves
read-only import protection; the native monotonic callback refreshes its retained
DWORD before original getter execution. Fresh4 allocator and23 IO callback CPU/
trace/whole-memory comparisons pass, plus13 focused regressions. Clock/kernel/
GPU-cache equivalence, full interleaved controller CPU proof, full original main
frame, live map activation and resource continuation remain open.

Next authenticated map activation is1252E0 (CALL1918D0),173 instructions, with
teardown125570 (CALL11E580/191B46),18 instructions and its real loader closure.
World still stops the worker and retains3 synchronous bridges. Acceptedv14 and
installedr2 are unchanged; privatev15 unaccepted; goal active0/102 whole sandbox
elements complete. No full-suite or fresh installed-delivery claim.


## v159-v184 original cached-map activation progress (2026-10-15)
Goal ACTIVE,0/102 whole elements complete. Privatev15 UNACCEPTED; acceptedv14
and installed immutable r2 unchanged. No agents, commit/push or root code edits.
Root concurrent work preserved; root goal document receives only this append.

Current resource module115 whole roots/5078instr(+34/+1218), final post-template
SHA413071803c0113219748c11ee5c284fb3832911bff3b0e274e44a0b7369eaae5.
resource-map-activation-native-v180/generation.json records literal CALL/JMP
proof for every34 added roots. Printed common-generator SHA is pre-template.

New whole bodies:1252E0(173instr),125570,191A10,125870 blocking reader,
1A3CA0 drain/123510 priority query, stock/build helpers and signature lifecycle
21B4E0/21B540/21B5B0/21B5F0/21B710. Whole SDK dispatch,12280/122C0 tagged
allocation,2DA8B0/2DA950,2DB41F/2DB43C,2DEE89 and free helpers are lowered.
Signature storage uses existing original process-heap constructor actual2DE2EA
(canonical2DA83A), World allocation body actual2DE70E and real native pages.
Native notification consumer/uncontended executor identity are explicit inputs;
contention rejects. Free reason11 CALL2DEED4 is3bytes, lock imports6bytes.
Three new authenticated TU1.5 lock frames reject wrong returns/tails/TU1.1.

Authored scenario input is scenarios\multi\lockout\lockout(header1BC).
d:\maps\lockout.map would skip stock signature logic and is not substituted.
Imports335/336/337(cells41B638/41B634/41B630) bind opaque OpenSSL SHA-1 state
with actual returns2E073B/2E0754/2E0772. Source allocates/writes SDK type/flags,
selects stream bytes and frees. Native code does not forge the Xbox private
0x74-byte SHA layout. Finalization/cancellation retire native state; both
continuation endpoints reject resource/clock/opaque-state omission. Xbox kernel
context/clobbers unproved. API:https://docs.openssl.org/3.0/man3/EVP_DigestInit/.

Evidence(all paths below underwork/sandbox-primary-v15):
-resource-map-activation-v181.json PASS: original1252E0 loads exact two owned
 sections303104+4014080=4317184bytes and publishes tag/scenario/globals.
 Whole125870 twice, SHA init1/update2, source heap alloc/free1 each. Whole
125570 cancels signature, drains source work and clears active selection/base.
-resource-map-signature-v181.json PASS: whole21B5F0 finalizes/frees once.
 Independent Python hashlib digest1c2945c6c945a524bc15a24ff5d6856859cb3968
 matches exact two unrounded update ranges. Primaryv172 stream report pins
 Lockout SHA14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9.
 This is finalization proof, not complete later signature verification.
-resource-map-read_failure-v181.json PASS: real native EOF after genuine
 warm header acceptance; one blocking read, branch125483, original cancellation/
 free/drain and source51DD08=0/51DB19=1. No fake done/count/status/header.
-Each case first copies4 assets(706572800bytes,2729source requests), destroys
 copy arena, and constructs a fresh original arena. Real cold language/version
 initialization runs; source closes cold banks; native filesystem installer
 renames unchanged source-produced caches; actual warm startup validates them
 and bank1 disc probe(7 UTC reads). Activation adds1. No native writes to
51DB38/51DD08 or partition boundaries. Full GPU transition is NOT executed.
-resource-regressions-v181.log17/17 CTests PASS; commands-v181.json retained.
-resource-activation-helpers-original-v182.json42 original CPU/path/all-memory
 comparisons PASS(previous27+15 stock/build/partition cases);6180930bytes each,
6180934 for loader-bound tick input. Standalone helpers, not full caller proof.
-resource-payload-original-v183.json4 allocator stages PASS/all57229378bytes.
-resource-copy-read-original-v184.json7 and file-v184.json16 callbacks PASS,
 full CPU/path/all57512002bytes and private file bytes. Captures/commandsv183.
 Full interleaved controller/worker and composed activation CPU proof FALSE.

Corrections/dead ends:
-v16512250 andv1662DA978 are rejected inspection starts, never lowered or
 executed; literal entries12280/2DA8B0 used instead.
-v169 red at unadmitted activation;v171 red at original process heap; source
 constructor/allocation reused, no native malloc. v173/v174 corrected actual
6-byte imports/3-byte notification frames from owned bytes, no CPU patches.
-v175/v176 original failure:12ECF0 consumes current partition's high range at
12ED48. No native boundary repair or skip of12EDA0 allowed. Correct test uses
 separate fresh arena. v177/v178 fresh startup properly invalidated caches
 before original language/version initialization(0 header clocks). Corrected
 fixture via genuine cold/close/native-install/warm sequence;v179/v180 pass.
-v183 read oracle resolved venv Python symlink to system Python, failed Capstone
 import BEFORE original execution. Native capture valid;v184 invokes unresolved
.tools/python/bin/python and both independent IO oracles pass.
-v184 documentation preflight expected older CTest summary wording, failed
 before any writes. Actual log says100% tests passed out of17; corrected.

Next real dependency: full parent191890 ->191EF0 ->12EDA0 ->12FBB0 always
 calls D3D404D10 and reads device411568 GPU status.404D10 is in D3D, not .text
 and NOT TLS. Bind real source device/command/native GPU synchronization;
 never fake idle/device fields or bypass it. Then11EF70/191A50 and full
 signature verification/all-map acceptance. World still stops worker atboot
 and keeps3 sync bridges; fd registry still has no added mutex. Persistent
 parallel lifetime/resource continuation and all whole-element, local/split/LAN,
 Xbox framebuffer/PCM and fresh installed delivery gates remain open.

Launcher-info+SHA verify installed r2 unchanged:
8be194ad2797f0b727212cc18c56910088f4c85b936288de7718c491396e1b40.
Acceptedv14 unchanged:
ca8d40a2b9ed609f7dc3705404a7fa81959239799796e4b68c854de18bbdcb61.
launcher-preservation-v184.json retained. Current source/executable fingerprints
in candidate-summary-v15.json versionv184. No tagged debug logs; read-only
activation paths are retained test evidence. All owned sessions terminal.
Goal turn classification:PROGRESS. No full game/parity/delivery claim.


## v185-v199 source map-transition prefix and shared tag-owner progress (2026-10-15)
Goal ACTIVE0/102 whole elements complete. Privatev15 UNACCEPTED. Acceptedv14
and installed immutable r2 unchanged. No agents, commits/pushes or root code
edits. Root goal documentation receives only this append; concurrent work kept.

Current resource module119 roots5357instructions(+4/+279 fromv184).
Final post-template SHA33819293ff2e6962b47f0d8d4d22d03149b3fd25f31c251641132f41d3f5999a.
Manifestresource-map-shared-postprocess-native-v196/generation.json; use its
final SHA, never the common generator's pre-template printed hash.
New whole source roots191EF0,166770,12EDA0,12FBB0 are authenticated by literal
CALLs. All original cold/UI/locale/other conditional required callees remain
required if entered. No fake GPU call result or idle/device fields supplied.

Source parent191EF0 clones its temporary partition, constructs the original
payload pool, executes the whole166770 ready-cache branch, and enters reset
12EDA0/12FBB0. It stops at actual CALL12FBDA->D3D404D10, actual return12FBDF.
The test stops/joins its worker only to capture a quiescent complete byte bank;
this ready-cache prefix does not enqueue work. Not a full transition claim.
resource-map-transition-prefix-original-v199.json PASS: unchanged owned XBE
runs independently,11476instructions, complete CPU/path/all57270342bytes.
Independent actual host mmap/mprotect; source request base80061000,size47079424,
protection404. D3D404D10 is NOT executed or returned by this comparison.
Native capture/reportv198 retained underwork/sandbox-primary-v15.

D3D primary authorityv185:404D10 is46instructions and404EC0 is9. Both are in
D3D, not .text. Actual SDK fields include device411568,+934 MMIO base,+1C20
command submission,+1C28 flush mapping. The current root native graphics API
publishes an opaque renderer token at515E90; it does not initialize this
original internal object. Root code was inspected read-only, not imported.
Pinned primary nxdk14d5ee97e73347c973f1f57b68b79ec08c9e77f2 confirms:
100410=PFB_WC_CACHE,10000=flush trigger/in-progress;3240=PFIFO_DMA_PUT,
3244=DMA_GET,400700=PGRAPH_STATUS. Source files/SHA manifest retained in
work/sandbox-primary-v15/gpu-register-authority-v189. Actual renderer device,
command progress, flush and status ownership remain required; no fabricated
NV2A profile, source device layout or ready status is admitted.

Tag postprocessing follows original parent CALL1918D9->11EF70. Important
correction: the live world ALREADY invokes this owner after its manually
bound tag image; Generic11EF70/1F31E0 and World canonical1EE380(actual1F2C70)
already existed. v192 temporarily duplicated three bodies into the resource
module for initial comparison. v196 REMOVES those duplicate bodies, reuses
the existing live-world owners in the native cache API, and translates only
canonical diagnostic instruction/frame addresses through existing verified
source_world_trace_tu15. Canonical1EE380 is an API ID, never a TU1.5 entry.
Strict original11F02C->1F2C70 caller/return11F031 is checked. v197 and current
v199 original comparisons prove the actual shared implementation used by
live gameplay, not an alternate parallel implementation.

Original postprocessor covers4512 authored tags. Native visit counts are
checked against original tag-table group counts:58physics-model,16vehicle,
81collision-model,3biped/creature definitions;4513 iterator invocations.
Whole source11EF70 completes with real caller stack. Its shape helpers and
static vtable3061F0 count method are source-owned. Exact216383instructions,
full CPU/path/all57270342bytes match unchanged original TU1.5 in
resource-map-tag-postprocess-original-v199.json. No gameplay returns supplied.
This establishes tag preparation, not whole physics/vehicle/gameplay parity.

For a stable full-bank comparison, the real worker is held at its existing
native wait hook only AFTER a genuine original125870 zero-byte read retires.
The original reader reuses caller offset wordSP+8 as done storage after
extracting the offset; barrier checks that actual publication. No done byte,
work permit, handle or callback result is invented. Stop token wakes the held
hook on exceptions; successful comparison releases it and original teardown
runs with the worker alive. Original125870 is now admitted as a public whole
blocking reader. Native physical protection diagnostics are read-only and
observe after actual host success. Authenticated11F0A2/11F1C4/11F2DB callers
are admitted; existing native2/4/404 protection policies reused. These
scenario metadata protection branches are NOT entered by owned Lockout.

Evidence(all underwork/sandbox-primary-v15 unless stated):
-resource-regressions-v198.log19/19 PASS with actual commands-v198.json.
 Includes previous17 plus exact GPU-boundary and shared tag-postprocess cases.
 One Ninja/CTest at a time. Current native before/after captures copied to
 resource-map-transition-prefix-capture-v198 and tag-postprocess-capture-v198.
-resource-payload-original-v199.json4 allocator stages PASS/all57229378bytes.
-resource-copy-read-original-v199.json7 and file-v199.json16 callback cases
 PASS, full CPU/path/all57512002bytes and complete private file bytes. Native
 capture/commandv199 retained; no resolved-venv Python symlink this time.
-Previous42pure helper proofv182 remains bounded evidence for unchanged helper
 bodies. No new broad109-test/full-game/Xbox pixel/PCM/delivery claim.

Corrections: v186 genuine red at missing191EF0 owner. v187 reaches exact GPU
 boundary. v191 genuine red at unadmitted11EF70 after actual zero read; held
 worker exits cleanly on failure. v192 duplicate tag bodies passed but were
 removed after checking existing live-world ownership; v196 reuses shared
 owner and v197/v199 prove it. v188/v193/v195 are historical intermediate
 evidence, not current final module/ownership. No source opcode patches or
 tagged debug logs; read-only instruction paths retained as test evidence.

Next: real GPU dependency at404D10 and direct status reads in12FBB0; full
191890 map transition/191A50 and signature verification. The existing live
world still stops worker after boot and manually binds map tags. Its three
1301B0/130F68/126139 bridges remain synchronous; native fd registry has no
added lifetime mutex. Persistent live integration/resource continuation,
all-map/menu acceptance, every whole-element local/split/LAN gate and original
Xbox framebuffer/PCM plus new installed delivery remain open. Do not close
scope from these component comparisons.0/102 complete, goal turn PROGRESS.

Launcher-info and actual runtime SHA preserve r2:
8be194ad2797f0b727212cc18c56910088f4c85b936288de7718c491396e1b40.
Acceptedv14 SHAca8d40a2b9ed609f7dc3705404a7fa81959239799796e4b68c854de18bbdcb61.
Preservationv199/current private source/executable fingerprints saved in
candidate-summary-v15.json. All owned sessions terminal; no owned processes.


## v200-v207 native file-registry concurrency fix (2026-10-15)
Goal ACTIVE,0/102 whole sandbox elements complete. Privatev15 UNACCEPTED.
Acceptedv14 and installed immutable r2 are unchanged. Private gameplay edits
only; root documentation receives this append. No agents, commits or pushes.

The original main-thread disc probe1A3C10 and worker open1AA750 can perform
successful OS opens concurrently. v202 reproduces the native bug by holding
both real opens at a declared host schedule barrier before publication: the
old unguarded next_file read issues the same alias, and the original worker
cannot finish. resource-file-concurrency-red-v202.log is the retained red.
No alias, descriptor, bytes or source done result is supplied by the hook.
1A3C10 is an existing whole generated body, now admitted by the public native
resource API; generated source remains119 roots/5357instructions with final
SHA33819293ff2e6962b47f0d8d4d22d03149b3fd25f31c251641132f41d3f5999a.

v203 moves alias allocation/publication into one native file-registry mutex,
with RAII ownership of a successful OS open before the scheduling hook. The
same guard protects all descriptor lookups and each real seek/read/write/
truncate/flush/size/close through the OS operation, so close cannot recycle
a descriptor being used by another native call. Alias exhaustion and duplicate
publication reject explicitly. Quiescent checkpoint/restore file access uses
the guard too. Existing worker-join and active-call barriers remain required.
This is an explicit native OS concurrency policy, not a claim about original
Xbox kernel scheduling, sharing flags, XAPI clobbers or IO latency.

v204 strengthens the regression: after the original main probe has read and
closed its own file, whole original1AAB20 queues a read through the worker's
separate alias. The worker publishes its real count2048 and exact owned map
header bytes, then whole1AB160 completes its close and original done store.
The hook is outside the registry guard and before alias selection, so both
real OS opens can rendezvous without holding the guard or choosing outcomes.
Source queue/free-list/request/done ownership remains original.

Current verification:
- resource-regressions-v205.log:20/20 focused CTests PASS, coherently rebuilt
  from19 targets. Actual commands retained incommands-v205.json. Includes
  memory concurrency, arena/continuation, cache startup, four full map copies,
  activation, signature/failure, GPU boundary, shared-tag processing and race.
- resource-file-concurrency-v204.log PASS; reportresource-parallel-file-v207.json
  records2 real simultaneous opens and2048 exact bytes after main-file close.
- Fresh native IO capturev206, read7/file16 unchanged-original comparisons
  PASS: full CPU/path/all57512002 retained bytes plus complete private files.
- resource-payload-original-v206.json:4 allocator cases PASS with original
  instruction counts5135/5932/456/176 and all57229378 retained bytes each.
- resource-map-transition-prefix-original-v206.json:11476 instructions and
  all57270342 bytes PASS, still stops BEFORE404D10 and supplies no GPU return.
- resource-map-tag-postprocess-original-v206.json:216383 instructions and all
 57270342 bytes PASS through existing Generic/World shared owners,4512tags.
- Total29 unchanged-original comparison cases. Fresh complete native captures
  are retained underwork/sandbox-primary-v15, including allocator/prefix/tag
  copies of the CTest-produced banks. Current source/executable/evidence SHA
  manifestresource-file-registry-acceptance-v207.json and candidate summary.
  No repeated broad109-test, Xbox pixel/PCM, full-game or new delivery claim.

GPU researchv200/v201 authenticates actual SDK cold factory3FF320(45 whole
instructions): singleton40F0C0, source publication411568 at3FF361, actual
initializationCALL3FF37B->405CF0(return3FF380). Initialization is556 whole
instructions and publishes+934/+1C20 beforeCALL404D10; it passes+1C28 to its
miniport constructor.406610 is153-instruction cleanup, not initialization.
Initial fields0/8/770/934/1C20/1C28 are zero; no fabricated nonzero device
layout, ready bit, push cursor, MMIO field or idle result has been admitted.
Reportsgpu-device-factory-primary-v200.json andinitialization-primary-v201.json
retain unchanged owned bytes. Root NativeTu15GraphicsDevice was inspected
read-only: it owns a genuine renderer lease/targets but publishes an opaque
native token at515E90, not the original internal SDK object at411568. Its
current concurrent code was preserved and not copied into this candidate.

Next dependency remains full191890->191EF0->12EDA0->12FBB0: actualD3D404D10
and direct GPU status reads need a real source/native SDK device and command
completion owner. Then full191A50/signature verification and all-map/menu
acceptance. World still stops its worker after boot, manually binds map tags,
and uses three synchronous1301B0/130F68/126139 resource bridges. Persistent
World integration/resource continuation and all102 whole-element local/split/
LAN, original Xbox rendered/audio and fresh installed delivery gates remain.
The previous statement that the native fd registry lacks a mutex is superseded
byv203; the persistent live-world integration limitation is unchanged.

Launcher-info and actual SHA reverified inlauncher-preservation-v207.json:
r2 runtime8be194ad2797f0b727212cc18c56910088f4c85b936288de7718c491396e1b40;
acceptedv14ca8d40a2b9ed609f7dc3705404a7fa81959239799796e4b68c854de18bbdcb61.
No repack or full parity acceptance. Goal classificationPROGRESS. All owned
build/CTest/oracle sessions terminal. No tagged debug logs or opcode patches.


## v208-v226 original GPU control recovery and native CPU comparison (2026-10-15)
Goal ACTIVE,0/102 whole sandbox elements complete. Privatev15 UNACCEPTED.
Acceptedv14 and installed immutable r2 remain unchanged. Private gameplay code
only; root documentation append only. No agents, commits/pushes or root GPU
code imports. Previous turnv207 was verified progress, not a completed goal.

The next map-reset dependency is actual titleCALL12FBDA->D3D404D10, return
12FBDF. Native source control is now a dedicated shared module with two whole
roots404D10(46instructions) and404EC0(9),55 total, authenticated by that literal
caller and internalCALL404D27. No duplicate404D10 body exists in the private
candidate. Current generated final post-template SHA:
7d6ccae6a1800f9cc2fadda5f0bdbeaad458848199f38af5cf8ccbb83b71f089.
Manifestgpu-control-native-v224/generation.json; common generator prints a
PRE-template hash first, so use the final module manifest. The existing
resource module remains119 roots5357instructions with itsv196 final SHA.

Authentic original device production is observed, not guessed. New private
toolcapture_source_gpu_device_boot_tu15.py cold-boots the unchanged owned
TU1.5 reference disc in xemu0.8.136(fc24584), with owned modifiedComplex4627
BIOS, separate HDD overlay/EEPROM, hidden private display, and hardware
breakpoints. No savestate, guest code/data writes, input patches or native
expected results are supplied. The original source3FF320 factory really
executes556-instruction405CF0 and returnsHRESULT0 at3FF380. Original full
.text, factory/initializer bytes, and both reached GPU-control code ranges
are checked against the owned image; source assets and HDD backing preserved.
This is controlled emulator resource evidence, not physical Xbox proof.

Actual original singleton40F0C0 publishes MMIO baseFD000000 at+934/+1C28 and
push-controlFD800000 at+1C20. CPU/push memory pointers come from its real
original allocations. Those values are retained reference data, NEVER seeded
into a production native device. Original cold presentation tuple is:
640,480,6,0,0,1,0,0,1,42,1,0,80000000(hex),0,0,0,0.
The inspected root NativeTu15GraphicsDevice gate uses different refresh and
presentation inputs and an opaque515E90 token. Do not import that gate or
reinterpret the token as original411568 device state. Its real renderer
lease/targets are preserved concurrent work; faithful native binding remains.

Current original observations:
-gpu-control-factory-original-boot-v219: SDKCALL406423->404D10,return406428.
-gpu-control-title-original-boot-v223: titleCALL12FBDA->404D10,return12FBDF.
Each executes25 actual retired instructions in the whole55-instruction closure.
NV2A register tracing observes exactly: PFB410read0, write10000, read0, then
USER40write of the actual original push cursor. Values are produced by real
emulator MMIO while original instructions execute, not supplied GPU readiness.
Normal input/output banks include9376 SDK-device bytes, globals, actual caller
stack, command context and recording data:10064 bytes per case. Original CPU
GP/SSE/eflags, FP controls/status and opcode bytes are retained. Unused x87
80-bit register/tag payload is not captured or compared; no full FPU claim.
prepare_gpu_control_replay_tu15.py independently verifies pinned original
code, complete original CFG edges, copied capture-tool SHA, raw hardware trace
and normal-bank hashes. Unknown MMIO blocks/widths reject, never default zero.

The native module preserves source loads, stores, branch/poll logic and actual
SFENCE via native_mm_sfence. The first attempt revealed a material portability
bug: the common diagnostic store helper reads old memory before writing even
with no observer. A GPU MOV must not add that MMIO read. GPU stores now emit
only the source write; SDK store-observer before is empty because it was not
observed, while normal stack diagnostics retain their existing behavior.
The real original four-operation MMIO stream rejects any extra access. A
separate observer-enabled native case records both original hardware writes
without adding reads. The common store helper and unrelated modules are kept.

Another genuine comparison failure showed hostFNSTCW returning27F while the
original captured logicalCW was23F. The entire closed55-instruction source
closure is checked to contain no FP operation/control/status writer. Its native
wrapper therefore retains incoming logical FP controls/status on return,
instead of substituting a host-normalized CW or unrelated host exception state.
This follows source instruction identity; no expected CPU result is used as an
input. Unused x87 payload stays outside the proof. No source opcode patch.

Verification and corrections:
-v210 test compile issue atstring/JSON equality corrected before execution;
 it is not the meaningful red. v212 compiled regression fails at unowned
 original GPU entry. v213 rejects extra diagnostic MMIO access. v215/v216
 expose raw control-word normalization, corrected by source FP identity.
-v208 initializer capture andv209/v217 preliminary control observations kept;
 current accepted CPU observations arev219/v223, with explicit GPU-code byte
 checks. v214 GDB step stopped again at the title CALL before entering SDK;
 corrected by a separate hardware breakpoint at SDK entry. v220 failed
 before QMP/guest execution when its private display handshake was incomplete;
 tool now consumes the full display-ready line. v221 includes debugger stops
 with unchanged PC; the preparer rejects such non-retired CFG edges. Current
 capture records non-retiring debug stops separately rather than calling them
 original instructions. Never restart on observation timeout; all failures
 were terminal and each owned process/display was stopped by its owner.
-resource-regressions-v225.log23/23 focused CTests PASS after one coherent
20-target build. Actual commands-v225.json. Includes previous20 plus factory,
 actual map-reset GPU control and diagnostic-observer replay cases.
-Native reportsgpu-control-{factory,title,observer}-native-v226.json:3 replays
 of2 distinct original observations PASS; each25 retired instructions, all
10064 captured normal bytes,4 actual observed hardware operations; observer2
 original MMIO writes. Full GP/SSE/flags and logical FP controls/status match.
-Previous29 unchanged-original resource comparisonsv206 remain bounded proof
 for unchanged resource bodies. Fingerprints verify thosev207 source files
 unchanged except CMake adding the new module/tests. Those29 comparisons are
 NOT represented as rerun this turn. No new broad109-test or full-game claim.
-gpu-control-acceptance-v226.json/candidate summary record current source,
 executable and evidence fingerprints. All original and native owned sessions
 terminal; emulator/display/build/test process audit finds none still live.

Next required: real live source/native SDK device, push-command ownership,
GPU/MMIO completion and Vulkan binding. The new CPU module consumes actual
hardware observations in tests; it is NOT bound into the retained live World.
Do not seed original captured device pointers, put/get counters or idle flags,
and do not replace source waits with success.405CF0 initializer is observed
but not implemented natively. Complete191890/191A50/signature verification,
all-map/menu acceptance and persistent resource worker integration remain.
World still stops worker after boot, manually binds tags, and uses three sync
1301B0/130F68/126139 bridges. All102 whole-element local/split/LAN, original
Xbox rendered/audio and fresh installed delivery gates remain required.

Launcher-info and actual hashes preserved inlauncher-preservation-v226.json:
r2 runtime8be194ad2797f0b727212cc18c56910088f4c85b936288de7718c491396e1b40;
acceptedv14ca8d40a2b9ed609f7dc3705404a7fa81959239799796e4b68c854de18bbdcb61.
No repack/new installed delivery or full parity acceptance. Goal turnPROGRESS.

Documentation spelling correction forv226: the actual native SFENCE intrinsic is `_mm_sfence()`.


## v227-v238 real renderer GPU queue/device ownership (2026-10-15)
Goal ACTIVE,0/102 whole sandbox elements complete. Privatev15 UNACCEPTED.
Acceptedv14/installed immutable r2 unchanged. Private gameplay code only; root
receives this documentation append. No agents, commits/pushes or root runtime
edits. Previousv226 is classifiedPROGRESS, not completion or a blocked turn.

A real native GPU completion owner now exists in the private renderer. Two
existing root RendererDeviceLease files were copied unchanged after before/
after/source-copy SHA verification(import-v227.json). The private renderer
retains its actual Vulkan implementation through device uses; retirement
rejects new uses while active native GPU resources remain alive. No other root
renderer/SDK/target code was imported, and concurrent MCC work is preserved.

New RendererGpuQueue is issued only by a genuine live Renderer and exposes
its actual logical/physical device, queue and family through NativeRuntimeAddress.
Native addresses are never truncated into original guest addresses. Acquired
uses and completion operations require the issuing renderer thread. Each
completion creates a real initially unsignaled VkFence and submits a native
ordered marker to the actual VkQueue. Public ready() queriesvkGetFenceStatus;
wait(timeout) usesvkWaitForFences. NOT_READY/TIMEOUT remain pending, and driver
errors throw. No source GPU pointer, put/get value, ready bit or idle status is
published. Marker ordinals are native ownership identifiers, NEVER original
push cursors. Fence scope covers earlier GPU command submissions, not scanout
or presentation-engine release. Existing presentation retirement is preserved.

The device optionally enables queried timelineSemaphore support through Vulkan
1.2 core or the availableKHR extension, preserving the ordinary1.1 renderer
contract when unsupported. This native capability does not alter Xbox source
state or replace an original SDK device initializer. Current live test exercises
the core path;KHR fallback/other platforms remain unverified.

Primary Vulkan authority:
-https://docs.vulkan.org/spec/latest/chapters/synchronization.html
-https://docs.vulkan.org/refpages/latest/refpages/source/vkSignalSemaphore.html
-https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceTimelineSemaphoreFeatures.html
-https://docs.vulkan.org/refpages/latest/refpages/source/vkSetEvent.html
A queue fence observes earlier command submissions; a timeline signal is a real
host operation. The initial host-event fixture was rejected by current validation:
vkSetEvent cannot signal an event waited on by a pending command buffer. It was
replaced with an actual timeline wait/signal, never a disabled check or supplied
readiness result. v230 failed BEFORE GPU work because validation-layer search
paths were missing; the repo's bundled layer/library were then bound explicitly.
v228 had a test compile redeclaration corrected before execution. These are
retained diagnostics, not accepted GPU evidence.

Real hardware verification onNVIDIA GeForceRTX5080:
- The renderer submits ordinary native frames. A separate actual GPU fill of
256 bytes is queued behind an unsignaled timeline value. The completion must
report pending and zero-time wait must return false before host signal. After
realvkSignalSemaphore, it must complete and all GPU-written words must match.
- Actual device/queue identity, foreign-thread rejection, moved-ticket rejection
 and new-use rejection after retirement are checked. No fake renderer/device
 can issue this owner through its public API.
- Enhancedv237 retires the renderer while real GPU work is still blocked. New
 use/status calls reject immediately, the retained real device accepts a host
 signal, and completion cleanup waits for its actual GPU fence. Another256
 GPU-written bytes are checked after retirement. A bounded test helper detects
 incorrect blocking retirement; it never supplies a completion status.
- Controlled native triangle frame pixels are byte-identical before/after the
 queue stimulus. This is native regression evidence, not Xbox pixel parity.
- Khronos validation is enabled and yields zero messages, including complete
 resource/device teardown. Window lifetime remains required through all uses.

Evidence:
-resource-renderer-regressions-v234.log26/26 PASS after a coherent24-target
 build including the actual privatehalo2_native. Includes previous23 resource/
 GPU-CPU cases and3 native renderer/device/material cases. Actual commands and
 targets retained. GPU pending-retirement test was strengthened afterward in
 test code only and passed inv237; other25 tests/production code are unchanged.
-renderer-gpu-queue-live-v234 preserves the earlier complete GPU case; final
 enhanced report/PPMs are inrenderer-gpu-queue-live-v237. No repeated broad109
 or all-game claim. New queue production files unchanged afterv234 build.
-native-runtime-smoke-v235-1-idle.json:20seconds,exit0. Four-view combat report:
30seconds,15 scripted events,exit0. Both execute actual rebuilt runtime SHA
72c1a7692441c354206bb5461f2626b179f5b0b8c54659199613a2d07a491e45.
 Private X11/XTest sessions and isolated save/state paths; only player1 receives
 input, audio sinkdummy, no extra game validation flag. They prove bounded
 runtime regression only, not4 real controllers/LAN/allmaps/menus/playthrough/
 original rendered frames orPCM. Their runtime is not installed r2.
-renderer-gpu-queue-acceptance-v238.json and candidate summary retain current
 source/executable/evidence fingerprints. Original resource119-root/GPU55-
 instruction CPU implementations remain exactly unchanged fromv226.

Next original SDK dependencies are further authenticated inminiport-primary-
v236.json:405D45->4046C0(49whole instructions) forwards allocator flagsBC800000
and64800000 to2DA8B0; do not guess their meaning/backing.405D6F->4080E5 is114
whole instructions and reaches actual kernel ordinals107/44/109/98/47. Existing
export-map labels areKeInitializeDpc,HalGetInterruptVector,KeInitializeInterrupt,
KeConnectInterrupt,HalRegisterShutdownNotification. These names are annotations,
not proof of kernel implementation or permission to return fake registration
success. Allocation, command, MMIO and IRQ/DPC ownership remain required.

The new native queue is NOT an initialized original SDK object or an MMIO
adapter. Source3FF320/405CF0 device production, GPU-visible buffers/commands,
real MMIO completion and IRQ/DPC lifetime binding must be composed next. Do
not seed captured pointers, reinterpret marker ordinals as original push
cursors, bypass source waits or synthesize idle. Whole original404D10/404EC0
CPU code remains available but is NOT bound to this renderer in the live World.
Complete191890/191A50/signature/all-map/menu acceptance remains open. World
still stops its worker after boot, manually binds tags and uses three sync
1301B0/130F68/126139 bridges. Every102-element local/split/LAN, original Xbox
render/audio and fresh installed delivery gate remains part of the goal.

Installed launcher-info/actual SHA preservationv238:
r2 runtime8be194ad2797f0b727212cc18c56910088f4c85b936288de7718c491396e1b40;
acceptedv14ca8d40a2b9ed609f7dc3705404a7fa81959239799796e4b68c854de18bbdcb61.
No repack/new installed delivery or full acceptance. All owned candidate
build/CTest/runtime/display sessions are terminal. Goal turnPROGRESS.


## v243: original SDK allocation prefix and actual GPU storage

Goal remains ACTIVE, unaccepted, with 0/102 whole sandbox elements complete.
This step advances the original graphics dependency of map activation; it does
not complete graphics startup or deliver a new playable build.

The owned TU1.5 XBE authenticates complete bodies 3FF320 (45 instructions),
405CF0 (556), 4046C0 (49), 2DB45A (28) and 2DB49D (18): 696 body instructions.
The generator supplies 695 CPU instruction lowerings. Privileged WBINVD at
40641F remains an explicit required-owner stop, never a no-op. Other SDK,
miniport, kernel, MMIO and command callees remain required. Generation replay
v243 reproduces the compiled C++ byte-for-byte, SHA256
539dac4df4a94604ffea9a766d3dab06ca8a1bff4bc8aaec1fbc3025a2e65866.
Original caller and six-byte ordinal166 import frames are authenticated.

The original factory/initializer/command constructor now executes through
original 2DA8B0/2DB45A and existing original RtlAllocateHeap. It publishes its
own singleton/global, 96-byte control allocation, 524288-byte command buffer
and heap metadata. No device fields or captured Xbox allocation pointers are
seeded. It stops BEFORE the genuine 405D6F call enters 4080E5; no miniport
return, interrupt success or ready/idle state is supplied.

SourceGpuPhysicalMemory owns actual Vulkan buffers/device memory on a genuine
renderer queue/device lease. Source tokens use an explicit native first-fit
interval 03000000..03FFFFFF, not a claim about Xbox kernel placement. Native
64-bit GPU handles stay separate from 32-bit source tokens. Newly allocated
Vulkan bytes are unspecified; actual mapped initial bytes are retained as
platform inputs, and original instructions initialize source storage. See
[Vulkan allocation](https://docs.vulkan.org/refpages/latest/refpages/source/vkAllocateMemory.html)
and [buffer binding](https://docs.vulkan.org/refpages/latest/refpages/source/vkBindBufferMemory.html).
SourceRuntimeMemory owns a COPY, not the VkMappedMemory pointer. Explicit
publish/refresh waits for real queue retirement and copies/flushes/invalidates
storage. This is not immediate CPU/GPU aliasing, an NV2A cache/MMIO adapter,
or execution of original push words. Source/kernel cache-policy parity stays
open. Operations and teardown require the issuing renderer thread and retained
bank/window lifetimes; real queue retirement protects buffer destruction after
logical renderer retirement.

Verification v242/v243:
- Unchanged original executable prefix: 520 instructions, full GP/XMM/defined
  flags/logical FP control and status/FS state, exact path, and ALL 6410306
  retained bytes match. New allocation placement is independently computed;
  only actual native allocation input bytes are replayed. Original code is
  never patched. Native single-thread heap locks and allocation notifications
  are explicit platform policy, not Xbox kernel/clobber proof.
- Real NVIDIA GeForce RTX 5080 copies 256 bytes between source-backed GPU
  buffers, and bank refresh observes exact GPU-written bytes. A further
  original 2DB49D constrained/aligned allocation reaches its distinct import
  frame; this extra ABI case is not part of the 520-instruction comparison.
  Actual native token exhaustion, invalid return rejection without state
  changes, foreign-thread rejection and renderer-retirement rejection pass.
- Coherent 25-target build and 27/27 resource/heap/GPU/renderer CTests pass.
  GPU fixtures enable Khronos validation, including teardown, with zero
  messages. v239's initial fixture failed because its native render mesh had
  zero materials; the valid fixture was repaired, not the renderer guard.
- Existing resource119-root and GPU-control55-instruction CPU source remains
  unchanged; only the private CMake lists differ among previous source pins.
  The 29 original resource oracle cases were not rerun: v206/v226 scoped
  evidence remains historical. Current CTests exercise their native paths.
- Private halo2_native stays byte-identical to v238/v235, SHA256
  72c1a7692441c354206bb5461f2626b179f5b0b8c54659199613a2d07a491e45.
  Those earlier bounded 1/4-view runtime smokes are not repeated or expanded
  into new live GPU integration, controller, LAN, pixel or PCM proof.

Evidence: gpu-device-acceptance-v243.json, gpu-device-live-v242,
gpu-device-original-comparison-v242.json, gpu-device-generation-replay-v243,
gpu-device-regressions-v242.log and launcher-preservation-v242.json under
work/sandbox-primary-v15. Current source/executable/evidence hashes are retained.
All owned build/CTest/Xvfb sessions are terminal.

Next is actual 4080E5 miniport/kernel107/44/109/98/47 ownership, its IRQ/DPC
lifetime and callbacks, GPU command/MMIO/cache ownership including 40641F, and
binding the real source device into live 404D10/404EC0/map transition. The root
opaque515E90 graphics adapter remains distinct from the SDK singleton411568.
Full191890/191A50, persistent World resource worker/three synchronous bridges,
all102-element local/split/LAN acceptance, original rendered/audio comparison,
all maps/menus and fresh installed delivery remain required. No fake idle,
IRQ success, captured fields or skipped source waits are acceptable.

Installed r2 remains SHA256
8be194ad2797f0b727212cc18c56910088f4c85b936288de7718c491396e1b40;
accepted v14 remains
ca8d40a2b9ed609f7dc3705404a7fa81959239799796e4b68c854de18bbdcb61.
Actual launcher-info selection and both hashes were verified. No repack or
new installed delivery. Goal turn classification: PROGRESS.


## v253: genuine kernel DPC initialization advances miniport startup

Goal remains ACTIVE and unaccepted: 0/102 whole sandbox elements complete.
Previous turn v243 was verified PROGRESS. Current work advances that original
SDK startup path; full device initialization and installed delivery remain open.

The native device module now authenticates ten complete original game bodies,
903 instructions: previous five plus 4080E5/408245/408270/4082B3/408334. There
are 901 CPU instruction lowerings and explicit required-owner stops for
40641F WBINVD and 408211 OUT DX,AL. External SDK, MMIO and kernel owners are
still required. Final generated source SHA256:
7c63c4eb6e1fea49f1e0b0ac9170425202bc6abaafe8f9125b80a7bcdf64a1e6.

Original miniport code calls ordinal107 at4080FC. Its actual eight-instruction
body was observed at8001A384 in the owned modified Complex4627 kernel under
pinned xemu. Thirty code bytes SHA256:
852645af25732043b62746a4193074038c4d91fb63c88e82219570f62ed19342.
This is a kernel capture authority, not XBE text or proof for every kernel
build/physical Xbox. The common generator's game-image selection binds the
caller ABI only; kernel code has a separate BIOS/code/capture pin. All eight
instructions compile to native CPU operations; no binary emulator/decoder
runs in production. Generated kernel C++ SHA256:
3d304aa445c43018ca24c16004462641674347c29ff86276e4ee3280a624d8d1.

The authenticated GPU call adapter validates its six-byte import return,
import cell and actual source arguments. The kernel body owns GP/flags,
stdcall cleanup and exact store order: AND byte+2, routine+12, type word+0,
context+16. It leaves non-GP/flags ABI state intact, including opaque FP/MMX
state. Missing caller FP controls remain required, with no selected default.
This initializes the DPC record only; delivery/scheduling, callback408CC0,
ISR408BA0, shutdown409570 and genuine IRQ registration/lifetimes remain open.
The existing process-startup compatibility DPC adapter was not silently
relabelled as this recovered kernel body.

The original factory/device allocation chain now enters 4080E5, executes its
DPC initialization/list/record producers and calls 408245. The original first
store publishes FD000000 in the miniport object. Native comparison stops
BEFORE40824B reads FD001804 (PBUS PCI config). No register value, MMIO success,
miniport return, interrupt success or idle/completion state is supplied.

Verification:
- Expanded original composition:556 instructions, exact GP/XMM/defined flags,
  logical FP controls/status and FS, full path and ALL6410306 retained bytes
  match. The owned XBE executes unchanged; the composed oracle executes the
  captured original kernel instructions too. Native physical allocation
  placement/copy policy remains explicit, as recorded inv243.
- Actual original kernel replay:8 instructions, GP/XMM/defined flags/FP modes
  and status, exact store order and ALL540 object/stack bytes match in two
  independent original boots. A separate native opaque FP/MMX sentinel is
  preserved; unused original physical FP payloads were not captured/compared.
- Coherent26-target build and28/28 focused CTests pass. Actual GPU storage
  transfer/retirement checks remain included, with Khronos validation enabled
  and zero messages. This is host GPU evidence, not original rendered parity.
- Final generation replay reproduces both compiled C++ files byte-for-byte.
  The fresh v251 boot also verifies all750 miniport/helper bytes against the
  owned XBE before and after setup, alongside existing full text and factory/
  initializer checks. Observed256-byte kernel export windows remain unchanged;
  that is not a claim about every kernel byte/function.
- Reference miniport completes with actual return1 and317 NV2A register
  events. Ordinal44/109/98/47 frames, object bytes and real reference outputs
  are retained. Only the first seven register accesses are instruction-
  attributed; other events remain unattributed observations. None are native
  provider outputs. Debugger scheduling and modified BIOS/xemu limits remain.
- First v244 observation failed on an assumed contiguous kernel PE header at
  inaccessible80030020. It is rejected evidence. The capture tool instead
  follows actual resolved import cells; no kernel-layout explanation is guessed.
  Successful boots use private EEPROM/HDD overlays, preserve original inputs,
  and perform no guest code/data writes. All owned sessions are terminal.

Pinned primary NV2A sources are retained ingpu-miniport-register-authority-v249.
[PBUS](https://raw.githubusercontent.com/xemu-project/xemu/fc24584ce88f0915ad7f04775bb7712c2e3f49ee/hw/xbox/nv2a/pbus.c)
uses a PCI configuration owner; source register1804 is not an arbitrary status
constant. [PFB](https://raw.githubusercontent.com/xemu-project/xemu/fc24584ce88f0915ad7f04775bb7712c2e3f49ee/hw/xbox/nv2a/pfb.c)
gets capacity from the VRAM owner but returns constant zero for WBC. That
compatibility behavior cannot establish native GPU completion or justify fake
idle. [NV2A IRQ routing](https://raw.githubusercontent.com/xemu-project/xemu/fc24584ce88f0915ad7f04775bb7712c2e3f49ee/hw/xbox/nv2a/nv2a.c)
also depends on actual masked pending sources. These LGPL reference files are
retained with their headers; no register provider was copied into production.

Evidence:gpu-miniport-acceptance-v253.json, gpu-miniport-original-boot-v251,
gpu-miniport-live-v250, gpu-miniport-original-comparison-v252.json,
gpu-kernel-dpc-original-report-v252.json, gpu-miniport-generation-replay-v252,
gpu-miniport-register-trace-v252.json andgpu-miniport-regressions-v250.log,
underwork/sandbox-primary-v15. Source/executable/evidence hashes are retained.

Next is actual native NV2A PCI/MMIO/VRAM, command/cache and IRQ/DPC delivery
ownership. Never seed captured fields/status or return fake interrupt success.
Then bind source404D10/404EC0/full191890/191A50 and the initialized SDK device
into live World; its stopped worker/three synchronous resource bridges remain
open. Every102-element local/split/LAN, original render/audio, all-map/menu and
fresh installed delivery gate remains part of the same full goal.

Existing resource119-root/GPU-control55-instruction source is unchanged; the
29 original resource oracle cases were not rerun (v206/v226 remain scoped
historical evidence). Current native CTests pass those paths. Private runtime
stays byte-identical to v238/v235 SHA256
72c1a7692441c354206bb5461f2626b179f5b0b8c54659199613a2d07a491e45;
previous bounded runtime smokes are not repeated or broadened into live GPU,
full controller/LAN/pixel/PCM proof. Installed r2 remains
8be194ad2797f0b727212cc18c56910088f4c85b936288de7718c491396e1b40;
accepted v14 remains
ca8d40a2b9ed609f7dc3705404a7fa81959239799796e4b68c854de18bbdcb61.
Actual launcher-info/SHAs were verified. No repack or new installed delivery.
Goal turn classification: PROGRESS.


## v261: original kernel PCI transport and its boot producer

Goal remains ACTIVE, unaccepted, with0/102 whole sandbox elements complete.
The interrupted work is verified PROGRESS, not a stopped job: the current
source/binaries and29/29 completed CTests were rechecked after resuming.
All owned test/build/Xvfb/xemu sessions are terminal.

The first genuine kernel DPC call retains an early loaded kernel image while
its NT headers and INIT section are readable. Read-only boot capturev254 saves
all five actual virtual sections and export bindings from owned modified
Complex4627 under pinned xemu. It preserves original ROM/BIOS/EEPROM/HDD inputs,
uses a private overlay, and performs no guest code/data writes. This is not
physical Xbox or every retail kernel build evidence. Late header inaccessibility
is not used to guess a different kernel layout.

The capture identifies ordinal46 HalReadWritePCISpace at80015178, six transfer
helpers, and complete boot PCI initializer80063908. Its GPU command producer
is the original OR7 at80063C75, not a saved command-register value. Eight
complete bodies/450 instructions compile to native C++; seven transport
bodies contain141 instructions and the whole boot initializer contains309.
No production instruction emulator/decoder is added. Generated source SHA256:
d5e4f5d67e4a080d9dec8a4370a32e795c5e9301347363a16032449936c5e1cb.
Generation replay reproduces it byte-for-byte. Only the original40-byte
read-only dispatch/index tables become native bank inputs. Captured dynamic
kernel data/thread objects/hardware-flags outputs are NOT initialized into
native memory. The game-image selector binds caller ABI only; the kernel
capture/BIOS/code authority is separate from XBE text.

SourceGpuPci supplies a native software NV2A reset-header plane tied to a real
renderer/device lease. Its identity/class/revision and PCI write masks come
from pinned primary
[NV2A initialization](https://raw.githubusercontent.com/xemu-project/xemu/fc24584ce88f0915ad7f04775bb7712c2e3f49ee/hw/xbox/nv2a/nv2a.c)
and
[PCI reset/write policy](https://raw.githubusercontent.com/xemu-project/xemu/fc24584ce88f0915ad7f04775bb7712c2e3f49ee/hw/pci/pci.c).
It explicitly starts command0, with no booted7 imported. This is the declared
native reference profile, not host-card identity or original physical PCI
behavior proof. Known bytes0..15 include identity/class, command/status and
cache/latency fields; the primary profile leaves latency read-only. BAR/VRAM,
vendor registers and other board devices remain required, never default0.

Original IN/OUT operations call owned CF8/CFC port services. CLI/STI retain
source IF and own a genuine native PCI transaction mutex; this is not Xbox
interrupt delivery or an STI interrupt-shadow implementation. Non-GP/flags
ABI state stays untouched by the complete integer bodies. Foreign, destroyed
and retired callbacks reject. Retirement detaches the renderer lease on the
issuing thread, so a rejected borrowed callback cannot retain the GPU lease
until later foreign-thread destruction. Concurrent rejected-callback teardown
is exercised with real renderer ownership and validation.

Whole original80063908 is retained, not replaced by its GPU-only tail. Native
execution stops at the first required MCPX PCI device bus0/device1, before the
GPU command producer. The failed source call retains IF clear and its actual
transaction/frame until explicit native teardown. No board-owner success,
GPU enable7 or kernel hardware flags are fabricated. The original SDK frontier
therefore remains before40824B readsFD001804; reset command0 is not admitted
as completed original board initialization.

Verificationv260/v261:
-210 read/write cases cover supported header offsets0..15, zero-length and
 byte/word/dword/remainder transfers. Unchanged original kernel code executes
 independently with the declared native reset/configuration policy. All19018
 retired instructions, GP/XMM/defined flags plusIF, logical FP controls/status,
 paths, I/O events, final PCI configuration and ALL1742160 retained bytes match.
 All141 static transport instructions across all seven transport roots are
 covered. Full boot is not part of that comparison; its genuine missing-owner
 boundary is separately tested. Original physical PCI/IRQ/MMIO is not proved.
-Coherent27-target build and29/29 focused resource/heap/kernel/GPU/renderer
 CTests pass. GPU fixtures enable Khronos validation with zero messages,
 including concurrent retirement/rejection and real device teardown.
-Final oracle and source/evidence fingerprints are revalidated after the
 intentional interruption. v255's initial compile error in local auto-type
 deduction was corrected; primary write-mask inspection also corrected latency
 ownership before the accepted210-case matrix. Neither failed/different-scope
 work is passed off as whole-game acceptance.

Evidence underwork/sandbox-primary-v15:kernel-pci-acceptance-v261.json,
gpu-kernel-early-original-boot-v254, kernel-pci-generation-replay-v258,
kernel-pci-live-v260, kernel-pci-original-comparison-v261.json,
kernel-pci-coverage-v261.json, kernel-pci-regressions-v260.log and
launcher-preservation-v260.json. Current48-source/27-executable and evidence
fingerprints are retained. Source/kernel table generation is reproducible.

Next is actual MCPX/board PCI configuration ownership, port80CD and the kernel
hardware-flags producer, followed by BAR/VRAM/vendor registers and GPU MMIO,
DMA/commands/cache and genuine IRQ/DPC delivery. Then bind source404D10/404EC0,
full191890/191A50 and the initialized SDK into live World. Its stopped resource
worker/three synchronous bridges, all102-element local/split/LAN, original
render/audio comparison, all maps/menus and fresh installed delivery remain
required. Full goal scope is unchanged.

Prior resource119-root/GPU-control55-instruction and556-instruction SDK prefix
implementations are unchanged. The earlier29 resource oracle cases and bounded
runtime smokes are not rerun or broadened. Private halo2_native remains identical
tov238/v235 SHA256
72c1a7692441c354206bb5461f2626b179f5b0b8c54659199613a2d07a491e45.
Installed r2 remains
8be194ad2797f0b727212cc18c56910088f4c85b936288de7718c491396e1b40;
accepted v14 remains
ca8d40a2b9ed609f7dc3705404a7fa81959239799796e4b68c854de18bbdcb61.
Actual launcher-info and all hashes were checked. No repack, fresh launcher
build or live World GPU integration is claimed. Goal classification:PROGRESS.


## v269: original boot produces native LPC routing/control state

Goal remains ACTIVE/unaccepted,0/102 whole sandbox elements complete. Previous
turnv261 was verified PROGRESS. This step advances the complete original board
initializer, while full board/GPU startup and installed delivery stay open.

SourceMcpxLpc owns the known native reference-profile configuration plane:
header00..0F, routing64..6F and control81. The nForce LPC identity/reset/write
schema and routing contract come from pinned primary
[xbox_pci.c](https://raw.githubusercontent.com/xemu-project/xemu/fc24584ce88f0915ad7f04775bb7712c2e3f49ee/hw/xbox/xbox_pci.c),
SHA22963a0ffab9a68ca8ba719419a129c87cafd99e32f13e5e13056ac808617f82.
It is explicitly a native software reference profile, not captured running
kernel state or physical Xbox proof. Unknown BAR/vendor/ROM registers stop;
control81 behavior beyond that reference configuration contract is not proved.
The owner is not copied between native threads. SourceGpuPci accepts explicit
shared LPC ownership for bus0/device1; absence remains required. GPU/LPC leases
are detached on the issuing thread before borrowed callback state can expire.

The genuine complete80063908 initializer now runs through five LPC transfers.
Original instructions produce64=00000B0C,6A=03,6C=0E065491 and81=08. Those are
outputs, not seeded native initialization. Root PCI/internal/PIRQ/ACPI routing
uses this actual state. Real PIC/ISR delivery requires a supplied owner; no
interrupt registration success or pending status is fabricated. Disabled routes
follow the reference drops. Unimplemented ROM control remains required.

Execution advances from missing LPC to the next actual IDE PCI owner0:9.
The native/original comparison stops BEFORE800150E4 IN EAX,DX on that device,
with the completed prior writes and source IF-clear transaction/frame retained.
GPU command remains0: its later original OR7 is not reached or imported. The
original SDK frontier still stops before40824B readsFD001804. Complete board
initialization cannot be inferred from this prefix.

Verificationv267/v268:
-386 retired original kernel instructions plus the final unretired required
 instruction attempt, exact CPU/path/I/O/PCI/LPC state and ALL8232 retained
 bytes match. Original kernel code executes unchanged; the oracle independently
 detects the unowned IDE target before I/O, without returning read data.
-210 transport regression cases still match:19018 instructions and ALL1742160
 retained bytes. Source kernel450/SDK556 CPU implementations are unchanged.
-4560 declared signal cases across60 configurations match independently
 compiled, verbatim pinned primary routing functions. Native tests cover disabled
 paths, active routes and explicit missing PIC/ROM/unknown-register/slot/thread
 boundaries. Signals and recording sinks are diagnostic input; actual World
 IRQ scheduling/delivery and physical hardware remain unproved.
-Coherent28-target build and30/30 resource/heap/kernel/GPU/renderer CTests pass.
 Real GPU fixtures use Khronos validation with zero messages. New pure LPC
 routing checks do not create a fake graphics or interrupt device.

Evidence underwork/sandbox-primary-v15:mcpx-lpc-acceptance-v269.json,
mcpx-lpc-live-v267/board-prefix, mcpx-lpc-board-original-comparison-v268.json,
mcpx-lpc-transport-original-comparison-v268.json,
mcpx-lpc-routing-reference-v265 (original source header/license retained),
mcpx-lpc-routing-native-report-v267.json, mcpx-lpc-regressions-v267.log and
launcher-preservation-v268.json. Current52-source/28-executable/evidence hashes
are retained. All owned Ninja/CTest/Xvfb sessions are terminal.

The next IDE configuration authority is pinned inmcpx-ide-authority-v268;
its owner is not implemented. Then remaining NIC/APU/ACI/USB/AGP/board devices,
port80CD/kernel hardware-flags producer, BAR/VRAM/vendor/ROM owners and actual
PIC/ISR/IRQ/DPC/commands/cache remain required. Live source404D10/404EC0/full
191890/191A50, persistent World worker/three synchronous bridges and every102-
element local/split/LAN/original-render/audio/all-map/menu/fresh-delivery gate
remain part of the same full goal.

Private halo2_native is still identical tov238/v235 SHA
72c1a7692441c354206bb5461f2626b179f5b0b8c54659199613a2d07a491e45.
The earlier bounded runtime smokes and29 resource oracle cases are not rerun or
broadened. Installed launcher-info still selects immutable r2 SHA
8be194ad2797f0b727212cc18c56910088f4c85b936288de7718c491396e1b40;
accepted v14 remains
ca8d40a2b9ed609f7dc3705404a7fa81959239799796e4b68c854de18bbdcb61.
Actual selection/SHAs were verified. No new launcher build or live GPU binding.
Goal turn classification:PROGRESS.


## v272 — original board IDE configuration advances to NIC (PROGRESS)

Full goal remains ACTIVE/unaccepted:0/102 whole sandbox elements complete. This
turn implements the optional native IDE PCI configuration plane in the private
v15 snapshot. Its authority is the pinned xemu Xbox piix.c reset/class/realize
schema, pci.c generic masks/BAR rules, and retained original kernel4627 code.
This compatibility profile is not physical MCPX storage proof. No captured
running configuration, ready status, disk sector or GPU command7 is seeded.

SourceMcpxIdePci owns the256 configuration bytes, immutable10DE:01BC/B1/0101:80
identity, status0280 capabilities, conventional command/cache/interrupt masks,
status W1C policy, vendor bytes and the actual16-byte BAR4 sizing/alignment
configuration. IO command/zero/wrap checks decode an assignment; they do not
create disk or DMA execution. SourceGpuPci routes bus0/device9/function0 only
when this owner is explicitly attached; absent ownership keeps the old stop.
Operations enforce the issuing thread and validate complete extents before
writes. Legacy ATA ports, disk backing, DMA transfers and IRQ delivery still
require their real owners and cannot supply invented data or completion.

The complete original80063908 boot root now produces IDE command0005,
BAR4=0000FF61 (decodedFF60), vendor50=2, vendor58=20202020 and
vendor60=C0C0C0C0, then stops BEFORE800150E4 IN EAX,DX for the next required
NIC bus0/device4/function0. Its original HAL read/write paths run unchanged:
3791 retired instructions plus1 unretired attempt,282 IO events and ALL8232
retained bytes match original-kernel execution against an independently
implemented declared configuration policy. GP/XMM/defined flags+IF/logical FP,
full instruction path, PCI/LPC/IDE state and every retained byte match. The
oracle independently detects the next unowned device before returning data.
GPU command remains0; neither full board boot nor original SDK initialization
is complete. The original SDK frontier is still40824B readingFD001804.

Verificationv270/v271:
-210 original HAL transport cases still match19018 instructions and ALL1742160
 bytes. The unattached-IDE LPC prefix still matches386 instructions/8232 bytes.
-New focused checks cover reset, BAR sizing/alignment/command/wrap, immutable
 header/unregistered BAR/ROM, status capabilities, vendor timing writes, atomic
 invalid extents and foreign threads. Unowned storage-port access rejects.
-Coherent29-target build and31/31 resource/heap/kernel/GPU/renderer CTests pass.
 Real GPU fixtures retain Khronos validation with zero messages. Captures are
 byte-identical after the coherent build. All owned sessions are terminal.
-SDK/kernel/resource generated CPU bodies are unchanged. The earlier broad109
 suite,29 resource oracle cases and bounded idle/combat smokes are not rerun or
 extended into whole-game/pixel/audio/physical-device acceptance.

Evidence:work/sandbox-primary-v15/mcpx-ide-acceptance-v272.json,
mcpx-ide-live-v270/ide-board-prefix, mcpx-ide-board-original-comparison-v270.json,
mcpx-ide-transport-original-comparison-v270.json, mcpx-ide-regressions-v271.log,
mcpx-ide-authority-v268 and launcher-preservation-v271.json. Current55-source
and29-executable fingerprints are retained. The next NIC primary reference is
pinned inmcpx-nic-authority-v271; its native owner is not yet implemented.

Remaining NIC/APU/ACI/USB/AGP/board, port80CD/hardware-flags, storage PIO/DMA,
NV2A BAR/VRAM/vendor/ROM, actual PIC/ISR/IRQ/DPC/commands/cache, live404D10/
404EC0/full191890/191A50, persistent World worker/three synchronous bridges,
and the full102-element local/split/LAN/original-render/audio/all-map/menu/
fresh-delivery gates remain required. This does not resolve the reported full
game/menu/map launch failure or produce a new playable launcher build.

Private halo2_native stays SHA
72c1a7692441c354206bb5461f2626b179f5b0b8c54659199613a2d07a491e45.
Installed launcher-info selects immutable r2 SHA
8be194ad2797f0b727212cc18c56910088f4c85b936288de7718c491396e1b40;
accepted v14 remains
ca8d40a2b9ed609f7dc3705404a7fa81959239799796e4b68c854de18bbdcb61.
Actual selection and hashes were verified. No installed update/live GPU binding.
Goal turn classification:PROGRESS.


## v276 — IDE/NIC boot configuration and restored test launcher (PROGRESS)

Full goal remains ACTIVE/unaccepted: 0/102 whole sandbox elements complete.
The private snapshot now owns optional LPC, IDE and NIC configuration planes.
SourceMcpxNicPci uses pinned primary nvnet.c's declared 10DE:01C3/B1/0200 reset
profile, immutable interrupt pin A, generic PCI masks, 1024-byte memory BAR0
and 8-byte I/O BAR1. Both BARs enforce sizing, alignment, command-enable,
zero-address and wrap rules. This is a compatibility configuration profile,
not physical MCPX/network proof. No packet, link/PHY-ready state, DMA or IRQ
completion is invented. SourceGpuPci only routes bus0/device4/function0 when
this owner is attached, and releases attached owners on the issuing thread.

The original whole 80063908 boot root now produces NIC command0007,
BAR0=FEF00000, BAR1=0000E001 and interrupt line4 through its own original
read/write transport. It then stops BEFORE800150E4 IN EAX,DX at the next
required USB bus0/device2/function0. 7195 retired original instructions plus
one unretired attempt match the native CPU/path/I/O/PCI/LPC/IDE/NIC state and
ALL8232 retained bytes. The oracle independently detects the missing device
before I/O; original kernel code is unchanged. Earlier LPC/IDE prefixes still
match386/3791 instructions respectively. All210 HAL transport cases match
19018 instructions and ALL1742160 bytes. GPU command stays0, full board boot
remains incomplete and the original SDK frontier stays40824B -> FD001804.

A coherent30-target build and32/32 selected resource/heap/kernel/GPU/renderer
CTests pass. Native NIC checks cover both BARs, reset/immutable header/pin,
status, vendor byte lanes, atomic invalid extents, foreign threads and required
actual packet/PHY I/O boundaries. Real GPU fixtures retain zero Khronos
validation messages. Source SDK/kernel/resource generated CPU bodies are
unchanged. Native captures reproduce after the coherent build. The prior109
suite and29 resource-oracle cases are not rerun or broadened. All owned
Ninja/CTest/runtime/Xvfb sessions are terminal.

Delivery verification found the previously selected immutable r2 executable
missing after the earlier v271 preservation check. The cause is unknown and
no exact retained copy was found. A separate experimental package now lives
at .local/sandbox-test-v15-20261015-r3; Halo2-Sandbox-Test selects its verified
runtime SHA72c1a7692441c354206bb5461f2626b179f5b0b8c54659199613a2d07a491e45.
This is the already built current private binary, not a claim that standalone
IDE/NIC owners have been connected to live World GPU initialization. Its
frozen shader hashes and owned Xbox inputs are verified by the launcher.
Fresh one-view idle20seconds and four-view combat30seconds pass, including
firing/reload/movement inputs and clean requested close. The actual launcher
entrypoint also executes that exact child SHA and exits0 after12 Lockout
frames. These are bounded native checks, not all-map/menu/controller/LAN/
original-framebuffer/PCM or whole-game acceptance. The old wrapper is retained
in work/sandbox-primary-v15/launcher-wrapper-before-r3-v275.txt.

Evidence: work/sandbox-primary-v15/mcpx-nic-acceptance-v276.json,
mcpx-nic-live-v273/nic-board-prefix, mcpx-nic-board-original-comparison-v273.json,
mcpx-nic-transport-original-comparison-v273.json, mcpx-nic-regressions-v274.log,
installed-r3-idle-v275.json, installed-r3-combat-four-v275.json and
launcher-delivery-r3-v275.json. Current58-source/30-executable and installed
package fingerprints are retained. Next USB primary OHCI references are pinned
in mcpx-usb-authority-v274; its native owner is not yet implemented.

Remaining USB/APU/ACI/AGP/board, port80CD/hardware-flags, actual storage and
network I/O/DMA/IRQ, NV2A BAR/VRAM/vendor/ROM, real PIC/ISR/IRQ/DPC/commands/cache,
live404D10/404EC0/full191890/191A50, persistent World worker/three synchronous
bridges, and full102-element local/split/LAN/original-render/audio/all-map/menu
acceptance remain required. Accepted v14 remains unchanged at SHA
ca8d40a2b9ed609f7dc3705404a7fa81959239799796e4b68c854de18bbdcb61.
Goal turn classification: PROGRESS; installed experimental delivery updated;
full goal remains active and incomplete.


## v281 — both original USB PCI configuration paths advance to ACI (PROGRESS)

Full goal remains ACTIVE/unaccepted: 0/102 whole elements complete. The private
snapshot now owns optional, distinct USB0/USB1 PCI configuration resources.
Pinned primary Xbox OHCI class/realize/board creation and generic PCI masks
specify 10DE:01C2/B1/0C03:10, interrupt pin A and a 256-byte memory BAR0. This is
a declared compatibility configuration profile, not physical OHCI/controller
proof. OHCI registers, device enumeration, transfers, DMA and IRQ scheduling/
delivery still require real owners; no ready/connected/completion state is
supplied. SourceGpuPci::Board attaches explicit typed owners at bus0/device2
and device3/function0. Old constructor interfaces remain available. Aliasing
one owner across both controllers rejects; an absent device cannot use the
other's configuration. Unknown buses/functions remain required. Issuing-thread
checks and retired weak-callback/resource lifetimes remain enforced.

The unchanged whole original80063908 root now produces both USB configurations:

- USB0 command0007, BAR0=FED00000, interrupt line1, vendor50=0000000F.
- USB1 command0007, BAR0=FED08000, interrupt line9, vendor50=00000030.

It stops BEFORE800150E4 IN EAX,DX for the next required ACI bus0/device6/function0.
14125 retired original instructions plus one unretired attempt,1070 IO events
and ALL8232 retained bytes match native GP/XMM/defined flags+IF/logical FP,
instruction path and PCI/LPC/IDE/NIC/USB/memory state. The independent oracle
executes unchanged retained kernel code and detects the absent next owner
before returning data. USB0-only stops at device3 after10660 instructions.
Earlier LPC/IDE/NIC prefixes still match386/3791/7195 instructions, and210 HAL
transport cases match19018 instructions/ALL1742160 bytes. GPU command remains0;
full board boot and original SDK initialization remain incomplete. SDK still
stops at40824B readingFD001804. No captured configuration or GPU command7 is seeded.

Verification: coherent31-target build;33 unique selected CTests pass with zero
skips. The initial run passed32 and failed the arena check because its original
stores.bin had disappeared. The unchanged authenticated oracle was rerun:
1065823 original TU1.5 instructions regenerate1049656 ordered stores, yielding
exactly26241400 bytes and recorded SHA
73ae9913a4e1be6066fff2fb3fad28cc156b2ca4f4e6eb0371a8c3cdb06a79ba.
Only this exact stream was restored; the old fixture report and test assertions
were preserved. The failed arena check then passes. Both original failure and
focused rerun logs are retained in the aggregate evidence. This is not a claim
that all33 passed in one run. New checks cover BAR sizing/alignment/command/
zero/wrap, immutable fields, independent controller/vendor lanes, invalid-write
atomicity, foreign threads, wrong bus/function, absent-owner routing and owner
release with concurrent retired callbacks. Real GPU fixtures retain zero
Khronos validation messages. Captures match after the coherent build. Source
SDK/kernel/resource generated bodies are unchanged. Broad109 tests and the
prior29 resource-oracle cases are not rerun. All owned sessions are terminal.

Installed r3 still selects unchanged verified runtime SHA
72c1a7692441c354206bb5461f2626b179f5b0b8c54659199613a2d07a491e45.
Its wrapper/package/shader/input verification passes. Earlier v275 bounded
idle/combat/entrypoint proof is retained, not repeated or broadened. No new
installed binary or live World GPU binding was produced this turn.

The historical accepted-checkpoint-v14.json also disappeared after its last
successful v275 verification (previous SHA
ca8d40a2b9ed609f7dc3705404a7fa81959239799796e4b68c854de18bbdcb61).
No exact retained copy was located. Cause is unknown. It remains a recorded
historical evidence gap; no substitute or recreated acceptance is claimed.
Current original-code comparisons and r3 delivery fingerprints remain directly
verified. Goal status stays active; meaningful ACI/APU/AGP/integration work is
available despite the missing historical checkpoint.

Evidence: work/sandbox-primary-v15/mcpx-usb-acceptance-v281.json,
mcpx-usb-live-v278/usb-both-board-prefix, mcpx-usb-both-original-comparison-v278.json,
mcpx-usb-transport-original-comparison-v278.json,
mcpx-usb-regression-aggregate-v280.json, arena-reference-restoration-v280.json,
launcher-preservation-v280.json and checkpoint-reference-missing-v280.json.
Current62-source/31-executable/installed fingerprints are retained. The next
ACI source is pinned in mcpx-aci-authority-v279; native ownership is not yet
implemented. Its primary reference registers only BAR2, with BAR0/1 registration
commented out; physical ACI I/O semantics must not be guessed from that model.

Remaining ACI/APU/AGP/board, port80CD/hardware-flags, actual USB/storage/network
owners, NV2A BAR/VRAM/vendor/ROM, actual PIC/ISR/IRQ/DPC/commands/cache, live
404D10/404EC0/full191890/191A50, persistent World worker/three synchronous
bridges, and all102-element local/split/LAN/original-render/audio/all-map/menu/
full installed acceptance remain required. Goal turn classification: PROGRESS.


## v286 — measured PCI geometry, ACI configuration and retail trait audit (PROGRESS)

Full goal stays ACTIVE/unaccepted: 0/102 whole elements complete. The private
snapshot now attaches an explicit ACI configuration owner at bus0/device6/
function0. Codec registers, PCM/DMA, IRQ delivery and power-transition owners
remain required. Original kernel code, GPU command7 and live GPU ownership
are not bypassed or seeded.

New first-party [DVT4 hardware BAR probes](https://xboxdevwiki.net/index.php?title=PCI&oldid=6609)
identify two ACI IO BARs and its memory BAR. The independent extraction excludes
captured operating addresses, command/status and vendor state. It supplies
only structural BAR probes and immutable header types. This is 5558-debug-ROM
hardware evidence, not retail TU1.5 reset/mask or full configuration proof.
The emulator aci.c omits both IO BAR registrations; copying that omission would
lose the original source's programmed IO assignments. SourceMcpxAciPci instead
uses the measured IO256/IO128/memory4096 geometry and immutable multifunction
header80. Remaining reset/write masks are explicitly conventional native
policy; their retail behavior is still unresolved.

The same hardware probe corrects USB BAR0 to4096 bytes. The earlier256-byte
value described xemu's implemented OHCI register window, not its hardware BAR.
Sizing/alignment/wrap logic and independent geometry checks now use the
measured aperture. Both existing original USB boot assignments are unchanged.
No controller, codec, DMA, PCM or IRQ state is invented by these configuration
resources. Unknown ACI functions and unowned IO accesses reject. Attached
ACI/USB resources are released on the issuing thread despite retained/racing
weak callbacks.

The whole unchanged original80063908 kernel root produces ACI command0007,
BAR0=0000D001, BAR1=0000D201, BAR2=FEC00000 and interrupt line6. It then stops
BEFORE800150E4 IN EAX,DX at required APU bus0/device5/function0.17530 retired
original instructions plus one unretired attempt and ALL8232 retained bytes
match native CPU/path/I/O/PCI/LPC/IDE/NIC/USB/ACI state. The oracle independently
executes unchanged kernel bytes against declared native reset/mask policy and
separately extracted structural geometry; this is not physical retail boot
proof. All earlier prefixes still match;210 HAL cases match19018 instructions
and ALL1742160 bytes. GPU command stays0; full board boot is incomplete and
SDK frontier stays40824B readingFD001804.

Verification: coherent32-target build and34/34 selected CTests pass in one
run, with zero skips.18 USB/ACI BAR probes match independently extracted
measurements. Checks cover BAR command/zero/wrap/alignment, fixed IO types and
header flags, invalid-request atomicity, wrong functions, foreign threads and
retired owner lifetimes. Real GPU fixtures retain zero Khronos validation
messages. Captures reproduce after the coherent build; source SDK/kernel/
resource generated CPU bodies are unchanged. Broad109 tests and prior29
resource-oracle cases are not rerun. All owned sessions are terminal.

The structural audit also identifies unresolved hardwired PCI capability/
DEVSEL status traits in the current native policies, and IDE's measured8A
programming-interface capabilities versus native80. These are explicit
remaining defects before retail/live acceptance, together with capability/
register/mask/power and IDE mode-transition owners. They are not hidden by a
passing original-CPU comparison under declared policy. See
work/sandbox-primary-v15/pci-structural-trait-audit-v285.json. Next work should
resolve these known trait gaps and implement the APU configuration dependency;
pinned APU source is in mcpx-apu-authority-v284.

Evidence: work/sandbox-primary-v15/mcpx-aci-acceptance-v286.json,
mcpx-aci-live-v283/aci-board-prefix,
mcpx-aci-aci-original-comparison-v283.json,
mcpx-aci-transport-original-comparison-v283.json,
mcpx-pci-bar-geometry-v282.json, mcpx-aci-regressions-v284.log and
launcher-preservation-v285.json. Current66-source/32-executable/installed
fingerprints are retained. The unchanged installed r3/private binary remains
SHA72c1a7692441c354206bb5461f2626b179f5b0b8c54659199613a2d07a491e45.
Earlier v275 bounded idle/combat/entrypoint evidence is not repeated or
broadened. The restored original arena reference stream still matches its
recorded SHA; historical accepted v14 remains missing and no acceptance is
recreated. No new installed update or live World GPU binding this turn.

Remaining retail PCI traits, APU/LPC vendor/AGP/board, port80CD/hardware-flags,
actual audio/USB/storage/network owners, NV2A BAR/VRAM/vendor/ROM, actual PIC/
ISR/IRQ/DPC/commands/cache, live404D10/404EC0/full191890/191A50, persistent
World worker/three synchronous bridges and all102-element local/split/LAN/
original-render/audio/all-map/menu/full installed acceptance remain required.
Goal turn classification: PROGRESS.


## v292 — immutable PCI traits, guarded PM metadata and APU configuration (PROGRESS)

Full goal remains ACTIVE/unaccepted: 0/102 whole sandbox elements complete.
The v285 measured immutable-trait differences are corrected across eight
native PCI profiles. Only read-only capability/DEVSEL status bits06F0 are
extracted; mutable operating status/commands/addresses are excluded. IDE now
advertises measured fixed8A capabilities, with native compatibility mode0 as
declared reset policy. Mode changes remain required, not silently acknowledged.

Six fixed PM capability headers match the independent measurement extraction:
IDE/ACI/APU00020001, NICFE020001, and USB0/1DA020001, with immutable pointer44.
Logical native reset D0/control0 follows the pinned conventional PCI reset
contract; it is not copied PMCSR, engine readiness, wake or completion. The
metadata never supplies codec/PHY/DSP/DMA/IRQ state. Changed power/PME/data-
selection requests reject before any mutation. Clearing already-zero PME
status is a valid no-op. GPU/LPC capability-chain reads still stop at their
required owners. These fixes resolve measured immutable discrepancies, not
all retail reset/RW-mask/dynamic-power behavior. The physical probe's NIC BAR1
raw mask implies8 bytes, agreeing with pinned nvnet.c, while its annotation
says16; both are retained as an explicit evidence discrepancy.

An explicit native APU PCI owner at bus0/device5/function0 uses measured/pinned
identity10DE:01B0/B1/0401 and BAR0 memory524288. The unchanged whole original
80063908 kernel root produces command0007, BAR0=FE800000 and interrupt line5.
No captured APU register bank, DSP/voice/PCM state or completion is installed.
Original initialization then stops BEFORE800150E4 IN EAX,DX at existing LPC
bus0/device1/function0, extended register8C, width4. Its producer/reset owner
is unavailable and no default/published/captured value is returned.

20933 retired original kernel instructions plus one unretired attempt,
1590 I/O events and ALL8232 retained bytes match native CPU/path/PCI/LPC/IDE/
NIC/USB/ACI/APU/memory state. The independent oracle executes unchanged kernel
bytes, validates declared reset against separately extracted immutable traits,
and independently stops at the unsupported LPC register before I/O. This is
conditional original-code proof under the declared configuration policy, not
physical retail boot proof. All earlier prefixes still match;210 HAL transport
cases match19018 instructions and ALL1742160 bytes. GPU command remains0;
complete board boot and original SDK initialization are unresolved. SDK still
stops at40824B readingFD001804.

Verification: coherent33-target build;35/35 selected CTests pass in one run,
zero skips. Eight immutable profiles and six fixed PM headers are checked,
including atomic unowned transitions, IDE mode rejection, clear-zero PME
status, capability-owner boundaries, wrong functions and APU/ACI/USB lifetime
retirement under concurrent callbacks. BAR sizing/command/zero/wrap/alignment
checks pass. Real GPU fixtures retain zero Khronos validation messages.
Captures reproduce after the coherent build. Source SDK/kernel/resource CPU
bodies are unchanged. Broad109 suite and prior29 resource-oracle cases are
not rerun. All owned Ninja/CTest/oracle/Xvfb sessions are terminal.

LPC8C research is retained in lpc8c-producer-research-v291. A published boot-
process description points to the earlier2BL "Enable IDE and NIC" step.
Owned MCPX1.0 ROM SHA
e99e3a772bf5f5d262786aee895664eb96136196e37732fe66e14ae062f20335
and its reset-prefix disassembly are authenticated. A direct LPC8C address
literal was not found; this does not prove absence of a producer. Owned2BL
code/full producer path still must be authenticated before implementation.
The published description is a research lead only, not an initialization
provider. No native LPC value or successful boot gate is supplied from it.

Evidence: work/sandbox-primary-v15/mcpx-apu-acceptance-v292.json,
mcpx-apu-live-v289/apu-board-prefix,
mcpx-apu-apu-original-comparison-v289.json,
mcpx-apu-transport-original-comparison-v289.json,
pci-immutable-trait-audit-v291.json, mcpx-pci-immutable-traits-v287.json,
mcpx-apu-regressions-v290.log and launcher-preservation-v291.json. Current
70-source/33-executable/installed fingerprints are retained. Installed r3 and
private runtime remain exact SHA
72c1a7692441c354206bb5461f2626b179f5b0b8c54659199613a2d07a491e45.
Earlier v275 bounded idle/combat/entrypoint evidence is not repeated or
broadened. Original arena reference remains exact; historical v14 checkpoint
is still missing, with no recreated acceptance. No new installed binary or
live World GPU binding this turn.

Remaining LPC8C/firmware producer, LPC vendor/AGP/board, port80CD/kernel flags,
entered PM/IDE transitions and relevant retail/reset/RW-mask validation,
actual APU/ACI/USB/storage/network owners, NV2A BAR/VRAM/vendor/ROM, actual PIC/
ISR/IRQ/DPC/commands/cache, live404D10/404EC0/full191890/191A50, persistent
World worker/three synchronous bridges and all102-element local/split/LAN/
original-render/audio/all-map/menu/full installed acceptance remain required.
Goal turn classification: PROGRESS.


## v295 — authenticated owned 2BL LPC producer and complete original call (PROGRESS)

Full goal remains ACTIVE/unaccepted: 0/102 whole elements complete. This turn
resolves the LPC8C producer's code provenance and required call graph; it does
not wire a guessed/default/captured control value into the native runtime.

The reference-only extractor executes unchanged owned MCPX1.0 cipher code
FFFFFEDD..FFFFFF92. ROM SHA
e99e3a772bf5f5d262786aee895664eb96136196e37732fe66e14ae062f20335
and owned modified Complex4627 BIOS SHA
34f1c8ded59116436065783f8ad2ef0939df3cbfc76277ec9e5c41bf9ccb93cd
are checked before execution.348262 original instructions,57 code sites and
74306 bounded memory writes recover24576 2BL bytes. The original signature
check passes and its transfer entry is0009024C. Recovered image SHA:
af41ab2fab1b07a2a203f30d7272b7ac00a33311530d67ac3cb9b74e16127bbb.
Cipher code/inputs are read-only; no algorithm reconstruction, guest patch,
ready/completion field, key print or separately saved key is used. An initial
incorrect256KiB size assertion was corrected to the inspected/authenticated
owned1MiB image before extraction; its failed log is retained.

Original copy instructions000902D9..000902EB execute and copy6144 dwords from
00090000 to00400000, matching all24576 bytes. Original call operands establish
main0040061D, producer00400553, caller00400630 and return00400635. The producer
is202 bytes/86 static instructions. It performs four SMBus/SMC reads, response
calculation and three writes before the final PCI OUT at00400619. Its optional
zero-challenge branch invokes a reset/HLT dependency. Selecting just the final
write would bypass required original logic and is not an admissible fix.

A new read-only wrapper reuses the existing isolated-xemu capture machinery
unchanged. Hardware execution breakpoints and stepping observe the actual
complete successful producer call in original boot:428 instructions,201 sites
and51 I/O events, returning to00400635 with its source image unchanged. The
relocated original-boot image matches the standalone extraction SHA exactly.
The caller, SMC read/calculate/write paths and final PCI outputs execute for
real in the reference; their captured operating values are evidence only,
never native defaults or replayed production I/O. Error/timeout/reset branches
are not all exercised. The original title SDK factory also completes after
the added observation; original TU1.5 text and initializer bytes are verified.
Private EEPROM/HDD overlay, unchanged backing and no GDB code/data writes are
confirmed. All owned xemu/GDB/Xvfb sessions are terminal. This owned modified
BIOS/emulator proof is not physical Xbox or full retail boot equivalence.

Next implementation must lower whole00400553 and every entered SMBus/SMC
helper, own actual transaction/challenge/response completion, preserve the
required reset/halt path if entered, and compose the resulting firmware write
with the later original kernel producer. Native LPC8C reads remain unowned
until that path exists. Do not seed the observed value, return fixed completion
flags, replay the51 captured I/O events or skip the SMC exchange.

Evidence: work/sandbox-primary-v15/owned-2bl-acceptance-v295.json,
owned-2bl-recovery-v293/report.json, relocation-proof.json,
lpc8c-producer-function.json, original-2bl-producer-live-v294/
original-2bl-producer.json, bootloader-capture-validation.json and
owned-2bl-source-audit-v294.json.72 current source fingerprints include the two
new reference tools; all previous70 source files/33 native executables are
byte-identical. The prior35 selected CTests are not repeated or broadened.
Native boot still stops before800150E4 at LPC8C after20933 original instructions;
GPU command remains0 and SDK frontier stays40824B -> FD001804. Installed r3
remains exact SHA
72c1a7692441c354206bb5461f2626b179f5b0b8c54659199613a2d07a491e45.
No new installed binary, live GPU binding or whole-element acceptance.

All remaining original producer/native hardware/IRQ/command/worker integration
and the full102-element local/split/LAN/original-render/audio/all-map/menu/
fresh acceptance scope remain required. Historical v14 checkpoint is still
missing; no acceptance is recreated. Goal turn classification: PROGRESS.


## v300 — complete native owned 2BL SMC/LPC call graph (PROGRESS)

Goal remains ACTIVE/unaccepted, 0/102 whole elements. Five complete authenticated
owned Complex4627 2BL functions (218 static instructions) are now compiled into
native C++:00400418/0040048C/004004EF/00400553/00400789. All source branches,
helper calls and the required reset/halt dependency are retained. Generation
requires exact agreement of MCPX cipher output and original-boot relocated
image (SHA af41ab2fab1b07a2a203f30d7272b7ac00a33311530d67ac3cb9b74e16127bbb).
No production opcode decoder, mounted firmware bank, captured port sequence,
tail-only call or supplied success state exists. Generator replay is identical.

The native whole routine matches the observed original boot call:428 instructions,
51 I/O operations, GP/defined flags and all65536 retained stack bytes. Captured
port inputs appear only in an explicitly offline test. Independently executing
unchanged original2BL instructions against64 declared native protocol cases
matches20975 instructions,2548 I/O operations and524288 retained bytes. Cases
cover byte/word reads/writes, collision/retry, protocol error, timeout, aborted
transactions and the zero-challenge reset/halt path. This is code-lowering proof;
it does not validate a physical SMC or every possible hardware timing branch.

New native synchronous SMBus ownership requires an actual device callback for
each transfer. Busy state belongs to the active callback; completion follows a
successful operation. Missing/faulted/foreign owners stop, zero acknowledgement
never manufactures completion, and unsupported cycles/IRQ controls stay required.
A production SMC endpoint/challenge/response/reset owner is NOT implemented.
The native event-ack policy is explicit; physical controller reset/masks/timing/
IRQ equivalence is unproved. Pinned primary xemu AMD756/SMC files are retained
as protocol compatibility references; their permissive/fixed challenge behavior
is not imported as a production SMC implementation.

LPC8C is unreadable until a complete original source write supplies its word;
partial/overlapping first writes reject atomically. Native backing retains the
requested control bits; physical vendor masks remain unproved. A renderer-owned
native boot transaction holds the actual PCI lock for2BL without inventing guest
CLI/STI/IF state, rejects nested control and releases the lock on exceptions.

With a declared SMC device fixture, complete00400553 executes all7 device calls/
51 I/O operations and produces LPC40000000 through actual native PCI. The full
original kernel initializer then produces48000000 and reaches21016 retired
instructions/8232 retained bytes/1596 I/O events. Unchanged original kernel
comparison matches and stops before80063BB3 OUT DX,AL:required port80CD,width1,
value8. GPU command remains0. This composed fixture is NOT a production SMC or
live world GPU binding. Without that endpoint, production composition stays
required. Remaining SDK frontier is40824B -> FD001804.

Coherent34-target build and36/36 selected CTests pass. All210 prior HAL transport
cases plus7 prior board prefixes and the new firmware-composed prefix match
unchanged original kernel execution.79 source/34 executable fingerprints and
all retained inputs/results are recorded in work/sandbox-primary-v15/
bootloader-native-acceptance-v300.json. The first test compile's unsupported
optional FP equality was replaced by explicit unchanged-control comparisons;
no implementation FP behavior changed. All owned sessions are terminal.

Installed r3 and launcher selection remain exact runtime SHA
72c1a7692441c354206bb5461f2626b179f5b0b8c54659199613a2d07a491e45.
The coherent main runtime remains identical; these new resource modules are
not yet bound to its live path. No new launcher/gameplay acceptance is claimed.
Historical v14 acceptance remains missing and is not recreated.

Next:actual production SMC/reset ownership; port80CD and remaining AGP/board/
flags/MMIO/native commands/IRQ owners; live worker/tag/cache/world GPU binding;
all102-element local/split/LAN/original-render/audio/all-map/menu/full-game
acceptance and verified installed delivery. Goal classification:PROGRESS.


## v306 — native PRDY selection and AGP header ownership (PROGRESS)

Previous turn was PROGRESS; its79 source/34 executable and installed-r3
fingerprints were revalidated before work. Goal stays ACTIVE/unaccepted,0/102
whole elements. Four new native files own explicit GPIO and AGP configuration;
no source body, hardware flag or unknown vendor value is bypassed or supplied.

Pinned [Cromwell PCI initialization](https://github.com/XboxDev/cromwell/blob/86f547387184d1001d377cce97d3756c8acf91cc/drivers/pci/pci.c#L486)
identifies port80CD/value08 as selecting the CPU PRDY pin function. This is the
role reference; owned4627 kernel80063BB3 writes the identical port/value. Pinned
xemu acpi_xbox.c places this in its GPIO window but ignores the write, which is
not adopted as a native implementation. SourceMcpxGpio instead retains the
function selection only after the actual source write through renderer-owned
transport. I/O-window assignment is explicit native policy, not proof that the
original ACPI BAR producer ran. Unknown pin/mode/width/input/electrical/debug
operations stop; no field-pin toggle or zero status is returned. Thread/lifetime,
retirement and non-PCI-lock GPIO access are verified.

SourceMcpxAgpPci owns a conventional64-byte type1 header. Pinned xbox_pci.c/
pci_bridge.c provide compatibility identity/schema; [DVT4/5558 measurements](https://xboxdevwiki.net/index.php?title=PCI&oldid=6609)
provide only immutable10DE:01B7/A1/type1, primary status0220, secondary status
02A0 and16-bit I/O/32-bit prefetch traits. Operating addresses/status/commands/
vendor fields are excluded. Existing eight extracted hardware profiles remain
identical; the new ninth profile does not misinterpret bridge windows as BARs.
Reset/standard masks are declared native policy, not full retail physical proof.
Bus fields now gate child GPU configuration when an AGP owner is attached.
Unowned vendor/upper-window/reset/control changes reject atomically; missing,
foreign/retired owners and unsupported functions stay required. Actual bridge
forwarding, DMA, IRQs and physical reset/masks are not claimed.

Complete original00400553 still executes its7 device operations/51 I/O events
under the declared SMC fixture, supplying LPC40000000. Whole original80063908
then produces48000000. With GPIO attached it reaches21187 retired instructions/
1607 I/O events before requiring AGP. With AGP header attached it reaches21603
retired+1 attempted instructions/8232 retained bytes/1639 I/O events and stops
before800150E4 IN EAX,DX at bus0/device30/function0,register40,width4. GPU command
remains0. Unchanged original kernel execution matches all retained bytes, source
instructions, ABI/I/O and native configuration for both new prefixes.

An initial test expectation incorrectly anticipated hardware-info flags as the
next stop. Actual original transport reads all256 PCI bytes, so the unowned AGP
vendor register40 is encountered first. The expectation/oracle were corrected,
with the failing logs and current required-owner diagnostic retained. The
implementation was not extended with guessed/default vendor bytes. Hardware
flags at8003B150 remain later/unreached dependencies.

Reference-only linear inventory of the authenticated owned BIOS finds authored
rows for LPC BAR80000810=8001 and AGP8000F04C=1. All four256KiB banks match.
The inventory does not prove branch reachability/executed producers and imports
no native initial values. Full original producer/native owner evidence remains
required; these rows are not substitutes for that composition.

Coherent34-target build and36/36 selected CTests pass (68.04 seconds), followed
by unchanged-original comparisons of210 HAL transport cases, all ten board
prefixes, and the prior64 whole-2BL differential cases.83 current source/34
executable fingerprints, primary references, native captures and comparisons
are recorded in work/sandbox-primary-v15/gpio-agp-native-acceptance-v306.json.
All owned processes/sessions are terminal. Root concurrent/MCC work is preserved.

Installed r3, rebuilt main runtime and launcher selection remain exact SHA
72c1a7692441c354206bb5461f2626b179f5b0b8c54659199613a2d07a491e45.
New resource modules remain unbound to the live runtime; no new launcher/live
GPU/full-game/physical acceptance is claimed. Historical v14 acceptance remains
missing and is not recreated. SDK frontier remains40824B -> FD001804.

Next:resolve original AGP extended/vendor40 ownership; actual production SMC/
challenge/response/reset and ACPI/native binding; later hardware flags, AGP/
GPU MMIO/commands/IRQ and worker/tag/cache/world composition. All102-element
local/split/LAN/original-render/audio/all-map/menu/full-game/fresh installed
acceptance remains required. Goal classification:PROGRESS.


## v312 — complete original PCI initializer/native comparison (PROGRESS)

Goal remains ACTIVE/unaccepted,0/102 whole elements. Previous v306 was PROGRESS;
83 source/34 executable and installed-r3 fingerprints were revalidated first.
This turn closes the complete software-call verification gap beyond the native
AGP40 stop. It does not fill that vendor register with a captured/default value.

A new read-only wrapper reuses the isolated reference infrastructure unchanged.
Hardware breakpoints stop the actual owned kernel at80063908 from authenticated
caller8006286D; original return80062872 is verified. All450 compiled instruction
sites are checked against the unchanged loaded owned kernel before/after. The
actual full call executes28082 instructions across413 sites,1066 calls and2133
I/O events, then returns with ESP+4. The original GPU command-enable write is
observed; source code/constants remain exact. Private EEPROM/HDD overlay and
unchanged original TU1.5 text/SDK factory are verified. All owned reference
xemu/GDB/Xvfb sessions are terminal. This is owned modified4627/xemu evidence,
not retail physical register semantics or every firmware/IRQ branch.

New native whole-call test executes the existing complete compiled initializer
against explicitly offline original port inputs. All28082 instructions,2133
port/CLI/STI operations, GP/EFLAGS/XMM outputs, source FP controls,40 constant
bytes and65536 retained window bytes match. The8-byte hardware-info input is
unchanged. Native logical x87 sentinel state is declared and stays untouched;
raw original physical x87 contents/tag interpretation were not captured/proved.
The retained64KiB window contains the hardware-info data bank:an initial fixture
overlap rejection was corrected by splitting its backing around the read-only
8-byte region, without weakening the memory guard. No native algorithm change
or production captured-I/O/hardware-info provider was added.

The original call reads and writes all256 AGP configuration bytes. Its192 vendor
bytes are returned unchanged in the observed write requests. That is NOT proof
that hardware writes have no side effects. Reference vendor40 is0, whereas the
published booted DVT4/5558 snapshot is1; neither is reset/write-mask authority.
The native register therefore remains unowned. Supplemental primary Linux
NVIDIA AGPGART code identifies later aperture fields for its supported PC
chipsets, but does not establish Xbox vendor40 or its reset semantics and is
not used to invent a native value.

Evidence:work/sandbox-primary-v15/original-kernel-pci-whole-call-v308/
original-kernel-pci-call.json and kernel-pci-call-validation.json,
agp-original-read-write-contract-v311.json, the new native full-call report,
and kernel-pci-original-call-acceptance-v312.json. Coherent35-target build and
new whole-call CTest pass. All previous34 executables and82 previous non-CMake
source files are byte-identical; prior36 selected CTests are not repeated or
broadened. Their existing evidence remains applicable.85 source/35 executable
fingerprints and all reference/native evidence are retained.

Native actual-owner composition is still21603 instructions and stops before
800150E4 at bus0/device30/function0,register40,width4. GPU command remains0 in
that composition. The compiled routine's matched enable REQUEST in an offline
test is not native device enablement. Production SMC/ACPI/native binding,
later hardware-info/AGP/GPU BAR/MMIO/command/IRQ owners, actual worker/tag/cache/
world GPU binding and SDK frontier40824B -> FD001804 remain unresolved.

Installed r3, rebuilt main runtime and launcher selection remain exact SHA
72c1a7692441c354206bb5461f2626b179f5b0b8c54659199613a2d07a491e45.
No fresh gameplay build or live/full/physical parity acceptance is claimed.
Historical v14 acceptance is still missing and is not recreated. All102-element
local/split/LAN/original-render/audio/all-map/menu/full-game/fresh installed
acceptance remains required. Goal classification:PROGRESS.


## v318 — actual native SMC boot resource (PROGRESS)

Goal remains ACTIVE/unaccepted,0/102 whole elements. SourceSmcBoot replaces the
fixed SMC callback in complete original firmware/kernel composition. Actual OS
entropy supplies a stable nonzero boot challenge; complete compiled owned2BL
004004EF computes the expected response. Both response bytes must match before
authentication cancels an actual native watchdog. The owner implements the
P01/P05 rotating version counter and its source-issued reset. Unsupported,
incomplete/bad-response, foreign, expired and retired operations stop explicitly.
Watchdog expiry produces a reset REQUEST; actual platform reset is still required.

Pinned unchanged Cromwell BootPicManipulation independently matches86273 vectors
against the owned compiled helper. Its source/GPL header and all generated
reference inputs/outputs are retained for offline verification only. PIC research
revision5623 supplies the protocol/version role reference, not exact physical
nonce generation, timing, invalid-response or reset semantics. Positive timeout,
P01/P05 selection and admission of a nonzero challenge are declared native policy;
physical1-to-1 SMC acceptance is not claimed. Archive handshake fetch429 remains
retained; it was not silently counted as successful authority.

Complete original00400553 now executes7 actual device operations/51 I/O events
and cancels the real watchdog, then produces LPC40000000 through native PCI.
Original80063908 produces48000000 and reaches21603 retired+1 attempted
instructions/1639 I/O, still stopping before800150E4 at AGP bus0/dev30/fn0,
register40,width4. GPU command remains0. Original ACPI BAR producer/native port
binding, AGP vendor fields, hardware-info and live world GPU binding remain open.
The valid protocol owner does not replace unknown physical device behavior.

An initial reverse static-library dependency caused a link failure. Moving the
SMC resource into halo2_systems beside its actual memory dependency fixes it.
Coherent36-target build and38/38 selected CTests pass in41.44 seconds, followed
by unchanged-original comparisons for210 transport cases, ten board prefixes
and64 whole-bootloader cases. The28082-instruction/2133-I/O offline whole kernel
test also passes. Fresh native captures are archived, including the random
challenge-dependent stack inputs, with89 source/36 executable fingerprints in
work/sandbox-primary-v15/smc-boot-native-acceptance-v318.json. Owned sessions
are terminal. Generated original routines remain byte-identical.

Installed r3/main/launcher selection remain exact SHA
72c1a7692441c354206bb5461f2626b179f5b0b8c54659199613a2d07a491e45.
Only the previous kernel composition test executable changes; the new SMC owner
is not yet bound into the playable runtime. No new launcher/full/physical gameplay
acceptance is claimed. SDK frontier remains40824B -> FD001804. Historical v14
acceptance remains missing. All102-element local/split/LAN/render/audio/all-map/
menu/full-game/fresh installed gates remain required. Classification:PROGRESS.


## v320 — executed original ACPI BAR producer (PROGRESS)

The previous static-only xcode inventory is now supplemented with actual original
CPU execution. An unchanged-infrastructure, read-only wrapper stops the owned
MCPX1.0 ROM atFFFFFE00, authenticates its complete bytecode-loop bytes, observes
all268 reached/unique firmware rows through actual exitFFFFFEBC, and captures48
actual original PCI IN/OUT operations. Executed flash rows and ROM code remain
unchanged. Later original TU1.5 SDK initialization still returns success with
unchanged text. Private EEPROM/HDD overlay, pinned firmware/xemu and terminal
reference processes are verified. No guest patches/data writes are introduced.

Original rowFF000092 executes FFFFFE61 OUT DX,EAX for PCI80000810=00008001,
confirming execution of the authored ACPI BAR producer. RowFF0000BF similarly
requests AGP8000F04C=1. No vendor40 access occurs in this firmware trace. These
are software request/reachability facts in owned modified4627/xemu; they do not
prove physical BAR/vendor masks, reset values, side effects or native binding.
Neither request is imported as a native configuration default. Native complete
firmware execution still needs the preceding hardware owners and actual binding.

Evidence:work/sandbox-primary-v15/original-mcpx-xcode-v319/ and
mcpx-xcode-producer-acceptance-v320.json.90 source/36 executable fingerprints
are retained. All previous native sources/executables and installed r3 remain
byte-identical to v318; no duplicate native regression run is needed. Actual
native composition still stops at21603 instructions/AGP40; SDK40824B->FD001804
and live GPU/world/launcher gates remain unresolved. Goal remains ACTIVE,
0/102 whole elements accepted, with full local/split/LAN/render/audio/map/menu/
game/delivery scope preserved. Classification:PROGRESS.


## v329 — compiled original MCPX bytecode and native ACPI binding (PROGRESS)

Previous v320 was PROGRESS;90 source/36 executable/installed fingerprints and
terminal reference processes were revalidated. Goal remains ACTIVE/unaccepted,
0/102 whole elements. All work stays in the private snapshot; root concurrent
and MCC changes remain preserved.

The complete original MCPX1.0 bytecode loop FFFFFE0A..FFFFFEBB is now compiled
to native C++:78 authenticated instruction sites, all source branches/opcode07
indirection/default fallthrough retained. A flat segment/memory ABI is required.
Its native stage boundary is original JMP FFFFFEBC; preceding segment setup and
following MTRR/cache/cipher stages are required separately. This is not a claim
that the complete native ROM boot has run or returned.

Expanded unchanged-original capture observes268 reached rows and every original
port/MMIO operation. The independent original-ROM oracle matches5586 instructions,
253 expanded hardware operations and actual original GP/flags output. Native
compiled execution matches those outputs/requests exactly. The actual boot visits
73 sites;64 explicitly declared opcode10/EBP and protected LPC80 branch cases
exercise the other five. Combined78-site coverage is not all2^32 argument or
physical branch proof. Declared native logical FP/SIMD state is preserved;
original physical FP contents were not captured.

Native LPC ACPI BAR ownership admits a complete source-issued write and follows
actual command bit0 for decode. No readable running address is seeded. Structural
256-byte I/O geometry follows pinned schema and DVT4/5558 FFFFFF01 probe, excluding
the published operating address. Unknown partial/probe/upper-address writes stay
required. GPIO can bind to its actual LPC and follows enable/disable/relocation;
foreign/missing/mismatched owners reject. Physical reset/masks/relocation and
electrical semantics remain unproved. Numeric GPIO policy remains explicit in
the separate existing later-phase probe.

Starting the complete compiled loop against actual renderer-owned native PCI,
132 instructions retire and produce BAR8001 and command3. The GPIO binding then
derives8000 from that source-written LPC state. Original prefix instructions and
GP/flags match independently. It stops before FFFFFE9F OUT DX,AL at port8049,
which still needs an actual owner. No transition from that stop into later boot
stages is performed. The separate2BL/SMC+kernel probe remains21603 instructions
to AGP40, with GPU command0; it is not a continuous native ROM boot.

The first whole-loop comparison caught an extra read-before-write inserted by
the shared store-observation helper. The new stage now issues the exact original
MMIO write directly through its memory owner, without synthetic before-image
reads. Initial display tests inherited Wayland and SDL failed to enumerate a
display; the private Xvfb harness now explicitly selects X11. Failure/diagnostic
logs are retained. No original hardware output was replaced to make tests pass.

Coherent37-target build,39/39 selected CTests (35.63 seconds), generation replay,
210 original HAL cases, ten board prefixes and64 whole-2BL differentials pass.
Current captures are archived;95 source/37 executable fingerprints and reference
evidence are recorded in work/sandbox-primary-v15/mcpx-xcode-native-acceptance-v329.json.
Owned sessions are terminal. Only prior kernel-PCI and LPC test executables change.

Installed r3/main/launcher selection remain SHA
72c1a7692441c354206bb5461f2626b179f5b0b8c54659199613a2d07a491e45.
New modules are not bound into the playable runtime. Full8049/firmware composition,
AGP/GPU startup, SDK40824B->FD001804, worker/tag/cache/world and fresh launcher
acceptance remain open. Historical v14 acceptance remains missing. All102-element
local/split/LAN/original render/audio/all-map/menu/full-game gates remain required.
Classification:PROGRESS.


## v331 — ACPI8049 authority gap (PROGRESS, research only)

Previous v329 was PROGRESS;95 native source/37 executable/installed fingerprints
and prior reference/regression process termination were revalidated. No native
source or binary changes this turn; previous39-test evidence remains applicable.

The owned original ROM writes byte08 to port8049 atFFFFFE9F, rowFF0000A4.
Interpreted as the upper byte of a16-bit register at8048, it selects bit11.
AMD's23167B AMD-766 data sheet (March2001), PM48 on page78, defines bit11 as
TCOHALT, freezing its TCO timer and suppressing timeout status. This supplies a
candidate role, not evidence that MCPX has the same register layout. The pinned
Linux nv_tco.h instead defines halt at bit12; its actual PCI table covers MCP51,
MCP55,MCP78S,MCP79, and establishes no original MCPX support. These are different
chips, not contradictory descriptions of one device. Family similarity cannot
justify importing either layout into the native Xbox owner.

Microsoft's pre-retail hardware-design revision1.02 describes the TCO block and
interrupt source but refers detailed register definitions to a separate NVIDIA
MCPX data sheet. It does not resolve8049 masks, reset/timer behavior or side
effects. The published XboxDev SMBus reference explicitly warns that the AMD
controllers are similar rather than identical; its scope is SMBus, not8049.
The pinned full Cromwell archive contains no direct8048/8049/TCO role match.

Primary source/driver metadata, exact header/table excerpts, observed request,
searches and failed fetches are retained under acpi-register-authority-v330/.
AMD756 Arrow fetches failed by timeout/HTTP2 reset; official alternatives were
missing or non-PDF; a university mirror failed certificate validation. AMD766
text was inspected through web extraction; direct HTML fetch returned403. These
failures are not successful document captures and no certificate check was
disabled. No native watchdog, host power or original firmware state was changed.

This evidence changes the next authority step: obtain an Xbox-specific MCPX
definition or sufficiently authenticated physical/reference behavior, rather
than treating the AMD or later NVIDIA layout as an implementation contract.
No no-op or captured-request completion is added. Actual native MCPX prefix
remains132 retired instructions ->8049; separate kernel probe21603 ->AGP40,
SDK40824B ->FD001804, and live worker/tag/cache/world composition remain open.
Installed r3/main/launcher remain unchanged SHA
72c1a7692441c354206bb5461f2626b179f5b0b8c54659199613a2d07a491e45.
Goal remains ACTIVE,0/102 whole elements, preserving all local/split/LAN/render/
audio/map/menu/full-game/fresh delivery gates. No new gameplay acceptance.
Evidence:work/sandbox-primary-v15/acpi-authority-progress-v331.json.


## v338 — complete AI storage startup and fresh sandbox delivery (PROGRESS)

Previous v331 was PROGRESS through authority research; this turn makes a native
implementation change toward the original tactical AI scope. The hand-written
1CFA40..1CFAB4 startup prefix is replaced by the complete literal44B6F8 callback
1CFA40 and its nine additional whole source bodies:670 static instructions.
Existing original arena/CRC/pool/native heap owners service the real calls.
The source constructs14 inactive datum pools, including256 actors of stride888,
and its additional actor structure. No actor, successful allocation address,
AI readiness, tactical decision or frame result is supplied. The arena startup
worker is stopped/joined before the read-only diagnostic observer is invoked.

Unchanged owned TU1.5 code executed on the independent host i386 CPU matches
98,725 source trace events (including string-instruction expansion), complete
GP/SSE/logical x87/control/status/MXCSR/defined flags and all60,084,310 retained
bytes. The backend observes22,942 original stores and touches200,704 bytes; all
remaining committed bytes are checked unchanged. There are no original code
patches, supplied gameplay returns or native expected outputs used by execution.
This is a fresh successful startup path on actual native-produced inputs, not
all branch/failure coverage or physical Xbox/campaign behavior acceptance.
Unicorn's differing x87/MXCSR status is retained; host original execution agrees.

The comparison exposed canonical addresses in the arena diagnostic callbacks.
A separately generated730-site authenticated translation fixes those observer
addresses at the live composition boundary. The original arena module's
canonical ABI remains unchanged. A first trace regeneration attempt found the
old world correspondence file missing; its existing tables were preserved, and
the new arena translation uses its available original correspondence. The first
link failure and intermediate diagnostics remain retained. Both new generators
replay byte-identically. No missing historical evidence is reconstructed.

Coherent39-target build and41/41 selected CTests pass in38.00 seconds, including
fresh live-world construction and the new full AI storage regression. Current
106 source/39 executable fingerprints and exact captures are recorded in
work/sandbox-primary-v15/ai-storage-native-acceptance-v338.json.

Fresh r4 runtime SHA98b4a17048fc866d14256db3d419a685fe3f0dfad861f657c078a3b04a7846b6
is selected by ./Halo2-Sandbox-Test and verified by --launcher-info. Its staged
identical entrypoint executes12 Lockout frames and records the actual child
inode SHA. Idle-one runs20.007 seconds; combat-four runs35.010 seconds with
firing/reload/move/jump/melee/grenade inputs and exits normally. The earlier v336
four-view process disappeared before a report; it is unaccepted, not treated as
a successful or still-running check. Only that unfinished check was rerun.
The old r3 package is preserved. Regular/MCC/title launchers remain untouched.
The screenshot is Netpbm despite its requested extension; a lossless PNG preview
was generated separately for inspection. No original pixel/PCM parity is claimed.

AI new-map activation, actor birth, perception/tactical/path/frame/encounter/
species/difficulty/animation and presentation are still required; the provisional
campaign brain remains. Actual MCPX8049, later AGP40/SDK and native worker/tag/cache/
world composition owners remain unresolved. Goal stays ACTIVE,0/102 whole elements.
All weapon/ability/vehicle/projectile/damage/world/objective/animation/render/audio,
local/split/LAN, every map/menu/full-game and fresh delivery gates retain full scope.
Classification:PROGRESS. Next unblocked AI owner is literal new-map callback1CFC90.


## v347 — original AI new-map callback and r5 delivery (PROGRESS)

Previous v338 was PROGRESS; all106 source/39 executable/installed fingerprints
were revalidated before edits. Goal remains ACTIVE,0/102 whole elements.
Complete original1CFC90, literal44B700, and its direct ten-body closure now run
in the native map initializer. The555 static instructions retain both branches:
multiplayer executes the original six-event AI-disable path; the campaign input
path resets its14 source datum pools/globals and authored squads/squad groups.
The original tail dispatch to208C90 is retained. Existing140CF0/16E600/16E6A0
source owners perform real resets and allocations; no successful result, actor
count, source-ready field or AI decision is supplied by the implementation.

Ten before/after comparisons cover both first and repeated reset on multiplayer,
empty campaign-mode input, tutorial's six squads, Cairo's117 squads/60 groups,
and High Charity's252 squads/52 groups. Unchanged original code on Unicorn and
independent host i386 CPU agrees with415,580 source trace events, complete CPU
ABI/defined flags and every60,084,310 retained bytes per case. The backend
observes125,134 original stores. There are no code patches, supplied gameplay
returns or native expected outputs used by execution. These cases visit465 of555
new static sites; the90 unexecuted sites and remaining populated-player/object
branches stay explicit. Source generation replays byte-identically.

The campaign-mode diagnostic delivers only the literal game-options mode1 input
after native multiplayer setup; it does not establish actual campaign session
boot. Real owned campaign tag/scenario data supply every squad/group row. All15
owned campaign maps pass the same native repeated-reset/count/activation probe.
This is bounded AI map initialization, not NPC birth, tactical decisions, ordinary
campaign traversal or matched Xbox rendering/audio. A permanent Cairo regression
adds nonempty squad-group coverage alongside the tutorial and empty/mode tests.

Coherent40-target build passes44 selected CTests in50.57seconds. After registering
the additional Cairo group test, unchanged implementation/executables pass that
new test in0.92seconds:45 distinct tests, not a claimed single45-test run. Current
110 source/40 executable fingerprints and exact original captures are retained
in work/sandbox-primary-v15/ai-map-native-acceptance-v347.json.

Fresh r5 runtime SHA45c752bea8e73fd5274145a1102dfe2a5015f0edcc532b2a981dca5328664ab3
is selected by ./Halo2-Sandbox-Test and --launcher-info. Its staged identical
entrypoint executes12 Lockout frames with actual child-inode hash verification.
Idle-one passes20.014seconds; combat-four passes30.020seconds with real native
input events and normal exits. Earlier r3/r4 packages and regular/MCC/title
launchers are preserved. Full-game/controller/LAN/original frame/PCM parity is
not inferred from these native delivery checks.

Next unblocked integration is actual actor placement/birth and AI frame/HS
ownership. Newer root campaign-world-api evidence already contains typed original
placement and a55-body HS/squad backend; inspect its fresh provenance/ABI and
source cohort before adapting it into this private sandbox. Do not recopy the
whole concurrent root or substitute its bounded proof for original CPU comparison.
The actual campaign runner still has14 mission-owner failures; source HS frame
scheduling/frontend/player/object lifetimes and broad NPC combat are unfinished.
All weapon/ability/vehicle/projectile/damage/world/objective/animation/render/audio,
local/split/LAN/map/menu/full-game gates retain full scope. MCPX8049/laterAGP40/SDK
hardware composition remains unresolved. Classification:PROGRESS.


## v359 — original NPC placement and live AI frames; r6 delivery (PROGRESS)

Previous v347 was PROGRESS. All110 source/40 executable/installed fingerprints
were revalidated. Goal remains ACTIVE,0/102 whole elements. Newer concurrent root
has diverged from the sealed campaign-world-api cohort; this adaptation uses the
sealed routine identities and redecodes every instruction from owned TU1.5.
The CPU/memory ABI headers match exactly. Existing owners are reused rather than
copying duplicate bodies; no cohort state, pointer, actor count or successful
callee result is imported. Eight native modules add293 complete routines and
26,229 static instructions, including71 authenticated CRT behavior-mask callbacks.

Original HS cold row54 now precedes AI row58. Original team relationships consume
admitted campaign options/difficulty at new-map reset. The original14EAD0 player
join continuation updates its true active census, then1D03A0 registers the player
with AI. Original1D0670 runs before PVS/object physics at actual13AB0D. Typed
place_ai retains the original counted/authored/packed-reference scheduling. The
256 overload cases independently replay unchanged original wrappers exactly;
argument/publication/action-observation services remain declared fixture scope.

Actual Cairo squad0,1st_hum_johnson, now creates an original actor and unit through
all three placement paths. Real actor/command/HS-thread bindings survive complete
world authority restoration, followed by three ordinary AI/object ticks per mode.
This snapshot rebuilds borrowed pool views from restored source pointers through
its existing API; no source pool contents or actor fields are supplied. The test
configures campaign options before map callbacks so team/AI resets consume them.
Adding actual feedback-duration156DD0 and complete new-player registry helpers
resolved the first real missing calls; failed v350/v351 logs remain retained.

The first bounded live-frame comparisons passed, but inspection found missing
original CRT behavior masks. All71 real callbacks are now executed before AI
storage; the storage regression authenticates their actual load/store operands
and published values. Current nine actual AI phases, across all placement modes,
match unchanged original host i386 execution:140,007 source trace events,3,519
original stores, complete CPU ABI/defined flags, every60,092,502 retained bytes
per case and exact instruction paths. No code patches, expected native outputs,
CPU-estimate substitutions or supplied gameplay returns are used. Unicorn's
MXCSR precision-status difference is retained; original host CPU matches native.
These are bounded first-three-tick source-world comparisons, not broad NPC combat,
original HS scheduling, all branches/species or physical Xbox/frame/PCM proof.

A new read-only whole placement capture records335,613 native events and85 real
host CPU estimate samples. Uninterrupted original execution first differs at
126139->1A3B70: the existing native animation-resource policy synchronously reads
real cache bytes and publishes its completed native job, while original source
enters its asynchronous queue. Later original execution reaches unmapped kernel
import800000EA. Full creation comparison is unaccepted; no completed call or
kernel success is supplied to hide that boundary. The existing async/cache/worker
composition requirement stays open. AI comparisons start after actual asset
loading and do not certify this earlier platform/resource lifecycle.

Coherent42-target build and49/49 selected CTests pass in41.26seconds. All eight
module generators replay byte-identically. Current 138 source/42 executable
fingerprints, cohort/ABI/registry pins and exact captures are recorded in
work/sandbox-primary-v15/campaign-ai-native-acceptance-v359.json.

Fresh r6 SHA5900bf2c7902976132a4eb17a709dd999f8ecaaa1f6904e0bc64b9c1c70b3884
is selected by ./Halo2-Sandbox-Test and --launcher-info. The identical staged
entrypoint verifies its actual child inode and12 Lockout frames. Idle-one passes
20.017seconds; combat-four passes30.014seconds with native inputs and normal
exits. Earlier r3/r4/r5 packages and regular/MCC/title launchers are preserved.
The launcher still exposes multiplayer tests; source-world NPC tests do not
establish a live campaign menu/Simulation bridge or campaign playthrough.

Next work must connect the actual campaign source world to frontend/Simulation/
player/object ownership and finish original HS frame scheduling, so original
placement and controls replace the provisional campaign brain in normal play.
Preserve broader tactical/perception/navigation/species/weapon/vehicle/difficulty/
encounter/death/recreation, render/audio/LAN and every102-element gate. The known
14 mission-entry failures and MCPX8049/laterAGP40/SDK hardware gaps remain open.
No whole element or full-game completion is inferred. Classification:PROGRESS.


## v390 — original HS scheduler and map-admission diagnosis (PROGRESS)

Previous v359 was PROGRESS. At turn entry all138 source/42 executable/installed
fingerprints were revalidated. Goal remains ACTIVE,0/102 whole elements. The
private snapshot now adds35 complete original routines and1,414 static source
instructions beyond v359, including actual2115D0 scheduling,1E7180 cleanup,
dynamic Cairo builtin/interpreter callbacks, and original HUD/cinematic storage.
Ten generators replay byte-identically. The source cohort still contributes
identities only; every native instruction is redecoded from the pinned TU1.5 XBE.

The scheduler runs in source AI ->HS ->PVS order. Actual Cairo startup threads
exposed missing begin/if/equality, player/input/camera, fade, cinematic and sleep
callbacks. Literal original function-table pointers/direct calls authenticate
each whole recovered owner. Callback212330 is the real original XOR/RET4 cast;
no callee completion or script result is supplied. Original13BC10/13BC32 and
13E9E0/13EA40 construct/reset the actual HUD/cinematic state before these actions.
The earlier progress description of HUD fade state as game speed was corrected
against Cairo's authored hud_cinematic_fade opcode and original instructions.

Renderer storage12842..12879 is explicitly an11-instruction bounded allocation,
CRC and publication span, not a complete original graphics constructor. Generated
code preserves guest ESP at this source boundary; it cannot satisfy an original
callee dispatch. Its real constructor capture matches unchanged original Unicorn
and host i386 execution:67 events,8 original stores, CPU/defined flags/exact trace,
and every60,084,310 committed bytes. No instruction patch, native expected output,
CPU estimate or gameplay return is supplied. Full preceding GPU startup stays open.

With the actual callbacks present, the Cairo interpreter loops instead of yielding.
The failed native v386 test was terminated; its unbounded trace was not accepted.
Attaching GDB was denied by host ptrace policy, so a separately owned GDB child
confirmed the native211B10 loop and was killed/closed. Unchanged original replay
from that exact captured entry state also executes200,000 instructions without
return: game+1120 is0, clock tick0, active thread sleep1. Source211BBA..211BCB only
honors future sleep times when the original map-ready byte is set. This is an
unadmitted world-state boundary; no native interpreter escape or readiness1 is
invented. The native campaign frame API now rejects that incomplete state before
executing a partial tick, preventing the known loop and trace growth.

The authenticated original readiness1 publication is191E2E, after191DF0 calls13A780 new-map,
D7AA0 placement, C29F0/13A920 player startup and successful11E8E0 BSP activation.
Current initialize_map composes only a subset. Historical v359 AI/object three-tick
comparisons remain bounded subsystem evidence; they did not execute HS or prove
this map admission. Do not restore green tests by setting game+1120 or bypassing HS.
The next13A820 BSP activation audit authenticates21 table callbacks (including
shared roots);67 previously missing complete routines/6,784 instructions form
their static closure. That audit is retained, not yet implemented or accepted.

Coherent42-target build passes. Selected CTests are47/50 in42.30seconds: the three
Cairo placement cases still create actual actors, validate command/thread links
and restore the complete authority bank, then explicitly fail at map admission.
The new empty-map scheduler case passes three ordinary native ticks; all three
whole2115D0 phases match unchanged original host execution:168 events,33 original
stores, full CPU/trace and every60,092,502 retained bytes per case. Empty/no-authored-
thread scheduling is not accepted Cairo/startup behavior.
Current 142 source/42 executable fingerprints and all retained evidence are in
work/sandbox-primary-v15/campaign-hs-native-progress-v390.json. No fresh candidate
is accepted or installed. ./Halo2-Sandbox-Test --launcher-info still selects the
verified r6 SHA5900bf2c7902976132a4eb17a709dd999f8ecaaa1f6904e0bc64b9c1c70b3884.

Next work is complete source map/BSP activation and native subsystem ownership,
then actual due-thread HS comparisons and campaign frontend/Simulation/player/
object integration. Broader tactical/species/weapon/vehicle/difficulty/encounter/
death, asynchronous resources, rendering/audio, local/split/LAN, menus/maps/full
game and every102-element gate retain full scope. MCPX8049/laterAGP40/SDK GPU
composition remains unresolved. No whole element or full-game completion is inferred.


## v404 — whole source map admission and real audio prerequisite (PROGRESS)

Previous v390 was PROGRESS. All142 source/42 executable/13 installed fingerprints
were revalidated. Full goal stays ACTIVE,0/102 whole elements. The private snapshot
adds whole191DF0 admission and the authentic new-map/new-BSP/before-BSP callbacks
at44AED0, stride24hex,68 rows. Exact source operands/count/stride/direct callers
authenticate selection. Source191E2E alone publishes ready after real11E8E0
success. No ready1, callee return, initialized device, controller or actor output
is supplied. The module adds466 complete decoded routines/25,892 static source
instructions, sharing prior owners. These are compiled bodies, not all-branch or
whole-game acceptance. FLDLG2 uses the actual native x87 instruction under the
retained source FP controls; that additional operation is not separately accepted.

The campaign initialize_map path invokes the complete original admission with
its admitted options and actual selected BSP input at options+120. New-map
execution exposed missing original level-handle pools and their qsort comparator,
then row29/36 storage. Authenticated cold rows13,29,36,43,44,51,55,64,66 are now
owned; straight storage/recorded-animation constructors are wired. Whole1410A7
replaces the earlier168BA3-only first-person cold setup and13BC10 child call,
constructing real1976CF/25A71C/25B5CA/1AB975/232A33 state without double allocation.
The original12879 ->20E90 continuation now constructs its0x24-byte renderer state.
The preceding full GPU cold constructor remains an explicit incomplete boundary.

Read-only observations surround whole callbacks at13A7BA/13A86A/11E94B, retaining
their actual source stack, ABI, full bank and instructions. Eight completed native
callbacks were captured before the next missing subsystem. Six match unchanged
original Unicorn and host i386 execution:21,095 reported events,2,791 original
stores, full defined CPU ABI/control/trace and all60,100,694 retained bytes per
case. No code patch, CPU estimate, native expected result or gameplay return is
supplied.126730 and692D0 remain unaccepted: uninterrupted original execution
reaches unmapped imports800000B6 and80000080. Both failures are retained; the six
independent passes do not imply full map/whole-original-kernel acceptance.

The real native map path reached sound map callback1281A0 ->2260E0 with no
52D674 device state. Instead of allocating a fake device bank, a constructor
accepting actual immutable SourceTu15UserSettings now invokes complete127EE0
sound cold startup, including224EC0. Native backing, locks and recovered pool
capacity remain owned by the existing resource adapter. Missing SDK/device/DSP
owners propagate. This full path replaces manual sound prefixes when explicit
settings are supplied; the previously bounded unactivated path remains labeled.

The actual256-byte owned EEPROM (SHAb002f5a9556b930f6076820616c0e98735f3e78653a58709cbef06984e42889a)
has valid UserSettings checksum. Only its immutable96-byte non-key section is
retained. With this real query input, all three campaign CTests reach127EE0 ->
224EC0 ->37DA9A ->33C29D and fail at the original unbound data import41B784,
ordinal356, HalBootSMCVideoMode. This is not XboxHardwareInfo (ordinal322).
The complete getter stays present; a named resource guard reports the missing
producer instead of dereferencing encoded80000164. Neither observed mode1 nor
observed initial7 is supplied. Existing source_boot_smc_video_mode_after_read
only owns80054F49..80054F58 publication after an actual read; it is not a current
SMBus/AV producer. SourceSmcBoot owns authentication/version protocol, with other
registers still required. Reuse those owners and prove actual command4/AV input
and the whole producer rather than repeating the existing publication fixtures.

Coherent42-target build passes. Selected CTests are47/50 in42.65seconds, without
skips: three campaign cases explicitly fail at that real mandatory data owner.
They no longer certify the earlier partial cold/map actor path. All eleven
module generators replay byte-identically. Current 159 source/42 executable
and13 installed fingerprints, source callback comparisons and retained failures
are recorded in work/sandbox-primary-v15/map-admission-native-progress-v404.json.
The validated r6 runtime remains SHA5900bf2c7902976132a4eb17a709dd999f8ecaaa1f6904e0bc64b9c1c70b3884
and is still selected by ./Halo2-Sandbox-Test --launcher-info. No new candidate
was accepted or installed. Unrelated concurrent builds/dirty root work are preserved.

Next is the actual native boot-SMC mode producer/import and audio SDK/device/DSP
ownership, then complete map activation, due-thread HS/source frame phases and
the campaign frontend/Simulation/player/object bridge. Later cold50/profile and
broader BSP/frame owners still require full original composition. Preserve all
weapon/ability/vehicle/projectile/damage/world/objective/animation/render/audio,
local/split/LAN, all maps/menus/missions and every102-element requirement. Native
asynchronous resource/worker and MCPX8049/laterAGP40/SDK GPU gaps remain open.
No whole element, full campaign or full-game completion is inferred.


## v406 — native AV transaction and boot-mode export (PROGRESS)

Full goal remains ACTIVE,0/102 accepted whole elements. All159 prior source,
42 prior executable and13 installed fingerprints were revalidated at turn start.
The private snapshot adds an explicitly configured native MODE1/2/3 pin resource;
there is no default AV profile or copied original kernel initialization state.
Raw electrical levels are grounded in the original Xbox Hardware Design
Specification1.02 sections2.7.6/2.13.3 and retail PIC register4 documentation.
The constructor does not name uncertain pre-release pack interpretations.
Register4 alone is owned. Other registers, AV-change interrupts, physical PIC
electrical/timing behavior and whole kernel boot remain unproved.

The actual native SMBus controller performs a byte transaction with this device.
The complete authenticated owned2BL00400418 reader drives its original ports,
including real collision retry. The independently checked source publication
rule then creates a named immutable ordinal356 export. A failed/missing device
cannot publish an initial7 or recorded1. This is an explicit native synchronous
transport policy, not a claim that the interrupt-driven kernel HalReadSMBusValue
or complete80054F2D boot parent has executed. Unsupported/foreign/destroyed
device callbacks and protocol/timeout/abort failures remain mandatory stops.

The image loader can retain both the actual monotonic tick-count import and
the boot-mode resource. A new world constructor takes that named resource with
actual UserSettings. The three full campaign fixtures explicitly configure raw
pins100; the launcher receives no implicit new AV setting. All eight pin inputs
pass through the actual bus and original byte reader to the read-only import,
whole original33C29D getter and37DA9A audio helper. Their special mode3/mode6
source branches execute. The prior592 helper/6240 publication fixtures pass
again. No new independent original CPU comparison or physical parity is inferred.

Campaign startup now passes its prior unbound356 failure and reaches actual
38EC8F ->38EC0E from the source SDK/APU initializer. Owned TU1.5 disassembly
authenticates that31-instruction routine, its five calls, tail and eleven direct
FE8xxxxx MMIO stores. This is a required native audio-device/DSP/platform owner,
not evidence of an allocation failure or a successful driver. No zero MMIO bank,
successful device/callee output, ready flag or captured initialized state is
supplied. Complete audio SDK/APU/DSP lifetimes/PCM and full map activation remain
required. The original full kernel/SMC interrupt parent is also not accepted.

Coherent49-target build passes. Main resource/world regression checks are48/51
in43.81seconds; the three campaign failures are exactly the new38EC8F boundary.
Six additional historical audio checks were attempted: audio-mode passes, four
stop before verification because original SDK/device/environment capture inputs
are absent, and pool-capacity skips for the same missing SDK call capture.
Those five checks are explicitly unverified, never counted as acceptance. Total
distinct checks:49 pass,3 native campaign failures,4 missing-fixture failures,
1 missing-fixture skip. Missing inputs are listed in the progress report.

Current160 source/49 executable/13 installed fingerprints, new AV cases,
source APU dependency disassembly and both regression logs are retained in
work/sandbox-primary-v15/smc-av-native-progress-v406.json. The launcher still
selects r6 SHA5900bf2c7902976132a4eb17a709dd999f8ecaaa1f6904e0bc64b9c1c70b3884.
No candidate was accepted or installed. Root/concurrent work is preserved.

Continue the actual audio SDK/APU/DSP owner, then complete map/new-BSP admission,
script/full game-frame phases and campaign frontend/Simulation/player/object
integration. Preserve all menus/maps/missions and the entire102-element
weapon/ability/vehicle/projectile/damage/world/objective/animation/render/audio,
local/split/LAN, independent original evidence and launcher delivery scope.
Historical missing capture inputs are a validation limitation, not permission
to recreate original state. No whole-element or full-game acceptance is claimed.


## v422 — whole source APU reset and exact original composition (PROGRESS)

Previous v406 was PROGRESS. Its160 source/49 executable/13 installed fingerprints
were revalidated before edits. Full goal remains ACTIVE,0/102 accepted elements.
The private snapshot now compiles the complete38EC0E APU reset graph:22 new whole
routines,594 static original instructions, sharing existing owners. TU1.5 XBE
bytes authenticate every instruction and the actual38EC8F caller. Generation
replays byte-identically. No initialized APU MMIO bank, device-ready bit, captured
kernel context or positive hardware/kernel result is supplied to the live world.
World dispatch now reaches this complete source graph rather than stopping at
its missing first callee. Native APU/DSP ownership is still mandatory.

Read-only original captures use hardware breakpoints, single stepping and the
existing GDB monitor connection, with isolated HDD/EEPROM and unchanged owned
inputs. The capture tool now bulk-authenticates original SDK instructions and
uses the existing debugger connection for FS state; opening a second QMP client
had stalled the first attempt. Stack capture follows the mapped current page.
Intermediate captures with incomplete FP or unrelated kernel-entry observations
were rejected by the comparisons and retained. Finalv420 conditions hardware
breakpoints on the exact audio caller's ESP and actual return address, and
asserts unchanged CALL registers; graphics IRQ callers cannot satisfy them.

The whole observed original reset matches377 source instructions,122 actual
source stack stores,20 original MMIO accesses,12 kernel calls, full defined GP/
flags/XMM/raw-x87 ABI and all59,428 retained stack/object/SDK bytes. Original
kernel and interrupted-step stack effects (880 bytes) and actual I/O/kernel
results are declared external inputs ONLY to this offline composition test.
Source stack outputs are checked separately before those external effects,
with their before-byte dependencies checked. Seven interrupted nonbranch step
observations are retained separately; no completed source instruction is
removed. No original patch, injected original callee return, native expected
data or result-masking byte normalization is used. This conditional composition
proof does not implement native IRQ/DSP/DMA/voices/PCM or physical hardware.

The reference caught a real lowering boundary defect: the shared arithmetic
executor normalizes host x87 precision even at integer-only external callbacks.
The APU graph now retains the actual untouched FP bundle at every callback and
final/exceptional exit, while accepting actual returned owner bundles. This
fix is local to the authenticated integer graph and replayed generator.
Ordinary missing native IRQL ownership still rejects before any fake MMIO.

Fresh SDK before/call/settings inputs restore three formerly unavailable checks:
SDK resources, pool capacity and actual audio settings all pass again. The
configured SDK capture is work/sandbox-primary-v15/original-apu-reset-v420/original-sdk.
Original output-driver before/create and environment-before inputs remain absent;
their two checks fail before verification, not accepted or reconstructed.

Coherent50-target build passes. Selected regressions are53/58 in51.25seconds,
no skips. Three full campaign cases reach127EE0 ->224EC0 ->SDK/APU ->38EC8B ->
38EC0E ->38EB16 ->383D70 and fail at383D88 ->encoded800000A0, ordinal160,
KfRaiseIrql, import41B7AC. Actual original export80014338 is four instructions;
lowering80014368 is25. Owned kernel code and reached bindings are retained in
apu-irql-frontier-v422.json. Actual original80035C00 IRQL and pending IRQ/PIC/
dispatch globals are not copied into native state. The native passive thread's
60200024 policy is not a completed interrupt domain. Native serialization,
current-level/IRQ/PIC/dispatch and APU control/FIFO/DSP/DMA/notification/lifetime
owners are next; a successful empty callback is not permitted.

Current166 source/50 executable/13 installed fingerprints and the exact
comparison/capture/failure records are in work/sandbox-primary-v15/apu-reset-native-progress-v422.json.
The launcher still selects r6 SHA5900bf2c7902976132a4eb17a709dd999f8ecaaa1f6904e0bc64b9c1c70b3884.
No candidate was accepted or installed. Dirty root/concurrent work is preserved.
All complete map/BSP, scripts/full frame, campaign frontend/Simulation/player/
objects, all maps/missions/menus, asynchronous animation/cache/worker, later GPU,
all102 sandbox elements and local/split/LAN/original render/PCM/full-game gates
remain required. A component comparison or compiled routine is not whole parity.


## v432 — native audio reset and complete storage composition (PROGRESS)

Previous v422 was PROGRESS. Its166 source/50 executable/13 installed fingerprints
were revalidated. Full goal remains ACTIVE,0/102 accepted whole elements. The
shared checkout now contained source-led native audio resources, so this turn
adopted their retained callback output/control/PCM reset and page resources into
the private snapshot, preserving the dirty root. Exact copy/adaptation hashes
are retained. The original owned TU1.5 executable remains authority; no MCC or
Vista gameplay source/visual reference is used by this adoption.

An explicit new world constructor retains a real SdlSoundPlatform shared owner
with actual UserSettings and the transaction-produced boot mode. It executes
the original SDK factory and constructors, preserving their COM object outputs;
an SDL ID is never substituted for an Xbox object pointer. Source38EC8F now
executes the complete original38EC0E graph against actual native control jobs,
recursive exclusion, live voice lists and PCM cancellation/receipt lifetimes.
The scoped original raise/lower callsites map to a named native serialization
policy. They do not execute the complete original kernel IRQ/PIC/scheduler or
copy its captured state. Processor contexts remain explicitly unconfigured.

Three independent unchanged-original CPU cases under those declared native
platform inputs match379/379/377 instructions and137 ordered source stores each,
GP and defined-maskCC5 flags, full tested APU/stack bytes. Native tests include
actual retained PCM, two-slot backpressure, warm reset, voice dispatch cutover,
Flushed receipt accounting/handoff and real DeviceCallback consumption. Their
backend is PipeWire, not dummy/disk. This proves native resource operations and
conditional source CPU composition, not DSP/physical IRQ/retail PCM parity.
The earlier actual-original-boot377-instruction comparison also still passes.

World construction exposed a real unwind bug: an interrupted audio transaction
faulted, then unrelated registered CRT exits tried to start that audio scope
again and aborted. Source exit retirement now permits those non-audio source
callbacks to release their own resources without re-entering the faulted audio
guard. Audio requests still reject faults. Retained audio output/control
ownership is explicitly rejected by checkpoint export, prepare and commit;
live resources cannot be silently dropped or imported from a byte checkpoint.

The complete next38E996 storage owner and direct closure add7 whole original
routines/257 static instructions. Existing owners remain shared. Both the new
storage and prior reset modules replay byte-identically. Source38E71C/3879CB
performs descriptor clears, real capacity accounting and logical physical
address publication using genuine native64 retained storage. The older snapshot
allocator lacked the source's FFFFFFFF unrestricted-high request. That sentinel
is now admitted while retaining the bounded token namespace; last-page,
exhaustion/no-wrap, malformed bound/extent/alignment, protection and retirement
guards pass. These addresses are logical resource identities, never truncated
host pointers or a claim of physical host PCI/DMA addressability.

Four unchanged-original DMA cases under explicit native resource inputs match
2100/8244/2100/19508 instructions and2070/8214/2070/19478 source stores, actual
capacity clears/descriptors/statistics, source GP/defined flags and complete
tested memory. Native failure/foreign/ownership guards and teardown also pass.
No successful empty hardware, initialized MMIO bank or original game result is
supplied. The actual campaign path now completes1 reset,16 retired control
commands,0 queued commands and16 allocations totaling339,968 bytes. GP/EP program
loaded flags remain false. All three campaign modes reach38ECA3 ->38E8C9,
the still-required original39-instruction interrupt/configuration method.
Its first IEN store is D8 atFE801004; full routes/frontend/codec/event producers
and correct global delivery must be owned before it can return successfully.

Coherent53-target build and61 distinct checks:56 pass,3 campaign cases fail at
that actual configuration dependency, and2 historical output-driver/environment
checks fail before verification for missing original capture inputs. No skips.
The main60-check run took47.37seconds; the additional physical namespace guard
passes. Earlier aborted runs and boundary failures remain retained, not relabeled.
Current179 source/53 executable/13 installed fingerprints and all evidence
are in work/sandbox-primary-v15/native-audio-storage-progress-v432.json. Launcher
r6 remains SHA5900bf2c7902976132a4eb17a709dd999f8ecaaa1f6904e0bc64b9c1c70b3884.
No new candidate is accepted/installed. No owned build/test jobs remain.

Continue complete38E8C9 with actual native frontend/interrupt/event/codec resource
owners; the shared checkout contains later related owners worth inspecting and
validating before reuse. No D8-ready routes, global-enable success or DSP program
may be invented. Then complete SDK/device lifetime and PCM, whole map/BSP,
script/full frame, campaign frontend/Simulation/player/object, all maps/missions/
menus, asynchronous animation/cache/worker and later GPU owners. All102 sandbox
elements, local/split/LAN and independent original rendered/audio/full-game
acceptance remain required. Native component tests do not accept a whole element.


## v452 — native cold audio graph and exact configuration boundary (PROGRESS)

Revalidated v432's179 source/53 executable/13 installed fingerprints before work.
Goal remains ACTIVE,0/102 accepted whole elements. All menus/maps/missions,
local/split/LAN and independent original visual/audio acceptance remain required.
Changes stay in the private v15 source tree; shared root source/MCC/shader work
is preserved. Shared native audio components were copied with exact hashes and
adapted to the snapshot's already authenticated reset and combined storage APIs.

The complete configuration/recovery CPU module has3 routines/100 static
instructions. Native event processing adds9 routines/249 instructions, and
deferred processing adds23/787. Existing source helpers retain their original
owner; a stable generation precedence prevents duplicate entry ownership. All
three new modules plus the existing22-routine reset and7-routine storage graphs
replay byte-identically. Integer configuration/event work retains actual caller
FP state across native helper calls; the deferred module retains its genuine
original floating-point instructions. Device action-store observation never
adds an MMIO preimage read. Fresh independent unchanged-XBE references include
12 complete conditional configuration cases and three deferred/registered cases.
The536-byte SDK fixture literal export also replays exactly. These are declared
boundary CPU comparisons, not observed physical hardware/default IRQ results.

The world now constructs a retained native graph after actual SDK/reset/storage:
shared callback output/control generation, all16 real DMA allocations, native EP
byte transports/prepared leases, five actual table ranges and physical windows,
completion-derived frontend events/source ISR/DPC, actual epoll/eventfd/timerfd
resources, and a disabled source-delivery policy. Lifetimes retain native64
backings; exception scopes cancel/fence callbacks before outer DMA/reset unwind.
Checkpoint rejection for live audio remains explicit. Native preference and
small SDK-data helpers are test fixtures, not substitute world EEPROM/boot data.

All three actual campaign modes now execute38E8C9's first IEN=D8 request against
the genuine disabled dispatcher, then stop at38E8D3 / FE801100=88. This request
contains unverified FECTL bit3 in addition to known method halt. No inert field,
ready value, emulator shortcut or successful configuration return is supplied.
Each mode retains1 reset,16 completed/0 queued control commands and16 allocations
totaling339,968 bytes; GP/EP programs remain unloaded, delivery=false and
configuration_returned=false. Requested routesC8 remain unresolved. The complete
source configuration/helper return and initializer45/full map admission are
not accepted. Physical or authenticated primary field evidence is still required.

Final61-target coherent build includes halo2_native. The71-check selected suite
passes66: all ten new component/graph checks pass; three campaign checks fail
at the required original policy, and two historical output/environment checks
fail before verification for missing original capture inputs. No skips/aborts;
49.55seconds. Real output is PipeWire, never dummy/disk. Exact traces, callbacks,
queue backpressure/retirement, table/native64/physical bounds, foreign ownership
and failed partial transaction behavior pass. Earlier failed adoption/replay
steps remain retained. Whole SDK/DSP/physical IRQ/retail PCM/full-game proof is open.

Current230 source/61 executable/13 installed fingerprints and evidence are in
work/sandbox-primary-v15/native-sound-configuration-progress-v452.json. Launcher
r6 is unchanged at SHA5900bf2c7902976132a4eb17a709dd999f8ecaaa1f6904e0bc64b9c1c70b3884.
No candidate is accepted or installed. No owned build/test/display processes remain.

Continue verified full configuration semantics and actual requested IRQ/codec/EP
providers, source processing and resource lifetime, then whole original map/BSP,
AI/scripts/full frame and frontend/Simulation/player integration. Other retained
source-led prerequisites may advance while the specific FECTL field is unresolved;
none may force that operation to succeed. All102 elements and full-game gates
remain required, including missing historical references and async/GPU work.


## v460 — native DSP algorithms and actual numeric PCM delivery (PROGRESS)

Previous v452 was PROGRESS and its230 source/61 executable/13 installed
fingerprints were revalidated before work. Full goal remains ACTIVE,0/102
accepted whole elements. The specific cold38E8D3 device-policy boundary stays
unresolved while independently necessary source-led DSP processing advances.
No shared root implementation, launcher, shader, original asset or memory was
edited. Root goal documentation is append-only.

Copied120 independently source-led files into the private v15 tree, preserving
all copy/adaptation hashes. Sixteen CMake components cover typed Word24 storage,
gain fanout, distortion, two-product accumulation, chorus, two circular-history
effects, crosstalk, controller arithmetic/retained sinks, status provenance, GP
value output,15-record literal storage and numeric EP mono/stereo/module0.
The compiler builds ordinary native C++; no original program/interpreter/core,
source bank or captured-ready payload is a new runtime dependency. Unsupported
mode/address/alias domains still reject. This is available processing within
those admitted domains, not an authenticated complete native processing graph.
The numeric FP proposal component retains the original contract; its unrelated
MCC cache definition provider was not adopted.

All37 new numeric/ownership checks pass. Evidence includes80 gain frames,105
distortion frames,40 accumulation frames,116 controller arithmetic calls,
retained chorus and sink/packet cases, and441 original reverb frames/882 calls
with871,392 ordered arithmetic values and every retained ring/workspace field.
The distortion proof keeps its57 unchanged-core/corrected-limiter-agree cases
separate from48 manufacturer-corrected-limiter stress frames; corrected DSP-core
tiers are not physical DSP evidence. Corrected crosstalk compares39 calls and
retains withdrawal of the legacy nonzero upstream-route claim. Explicit input
fixtures do not establish source voice production, PCM scale or full status/clock.

Verified124 direct numeric provenance reference pins. Independently reran the
unchanged owned TU1.5 x86 seed/key/data transformation and compared the current
native constructor's462 typed fields across all15 original records, retaining
774,144 real scratch bytes. This proves the literal pre-effect phase, not an
installation, supervisor/mailbox/clock, whole frame or source acknowledgement.

Added an actual native format bridge from all three numeric EP result types
through their original format1 S16 packing into independently owned float48k
SdlSoundPackets. The bridge preserves64-bit context and owns PCM after temporary
results/history disappear. Exhaustive65,536 S16 bit patterns match the real SDL
converter exactly. Calculated nonzero stereo/mono/module0 outputs enter the
existing native mixer and real PipeWire callback; exact manual values, three
callback completions, two-slot backpressure and retention until acknowledgement
pass. No source producer, cursor/clock advance, frame/factory/installation ready
result, hardware FIFO receipt or physical speaker claim is supplied by this API.

Final96-target coherent build includes halo2_native. The109-check selected run
passes104 in61.27seconds. All38 new checks pass; three campaign checks still
reject the required38E8D3 / FE801100=88 FECTL bit3 request, and two historical
checks fail before verification for missing original capture inputs. No skips
or aborts. Each actual campaign mode still reports1 reset,16 completed/0 queued
commands,16 allocations/339,968 bytes, unloaded GP/EP programs, pendingD8 and
delivery=false/configuration_returned=false. The native libraries and format
bridge do not substitute for this original operation or complete initializer45.

Current354 source/96 executable/13 installed fingerprints, direct original
proofs and test reports are in
work/sandbox-primary-v15/native-sound-algorithms-progress-v460.json. Launcher r6
remains SHA5900bf2c7902976132a4eb17a709dd999f8ecaaa1f6904e0bc64b9c1c70b3884.
No candidate is accepted or installed; no owned build/test/display jobs remain.

Next work remains actual verified configuration/IRQ effects and full source
voice/mix acquisition (including the six-prefix producer), cross-record status,
GP/EP supervisor/clock/transport, installation/update and lifecycle. Continue
real codec/AC97/PCM providers and recover fresh genuine missing device/environment
captures without copying successful state. Then complete whole map/BSP, AI/scripts/
full frame and frontend/Simulation/player/object integration, all maps/missions/
menus, all102 sandbox elements and local/split/LAN/original rendering/audio/full
game gates. Native arithmetic/format/component evidence cannot accept a whole
element or full DSP. Async/cache/animation and later original kernel/GPU work
remain required as before.


## v468 — original audio references and full factory routing (PROGRESS)

Previous v460 was PROGRESS. Revalidated its354 tracked source/96 executable/13
installed fingerprints before work. Goal stays ACTIVE,0/102 accepted elements.
Source edits remain in the private v15 tree; shared root implementation/shader/
MCC work is preserved and root goal documentation is append-only.

A new single isolated cold-boot wrapper captured actual original224EC0 driver
input, complete387877 creation and38568E capabilities, then complete1972C0
environment and1973E0 mapping with7 and9 real I/O calls. Paired DSP section
loads naturally follow1->2->1 references with owned immutable bytes. Hardware
breakpoints/reads/normal continuation only; no guest code/data/register write
or supplied callee return. Dense logical x87 occupancy and actual FS are read
from the stopped CPU using the existing GDB connection. The original title/
disc and backing assets remain unchanged; the private EEPROM/HDD overlay and
owned processes retire. This is original code under pinned xemu/modified BIOS,
not physical console/PCM proof. Actual stopped code matches the owned executable
at3,826 instruction sites across seven native audio CPU modules. All state is
offline reference input, never a native initialized kernel/IRQ/device image.

Fresh artifacts are work/sandbox-primary-v15/original-sound-boot-resources-v461/
(original-driver,original-environment,original-dsp-section.json). Resource CTests
now select these genuine paths. A separate environment-reference setting keeps
historical CPU oracle/capture pairs unchanged. Missing old historical files
were not reconstructed, overwritten or mislabeled as restored originals.

This investigation also exposed a legacy output-adapter defect. It intercepted
whole387877, opened SDL, stored the SDL ID in the source COM output, then manually
returned success/consumed the stack. Its header claim that no COM substitution
occurred was wrong. A fresh actual-caller regression reproduced that exact red
condition before replacement (v462). Production world had already used the full
SDK/resource route; the obsolete adapter could still reintroduce a bypass.

SourceRuntimeSoundOutput now retains an actual shared stereo output and executes
the whole unchanged387877 CPU body reached by224F26. Original constructors own
COM objects/refcounts/output publication; native imports/control/configuration
still require their real providers. It supplies no source store, HRESULT, stack
return, singleton/AddRef or factory readiness. The world routes its factory
through this same corrected owner. Old source-binding close/CallScope logic is
removed; actual backend/control/SDK owners retain their own lifetimes. Bank,
source frame, actual FP state and creating-executor guards reject malformed or
foreign calls before mutation. The native diagnostic SDL ID remains separate.

The first replacement fixture correctly rejected its missing captured kernel FS
backing. The final test binds the same actual native passive-user executor policy
as the world, rather than importing/zeroing captured kernel state. It executes
11 original source instructions and stops at the mandatory383F92 import while
source device outputs remain zero. This is a factory-ownership/import-boundary
proof, not whole SDK, capabilities, DSP or PCM success. The fresh DSP resource
test also passes real retain/release, nesting/abort, restore and quiescence.

Final96-target build and109-check composed run:106 pass in52.53seconds; only
three campaign modes fail at the required38E8D3 / FE801100=88 FECTL bit3 policy.
Missing-reference failures fall from2 to0; no skips or aborts. All prior38 DSP/
PCM checks remain green with real PipeWire. Each actual campaign mode still has
1 reset,16 completed/0 queued commands,16 allocations/339,968 bytes, unloaded
GP/EP programs, pendingD8, delivery=false and configuration_returned=false.
No factory/configuration/device-ready result was invented to pass this boundary.

Current358 tracked source/96 executable/13 installed fingerprints and evidence
are in work/sandbox-primary-v15/native-sound-reference-recovery-progress-v468.json.
The former354-file subset had omitted the three existing legacy output files;
those are tracked from their corrected versions now, alongside the new capture
wrapper. Do not claim their original bytes were covered by the old subset.
Launcher r6 remains SHA5900bf2c7902976132a4eb17a709dd999f8ecaaa1f6904e0bc64b9c1c70b3884.
No candidate accepted/installed; no owned capture/build/test/display jobs remain.

Continue actual configuration/IRQ effects and complete source SDK/DSP download/
installation/update, native voice/mix/status/supervisor/clock/GP-EP transport and
real codec/AC97/PCM lifetime. The newly recovered original environment/mapping
inputs and real I/O can support further whole-graph source comparisons. Unknown
FECTL semantics remain unresolved. Whole map/BSP, AI/scripts/frame and frontend/
Simulation/player/object integration, all menus/maps/missions, all102 elements
and local/split/LAN/original visual/audio/full-game gates stay required. Async/
cache/animation, other missing historical evidence and later kernel/GPU work
remain open. Component/reference checks do not accept a whole element.


## v477 — whole DSP download CPU closure and actual GP pool resources (PROGRESS)

Previous v468 was PROGRESS; its358 source/96 executable/13 installed fingerprints
were revalidated before edits. Goal remains ACTIVE,0/102 whole elements accepted.
All implementation edits stay in the private v15 source. Shared root code, MCC/
shader work and installed launcher remain preserved; root goal edits append only.

The genuine197364 ->38594D DSP download call now has its whole previously missing
CPU closure:16 routines/756 static instruction sites. Direct and tail dependencies
are followed against the authenticated TU1.5 XBE; existing SDK owners are reused.
The native world routes this module, including original resource wrappers, effect
description relocation, byte transforms and GP scratch allocation requests. CPU
source keeps actual call frames and FP state, and store observation adds no MMIO
read. Generated output replays byte-identically. New plus previous audio modules
match stopped actual original bytes and owned XBE at4,582 sites across8 modules.
This is code-integrity/CPU implementation evidence, not a completed DSP download.

SourceRuntimeSoundDmaResources now owns the reached large DSad pool requests from
actual38DE81. Their allocation/capacity/lock/unlock/physical/release imports require
exact source/parent frames, bank, creating thread and actual36-byte context read
footprint. Real native64 bank storage and lock lifetime remain retained. Pools use
the same allocator namespace as the existing16 DMA descriptors. Synchronous reads/
writes alias those exact locked resources; foreign/unaligned/unlocked/cross-range
requests reject. Native physical release checks unchanged owned backing and rejects
staged/protected release. Checkpoint publication rebinds current native bank identity;
teardown now preserves foreign replacement banks. This private bank owns a vector
copy of page bytes: no page-aligned host pointer, Xbox pinning, host PCI DMA or native
DSP scheduling equivalence is claimed. Smaller unreached GP pool requests remain
unsupported. Source execution still owns clears/counters/context/SGE publication.

The GP provider identifies its genuine parent before generic SDK allocation, so
existing SDK users retain their owner. Three authenticated SDK lock-release sites
now support whole source download failure branches through the actual native mutex.
No HRESULT/stack/device success is supplied at a whole download/factory boundary.

Independent offline original x86 experiments supply declared synthetic input and
no callee returns.32 cases match143,601 executed instructions,29,028 ordered stores,
returned registers/defined flags/full final storage, and unchanged caller FP state.
All15 native CPU effect-data transforms also match independent original hashes.
The original825-site installation research replays with identical owned metadata/
contracts/hashes; transformed executable programs are discarded, never exported,
packaged or interpreted as a native DSP. Whole original missing-device/disabled
failure routes execute and release the actual mutex correctly (3 cases).

The native GP composition executes89 source instruction addresses,193,536 original
zero stores and writes189 data SGEs for the original774,144-byte request. Its7
declared prefix entries remain explicitly unconfigured; it stops before38DF31
table-limit MMIO publication. Resource checks cover growth/release, partial pages,
namespace collisions, exact context, bank/thread/lock/range guards, fault lifetime
and foreign replacement teardown. This is a declared resource composition, not
original device initialization or source command acknowledgement.11 actual import/
indirect boundaries and literal ordinals were reauthenticated against the XBE.

Final coherent98-target build and111-check run:108 pass in52.85seconds; only the
same3 campaign modes fail at38E8D3 / FE801100=88, unverified FECTL bit3. No skips,
aborts or additional regression failures; existing physical checkpoint/byte-range
and DSP/PCM checks remain green. Actual backend is PipeWire. Each campaign mode
still has1 reset,16 completed/0 queued commands,16 DMA allocations/339,968 bytes,
GP/EP programs unloaded, pendingD8, delivery=false and configuration_returned=false.
No opaque bit acceptance, GP ACK0, empty successful installer or ready bit was added.

Current365 source/98 executable/13 installed fingerprints and evidence are retained
in work/sandbox-primary-v15/native-sound-download-progress-v477.json. Launcher r6
still selects SHA5900bf2c7902976132a4eb17a709dd999f8ecaaa1f6904e0bc64b9c1c70b3884.
No candidate accepted/installed; owned build/test/capture/display processes retired.

Continue actual DSP resource lookup/import composition, GP prefix/table publication,
complete installation/command/ACK and native processing/status/clock/voice/GP-EP
ownership, alongside verified configuration/IRQ semantics and actual codec/PCM.
Whole map/BSP, AI/scripts/frame, Simulation/player/object/frontend integration,
all menus/maps/missions, all102 whole sandbox elements, local/split/LAN and original
rendered/audio/full-game acceptance remain required. No component pass accepts
a whole element. Async/cache/animation and later kernel/GPU obligations remain.


## v490 — original section lookup and real DSP retention (PROGRESS)

Previous v477 was PROGRESS. Revalidated all365 tracked source/98 executable/13
installed fingerprints before edits. Goal stays ACTIVE,0/102 whole elements.
Implementation remains private v15; shared root code/shader/MCC work is preserved.
Root goal changes append only. No launcher/runtime installation was performed.

The complete owned4627 RtlInitAnsiString/RtlEqualString/RtlUpperChar now run as
compiled native CPU functions:3 whole bodies/98 static instructions. Their116
immutable NLS table bytes come only from authenticated read-only PE code/data.
Every byte input0..255 was independently executed; observed original data reads
equal exactly those116 retained bytes. No captured mutable kernel/thread state,
TLS array, section counter, VM image, or ready word is used as native initialization.
All four actual original title imports agree with the owned loaded-kernel entries:
41B620/289 ->8002342C,41B6E4/279 ->80022678,41B6E8/327 ->8002EB0F,
41B6EC/328 ->8002EB7E. Authority is the pinned owned modified4627 under xemu;
this does not certify every retail kernel or physical Xbox operation.

SourceRuntimeDspSection now routes the reached original section search imports
through those whole CPU functions. Original2DF7AB resolves the authenticated
DSPImage header, including case variants. Original2DF887/2DF812/2DF833 retain
their actual bodies. The reached XeLoadSection/XeUnloadSection imports operate
the actual retained preloaded DSP resource with exact call/import/frame/bank/
executor/header-metadata guards. They map only a completed native retain/release
to its import status; no whole wrapper/factory/download result is supplied.
Original Xbox page mapping, mutable header/page counters and volatile Xe import
clobbers are not emulated. Other section/VM domains remain unsupported.

The native world passes actual source ABI, instruction/store observers and
required services through this path. Creating-executor guards cover retention
and scope creation. Shared resource nesting runs1->2->3->2->1; a completed native
load followed by caller failure unwinds to real residency1 with0 active requests.
Unmatched unload, foreign header/bank/target/thread/scope and forged constants
reject without returning success. Missing/altered NLS data rejects checkpoint
admission; read-only validation never creates an absent original data region.

Independent original CPU experiments cover289 cases and match284,069 instruction
steps,3,925 ordered stores, GP/defined flags/full final storage and caller FP state.
They cover all256 character inputs, null/empty/large ANSI length overflow, binary
strings/case folding, and7 title lookup cases.5 lookups return naturally;2 missing-
name fixtures stop before CRT last-error setup. Their minimal fixture does not
construct a captured TLS/CRT state or supply a last-error result. The production
world retains its existing complete2DBC56/CRT owner. Source-side resource tests
also use the actual native ordinal import policy, not only reference-resolved
kernel pointers. The generated kernel CPU/data replay byte-identically.

Final coherent99-target build and112 selected regressions:109 pass in54.17seconds.
Only the same3 campaign modes fail at required38E8D3 / FE801100=88 FECTL bit3.
No skips/aborts/additional failures. Existing original DSP section, download CPU,
GP pool, physical checkpoint/byte-range and DSP/PCM checks remain green, with
actual PipeWire. Each campaign mode still has1 reset,16 completed/0 queued commands,
16 DMA allocations/339,968 bytes, unloaded GP/EP programs, pendingD8, delivery=false
and configuration_returned=false. No empty installer, opaque bit accept, GP ACK0
or device-ready signal was added. New lookup/resource code is wired but has not
been reached by a successfully completed whole live audio initialization.

Current373 tracked source/99 executable/13 installed fingerprints and evidence are in
work/sandbox-primary-v15/native-dsp-lookup-progress-v490.json. Launcher r6 still
selects5900bf2c7902976132a4eb17a709dd999f8ecaaa1f6904e0bc64b9c1c70b3884.
No candidate accepted/installed; owned build/test/capture/display jobs retired.
The previous365-file source subset omitted the existing DSP resource header,
implementation and legacy test. Those were read live and are fingerprinted from
their current versions now, alongside the5 new kernel/lookup files. Do not claim
their previous bytes were covered by the old source subset.

Continue GP prefix/table publication, complete DSP installation/command/ACK and
native processing/status/supervisor/clock/voice/GP-EP ownership, plus actual MCPX
configuration/IRQ/codec/PCM. All map/BSP/AI/scripts/frame, Simulation/player/object/
frontend integration, menus/maps/missions, all102 elements, local/split/LAN and
original rendered/audio/full-game gates remain. Async/cache/animation and later
kernel/GPU obligations stay open. No component check accepts a whole element.


## v504 — GP scratch ownership and whole CPU upload preparation (PROGRESS)

Previous v490 was PROGRESS. Revalidated373 tracked source/99 executable/13 installed
fingerprints before edits. Goal remains ACTIVE,0/102 whole elements. Implementation
stays in private v15; shared root code/MCC/shaders are preserved and root goal
documentation is append-only. Installed launcher remains the preserved r6 build.

Added9 whole authenticated GP CPU routines/266 static instructions:388275,388294,
38850A,38DD0C,38DD26,38DD54,38DE07,38E668,38E6DF. Shared SDK/reset/storage/download
owners are reused without duplicate entry ownership. Actual FP state is retained.
The GP module and existing download module now route only the actual table stores
38DDE3/38DDED/38DE4D/38DF31 through a typed native resource publication. This does
not add an MMIO preimage read or a byte-backed device register/status bank. Both
modules replay byte-identically; stopped actual original code and owned XBE agree
at4,848 unique static sites across9 audio CPU modules. Kernel string authority
remains the separately authenticated owned4627 module from v490.

The DMA owner now retains the actual source-produced GP table base/count and walks
its source SGE bytes into existing native64 prefix/descriptor/locked pool storage.
There is no parallel scratch buffer. Bank identity, creating executor, descriptors,
context, physical-page identity, limits, high words and locks are checked. Cross-
resource writes preflight the whole range before any mutation. Real growth frees
the old pool; source destruction unlocks/frees the current pool, clears its source
pointer and decrements source statistics. Retired pool access rejects. Native page
tokens remain an explicit host namespace, not Xbox MMU/pinning/peripheral DMA.

SourceRuntimeSoundGpScratch composes these CPU/resource paths and is wired into
the native world. It derives context only from actual original caller/object fields,
retains the matching DMA bank, forwards shared services with actual ABI and handles
synchronous nested GP/download bodies through the same memory view. Unowned MMIO
reads/writes reject. The actual38846E command store is explicitly rejected before
commit until a real native GP command consumer exists; no request3/ACK0 or ready
word is supplied. A failed upload cannot reuse this graph owner.

The composed native test now allocates the manager/control descriptor/context
through the real SDK owner, then runs whole388275/388294, including the original
copy of its5CC-byte control input from the already-owned XBE. No DSP interpreter,
prepared opaque program loader or newly packaged processor payload was imported.
This is CPU data preparation; those bytes are not executed as a native DSP.
Original38E996's C000/1060 GP requests are actual native allocations. Original
initializer derives524 total/reserve516/8 prefix entries. Four extra actual page
descriptors are explicitly declared upstream inputs for whole38850A, producing12
prefix entries; their complete upstream factory is not claimed. The original data
allocator adds189 pages/count201, grows to190/count202, and the source destructor
retires them.433 original instruction addresses execute in this resource case.

The first test exposed a fixture accounting error: using whole SDK construction
adds136 actual pool bytes for its live manager/control/context. The earlier fixed-
context root fixture had no such allocation. Growth/destruction now compare the
actual source GP allocation delta while preserving that136-byte baseline; the
production allocator/counter logic was unchanged by this correction.

Whole388576 now runs native resource copying, original38DFFF decoding, actual pool
allocation and all15 source effect-description relocations before reaching38846E.
All15 records match original raw maps plus the exact source relocation arithmetic;
all15 transformed effect-data hashes match independent original CPU evidence.
The command word stays at its pre-publication0 because the actual store is rejected,
not because a consumer cleared a request. No transformed processor program is
exported or packaged. This comparison is post-initialization resource composition,
not a completed ordinary SDK/APU factory, live DSP execution or audio parity.

Independent original scratch CPU experiments add27 cases/881 exact instructions/
349 ordered stores and11 stops before unsupplied external imports/helpers. Source
registers/defined flags/final storage/FP state match. MMIO in those offline cases
is a declared write sink only; no device read/result is supplied. Native composition
also validates SGE aliasing, transfer preflight, high words/bounds, foreign bank/
executor, invalid counts and retired storage, with real PipeWire output retained.

Final coherent101-target build and114-check run:111 pass in56.36seconds. The same
3 campaign modes stop at38E8D3 / FE801100=88 FECTL bit3; no skips/aborts/additional
failures. Each retains1 reset,16 completed/0 queued commands,16 DMA allocations/
339,968 bytes, unloaded GP/EP programs, pendingD8, delivery=false and configuration_
returned=false. Ordinary live audio startup has not reached the newly wired GP
upload path. No opaque bit acceptance, empty installer or synthetic ACK was added.

Current382 tracked source/101 executable/13 installed fingerprints and evidence:
work/sandbox-primary-v15/native-gp-scratch-progress-v504.json. Launcher r6 remains
5900bf2c7902976132a4eb17a709dd999f8ecaaa1f6904e0bc64b9c1c70b3884. No candidate
accepted/installed; owned build/test/capture/display jobs retired.

Continue actual GP command/installation/ACK and native processing/status/supervisor/
clock/voice/GP-EP ownership, plus verified MCPX configuration/IRQ and codec/PCM.
Complete upstream descriptor/factory producers and asynchronous GP lifetime remain.
All map/BSP/AI/scripts/frame, Simulation/player/object/frontend, menus/maps/missions,
all102 elements, local/split/LAN and original rendered/audio/full-game gates remain.
Async/cache/animation and later kernel/GPU work stay open. Component passes do not
accept a whole element or replace the full requested goal.


## v508 — original EP preparation and real descriptor producers (PROGRESS)

Previous v504 was PROGRESS. Revalidated382 tracked source/101 executable/13
installed fingerprints before edits. Whole goal is uncompleted,0/102 whole elements.
Final get_goal reports usageLimited; no completion/pause/blocked update or
attempt to resume that externally controlled status was made.
All implementation edits stay private v15. Shared root code/MCC/shaders, existing
reference evidence/frozen snapshots and launcher-selected r6 remain preserved.
Root goal updates append only; no candidate was accepted or installed.

The command3 reference requires completed program/state installation before its
acknowledgement. Existing logical graph/output owners cannot certify that native
action. The authentic CPU callers also reveal the upstream four GP append pages:
whole38F6F4 ->38F5F3 produces3 actual EP pages, then38851E appends the separate
391410 descriptor. Continued those actual producers rather than minting an ACK.

Added8 whole original EP preparation CPU routines/242 static instructions:38851E,
38F544,38F5E2,38F5EF,38F5F3,38F66A,38F692,38F6F4. Shared SDK/GP/download CPU owners
are reused with unique entry ownership. Whole selector, source object/context
construction,9 owned source-data block copies and original parameter stores run.
Data is copied from the existing owned XBE bank; no new opaque processor payload,
DSP interpreter, runtime opcode executor or processor-ready result was introduced.
This does not execute those copied GP/EP data words as processor instructions.

The retained native DMA owner supports original mode0 EP table base/count stores
38DDF4/38DDFB, separately from GP mode1. Actual19 EP page entries resolve into the
same native64 backing used by CPU preparation. The actual parent page-translation
frame retains the original context in the saved ESI slot atSP+40; GP/EP prefix
selection follows that actual mode. The source GP scratch owner routes the EP
module and restores nested source context across synchronous calls/faults. Unknown
device reads/commands and original38846E remain unowned; no status bank or ACK0.

The new native composition uses the real SDK owner for its EP manager/context,
original38F53E and exact38E996 request arithmetic for the13000-byte EP allocation
and98-byte table, and original38F6F4 for source preparation. Its3 produced pages
append to GP through38F5F3/38850A; original38851E appends the fourth real descriptor.
GP prefix goes8->11->12 and accepts the real189-page title pool. No four synthetic
append descriptors are used in this new composition. The earlier v504 focused
fixture remains its explicitly declared-input regression. The complete upstream
38E2E0 hardware/factory caller remains unproved and is not bypassed in live startup.
Review also corrected the shared GP fixture's declared manager request from24
to the actual source38E253 request3C, retaining manager24..3B's six-word header
extent. Production already follows that original allocator caller; this is a
fixture correction, not a completed header publication or command ACK.

All6 selector blocks plus initial/loader/configuration copies match the actual
owned source bytes; configuration offsets and FIFO parameters match original
stores. GP/EP reads and writes alias the exact shared EP pages through their real
SGEs. Invalid bounds, high words, counts and executor/bank ownership reject.
The independent original EP CPU suite has26 cases, 639 exact
instructions, 162 ordered stores and4 stops before unsupplied
external services. Original registers/defined flags/full final storage and caller
FP state match. New EP and updated GP generation replay byte-identically. All
5,090 static instruction sites across10 audio CPU modules match the authenticated
owned XBE and stopped actual title code. This does not establish DSP/device parity.

Storage policy from current AGENTS.md/docs/TEST_STORAGE.md was incorporated before
validation. Used tools/run_tests.py with7 focused targets/checks in its own private
build; all7 pass. One-off generation/replay used tools/run_test.py. New original
experiment banks live only in an owned temporary-directory context inside
HALO2_TEST_SCRATCH. The private build/parent and replay scratch were verified gone;
only6 selected small native reports (2902 bytes total),
bounded logs, commands, source hashes and diagnostics remain. No new large artifact
was retained. No operation targeted the existing persistent build, prior
references, frozen snapshots or installed runtimes. Final audit found
97 of the prior101 binary paths absent; this
run did not remove or rebuild them. Their hashes remain historical v504 receipts,
not current executable evidence. Installed13 file hashes still match exactly.
Current source validation executables were disposable and are not installed.

Current389 source/13 installed fingerprints and prior101 executable audit
are recorded in work/sandbox-primary-v15/native-EP-source-progress-v508.json. The
current focused7 checks do not claim a fresh full114-check run. Prior111/114 and
its3 campaign FECTL failures remain historical v504 evidence, not re-run this turn.
Launcher r6 still has SHA5900bf2c7902976132a4eb17a709dd999f8ecaaa1f6904e0bc64b9c1c70b3884.
No owned build/test/capture jobs remain. No candidate or whole element accepted.

Continue verified MCPX configuration/IRQ semantics, actual native GP install/
command/ACK and full processing/status/supervisor/clock/voice/GP-EP/codec/PCM.
The complete hardware factory and asynchronous resource retirement remain required.
All map/BSP/AI/scripts/frame, Simulation/player/object/frontend, menus/maps/missions,
102 sandbox elements, local/split/LAN and original rendered/audio/full-game gates
remain. Async/cache/animation/later kernel/GPU obligations are still open. Bounded
source/resource checks do not substitute for the requested complete native game.

## Resumed full-game launcher checklist, 2026-10-19

The user explicitly resumed the full game/launcher goal and requested a persistent
checklist. [Full game launcher checklist](FULL_GAME_LAUNCHER_CHECKLIST.md) is the
maintained current view: G01-G14, ordered next actions, original owners/acceptance,
the unchanged 102-element matrix and a bounded evidence ledger to avoid repeating
completed work. Its private v15 counterpart is kept alongside this goal copy.
The service still reported `usageLimited` after the explicit resume request;
available goal tools cannot edit an unfinished objective or change that
host-controlled status. This attachment records the authorized continuation.

Active work is G02's explicit Xbox menu/launcher path, whose mandatory Vista
loading/menu dependencies prevent the original-owned menu route. The source
owners and required actual native/launcher validation are in the checklist.
Original lifecycle/menu-return, composed map/gameplay, every menu/map/mission,
all 102 elements and all six gates remain open. Installed r6 does not contain
source v508. This entry claims no new build, test, delivery or parity result.

## Current-state checkpoint before r9 qualification, 2026-10-19

The maintained [full-game checklist](FULL_GAME_LAUNCHER_CHECKLIST.md) now records
the actual candidate outcome. r8 is **unselected and not installed**. Its v515
build passed all six focused frontend CTests (1.26 seconds), the 84-definition /
150-pane authored check, and complete before/after source/native-guard checks.
v516 native smoke retains 39 passing cases, including 279 GPU capture rows, but
the 40th attempted case genuinely fails Tombstone/highplains2 startup. Later real
X11 idle/combat cases were not reached. These are bounded component results,
not complete qualification, original visual/audio parity or launcher delivery.

Exact reproduction is `work/sandbox-primary-v15/tombstone-r8-reproduction-v518.json`
and its retained native log/receipt. Runtime SHA-256:
`bb255d2739b35c890272a688ec0a5b8a25efc6fbd7d94cd538d8f9897ea6c0c5`.
Actual root `1CC8D0` reaches the missing original owner `1F7330`, called at
`1F7D4A`, return `1F7D4F`. G08/G10 now prioritize the controlled port of its whole
224-instruction shape-reference/object-node matrix body and actual live entry /
dispatch. Original ABI/callees remain mandatory. New compilation, independent
checks and real Tombstone admission remain unverified at this checkpoint.

Planned fresh r9 stages are v519 build, v520 native smoke and v521 installation;
none has run at this checkpoint. Installed r6 and existing reference evidence
remain preserved. The prepared post-install r8 updater was never executed and
must not claim delivery. The earlier r8 full-tree fingerprint is now historical:
the controlled code port and this docs-only checkpoint occur after qualification.
Changed private documentation paths/hashes are recorded separately in
`work/sandbox-primary-v15/current-goal-checkpoint-before-r9-v519.json`.

Queued G02/G11 work is current-map MP screen195/C3h: authenticated TU1.5 factory
`14A09B -> 14A0D9 -> 239104`, input `2390CA`, dispatch `2392B1`. v517 provides
static correspondence only; actual retained UI/channel/session owners, input,
Resume/cancel and disposal/return consumers remain required. Solo screen18
`238B15 -> 238AC2`, global `51DF54`, dispatch `238A67` and `1285A0` effects stay
separate. All G01-G14 gates and all 102 sandbox elements remain incomplete,
with the original six acceptance gates and every menu/map/mission/local/split/
LAN/render/PCM obligation preserved. No new large artifact is retained by this
documentation checkpoint.

## Current-state checkpoint before r10 qualification, 2026-10-19

r9 is **unselected and not installed**. v519 passes all seven production-library
CTests in 1.95 seconds, including whole `1F7330`: seven original CPU contexts,
768 instructions, 1,248 byte stores, four natural returns and three stops before
unsupplied external results. That component seam is resolved within those limits.
The actual Tombstone process passes its former `1F7330` failure and initializes
the GPU, then fails the next original shape-dispatch target `2F75F0`. It does
not complete six gameplay frames or reach later menu/X11-input smoke.

v520 retains 30 passing admission/cache cases and fails attempted case 31.
Immutable `work/sandbox-primary-v15/xbox-menu-smoke-v520-first-attempt.json`, SHA-256
`32ea30f68ffe4372116069b9038ec2e1ed0d7958195ef745234f98a84cea5142`;
actual runtime SHA-256
`e8920d27cd38863a0c7b3137eec6877a22065da29e952efd08594267388f486f`.
Diagnostic site `2EF6BB` is retail; actual TU1.5 dispatcher calls at `2F317B`,
returns `2F317D`, with actual retained shape vtable `461488`. The authenticated
closure audit/listings are `work/sandbox-primary-v15/tombstone-callee-closure-v524.json`.
G08/G10 prioritize this actual bounded closure and its shared/virtual owners,
preserving original ABI and unknown boundaries. No empty virtual result is allowed.

Planned r10: build v524, native smoke v525, installer v526; not run at this
checkpoint. r6 remains selected; historical r8/r9 receipts and failures are
preserved. Neither prepared post-install updater was executed. The prior r9
complete-tree fingerprint becomes historical after the controlled closure port
and this current-state documentation update. Exact changed private documentation
paths/hashes are in `work/sandbox-primary-v15/current-goal-checkpoint-before-r10-v524.json`.

The maintained checklist and its private copy retain all G01-G14 gates open,
0/102 accepted elements and all six original acceptance gates. Queued MP195
factory/list/input/lifecycle work remains static-only v517 evidence; solo pause
stays separate. Full menus/maps/missions/local/split/LAN/original frames/PCM and
historical MCPX/GP processing obligations remain. This docs checkpoint runs no
new build/test/capture and retains no new large artifact.

## Planned r10 map coverage amendment, 2026-10-19

Before the new build, planned v525 smoke now covers actual six-frame native
startup on **all 23 owned multiplayer maps**, 24 map/player cases including
Lockout with four local players. Tombstone runs first, then Lockout one/four,
Zanzibar, Desolation and the remaining canonical inventory. Require actual
runtime frame/player summaries and executed inode hashes for every case.
This plan is stronger than cache inspection; it still does not prove full map
lifecycle/gameplay, original pixels/PCM or a playthrough. These cases and
r10 v524/v525/v526 stages have not run at this checkpoint.

Read-only native-bank metadata at the r9 failure:
`work/sandbox-primary-v15/tombstone-r9-native-bank-v524.json`, SHA-256
`51cd741aec12250e987c4bfe1b9480d178bb14dc232ed4d47782990c36bb7e4a`.
It is native-only metadata, not original runtime execution or a supplied result.
Authenticated source now follows `2F75F0` wrapper/child dispatch to `3040D0` and
the published `41CC84` lifecycle. The missing bounded closure (ten bodies /
292 sites) is being batched; no completed native port or validation is claimed.
Frontend object-lifecycle work remains an independent check, not implied success.

Both checklists retain all G01-G14 gates open, 0/102 accepted. Exact amendment
documentation hashes: `work/sandbox-primary-v15/current-goal-coverage-amendment-before-r10-v524.json`.
No post-install updater, new build/test/capture or large artifact is produced
by this documentation amendment.

## Pre-r10 source and coverage correction, 2026-10-19

Correct the previous priority-map name: `cyclotron` is **Ivory Tower**;
**Desolation** is `derelict`. The maintained checklist now uses the original
inventory's titles; the earlier appended wording remains historical.

Latest corrected independent missing closure is **23 bodies / 1,586 sites**,
covering both published agent lifecycles, actual type20 helpers and nested
type20/type23 dispatch `3040D0`. Native source is generated; original CPU,
full production-library and actual live qualification are still pending.
Evidence `work/sandbox-primary-v15/tombstone-child-convex-welder-lifecycle-v525.json`,
SHA-256 `94a30251e62acc7ab5647ec6843da6ec4431e9fa0ec0b2d8e3548a9dcc8b2a51`.
Corrected `tombstone-transform-wrapper-independent-v524.json`, SHA-256
`9f16128ed2c4e4854a1700d49e9cb26286c03f5a55061aeb6bc95a02328d1de9`,
supersedes the earlier revision with reversed registry columns. Use authenticated
current ownership/entry evidence; no entry logic may be guessed from that error.

Planned v525 adds 24 native frames, one per map/player case, and a review contact
sheet in disposable scratch only. The 23-map/24-case startup plan and these
captures have not run. G01-G14 remain open, 0/102 accepted; no new candidate
delivery, original CPU/live result or retained large artifact is claimed.
Documentation hash receipt:
`work/sandbox-primary-v15/current-goal-source-correction-before-r10-v525.json`.

## Final source checkpoint before r10 build, 2026-10-19

Collision source port is complete and frozen: manifest
`work/sandbox-primary-v15/tombstone-collision-dispatch-port-v527.json`, SHA-256
`01b0b1d7f4d23dbbcd5327e4ffa103181abf0f72d506e5e02d1347b6ed0bbde6`.
Independent bounded original CPU report v526, SHA-256
`c112a8f896b65b3c70d0f38843386da24ac5ce38a7232907b79b793c731b89b5`,
passes 35 cases/all 23 entries, covering 541/1,586 sites: 1,131 observations
(1,129 executed instructions plus two pre-RDTSC stops), 2,163 byte writes,
12 returns and 21 external plus two timestamp boundaries without supplied results.
This is bounded standalone comparison, not full production-library/live proof.

Disposable injected native-module diagnostic v527 completes six frames / 15
ticks, report SHA-256
`988f24f80ddc0ad22cbc5b099c6205612d1115123aa294590bb0223badffaba6`.
It is DIAGNOSTIC ONLY, not untouched-r9 or fresh/uninjected-r10 acceptance;
installed runtime and prior real failures remain unchanged. Ordinary r10 build,
live startup on all 23 owned maps / 24 map-player cases, scratch-only frame /
contact-sheet review and installation are still planned, not run.

All 14 checklist gates stay open, 0/102 accepted, with full original scope and
all six acceptance gates preserved. No post-install updater ran. Documentation
hash receipt: `work/sandbox-primary-v15/current-goal-final-prebuild-r10-v527.json`.
This checkpoint adds no new build/test/capture or large retained artifact.

## User authority correction — MCC-only runtime assets, 2026-10-19

The user's current instruction supersedes every earlier Xbox-asset runtime /
delivery plan: **all runtime/menu/map tests use the port-owned imported/converted
MCC cache**. Xbox ISO data is only for offline validation of converted MCC tags;
Xbox/Vista reference banks, original XBE admission and MCPX material are not
onboarding or runtime dependencies. The decompiled OG Halo 2/TU1.5 behavior,
timing, precision and full one-to-one acceptance scope remain the target.

The maintained [full-game checklist](FULL_GAME_LAUNCHER_CHECKLIST.md) now places
the old Xbox priority/r10 plan behind an explicit supersession banner and marks
all prior Xbox startup/smoke/visual evidence as historical reference/component
evidence only. None passes MCC runtime or delivery acceptance. The canonical
[MCC integration checklist](/home/eric/Developer/Halo-2-Decompiled/docs/MCC_GAMEPLAY_GOAL_CHECKLIST.md)
is the active frontend/cache/gameplay dependency; reuse and update its checked
milestones rather than copying/resetting them. Its validated native default test
is Lockout. Bounded private frontend/unmuted/input progress and actual selected
runtime publication are distinct; menu delivery is not declared accepted here.

Current priority is that MCC path's actual frontend caller/integration and
selected-runtime gates, followed by menus and every imported MCC campaign/MP map
with cache/conversion/runtime hashes and opened-file provenance. Historical
FECTL `0x88`, GP/EP and Xbox-authored shape dispatch are not current MCC launch
blockers; reconstruct only a demonstrated dependency of the reused MCC path.

The outer storage owner canceled r10 build session 28121 with exit 143 and
verified `halo2-test-q5h6cwi5` scratch removed. No r10 candidate exists; no
asset/source-backed tests, stage or installation completed. The v528 Xbox-r10
post-install helper is retired and refuses execution; it was never applied.
All G01-G14 gates remain open, 0/102 accepted, with all original six acceptance
gates and menus/maps/missions/local/split/LAN/render/PCM scope preserved.
Docs/provenance receipt: `work/sandbox-primary-v15/mcc-runtime-authority-correction-v529.json`.
This update executes no game/test/import and retains no new large artifact.

## v530 bounded MCC shortcut delivery, 2026-10-19

Verified root `Halo2-Sandbox-Test` now selects the MCC helper, GUI label
`MCC sandbox test  ›`. Installed `--check` passes: cache v13, native capability
true, frontend false. Seven installed CLI guard/info cases and 18 existing real
Tk launcher tests pass; unsupported source/runtime/map overrides reject and
`--menu` refuses the unqualified frontend. Eight protected files remain exact,
scratch root is empty and no new large artifact is retained. Receipt:
`work/sandbox-primary-v15/mcc-sandbox-shortcut-delivery-v530.json`, SHA-256
`2725075b6d56d4930b7c7f7ca40f15c95ff7126fed1d3fb54ec84c148f9c9736`, 5,270 bytes.

The named shortcut-delivery subtask is checked in the canonical MCC checklist
and both full-game ledgers. Earlier synthetic routing and direct six-hidden /
muted-frame asset-source proof retain their separate bounds. Installed-shortcut
through actual gameplay, visible/audio, menus, all-map lifecycle and full original
parity are not established here. All G01-G14 gates remain open, 0/102 accepted.
MCC-only runtime authority and ISO-only-for-offline-converted-tag-validation
remain unchanged. This record repeats no tests or audits.


2026-10-19 v532: goal service is active. Actual MCC menu catalogue integration passes48previews/40launch-header routes, with full-source/frozen-input ownership and no forbidden asset reads. The twelve-map hard limit is removed in source; selected renderer/launcher delivery,2blank DLC identities, energy-map gameplay, pause/sessionreturn and all102elements/sixgates remain open. See `work/sandbox-primary-v15/mcc-map-picker-integration-v532.json` (SHA4f983c985d3fba34f833cd04821340e82bd54cfb216dcc0dd340750e8887178c). NativeMCCcharge/release composition and whole CTF/Assault initializer ownership are now the parallel active frontiers.


2026-10-19 v535: complete original CTF/Assault initializer closure is ported and production-registered. Three original CPU versions pass879 calls/293 normalized cases; optimized native, ASan/UBSan and focused CMake comparisons pass with5MCC marker inputs. Oracle/compiler/CMake scratch removed. Canonical MCC and both identical FULL checklists updated before dependent activation/objectbirth work. Objective flag/bomb logic gates marked partial only; all102 IDs/sixgates preserved,0accepted. Receipt `work/sandbox-primary-v15/ctf-initializer-milestone-v535.json`; no mode admission, selected-runtime change or full parity claim.


2026-10-19 v542: complete original1030F0 programtables restored (26words/104B,8newimmutablewords,51originalselectors), coherent cold native comparison and actual MCC SMG pass. Strict charge advances past104698 through hold/release and141effectframes, then real2F8B27 needs vtable45F8B4; positivechargegraph absent/unverified. Current-rootcold executable passed on frozen inputs but aggregate sourceguard correctly detects concurrent changes. Canonical MCC and identical FULL checklists now track this single current charge frontier; all102IDs/sixgates preserved,0accepted. Receipt `work/sandbox-primary-v15/mcc-weapon-program-tables-progress-v542.json`. Scratch removed, no runtime install or new largeartifact.


2026-10-19 v552: realprivate43-map MCC preparation/bitmap extraction/strict648-frame inventory/563-DDS+40-font assembly PASSED;14exactinstant-chargewords and6cataloguestores changed, distinctFloodlabtuplepreservedunqualified.4,332,372,016 generatedbytes removed, allselectedassets/source/launcherowners preserved. Combinednativechildstoppedbeforelink at copiedfactory extra-brace syntaxerror, sincecorrectedbyconcurrentowner; failedreceiptpreserved. RerunONLYnativeprobeusingfreshownedMCCcorecopies; doNOTrepeatpassedfullpreparation. Receipt `work/sandbox-primary-v15/mcc-private-preparation-milestone-v552.json`. NewCTFactivation278case/CMake qualificationv551 is separatelysource-only,actualsamebankfactorypending. Fullgame/102elements/sixgates remainunaccepted.
