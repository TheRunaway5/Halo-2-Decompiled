# Original TU1.5 property creation

The current reference, `campaign_ai_prop_creation_admissible_tu15.json`,
records 576 unchanged whole calls to `274450`/`274490`. Both recovered bodies
cover all 50 original instructions. They use the existing `273B70`, `274200`
and `274020` owners for classification, real allocation and initialization;
no service, allocation or classification result is supplied.

Declared component fields exercise object indices 0 and 2047, fresh/recycled/
full pools, priority rejection/admission, allocation-salt rollover, epoch wrap
and duplicate classification stamps. Recycled-pool live counts match the
occupied bitmap. These are consumed component inputs, not proof of complete
pool construction or every older record's gameplay state. The earlier
`campaign_ai_prop_creation_tu15.json` is preserved unchanged as a diagnostic:
its recycled-pool live count was three with two occupied entries. Its older
cohort remains historical evidence. Only the corrected 576 cases execute in
the current combined 13,695-case target; the old reference is input-only.

The existing object-decision module now contains eight whole bodies and 447
original instruction sites. Its six earlier bodies and 20 immutable operand
bytes are unchanged. Original Frame siblings, including `274670`, are reused.
The reference compares general/SIMD registers, modeled flags, ordered original
instruction sites, ordered stores with prior bytes, and retained pool, object,
recipient, stamp and stack bytes. Checkpoint checks restore these outputs.
Full hardware floating-point status is not established by this emulator
reference. Dependency coverage remains explicit: `273B70` 125/233, `274200`
45/170, `274020` 93/100 and the generic allocator 77/78 sites. No complete
reclamation, perception or gameplay qualification follows from these calls.

Exact startup storage totals 8,197 writable loader-zero bytes: latch `4EABC8`
(1), epoch `4EABCC` (4), and 2,048 DWORD object stamps at `4EABD0` (8,192).
Original constructor `B87F0` supplies 2,048 object-header slots; the generic
header/allocator bounds high-water growth, and valid-object lookup precedes
stamp indexing. The extent is established by these owners, not neighboring
padding. The three alignment bytes between latch and epoch stay unowned.
The publisher validates existing access before adding missing fields and
preserves prior writable values and native backing lifetimes. Native checks
cover fresh zeros, mutability, checkpoint restore, shared live owners and
rejection of a read-only stamp before missing fields are admitted.

Regenerate the corrected original reference without replacing it:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tools/run_test.py \
  --scratch-root .local/test-scratch/campaign-ai-coop -- \
  .tools/python/bin/python tools/oracle_campaign_ai_prop_creation_tu15.py \
  --admissible-pools --verify \
  --fixture tests/fixtures/campaign_ai_prop_creation_admissible_tu15.json \
  --report analysis/campaign-ai-coop-20261020/prop-create/original-admissible-repeat-v2.json
```

Omit `--admissible-pools` and use the older filename to repeat the diagnostic
reference. Generate the whole module with
`tools/generate_campaign_ai_object_decisions_tu15.py` and cold storage with
`tools/export_campaign_ai_prop_creation_storage_tu15.py`, through the same
owned storage lifecycle. Generated source/data outputs belong inside
`HALO2_TEST_SCRATCH`; compare them with their canonical source files.
The immutable component check uses
`tools/test_campaign_ai_storage_cohort.py --object-decisions` through
`tools/run_test.py`. Main CMake target:
`campaign_ai_object_decisions_tu15_tests`; filter
`^original_campaign_ai_object_decisions_tu15$`.

The coherent MCC Marine probe passes property creation and advances from
tick45 to tick51 with three authored actors, zero script faults and an initial
checkpoint. It next stops in the existing `219330` owner at
`21942E → 505E88`, under root `17EAD0`. Shots/hits remain zero; changed shield
state is not combat acceptance. Full mission, navigation/combat, checkpoint
replay and co-op remain open in the [active checklist](../../docs/CAMPAIGN_AI_COOP_GOAL_CHECKLIST.md).

Retained original references are 5,773,582 bytes each (11,547,164 total):

- Corrected: SHA-256 `d0e76ed06f9881b737d9d511e3482c89e3e5cfdace39104ef20ef5b5f8235463`.
- Earlier diagnostic: SHA-256 `994f46fb14e2f0356f745f9abfab66b23fab9ecda2f91707ff38956f6d4d3e3f`.

The whole module is SHA-256
`17e53643ab8fa8cf5c15417dfc1ca1415406b5c3100f9278e0bc21d928e04cd4`;
the storage publisher output is
`e17858afb3d2c8dba6481fab574870508a8b1f84fb8c174ba347c447ea82d3a3`.
[Corrected original coverage](../../analysis/campaign-ai-coop-20261020/prop-create/original-admissible-v2.json),
[bounds](../../analysis/campaign-ai-coop-20261020/prop-create/object-pool-bounds-v1.json),
[storage](../../analysis/campaign-ai-coop-20261020/prop-create/storage-v1.json),
[publication](../../analysis/campaign-ai-coop-20261020/prop-create/publication-v1.json),
[actual MCC continuation](../../analysis/campaign-ai-coop-20261020/ai-runtime/c55-prop-create-four-families-threehundred-v1-runtime.json).

All13,695 current cases pass normal, ASan/UBSan and main CMake with the
corrected reference. All192 recycled-pool input and original output live
counts match bitmap occupancy. The final component snapshot and checkout
match; all355 frozen C55 providers remain pinned. Johnson, Elite and three-
Grunt controls each pass300 source ticks with zero script faults and initial
checkpoints. None of the four probes opens original references or writes MCC
caches. All16 recorded owned source/build/client/dependency paths are removed
and scratch is empty. The references above are retained; no binary, raw bank,
capture or promoted runtime remains.
[Source hashes, corrected counts and cleanup](../../analysis/campaign-ai-coop-20261020/prop-create/component-proof-v2.json).
