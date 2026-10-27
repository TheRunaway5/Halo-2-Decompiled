# Native TU1.5 cached motion advance

`native_tu15_advance_cached_motion` is the complete native port of actual
`325810` and its quaternion/rotation/origin helper chain. It rewinds the cached
fraction, then advances to the actual new fraction using native x87/SSE math.
It contains no guest GP/register file, instruction decoder, source instruction
execution, supplied pose, supplied motion result or supplied RSQRT estimate.

## Source and typed API

The authority is the English TU1.5 image SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The source family is `325810` (160 sites), `323430` (58), `1E1010` (31),
`37C5D0` (69), `2E5810` (62), and the separately recognized `08FDB0` RET8
no-op (one site). All 381 unique sites are authenticated and exercised.

| Stable source | SHA-256 |
| --- | --- |
| `src/physics/native_tu15_cached_motion.cpp` | `595bec38e700cf4de0045c38366f04d0df7fc28b9a02837bc221d6e926b3b5fc` |
| `include/halo2/native_tu15_cached_motion.hpp` | `22f798c99bfebee45a3472b4f72d9afdca6209177f90b369dfc84e175b8880ad` |

The API consumes a typed motion identity, two immutable source binary32 value
arguments, current FP control/status and actual incoming x87 depth. It has no
physics service callback. Its optional RSQRT observer records actual native
inputs/estimates and never supplies them. The existing qualified
`native_tu15_apply_prescribed_motion_forces` implements the same authenticated
`08FDB0` no-op; its existing source is unchanged.

The original retains the extended old/new fraction-times-step product for
angular travel while separately spilling a binary32 linear/quaternion scale.
The port preserves that distinction. Composing two existing pose calls with
only a rounded step would lose the source extended product. Source angular
vector rereads, both quaternion updates, matrix scalar publications, origin
updates and temporary/final cached fraction writes remain ordered.

## Physical differential result

`build/test-results/native-cached-motion-expanded-tu15/report.json` is GREEN
with GCC 16.2.1 and Clang 23.1.4. The original i386 program independently
executes all genuine closed helpers. The native program performs its complete
construction from the same declared inputs.

| Check | Result |
| --- | ---: |
| Original calls and bounded prefixes | 697 |
| Complete original calls | 398 |
| Source sites exercised | 381 of 381 |
| Read rejection prefixes | 146 |
| Store rejection prefixes | 76 |
| Completed-store observer rejection prefixes | 76 |
| Real original alignment fault | 1 |
| Exact native RSQRT observations | 1,128 |
| Ordered original reads | 40,221 |
| Ordered original stores | 19,860 |
| Unchanged original instructions | 325,049 |

Every retained byte, source ordered read, source store PC and before/written
bytes, RSQRT input/estimate, modeled FP control/status and MXCSR match. Public
x87 scalar stores retain their live extended value until the actual accepted
store: rejected stores restore pre-FSTP state, and completed observer failures
retain the accepted pop/status. The full 108-byte host FP state and host MXCSR
are guarded in both observation passes.

There are 192 independent combinations of three x87 precisions, four x87
rounding directions, four SIMD rounding directions and four DAZ/FTZ settings.
Other inputs cover signed zero, denormals, infinities, quiet/signaling paired
NaNs, fraction/step edges, variable cached fractions and preserved incoming
extended values. Five private mutations are rejected by both compilers:
omitted rewind sign, wrong quaternion half, omitted angular reread, precise
reciprocal-root substitution and prematurely rounded angular time product.

## Actual MCC BEFORE data

The proof also uses the independently retained v1461 Warthog BEFORE archive
SHA-256 `3f87636519456e84f8d8bf6cc3754bda002f7dfb42b361bd65fd803c7c4b7185`,
documented in the contact-advance proof. It reads actual dynamic/fixed motion
records and configuration step, authenticates their real +74 targets as
`325810`/`08FDB0`, and checks the actually read constants against the original
image. Four declared dynamic fractions and the genuine fixed no-op run at
their original retained addresses. No captured-after bytes are expected
outputs. The report retains archive/metadata/read-bank hashes and actual
identities; duplicate raw banks remain temporary.

## ABI and remaining gates

`native-cached-motion-abi-tu15/report.json` proves all helper calls have private
x87 depth zero, every return balances private values, and no incoming value is
read. The closed family reaches maximum private depth three, so it admits at
most **five** preserved incoming values. A containing `3205B0` kernel's isolated
preserve-six capacity must not widen this dependency's capacity. The no-op does
no FP work and is separately checked with incoming depths zero through eight.

`native-cached-motion-simd-abi-tu15/report.json` conservatively tracks source
XMM lanes through the entire closed call sequence. No incoming lane is read
before definition, and no encountered SSE opcode is unqualified. This supports
the typed motion inputs. Volatile register outputs are not part of this API;
remaining original callers/agents require their own liveness/ABI qualification.

The API declares ordinary immutable value arguments and private native cached
locals; guest control-frame aliases are outside this proof. Private transient
raw x87 payload on exceptional return, unmasked platform traps/unwind and
original Xbox/PentiumIII RSQRT bit identity beyond this physical host remain
open. Full native `3205B0` + native motion + genuine remaining-agent composition
and live world activation have separate acceptance gates. This report does not
establish complete multiplayer or physics parity.

The initial `native-cached-motion-first-tu15` RED correctly caught a missing
source angular reread; its exact compiled source is retained. The corrected
57-case second report is GREEN and is superseded in scope by the 697-case
report. Existing pose/commit/kinematic sources were not changed.

## Storage lifecycle

All reference/compiler execution uses `tools/run_test.py` and disposable
`tools/run_tests.py` builds with one build job. The final owner
`.local/test-scratch/halo2-test-u7pah4jd` was removed, including 136,712,192
allocated temporary bytes measured after inner build cleanup. Both initial
owners were also removed, each accounting for 12,754,944 allocated bytes.
Cleanup receipts retain the exact paths and measurements. Only small reports,
commands, source hashes and necessary diagnostics remain; the coordinator's
required BEFORE archive stays retained with its original receipt.
