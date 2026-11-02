# Native TU1.5 camera projection

`src/rendering/world/native_tu15_camera_projection.cpp` implements originals
`2FB50` (138 sites), `2FD70` (315 sites) and its affine-inverse dependency
`144090` (82 sites). These are semantic native algorithms over named camera,
basis, projection-program and matrix values. No source instruction dispatcher,
virtual CPU, guest private stack, substituted projection formula or original
runtime execution is involved.

The program helper computes signed rectangle dimensions, optional clipping
bounds, half spans, centers and frustum coefficients. It retains the actual
mutable layout, widescreen and auxiliary context reads and modifier branches.
Its typed value API keeps the parent's private program in native storage;
the retained-layout API preserves alias-sensitive reads and publications.

The parent constructs right/up/backward axes from the authored camera vectors,
normalizes all three with the original extended square/sum/square-root,
float spill and threshold decisions, publishes the frame, calls the typed
affine inverse, and constructs the clipping/depth matrix. The first norm is
staged across its original intervening camera reads. The oblique-plane and
depth-flip branches retain their ordered comparisons, fresh literals and
extended arithmetic before final float publication. The affine inverse keeps
ordered zero/unit-scale branches, a forward thirteen-DWORD zero clear and
the source/output rereads required by overlapping frames.

SSE and x87 operations execute on real hardware with the source control word,
status and MXCSR. Native hardware stages preserve the original rounding and
source-fault cuts; they are not an arithmetic interpreter. An explicit logical
80-bit context with occupancy preserves actual older hardware values,
including overflow/underflow, sparse masks, special values and FPTAN's
out-of-range behavior. The complete host x87 image and MXCSR are restored.
Unmasked exceptions and pending exception-summary states are outside the
admitted profile. Ordinary callers have DF=0. Empty diagnostic observers add
no reads; a requested store preimage observer performs its own explicit read.

The binding admits only authenticated calls `2FD79→2FB50` and
`13C9B/2BAB7/2BAFC→2FD70`. `owns_call(entry, site)` is the production routing
predicate. Shared helper `144090` is a typed internal dependency and is absent
from public entry ownership; its original `2FFE9` ABI is exercised only by
the isolated transport test. Each function is void. Nonvolatile GP state,
opaque dead fields and FP outputs are retained, with only the real RET word
read and SP advance owed by the transport. A failed RET read retains completed
data/FP state and does not advance SP. A sparse x87 output that the existing
dense ABI cannot represent raises an explicit required-owner exception
containing the actual context and any original failure; it never invents a
successful return.

Independent primary review found a real defect in the first draft: the
`144199` multiply had moved before the `144194` publication, though its operand
read correctly preceded that store. With zero axes except MAX_FLOAT and a
negative two-unit translation, this changed MXCSR at a rejected store from
`1F80` to `1FA8`. The corrected native order is operand read, publication,
multiply. The v2 physical fixture adds the complete witness and all its read,
before-write and after-write cuts, including clean-MXCSR cuts on both sides
of `144194`. An actual-source mutation restoring the early multiply is a
required negative control. The initial passing finite compiler matrix remains
historical evidence and does not qualify the corrected source.

Reference and validation receipts live under
`analysis/campaign-native-new-game-camera-20261028/projection-*`. The v2
fixture contains 1,878 physical calls/prefixes, all 535 sites, 485,016 unchanged
instructions, 94,580 retained reads, 50,587 stores, 1,066 diagnostic prefixes
and five actual unmapped accesses. Its required compressed original input is
`tests/fixtures/native_tu15_camera_projection_v2.json.gz` (1,384,468 bytes).
The prior 1,828 cases retain their data, FP, access and execution results;
only prefix IDs and an unused ID-seeded incoming XMM0 diagnostic can differ.
The v2 independent physical repeat is byte-identical. GCC1/1, Clang
ASan/UBSan1/1 and Clang10/10 including nine actual-source negative controls
pass against current source pins. The early-multiply mutant fails exactly at
`fault_seed_inverse_fp_store_before_write_11`, with native `1FA8` versus
original `1F80`. Tests also exercise ordered accesses/publications, full live
80-bit values, aliases, late input changes, all public caller transports,
private program values, rejected admission, occupied host x87 restoration
and failure to read the real RET word. The superseded owned fixture and all
three disposable oracle/compiler trees were removed; bounded receipts remain.

Component qualification, producer/generator retirement, shared World routing,
a coherent MCC/GPU run and complete camera-parent/New Game closure are distinct
gates. This additive module does not itself remove the legacy producer bodies
or claim a native rendered frame.
