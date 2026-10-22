# Original behavior-child list construction

`campaign_ai_children_tu15.json` is a local ignored independent reference input
from unchanged English Xbox TU1.5, SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
It is 356,738 bytes, SHA256
`d39b338e0ee18a53b0fc53e4c553eb02101843a901bc35c2675bb644a2a56463`.

The original descriptor directory supplies 16 kind-1 parent lists and 105
20-byte child rows. Original `1AF9F1`/`1AF9F6` provide their literal table/count;
whole `1AFA90` owns property-command merging, ordering and the bounded output
pointer list. The fixture exercises all 65 original constructor instructions
with all five command variants and absent property commands. Of its 96 cases,
95 complete; one deliberately retains the original opcode-2 read through an
empty/null child table at `1AFB1C`. No instruction, row, return or successful
outcome is supplied to bypass that failure.

The property commands are declared component inputs. Their embedded child row
is an unchanged original authored row, and the original produces every output
pointer and count. These inputs do not qualify live mission property admission.

The data exporter publishes only each selector's actual ID/flag, timer word and
seconds fields (offsets 0..5 and 8..11), totaling 1,050 bytes. Original startup
already provides each first DWORD. The original 20-byte stride does not grant
ownership of the unread padding at 6/7 or the tail at 12..19. The original
five-command jump table also needs all 20 bytes; startup previously exercised
only its first DWORD. Descriptor caches, CRT mask results and AI decisions
remain original-runtime-owned.

```sh
python3 tools/run_test.py -- .tools/python/bin/python tools/oracle_campaign_ai_children_tu15.py --fixture tests/fixtures/campaign_ai_children_tu15.json --report analysis/campaign-ai-coop/children-original.json --verify
python3 tools/run_tests.py --target campaign_ai_working_mask_tu15_tests --filter '^original_campaign_ai_working_mask_tu15$'
python3 tools/run_tests.py --sanitizers --target campaign_ai_working_mask_tu15_tests --filter '^original_campaign_ai_working_mask_tu15$'
```

For data regeneration, run `tools/export_ai_behavior_descriptor_data_tu15.py`
through `tools/run_test.py`, with `--output` and `--child-output` under
`HALO2_TEST_SCRATCH` and a bounded `--manifest` outside scratch. Compare the
descriptor output before publishing: its existing bytes are unchanged by the
child extension. Preserve previous original fixtures; the oracle refuses to
replace different evidence. No Xbox executable is read by the native runtime.

The sentinel audit still pins the original compiled-frame snapshot with SHA256
`0faad001eff14edaaa31df28eddcde059ab855a5a3639fff4b81e3526ee15308`.
If independent frame work has changed that file, supply the authenticated
snapshot with `--frame-source`; do not relax the pin. At this increment, the
repository's `HEAD:src/source_ai_frame_tu15_generated.cpp` has that hash and can
be copied into owned scratch for regeneration. The current native component is
qualified separately against its exact source snapshot.

`tools/test_campaign_ai_storage_cohort.py --output PATH`, through
`tools/run_test.py`, freezes the production target and actual include closure
before running both normal and sanitizer checks with `tools/run_tests.py`.
Its source receipt identifies every copied input and subsequent checkout drift.
The wrappers remove the source snapshot and both build trees at the end.

Later behavior callbacks, the complete AI decision phase and authored MCC
continuation remain separate acceptance gates in the active
[checklist](../../docs/CAMPAIGN_AI_COOP_GOAL_CHECKLIST.md).

The current 134-case cohort passes normal and ASan/UBSan comparisons of complete
GPR/SIMD state, defined flags, instruction paths, ordered stores and caller
stack, preserved output tail, exact readonly fields/holes and whole-bank
restoration. Actual MCC continuation passes the former ordinary-child field
failure, then reaches property-inserted child metadata `1B02AF -> 47D3CC`.
The 60-tick test remains failing. The 15 original property-command rows and
native whole-directory producer comparison remain the next qualification.
