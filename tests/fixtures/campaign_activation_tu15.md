# Original TU1.5 activation caller fixtures

Authority: unchanged English Xbox TU1.5 executable, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
`tools/generate_campaign_activation_tu15.py` recovers complete original owners
from actual map-start calls and the authenticated lifecycle registration table.
The original fixture recovery selected47 bodies /3078 instructions. The cleanup
addition previously expanded the module to48 bodies /3093 instructions. The
current module recovers49 bodies /3138 instructions, including complete15-PC
cleanup13A7E0 and complete45-PC /152-byte resource-stage191CF0. Their independent
[cleanup fixtures](campaign_cleanup_tu15.md) and
[resource-stage fixtures](campaign_resource_stage_tu15.md) retain the separate
bounded proofs; the historical activation fixture recovery remains47 bodies. The level-handle
constructor and complete reset retain their existing separate owners; the
activation module does not duplicate their exported entry.

Run the generator with fresh `--output NEW_CPP --directory NEW_RECOVERY`, then
run `tools/oracle_campaign_activation_tu15.py --recovery NEW_RECOVERY --output
tests/fixtures/campaign_activation_tu15`. Both run serially through the existing
`tools/tag_match_memory_budget.py`, unchanged60MiB hard/64MiB measured ceiling,
zero swap and host reserve. Existing fixture directories are never overwritten.
The optional CTest skips with77 if these local original-data fixtures are absent.

The70 declared caller cases execute1184 original instruction addresses. They
exercise invalid/same/NONE BSP selection, real registered callback boundaries,
option copying/RNG delivery, membership gates, shader/effect reset, palette
ownership, physical-arena/new-map callbacks, simulation/statistics reset,
seeded vector generation, renderer handle reset, sound-environment parameter
transfer/effect-data callers and11 variant types.
Explicit destinations for these CPU callers are not native constructor results.
The original registration and jump-table bytes
are retained. External owners stop before execution, without supplied results;
this includes the original GPU call and dynamic lifecycle callbacks.

The shared native fixture reader runs each case with and without write
observation:140 comparisons,29939 ordered stores and70 external-owner stops match
all declared retained bytes, GP/XMM registers, defined arithmetic flags and DF.
`--omit-bsp-bind` is a failing control. The reader also reruns the independent
completion fixture suite after its shared dispatch change.

These inputs are not an admitted Xbox session. A source early-return case's
true AL is not production readiness. The actual isolated native startup probes
keep game-active0. With an actual renderer, all21 original BSP-start callbacks
now return. The four real native palette resources have the exact1024 authored
color bytes, and actual GPU image/rejection/retirement checks pass. The native
4025B0 API boundary is not original SDK instruction recovery. Headless execution
still stops before that GPU owner. Full map startup runs the actual1A5410 pool
constructor, complete existing1A59D0 reset, physical-arena callback126730 and
simulation/statistics, vector-table and renderer reset bodies. Original171E10
and228260 constructors fixed the row36/43 storage failures in those probes.
An earlier isolated probe stopped at2260E8 while writing through the then-absent
52D674 driver binding; this is historical evidence, not a verified current
blocker. Current root has independent224EC0/device work, and a fresh coherent
diagnostic is pending. No ready flag or external
success is supplied. Production campaign setup has not been switched to an
incomplete full-startup path. Full activation, GPU/platform coverage, audio,
profile saves and actual level loading remain acceptance gates.

The vector table uses all0x402 original12-byte entries admitted by the actual
219FD2 index bound; its generated float bytes and RNG stores match. Renderer
reset cases cover inactive state, invalid handles and512 cells with salted live
handles. The larger statistics case hit the unchanged60MiB guard while building
a complete JSON string; streaming serialization fixes that allocation without
changing the inputs or serialized results. The failed evidence is preserved.

Environment fixtures include absent and present tag references, plus zero and
declared plugin/device identities. These opaque test identities supply no native
resource or method result. The original caller stops before the required owner;
the complete stack/register state and parameter bytes are compared there.
