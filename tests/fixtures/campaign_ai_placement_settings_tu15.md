# Original campaign AI placement settings

The local ignored `campaign_ai_placement_settings_tu15.json` contains 272 cases
executed from the authenticated English Xbox TU1.5 image, SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Its SHA256 is
`831b51a76c98375c192a6091081909de51bf899ba25cb5503a5af1a1b66d8071`;
size is 655,150 bytes. This is a required small reference input, not a runtime
asset or a supplied gameplay result.

Actual authored Cairo squad `1st_cov_wv1_grt` reaches `1E8BBF -> 20B980`.
`20B980` selects character settings using original game type/difficulty and
the signed-short selector; its `1ECC80` dependency follows parent characters
until it finds a positive block count or no parent. The new module preserves
both whole bodies, all 74 instructions and the actual four-entry jump table.
No semantic name is inferred for the selected character values.

The oracle executes every original instruction and all switch branches across
all four difficulties, direct/inherited/absent blocks, nonpositive block counts,
non-campaign default and the actual campaign-only override. It includes
positive, zero, negative and NaN setting bits and signed-short selector handling.
It patches no code, stubs no calls and supplies no dependency returns. Declared
raw character/options records are component inputs, not a full encounter.
Native comparison covers complete general/SIMD registers, defined flags,
instruction order, ordered stores and their previous bytes, call-stack writes
and unchanged output bytes. Five missing-owner controls require failure at the
reached original read. AF is excluded where x86 leaves it undefined.

Use the owned offline original only for regeneration/verification. Preserve
existing evidence; regenerate only if absent, and verify byte-for-byte on a
separate execution:

```sh
python3 tools/run_test.py -- .tools/python/bin/python tools/oracle_campaign_ai_placement_settings_tu15.py --fixture tests/fixtures/campaign_ai_placement_settings_tu15.json
python3 tools/run_test.py -- .tools/python/bin/python tools/oracle_campaign_ai_placement_settings_tu15.py --fixture tests/fixtures/campaign_ai_placement_settings_tu15.json --verify
python3 tools/run_test.py -- .tools/python/bin/python tools/generate_campaign_ai_placement_settings_tu15.py --output src/source_campaign_ai_placement_settings_tu15_generated.cpp --directory analysis/campaign-ai-coop/placement-settings/generation
python3 tools/run_tests.py --target campaign_ai_placement_settings_tu15_tests --filter '^original_campaign_ai_placement_settings_tu15$'
python3 tools/run_tests.py --sanitizers --target campaign_ai_placement_settings_tu15_tests --filter '^original_campaign_ai_placement_settings_tu15$'
```

The runtime dispatcher forwards the actual placement call to this module in
the same retained bank. Its generated companion
`src/native_tu15_campaign_ai_placement_settings_tables.inc` publishes the exact
16-byte original switch through the existing immutable engine-table owner.
Actual MCC encounter tests follow separately from the
component comparison. The running goal and acceptance gates are in
[`docs/CAMPAIGN_AI_COOP_GOAL_CHECKLIST.md`](../../docs/CAMPAIGN_AI_COOP_GOAL_CHECKLIST.md).
