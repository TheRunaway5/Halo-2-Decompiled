# Original reference-frame projection tables

The complete v4 client enters the recovered actor decision path, then stops at
`219041` in the existing `219030` owner: the valid sixth-row table word at
`44ACD2` has no runtime backing. Three Marines reach tick125, with zero script
faults and an initial checkpoint. This is an engine table dependency, not a
converted-tag readiness value or a missing navigation result.

Both existing whole owners are preserved: `218FE0` projects components using
`44AC8C`; `219030` uses its inverse at `44ACB0`. Each table has six rows of
three signed shorts, 36 bytes. Original `123210` returns axes0..2; original
`14CEA0` forms a Boolean orientation and indexes by `2*axis+orientation`.
Every row is a permutation of components0..2, and the paired rows are exact
inverses. Authored reference-frame metadata is copied by original `2184A0`;
consumers use its selectors without introducing a new clamp. Malformed
selectors and arbitrary metadata outside the established domain are unqualified.

`campaign_geometry_projection_tu15.json` executes 1,152 unchanged whole calls.
Both 17-instruction bodies execute completely. Cases cover all six selectors,
poisoned upper selector bits, six raw vector bit patterns, two caller padding
patterns and separate/exact/forward-overlapping/backward-overlapping outputs.
Signed zeros, subnormals, infinities and NaN payloads are copied without
supplying a transformed coordinate. Registers, SIMD lanes, modeled flags,
instruction order, ordered writes and retained memory are compared. Native
checks also verify the canonical read-only tables and checkpoint restoration.
This does not qualify full navigation, collision queries or actor decisions.

Production reuses the existing36-byte forward provider in native world memory
and adds only the authenticated36-byte inverse table. Existing algorithms
and their bodies remain unchanged. The component test extracts those exact
bodies and their complete production address mapper; it supplies no dependency
returns or identity-map replacement. The first component build caught a
missing mapper in the test extractor. That failed run is retained as a
diagnostic and is not acceptance. Corrected normal/ASan/UBSan checks use the
immutable `decision-geometry/component-v2` cohort and pass all15,125 combined
cases. Its snapshot is unchanged; the later callable-data checkout edit is
recorded separately. The owned component source/builds are removed.

The full v5 engine build received an explicitly recorded
validation-extractor correction before its custom command started. Engine
sources, fixtures and runtime results are unchanged. Its original input receipt
is preserved; an effective input receipt records the one helper refresh. The
runner reports exactly that snapshot difference, and its compiled slice
matches the declared mapper/body/output hashes. This audit does not override
the actual failures: all six native startup/actor cases fail on duplicate
forward-table publication, despite the isolated component pass. V5 is not
runtime acceptance; its owned source/builds are removed.

The corrected publisher leaves the existing forward provider untouched and
exports only the missing inverse. The focused test now composes the exact
production forward publication and its encoder with final startup, detecting
the duplicate-owner failure before a complete client build. Corrected
component-v4 normal and ASan/UBSan checks pass in an unchanged snapshot and
captured checkout, and its owned source/builds are removed. The full immutable
v6 client is running. V6 also runs the existing complete cold-world ownership test.

Repeat the original reference and regenerate its data through owned storage:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tools/run_test.py \
  --scratch-root .local/test-scratch/campaign-ai-coop -- \
  .tools/python/bin/python tools/oracle_campaign_geometry_projection_tu15.py \
  --verify --fixture tests/fixtures/campaign_geometry_projection_tu15.json \
  --data-output '@HALO2_TEST_SCRATCH@/projection-data.inc' \
  --report analysis/campaign-ai-coop-20261020/decision-geometry/original-repeat-v1.json
```

Compare the generated include with
`src/native_tu15_campaign_geometry_projection_data.inc`. The combined native
component target now has 15,125 cases. Use
`tools/test_campaign_ai_storage_cohort.py --object-decisions` through
`tools/run_test.py`, or the main `campaign_ai_object_decisions_tu15_tests`
target and `^original_campaign_ai_object_decisions_tu15$` filter.

Required original reference: 1,815,380 bytes, SHA-256
`c322bbeae1fedbc736f672f7813c97bdcbdbf742881689066b7340a285cab059`.
Data include SHA-256:
`a81e6e0bfd06f4581af96b1ce6dc1d9275250b7fb28b48889a224a1bdecd85ae`.
The earlier72-byte publisher is preserved only in diagnostic source receipts;
it is superseded because it duplicated the existing forward provider.
Original executable SHA-256:
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
No binary, raw bank or render capture is retained by this reference.
[Original calls and tables](../../analysis/campaign-ai-coop-20261020/decision-geometry/original-repeat-v1.json),
[native component checks](../../analysis/campaign-ai-coop-20261020/decision-geometry/component-v4/source-cohort.json),
[declared full-build helper correction](../../analysis/campaign-ai-coop-20261020/current-source-v5/declared-test-helper-refresh.json).

[Failed v5 composition and declared-helper audit](../../analysis/campaign-ai-coop-20261020/current-source-v5/declared-refresh-and-failure-audit.json),
[Current original/data reference](../../analysis/campaign-ai-coop-20261020/decision-geometry/original-reused-forward-v2.json),
[Complete v6 checks](../../analysis/campaign-ai-coop-20261020/current-source-v6/current-source-cohort.json).

[Corrected provider composition and cleanup](../../analysis/campaign-ai-coop-20261020/decision-geometry/component-proof-corrected-v4.json).

The complete unchanged v6 client now passes cold-world ownership, original
activation, the15,125-case main component target, both native seed cases and
three300-tick actor controls. Marines reach199 ticks before a new mutable
child-row head write at1AFE1B ->47BE14; full actor/mission acceptance remains
open. No original references are opened and no MCC caches are written. Its
owned source/builds are removed and later checkout drift is recorded.
[Full v6 result and cleanup](../../analysis/campaign-ai-coop-20261020/decision-geometry/full-native-proof-v6.json).
