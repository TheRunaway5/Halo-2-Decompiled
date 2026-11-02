# Native authored area choice

The native component implements `1BBBD0` and the `1BBEC0` initial-choice entry.
It shares the existing authored-area iterator and point projection. It scores
up to 32 returned candidates using original facing/distance arithmetic, retains
strict score ties and updates the real mask, zone and area fields. Exhaustion
resets the original actor fields and retries the real selection algorithm.
The initial-choice entry clears request+C before actor or iterator reads.

The [component proof](../analysis/campaign-native-ai-area-choice-20261030/proof.json)
records 877 original observations: 66 completed calls, 807 read/write prefixes
and four actual missing-storage faults. All 206 root sites and 566 reached
closure sites are covered. Native comparisons check 76,799 ordered reads,
1,225 stores and 180 pending-quantity comparisons, along with hardware FP
state, host FP preservation, live original values and mutable-input histories.
GCC, Clang, actual-core ASan/UBSan and nine meaningful incorrect variants pass.

The masked scoring profile requires two free x87 temporaries. Interrupted
scores expose named mathematical quantities with their actual extended bits;
the component models no CPU stack. Full-stack and unmasked exception handling
remain outside the profile. Nonterminating original retry inputs are not
converted into a successful or capped selection.

The initial reference correctly executed original instructions but omitted a
memory read because Capstone did not mark `TEST [mask], bit` as reading memory.
The canonical v2 explicitly records compare/test memory reads. The
[initial diagnostic](../analysis/campaign-native-ai-area-choice-20261030/metadata-v1-diagnostic/diagnostic.json)
and v1 input remain preserved; no native algorithm change was made to fit that
incomplete observation.

The iterator's new read-only zone accessor is covered by complete reruns of
both earlier area-query suites: 480 previous first-area cases and 271 two-area
cases, including their previous compiler, sanitizer and mutation checks.

The numeric binding now passes 421 independent original caller cases: ten
completed calls and 411 interrupted packets. The reference executes the real
`1B0D1C` CALL, captures its actual callee frame and observes retained argument
publications and late RET reads. Native comparisons cover 39,432 reads, 876
stores, 80 pending-result comparisons and six late-return cuts. GCC, Clang,
actual-core ASan/UBSan, eight transport mutations and an identical original
repeat pass. Unavailable caller/FP ownership is rejected before data access.

The wrapper reads request SP+8, clears request+C, then publishes the adjusted
pointer in the actual retained argument word at `1BBECD`. The shared core reads
the actor only after the original pool identity and resolves later selection
pointers lazily. Rejected score operands retain the real named extended values
ahead of older caller values. Completed AL is published before the late return
read; a rejected return never advances SP. Private arithmetic/GP frames are not
reconstructed or resumed.

The actual cold factory previously lacked the original distance-penalty
operand. The source audit authenticates `1BBDA6` and the eight original bytes
at `46A328` in `.rdata`. The exact immutable field is now declared, and the
real factory regression checks its extent, bytes, rejected writes and restored
ownership. No live navigation or actor state is initialized by that declaration.

The [caller audit](../analysis/campaign-native-ai-area-choice-20261030/caller-audit.json)
authenticates descriptor `48A9D8+18 -> 1BBEC0`, the complete `1B0CD0` owner and
its actual `1B0D1C` bool consumer. Ten independent physical packets execute the
real CALL and RET8, including true/false results and real frame dependencies.
The main CMake graph and actual World route include the qualified binding. The
[cohort-v2 client](../analysis/campaign-native-armory-20261030/cohort-v2/current-source-cohort.json)
executes it once during natural authored Armory startup, with one two-area
query, zero script/content faults and no reference/cache writes. It continues
to the next actual missing callback, `1B0AD9 -> 1BBEE0`, at tick10. Both the
initial frozen build and authenticated source refresh pass all five component/
timer tests while preserving that honest Armory failure. Complete training,
native-only closure and launcher promotion remain open.

The [World cleanup receipt](../analysis/campaign-native-armory-20261030/cohort-v2/cleanup.json)
verifies the finite source/build owner removed, captured source unchanged and
normal launcher selections preserved. No runtime binary or render frame is
retained from this investigation.

The proof records retained original inputs with exact sizes and hashes, and
verifies completed source/build/original-execution scratch cleanup. No binary,
raw memory bank or rendered frame is retained.

```sh
python3 tools/run_test.py -- .tools/python/bin/python3 -B tools/oracle_native_campaign_ai_area_choice_tu15.py --output @HALO2_TEST_SCRATCH@/repeat.json.gz --report analysis/area-choice-repeat.json --verify tests/fixtures/native_campaign_ai_area_choice_tu15_v2.json.gz
python3 tools/run_test.py -- python3 -B tools/validate_native_campaign_ai_area_choice_tu15.py --output analysis/area-choice-validation
```
