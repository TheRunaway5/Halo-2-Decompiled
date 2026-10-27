# Native TU1.5 contact record owners

The typed native module now implements actual English TU1.5 `303BA0`,
`303800` and `304130`. The [final qualification](../analysis/physics/native-contact-records-tu15-20261026-qualified/report.json)
passes796 physical original comparisons with GCC16 and Clang23. This is a
component proof; live bindings and whole-frame raw equality remain separate.

| Original owner | Sites | Native behavior |
| --- | ---: | --- |
| `303BA0` | 29 | Ordered fieldwise copy, returning the supplied destination |
| `303800` | 60 | Exact SSE/x87 predicate and byte result |
| `304130` | 142 | Flag selection, actual predicate and fieldwise swap compaction |

Authority is the owned unchanged English TU1.5 XBE,
SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The kernels use typed data references and opaque verified offsets. They have
no guest register file, instruction decoder or dispatcher. Remaining field
meanings are deliberately unnamed. The source copy and swap omit28..2F and
7E..7F, leaving those bytes in their actual storage slots. No original
uninitialized byte is zeroed or copied through an invented whole-record type.

The copy transfers vectors00/10, words20/24, vectors30/40/50/60, words70/74/78
and bytes7C/7D, in that exact read/store order. Reads remain lazy, including
overlapping source and destination records. It performs no FP arithmetic.

The predicate consumes four stack arguments in the original: output byte,
first packet, second packet and settings. Incoming ECX is overwritten before
use; RET16 returns the actual output pointer. The typed API computes the byte
directly, and the retained-layout adapter can publish that byte. It does not
invent original volatile GP/XMM postimages. The original first compares the
second packet's scalar0C against authored constant `BDCCCCCD` at41D018. Its
packed products and differences keep operand order and all four arithmetic
lanes. Both dot products use Z+(Y+X), followed by actual x87 FLD/FCOMP against
settings10 or0C. Ordered, equal and unordered cases preserve the original
FNSTSW/test/parity decisions. Real native SSE and x87 instructions own their
rounding and exception status; C++ comparisons do not substitute for them.

Compaction tests byte7C for exactly1, rereads the array data pointer each
iteration, and calls this actual predicate. A zero result swaps the selected
record with the last record and retries the same index. Index/count comparisons
are signed while offsets and decrements retain32-bit wrap. The two source
scalar20/24 values remain in actual x87 temporaries through the first half of
the swap, then publish through genuine FSTP m32 operations in source order.
If the actual destination rejects a scalar write, the previous real native
x87 state is restored. Once memory accepts it, the commit acknowledgement
precedes observer delivery, so an observer failure retains the genuine pop
and arithmetic status. The scalar fields are not replaced by integer copies.

The physical reference executes unchanged original `.text` in one private ELF.
Only declared input banks, arguments and FP state are installed before entry;
the actual original303800 runs inside original304130. No callback supplies a
predicate, allocator result or expected output. The matrix records65,194
unchanged instructions,12,741 ordered reads and5,449 stores. It includes all
four rounding modes crossed with all four DAZ/FTZ settings, paired NaNs,
subnormals, signed zeros, scalar conversions, actual MOVAPS alignment faults,
overlapping copies, self-swaps, aliased probes/settings/header fields and signed
index boundaries. There are118 read-failure prefixes,80 rejected writes and80
committed-store observer failures. The complete retained banks, semantic
results, ordered accesses, store preimages and CW/SW/MXCSR match. Caller live
x87 values are checked in the original; the native call preserves the full
installed108-byte host x87 witness and host SIMD controls/status.

Of those796 cases,130 use genuine original-created128-byte records retained at
the actual303BA0 copy entries in Gemini column0, Gemini column1 and Elongation.
Those prior hardware observations matched ordinary original replay across the
entire1,077,248-byte heap. The final validator pins each small source report and
records event indexes, original source addresses and payload hashes. It only
relocates these bytes as new declared inputs. Settings/controls remain explicit
variations; this does not claim to reproduce an entire original caller context
or later frame. No raw bank or supplied original result was retained or used as
a generated native outcome.

Four private negative controls are rejected: changed dot association, skipped
first limit, changed scalar publication and restoring pre-store x87 state after
a committed observer failure. The last fails at a genuine scalar-store observer
boundary, with a different TOP/status despite an already committed bank store.
During reference-tool qualification, Capstone's READ|WRITE annotation for
FSTP m32 was narrowed to its actual write-only destination at3042B4/3042C1.
This corrects operand metadata only; no original instruction or raw-bank gate
was changed. A declared unaligned record case was also corrected to reach its
real flag==1 vector path; the observer now requires every declared alignment
case to fault in actual original execution.

The first caller-safe consumer is a wholly typed native304350 contact producer,
which already invokes the geometry and compaction owners in the original.
Directly binding these leaves into legacy machine-state entries would require
separate caller volatile-register and fault-resume proofs. The current public
FP packet carries control/status, not restartable private source temporaries;
fault-prefix equality does not certify resuming an interrupted source stack.
Unmasked traps, overfull source x87 stacks, undiscovered record semantics,
Xbox CPU bit identity, live multiplayer and paired native/future-frame histories
remain explicit open gates. The protected Gemini129-byte and Elongation27-byte
whole-heap failures remain strict RED, without exclusions or tolerance changes.

The [cleanup receipt](../analysis/physics/native-contact-records-tu15-20261026-qualified/cleanup.json)
verifies all five contact qualification owners are absent and reports370,372,608
allocated bytes removed. This excludes products already removed by nested
CMake runners, avoiding duplicate accounting. No large artifacts were retained.
All12 earlier vehicle-rate source pins remain unchanged. Final module, test,
observer, validator and dependency hashes appear in the qualification report
and cleanup receipt. No CMake, world binding, generated owner or shared type was
edited by this component work.
