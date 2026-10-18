# Native TU1.5 GP instruction owner

`SoundGpCpuTu15` owns persistent X, Y, mix and program RAM and executes the
actual uploaded instruction words through the pinned MIT DSP56300 core. Its
C bridge preserves upstream's generated C ABI without changing that header
or the instruction implementation. Native host accesses and executed DSP
accesses use the original two X/mix windows. Program writes during a callback
yield before invalidating translated code and are published before the next
execution call; core-state re-entry is rejected. Callback
exceptions fault the owner and cannot become a completed frame or ready device.
This does not promise rollback of external callback side effects.

The caller must supply the initial scalar register context and both peripheral
owners. Construction is not a live-state restore API or authenticated MCPX
reset. This component supplies no timer, DMA, idle, IRQ, FIFO, source callee
return, SDK allocation or device-ready value. Unsupported arithmetic modes
are rejected. The verified domain is the recovered original GP program, not
arbitrary DSP applications or the EP processor's additional modes.

## Build

The dependency is revision `82fb25a6917a36ac66212ae31f02aab62211f979`, MIT
licensed. `third_party/dsp56300/HALO2_ORIGIN.json` records the authenticated
archive and all241 original file hashes. The final archive comparison confirms
that all upstream files remain unchanged, including the generated C header.
The GPL reference interpreter/peripheral files are analysis-only.

Use separate serial stages inside the existing1GiB/zero-swap build container:

```sh
python3 tools/build_sound_gp_cpu_dependency.py --target-directory NEW_RUST_OUTPUT \
  --rustup-home PRIVATE_RUSTUP_HOME

cmake -S . -B NEW_CPP_BUILD -DBUILD_TESTING=ON -DHALO2_BUILD_GP_CPU_TESTS=ON \
  -DHALO2_DSP_PREBUILT_LIBRARY=/absolute/NEW_RUST_OUTPUT/release/libdsp56300_emu_ffi.a
cmake --build NEW_CPP_BUILD --target sound_gp_cpu_tu15_tests -j1
```

The dependency helper authenticates the vendored files, records its command
and profile, then replaces itself with pinned Rust1.99 Cargo. It does not
change the global toolchain or any memory limit. The three nested CMake Cargo
attempts v303–v305 exceed1GiB; changing codegen units or generator does not fix
that. Direct Cargo v307 and the committed helper v319 both finish within the
same hard limit. The final optimized dependency build v349 also succeeds:
the runtime core, emulator and FFI packages use O2 while the large compiler
dependencies retain O0, no debug information and256 codegen units. The helper
records these package overrides. Use the two-stage bounded build workflow.

`halo2_sound_gp_cpu_tu15` is an explicit CMake target, excluded from ordinary
builds until a real consumer links it. Its tests require the explicit option.
This avoids introducing a partially connected processor into the live mixer.
The existing CMake Cargo producer remains available for builds without this
constrained nesting; it is not the validated1GiB workflow.

Run tests serially through `tools/tag_match_memory_budget.py`; final CMake
owner/CTest evidence is v359/v360 and frozen GCC/Clang evidence is v347/v348/v350. The
tests check both host and actual instruction access through the mix aliases,
initial modes, bootstrap lifetime, changed program operands, callback writes,
callback faults and unmapped X accesses. Private wrong-alias and ignored-fault
variants fail at the corresponding instruction-read and failed-read checks
(v326/v327). The earlier v321 alias mutation passed because the test only
checked the host API; that coverage gap is preserved and corrected by the
actual instruction cases. It is not claimed as a successful rejection.

## Read-only complete core-bank/stack audit

`execution_state()` retains actual PC/cycles, the pinned core's64-slot bank,
all32 stack words and both repeat fields. The bank includes internal TEMP
bookkeeping, so it is not labelled a physical MCPX register dump. Callbacks
cannot read it while translated execution is active. This provides no restore,
interrupt/peripheral checkpoint, hardware reset context or ready value.

Both `oracle_native_sound_gp_tu15` and `oracle_sound_gp_boot_tu15` accept
`--state-trace FILE`. Each real completed idle, including both bootstrap frames,
writes99 explicit LE32 words: PC,64 core-bank slots,32 stack words, repeat and
PC-on-repeat. Core cycle counters remain separately reported for the native
step/block comparison; the older reference's instruction-step API does not
advance an equivalent counter. No C ABI padding is treated as state.

The original two-word JSR atP0 must preserve returnPC2 in the actual stack.
A private bridge that reports zero stack data fails this regression
(v370/v374). Final current-root CMake v373 and both tests v375 pass, including
the original MOVEC/ASL ISA cases, actual stack and callback upload/continuation.

The three-case expanded raw-state audit v367 **does not pass**. Output and
final P/X/Y/scratch bytes still match all cases. Silent/direct stack/repeat
fields agree. All-bus SR.L and saved SSL copies differ for40 early frames.
Every raw difference is retained; none is masked or rewritten to pass.
Native step/block snapshots agree with each other. Diagnosis records:

- Original P70/P71 is `05F420 FFFFFF`, with SR15 before/after. Reference
  modifier writes truncate to16 bits; native retains24. The
  [DSP56300FM Rev5](https://www.nxp.com/docs/en/reference-manual/DSP56300FM.pdf)
  sections4.2/4.3.4 specify24-bit modifier storage outside compatibility mode.
- The first L difference is original step40766 atP1C2, `0C1D08`
  (`ASL #4,A,A`). FF:FE6418:000000 shifts without bit55 changing. Reference
  `dsp_asl56` sets L when unsigned shifted-out bits are nonzero; native
  retains L, consistent with the manual's overflow/limiting definitions.
  Exact sites and declared inputs are retained in v365/v366/v368, with
  bounded executions v365/v366/v371. Physical MCPX behavior remains unproved.
- REP saves LC in implementation slot48 (`LCSAVE`) in the reference and
  slot43 (`TEMP`) in native. Comparing all64 raw slots also retains this
  layout difference. It is not silently mapped onto an architectural register.

No upstream source or original program was changed for these differences.
Tests for the original MOVEC/ASL opcodes use declared inputs and manufacturer
ISA rules; they do not turn emulator agreement into physical Xbox evidence.

## Compiled original-program execution

`run_original_gp(max_cycles)` authenticates all15684 loaded program bytes with
SHA-256 before every execution call. It rejects unknown or modified programs.
`max_cycles` is a host scheduling slice checked at compiled block boundaries,
not a strict hardware deadline. The actual DSP cycle counter is preserved and
the consumed counter delta is returned. `stopped` means a callback requested
a host yield; only the real peripheral owner can establish idle/readiness.
`request_stop()` is valid only during execution. The core flushes its pending
cycles/registers and retains the next PC after the peripheral instruction.
It does not edit guest registers, stack, source returns or device status.

PC/register access returns the actual state captured after the last execution
call. Callbacks may access RAM but cannot inspect or re-enter scalar state.
Program writes from callbacks automatically yield and invalidate code after
returning from the translated block. The next execution must authenticate the
program again. A permanently faulted owner cannot return readable valid state.

Original-program tests verify the first peripheral-write yields atPC157,
resumption reachesPC10C, actual counters/registers match single stepping, an
unused modified word rejects before executing, and callback program upload
yields and resumes correctly after authenticated restoration. A failed
peripheral read faults permanently. Private stale-state, hash-bypass and
ignored-stop controls fail (v351–v353); current final tests pass (v360).

## Original-program comparison

`tools/oracle_native_sound_gp_tu15.cpp` links the actual native owner with the
unchanged pinned reference peripheral/DMA model in an analysis executable.
Only the DMA model's two memory-access symbol names are redirected to the
native owner's actual RAM API. No math, descriptor, transfer, device result
or ready-word behavior is replaced. The compile commands are retained in
`dsp-gp-block-final-probe-v357/commands.json`.

The inputs are produced by the authenticated native executable loader/exporter;
captured DSP RAM is not an input. Reset PC0, the original bootstrap transfers,
image request and all15 effect initializations execute before the test frames.
The original program clears its own request. All output mix words and final
PRAM/X/Y/scratch bytes match the original interpreter for16 silent frames,
1024 direct-bus frames and1024 all-bus frames. Instruction and scratch-transfer
counts also match:48910966 instructions,130868 reads and80816 writes per run
of all three cases. The first harness v315 omitted the bootstrap read event
from its count; its bytes already match, and v317 fixes the actual transfer
accounting rather than normalizing a counter.

The final block/step comparison is `dsp-gp-block-final-comparison-v358`.
In addition to the retained bytes above, both paths have equal final64 scalar
registers, actual cycle counters and a SHA-256 trace of PC/counter/all64
registers at every completed idle, including the two bootstrap frames.
The block path reports bootstrap single steps and actual compiled cycles
separately; it does not label those cycles as instruction counts.

`--timing` records wall/thread CPU around frame execution, excluding input
fill/output file writes. The first compiled frame costs630–665ms CPU in the
retained runs. Excluding16 declared warmup frames, direct/all-bus means are
90.2/91.2us, p99 values122.9/120.4us, and maxima253/252us. This meets the modeled
32/48000-second compute interval; single stepping still misses it. Startup
must handle cold compilation before unpausing real audio, without adding fake
frames, trimming PCM, editing source sleeps or rebasing its clock.

Both sides still share the reference peripheral model and its known timing
limitations. Native SDK/APU/DMA/IRQ ownership, final voice routing, output clock,
EP processing, checkpoint lifetime and live playback remain open. The warmed
compute result does not certify native startup or device scheduling. This component does
not establish final volume/timing, physical Xbox equivalence, complete visual
parity, installed-runtime acceptance or the requested finished video.
