# Original campaign AI working storage

`campaign_ai_working_mask_tu15.json` is a local ignored reference input from
unchanged English Xbox TU1.5, SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The 20-case fixture is 60,804 bytes, SHA256
`60ca8b2a9cd044bb905c161c97f3c46139d6d5f54da610b0a9e1c829db5120fe`.

Actual caller `1B08D9` passes `56AE84` to complete `1B0720` at `1B08F1`.
The original source writes all five words of the 20-byte working mask before
reading it. The address lies beyond the original writable section's file
payload, in its loader-zeroed virtual tail. The cold declaration supplies
only that destination; source actor/tag/squad/default-mask owners still produce
every resulting mask bit.

Original runs use squad, property and fallback mask sources, inherited masks,
guarded target state and four different prior-content patterns. No instruction
patch or dependency return is supplied. They execute 97 of the complete
consumer's 122 instructions; target-query branches remain separate dependencies.
All 20 output bytes are independent of prior destination contents in these
cases. Native tests reuse the complete production AI module and actual startup
data mounter, comparing GPR/SIMD state, defined flags, ordered stores and original
caller-stack bytes. They check exact storage size, adjacent holes, cold access,
whole-bank checkpoint/restore and rejection of repeated cold construction.

The initial real MCC continuation passes the previous `1B07AC -> 56AE84`
failure and next stops at `1B0902 -> 4FD524`. It does not pass 60 ticks.

```sh
python3 tools/run_test.py -- .tools/python/bin/python tools/oracle_campaign_ai_working_mask_tu15.py --fixture tests/fixtures/campaign_ai_working_mask_tu15.json --report analysis/campaign-ai-coop/working-mask-original.json --verify
python3 tools/run_tests.py --target campaign_ai_working_mask_tu15_tests --filter '^original_campaign_ai_working_mask_tu15$'
python3 tools/run_tests.py --sanitizers --target campaign_ai_working_mask_tu15_tests --filter '^original_campaign_ai_working_mask_tu15$' --jobs 1
```

`campaign_ai_decision_scratch_tu15.json` records the next original gathering
prefix, stopping at `1B09E0` before later behavior callbacks. Its 18 cases are
434,736 bytes, SHA256
`74c20aab4dd7308600a34eb890d2a1500349bc75868c175c8cd6cddd71f25b4d`.
The focused normal and full-module ASan/UBSan tests now match all 18 prefixes,
including complete register/store paths, untouched bytes and whole-bank
restoration (38 mask/gathering cases total). This does not qualify the later decision
callbacks. The prefix establishes the context scalar, 131-kind/three-lane/24-byte candidate matrix,
131 count bytes and five-word bitmap. Each candidate writes 17 distinct bytes
within a 20-byte extent, preserving bytes 7, 10, 11 and 20..23. It covers
high/cross-word IDs, repeated IDs, inactive entries and poisoned previous
storage. The prefix never reads a watched byte before its source write.
The canonical cold-data exporter and native declarations supply exactly 9,607
loader-zero bytes across the five new bounded owners. They do not set the
computed mask, counts, candidates or context in place of original execution.

The existing descriptor data exporter also authenticates 35 compiled-frame
reads of the original behavior sentinels. Their shared extent is exactly ten
bytes at `47C1A4`; an existing owner supplies the first four, and six missing
bytes are published. The last two unread file bytes remain unowned. Native
normal/sanitized checks verify their readonly bytes, the trailing hole and
rejection of incompatible existing owners. The actual MCC continuation passes
these reads and next reaches child metadata `1B00A6 -> 47BE0C`. Three actors and
zero script faults are reported through tick 30; the 60-tick run still fails.

```sh
python3 tools/run_test.py -- .tools/python/bin/python tools/oracle_campaign_ai_decision_scratch_tu15.py --fixture tests/fixtures/campaign_ai_decision_scratch_tu15.json --report analysis/campaign-ai-coop/decision-scratch-original.json --verify
```

Preserve existing reference inputs; regenerate absent fixtures only. Scratch
storage, binaries and raw traces are removed by the wrappers. Full AI decision,
mission/command-script, navigation, checkpoint and cooperative-session gates
remain in [`CAMPAIGN_AI_COOP_GOAL_CHECKLIST.md`](../../docs/CAMPAIGN_AI_COOP_GOAL_CHECKLIST.md).
