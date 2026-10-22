# Original actor decision-workspace and property-query components

The complete current-source Marine probe reaches `1B8612 -> 1B7D20` at tick63
after three authored actor births, zero script faults and an initial checkpoint.
Its missing original closure contains 14 bodies and 1,736 instruction sites.
The actor behavior module now has 90 bodies and 6,880 original sites; its
previous 76 bodies and 188 immutable operand bytes are preserved exactly.
It exports 24 additional authenticated read-only operand bytes. Mutable actor,
property, workspace and pool fields remain owned runtime inputs.

`campaign_ai_decision_workspace_tu15.json` independently executes 278 cases
from unchanged English Xbox TU1.5 instructions: 230 whole calls and 48
prefixes. It exercises the root's real aligned stack probe, actor/property
lookup, output clearing, workspace lease acquisition and planning handoff;
pool bitmap scanning and matching; actor/query association with wrapping
reference count; original cached/invalid selector guards; and zero-distance
geometry handling. The root stops before planning selection. Geometry,
navigation, object placement, allocation and collision service boundaries are
explicit, with no supplied return, candidate or path result.

Complete bodies are translated from authenticated original instruction
graphs. Component execution covers selected paths, including every instruction
of `277860` and `2778E0`; unexecuted branches remain unqualified. The native
test compares general/SIMD registers, modeled flags, original instruction
order, writes with prior bytes, retained outputs and checkpoint restoration.
Full hardware floating-point status, complete navigation/combat and a full
mission are not established by these references. All13,973 combined native
component cases pass normally and with ASan/UBSan in one unchanged snapshot;
the captured checkout inputs also match. Module/data generation repeats
exactly. The component source/builds are removed. Composed runtime acceptance remains open in the
separate complete current-source v4 client build.

Repeat without replacing the reference:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tools/run_test.py \
  --scratch-root .local/test-scratch/campaign-ai-coop -- \
  .tools/python/bin/python tools/oracle_campaign_ai_decision_workspace_tu15.py \
  --generation analysis/campaign-ai-coop-20261020/actor-current/generation-v2/generation.json \
  --verify --fixture tests/fixtures/campaign_ai_decision_workspace_tu15.json \
  --report analysis/campaign-ai-coop-20261020/actor-current/reference-repeat-v1.json
```

Regenerate the module and operands with
`tools/generate_campaign_ai_actor_behavior_tu15.py` under
`HALO2_TEST_SCRATCH`, then compare canonical source files. The existing
`tools/test_campaign_ai_storage_cohort.py --object-decisions` freezes the
production component target and runs normal plus ASan/UBSan validation.
Main CMake target: `campaign_ai_object_decisions_tu15_tests`; filter
`^original_campaign_ai_object_decisions_tu15$`.

Required reference: 1,123,199 bytes, SHA-256
`7f56b5144cdcd3082e5769bfebab3d926dafca7a98285182fa8d12ddb2f21778`.
Original executable SHA-256:
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
No instruction patches or dependency returns are supplied. No binary, raw
bank or render capture is retained.
[Original coverage](../../analysis/campaign-ai-coop-20261020/actor-current/reference-v1.json),
[source preservation](../../analysis/campaign-ai-coop-20261020/actor-current/generation-v2/preservation.json),
[native component qualification](../../analysis/campaign-ai-coop-20261020/actor-current/component-v1/source-cohort.json).
