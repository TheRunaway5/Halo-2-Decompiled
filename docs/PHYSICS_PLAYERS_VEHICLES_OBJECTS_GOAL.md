# Player, vehicle and object physics: 1:1 port goal

The active goal is to port **all physics logic for players, vehicles and objects
until it matches English TU1.5 retail 1:1**. TU1.5 is the sole parity target,
per the user's scope correction on 2026-10-11. Existing Map Pack 1.1 evidence
is historical and does not create another parity requirement. Selected arithmetic tests do not complete this
goal. Authority is original executable instructions, original assets and
verified retail behavior. Unknown behavior stays unresolved until evidence is
available; substitute motion, contacts, solver results or tuning cannot satisfy
parity.

The owned English TU1.5 executable is pinned to
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Legacy correspondence IDs remain internal references in retained source modules.
All new parity execution and acceptance validation target TU1.5.

## Required coverage

| Area | Required behavior and integration |
| --- | --- |
| Shared engine | Actual body/shape/world/allocator construction and lifetime; authored shapes and collision filters; broadphase, terrain/MOPP and narrowphase; contact generation/retention/removal and game callbacks; material friction/restitution; constraint/Jacobian generation, iterative solver, warm starting, integration/readback; islands, activation/sleep, deferred operations, forces/torques/impulses and retirement. |
| Players and other bipeds | Original controller dispatch and tick ordering; walking, air movement, crouching, jump/rearm/fatigue, slopes, steps, ceilings/ledges and support transitions; moving/rotating supports and carriers; other authored movement modes; physical activation, root-motion/animation history, collision contacts and authoritative object state; death transitions, ragdolls and corpse lifetime. |
| Vehicles | Source-created vehicles and components; each original vehicle/controller and motion class; authored steering, acceleration/deceleration, engine torque/gears, suspension/contact forces and applicable hover/flight behavior; collision, impulses, activation and destruction; seat/driver/boarding transitions that alter physics; controller-to-body force delivery and actual solved readback. |
| Objects | Dynamic crates/props/items, equipment and projectile physical behavior, attached/parented objects, devices and moving machinery; lifts/conveyors and occupancy queries; breakage and destructive state transitions; collision masks, contact/material/impulse delivery and object creation/retirement. Combat effects/damage remain separately owned, with the actual physics events delivered to their owners. |
| Composed runtime | Replace the temporary player terrain mover only after real source contact/solver ownership exists. Preserve identity, state ordering, moving supports, player interactions and multiplayer authority. Verify campaign and multiplayer authored scenarios, maps, split-screen and LAN paths. |

## Current verified boundary

The preexisting native world component has extensive authenticated bodies, but
the live `Simulation::move_unit` path still uses the temporary host terrain
mover. [World/motion progress](PHYSICS_ENGINE_PORT_20261010.md),
[solver progress](PHYSICS_SOLVER_PORT_20261011.md) and
[world callback work](PHYSICS_WORLD_LISTENER_ABI_RUNTIME_PORT_20261011.md)
record their individual boundaries. Another active chat is extending that shared
world/contact graph; this goal's initial implementation adds a separate vehicle
control component.

`source_vehicle_control_accelerate` ports complete `208D80` / `20D680`.
`source_vehicle_control_approach` ports complete `208E70` / `20D770` and its actual
acceleration callee. Their real vehicle-update/controller callers are separately
authenticated in both executable images. Native execution preserves authored
limits, forward/reverse rates, turn-through-zero interpolation, original x87
multiply/float-store boundaries, SSE comparisons, target clamping, and every
value store, including repeated stores and aliasing with the rate bank.

The initial differential corpus executes **33,696 cases per original image** and
compares each with the native selected-image API: **67,392 native comparisons,
118,824 ordered stores and 4,313,088 full output-bank bytes**. All 94 original
instructions are exercised. Inputs include all four rounding modes, x87 24/53/64
precision, signed zero, finite boundary values, subnormals, infinities, quiet
NaNs, reversal, overshoot and value/rate aliasing. This is declared scalar input
coverage, not that many vehicle-driving scenarios.

The semantic API checks the output bank, store ordering and target result. It
does not preserve an original caller's scratch stack, registers, flags or x87
stack. A future controller integration must supply those owners wherever they
are observable. The x87 backend currently requires GCC/Clang on x86; masked FP
exceptions are required. Complete driving controllers, force/body delivery,
seats, suspension, hover/flight classes, collisions and live vehicle scenarios
remain unfinished.

Evidence is retained under
`analysis/multiplayer/sandbox-source/vehicle-control-20261011/`. The original-only
oracle never executes the native implementation and patches no instructions.
`tools/research_vehicle_control_20261011.py` authenticates the complete caller
chain; `tools/oracle_vehicle_control_20261011.py` independently executes both
original images. `original_source_vehicle_control` runs the native comparisons.
Release and sanitizer evidence must be recorded separately from live parity.

## TU1.5 vehicle remainder and retained FP state

The [TU1.5 remainder component](PHYSICS_VEHICLE_MOTION_MATH_TU15_20261011.md)
adds 16 complete native bodies and 443 distinct instruction sites. Its composed
graph has 96 bodies and 11,056 sites. A test-only Linux i386 harness single-steps
the owned TU1.5 instructions on the host CPU and preserves actual unported-owner
boundaries. Native comparison passes 8,598 cases, including 506,987 ordered
writes, full retained state, active extended values and FP controls. Original
hardware execution, rather than Unicorn's precision-flag and signed-zero
behavior, is the final authority for this component. Only TU1.5 execution is
part of the new acceptance evidence.

The sealed 53-file source cohort passes all four TU1.5 vehicle checks in Release
and under ASan/UBSan. Production passes those four checks and ten TU1.5 world
checks. Supplemental contact checks pass 50 complete cases and two retained
earlier-boundary cases with the new shared headers. The
[validation record](../analysis/multiplayer/sandbox-source/vehicle-motion-math-20261011/validation.json)
pins source, generated code, repeated original hardware fixtures, binaries and
test logs. Its SHA-256 is
`57b1b0db72d685185cbefab2ba9ab269acdf4e1f59eddcd68fa22984105441bb`.

The shared ABI retains FP controls, MXCSR and x87 sticky exception flags through
nested owners. Original source bytes remain outside production execution.
Tank and wheeled staged paths now reach TU1.5 contact-state delivery `20FD70`;
their declared no-solver inputs do not prove suspension or live contacts.
Unported math-error, contact/solver, animation and activation owners, live
movement integration and the rest of the required scope remain open.

## TU1.5 contact-state delivery and reference release

The [contact-state component](PHYSICS_VEHICLE_CONTACT_STATE_TU15_20261011.md)
adds complete TU1.5 `20FD70`, `20FCB0` and `22E1B0`: three bodies and 160 sites.
The composed vehicle graph now contains 99 bodies and 11,216 distinct sites.
All new recovery, generation and parity execution use the owned TU1.5 image.

Native comparison passes 12,518 hardware-backed cases: 10,086 complete calls
and 2,432 actual required-owner prefixes, 3,433,952 observer events, 581,756
ordered writes and 1,865,363 retained bytes. The hardware observer single-steps
3,430,820 original instructions. Repeated recovery, generation, diagnostic
original execution and original hardware execution match exactly. Release,
ASan/UBSan and production checks pass. The sealed source cohort includes 90
native and local Python source files; unchanged parent archives are reused for
standalone checks with their hashes recorded.

All 432 independently computed builder-to-motion cases now return, including
27 tank and 27 wheeled inputs. Their declared component still has no actual
vehicle solver object or constraint list, so this is bounded state/force
calculation evidence. It does not establish suspension, collisions or driving.
151 of the 160 new sites are exercised; nine sites after required cleanup
calls remain unexercised at that stage. That stage identified TU1.5 cleanup
`22E390` (121 instructions), `22FE30` (27) and `22FEA0` (775) as its next
frontier. The following component ports those bodies and their reached
dependencies while preserving deeper required owners.

The [validation record](../analysis/multiplayer/sandbox-source/vehicle-contact-state-tu15-20261011/validation.json)
pins source, tools, repeated original fixtures, native binaries, parent archives
and logs. Its SHA-256 is
`b6c71f4725c803f9e47f4a39b7627699baf6a34327a722f8908bdb4471040430`.
Full player, vehicle and object physics parity remains incomplete.

## TU1.5 constraint cleanup, allocation and force delivery

The [constraint-cleanup component](PHYSICS_VEHICLE_CONSTRAINT_CLEANUP_TU15_20261011.md)
adds 21 complete TU1.5 native bodies and 2,565 distinct sites. Its composed
vehicle graph contains 120 bodies and 13,781 sites. The already compiled
TU1.5 `1D8C80` is reused. All new recovery, generation and parity execution
use only the owned English TU1.5 image.

Native comparison passes 15,141 original hardware cases: 12,689 complete calls
and 2,452 actual required-owner prefixes, 3,566,476 observer events, 603,422
ordered writes and 2,010,369 retained bytes. The hardware observer single-steps
3,563,085 original instructions. Repeated recovery, generation, diagnostic
original execution and hardware execution match exactly. Release, ASan/UBSan
and all six TU1.5 production vehicle checks pass. The sealed source cohort
contains 99 native and local Python files; the unchanged validated parent
archives are reused with their exact hashes recorded.

The new corpus includes 120 real datum-pool allocations, 40 source-created
reference records, 80 force updates, 12 cleanup-to-force calls and 288
contact-basis cases. Both implementations compute their own allocated slots,
force records and linked 13-float transform copies. New floating-point paths
preserve the original defined x87 C1 result as well as retained extended values,
FP controls, MXCSR and sticky exceptions. 1,351 of the 2,565 new sites are
exercised. Object lifecycle `178E30` is compiled but unexercised, and 1,214 new
sites remain uncovered.

The [validation record](../analysis/multiplayer/sandbox-source/vehicle-constraint-cleanup-tu15-20261011/validation.json)
has SHA-256
`41748ed540d156528c696f472011771673027280757c4445787a6eacf2b53f57`.
Deeper object, activation, point/pose-transform and force-cleanup owners remain
required. The 432 staged builder-to-motion inputs still declare no actual
vehicle solver object or constraint list. The live `Simulation::move_unit`
still uses the temporary host terrain mover. World/contact/solver ownership,
live player/vehicle/object integration, complete controller classes and authored
scenario traces remain required. Full physics parity is incomplete.

## TU1.5 environment queries, pose composition and force retirement

The [environment-state component](PHYSICS_VEHICLE_ENVIRONMENT_STATE_TU15_20261011.md)
adds 26 complete TU1.5 bodies and 1,307 distinct sites. The composed vehicle
graph contains 146 bodies and 15,088 sites. New owners cover point/portal and
packed-plane-tree queries, cluster/category and activation metadata, datum and
linked-record retirement, force detachment, scalar delivery and source pose
composition. All new recovery and original execution use English TU1.5 only.

The corpus contains 16,381 original hardware cases: 13,942 complete calls and
2,439 actual required-owner prefixes. It exercises 1,226 of the 1,307 new sites
and 10,048 sites across the composed graph. The hardware observer single-steps
3,648,075 original instructions; the normalized native comparison checks
3,651,499 observer events, 613,055 ordered writes and 2,082,340 retained bytes.
Repeated recovery, generation, diagnostic original execution and hardware
execution match exactly. All 432 staged builder-to-motion cases return with
their declared inputs; these still lack a vehicle solver object and constraint
list and do not establish solved contacts or driving.

The 108-file source cohort pins three shared ABI header changes for explicit
MMX state ownership. The accepted standalone builds compile the full graph
from source, because the earlier parent archives predate that ABI. These
vehicle inputs enter with no active MMX state; mixed MMX/x87 parity remains
outside this corpus. Release, the full ASan/UBSan rebuild and differential
check, and all seven TU1.5 production vehicle checks pass.

The [validation record](../analysis/multiplayer/sandbox-source/vehicle-environment-state-tu15-20261011/validation.json)
pins the 108-file source cohort, repeated original fixtures, full source builds,
native binaries and successful logs. Its SHA-256 is
`239bb07c92de97930be7ccb27400982621915e21e1377e8165fd1c48c8a8c50a`.

81 new sites remain unexercised. Linked-record cleanup `178290`, object
lifecycle and other retained required owners remain open. `223B20` is the
sound scheduler submit owner, not physics activation. Its presence in a
physics callback chain does not make its implementation a physics requirement.
The [semantic correction](PHYSICS_TU15_OWNER_SCOPE_CORRECTION_20261011.md)
supersedes the earlier labels while preserving the sealed evidence.
Existing bounded vehicle cases are not an authored live scenario.

## TU1.5 world startup remains required

The latest shared [world-construction component](PHYSICS_WORLD_CONSTRUCTION_RUNTIME_20261011.md)
contains 421 authenticated bodies and 33,237 sites. Its complete
116,677-instruction construction-prefix scenario still executes the older
image, so it does not satisfy this goal's TU1.5 construction requirement.
Earlier TU1.5 world checks explicitly rebind retained raw inputs; they do not
establish execution of the TU1.5 startup caller or complete game-world
construction.

The next shared-engine acceptance work must establish TU1.5 process/heap,
world/BSP/body and controller construction with original TU1.5 inputs and
composed traces. Actual allocation, lifetime, contact/solver ownership and
solved readback must precede replacement of the temporary live terrain mover.
Player, vehicle and object integration, all authored controller classes,
campaign/multiplayer scenarios, split-screen and LAN remain required.

## Fresh TU1.5 startup record owners

The [TU1.5 startup-record component](PHYSICS_TU15_STARTUP_RECORDS_20261011.md)
adds native ownership of the exact DPC and timer-record initialization calls
reached before process-heap creation. It composes the existing source heap and
shutdown owners, validates the actual TU1.5 return/argument frames and preserves
all fields outside those initializers' ownership. Timer scheduling, DPC dispatch
and hardware/settings outcomes are not supplied by these operations.

24 fresh TU1.5 procedure-entry prefixes use literal image/header storage and
declared standalone stack/CPU inputs. They restore no old world checkpoint and
execute no older image. Independent original execution and compiled primary
record expressions match 504 source instruction observations, 348 source
stores, 360 record stores and 48 registry stores, including their interleaving
and retained CPU/memory state. Another 36 patterned record cases compare 270
independent primary writes and untouched fields. All 169 instructions in the
three retained startup/setup/timer-arm bodies are authenticated; these prefixes
exercise 28 distinct sites. Repeated original recovery, primary compilation
and measurements are identical.

The 108-file cohort passes sealed Release, ASan/UBSan/leak and normal project
target checks. The [validation record](../analysis/multiplayer/sandbox-source/tu15-startup-records-20261011/validation.json)
has SHA-256
`40df49b2c1698e39c63d913bc1f9b275c9ce6270dd8a0a56898097ce77e2eefd`.
This establishes declared native record/void-ABI semantics against the pinned
primary compatibility implementation, not original Xbox kernel clobbers.

That sealed stage reached TU1.5 `2E00E7`'s unresolved hardware-data322 read through
`41B66C`. The encoded import is not a hardware record. Original measurement
stops before consuming it, and native execution refuses the missing bank while
retaining the same preceding state. Hardware data, conditional settings/error
conversion, real timer clock/queue/dispatch ownership and later startup owners
remain required. The process heap has not been installed; the XBE entry point,
full TU1.5 startup/world creation and live physics remain unproven.

## TU1.5 phantom forces and body velocity delivery

The [phantom-force component](PHYSICS_PHANTOM_FORCE_TU15_20261011.md) adds
12 complete TU1.5 bodies and 1,314 instruction sites. The composed dependency
graph contains 158 bodies and 16,402 sites; semantic sound/effect dependencies
in the retained graph do not count as exclusively physical behavior. The new
owners follow the actual vehicle call into phantom updater `1DA750`, physical
membership and force-factor logic, and the real linear/angular velocity methods
selected by the original motion vtables.

528 independently executed TU1.5 cases contain 377 complete calls and 151
required-owner prefixes. Of 193 complete updater calls, 153 reach both motion
setters, 81 change linear velocity and 15 change angular velocity. Original and
native execution compute these results independently from declared raw component,
body, tag/model/physics, membership, motion and island inputs. These records
were not created by a source factory or collision producer in this corpus.
The comparison retains 195,014 normalized instruction observations, 39,156
ordered writes and 153,663 bytes; hardware single-steps 194,838 unchanged
original instructions. Recovery, generation, diagnostic original execution and
original hardware execution repeat identically. 1,214 of 1,314 new sites are
exercised, leaving 100 uncovered sites.

The sealed 118-file source cohort passes all eight TU1.5 vehicle/phantom suites
in Release, ASan/UBSan with leak checks, and the normal project build. Parent
sources are unchanged and every standalone dependency is rebuilt from source.
The [validation record](../analysis/multiplayer/sandbox-source/phantom-force-tu15-20261011/validation.json)
has SHA-256
`9b106d2991f0b2521668c03b360440c731cc869848a13b83c1cc41f29c831a2b`.

CRT power `37CE0C`, world activation `3165D0`, empty-membership allocator
ownership, TU1.5 source-created world/body/contact state and live integration
remain required. The retained world-construction graph already compiles
`3165D0` under its internal `312A90` identifier, and `316D40` under `3131C0`.
Their phantom composition needs valid TU1.5 world state and the actual ABI
binding; duplicate recovery of those bodies is unnecessary. There is no host
power or fabricated allocator/activation result.
This is physical force and velocity-delivery evidence, not a working
live lift, vehicle suspension, solved collision trajectory or authored scenario.
The full goal remains active.

## Acceptance

The user explicitly requires **visible testing in actual multiplayer** so the
physics can be seen working, as of 2026-10-11. Feature acceptance must include
real authored-map match steps, normal input, split-screen and LAN authority
checks, saved tick observations and playable recordings. Recovered components
must own the live bodies/contacts/world; merely linking them or showing motion
from the temporary mover cannot establish parity.

The [first visible TU1.5 acceptance run](PHYSICS_MULTIPLAYER_VISIBLE_TU15_20261012.md)
records native walking/crouching/jumping, a bounded grenade lifecycle and exact
same-tick UDP host/client movement observations. **The authored Lockout lift is
red (zero upward velocity), and Coagulation driving is unavailable (six authored
vehicle candidates, zero live scene vehicle roots).** Other dynamic-object
impulses and the complete original contact/solver integration remain open.
The videos retain these failures; neither a component pass nor this bounded
native gameplay check completes 1:1 physics parity.

Each missing owner must be recovered and integrated rather than supplied by a
recorded output or successful default. Completion requires the full shared
engine, every applicable player/vehicle/object path and real composed original
versus native scenario traces. Builds, scalar fixtures and isolated solver
comparisons provide evidence for their specific contracts; they do not establish
continuous gameplay or Xbox hardware equivalence. The goal stays active until
the entire stated scope has supporting evidence.

## Wiring each completed system

Per the user's explicit requirement, each completed system must be wired into
its actual runtime caller as it is finished. Linking a library or passing an
isolated fixture is not completion. Required evidence includes source-created
input/lifetime owners, original tick ordering and state readback, and actual
multiplayer operation with saved traces and recordings. Until those exist,
record the component as procedure-verified and keep its live integration open.
Prioritize the fresh TU1.5 world/body/contact producers needed to connect the
already ported systems, rather than treating additional isolated bodies as
finished gameplay.

The new activation callbacks are connected to their real source callers inside
the compiled graph: velocity setters enter `315CB0`, world activation enters
`316D40`/`322660`, and installed world/body listeners dispatch to the new native
bodies. All 36 composed wake cases exercise that chain. `Simulation::step` still
has no source-created world/body context to call this chain; the live lift and
vehicle failures remain required integration work.

## TU1.5 phantom/world activation composition

The [activation component](PHYSICS_PHANTOM_WORLD_TU15_20261011.md) joins the
verified TU1.5 vehicle and current world graphs and adds five complete native
callbacks and helpers, 477 sites. The compiled union contains 609 bodies and
51,007 sites. Constructor stores authenticate the installed world/body listeners
and the body owner getter. Entry, instruction and store observations use actual
TU1.5 addresses across component boundaries.

The independent original i386 hardware corpus contains 671 declared-input
procedure cases: 521 complete returns and 150 retained CRT power frontiers.
It records 233,894 hardware instructions, 234,090 normalized observations,
46,208 ordered writes and 191,303 state bytes. All 36 lower velocity-setter
cases wake sleeping islands and run both real installed listener classes before
storing velocity. The full phantom updater skips inactive members, preserved by
20 additional negative cases. Only 249 of the 477 new sites are exercised;
deeper pair-agent and constraint builders remain outside this corpus.

This is raw declared world/body/island input evidence. It does not create live
bodies or contacts and does not resolve the recorded multiplayer lift/vehicle
failures. The live mover still uses host terrain collision. Full TU1.5 factory,
world/contact producers and actual solved readback remain required.

The production build exposed an unexercised parent-world lowering defect:
TU1.5 `2830FE` is a qword x87 load from `46A688`, but the shared world generator
emitted a float load through its four-byte scalar reader. The
[TU1.5 callback correction](PHYSICS_WORLD_QWORD_TU15_20261012.md) now reads the
binary64 value directly into extended precision, and the scalar reader rejects
oversized operands before touching memory. This is installed in the shared
477-body world graph; regeneration changes exactly one lowered instruction.
Exact original callback and shape-constructor bindings were retained in
`analysis/multiplayer/sandbox-source/phantom-world-tu15-20261011/shared-world-qword-load-frontier.json`.
That original frontier record remains historical. The new original TU1.5
hardware corpus completes 288 callback cases, including 240 executions of the
qword load, 31,488 instructions and 4,512 ordered writes. The pre-fix native
binary terminated with stack corruption; the corrected Release, ASAN/UBSAN/leak
and main-project builds each pass all 11 selected TU1.5 checks. The sanitizer
suite completed in 697.62 seconds. Results and the immutable source cohort are retained under
`analysis/multiplayer/sandbox-source/world-qword-tu15-20261012/`.
Its `validation.json` SHA-256 is
`86ee686a0fe715800a528afe8c310ddc0764becd71b2bdbd861d4c9d644f88c3`.
These comparisons consume declared raw pair/shape/transform inputs. Fresh body
and contact creation, live world ownership and the multiplayer lift/vehicle
acceptance failures remain open.

The activation slice is sealed against frozen source-v5 by
`analysis/multiplayer/sandbox-source/phantom-world-tu15-20261011/validation.json`
(SHA-256 `f04a4799672b5e50e43d2a39a26db17dc628d4d36e66e1dbd0bf2737a6ccdab4`).
All nine selected TU1.5 checks pass in Release and ASAN/UBSAN/leak builds.
Production registration separately passes the same nine checks using the
recorded v18 world source. The newer 477-body graph with the qword correction is
verified separately by the callback correction cohort above. The earlier
activation seal and its v18 production record are not rewritten to claim those
later inputs. No new live gameplay parity is claimed by either component slice.

## Fresh TU1.5 kernel data and startup dispatch ownership

The [kernel-data startup component](PHYSICS_TU15_KERNEL_DATA_STARTUP_20261012.md)
adds actual externally owned export322 input to the authenticated native image
loader and public startup join. Two isolated fresh xemu boots and an additional
same-boot read retain identical hardware/kernel-version data and export
metadata. The reference is the user's owned modified Complex4627 BIOS in xemu,
with private EEPROM copies and HDD overlays; it is not physical Xbox or title
execution proof. The observed hardware record differs from the compatibility
library default, so no default hardware flags are substituted.

The loader authenticates the whole TU1.5 image, binds only the actual four-byte
import cell at `41B66C` before mounting its read-only section, and retains the
actual eight-byte record as immutable resource backing. The public startup
wrapper now observes the current generic DPC/timer dispatcher, fixing its
previously unreachable interception. Missing ownership stops explicitly before
the original `2E00D4` load. The older sealed missing-data fixture is preserved;
its regression derives that earlier boundary from the independent timer entry
and declared preserving void ABI.

All 24 fresh original/native startup cases now reach the unchanged settings
callee at actual `2E0FE2`, retaining 744 source instructions, 492 source stores and
408 primary platform stores. Exact call order, pending frame, memory and live
CPU state compare, with no settings result supplied. All three selected TU1.5
checks pass fresh Release and ASAN/UBSAN/leak builds; both startup checks also
pass the main-project build. Final source-v3 contains 76 files; its native inputs
are byte-identical to compiled source-v2. The only v3 change fixes hash-variable
shadowing in the evidence validator. Both root and frozen validators reproduce
the final seal, `validation.json` SHA-256
`e7e36d375d773028bd22f7574bf56d7833892d3f0b1323cf7bf5ae544a4b32b3`, under
`analysis/multiplayer/sandbox-source/tu15-kernel-hardware-20261012/`.

The next authenticated dependency is the original settings wrapper/thunk,
which calls EEPROM query 24 for `XC_MISC` and status conversion 301. The original
startup later invokes timer 149. Static original listings and pinned primary
compatibility references are retained in `settings-owner-research-v2/`; these
owners have not been supplied successful defaults. The full startup caller,
heap installation, world/body/contact producers and actual `Simulation::step`
integration remain open. The multiplayer lift/vehicle failures still require
live acceptance after that integration. This startup component is not a
completed player, vehicle or object physics system.

## TU1.5 settings caller and immutable query owner

The [settings startup component](PHYSICS_TU15_SETTINGS_STARTUP_20261012.md)
now connects the complete original settings wrapper/thunk to the fresh startup
caller. The query consumes the owned EEPROM's actual UserSettings section and
computes its checksum. Invalid data follows the original failure path. No
settings-success return or kernel default is substituted. Independent original
execution uses pinned primary query/checksum/lookup code with explicitly
declared immutable resource, synchronization and byte-copy primitives.

All 48 fresh original/native prefixes compare through the actual status
conversion call: 2,016 source instructions, 1,416 source writes and 264 query
writes, with 24 actual successes and 24 checksum failures. Release and
ASAN/UBSAN/leak builds each pass four selected TU1.5 checks. The main project
passes all three startup checks. Production artifacts are retained in an
immutable snapshot. Root and frozen validators reproduce the evidence seal at
`analysis/multiplayer/sandbox-source/tu15-settings-startup-20261012/validation.json`,
SHA-256 `ba97568822148cee816511208e6f5415ca417a7cd8afcffe37774120bd522ab1`.

Status conversion301 and timer149 remain required. The source process heap,
world/body/contact creation and `Simulation::step` integration are still open;
the visible multiplayer lift and vehicle failures remain acceptance failures.
This component supplies no new live physics parity claim.

## TU1.5 complete settings return and connected status conversion

The [status startup component](PHYSICS_TU15_STATUS_STARTUP_20261012.md) now
connects the complete settings return path into the actual fresh startup
caller. It statically translates the complete owned modified Complex4627
export301 and consumes its exact immutable mapping tables. Two isolated boots
retain identical code/tables. No mapping result, Windows default or rewritten
table is supplied. This reference proves the owned modified BIOS routine,
with physical Xbox behavior explicitly unverified.

All 48 fresh original/native prefixes now complete query, status conversion,
source wrapper return and caller success/failure stores, then reach the actual
timer149 frame. They retain 2,760 title instructions, 1,800 title stores, 8,112
kernel instructions and 144 kernel stores. A separate 786-case corpus covers
every mapped status and range edges through 366,026 original instructions;
58 of 59 sites execute. The remaining jump is unreachable with the preserved
tables and the routine's input-bit early return. Both corpora and native
generation repeat byte-identically. Six selected TU1.5 checks pass fresh
Release and ASAN/UBSAN/leak builds, and all five production startup checks pass.
Root and frozen validators reproduce the immutable source/artifact seal at
`analysis/multiplayer/sandbox-source/tu15-status-startup-20261012/validation.json`,
SHA-256 `3badeb60e3975b150330470a587f453cc81b2c77687d17393ffefea4136c23db`.

Timer149, full process startup and source heap/global installation remain
required. No source world/body/contact result has been supplied. The
`Simulation::step` integration and visible multiplayer lift/vehicle acceptance
failures remain open; this connected startup prerequisite is not a completed
player, vehicle or object physics system.

The next timer owner's original modified-BIOS bytes are retained under
`analysis/multiplayer/sandbox-source/tu15-timer-startup-20261012/`. Static
recovery authenticates complete149/150 functions (26/186 bytes, 7/72 sites),
including the actual call to150 and six remaining kernel helpers. Export
metadata identifies129,163 and119 among those helpers; three are internal.
The actual title frame requests relative time `-216000000000` in100ns units.
Neither scheduler/clock state nor a timer return has been supplied. These
static windows establish the next owner boundary, not completed scheduling.

## Connected TU1.5 timer callers and original queue construction

The [timer prerequisite component](PHYSICS_TU15_TIMER_STARTUP_20261012.md)
now connects original149/150 to the public fresh startup composition. It
recovers nine complete modified-Complex4627 bodies and304 sites, including
actual timer/ready-queue construction, DPC/timer record initialization and
due/index/insertion arithmetic. Production uses native control flow. Only nine
authenticated read-only arithmetic bytes are mounted from the captured text;
no mutable scheduler state, clock value or timer result is substituted.

The original producer corpus has37 fresh cases,15,536 instructions and6,317
stores. It constructs queues and timer records before dependent calls and
compares maximum64-bit arithmetic, carries, absolute/relative time, expiry and
multiple insertions. Clock values are declared procedure inputs, not a live
clock resource. An ADC/SBB lowering defect found by the maximum input case is
fixed; failed evidence and diagnostic traces remain historical.

All48 fresh startup prefixes now execute the actual timer prologues and reach
`8001434A`, which requires the real interrupt-context byte at `80035C00`.
They retain2,760 title instructions,1,800 title stores,8,832 kernel instructions
and720 kernel stores. No context byte is read or injected. Combined new graph
coverage is210/304 sites;94 remain unverified. Original corpora and generation
repeat byte-identically. Eight selected checks pass fresh Release and
ASAN/UBSAN/leak builds; all seven production startup checks pass. Root and
frozen validators reproduce the immutable119-file source/artifact seal at
`analysis/multiplayer/sandbox-source/tu15-timer-startup-20261012/validation.json`,
SHA-256 `11ef159431dbcac9f0a6f3909a1c3519c812b9f3cf97edb0f8f714dd886acc57`.

Actual native interrupt-context/clock construction, resource lifetime and
expiry/DPC servicing remain required before the timer can arm and startup can
return. The process heap/global, source world/body/contact/controller creation,
`Simulation::step` integration and visible multiplayer lift/vehicle acceptance
failures remain open. This is not a completed scheduling or physics system.

## TU1.5 connected interrupt lowering and loader-phase evidence

The [interrupt-lowering component](PHYSICS_TU15_INTERRUPT_LOWERING_20261012.md)
adds two complete original modified-Complex4627 bodies and67 sites. The actual
timer call now dispatches through both bodies in the public startup composition.
Original IRQ masks are authenticated and read-only. Context, clock, scheduler,
dynamic interrupt dispatch and PIC ownership remain explicit; missing callees
do not receive successful returns. Retained CLI/PUSHFD/POPFD preserve guest
flags without affecting host interrupts.

The independent unchanged original corpus has55 cases,5,298 instructions,
2,021 ordered stores and24 PIC outputs. Nine cases execute original queue/timer
constructors and actual149/150/lowering through their returns, using declared
procedure context/clock inputs. Coverage is44/67 new sites, with23 sites after
missing scheduler/dispatch owners unverified. This is not live timer ownership.
The public composition also preserves all48 fresh startup prefixes and their
actual missing context boundary. Original generation and fixtures repeat
byte-identically.

Two isolated owned boots stop at PE entry8005504C before its first instruction,
with an execution breakpoint and no guest memory writes. Identical loader
metadata, INIT bytes and named initial interrupt/clock records are retained.
Loader zeros do not establish the subsequently initialized title-thread
context; kernel initialization and live clock production remain required.

Ten selected checks pass fresh Release and ASAN/UBSAN/leak builds using the
pinned prior source foundation. Both new production targets build from the
actual root CMake project and both checks pass. An initial configure failure
while tag files were absent remains historical evidence; those files later
appeared in the shared checkout. Production artifacts are retained immutably.
Root and frozen validators reproduce the131-file source/artifact seal at
`analysis/multiplayer/sandbox-source/tu15-interrupt-startup-20261012/validation.json`,
SHA-256 `5d3f4dabb425bc6d73034189df76211b6c1ec99f59887cd198f4589ed3d0f4a3`.

Fresh native context/clock construction, timer expiry/DPC servicing, process
heap/global installation, source world/body/contact/controller creation and
`Simulation::step` integration remain open. Visible multiplayer lift and
vehicle acceptance remains red. The complete physics goal is unfinished.
