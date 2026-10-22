# Original AI object-decision and workspace evidence

All references use TU1.5 XBE SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
They are required independent test inputs; the native game never imports them.

| Reference | Bytes | SHA-256 |
| --- | ---: | --- |
| `campaign_ai_object_decisions_tu15.json` | 671,407 | `d76850c8e34c658568756156aadc7521b907989e6b3b37a38304d5baf6a082d1` |
| `campaign_ai_workspace_leases_tu15.json` | 558,643 | `434b8a96dd80b0e61d40b55d9d2de1a6a07624bdadb8fcea21dab75eb3bc0406` |
| `campaign_ai_object_callbacks_tu15.json` | 55,780 | `e336503a48db17fb69e1bde7a7cdc684d63cf01bcc23d1a1c7722d49df6ff90d` |

`tools/generate_campaign_ai_object_decisions_tu15.py` lowers complete original
`26EFA0` (162 instructions), `26EEF0` (56), `265620` (69) and `2753C0` (11).
It rejects duplicate original owners and reuses the already-compiled Frame
`1E7810`, `1ECD00`, `213D30` and `25980` with the real ABI. Twenty immutable
operand bytes are authenticated separately; live fields and completed decisions
are never exported. Generation repeats byte-for-byte, including untaken CFGs.

`tools/oracle_campaign_ai_object_decisions_tu15.py` runs 234 whole original cases
and ten prefixes stopped before the actual HS call. It executes every query
and tracking-handler instruction and 276/298 new sites. The remaining team/HS
instructions are explicitly listed as uncovered. Real original character and
RNG dependencies execute; GP/SIMD, modeled flags exceptAF, all ordered writes,
prior bytes, RNG mutation and produced state are compared. REP instruction
observations are normalized to one site; every individual store is retained.
Raw caller/pool/tag inputs are declared component controls, never runtime state.

`tools/oracle_campaign_ai_workspace_leases_tu15.py` independently executes two
whole acquire/release sequences under zero and A5 prior descriptor contents,
plus two prefixes stopped before the nonnull-heap lock call. Source lease
producers initialize descriptors, handle exhaustion/release/reuse and clear
their latch after the last lease. Native callers feed their own returned
pointers back to release. Exactly 29 loader-zero bytes belong to these fields;
descriptor padding and adjacent latch bytes keep their original ownership.

`tools/oracle_campaign_ai_object_callbacks_tu15.py` authenticates all eleven
signed WORD masks/DWORD identities and the null callback terminator consumed
by `269D70`: 70 bytes. 36 prefixes cover byte types, signed extension and masked
x86 shift counts, stopping before callback execution. Static identities do not
qualify callback behavior. The later complete-family increment ports all
fourteen missing callback bodies and seven direct dependencies across both
interfaces; its separate reference and coverage limits follow below.

The initial increment ran302 cases in one immutable source/include/fixture
cohort, normal and ASan/UBSan, with exact stores, padding and checkpoint restore.
The same production target now includes the later callback and query references:

```sh
python3 tools/run_test.py -- python3 tools/test_campaign_ai_storage_cohort.py --object-decisions --compiler clang++ --output analysis/campaign-ai-coop/object-decisions-cohort
python3 tools/run_tests.py --target campaign_ai_object_decisions_tu15_tests --filter '^original_campaign_ai_object_decisions_tu15$' --cmake-arg=-DCMAKE_CXX_COMPILER=/usr/bin/clang++
```

Each original oracle accepts `--fixture`, `--report` and `--verify`; the callback
oracle also requires `--data-output` under `HALO2_TEST_SCRATCH`. The source
generator requires scratch `--output`/`--data-output` and a bounded report
`--directory`. Use the owned wrapper, preserve existing fixtures, and compare
regenerated source/data before publishing. All copied sources, objects, builds,
clients and raw traces are disposable. Retain only these 1,285,830 reference
bytes and small reports. This does not certify full campaign AI or co-op.

## Complete callback families and registered query

| Additional reference | Bytes | SHA-256 |
| --- | ---: | --- |
| `campaign_ai_object_callback_bodies_tu15.json` | 8,321,491 | `ad67500399a6095c8f6c1e27c20ff5fb201f55d9b92dd270312eaf14de12dac6` |
| `campaign_ai_callback_numeric_data_tu15.json` | 1,835 | `1b71260ab2e94a502615b61b4b4efb6aae4c4a45849fb4ea39b315dd37b55dbf` |
| `campaign_ai_query_identity_tu15.json` | 43,915 | `2873817160e19ba21b9ac5c008db9acd6c1259052db48ec18680fdec07ce7f10` |

`tools/generate_campaign_ai_object_callbacks_tu15.py` reads both original
callback tables, recovers the entire missing direct-call closure and reuses
existing whole Frame owners with their original ABI. It exports256 exact
immutable bytes, including bounded switch extents and the second interface's
typed fields; table padding and live state remain unowned. The two source
context destinations add six loader-zero writable bytes, preserving their
two-byte gap. Original producers write their values during the real AI frame.

`tools/oracle_campaign_ai_object_callback_bodies_tu15.py` executes 592 whole
cases and 18 prefixes stopped before the required original `1EA1D0` call.
It covers callback argument kinds, enabled controls, candidate presence and
three explicit component input profiles, plus original vector-helper cases.
The report lists every executed and unexecuted source instruction. In
particular, `CE260`, `E77D0`, `271790`, `2A7450` and `2A74C0` have no executed
instructions in this reference. These bodies are compiled, not behaviorally
qualified. No outside-closure return is supplied. GP/SIMD, modeled flags
exceptAF, ordered stores and source component outputs are compared; full
x87 status/MXCSR parity and live gameplay remain separate requirements.

`tools/export_ai_callback_numeric_data_tu15.py` authenticates all 380 sites of
the existing `2CC8F0` source owner and fifteen fixed readonly reads. Its five
ranges contain24 bytes, including the complete eight-byte reached FLD operand.
Three ranges already have compatible owners, so only12 bytes gain ownership.
This is an operand audit, not a new numeric-algorithm comparison.

`tools/oracle_campaign_ai_query_identity_tu15.py` executes all six instructions
of `267A40`, registered by `1E7E00`, in36 whole original cases. It preserves
full DWORD identities and actual `RET8`, including upper-word/signed controls,
registers, modeled flags and unchanged caller stack. There are no instruction
patches or supplied dependency returns. The complete callback module now owns
22 routines /2,025 original instructions; this target runs 948 reference cases.

Regenerate with the owned wrapper and each helper's `--fixture`/`--report`
arguments (`--verify` preserves existing original references). The numeric
exporter also requires scratch `--output`; the callback source generator needs
scratch `--output` and `--data-output` with a bounded `--directory`. The three
additional required references total8,367,241 bytes. Preserve these inputs;
copied sources, clients, objects, builds and raw traces remain disposable.

`tools/oracle_campaign_ai_prop_selector_tu15.py` adds
`campaign_ai_prop_selector_tu15.json` (2,344,497 bytes, SHA-256
`98fdb81e72bf62764e562bcc3378bb5e5a0113cc49f84497ef49eb6c6e412df7`).
Its964 whole original calls exercise all 61 `272100` instructions, all 14
`26DA40` classifier sites and all twelve WORD cells in the declared three-row,
four-column domain. Actual original lookup/helper calls execute; no dependency
results or computed selectors are supplied. Exactly24 new immutable bytes
belong to that consumed matrix; its adjacent row remains unowned. Inputs
include null context, guarded property kinds, three produced classes, target
presence and zero/negative/positive/NaN response fields. The original report
lists unexecuted sites in the two existing lookup helpers. The same target
now contains 1,912 reference cases, including the earlier948. Regeneration
requires `--fixture`, scratch `--data-output`, `--report` and `--verify`.

Those cases also reuse the existing nine original type-field rows from
`campaign_ai_object_tracking_tu15.json` for the real nested `2673A0` reads.
The first component attempt lacked the second kind's field at `47D646`.
The corrected test merges the two authenticated immutable input owners,
rejects disagreeing overlaps and leaves the original case fixture unchanged.
All 1,912 cases then pass in the same normal/ASan/UBSan source cohort and the
main project CMake target. This is a test input declaration correction.

`tools/oracle_campaign_ai_actor_state_tu15.py` adds
`campaign_ai_actor_state_tu15.json` (1,072,621 bytes, SHA-256
`e230cdd6b2de77d8023d8cf3d640e00c7bd9ed0428fa6880ecd5ded72824b90e`).
Its 168 whole original calls reach every `1FB3C0` instruction (43/43), with
real original `1FB2A0` waypoint/vector and `200A40` fallback-state execution.
Their narrower 39/63 and 37/127 coverage is explicit; their unobserved branches
remain requirements. State/active guards, signed count/index boundaries,
fallback controls, zero/A5 priors, complete actor bytes and stack outputs are
retained. This port extends the decision module to five bodies /341 original
instructions, reusing both existing Frame owners and preserving its 20 immutable
operand bytes. The production target now runs 2,080 original reference cases.

The registered `267A30` bucket query already has a complete original Frame
implementation and public dispatcher case. Its missing runtime route now
forwards to that owner through the callback module; no second implementation
is generated. Original `1E7E00` registers it with `267A40` in the adjacent push.
`tools/oracle_campaign_ai_query_bucket_tu15.py` executes 286 whole calls covering
all 256 buckets, high-word/signed key controls, all four instructions and actual
`RET4`. `campaign_ai_query_bucket_tu15.json` is330,266 bytes, SHA-256
`a3ef82e83d44afd9703cb127b325c925782ce24bfa67d8541c8e0db8b8aa067b`.
The callback module still compiles22 new bodies /2,025 original instructions
and delegates that existing query. The combined target now runs 2,366 cases.
