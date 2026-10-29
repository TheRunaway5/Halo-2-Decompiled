# Native campaign combat counts

`ai_fighting_count` now uses native TU1.5 `27DDA0`, `1EA090` and `1AFBD0` semantics through the actual campaign script binding. The MCC builtin is opcode327, translated to TU1.5 opcode325/evaluator`2AF650`, with an AI reference argument and SHORT result. The evaluator descriptor and resumed original evaluator are checked against the authenticated original image.

Squad and squad-group references read their maintained signed combat counters at `+16` and `+2C`, after the original scenario bounds checks. They do not count host actors or derive combat from health. Actor and authored starting-location references reuse the native `27C880` resolver. An actor counts only when active, awareness exceeds two, its signed age is at most twenty signed tick-rate units, and its inclusive behavior stack has no behavior`38`. The behavior owner repeats its actual actor-pool lookup. Reads remain lazy and ordered; a missing input faults instead of returning an invented count. The query does not change floating-point state.

[The native algorithm](../src/campaign/ai/native_campaign_ai_fighting_count_tu15.cpp) uses ordinary typed values and source-record access. It contains no program counter dispatch, virtual registers, instruction decoding, original executable execution or lifted dependency. The existing query enum appends this operation; prior operation values and algorithms remain intact.

[The proof](../analysis/campaign-native-kernels-20261028/ai-fighting-count/proof.json) records:

- 168 complete original queries and resumed evaluator comparisons, plus 78 read-fault prefixes. Unchanged original i386 execution supplies the results and FP state; an offline reference observer records ordered reads. No instructions or dependency results are patched.
- All original sites in`27DDA0`, `1EA090`, `1AFBD0` and the reused`27C880` resolver are exercised. The global-actor branch of the more general iterator is unreachable from this resolver; full HS first-dispatch and result-publication branches remain outside this component's acceptance.
- GCC, Clang ASan/UBSan, ordered reads, immutable backing, signed result bits and eight FP policies pass. Actual campaign caller translation units compile with the new binding.
- Actual source mutations that change the age equality, omit the final behavior entry or substitute the old living-count field fail the retained original comparisons.
- The new 754,127-byte reference repeats exactly. Earlier living-count and sibling-family reference payloads also regenerate byte-for-byte; all three current native regressions pass.

The original reference is required input/output data, not executable instructions. Compiler products, offline reference executables and raw banks were removed from the owned scratch root. Full authored-script scheduling, composed missions and the native-only whole engine remain separate open gates in [the campaign checklist](CAMPAIGN_NATIVE_ENGINE_GOAL_CHECKLIST.md).
