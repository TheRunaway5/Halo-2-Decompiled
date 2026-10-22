# Original TU1.5 property-owner lookup

`campaign_ai_prop_owner_lookup_tu15.json` records 720 unchanged whole calls to
`274E30`, independently executed from the authenticated original TU1.5 XBE.
Every one of the routine's 56 instructions is observed. There are no patched
instructions, supplied dependency returns or callback prefixes.

The declared component inputs exercise object types 0, 12 and 255; zero and
nonzero root fields; positive and negative signed relative-part offsets; the
relative addition's zero-result guard; and two pool indices with different
handle salts. Ten property-list forms exercise empty lists, direct first/last
matches, related-owner first/last matches, zero flags, related-owner mismatch,
same-index/different-salt object rejection, list exhaustion and the invalid
related-owner sentinel. Two initial byte patterns expose preserved state.
The low object address used for the zero-result guard is a declared component
input, not a captured runtime address or an exported campaign record.

The native implementation preserves full object-handle comparisons, signed
relative addressing, original linked-list traversal and every early return.
It is generated into the existing object-decision module; the five earlier
bodies are byte-for-byte unchanged. The module now has six complete bodies,
397 distinct original instruction sites and the same 20 immutable operand
bytes. The new lookup exports no mutable runtime state or additional operands.
Existing native dispatch uses the module's owner query, so no separate
production implementation or fabricated fallback is introduced.

The original/native comparison checks all general and SIMD registers, modeled
flags, floating-point control preservation, original instruction order, ordered
stack stores with prior bytes, and preserved object/property/list/stack bytes.
The new reference is appended to the existing CMake target as actual execution
cases. The combined target now has 13,119 executions; input-only numeric/type
data and the earlier camera hardware sidecar do not inflate that count.

Regenerate while preserving the reference:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tools/run_test.py \
  --scratch-root .local/test-scratch/campaign-ai-coop -- \
  .tools/python/bin/python tools/oracle_campaign_ai_prop_owner_lookup_tu15.py \
  --verify --fixture tests/fixtures/campaign_ai_prop_owner_lookup_tu15.json \
  --report analysis/campaign-ai-coop-20261020/prop-owner/original-repeat-v1.json
```

Run `tools/generate_campaign_ai_object_decisions_tu15.py` through the same
storage lifecycle with `--output` and `--data-output` inside
`HALO2_TEST_SCRATCH`, plus a small-report `--directory`. Compare generated
bytes with `src/source_campaign_ai_object_decisions_tu15_generated.cpp` and
`src/native_tu15_ai_object_decision_data.inc`. Both repeat exactly. Use
`tools/test_campaign_ai_storage_cohort.py --object-decisions` through
`tools/run_test.py` for the immutable normal/ASan/UBSan component checks.
The main target is `campaign_ai_object_decisions_tu15_tests`, filter
`^original_campaign_ai_object_decisions_tu15$`.

Before recovery, the native reference fails at the exact missing `274E30`
entry. That failure is retained alongside original coverage, publication and
repeat receipts. Component coverage does not establish actual campaign pool
production, combat decisions, complete missions, checkpoint replay or co-op.
Those remain separate gates in the [active checklist](../../docs/CAMPAIGN_AI_COOP_GOAL_CHECKLIST.md).

The required original reference is 4,900,620 bytes, SHA-256
`34235169d0322374cdaad12a29b003eebe092f56090a4df4107a5ef16be43134`.
The generated production module is SHA-256
`32a21a73b3c6febbaca227ea21586b8c7927d37bdb022539b4ed8b6011df4bc4`.
[Original whole calls](../../analysis/campaign-ai-coop-20261020/prop-owner/original-v1.json),
[missing-entry regression](../../analysis/campaign-ai-coop-20261020/prop-owner/cohort-red-v1/normal/report.json),
[publication](../../analysis/campaign-ai-coop-20261020/prop-owner/publication-v1.json),
[exact repeats](../../analysis/campaign-ai-coop-20261020/prop-owner/regeneration-identity-v1.json).

Normal, ASan/UBSan and main CMake pass all13,119 combined execution cases.
The actual MCC Marine frame passes the recovered lookup and next requires
`274450` at `274A4E`, still at tick45. The original no-match result is preserved;
the test does not manufacture an existing property or allocate one itself.
The next creation audit identifies only two absent bodies (`274450`/`274490`,
50 sites); sibling `274670` and the other required Frame bodies already exist.
The exact four-plus-one counter/latch bytes are original loader-zero fields,
whose live ownership and producers remain separate work.
[Actual MCC boundary](../../analysis/campaign-ai-coop-20261020/ai-runtime/c55-prop-owner-four-families-threehundred-v1-runtime.json),
[complete next family](../../analysis/campaign-ai-coop-20261020/prop-owner/next-complete-prop-family-v1.json).

The final immutable component snapshot has no checkout drift. All355 original
C55 provider pins remain unchanged. Johnson, Elite and three-Grunt controls
separately pass300 ticks with zero script faults and initial checkpoints; none
of the four probes opens original references or writes MCC caches. All10
recorded owned paths are removed and scratch is empty. Retain the required
4,900,620-byte original JSON reference; no binary, raw bank, capture or promoted
runtime remains. [Validation, source hashes and cleanup](../../analysis/campaign-ai-coop-20261020/prop-owner/component-proof-v1.json).

The subsequent [creation increment](campaign_ai_prop_creation_tu15.md) raises
the combined target to13,695 cases and advances Marines to tick51. The older
lookup and45-tick receipts above remain historical regression evidence.
