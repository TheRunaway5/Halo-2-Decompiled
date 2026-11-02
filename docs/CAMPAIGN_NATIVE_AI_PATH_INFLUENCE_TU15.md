# Native campaign path influence

The native `27C520` component projects an authored influence onto a framed
segment, computes the original radius test and publishes the original weighted
falloff and distance. It uses the shared native inverse-frame transforms.
Private transformed coordinates and optional private distance results are
ordinary native values; no synthetic source stack or intermediate output bank
is mounted.

The [component proof](../analysis/campaign-native-ai-path-influence-20261030/proof.json)
records 358 unchanged original observations: 180 completed calls, 174 interrupted
prefixes and four actual missing-source faults. All 98 root sites and 258 reached
closure sites are covered. Comparisons include 9,007 ordered reads, 185 stores,
30 pending-quantity comparisons and 174 completed private outputs. GCC, Clang,
actual-core ASan/UBSan, ten deliberately incorrect variants and an identical
independent physical repeat pass.

The projection direction, endpoint fallback, strict radius test, fresh radius
and frame reads, and extended x87 square root follow the original operations.
The first radius read precedes the distance additions; preserving that order
also preserves the FP state when that read is rejected. Rejected reads during
falloff expose a named mathematical quantity with its original extended bits.
Host FP state is restored while the source policy/status remains explicit.

The qualified masked FP profile requires one free x87 temporary when falloff
executes. Full-stack and unmasked exception owners remain open. Private outputs
are qualified for completed disjoint output profiles. Numeric World/parent
transport and complete `1C9F10`/`27B340` navigation search remain open.

The main CMake graph now compiles this component and the shared frame component.
The frozen client compiled and linked both; it does not yet route a real World
call to this kernel. All completed validation and original-execution scratch
owners are removed. The 426,549-byte canonical v2 fixture and the initial
diagnostic reference are retained as original inputs, with exact sizes and
hashes in the proof. No binary, raw bank or render capture is retained.

```sh
python3 tools/run_test.py -- .tools/python/bin/python3 -B tools/oracle_native_campaign_ai_path_influence_tu15.py --output @HALO2_TEST_SCRATCH@/repeat.json.gz --report analysis/influence-repeat.json --verify tests/fixtures/native_campaign_ai_path_influence_tu15_v2.json.gz
python3 tools/run_test.py -- python3 -B tools/validate_native_campaign_ai_path_influence_tu15.py --output analysis/influence-validation
```
