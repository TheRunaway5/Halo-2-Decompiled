# Campaign source world

Active combined scope and acceptance gates:
[campaign AI, scripting, navigation and co-op checklist](CAMPAIGN_AI_COOP_GOAL_CHECKLIST.md).

Campaign host script queries now borrow the actual source options/player bank.
`game_is_cooperative` uses TU1.5's campaign-type/cooperative-byte predicate.
`players` allocates a fresh native HS list, preserves full current-unit datums,
skips NONE, and retains original prepend order, allocation and checkpoint/GC
semantics. The component matches25 predicate and 161 complete original list cases;
normal/root and sanitizer tests pass and the Simulation caller compiles. These
queries do not construct a cooperative session or qualify live mission execution.

The October 20 engine increment uses MCC tags as converted TU1.5 inputs. In a
private coherent MCC client, authored Johnson birth advances four source ticks.
Actual Grunt squad `1st_cov_wv1_grt` exposes missing `1E8BBF -> 20B980`;
`source_campaign_ai_placement_settings_tu15` now owns that complete settings
selector and its inherited-character lookup `1ECC80`, with the original
read-only switch. All 272 independently executed original cases match complete
GPR/SIMD state, defined flags, instruction paths and ordered writes under normal
and ASan/UBSan checks. Five absent-owner controls fail at the actual source read.
The composed client places three actual Grunts and advances four ticks with
zero script faults and a checkpoint. A longer run stops after tick 10 at
`1D0670 -> 29ACAF -> 29AFD0`; 60-tick NPC continuation remains unverified.
No selected runtime, tag conversion or gameplay outcome is replaced. Detailed
commands, pins and remaining gates are in the active checklist linked above.

The next increment adds whole original AI condition/request owners (453
instructions), reusing existing frame and iterator services. All 330 component
cases match general/SIMD registers, defined flags, instruction order and ordered
stores under normal and ASan/UBSan checks. Of these, 307 complete and 23 compare
only the actual safety/multiplayer dependency-entry prefix; no service return
is supplied. Three instructions beyond those boundaries remain unverified.
The first original fixture is retained byte-for-byte alongside the extension.

The original request path now advances the actual three-Grunt MCC run through
tick 30. The immutable data owner also publishes the pinned AI frame's 818
fixed readonly reads (724 bytes), preserving all writable/indexed/tag/clock
owners. Its ownership/conflict tests pass normal and sanitizer checks. The
next actual frame stopped at writable destination `1B07AC -> 56AE84`.

The bounded working-mask and decision-gathering destinations are now published
through their original cold-data owner: five exact extents, 9,607 bytes, all
authenticated loader-zero storage. The production frame matches 20 original
mask cases and 18 gathering prefixes, including poisoned prior contents,
ordered stores, packed padding and whole-bank restoration under normal and
ASan/UBSan checks. Whole selection and
its later callbacks remain separate from those prefixes. The existing
descriptor metadata owner also supplies six missing original sentinel bytes;
35 compiled-frame reads define the exact ten-byte shared extent. The latest
actual three-Grunt run passes those reads and next stops at descriptor child
metadata `1B00A6 -> 47BE0C`, retaining zero script faults through tick 30.
The next metadata increment now supplies all 105 ordinary child rows' actual
ID/flag/timer/seconds fields (1,050 bytes) and the complete 20-byte command
switch, preserving unread row gaps and existing owners. The existing Actor
module matches 96 independent constructor cases, exercising all 65 original
instructions: 95 complete and one retains the original empty/null-table failure.
Together with mask/gathering checks, all 134 normal and ASan/UBSan comparisons
pass in one frozen current-source cohort. The coherent MCC client passes the ordinary
child-field failure and now reaches property-inserted child metadata
`1B02AF -> 47D3CC`. Whole `1AF970` and all authored property lists now match
17 original/native cases, including every pointer/header store and preserved
padding, under normal and ASan/UBSan checks. The 15 property-command rows'
150 selector bytes pass actual MCC reads. The next 49 bounded switch tables
(1,836 immutable bytes) also pass their original extent/target audit, native
ownership controls and reached MCC reads. The actual next failure is the
unproduced scalar `2A144E -> 56E08C`. Its original three-callback `_cinit`
family now matches all 11 instructions across 36 independent original/native
cases under normal and ASan/UBSan checks. The coherent MCC client now passes
those producers, reaches tick 31 with three actors and zero script faults,
and next stops at kind2 callback metadata `1B0EFD -> 48A82C`. That entire
55-record callback field family now passes 230 original/native selector cases
(155 whole, 75 callback-boundary prefixes). Nine authored object-tracking
rows supply 72 consumed WORD bytes, with 54 whole predicate and 18 constructor
prefix comparisons. Four predicate/handler rows supply another 32 callback
bytes. All 302 cases pass normal and ASan/UBSan checks in one immutable source
cohort; callback behavior beyond those boundaries remains separate.
The composed MCC run now passes those reads and reaches tick 34, three actors,
zero script faults and a checkpoint, then requires `26E60E -> 26EFA0`.
Whole `26EFA0`, `26EEF0`, `265620` and `2753C0` now compile all 298 original
instructions and match 244 independent original/native cases. The existing
character/RNG/HS owners retain their real ABI. Twenty whole scratch-lease
acquire/release calls plus two lock prefixes qualify the reached 29-byte
storage declaration; no completed lease pointer is supplied. The next eleven
object-callback masks/identities/terminator add 70 readonly bytes and 36
original/native selector prefixes. All 302 cases pass normal and ASan/UBSan
checks in one immutable source cohort. The actual MCC client now reaches
tick45 and stops on original context write `26ACAF -> 52D874`. Its WORD and
the following DWORD destination, and fourteen missing bodies in the two
complete callback families, remain open in the active checklist.
The 60-tick test remains failing, and full command/HS scheduling, navigation,
combat, mission and cooperative session acceptance remain open.

`PhysicsLiveWorldTu15` owns one retained world: tags, objects, actors, command
scripts, HS threads, physics, clock and their resource owners share that bank.
Campaign integration must borrow this world rather than allocate a second actor
or object authority.

Initialize the map, then call `configure_campaign_session` with the actual
difficulty, simulation mode, tick rate and random seed. The world invokes the
original HS storage and map lifecycle before AI initialization. Player creation,
machine identity, command delivery and view selection remain explicit native
inputs; initializing a world does not fabricate them.

Use `place_ai(reference)` for authored placement or `place_ai(reference, count)`
for the signed-short overload. References are the original compiled HS AI values,
including packed group and starting-location identities. The original actions
own difficulty counts, authored starts, object creation and actor/script binding.

The overloads intentionally differ. The default overload delegates every value,
including NONE, to the original reference action. The counted overload skips
NONE; it places only plain references with a positive count, then records every
non-NONE pending reference, including packed references and nonpositive counts.
An unavailable reached owner throws before later writes. Callers must not
normalize these distinctions or interpret a pending reference as actor creation.

`CampaignSourceRuntime::place_ai` forwards those same compiled references and
optional signed-short counts, then republishes the existing source world to
Simulation. Simulation admits both original `ai_place` signatures before its
older squad-only host path. It does not strip packed bits, filter NONE, or
replace a nonpositive count with an authored default.

The 2026-10-15 bridge compiles against a coherent captured engine cohort and
the regenerated 256-case original wrapper fixture matches the existing bytes.
The campaign goal has since resumed. Immutable interaction-clip sharing and
earlier release of the copied tag snapshot reduce setup retention. The bounded
Cairo probe now completes source-world and Simulation construction, including
actual player and biped birth. Five noncreating `ai_place` overloads pass actual
source ticks. Counted actor creation and equipment adoption now also pass in
the coherent engine build. The original E558D -> 2039B0 leaf reached by authored
packed starting-location placement is now recovered and passes 108 complete
original/native component comparisons. Its coherent rebuild passes. The next
query fixture reaches missing original callback 1B0195 -> 1C9C90 after its NONE
getters and five noncreating placements pass; it stops before non-NONE getter
validation. The authenticated callback is now in the canonical module and passes
432 original/native component comparisons under the serial memory guard. The
next integration proceeds beyond it and reaches missing original selector
1B0128 -> 1BFE00 before counted-birth completion or non-NONE getter assertions.
The current 256-case overload
differential and default campaign regressions pass; full bridge runtime
validation and campaign gameplay remain unverified. The
[implementation handoff](../analysis/tags/campaign-ai-bridge-20261015/RESULTS.md)
retains the original pins and failures; the
[memory continuation](../analysis/tags/campaign-memory-20261015/RESULTS.md)
records the newer checks without treating asset parity as mission completion.

`source_ai_placement` is the small internal overload seam. The world supplies
its actual actions and pending-reference field; differential tests compare this
same seam with unmodified original wrapper execution. The generated
`source_campaign_runtime_tu15` module retains 55 complete original routine
bodies. Shared AI-frame helpers keep their existing owner. Regenerate with
`tools/generate_campaign_runtime_tu15.py`; the generator rejects duplicate owners
and authenticates the original executable before lowering instructions.

Checkpoint publication replaces the bank's backing. Borrowed `DatumPool` views
must stage replacement bindings with `prepare_source_view`, then commit after
`commit_authority`. Do not keep old actor, command, thread or player spans across
publication. The complete authority transaction retains actual script links;
reconstructing equivalent-looking actor records is not restoration.

Validation is incremental. The public Cairo fixtures pass counted, authored and
packed starting-location placement, actual actor/command/thread binding, whole
authority restoration, and bounded original AI/object ticks. The counted fixture
passes three later ticks. The controlled original overload comparison covers 256
cases. These checks do not establish every campaign mission, frontend integration,
audio, rendering or Xbox parity. The full campaign goal remains open until the
actual mission, BSP and checkpoint paths pass.

The subsequent [physical request increment](CAMPAIGN_PHYSICAL_REQUESTS_20261015.md)
reuses the original allocation/protection wrappers for unrestricted SDK requests
and the reached write-combined pages. Native page ownership survives startup
worker retirement. The complete original physical-stage comparison now streams
its expected arena bytes and passes under the serial simulation memory cap.
This increment does not complete renderer startup or the campaign HS frame.

Current campaign AI continuation has qualified the two original callback
context destinations, both complete object-callback families and their missing
direct-call closure, the reached numeric operands, and the registered identity
query. The latest948-case immutable component cohort passes normal and
ASan/UBSan checks. Its actual MCC three-actor run reaches tick48 with zero
script faults and an initial checkpoint before requiring the original
`272100` matrix at `272164 -> 4568F6`. The matrix's original producer and
whole 61-instruction selector now have964 independent reference calls;
native/composed qualification follows. Original helper coverage, the 60-tick
gate, live combat, mission progression and real co-op remain separate open
requirements in the [goal checklist](CAMPAIGN_AI_COOP_GOAL_CHECKLIST.md).

The subsequent selector/actor-state/query qualification now passes 2,366
original/native cases in one immutable normal/ASan/UBSan source cohort and
the main CMake target. All 43 `1FB3C0` sites execute in its 168 whole original
cases with real nested Frame owners. The registered bucket query uses its
existing whole Frame body; its runtime route now reaches that owner and 286
whole original cases cover every bucket. The actual authored three-Grunt Cairo
encounter completes300 source ticks with zero script faults and its initial
checkpoint. This is bounded engine integration; complete mission progression,
visible combat/navigation, actual checkpoint replay and real co-op remain open.

The current query increment adds a 21-entry source module covering 6,101
distinct original instruction sites behind the existing birth/object-update
selectors. All 65 static targets of the eleven-selector group now have bodies;
unexecuted callback/SIMD/estimate paths remain explicitly unqualified. The
combined 6,194-case original/native target passes normal, ASan/UBSan and the
main CMake build. The actual authored Marine case now creates three actors
and its initial checkpoint, reaching 34 ticks before `1B0128 -> 1B7240` in AI.
The Elite control still passes 300 ticks; Johnson's later biped update needs
`E57EE -> 1EA360`. This does not create a cooperative session or qualify a full
mission. The 15-map scenario inventory records the full authored squad/script
set and declared BSP count, keeping metadata coverage separate from runtime
admission and behavior. See the [active goal checklist](CAMPAIGN_AI_COOP_GOAL_CHECKLIST.md).

The actor-behavior/scalar module now preserves18 original bodies /1,680
instructions and reuses existing Frame/physics owners. It adds72 authenticated
read-only operand bytes; the original262590 actor/script publication also has
its separate eight-byte loader-zero mutable context. The combined7,026-case
original/native target passes normal, ASan/UBSan and the main CMake build.
Actual Marines now pass1B7240 and next require1B7000 at tick34. Johnson passes
1EA360 and reaches the real script callback at tick30, where211C0D needs the
table based at480C00. The context is produced by original source writes;
neither actor/script IDs nor callback success are supplied. The Grunt and
Elite300-tick controls pass. Full mission, navigation/combat, checkpoint replay
and a cooperative session remain open.

The original behavior/command/property recovery now totals76 whole bodies /
5,144 distinct sites and188 authenticated read-only operand bytes. A separate
70-byte original table supplies allied-team difficulty remapping; the BSP
audibility cache adds its64-byte loader-zero mutable buffer with its original
producer, separate key and constructor. The
complete948-entry HS registry preserves its3,792 authored mutable pointer
bytes and18,892 read-only definition/count/usage bytes, checking existing
shared storage before admitting missing ranges. All13,695 combined original/
native cases pass normal, ASan/UBSan and the main CMake target. Registry cases preserve actual
evaluator-call boundaries, while the additional property helper executes every
instruction across192 whole original calls. The coherent C55 client now passes
300 ticks separately for authored Johnson, Grunts and an Elite. Marines reach
51 ticks after105F70, cache construction, difficulty selection, camera-effect
requests, fixed/indexed damage operand reads and the whole274E30 property-owner
lookup and274450/274490 creation, before21942E requires505E88 under17EAD0. The
original14-byte writable camera type table and128 fixed read-only damage bytes
have their exact owners. The343 whole difficulty cases
exercise every instruction in1F19D0/1F1950/C1E90; the105 whole cache cases
exercise every instruction of203C00/252F30/253070 with actual dependencies.
The204 whole camera-request cases exercise every1578A0 instruction, with full
MXCSR compared against separate unchanged whole host-i386 execution;98 emulator
sticky-status omissions are retained explicitly. Original CBC50/CBB90 bounds
now establish96 immutable bytes in the three indexed damage tables and16
authored name-ID bytes. The446 new reaction cases retain34 actual CALL
prefixes and explicit unexecuted sites. Shared read-only/writable owners keep
their original access and lifetimes; conflicting metadata is rejected before
missing names are published. See the [reaction reference guide](../tests/fixtures/campaign_damage_reaction_tu15.md).
These are component
and bounded MCC checks; physical Xbox CPU and visual acceptance remain open.
The additional720 whole property-owner comparisons cover every274E30
instruction, preserving direct/related full-handle checks and list traversal.
The object-decision module has six bodies /397 sites, with all five earlier
bodies and20 immutable bytes unchanged. Its [reference guide](../tests/fixtures/campaign_ai_prop_owner_lookup_tu15.md)
records the next creation-family boundary and scoped validation.
The576 subsequent whole creation calls cover both recovered bodies'50 sites
with real Frame classification/allocation/init services. The module now has
eight bodies /447 sites with the same20 immutable bytes. Exact mutable storage
adds one latch byte, four epoch bytes and2,048 DWORD object stamps; shared live
owners retain their values/access/backing and checkpoints preserve the fields.
The corrected fixture uses consistent live counts and the earlier reference
stays diagnostic/input-only. The [creation guide](../tests/fixtures/campaign_ai_prop_creation_tu15.md)
records the source evidence, reference correction and new parameter boundary.
That boundary is from the older C55 client: current source already has the
contrail scalar/noise storage and original constructor/new-map calls. The
[current-source audit](../analysis/campaign-ai-coop-20261020/contrail-init/current-versus-c55-v1.json)
records the difference. The complete current-source v1 executable builds and
passes original activation plus three300-tick actor controls; Marines reach63
ticks before missing1B7D20. Original session-order evidence exposed noise
initialization before seed admission. Existing original initialization now runs
in new-map order after admission; independent two-seed original outputs are
retained. Post-fix full-world regressions and four actor probes are building in
[immutable v3](../analysis/campaign-ai-coop-20261020/current-source-v3/current-source-cohort.json).
Neither a complete mission nor visible
navigation/combat, checkpoint replay or real co-op is established by these
bounded runs.

The current-source1B7D20 boundary now has its complete14-body static missing
closure, preserving earlier bodies and read-only operands. All13,973 combined
component cases pass normal and ASan/UBSan;278 new original calls/prefixes and
the generated source/data repeat exactly. These qualify component execution
and storage, with explicit planning/collision/placement service boundaries.
The separate v4 full client is still building actual authored encounter
acceptance. [Component proof and cleanup](../analysis/campaign-ai-coop-20261020/actor-current/component-proof-v1.json),
[Original scope](../tests/fixtures/campaign_ai_decision_workspace_tu15.md),
[Current complete-client qualification](../analysis/campaign-ai-coop-20261020/current-source-v4/current-source-cohort.json).

V3 completes with original activation, both admitted-seed regressions and
three300-tick actor controls passing. Marines reach125 ticks before the known
missing1B7D20 owner, so the batch remains incomplete. Its snapshot is unchanged
and its owned source/builds are removed. V4 is still compiling the new actor
closure. [Completed native seed/actor evidence](../analysis/campaign-ai-coop-20261020/contrail-init/native-two-seeds-v3.json).

V4 builds and passes the13,973-case main component target, original activation,
both native seed checks and three300-tick controls. Marines now enter1B7D20,
then reach missing read-only44ACD2 in the existing219030 geometry owner. The
original projection/inverse family is two six-row signed-short tables,72 bytes.
All1,152 new original calls execute both complete17-instruction bodies; native
component and full v5 composition are pending. V4 owned storage is removed.
[Full-client proof](../analysis/campaign-ai-coop-20261020/actor-current/full-client-proof-v4.json),
[Original geometry reference](../analysis/campaign-ai-coop-20261020/decision-geometry/original-v1.json).

The projection increment passes all15,125 combined native component cases
normal and ASan/UBSan in an unchanged snapshot. Both original17-instruction
geometry bodies and the72-byte table include repeat exactly. The concurrent
callable-data checkout edit is recorded; owned component source/builds are
removed. V5 still builds the full client. Its validation extractor has one
explicitly declared address-mapper correction, with unchanged engine inputs;
the final snapshot guard and effective input audit must remain visible.
[Component proof](../analysis/campaign-ai-coop-20261020/decision-geometry/component-proof-v2.json),
[Original reference and validation scope](../tests/fixtures/campaign_geometry_projection_tu15.md).

V5 exposes a composed-startup regression: adding the forward table in final
startup duplicates its existing native-world provider. Original activation and
isolated15,125-case tests pass, but both native seed tests and all four actor
cases fail during construction. The declared test-helper refresh is audited
without overriding those failures; v5 source/builds are removed. The publisher
now reuses the existing36-byte forward table and adds only36 inverse bytes.
The component test composes both exact production providers to catch overlap;
corrected normal checks pass, with ASan/UBSan and full v6 validation running.
V6 also runs the complete cold-world ownership test. The original1,152-case
reference is unchanged.
[V5 audit](../analysis/campaign-ai-coop-20261020/current-source-v5/declared-refresh-and-failure-audit.json),
[Corrected reference/data](../analysis/campaign-ai-coop-20261020/decision-geometry/original-reused-forward-v2.json),
[V6 validation](../analysis/campaign-ai-coop-20261020/current-source-v6/current-source-cohort.json).

Corrected component-v4 provider composition now passes all15,125 cases
normal and ASan/UBSan in an unchanged snapshot and captured checkout. The
existing forward provider and both geometry bodies are preserved; only36
inverse bytes are added. Original references and the corrected include repeat
exactly. Owned component source/builds are removed. Full v6 runtime and
cold-world ownership checks remain in progress.
[Corrected proof and cleanup](../analysis/campaign-ai-coop-20261020/decision-geometry/component-proof-corrected-v4.json).

The unchanged complete v6 client now passes cold-world ownership, original
activation, the15,125-case main component target, both seed regressions and
three300-tick actor controls. Marines reach199 ticks with three actors, zero
script faults and an initial checkpoint before1AFE1B, in original1AFDE0,
stores into missing mutable child-row link-head47BE14. All probes use prepared
MCC input without original-reference opens or cache writes. Later checkout
drift is recorded; owned v6 source/builds are removed. The original forward
provider is preserved and only36 inverse bytes are new. Command-list links,
complete decisions/navigation/combat, mission replay and co-op remain open.
[Full v6 proof and cleanup](../analysis/campaign-ai-coop-20261020/decision-geometry/full-native-proof-v6.json).

The next engine increment recovers120 authored child/command-row list heads
(480 exact writable bytes). The unchanged original1AFDE0 clears and constructs
the lists;722 complete original/native calls agree on register/flag state,
instruction order, ordered stores and checkpoint restoration. All15,847
combined component cases pass normally and with ASan/UBSan in an unchanged
snapshot; owned component source/builds are removed. Authored headers/timers
stay immutable, existing mutable backing survives publication, and unread row
gaps remain unowned. The complete v7 client is building to continue the actual
199-tick Marine path. This does not qualify complete combat or mission/co-op
behavior. The required original fixture is2,596,841 bytes; no build product or
raw bank is retained.
[Original/native component proof](../analysis/campaign-ai-coop-20261020/behavior-links/component-proof-v1.json),
[Original reference guide](../tests/fixtures/campaign_ai_child_links_tu15.md).

The complete v7 client now passes cold-world ownership, original activation,
all15,847 combined component cases, both seed regressions and the three300-tick
actor controls. The three-Marines case passes the former199-tick head-store
boundary and reaches283 ticks with zero script faults and an initial checkpoint.
It then stops at1B8938 requiring whole200C00. Snapshot is unchanged; later
checkout drift is recorded, all probes remain MCC-only with no cache writes,
and owned source/builds are removed. Complete path/combat/mission/co-op and
live checkpoint replay remain open.
[Full v7 proof and cleanup](../analysis/campaign-ai-coop-20261020/behavior-links/full-native-proof-v7.json).

The current actor module now adds whole200C00 without changing its90 earlier
bodies or212 immutable bytes:91 bodies /6,964 original instruction sites.
Independent original execution repeats96 whole finite world-space distance
calls and four prefixes before actual218780. All15,947 combined native cases
pass normally and with ASan/UBSan in an unchanged snapshot; owned component
source/builds are removed. Transformed-path completion and nonfinite/x87 status
behavior remain unqualified by these component cases. The v8 capture was rejected before building because a concurrent multiplayer
test edit changed during copying; its scratch is removed and the guard remains
intact. The complete v9 client is building to qualify the actual283-tick path. Retain the required296,037-byte
original reference, no binary or raw bank.
[Path-distance component proof](../analysis/campaign-ai-coop-20261020/path-distance/component-proof-v1.json),
[Reference and boundaries](../tests/fixtures/campaign_ai_path_distance_tu15.md).

The complete unchanged v9 client including200C00 now passes cold-world
ownership, original activation,15,947 combined component cases, both seed
regressions and all three300-tick actor controls. Three Marines progress to288
ticks with zero script faults and an initial checkpoint, then original1B0195
invokes missing1BD1E0 through descriptor48ABE0+0x10 (kind1/index27). Its missing
static closure is two whole bodies /148 original sites, including1ED100;
267450,278DD0 and27C710 already have owners. Full Marine completion, transformed
paths, live replay and mission/combat/co-op remain open. This is a composed
progression result, not a controlled binary A/B or a separate200C00 live trace.
The snapshot is unchanged; later checkout edits are recorded. All probes use
prepared MCC input without original-reference opens or cache writes, and owned
source/builds are removed. V8 was rejected before building due to a concurrent
test edit during capture; its scratch is also removed without bypassing the
consistency guard.
[Full v9 proof](../analysis/campaign-ai-coop-20261020/path-distance/full-native-proof-v9.json),
[Next original callback closure](../analysis/campaign-ai-coop-20261020/path-distance/after-288-closure-v1.json).

Descriptor27 now has whole original1BD1E0 and1ED100 in the current actor
module:93 bodies /7,112 original sites, preserving all91 prior bodies and212
immutable bytes.372 complete original/native cases exercise every new site,
all11 property-lookup sites and all50 existing iterator sites, without supplied
returns or rewritten lookup/iterator bodies. All16,319 combined component cases
pass normally and with ASan/UBSan in an unchanged snapshot; owned source/builds
are removed. The separate squad-style fallback remains unqualified. The only
component checkout drift is an authenticated opening-docstring-only change.
Retain the required1,510,016-byte original reference, no binary/raw bank.
Complete v10 composition requests900 ticks per authored encounter to continue
the actual288-tick Marine path and expose later lifecycle/behavior dependencies.
[Component proof and cleanup](../analysis/campaign-ai-coop-20261020/behavior-27/component-proof-v1.json),
[Original reference guide](../tests/fixtures/campaign_ai_behavior_27_tu15.md).
