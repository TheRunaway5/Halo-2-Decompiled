# Original TU1.5 CPU upload return after real GP consumption

The full opening-cutscene goal remains active. This closes the declared-context
CPU upload/return comparison, not physical MCPX startup, scheduling, final audio
mix, audiovisual parity or live-world integration. No original CPU instruction,
source-call result or request-clear literal is supplied by the audit.

## Independent execution

The native constructor/resource path still allocates actual retained pages,
decodes and relocates the original DSP image, and publishes request3. Its
first-poll prefix was already compared against unchanged Xbox CPU instructions.
The new capture additionally exports the actual201-SGE mapping, resolving each
physical page through its retained native allocation or locked GP pool. It
does not assume a physical-to-source pointer subtraction or original kernel
allocation semantics.

`tools/oracle_sound_gp_upload_return_tu15.py` has two CPU stages so the original
x86 audit and GP process do not overlap inside the60/64MiB memory budget:

1. Execute the unchanged original CPU bytes from388383 to the first388470
   poll. Retain the actual CPU registers/banks and reconstruct GP scratch from
   that CPU-produced page table. Every executed opcode is checked against the
   pinned original XBE. Data access outside a declared committed range,
   including otherwise mapped page padding, is rejected.
2. Run the original GP program independently through the analysis probe with
   unchanged reference DMA/peripherals and declared reference startup/frame/
   timer inputs. It produces the actual24-byte header write at P11C and stops
   the host audit at P11E. The disputed format2 is unverified and unexecuted.
3. Resume the unchanged original CPU at388470 with exactly those recorded GP
   bus payload bytes applied to the actual SGE mapping. Check payload hashes,
   the complete resulting scratch and reference context. The CPU then executes
   its own wait condition, result, descriptor publication, header copy and
   return to the real saved caller address2100000. No callee return or ready
   flag is substituted.

The native composition runs the same upload call uninterrupted. A cooperative
**audit dispatch** at its first actual CPU poll runs the native GP against the
real retained pages. The original CPU then makes its own decision and returns.
This dispatch point is a declared test interleaving, not an authenticated MCPX
clock or device scheduler. The GP audit ends after its real header transfer;
this is not a persistent whole-device checkpoint/lifetime test.

## Evidence

Native prefix capture v505 and unchanged-original CPU prefix v506 retain189
original instruction addresses and187654 ordered stores. Their store digest
equals the previous independent prefix fixture:
`fcf291d1fb29ad4fb2e1b48a7faa380a5f89cdcb44e093e9cb06e04f314bc96f`.

The separate GP run v507 and native composition v509/v511 agree on all823296
input scratch bytes, all860160 output P/X/Y/scratch bytes, all99 captured
processor-state words, and PC/instruction/cycle counts. Actual GP execution
uses12495 instructions/12974 core cycles and one explicitly declared frame
event. Input SHA-256 is
`31747d03b42433aa5ac2b71b4d36a52045953e30b90a10b8c526e6a7c3992f0e`;
output SHA-256 is
`0d2ade74a276bfec867d9512271e716fbb1812ea9260fc6ac8544b4adbb185ab`.

Original CPU continuation v508 executes24 additional instruction addresses
and eight ordered stores. The native complete composition executes687 unique
original CPU addresses across its constructor/resource/upload dependencies.
It matches all retained return ranges, GP/XMM, defined flags, floating-point/
executor state and ordered return stores. The return digest is
`136abe72b0ab4fda6a6de678a28a985b533f88cfb21a6f7ca9c9281338aa8814`.
The descriptor output, actual six-word header and original RET12 cleanup are
checked. Prefix and return store streams are compared separately on either
side of the recorded GP bus transfer; GP writes are not counted as CPU stores.

Current-root Clang and GCC builds v510/v512 and six focused registered tests
per compiler pass in v513. Both compilers' complete CPU-return reports, GP
processor-state reports and GP output bytes are identical. Three private
native module mutations omit the descriptor store, omit the header copy, or
change RET12 cleanup. All three fail their specific new return gate after the
actual GP has successfully cleared the actual request (v514/v515). Production
source and original bytes are unchanged by those controls.

Final integrity receipt v517 checks current/frozen sources, the unchanged
production DSP/DMA bodies, fixture hash, eight successful guarded stages, both
six-test CTest logs, cross-compiler reports, negative controls and build journals.
The preceding v516 integrity attempt failed because its log parser expected a
different CTest summary string; that failed receipt is retained. v517 requires
all six actual Passed entries per compiler and rejects skips. It retains the
live publication guard and keeps full-engine/live-clock/video gates false.

This adds behavioral coverage for existing complete recovered bodies; it adds
no new original CPU body to the184-body provenance inventory. Original GP ISA
status/reset discrepancies and format2 byte placement remain unresolved.

## Reproduction

The owned original executable is pinned to SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Build the optional native GP consumer target as described in
`tests/fixtures/sound_gp_dma_tu15.md`, retaining the separately built pinned
MIT core archive and serial1GiB/zero-swap build containment. Analysis reference
code remains unchanged and analysis-only; it is never linked into the game.

Each source/original execution below must be a separate invocation inside
`tools/tag_match_memory_budget.py`, with fresh output and the existing60MiB
hard/64MiB measured ceiling, zero swap and exclusive validation lock:

```sh
NATIVE_CONSUMER OWNED_XBE NEW_NATIVE_BEFORE
/tmp/halo2-fov-reference-env/bin/python tools/oracle_sound_gp_upload_return_tu15.py \
  --native-before NEW_NATIVE_BEFORE --output NEW_ORIGINAL_PREFIX
```

The middle GP stage uses the frozen analysis probe build/commands in
`analysis/opening-cinematic-20261015/dsp-gp-native-dma-state-build-v497`.
v507's `run.py` records the exact invocation and artifact hashes. It reads
`NEW_ORIGINAL_PREFIX/gp-input`, runs `--upload-only`, and retains
`gp-output.bin`, `gp-state.bin`, `gp-events.jsonl`, `gp.log` and the exported
`reference-context.json` in a fresh `NEW_GP_RUN` directory. The context pin is
`9557278da666fe0f7de21831140f86bfc4ccc882064e6b45ed318a45228433f4`.

```sh
/tmp/halo2-fov-reference-env/bin/python tools/oracle_sound_gp_upload_return_tu15.py \
  --checkpoint NEW_ORIGINAL_PREFIX --gp-run NEW_GP_RUN --output NEW_ORIGINAL_RETURN
NATIVE_CONSUMER OWNED_XBE NEW_NATIVE_RESULT tests/fixtures/sound_gp_upload_suffix_tu15 \
  --complete-gp tests/fixtures/sound_gp_dma_tu15/reference-context.json \
  --return-reference NEW_ORIGINAL_RETURN
```

The ignored local `sound_gp_upload_return_tu15/case.json` is retained from v508
with SHA-256
`f0e50d776c49bebfd7b90d34a1a9ef52435370706423e739e6da8a3ce01f690b`.
The registered `original_native_tu15_sound_gp_upload_return` test uses the same
production modules and consumer executable. The runner retains a fresh result
directory on every run and skips77 if required owned data/reference/context is
absent; a skip does not prove coverage.

The full parent APU factory, authenticated reset/peripheral/IRQ/timer/FIFO/
format2 behavior, GP/EP/output clocks, live audio-frame integration and sample
timing remain open. The live-world guard before38846E is retained. Complete
visual/effect/interpolation checks, handoff, coherent installation, corrected
visible playback and the finished video are still required.
