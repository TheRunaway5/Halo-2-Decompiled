# Original camera-effect requests and damage operands

The offline authority is English TU1.5, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Campaign runs use prepared MCC content. No original executable is opened by
the campaign runtime.

The existing original1578A0 owner consumes the seven-WORD camera-effect
request type table at47524C. Its14 bytes are authored writable .data, not a
completed camera result or an actor binding. The recovered source declaration
in `unknown_153950.cpp` supplies the bound; every value agrees with TU1.5.
The following two-byte gap and separate director-name table remain unowned by
this publisher. Native tests check writable access and checkpoint restoration.

`campaign_camera_effect_request_tu15.json` independently executes204 whole
original calls. All33 instructions are observed across type/priority/time
guards,18 overlapping copies,18 mutable type overrides and raw float inputs.
The focused CMake target extracts the exact existing production body into
temporary build source. The game continues to use its existing owner.

Unicorn omits SSE sticky exception updates. An audit of unchanged original
MULSS1578BB confirms matching result bits but a missing precision flag. The
original emulator fixture remains unchanged. The separate hardware reference
executes every complete case on the host i386 CPU, single-stepping6,091 original
instructions without patches or supplied returns. The actual156A2E CALL's
following instruction supplies the retained stop router. GP/SIMD, defined
flags, normalized instruction order, exact ordered writes and output bytes
agree with the emulator reference. Its98 sticky-status differences are
retained explicitly. The native regression compares **full MXCSR** against
`campaign_camera_effect_request_hardware_tu15.json`; no status bits are masked
from that comparison. Host CPU evidence does not prove physical Xbox CPU or
visual acceptance.

The next actual Marine path reached the existing damage-delivery ownerCBD80
(canonicalCAC00), whoseCBEEB comparison needed46AB2C. Existing AI literals
end at46AB2C and exclude that four-byte read. The complete fixed-operand
inventory covers8,976 authenticated selected-image sites in the existing
damage module. Its125 fixed reads consume128 .rdata bytes in24 exact regions.
`campaign_damage_readonly_tu15.json` authenticates those bytes and read sites;
native tests check every published byte and read-only access. Four indexed
operands retain their separate bounds/ownership requirements. Mutable fields,
tag bindings, damage outcomes and service returns are not exported. This data
publication does not qualify full damage delivery or every branch of the module.

Required independent reference inputs added by this increment:

| Reference | Bytes | SHA-256 |
| --- | ---: | --- |
| `campaign_camera_effect_request_tu15.json` | 497,866 | `56a02f8477d4386a25b0edf57c0e54fe9a91796602b1612bca4c706a607139c5` |
| `campaign_camera_effect_request_hardware_tu15.json` | 23,455 | `01155ee638058c4ac027293dbb6bf62425dcbd3385ee4f1ef6703f4d649e2dd2` |
| `campaign_damage_readonly_tu15.json` | 10,980 | `2ec2975fb45b2e3d82d2da064c3f0946363e23c9df81f8d878d31f5e1bbdb829` |

Total532,301 bytes. All ELF/image/bank/trace outputs, copied sources, objects,
test builds and private clients use disposable storage. Preserve required
references, frozen sources, original assets and selected runtimes.

The combined component target remains
`campaign_ai_object_decisions_tu15_tests`, filter
`^original_campaign_ai_object_decisions_tu15$`. The camera/fixed-damage increment executes11,953 cases;
the hardware status sidecar and damage operand inventory add verification,
not extra native execution cases.

That increment's source cohort passes normal and ASan/UBSan; main CMake also passes.
Actual Johnson, Elite and Grunt cases each pass300 source ticks with initial
checkpoints and zero script faults. Marines pass the new camera/fixed damage
reads and stop at11266F ->44AA04, within an indexed table whose bounds and
element meaning remain unqualified. The final scoped
[proof and cleanup](../../analysis/campaign-ai-coop-20261020/damage-operands/component-proof-v1.json)
records source pins, repeats and removal of18 owned paths with empty scratch.
A later concurrent edit changed the shared physics file; its five tested
bodies and shared helper are unchanged, as recorded by the separate hash audit.
No full-client or live mission/co-op acceptance follows from these checks.

The subsequent [reaction increment](campaign_damage_reaction_tu15.md) qualifies
the indexed tables and authored name IDs, raising the combined target to12,399
cases. It preserves existing read-only/writable owners. Marines pass44AA04 and
next stop at274A40 ->274E30, still at tick45; the earlier reports above remain
historical regression evidence.

Run the source cohort through its owned lifecycle:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tools/run_test.py \
  --scratch-root .local/test-scratch/campaign-ai-coop -- \
  python3 tools/test_campaign_ai_storage_cohort.py --object-decisions \
  --compiler clang++ --output analysis/campaign-ai-coop-20261020/damage-operands/cohort-v1
```

Regeneration helpers, all run through `tools/run_test.py`:

- `export_campaign_camera_effect_type_data_tu15.py`: generated14-byte table;
  write `--output` under `HALO2_TEST_SCRATCH`, retain its small `--report`.
- `oracle_campaign_camera_effect_request_tu15.py`: preserve the emulator
  reference with `--fixture`, `--report` and `--verify`.
- `oracle_campaign_camera_effect_hardware_tu15.py`: read the emulator reference
  with `--input`; preserve the hardware sidecar with `--fixture`, `--report`
  and `--verify`. Its original CPU binaries and raw banks remain in scratch.
- `export_campaign_damage_literal_data_tu15.py`: regenerate exact fixed data
  under scratch with `--output`; preserve the byte/read-site `--fixture` using
  `--verify` and retain its small `--report`.

Actual mission/navigation/combat, checkpoint replay and cooperative session
acceptance remain open in the [goal checklist](../../docs/CAMPAIGN_AI_COOP_GOAL_CHECKLIST.md).
