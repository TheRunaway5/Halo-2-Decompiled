# Native campaign living-count reference

Authority: English Xbox TU1.5 executable SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The executable is an **offline reference only**; the production query contains
typed C++ traversal and native scalar arithmetic, with no executable loader,
instruction decoder, GPR transport, lifted dispatch or fallback.

The canonical input is `native_campaign_ai_living_count_tu15_v3.json`.
It retains 96 whole calls to mode0 of original `27DC40` and the corresponding
whole resumed `2AF6A0` evaluator calls, plus 50 read-fault prefixes through an
SNaN-bearing group traversal. The real original `212010` argument owner and
`211DA0` publisher execute; no instruction is patched and no callee result is
supplied. The ready HS continuation is a declared input, so this does not
qualify first-dispatch expression evaluation, all HS scheduling, or the other
count modes and optional outputs of `27DC40`.

Physical i386 execution observes returned count bits and SSE exception/status
bits. Unicorn provides ordered data-read provenance, checked against physical
results. Its SSE status is retained as a diagnostic, **never** as the native
FP expectation: 70 of the final 146 cases differ from the physical result.
The eight masked SSE policies cover all four rounding modes with and without
FTZ/DAZ. The current world's policy is `1F80`; the additional policies are
component controls, not a claim about live world selection. x87 state is untouched
by this query. Unmasked hardware traps and non-x86 arithmetic remain separate
qualification work.

Important original behavior:

- Squad/group counts sum **signed squad+0xA counters**, then the evaluator
  preserves the low16 SHORT bits. They do not count host actor vectors or
  recalculate counts from presentation health.
- The group iterator walks descendants and linked squads in original order.
  Original source pointers and links stay borrowed from the retained world.
- Packed direct/start references resolve original salt/location-name behavior.
  A resolved actor counts once and still reads its unit's vitality, even when
  both optional numeric outputs are null.
- Squad queries still read population and strength and perform the original
  scalar FP accumulation, including its exception flags, before following the
  next link. Read failures preserve the prefix and explicit FP status.

The native kernel accepts an explicit mutable SSE context and restores the
caller's ambient host environment on success and exceptions. The campaign
adapter initializes it from the actual world's `prepare_source_call()` policy
and exposes the resulting context alongside the count. The existing host
`ScriptVm` consumes the count as a SHORT. **Publishing sticky FP status through
the shared native script/frame CPU owner remains open**; its checkpoint
ownership must be established from original execution before adding state.
No per-query shadow
world bank is introduced. Caller translation-unit compilation and component
tests do not qualify a complete campaign mission or live co-op.

Regenerate/verify under the repository storage wrapper:

```sh
python3 tools/run_test.py --scratch-root .local/test-scratch/campaign-native-ai-query -- \
  uv run --no-project --with capstone --with unicorn python \
  tools/oracle_native_campaign_ai_query_tu15.py \
  --fixture tests/fixtures/native_campaign_ai_living_count_tu15_v3.json --verify \
  --report analysis/campaign-native-kernels-20261028/ai-living-count/original-repeat.json
python3 tools/run_test.py --scratch-root .local/test-scratch/campaign-native-ai-query -- \
  python3 tools/test_native_campaign_ai_query_tu15.py \
  --output analysis/campaign-native-kernels-20261028/ai-living-count/cohort-physical-v3
```

The cohort invokes production `cmake/CampaignNativeAiQuery.cmake`, compiles the
real campaign caller translation units, and privately changes the counter
read to squad+8. That compiled mutation must fail the original ordered-read
test. Generated ELF images, original data banks, compiler output, and the
private mutated source belong to disposable scratch and are removed.

The earlier `.json` and `_v2.json` payloads created during this task were removed
after physical v3 superseded them. Compact provenance/rejection reports remain
in `analysis/campaign-native-kernels-20261028/ai-living-count/`; only `_v3.json`
is registered with CTest. The initial emulator-only capture's FP status was
rejected by validation and is not acceptance evidence.
