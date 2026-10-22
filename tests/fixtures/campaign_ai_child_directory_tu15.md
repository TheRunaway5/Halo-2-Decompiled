# Original authored-property and child-directory reference

`campaign_ai_child_directory_tu15.json` is an explicitly retained required test
reference: 4,463,734 bytes, SHA256
`83c28fc1455d93d9e939cff47978c202385a0533e3527dc88b041ad8e53e0abd`.
It records unchanged English Xbox TU1.5 execution, image SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The size comes from complete instruction/store paths for the whole constructor
under three prior-content patterns; these are independent golden test outputs,
not banks imported into a game session. No runtime reads the original image.

Fourteen cases exercise each actual authored property list through whole
`1AFA90`. Three more execute whole `1AF970`, which builds all 20 style
directories using those lists and the ordinary child records. All 89 producer
instructions execute, together with 63 of the constructor's 65 instructions.
The existing independent 96-case constructor reference covers all 65. No code
patches, actor records or dependency return values are supplied.

The mutable pointer area is 8,444 bytes at `4FD538`; the 2,620 compact headers
are 10,480 bytes at `4FF798`. Zero/A5/FF prior contents verify that the source
preserves each header's fourth-byte padding. Both complete output areas,
GPR/SIMD state, modeled arithmetic flags (AF excluded), ordered writes and
caller stack are compared to original execution, then restored through the
production whole-bank checkpoint. The native test also verifies the 150 exact
readonly bytes of property-inserted child inputs, unread row holes and rejection
of incompatible existing owners.

The original producer places `47D3CC` in the pointer slot `4FD8CC`, matching
the actual MCC selector's list base `4FD88C` plus index 16. Publishing that
child's authored selection fields closes the reached missing-input boundary;
it does not supply a chosen behavior or qualify the later callbacks.

```sh
python3 tools/run_test.py -- .tools/python/bin/python tools/oracle_campaign_ai_child_directory_tu15.py --fixture tests/fixtures/campaign_ai_child_directory_tu15.json --report analysis/campaign-ai-coop/child-directory-original.json --verify
python3 tools/run_test.py -- python3 tools/test_campaign_ai_storage_cohort.py --child-directory --output analysis/campaign-ai-coop/child-directory-cohort
python3 tools/run_tests.py --target campaign_ai_child_directory_tu15_tests --filter '^original_campaign_ai_child_directory_tu15$'
```

The immutable cohort uses the production Actor/data modules and CMake target,
with the real weapon-query-symbol provider required by concurrent Actor work.
It runs normal and ASan/UBSan checks against the same exact source snapshot.
The enclosing storage wrappers remove every copied source and generated build.
Preserve this original reference; the oracle rejects different replacement bytes.

Regenerate data using `tools/export_ai_behavior_descriptor_data_tu15.py` with
`--property-output` under `HALO2_TEST_SCRATCH`, in addition to the existing
descriptor/child outputs. Use the pinned `--frame-source` described in
[`campaign_ai_children_tu15.md`](campaign_ai_children_tu15.md). Ordinary child
and descriptor bytes remain unchanged. Native component comparisons and actual
MCC campaign continuation have separate evidence in the active
[checklist](../../docs/CAMPAIGN_AI_COOP_GOAL_CHECKLIST.md).
