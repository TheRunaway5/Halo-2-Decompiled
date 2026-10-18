# Cutscene source recovery and parity gates — 2026-10-18

The full opening-cinematic goal remains active. The native engine has not yet
passed complete byte-level source recovery, full behavioral comparison, final
Xbox presentation comparison or installed-runtime acceptance. This ledger keeps
the recovered scope and the missing scope explicit.

## Authority and reproducible recovery

The authority is the user's original English Xbox TU1.5 executable:
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The original opening map is build `02.09.27.09809`, SHA-256
`5093e2f8ef7f0df41c11c6621bac5785a36f78a1cff3b8b2bc6c14b561c02a28`.
Future MCC imports must be converted to the admitted TU1.5 contracts. MCC/Vista
presentation does not establish original Xbox visual or audio parity.

`tools/audit_cutscene_source_provenance_tu15.py` regenerates each selected module
into a fresh evidence directory using the pinned executable and its actual
generator. It records every recovered instruction's address and source bytes,
per-body byte counts/hashes, source/generator hashes, original call sites and
external owners. It preserves both raw generated-source equality and equality
after a narrowly verified identity lookup/dispatch conversion. That conversion
accepts only the two exact generator forms and proves every selected literal
maps to itself. Arithmetic, memory accesses, source labels and other text are
compared unchanged. The report always states
`complete_cutscene_engine_verified: false`.

| Selected module | Complete recovered bodies | Instructions | What this audit establishes |
| --- | ---: | ---: | --- |
| Cinematic controls | 11 | 320 | Reproducible storage/reset/start/stop/LOD source |
| Camera | 30 | 4,490 | Reproducible selected original camera CPU bodies |
| Screen | 4 | 513 | Reproducible selected original screen-control bodies |
| Emotions | 5 | 191 | Reproducible selected unit cinematic controls |
| Speech | 9 | 721 | Reproducible speech binding, curve sampling, normalization and lookup source |
| Shadow controls | 1 | 49 | Reproducible selected original shadow control |
| Completion | 13 | 773 | Reproducible progress/options/transition-request CPU bodies and actual path-search callee |
| Activation | 49 | 3,138 | Reproducible map/BSP activation, registered callbacks, simulation/statistics reset, seeded vector generation, renderer reset, sound-environment parameter/reset callers, variant normalization and shared191CF0 CPU body |
| Object sound | 6 | 332 | Reproducible selected object sound request/cleanup bodies; external audio owners remain open |
| Audio frame/listeners | 40 | 6,092 | Complete original128650 frame,129F10 listener, elapsed-time/gain ramp,54-class fades, voice processing, sound-driver wrappers and selected routing/SDK callers; actual device ownership remains required |
| DSP image/upload/scratch CPU | 19 | 782 | Complete object/bootstrap/resource preparation, upload/allocation, scratch constructor/release/table/copy/translation, EP program selector/three-page GP append bridge, pool-release wrapper and decoder bodies; full parent/device composition and live acknowledgement/return remain required |
| **Selected total** | **187** | **17,401** | **Recovery provenance for these bodies;63,056 original bytes** |

The fresh v559 audit passes all eleven selections at the current shared-checkout state and compares every emitted instruction line,
including missing/unexpected sites and changed arithmetic or FP bookkeeping. A
prior private speech mutation is rejected at the exact changed instruction110752. Private speech and activation RNG
constant mutations fail the comparison. A private EP descriptor-stride mutation
also fails at exact instruction38F64E (v539). The strict v536 attempt preserved
header-include drift; synchronizing that include in ten modules restored v537
without weakening helper-text equality. Regeneration alone does not test every path,
establish the correctness of external callees, close the engine call graph,
validate native host integration, or compare final audio/pixels. No completion
percentage is assigned to the full engine from this selected total.

Run each operation serially through the existing memory guard, with a fresh
output directory:

```sh
python3 tools/tag_match_memory_budget.py --output-root NEW_GUARD --timeout 240 -- \
  /tmp/halo2-fov-reference-env/bin/python tools/audit_cutscene_source_provenance_tu15.py \
  --module cinematic --module camera --module screen --module emotion \
  --module speech --module object-sound --module shadow-control --module completion --module activation --module audio-frame --module sound-dsp-image --output NEW_EVIDENCE
```

The environment must provide the original executable at the repository's pinned
`NEW_DEFAULT` location and the offline generation dependencies. The guard keeps
the existing 60 MiB hard limit, independently measured 64 MiB ceiling, zero swap,
exclusive source-test lock and host reserve. Builds retain their separate
serial 1 GiB/zero-swap contract; graphics recording retains 3 GiB/zero swap.

## Behavioral comparisons and remaining owners

| Area | Current bounded evidence | Remaining acceptance |
| --- | --- | --- |
| Performance | Clean visible v560 CPU/GPU/cadence probe; CPU simulation/memory work dominates samples. Unobserved scalar stores now omit synthetic destination reads; Clang/GCC v557/v558 pass the regression and158 original caller comparisons/196617 ordered stores. See `OPENING_CINEMATIC_PERFORMANCE.md` | Coherent current runtime, delivered before/after frame-pacing measurements, remaining CPU skinning/material/translation costs and full-cutscene acceptance |
| EP scratch software initializer | Original38F6F4 constructs its actual native SDK context, publishes19 EP SGEs, copies original programs through38F544/38DE59 and appends three actual EP-tail pages through38F5F3. Shared GP/EP bytes, publication identity, invalid SGE/high-word/count/base, atomic rejection and creating-thread ownership pass Clang/GCC v557/v558 | Full upstream allocation/PIO/device factory, EP instruction execution, interrupts/output clock, final mix and live-world integration |
| Authored dialogue requests | Unchanged Xbox HS execution and native VM agree for all 49 cue-thread relative request ticks | Full scene wake ordering, wall/device timing and replay/skip behavior |
| Native delivery | All 49 admitted on their original request tick; 47 consume PCM, 2 authored silent | Native deadline bridge and moving emitters now wired; two full runs still miss the original gap by1,344 /3,392 samples (28 /70.667ms) |
| Volume | Original literal gain banks and selected centered kernels checked for all 49 cues | Live21 spatial voices sampled;19 WORLD points move and8–9 gains vary. Original listener/emitter ownership, DSP and complete final mix remain unverified |
| Audio frame/listener writer | 262 unchanged-original caller cases;524 native comparisons,9053 ordered stores,68 external stops; retained banks/GP/XMM/defined flags match. Actual clock/pause/gain/class-fade CPU paths execute | Actual game-active producer, full audio-frame SDK dependencies, active native listeners and final spatial/DSP mix |
| Speech controls | 2,166 original curve cases; actual Prophet rendered vertices and Armorer WORLD nodes change; Chief retains authored absent channel | Every speaking character's visible GPU mouth motion and reference synchronization |
| Camera and screen presentation | Original position/direction samples and fractional track; v121/122 fade/HUD/letterbox fractional sampler and live same-tick changes | Remaining capped elements, original final overlay/capture pixels and actual cuts/transitions/DOF |
| Materials | All 13 cast placements, 157 part bindings and 54 shaders have no reported binding/lighting/tint mismatch; exact white-area symptom corrected | Original framebuffer/shader/shadow/effect parity for every shot |
| Completion | 47 original caller cases, each observed/unobserved: 94 native comparisons, 10,252 ordered stores, 16 external stops, complete retained banks/GP/XMM and defined arithmetic/direction flags match | Actual profiles, statistics, activation, transition owners, next-level load and gameplay |
| Campaign identity | Old setup fails with0/0; admitted path now passes original1A5B60/32C0A0 and delivers authored1/1. Original successor lookup passes the unsorted705/801 boundary and final-level NONE | Full original session admission and completion/loader sequence |
| Activation | 70 original caller cases /140 native comparisons,29939 ordered stores and70 external stops match retained banks/registers/flags. All21 original BSP-start callbacks return with a real renderer; full map startup passes rows36/43 and reaches the missing sound-driver binding at2260E8 with game-active0 | Complete54-callback new-map chain, real sound-driver/device activation, remaining owners and the actual readiness producer |
| Sound environment | Original128900/197230/2260E0 CPU callers recovered; absent/present environment and missing/declared plugin/device inputs compared without external success | Actual224EC0 driver activation,52D674 ownership, plugin lifecycle and native effect-data services |
| Driver diagnostics/teardown | Isolated complete224EC0 constructor passes the actual audio-settings query when bound to the existing EEPROM's non-key UserSettings; primary failure is preserved, and failed exit callbacks are reported without aborting | Frozen opening probe stops at boot-SMC import356; current sandbox source reaches383D88/KfRaiseIrql. Actual IRQ/APU/DSP ownership and complete cleanup/restart remain open |
| GP upload/scratch CPU | Nineteen complete bodies preserve782 instructions.79 original cases pass158 comparisons/196617 stores, including the EP program selector and metadata/three-page append bridge. Clang/GCC v557/v558 pass; missing metadata/wrong stride controls fail (v535/v539). Actual native constructors/allocation/resource preparation compose through388309's decoder, descriptor relocation and request store. Native GP consumes real201-SGE pages and clears request3 through its actual original header transfer. The complete declared-context native CPU call then returns:687 unique original addresses,187654 prefix stores plus eight return stores match separate unchanged original execution, including24 continuation addresses and full output/header/stack behavior. Clang/GCC reports match; descriptor/header/cleanup controls fail (v505–v515) | Complete parent factory and integration into the live owner. Initial GP registers/frame/timer and cooperative audit dispatch remain declared inputs, not authenticated MCPX reset or scheduling. Native world still requires a real live GP producer before publishing at38846E |
| Native GP pool pages | Actual native64 allocations share the descriptor-DMA namespace. Exact source ABI, locked access, original growth/free and context destructor pass Clang/GCC; four earlier owner controls fail | Native kernel/device scheduling, asynchronous DMA ownership and full descriptor/APU teardown. Actual native resources do not establish Xbox kernel or audible parity |
| Native GP scratch publication | Original CPU constructors publish an actual8-entry prefix; original38F5F3 appends three real EP-tail pages and the parent supplies a separate fourth page, followed by189 data SGEs. All823296 prepared-image bytes map to real retained pages; request3 remains until a consumer executes. Full growth/destructor composition executes403 original addresses. Clang/GCC v528/v529/v530 pass; shifted page, discarded count, ignored high-word, omitted EP metadata and wrong descriptor-stride controls fail (v447–v449/v535). The subsequent declared-context GP bootstrap is covered below | Complete upstream software-context/device producer, full peripheral execution, IRQ/timer/device/clock integration and live-world connection. This is a synchronous native resource view, not an MCPX register/status model |
| Native GP scratch DMA/consumer | Native transfer owner and actual201-SGE resource composition pass Clang/GCC. Original12495-instruction bootstrap clears the actual request, with exactly one changed scratch byte. All860160 retained P/X/Y/scratch bytes and99 processor-state words, cycles and instruction counts match the separate reference-DMA composition. The CPU's subsequent return also matches unchanged original execution. Omitted output/EOL controls fail. A1024-frame all-bus run matches under an explicit reference format2 declaration (v498/v500/v502/v511/v513) | The first comparison v484 retains2590 scratch-byte differences between reference and documented format2 placement; default native behavior rejects that unresolved format. Declared initial registers/frame/timer/dispatch do not certify MCPX hardware reset, scheduling, registers, interrupts or clocks. Live audio connection and complete device lifetime remain open |
| DSP image/GP initialization | Native executable-only loader matches all28768 resource/15684 uploaded program bytes. Native persistent GP execution matches declared reference-model startup/request,15-effect initialization and2064 output frames, including every retained output/final PRAM/X/Y/scratch byte. Compiled blocks match native single-step state/counters; warmed mean90–91us/p99 120–123us fits the modeled667us interval (v358). Expanded unchanged-program reference audit v367 retains raw modifier/bookkeeping/status/stack differences and fails full state equality | Actual MCPX register/status/timing evidence; complete upstream factory and device/IRQ/DMA semantics beyond the verified transfer domain, routing, host/EP output clock, checkpoint lifetime and audible parity. Cold compilation costs630–665ms and needs real startup integration. The MIT instruction target is not connected to live audio; the GPL reference model remains analysis-only |
| Native palette | Exact1024 authored color bytes in four real native stage resources;11 rejection controls, cold API regression, stable baseline/control images, changed actual P8 GPU image and retirement rejection pass on RTX5080 | Full original SDK/driver behavior and final Xbox framebuffer parity; Vulkan validation layer is unavailable and disabled in this probe |
| Delivery | Diagnostic all-13-scene replay with an explicitly verified X11 capture route | Coherent current-source build/install, visible corrected playback and finished video |

Completion differential coverage is 258 executed instruction addresses within
the 773 recovered instructions. External stops supply no result and do not
claim execution beyond the stop. Omitting the original player-option transfer
fails the retained-byte comparison. See
[`campaign_completion_tu15.md`](../tests/fixtures/campaign_completion_tu15.md)
for reproduction and the declared input boundary.

Original `game_won` evaluator `2B3710` calls `13B3C0`, which updates progress
through `193257`, emits completion statistics through `12E490`, then reaches
`191750`. It uses the admitted campaign/map IDs and original `1A5C80` selector
plus `1A5AB0` record lookup, rather than a hard-coded next filename. `191580`
queues the real options for the transition owner. The shared live world can
dispatch the newly recovered bodies, with absent owners still stopping at
their complete caller ABI. The existing native HS `game_won` producer still
sets `level_finished_`, and Main exits; that producer/loader integration remains
an explicit gap. Compiling the bodies does not fix that gap by itself.

The upstream activation gate is now explicit: original191DF0 stores game-active1
only after original11E8E0 returns true. Native startup currently leaves that
byte0. The actual isolated BSP-start probe now executes all21 original callbacks,
including authored object placement and real object-memory compaction. Its
13A820 return value is not a Boolean readiness result. Native physics storage
actually moves during compaction; the probe retains that source-produced pointer.

15B68 calls SDK4025B0 to bind a palette. The declared native API owner uses the
live RendererPaletteFactory and actual GPU resources, validates the complete
caller and stage order, and preserves the original stack cleanup. It is not a
no-op or original SDK instruction recovery. A headless world has no such owner
and still stops at that call. The separate cold API route also passes its
regression. A zero-instruction `.text` trace of4025B0 is not a recovered body:
the SDK entry lies outside that section.

Map startup now runs the original registered1A5410 pool constructor and the
existing complete1A59D0 reset owner. Removing a duplicate partial owner from
the activation module allows the full reset's recovered callees to execute.
The original physical-arena, simulation/statistics, seeded vector generation
and renderer handle reset now run. Existing complete constructors171E10 and228260
are now called through their original render-object owner. Their actual arena
allocations, CRC updates and clears fix the row36/43 missing-storage failures.
These two bodies and their integer CRC helpers reproduce exactly. The broader
render-object module has unrelated differences in unused floating-point helpers;
this constructor check does not certify that whole module.

The next actual stop is2260E8's write through the absent52D674 sound-driver
binding. The current live sound-storage prefix explicitly omits complete224EC0
DirectSound activation. Its real driver/device/plugin lifecycle must be connected;
an allocated placeholder or a successful callback result would not close this gap.
SDK3856ED was traced in its actual DSOUND section, but is not counted as compiled
SDK recovery. Both startup probes retain game-active0, with no supplied ready flag
or external success. The incomplete full-startup probe is not used
by ordinary campaign construction. See the
[activation fixture boundary](../tests/fixtures/campaign_activation_tu15.md).

The separate `sound-start` probe invokes complete224EC0 with the actual authored
455B20 policy. With the real saved-settings resource, it passes query9 and stops
at33C2A2 reading unresolved import356, `HalBootSMCVideoMode`. This uses only the
EEPROM's immutable non-key UserSettings; it does not establish a native AV/SMBus
producer. Observed mode1 or initial7 must not be supplied as host defaults.
This is historical evidence from the opening-owned frozen cohort. The current
shared sandbox source has since advanced through boot-SMC into the SDK/APU
reset graph and stops at383D88 calling import41B7AC, ordinal160 KfRaiseIrql.
Actual IRQL/IRQ/PIC/APU/DSP/DMA owners remain open; see
[SANDBOX_TU15_PARITY_GOAL.md](SANDBOX_TU15_PARITY_GOAL.md). The old frozen stop
must not be presented as the current root frontier.

The first failed driver attempt also exposed a native teardown abort: faulted
SDK resources reject registered exit calls, and their exceptions escaped a
destructor. Exit registration teardown now catches and reports those failures,
keeps the world faulted, and preserves the initiating error. The regression
changes SIGABRT6 to failure exit1; it does not certify that skipped source exit
callbacks completed. The successful renderer-backed BSP probe still returns0
with no exit-callback failures. No sound initialization success is supplied.

## Native audio integration and source regeneration — v110–v118

The native mixer now refreshes a spatial voice from its original full salted
speaker's completed WORLD marker. It preserves the selected PCM, pitch, RNG,
first sample and source progress; an expired object cannot rebind a recycled
index. Actual Arbiter PCM changes from−3296 to−100 hundredths dB under declared
17.5→1 WU adapter inputs. The frozen-emitter control fails. Actual source
Chief/Armorer/Tartarus/Truth marker checks also observe an animated WORLD point.
This does not establish the original final listener, spatial mix or DSP.

Main now maps actual source ticks to the pause-preserving48kHz output clock,
with an observed epoch,1600 frames per30Hz tick, checkpoint re-anchoring and
restart invalidation. Selection/RNG remain synchronous with HSC admission.
No lookahead or late-PCM trimming is introduced. Real Prophet PCM and all49
original relative cue ticks pass the nonzero-epoch/pause/restore/late/cancel/
overflow regression; immediate playback fails. Both actual source Main and
adapter compile. All49 live requests are nevertheless late at submission.
Full v113b/v114 replay returns all13 scenes, admits49 cues, consumes47 PCM
voices and retains2 authored silent cues, with zero script/audio errors and
20 facial-capable control bindings. The remaining Prophet error varies28–70.667ms.
The store optimization alone does not establish producer timing parity.

The object-sound audit initially finds four x87 bookkeeping differences at
18C362/18C366/18C368/18C370, plus unused helper drift. Eighty declared normal
finite instruction-fragment cases run on actual host x86, using the original
16 instruction bytes and authored constant, and match both native variants for
C1/TOP/depth/defined comparison flags. The emulator's retained-C1 result is
rejected for this comparison; the first hardware attempt also lacked explicit
input flags and is retained as rejected evidence. These are diagnostic fragment
entry probes, not complete18C1F0 behavior or physical Xbox evidence. The module
is then regenerated from the pinned original bytes for reproducible source.
Unmodified current output compiles, and marker/timeline integration tests pass.
A private omitted-FABS control fails on the actual negative0.5 input,
confirming the finite-fragment comparison rejects wrong arithmetic. No new
gameplay or audible bug fix is claimed from that regeneration.

The latest diagnostic video is v114,198.688seconds, actual1280×720 H.264 plus
48kHz stereo AAC. Its module predates the provenance-only object-sound refresh.
It is not the requested finished video. A full current-source build/install,
visible corrected playback and final video remain required.

## Fractional screen presentation — v120–v125

`Presentation::effects(float)` samples the existing transition interval, using
the actual Xbox fade table and existing linear letterbox/HUD intervals. The
no-argument tick API is unchanged. Fraction0 retains its exact completed value;
fraction1 meets the next tick. Sampling changes no authoritative state, adds no
transition history or pending game action, and does not blend a new color or
immediate command with a departed fade. Main uses its existing consecutive-step
fraction/reset gate. This native presentation change is reusable across campaign
cutscenes; original setter/letterbox timing and final pixels still require proof.

The baseline and held-render controls fail for tick-stepped opacity. The new
regression passes1862 samples/266 exact boundaries, cut/replacement/restore/end
hold and invalid inputs. Actual current-root helper and Main snapshot compile.
The full immutable v122 graphics replay returns all13 scenes with49 admissions,
47 PCM voices,2 authored silent cues and no script/audio faults. It logs136 screen
samples,129 differences from tick-only values and6 changes at an unchanged game
tick/tick-level state. Logging precedes explicit bootstrap/awaiting-camera/end
masks. It is evidence of active interpolation, not matched Xbox framebuffer proof.
Audio still fails with a339968-sample Prophet gap (expected337600, +49.333ms).
No timing lead, source sleep, audio RNG or active DoF state was changed.

Static original frame CFG2B5B0 calls13B70 at2B6D2 after the view loop. Native
crossfade placement remains consistent. Static ordering is not whole outer-frame
execution or capture acceptance. The original opening's only authored DoF helper
is dormant with no static/wake reference; normal playback does not activate it.
Full DoF for other cutscenes remains a separate open engine path.

## Original audio frame/listener recovery — v126–v147

Complete128650/129F10 recovery authenticates2 CFGs/691 instructions/2822 bytes
and original frame calls12E16E/12E18C→128650 and12872E→129F10. The writer uses
registered local users and completed observer transforms, writes scale1 at
12A4DA, retains invalid-user bytes exactly, and reaches the genuine2271A0 SDK
owner only on the install path. Original game-active readiness remains required.
No scale, distance, sample lead or readiness adjustment is inferred from this.

The80 declared unchanged-original cases execute436 instruction addresses. The
reader matches160 observed/unobserved native comparisons,6032 ordered stores,
30 external stops,218 original director queries and48 original stack probes.
Held-listener output fails. The frozen actual-world probe runs both install
arguments through the new dispatch with game-active0 and preserves game/sound
storage. Exact current-root world and audio module compile. This is a CPU gate
check; active native listener startup and final mixing remain unverified.

The first new ten-module audit detects unused duration-helper/header drift in
the nine older modules, with no changed/missing/unexpected instruction lines.
The generator now emits that helper/header only where referenced, preserving
the original source comparison rather than relaxing it. The positive67-instruction
original21F590 generation keeps its actual helper and required layout declaration;
it also exposed a mistaken mnemonic assertion, corrected from FDIVRP to
Capstone's FDIVP for the authenticatedDE F9 bytes. No helper arithmetic changes.
The fresh strict v147 audit passes all128 selected bodies/11158 instructions.
A private wrong-register scale write fails at12A4DA. Earlier failed attempts
remain preserved. This is source recovery/dependency evidence, not whole-engine
closure or duration arithmetic verification.

The full v122 playback remains the latest visual/audio replay. Its2368-sample
Prophet error remains open. No new finished recording, installed build or
corrected visible cutscene is claimed from this CPU work.

## Connected audio-frame dependencies — v149–v165

The audio module now recovers24 complete CFGs/3388 instructions/12069 original
bytes. Added entries are authenticated directly from original frame/listener
CALLs, plus12964F→12E9A0. They include129500 elapsed-time/pause gain,228310's
54 class fades,225D70 voice bitmap export, per-voice processing and2271A0's
listener-to-driver cache/SDK handoff. Previously owned director, stack, clock
and other existing callees remain in their own modules. The83 external call
sites in this module are not counted as completed dependencies.

The initial generator rejects reached2221BB FIADD32. Its module-local helper
now executes actual FIADD and captures C1/exception flags before the extended
spill. The unchanged six original bytes execute on actual32-bit host x86 for
1092 normal finite cases. Native80-bit result and used status bits match under
three precision/four rounding modes; omitted addition fails. This is an
instruction-fragment proof, not whole221AF0 or physical Xbox behavior.

The262 unchanged-original caller cases execute1060 instruction addresses;
524 observed/unobserved native comparisons match9053 ordered stores,68 external
stops,228 actual director queries,56 stack probes and242 real33B590 clock-data
reads. They cover every local-user subset, class fades, multiword voice masks,
camera basis outputs before normalization, full frame/listener caller chains,
SDK cache paths, clock wrap/backward delta, pause countdowns/resume and every
12E9A0 gate. Held class fades and frozen clock gain fail at the correct banks.
No original instruction or callee return is patched. Declared SDK storage is
an offline CPU input, not native device construction or admission.

The promoted module and exact current-root world compile under the existing
strict-FP1GiB build bound. A concurrent world edit is detected after v115;
the fresh immutable v116 snapshot compiles and matches the current root again.
Current module/source hashes are recorded. The strict
v161 source audit passes150 selected complete bodies/13855 instructions. CTest
now has all262 byte-identical local fixtures; the prior80 are preserved, and
the canonical-path run passes. The prior actual cold-world probe retains0. No Main
audio-frame bypass, gain scale, deadline lead, source sleep, PCM trim or driver
success was added. The latest full playback remains v122 with the same failed
2368-sample Prophet gap. Full active source audio, final spatial/DSP mix,
rendering/visible playback, coherent installation and finished video remain open.

## Evidence locations

- `analysis/opening-cinematic-20261015/dsp-gp-final-native-comparison-v329/report.json`
- `analysis/opening-cinematic-20261015/dsp-gp-owner-ctest-guard-v330/`
- `analysis/opening-cinematic-20261015/dsp-gp-vendor-final-audit-v331/report.json`
- `analysis/opening-cinematic-20261015/cutscene-source-v302/report.json`
- `analysis/opening-cinematic-20261015/dsp-gp-final-validation-v301/report.json`
- `analysis/opening-cinematic-20261015/dsp-gp-original-bootstrap-comparison-v297/report.json`
- `analysis/opening-cinematic-20261015/cutscene-provenance-nine-v118/report.json`
- `analysis/opening-cinematic-20261015/cutscene-provenance-ten-v147/report.json`
- `analysis/opening-cinematic-20261015/cutscene-provenance-ten-v161/report.json`
- `analysis/opening-cinematic-20261015/audio-connected-original-v164/report.json`
- `analysis/opening-cinematic-20261015/audio-connected-native-guard-v164/`
- `analysis/opening-cinematic-20261015/audio-fiadd-hardware-native-guard-v154/`
- `analysis/opening-cinematic-20261015/audio-canonical-fixture-guard-v165/`
- `analysis/opening-cinematic-20261015/current-engine-build-v1/owned-overlay-v115-audio-promoted/`
- `analysis/opening-cinematic-20261015/current-engine-build-v1/owned-overlay-v116-current-world/`
- `analysis/opening-cinematic-20261015/campaign-audio-original-v138/report.json`
- `analysis/opening-cinematic-20261015/campaign-audio-native-current-guard-v146/`
- `analysis/opening-cinematic-20261015/campaign-audio-cold-world-guard-v137/`
- `analysis/opening-cinematic-20261015/audio-provenance-mutation-result-v145/report.json`
- `analysis/opening-cinematic-20261015/duration-helper-emission-v144c/report.json`
- `analysis/opening-cinematic-20261015/screen-interpolation-replay-v122/screen-delivery-audit.json`
- `analysis/opening-cinematic-20261015/original-frame-order-v123/report.json`
- `analysis/opening-cinematic-20261015/current-engine-build-v1/owned-overlay-v106-screen-interpolation/`
- `analysis/opening-cinematic-20261015/timeline-emitter-replay-v113b/`
- `analysis/opening-cinematic-20261015/timeline-emitter-replay-v114/`
- `analysis/opening-cinematic-20261015/object-sound-x87-hardware-v117b/`
- `analysis/opening-cinematic-20261015/current-engine-build-v1/owned-overlay-v102-regenerated-object-sound/`
- `analysis/opening-cinematic-20261015/activation-environment-recovery-v106/`
- `analysis/opening-cinematic-20261015/activation-environment-original-v106/`
- `analysis/opening-cinematic-20261015/activation-environment-native-guard-v106/`
- `analysis/opening-cinematic-20261015/bsp-environment-graphics-v106/`
- `analysis/opening-cinematic-20261015/map-environment-native-guard-v106/`
- `analysis/opening-cinematic-20261015/row36-owner-comparison-guard-v104/`
- `analysis/opening-cinematic-20261015/delivery-evidence-v106/report.json`
- `analysis/opening-cinematic-20261015/sound-driver-settings-guard-v108b/`
- `analysis/opening-cinematic-20261015/sound-driver-retirement-guard-v109/`
- `analysis/opening-cinematic-20261015/bsp-retirement-graphics-v109/`
- `analysis/opening-cinematic-20261015/retirement-evidence-v109/report.json`
- `analysis/opening-cinematic-20261015/completion-path-recovery-v80/`
- `analysis/opening-cinematic-20261015/completion-path-original-v80/`
- `analysis/opening-cinematic-20261015/completion-path-native-guard-v80/`
- `analysis/opening-cinematic-20261015/campaign-identity-red-guard-v80/`
- `analysis/opening-cinematic-20261015/campaign-identity-green-guard-v80/`
- `analysis/opening-cinematic-20261015/completion-options-red-guard-v79/`
- `analysis/opening-cinematic-20261015/scalar-store-visual-pass-v75/`
- `analysis/opening-cinematic-20261015/world-phase-visual-pass-v74/`

The first monolithic completion-fixture oracle exceeded the unchanged memory
limit. The final oracle streams one case per file and passes within the same
limit. Earlier failures and diagnostic recordings remain preserved. The
immutable graphics cohort omits later concurrent root camera/effect/MCC edits;
its playback results do not prove a coherent whole-current-source installation.
