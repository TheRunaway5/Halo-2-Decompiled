# Original AI object-tracking inputs

Two required independent reference inputs use original TU1.5 XBE SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`:

| File | Bytes | SHA-256 |
| --- | ---: | --- |
| `campaign_ai_object_tracking_tu15.json` | 115,755 | `ab142ec7909b99d4d3fda51d6bdfed55c4fef42c9135820b34083bf382b7efb8` |
| `campaign_ai_tracking_dispatch_tu15.json` | 1,577 | `ead71500e583e9c95e980cea6aee6ae85fc7f51386068c50854acd3d7ac15df9` |

`tools/oracle_campaign_ai_object_tracking_tu15.py` authenticates nine rows with
original IDs0..8 and sixteen-byte stride. Twelve indexed original consumers
and `267320`'s row-pointer reads select four WORD fields at+2/+4/+6/+8:
72 bytes. The data owner excludes row IDs and unread six-byte tails. This
defines no new runtime index bounds rule and claims no other caller indices.

54 whole `267320` cases cover all nine rows, statuses0/1/2 and both flag states,
executing all 28 instructions.18 `274020` prefixes cover all rows and zero/A5
previous contents, executing14/100 instructions through the actual type-field
comparison. The prefix stops before object-pool lookup or later dependencies.
GPR/SIMD, modeled flags exceptAF, exact paths/stores and output padding match.

`tools/export_native_tu15_ai_tracking_dispatch_data.py` authenticates the
four-row descending predicate search and indexed fallback in whole `274FA0`.
Only its predicate/handler identities are published:32 bytes, excluding row
headers. The original rows retain the uncompiled handler `2753C0`; the audit
reports this required owner explicitly and supplies no successful return.
Static table evidence does not qualify behavior of those callbacks.

Both generators require the pinned original compiled-frame snapshot SHA-256
`0faad001eff14edaaa31df28eddcde059ab855a5a3639fff4b81e3526ee15308`
through `--frame-source`, and put generated `.inc` data under
`HALO2_TEST_SCRATCH`. Preserve current concurrent Frame changes; export the
reference from its recorded Git revision into owned scratch. They reject any
change to an existing reference fixture. Run normal/sanitizer checks with:

```sh
python3 tools/run_test.py -- python3 tools/test_campaign_ai_storage_cohort.py --descriptor-callbacks --compiler clang++ --output analysis/campaign-ai-coop/tracking-cohort
```

This reruns all230 descriptor selector cases too. It tests the actual Frame and
production data mounter, with real weapon-query-symbol provider and exact
readonly field extents. Builds, snapshots and clients are removed by wrappers.
