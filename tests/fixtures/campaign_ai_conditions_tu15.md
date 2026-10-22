# Original campaign AI conditions

The local ignored `campaign_ai_conditions_tu15.json` is a required original
reference input, executed from English Xbox TU1.5, SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The current 330-case fixture is 770,476 bytes, SHA256
`62a652f3c8214cc853142b322df4a4119758449ec199c1909822a1879b005ac7`.
The first 210-case reference is preserved byte-for-byte in
`campaign_ai_conditions_tu15_v1.json` (506,921 bytes, SHA256
`35673dec3db6404a2cdeb042a5b7693fc697a31ef47e87e3abedf922a4243476`).

Actual MCC three-Grunt continuation reaches `29ACAF -> 29AFD0` inside original
AI frame `1D0670`. The new condition module retains whole `29AFD0`, `20B500`,
`20B570` and `2802F0`: 393 original instructions. It reuses existing original
frame/HS/actor iterators (`1494F0`, `211750`, `29AC70`, `29B380`, `27CAA0`,
`27CB30`) and campaign group iterators (`20D070`, `20D0D0`) in the same bank.
Recursive calls use the same dependency routing. No copied actor, supplied
script return or new owner for an already implemented routine is used.

The oracle declares raw source records, executes unchanged instructions and
performs actual nested calls. It covers signed/float comparisons, NaNs,
negation, inactive/absent references, time, recursive conditions, player-slot
holes, actual actor traversal and distance arithmetic. Native checks compare
complete GPR/SIMD state, defined flags, instruction order and ordered writes,
including previous bytes and the original caller stack. Architecturally
undefined AF is excluded. Full x87/hardware-frame parity is a separate gate.

In the first reference, 202 cases complete. Eight stop only at the actual `22CB90` safety-service
entry and compare the real prefix, stores and call ABI; no return is supplied.
391/393 new instructions execute. `29B2E7`/`29B2E9`, after that safety call,
remain unverified. The safety implementation and complete original HS mission
scheduler remain open; an inactive raw HS component input is not a campaign.

The extension adds whole original `203B60` (60 instructions), preserving its
source request admission and actual `213D30` handoff. Its 120 added cases cover
the special request kinds, signed-byte limits, campaign bit test, other source
game types and the original nonvolatile/caller-stack contract. Existing `C1E90`
and `213D30` bodies retain their source owner. Fifteen of these cases stop at
the real multiplayer `160A60` entry, comparing its call ABI without supplying
a return; that function's existing world owner remains separate.

The complete extension has 453 emitted instructions and 330 cases: 307 complete,
23 explicit dependency-boundary prefixes. Coverage executes 450/453 new
instructions; `203BDB`, `29B2E7` and `29B2E9` remain beyond the declared component
boundaries. Normal and ASan/UBSan comparisons pass. This is neither complete
request/safety parity nor original mission/command-script scheduling.

The generated table companion publishes both `29AFD0` and reused `29AC70`
dispatch/byte-translation tables, including exact original padding, through the
existing read-only engine constant owner. Original table contents choose branch
targets; no captured decision is installed as data.

Regenerate only when the fixture is absent; preserve existing reference data.
Use the owned TU1.5 image only offline, through disposable test storage:

```sh
python3 tools/run_test.py -- .tools/python/bin/python tools/oracle_campaign_ai_conditions_tu15.py --fixture tests/fixtures/campaign_ai_conditions_tu15.json
python3 tools/run_test.py -- .tools/python/bin/python tools/oracle_campaign_ai_conditions_tu15.py --fixture tests/fixtures/campaign_ai_conditions_tu15.json --verify
python3 tools/run_test.py -- .tools/python/bin/python tools/generate_campaign_ai_conditions_tu15.py --output src/source_campaign_ai_conditions_tu15_generated.cpp --directory analysis/campaign-ai-coop/ai-conditions/generation
python3 tools/run_tests.py --target campaign_ai_conditions_tu15_tests --filter '^original_campaign_ai_conditions_tu15$'
python3 tools/run_tests.py --sanitizers --target campaign_ai_conditions_tu15_tests --filter '^original_campaign_ai_conditions_tu15$' --jobs 1
```

The composed MCC test passes both former owner failures, advances through tick
30 with three actual Grunts, and uses the complete AI fixed-readonly profile
(`tools/export_native_tu15_ai_literal_data.py`, 818 reads, 724 bytes). Its next
failure is writable destination `1B07AC -> 56AE84`. The 60-tick run has not passed. Current evidence and remaining
requirements live in [`CAMPAIGN_AI_COOP_GOAL_CHECKLIST.md`](../../docs/CAMPAIGN_AI_COOP_GOAL_CHECKLIST.md).
