# Original TU1.5 damage reaction selection and backing

`campaign_damage_reaction_tu15.json` independently records 446 unchanged
original executions: 412 whole returns and 34 prefixes stopped before real
actor-limit or animation service calls. No instruction patches or supplied
dependency returns are used. Runtime content comes from MCC; the original
executable is an offline reference only.

The original selector `CBC50` (canonical `CAAD0`) emits 0, 1, 2, 3, 5, 6 and 7.
Its lower range owner `CBB90` writes only immediate 0, 1 or 2 to its range output.
The real consumer `112620` (canonical `111700`) reads DWORDs at index × 4 from
`44A9E0`, `44AA00` and `44AA20`. These bounds require eight rows in each table,
or 96 read-only bytes. Row 4 preserves original bytes; natural selector
reachability for that row is not claimed. The class table ranges from -1 to 3;
nonnegative classes index four authored string IDs at `47C1F0` (16 bytes).
These values are IDs, not pointers.

The existing production damage-delivery module supplies both native routines.
The test target links that module directly; no second implementation is added.
It compares general and SIMD registers, flags, ordered original instruction
sites and stores, stack/output bytes and mutable bank observations. Static
coverage remains explicit: `CBB90` 34/47 sites, `CBC50` 89/92 and `112620`
116/201. Unexecuted branches and callback bodies remain unqualified. The
emulator reference does not establish complete hardware floating-point status
or a complete damage/animation outcome.

The data publisher validates any earlier shared owners before adding missing
authored ranges. Existing access rights and backing lifetimes are preserved;
new authored ranges are writable. Native regression checks cover fresh
publication, exact read-only operands, writable mutation/checkpoint restore,
mixed existing read-only/writable owners, unchanged native pointers and
rejection of conflicting metadata before admitting missing name bytes. This
matters in the C55 client: its original factory already owns the word at
`47C1F8` read-only. The earlier unconditional 16-byte mount failed at startup;
the composed publisher preserves that owner.

Regenerate into disposable storage, preserving the committed reference:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tools/run_test.py \
  --scratch-root .local/test-scratch/campaign-ai-coop -- \
  .tools/python/bin/python tools/oracle_campaign_damage_reaction_tu15.py \
  --verify --fixture tests/fixtures/campaign_damage_reaction_tu15.json \
  --report analysis/campaign-ai-coop-20261020/damage-indexed/original-repeat-v1.json
```

`tools/export_campaign_damage_reaction_data_tu15.py` accepts `--original-report`,
`--output` inside `HALO2_TEST_SCRATCH`, and `--report`. Compare its generated
bytes with `src/native_tu15_campaign_damage_reaction_data.inc`; never silently
replace an existing reference. The canonical startup mounter includes that
data. The native CMake target is `campaign_ai_object_decisions_tu15_tests`,
filter `^original_campaign_ai_object_decisions_tu15$`; all 446 reaction cases
are executions within the combined 12,399-case target. Byte inventories and
the older camera hardware sidecar are verification inputs, not additional
execution counts. Use `tools/test_campaign_ai_storage_cohort.py
--object-decisions` through `tools/run_test.py` for a coherent normal and
ASan/UBSan source snapshot.

The unchanged C55 providers now pass the indexed read at `11266F → 44AA04`.
The authored three-Marine case reaches the next real call at
`274A40 → 274E30`, still at tick 45, with zero script faults and an initial
checkpoint. This is a new source boundary, not evidence of more completed
ticks or a complete mission. No runtime reference opens or cache writes are
observed. Full combat, navigation, mission scripting, checkpoint replay and
co-op remain open in the [active checklist](../../docs/CAMPAIGN_AI_COOP_GOAL_CHECKLIST.md).

The required independent reference is 1,611,598 bytes, SHA-256
`472022f614bb16f7b4d5ada07350814a802fb15b22f9095a2fa9fc36f1928b5e`.
The composed generated data repeats exactly, SHA-256
`8a2030d5b7b80a2a55dac7a672e322d774e679312a7989633f8b49b611743dda`.
[Original coverage](../../analysis/campaign-ai-coop-20261020/damage-indexed/original-v1.json),
[composed data](../../analysis/campaign-ai-coop-20261020/damage-indexed/generation-v2.json),
[original repeat](../../analysis/campaign-ai-coop-20261020/damage-indexed/original-repeat-v1.json),
[data repeat](../../analysis/campaign-ai-coop-20261020/damage-indexed/regeneration-identity-v2.json),
[actual MCC continuation](../../analysis/campaign-ai-coop-20261020/ai-runtime/c55-damage-indexed-four-families-threehundred-v2-runtime.json).

Normal, ASan/UBSan and main CMake pass all 12,399 combined cases. Johnson,
Elite and three-Grunt controls separately pass 300 ticks with zero script
faults and initial checkpoints. All 355 frozen provider pins remain unchanged;
the final component snapshot and checkout match. All 14 recorded owned paths
are removed and scratch is empty. No test binary, raw bank, capture or promoted
runtime is retained. [Source pins, validation and cleanup](../../analysis/campaign-ai-coop-20261020/damage-indexed/component-proof-v1.json).
