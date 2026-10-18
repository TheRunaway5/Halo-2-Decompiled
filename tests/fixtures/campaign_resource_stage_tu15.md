# Original TU1.5 campaign resource-stage fixtures

Authority: unchanged English OG Xbox TU1.5 XBE, SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The activation generator adds exactly the complete191CF0 resource-stage owner:
45 static PCs /152 unchanged original bytes. The shared module now has49
complete bodies /3138 instructions; all48 prior complete bodies are preserved.
The actual191CA0 selector remains a separate required owner.

Generate fresh local fixtures with the owned original executable and the local
Unicorn/Capstone interpreter; existing evidence is never overwritten:

```sh
./.tools/python/bin/python -B tools/oracle_campaign_resource_stage_tu15.py \
  --xbe "$TU15_XBE" \
  --helpers tools/reference/halo2-decompiled/tools \
  --output tests/fixtures/campaign_resource_stage_tu15 \
  --execute-original-after-grant
```

The original-data payloads are ignored local assets. Retain this document and
the oracle; do not force-track XBE bytes or generated JSON. The standalone
`original_campaign_resource_stage_tu15` CTest compiles the complete generated
module and caller, includes include/src and links only nlohmann_json. It shares
the cleanup target's supported GNU/Clang non-MSVC x86 guard and strict source FP
options. Missing report.json returns77; corrupt present fixtures fail1. No
World or Halo archive is linked. The ordinary root CMake build/CTest is pending.

Twelve declared contexts cover equal positive/zero/negative selectors, changes
from zero/negative/positive selectors, empty and sparse callback tables,
duplicate targets and missing selector/release/acquire owners. The tested152
original bytes are unchanged. The external selector is explicitly a test
MOV-EAX/RET program. Resource callbacks are explicit test canary writes,
volatile-register clobbers and RET/RET4 programs, not recovered resource
implementations or supplied engine success. Unavailable owners stop before
execution with partial original state preserved.

Independent original generation passed12 cases:890 original instructions,
277 fixture instructions,150 ordered source stores,51 fixture stores,
11 executed selector fixtures,24 release and27 acquire fixtures, and3 missing
owner stops. Its13 files are report.json plus the12 named cases.
Native observed/unobserved comparison passes24 cases,22 selector fixtures,
102 resource fixtures,6 preserved missing-owner stops and150 observed ordered
source stores. It checks complete declared banks, dynamic source-PC order,
source store PC/order/before/written, external return PCs and the actual acquire
stack argument, final or failed GP/XMM and defined flags mask0xCC5, empty x87/MMX
state and source/host FP controls. The old48-body module rejects the missing
entry. All five declared controls have their expected exits0/1/0/77/1:
baseline missing-capability handling, baseline rejection, candidate comparison,
missing-fixture77 and corrupt-present1. Actual input/open qualification and
terminal cleanup pass; the earlier control-routing failure remains retained.

Evidence is retained under
`analysis/opening-cinematic-20261015/campaign-resource-stage-owner-v1`:
`original-stage-v2/terminal.json` and the complete final native controls in
`native-controls-stage-v3/result.json` and `native-controls-stage-v3/terminal.json`. No actual allocation/free callbacks,
resource readiness, full startup, World/session, camera, Vulkan output or visual
parity is proved by this bounded CPU fixture suite. No readiness flag is set.
