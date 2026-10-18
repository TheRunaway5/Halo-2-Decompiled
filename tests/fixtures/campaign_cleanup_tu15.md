# Original TU1.5 campaign cleanup fixture

Authority: unchanged English OG Xbox TU1.5 XBE, SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The existing activation generator adds exactly the original13A7E0 cleanup root:
15 unique PCs /49 unchanged original bytes. Its existing191DF0 failure call at
191E7B then stays inside the recovered module. Public interfaces do not change.

Original13A7E6 clears game+1120; callbacks run from44B848 down to44AEDC inclusive,
stride -0x24 (36bytes),68 rows, skipping null cells. Original13A80B clears game+1.
The routine preserves ESI and returns void. It does not fabricate activation
success or integrate outer map startup.

Regenerate into a fresh directory with the local Unicorn/Capstone interpreter:

```sh
./.tools/python/bin/python -B tools/oracle_campaign_cleanup_tu15.py \
  --xbe "$TU15_XBE" \
  --helpers tools/reference/halo2-decompiled/tools \
  --output tests/fixtures/campaign_cleanup_tu15 \
  --execute-original-after-grant
```

The generator refuses existing evidence and skips77 for an absent owned XBE.
Wrong images or corrupt present inputs fail. The optional standalone CTest is
`original_campaign_cleanup_tu15`; it compiles both the complete generated module
and test caller, includes only include/src and links only nlohmann_json. It is
registered for the supported GNU/Clang non-MSVC x86 backend and skips77 when
the ignored local fixture folder is absent. MSVC/x87 support is not supplied by
FP compiler options and remains unqualified. JSON payloads remain untracked, under the repository fixture
policy; retain this document and the Python generator. Do not force-track XBE
bytes or generated original payloads.

Seven declared unit contexts cover empty-active, empty-inactive, sparse edge
callbacks, all68, duplicate targets, missing-first and missing-middle owners.
Each callback is an explicitly declared test x86 MOV-canary/MOV-EAX/MOV-ECX/
MOV-EDX/RET program, not a recovered engine callback or a supplied engine result.
Its instructions/stores remain separate from original instructions/stores.
Missing implementations stop before execution and retain partial source state.
Each original data access must fit one declared test bank; page mappings do not
admit broader guessed game owners.

Qualified original generation has2387 unchanged source instructions,375 declared
callback instructions,96 ordered source stores and77 actual callback boundaries
including2 missing-owner stops. The seven original goldens are generated without
candidate execution. Their native test runs observed/unobserved for14 comparisons,
150 declared callback invocations and4 retained missing-owner stops. It compares
all owner bytes, source store PCs/order/before/written, dynamic source-PC order,
callback ABI/return stack, final or failed GP/XMM/defined flags and FP controls.
The arithmetic/DF comparison mask is0xCC5; the provided x87 stack is empty.

Native baseline-red/candidate-green validation passes against these exact
original fixtures:14 comparisons,150 declared callbacks,4 preserved missing
owners and96 ordered source stores. The old complete module rejects the
missing entry; missing/corrupt fixtures return77/1. The three complete TUs and
two direct links have qualified dependency, selected-input and source-FP checks.
Evidence: `analysis/opening-cinematic-20261015/campaign-activation-cleanup-owner-v1/native-controls-stage-v3/result.json`.
The current-root standalone CMake target still needs its own build/CTest run.
Full lifecycle callbacks, actual outer map startup, game readiness, World/session
behavior and Vulkan appearance remain separate gates.
