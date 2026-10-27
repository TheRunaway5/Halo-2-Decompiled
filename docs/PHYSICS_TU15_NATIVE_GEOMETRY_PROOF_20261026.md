# Native TU1.5 geometry leaves

The three geometry kernels now reproduce the tested original memory and FP
behavior with native SIMD arithmetic. They have no Xbox register file,
instruction decoder, dispatch loop, binary execution dependency, RSQRT call,
or supplied physics outcome. Live integration remains a separate gate.

| Original English TU1.5 entry | Sites | Native semantic owner |
| --- | ---: | --- |
| `2E44D0` | 40 | Compose all four affine columns, then add translation |
| `2E47B0` | 35 | Transpose direction transform; lane W repeats result Z |
| `2E4690` | 26 | Position transform, preserving X+Y+(Z+translation) |

Authority is the unchanged owned English TU1.5 image,
SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The complete CFG instruction listings are authenticated from that image.
These functions contain no x87 operation. Their native environment owns only
MXCSR, preserves operand order for paired NaNs, and restores the host SIMD
state. Lazy semantic access preserves actual source aliases and fault order.
The retained-layout adapter checks MOVAPS alignment and commits each actual
memory write before delivering its optional observer.

The final [qualification report](../analysis/physics/native-geometry-tu15-20261026-qualified/report.json)
records 758 physical original cases, 38,585 unchanged original instructions,
6,623 ordered reads and 1,673 stores. GCC16 and Clang23 both pass the complete
banks, arithmetic status, ordered accesses and store preimages. Cases include
all four rounding modes and four DAZ/FTZ combinations, 288 paired NaN cases,
80 alias cases, nine real alignment faults, 54 read-failure prefixes, 14 rejected
writes and 14 committed-store observer failures. Pure value-storage controls
also use the semantic API without retained memory addresses. Three private
arithmetic mutations are rejected: omitted Z, wrong transpose swizzle and
reassociated translation.

The original ELF contains unchanged original `.text`; ptrace installs only
declared input banks, argument registers and FP inputs before entering it.
It single-steps the actual source instructions and captures their actual
stores. It never patches code or supplies geometry results. This physical
host comparison does not establish Xbox CPU bit identity or unmasked SIMD
trap handling.

The saved host x87 witness originally failed intermittently. An independent
[control](../analysis/physics/native-geometry-tu15-20261026-host-metadata-control/report.json)
reproduced the identical change with only FLD1/FLDZ/FLDPI, FNSAVE/FRSTOR and
`sched_yield`: this host clears FIP/FCS/FOP/FDP metadata bytes12..23 while
preserving CW/SW/TOP, tags and every saved register byte. The test now installs
zero pointer metadata before the witness and still requires complete108-byte
equality after each native call, including fault cases. No comparison bytes
were removed from that gate.

The [caller audit](../analysis/physics/native-geometry-caller-seams-tu15-20261026-qualified/report.json)
authenticates 48 direct call sites: 17 composition, eight transpose and 23
position calls. Two registry omissions are authenticated from actual original
vtable pointers. All leaves accept the destination in ECX and two stack
pointers, then RET8. The pure native API returns their semantic memory/FP
outputs. It does not fabricate the legacy volatile EAX, EDX, XMM or flag
postimages. A blanket replacement at those legacy entries is therefore not
qualified.

Two narrow caller seams are concrete. Actual `2E8870` is a 25-site shape-bounds
wrapper: it composes a caller-owned64-byte local transform at `2E8888` and
passes it to the actual child vtable slot18 call at `2E889F`. Converting that
whole owner requires authenticating the child call's input ABI and retaining
the local transform across it. Actual `304350` is a 534-site contact producer
that calls the transpose leaf at `3045C1` and `304904`; its fully native owner
can consume the semantic geometry API directly. Neither seam was activated by
this component proof.

The [cleanup receipt](../analysis/physics/native-geometry-tu15-20261026-qualified/cleanup.json)
verifies all six measured outer owners and the independent control owner are
absent. Those six owners removed245,649,408 allocated bytes. This conservative
measurement excludes products already removed by nested CMake runners and the
unmeasured independent control. No large binary, raw bank or frame was retained.
Source, bounded logs, original report inputs and all final source/dependency
hashes remain. Existing required raw-bank evidence and its strict RED gates
were unchanged.
