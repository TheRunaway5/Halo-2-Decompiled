# Original TU1.5 automatic scenario placement scan

Authority: unchanged OG Xbox English TU1.5 executable, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Complete `D7AA0` contains 72 instructions and consumes the literal 13 source
type descriptors at `474B90`. It preserves signed count/stride/ordinal handling,
the low byte of the suppress-existing argument, predicate/existing checks,
five automatic factory operands and the following `C13B0` call. It returns
with the original `RET4` ABI. Predicates, lookup and factory results in this
fixture are explicitly declared test inputs; they do not implement those
external owners or prove World activation.

Regenerate the required local reference inputs through `tools/run_test.py`:

```text
.tools/python/bin/python -B tools/oracle_campaign_scenario_scan_tu15.py
  --xbe ORIGINAL_TU15_XBE --helpers tools/reference/halo2-decompiled/tools
  --output FRESH_SCRATCH_DIRECTORY --execute-original-after-grant
```

The oracle authenticates the executable and complete unpatched body, uses the
actual type pointer/descriptor bytes, bounds memory reads/writes and execution,
and executes 17 cases: empty/negative counts, every admitted source type,
multiple placements, predicate low-byte results, suppression/lookup choices,
argument high bytes and each of four missing external owners. Retained inputs
in `tests/fixtures/campaign_scenario_scan_tu15` total 767,112 bytes across
18 JSON files. These small original reference inputs are required by the
durable native differential test; they contain no runtime binary or capture.

The native observed/unobserved comparisons pass 34 cases, 208 declared callee
entries, eight exact missing-owner stops and 1,110 ordered stack stores. All 72
instructions are exercised. Complete declared banks/stack, registers, XMM,
defined flags, store order, source FP controls and host FP restoration match.
Five standalone CMake controls pass actual child exits `0/0/1/77/1`, including
old-owner rejection, an old-owner negative comparison, missing fixture and
corrupt final state. Evidence is under
`analysis/current-root-cutscene-20261019/automatic-scenario-native-v1`.
Its owned source/build/test scratch was removed after completion.

The generator now recovers 51 bodies / 3,215 instructions. Forty-nine prior
body source strings are byte-identical; the sole prior-body change binds
`191DF0`'s existing `191DFE -> D7AA0` call directly to the newly recovered
owner. Instructions, operands and source chronology are unchanged. Recovery
does not append a duplicate startup scan to ordinary native initialization.
Complete resource/audio/BSP prerequisites, fresh-start integration, frame
clock ownership and matched Vulkan/TU1.5 visual acceptance remain open.
