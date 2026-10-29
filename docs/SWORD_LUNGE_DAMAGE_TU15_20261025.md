# Sword lunge damage: TU1.5 source path and MCC timing lookup

Status on October 28, 2026: a fresh owned rebuild of the retained native
Sword cohort passes five actual lunge/close-damage controls, seventeen
cancellation/held-trigger/airborne/equip controls, and both host/joined-player
actual ONLINE socket lunge cases. The socket rerun uses an explicitly frozen,
matching local test service/library; two earlier CTest attempts failed during
service import, before gameplay, and remain recorded. The [recovery evidence](../analysis/sword-lunge-tu15-20261025/recovery-20261028/README.md)
separates these results from installed-runtime admission. The selected runtime
advanced to `6ef5ae1281fd4b679b4574da6e7da8ee` and its actual admission check
passes; earlier 91-case results are not relabeled for this newer product.
Vehicle stored-flight and complete original local/joined targeting remain open.

Continued October28 work adds the complete native`1AD4F0`/`1AF490` targeting
module and live-world entry dispatch. Actual accepted/rejected Sword and
firearm queries pass comparison with unchanged original i386 code, including
full floating-point state and memory; generation reproduces the committed
337-instruction module. The [new execution evidence](../analysis/sword-lunge-tu15-20261025/targeting-original-proof-20261028/README.md)
also passes a reload-cancel control through the new query: no pending hit,
continued physical mode6, and seven original movement-call comparisons within
their declared comparison scope. It preserves a red late-seat-exit movement
gate and correctly constructed two-weapon far-range controls. Normal local collector ordering and joined
target ownership remain unfinished; the new entry has not been substituted
into the old composed query or published as complete flying parity.

Historical qualification through October 26, 2026: the impact-timing and primary-input corrections
have direct red/green evidence. The earlier 23-case native suite and 52
original movement-call comparisons pass. Continued qualification has also
corrected first-person birth/caller handoffs, three exact missing native
callee routes, and joined-client preservation of received physical mode6.
The recorded protocol116 cohort passes twelve focused component checks and
the complete 91-case directed matrix, including 24 real socket cases and six
under the shipped default network profile. After that cohort's supervised
build was removed on interruption, a fresh coherent source/build cohort passed
all twelve focused checks and full engine/launcher qualification: thirteen
actual map/player smokes and four frontend captures. Its fresh 91-case Sword
matrix passes all cases, including 24 socket cases and 52 original movement
comparisons. The qualified `4b` native build was launcher-selected; a newer
concurrent publication then selected `9d` and was preserved. For `4b`, full
inventory admission and launcher routing pass; a cold four-frame launcher
check timed out under its combined 180-second preflight/render deadline.
That delivery check remains failed while startup timing is investigated.
The newer `9d` native cold run succeeds; its launcher delay is isolated to
readiness probes. A corrected launcher candidate passes routing and the
same cold four-frame deadline, with official launcher delivery in progress.

Historical protocol113 evidence includes 28 rapid-input cases and eighteen
real same-host ONLINE socket cases. The first protocol116 matrix passed all
eighteen socket cases but failed 55 logical cases at a fixture receipt wait;
the exact assembly-rate boundary and its bounded IO correction are recorded
below. These results do not establish WAN, interactive Internet, Campaign
vehicle-flight or Xbox framebuffer acceptance. The earlier selected-runtime
result contains the timing correction only; it is not delivery of the later
input, first-person or protocol116 changes.

## Authenticated action, contact and damage owners

The behavioral authority is English Xbox TU1.5, executable SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Production uses the existing compiled native translations; the executable is
an offline reference, while runtime acceptance uses prepared MCC content.

| TU1.5 owner | Required behavior |
|---|---|
| `EBFA0` | Retains canonical melee activity and authored impact/recovery/duration clocks. Dash activities `0A0005BB`/`130005BC` have no impact event; release selects ground/air lunge `0B0005B2`/`140005B3`. |
| `EC620` | At exact component progress `+32` versus impact `+33` equality, `EC684` calls `D18C0` with the real semantic and modifiers. Physical mode6 and its retained clock govern dash release. |
| `D18C0` | Uses current-pose source geometry and 25 ordered `16C620` casts. Lunge semantics select the weapon's `+1EC` damage/reference pair, with the original fallback rules. |
| `D2180` | Receives the chosen hit packet at `D1F5B`; builds the real damage request, attacker attribution, material and scale. |
| `DA0D0` | Receives the attributed object damage request at `D22F9`, with original collision-region arguments `NONE,NONE,NONE,0`. |
| `D13D0` | Publishes later contact presentation through `D1FBF`; observing presentation alone does not prove vitality loss. |

The implementations are [physics support](../src/physics/physics_live_support_tu15_generated.cpp),
[combat/action](../src/gameplay/units/source_combat_action_tu15_generated.cpp),
and [damage delivery](../src/gameplay/units/source_damage_delivery_runtime_generated.cpp).
The [combat generator](../tools/generate_combat_action_tu15.py) authenticates
complete bodies against the pinned image. Earlier correspondence and source
ABI evidence are recorded in [melee action research](MP_MELEE_ACTION_SOURCE_RESEARCH.md)
and [damage/melee source research](DAMAGE_MELEE_KILL_LIVE_JOIN_RESEARCH.md).

## Timing namespace correction

Playback and gameplay timing have separate lookups. The existing scoped
`169A56 -> 1D5320 -> 1E34A3` playback lookup already admits the named MCC
dash/lunge clips while retaining canonical source activities. That does not
qualify the separate `EC22B -> 16B31B -> 16B3F1 -> 1D3D30` metadata lookup
which supplies impact events to `EBFA0`.

Before this change, the latter boundary translated ordinary strike keys but
returned canonical lunge keys unchanged. The correction in
[the namespace helper](../src/gameplay/weapons/source_weapon_animation_query_symbols_tu15.cpp)
admits only these two copied keys at caller return `EC230`:

| Canonical TU1.5 activity | Named MCC key | Authored name |
|---|---|---|
| `0B0005B2` | `0B0005BE` | `melee_lunge` |
| `140005B3` | `140005BF` | `melee_lunge_airborne` |

`EBF41` remains the four-candidate strike query. `EC154` remains the existing
ordinary fallback boundary; its source call hardcodes `melee_1sthit`, not a
lunge. Dash keys, neighboring callers, raw MCC IDs and original/unbound banks
retain their existing behavior. No damage value, clock scalar, RNG result,
source activity or authored graph bytes are changed. The actual metadata
continues through original `1E30D0`/`1E3100` event extraction and `1494F0`
clock conversion before `EC2AA`/`EC2CF`/`EC307` publish impact/recovery/duration.

The [MCC binding regression](../tests/mcc_first_person_binding_tests.cpp)
independently resolves the actual named ground/air clips for both species and
compares their packed identifiers, graph, frame counts and complete event
lists with the timing lookup. All four ground/air/species timing checks pass
in the [canonical metadata run](../analysis/sword-lunge-tu15-20261025/canonical-metadata/report.json).

## Damage data and trigger authority

Read-only [damage-definition evidence](../build/test-results/sword-damage-data-20261025.json)
records authored `dash_melee` damage 150 in both Xbox and MCC reference inputs.
[Player vitality evidence](../build/test-results/sword-vitality-data-20261025.json)
records 70 shield plus 45 body for Spartan and Elite multiplayer definitions;
the checked [material multipliers](../build/test-results/sword-material-data-20261025.json)
are 1. These establish the baseline lethal-damage expectation after ordinary
source contact/admission at full scale. They do not justify bypassing original
damage gates or assigning a native instant kill. Raw prepared input equivalence
between the launcher-era v3 and investigation v7 caches is recorded
[separately](../build/test-results/sword-cache-v3-v7-inputs-20261025.json).

[Authenticated trigger listings](../build/test-results/sword-trigger-source-20261025.json)
show `1889D0` deriving command bits6 and16 from the same nonzero current trigger
state and `1898A0` preserving those flags. Original `1521B0` then edge-filters
bit6 while retaining held bit16, before `1522E0` applies weapon, cooldown and
action admission. The input adapter must publish both original held bits and
leave that edge filter to its native owner. The probe includes one-tick
presses and six-tick holds through its declared input boundary.

## ONLINE-shaped acceptance and current evidence

The native online host runs the authoritative source world. Remote clients
send controls; host source phases own targeting, movement, contact and damage.
Native snapshots then carry completed vitality, death, attribution and contact
state. [World construction](../src/gameplay/multiplayer/multiplayer_world.cpp)
uses original simulation mode1, so the original mode4 Xbox event-submission
branch is not a second native online damage owner.

[The probe](../tools/capture_source_melee_contact.cpp) covers host and remote
attackers with actual source-created players, delivered movement/aim/fire and
normal `LanHost`/`LanClient` logical-endpoint transport. It checks authoritative
and received vitality/death/contact state. Deterministic endpoint delivery does
not establish DTLS, NAT, WAN latency or interactive online acceptance.
`HALO2_SWORD_LUNGE_TRACE=1` adds a bounded read-only clock/contact/dispatch trace;
it supplies no action, pose, collision or damage result.

| Evidence | Current result |
|---|---|
| [Isolated red report](../analysis/sword-lunge-tu15-20261025/symbols-red/report.json) / [log](../analysis/sword-lunge-tu15-20261025/symbols-red/tests.log) | Fails with `lunge impact timing EC230 did not select the authenticated MCC name: melee_lunge`. |
| [Isolated green report](../analysis/sword-lunge-tu15-20261025/symbols-green/report.json) / [log](../analysis/sword-lunge-tu15-20261025/symbols-green/tests.log) | Passes after the scoped correction. |
| [Whole-world pre-fix report](../analysis/sword-lunge-tu15-20261025/red-network-recovered/report.json) | Five lunge cases fail with no impact/contact/damage; both ordinary close-attack controls pass. |
| [Canonical whole-world corrected report](../analysis/sword-lunge-tu15-20261025/canonical-green/report.json) / [detailed evidence](../analysis/sword-lunge-tu15-20261025/canonical-green/evidence/Testing/Temporary/LastTest.log) | All seven pass; the complete original source engine owns factory, live round, contact, damage, death and kill statistics. |
| [Actual MCC timing metadata](../analysis/sword-lunge-tu15-20261025/canonical-metadata/report.json) | Passes for both species and ground/air lunge clips and impact events. |
| [Current-source namespace check](../analysis/sword-lunge-tu15-20261025/current-symbols-green/report.json) | Passes after concurrent unrelated namespace additions; exact-call negative controls remain covered. |
| [Initial combined native suite](../analysis/sword-lunge-tu15-20261025/final/report.json) | All 23 pass in 58.30 seconds: metadata, nine damage cases and thirteen delivered cancellation/switch cases. This precedes the continued input/first-person/protocol116 work. |
| [Earlier launcher-selected runtime](../analysis/sword-lunge-tu15-20261025/selected-runtime/report.json) | `36b3c2d115e548f8a7b420789fd4cc29` contains the corrected `EC230` machine code and aliases. Full runtime admission passes; fresh direct-native and installed-launcher Lockout runs each submit four frames and advance ten source ticks. This does not qualify delivery of subsequent corrections. |
| Interactive ONLINE sword kill | Not recorded by this bounded deterministic fixture. |

The original custom-spawn fixture selected the adapter rules path, where a
native fatal readback deliberately carries no attacker. Its lethal damage
with zero attacker credit was not treated as a native kill-counter defect.
The corrected fixture retains the map's complete authored start bank and
original factory, waits for the genuine live statistics phase, and places
the born participants through original `B95D0`. It requires raw and published
native stat rows to agree and requires the peer to receive the original kill
and death. No gameplay statistics implementation was changed.

Unrelated work continued in the checkout during this investigation. The
canonical damage fixture uses its authenticated frozen cohort with declared
source refreshes. [Eighteen complete relevant action, damage and physical
owners](../analysis/sword-lunge-tu15-20261025/relevant-owner-current-comparison.json)
are byte-identical as translated source between that cohort and the current
checkout. This is a bounded owner comparison, not a claim that the entire
checkout and the selected runtime are identical.

## Cancellation and flight scope

The user requires TU1.5's admitted cancellation and flight behavior, including
bugs that still exist in that version. The initial production correction changes only
the copied impact-timing query operand. It changes no cancellation, inventory,
physical-mode, velocity, input, target or recovery implementation.

[Authenticated source authority](../analysis/sword-lunge-tu15-20261025/cancel-fly-authority.md)
records the reload path `E9741 -> E8F41 -> EC770`, genuine switch path
`EB00C -> E8F41 -> EC770`, and subsequent physical owners
`1ED880 -> 1F39D0 -> 1F3B80` and `E4A40`.
[Thirty-six unchanged original cancellation cases](../analysis/sword-lunge-tu15-20261025/cancel-source-original-abi.json)
confirm that interruption clears the activity and generic active bit while
leaving mode, timer, velocity and every other unit byte unchanged. These raw
ABI cases establish that exact interrupt contract; later physical continuation
requires delivered-input and original-instruction comparison.

The [eleven canonical input cases](../analysis/sword-lunge-tu15-20261025/cancel-input/report.json)
pass: host/remote reload cancellation, cancellation followed by jump or
reattack, genuine Sword-to-SMG switch cancellation, genuine SMG fire-to-Sword
switch-to-dash-to-reload, and an Elite remote jump-cancel control. Inventory
comes from the original variant/factory, not an injected item. Each requires
the actual interrupt caller, exact activity/active-bit body changes, no old
impact, later physical observations and matching received movement, weapon,
action and statistics. Actual factory components can lie beyond the first
`0x700` bytes: snapshots cover through their genuine component, with a bounded
mapped read. Spartan snapshots are 5738 bytes; Elite snapshots are 8284 bytes.

[Seven directed live replay scenarios](../analysis/sword-lunge-tu15-20261025/live-movement/summary.json)
add genuine airborne admission and replay 52 actual native `1F39D0` calls
through the unchanged original `1F39D0/1F3B80/10F080/30BD0` closure. They cover
host reload cancellation, remote jump and switch cancellation, firearm/Sword
swap, Elite jump cancellation, remote airborne lethal lunge and airborne
host cancellation. Both airborne cases execute canonical `130005BC`; the
lethal case transitions to `140005B3` and dispatches its authored impact.
All six cancellation cohorts contain the same full unit datum, physical
component and parent before and at/after the observed cancel clock, and
include continuing movement followed by the original physical termination.

The [test-only capture wrapper](../tools/capture_sword_movement_tu15.cpp)
forwards the actual native kernel. The [offline replay](../tools/replay_sword_movement_tu15.py)
requires pinned original instruction bytes and exact captured read/write
coverage; it supplies no callee returns. Compared instruction paths, GP/SSE
outputs, defined arithmetic flags/DF, ordered stores, retained bytes, live
x87 values/depth/TOP, x87 control/exception bits and MXCSR all match. As the
[independent review](../analysis/sword-lunge-tu15-20261025/movement-review.md)
records, excluded x87 condition/status bits are not full FP/ABI equality:
24 calls differ in C1 alone. This is bounded movement-owner parity plus live
canonical behavior, not a complete original-world/framebuffer comparison.
No code clears momentum or holds mode6 beyond its source termination.

Four additional permanent registrations exercise delivered jump before host
and remote airborne damage and cancellation; a ground fallback cannot satisfy
their explicit airborne-activity gate. The complete combined regression result
is recorded in [the final suite](../analysis/sword-lunge-tu15-20261025/final/report.json).

The earlier vehicle-stored flight exploit cannot be made an unconditional
multiplayer requirement: [Bungie's original April 2005 update announcement](https://halo.bungie.org/bwu/index.html?item=21)
explicitly removed that variant from multiplayer while retaining Campaign
behavior. No older-version exploit is added to this multiplayer correction;
the pinned TU1.5 executable decides each admitted/rejected sequence. The
directed firearm/Sword case above uses an admitted Sword-range target and is
not evidence for a long-range rocket-lock exploit or Campaign vehicle storage.

## Continued rapid-input and ONLINE qualification

The native command adapter had emitted primary bit6 only when the weapon
selected before the tick was a Sword, while emitting bit16 for every held
primary. The original `1889D0` fragments independently derive both bits from
the same nonzero input byte, with no weapon read. The unchanged original
[fragment oracle](../analysis/sword-lunge-tu15-20261025/rapid-primary-fragments-original.json)
passes 17,664 held-byte/flag combinations across 52,992 fragment executions.
Normal `1898A0` copies preserve those flags; its separate whole-command
inhibition branch remains a distinct original gate.

The canonical [red case](../analysis/sword-lunge-tu15-20261025/archive-reuse/primary-intent-red-v2/report.json)
reproduces the mismatch with the genuinely selected SMG at tick 40: actual
retained command flags `10000`, expected `10040`. The correction in
[the native input boundary](../src/gameplay/multiplayer/multiplayer.cpp)
emits bits6/16 together for held primary and leaves the original weapon
predicates, cooldown and current/requested-selector gates to `1522E0`.
[All six directed green cases](../analysis/sword-lunge-tu15-20261025/archive-reuse/primary-intent-green/report.json)
pass: host/remote primary intent, ordinary firearm/Sword swap and lethal
lunges. This fixes a demonstrated input-contract mismatch; it is not by
itself proof of long-range flying.

The [real socket cohort](../analysis/sword-lunge-tu15-20261025/archive-reuse/online-socket-two-v3/report.json)
also passes both host and remote lunge damage, received death and kill
credit. It uses two owned `OnlineMatchSession` processes, actual local
signaling, reciprocal measured host election, ICE/DTLS/SCTP/UDP and zero
master gameplay-relay requests. The remote session/socket owner is an
executed child; the native client codec remains on the parent executor.
External account verification and the two-owner playlist are explicit local
fixtures. These are real same-host socket results, not WAN or NAT-traversal
proof.

These follow-up cases use a freshly authenticated, coherent protocol 113
reference cohort and only one declared copied archive-member input change.
The reference bank is unchanged and every temporary copy was removed.
[The delivery comparison](../analysis/sword-lunge-tu15-20261025/delivery-coherence-36b-vs-n4.json)
shows why that bank cannot be shipped as though it were the selected 36b
cohort. The [first current-root build command](../analysis/sword-lunge-tu15-20261025/delivery-current-root-command.json)
froze both production corrections and protocol115 under an owned build.
Later declared refreshes add the first-person repairs and protocol116 below;
launcher selection remains a separate qualification gate.

The additional [eight socket movement cases](../analysis/sword-lunge-tu15-20261025/archive-reuse/online-socket-movement/report.json)
pass host and remote reload cancellation, post-cancel jump, genuine switch
cancellation, and firearm-to-Sword cancellation. They require the same native
interrupt stores, no old impact, continuing physics and exact received state
as the directed logical cases. Together with the two damage cases above,
these establish ten directed real local ONLINE socket cases on the reference
cohort. The temporary 697,418,476-byte movement-test owner was removed;
reference inputs and selected runtimes are unchanged.

The initial rapid matrix retained twelve valid near results and twelve
fixture-baseline failures. The far SMG placement at10 incorrectly treated a
maximum range as a full-confidence range. Authenticated original `1AD460`
uses `46A0CC=0.5`, so the SMG primary distance reaches full confidence only
through6. [The corrected range evidence](../analysis/sword-lunge-tu15-20261025/rapid-range-confidence-control-source.json)
independently authenticates that literal, original instructions and actual
prepared weapon fields. A battle rifle has primary range17 and full-distance
limit8.5; the new far placement8.25 is beyond the Sword maximum8. The live
query must still reach confidence1 before any sequence buttons. Authored
starting choices are resolved by name and require one distinct resident
weapon datum. Multiple actual choices naming the same datum are aliases;
two distinct matching weapons remain an error. Only an unrelated unavailable
enumeration entry may be skipped.

Original `1521B0` edge-filters bit6 while preserving held bit16. A new finite
control switches at80, keeps RT released, and presses at110. Both near roles
already produce a genuine dash, lunge, lethal contact and received single
kill/death credit. Existing immediate/held sequences remain separately
recorded. The [timing analysis](../analysis/sword-lunge-tu15-20261025/rapid-trigger-timing-and-current115.md)
distinguishes raw delivered flags from the prior retained flags observed at
admission. No target confidence, selector, cooldown or activity is assigned
to obtain these outcomes.

The corrected [28-case rapid matrix](../analysis/sword-lunge-tu15-20261025/archive-reuse/rapid-twentyeight/summary.json)
passes both roles for seven finite sequences at near and far ranges. Twenty
cases produce no melee, six produce ordinary melee, and the two near
release/press controls produce lethal lunges. All fourteen far cases retain
at least95 living-target Sword query rejections. These are declared inputs
and states, not a claim that every possible colloquial flying sequence is
rejected. The [independent outcome review](../analysis/sword-lunge-tu15-20261025/rapid-outcome-assertion-review.md)
binds the edge, weapon-predicate and selector reasoning to original TU1.5.

The corresponding [eight actual socket rapid cases](../analysis/sword-lunge-tu15-20261025/archive-reuse/online-socket-rapid-eight/report.json)
pass X/Y/B/RT and the finite released-trigger control at both distances and
roles, including received contact, movement and stats. Their 697,422,877-byte
test owner was removed. Four initial rocket probes failed weapon-name
enumeration before world creation because two authored choices named the same
resident weapon. The distinct-datum correction preserves both actual choices
and uses their genuine factory weapon. Four subsequent rocket probes passed
in the [protocol115 matrix](../analysis/sword-lunge-tu15-20261025/archive-reuse/current115-vector-eightyfive/report.json),
with actual full firearm confidence and distance15.25; the complete matrix
was still red on three missing first-person dispatches. Those four passes do
not close the later protocol116 matrix. A diagnostic also correctly rejected
historical reference-bank drift before compiling; that bank is barred from
reuse under its old pins.

The first fresh delivery attempt compiled the engine and passed all eight
launcher/shadow CTests, but failed the subsequent wiring audit. It did not
install a runtime or run gameplay/front-end admission. Its owner was removed.
The [audit correction](../analysis/sword-lunge-tu15-20261025/delivery-wiring-correction.json)
adds canonical absolute path handling and retains exact object-library
ownership traversal; it does not exempt missing production sources. A second
owned candidate retains guarded publication identities captured before
compilation. Declared source refreshes and component checks do not establish
launcher publication: final same-cohort gameplay and engine/front-end
qualification remain open until that candidate passes.

Current-cohort qualification exposed additional first-person lifecycle faults
before the Sword checks could run. All13 actual map/player smokes rejected an
unconstructed look record at startup. [Original layout evidence](../analysis/sword-lunge-tu15-20261025/fp-look-layout-review.md)
confirms unit+0, manager+14 and stride94; the strict guard is correct. The
[spawn-boundary repair](../analysis/sword-lunge-tu15-20261025/fp-look-startup/diagnosis.md)
binds real locals before the original spawn tick, copies their actual native
record, and invokes unchanged18A0C0 only for missing remote birth records in
saved/restored logical-view scratch. All four Human/Elite local/remote birth
checks pass, including exact148-byte comparisons and first-input continuity.
No command delta, action, clock advance or look result is supplied.

The next preflight passed six cases but rejected five joined-player action
cases at the actual caller FP guard. [Live/native evidence](../analysis/sword-lunge-tu15-20261025/fp-action-abi/entry.json)
and the [independent original-call audit](../analysis/sword-lunge-tu15-20261025/remote-action-fp-caller-audit.json)
show valid CW027F/ESP600EFE34 and complete MXCSR1FA0. Its difference from1F80
is sticky inexact status0x20, with identical control bits. The correction
validates CW exactly and MXCSR controls with `~0x3F`, then forwards the actual
complete caller state through the root entry and nested native owners. It
does not mask or clear the incoming status. Missing FP, invalid stacks and
unsupported controls remain rejected.

The [real-context regression](../analysis/sword-lunge-tu15-20261025/fp-action-abi/diagnosis.md)
is red against the old guard for both species and green for1F80/1FA0/1FA1.
Each status case observes152 real nested CPU callbacks for Human and76 for
Elite, retaining the incoming status; caller and enclosing MXCSR remain
unchanged. Nine missing-state/control/stack negatives remain rejected.
This is a caller-handoff check, not a full x87 register/status oracle.

Live initialization and cancellation also exposed three missing routes in
[the first-person context](../src/gameplay/animation/source_first_person_context_tu15.cpp).
Each exact non-tail caller/target pair now forwards full ABI, FP, services and
stores to its existing compiled native body. There is no replacement math,
fabricated return or general callee allowlist.

| Exact route | Existing owner and focused original evidence |
|---|---|
| `17B262 -> 17A230` | Effect retirement: [ten cases](../analysis/sword-lunge-tu15-20261025/elite-fp-effect-retirement/original-proof.json),312 instructions,33 ordered stores, including a real nested16E8A0 call. |
| `18F2FE -> 18BBC0` | Vector packing: [132 cases](../analysis/sword-lunge-tu15-20261025/fp-vector-pack/original-proof.json),6168 instructions,792 ordered stores, eleven vectors across all four SSE roundings and incoming sticky statuses0/20/21. |
| `17A6A0 -> 16E8A0` | Effect-part datum release: [eighteen cases](../analysis/sword-lunge-tu15-20261025/fp-part-retirement/original-proof.json),1032 instructions,276 ordered stores, genuine pool salts/bitmap/list state and free-pattern flags0/8. |

The vector/part references execute unchanged pinned TU1.5 instructions on
i386 host hardware; they are not Xbox console captures. All three permanent
[dispatch regressions](../analysis/sword-lunge-tu15-20261025/fp-part-retirement/native/report.json)
pass, including wrong caller/target/tail rejection without memory mutation.
Their reference fixtures are27,716,420,793 and77,652 bytes respectively;
generated harnesses, raw banks and private builds were removed.

The Campaign input adapter contained the same Sword-only bit6 producer. It
now emits the original paired primary bits for every item, preserving the
original mode and cooldown gates. This uses the same mode-independent pinned
1889D0 fragment authority; it does not establish a Campaign vehicle-flight
runtime result or add its behavior to multiplayer.

## Protocol116 physical-state preservation and final qualification

The source readback now publishes the actual biped physical-state byte in
`Player.native_physical_state`, and protocol116 carries that byte through the
native snapshot codec. Unknown state remains `FF`; mode6 is read from the
real biped motion owner and body+`3DC`, rather than inferred from a melee
activity that cancellation has already cleared.

[The joined movement consumer](../src/gameplay/multiplayer/nonhost_movement_client.cpp)
admits ordinary prediction only for a live, unseated source unit in physical
state1. Receiving another state retires ordinary walking prediction and its
pending controls; returning to state1 starts from fresh authoritative motion.
Presentation also checks the physical state of the displayed buffered frame,
so a newer walking prediction cannot overwrite an older received mode6
endpoint. The [controlled red/green comparison](../analysis/sword-lunge-tu15-20261025/nonhost-preservation/summary.json)
holds the test and owned headers fixed and recompiles only that consumer.
It covers mode6 with known/unknown input receipts, mode2, unknownFF, unchanged
controls, ordinary reseeding and buffered mode6 after walking resumes.

The live helper constructs the actual `NonHostMovementClient` with prepared
movement/deadzone data and the real remote biped definition. It calls
predict before the existing input send, reconciles actual decoded states
and their input receipts, and calls real presentation at0,.5,1. It does not
adopt the preview report or change the authoritative world or delivered
controls. Assertions compare actual source, readback and received physical
bytes; nonwalking displayed position/velocity must preserve the received
bits. Remote attacker lunge/cancellation cases require actual received mode6
samples, and cancellation observes mode6 after activity becomesNONE. Warm
ordinary prediction must also run, so an inert preview cannot pass.

The combined protocol116 [focused run](../analysis/sword-lunge-tu15-20261025/delivery-current-root-v2/combined116-capture-green/report.json)
passes twelve checks: movement ownership, packet transport, both species'
caller ABI, four birth cases, all three exact leaf routes and timing metadata.
The first [85-case matrix](../analysis/sword-lunge-tu15-20261025/archive-reuse/current116-eightyfive/report.json)
finished30 pass/55 fail; all eighteen real socket cases passed. Every failure
was the strict final-tick receipt check in an unpaced logical endpoint, so
that matrix remains a failed result.

The [paired receipt investigation](../analysis/sword-lunge-tu15-20261025/logical-receive-rate/diagnosis.md)
observed both roles receive the129th complete-snapshot start within the
first real second, with expected/source tick128, received tick127, empty
queues, no buffered bytes and zero decoder rejections. This matches the
native receiver's128-new-assemblies-per-second bound. Repeating only snapshot
broadcast/receive at the unchanged source tick accepted tick128 when that
window ended; the helper never repeats an input or advances the source world
to obtain a receipt. A ten-second deadline and2ms IO yield retain exact tick
equality and print bounded timeout diagnostics. Both [paced lunge controls](../analysis/sword-lunge-tu15-20261025/logical-receive-rate/paced-two-v2/report.json)
pass with unchanged reference pins; the652,818,426-byte private owner was
removed. Production transport limits are unchanged.

The final [paced 91-case matrix](../analysis/sword-lunge-tu15-20261025/archive-reuse/current116-paced-ninetyone/summary.json)
passes all cases in 695.16 seconds. This includes 24 real socket cases, six
using the default `original` stream profile, and all seven original movement
comparisons (52 actual calls). Its 704,981 bytes of raw movement captures and
653,927,057-byte copied build/archive owner were removed; reference-after pins
and cleanup both pass. The earlier failed 85-case result is retained unchanged.

Full delivery exposed a separate double-normalization of already-native HUD
emblem inputs. The two existing working-tree fixes passed four focused checks;
[archive comparison](../analysis/sword-lunge-tu15-20261025/delivery-current-root-v2/hud-emblem-refresh/archive-member-comparison.json)
shows only `hud_content.cpp.o` and `objective_waypoints.cpp.o` changed, with all
2,189 other members identical to the 91-case cohort. The following launcher
qualification was cancelled by an interruption before completion. Its stale
running report is not acceptance, and the installed runtime was not changed.
That interrupted cohort was superseded by a complete fresh build, rather than
reusing its cancelled result. The [fresh source closure](../analysis/sword-lunge-tu15-20261025/delivery-current-root-v3/current-frozen-stable-closure.json)
pins 12,723 canonical source files, the build cache, compiler/link inputs and
native archive members. All twelve [focused checks](../analysis/sword-lunge-tu15-20261025/delivery-current-root-v3/focused-complete/report.json)
pass. The [qualified candidate](../analysis/sword-lunge-tu15-20261025/delivery-current-root-v3/qualification-3/report.json)
is `4b3177b2d1ca485683800f225421422d`, runtime SHA-256
`db2176f90c6281b9b1209a7232c0475827f5152d8d15ae34b12891ebda963bb6`.
All eight launcher/shadow checks, thirteen actual map/player smokes and four
frontend captures pass with frozen-source helper imports. The [fresh 91-case result](../analysis/sword-lunge-tu15-20261025/archive-reuse/current-v3-paced-ninetyone/summary.json)
passes in 3,231.08 seconds with no failures or missing cases. Original source
and archive pins remain equal, and its 657,754,375-byte private owner and
704,981 bytes of raw movement captures were removed.

[Guarded publication](../analysis/sword-lunge-tu15-20261025/delivery-current-root-v3/publication-result.json)
selected that exact `4b3177b2d1ca485683800f225421422d` build under the original
launcher/selection identities, with a complete transaction and unchanged
source inventory. The [post-publication check](../analysis/sword-lunge-tu15-20261025/delivery-current-root-v3/published-launcher-check.json)
passes full inventory admission and actual launcher-info routing. Its
four-frame Lockout launch reaches the selected ELF and initial native render
work, then times out at 180 seconds without a final submitted-frame counter.
The failed result and completed cleanup remain intact. Earlier direct-engine
qualification reused the owner's GPU caches; the failed check used fresh
caches and included launcher preflight inside its deadline. These are measured
setup differences, not yet a diagnosed cause or a passing startup result.

A later [concurrent publication](../analysis/sword-lunge-tu15-20261025/newer-9d-selection-provenance.json)
atomically selected `9d531caa3b6a4b0c9284b954c3874139`, native SHA-256
`ea2a889433821435231f4d504099e5b50b1f0d232f617a3c437c5c1cf38a05b5`.
The prepared diagnostic pair stopped at its hash guard before launching.
This newer selection is preserved. Its core Sword timing, primary input,
nonhost movement and first-person helper hashes match `4b`, but 45 other
source paths differ, including physics/world integration and rendering.
Consequently the 91-case result remains evidence for the sealed `4b` cohort;
it is not recast as a `9d` runtime result. Both products omit the later AI
owner additions described below.

The subsequent [cold `9d` startup pair](../analysis/sword-lunge-tu15-20261025/current9d-startup-pair-summary.json)
separates native execution from launcher startup. Direct execution completes
four frames and thirteen source ticks in 92.17 seconds with empty initial
GPU caches. The installed launcher reaches its 180-second deadline during
readiness checks, before executing the native engine. Its extra legacy-test
and frontend probes run even for an explicitly selected multiplayer route.
A narrow launcher correction skips those unused probes while retaining
selected-runtime admission and argument validation. Routing validation also
reproduced an existing invalid-selection fallback in both old and candidate
launchers; the corrected guard rejects that explicit invalid selection.
The original failed startup receipts remain unchanged. Candidate routing and
cold native validation are separate from engine or Sword parity qualification.
The [corrected uninstalled launcher](../analysis/sword-lunge-tu15-20261025/launcher-preflight-owned-validation-03/report.json)
passes all routing suites and the same cold 180-second total control. It
executes the actual selected `9d` ELF after 73.29 seconds and finishes four
frames/thirteen ticks with exit0 after 159.47 seconds. All 161 launcher and
selection identity checks remain unchanged, and its private build/caches
were removed. This is a passing candidate check; it does not itself install
the corrected launcher or alter either engine selection.

Remaining scope limits are explicit:

- Immediate joined-client native Sword prediction is not implemented; this
  change preserves the authoritative trajectory after its receipt.
- Original commands carry target and confidence fields produced from the
  preceding retained look/zoom and camera context. Current remote controls
  instead cause an authority-side target query. The [authenticated route](../analysis/sword-lunge-tu15-20261025/command-target-route.md)
  and [native producer proposal](../analysis/sword-lunge-tu15-20261025/target-producer-native-feasibility/proposal.md)
  identify this missing ownership. Directed rapid-input negatives therefore
  do not prove every original joined-client range-inheritance variant absent.
- Real ONLINE socket evidence uses owned peers on one host with direct
  ICE/DTLS/SCTP/UDP and local admission/signaling fixtures. WAN, NAT traversal,
  network impairment and interactive public matchmaking remain unqualified.
- Campaign vehicle-stored flight is under separate investigation. The [real
  Ghost entry/exit control](../analysis/sword-lunge-tu15-20261025/campaign-controls-review.md)
  passes, including action29 and parent/seat transitions. Complete native
  `20BBC0` now has 522 passing unchanged-original i386 comparisons and the
  actual AI control advances beyond it. The latest [three Campaign controls](../analysis/sword-lunge-tu15-20261025/fresh-campaign-three-ai-death-observer-v2/execution/diagnostic-summary.json)
  all pass on the separately refreshed bank: authored AI lunge/contact/damage
  and actual native death cleanup, independent native-biped damage, and
  ordinary Ghost entry/exit. Earlier missing-owner and diagnostic failures
  remain preserved. No actor, target, confidence or death result is supplied.
  No positive flight result is inferred from the seat control. Multiplayer
  does not acquire an older exploit.
- Original Xbox/NV2A framebuffer and complete original-world parity remain
  unqualified by these native/kernel and host-hardware comparisons.

The subsequent [AI helper comparison](../analysis/sword-lunge-tu15-20261025/campaign-actor-mask-native-proof-attempt2/report.json)
passes all 1,536 isolated cases against the unchanged 88-byte original body
on host i386 hardware, including all 256 shift inputs, precedence branches,
complete flags/FP state and ordered stack stores. For `1B06C0`, the change only
exports the already-present native body to its existing World route; it does
not replace the algorithm or create an actor. This delta is newer than the
qualified online candidate. The [biped diagnostic](../analysis/sword-lunge-tu15-20261025/campaign-biped-target-diagnostics-v1/diagnostic-summary.json)
also identifies a fixture birth-look offset: all sixty frames retain a
12.25-degree pitch error, while the native query finds the genuine hostile
target with zero confidence. A genuine neutral birth-look command removes
that fixture bias before later target-directed commands; the corrected biped
control observes six qualified target samples, nine mode6 samples, one native
contact and vitality loss. The earlier three-control run retains its authored-AI
failure; the refreshed run above completes it. The [stored-lunge witness audit](../analysis/sword-lunge-tu15-20261025/campaign-stored-lunge-provenance.md)
requires seated acceptance and native physical initialization before exit,
same-unit retained state across exit, and detection of any fresh post-exit
attack. A post-exit action27/mode6 sample alone cannot prove stored flight.

The [separate native targeting component](../analysis/sword-lunge-tu15-20261025/local-target-stage-component-v2/execution/summary.json)
executes the complete 337-instruction producer from an actual preceding
private camera/input capability. Its on-foot native-biped case passes lunge,
contact and damage. Both Ghost cases fail seated target admission before
exit; the [single-variable neutral-prime follow-up](../analysis/sword-lunge-tu15-20261025/local-target-stage-component-v3/execution/summary.json)
also remains failed. It verifies the birth prime, then observes seat
reorientation change the retained view while the adapter's prior raw-angle
baseline stays unchanged. No target, confidence or movement result replaces
these failures. The [raw-angle conversion follow-up](../analysis/sword-lunge-tu15-20261025/local-target-stage-component-v4-fresh/execution/gameplay/report.json)
correctly tracks the requested native look but still admits no seated Ghost
target. The [original equipment and seat audit](../analysis/sword-lunge-tu15-20261025/vehicle-equipment-lifecycle-source-contract.md)
establishes that generic vehicle birth creates authored equipment, and the
Ghost driver normally holsters handheld weapons. Prepared Delta data includes
an authored Ghost cannon and a Warthog passenger seat allowing handheld
weapons; those static prerequisites do not prove a lunge request or flight.
The [read-only Ghost query diagnostic](../analysis/sword-lunge-tu15-20261025/local-target-stage-component-v5-ghost-query-diagnostic/execution/diagnostic-summary.json)
confirms factory-created cannon equipment and native parameter admission.
Eight pre-entry queries use the biped and nine seated queries use the actual
controlled Ghost; the seated RT query uses its cannon. The later spatial
query rejects the target. The exact candidate/root/visibility cause remains
unresolved, with all failed inputs, history and cleanup receipts preserved.
The separate [ordinary Warthog passenger control](../analysis/sword-lunge-tu15-20261025/warthog-passenger-ordinary-control/execution/diagnostic-summary.json)
passes through the existing production composed-query path. Normal mapped X
input and actual `153BA0` prompts enter authored seat1; real `CDBB0` retains
the biped, and native equipment retains the selected/requested Sword. Actual
action29 and parent/seat stores complete exit. All 73 commands release RT,
with zero action27/mode6 stores, so this establishes the passenger setup
without claiming stored-lunge acceptance. The case preserves the reference
bank and removes both private execution owners.
Neither stored-flight nor full original collector/client parity is qualified.

The reached AI death owner is now lowered as five complete original bodies
in the existing native AI module. The [source and leaf proof](../analysis/sword-lunge-tu15-20261025/campaign-actor-death-native-proof.md)
preserves all 686 earlier bodies/helpers, passes the 691-body/12-partition
guard and strict syntax, and matches 333 physical i386 cases for its two
call-free leaves. Actual authored-AI execution now returns normally through
`1EB1FD -> CD057` with original `RET4` stack movement, including the nested
`1E9CB0` cleanup's `RET8`. Original `1E9FAA` already unlinks the unit/actor,
so caller `CD057` correctly stores `NONE` over `NONE`; an added diagnostic
incorrectly required a non-NONE preimage, and its failed receipt is retained.
The corrected read-only check passes all three controls with reference-after
and cleanup verified. The actual catalog selects a null callback, so
`1E9D13` is unexecuted and no callback result is inferred. These later AI
changes are in neither published `4b` nor the newer selected `9d` build.

Focused reproducible command, from the repository root:

```sh
python3 tools/run_tests.py --target source_weapon_animation_query_symbols_tu15_tests \
  --filter '^source_weapon_animation_query_symbols_tu15$' --jobs 2 --test-jobs 1 \
  --output analysis/sword-lunge-tu15-20261025/symbols-green
```

Full-world regression command using the prepared investigation cache:

```sh
python3 tools/run_tests.py \
  --target capture_source_melee_contact --target mcc_first_person_binding_tests \
  --filter '^native_mcc_sword_' \
  --jobs 2 --test-jobs 1 \
  --cmake-arg='-DCMAKE_CXX_FLAGS_RELEASE=-O2 -DNDEBUG' \
  --cmake-arg=-DHALO2_STAGE_BUILD_OUTPUT=OFF \
  --cmake-arg=-DHALO2_BUNDLE_MOVIES=OFF \
  --cmake-arg=-DHALO2_MCC_SWORD_LUNGE_MAP=/home/eric/Developer/Halo-2-Decompiled/assets/imported/mcc-sky-water-tu15-20261025-v7/halo2/h2_maps_win64_dx11/lockout.map \
  --retain-report Testing/Temporary/LastTest.log \
  --output analysis/sword-lunge-tu15-20261025/final
```

The completed incremental investigation used the official `--build-worker` inside
the enclosing `tools/run_test.py` owner's source/build tree. Its source freezes
and refresh hashes are retained in the [session directory](../analysis/sword-lunge-tu15-20261025/).
The two isolated test owners `halo2-test-oftisk6b` and `halo2-test-iq4a413i` were
verified removed. [Final cleanup](../analysis/sword-lunge-tu15-20261025/cleanup.json)
also verified removal of the earlier recovered `halo2-test-olbts3vj` source/build bank,
outer `halo2-test-w26tht7x` supervisor and current-component `halo2-test-86yjoxbk`
owner. The recovered bank contained 2,397,168,423 bytes before cleanup, including
all 704,981 bytes of raw movement captures. That earlier cleanup retained no
new large artifact.
Existing assets, reference evidence and launcher-selected runtimes are preserved.
For the fresh continuation, the qualified `4b` product occupies 3,153,147,826
bytes and is preserved alongside the newer selected `9d` product.
The explicitly required immutable source reference at
`.local/reference-inputs/sword-tu15-20261026-78076bf9/source` retains
373,081,847 regular source bytes and no native object/archive/capture copies;
its [retention receipt](../analysis/sword-lunge-tu15-20261025/delivery-current-root-v3/source-retention-receipt.json)
pins the exact inventory. The supervised `r57rauh0` source/build owner remains
active for the declared two-file AI refresh and further native controls;
its final removal is still required when that work ends.
The required read-only AI delta at
`.local/reference-inputs/sword-tu15-20261026-ai-664ac1d6` adds only the two
refreshed source files (8,637,384 bytes), with
[projection and cleanup evidence](../analysis/sword-lunge-tu15-20261025/delivery-current-root-v3/ai-refresh/source-delta-retention.json).
It retains no object, archive, executable or raw capture.
