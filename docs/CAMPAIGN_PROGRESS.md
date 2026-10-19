# Campaign implementation status

## Memory continuation — 2026-10-15

The user resumed the campaign goal. The interaction asset loader now shares
immutable clips within one load, preserving graph/index/name identity and each
unit's independent pose and clock. Weak references preserve the original release
points for discarded clips. All 15 original campaign maps retain byte-identical
decoded clip fields and sampled poses in serial, zero-swap checks under the
64 MiB ceiling. Cairo retains 59 clip allocations for its 119 references and
uses 5,295,152 fewer heap bytes in the controlled minimal-content probe.

The source-world constructor also releases its copied tag snapshot immediately
after publication into source memory, removing 8,138,768 bytes in the measured
Cairo startup phase. The bounded probe now completes source-world and Simulation
construction, including actual player and biped birth. Five noncreating
`ai_place` overload cases and counted actor creation now pass actual source
ticks in the coherent engine build, including equipment adoption. Authored
packed starting-location placement reached E558D -> 2039B0. That original leaf
is now recovered, with 108 original/native ABI, instruction and memory comparisons
passing under the serial memory guard. Its coherent engine rebuild passes. A
subsequent query fixture passes NONE getters and five noncreating placements,
then reaches missing original callback 1B0195 -> 1C9C90 before its non-NONE getter
checks. The authenticated callback is now in the canonical module, preserving all
previous bodies. Its 432 original/native full ABI, instruction and memory cases
pass under the serial memory guard. The next integration proceeds beyond it
and reaches original selector 1B0128 -> 1BFE00 before counted-birth completion
or non-NONE getter assertions. The complete 98-instruction selector is now in
the coherent private engine, preserving all previous routines. Its integration
gets beyond that selector and reaches missing 1BFCE0 in descriptor row51.
All 444 emulated original component cases cover the selector's instruction and
branch sites. Native comparison found an x87 precision-flag discrepancy;
unchanged original x86 execution on hardware matches the port in the first two
cases. All 444 original-hardware cases now qualify the complete non-FP ABI and
final banks, with measured FP status replacing only the emulator's status
fields in a fresh golden. That controller passes independent aggregate RSS
accounting at 40,640 KiB with zero swap. The complete native comparison now
passes all 444 cases at 49,217,536 bytes conservative aggregate RSS with zero
swap. Its full raw ledger separately authenticates 9,420 store-observer
snapshots while retaining all 30,444 original CPU reads and every original
assertion. The qualified selector is applied to the root generator/module,
preserving all 679 prior C++ definitions and their order. The next 47-instruction
selector also passes all 702 original/hardware/native cases against the complete
coherent engine, retaining all 21,978 original CPU reads, 5,766 stores and complete
final banks. It is applied to the root generator/module without changing the
680 preceding bodies; root now has 681 C++ definitions. Full campaign integration
remains open.
The 432-case 1C9C90 regression is registered in CTest with an exact compressed
original fixture and passes against the coherent private engine under the same
serial memory guard. The root test target has not yet been configured or built.
The later memory audit found that externally charged shared-library pages can
make total process residency exceed the earlier cgroup measurements. The owned
cold-query experiment was stopped; total-residency compliance is unproven for
that engine fixture. Subsequent original/hardware reference recovery passes
both cgroup and independent aggregate RSS checks, restoring six exact missing
historical banks. A private headless dependency reduction and independently
calibrated phase-aware RSS observer still find a real 69,300,224-byte query
footprint during construction. The borrowed tag-image change now passes 32
serial before/after checks across all 15 maps, including exact bytes/publication,
17 admission cases and five alias rejections. Cairo's measured duplicate heap
allocation decreases by 8,138,848 bytes. Five narrow root changes and current-root
loader/binder/world syntax checks pass. The coherent private engine now rebuilds
all 584 affected header consumers, four targets and three sealed fixtures,
retaining all 636 original AI definitions and source/dependency proofs. The
serialization translation unit exceeds its 1 GiB build group; its unchanged
recipe passes in the existing isolated 2 GiB build allowance, with zero swap.
The remaining build and fresh headless query link pass under 1 GiB. The actual
query rerun still fails the independent RAM gate during world construction:
75,702,272 bytes conservative aggregate RSS, before the intended gameplay
assertions. Constructor phase diagnostics locate that first failure in resource
authentication, before executable-image loading. Bounded SHA256 updates and
content-preserving page reclamation now pass complete Cairo/shared-file hashes,
dirty private-page, anonymous/owned-buffer and boundary checks under strict RAM
accounting; the one-file loader change is applied to root. The actual query now
passes resource, executable-image, BSP and DSP loading, then reaches a later
constructor memory failure at 68,194,304 aggregate RSS bytes. Ownership-group
diagnostics identify tag delivery as a later peak: 79,659,008 aggregate RSS bytes
as source pages and the independent destination become resident. A private
probe returns unused allocator pages after executable/DSP loading and now
passes the complete query's RAM check at 59,883,520 bytes with zero swap. World
and Simulation construction, NONE getters and all five noncreating placements
pass; counted placement reaches the next missing original AI owner,
1B0128 -> 1B7240, before actor-birth/getter completion. The clean build without
constructor markers also passes RAM at 59,621,376 aggregate RSS bytes in 106.147
seconds, with zero swap and the same original owner gap. The heap hint is now
applied to root through three narrow changes, preserving the current source
and passing root-world syntax checks. The generic
memory-copy optimization remains unadopted. The focused
tag-view jobs pass independent 64 MiB RSS and zero-swap checks; they do not prove
full-world residency. A low-host-memory build attempt stopped before compiling;
other running applications remain untouched. The
current 256-case overload differential and default campaign regression suite
also pass. Heavy paging slowed the latest integration jobs; an unsuccessful
private paging hint remains disabled. Full AI-dispatch integration and campaign
completion remain open. The
[continuation evidence](../analysis/tags/campaign-memory-20261015/RESULTS.md)
distinguishes the completed asset checks from those remaining requirements.

The pending callback now has ten complete statically audited original bodies
(1,289 instructions total), with no unresolved direct owners in that new-body
graph. Existing transitive shared owners, real Support virtual callbacks and
conditional arena routes still require qualification. Existing Damage and Support owners
will be reused through their real routes. The current hit-region implementation
and ordinary navigation-adjacency fix are present; stale host-only fixtures do
not establish a new source-runtime regression. Fresh source-backed campaign
navigation/hit checks remain pending after AI placement/getter integration.

Fresh capture runs reproduce a warmer startup residency failure despite the
earlier 59.6 MB pass. Unchanged phase diagnostics locate it at tag delivery.
Private content-preserving code/cache paging probes are being tested under the
same 64 MiB total-residency ceiling; none is applied to the engine. The latest
probe completes its pending-call input capture at 58,294,272 aggregate RSS bytes,
passing World/Simulation construction, NONE getters and all five noncreating
placements before the original 1B7240 failure. Complete input transport and RSS
verify independently; the interrupted outer controller's final kernel accounting
file is missing and is explicitly unclaimed. The callback itself remains untested.

Both private and current-root ten-body generation proofs now pass while
preserving all prior whole bodies, helpers, FP semantics, API ownership and
order. The private636 candidate has646 definitions; the current686 candidate
has696. Neither is activated. Exact input inspection reaches the genuine unit
predicate, and the bounded original-code catalog is prepared (645 complete AI
functions plus three real shared owners). The next strict serial run executes
the captured original call; hardware/native/finite-case parity and complete
campaign integration remain open.

The exact captured1B7240 call now passes original discovery/formal execution
and a complete646-body native component comparison:223 original instructions,
69 real reads,21 stores/snapshots, all180 ABI/FP return bytes and all67 final
bank runs. Original formal peak aggregate RAM is36.9MiB; native is12.6MiB,
with zero swap and complete persistent controller reports. Independent original
ledger/final-bank verification passes. Native raw-ledger partition, hardware CPU
qualification, wider finite branches and isolated query integration remain open;
the candidate is not applied to root.

The callback's independent raw-ledger verification now passes, retaining all
69 genuine operand reads and 21 immediate writer snapshots separately. The
exact captured call also passes on the host CPU under KVM: actual XSAVE
readback preserves all 180 entry/exit ABI/FP bytes and all 67 final accessible
runs (60,092,502 payload bytes). Aggregate RSS is 13,201,408 bytes with zero
swap; the persistent controller and separate full-bank verifier pass. This is
one captured call, not console or full-campaign parity.

All eight sealed original sparse cases now pass discovery and fixed-page formal
execution serially, at most 39,059,456 aggregate RSS bytes with zero swap. Seven
are explicit diagnostic inputs. Independent case-ledger verification and native
case comparisons remain pending; case names are not assertions of a predicate
result. The private query still exceeds RAM before birth/getter assertions.
Twenty phase markers reach the end of private World::Impl construction, which
precedes map/session initialization and does not prove campaign construction
returned. A 40-phase successor builds successfully to locate that later peak.
No new callback/public API/paging proposal is applied to root; the goal remains
active and cleanup was canceled without deleting files.

Independent reconstruction of all eight originals now passes: 651 total trace
instructions, 220 genuine reads, 69 stores, every complete ABI/final bank and
both strict controllers per case. The measured writer union retains 21 pairs
and 32 source emissions. The captured seed takes JNE1B72CB's real fallthrough
1B72D1; a separate erratum corrects its misleading sealed name. Native finite
comparisons are next. Forty-phase diagnostics locate the later query RAM stop
after map initialization and191CA0, before125D40 returns, with unchanged
anonymous/live heap usage and higher file residency. A private existing-helper
code-page advice candidate is being prepared; no source change is activated.

The three-handoff marker-free candidate now passes the actual query's RAM check
at56,123,392 bytes with zero swap and complete controller evidence. World and
Simulation construction, NONE getters and five noncreating placement overloads
pass. Counted placement gets beyond1B7240 and reaches the next missing original
owner1B7000; counted birth and the targeted getter RED remain open. All eight
native component cases also pass their independent full trace/ABI/bank and raw
read/store/snapshot checks serially at13,201,408 bytes each with zero swap.
Measured new-closure coverage is54 of1,289 sites,13 of234 conditional edges and
3 of38 calls; this does not certify the other branches or campaign completion.
The fresh original1B7000 audit passes:146 instructions,10 existing direct
callees, complete decoded control flow and RET8. Original row20 resolves to the
actual48A5FC descriptor, without relying on the different base-retail row.
Private647 generation preparation found a missing consumed historical reference
bank; exact chunk-backed restoration is required before generation can proceed.
The narrow root memory application now passes current-project World compilation,
Linux helper link and independent dependency/source qualification. Its three-file
application preserves the concurrent MCC source/CMake changes through exact
before/after and inverse checks. The private query's RAM result does not certify
the complete root runtime. Fresh root697 generation also passes, preserving all
current686 bodies/helpers/FP/API/MCC/order with11 pending audited additions and
1435 new instructions; that AI candidate remains unadopted. The captured
descriptor mismatch is only+4 changing0 to7F0. The original MOV/MOV/RET0
initializer is now authenticated against TU1.5, and fresh complete input
qualification passes with the exact whole descriptor after that single4-byte
store. All other bytes, full ABI/FP/banks and strict RAM/controller gates remain
intact. Original/native execution of1B7000 is the next component check.
The original caller's131 admitted descriptor slots are now inventoried with
authenticated signed-index and kind0 dispatch guards. Forty-four are kind0
callbacks;35 lack a qualified AI-frame body in the three current source
cohorts and require cross-module ownership checks before being called missing
engine implementations. Other kinds/layouts remain uninterpreted. This is a
static inventory, with no callback activation or campaign completion claim.
Its bounded successor also connects the table-loaded EDI to the kind0 load
across every reached forward branch, preserving all register aliases and the
exact early-exit epilogue. The original1B7000 discovery and formal replays now
complete198 instructions with exact full180-byte entry state, real RET8 and
matching full final-bank hashes; the formal ledger records60 reads/19 stores.
Both pass complete serial memory evidence below40.2MB aggregate RSS with zero
swap. The captured native647 comparison is the next component check.
Independent original reconstruction now passes: every198 trace instruction,
60 reads/19 stores, full180-byte entry/exit and all67 final bank headers/payloads
are verified. Nineteen measured writer pairs have30 exact AI/Support emissions.
Native647 comparison and counted actor placement remain open; this does not
activate root AI ownership or close the campaign goal.
The complete647 isolated build and independent native comparison now pass:
all198 PCs,60 reads/19 stores/19 snapshots,180 ABI/FP bytes and every67-run
final bank byte match the original. Native aggregate RSS is13,201,408 bytes
with zero swap and complete controller evidence. This qualifies one captured
component input. The actual-query647 successor and counted actor placement
remain open; root AI ownership is unchanged.
The actual private647 query now passes counted original actor birth, NONE
getters and all five noncreating placement overloads at61,657,088 aggregate
RSS/HWM bytes with zero swap and complete controller evidence. Its next failure
is ai_get_object rejecting the actor reference. Prequery source binding and
the exact packed reference still need a narrow diagnostic before any getter
API change; no main-engine AI owner is activated by this private test.
The closed36 source candidates now pass private683/root733 generation while
preserving every prior body, helper, FP/API/MCC path and public owner list.
Four existing external owners are reused. Both candidates remain uncompiled
and unadopted; the main AI module remains at its current686 definitions.
A bounded diagnostic now proves the getter regression: valid AI19 reference
C0010000 is rejected while the original full actor/unit/SID binding remains
stable across all20 sampled prequery/dispatch/exit points. Its complete native
run uses60,354,560 aggregate RSS/HWM bytes and zero swap. Original getter types
and code bytes have fresh full-image authentication. The source-backed API
fix is being refreshed against private647 and the current root; it has not
been applied yet.
The isolated clean API build passes all five compile/link stages, but its
runtime attempt is stopped before getter assertions at68,444,160 aggregate
RSS/HWM bytes. The64MiB guard and zero-swap policy remain unchanged. A phase
memory diagnostic is being prepared; the root API patch remains unapplied.
The full goal remains active.

The fixed-buffer phase diagnostic measured file-backed residency growth during
player creation; both NONE getters entered/exited without sampled heap or
anonymous growth. That diagnostic still failed the unchanged64MiB limit. A
fresh observer-free candidate reuses the existing code-only paging helper after
completed initial source respawn. Its two-stage isolated build passes; native
behavior and memory qualification are pending. Root API remains unapplied.

The observer-free post-respawn candidate now passes the unchanged memory gate:
59,596,800 aggregateRSS/HWM bytes and zero swap with complete evidence. Actual
actor birth and all typed squad/direct/location/empty getter assertions pass,
preserving the full unit datum. Final authored packed placement reaches a
distinct original2039C3 backing fault at address38. That failure still blocks
full query success, root API application and the checkpoint successor.

The isolated recorded-animation startup backport passes fresh original11-body/
377-instruction authentication and its exact three-stage build. Its runtime
attempt stops safely during player creation at67,112,960 aggregateRSS/HWM,
one4KiBpage above the unchanged64MiB limit, with zero swap. It produces no
semantic assertion result. A minimal additional code-only handoff before initial
respawn is being prepared; existing post-respawn paging stays. Root API/birth
application and full campaign completion remain open.

The additional pre-respawn handoff builds, but its native attempt still stops
during construction at67,395,584 aggregateRSS/HWM bytes (286,720B over64MiB),
zero swap. Both hints run after member initialization, so this result does not
locate the peak. Fresh fixed-buffer phase instrumentation will measure those
initializers and actual respawn/player creation before another memory change.
No semantic assertions or root application qualify from either failed run.

The completed startup diagnostic stops inside actual player biped creation:
26 complete records end at Campaign-create-biped-pre. Its aggregate67,334,144B
exceeds64MiB by225,280B; swap remains zero. The pre-respawn hint drops1,425,408B
of sampled file residency without changing sampled heap/anonymous memory.
The biped's unobserved tail has no measured anonymous/file split. A separate
1,048,656B eager native-adapter arena lies in the member-initialization interval;
actual campaign birth uses the retained original World allocator. Source review
is preparing explicit deferred adapter initialization while preserving public
eager constructors, logical capacity, const access, copies and archive bytes.
No root patch or memory/runtime success is claimed; the goal remains active.

The standalone deferred-storage component now passes its actual old-owner
comparison at20,267,008B aggregateRSS/HWM (19.33MiB), zero swap. Exact arena,
scratch, handles and logical capacity agree; allocation counts, eager capacity
reuse, deferred copy/restore, salted allocator reuse, ordered store observers,
concurrent const reads/copies and extracted codec-field branches pass. Actual
member-offset syntax checks and all11 retained old-member dependency exclusions
also pass. This qualifies the host adapter optimization only. The private main
successor must rebuild the31TU union of actual storage/API class consumers,
retain original startup order and pass the unchanged full campaign query before
root API/birth application. Full production codec and campaign parity stay open.

The coherent31 main build passes all31 compiles and link, retaining actual
class definitions and ordered startup. Its observer-free campaign query still
stops during Simulation construction at67,272,704 aggregateRSS/HWM,
163,840B above the unchanged64MiB limit, zero swap. No semantic assertion or
full query success is recorded. The next diagnostic will measure current
startup and inner-biped Anon/File/heap, since prior-cohort measurements cannot
locate this peak. Root API/birth application stays gated. New test jobs use
the repository disposable scratch lifecycle; small failure evidence is retained.

The fresh diagnostic stops before compilation because61 required retained link
archives are missing. All three source/compiler dependency closures remain
present. No native test runs in this attempt; both owned scratch directories
are verified removed and all owned services terminal. Exact archive recovery
from surviving authenticated objects/source metadata is the next prerequisite.

The source-authority correction resolves eight stale hashes in the recovery
manifest without changing source behavior. Recovery now completes all405
selected compiles,61 derived archives and the first relink in302.101 seconds
under1GiB with zero swap/OOM. The complete executable hash differs from the
retained reference, so strict admission rejects it before any gameplay run.
Both owned scratch directories are verified removed, with no new large
artifacts retained. Bounded ELF and ordered-link forensics are the next step;
reference equivalence, the campaign query and full campaign parity remain open.

The recovered baseline now passes a distinct, complete metadata-equivalence
check against the retained reference:19,763 full symbol records and every
static-string byte agree in meaning;90,809,506 bytes outside the two unallocated
static tables and20-byte GNU build-ID descriptor are identical. The entire
normalized linker map agrees, including405member order,8,189input sections and
initialization. The original full-file SHA remains different. The five-stage
owned diagnostic then completes its build/admission/analysis lifecycle; its
native run stops at67,883,008aggregate bytes, before the query assertions.32
phase records reach source biped call B9B70 after frame/B9960 setup. From the
earlier paging hint to that boundary, File rises3,895,296bytes with Anon flat;
the following unobserved peak is not assigned a cause. The private code-page
hint and paired unused-heap/code-page hint experiments both exceed the strict
64 MiB aggregate ceiling before the original query assertions complete. Their
threshold-crossing measurements are incomplete peaks and do not establish an
improvement. Neither engine patch is applied. Both owned scratch directories
from each run are independently verified removed; no new large artifacts are
retained. The next bounded diagnostic measures original calls inside B9B70,
with every other archive member unchanged. Full query/campaign parity remains
open.


The live checkout now includes the typed AI getter implementation. Campaign
`ai_get_object`/`ai_get_unit` queries pass the original packed reference through
one Campaign/World API to the existing actor iterator; those two original
entries are exported without changing the 686 generated AI bodies. The unused
native body adapter in a source campaign now allocates backing on first access;
ordinary construction and the serialized field order stay unchanged. All 24
reviewed source operations across 11 files are applied, preserving concurrent
World edits. Fresh targeted compilation and the original reference parsing test
are in progress. This source implementation does not establish full campaign,
checkpoint, or gameplay parity. The frozen native query still exceeds its
strict 64 MiB aggregate ceiling; the failed before-biped paging hints and direct
route optimization remain unapplied.

## Opening cinematic audit — 2026-10-15

The opening goal is active. Separate candidates repair relative animation,
cinematic lighting/bloom, shader controls, HUD preload, cinematic frame accounting,
startup coverage, fade/letterbox ordering and captured-frame crossfades. Initial throne attachment,
authored cleanup, the prophets'89-frame ordinary idle loop and checkpoint
restoration pass. Original numeric fixtures and the default checkpoint suite
pass. Source model-order sampling repairs the Pillar of Autumn binding and
matches156 original WORLD matrix words. Normal native playback now reaches
scene07/tick3462; missing marker effects stop the main script and Halo explosion.
Cinematic shadows and emotional-state calls also retain faults.
Shared marker-effect request construction now matches192 unchanged TU1.5
cases. The actual two scene07 graphs require one direct lens plus three light
lenses; strict complete-graph checks retain these missing consumers instead of
counting particle-only preparation as complete playback. The existing
Elongation component regression still passes. The retained campaign host now
calls the complete original marker wrapper and cinematic start/stop/LOD owners.
Actual opening scenery retains both source allocations; deferred particle
retirement,480 source game/particle phases and source checkpoint restoration
pass. Renderer CPU storage matches six complete original arena/query/CRC
fixtures without setting the graphics-initialized flag. Per-view particle,
light-lens query/draw remain open; this is
not a new full-movie run. The latest full candidate still has the faults above.
The subsequent retained-world controller bridge passes actual relative unit
and scenery starts, source WORLD movement and Regret/throne parenting through
updates and recursive deletion. The unit advances60 frames over60 phases;
the Pillar initially advances only three. The subsequent original scripted-camera
bridge repairs that observed activation failure: actual scene03 camera/PVS
admission advances both Pillar and camera time60 ticks over60 source phases.
Real camera/WORLD motion, FOV storage, checkpoint repeat and stop pass;192
original camera-time values and192 bounded director tick banks match. Native
Presentation still supplies rendered camera pixels. The subsequent source timed
screen bridge passes864 complete original bank comparisons and actual-world
lifecycle/checkpoint repeat. It executes original update/stop/acknowledgement
over the actual arena bank. A complete additive-animation owner then repairs
the source scene01b failure;120 phases with both prophets/thrones and all ten
authored unit tracks pass, with all ten WORLD hierarchies moving. The traced
source-integrated GPU run records a crossfade copy acknowledgement and enters
scene02. The subsequent source shader bridge retains original construction,
interpolation, clear and32-slot replacement, with borrowed renderer samples.
381 complete original bank/body/query comparisons and the actual Banshee
clock/flag/renderer/checkpoint checks pass. Recorded-animation storage/map reset
and query are coherently included in that candidate. Its virtual X11/RTX5080
run passes the earlier shader/recorded-query failures and reaches scene04;
cinematic shadow, graph-switch and source emotional-state handling remain open
in that older run. The subsequent unit-control cohort restores whole graph
reset/emotional owners and the registered facial-weight normalization initializer.
192 complete original wrapper/body/clock cases pass. Actual authored prophet
starts, six graph resets, facial binding/target/progress,30 real source phases,
custom clocks and checkpoint replay pass; the original model builder produces
different emotional versus neutral rendered WORLD bones. That check also exposed
and repaired the missing whole weighted animation composition owner. All eight
serial regressions pass under the unchanged guard, and seven current translation
units compile from a stable1102-file capture. The main campaign renderer still
needs to consume that deferred facial pose. The subsequent longer RTX5080 run
passes the former graph-switch/emotional command faults and enters scene08,
then exits on cinematic model variant admission that lacks the live source
selection. The scene02 cinematic-shadow fault remains. The same binary/data
observation exits after256.833s under2GiB/zero swap, with muted audio and no final
numeric tick/frame summary or screenshot. Script progression through the scene07
Halo-explosion request does not establish visible effect acceptance. No complete
playback or facial pixels are inferred. The subsequent live model resource
handoff removes random-preview selection and consumes the original builder's
selected LOD/regions and deferred WORLD pose. Four guarded actual-world checks
pass, including Tartarus's unresolved-preview/live-selection case and Regret's
emotional vertex differential. Exact LOD-bank sharing and selected-section
skinning retain all authored resources. The2GiB graphics probes stop in scene02
on host pressure; telemetry shows substantial free GPU memory. A separate3GiB
graphics observation is retained in the source-model-render-integration-v11
cohort, while the60MiB hard/64MiB measured source-match guard remains unchanged.
That observation reaches scene09/tick3760 and exits after308.902s at missing
original resource-pressure owner125FB0; peak host charge is3,082,862,592 bytes,
zero swap and no cap/OOM events. The three whole pressure/eviction owners are
now restored (108 instructions), and the ordinary control/vertex regression
passes under the unchanged guard. Playback now exercises pressure/eviction and
returns valid salted allocations. Including the existing whole cloth constructor
module passes the later guard-birth failure. Two actual guard births and two
unchanged original constructor replays pass under the same guard (2.260s);
both declared variants take the original no-marker/free branch. The latest
graphics probe identified stale `throne_regret` after original parent deletion.
The shared scene now observes all actual retired identities immediately; the
two-generation Regret/throne check passes (1.858s). Playback passes that cleanup
and enters `c01_intro_scene_01`, scene12. It exits on ship-panel shader admission.
Both affected materials use the existing original `stencil_env` program; four
material/LOD binding checks pass after correcting the dynamic environment guard.
The full retry remains pending. The shadow thread still faults,
and full effects, DOF, original pixels, audio timing and all13-scene/handoff
acceptance remain open.
Headless startup reaches authored predictions, then exceeds
the unchanged cap while retaining geometry/bitmap resources. Full playback is
not accepted. Original unit-time value projection matches192 cases.
The production crossfade consumer passes3,477,548 exact GPU pixels, including
resize history and point-sampling checks. Full authored playback remains open, and
GPU captures still show incorrect visibility/transparency. Four independent
scene helpers pass 1,200 ticks; this does not establish full cinematic playback,
audio, transitions, handoff or TU1.5 visual parity. MCC imports followed by TU1.5
conversion remain the intended pipeline. See
[the scene matrix and evidence](OPENING_CINEMATIC_STATUS.md).
Delivery includes a video of the completed opening with audio, after these gates pass.

## Implementation handoff — 2026-10-15

The user will perform proper gameplay testing later and requested pausing the
campaign goal after the current implementation checks. The source `ai_place`
bridge now admits both original signatures and forwards packed references,
NONE and signed-short counts unchanged. The original 256-case wrapper evidence
was regenerated byte for byte, and the captured bridge compiles. An interaction
asset loader also releases 3,801,088 bytes of measured empty render-vector
capacity in Cairo without removing live hierarchy or animation values.

The combined native Cairo/Simulation check still exceeds the memory cap during
setup, so no new mission completion, bridge integration pass, or gameplay parity
is claimed. The final attempted build guard stopped at the 4 GiB host reserve;
the unverified additional optimization was removed. Source changes, explicit
limits and reproducible evidence are in the
[handoff](../analysis/tags/campaign-ai-bridge-20261015/RESULTS.md).

## Prior simulation continuation — 2026-10-14

The Armory now reaches its authored completion through ordinary player Controls
in 4,150 game ticks, without cinematic skip or script faults. A second run
restarts the authored checkpoint once and completes at the same game tick after
4,983 submitted steps. Both runs fit the aggregate memory ceiling and reproduce
their pre-compaction tick traces and raw logs byte for byte.

**The full campaign remains unfinished.** The other 14 authored entry points
still fail on unsupported animation, attachment, AI, save, interpolation,
kill-volume or visibility services. Native Armory completion does not establish
original animation/combat behavior, rendered or audio parity, later map handoff,
or complete campaign/BSP/checkpoint coverage.

The continuation fixes actor-start reference frames, nonpositive short sleeps,
posed unit query centers, ordinary authored navigation adjacency, first-chunk
sound prediction and shared collision storage. Original executable fixtures
cover the relevant scalar/selection/transform behavior. Unit query centers use
the existing native pose owner; full original animation scheduling and posed
combat hit regions remain open.

All simulations are serial and zero-swap. The guard configures 60 MiB for the
whole job and all descendants, leaving 4 MiB accounting headroom, and rejects
any kernel-measured peak above 64 MiB. The Armory and its checkpoint replay
peak at 60 MiB. Three full multiplayer simulations pass the same policy and
retain byte-identical tick/final traces against both captured baselines.
Compilation runs separately, serially, under 1 GiB with zero swap.

See the [complete results and remaining map matrix](../analysis/tags/gameplay-validation-20261013/campaign-completion/RESULTS.md),
[aggregate report](../analysis/tags/gameplay-validation-20261013/campaign-completion/report.json),
and [current reproduction procedure](../analysis/tags/gameplay-validation-20261013/campaign-completion/REPRODUCE.md).
The captured application and validation targets link successfully; the
concurrently edited root checkout and installed launcher are separate delivery
checks. Original base/TU1.1 CPU evidence is not Xbox TU1.5 visual proof.

The subsequent [AI owner continuation](../analysis/tags/gameplay-validation-20261014/campaign-ai-references/RESULTS.md)
adds separate source squad/group records and packed-reference iteration. Forty
whole-original banks and 9,002 original queries validate linked order, group
traversal, starting-location SID matching and inactive/direct-reference behavior.
The canonical importer now retains squad/group parents, starting-location SIDs
and inherited general flags, and resolves team bytes using the original direct
unit/creature/general-type precedence. All 15 actual campaign maps pass the
import/constructor checks. The actor bank's 96 cold-constructor cases remain
passing. Armory completion/checkpoint and all three full multiplayer traces are
unchanged; the measured continuation peak is below 61 MiB, with zero swap.

These AI banks are not yet connected to Simulation's actor birth, activation,
perception or script-list lifetime. The same 14 campaign entry points remain
failed. No additional mission or original AI/gameplay parity is claimed. The
captured application links; the concurrently edited root application's full
build and launcher delivery remain separate checks.

## Historical implementation record — 2026-10-07

2026-10-07 historical status. The goal was the complete original campaign. **No campaign mission
was then claimed complete or playable end to end.** Fourteen of 15
authored map entry points still encounter unsupported services in the first
60 simulation ticks. Delta Towers passes that window, then stops at weapon
cloning in the 300-tick check. This report distinguishes implemented services
from the remaining campaign work.

## Implemented this pass

* Added pending `switch_bsp` and `switch_bsp_by_name` requests using their
  original compiled ordinals. The tick boundary replaces collision/navigation
  data while preserving the interpreter, globals, game clock and player state.
  The renderer replaces static geometry, lightmaps and sky, and rebases shared
  model materials. Requesting the current BSP cancels an earlier request;
  invalid ordinals are ignored.
* Added cross-BSP checkpoint restoration. The saved immutable collision and
  navigation world is restored before the saved VM/world continuation, pending
  switches are cancelled, and the renderer follows the restored BSP revision.
* Added scene placement/lifetime policy using authored origin/manual BSP masks,
  sorted transition-volume pairs, strict interior containment, and persistent
  versus automatic placements. Named persistent objects retain handles while
  hidden, and script visibility survives reactivation. Missing eligible
  automatic scenery is created on adoption. General parent trees and attached
  trigger-node transforms remain unfinished.
* Connected `sound_impulse_predict`, `predict_model_section` and
  `predict_structure_section` to retained native assets. As updated on 2026-10-14, sound prediction retains the first compressed chunk;
  full PCM is decoded by the existing playback path on demand. Model prediction retains every section/PRT payload
  and every referenced material bitmap record; BSP prediction retains every
  cluster/instance-definition geometry payload. Cluster prediction also retains
  dependent models and indexed bitmap payloads, including volume/vector formats
  and all three authored cache LOD byte ranges. This backend loads whole native assets
  synchronously; it does not reproduce Xbox chunk queues, cache eviction,
  or object/instance lightmap bucket residency. Rendering still selects its
  currently supported display geometry/formats separately.
* Added original 54-slot sound-class names and substring-selected dB fades to
  the live SDL mixer. Class identity survives start/loop/end concatenation;
  existing and future voices use current gain. Fade clocks follow wall/audio
  time and checkpoint restoration retains in-flight fade state. Ducking,
  DirectSound quantization, saved voice cursors and full spatial playback
  remain unfinished.
* Added original five objective states, cooperative-mode query for the actual
  solo backend, flashlight availability/change tick, cinematic ambience-detail
  enable state and one-shot ambience-rebuild suppression state. Objectives
  still need their pause-menu text/checkbox UI; flashlight input/rendering and
  ambient-detail scheduling/portal rebuild remain unfinished.
* Added type-mask cinematic object cleanup, retaining native player ownership
  and stable actor indices across checkpoint restore. Stored cinematic LOD
  flags in scene objects; the current model backend already admits its highest
  LOD without a distance fade. General parenting/protection and original LOD
  selection remain unfinished.
* Corrected `sleep_until` to match original `2089B0/2099F0`: queue a nonliteral
  predicate, sleep, then evaluate it. Literal/global predicates are sampled
  immediately. Timeout zero bypasses predicate evaluation. Poll/timeout duration
  conversion uses the original float32 seconds/rate calculation and nearest-even
  rounding at the native 30 Hz game frequency.
* Added immutable VM checkpoints containing globals, all threads, nested
  expression frames, polling state and wake deadlines. Restore is permitted only
  outside dispatch and only for the same script program.
* Implemented `cinematic_skip_start_internal`, `cinematic_skip_stop_internal`,
  `game_save_cinematic_skip`, `game_saving`, `game_reverted` and `game_revert`.
  Save requests are captured after world updates and the game-clock increment.
  Restore resumes the saved continuation at the restored tick before increment;
  the reverted signal is a timestamp comparison, as in the original.
* Restored native player/actor vitality, movement and weapon state, active
  projectiles, random generator, allegiances, scene object generations and names,
  camera/transition state, input flags, mission segment and game counters.
  A restore requested inside a builtin stops further VM dispatch before applying
  the world restore. Observed script faults remain in diagnostic history.
* Added Escape to request a cinematic skip when a recoverable checkpoint exists.
  Repeated held input cannot repeatedly restore the same checkpoint. The native
  input bridge does not establish physical Xbox controller parity.
* Corrected player `object_hide` handling and implemented the player-list damage
  protection used by Cairo's unchanged stash/unstash helpers. Object vitality
  immunity and unit reaction/impulse immunity are separate flags. Movement
  disabling is independent of general player input disabling.
* Added external-global access with explicit rejection of unknown services.
  The verified null-backed `cinematic_letterbox_style` short ignores writes and
  refreshes reads to zero, including Cairo's compiled long expression with
  conversion at the declared short binding. Texture/geometry flushes are
  admitted because their original evaluators really are return-only operations.
* Discarded-timeline audio voices are stopped, and saved scripted loops/alternate
  selection are restarted. This is native audio restoration, not a claim of
  original mixer cursor, ambience or module callback parity.

## Evidence and reproducible checks

The authority is the local original XBE, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`,
and the authored nodes in `01b_spacestation.map`. Primary instruction analysis:
[skip controller](CAMPAIGN_SKIP_RESEARCH.md),
[BSP lifetime and commit](CAMPAIGN_BSP_RESEARCH.md),
[audio classes and saved state](CAMPAIGN_AUDIO_RESEARCH.md),
[asset residency](CAMPAIGN_RESIDENCY_RESEARCH.md),
[actor/scenery animation playback](CAMPAIGN_ANIMATION_PLAYBACK.md),
[animation and supporting services](CAMPAIGN_ANIMATION_RESEARCH.md), and
[earlier Cairo service research](CAMPAIGN_RESEARCH.md).

The original-instruction fixture in
`tools/oracle_campaign_skip.py` executes Cairo's unchanged static
skip-start/stop helpers with the original HS interpreter and checkpoint
controller. It supplies storage, BSP/header compatibility and unrelated module
callbacks. Both natural completion and admitted skip input restore a pending
predicate at the saved tick, return false from skip-start, and reach skip-stop
without a second revert. This is bounded script/controller proof, not a full
emulated console playthrough.
`tools/oracle_campaign_skip_edges.py` separately checks 66 original controller
edge cases; the complete input/cache/module bridge is still incomplete.

`tests/campaign_tests.cpp` independently exercises the native implementation:
nested VM continuation restore, polling/global/timeout boundaries, unsafe and
cross-program rejection, post-call restore suspension, object generations,
presentation transitions, both cinematic completion routes, real Cairo compiled
skip/stash/unstash helpers, protection distinctions, audio cleanup and retained
fault history. A combat replay also checks identical movement, AI/weapon state,
projectile positions and random firing spread after restoring a checkpoint.
It verifies replay determinism of the current native systems; it does not prove
retail Havok or actor behavior parity.

`tools/oracle_campaign_bsp.py` passes 143 unchanged-XBE request, commit,
lifetime, placement and cross-BSP restore cases. Its external resource and
object effects are fixture adapters, not a console playthrough. Native tests
independently verify changing collision floors, continued VM execution,
revert/cancellation, stable generations and visibility, original Cairo audio
classes and retained payloads, and actual offline SDL mixing across class
phases. The audio/channel instruction fixtures run through
`tools/oracle_campaign_audio.py` and `tools/oracle_campaign_animation.py`.
Animation research is evidence for the next controller work; native actor
relative-animation playback has not been implemented by this pass.

```sh
cmake -S . -B build-campaign -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-campaign -j 8
ctest --test-dir build-campaign --output-on-failure
python3 tools/audit_campaign.py --native build-campaign/halo2_native
```

The audit deliberately returns failure while any authored startup faults. Its
JSON records runtime/map hashes, tick count, exit status and actual diagnostics.
The baseline is `analysis/campaign/startup-before.json`; the current report is
`analysis/campaign/startup-latest.json`. A successful short startup window would
still not prove mission completion.

Delivery verification: all 26 normal CTest groups and four focused
address/undefined sanitizer groups passed. The rebuilt title-first launcher
rendered 30 title frames on the Linux RTX 5080 with synchronization validation
enabled and no Vulkan validation messages. A six-frame Vulkan replay also
loaded Cairo BSP 0, switched to BSP 1 and restored the checkpoint to BSP 0,
with one restore, revision 2 and no script faults or validation messages.
These GPU checks cover title startup and resource replacement; they do not
cover authored mission progression or full cutscenes.

Use the root `Halo2-Title-Launcher` or the refreshed **Halo 2 Native (Title)**
desktop shortcut. Its runtime is `build-campaign/halo2_native`; keep that build
and the project assets with the launcher. Checksums/platform/validation limits:
[build manifest](../analysis/campaign/build-manifest.json),
[SHA-256 list](../analysis/campaign/build.sha256). Test evidence:
`analysis/campaign/{ctest-delivery.log,asan-delivery-ctest.log,
launcher-title.log,launcher-title.png,bsp-replay.log,bsp-replay.png,
residency-tests.log,deltatowers-300.log}`.

## Remaining campaign dependencies

| Area | Current blocker or required behavior |
| --- | --- |
| Mission entry and level streaming | Complete parent/source protection, attached transition volumes, actor admission/lifecycle, granular resource residency, later map handoffs and authored streaming paths. |
| Cinematics | Actor/scenery relative animation, live anchor node transforms, non-looping playback, interpolation, custom-duration queries, variants/permutations, attachments, weapon cloning, cinematic lighting services and camera/object synchronization. |
| Sound | Ambient details/portal rebuild, ducking, dialogue timing/localization, full cache residency, restore lifecycle and saved mixer positions. |
| Encounters | Complete AI expressions/groups/orders/command scripts, dynamic navigation, original combat status, squad/actor object handles and authored encounter conditions. |
| Sandbox | Vehicles, equipment, grenades, melee, dual wield, weapon interactions, damage attachments and original physics/AI behavior. |
| Mission progression | Objectives, devices and moving collision, trigger attachments, HUD/messages, checkpoints conditioned on safety, death/restart, persistent disk saves, difficulty/loadouts, mission completion and next-level transitions. |
| Validation | Every mission on authored paths, all difficulties and checkpoint/skip/death routes, dialogue/audio, gameplay and visual checks; original-console comparisons and full playthrough evidence. |

Current Cairo startup reaches `object_set_permutation` after saving/stashing
players, predicting assets and cleaning up objects. The tutorial next requires
`hud_enable_training`; the prologue next requires `cinematic_outro_start`.
Delta Towers reaches `cinematic_clone_players_weapon` in the longer check.
All other actual entry failures are retained in the audit. Shader and
lighting implementation remains in the user's separate conversation.

The current checkpoint is in memory, scoped to the same map and implemented
native subsystems. It is not compatible with Xbox save files and is not a disk
save system. General safe-save/restart, map handoffs and complete object-list
handling remain explicit dependencies.

## 2026-10-14 actor lifecycle validation

The active campaign goal remains unfinished. The new source actor lifecycle now
handles linked squad/global membership, joint/observation/tracking cleanup,
clumps, dynamic firing points, swarms, command participants, HS thread retirement,
unit detachment and original counters. Non-NULL slot callbacks and a nonempty
followed-reference registry still require their actual owners. No fallback success
or separate tag decoder is supplied.

384 whole-original lifecycle cases match in both owned native pools and actual
source-backed bank views.288 unit binding/unbinding cases also match through
cold actor retirement. Constructor, control and recursive-seat regressions pass.
The eight final native jobs ran serially with zero swap; their maximum measured
peak was36,753,408bytes. Original-reference jobs peaked at62,914,560bytes, below
the64MiB ceiling. The component builds directly from captured current sources;
this stage did not link the full current application or refresh its launchers.

These actor/control owners are not connected to Simulation yet. Complete source
object/actor birth, admission, activation and shared ownership are prerequisites.
The sealed campaign runtime still has Armory completion/checkpoint evidence and
14 explicit failed mission entries; this component work does not establish new
mission completion. All previous runtime/source evidence pins remain unchanged.
Reference execution uses the base retail CPU image and is not TU1.5 visual proof.
Details and reproduction: [lifecycle results](../analysis/tags/gameplay-validation-20261014/campaign-ai-lifecycle/RESULTS.md).

The subsequent actor-control stage completes retail eligibility, activation,
clump wake and team propagation.256 whole-original cases match in both owned
storage and a borrowed source actor bank. Binding and teardown now also pass
through the bank's public API against the same backing. All12 serial component
checks pass; final native peak25,055,232bytes, zero swap. The source-bank view
does not allocate or copy records and must be reconstructed after whole-bank
backing replacement. Whole birth/admission, squad membership, world checkpoint
integration and campaign wiring remain open. No additional mission completion
is claimed. [Actor-control results](../analysis/tags/gameplay-validation-20261014/campaign-ai-birth/RESULTS.md).

Squad join/activation, global membership and location-reference reset now share
the actor bank API and original unlink owners with teardown. All 256 new
whole-original membership cases match in owned and source-backed storage;
all 14 final serial component jobs pass. Their peak was 35,536,896 bytes,
zero swap. The original-reference peak was 62,914,560 bytes. The captured
six-executable cohort and earlier evidence are sealed without changing old
results. Full birth/admission, source-world startup RAM and campaign wiring
remain open; Armory remains the only retained authored mission completion.
[Membership results](../analysis/tags/gameplay-validation-20261014/campaign-ai-membership/RESULTS.md).

The source-memory transfer stage removes full temporary copies for validated
single-region reads/writes, retaining cross-region staging and byte/permission/
checkpoint behavior. The actual 40 MiB overlap reproducers now pass at
42,655,744 bytes; their prior versions hit the contained guard. All 16 actor and
resource regressions plus three new guarded CTest cases pass. Armory completion
and checkpoint traces remain identical before/after. Three full serial native
multiplayer simulations also match the prior sealed traces; maximum final peak
64,372,736 bytes, zero swap.

The campaign remains unfinished: 13 other entries still fail on required script
owners, and the current Oracle entry hits a separate BSP-import memory failure.
Equal partial Oracle logs are not parity proof. Current-source Lockout/Cairo/
Armory source-world startup also remains above the contained memory budget.
A fresh root configuration error was fixed by registering multiplayer
continuation after its physics target exists. No launcher update or new mission
completion is claimed. [Memory results](../analysis/tags/gameplay-validation-20261014/campaign-runtime-memory/RESULTS.md).

The subsequent BSP-memory stage restores Oracle's resource admission. The loader
reserves the exact decoded instance triangle total, and forced collision
composition returns freed host allocator buffers at BSP/checkpoint boundaries.
All 65 BSP fingerprints across the 15 campaign maps retain every ordered geometry,
normal/material/identity and navigation field. All 20 final runtime jobs satisfy
the serial 64 MiB measured ceiling and zero swap; maximum peak 64,765,952 bytes.
Armory completion/checkpoint and the three full multiplayer traces remain
identical. Oracle now reaches its genuine unsupported `ai_place` overload.

All 14 other campaign entries still require script/AI owners. Actual source-world
physical-bank startup remains above the budget; allocator reclamation alone did
not fix it. The active goal next needs shared actual backing/checkpoint ownership,
full birth/admission and campaign integration. No new mission completion or
launcher update is claimed. [BSP-memory results](../analysis/tags/gameplay-validation-20261014/campaign-bsp-memory/RESULTS.md).


The physical-backing stage removes the duplicate source copy of the actual host
physical bank. Memory/native addresses and platform protection now share one
retained page lease; checkpoint staging binds that lease into the replacement
before publication. Existing small memory/resource/actor regressions and all six
guarded CTest cases pass. Fresh consistently compiled engine/renderer cohorts
build under the separate 1 GiB, zero-swap compiler cap.

Actual English Xbox TU1.5 Lockout, Cairo and Armory map/world startup now pass
under the simulation ceiling. Lockout additionally births and binds its actual
source player/biped/weapon and completes 180 original controller/world ticks,
including forward movement. Both Armory authored routes and all three full native
multiplayer traces remain byte-identical; the 14 unfinished mission entries retain
their real required-owner failures. Across the 37 final serial jobs, the largest
measured peak is 63,827,968 bytes with zero swap.

Large real arena/world continuation remains unfinished: both tests hit contained
OOM in dense SourceRuntimeMemory::checkpoint copies. The arena requests a
51,265,536-byte copy; the world reaches a 3,611,948-byte copy after ten real ticks.
The next memory work must preserve full snapshot bytes/isolation, native owner
protections and quiescent publication within the existing limit. Actual campaign
actor birth/admission and all remaining authored mission/BSP/checkpoint paths
still require completion. Other chats' later root world/CMake changes were
preserved; current touched API consumers pass syntax checks, but no full current
root application/launcher or campaign-wide behavior/visual parity is claimed.
[Physical-backing results](../analysis/tags/gameplay-validation-20261014/campaign-physical-backing/RESULTS.md).


The snapshot-memory stage replaces large temporary source/arena-file buffers
with independent disk-backed byte snapshots, shared through a small storage
module. Exact byte/hole/protection/ownership behavior remains checked. The old
48 MiB restore and 40 MiB copy OOMs now pass; actual arena continuation passes
original startup, private files/cursors, repeated handoff, rejection/cancellation
and independent owner lifetime. Private physical snapshot mappings are adopted
without a second resident copy, and fresh native files preserve zero ranges as
sparse holes. A 52 MiB live-data adoption stress case fails before and passes
after the change under the same cap.

Current root storage configuration/build, all 12 guarded CTest cases and the
resource/file checks pass. Both Armory routes and all three full multiplayer
traces retain identical outputs. The full fresh engine/renderer cohorts build
under the separate 1 GiB compiler guard; native jobs remain serial under 64 MiB
measured with zero swap.

Real world continuation still hits contained OOM. Export and canceled preparation
now complete; a subsequent malformed-import preparation reaches the redundant
full-bank checkpoint used only for immutable-image comparison, increasing anon
RSS 53,772 -> 55,660 KiB before failure. The next memory work must compare immutable
regions exactly without that temporary snapshot, then finish the complete world
continuation test. The 14 remaining mission entries, actual campaign birth and
all authored mission/BSP/checkpoint paths remain unfinished. No current full root
app/launcher or campaign-wide behavior/visual parity is claimed.
[Snapshot-memory results](../analysis/tags/gameplay-validation-20261014/campaign-snapshot-memory/RESULTS.md).


The immutable-image admission stage removes the redundant whole live-bank
checkpoint from world import. Exact byte/metadata/topology comparisons now run
without heap allocation; all 22 legacy decisions match. The unchanged complete
arena and world continuation tests pass, including rollback, repeated restore,
salted identities, clock rebasing and every-byte equality over 60 later original
controller/physics ticks. All 13 current-root guarded storage checks and both
resource/file checks pass.

Across 36 final serial jobs, 22 pass and 14 retain genuine campaign-owner failures;
maximum measured RAM is 65,019,904 bytes with zero swap. Both Armory routes and all
three full multiplayer traces remain byte-identical. Actual campaign birth,
remaining script owners and every mission/BSP/checkpoint path are still unfinished.
Current concurrent world/AI changes were preserved. No full current application,
launcher delivery or campaign-wide console parity is claimed.
[Immutable-memory results](../analysis/tags/gameplay-validation-20261014/campaign-immutable-memory/RESULTS.md).


The kill-volume stage fixes a campaign tag-import omission: the existing decoder
now retains the authored kill-trigger list for every scenario. The prior importer
fails Cairo's list comparison; all 15 actual campaign maps and two multiplayer
maps pass afterward, preserving all 95 campaign kill rows. Current-root content
syntax and the independent module CMake build/guarded CTest pass.

The new borrowed SourceKillVolumeView matches 256 paired base/TU1.5 original
state/scheduling cases in both owned and source-bank views, including full bytes,
required containment calls and checkpoint rebinding. It uses one existing record;
HS argument/result and containment callees are declared in the original fixture.
Real geometry/fatal damage and Simulation HS integration remain unfinished.

Across 39 final serial reference/native jobs, 25 pass and 14 retain actual missing
mission-owner failures. Maximum measured RAM is 64,479,232 bytes with zero swap.
Armory completion/checkpoint and all three complete multiplayer traces remain
byte-identical. No new mission completion, current full application/launcher
delivery or original-console parity is claimed. The full campaign goal remains
active. [Kill-volume results](../analysis/tags/gameplay-validation-20261014/campaign-kill-volumes/RESULTS.md).

The actual campaign-world stage adds typed difficulty/options, original position
delivery and kill-volume controls to the retained source world. A proper Cairo
player now completes original geometry containment, disabled-volume checkpoint
restoration and fatal damage through its ordinary world tick. The reached team
table was missing; the world now invokes the existing complete original row-18
constructor and map reset. No body, health, death or containment result is
assigned as a replacement. Position inside the authored volume is a declared
diagnostic input rather than a natural traversal test.

The unchanged complete world continuation passes rollback, repeated restore,
salted identities, clock rebasing and every-byte equality across 60 later actual
controller/physics ticks. A fresh consistent current-source engine cohort and
rebuilt header consumers pass; native checks remain serial under the 64 MiB
measured ceiling with zero swap. Actual AI lifecycle, campaign Simulation/HS
wiring and all remaining mission/BSP/checkpoint paths are still unfinished.
No full application/launcher or console-parity claim is made. The full campaign
goal remains active. [Actual world results](../analysis/tags/gameplay-validation-20261014/campaign-live-owners/RESULTS.md).

The AI lifecycle stage replaces the manual allocation prefix with complete
original 1CFA40 and runs complete 1CFC90 after the source game options, clock and
engine reset. The readiness regression fails before and passes after the reset
call. The published Cairo player/geometry/checkpoint/fatal-damage test and the
full 60-tick world continuation pass with the original campaign AI frame enabled.
This fixture has no NPC actors and does not prove their birth or decisions.

Both Armory routes and all three full multiplayer traces remain byte-identical.
Across 21 final serial jobs, seven pass and 14 retain the same actual mission
owner failures. Maximum measured RAM is 64,569,344 bytes, with zero swap.
Published files pass current-root syntax checks while preserving six concurrent
header changes. Source campaign Simulation/HS integration, actual NPC birth and
all remaining mission/BSP/checkpoint paths stay open; the goal remains active.
[AI lifecycle results](../analysis/tags/gameplay-validation-20261014/campaign-ai-world-startup/RESULTS.md).

The private AI/HS integration proof now completes original counted placement of
Cairo's authored Johnson squad, validates actual actor/unit/command/HS-thread
bindings, restores whole authority with correctly staged pool views, and passes
an ordinary campaign AI/object tick. It recovers 55 complete original routines
and reuses three existing frame helpers without duplicate generated bodies.
The accepted serial job peaks at 62,914,560 bytes with zero swap.

This backend remains isolated from root pending a clean public interface and
broader validation. The legacy campaign runner still has 14 required-owner entry
failures; no new full mission, broad actor matrix or console-parity claim is made.
The full campaign goal remains active. [Private AI/HS proof](../analysis/tags/gameplay-validation-20261014/campaign-ai-script-entry/RESULTS.md).

The public campaign world stage now exposes original AI placement through a
readable typed interface. Its canonical backend contains 55 complete original
squad/command/HS routines and reuses three existing frame helpers. Actual Cairo
counted, authored and packed starting-location placement passes actor/unit/
command/HS-thread binding, whole authority restoration and bounded ordinary
AI/object ticks (3/1/1). The original overload comparison covers 256 cases.

Campaign kill-volume/fatal-damage, full 60-tick world continuation and the
180-tick multiplayer world smoke test pass. Both Armory routes and all three
match traces remain byte-identical. Across 29 final serial jobs, 15 pass and
14 retain the same actual mission-entry failures. Maximum RAM is 64,131,072
bytes, below 64 MiB, with zero swap. A fresh internally consistent source cohort
builds; focused current-root syntax preserves 11 concurrent header changes.

The original HS frame scheduler and campaign frontend/player/object lifetime
integration remain unfinished. Thread creation and linkage do not prove full
command-script execution. All remaining mission/BSP/checkpoint/restart/death,
navigation, combat/hit-region and presentation paths stay open. The full campaign
goal remains active. [Public world results](../analysis/tags/gameplay-validation-20261014/campaign-world-api/RESULTS.md).
