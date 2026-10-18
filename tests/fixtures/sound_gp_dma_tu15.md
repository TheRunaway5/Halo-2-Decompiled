# Original TU1.5 GP scratch DMA recovery

This is synchronous native transfer ownership, not a complete MCPX device,
hardware clock, final audio mix or cutscene-engine certification. The ordinary
game does not select this explicit target. Source recovery remains governed by
`docs/CUTSCENE_TU15_SOURCE_COVERAGE.md`; the handwritten resource owner does not
add authenticated Xbox CPU function bodies to that inventory.

The original program is owned-XBE data. Its 15684-byte GP image has SHA-256
`a5b3859a22b68c6a90b14419b442471d51fb8475836371ea804cd50dad96241f`.
The authenticated initial control disassembly and original instruction traffic
are retained under
`analysis/opening-cinematic-20261015/dsp-gp-transfer-source-v481` (the audit
actually ran inside the successful v482 guard). Traffic is from the declared
reference peripheral model, not an original physical-console capture.

`SoundGpDmaTu15` consumes actual GP X-memory descriptors and accesses retained
RAM and scratch through caller-owned callbacks. It supports only the six
observed noninterleaved, stride-one scratch controls:59E0,59E2,59D2,49E2,4BD0,
4BD2. It preserves linked-node order, circular offset writeback, physical GP
space/alias access, actual completion and the source STOP/FREEZE/UNFREEZE/START
sequence. Unsupported commands, configuration, RAM holes, callback reentry,
foreign executors and transfer faults fail explicitly. No request clear,
kernel result, physical reset, timer, interrupt delivery or ready flag is
supplied by this owner. Configuration bit9 in the observed packets remains
opaque; this implementation assigns it no interrupt meaning.

## Format2 remains unresolved

The very first all-bus native/reference comparison v484 **fails**. Both modes
use the same native GP CPU. All frame processor states and final GP P/X/Y RAM
match, but2590 retained scratch bytes differ, starting at output byte86016
(scratch B000). Seven actual scratch-output events use different byte placement.
The complete difference audit is v486. The failure is preserved.

The [public hardware notes](https://xboxdevwiki.net/DSP) describe format2 as
placing24 bits in the high end of a32-bit slot, whereas the pinned reference
model produces low-end placement for the observed transfers. Those notes
explicitly describe incomplete hardware testing. Their linked
[hardware experiment](https://github.com/JayFoxRox/xbox-tools/pull/60) is marked
WIP. Neither disagreement is silently normalized into hardware authority.

The default native owner therefore **rejects format2**. Explicit
`declared_low24` and `declared_high24` inputs exist for analysis comparisons;
neither is an authenticated physical-device choice. They cannot be selected
implicitly. The analysis probe prints the chosen declaration. An emulator
agreement under `declared_low24` would establish only reference-model
equivalence, not original Xbox byte parity.

## Verification and resource composition

Current-root Clang/CMake v488 builds the explicit DMA and GP core tests. The
three registered tests pass in the v48960/64MiB, zero-swap guard. They cover
the six control shapes with explicit test inputs, actual EOL acknowledgement,
linked transfers, padding, P/Y/mix access, circular wrap/cursors and ownership
failures. Format2 is rejected without a declaration. v485's earlier configure
failed while an unrelated source file was temporarily absent; v488 succeeds
once that shared-checkout file exists.

The optional `HALO2_BUILD_GP_UPLOAD_CONSUMER_TESTS` target composes the existing
original CPU upload test with the actual source-published201-SGE native pages,
the GP instruction owner and this DMA owner. Processor startup registers and
the frame/timer event are explicitly declared reference-model inputs, not
asserted MCPX reset or clock behavior. The GP must clear the actual retained
request through its own original instruction/header transfer. The original
`--consume-gp` mode stops the CPU at its first poll. The subsequent
`--complete-gp --return-reference` mode additionally compares the CPU's own wait,
output/header stores and complete return; see
`tests/fixtures/sound_gp_upload_return_tu15.md`. Physical clock and live device
integration remain unverified in both modes.

The source test retains its input scratch, complete final P/X/Y/scratch and
processor state for independent comparison. It does not execute the disputed
format2. The initial upload-only attempt v491, without a frame event, reaches
the original wait and fails its instruction bound. The corrected audit names
the declared frame event explicitly and supplies it only after an actual GP
idle. No physical frame producer is inferred.

The successful upload pair v495 matches all860160 retained bytes and74
peripheral events after12495 instructions/12974 cycles, stopping at original
P11E immediately after P11C's header store. The real201-SGE composition v496
changes only request byte810 from3 to0. Independent v498 comparison matches
its full860160-byte output and99 captured processor-state words, plus PC,
instruction and cycle counts. The actual read callbacks differ by the
reference harness's separately counted bootstrap read; no data/state difference
is hidden by that accounting distinction.

The same v498 audit runs1024 all-bus frames plus two bootstrap frames with the
explicit `declared_low24` hypothesis. All5054464 retained output/final RAM/
scratch bytes and406296 frame-state bytes match, including1026 full processor
states and raw final registers. Instruction/core-cycle totals and scratch
read/write counts match. This remains reference equivalence, not a hardware
choice. The audit also confirms that actual program execution rejects format2
with no declaration, and checks that peripheral tracing preserved the previous
reference output bytes.

Current-root Clang build v492/v497 and its three registered tests v500 pass.
GCC build v499 and four registered tests v502 pass. The entire GCC actual GP
output and processor-state report equal Clang. Two private DMA mutations,
omitted actual scratch output and omitted EOL, fail both the owner tests and
the actual SGE consumer; neither produces a successful GP report. Those frozen
private binaries are v501, with failures retained in v502. Production code is
unchanged by the controls.

Final integrity receipt v503 checks the current source manifests, frozen probe
source, original GP and declared-context hashes, nine unchanged pinned reference
files, six successful validation receipts, retained earlier failures and build
journals. It keeps `whole_engine_verified`, `live_runtime_connected` and
`finished_video_delivered` false and the goal active.

To reproduce with the separately built pinned MIT core archive:

```sh
cmake -S . -B NEW_BUILD -G Ninja -DBUILD_TESTING=ON \
  -DHALO2_BUILD_GP_CPU_TESTS=ON -DHALO2_BUILD_GP_DMA_TESTS=ON \
  -DHALO2_BUILD_GP_UPLOAD_CONSUMER_TESTS=ON \
  -DHALO2_DSP_PREBUILT_LIBRARY=ABSOLUTE_PINNED_ARCHIVE
cmake --build NEW_BUILD --target native_tu15_sound_gp_consumer_tests \
  native_tu15_sound_gp_upload_tests sound_gp_dma_tu15_tests \
  sound_gp_cpu_tu15_tests -j1
python3 tools/tag_match_memory_budget.py --output-root NEW_GUARD --timeout 180 -- \
  ctest --test-dir NEW_BUILD \
  -R '(native_sound_gp_(dma|cpu)_owner_tu15|original_sound_gp_block_owner_tu15|original_native_tu15_sound_gp_(consumer|upload_suffix))$' \
  --output-on-failure -j1
```

Run the configure/build commands inside the established serial1GiB/zero-swap
systemd build containment. The consumer test skips77 when the owned XBE,
unchanged-original upload fixture or declared context is absent. The context
is produced by the analysis probe's `--export-reference-context FILE`; v495
retains that declared input with SHA-256
`9557278da666fe0f7de21831140f86bfc4ccc882064e6b45ed318a45228433f4`.
It is not a console register capture or asserted hardware reset.

Builds use the existing separately built pinned Rust archive, serial1GiB,
zero swap. Validation must run inside `tools/tag_match_memory_budget.py`.
Do not raise the60MiB hard/64MiB measured ceiling, nest Cargo under CMake or
replace failed receipts. Reference peripheral/DMA code remains unchanged and
analysis-only; no reference implementation is linked into the game.

Full device/IRQ/timer/FIFO/GP/EP/output clocks, the upstream APU factory, final
mix/sample timing, complete source call return, audiovisual acceptance,
installation, corrected visible playback and the finished video remain open.
