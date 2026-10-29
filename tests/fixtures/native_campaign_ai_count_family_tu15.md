# Native campaign count and strength family

English Xbox TU1.5 executable SHA256:
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

The generic typed `native_campaign_ai_query_tu15` implements these authored
script operations over the actual retained AI bank:

| Query | Original evaluator | Original `27DC40` input/observable result |
| --- | --- | --- |
| `ai_living_count` | `2AF6A0` | mode0; signed squad+0xA counters; SHORT bits |
| `ai_swarm_count` | `2AF7D0` | mode1; signed squad+0xC counters; SHORT bits |
| `ai_nonswarm_count` | `2AF830` | mode2; clamp each squad's signed living−swarm difference to zero, then sum; SHORT bits |
| `ai_strength` | `2AF770` | mode0 with mean-output request; REAL bits from original population-weighted vitality |

The count modes share the existing source-faithful group iterator and packed
actor/starting-location resolver. All retain the original population/vitality
reads and scalar arithmetic. A resolved direct actor returns count1 for each
count mode and reads the actual unit vitality. Strength uses that raw vitality
for a direct actor; squad/group strength divides the accumulated weighted
vitality by the signed population only when the total is positive, otherwise
publishing positive zero. It does not weight by the living counter.

`native_campaign_ai_count_family_tu15.json` independently captures the three
new operations: 192 complete calls with their resumed original evaluators and
154 read-fault prefixes. Its required payload is 953,375 bytes. The existing
383,485-byte canonical living-count `_v3.json` remains
unchanged and registered as its own regression. Both execute the real
resumed HS argument/result owners without patched instructions or supplied
callee outcomes. Physical i386 execution owns the result bits and FP status;
Unicorn supplies ordered-read provenance. Paired NaN controls distinguish
operand order in weighted accumulation and division. Source reads, fault
prefixes, ambient FP restoration and all eight masked rounding/FTZ/DAZ policies
are compared by the same native executable.
These results qualify the tested x86_64 GCC/Clang builds; MSVC and non-x86
floating-point behavior are separate qualification gates.

The production query has no executable loader, CPU emulator or lifted
fallback. Its explicit FP policy/result comes from the current world policy
(`027F`/`1F80`); the other policies are component controls. Shared CPU
thread/frame sticky-status ownership, including whether a particular save or
reset owner captures it, remains unqualified. No speculative HS/checkpoint
state was added. This component does not qualify first-dispatch HS expression
evaluation, every arbitrary `27DC40` caller/output combination, complete AI
behavior or full campaign progression. `ai_fighting_count` uses the separate
`27DDA0`/`1EA090` closure and is not included here.

Regenerate the new original reference through the storage wrapper:

```sh
python3 tools/run_test.py --scratch-root .local/test-scratch/campaign-native-ai-query -- \
  uv run --no-project --with capstone --with unicorn python \
  tools/oracle_native_campaign_ai_query_tu15.py --family \
  --fixture tests/fixtures/native_campaign_ai_count_family_tu15.json --verify \
  --report analysis/campaign-native-kernels-20261028/ai-count-family/original-repeat.json
```

Use `tools/test_native_campaign_ai_query_tu15.py` under `tools/run_test.py` to
exercise both production CMake registrations, compile the real campaign
callers, and reject a privately compiled wrong-counter mutation. `--sanitizers`
explicitly enables ASan/UBSan in that disposable cohort. Required JSON inputs
and compact evidence survive; ELF images, raw source banks and builds do not.
