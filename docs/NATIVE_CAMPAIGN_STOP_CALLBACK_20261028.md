# Native ordinary/weapon sound stop callback

The complete `18F6B0` stop notification and its `BCDF0`/`D3300` dependencies are
semantic native code in `native_campaign_audio_stop_callback.hpp`. This closes
the ordinary/weapon stop-notification boundary recorded in the preceding audio
retirement audit. Request predicate `18F1A0`, effect callbacks and stream/backend
teardown remain separate unresolved capabilities.

The callback checks the actual game-state byte, validates the retained object's
salt and type mask, reads the sound tag from the actual source row, then clears
the object's sound association only if it still matches that tag. Object type
shifts preserve the original modulo32 rule. Association lookup performs the
original fresh fixed-stride header lookup and signed attachment-offset read;
it does not reuse the validation query's body pointer. No transform, PVS,
provider success, backend result or COM operation is supplied.

All three algorithms use typed game, source, object-header and object-body
views. There is no CPU/register machine, virtual x87 stack, instruction dispatch
or lifted fallback in their implementation. The ABI adapters only transport
actual arguments, the object-query result and the original stack cleanup.

## Actual original caller contract

Ten cases execute unchanged `129E30` with the actual pinned ordinary `44EBF4`
and weapon `44EC14` descriptors. Each enters the real indirect call at `129E88`,
captures the actual callee frame at `18F6B0`, and executes the unchanged callback
through its return to `129E8A`. No caller instruction, register or dependency
return is replaced. The callback frame contains actual context, source and
reason words pushed by the original caller; the original callback ignores reason.

These cases select the original voice-NONE and cache-unpinned branches. Their
caller prefix performs no retained-bank write, which is checked at the actual
callee entry. This authenticates the callback ABI, not the unresolved stream
branch or the remainder of `129E30` after `129E8A`.

The receipts retain descriptor hashes, actual frames, prefix PCs, input/output
FP state and actual live EBX/EBP/ESI/EDI/ESP values. The callback pops 12 argument
bytes plus its return address. Its return is void: the next original instruction
calls `129EA0`, whose initial assignments overwrite EAX/ECX/EDX before consuming
them. The native contract therefore owes preserved GPRs, stack cleanup, all SIMD
registers, x87 operands and FP state, without reconstructing dead scratch GPRs.

## Physical original and manager evidence

The offline reference runs unchanged i386 instructions from the user's owned
TU1.5 image on the physical CPU, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
There is no emulator, instruction patch, supplied original return, unknown-owner
resumption or fake native provider. Native results are independently computed.

119 cases execute 4,613 original instructions: 4,293 callback/helper instructions
and 320 original caller-prefix instructions. All 69 callback/helper instruction
sites execute (22 callback, 35 object lookup, 12 association clear). Comparison
matches 935 mutable callback-domain reads with store-count interleaving, 21 ordered
stores, complete retained banks, actual return widths/stack cleanup, preserved
registers and 80-bit operands, and unchanged FP controls/status/MXCSR.

Cases include game inactive, NONE/invalid/stale object handles, signed salts,
extent/type-mask gates, zero bodies, signed attachment offsets, matching and
nonmatching associations, an association aliasing the source tag, and different
generic/fixed header strides. Unmapped pools on rejected branches remain unread.
Three independent mutations are rejected: skipping the type mask, inverting the
association match and using generic stride for the fixed-stride association lookup.

Clang ASan/UBSan and GCC pass through the actual `native_tu15_sound_manager_run`
path. The focused test omits unrelated group inventories; it supplies no
algorithm dependency or provider result. Only the complete `18F6B0` callback is
independently registered in the production manager. The test verifies that
`BCDF0` and `D3300` are not globally overridden. Their typed implementations stay
internal to the callback/campaign candidate graph. Live reference/oracle modes
and whole gameplay acceptance are outside this offline qualification.

Root focused CMake tests also pass the actual callback manager route, existing
low8/full/low16 contract regression and campaign semantic gate. Eleven actual
campaign candidates have explicit semantic/original proof (four required and
seven additional dependency owners). The forty-entry frame gate remains closed: fifteen
existing required candidates remain unqualified and twenty-one remain absent.

Final evidence under `analysis/campaign-native-stop-callback-20261028/`:

- `clang-manager-caller-sanitizers-v1/report.json`: original cases, all 69 sites,
  ten actual-caller receipts and rejected mutations.
- `root-manager-targets-v1/report.json`: three focused registered tests and the
  retained final GCC/original report.
- `semantic-gate-v4/report.json`: real eleven-candidate metadata and rejection
  of presence-only qualification.
- `caller-continuation.json`: authenticated caller continuation and receipt summary.
- `table-metadata-proof.json`, `cleanup.json`: stable algorithm/adapter prefix,
  final source hashes and scratch removal.

The previously completed channel, ducking, fade, listener and retirement
algorithms remain unchanged, including the single shared pool-delete owner.
Raw banks, ELF files, vectors, temporary group/mutation fixtures and builds are
removed by the official scratch wrappers. No large artifact is intentionally retained.
