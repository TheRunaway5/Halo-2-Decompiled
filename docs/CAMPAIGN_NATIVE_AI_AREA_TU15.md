# Native campaign area candidate qualification

The native area module implements the complete `1C5870` area-availability
query for its inherent flags5/mode0-or1 profile, and the `1B6F20` behavior
candidate callback that invokes it. The query follows the actual squad/order
inheritance chain and selects explicit pairs, an order area list, or the squad's
default zone. It does not substitute navigation geometry or host pathfinding.
The query uses typed local iterator state and performs no persistent gameplay
writes. Original private stack saves, iterator stores and dead registers are
outside the public semantic interface.

`1B6F20` returns candidate behavior ID6B when the enabled actor has an eligible
area; otherwise it returns the low16 bits of the actual default word. This is
not a score return. Its real caller tests signedAX at1B012B, rejects values
outside[0,83), indexes the registry from MOVSXEDX,AX, and writes score3 only
after accepting a candidate. The common continuation at1B01A9 overwritesAX;
full-width EAX/ECX/EDX uses have their own definitions. The numeric transport
therefore publishes AX16, consumes the real RET plus eight argument bytes and
preserves the other provided state, including active MMX. It owns only1B6F20;
1C5870 is a typed API in this qualification.

The reference exercises original1B6F20,1C5870,20D180 and20D2D0 together in an
owned physical i386 process. No callee return is supplied, no instruction is
patched, and no stopped prefix resumes. The retained fixture has210 complete
calls plus270 before-read prefixes. The210 whole calls were independently
repeated and their complete results, banks, reads, FP, register boundaries and
input timelines were identical. The337 executed sites cover every site in the
fixed profile; the other27 sites belong to flags5-disabled prescan or mode2.
This is site coverage within a finite input cohort, not exhaustive input or
branch coverage of the general helper APIs.

The matrix includes salted actors, disabled selectors, negative/positive/default
candidate IDs, inherited orders, missing squad/default-zone gates, all three
scan paths, primary/secondary selectors, signed count and index boundaries,
zero-count explicit pairs, modulo32 kind shifts, terminal NONE area results,
raw unsigned zone arithmetic, extra area flag bits and a valid entry/area alias.
It also declares external input changes after completed reads to test selector
changes, reloaded squad rows, both default-zone word reads, fresh counts and
zone pointers, and the default scan's cached scenario pointer. Those edits are
input timelines; none changes private iterator state or supplies an engine
outcome. Every native run compares the ordered reads and the same applied input
changes against unchanged original execution.

`analysis/campaign-native-ai-area-20261028/proof.json` joins the final original
and caller evidence with the current-source component results. The required
reference fixture is `tests/fixtures/native_campaign_ai_area_tu15.json.gz`
(786933bytes, SHA256
`baeb313ac8aa802d9dd447964f77eeae25a0a2067a0cc790255d28a80c6ff755`).
The480 retained original cases produce720 typed/binding comparisons,14285
ordered native reads and63 applied declared input edits. Tests verify exact
results, retained data, no query writes, actual missing-RET failure, host FP
restoration and preservation of provided GP, SIMD, live x87, active MMX and
policy state. The corresponding retained original traces contain9530 reads
and52668 unchanged instructions; repeating the whole cohort executes29570
additional original instructions.

GCC normal and actual ASan/UBSan runs pass1/1 each. Clang passes6/6: normal plus
rejection of actual source mutations rejecting zero-count pairs, omitting
modulo32 type behavior, inverting area class, collapsing the repeated zone
read, and returning onlyAL. All declared compiled sources stayed unchanged.
The first wider input grid's receipt remains diagnostic; final acceptance uses
`original.json`, `callers.json` and `validation/validation.json`.

Only this query's fixed parameters are qualified. Arbitrary general20D180
flags/mode2, huge-count counter-wrap paths and nonterminating inheritance cycles
were not separately executed by the finite cohort. The original has no cycle
breaker; the native implementation adds none. Rendered behavior and passage
through the next coherent campaign runtime remain separate integration gates.
All owned ELFs, probes, mutant binaries, raw temporary banks and build trees
were removed; only the required compressed reference and compact receipts remain.
