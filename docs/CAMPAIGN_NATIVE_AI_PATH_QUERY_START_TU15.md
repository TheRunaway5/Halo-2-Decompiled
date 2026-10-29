# Native TU1.5 path-query start

`src/campaign/ai/native_campaign_ai_path_query_start_tu15.cpp` implements complete
`27B100` query start: actual root/sector/height admission, framed-point distance,
original cost quantization, source-node publications and insertion through the
proved native retained priority heap. The public typed API is
`native_campaign_ai_path_query_start_tu15(memory,workspace,fp,observer)` in
`include/halo2/native_campaign_ai_path_query_start_tu15.hpp`; its CMake module is
`cmake/CampaignNativeAiPathQueryStart.cmake`. No numeric router or live campaign
activation is added.

The implementation contains no virtual CPU, virtual FP stack, interpreter,
original-code execution, captured dependency result or pathability substitute.
Actual retained records supply all data; x87/SSE operations execute on native
hardware under the explicit source FP policy. The host's unrelated live x87
values, complete saved environment and MXCSR are restored on success and failure.

`218C40` distance has its own source operand order. Equal frame IDs perform
read/subtract X, then Z, then Y before `(dx² + dz²) + dy²`. Differing frames project
both points and reduce `(dz² + dy²) + dx²`. The new
`native_campaign_ai_project_point_value_tu15` seam reuses the existing proved
geometry algorithm while keeping private intermediate coordinates in typed
host arrays. No original private stack is constructed and no persistent scratch
fields are fabricated. Existing geometry algorithms are unchanged.

The distance remains an actual extended hardware sqrt. Its float spill rounds
under the original CW, but scaling consumes the retained extended value. The
scale is read only after that spill. Conversion follows the actual CRT
`32CC0C` algorithm: hardware rounded 64-bit conversion, signed float-difference
correction toward zero, and its separate zero/integer-indefinite cleanup path.
Native typed integers implement the carry correction mathematically; there is
no GPR/flags emulator. `27B100` compares only signed low 32 bits against 7FFF. NaN and
overflow can therefore yield integer-indefinite low 32 bits=0 and still construct a
node, just as the original does. The native implementation does not replace
those outcomes with a sanitized failure or a full 64-bit comparison.

All workspace reads, widths and publications preserve source order. Node
indices, heap counters, source poses, sector flags, last-point data and hash
slots remain lazy and may alias. The node index is signed 16-bit for address math.
The caller's heap retains its original capacity/refusal behavior; no host heap
or host navigation graph is used. All dependency instructions in the reference
execute for real, including native-heap reference owners `27BCE0`/`27BB60`.

`tools/oracle_native_campaign_ai_path_query_start_tu15.py` executes unchanged
physical i386 original code from authenticated English TU1.5, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Only declared pre-entry data, GPR and FP inputs are installed. No instruction,
return value or resumed fault outcome is supplied. The observer rejects any
executed persistent RAM operand outside declared retained regions. Faults stop
before source reads/stores or after actual committed stores. Private stack
accesses and intermediate FP values are not resumable native ABI outputs.

The required fixture is
`tests/fixtures/native_campaign_ai_path_query_start_tu15.json.gz`, 216,081 bytes,
SHA-256 `3f95d8008c00571a44dd46a8d080f570be0d5ccc35b52624475a19305927872a`.
An independent physical repeat produced identical bytes. Its 910 cases contain
414 whole calls and 496 source/publication prefixes, with 169,947 original
instructions, 34,846 persistent reads and 18,254 stores. Tests compare exact bool
outcomes, retained bytes, ordered operand reads, stores/sites/widths, full FP
policy/status and unrelated host x87/MXCSR preservation. Cases include all
supported CW rounding/precision and SIMD DAZ/FTZ policies, source aliases,
signed/count/capacity edges, direct/projected/failed frames, exceptional values,
NaN payloads and quantization boundaries.

Canonical calls retain the exact original immutable scalar literals and execute
422 distinct sites. Thirty-seven explicitly labeled adversarial readonly
operand-bank calls additionally execute eight CRT negative-correction sites;
they are semantic arithmetic stress cases and are **not** canonical engine
input or startup-data qualification. The combined reference covers 430 reachable
closure sites and all 117 `27B100` sites. Ten `218780` NONE-entry sites cannot be
called through `218B20`, which branches to its own local copy on captured frame
FFFF. Those ten sites remain covered by the independent direct geometry fixture.
The original selector/call conditions and site sets are retained in the report.

GCC passed 2/2 CTests: the new composed owner and all 778 original direct geometry
regressions on current sources. GCC ASan/UBSan passed 1/1. Clang passed 7/7,
including six actual source mutations rejected by the unchanged original:
rounding distance before scaling, comparing full 64-bit, sanitizing nonfinite cost,
reordering point reads, changing the sector mask and omitting native heap
insertion. Current compiled pins and geometry requalification are retained in
`analysis/campaign-native-ai-path-query-start-20261028/geometry-regression.json`.
The earlier geometry proof remains historical evidence for its original pins.

`analysis/campaign-native-ai-path-query-start-20261028/proof.json` retains source
hashes, commands, physical coverage, fixture provenance and verified cleanup.
All original executables/observers, raw banks and compiler products were removed;
only the required compact fixture and small receipts remain.

The scope requires ordinary valid callers with room for two x87 temporaries and
masked FP exceptions. Private FP values on a fatal source failure are not
reconstructed. Actual arena teardown/reconstruction, `1D2D90`/`1D2E30` leasing,
full `27B340`/`27BD30` search, `27A460` cost query, `1C9770` selector and `200D30`
update remain open, as do numeric live dispatch and full native campaign parity.
