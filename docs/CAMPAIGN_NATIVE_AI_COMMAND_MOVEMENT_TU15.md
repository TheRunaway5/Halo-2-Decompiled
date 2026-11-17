# Native TU1.5 command movement

`native_campaign_ai_command_movement_tu15.cpp` owns the ground, nonswarm
kind 1 (`cs_go_to`) and kind 3 (`cs_go_to_and_face`) command paths. It publishes
the original navigation and facing requests into the actual actor record.
The existing native actor, navigation and physics owners consume these requests.

| Entry | ABI | Work |
| --- | --- | --- |
| `262A00` | stack actor, unit, state pointer, command; AX; RET10 | Reset, validate authored points, initialize navigation |
| `263140` | same arguments; EAX; RET10 | Update navigation, wait for actual facing, finish or retry |
| `263AA0` | same arguments; AX; RET10 | Clear movement request or state flag |
| `263BD0` | ECX actor, EAX command; AL; RET | Publish authored navigation and kind 3 facing direction |
| `264860` | EDX point reference; AL; RET | Validate actual scenario point set/index |

Kind 3 is required by Armory scripts 39/40 (`cs_guns_zapper` and
`cs_guns_zapper_prompt`), script 45 (`cs_johnson_tram`), and script 46
(`cs_lookat_macgun`). The first pair uses `guns_points/zapper` and
`guns_points/zapper_face`; the later scripts use `johnson_points/tram` / `earth`
and `tram_2` / `macgun`. Authored evidence is retained in
`build/test-results/armory-native-progress-20261113/authored-training.json`.
The actual run reached the missing evaluator at tick 3014 after the first shield
dialogue. The raw HS evaluator and its real parser/completion/yield belong to
`native_campaign_hs_command_builtins_tu15.cpp`.

Kind 3 first clears its state counter and validates both points. Its actual
`1FC720` navigation call is shared with kind 1. On successful navigation it uses
native `218EB0` and `30BD0` to compute and normalize the authored facing vector.
When navigation finishes, native `2B3E0` normalizes its XY projection. The exact
source dot-product comparison determines whether to publish a facing request
and continue waiting or invoke the real movement cleanup. The planar helper
retains the original x87/SSE operations, FP controls, rounding and store order.
It restores the host FP environment before observer callbacks.

The unchanged physical English TU1.5 XBE is pinned to
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The kind 3 reference has 581 cases: 567 whole returns and 14 explicit
`218EFB -> 218880` boundaries for two points in the same non-NONE frame.
It includes the exact zapper coordinates, differing frames, invalid point
references, zero/finite/NaN facing vectors, and accepted/rejected facing dots
across seven FP profiles. The original and native replay agree on all retained
DATA, 112,420 ordered stores (111,748 with exact exposed PCs), defined results,
saved GP/ESP and retained FP state. The existing 348 kind 1 cases still pass.
The complete initializers/updaters/cleanup calls execute actual native children;
no child results are supplied by the fixture.

A further 49 whole `262710` scheduler calls compose the real `262870` callback
frames with initialization, ongoing navigation, facing wait, completion and
cleanup. All return normally, matching 16,471 ordered stores (16,387 exposed
PCs), retained DATA and 343 result/saved-GP/FP assertions. The actual sleeping HS
record and clock are input state; no evaluator result or wake value is supplied.

Eight deliberate errors are rejected, including skipped second-point validation,
reversed facing vectors, premature completion, and an incorrect normalization.
The movement owner, vector-math owner and replay driver pass ASan/UBSan/LSan on
all 581 cases; reused child objects are not reinstrumented. Ordered reads,
unconsumed volatile GP/XMM, private child frames, arbitrary aliases and arbitrary
fault continuations are outside this proof. Other command kinds and auxiliary
unported children remain explicit refusals.

The canonical input is
`tests/fixtures/native_campaign_ai_command_movement_kind3_tu15.json.gz`
(1,544,059 bytes). Evidence, source hashes, mutation diagnostics, parent
composition and verified scratch removal are recorded under
`build/test-results/armory-native-progress-20261113/command-kind3/`.
The scheduler reference is
`tests/fixtures/native_campaign_ai_command_lifecycle_kind3_tu15.json.gz`
(234,007 bytes).
The superseded new 504-case capture is removed after promotion; the prior kind 1
canonical input and historical references are preserved.
