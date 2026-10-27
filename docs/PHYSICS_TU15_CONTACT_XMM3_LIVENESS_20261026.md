# TU1.5 contact-copy XMM3 liveness, 2026-10-26

In the three exact v1458 expanded-field captures, the copied XMM3 value has **no register read before an actual full overwrite**, including the real branch, call and return successors after `30482B` and `3042C7`. It does survive several returns as register contents; those contents are not consumed before the overwrite. This closes the requested captured-frame register-liveness question without changing any raw comparison gate.

The [summary and source pins](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/contact-xmm3-liveness-summary/report.json) link the independent observations and [full local CFG audit](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/contact-xmm3-cfg-final/report.json).

| Exact capture | Post-store segments | Unchanged original instructions stepped | Real overwrite sites |
| --- | --- | --- | --- |
| Elongation crates | 16 | 1,249 | `304792` ten times; `30383C` three; `2E47B9` three |
| Gemini column0 | 12 | 3,670 | `304792` ten times; `2E44F7` once; `325221` once |
| Gemini column1 | 12 | 3,670 | `304792` ten times; `2E44F7` once; `325221` once |

All 40 segments reach a genuine full definition; none reaches an unknown read, instruction-budget frontier or root return with the old value. An independent disassembly check finds zero XMM3 read sites in the recorded pre-definition instructions. Each executed instruction byte is authenticated against the unchanged English TU1.5 image. The debugger also reproduces the complete 1,077,248-byte ordinary-original heap and exact CW/SW/MXCSR for each capture.

The longer Gemini paths demonstrate why a function-local claim was insufficient. The value survives `304350`'s return to the actual indirect caller at `321B61` (call site `321B5E`, owner `321A70`), the allocator path `149E00 → 233420`, listener/body callbacks, and later world phases. Its first register read still occurs only after a new definition: a matrix load at `2E44F7` in one lifetime, or a pose definition at `325221` in the other. Elongation also includes `304130` returns to `304884` and real calls into `303800` and `2E47B0` before the observed definitions.

## Static scope and remaining boundaries

The source scan identifies two direct callers of `304130`, at `30487F` and `304931`, and the direct `304350` call at `304AE0` in `304AC0`. Every match belongs to an authenticated full CFG. The actual captured indirect call through `321A70` is retained separately. Neither `304AC0` nor `321A70` directly reads or writes XMM3; their callees cannot be assumed to overwrite it.

The local CFG audit explicitly retains branches that can return with the incoming register value or reach an unexamined call. Examples include `303800`'s early return, `30CF40`'s pre-definition allocator call and zero-count return, and `303D00`'s allocator callbacks. Unknown calls are frontiers, not assumed clobbers. The actual captured paths are resolved by unchanged instruction stepping; this does not authenticate every possible indirect caller or every future input.

A definition from another memory record kills the prior **register provenance**. It does not prove that all backing-memory origins are dead. The earlier [complete 27-byte Elongation memory-access observation](PHYSICS_TU15_V1458_EXPANDED_CONTACT_DIAGNOSIS_20261026.md) and the straight copy regions remain separate evidence. Full memory provenance, future-frame/all-input closure, and paired native cohort/allocation history remain open. The strict raw gates stay red: Gemini0/1 retain 129 differing bytes each and Elongation retains 27.

## Concrete native owner candidates

These are authenticated source boundaries to port next, not accepted native implementations. They currently have no corresponding proper native kernel owner in `include/halo2/native*` or `src/physics/native*`.

| TU1.5 owner | Complete source sites | Boundary and dependencies |
| --- | --- | --- |
| `303BA0` | 29 | Record field copy, no calls. Preserve exact read/write order and the omitted `+28..2F`/`+7E..7F` gaps; do not substitute a whole-record copy. |
| `303800` | 60 | Leaf used by the record-filter loop; no calls. Its native numerical port must preserve the original packed/scalar SSE order and FP status. |
| `304130` | 142 | Record-filter/removal loop; its sole direct callee is `303800`. |
| `303EE0` | 36 | Append path; depends on `2E3250` growth and `303BA0`. The source local record preimage cannot be guessed or zero-filled. |
| `2E44D0` | 40 | Call-free SIMD matrix/vector helper, actually reached by the broad-phase path. |
| `2E47B0` | 35 | Call-free SIMD matrix/vector helper, actually reached after contact processing. |
| `2E4690` | 26 | Call-free SIMD helper reached from `30CD00`; full operand/FP qualification is still required. |

All numerical candidates require unchanged-original physical comparisons, alias/fault-prefix coverage and source-faithful live bindings before any native-only gate can close.

## Storage

All three observation owners were verified absent after `tools/run_test.py` cleanup. Their measured allocated temporary footprint was **860,876,800 bytes**, removed after the reports were saved. No new large artifact is retained. Small instruction/call/branch metadata and authenticated source listings remain; protected reference inputs and the selected runtime were untouched. The previously qualified vehicle-rate source pins were independently checked unchanged.
