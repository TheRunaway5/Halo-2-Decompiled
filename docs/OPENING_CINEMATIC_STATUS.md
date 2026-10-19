# Opening campaign cinematic — 2026-10-15

**2026-10-18 NPC interpolation / camera cuts / Arbiter reply follow-up: active.**
The campaign render collector's previous pose was discarded after any draw
that spanned multiple simulation ticks. A render-specific history now retains
the actual endpoint ticks and samples at `current_tick - 1 + fraction`; it
does not replay an entire multi-tick interval in one fractional tick. The
separate simulation-tick history still rejects missing authoritative samples.
Placed cinematic characters and source actors use the render sampler. Removed,
hidden/unadmitted owners, timeline restore and BSP replacement discard history.
The focused v562 regression fails before this change and passes afterward,
covering alternating1/2/3-tick draws, same-tick views, bone lengths and rewind.
This is a render-only extension, not an additional original Xbox game update.

The actual source observer already supplies scripted campaign rendering.
`camera_set_animation[_relative]` now avoids also loading/decoding the unused
legacy presentation camera clip. Source camera calls and authored timing
remain intact. The actual source-camera fixture now counts legacy loader
calls. The coherent v586 build and visible acceptance are still pending;
removing this duplicate work does not prove every reported camera pause gone.

The serial1GiB build initially exhausted its memory limit compiling the20MB
generated physics-support translation unit (v565). Its2025 recovered function
bodies now compile in32 balanced translation units with shared helper state
and unchanged instruction bodies. A Clang compile of two actual units peaked
at352MiB (v570); cross-file calls and helper-local state match the monolithic
reference with Clang and GCC (v569). All32 units have compiled in v571.
This is a compiler-memory change, not additional recovered engine behavior.
The original v565 failure and these bounded checks remain recorded separately.
The v571 runtime build then hit its1GiB cap in the unrelated authority archive
serializer. Its reader and writer now instantiate in separate translation
units with unchanged public bodies and shared schema. Both compile at ClangO1
under the same cap (v578,14.8 seconds,972.3MiB peak). That compile-only project
registered no CTests, so its final runner status is a failure rather than a
functional pass. The real nested codec round-trip check is included in v579.
Both failed runtime builds removed their private source/build trees; neither
delivered a new executable. The first camera control v576 also exposed an
invalid test-only NONE literal (type45 truncated it to0xffff). It is corrected
to object type50 with short type7 for the flag. Actual controls are v580.

The v580 controls now pass: the corrected real00a/03b source-camera fixture
passes under60MiB hard/64MiB measured/zero swap; restoring the old duplicate
decode fails at the exact legacy-loader assertion. Its temporary binaries
were removed. This proves the removed work, not complete hitch elimination.
The v579 build passed the serializer and reached1402 objects, then exhausted
its1GiB cap in `multiplayer_runtime.cpp`. ClangO0 and a bounded GCC probe also
failed there. A ClangO1 compile under2GiB passes in11.75 seconds with
1276220KiB compiler peak (v585); unchanged headers can also compile via a PCH
within1GiB (v584). The earlier1GiB compiler cap was a build choice, not an
AGENTS.md requirement. The v586 serial delivery build uses2GiB/zero swap and
LLD; original-data comparison guards remain60/64MiB/zero swap. No source
routine optimization or authored clock was lowered to obtain that build.

The cinematic model resource now retains the immutable variant-name table
already decoded during construction. Each draw still uses the current source
variant index and the same negative-index/default and bounds behavior, without
reparsing the entire model definition. Its v579 source amendment was pinned
before both consumer translation units compiled; v586 includes it initially.
End-to-end CPU pacing, NPC interpolation in a visible runtime and the audible
Arbiter reply remain unverified/open.

The last visible v543 log requests Arbiter's `x01_0030_der` / `E9E60871` at
tick1113 and consumes its157850 PCM frames. Its emitter updates apply -3306
hundredths of a dB at a roughly17.5-unit camera distance, an audible acceptance
failure. Original request flag0800 affects admission and separate
direct/spatial routing; it is not by itself a proved final-volume bypass.
The earlier v175 original routing evidence still applies: one SDL output volume
cannot represent those separate paths. No guessed gain override is applied.
Original final DSP/output composition, the missing audible reply, full
audiovisual parity and the finished video remain open.

**2026-10-18 slideshow report: performance is first priority; unfinished.**
The visible diagnostic preview completed with status0, and its scratch was
removed. The user reports severe stutter, fade issues, missing Arbiter “Pillar
of Autumn” dialogue, flicker and planet/High Charity ordering problems. These
remain acceptance failures. CPU samples identify translated simulation/memory
validation, skinning and material work; the RTX5080 is the actual renderer.
The sampling probes include debugger pauses and do not establish clean FPS.

The native scalar-store helper now omits an unobserved destination-preimage
read that the original MOV does not perform. Actual write validation remains;
observed records retain their original before/write/observer sequence. The old
helper fails the new regression v553. Clang/GCC v557/v558 pass the scalar
regression, actual EP initializer and158 observed/unobserved comparisons of79
original caller cases/196617 ordered writes. A clean visible cadence/GPU probe
is v560. A coherent new runtime and before/after pacing measurements remain
required; no end-to-end performance improvement is claimed yet. Details are in
`docs/OPENING_CINEMATIC_PERFORMANCE.md`.
The clean visible60-second probe has23.45ms median preparation intervals,
66.49ms at95th percentile and408.45ms maximum. GPU utilization has17.5% median
and40% maximum; the game uses roughly one CPU core. This supports a CPU-side
bottleneck in that segment without proving sustained0.5FPS. Preparation logs
do not measure physical display scanout, and the executable remains unchanged.

The EP context is now constructed by original38F6F4 using actual native SDK
storage. Its19-page scratch table publishes through the two original EP stores;
original38F544 selects the six program blocks, and38F5F3 derives the actual
three-page GP append. Native EP/GP views share those pages, reject invalid
counts/bases/high words, preflight cross-page writes and reject foreign-thread
access. This is software/resource composition, not the upstream PIO factory,
EP processor or device clock. Fresh source regeneration v559 passes all eleven
selected modules:187 bodies/17401 instructions/63056 original bytes. The full
engine, audiovisual parity and finished video remain unverified.

**2026-10-18 EP scratch bridge and strict source refresh: still active.**
Original38F5F3 is now recovered from41 authenticated instructions/119 bytes.
It writes EP metadata and derives three GP descriptors from the last three
pages of the real19-page EP allocation. The separate391410 page completes the
original12-page GP prefix. The native test now executes that source producer
against actual allocations, preserving the existing823296-byte image, growth
and destructor checks; it reaches403 original instruction addresses. Complete
upstream EP/GP software contexts and device/PIO startup remain open.

The original-data suite preserves67 earlier cases and adds three bridge cases:
140 observed/unobserved comparisons and196590 ordered stores. Current-root
Clang/GCC v528 builds and nine registered checks per compiler v529/v530 pass.
Omitted metadata and wrong descriptor stride are rejected by both the original
comparison and native resource test (v535); strict provenance rejects the wrong
stride at38F64E (v539). Full module regeneration v537 passes all eleven selected
modules with186 complete bodies/17350 instructions/62898 original bytes. This
also includes the shared activation module's newly selected191CF0 body; the
inventory does not certify behavior beyond tested callers. Ten generated files
needed the current generator's header include; all emitted instruction lines
and other helper text remain compared without a broader normalization rule.
All eleven selected translation units also compile independently with both
Clang and GCC under the strict FP flags (22 successful commands, v538).

Reproduction and declared input boundaries are in
`tests/fixtures/sound_gp_ep_append_bridge_tu15.md`. Full engine recovery, original
hardware/device timing, final mix, audiovisual parity, handoff, installation,
corrected visible playback and the finished video remain open. The live guard
before38846E remains; no request clear or readiness result is supplied.

**2026-10-18 original CPU upload wait and return: still active.** The native
upload now has a complete declared-context return comparison. At its actual
first poll, a cooperative audit dispatch runs the original GP against the
real201-SGE native pages. Its own header transfer clears request3; the original
CPU then evaluates its actual wait, publishes the output descriptor, copies
the six-word header and performs its original RET12 cleanup. The dispatch is
an explicit audit interleaving, not a recovered hardware scheduler or clock.

Independent execution is split into original-x86 prefix, original GP with
unchanged reference DMA, and original-x86 continuation. Every CPU opcode is
authenticated against the owned XBE; committed-range and payload hashes are
checked. The GP bus write is actual recorded output, not a supplied request
clear or callee result. Original prefix v506 retains the same187654 ordered
stores as before. Original continuation v508 executes24 additional addresses
and eight ordered stores. Native complete composition executes687 unique CPU
addresses and matches return memory, GP/XMM, defined flags, floating-point/
executor state and ordered stores. All860160 GP P/X/Y/scratch output bytes and
99 processor-state words match the independent composition (v511).

Current-root Clang/GCC v510/v512 and six registered tests per compiler v513
pass. Their complete CPU-return reports, GP state and output bytes match.
Omitted descriptor, omitted header copy and wrong RET12 cleanup controls fail
after the real GP has cleared the real request (v514/v515). Builds retain
serial1GiB/zero swap; validation retains60/64MiB/zero swap. Reproduction and
the declared-input boundary are in
`tests/fixtures/sound_gp_upload_return_tu15.md`.

This closes selected CPU return behavior under the declared GP startup/frame/
timer context. It does not recover the complete parent APU factory or physical
reset/format2/register/IRQ/timer/FIFO/clock behavior. The source body inventory
remains184, with no new original instructions emitted this turn. The live
guard before38846E remains. Final mix/sample timing, audiovisual parity,
handoff, installation, corrected visible playback and the finished video
remain open; the full engine is not certified byte-for-byte recovered.

**2026-10-18 native scratch DMA and actual GP upload consumption: still
active.** The new native transfer owner executes the original program's
observed scratch packet domain against actual GP RAM and caller-owned scratch.
Source STOP/FREEZE/UNFREEZE/START, linked transfers, circular cursors, completion
and ownership/fault rejection are explicit. It supplies no request clear or
processor-ready result. The unchanged reference implementation stays
analysis-only and is not linked into the game.

The actual native CPU constructors/upload still match the original suffix's
187654 ordered stores at the first poll. The composed GP now consumes those
real201-SGE native pages. Its12495 original instructions/12974 core cycles
clear the actual request3 themselves through the original P11C header transfer,
stopping the host audit at P11E. Exactly one scratch byte changes. Initial
processor state and one frame/timer event remain explicitly declared
reference-model inputs; source upload return and physical timing are not
claimed. No frame event means the original wait does not complete (v491).

Independent v498 comparison matches all860160 actual retained P/X/Y/scratch
bytes and all99 captured processor-state words, plus cycles/instruction counts.
The offline upload pair v495 also matches74 peripheral events. The separate
1024-frame all-bus experiment matches5054464 output bytes/406296 state bytes
only under an explicitly declared reference format2 layout. The first v484
comparison instead retains2590 differing scratch bytes: public hardware notes
and the pinned reference model disagree on24-bit format2 byte placement.
The default native owner rejects that format; no layout is silently selected.
The prior reference-core ISA-state discrepancies v367 also remain unresolved.

Current-root Clang/CMake v492/v497 and its three registered tests v500 pass.
GCC v499 and four registered tests v502 pass; its complete actual GP output and
state match Clang. Private omitted scratch-output and omitted EOL controls
fail both the owner test and the real SGE/GP composition (v501/v502). Builds
retain serial1GiB/zero swap; validation retains60/64MiB/zero swap.
Reproduction and the remaining evidence boundaries are in
`tests/fixtures/sound_gp_dma_tu15.md`.

This explicit owner is not connected to live audio. Complete parent/device
startup, authenticated format2/reset/register/IRQ/timer/FIFO semantics, GP/EP/
output clocks, source upload return, final routing/mix/sample timing, audiovisual
acceptance, handoff, installation, corrected visible playback and the finished
video remain open. The live guard before38846E remains in place. The complete
cutscene engine is not certified as byte-for-byte recovered.

**2026-10-18 original CPU image upload through its first GP poll: still
active.** The module now owns the actual388275/388294/38850A/388576 object,
bootstrap, append and resource-preparation callers:114 additional original
instructions/321 bytes, with all576 previous instruction lines unchanged.
Actual native SDK allocations, the retained original DSPImage and original
constructors now reach388309's decoder/relocation/request publication path.
The caller-provided root extent and four upstream page descriptors remain
declared resource inputs; the complete parent APU factory is not established.

The original constructor copies1484 bootstrap bytes, including eight bytes
beyond the1476-byte control-program extent. Source preparation clears the
header before decoding; the upload writes request3 itself. The native prefix
exports its actual388383 caller/resources as declared inputs to unchanged
Xbox instructions. That independent suffix executes189 original instruction
addresses and187654 ordered stores before the first poll. Clang and GCC native
composition match its complete retained ranges, GP/XMM, defined flags and store
digest. The native path executes663 original instruction addresses and retains
its actual descriptor table,201 SGEs and request3; no acknowledgement is supplied.

The prior module fails the missing initializer (v460). Shortened bootstrap,
wrong relocation and omitted request-publication controls fail (v471–v473).
The extended ordinary oracle adds five caller cases; all67 cases pass134 native
comparisons/196479 ordered stores. Current-root Clang/CMake v474 and seven
registered source/resource tests v475 pass. Frozen GCC v469/v470 passes the
full upload comparison. Fresh strict regeneration v467 reproduces all eleven
selections:184 complete bodies/17264 instructions/62633 original bytes.
Reproduction and declared-input boundaries are in
`tests/fixtures/sound_gp_upload_cpu_tu15.md`.

The native core still requires an MCPX peripheral/transfer owner and the real
GP consumer, device/IRQ/timer/GP/EP/output clocks. The live world retains its
guard before38846E. Upload acknowledgement/return, complete parent factory,
final routing/mix/reference sample timing, audiovisual parity, handoff,
installation, corrected visible playback and the finished video remain open.

**2026-10-18 native GP table publication and retained scratch view: still
active.** The original GP table constructors now publish into a real native
resource owner. Original factory requests are C000 prefix bytes and1060 table
bytes; the516-entry reservation initially leaves eight prefix SGEs. The
composed probe appends four actual native page descriptors through38DE07,
then allocates/locks189 data pages and publishes201 entries through38DF31.
Those four upstream descriptors remain declared resource inputs, not the
complete device/factory producer.

The source-produced SGE view matches all823296 bytes of the prepared immutable
Xbox boot image after original38DE59 copies. The request word remains3.
Original growth publishes190 new pages and unlocks/frees the old allocation;
original38DD26 also retires the current pool and clears its pointer/statistics.
The final native composition executes356 original instruction addresses and
uses actual retained pages on PipeWire. It supplies no processor acknowledgement.

Native probe v435/v436 rejected an extra MMIO preimage read made by the generic
store helper. Exactly four GP table stores now use a typed native resource
transition. Ordinary-memory byte observers retain their existing behavior;
no unknown MMIO preimage or synthetic register bank is supplied. The complete
original3879B3 release wrapper adds five instructions/24 bytes. Fresh v445
regeneration passes all eleven current selections:180 complete bodies,17150
instructions and62312 original bytes. The62-case CPU suite passes124 native
comparisons/189731 ordered stores/34 external stops (v440/v442).

Current-root Clang/CMake v450 and all five registered tests v451 pass. Frozen
GCC v444/v446 also passes including the destructor. Wrong prefix translation,
discarded publication and ignored SGE high-word controls fail (v447–v449).
The recompiled existing descriptor-DMA regression also passes (v452/v453).
Serial builds retain1GiB/zero swap; tests retain60/64MiB/zero swap. See
`tests/fixtures/sound_gp_scratch_resources_tu15.md` for reproduction and scope.

The live world is not connected to this scratch view. The complete upstream
factory, actual388309 decode/descriptor-relocation/GP acknowledgement, GP
peripheral DMA execution, device/IRQ/timer/GP/EP/output clocks, final mix and
reference sample timing remain open. Visuals/interpolation, handoff,
installation, visible corrected playback and the finished video remain required.
The existing world guard before38846E remains in place.

**2026-10-18 native GP pages and scratch-table recovery: still active.** The
actual sound DMA resource owner now retains GP pool pages in the same native
allocation namespace. It owns the authenticated allocation/capacity,
lock/unlock, physical-address and release ABI, with strict bank/frame/thread
and lifetime checks. These are real native64-backed resources and logical
physical tokens, not Xbox kernel or host PCI/DMA equivalence.

Original38DE81 composes with the real owner:193536 original zero stores and
189 data-SGE pairs reach the real retained pages. It stops before38DF31's
table-limit MMIO write; its seven prefix entries remain explicitly
unconfigured. GCC and Clang pass; four private owner faults fail (v404–v411).
The existing original descriptor-DMA regression also passes (v412/v413).
The required36-byte caller footprint includes four original MOVSD reads;
intermediate32-byte fixture experiments are superseded, not a production fix.

The DSP module additionally recovers seven complete original scratch
constructor/release/table/copy/translation bodies:225 instructions/553 bytes.
All346 prior instruction lines remain unchanged. The new independent oracle
adds31 cases to the existing30; Clang/CMake v422/v423 and frozen GCC v425/v426
pass122 native comparisons and189729 ordered stores, with32 external stops.
Actual kernel calls stop before execution; MMIO is a declared write-only store
sink. No kernel return, device read, ready flag or GP acknowledgement is supplied.
The earlier module fails the new cases (v420), and wrong descriptor stride and
omitted trailing-copy controls fail retained-byte checks (v427/v428).

Fresh strict provenance v424 reproduces all eleven tracked modules at their
current shared-checkout state:179 unique complete bodies,17145 instructions,
62288 original bytes. This is selected recovery scope, not the full engine.
The complete prefix/resource factory, real GP table publication, device/IRQ/
DMA/GP/EP/output-clock composition, final mix, reference volume/sample timing,
visuals/interpolation, level handoff, installation, visible corrected playback
and the finished video remain open. Reproduction is in
`tests/fixtures/sound_gp_pool_tu15.md` and
`tests/fixtures/sound_dsp_image_tu15.md`.

**2026-10-18 original GP image upload CPU recovery: still active.** The existing
DSP image module now includes complete original388309 and38DE81, another204
instructions/589 original bytes. These preserve allocation/release requests,
descriptor relocation, the request3 publication and the original wait for the
GP to clear it. Native world dispatch recognizes the complete bodies through
the existing module. Its unowned GP producer remains explicit: execution stops
before38846E's request publication until a real scratch/DMA execution owner can
consume it. No request clear, platform success or source readiness is supplied.

The new standalone CMake target compiles the same production module and fixture
reader. Current-root Clang/CMake v382/v383 and frozen GCC v385/v387 pass60
native comparisons,188347 ordered stores and12 external stops. Thirty original
cases comprise the existing18 decoder cases and12 new declared-context
upload/allocation cases. The new cases execute68 instruction addresses and
cover retained storage, reservation failure and the first real allocation/free
boundaries. Descriptor relocation, physical translation and the later GP wait
are recovered source but are not behaviorally verified by these prefix tests.

The actual prior decoder-only owner fails the new fixture (v386/v390). Private
descriptor-size and page-shift errors also fail retained-byte comparisons
(v388/v389). Fresh strict provenance v384 reproduces all eleven selected
modules:171 unique complete bodies,16905 instructions and61686 original bytes.
These counts certify the named recovery scope, not the entire engine. Tests
retain60/64MiB and zero swap; serial builds retain1GiB and zero swap.
The actual current world-owner object also compiles with its CMake flags
(v391,666.7MiB peak/zero swap); this is not a full game link or live startup.
Reproduction and limits are in `tests/fixtures/sound_dsp_image_tu15.md`.

The real GP scratch/SGE allocation owner, SDK physical translation, completed
transfers, native GP/EP/output-clock lifecycle and final direct/spatial mix still
need composition. The live stereo adapter remains unchanged. Full reference
state, original volume/timing, visuals/interpolation, level handoff, coherent
installation, visible corrected playback and the finished video remain open.

**2026-10-17 expanded GP state audit: full state equality is not established.**
The owner now exposes read-only actual PC/counter, its complete64-slot core
bank,32 return-stack words and repeat fields. The bank includes emulator
bookkeeping; it is not a physical MCPX register dump or device checkpoint.
Both comparison tools can retain every idle's raw state as explicit LE32
words, including bootstrap, without comparing C struct padding.

The expanded unchanged-program reference audit v367 still matches every
output/final PRAM/X/Y/scratch byte in all three retained cases. It **fails**
full raw state equality. Silent/direct return stacks and repeat fields agree;
the all-bus case differs in SR.L and its saved stack copies for40 early frames.
The complete differing values remain in the report, with no masks, expected
byte substitutions or normalization. Native single-step and compiled-block
traces agree with each other, including these differences from the reference.

The actual original instructions identify two model boundaries. At P70,
`05F420 FFFFFF` writes M0 with SR15. The pinned reference truncates modifier
writes to16 bits; the native core retains24. At P1C2, `0C1D08` shifts
FF:FE6418:000000 left four without sign overflow; the reference helper sets L
from shifted-out sign bits, while native retains the prior L. The
[manufacturer manual](https://www.nxp.com/docs/en/reference-manual/DSP56300FM.pdf)
sections4.2/4.3.4 and5.4.1.2/ASL support the native ISA interpretation for these
declared cases. This is not independent proof of MCPX hardware behavior.
The other raw-slot difference is REP bookkeeping: the reference uses slot48
for LCSAVE, while the pinned native core uses slot43 for TEMP. These are
implementation layouts, not interchangeable physical-register indices.

Instruction probes v365/v366/v368 retain the exact sites, inputs and results.
Neither the original program nor either upstream core was changed to force
agreement. The new JSR stack regression rejects a private zero-stack observer
(v370/v374); owner tests pass in v369/v372. Final current-root CMake v373 and
both tests v375 pass, including the original MOVEC/ASL ISA cases and callback
upload/resume. Those checks use declared contexts. Exact MCPX
register/status/timing evidence remains required before a whole-engine claim.

**2026-10-17 authenticated native GP block execution: still active.** The native
owner now runs compiled blocks of the complete, authenticated original GP
program. Peripheral owners can yield immediately after an actual peripheral
instruction. Program uploads from a callback also yield before invalidating
translated code. Unknown programs and callback faults remain rejected; this
host scheduling mechanism supplies no source readiness or guest state.

Final comparison v358 matches the original interpreter's output and final
PRAM/X/Y/scratch bytes for16 silent,1024 direct-bus and1024 all-bus frames.
Compiled and single-step paths additionally match all64 scalar registers and
the actual DSP cycle counter at every completed idle, including bootstrap.
Current-root CMake v359 and both tests v360 pass, including callback program
upload/reload and exact continuation. Wrong-state, authentication-bypass and
ignored-stop controls v351–v353 fail. All241 upstream files remain byte equal
to the pinned MIT archive after the optimized dependency build (v361).

With the runtime core packages optimized separately from the large compiler
dependencies, warmed direct/all-bus processing averages90.2/91.2 microseconds
per32-sample frame; p99 is122.9/120.4 microseconds. Both are within the modeled
666.7-microsecond audio interval. The first compiled frame still costs630–665
milliseconds of CPU. These measured cold costs are retained, not trimmed or
hidden by rebasing a clock. Correct native startup must handle compilation
before device playback. Timings exclude16 explicitly declared warmup frames
and do not establish physical Xbox timing or real device deadlines.

The component is not connected to the live mixer. Native peripheral/device
ownership, original bus routing, EP/output-clock coupling and GP checkpoint
lifetime remain open. Complete visuals/interpolation, full source integration,
level handoff, installation, visible corrected playback and the finished video
also remain required. The older single-step performance limitation below is
historical; this checkpoint closes only warmed GP compute in the shared model.

**2026-10-17 persistent native GP instruction owner: still active.** The new
`SoundGpCpuTu15` owns actual X/Y/mix/program RAM and runs the original uploaded
words with the pinned MIT DSP56300 core. A private C bridge handles its C-only
ABI without changing upstream source. The native owner is now tested against
the original interpreter using the same XBE-derived boot input and declared
reference peripheral/DMA model. Every retained output and final PRAM/X/Y/scratch
byte matches for16 silent,1024 direct-bus and1024 all-bus frames. All15 effects
initialize;48910966 instructions,130868 scratch reads and80816 writes match.
No captured RAM, source callee return or readiness flag is supplied.

Current-root Clang/CMake and frozen GCC owner tests pass. A deliberately broken
C-bridge mix alias fails an actual DSP instruction read; ignoring a peripheral
fault also fails. The first alias test exercised only the host API and missed
the private mutation; that coverage gap is corrected, and its result remains
preserved. The native comparison is v317; final dependency/CMake/comparison
evidence is v319/v324/v329. Unit negative controls and CTest are v325–v327/v330.
All source validations retain60/64MiB and zero swap. The unmodified241-file
MIT archive comparison passes again after the dependency build in v331.

The separate Cargo stage builds within1GiB/zero swap. Nested CMake Cargo builds
exceed that limit, so the committed dependency helper and explicit prebuilt
CMake binding provide the validated workflow. The GP target is excluded from
ordinary builds until a real consumer links it. It requires actual peripheral
owners and an explicit initial register context; it supplies no MCPX reset,
timer, IRQ, DMA, FIFO or device-ready implementation. The GPL reference model
still links only into analysis executables. See
`tests/fixtures/sound_gp_cpu_tu15.md` for reproduction and remaining limits.

The live stereo adapter remains unchanged. Native device/peripheral ownership,
bus routing, EP/output-clock coupling, GP checkpoint lifetime and a faster frame
execution path remain required. Single-step processing is slower than real-time
audio on this host. Complete visuals/interpolation, full source integration,
level handoff, installation, visible corrected playback and the finished video
remain open. This is progress toward the full engine, not whole-engine parity.

**2026-10-17 executable-derived GP initialization: still active.** The native
`sound_gp_image_tu15` loader now authenticates the owned TU1.5 executable,
retains its original control program and initial data, and decodes the actual
seed table/all15 effects. Its28768 resource bytes match unchanged original CPU
execution; its15684 loaded program bytes match the authenticated live upload.
GCC and Clang comparisons pass, a private79-versus80 warmup error fails, and
the current-root CMake/CTest pair passes. The original resource request word
is3, not0; the loader preserves the source value and supplies no readiness.
See `tests/fixtures/sound_gp_image_tu15.md` for reproduction and scope.
The final current-root build/export/CTest check is v300/v301. Fresh strict
source regeneration v302 still matches all11 selected modules,169 complete
CPU bodies and16701 instructions; full-engine verification remains false.

The separate original-program boot probe now starts at reset PC0 with inputs
produced by that native loader. Captured X/Y/mix/scratch memory is no longer an
input. With declared zero-backed offline scratch and the existing pinned
emulator peripheral model, the original instructions clear X/Y, load their
program/data, initialize all15 effects and clear the request themselves.
Two instruction cores match every retained mix output and final PRAM/X/Y/scratch
byte across16 silent,1024 direct-bus and1024 all-bus frames, including their
three boots:48910966 instructions per core. Omitting only the original GP P98
request-clear instruction in a private input fails the post-execution request
check. Evidence is in `dsp-gp-original-bootstrap-comparison-v297` and
`dsp-gp-omit-clear-guard-v299`, under the unchanged60/64MiB/zero-swap bound.

This removes a capture dependency from image preparation and proves the tested
boot interval within the declared model. It does not implement the native
SDK allocation/relocation, IRQ/APU/device lifecycle, real peripheral timing,
voice routing, output clock or playback. The reference interpreters remain
analysis tools. The game gains only the C++ image loader. All existing audio,
visual, interpolation, handoff, installation, visible playback and finished
video gates remain open; the whole engine is not yet verified byte for byte.

**2026-10-17 native encoder stereo calculation: still active.** The new
`sound_dsp_encoder_stereo_tu15` helper implements the original stereo overlay's
coefficient setup, optional coefficient scale, five-channel multiply/accumulate,
convergent rounding, 24-bit limiting and FIFO 16-bit conversion. It matches100
original-program reference cases/102400 raw output and PCM values. A private
version that omits rounding fails by one raw bit on the isolated FL case.
The original EP boot, not a separately written expected mixer, produces each
fixture's first real FIFO0 emission. Silence, all six physical input rings,
signed scale values, clipping rails, rounding boundaries and random input are
covered. The helper is pure C++; no reference emulator or Rust dependency has
been added to the game.

The current-root CMake configuration and the actual new library/test target
build pass in `dsp-encoder-cmake-v278`. The configured test executable passes
under the60/64MiB/zero-swap guard in
`dsp-encoder-cmake-native-guard-v281`. Earlier frozen GCC comparisons and the
rounding control are in `dsp-encoder-native-guard-v276` and
`dsp-encoder-negative-guard-v277`; original fixture generation is in
`dsp-encoder-fixtures-v274`. Reproduction is documented in
`tests/fixtures/sound_dsp_encoder_stereo_tu15.md`.

The new original cold boot `dsp-live-original-v266` also retains the enabled
encoder's bootstrap program and77824-byte scratch image. Its19 populated
physical pages include the three actual GP/EP shared output pages. The next
entry permitted by the emulator's inclusive limit is unpopulated and is
recorded without dereferencing it. The current observer and
`dsp-encoder-capture-v280` authenticate these bytes, the immutable original EP
boot instructions and the shared page identities. Current PRAM contains
runtime counters, descriptors and loaded overlays; those are retained as read,
not silently normalized to immutable code.

In the captured default stereo path, separate original-program channel tests
establish the actual coefficient setup and show that the sixth/LFE ring is
not read by this overlay (`dsp-encoder-channel-sweep-v271`). These measurements
are not an excuse to replace the stateful GP effects by a stereo matrix. The
new helper covers a numerical interval, not the entire EP processor or its
metering/command state. Native audio routing, GP processing and output-clock
integration remain required before using it in cutscene playback. Full-runtime
volume/timing, visuals, installation, visible corrected playback and the
finished video remain unverified and unfinished.

**2026-10-17 complete GP-loop follow-up: still active.** The original1476-byte
GP control program is also present verbatim in the pinned TU1.5 executable.
The upload audit now authenticates all15684 loaded GP bytes: that control
program, all15 decoded effects, and the original final return. The latest
unchanged cold boot is `dsp-live-original-v258`; its upload/native-decoder
comparison passes in `dsp-sg-native-v261`.

The observer now captures the **actual APU scratch address space**, following
GPSADDR/GPSMAXSGE and201 physical scatter/gather entries. The819392 retained
bytes match the GP header and uploaded programs, and each effect's CPU-visible
delay allocation agrees with its actual mapped scratch interval. Reads are
stable. Physical reads use the debugger's
[documented memory mode](https://www.qemu.org/docs/master/system/gdb.html#examining-physical-memory),
which is restored afterward; no guest state is written. The earlier v249
scratch prefix was invalid: effect-allocation relocation does not describe
the separately allocated control/image area below scratch offset C000. A full
GP-loop probe exposed that mistake before integration. Its failure remains in
`dsp-full-frame-comparison-guard-v257`; v249 is explicitly annotated, and the
current audit rejects that mapping in `dsp-sg-negative-v262`. Its uploaded
program/X/Y/mix capture remains valid.

Two independent DSP instruction interpreters now run the unchanged control
loop continuously from idle PC2B back to PC2B, including the actual start-frame
interrupt consumer, all15 effects, reverb transfers and six-channel output
transfers. With declared offline initial registers and the same pinned xemu
DMA/peripheral model,1024 direct-bus frames and1024 all-bus frames match every
retained output and final X/Y/scratch byte. This covers48415064 instructions,
129648 scratch reads and80076 writes per interpreter, including output-buffer
wraparound. Evidence is in `dsp-full-frame-v255` and
`dsp-full-frame-comparison-v260`; the aggregate60/64MiB/zero-swap guard passes.
An earlier effect-call sweep covers every one of32 buses plus two2048-block
history cases, matching102461876 instructions per interpreter in
`dsp-full-comparison-v253`. That earlier interval does not read the invalid
scratch prefix; its shared peripheral model remains an explicit fixture bound.

The reference interpreter/DMA files are unchanged pinned xemu sources, used
only in analysis executables. They introduce no GPL game dependency. The
second interpreter is the pinned MIT DSP56300 toolkit, also still analysis
only. Their agreement does **not** verify the shared emulator DMA timing,
EP/host output, native PCM integration, cutscene sample timing or physical
Xbox audio. The native SDL adapter's direct/spatial routing discrepancy is
still open. No speculative gain adjustment has been applied. The remaining
visual, interpolation, installation, handoff and finished-video gates remain
unchanged.

**2026-10-17 live DSP upload verification: still active.** An isolated,
unchanged Xbox TU1.5 cold boot now confirms every recovered DSP effect payload
against actual GP program memory. All15 payloads/14204 bytes match, together
with the original unencrypted four-byte final `RTS` (14208 bytes for the
complete uploaded effect program). The original environment constructor returns
naturally; no original instruction, register, memory, readiness flag or callee
return is supplied. All15 effects initialize and repeated X/Y/mix-memory reads
remain identical while stopped. This is xemu0.8.136 at revision
`fc24584ce88f0915ad7f04775bb7712c2e3f49ee`, not a physical Xbox capture.

The reusable observer is `tools/capture_live_sound_dsp_tu15.py`. It locates
the effect program through the actual GP control program's `JSR` operand;
the CPU effect descriptors point eight bytes beyond each two-word `R5`
prologue. Those descriptor pointers must not be treated as program entries.
The capture retains the entire loaded control/effect program through the
original final return, instead of reading unallocated program RAM. The private
reference configuration enables `audio.use_dsp` so the installed emulator
finishes its DSP execution for each frame. This behavior is established by its
pinned [GP/EP execution source](https://github.com/xemu-project/xemu/blob/fc24584ce88f0915ad7f04775bb7712c2e3f49ee/hw/xbox/mcpx/apu/dsp/gp_ep.c).

`tools/audit_uploaded_sound_dsp_tu15.py` authenticates the original image,
observer, decoder fixtures, live regions, every effect and the original return
word. The combined v241 audit also passes36 native decoder comparisons and
188232 ordered stores. A deliberate uploaded-opcode bit change, with dependent
capture hashes refreshed, is rejected at the exact original GP byte address
(`FE83A5CC`, expected94/observed95). Evidence is in
`dsp-live-original-v237`, `dsp-upload-native-v241` and
`dsp-upload-negative-v240`. The fresh v242 eleven-module source audit still
reproduces169 unique complete CPU bodies/16701 instructions/61097 original
bytes. Both audits explicitly retain full-engine verification as false.

The analysis-only DSP56300 probe executes the unchanged direct-channel
intervals with declared inputs and retained live effect parameters. Across128
32-sample blocks/526952 instructions, all8192 direct input-to-scratch sample
comparisons match. The later output filter changes the signal over time;
its bus levels cannot be substituted directly for final audible levels.
This probe skips the reverb/DMA intervals and uses a declared bootstrap-derived
register ABI. It is **not** a complete original DSP frame or native PCM proof.
Its source, executable and output are in `dsp-direct-probe-v234`, with the
60/64MiB/zero-swap result in `dsp-direct-execution-guard-v236`. The pinned MIT
toolkit's emulator builds under Rust1.99 in `dsp-emulator-build-v229`, at
optimization0 inside the unchanged1GiB/zero-swap build bound. It remains an
analysis tool, not a new game dependency.

These changes strengthen byte-level verification; they do not fix or close
the outstanding audible-volume/timing, complete DSP/platform ownership,
shader/visual/interpolation, full current-source integration, handoff,
installation or finished visible playback/video gates. No runtime from this
task has been installed and no finished video has been delivered.

**2026-10-17 DSP decoder follow-up: still active.** Three complete original
DSOUND decoder bodies are now available through source-world dispatch. The
strict eleven-module audit reproduces169 bodies/16701 instructions/61097
original bytes. Unchanged original CPU execution decodes all15 embedded DSP
effect payloads (14204 bytes).18 declared calls, including an intermediate
instruction boundary, match36 native comparisons and188232 ordered stores.
The store digest retains every instruction/address/extent/before/written byte
in execution order; it avoids retaining a large Python trace in memory.

The intermediate boundary exposed a real translator defect: single-operand
`IMUL BL` used32-bit EAX/EDX multiplication instead of signed AL-to-AX semantics.
The returned decoded bytes alone passed because subsequent code consumes AL.
The original intermediate register comparison fails before the fix and passes
after it, including preserved upper EAX and EDX. Existing32-bit lowering stays
unchanged. Original routing and frame regressions still pass. Evidence is in
`dsp-decode-prefix-original-v201`, `dsp-prefix-native-before-guard-v203`,
`dsp-promoted-native-guard-v211`, `dsp-audio-routing-regression-guard-v212`,
`dsp-audio-frame-regression-guard-v213` and `cutscene-provenance-dsp-v208`.
The promoted decoder, reader and snapshot of current-root world dispatch
compile in `owned-overlay-v122-dsp-promoted`.

These are byte-buffer CPU calls. They supply no device, original allocation,
DMA, address relocation, DSP execution or platform return. Decoding the effect
programs opens their analysis; it does not prove final audio composition or
authorize substituting bus gains for audible gains. Full playback, shader/
visual parity, remaining interpolation, handoff, installation and the finished
visible cutscene/video are still open.

All15 decoded payloads and their original unrelocated parameter blocks are
retained in `dsp-effects-disasm-v215`. Static assembly uses the analysis-only
[DSP56300 toolkit](https://github.com/mborgerson/dsp56300) pinned to revision
`82fb25a6917a36ac66212ae31f02aab62211f979`; archive, executable, payload and
parameter hashes are recorded. That tool is not a game dependency. Static
address/data-flow observations do not establish live DSP state or audible
gains. The corresponding original SDK upload, parameter and bus-setter CFGs
are retained in `dsp-output-v186` through `dsp-output-v193`.

**2026-10-17 byte recovery follow-up: still active.** The strict source audit
now reproduces166 complete TU1.5 bodies/16559 instructions/60724 original bytes
across the ten selected modules. The audio module has40 complete bodies/6092
instructions, including the spatial-gain, channel-routing, stream-parameter and
mixer-control bodies. These source bodies are available through the existing
source-world audio dispatch. This remains selected CPU recovery, not complete
cutscene-engine or final audible/visual parity.

The new original-execution oracle uses the actual38 opening cinematic-dialogue
tags at three explicitly declared world distances. It executes unchanged
18CDD0 request construction to the submit boundary, complete12C5B0 spatial
gain,12CCC0 packing to its effect boundary, and complete2B92E0 channel routing.
It retains the map's actual snmx controls copied by the original constructor
before its first SDK call. No source availability, startup or SDK return is
supplied.685 original caller states match1370 native comparisons and44474
ordered stores, including all retained bytes, GP/XMM state and defined flags.
Omitting the cinematic routing write fails. The prior262 cases still pass524
comparisons/9053 ordered stores. Evidence is retained under
`scripted-routing-fixture-v175`, `scripted-routing-native-guard-v178`,
`scripted-routing-negative-guard-v179` and `audio-baseline-regression-guard-v180`.

This confirms an unresolved adapter discrepancy: cinematic dialogue retains
separate direct and spatial/effect gains, whereas the SDL adapter currently
applies distance gain to its single output volume. Original bins6/7 are stable
across these declared distances. Their final DSP/output composition is still
unverified; the bus values have not been substituted for audible output gains.
The native comparison build uses unchanged selected existing helper bodies
with narrowed test dispatch after the full live-support compile reached its
1GiB memory limit and was stopped. The complete current-source runtime build,
original DSP ownership, timing failures, remaining visuals/interpolation,
installation and finished visible playback/video remain open.

**2026-10-16 follow-up: still active; no finished video.** The visible v27
replay reached all13 scenes with zero script faults, but the user reported
missing/quiet dialogue, incorrect transitions, missing mouth motion, white
chamber surroundings and a dark obstructed-looking Halo shot. These are explicit
acceptance failures. The user also requests interpolation of every capped
presentation element, a shader sweep of every character, and byte-level audio
presence/volume/timing comparison with OG Xbox/TU1.5. The Pillar of Autumn line
is the Arbiter's `x01_0030_der`; the Prophet timing pair is `x01_0220_por` /
`x01_0230_pot`. All remain audio acceptance gates.

The latest v161 strict source audit passes150 complete TU1.5 bodies/13855
instructions across ten selected modules. The audio module now has24 complete
frame/listener/voice/class-gain/driver-wrapper bodies.262 unchanged-original
caller cases match524 native comparisons,9053 ordered stores and68 external
stops. Original director queries, stack probes, clock-data readers and pause
queries execute; no platform return or active-game flag is supplied. The actual
elapsed-time/gain ramp and54-class fades are covered; holding either fails.
The new FIADD32 helper matches1092 actual original-host32 instruction cases
for normal finite inputs across precision/rounding modes. This is not a full
caller/x87/SDK or physical Xbox proof.
The actual cold-world dispatch preserves game-active0. The original writer sets
listener scale1, so this supplies no justification for a volume/scale adjustment.
Held listener output and a wrong-register scale mutation fail their comparisons.
Generator dependency drift is corrected without relaxing source equality; the
actual duration body retains its required helper/declaration. These are bounded
CPU/source checks. Final mix, sample timing, complete engine coverage, rendering,
current-source installation and the finished visible playback/video remain open.

The v122 full diagnostic replay now includes fractional screen transitions.
Fade opacity samples the same original Xbox curve between game ticks; letterbox
and HUD opacity sample their existing transition intervals. Fraction0 preserves
the completed tick value, fraction1 meets the next completed tick, and immediate
commands/cuts remain immediate. Sampling is read-only and does not advance HS,
source animation, sound selection or any game clock. Main disables the extra
fraction at restore/BSP/reset discontinuities through its existing pairing gate.
This is native render presentation, not additional original screen-body recovery.

The new original-table regression passes1862 fractional samples and266 exact
boundaries, plus command replacement, end hold, immediate cuts, invalid fractions
and checkpoint restore. Both the actual old path and the explicit held-render
control fail. The matching helper/Main and a snapshot of current-root Main compile.
Full v122 playback returns all13 scenes, admits49 sounds, consumes47 PCM voices,
retains2 authored silent cues, and has no script/audio errors. Its136 sampled
screen outputs contain129 fractional differences and6 render changes at the same
unchanged game tick/tick-level values. These diagnostics precede explicit native
bootstrap/awaiting-camera/end masks; they do not establish final Xbox pixels.
The Prophet gap remains339968 rather than337600 samples (+2368/49.333ms).

An authenticated static TU1.5 frame-order check confirms2B5B0 calls13B70 after
its completed-view loop. Existing native crossfade placement is consistent with
that order; this does not execute all outer-frame branches or establish matched
captures. The original opening's DoF helper remains dormant and is not activated.
The new diagnostic recording is not a finished deliverable. Full visual/audio,
remaining capped elements, source activation and installed-runtime gates stay open.

The latest full v113b/v114 diagnostic runs return all13 scenes with zero
script/audio faults. Both admit49 requests at the original tick, consume47 PCM
voices and retain2 authored silent cues. Main now schedules original30Hz ticks
against the observed pause-preserving48kHz output epoch. It adds no lookahead,
changes no HSC sleeps and never trims late PCM. All49 requests are still late
at submission. The Prophet gap is338944 /340992 samples instead of337600:
28 /70.667ms too long. This remains a failed timing acceptance gate.

Moving spatial emitters are now refreshed from the full salted speaker's actual
WORLD marker while preserving its playing PCM/pitch/RNG/progress. Across the
full runs,21 spatial voices have138 /134 snapshots,19 speaker points move,
and9 /8 gains change. The actual Arbiter PCM regression passes and the frozen
emitter control fails. Exact original listener lifecycle, DSP and final volume
still require comparison. Nonzero timeline epochs, pause, restore, late PCM,
cancellation and overflow pass; immediate playback fails. These tests do not
supply missing source readiness or audio-driver success.

The original speech resource owner now loads38 actual facial curves. Unchanged
TU1.5 CPU execution supplies2166 sampler cases; the native module matches every
48-byte result. Real Prophet PCM progress drives original facial controls and
changes source-selected rendered vertices. The production bridge retains the
full salted speaker and actual sound/permutation, advances smoothing on game
ticks, and retires only the matching sound. It does not fabricate Xbox voices
or resource readiness. A missing original animation lookup exposed by the
final Armorer section is now compiled from its complete TU1.5 body.

Full v69/v71 playback observes nonzero controls for20 of21 bound speech cues.
The initially misidentified final cue (`ea2d08b8`, owner `e37e0037`, request5644)
is Chief's `c01_1050_mas`, object-name52. The original script assigns the three
preceding `qtm` cues to Armorer/master_guns, object-name53, and all three produce
controls. Live v71 records a valid Chief lifetime and PCM, with the original
facial channel remaining NONE. The separate actual Chief placement/graph/cue
test confirms that absent channel; the corrected Armorer test uses his actual
`ea2c08b7` cue after173 authored ticks and changes rendered WORLD nodes. The
Prophet still changes rendered vertices. All20 facial-capable bound cues are
therefore observed producing controls. This does not establish every character's
GPU mouth pixels, interpolation, final spatial volume/DSP or transitions.

The v71 phase profile near the Prophet pair measures a median29.908ms per
source simulation tick,7.150ms model preparation and2.310ms draw. Batching can
therefore delay immediate sound admission in wall time despite correct authored
tick spacing. No timing policy or original sleeps have been changed to conceal
this. The recorded v71 X11 surface is blank: SDL selected a different display
surface. The saved native PPM captures remain valid, but that video is invalid
as visual evidence. Explicit SDL_VIDEODRIVER=x11 corrects capture routing in
v74/v75. Actual captured v74 frames show the restored chamber and visible
Prophet mouth motion. This remains diagnostic evidence; no finished video or
installed current-source runtime is claimed. The fixed unobserved scalar-store
path preserves original reads/writes/exceptions and observer behavior while
removing per-store vector allocations. Its contract, all2166 original facial
curve cases and live Prophet vertex test pass. The v75 replay renders2786
frames; the prior v74 renders2164. A single replay's faster frame count and92ms
remaining pair error do not establish complete pacing or audio timing parity.

The new [source coverage ledger](CUTSCENE_TU15_SOURCE_COVERAGE.md) records150
complete recovered TU1.5 bodies /13855 instructions across ten selected
modules. The reproducible source audit passes and identifies a deliberate speech
constant mutation at instruction110752. The added object-sound module was
regenerated after80 normal finite instruction-fragment comparisons on actual
host x86 resolved emulator x87 flag drift; this is no complete sound-driver
or physical Xbox proof. New completion/progress/options/transition-request recovery
matches47 original caller cases in both native store paths:94 comparisons,
10252 ordered stores and16 explicit external-owner stops. The original path
search is also recovered. Campaign setup previously delivered0/0 instead of the
opening's authored campaign/map1/1; it now resolves the admitted scenario through
original1A5B60/32C0A0. The regression fails before and passes after, including the
unsorted705/801 successor-table boundary and final-level NONE. These checks do not
close every engine path. The native HS game_won producer still exits playback;
actual profile progress and next-scenario loader integration remain unfinished.

The upstream original map-active gate is also unfinished: native leaves0;
original191DF0 writes1 only after complete11E8E0 success. Newly recovered
activation/variant/BSP/simulation callbacks match70 original caller cases in140
native comparisons,29939 ordered stores and70 explicit external stops. All21
original BSP-start callbacks now return with a live renderer, including authored
object placement and actual allocator compaction. The native palette API at
15B68->4025B0 binds four real stage resources using the exact1024 authored bytes;
cold API regression,11 rejection controls, actual GPU image checks and retirement
rejection pass on RTX5080. This is a declared native SDK boundary; original
driver instruction and final framebuffer parity remain open. Vulkan validation
is disabled because its layer is not installed. Headless execution still stops
before the GPU owner. Full map startup now runs the original level-handle
constructor/reset, physical-arena callback, simulation/statistics reset, seeded
vector generation and renderer handle reset. Original171E10/228260 constructors
now fix the row36/43 missing-storage failures through real arena/CRC operations.
Sound-environment CPU128900/197230/2260E0 callers are recovered; startup now stops
at2260E8 writing through the absent52D674 driver binding. The live sound-storage
prefix omits complete224EC0 DirectSound activation, so real driver/device/plugin
ownership remains necessary. Game-active remains0. No ready flag or external success is
supplied. Ordinary campaign construction has not been switched to an incomplete
startup sequence.

A separate complete sound-driver probe now reaches the actual boot-SMC data
dependency after consuming the existing EEPROM's non-key audio settings. It
stops at33C2A2/unbound import356; no current AV/SMBus producer or native sound
readiness is asserted. Failed native exit callbacks now report their errors
without throwing from teardown, preserving the initiating driver failure
(failure exit1 replaces SIGABRT6). A successful real-renderer BSP run still
passes all21 callbacks with no teardown errors. Full source cleanup, skip/restart,
driver/device/DSP activation and final audio remain acceptance gates.

The earlier v58 full run measures actual first PCM consumption for all47 admitted
cues, including leading silence; the other2 are authored silent. It reaches
all13 scenes with zero script/audio faults. The Prophet requests still differ
by211 ticks, but their consumed gap is333824 samples instead of337600:
78.667ms too short in this run. The largest request-to-drain delay is41 ticks;
submission-to-consumption is at most1024 samples. This confirms variable native
producer/delivery jitter; the earlier v54 submission gap was156ms too long.
Neither measurement authorizes changing the original sleeps.

The shared SDL service now accepts explicit sample deadlines, records actual
first consumption and decoded-input progress, and has a pause-preserving48kHz
presentation clock. `MultiplayerAudio` exposes this same contract without
fabricating an Xbox voice/readiness record or moving RNG selection to a future
time. All49 explicit deadlines pass across three mixer partitions, and actual
Prophet PCM preserves the337600-sample gap through a separate pause. The old
immediate-start path fails these tests. Existing SDL stream/volume regressions
pass. The current-root adapter compiles, but the full graphics probe still uses
the immutable engine cohort. The request producer is now wired to these deadlines in v112; the live
requests still arrive late. Live timing parity remains a failed gate. Mouth motion,
live spatial volume and remaining GPU/transition gates remain open.

The unchanged TU1.5 HS runner/wake/sleep now supplies a reproducible fixture for
all49 authored cue threads. Native ScriptVm matches their wake-relative request
ticks. Arbiter's Pillar cue is100; the Prophet pair is279/490, a211-tick gap.
This does not measure scene wake ordering or audio-device delivery. A second
unchanged control/packet/driver oracle verifies all49 literal sound/playback/
class/scale banks, caller-gain bits and centered gain kernels with neutral class
fades.47 are above the admission floor and2 are authored silent. It does not
establish live spatial gain, Xbox DSP, decoded PCM equality or final mix parity.
The new reproducible tools are `oracle_opening_dialogue_timing_tu15.py` and
`oracle_opening_audio_gains_tu15.py`; their native comparisons run within the
unchanged60/64MiB source-test guard. An initial all-PCM adapter test exceeded that
budget and was replaced by the bounded CPU comparison, without increasing it.

Animated camera tracks previously ignored the render fraction. The shared
Presentation camera now samples the authored track between ticks without
advancing its script clock. A regression fails before the change, then passes
all269 boundaries of the moving01b shot, final hold, repeated sampling and a
cut to07. The four original camera position/direction samples still pass with
the fixed module. Other capped presentation elements remain open.

The v28 runtime material audit records13 character placements,157 part/LOD
bindings and54 shaders with no reported unresolved material binding. This is
selected-resource evidence, not pixel parity. A v29 comparison finds the script
renderer and retained original model contexts had different lighting: the HSC
lighting commands and bit24 object selector only updated native presentation.
The shared TU1.5 lighting bridge now updates the actual80-byte source bank and
metadata/selector bits. The regression first fails on the old simulation and
then passes all13 actual authored lighting scripts under the unchanged60/64MiB
RAM guard. In the v31 full playback, all11 observed character placements have
matching lighting and packed tint inputs; the two background Prophet councilors
seen in v28 still need follow-up coverage. This does not establish shader pixels,
shadow pass completeness, or per-part override equivalence.
The newer v54 run covers all13 character placements again,157 bindings and54
shaders, with zero reported lighting or packed-color mismatches and no binding
gaps. This includes both background Prophet councilors and15 Arbiter bindings.

The tick972 white-area screenshot was reproduced exactly. Passing computed BSP
admission to ModelView leaves this failure unchanged, so this pointer alone is
not its fix and has not been promoted to production. Original/native camera
position and direction agree at four samples across scenes01b/07 with their
actual authored anchors. The supplied chamber reference is retained as a layout
reference with platform/update unspecified. The private per-cluster sky v32
run finished all13 scenes, but its exact972 capture retains the white failure;
sky selection alone is ruled out. A bounded frozen972 GPU comparison now shows
that omitting all dynamic model submissions restores the tall walls and lower
stairs, while including them reproduces the white obstruction. The underlying
BSP is present. Group isolation identifies ordinary scenery's independent
volume/overlay/emissive materials reaching the generic opaque path. The shared
independent-material binder now retains those authored programs, declarations,
colors and blend states for ordinary model submissions. At exact972, the full
model render restores the chamber walls, lower stairs and light shafts:
476452 white obstruction pixels become0 with the same camera,FOV,5 character
draws,10224 dynamic vertices and113658 BSP vertices. Three literal material
bindings and the full-frame symptom regression pass. No model was omitted.
Other shots and complete TU1.5 framebuffer parity remain acceptance gates.

Audio v27 fixes the previous parked gameplay listener and placement-root emitter,
and consumes authored playback/scale/class/permutation gain through the shared
native SDL adapter. All49 cue requests were logged in v31,47 admitted, with no
native audio faults. Two authored silent cues are rejected. This is not proof of
all dialogue being audible or byte-exact Xbox mix/scheduling. The Arbiter's Pillar
line remains approximately33dB attenuated; queued and post-tick mouth-marker
points agree for that cue. Original source admission, attenuation, packet gain,
callback, speech-controller and listener paths are still under investigation.
Renderer-end event logs are not original request ticks: multiple game phases can
complete before native audio delivery. A timing differential must retain both.
The current diagnostic candidate records those separately. Authored sound
prediction now warms the actual scripted playback adapter rather than only the
separate legacy audio cache; prediction consumes neither RNG nor a packet/voice
or manual playback clock. The six real SDL volume checks still pass after
prediction. Full playback/delivery and remaining spatial volume are being checked.
The v54 pass reaches all13 scenes with0 script/audio faults in198.29 seconds
including startup,2624 rendered frames and9186304 native audioframes. It logs49
requests and47 admissions. The live Prophet request gap is exactly211ticks;
submission counters differ345088frames instead of337600, an extra156ms in
this run. Frame-end delivery remains a measured failure, despite correct HSC
requests. This recording is diagnostic and is not the requested finished video.

Current evidence and live job handles are in
[the latest continuation](../analysis/opening-cinematic-20261015/CONTINUATION_20261016_CAST_AUDIO.md).

**The goal is active and the opening is not finished.** The coherent current
source candidate now enters and returns from all13 authored scenes in order,
with zero script faults. It reaches the authored revert and `game_won` exit.
The original cinematic-shadow command, cascaded scene retirement and intro-panel
material fixes are included. The separate optimized graphics/audio run renders
1,482 frames in230.85 wall seconds including startup, and mixes9,187,328 native
audio frames. This proves sequence execution and an active audio consumer, not
smooth playback, synchronization, visible shadows, visual parity or next-level
handoff. No finished video has been delivered.

A diagnostic recording exposes opaque backdrop rectangles and a separate bright
cross-shaped artifact. Four actual matte paintings use only layer15 /
implementation0 `alpha_detail_masked`; stardust uses the independent
`sky_clouds_simple_transparent` pass. Those unlit scenery models previously used
the generic material path. Layer15-only scenery now consumes the existing source
collector, deferred WORLD pose, shader-LOD selection, original vertex/pixel
programs and authored RGBA blend. The four black matte boundaries disappear in
the first new GPU observation; the bright artifact remains. The full new run also enters/returns all13 scenes, with zero script faults,
1,491 rendered frames and9,185,280 mixed audio frames. Remaining rendering
owners still require acceptance. The separate tick450 diagnostic retains the
bright cross even when only static BSP vertices are omitted; that geometry span
is ruled out as its owner. Production BSP visibility was not changed. Further isolation rules out
stardust, the invisible fleet, named anchors and device machines as individual
omitted groups; retaining only the five principal shot objects removes the
artifact. Actual source admission for camera cluster27 (a zero-portal cluster)
returns21 objects. A new private probe uses that complete admission result,
restores all BSP/scenery and removes all tag-name omission rules: the cross is
absent. This is first-shot diagnostic evidence. Production integration still
needs the connected-room portal-mask producer and source parent/child render
presentation; no whole-opening visibility acceptance is inferred. Five actual material
bindings pass in both lighting classifications under the unchanged60/64MiB guard.

The authored `c01_intro_dof_01` script (147) is dormant and has no wake or static
call reference in this cache's165-script inventory. Its DOF command is therefore
not reached by normal opening playback. This is evidence about these original
tags; it does not implement DOF for other cutscenes or converted MCC tags.

The intended pipeline imports MCC data/tags and converts them to TU1.5
equivalents later. TU1.5 remains the behavior and visual authority. Current
Xbox-disc runs are reference diagnostics, not acceptance of converted MCC data.
This work does not replace or bypass that conversion pipeline.

Delivery also requires a video of the finished, uninterrupted opening with
audio. A muted diagnostic run or partial scene capture does not satisfy that
request. The recording remains pending until the full playback gates pass.

## Implemented and checked

* Release `time_code_reset` matches both complete original evaluators (base
  `2AB130`, TU1.5 `2AC980`): finish the HS call with void and no clock write.
* Relative scenery and unit animation retain actual graph/clip/event metadata,
  the original channel clock/completion rules, and full live anchor handles.
  Anchor node0 is composed before hierarchy evaluation. Rendering consumes the
  resulting WORLD nodes without applying placement twice. The retained source
  campaign now forwards starts/queries through original controllers; its
  original scripted-camera/PVS admission and its source timer now pass the
  actual-world scenery timing/checkpoint check recorded below.
* Cinematic primary, secondary and ambient lighting use the shared original
  math. Live object flags select independent per-object rendering constants.
  Deleted generations lose their flag. Campaign checkpoints copy lighting and
  bloom values instead of sharing mutable service state.
* Bloom threshold and brightness override the final threshold/strength after
  sky aggregation, only while cinematic and enabled. `-1` retains the source
  value. The actual BSP postprocessor consumes these controls.
* Native base-route object shader set/clear commands reach scene overrides and per-object shader
  inputs. Duration uses the original float frames-to-seconds-to-game-frequency
  conversion. The opening's bounded override set is covered; the original
  shared 32-slot eviction policy is not established by those native value tests.
  The later source bridge retains the original32-slot owner and renderer samples;
  381 complete-bank/body/query comparisons and actual Banshee interpolation,
  replacement, clear and coupled checkpoint tests pass.
* Screen start/stop and crossfades retain the original timed controls, output
  bank, float rounding, capture request and quantized pixel program. The Vulkan
  consumer retains a fixed640x480 BGRA8 image, uses point/clamp sampling and the
  original premultiplied blend, and runs after all views/UI and before the native
  display transform. History survives window resizing. Checkpoint restore
  preserves the current external render clock and GPU binding while restoring
  timed controls. Headless playback rejects a missing captured-frame consumer;
  DOF still lacks its required passes.
  The source campaign now stores commands in its real arena bank and executes
  whole original timed update, stop and capture acknowledgement. Its existing
  Vulkan consumer reads the source output.864 complete original bank comparisons,
  actual-world lifecycle/checkpoint repeat and the default campaign suite pass;
  the subsequent GPU run records an actual native copy acknowledgement.
* Native frame accounting now follows TU1.5 `149160`'s admitted solo/30 Hz/scale1
  branch. Cinematics admit up to 150 ticks per frame; ordinary solo admits two.
  The prior native 50 ms rendering clamp dropped cinematic ticks while audio
  advanced. Fractional ticks are checkpointed, paused leftovers reset, and a
  restore discards the remaining tick budget from the abandoned timeline.
  Network synchronization, speed ramps and the complete source render-frame
  clock chain are not established by this adapter. The selected source screen
  clock values are now published by the native bridge below.
  `13AA50` corrects accepted elapsed time when fewer ticks complete; the native
  timed-effect clock advances by that float result after game ticks, as in
  `13A9B0/12A30`. Commands see the previous published render clock. This clock
  adapter does not establish original audio/game/render synchronization.
* Native skip/save bootstrap stays covered until the authored camera/cinematic
  admission. Fades and letterbox bars now follow the completed campaign world,
  before the HUD. Previously ModelView drew the world over those effects.
* `objects_attach` aligns the child marker once, stores its canonical local
  frame relative to the selected parent node, and publishes its WORLD hierarchy.
  Parent retirement deletes native attached children; checkpoints retain the
  full handles and local frames. Three authored initial throne attachments,
  authored scene cleanup and checkpoint restoration pass. Fresh prophets retain
  their actual resident default channel and advance the original manager clock.
  The live test crosses its89-frame rounded loop while checking current parent
  poses and retained child frames, then cleans up and restores the earlier clock.
  This does not establish complete scene pixels or all controller branches.
  Multiple native children and live native region filtering retain their
  boundaries. The separate source bridge now executes complete original
  parenting, including lookup and spatial publication, as recorded below.
* Default-primary initialization uses actual normalized graph identifiers,
  clip/definition metadata, the shared source channel installer and ordinary
  manager/transition updates. Admission currently requires a fresh biped/vehicle,
  resident local unlinked type0 clip, authored flags0 and empty root-motion banks.
  Inherited controllers, linked variant/RNG selection, scenery named-idle
  overrides, old births and physical root motion keep their explicit boundaries.
  TU1.5 `11DD90` also confirms that an ordinary idle reports zero through the
  unit custom-time query; the native helper preserves that activity guard.
* Active world-object poses use model-order bind, primary sampling and matrix
  banks, as in TU1.5 `BF0C0`. Model/graph names can differ. Source raw integer
  rotation keys and grouped normalization are shared with the existing primary
  codec implementation; generic float-key conversion introduced a one-bit
  matrix difference in the first attempted path. Source resources track live
  and checkpoint clip ownership. The named frontend/first-person routes retain
  their existing binding behavior. Deferred masks, layers, type callbacks and
  the retained source-world controller bridge remain distinct owners.
* Vehicle/seat HUD preload admits resident definitions. The opening's Banshee
  gun HUD (`E9070792`) is an actual size-zero reference and must not prevent
  cinematic startup. The unit HUD is resident; an earlier unit-HUD hypothesis
  was disproved and its changes removed.
* Resident common material images transfer across BSP changes without retaining
  a separate full copy. Headless and rendered native errors print their current status
  and already-retained script diagnostics before propagating the exception.

Reference checks pass for 52 original animation/channel/seek/sampler/anchor
cases, 402 TU1.5 lighting cases (8,040 exact float words), 284 TU1.5 bloom cases
(51,120 program words and 1,420 blur radii), and 99 exact TU1.5 object-function
samples, 473 exact TU1.5 frame-clock samples across 1..120 fps schedules, and
80 exact TU1.5 attachment alignment/local-frame pairs. Crossfade checks add
408 original lifecycle actions, 298 complete60-word pixel programs and
60 original accepted-elapsed corrections. The complete default
campaign checkpoint suite passes, including cinematic state restore, fractional
clock restore, crossfade capture/checkpoint behavior, attachment-tree retirement
and generation retirement.

Initialized-controller reference checks pass for six complete144-byte manager
initializations,1,080 complete clock updates and12 ordered loop-variant requests.
The unchanged original probe covers all three prophets and the empty throne
manager with zero andA5 caller backing, through180 updates each. Its actual
opening metadata and admitted30Hz game clock are explicit inputs; the resource
subsystem retains original image defaults. These calls establish controller
fields and clock behavior, not complete resource startup or object birth.
The native resident Cache clip owns decoded resource data for the admitted branch;
the retained source-world resource/controller bridge remains separate.

The new world-node fixture executes the actual Pillar clip through unchanged
TU1.5 standalone sampler`2835B0`, then the declared post-sampling matrix intervals
`BF501..52B` and `BF53C..5B6/BF625`. Six actual frames and two explicit roots
produce156 exact native/original WORLD matrix words despite distinct authored
node names. These intervals exclude independent root/type/deferred-mask owners;
they do not establish whole object birth or physical Xbox reciprocal estimates.
`tools/oracle_campaign_world_nodes_tu15.py` reproduces the fixture. The separate
`tools/audit_campaign_primary_formats.py` audit admits the raw formats of all114
static object-animation requests (69 scenery and45 unit starts), covering actual
codecs1/3/4/5/7. That is asset-format coverage, not114 successful playback cases.

The crossfade oracle executes unchanged TU1.5 screen evaluators, `1FD00`,
`13B70`, `22050`, CRT power and `13AA50`. GPU/SDK submission boundaries are
recorded, not rasterized by the oracle. Original target initialization confirms
640x480 linear ARGB8/NV2A format0x12; the pinned local xemu format mapping is
BGRA8 UNORM. The separate production Vulkan test passes3,477,548 exact BGRA8
pixels on an RTX5080: capture/reuse/recapture, quantized blend, spatial
orientation, point sampling, odd window dimensions, retained history after
resize and inactive controls. It caught a center-row/column UV-rounding failure
before integer pixel-center sampling was installed. This is native GPU evidence,
not an original Xbox framebuffer comparison; no Vulkan validation layer was
installed. Commands, failure and passing outputs are retained in
`crossfade-gpu-v1/v2/v3` under the analysis directory.

The lighting/bloom captures execute unchanged TU1.5 code. Only GPU/SDK bloom
submission boundaries are intercepted; they are CPU program evidence. The
animation fixtures include earlier original-image evidence and do not certify
TU1.5 rendered animation. Test fixtures and generators are retained under
`tests/fixtures` and `tools/oracle_campaign_*.py`.

The attachment oracle executes unchanged TU1.5 `BAF10/BB300/BB3E0` arithmetic.
Marker lookup supplies explicit completed local/world matrices; `B9460`
publication is intercepted to capture alignment and retained local frames
before later spatial/physics owners. This does not prove complete object birth,
retirement or animation initialization. The separate actual-cache initial
attachment/checkpoint test is intentionally limited to constructor-time setup;
the long scene audit retains its later idle-clock failure.

The [fresh metadata-only idle inspection](../analysis/opening-cinematic-20261015/attachment-unit-v1/idle-metadata-v2/b2a1f85d-dd65-4222-80fe-89ea1b88aa28/memory-budget-runtime.log)
resolves all three prophets to graph `E6AA0535`, clip43, type0, 89 frames,
loop frame0, authored flags0, and no linked variants. The thrones resolve to
initialized idle NONE. The subsequent direct original manager/clock probes are
retained as `idle-manager-original-v1/v2.json` and
`idle-manager-clock-original-v1/v2.json`. The first probe records a missing game
clock; the next admits the same explicit30Hz clock as the frame-clock fixtures.
`tools/research_campaign_idle_manager_tu15.py` reproduces the final probe;
`tools/export_campaign_idle_fixtures.py` retains its controller outputs as test
data. Source-world startup/resource/event/root consumers outside the admitted
resident branch remain open.

## Marker-effect request and graph continuation

The shared script request builder now matches unchanged TU1.5
`2AA360 ->179700 ->1791C0`: NONE gating, the first marker-count check, the
second independent noninterpolated marker lookup, attached/force flags,
owner/region fields, controls/colors and both gravity/up seed records.
The original final damage-owner field is a signed short with zeroed padding.
Its first native full-word implementation failed the fixture and was corrected.
All192 cases now pass, including72 complete96-byte parameter/two28-byte seed
packets. `BAD60` supplies declared completed records and `178E30` captures the
request before allocation; these checks do not execute object lookup, effect
allocation, PVS, RNG, particle/light/lens consumers or pixels. This helper is
the independent typed request builder. The source-backed campaign host now
uses the complete compiled original wrapper instead, as described below.

The [actual static tag audit](../analysis/opening-cinematic-20261015/marker-effect-authoring-v1.json)
resolves both requests to `halo_exploding` (definition`E201008C`) and its
node0 marker`130008FD`, `explosion_poa_front`. `halo_destruction` (`EA03088E`)
has three particle systems followed by a direct lens part. The other graph,
`alpha_halo_explosion` (`EA070892`), has a two-second delay followed by three
lights; all three reference lens definitions. A particle-only preload cannot
establish either graph's complete playback.

`NativeWorldEffects::preload_complete` now rejects missing original parts,
light lenses, skipped environment/camera/flag owners and unresolved linked
decal resources, including reachable particle death graphs. Existing explicit
component delivery remains available through `preload_attached`. The actual
opening check retains both lens boundaries and admits the actual particle-only
`halo_venting_debris` control. Elongation's existing partial delivery still
passes180 ticks, per-view invariance, retirement and salted recreation: three
starts, two complete graphs and111 peak particles, with its lens boundary
explicit. Neither preload API establishes GPU/audio resources or consumption.

The [graph/component cohort](../analysis/opening-cinematic-20261015/script-effect-integration-v4/RESULTS.md)
pins source, objects, immutable archive inputs, commands and results. All three
checks pass together under the60MiB aggregate cap/64MiB measured ceiling and
zero swap. This is not a fresh native movie run or whole-checkout delivery.
The current shared lens request/query/draw stack uses `PhysicsLiveWorldTu15`;
campaign still needs its per-frame and per-view presentation integration,
query completion and draw wiring. Creating substitute lens requests would not
close that gate.

### Retained source allocations and cinematic lifecycle

`source_script_marker_effect_tu15` now calls whole original `2AA360` on the
retained campaign world. Three authenticated bodies contain131 instructions;
all192 original fixture cases pass through this compiled kernel as well as the
earlier typed builder. `BAD60` and `178E30` retain their full source ABI. The
live source-world test also executes their actual lookup and allocation owners
against the opening's authored scenery and marker, without supplying an
allocation, location, visibility, RNG or retirement result.

The first actual-world check found both allocations immediately retired.
Diagnostics established a valid actual marker and cluster27, but no cinematic
interface at `51DF50`. Original `17C5C0` consumes that interface's active byte;
force-on-creation alone does not replace it. The shared world now constructs
the original row53 interface (`13E9E0`), resets it (`13EA40`) after delivering
campaign options, and forwards cinematic start/stop (`13EAD0/13F560`) and
object LOD (`BDF70`) through complete original bodies. Those controls also own
the actual projectile cleanup, object/player/AI, render, sound and director
state. No cinematic byte or successful GPU result is forced by the adapter.

The original controls require renderer CPU storage. A native composition
initializes its query bank through whole `20E30`, allocates the actual0x34
interface through whole `126850`, and invokes whole `20E90`, after the model
and texture storage constructors. Six independent original fixtures match
the complete arena/query/CRC bytes and preserve the caller's graphics flag.
This composition does not claim the original suffix's ABI or ordered stores,
and leaves the actual graphics-initialized flag unset. The unchanged original
suffix comparison still passes separately.

The [retained-source cohort](../analysis/opening-cinematic-20261015/script-effect-runtime-integration-v7/RESULTS.md)
passes actual scenery birth, campaign defaults, LOD set/clear, both marker
effect allocations, cinematic stop and source checkpoint restoration. Original
deletion frees the light-only effect immediately; the particle effect detaches,
stops emitting and remains until its particles finish. A first regression
incorrectly demanded immediate deletion of both and failed. The final check
verifies the original deferred retirement, advances240 real source game/particle
phases, restores the earlier complete control/effect bank, and repeats that
lifetime:480 phases in total. All five focused commands pass serially with a
60MiB aggregate peak and zero swap. A separate fresh current-header/source
compilation passes for all ten owned translation units.

This is source allocation/lifecycle evidence, not visible explosions or a new
13-scene playback run. The native base route still rejects marker effects
without the retained TU1.5 owner. Source camera/activation and broader controller/attachment coverage,
per-view particles and light lenses, actual GPU completion, audio and full
authored playback remain open. The latest full-movie candidate and its five
faults below predate this source-backed increment; no launcher was installed.

### Retained source controllers and scripted camera

The campaign's scenery/unit starts now call complete original `10F710` and
`11DC00` on the retained world; native presentation borrows the resulting
source WORLD matrices. Original `10FFD0` supplies attachment admission and
executes the existing whole `BAF10` path, including actual marker lookup,
alignment, retained parenting and spatial/physics publication. Original
`10F510` supplies scenery time. Unit time uses whole `11DD90` and the original
evaluator's value projection;192 independent unchanged `2AD170` cases match
that float32/x87/clamp/short projection. Their declared query output does not
establish lookup or whole HS state. The expanded cinematic module retains
11 complete bodies and320 instructions.

The [source controller cohort](../analysis/opening-cinematic-20261015/source-controller-integration-v4/RESULTS.md)
passes actual authored relative scenery/unit starts, source WORLD movement,
Regret/throne attachment, the already-attached guard,120 source updates and
recursive source deletion. The unit's remaining query advances278 to218 over
60 game phases and its actual WORLD hierarchy moves. In that earlier cohort,
the Pillar advances173 to170 over60 phases; the camera integration below
repairs that observed scheduling failure. The default campaign
suite also passes. All three commands finish under the60MiB aggregate cap
with zero swap;13 current-header/source translation units compile separately.

Unchanged original `13B610` admits cinematic PVS through the separate camera
state (`4F5504`) and director query `16FD40`. The read-only store diagnostic observes
the Pillar advancing0/1/2/3 and losing its source header's active admission at
original `B9356` on tick3. Later ticks leave its frame at3. A fresh linked native
candidate then attempted normal opening startup with the TU1.5 source world;
the aggregate60MiB cgroup OOM-killed that probe before a completion report.
That earlier attempt is not successful VM startup or playback.

The [source camera cohort](../analysis/opening-cinematic-20261015/source-camera-integration-v3/RESULTS.md)
now forwards camera control, FOV and authored animated-camera requests to
complete original owners. Ten complete bodies/1,028 instructions retain actual
animation resources, flag composition, camera cluster lookup and all branches.
The original director decrement/FOV elapsed interval now runs after the existing
object/first-person phases and before the source game-clock increment. Native
camera-time projection matches192 unchanged whole evaluator results;192 bounded
original director-bank intervals also match. These value seams do not claim
complete surrounding frame, HS thread, ABI or floating-point status equivalence.

With actual authored scene03 camera admission, Pillar time now advances173
to113 over60 source phases, and camera time186 to126. The real camera matrix
and scenery WORLD hierarchy move; requested FOV storage, actual source BSP
cluster27, stop and checkpoint/repeat all pass. Another60 restored phases
produce byte-identical camera matrix/time and WORLD outputs. All three commands,
including the default campaign suite, pass under the aggregate60MiB hard cap
and64MiB measured ceiling with zero swap. Six current-header/source translation
units also compile. Native Presentation still supplies the rendered camera;
these checks do not prove source-camera GPU pixels or all controller modes.

The new normal600-tick headless command still encounters contained OOM. With
the current immutable interaction-clip sharing fix, private diagnostics confirm
source-world and Simulation construction, original player/biped birth and
startup script execution finish. A host-call trace locates the subsequent peak
at tick3 during authored `predict_structure_section` and `predict_model_section`
resource loading. No prediction or retained resource was removed to fit the cap.
The separate source-integrated graphics run and its crossfade failure are below.
No launcher was installed, and converted MCC acceptance remains open.

### Source screen-effect and additive animation continuation

The [source screen cohort](../analysis/opening-cinematic-20261015/source-screen-integration-v2/RESULTS.md)
now forwards start/reset and both crossfade commands into actual `515EC0` storage.
Whole `1FD00` updates its timed effects, `1FCE0` acknowledges the native renderer's
copy and `1FCB0` clears/stops the bank. Three complete bodies/482 instructions
retain all branches. Native publication of accepted render-clock values and
native command stores remain explicit value seams, not complete HS-thread,
render-frame or SDK execution.864 unchanged complete-bank/target-flag comparisons,
the actual arena/lifecycle/checkpoint test and the campaign suite pass under the
unchanged60MiB hard cap/64MiB ceiling/zero swap. Other screen controls and DOF
are not established by that crossfade fixture.

Its GPU run enters scene01b and reaches actual tail jump `28420D -> 28C8F0`.
The subsequent [animation composition cohort](../analysis/opening-cinematic-20261015/source-animation-composition-integration-v1/RESULTS.md)
retains that complete240-instruction rotation/translation/scale body and its
actual indirect decoder owners. The regression creates the scene01b camera,
Regret/Mercy with thrones, anchor22 and all ten authored animated units/custom tracks.
All ten clocks advance120 source phases and all ten WORLD hierarchies move;
the read-only observer counts238 composition entries. This and the preceding
screen checks pass under the unchanged aggregate guard. No codec differential
or complete scene/pixel result is inferred from those checks.

### Original shader-function bank continuation — 2026-10-16

The [source shader-function cohort](../analysis/opening-cinematic-20261015/source-object-functions-integration-v2/RESULTS.md)
executes complete original set/get/clear and registered storage/map lifecycle
owners: seven bodies/230 instructions. Its first actual-world check caught the
missing constructor; that failure is retained. The repaired world allocates
the real arena/CRC bank and resets its original32 links. Native shader input
sampling reads the actual getter and source clock, with no advancing native
timer or eviction policy. The existing main renderer consumes those borrowed
scene function values.

381 unchanged complete slot-bank/body/query comparisons cover interpolation,
rounding, tied/oldest replacement, two identities and clear. The actual authored
Banshee test verifies source phases, object flag, renderer values, coupled
checkpoint, clear and33-key replacement. Those, the preceding screen/scene01b
checks and campaign suite all pass under the unchanged aggregate guard. Eight
current translation units also compile from a stable1097-file source/header
capture. No whole-current-engine link or shader-pixel parity is inferred.

The candidate also includes the already recovered recorded-animation constructor,
new-map activation/reset and whole query in their actual registration order.
The latest graphics run passes the former shader and recorded-query failures
and reaches scene04. Full recorded-animation playback and level disposal/handoff
remain distinct unaccepted requirements.

### Original unit controls and deferred facial pose — 2026-10-16

The [unit-control cohort](../analysis/opening-cinematic-20261015/source-unit-controls-integration-v5/RESULTS.md)
restores whole original graph reset and emotional-state owners, plus registered
facial normalization initialization: five bodies/191 instructions. Original
signed-short duration conversion forwards the actual source object and emotional
name. 192 complete unchanged original wrapper/body/clock comparisons pass.

Actual authored Regret/Mercy graph starts exercise six resets. Their facial
reference, emotional target, elapsed tick counter and independent custom clocks
advance through 30 real source phases. Restoring the same authority checkpoint
and replaying a neutral weight changes the actual rendered WORLD result. The
test uses the complete model builder and deferred `BF640` pose owner, with
declared view0/solo/unmodified view and zero distance metrics. These inputs are
not a production camera implementation.

The check first exposed a missing registered CRT initializer `380DF0`:
`46D968` selects it to publish the original 1/128 facial-weight constant at
`5564DC`. Restoring it then exposed weighted animation owner `290710`, now
compiled whole alongside the existing additive owner (507 instructions total).
Earlier failures are preserved. All eight serial checks pass under the unchanged
aggregate guard; seven current translation units compile from a stable1102-file
capture. The compatible private link has immutable input/output receipts.

These shared owners apply to other campaign units and cutscenes. The campaign
renderer currently consumes the collision/base WORLD bank; connecting its
deferred rendered pose is still required. Changed CPU render bones do not prove
facial pixels, every emotional state, original codec numeric parity or complete
movie playback. The longer RTX5080 run reaches scene08 without the earlier graph
switch or emotional-state faults, then exits on unresolved cinematic variant
selection. It retains the scene02 cinematic-shadow fault. The same binary/data
run exits after256.833s under2GiB/zero swap; its300s ceiling is not reached.
Script progression through the Halo-explosion request is not visible effect
acceptance. Audio remains muted; no final numeric frame/tick summary, screenshot
or completed movie is produced.

### Live source model selection and facial vertices — 2026-10-16

The [source model cohort](../analysis/opening-cinematic-20261015/source-model-render-integration-v8/RESULTS.md)
prepares all authored section resources without choosing a random permutation.
The retained original model builder selects geometry LOD, ordered regions and
deferred rendered WORLD nodes. Main consumes that pose and existing model-node
interpolation, with history cuts on checkpoint/BSP/restart/teleport. Source cached
bounds and current camera metric/coverage producers replace zero test metrics
in production. The later candidate also rebases this material namespace on BSP
replacement. Shader preparation preserves each LOD's decoded stream/palette;
skin processing visits only selected sections and retains duplicate requests.

All four guarded checks pass: scene08 Dervish, Tartarus and council-prophet source
admission/poses select6804/9663/8100 vertices matching the cache loader's live
region selection. Tartarus's unseeded preview remains unresolved. Preparation
and consumption leave the original source RNG unchanged. Regret's emotional and
neutral checkpoint replays produce different source-selected vertices after30
real source phases. These are native geometry/pose comparisons over original
admission, not original decoder or shader-pixel differentials.

Identical section lists share a bank only with bit-identical compound palettes.
This repairs duplicated LOD storage found by the unchanged aggregate memory
guard. Earlier combined/fresh-world OOMs and an emotional-test timeout remain
preserved. The final four checks pass within60MiB hard/64MiB measured and zero
swap; unchanged original executables retain the earlier eight-check receipt.
Current source compilation is captured separately, including the new renderer
allocation diagnostics. The private link freezes its148-byte vertex ABI; current
compiles use the live152-byte ABI. No mixed ABI or installed-build claim is made.

The first2GiB RTX5080 run no longer logs random-variant admission failures, but
exits in scene02 on a Vulkan buffer allocation failure after52.988s. A diagnostic
repeat is OOM-killed after60.5s. Its telemetry records roughly2.6GiB GPU use with
over13GiB free while host memory reaches the separate2GiB limit; this is host
pressure, not evidence of exhausted GPU memory. The source-match guard is
unchanged. [The next graphics observation](../analysis/opening-cinematic-20261015/source-model-render-integration-v11/opening-live-models-command.json)
uses a separately declared3GiB/zero-swap cap and600s ceiling, with memory samples
and terminal status recorded in that cohort. Inspect its final log/result before
claiming further scene progress. That3GiB run enters scene09/tick3760, then exits
after308.902s at original126319->125FB0. Peak host charge is3,082,862,592 bytes,
zero swap and no cap/OOM events. It passes the prior random-variant stop and
emotional-control faults. The shadow thread remains faulted; no screenshot or
complete movie is produced. The new
[resource-pressure continuation](../analysis/opening-cinematic-20261015/resource-cache-pressure-integration-v2/RESULTS.md)
restores three whole original owners/108 instructions. Its ordinary actual-world
control/vertex regression passes under the unchanged guard; the pressure path
requires the separately recorded playback retry. Shadows, DOF, complete effects, original pixels,
audio/event synchronization, all13 scenes and handoff remain required.

## Current scene matrix

The [earlier 1,200-tick headless probes](../analysis/opening-cinematic-20261015/attachment-integration-v2/scene-probes-v1/report.json)
retain commands, input hashes, diagnostics, runtime errors and containment
reports. Independent helpers do not inherit the preceding scenes' cinematic
mode, so non-held unit completion in that context is a distinct controller
gap, not proof that normal opening playback reaches that failure. These probes
predate the captured-frame consumer; their crossfade failures do not describe the
new native renderer. Latest normal playback is recorded separately below.

| Scene | Result of the independent 1,200-tick probe |
| --- | --- |
| Normal startup / `x01_scene_01a` | Stops on `cinematic_screen_effect_set_crossfade` |
| `x01_scene_01b` | Initial attachments pass; later ordinary prophet idle clock unavailable |
| `x01_scene_02` | Cinematic-shadow fault, then non-held unit completion owner unavailable |
| `x01_scene_03` | Stops on crossfade |
| `x01_scene_04` | Helper completes without script faults |
| `x01_scene_05` | Helper completes without script faults |
| `x01_scene_06` | Initial Truth attachment passes; later non-held unit completion owner unavailable |
| `x01_scene_07` | Marker effect and concurrent `halo_explosion` effect unsupported |
| `x01_scene_08` | Non-held unit completion owner unavailable |
| `x01_scene_09` | Helper completes without script faults |
| `x01_scene_10` | Stops on crossfade |
| `c01_intro_scene_01` | Helper completes without script faults |
| `c01_intro_scene_02` | Cinematic-shadow fault, then non-held unit completion owner unavailable |

The new diagnostic owner preserves both a concurrent script fault and a later
native exception. Scene02 and the final scene therefore report
`render_lights_enable_cinematic_shadow` alongside the later completion error.

The inventory covers 165 scripts, 9,030 nodes and 13 authored scene/camera owners:
69 relative scenery starts, 45 relative unit starts (all interpolation arguments
false), 169 cinematic lighting admissions, four crossfades, one DOF command,
three screen starts/stops, two cinematic shadow requests and two marker effects.
These are authored call counts, not effects successfully played.

## Actual renderer evidence and remaining gates

The [latest completed source-model observation](../analysis/opening-cinematic-20261015/source-model-render-integration-v11/RESULTS.md)
reaches scene09/tick3760 under the separate3GiB graphics limit, with no cap/OOM
events. Original126319->125FB0 stops custom animation and the following B8F40
operation. The original pressure/eviction owners are now compiled and included
in a new retry, whose terminal outcome remains required. The same run retains
the cinematic-shadow thread fault. No completed movie or all-effect acceptance
is inferred from progression through scene09.

The preceding [unit-control candidate](../analysis/opening-cinematic-20261015/source-unit-controls-integration-v5/RESULTS.md)
enters scene08 and exits on unresolved source variant selection. Its longer
muted RTX5080 observation passes the earlier graph-switch/emotional command
failures while retaining the cinematic-shadow thread fault. The error occurs
in cinematic visual preparation's deterministic variant admission; the actual
source object already owns its completed random variant/permutation bank.
Connecting that selection, selected geometry and deferred facial WORLD pose
is the next rendering requirement. The300s observation exits with status1 after
256.833s, under the same2GiB/zero-swap graphics limit. No launcher was installed.

The preceding source shader/recorded-owner candidate at
`analysis/opening-cinematic-20261015/source-object-functions-integration-v2/halo2_native`
enters scene04. The desktop reports zero displays, so its initial run cannot
start video. A subsequent owned virtual X11 run selects the actual RTX5080,
passes both former shader-function faults and the recorded-query failure, and
emits the contrail script's start/stop messages. It retains cinematic-shadow
control, actual graph-switch `11DE6C -> 1118C0`, and source-unit emotional-state
lookup failures. These are not completed effects or scene acceptance.
The muted run reaches the declared120s graphics limit and separate2GiB cap,
with zero swap. The virtual display closes during shutdown; no final tick/frame
summary or screenshot is written. [The log](../analysis/opening-cinematic-20261015/source-object-functions-integration-v2/opening-virtual-frontier.log)
preserves progression and failures. Wrapper exit is failure after125.099s.
Shader/variant and sky-selection warnings remain. No launcher was installed.

The traced source animation/screen candidate at
`analysis/opening-cinematic-20261015/source-animation-composition-integration-v3/halo2_native`
enters scene02. At tick758 its actual source bank publishes alpha0.31408149 and
the native renderer acknowledges a crossfade copy. It ends at tick1012 with two
shader-function faults and source owner gap `E558D -> 2039B0` (recorded animation).
The current root contains an authenticated recorded-animation module and
lifecycle that the frozen candidate predates; coherent inclusion remains next.
The muted RTX5080 run reaches its separate2GiB/zero-swap cap and exits with failure
after44.093 seconds. Private crossfade logging is the only Simulation diagnostic
change. [The log](../analysis/opening-cinematic-20261015/source-animation-composition-integration-v3/opening-frontier.log)
retains status, acknowledgement and exact failures. It is not an installed or
completed movie, rendered pixel comparison, sound/subtitle or all-transition pass.

The source-integrated camera candidate at
`analysis/opening-cinematic-20261015/source-camera-integration-v3/halo2_native`
runs the actual TU1.5 source world and reaches2629 game ticks/201 rendered frames
before its declared120s graphics limit. It retains one script fault: opening
`fade_out` requests a crossfade whose retained source timed-effect bank bridge
is still unavailable. The main script stops there; this does not establish
scene progression or completed cinematic playback. The run also retains
unresolved cinematic shaders/variants and per-cluster sky selection. Audio was
muted, the separate2GiB graphics cap was reached with zero swap, and no completed
screenshot was written. [The run log](../analysis/opening-cinematic-20261015/source-camera-integration-v3/opening-frontier.log)
preserves the failure and final status. This candidate is not installed.

The earlier native base-route candidate at
`analysis/opening-cinematic-20261015/world-node-integration-v2/halo2_native`
(SHA-256 `af9a2308d1614813d07ca6889f129b40c3ce8c0b51184ea9ad977b9f20df5554`)
passes the prior Pillar binding frontier and reaches scene07, tick3462, in844
rendered frames. Its muted RTX5080 run retains the scene02 cinematic-shadow
failure, two prophet emotional-state failures, and both marker-effect failures.
The main script and concurrent `halo_explosion` stop at the missing marker
consumer; later scenes are not reached. The process ends at the declared120s
graphics limit with five script faults. [The retained run log](../analysis/opening-cinematic-20261015/world-node-integration-v2/opening-frontier.log)
is progression evidence, not successful cinematic completion. The exact
world-matrix fixture, default suite and actual-cache attachment loop/restore
test pass. The attachment test reaches the configured60MiB accounting cap and
passes the64MiB measured ceiling without swap. The graphics run peaks at1.7GiB.

The preceding initialized-controller candidate at
`analysis/opening-cinematic-20261015/initialized-idle-integration-v3/halo2_native`
(SHA-256 `0ca4d5a1f0502ac660060ed09b6987c2245c9c8bedf0f4f8199c52017645a9b8`)
gets through scene01b and into scene02 in a muted RTX5080 run. At tick1490 it
retains a cinematic-shadow fault and then fails on the Pillar of Autumn scenery
model/clip node binding. [Its run log](../analysis/opening-cinematic-20261015/initialized-idle-integration-v3/opening-frontier.log)
identifies object`E32F00B8`, definition`E21C00A7`, render model
`objects\cinematics\human\pillarofautumn\pillarofautumn` and clip
`objects\cinematics\human\pillarofautumn\x01\x01:0`. This is playback progress,
not successful cinematic completion. The default suite, original custom-channel
fixtures, initialized manager fixtures and actual-cache attachment loop/restore
test pass with the shared animation objects. The v3 default suite, initialized
manager fixture and actual-cache attachment loop/restore tests pass again;
the custom-channel fixture passed in v2 before v3 added admission guards for
scenery named-idle and physical-root branches. The v3 run peaks at1.8GiB.

The [actual-cache binding probe](../analysis/opening-cinematic-20261015/pillar-binding-probe-v1/actual-cache-v1/61f2732a-115a-4ab3-a072-c51a1b0cffac/memory-budget-runtime.log)
narrowed that failure: both the Pillar model and its cinematic graph have one
root node, but their authored StringIDs differ (`poa`, `0300090B`, versus
`poa_class`, `09001D04`). The host pose helper currently requires a name match.
The original world path samples and publishes model-order banks, which now
drives active cinematic object poses. Both authored names are preserved.

The preceding crossfade candidate at
`analysis/opening-cinematic-20261015/crossfade-integration-v2/halo2_native`
(SHA-256 `f161311eb1243941a34b358933d3c97938d371531170f38ab3e5f6f58e96fc47`)
executes the first authored crossfade and enters scene01b. Its muted RTX5080 run
stops at tick744 with zero script faults on the explicitly unavailable ordinary
idle animation owner. Retained status shows the capture request consumed and
GPU-acknowledged (`pending=0`, `capture=0`, alpha0.931). This establishes actual
capture/composite admission in normal playback, not completed transition or
scene01b playback. [The retained run log](../analysis/opening-cinematic-20261015/crossfade-integration-v2/opening-frontier.log)
contains the full next failure. Native shader binaries are frozen and hashed in
this cohort; the new crossfade fragment is rebuilt from its pinned source.

The preceding renderer candidate at
`analysis/opening-cinematic-20261015/attachment-render-integration-v1/halo2_native`
(SHA-256 `44ba354e9cd9f51a53df11a09a95fbd117210b27eb99e3b1ff14597116ed252e`)
successfully renders the first frame and a 300-frame run on an RTX 5080. The
latter reaches game tick556, camera time187 and zero script faults, with audio
muted. [The captured shot](../analysis/opening-cinematic-20261015/attachment-render-integration-v1/animated-shot-v1.png)
is **visibly incorrect**: opacity/visibility and scene presentation still need
source-led repair. It also exposes a literal `\n` in the authored title text;
original text-command handling needs verification.
[The first frame](../analysis/opening-cinematic-20261015/attachment-render-integration-v1/render-setup-v1.png)
is now entirely black: all 1280x720 RGB pixels have zero-valued channels, as
recorded in `first-frame-pixels.json`. The earlier room-flash capture is retained
under `hud-preload-integration-v1` as failure evidence.

The default campaign suite and actual-cache initial attachment/cleanup/restore
test pass again in `crossfade-integration-v1/campaign_tests`, including the new
crossfade checkpoint integration. The crossfade v2 candidate shares those same
Simulation/main objects; its changes are the GPU point-sampling correction and
private shader-directory binding. The v1 first-frame capture remains entirely
black, with all2,764,800 RGB channels zero in its `first-frame-pixels.json`.
The earlier scene matrix uses
`attachment-integration-v2/halo2_native`; its Simulation, scene and attachment
objects are also the linked objects in the attachment-era render candidate. Graphics
changes between those candidates only narrow delayed overlay submission to
actual world views, preserving UI-only ModelView behavior.

Asset admission reports unresolved Cairo window/door-frame material banks and
character variants requiring the original creation RNG. Ordinary model loading
is preserved when cinematic preparation fails; a live request for an unresolved
cinematic bank fails explicitly. Multiple visible skies also lack the complete
per-cluster sky presentation path.

Required work remains: visibility/transparency and exact material classification,
remaining controller ownership and initialization, full attachment playback,
emotional-state/secondary-layer ownership, authored effects and animation-event consumers,
DOF GPU passes, remaining authored crossfade playback, cinematic shadows, original render/game/audio clock
coupling, subtitles and sound synchronization, uninterrupted 13-scene playback,
skip/restore, subsequent level handoff, matched TU1.5 pixels/audio, converted MCC
acceptance, and installed launcher delivery. The current main loop exits on
`game_won`; it does not yet establish the required next-level handoff.

No cinematic candidate has been installed by this task. Isolated source/object
and archive inputs avoid races with other work in the dirty shared checkout.
The renderer snapshot excludes unrelated concurrent renderer ABI changes;
the simulation snapshot also excludes a concurrent allegiance bridge that needs
its newer source-world cohort. Both changes remain in the live checkout.
Whole-current-checkout integration is still a delivery gate. Earlier attachment
captures read shaders from the shared build directory. The crossfade cohorts
freeze compiled shader banks and source hashes; only the crossfade fragment is
rebuilt here, so this does not establish a complete shader-bank rebuild.

## Reproduction and bounds

The map is build `02.09.27.09809`, SHA-256
`5093e2f8ef7f0df41c11c6621bac5785a36f78a1cff3b8b2bc6c14b561c02a28`.
Earlier native diagnostic playback used the base XBE, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
TU1.5 CPU captures use SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The latest v69 complete playback also uses that TU1.5 executable.

Headless simulations run serially with 60 MiB RAM, a 64 MiB measured ceiling and
zero swap. Builds use 1 GiB and zero swap; selected source compiles use Clang
because GCC's main compile exceeded that budget. Separate graphics probes use
2 GiB and zero swap after 1 GiB graphics attempts failed. The successful graphics
runs peak at 1.5–1.8 GiB. These bounds do not establish production performance.
Later combined native graphics, private Xvfb/Pulse and video recording jobs use
a separate3GiB,zero-swap aggregate bound. Source tests retain60/64MiB unchanged.

Use a fresh output directory:

```bash
python3 tools/audit_opening_cinematic.py \
  --output analysis/opening-cinematic-NEW \
  --native analysis/opening-cinematic-20261015/attachment-render-integration-v1/halo2_native \
  --xbe assets/disc/default.xbe --probe-scenes --ticks 1200
```

Omit the native options for metadata inventory. Guard failures stop the audit;
native errors after scene setup are retained separately and do not prevent
other independent helpers from being probed. Never interpret this audit as
uninterrupted playback, audio, rendering or Xbox TU1.5 framebuffer proof.

The actual-cache initial attachment test is a separate serial guard job:

```bash
python3 tools/tag_match_memory_budget.py \
  --output-root analysis/opening-attachment-checkpoint-NEW --timeout 60 -- \
  analysis/opening-cinematic-20261015/attachment-integration-v4/campaign_tests \
  --opening-attachments assets/disc/maps/00a_introduction.map assets/disc/default.xbe
```
