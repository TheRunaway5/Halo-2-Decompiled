# TU1.5 native camera and look work, 2026-11-02

The selected Host constructor routes are native and live tested. The whole look update is still a private integration candidate; the collision-query service remains an explicit native requirement.

## Completed source kernels

- `18A0C0`: all 92 original instructions, 317 original/native profiles per compiler, including 140 read/write/observer prefixes. All three Host initialization sites use the same FP-field binding. Multiplayer 1475/01 driving and grenade flows completed strict original comparisons of 114,363,573 and 114,887,921 bytes plus FP, with actual vehicle cameras. Each reported five native constructor calls and zero refusals. Internal generated constructor callers remain open.
- `B1720/AA200/AA270`: 335 profiles per compiler, all 96 instructions, 23 read prefixes, three rejected mutations.
- `E05D0`: 214 profiles per compiler, all 88 instructions, 93 fault/observer prefixes, aliasing and three rejected mutations.
- `E3310/11FA50`: 231 profiles per compiler, all 80 instructions, 41 fault/observer prefixes, three rejected mutations.
- `E18D0`: all 142 instructions. The composed native vector, basis, curve and finite-acos path passes 209 profiles per compiler, including 151 read/write/observer prefixes, 9,812 ordered reads and 1,188 stores. Three mutations are rejected. This proof uses the private source-phase-correct curve replacement described below.

The original i386 code executes only as an offline oracle. The native kernels use ordinary DATA interfaces and real native arithmetic. They do not call the original instruction executor or consume computed child outcomes.

## A real curve read-fault correction

The original `17F8A0` loads its scaled value onto x87 at `17F8D9`, then reads the modulus constant at `17F8E3`. The old native helper read the constant first. Normal returns matched, but a fault on the constant read exposed a different x87 TOP/status.

The failing report is preserved at `analysis/physics/native-camera-adjustment-faults-tu15-20261102-second/report.json`. The private replacement loads the actual native value before the read and passes that live value through the sole remainder helper. It represents no original GP/control frame. Both the 640-case complete curve regression and the 209-case camera composition pass on GCC and Clang. The single-file candidate is sealed in `analysis/physics/native-function-curves-read-fault-stage-20261102`; it has an independent peer receipt and has not overwritten the frozen runtime source.

## Camera parent and workspace scope

The new `CD4A0` semantic parent has 287 original instructions. Current qualification covers all 190 instructions reachable before a geometric query, including on-foot arithmetic, parented camera origins, special markers, seat markers and genuine native marker fallback computation. Its 183-profile proof includes 118 parent fault/observer prefixes, 4,141 ordered parent reads, 1,385 parent stores, exact declared 164-byte parent DATA and three rejected mutations. The original/native child algorithms compute their own outputs; their operand/fault proofs are separate. The 97 instructions following `16C620` remain unqualified and cannot execute through the empty native query service.

Adding the parented cases exposed and corrected a real source argument error: `CD4A0` passes its cached parent datum to `BBF20`, rather than the original child datum. The failing third parent report remains preserved; the fourth report passes both compilers.

`analysis/physics/native-camera-workspace-lifetime-audit-20261102/audit.json` records the exact source allocation and alias coordinates. Every consumed `E18D0` workspace field is initialized by its native/source writer before use. Its two unused words can contain prior private lifetime data, including a prior saved ESI; they are never serialized or manufactured by the native pipeline. The source caller neither reads nor retains the child's workspace after return. No raw private-control before-byte ledger is claimed green or waived.

## Actual input composition and remaining work

The authentic 1474/02 typed look BEFORE is protected as archive SHA-256 `9ed85894c66577b59bc9722db92efec51558bf8c41375ed4b17e2d52352a91e4`. On that actual on-foot input plus three separately declared delta controls, the native `18A300 → 189FD0 → CD4A0 → E18D0` composition matches all 89,795,481 captured committed bytes, complete camera position, defined context fields and FP on both compilers. It executes no original CPU child on the native side. This does not claim a captured original GP ABI or Source private-stack parity.

The same-production native-only pipeline helper is under direct actual-input qualification. Production wiring, caller/scratch/observer models and actual multiplayer use are not yet claimed. A separate readonly capture filter seeks an ordinary real seated-player input without changing pose, seat, camera, controls or physics state. `16C620` collision queries, exceptional acos/CRT policy paths, and remaining internal constructor callers stay explicit checklist items.

Disposable binaries and banks are owned by `run_test.py`; only small receipts/source pins and required original reference archives are retained. The camera cleanup receipt verifies 18 more removed owners and 5,307,166,720 allocated bytes freed; later probes are added once their owners finish.
