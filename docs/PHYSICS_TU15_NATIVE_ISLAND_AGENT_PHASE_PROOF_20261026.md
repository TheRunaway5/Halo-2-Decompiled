# Native TU1.5 island agent phases

The candidate native module implements actual TU1.5 `321D20` (97 sites),
`321A70` (205), contact-buffer construction `321180` (51), destruction
`321220` (55), and contact minimum `320510` (63). It uses typed memory,
object identities, native integer/x87 expressions and actual service owners.
There is no GP register file, instruction decoder or supplied collision result
inside the production kernels.

The executable authority is English TU1.5 XBE
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Canonical old address names elsewhere are lookup identities, not TU1.1 evidence.

## Current source and API

- `include/halo2/native_tu15_island_agent_phase.hpp` SHA-256
  `1b1a2e96279e9fdad19a46b711718e4995cf1d351cde208ca569c315cba0e31e`.
- `src/physics/native_tu15_island_agent_phase.cpp` SHA-256
  `8a6c1b66c2e38a318818f2da3e985e8a2a7d2e69fbba6c51f5d00d554ef875ad`.

`native_tu15_collide_island_flag_clear` and `...flag_set` take the actual
island, configuration, mutable memory, FP status/control, and caller-owned
scratch header/output addresses. The header needs 20 bytes; the pointer-holder
output needs 4 bytes for flag clear or 8 for flag set. Their lifetimes must
extend through every genuine callback, outside its private residual call frame.
Flag set takes the actual preserved incoming x87 depth; at most seven values
may remain underneath its private scalar. The native expression never reads or
replaces these incoming values. The host floating state is saved/restored.

Process and finish callbacks receive their actual loaded receiver and vtable
owner plus four actual arguments `(firstBody, secondBody, configuration,
output)`, with true `RET16`. Finish uses the contact manager at current
`agent+8`. Native `320510` stays in the flag-set floating expression, avoiding
an emulated floating-return transport. The separate helper exposes the actual
80-bit result through a ten-byte integer aggregate and records source status
before transport.

Actual `3205B0` remains a named void service with receiver island and arguments
`(configuration, entry, output, timeBits)`, true `RET16`. Allocation and both
release services retain their genuine current object/vtable owner and exact
arguments. Unimplemented/missing owners throw explicitly. No agent count,
contact distance, allocator pointer or admission result is provided in advance.
The timestamp service supplies only the actual external RDTSC input used by
source profiling. Unmasked floating traps remain explicit unsupported owners.

## Preserved semantics

The module preserves initial allocator header writes before allocation, signed
fit and unsigned wrap arithmetic, 48-byte contact stride, borrowed capacity bit,
independent initial/grown buffer lifetimes, current global allocator rereads,
and final destructor rewind including zero bytes. The loop uses the original
signed `(count-1)` admission, retaining its `0x80000000` read-fault edge.

Flag set preserves raw rank publication before FILD, source multiply/add order,
controller mode stores, source pointer rereads, strict ordered comparisons,
NaN behavior, rank update order, minimum selection rereads, and unconditional
finish for non-null agents. Flag clear finishes only after the current mutable
output count becomes nonzero. The callback packet and mutable entry/agent words
are reread at the actual source boundaries.

Floating stores preserve both rejected-write and post-commit observer semantics:
a rejected store restores the source pre-store floating state, while a later
observer exception retains the committed store and true pop/status.

## Physical component proof

`build/test-results/native-island-agent-phase-final-tu15/report.json` is GREEN
for GCC and Clang. It contains 394 unchanged-i386 original comparisons:

- 112 complete closed original calls.
- 75 genuine allocator/agent owner-entry prefixes, including four captured
  Gemini imported-MCC before-input cases.
- 97 read-fault, 73 store-fault, and 37 post-store observer prefixes.
- 4,297 ordered reads, 2,868 ordered stores, 28,682 original instructions.
- All four rounding modes and three precision modes, signed count edges,
  aliased allocator words, finite/infinite/subnormal/signed-zero/quiet-NaN and
  signaling-NaN inputs, and native host FP preservation.

Private mutations of the borrowed flag, minimum stride and FMUL/FADD order are
rejected by unchanged original execution. The four MCC input cases preserve
actual island/agent/controller/allocator slices; they do not supply actual
callback outputs. Their authenticated slice provenance is retained in
`captured-MCC-input-provenance.json`. The historical whole-world raw mismatch
on the selected capture is unchanged and is not waived by this component proof.

The final owner `halo2-test-bsdn4ay4` was removed. It freed 52,006,912 allocated
bytes. Its predecessor `halo2-test-vmluxygp` was also removed, another 52,006,912
allocated bytes. `handoff-cleanup.json` records current source pins and verified
absence. No new raw banks, ELF files, object files, binaries or frames remain.
Original retained reference inputs remain intact.

## Composition and acceptance boundary

The component proof stops at genuine nonconverted owner entries. It does not
by itself qualify the post-agent-return branches, complete live integration,
all physics interactions, multiplayer presentation, or original byte parity.
A separate hardware composition helper executes each genuine original owner
against the current mutable native banks and compares complete native phases
with independently executed unchanged original phases. Its report must pass
before that additional qualification is claimed. Production services must bind
actual owners; recorded callback outcomes or empty success services are invalid.

## Complete phase composition and exact residual inputs

`native-island-agent-hardware-context-final-tu15` passes 20 complete cases on
both compilers, comparing every one of 91,356,057 captured bytes per case,
1,228 ordered kernel reads, 424 ordered stores, complete FP status/control,
40 genuine process returns, 24 genuine finish returns, and 16 native minimum
calls (303,384 original instructions). Each residual executes unchanged i386
code against the current mutable native bank. This first proof also supplies
independently observed actual caller GP/flags and 16,640 bytes of private entry
stack as explicit **inputs**. It supplies no owner result or after image.
Before each call, current native header/output packets are independently
asserted equal, then overwrite the retained input context. The rank-decrement
mutation is rejected. This is not proof that production constructs that caller
context. Its owner was removed, freeing 197,058,560 allocated bytes.

`native-island-agent-semantic-ebp-tu15` removes that input dependency for the
flag-set cases: 12 complete cases pass on GCC and Clang with **no captured GP,
flags or private stack supplied**. Only EBP is constructed from the actual
semantic configuration identity. ECX/four arguments/FP remain real typed
inputs; callback SP derives from declared phase entry SP minus `0x50`, and all
other GP/flags start neutral. Actual sequential owner execution creates its own
private bytes naturally. All banks, ordered reads/stores and FP still match.
This is a qualified transitional residual ABI construction, not final native
owner conversion. Flag clear is explicitly excluded from this new proof.

Original `321A70` loads EBP from its true configuration argument at `321B11`
and `321CA0`, after one packet push. Original `321D20` instead keeps the frame
identity `entrySP-4` in EBP (`321D20`/`321D21`) and reads configuration through
`[EBP+8]`. Configuration-as-EBP must never be applied universally.

The neutral-context diagnostic remains raw RED. In four complete flag-set
cases, the sole differing bytes are `1008257E..1008257F`: original `06 10`,
neutral `00 00`. The unchanged original lifetime explains them:

1. The preceding real `3230C0` call saves configuration EBP at `323117 PUSH EBP`.
2. Its high bytes remain at private addresses `0200EF8E..0200EF8F`.
3. The next real `304350` leaves them untouched at its entry SP minus 34/33.
4. `2E2E43 REP MOVSD` copies a 512-byte initial four-record packet span from
   `0200ED90` to grown storage `10082380`; these bytes become record 3 offsets
   `7E..7F` at `1008257E..1008257F`.
5. Actual selected-record access observation shows only that raw copy touches
   the pair in these complete phase paths. The adjacent validity byte `7D` is
   zero. Source gates `304858`/`304862` and `3049A0`/`3049A5` skip the contact
   consumers `304130` and `30CD00` when that byte is zero.

The audit decodes every observed owner instruction (3,747 unique owner PCs)
and the phase/helper bodies. It records initial GP reads and private byte
reads before writes, including prior writer provenance. Actual incoming GP
reads are ECX, ESP, and EBX/EBP/ESI/EDI initially saved by PUSH; no incoming
EAX/EDX dependency appeared on these paths. Other private bytes are read too;
this does not establish a universal absence of semantic dependencies.

A future native `304350` producer needs actual caller-owned temporary packet
storage with the verified lifetime. The source fieldwise copy/swap leaves
`28..2F` and `7E..7F` attached to their real destination storage slots, while
array growth copies the complete raw record span. Initialize only fields the
source initializes and preserve untouched incoming bytes. Zero filling or
seeding captured output bytes would not implement this contract.

The neutral raw mismatch is retained as evidence and is not waived. Complete
world lifetime and every future validity consumer remain separate gates.
`3205B0` returns and source exception unwind also remain outside these component
reports; live-world acceptance must retain those requirements explicitly.
