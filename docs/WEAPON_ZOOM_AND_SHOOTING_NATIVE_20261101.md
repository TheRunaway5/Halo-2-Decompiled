# Weapon zoom audio and native shooting effects

The requested completion condition is every authored weapon zoom sound and
shooting effect matching TU1.5, with native kernels. This checkpoint does not
certify that complete condition.

## Native effect frame and pool traversal

TU1.5 `179DF0`, `16EA60` and `177830` now execute through native kernels in
their real frame, light, AI and combat callers. The frame owner preserves full
salted identities and the tail transfer to the particle updater. The updater
preserves its saved pool pointer, low-byte return test and lower-call FP/MMX
state. The integer-only slot scan leaves the caller's FP/MMX file in place.
All bridges publish partial state on faults. Their common generator hook and
exact owned regions match the root checkout; unrelated generated-file changes
remain outside this frozen cohort.

Physical unchanged-code references found three real stack-fault discrepancies:
the preceding compiled frame, slot scan and particle updater committed ESP
before a faulting PUSH store. Each native owner now retains the original ESP
until that store succeeds. The initial test-only missing STOP-page mapping was
corrected separately; it is not counted as a production defect. The original
red runs and each successive reached failure remain in the checkpoint reports.

`frame-original-reference-v2.json` records 327 physical i386 cases and 165,198
instruction steps. These execute actual original lower calls, with no supplied
callee results: 304 returns, 20 memory faults and three explicitly bounded
prefixes. The 150 standalone slot-scan rows also pass through light, AI and
combat. `particle-frame-original-reference-v1.json` adds 118 cases and 74,784
physical instructions: 103 returns, 13 memory faults and two bounded prefixes.
Its 136 lower-call results are explicitly declared ABI/bank-write controls;
they qualify the updater owner, not those lower callees or completed output.
Tests compare instruction/store order, mapped memory, live and dead stack,
defined flags, GP, FP, XMM and MMX state, including fault prefixes.

`effect-frame-module-validation.json` passes all seven new routes under Clang,
GCC and ASAN/UBSAN. It recompiles the native kernel, actual specialized frame
caller and probe; other linked libraries retain their qualified Release build.
`effect-frame-prior-components-v1/report.json` passes all 30 preceding component
CTests under all three modes with the current kernel and 93 actual source
inputs. The combined component scope is now 35 native entries and 5,831 original
cases, with the declared controls and bounded prefixes above. The separate
5,917-case semantic timeline reference is not added to that physical count.

`effect-frame-controls-validation.json` rejects eight private defects covering
early ESP commitment, tail-as-call, discarded salt, negative scan starts,
incorrect bit lanes, whole-EAX tests, global pool reload and lost FP state.
Restored baselines pass and original inputs remain unchanged. Two earlier
driver reports preserve incorrect expectations about which mismatch appeared
first; both mutants were already rejected. No production change was needed
for those receipt corrections.

`effect-frame-runtime-qualification-v1` passes all 161 selected CTests on the
unchanged 12,867-file v7 source cohort, with unchanged prepared-MCC inputs.
The current zoom check covers 15 weapon worlds, 34 authored requests and 34
PCM submissions; the four UDP recipient views deliver 88 PCM submissions over
68 explicit zoom levels and 60 melee resets. Authored NONE/NONE zoom references
remain silent. Joined firing with actual downstream packet loss delivers
43 rounds/43 primary PCM submissions and 153 particle draws for Human;
Elite delivers 42/42 and 133 draws. Both use the two actual weapon owners.
These are bounded native runtime checks, not independent waveform/framebuffer
or all-lifecycle acceptance.

`native-effect-frame-validation.json` pins the source, references, compiler
checks, controls and runtime results. The two new required original expected
inputs are `tests/fixtures/weapon_effect_frame_tu15_original.json.gz`
(648,740 bytes) and `tests/fixtures/weapon_particle_frame_tu15_original.json.gz`
(255,300 bytes). They total 904,040 bytes and are retained as reference inputs.
The retention receipt verifies removal of completed scratch owners and helper
sources; the supervised source/build remain temporary for this active goal.
Native current-admission client effect residency, remaining reached lower
owners, complete authored roles/lifecycles, independent original output
comparison and launcher-selected runtime delivery remain unfinished.

## Shared world/first-person catch-up

The genuine shared-RNG failure in `native-shared-timeline-diagnosis.json` is now
covered by the registered `native_weapon_shared_timeline_tu15` regression. Whole
world catch-up followed by whole FP catch-up changed exact particle GPU inputs
for two or more actual native SMG requests, despite equal cue counts, particle
counts and final RNG words. The earlier v3/v4 diagnostic reports remain as red
reference evidence.

`catch_up_effect_presentation` now finishes each older tick in world/audio/FP/audio
order. The existing current-tick world, FP animation audio and FP phases remain
with the runtime caller. `stage_pose` copies actual received marker/view operands
before catch-up without allocating emissions, consuming RNG or advancing clocks.
Attachments are still created in their existing current-tick phase. The helper
refuses mismatched completed owner clocks before advancing either one. It derives
its cursor from the real owners, so it adds no separate checkpoint clock.

The accepted-client path queues the genuine source requests before catch-up and
defers current FP-animation audio until the older ticks finish. It runs the
ordinary completed-packet FP sampler, which remains read-only; no missing input
command, actor admission, gameplay update or native source service is supplied.
Passive counters record returned older ticks, receipts and actual queued-request
dequeues. They never participate in admission or publication.

`shared-timeline-delivery-v6` passes the nine-request regression with both recipients
borrowing the same actual captured native RNG pair and real manual SDL audio.
Individual/coalesced delivery matches exact FP particle attributes/constants,
owner/definition, residency, sound dispatch tick/order and both RNG words. Both
submit 18 authored sound requests and nine primary native PCM packets. The minimal
one- and two-request cases also match. Fourteen genuine native projectile contacts
are retained in that run; their two authored effect definitions have zero sound
parts, and both recipients emit zero contact sound requests. Earlier v5 incorrectly
required positive contact audio and is retained as a failed test assumption.
Authenticated fixed marker poses qualify chronology, not live placement or Xbox
waveform/framebuffer output. The audio provider's secondary seed is declared.

`shared-timeline-module-validation.json` passes Clang, GCC and ASAN/UBSAN for the
changed Muzzle, World and catch-up modules plus the genuine-producer regression.
The exact preceding Muzzle source is reconstructed and hash-checked against its
frozen parent; 24 individual frames per mode match current cue rows, RNG, residency
and particle GPU bytes. Other linked libraries retain their qualified Release
build, so this is not full-project sanitizer acceptance.

`shared-timeline-controls-validation.json` rejects all five private defects:
grouped consumers, reversed consumers, late FP audio, omitted pose staging and
omitted mismatched-clock refusal. The restored baseline passes, and all original
source, library and test-object inputs remain byte-identical. The two standalone
compiler/control owners have been removed.

The dedicated joined firing tests briefly drop genuine downstream UDP datagrams
through a test relay after observing actual ammunition consumption. Upstream
controls, native authority, input leases, admissions and existing deadlines remain
ordinary. A pause alone in v5 preserved all accepted snapshots and proved no
firing catch-up; v6/v7 relay startup failures are also preserved. The relay now
accepts the runtime's discovery-to-client endpoint change and reserves a bounded
8 MiB receive queue for genuine native snapshot fragments. These are test-fixture
changes. No production authentication or physics admission was relaxed.

`shared-timeline-buffer-v8` passes Human and Elite. Human records 199 dropped host
datagrams, 85 returned older ticks, seven gap receipts and one dequeued source
request, with 50 consumed rounds/50 primary PCM submissions and 153 particle
draws. Elite records 213 dropped datagrams, 72 older ticks, four gap receipts and
two dequeued source requests, with 42 rounds/42 PCM and 146 draws. Both use the two
actual weapon owners. These are bounded localhost native delivery checks.

`shared-timeline-runtime-qualification-v1` passes all 154 selected CTests on
one unchanged 12,860-file v8 source cohort. Prepared-MCC registry/map/shared
inputs remain unchanged, as do all 92 qualified native component inputs. The
separate 5,917-case semantic clock reference is included in those 154 tests.
The final registration refresh limits the Linux relay tests to Linux, retaining
identical Linux test commands/properties and selected executable bytes; it adds
only that CMake guard and this documentation. The full concurrent root checkout
and launcher-selected runtime are outside this source cohort's acceptance.

`shared-timeline-scheduling-retention.json` records cleanup of the three
completed compiler/control/audit owners and fifteen retired helper sources.
Only source/build remain inside the supervised active owner for the same
unfinished goal; that receipt records the measured temporary size. No new
rebuildable binary or capture is retained permanently.

Native current-admission client effect residency, complete authored roles and
lifecycles, independent original output comparison and launcher-selected runtime
delivery remain open. The complete user goal remains active.

`native-effect-frame-original-audit-v1.json` is the preceding static audit of
179DF0. The later physical execution and native-owner qualification are recorded
above; the static audit alone remains insufficient proof.

## World recipient request chronology

The remote/world recipient had a separate coalescing defect. The same actual
native SMG journal produced nine primary cues individually and one when received
together. Two genuine requests reproduce the loss; a single request passes.
Private instrumentation confirms that refreshes replace an unscheduled start
before it reaches its parts. The red reports are
`native-world-coalesced-journal-v2.json` through `v4.json` and the registered
`world-journal-red-v1` CTest.

`NativeWorldEffects::request_weapon` now retains requests until their source
tick. Receipt does not allocate or consume RNG. At that tick the existing native
event/particle/sound kernels execute the creation or retained-instance refresh.
The queue owns the actual completed location copies. New births retain receipt
order alongside contact births; scheduled effects retain stable traversal order.
Checkpoint/fork state includes the queue and ordering, and object removal cancels
its pending requests. No sound or admission is synthesized from ammunition.

`world-journal-regression-v5` passes nine actual requests, genuine native generic
projectile contacts, exact cue owner/order/gain/tick, both RNG streams and particle
GPU input bytes. It checks queued and active checkpoint replay, independent forks
and removal. An earlier v4 sample contains 16 contacts and 14 active particle
submissions. Each sample retains its actual counts.

`world-journal-controls-validation-v2.json` rejects four private defects: early
application, lost checkpoint requests, missing source locations and omitted
object cancellation. Restoring the code reproduces the library, object and test
executable byte for byte. The first control-driver version expected a later error
label for the lost queue; the actual test already rejected it at its active/pending
state precondition. Its original failed driver report remains preserved.

`world-journal-module-validation.json` compares the preceding and fixed individual
recipients against the same genuine producer inputs and RNG owners within each
run. All 24 frames match exact cue rows, RNG, residency and particle input bytes
under Clang, GCC and ASAN/UBSAN. The latter instruments the changed module and
probe; other linked libraries retain their qualified Release build. A separate
birth-order reversal fails the particle-byte comparison. The standalone owner is
removed. These checks preserve compared recipient behavior; they do not establish
Xbox output parity or complete native client effect-pool ownership.

The expanded world-journal-runtime-qualification-v1 passes all 150 selected
CTests on the unchanged 12,857-file v5 cohort, including prepared-MCC resource
admission. The registry, map/shared inputs and all 92 qualified native component
inputs remain unchanged. native-world-journal-validation.json pins the complete
scoped evidence and actual joined firing results. No launcher runtime has been
promoted. Current-admission native client effect ownership, shared cross-recipient
RNG/timing, remaining authored lifecycles and independent output comparison remain
open; the full user goal is active.

The final run contains nine native weapon requests, 18 generic projectile contact
events and 13 active particle submissions in the world comparison. Joined Human
delivers 49 rounds/49 native PCM submissions with 130 particle draws; Elite delivers
37/37 with 160 draws. Both use the two actual weapon owners.

The separate `world-journal-typed-clock-v1` CTest also passes all 5,917 existing
TU1.5 semantic clock/RNG/service cases on the same source cohort. Its unchanged
test enforces that exact case count. The reference uses Unicorn/default 037F with
controlled downstream services, so it is separate from the physical machine-ABI
case count. `world-journal-typed-clock-validation-v2.json` corrects a receipt that
expected successful program stdout in CTest's abbreviated log; the actual test
already passed and was not rerun or reclassified.

The world-journal-retention.json receipt verifies removal of all five completed
probe/compiler owners and twelve retired helper sources. No new rebuildable binary
or capture is retained permanently. Only source/build remain in the active
supervised owner for this same unfinished goal; the receipt records their paths
and measured size.

## Native random-range qualification

TU1.5 `259B0` now runs in the shared native weapon kernel and its actual
physics-support and AI callers. The owner preserves the source RNG store and
subsequent aliased operand reads, overwritten scratch/returned ECX, exact
instruction/store order and the live masked x87 file. The preceding compiled
callers lose C1 on original reference case 14; the native owner matches it.

`random-range-original-reference-v1.json` records 300 unchanged physical i386
cases, 5,046 instruction steps, 17 executed instruction sites, eight aliases,
two read faults and two write faults. Live depth is measured from the physical
tag file at every step, including six input and seven output live lanes. No
callee, game-service result or instruction patch is supplied. The test-only
read-only ELF backing was corrected after an initial zero-file-size segment
proved writable; its default ELF remains byte-identical. This backing issue
was separate from the real compiled C1 discrepancy.

`random-range-components-v1/report.json` passes all 30 component CTests under
GCC, Clang and ASAN/UBSAN, covering 32 native owners and 5,386 original cases.
All 92 actual component inputs match the full cohort and root checkout. Eight
new wrong-code controls fail as required. An independent private C1 mutation
fails case 14 through direct dispatch and both compiled callers; restoration
reproduces all four comparison outputs byte for byte.

`random-range-runtime-qualification-v1` passes all 148 selected CTests on the
unchanged 12,856-file v3 cohort. The registry and prepared map/shared inputs
remain unchanged. Joined firing delivers 51 rounds/51 native PCM submissions
with 163 particle draws for Human, and 39/39 with 149 draws for Elite, using
both actual weapon owners. `native-random-range-validation.json` pins the
scoped evidence. These results do not establish waveform/framebuffer parity,
all recipient roles/lifecycles or launcher delivery. The full goal remains active.

The required physical expected-data fixture is
`tests/fixtures/weapon_effect_random_range_tu15_original.json.gz` (142,331 bytes).
The completed standalone component owner is removed; the supervised incremental
source/build remains active for the same unfinished goal.

## Native next-event selection qualification

TU1.5 `17C020` now has a shared native owner for loop selection, full-salt
metadata reads, actual primary/auxiliary RNG updates, ordered stores and the
live masked x87 file. The real compiled death-cycle copy selects this owner;
other source images retain their original route. Generator installation is
idempotent, and unrelated root generated-source changes are preserved.

`next-event-original-reference-v3.json` records 185 unchanged physical i386
cases, 11,475 instruction steps, 61 real RNG updates, seven read-fault prefixes
and five aliases. No callee, game-service result or instruction patch is supplied.
Every final declared live x87 depth is checked against the physical tag file.
`next-event-original-site-audit-v1.json` finds 89 static instruction sites and
88 executed sites. `SHR ECX,16` followed by `TEST ECX,ECX` makes `JGE` always skip
the unsigned correction at `17C10B`; the native owner retains that branch/site.

The initial v1/v2 expected files miscounted Capstone's `fcompi` spelling as a
non-popping instruction. Their depth mismatch is an oracle bookkeeping error,
not evidence of a production FP defect. Both superseded encodings remain
preserved with hashes and this limitation in `next-event-reference-history.json`.
With the corrected reference, both the new kernel and the preceding generated
caller match all 185 cases. Migration therefore preserves this compared behavior.

`next-event-components-v1/report.json` passes all 29 component CTests under GCC,
Clang and ASAN/UBSAN. The shared cohort contains 31 native owners and 5,086
original cases with 88 actual component inputs. Eight new wrong-code controls
are rejected, including RNG stream/multiplier, default loop, MOVSS upper lanes,
x87 precision, FCOMIP ordering, store PC and signed current-event selection.
The standalone component owner has been removed.

`next-event-native-callers-v4` passes the direct and actual compiled-caller
checks. A separate private RNG multiplier mutation fails both routes at case 0;
restoration reproduces all four kernel/comparison outputs byte for byte.
`next-event-compiled-control-validation.json` records that activation evidence.
The 145-test current-cache qualification passes on the coherent 12,851-file v4
source cohort. The asset registry, prepared map/shared inputs and all 88 actual
component inputs remain unchanged; those component inputs also match the root
checkout. `native-next-event-validation.json` records the complete scoped proof
and joined firing results. This selection work does not establish complete
recipient native particle ownership, authored lifecycle coverage or
output/launcher parity.

`next-event-retention.json` verifies removal of all completed standalone
component/oracle storage and retired helper sources. The new required physical
expected-data fixture is
`tests/fixtures/weapon_effect_next_event_tu15_original.json.gz` (96,510 bytes).
The two superseded small encodings remain as explicitly invalid-depth reference
history under the report root (84,003 and 96,555 bytes). No new rebuildable binary
or capture is retained permanently. Only source/build remain in the supervised
active owner for this unfinished goal, with their measured size in that receipt.

## Native event advance and coalesced recipient requests

Complete TU1.5 `17C140` now runs in the shared native weapon kernel, including
its two compiled caller copies. `advance-original-reference-v1.json` records
142 cases, 19,836 unchanged physical i386 instruction steps and all 150 original
instruction sites. The reference retains exact GP/FP state, memory/store order,
read-fault prefixes and aliases. Nine lower-owner ABI controls are declared;
those results do not certify the controlled lower bodies.

`advance-components-v1/report.json` passes all 28 component CTests under GCC,
Clang and ASAN/UBSAN. This cohort contains 30 native owners and 4,901 original
cases. Eight new wrong-code controls fail as required. An independent private
iteration-cap mutation also fails the direct, physics-support and death-cycle
routes on the same reference case. Restoring the source restores all four
comparison outputs byte for byte; `advance-compiled-control-validation.json`
records the compiler outputs and generator/caller correspondence.

The first expanded current-cache run, `advance-runtime-qualification-v1`,
passes 140/142 selected CTests. Its two joined firing failures remain evidence:
several real native requests received together reset the same local effect
before its pending start parts executed. The minimized actual-native journal
in `native-client-coalesced-muzzle-v2.json` contains nine consumed SMG rounds,
nine genuine requests and a valid reused producer instance. Individual receipt
produces nine primary firing cues; one coalesced receipt produces two.

The native C++ recipient interface now queues genuine requests by source tick
and order, retaining the producer identity separately from local allocation.
The existing event, particle and sound kernels execute each request when that
timeline reaches it. Requests do not allocate or consume RNG on receipt.
Queued requests, bindings and retained marker poses are presentation checkpoint
state. No sound is synthesized from ammunition, no admission result is supplied,
and deadlines and input leases are unchanged.

`native-client-coalesced-muzzle-v3.json` reproduces all nine primary cues and
both unchanged RNG streams for the same requests. The actual joined SDL tests
in `coalesced-joined-firing-v1` also pass: Human delivers 53 rounds/53 native PCM
submissions with 160 particle draws; Elite delivers 37/37 with 147 draws. Both
use the two actual weapon owners. These results certify those runs, not exact
waveforms or Xbox framebuffers. The first-class queued-request/checkpoint
regression now also compares particle/location retention and exact submitted
attribute/constant bytes at an authenticated fixed model pose. Its active sample
has four particle draws. The three private wrong-code controls (ignore request
timing, lose queued requests, lose active bindings) all fail; restored library,
object and test executable hashes match the baseline exactly.

`coalesced-runtime-qualification-v2` passes all 143 selected CTests on one
12,847-file cohort, with the registry, prepared map/shared bytes and all 85
qualified component inputs unchanged. Those component inputs also match the
root checkout. `native-advance-queue-validation.json` pins that source cohort,
owned root changes, compiled caller correspondence and both joined firing
reports. Whole generated/runtime files may differ because concurrent unrelated
work is preserved. No launcher runtime has been promoted.

`advance-queue-retention.json` verifies removal of superseded probes, binaries,
objects, savegames and helper sources. The completed standalone component owner
is removed. Only source/build remain in the active supervised owner for this
same unfinished goal; the receipt records their current size. The new required
original reference input is
`tests/fixtures/weapon_effect_advance_tu15_original.json.gz` (291,699 bytes).
No new rebuildable binary or capture is retained permanently.

This chronology fix still uses the recipient's existing C++ effect residency.
The current-admission native-world client particle boundary described below,
remaining reached owners, complete authored lifecycles and independent output
comparisons remain open. The full user goal is active.

## Native particle import boundary and continuation

`native-fx-import-boundary-v1-receipt.json` locates the previously observed
allocation loss. Both watched full-salt native effects exist before preparation
and are already absent from the incoming prepared bank. Render-history staging
and local-view rebinding do not cause their retirement. No particle frame runs
between those observations.

The explicit `Prepared::retain_native_particle_history_from` interface now
retains genuine executed bytes in an unpublished replacement at the same
gameplay tick: six native effect/particle pools, their allocator state,
occupancy and uncleared records, and the native RNG pair. Executor barriers,
resource/BSP identity, matching native layouts and complete mutable extents are
checked before publication. It creates no effect, supplies no game-service
result and advances no replacement kernel. Other effect subowners are outside
this interface.

`native-particle-history-tests-v2` passes both registered CTests against the
current prepared-MCC cache. One actual native SMG shot consumes 60 to 59 rounds;
24 subsequent native frames match uninterrupted execution across all retained
bytes and eight submitted FP particle attribute/constant banks. The control
without retention detects stale particle records. A staging source, a different
gameplay tick and an invalid final-pool allocator count are rejected; the latter
two controls also verify that rejection leaves staged particle bytes intact.
The diagnostic overlays were removed before this interface was compiled.

`native-particle-surrounding-tests-v1` also passes all nine selected zoom,
native transport, joined firing and dual reload checks for Human and Elite on
that same 12,841-file cohort. The registry and prepared map/shared hashes remain
unchanged, as do all 64 qualified native kernel inputs. These nine names repeat
existing checks; the two particle-continuation tests are new. Superseded probe
sources remain in `native-fx-import-retired-helpers.json`; their generated
binaries, objects and savegame directories were removed (611,380,843 bytes).
`native-fx-import-retention.json` records cleanup and the corrected meaning of
the write census's adapter cursor field.

`native-particle-history-tests-v3` passes three registered CTests after a
test-only refresh: the two continuation cases and the new native Plasma Pistol
delivery regression. Production runtime inputs are unchanged from v2. A real
tap produces one authored `fire_bolt` event, native FP particles with the full
weapon owner, and both authored sound requests exactly once. A full held charge
and release produces one `charged_bolt_fire` event and its authored sound
request exactly once. That effect's actual native definition has no particle
systems, and the test requires zero FP particle draws for it. The charging and
overheat particles have distinct effect identities. The earlier hold-fire
matrix's zero does not establish a missing charged-shot flash; this regression
does not invent one or claim PCM/framebuffer parity.

`native-particle-validation.json` pins the final 12,842-file cohort and records
the test-only lineage from the nine surrounding checks to the three final
tests. Owned changes match the root checkout; unrelated differences in the
physics source/header and CMake remain explicit. No launcher runtime was
promoted. `native-particle-retention.json` verifies removal of the later probe
binaries, objects, savegames and 15 retired helpers (203,443,926 bytes). Only
`source` and `build` remain in the supervised active owner, measuring
7,223,823,293 apparent bytes for this same unfinished goal. No new large output
is retained permanently.

This interface is not selected by ordinary client rendering. Ordinary accepted
weapon snapshots advance separately from the sparse native shadow cuts, while
the recipient FP sampler owns a separate native context. Turning on particles
in that sparse shadow world would use stale object/view inputs. A complete
recipient particle owner still needs current admitted native object, pose,
view, RNG and lifecycle inputs. The same-tick SMG continuation test supplies no
remote-role, all-weapon, framebuffer or PCM parity proof. The full user goal
remains active.

## Current native interaction and delivery qualification

`native-hold-timeout-validation.json` records 136/136 selected CTests on one
12,838-file frozen source cohort. Six additional checks repeat relevant names
against the prepared cache selected by the current asset registry. All 64
qualified native-kernel inputs and seven owned root files match this cohort.
The concurrent checkout as a whole and the launcher-selected runtime remain
unqualified; this goal has promoted no runtime.

`native-hold-timeout-green-v1` supersedes the offhand-loss finding below.
The minimized actual-MCC UDP reproduction distinguishes continuous input,
fresh input during a 660 ms delay, and an expired input lease. Only expiry
cleared the native consumed-interaction latch before the real completed hold
arrived. Original TU1.5 `EAA41` then cleared the secondary inventory selector.
The published player flags match the actual full-salt native record throughout.

The host now retains only interaction levels already masked by original
`1521B0` until physical release. Expired movement/fire/reload remain neutral,
elapsed interaction durations remain zero, and neither the 500 ms input lease
nor connection liveness is extended. Original action and inventory kernels are
unchanged. The new six-case native UDP regression fails two cases before the
fix and passes all six afterward, together with 4,096 original player-action
buffer cases. Authentication uses TU1.5 `1521B0`; `150DD0` is the corresponding
MapPack entry. The corrected original-site report is
`native-hold-drop-original-sites-v2.json`; v1 is retained as superseded failure.

Both legacy joined dual firing/reload cases pass after this fix. The firing-only
test also exposed a shutdown race: transient PCM/ammo equality could queue
`SDL_QUIT` while native unit trigger bit 0 remained held, then count another
round before polling quit. `native-joined-firing-exit-red-v1` locks down that
failure. The test now waits for actual local/native trigger release and ten
completed source ticks after the final ammo change, retaining the original
deadline and exact PCM equality requirement. Production audio is unchanged.
`native-joined-firing-exit-green-v1` passes both representations. The final
136-test run delivers 52 rounds/52 PCM submissions for Human and 49/49 for Elite,
with 157/137 particle draws respectively, both actual weapon owners and no ammo
or PCM events after quit. These are bounded native delivery checks, not
waveform, framebuffer or full-weapon parity proof.

The broader `native-hold-timeout-qualification-v2` run passed 134/136 cases.
Both legacy reload cases delivered their one R press on primary-trigger release,
refilled only the already idle offhand, and reached their existing deadline.
The test now waits for ten actual released native ticks before pressing R once.
`native-joined-reload-settled-v1` and the final 136-test run pass all four joined
cases, including both hands reloading for Human and Elite. Both failed broad-run
reports remain preserved. Original reload kernels and the both-full requirement
are unchanged.

`native-current-cache-zoom-v1` passes all six checks against the current registry's
v9 prepared cache, with unchanged input hashes: 15 weapon worlds, 34 source zoom
requests/34 actual PCM submissions, and four native UDP recipient views with
68 explicit levels, 88 PCM submissions and 60 melee resets. Authored binocular
NONE/NONE references remain silent.

`native-weapon-draw-matrix-v2.json` records 4,293 actual native FP particle
submissions across 15 starting weapons using that current cache, with 60 warmup,
72 held-fire and 60 release ticks per world. All 2,880 repeated FP views preserve
particle bytes/RNG and exact submitted attribute/constant bits. No resource
quarantine occurs. Plasma Pistol and Sword produce no matched FP draw in this
workload; those zeros do not establish complete lifecycle coverage.

`native-recipient-births-v3.json` observes 73 actual native admissions from 74
requests in authenticated, rebound recipient worlds. Two subsequent imports
lose the previously occupied full effect identity without a particle advance
between creation and import. This is a concrete remaining continuity boundary,
not a qualified client particle timeline. The earlier v2 diagnostic's field
named `producer_view` reads the imported recipient bank; its interpretation
receipt explicitly rejects using it as producer-view evidence. No client native
particle route is enabled by these probes. Storage and temporary retention are
recorded in `native-hold-timeout-retention.json`.

## Native LAN presentation and reached firing limits

`lan-binding-validation.json` records a bounded source fix and 130 passing
selected CTests: 128 native/zoom/local-render regressions and two new prepared-MCC
native transport checks. Private LAN presentation worlds now load the complete
authored replicated first-person definition bank used by their producer. The
strict configuration digest is preserved. Before this change, all four reached
joined cases rejected `configuration_binding`; actual imports now complete.

The transport checks validate two producer views rebound to one recipient view,
three actual native timeline frames per route, real host acknowledgements,
expired-cut recovery and rejection of late old readiness. They consume prepared
MCC maps and their shared directory without an original executable runtime input.
Their native graphics-policy checks are separate from GPU output proof.

Both local representations pass actual firing PCM/particle delivery and the
existing GPU hand-deformation checks. The 29 native kernels retain all 64
byte-identical qualified component inputs, 4,759 original cases and 67 rejected
wrong-code controls from the preceding component qualification. The new source
manifest pins 12,837 files. The four changed root inputs match this private
cohort; the entire concurrent checkout and launcher-selected runtime remain
unqualified by this work. No runtime was promoted.

The earlier joined firing failure is retained in `joined-firing-frontier-v1.json`:
after a neutral gap, a completed dual-wield interaction is delivered again and
the offhand is lost before firing. That human case records zero fired rounds,
zero PCM voices and zero particle submissions. The timeout causality and fix
above supersede this failure; the failed reports remain preserved. No source
return, input-liveness or validation bypass was added.

The custom rendered test host now publishes the actual native shadow lane.
Trigger stages wait for observed ammunition consumption. The new firing-only
mode ends after both triggers; the legacy mode retains its reload assertions.
A separate native UDP diagnostic keeps both weapons and fires both hands; its
bounded reload wait is not accepted as complete reload qualification.

`native-particle-role-probe-v2.json` records 196 original effect births in 15
local/remote weapon worlds and 1,587 system observations. It observes no particle
locations without a render owner and explicitly claims no emission/render parity.
Producer-local view 0 and nonlocal NONE births alone cannot authenticate recipient
particle simulation. Client native particle/RNG ownership, all authored
lifecycles/roles, independent rendered/PCM comparison and current-runtime delivery
remain open. The complete requested 1-to-1 condition is unfinished; the goal stays
active. Storage and explicit temporary retention are recorded in
`lan-binding-retention.json`.

## Current native emission/math checkpoint

`marker-emission-validation.json` is the preceding native-kernel checkpoint:
29 complete native owners, 4,759 detailed cases, 196,608 exhaustive iterator
admissions, GCC/Clang/ASAN/UBSAN, 67 rejected wrong-code controls and 124 integrated
CTests. All 64 used inputs match the component freeze, private runtime and
current owned root inputs byte-for-byte. Actual dependencies include 24 driver
objects and their one shared native kernel object. The source manifest pins
12,837 files; this does not qualify the entire concurrent checkout.

Complete 1E1200 emission, 144910 quaternion conversion and 145560 transform
composition now have explicit shared native kernels. The physical reference
executes all three together: 181 inputs, all 252 original instruction sites and
26,318 unchanged i386 steps, with no instruction patch or supplied game-service
return. It contains 199 real callee entries, 197 returns and two callee read
faults. All 392 eight-byte stores are recorded, including 386 that change bytes.
Signed bone filtering, signed capacities/node remaps, an aliased group count,
COMISS exception behavior, packed SSE lanes and half-vector loads/stores have
specific wrong-code controls. The first 172 inputs remain byte-identical to the
preserved preceding reference; nine inputs add signed-remap/count-alias coverage.

Five emission caller copies use the new native owner: support, AI, scene-device,
sandbox and the authenticated vehicle 1DC940 copy. Their literal bodies remain
exact after removing only the new prefixes. All five callers pass 905 complete
physical-input replays. Four APIs expose their real required math boundaries;
each checks 199 entry/unavailable frames and 197 native returns. The vehicle API
already owns both native math callees and reports their canonical PCs. Its
existing ownership is preserved, with 146 independently authenticated paired
instruction/operand maps, 199 entry-stack checks, 197 return-stack checks and the
complete trace, reads, ordered stores, memory and final GP/FP comparison. Bound
owners are not treated as unavailable caller-supplied services. No gameplay
change was made to accommodate the vehicle observation correction.

All 22 loop inputs retain complete physical traces in 44 bound/unbound
comparisons. The 15-weapon/70-phase workload retains every non-registry output
and its full 11,040-site instruction histogram. Registration metadata marks
1E1200 native and adds the two native math targets to enumeration. Their prior
false ownership is proven by the exact preceding qualified predicate; the old
runtime did not enumerate those two queries. The diagnostic overlay is removed,
its target rebuilt and all qualified source hashes restored. Complete support
regeneration retains all five marker-family routes. The fixed MapPack startup
path and previous vehicle routes remain preserved.

`weapon-effects-completion-audit-v1.json` checks all three user requirements.
Authored contracts cover 1,110 rows, 63 weapon paths and 38 maps. The native zoom
test consumes 34 source requests as 34 PCM submissions across 15 weapon worlds;
native UDP-loopback coverage includes four views. These results do not prove
independent Xbox PCM/GPU equality or complete joined-client presentation.
The selected runtime's real binary digest is verified, but this goal has not
promoted or qualified that selected runtime against this checkpoint.

The next work is complete authored lifecycle/role and native rendered/audio
delivery coverage, using actual original/native ownership. Other compiled
native owners must not be classified as missing merely because they are outside
the weapon/sound registries. Independent rendered/source-output comparison,
generator closure and selected current-runtime delivery remain open. Full
requested 1-to-1 parity is unfinished and the goal remains active.

`marker-emission-retention.json` records completed standalone scratch cleanup,
compressed logs and exact retired helper sources. Required references are the
506,940-byte current graph fixture and its preserved 479,486-byte predecessor.
No large runtime binaries, banks or captures are retained permanently. The
supervised temporary build remains for this same unfinished goal; its path and
measured size are in the receipt.

## Preceding reference-corrected native checkpoint

`wide-store-validation.json` records the preceding qualified checkpoint: 26 complete
native owners, 4,578 detailed cases, 196,608 exhaustive iterator admissions,
GCC/Clang/ASAN/UBSAN, 58 rejected wrong-code controls and 118 integrated CTests.
All qualified native C++ sources and headers are identical to the preceding lookup
checkpoint. All 60 used inputs match the new component freeze and private
runtime, with 24 actual dependency objects and a 12,833-file source manifest.

The correction is in the original-reference recorder. Capstone labels the
authenticated `0F17` MOVHPS memory destination as a read; the recorder now uses
the actual eight-byte store form. The location-copy reference gains 36 real
stores across nine lower calls. All other initial inputs, parent traces,
GP/FP files, faults and lower returns in its 138 cases are byte-identical.
The older fixture and exact generator are preserved as superseded evidence.
The complete native suite passes with the corrected memory events.

A fresh physical replay confirms all 20 MOVHPS stores in the separate 22-case
loop reference target stack memory. Its nonstack-write comparison and entire
111,601-byte physical fixture are byte-identical, and its 44 bound/unbound
native comparisons pass again. The preceding 15-weapon/70-phase instruction
census remains separate evidence over identical native C++ sources; this
reference refresh does not claim a new census or launcher delivery.

The next emission reference executes 1E1200, 144910 and 145560 together on
unchanged physical i386: 172 inputs, all 252 sites, 24,205 steps, 181 real callee
entries, 179 returns and two callee read faults. Its 356 eight-byte stores are
recorded explicitly, including 351 that change destination bytes. It has no
instruction patch or supplied game-service return. Its first recorder version
is preserved and explicitly marked invalid for complete operand events.

Two native math bodies are staged with all 146 original sites accounted for.
GCC/Clang compile-only checks pass; they are unbound and have not passed
behavioral validation. Complete native
emission/math integration, remaining reached marker/matrix owners, all authored
lifecycle and remote roles, independent rendered/Xbox waveform parity,
generator closure and selected-runtime delivery remain open. The full requested
1-to-1 condition is unfinished and the goal remains active.

`wide-store-retention.json` records compressed logs, retired exact helper
sources, required reference paths/sizes and completed standalone scratch
cleanup. No large runtime binaries, banks or captures are retained permanently.
The supervised temporary build remains for this same unfinished goal; its path
and measured size are in the receipt.

## Preceding native marker-lookup checkpoint

`marker-lookup-validation.json` records the preceding qualified source checkpoint.
Complete TU1.5 1E11B0 now uses one shared native lookup in physics support, AI,
scene-device construction, sandbox actions and the operand-authenticated vehicle
1DC8F0 copy. It preserves all 28 original sites, early exits, signed counts,
first-match order, 12-byte record stride, stack aliases, registers and flags.
All five previous callers failed at the original faulting PUSH: they committed
ESP before its failed store. The native helper commits ESP after a successful
store, and all five callers now pass 480 full physical-input replays.

The unchanged-original lookup reference covers 96 inputs and 2,179 physical
Linux i386 steps, including ten read faults, one write fault, five aliases and
four active MMX files. It has no instruction patch or supplied game-service
return. All 28 paired original instruction/operand correspondences are checked.
The fixed MapPack startup copy and an unrelated animation routine at the same
numeric address are preserved. Both source cohorts retain the exact older
literal bodies after removing only the five new prefixes. Vehicle parent and
matrix native routes remain present, and complete support regeneration retains
all four marker-family routes.

The component suite passes 4,578 cases across 26 complete native owners,
196,608 exhaustive iterator admissions, GCC/Clang/ASAN/UBSAN and 58 wrong-code
controls. All 118 integrated CTests pass. All 60 used inputs match the component
freeze and private runtime byte-for-byte; actual dependencies include 23 driver
objects and their shared production kernel object. One aggregate test target
traverses the common build dependencies in a single Make invocation. The frozen
source manifest pins 12,829 files. Current root drift and the entire runtime
dependency graph remain separate qualification limits in the receipt.

The 15-weapon/70-phase workload retains every non-registry output and the full
11,040-site instruction histogram. Only 1E11B0's native registration changes.
All 22 whole-loop inputs retain their complete original traces in 44 bound and
unbound comparisons. The diagnostic observer is removed, its target rebuilt and
all qualified source hashes restored. No launcher runtime was promoted.

The next pinned emission frontier has three routines: 1E1200 (106 sites),
144910 (77 sites) and 145560 (69 sites). Its static direct-call graph has no
further callees; this audit does not qualify their behavior. These owners,
remaining reached marker/matrix owners, every authored lifecycle and remote
role, independent rendered/Xbox waveform parity, generator closure and selected
runtime delivery remain open. The full requested 1-to-1 condition is unfinished
and the goal remains active.

`marker-lookup-retention.json` records completed scratch cleanup, compressed logs
and the still-active supervised build's path and measured size. The previously
retained 156,065-byte lookup reference is now qualified. No large runtime
binaries, raw banks or captures are retained permanently.

## Preceding native marker-mode checkpoint

`marker-mode-validation.json` records the preceding qualified source checkpoint.
FC510 now has an explicit native owner and the physics-support caller uses it.
The original 15-instruction query reads the actual object flag byte, preserves
partial registers and the caller's FP/MMX state, and keeps the original return
and fault behavior. Its older literal body remains byte-identical after removing
the new native prefix. The support generator retains this route and both
preceding marker-family native routes.

The physical leaf reference covers 271 inputs, including every one of the 256
flag bytes, four aliases, seven read faults and four active MMX files. The
generated leaf already passed this bounded comparison before its new prefix;
this change establishes explicit shared native ownership without claiming a
previous leaf behavior defect.

The new composed reference executes the entire BACD0/FC510 graph unchanged on
physical Linux i386: 238 inputs, all 59 instruction sites and 16,682 steps.
All 66 callee entries are real executions: 63 return and three stop at original
callee read faults. Stack aliases mutate a pool/header pointer before FC510
re-reads it. There is no instruction patch or supplied game-service return.
The native composition compares full traces, operand reads, ordered stores,
defined flags, GP/FP/MMX state, memory and caller frames through four exported
caller APIs. The earlier 235-input reference and its exact generator source are
preserved separately. These inputs establish this bounded complete subgraph;
they do not establish every weapon lifecycle.

The component suite passes 4,482 cases across 25 complete native owners,
196,608 exhaustive iterator admissions, GCC/Clang/ASAN/UBSAN and 55 wrong-code
controls. All 112 integrated CTests pass. Twenty-two test executables now link
the existing production native library. The same kernel source and strict FP
options are preserved, and the actual dependency audit includes its one shared
object alongside the 22 driver objects.

The final runtime adopts the exact public declarations in the component-v3
snapshot: the optional memory extent interface and normalization statistics.
The new extent method defaults to no borrow, and foreign normalization shortcut
implementations remain outside this cohort. The first refresh stopped at a
changed-header guard; the replacement preflights every input before copying.
All 57 used inputs now match the component freeze and private runtime
byte-for-byte. The earlier comment-only preprocessing proof remains historical
evidence. Later root drift and the entire current runtime graph are separate
qualification limits recorded in the receipt.

All 22 whole-loop inputs still pass 44 comparisons against their complete
physical traces. The 15-weapon/70-phase workload retains identical non-registry
outputs and its full 11,040-site instruction histogram. Only FC510's native
registration changes. The diagnostic observer is removed, its target rebuilt
and every final source pin restored. No launcher runtime was promoted.

Remaining work includes 1E11B0 marker selection, 1E1200 marker emission, reached
matrix/math ownership, all authored lifecycle and remote roles, independent
rendered/Xbox waveform parity, generator closure and selected-runtime delivery.
The full requested 1-to-1 condition is unfinished and the goal remains active.

`marker-mode-retention.json` records scratch cleanup and compressed logs. Added
required references are the 429,487-byte mode fixture, the 431,297-byte composed
fixture and its preserved 425,807-byte first version. A separate 156,065-byte
lookup fixture captures 96 unchanged-original inputs for the next owner; it is
excluded from the 25-owner qualification counts. Its paired operand audit finds
five TU1.5-capable callers and preserves a fixed MapPack startup copy and an
unrelated animation address collision. No large runtime binaries,
raw banks or captures are retained permanently. The supervised temporary build
continues for the same unfinished goal; its path and size are in the receipt.

## Preceding marker-parent checkpoint

`marker-parent-validation.json` records the preceding qualified source checkpoint.
Complete BACD0 now uses a native kernel in physics support, AI, scene-device
construction and the operand-authenticated vehicle B9B90 copy. It retains all
44 original instruction sites, fresh parent lookups, ordered argument rewrites,
partial-byte registers, real FC510 calls and RET4 frames. The tail walk reuses
the original caller frame. The older literal bodies remain byte-identical after
removing only their native prefixes.

All four old caller modes failed the new tests. Physics support executed local
FC510 outside the declared caller boundary. AI/device/vehicle committed ESP on a
faulting PUSH; the physical original retains its previous ESP. The native push
helper commits the stack pointer only after a successful store. All four caller
modes now pass940 complete physical-input replays.

The new unchanged-original reference covers235 inputs and15,743 physical host
steps, with10 aliases,11 read faults, two write faults, four active MMX files and
up to five parent hops. Its63 FC510 boundaries have declared component returns;
those controls do not qualify native FC510 gameplay. Production retains the
real callee owner. The component suite passes3,973 detailed cases across24
complete owners,196,608 exhaustive iterator admissions, GCC/Clang/ASAN/UBSAN
and52 wrong-code controls. All105 integrated CTests pass.

Actual dependency files account for53 effective inputs and40 compiled objects.
52 inputs match the component freeze and private runtime byte-for-byte. One
shared header differs by an exact comment replacement. GCC and Clang produce
byte-identical complete preprocessed output for all21 unique affected compile
commands, including actual target and source-specific options:42 comparisons.
The private header and objects remain unchanged. The receipt explicitly records
the byte difference. Unrelated copied-header changes do not qualify the entire
current checkout or runtime graph. The immutable source manifest pins12,818
files and the current native ABI has identical effective tokens.

The22 retained whole-loop inputs still pass44 bound/unbound comparisons against
their complete physical original traces. Every non-registry field of the
15-weapon/70-phase workload equals the preceding checkpoint, including the full
11,040-site PC histogram, zoom/firing results and PCM. Only registration metadata
changes: BACD0 is native and FC510 is explicitly listed as its required lower
target. The temporary observer is removed, its test target rebuilt and all pins
restored. These bounded inputs do not prove every authored lifecycle.

All four generation backends emit the native parent prefixes. Support, device
and vehicle finish complete generation; AI retains its independently guarded
difficulty-body postprocessing failure. The vehicle generator also retains the
preceding BAD60 prefix. No foreign normalizer implementation or weakened guard
was imported to obtain these results.

`native-marker-parent-lower-frontier-v1.json` authenticates the next owners:
FC510 has15 instruction sites,1E11B0 has28 and1E1200 has106. These, the remaining
reached matrix owners, full authored lifecycle/remote roles, independent rendered
and Xbox waveform parity, generator closure and selected-runtime delivery remain
open. The requested full1-to-1 completion is unfinished and the goal is active.

`marker-parent-retention.json` records compressed logs and completed scratch
cleanup. The added required reference is
`tests/fixtures/weapon_effect_marker_parent_tu15_original.json.gz` at433,346 bytes.
No large runtime binaries, raw banks or captures are retained permanently. The
same supervised temporary build remains active for this unfinished goal; its
path and measured size are recorded in the validation receipt.

## Preceding marker-matrix checkpoint

`marker-matrix-validation.json` records the preceding qualified source checkpoint.
Complete BAD60 now has a native kernel and six caller source copies: physics
support, AI, sandbox, scene-device construction, damage delivery and vehicle
motion. Seven exported API modes preserve canonical and selected observation
PCs, actual lower targets and return words. The two paired copies have all236
original instruction/operand mappings authenticated. The older literal bodies
are retained byte-for-byte after removing only the new prefixes. The seven old
caller modes failed the strict original tests; six now pass1,206 complete replays. Vehicle composition passes126 complete
inputs and75 exact real-owned lower entry comparisons, without supplied lower
returns. Its original public ownership is retained.

The new unchanged-original reference executes201 cases over118 owner instruction
sites and15,941 physical host steps. It covers18 aliases,13 backward inputs,
17 read faults,16 write faults and four active MMX files. It checks fresh pool
and body reads, both ordered stack writes, partial-AX returns and the real SSE
environment. The134 lower boundaries in this component reference have declared
ABI controls; these supplied controls do not establish lower gameplay parity.
The production kernel delegates those boundaries to their existing real owners.

The component suite passes3,738 detailed cases across23 complete native owners,
196,608 exhaustive iterator admissions, GCC/Clang/ASAN/UBSAN and49 wrong-code
controls. All100 integrated CTests pass. The immutable source manifest pins
12,815 files, including the current guarded normalization callback declaration
and its header dependency. Foreign normalization shortcut implementations were
not copied and remain unbound in this frozen implementation. Actual dependency
files and compressed runtime inputs verify51 used inputs and38 compiled
dependency objects match the component freeze, private runtime and current
owned root sources. Any later drift must be qualified separately. This does not qualify
the entire concurrent checkout or promote a launcher runtime.

The new physical loop companion retains all22 original inputs and3,826 unchanged
host instructions, with no instruction patch or supplied game service. The
strict composed test now compares each complete physical PC trace, ordered
nonstack writes and full hardware exit except IF/TF/RF, in44 bound/unbound
observer comparisons. Both previous loop fixtures remain unchanged. These22
complete input traces do not establish every branch or lifecycle in the graph.

Every non-census field of the15-weapon/70-phase workload equals the preceding
checkpoint, including zoom, firing and PCM results. The only PC-count difference
is BAEB0:28 observations become26, removing one emulator-only zero-count REP
event from each of two completed copies. The temporary diagnostic observer is
removed, its target rebuilt and every source pin restored.

Six generator modes pass complete regeneration and hook checks in owned scratch.
AI regeneration still stops at an independently guarded difficulty-body hash
mismatch after lowering its source. Current root support/sandbox generation also
encounters concurrent normalization guards. Those failures are preserved; no
guard was weakened or foreign source replaced to obtain a passing result.

`native-marker-lower-frontier-v1.json` authenticates the next three lower owners:
BACD0 has44 instruction sites,1E11B0 has28 and1E1200 has106. These owners,
reached169376/1693AB/16945E/145560 ownership, all authored lifecycle/remote roles,
independent rendered and Xbox waveform parity, generator closure and selected
runtime delivery remain open. The full requested1-to-1 completion is unfinished
and the goal remains active.

`marker-matrix-retention.json` records completed scratch cleanup, compressed
logs and retired helpers. Added required reference inputs are the587,062-byte
compressed marker reference and111,601-byte physical loop companion. No large
runtime binaries, raw banks or captures are retained permanently. The supervised
temporary build remains needed for this same unfinished goal; its measured size
and path are recorded in the validation receipt.

## Preceding location-copy checkpoint

`location-copy-validation.json` records the preceding qualified source checkpoint.
Complete17DD50 and17DDC0 now use native C++ kernels, retaining their matrix
source selection, actual169376/1693AB lower calls and real return/stack ABI.
Each MOVSD reads before writing and advances its cursors only after a successful
store. Backward copies, overlapping operands and partial faults keep the original
GP/defined flags and complete FP/MMX state. Seven local copies in physics
support, particle frame, combat and death cycle use these owners. Both generator
hooks and all four real module generators retain the routes; literal fallbacks
remain intact. All four old caller modules failed on their extra zero-count
REP observation; all four now pass483 caller-case replays.

The new physical reference executes138 cases over all71 owner instruction sites,
including24 aliases,60 backward copies,24 real read faults,22 real write faults
and four live MMX files. It executes117 actual lower instruction sites and18
real lower calls unchanged, with no patched instruction or supplied lower result.
The native caller test replays those recorded lower boundaries and additionally
checks56 native fallback boundaries and74 unavailable-owner stops. That does
not qualify a native replacement for the lower owners themselves.

The component suite passes3,537 detailed cases across22 complete native owners,
196,608 exhaustive iterator admissions, GCC/Clang/ASAN/UBSAN and46 wrong-code
controls. All92 integrated CTests pass in`location-copy-runtime-v2`, after a
declared shared-public-ABI refresh. The owner-name comment correction is also
requalified separately. The final immutable source manifest pins12,808 files.
Actual dependency files verify46 used inputs match the component freeze and
private runtime. The current workspace subsequently added a guarded normalization
callback to one used shared header. That newer ABI/dependency graph is explicitly
unqualified; the report records its hash drift. The first optional normalization
ABI was adopted, with its callback left null in this frozen runtime. The foreign
shortcut implementation was not imported or launcher-promoted.

The first expanded integration run exposed the legacy loop-audio fixture's
emulator REP observation. A separate physical run executes all22 original
inputs, checking3,826 unchanged instructions, identical ordered nonstack writes
and full hardware exits except IF/TF/RF. The original fixture remains unchanged.
Its physical companion authenticates13 executed MOVSD observations where the
emulator recorded14. The composed test uses physical observations for the two
new native owners and retains the declared legacy contract for unconverted
BAD60's BAEB0 instruction. It does not claim a complete physical whole-graph
trace. All22 cases pass44 bound/unbound observer comparisons. The earlier BAC70
owner label was incorrect; original CFG/source inspection identifies BAD60.

Every non-census field of the15-weapon/70-phase live workload equals the previous
checkpoint, including PCM and zoom/firing results. Only17DDB4 changes4,256 to3,952
observations and17DDFB changes1,680 to1,560: one extra zero-count observation is
removed from each of304 and120 completed copies. The new registry records the
two native owners and their two required lower targets. The diagnostic observer
is removed, its target rebuilt and all source pins restored.

`native-loop-location-frontier-v1.json` identifies the next complete owner:
BAD60 has118 instructions, four named copies and two paired-image copies that
require an operand-map audit. Reached169376/1693AB/16945E/145560 lower ownership,
the current shared ABI, all authored lifecycles/remote roles, independent
rendered/waveform parity and selected-launcher delivery remain open. The goal
remains active. This checkpoint does not certify full1-to-1 weapon completion.

`location-copy-retention.json` records completed scratch cleanup, compressed
logs and retired helpers. Required reference inputs added here are the457,760-byte
compressed location-copy reference and111,622-byte physical loop-trace companion.
No large runtime binaries, raw banks or captures are retained permanently. The
supervised temporary build remains needed for the same unfinished goal; its
path and measured size are in the current validation receipt.

## Preceding paired-caller checkpoint

`paired-routes-validation.json` records the preceding qualified source checkpoint.
All five authenticated paired TU1.5 copies now use the complete native owners:
BACC0/BBE00 in biped falling damage, and BACC0/BBE00, C9A40/CABC0,
177DA0/1791C0 and1781F0/179610 in damage delivery. The public canonical and
selected-observation APIs retain their own PC domains. Required-owner sites
remain canonical, while targets, real PUSH/return words, registers, flags and
the complete FP/MMX file retain the original TU1.5 ABI. Callee observations
keep their separate ownership. Older-image bodies remain byte-identical after
stripping only the declared native routes. All three generator modes pass
regeneration and exact strip/reinstall checks against their authenticated
original inputs and guarded canonical baseline.

The actual CC95D CALL inside complete CC590 had the same lost-journal defect
as the previous firing caller: one real CALL and three completed reset stores,
but zero journal events. Its route now enters the existing native world zoom
context and publishes exactly one reset for both local and nonlocal player
roles. The actual player/unit generations, authored sound and tick survive.
The original producer-local gate emits one raw sound request locally and zero
remotely. Local actual-request and recipient-projected PCM are bit-identical
and have positive energy in this test.

This damage test computes upstream nonlethal damage from the actual MCC cache
and vitality, executes the native request/report/reaction owners, then explicitly
runs complete CC590 on that real completed report. The delivery adapter's report
preparation is a distinct stage. No lower result or reset event is supplied.
This proves that original caller boundary and journal/audio route; it does not
prove ordinary hit admission, joined transport or Xbox waveform equality.

All87 integrated CTests pass. Nine new paired caller tests replay1,003 physical
original cases through both damage observation modes and biped falling damage;
all nine failed on the previous local paths. Together with the preceding18
caller tests,27 exported-caller tests replay3,430 cases. The native component
suite still passes3,399 detailed cases across20 owners,196,608 exhaustive
iterator admissions, GCC/Clang/ASAN/UBSAN and43 wrong-code controls. Actual
compiler dependency files verify44 used inputs agree with the component freeze,
private runtime and current owned root sources. Both changed generated modules
were rebuilt after their declared source refresh.

The15-weapon/70-phase runtime census is exactly JSON-equal to the preceding
checkpoint, including audio values and the full11,040-site PC histogram. Its
temporary observer is removed and the test target rebuilt. The immutable
qualified manifest pins12,803 source files.

`native-owner-paired-copy-census-v2.json` closes the five TU1.5 gaps in that
census. Three explicit MapPack1.1 bodies remain preserved: two in death startup
and one in the unit object graph. Their State cannot select TU1.5. Their legacy
public APIs remain reachable through the older-image constructor branch; the
TU1.5 branch selects PhysicsLiveWorldTu15, whose dispatcher does not call those
legacy APIs. This is a bounded source-routing audit, not proof of every dynamic
call path or a conversion of older-image behavior.

Remaining lower owners, full authored lifecycles, remote firing presentation,
independent rendered/waveform comparison and launcher delivery remain open.
`native-location-copy-frontier-v1.json` authenticates the next reached17DD50/
17DDC0 instruction sites and named local copies; it is an input to their native
qualification, not an implemented replacement. The goal remains active.
`paired-routes-retention.json` records completed scratch cleanup, compressed
logs and retired helpers. The same supervised temporary build remains needed
for the unfinished goal; its path and measured size are recorded in the current
validation receipt. No large runtime artifacts are retained permanently.

## Preceding caller-routing checkpoint

`native-routes-validation.json` records the preceding qualified source checkpoint.
Eighteen canonical-name TU1.5 local copies now call the complete native owners.
The actual1088E5 reset CALL inside108080 and the module's dynamic CABC0 dispatch
retain their real call site/target/return/stack ABI while using the existing
world-owned zoom-unit context and committed-reset callback. Explicit older-image
fallbacks are preserved.

A live MCC regression reproduced the previous failure: complete108080 reached
its actual1088E5 CALL and committed all three zoom-reset stores, but published
zero journal resets. After the routing fix, the same whole owner publishes
exactly one reset for both local and nonlocal player roles, retaining the full
unit/player generation and original tick. Its original two stack arguments
are an actual ready weapon and barrel0 on the native world's retained stack,
TLS and FP frame; no lower-owner results are supplied. The authored binocular
sound is NONE. A separate stronger run observes zero matching zoom sound
requests in both roles, retaining the original silence without inventing a cue.
This is request-level evidence, not independent Xbox waveform equivalence.

The18 new exported-caller tests replay2,427 unchanged-original cases, comparing
GP/defined flags, complete FP/MMX, PCs, ordered reads/stores, memory and fault
boundaries. Fourteen tests failed against the old local paths; all18 now pass.
The direct sandbox CABC0 CPU API has no reset-publication callback; its171 cases
qualify its raw ABI, while the whole108080 live test above proves actual journal
and context delivery. The native-owner suite still checks its108 committed
reset decisions independently.

All77 integrated CTests pass, including existing zoom/network/audio/firing
checks. The stronger request-observation variant of the live caller test is
requalified separately; the other76 tests and production inputs are unchanged.
The native suite still passes3,399 detailed checks across20 native owners,
196,608 exhaustive iterator admissions, GCC/Clang/ASAN/UBSAN and43 semantic
wrong-code controls. Actual compiler dependencies verify44 used inputs match
across the component freeze, runtime cohort and current owned root sources;
unused copied-header drift remains outside that dependency set. The existing
15-weapon/70-phase runtime census is byte-for-byte JSON-equal to its preceding
run. Diagnostic source is restored and rebuilt.

The canonical-name census now has43 native prefixes and one explicit disc1.1
copy. A broader authoritative actual/canonical mapping audit found five more
TU1.5 copies: BACC0 in biped-falling and BACC0/C9A40/177DA0/1781F0 in damage
delivery. They still need native routing with their canonical/selected PC and
lower-owner contracts, including the damage reset context. An offline audit
verified all356 instruction sites and operands against both unchanged pinned
originals. These maps are ready inputs, not completed native bridges. Three
explicit disc1.1 copies also require runtime-use/correspondence audit. Remaining
lower owners, complete authored lifecycles and independent rendered/waveform
parity remain open. The selected launcher runtime is preserved.

`native-routes-retention.json` records compressed logs, retired helper sources,
and completed standalone/oracle cleanup. The supervised temporary main build
remains active for the same unfinished goal; its exact path/size and next work
are recorded in`native-routes-validation.json`. No large runtime binary, raw
bank or capture is retained permanently.

## Current local-sound owner checkpoint

`sound-local-validation.json` records the preceding qualified source checkpoint.
Native18BE00 now retains the original local-sound decision: sound flag0x40,
NONE gates, all256 owner type bytes with the actual32-bit shift masking, and
preserved upper byte-register contents. Both local copies (physics support and
map-world support) use its integer kernel without exporting/reloading live FP.

All316 physical-original cases pass through the native dispatcher and both
compiled caller modules. They execute7,210 unchanged instructions over all26
sites, including8 genuine memory faults and4 live MMX files, with no patches or
supplied lower owners. The complete component suite passes3,399 detailed checks
across20 native owners, plus196,608 exhaustive iterator admissions, under GCC,
Clang and explicit ASAN/UBSAN. All43 semantic wrong-code controls are rejected;
restored source passes. All58 integrated CTests pass. Actual dependency files
verify43 used inputs agree across component, private runtime and current owned
root sources; three drifting copied headers are outside those dependency sets.

The actual15-weapon/70-phase workload selects the registered native18BE00 owner
194 times. All non-census report values, including PCM energy/submissions,
and the complete observed instruction histogram match the prior run. Direct
call sites/counts are unchanged. The diagnostic observer is removed and its
target rebuilt; the immutable qualified source manifest retains12,798 pins.

A separate canonical-name audit found44 generated copies of these implemented
owners:25 have native prefixes and18 additional copies explicitly select
TU1.5 but still need their bridges. One disc1.1 unit-graph copy needs separate
correspondence before conversion. The scope-reset copy has an actual1088E5 call within108080. Its former bypass
and subsequent repair are recorded in the current caller-routing checkpoint. `native-local-copy-census-v1.json` records the
precise rows, version admissions and proposed boundary; it is not completed
routing or whole-graph acceptance. Remaining lower owners, full lifecycles and
independent rendered/waveform parity remain open. The launcher is unchanged.

`sound-local-retention.json` records completed-probe cleanup, compressed logs
and retired command sources. The same temporary main build remains active for
this unfinished goal; `sound-local-validation.json` records its path, size and
purpose. No large runtime binary, bank or capture is retained permanently.

## Current native datum allocator checkpoint

`datum-validation.json` records the preceding qualified source checkpoint. Native
16E6A0 now owns the complete raw pool-allocation ABI. Seven local generated
copies route to that kernel: physics support/world/startup, AI, level setup,
scene-device construction and shadows. The existing typed physics allocator
is preserved. All eight public caller routes retain their observation address
contracts and pass the same 213 physical-original cases.

Those cases execute12,756 unchanged i386 instructions across all78 sites,
replay all162 prior completed allocator inputs, and include18 backward REP
cases,23 real read/write faults, stack aliases and4 live MMX files. The native
code retains signed scans/capacity gates, fresh reads after bitmap/header
aliases, zero-count clearing, partial REP fault state, salt wrap, exact stores,
GP/defined flags and the caller's complete FP/MMX file.

The expanded component suite passes3,083 detailed checks across19 native
owners, plus196,608 exhaustive iterator admissions, under GCC, Clang and explicit
ASAN/UBSAN. All40 semantic wrong-code controls are rejected; restored source
passes. All55 integrated runtime CTests pass. Actual compiler dependencies
verify41 used inputs match the component freeze, qualified runtime cohort and
current owned root sources. Surrounding foreign changes remain outside the
whole-checkout acceptance claim.

The actual15-weapon/70-phase MCC workload now reaches the registered native
allocator815 times. Every non-census report field, including zoom/firing/reset
results and PCM energy/submissions, matches the preceding run exactly. Direct
call sites/counts are unchanged. Only16E733 trace counts change17123 to16308:
the native REP trace matches physical single stepping, removing the generated
predecessor's extra zero-count observation after815 nonempty clears. The test
observer overlay is removed, its target rebuilt, and all12,794 source pins
restored. This does not certify independent Xbox waveform or rendered equality.

The following18BE00 conversion is recorded in the current checkpoint above.
Remaining lower owners, full authored weapon lifecycles, remote firing
presentation and independent GPU/waveform comparison remain open. The selected
launcher runtime has not been promoted.

The required compressed allocator reference is
`tests/fixtures/weapon_effect_datum_allocate_tu15_original.json.gz` (196,892 bytes).
The next sound-owner reference is
`tests/fixtures/weapon_sound_local_tu15_original.json.gz` (172,666 bytes).
`datum-retention.json` records bounded reports/log compression, driver retirement
and removal of completed standalone/oracle builds. The same supervised main
owner remains active for this unfinished goal; its measured size and purpose
are recorded in`datum-validation.json`. It is temporary, not a published runtime.

## Implemented

The native multiplayer playback adapter previously required an object-owned
request and matched its owner only against the primary/offhand weapon or an
occupied vehicle. Actual zoom requests can be unowned. The C7DD0 caller's unit
was lost before the PC adapter received the completed request. A native scoped
observer retains that actual unit around C7DD0's C8106 call and CABC0's
forced reset. Normal zoom packets now enter a native journal with the full
player and unit datums. Committed CABC0 resets enter that journal before the
producer-local sound gate. `native_weapon_zoom_audio_tu15` selects the receiver's
local view by its complete player datum; the original normal request bytes stay
unchanged. A reset cue survives unit retirement without passing to a reused
player slot. The earlier unit-only route remains component reference evidence.
Ordinary unit sounds keep their separate audio owners and cannot acquire a
duplicate weapon-audio route. Playback still consumes the original sound,
control, gain and location request. It does not infer audio from FOV or names.

`native_weapon_effect_dispatch_tu15` implements TU1.5 106F90/108A70
selection/dispatch, 17DFF0 live-generation refresh, 178E30 effect creation,
17A1A0 effect restart,
17B130 event reset, 17DC60 component reset,
17AFB0 allocation/retry, 17B960 location-pair initialization, 16E6A0 raw datum allocation,
17B0D0 placement/scalars, 17E5E0 component-handle initialization,
17DCE0 location iteration,
BBE00 object-origin calculation, 1791C0 direction records, 179610 attached-effect
request construction, CABC0 scope reset, 18BE00 local-sound decisions,
18BE50 direct firing-sound requests, and 18C4B0
unowned forced zoom-out requests in native C++. The live dispatcher uses these kernels after the existing request
observer, so actual birth/color/location publication remains in place. The
kernels preserve:

- The original model-object/parent selection, including eager tag lookup and
  lazy object/model-field reads.
- Direct `snd!`, `effe`, NONE and unrelated tag-group behavior.
- Signed tag indexing, full owner identities, raw A/B control words, refresh
  before creation, actual factory return values, and original call/stack ABI.
- The existing required allocation, location, event-restart and voice owners.
  No replacement effect, successful handle, voice or source admission is supplied.

All added production logic is native C++. The remaining lower required
callees retain their current implementations; this does not claim their conversion from
lifted code or removal of every lifted weapon/effect routine.

CABC0 now publishes its committed reset decision at CAC96, after the three
unit stores and before the producer-local sound gate. The observer retains
the actual full unit/player/weapon datums, previous selector, request byte and
106040 result. The additional 144 unchanged-original cases compare all stores,
registers, lower call/stack ABIs and 92 such decisions, including nonlocal
players; 44 cases reach the original sound tail call. The five component suites
pass 1,024 cases under GCC, Clang and sanitizers. A fourth wrong-code control
that filters committed resets by producer locality is rejected.

The native zoom journal carries actual normal request bytes and the authored
sound selected from committed resets. Native protocol124 carries it to peers;
older body codecs omit it. Match authority checkpoint4 retains its serial and
events. Both authority and joined presentation use the same recipient-player
route; the old host audio lane excludes zoom to prevent duplicates. Pending
cues and their cursor belong to the accepted presentation receipt. NONE stays
silent, repeated receipts do not replay cues, and retention gaps are counted
without reconstructing absent requests. The complete authority archive also
requires Match checkpoint4; a real MCC restore regression caught and corrected
its stale version3 guard. Runtime/UDP/PCM qualification for this transport phase
is recorded separately from the earlier checkpoint below.

The next native owner is 17DCE0, shared by effect restart, particle frame,
physics support and death-cycle callers. Its integer kernel preserves the
WORLD (mode 0), first-person (mode 1) and unfiltered (mode 3) gates, signed low-word
selectors, partial-register writes, cursor aliases, recursive source stack
bytes and defined arithmetic flags. Caller XMM/x87/MMX files stay in place.
All three complete caller modules enter the same kernel through their internal
17DCE0 function, and their generators preserve that route. A separate native
library supplies both gameplay and lower physics-only consumers.

The new unchanged-original reference includes 156 complete/read-stop cases,
62 actual recursive calls, 20 cursor aliases and 8 genuine unmapped reads.
Its three 65,536-byte admission tables exhaust all 196,608 node/mode inputs;
the WORLD table matches the existing independent original fixture byte for
byte. No instruction or callee is replaced. Six component checks now pass
1,180 detailed original cases and the exhaustive domain under GCC, Clang and
explicit address/undefined sanitizers. Six deliberately incorrect variants are
rejected, including cursor read-order and mode3 filtering. The component receipt
is `native-eleven-owner-traced/report.json` under the report root. The fresh
full-runtime caller/weapon/audio cohort now passes all 26 checks in
`iterator-runtime-traced/report.json`. Its three compiled caller APIs match the
same complete/read-stop cases and exhaustive admission domain, including full
returned GP/XMM/defined flags, memory, stack and FP/MMX preservation.

The first runtime pass exposed the existing looping-audio requirement for a
complete instruction-PC trace. The iterator now publishes every original PC
at its actual native operation/branch boundary. Independent original trace
offsets also qualify this path in all 156 detailed cases, including read stops.
The strict looping-audio test passes unchanged. Three new caller tests initially
read the retained public ABI during a compiled call, although that API publishes
it on return. Their corrected observation checks the full returned ABI,
recursive stack and instruction trace; the direct kernel test still checks
every original recursive GP boundary. The initial red results remain recorded.

The next two native owners are complete 17B130 event reset and 17DC60 component
reset. They retain signed generation checks, eager tag lookup, signed event
selectors, component iteration and the actual17AAB0/259B0 results. The timer
spill uses the returned x87 control word, live extended operands and source
status; it saves and restores the host FP file around that exact single store.
Signed zero and unordered bounds follow UCOMISS rather than raw-bit equality.
The component reset reads duration before changing an aliased row flag.

The unchanged physical i386 reference covers 208 cases, 158 declared lower ABI
controls, 12 genuine unmapped-read stops and four duration/flag aliases, with
13,092 original instructions. GCC, Clang and explicit address/undefined
sanitizers pass 1,388 detailed original cases plus 196,608 iterator node/mode
admissions. Nine incorrect variants are rejected. Both compiled caller modules
match the reference, and all 29 runtime checks pass in `state-runtime-v2`.
`state-validation.json` pins an immutable source manifest for this checkpoint.
The first state runtime build retained a real compile failure after a refresh
picked up concurrent object-region work. The corrected private sources
reconstruct the prior qualified caller files byte for byte and apply only the
owned native bridges; concurrent root changes remain in place.

The native 178E30 creator preserves admission, attachment, placement, restart
and final validation in source order. It retains the actual allocator and
attachment returns, including NONE, and cannot replace an unavailable owner.
The physical reference covers 64 cases, all 214 instruction sites, 535 lower
calls, eight unmapped-read stops, four overlapping input/position cases and
four active-MMX inputs. Every one of those 535 unavailable-owner boundaries
is checked separately. Both complete caller modules use the native creator.

The fourteen-owner cohort passes 1,452 detailed original cases under GCC,
Clang and explicit address/undefined sanitizers, plus the exhaustive iterator
domain. Twelve incorrect variants are rejected. All 32 runtime checks pass;
the three creator checks also pass the extended MMX reference through the
same unchanged production code. `creation-validation.json` records that union
and its immutable manifest. The component reference controls fifteen lower
owners; their actual allocation, attachment, random, audio and rendered results
still require their own qualification.

The next four native owners cover placement, handle initialization, allocation
and location-pair initialization. Complete 17B0D0 retains the eager CMOV default
pointer read and interleaved vector copies. Complete 17E5E0 writes exactly 32
handles in source order. Their 172-case physical reference covers all 62 sites,
20 overlaps, eight active-MMX inputs, 11 unmapped reads and five unmapped writes.
The first read-ledger attempt misclassified LEA as a data read; its corrected
encoding reconstructs that prior reference exactly. The initial red component
report and `initialization-reference-history.json` preserve this diagnosis.

Complete 17AFB0 preserves admission, the real allocator result, reclamation
and retry, source record stores and actual child calls. Complete 17B960 resets
16 location pairs and preserves the original ADD/DEC flags and fault prefixes.
The 68-case composed hardware reference executes the real 17B960 and
17B130/17DC60 children: 61 actual child calls and 116 declared allocation,
reclamation and RNG/bounded-real controls. It includes 12 access stops, four
overlaps and four MMX inputs. All 177 unavailable-owner boundaries are checked
separately. The reference generator also needed an explicit FSTP write
annotation because Capstone omitted its write bit; the unchanged CPU had
already committed those stores. `allocation-reference-history.json` preserves
the exact preceding encoding. Both complete compiled callers use these kernels.

A stricter audit then found an earlier selector gap: 106F90 returned correct
GP values but left EFLAGS at 0x202 instead of the original 0x297 and emitted no
complete PC trace. The corrected native selector retains the original flags,
all 30 instruction sites and the exact eager/lazy read order. Its physical
reference replays all 256 prior selector inputs and adds eight unmapped reads
and four MMX cases. The regression fails before the correction and passes
under GCC, Clang and address/undefined sanitizers afterward.

The dispatcher audit then found that unchanged TU1.5 108A70 returns EFLAGS
0x246 where the earlier kernel returned 0x202, with its full PC trace also
missing. The corrected native dispatcher now replays all 256 preceding inputs,
eight unmapped reads and four MMX inputs against unchanged physical i386
execution. It executes the actual106F90 selector in 252 cases, records all97
instruction sites and checks all140 lower-call and unavailable-owner boundaries.
The reference covers defined flags, full GP/XMM/live80/MMX state, stack bytes,
ordered operand reads, stores and fault stops. The prior counterexample remains
as historical evidence in `strict-dispatcher-abi-gap.json`; it is resolved.

The CABC0 reset audit then added a complete physical i386 reference. It replays
all144 earlier inputs and adds11 read faults, four write faults, eight overlaps
and four MMX inputs. The corrected native reset retains all108 instruction
sites, defined flags and original arithmetic/register staging, including state
after partially committed resets. It checks473 lower-call and unavailable-owner
boundaries,108 committed reset publications and52 real sound-tail boundaries.
The original144 state, publication and sound decisions are preserved. The
stricter regression fails before the correction and passes afterward.

The18BE50 direct-sound and18C4B0 forced zoom-out request kernels now also
retain all source flags and instruction boundaries. Their physical reference
replays all48 previous request inputs and adds13 genuine stack read/write
faults and four MMX inputs. All52 instruction sites and122 lower-call and
unavailable-owner ABIs are checked. The original request bytes, GP/XMM outputs
and full544-byte live/dead stack domain remain identical on earlier inputs.

This audit exposed a shared PUSH/CALL fault bug: the helper committed ESP
before its stack write. Unchanged i386 execution leaves ESP unchanged when
the write faults. The helper now commits ESP after a successful write and
before the store observer. With the revised sound kernels, the regression
passes all48 earlier inputs and fails at the first new push fault before this
fix. The corrected source passes all65 audio cases, including three read
faults, ten write faults and eight faulting PUSH/CALL boundaries. The two red
reports preserve the missing-trace failure and the subsequent GP fault failure.

The17DFF0 refresh,17A1A0 restart and179610 attached-request owners now retain
complete native flags, instruction traces and exact source register staging.
The physical references replay all128 preceding inputs and add signed bounds,
signed generations, twelve overlaps, eight MMX inputs and29 genuine read/write
faults. They check184 cases,144 instruction sites and205 lower-call and
unavailable-owner boundaries. REP STOS follows the caller's direction flag;
a stopped iteration retains its actual cursor/count and committed store prefix.
An overlapping restart output can overwrite a saved register, and the kernel
returns that original result. RMW operand reads remain distinct from observer
preimage reads. Lower reset, iterator, origin, direction, creation and sound
results remain explicit declared controls in these component references.

BBE00 origin and1791C0 direction now also retain their complete source PC
traces and operand/store ordering. The origin kernel reproduces the copy-path
arithmetic flags, signed matrix offsets, UCOMISS/LAHF/TEST flags and SSE sticky
state, including partial transforms and stopped stores. Direction retains its
caller's flags through all records. The new physical references replay all192
preceding inputs, with no supplied callees, and add partial-vector, matrix,
stack and return faults plus eight MMX cases. Both111-case groups pass, covering
114 instruction sites and8,905 unchanged physical i386 instructions. Caller
SSE state is restored after normal returns and faults.

The current qualified cohort passes46 runtime checks and2,870 detailed
reference case checks, plus the exhaustive196,608 iterator admissions. All36
incorrect variants are rejected. Component validation shares one native kernel
library and uses the supervised official worker for its private mutations.
Every mutation rebuilds exactly its kernel object, reuses unchanged test
drivers and runs the same suite; restored production source must pass again.
GCC, Clang and explicit address/undefined sanitizers pass18 component checks.
Mutation selectors are now checked before compilation. The first location
attempt was cancelled because two selectors still used the preceding source
layout; its interrupted report remains as a diagnostic.

The broad copied include tree changed during validation. Actual compiler
`.o.d` files identify38 used component inputs; those match the private cohort
and current owned sources. The six changed root headers are outside that
set, and the broader private/root drift is recorded separately. This qualifies
the declared source cohort and does not qualify the entire concurrent worktree.

`native-owner-reference-coverage.json` now records bounded strict flags/PC
qualification for all eighteen currently implemented weapon owners. This
closes the earlier-owner ABI audit, not the whole lower-owner graph. These
component and runtime results do not establish every authored weapon lifecycle,
waveform or framebuffer parity, or launcher delivery.

The native MCC scalar converter also restores twelve missing exact authored
effect flag tuples. Original17AAB0 chooses the primary RNG for bit2 and the
auxiliary RNG otherwise. The independent38-map weapon/projectile/child-effect
audit covers70 effect paths and finds sixteen stable flag differences; four
already had profiles. The twelve additions include beam-rifle/carbine/Brute
Shot firing, plasma charging, several detonations, Spectre and Wraith fire.
Wraith clears the bit; this is not a blanket bit set. Exact group/name/whole-
word guards and the existing custom-authoring admission preserve other data.
Original and MCC assets on disk are not edited.

The full native catalog check exposed one unavailable dependency in Cairo:
its MCC directory lacks the Sniper Rifle zoom identities authored by the Xbox
Battle Rifle. The native loader now projects a small, private sound dependency
bank from prepared `single_player_shared.map`, with four genuine Classic/
Remastered roots and only the table rows/chunks they reference. It publishes
new local salted handles, rebases all metadata indices and pointers, and maps
import-name StringIDs by their actual text. The existing guarded Battle Rifle
profile then selects those local identities. Custom authoring, other maps and
unknown reference tuples preserve their existing data. No original/MCC file,
PCM data, voice, source result or gameplay field is supplied by this projection.

## Independent component reference

`tools/oracle_weapon_effect_dispatch_tu15.py` executes the unchanged owned
TU1.5 instructions, pinned by SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Refresh, creation and sound calls are declared test service boundaries with
arbitrary return/clobber controls. They are not actual allocation/audio results.
The 512-case, 673,540-byte JSON reference contains exact completed GP/XMM
results and every boundary's arguments/registers. It is a bounded required
reference input, not a retained executable or raw runtime bank.

The native component test compares those outputs and also checks unrelated
x87/control-state preservation, missing-owner stopped-call ABI and salted
unit/primary/offhand audio routing. A failing missing-owner regression exposed
an early provider check; the corrected kernel first pushes the actual return
address, then stops at the unavailable callee. Release and address/undefined-
sanitizer CMake runs pass, and four deliberately incorrect variants are
rejected. Reports are under
`build/test-results/weapon-effects-native-20261101/`.

`tools/oracle_weapon_effect_request_tu15.py` independently executes the four
new request/refresh owners from that same pinned image. Its 144 cases compare
all instruction-tagged writes, complete live/dead request-stack bytes, required
GP/XMM call ABIs, generation rejection, full-list fill direction and final
results. The explicitly controlled lower services in these component cases are 17A1A0, BBE00, 1791C0,
178E30, 18C090, 18BE00, 18C130 and 18CDD0. This is request-owner evidence, not
an allocation or waveform oracle.

The additional 32-case restart reference executes unchanged 17A1A0 with
declared reset/location/sound callees. It checks signed location selection,
existing-sound suppression, lazy reads and retention of the actual lower
sound return. No successful sound/allocation result is supplied in production.

`tools/oracle_weapon_effect_location_tu15.py` checks BBE00/1791C0 against
192 executions of unchanged original instructions on a physical Linux i386
compatibility child, including 16 genuine unmapped-read stops. The 8,120
instructions preserve signed parent-node/matrix offsets, overlapping copies,
SSE operation order, special float payloads and caller rounding/FTZ/DAZ
controls. Physical execution corrected 33 emulator-only MXCSR expectations;
the final reference uses hardware flags. No original instruction is patched
and these two leaves have no substituted callees. This is host CPU evidence,
not a physical Xbox or framebuffer comparison.

The earlier four component suites cover 880 original-reference cases. That frozen
component cohort passes GCC, Clang and explicit address/undefined sanitizer
instrumentation. Three wrong-code controls (unit-scale branch, direction read
and discarded sound return) fail as expected. The bounded receipt is
`build/test-results/weapon-effects-native-20261101/native-nine-owner-cohort/report.json`.

`tools/audit_weapon_zoom_and_shooting_tu15.py` reads all 1,110 original weapon
rows across 38 maps and creates 40 distinct zoom/ready/charge/firing contracts
covering all 63 weapon paths. Its raw MCC comparison finds 32 Battle Rifle
zoom dependency differences and 28 human-turret firing dependency differences;
the existing native reference converter already contains both exact guarded
profiles. `native_weapon_presentation_contracts_tu15` compares the actual native
Cache publication with the independent original contracts. Its execution result
is recorded separately from the raw inventory.

The separate `native_weapon_zoom_audio_tu15` test runs actual prepared MCC
worlds, selecting each available starting weapon and binocular fallback. It
checks the actual source selector/request, converted native sound reference
and level control, independent local views, native PCM submission, forced
firing resets and manual mixer consumption. It also checks actual scheduled
firing-effect journals against the authored barrel references and source birth/
location publications. Its runtime result is recorded separately from
the component result; registration or compilation alone is not acceptance.

## Qualified results

The latest frozen native cohort passes 26 focused CTests in `iterator-runtime-traced`.
It includes the six native component checks, three compiled iterator caller
checks, strict original looping-audio qualification and every earlier zoom,
source firing, retirement, catalog, wire and continuation check. The production
multiplayer renderer adapter also compiles in that cohort; it is compile
evidence, not a windowed or GPU run.

The previous frozen native cohort passes 21 distinct focused CTests: twenty in
`zoom-transport-runtime-v3`, followed by the new continuation test and repeated
wire/snapshot checks in `zoom-transport-runtime-continuation-v2`. The latter
refresh changes only the archive's checkpoint-version guard and adds the
continuation test. The production multiplayer adapter also rebuilds afterward
in `zoom-transport-runtime-final-adapter-build`; that is compile evidence, not a
windowed or GPU run. New delivery results include:

- Fifteen distinct resident weapon definitions in four joined UDP views,
  including all five audible scoped weapons and ten silent binocular weapons.
  Sixty-eight explicit level transitions, sixty actual admitted melee resets,
  and 88 native PCM submissions pass. Complete journal bytes, source ticks,
  recipient views and authored sound identities match. Duplicate receipts
  produce no replay, the host hears no remote local-view cue, and both cursors
  report zero missing retained events in this workload.
- The same Battle Rifle unit as a local and nonlocal source player publishes
  the same committed reset. Only the local source emits the old sound packet.
  Manual native PCM for that actual local packet and the recipient's reset
  route is bit-identical and nonzero over eight 4,096-frame blocks.
- A real prepared-MCC checkpoint preserves the normal 160-byte request,
  complete owners, serial and pending second-view cue through canonical native
  archive encode/decode and restoration into a second world. Thirty-six future
  source ticks produce the same journal and exactly one actual melee reset.
  Old Match versions and inconsistent event counters are rejected. The
  19,431,750-byte encoded checkpoint stays temporary.
- Ten native owners pass 1,024 unchanged-original component cases under GCC,
  Clang and explicit address/undefined sanitizers. All four deliberately wrong
  implementations are rejected. This remains separate from runtime and GPU
  evidence.

The earlier local/runtime and catalog checks remain covered by the latest
twenty-test run:

- Fifteen actual weapon definitions and seventeen authored zoom levels in both
  local views (34 level transitions). Thirty-four real source requests produce
  34 native PCM submissions, including five audible scoped-melee resets; all
  original caller packets are unowned. Every audible explicit transition
  produces finite, nonzero mixed PCM; intentional NONE transitions stay silent. The original Lockout binocular in/out references
  are NONE, so ten binocular-capable weapons correctly produce no invented cue.
- Seventy actual firing-effect publications, with authored reference identity,
  full unit/weapon ownership and retained source births/locations.
- Plasma-pistol charge metadata changes from flags2 to original6. Its240-frame
  sequence reaches a genuine allocation, player response, retirement before
  NONE, charge readiness/release and the charged projectile.
- Source scheduled/unmapped firing, birth retirement and LAN location transport.
- Every zoom/ready/charging/firing/misfire/empty reference contract across all
  63 weapon paths, 1,110 resident rows and 38 maps matches the independent
  original catalog. Cairo also passes full canonical sound publication with
  unchanged class authentication, exact donor payload/codec/chunk equality,
  and nonzero decoded PCM for both restored zoom clips.
- All sixteen event-RNG tuples pass byte preservation, idempotence, authored-
  value and truncation guards. Thirteen actual original/MCC Lockout metadata
  comparisons also pass.

The first root build failed at link because concurrent physics CMake additions
arrived after configuration. Its disposable build was removed. Qualification
then used an immutable source snapshot with declared refreshes through the
official owned worker. A later current-source refresh exposed strict new physics admission failures.
The native collector already implemented original full-list growth; its
admission now qualifies bounded real pool growth instead of refusing a full
list. Forty-nine unchanged-original array cases and 63 genuine allocator calls
pass, with three incorrect variants rejected. State-only effect-phase
observation and explicitly read-only store observers let retirement/charge
checks retain actual births, colors, locations and NONE returns without claiming
complete instruction traces for native callees.

Concurrent changes remain outside the qualified cohort, so these results do
not qualify the entire current shared working tree. The later frozen cohort
passes fifteen distinct checks across `location-runtime`,
`location-runtime-followup` and `location-recipient-v2`; the initial red
recipient probe remains recorded. Its corrected test waits for readiness,
requires an actual admitted melee action and acknowledges the actual reset.
Production admission and gameplay state are unchanged by the test.

The new scalar-compare compile guard preserves the existing unavailable-backend
failure on unsupported targets. GCC and Clang produce byte-identical x86
objects before and after that guard; it introduces no numerical fallback or
ARM runtime claim. The proof is `native-sse-backend-guard.json`.

The current bounded receipt is
`build/test-results/weapon-effects-native-20261101/location-strict-validation.json`.
`lifecycle-strict-validation.json`, `audio-request-strict-validation.json`, `zoom-reset-strict-validation.json`, `dispatch-strict-validation.json`, `selector-validation.json`, `allocation-validation.json`, `initialization-validation.json`,
`creation-validation.json`, `state-validation.json` and `iterator-validation.json`
retain earlier checkpoints.
`zoom-transport-validation.json`, `location-validation.json`, `finish-validation.json` and `validation.json`
retain earlier checkpoints. The private source matches its declared qualified
manifest. The shared root has unrelated concurrent drift, so this is not a
qualification of the entire current working tree or the launcher-selected runtime.


The subsequent read-only runtime census executes the same15-weapon MCC
zoom/fire/melee workload with full effect-phase observation. Its70 scheduled
effect phases reach11,040 distinct instruction sites,277 direct call sites
and196 direct targets; six indirect sites remain unresolved. Original TU1.5
bytes resolve those sites offline after runtime exit. The sound manager is in
native mode. This census queries only the weapon and sound-manager registries;
other native/platform ownership remains to classify. It is not whole authored
lifecycle coverage.

16E6A0 allocation is reached815 times from11 direct sites, including27 actual
17B000 effect allocations. The existing typed semantic allocator remains
preserved; its raw source-call ABI and mapped frame route need qualification
before another native bridge is added. The diagnostic changes one private
test source only, then restores its exact qualified bytes and rebuilds that
target. `native-owner-runtime-census-validation.json` records the reached
boundaries and restoration. No production source is changed by the census.

## Work still required for complete parity

The recipient boundary test now executes the same Battle Rifle unit as both
local and nonlocal, using actual admitted melee and acknowledged reset input.
Both roles produce one normal zoom-in request. Only the local role produces
the unowned forced zoom-out request; the nonlocal role resets scope without
one. This confirms CABC0's local-user gate in actual MCC execution. Forwarding
host packets alone cannot supply joining-client forced reset audio. This
finding motivated the committed-reset publication and native recipient route
above; the new runtime/UDP checks now pass that delivery boundary.
No reset is inferred from FOV or invented from an absent sound packet. Earlier evidence is in
`location-recipient-v2/evidence/native-weapon-zoom-recipient.json` under the
report root. This check does not establish UDP/client audio delivery.

- Qualify all campaign-only, AI, vehicle, secondary, misfire, empty, sustained,
  charge/release, interruption and dual-hand effect paths.
- Convert and qualify every remaining reached weapon/effect/audio owner;
  native dispatch alone does not make the whole effect graph native.
- Qualify every remote shooting action and full presentation lifecycle beyond
  the joined-client zoom/acknowledged-melee workload above.
- Compare actual particles, trails, lights, lens effects, timing, random
  selection, placement and audio output against independent original
  references across the complete authored scope.

Installed launcher runtimes and existing assets/reference evidence are
preserved. Disposable build/capture storage follows `TEST_STORAGE.md`; only
bounded reports and the required original component reference are retained.

The previous location source/build tree measured 3,034,919,074 bytes before its
`run_test.py` supervisor removed it. The new zoom transport owner's measured
size and verified removal are recorded in `zoom-transport-validation.json`.
The later iterator source/build owner remains active for the remaining strict
ABI and native graph work, at `.local/test-scratch/halo2-test-800zz8ox` (size recorded in the current checkpoint). Its `run_test.py` coordinator is session69153,
PID269302, with a one-hour idle limit. `location-strict-validation.json` records this
active lifecycle, source hashes and remaining work. The same official worker
can reuse its build; completion/cancellation/idle expiry removes it. No new
runtime binaries, banks or captures are retained permanently. Required reference inputs include
`tests/fixtures/weapon_effect_location_tu15_original.json` (776,489 bytes) and
`tests/fixtures/weapon_effect_restart_tu15_original.json` (183,488 bytes), plus
`tests/fixtures/weapon_zoom_reset_tu15_original.json` (605,019 bytes).
The iterator's required expected-data fixture is
`tests/fixtures/weapon_effect_iterator_tu15_original.json` (1,035,249 bytes).
Its current form also reconstructs the two preceding reference encodings
exactly; `iterator-reference-history.json` records the transformations/hashes.
The state reference is `tests/fixtures/weapon_effect_state_tu15_original.json`
(810,782 bytes), with its prior192-case encoding exactly reconstructable as
recorded in `state-reference-history.json`. The creation reference is
`tests/fixtures/weapon_effect_creation_tu15_original.json` (1,011,897 bytes).
Its preceding 60-case encoding is exactly reconstructable as recorded in
`creation-reference-history.json`. Completed standalone component/oracle
scratch is removed. The active main owner remains needed for the same goal;
`state-creation-report-retention.json` records compressed logs and removal of
the completed MMX component source/build copies.
The newer required inputs are
`tests/fixtures/weapon_effect_initialization_tu15_original.json` (674,233 bytes),
`tests/fixtures/weapon_effect_allocation_tu15_original.json` (544,202 bytes) and
`tests/fixtures/weapon_effect_selector_tu15_original.json` (676,232 bytes).
These expected-data fixtures remain source inputs; bounded diagnostics
and losslessly compressed source manifests/build logs remain under the report
root, with hashes and restoration paths in `location-report-retention.json`
and `zoom-transport-report-retention.json`, with the iterator phase recorded
in `iterator-report-retention.json`. The latest log compression and retirement
of superseded owned drivers are recorded in
`initialization-allocation-selector-retention.json`; their small command
sources and hashes remain in `retired-driver-commands.json`. All completed
standalone component, oracle and probe storage is removed. The main supervised
build remains active for the same unfinished goal.

The required strict dispatcher reference is
`tests/fixtures/weapon_effect_dispatch_strict_tu15_original.json` (1,024,281 bytes).
Its generator records16,484 unchanged physical i386 instructions with no patches;
it supplies only the three declared lower-service ABIs.

The strict reset reference is
`tests/fixtures/weapon_zoom_reset_strict_tu15_original.json` (765,768 bytes),
from11,298 unchanged physical i386 instructions. FP snapshot fields equal to
the declared input are compacted only after equality is verified; test replay
reconstructs the complete state before comparison. Its BCDF0,106040 and18C4B0
results are explicit lower-service controls and do not prove those lower owners.

The strict sound-request reference is
`tests/fixtures/weapon_audio_request_strict_tu15_original.json` (445,220 bytes),
from1,555 unchanged physical i386 instructions. Lower18C090,18BE00,18C130
and18CDD0 results and writes are declared controls. Actual compiler dependency
files verify the component inputs match the qualified private cohort; copied
foreign headers outside those dependency sets are recorded separately.

The lifecycle and attached-request references are
`tests/fixtures/weapon_effect_lifecycle_strict_tu15_original.json` (587,998 bytes)
and`tests/fixtures/weapon_effect_attached_strict_tu15_original.json` (483,983 bytes).
They come from7,069 unchanged physical i386 instructions with no patches.
The source hash, two original-input hashes, compiler dependency identity and
red/green reports are recorded in`lifecycle-strict-validation.json`.

The final earlier-owner references are
`tests/fixtures/weapon_effect_location_strict_tu15_original.json` (440,130 bytes)
and`tests/fixtures/weapon_effect_direction_strict_tu15_original.json` (468,998 bytes).
They retain the original128-byte stack domain, all declared input banks and
complete live FP/MMX files. `location-strict-validation.json` records the
qualified source manifest, dependency identity and completed-probe cleanup.
