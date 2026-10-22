# Original actor behavior, scalar and script context

The offline authority is the original English TU1.5 executable, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The native campaign uses prepared MCC content. No original executable or
reference cache is opened by the campaign acceptance runs.

The generated module now contains76 whole original bodies /5,144 instructions.
It reuses the existing Frame and physics owners instead of duplicating them.
It publishes188 authenticated read-only operand bytes. The existing authored
descriptor fields and original mutable-state producers retain their ownership.

The active behavior reference is
`campaign_ai_actor_behavior_inputs_tu15.json`:808 independent original cases,
802 whole returns and six prefixes before the real275FE0 object-update call.
They cover every instruction in1EA360's scalar calculation and1ED690's
inherited behavior lookup, and99/109 instructions in1B7240. They compare full
GPR/SIMD state, modeled flags, normalized instruction order, exact ordered
writes, prior bytes, output preservation and checkpoint restoration. Generated
bodies and branches that are not exercised remain unqualified. These checks
do not establish full x87 status/MXCSR or physical Xbox floating-point parity.

The original262590 context reference,
`campaign_ai_script_context_tu15.json`, has24 cases:18 whole empty/busy guards
and six prefixes before the real211840 callback, observing59/123 instructions.
Its two context DWORDs at50EEE4/50EEE8 lie beyond the original section's raw
bytes in its loader-zero virtual range. The runtime declares only that eight-
byte mutable storage. Original source writes publish actor/script IDs and own
post-callback cleanup. The component prefixes do not qualify that cleanup or
the callback's execution.

The Marine command reference, `campaign_marine_commands_tu15.json`, adds895
cases with880 whole returns and15 actual CALL prefixes. It reuses only the
earlier reference's declared inputs. Nine added helpers execute every original
instruction, including the complete nine-way1D2100 switch. Other helpers retain
their recorded unexecuted branches. The reached Frame parent26C8B0 remains
with its existing owner;22 missing nested bodies are added without duplication.
`campaign_ai_shared_prop_helper_tu15.json` independently exercises all39
instructions of267750 across192 whole calls, including each property/actor
guard and byte return with preserved upper register bits.

`campaign_hs_function_registry_tu15.json` validates all948 original function
slots through211B10, with952 evaluator/script CALL prefixes and four whole
disabled guards, observing70/133 instructions. The original stored count word,
packed definition extent and all389 admitted original MCC profile signatures
agree. The3,792-byte pointer registry keeps authored mutable storage. Its18,892
definition/count/usage bytes are read-only. Existing shared owners are checked
before missing ranges are mounted; their bytes, access and lifetime are kept.
Conflicting metadata is rejected before admitting any missing registry range.
The original null evaluator is preserved. These checks do not qualify all
evaluator bodies or complete HS execution.

`campaign_actor_scalars_tu15.json` adds2,232 cases:2,227 whole returns and
five physical/action CALL prefixes. These cover the21 newly recovered204AA0
dependencies, including105F70/FF7B0, action guards and byte-counter aliasing.
Of these,840 explicitly check original32ECD2 status capture and32ECDD/FNCLEX
over exception, condition, TOP and busy inputs. FNCLEX clears actual backend
sticky flags and retained status bits; full original FP-environment parity
and unexecuted actor/CRT branches remain separate requirements.

`campaign_ai_bsp_audibility_cache_tu15.json` adds105 whole original cache
calls across hits/misses, both audibility representations and counts0..512.
It observes every instruction of203C00,252F30 and253070. The2531E0 equal-
cluster return is unreachable through those real parents and remains
unexercised. The runtime declares the original64-byte loader-zero buffer,
retaining the separate cluster-key owner and originalFFFF constructor.
Expected masks and keys come from original instructions, never cached results.
The test target extracts exact existing production bodies into temporary build
source; production continues to use its existing physics/sound owners.

`campaign_ai_difficulty_tu15.json` adds343 whole original calls, exercising
every instruction of1F19D0,1F1950 andC1E90. All35 remapping rows, team guards,
allied/enemy input bits, modes, difficulty clamps and missing global data are
declared component inputs. The complete70-byte table's bound comes from the
typed source declaration, and every value agrees with TU1.5. This is original
engine metadata; it supplies no live gameplay outcome or converted tag fix.

`campaign_ai_actor_behavior_tu15.json` is retained solely as the initial
control-trace audit reference. It is not in CTest: its native input declarations
were incomplete, and its x87-depth metadata reflected Unicorn's default tag
word rather than an empty caller. The explicit-input reference declares the
reused authored action/type tables and CRT mode and initializes the original
x87 tag word empty. Small migration receipts prove every original instruction
trace, store, GPR/SIMD result, flag and output is unchanged; only input
declarations and the incorrect depth metadata were corrected.

Required independent reference retention for this increment:

| Reference | Bytes | SHA-256 |
| --- | ---: | --- |
| `campaign_ai_actor_behavior_inputs_tu15.json` | 3,467,887 | `ff1727b96f9b49a1d3637868296f1734761a3d3ee9483c9b38303e81235a6330` |
| `campaign_ai_script_context_tu15.json` | 63,201 | `8eea89a86d0a0c8e0bd23adfacd164fd2c1b2153d22b86f59f20338f251d014b` |
| Initial control-trace audit reference | 3,394,322 | `6738b3045cc3a9b8c3e68e121b621a657f72f1a2f50ce9fe280b29dde142e1a0` |
| `campaign_marine_commands_tu15.json` | 5,926,899 | `1d3a09119885e8d902f50feba01d329c771c144e6bec5094324cf62b6b57e759` |
| `campaign_ai_shared_prop_helper_tu15.json` | 1,086,691 | `195076a07f6253e490aace0129720f543bdd1b7679d3395e8ab7ed458ea8bf7a` |
| `campaign_hs_function_registry_tu15.json` | 2,542,940 | `4f5fb6b1493332a231b4372f027556c56c0924565bed969f40bd9d3a842f4622` |
| `campaign_actor_scalars_tu15.json` | 9,820,375 | `93a459131651f1507eff360ae043e8dbbb751984efd51c25d0bde7506622bc4e` |
| `campaign_ai_bsp_audibility_cache_tu15.json` | 3,453,006 | `3d167908f5ec15ec00f8f60dd18bf8b355c2b5c753f891dd7e19be2861ddff83` |
| `campaign_ai_difficulty_tu15.json` | 1,117,744 | `7e903a688cdc6f4c839bde18744bca3fd1c7e4d03381e3a55efbdf7806258f97` |

Total30,873,065 bytes. The scalar/cache/difficulty increment adds14,391,125
bytes of required independent original reference input. Test binaries, copied sources, object files, memory banks
and captures are disposable; none are retained by this increment.

Run the production component target through its storage lifecycle:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tools/run_test.py \
  --scratch-root .local/test-scratch/campaign-ai-coop -- \
  python3 tools/test_campaign_ai_storage_cohort.py --object-decisions \
  --compiler clang++ --output analysis/campaign-ai-coop-20261020/ai-difficulty/cohort-v1
```

The helper freezes the compiler-discovered source/include closure and fixtures
for both normal and ASan/UBSan checks. The main CMake target is
`campaign_ai_object_decisions_tu15_tests`; filter
`^original_campaign_ai_object_decisions_tu15$`. The combined suite has13,695
cases. Original fixtures and generated source repeat exactly through
`oracle_campaign_ai_actor_behavior_tu15.py`,
`oracle_campaign_ai_script_context_tu15.py` and
`generate_campaign_ai_actor_behavior_tu15.py`, all under `tools/run_test.py`.
The added references use `oracle_campaign_marine_commands_tu15.py` (with
`--shared-helper` for267750), `oracle_campaign_hs_function_registry_tu15.py`
and `export_campaign_hs_function_registry_tu15.py`. The current additions use
`oracle_campaign_actor_scalars_tu15.py`,
`oracle_campaign_ai_bsp_audibility_cache_tu15.py`,
`oracle_campaign_ai_difficulty_tu15.py` and
`export_campaign_ai_difficulty_data_tu15.py`; preserve existing fixtures
with `--verify`.

The coherent C55 client now advances actual Johnson through300 ticks. Actual
Marines pass1B7240,1B7000,267750,105F70, the BSP-cache builder and difficulty
selection, camera-effect requests and fixed/indexed damage operands, passing
11266F's44AA04 read,274E30's property-owner lookup and274450/274490 creation
before21942E requires505E88 at tick51. Authored Grunt and
Elite controls pass300 ticks. All355 original
providers remain pinned, and there are no
runtime original-reference opens or MCC cache writes. Complete mission,
navigation/combat, checkpoint replay and actual co-op remain open in
[the goal checklist](../../docs/CAMPAIGN_AI_COOP_GOAL_CHECKLIST.md).

The previous11,749-case scalar/cache/difficulty target passes normal, ASan/UBSan and main CMake.
Generation and all three new original fixtures repeat exactly. Source hashes,
case counts, scoped authored-family results and removal of all16 recorded
owned source/build/client/dependency paths are retained in the
[increment proof and cleanup](../../analysis/campaign-ai-coop-20261020/ai-difficulty/component-proof-v1.json).
The scratch parent is empty. The14,391,125-byte reference addition is retained
as required independent test input; no test binary, raw bank or capture remains.

The earlier11,953-case target additionally checks204 whole camera requests,
their separate full-status hardware reference and the complete128-byte fixed
damage operand set. The [camera/damage fixture guide](campaign_camera_effect_request_tu15.md)
records its532,301-byte required reference addition, exact hashes and
regeneration. The hardware sidecar and data inventory are verification inputs,
not additional native execution cases. Full mission and co-op remain open.

The12,399-case reaction target adds446 selection/preparation comparisons
through the existing production damage module:412 whole returns and34 actual
CALL prefixes. It checks96 immutable indexed bytes and16 authored name-ID
bytes, preserving existing shared owners and rejecting conflicting metadata.
Original bounds and retained coverage limits, regeneration and the1,611,598-byte
required reference are described in the [reaction guide](campaign_damage_reaction_tu15.md).

The13,119-case lookup target adds720 whole original property-owner lookups,
covering every one of274E30's56 instructions, with no supplied lookup result.
The object-decision module now has six bodies /397 sites and the same20
immutable bytes. See the [lookup guide](campaign_ai_prop_owner_lookup_tu15.md).

The current13,695-case target adds576 whole creation calls with bitmap-
consistent live counts, real Frame classification/allocation/init owners and
all50 new sites. The module now contains eight bodies /447 sites and the same
20 immutable bytes. Exact epoch/latch/object-stamp backing totals8,197 mutable
bytes; shared values and native lifetimes survive publication/checkpoints.
The [creation guide](campaign_ai_prop_creation_tu15.md) records original
coverage, retained diagnostic inputs and the new51-tick Marine boundary.
