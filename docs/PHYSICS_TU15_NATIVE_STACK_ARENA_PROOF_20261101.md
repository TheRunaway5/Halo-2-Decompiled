# Native TU1.5 slow stack arena

`native_tu15_stack_arena` implements the complete authenticated `2E10A0`
allocation owner (40 sites) and `2E1110` release owner (26 sites) with semantic
native C++. The [physical component report](../analysis/physics/native-stack-arena-tu15-20261101-qualified/report.json)
passes 33 cases with GCC and Clang, exercises all 66 owner sites, and rejects
three semantic mutations. This is component qualification; live activation
and persistent pair-array growth require their separate acceptance evidence.

The original chooses the signed maximum of the requested byte count and
`0x1000`, allocates that size plus a 16-byte header through the current global
allocator's `+0x10` method, then saves the arena's four current DATA words.
It publishes remaining bytes, backing identity, boundary and cursor in source
order. Release chooses the backing from the current arena boundary minus
`0x10`, restores each word in source read/store order, and calls the current
global allocator's `+0x14` method with the derived byte count and class `0x12`.
The first formal release argument is unused by the authenticated body and
remains unused in the native API. It is not substituted for the current boundary.

No control frame, saved GP/SIMD contents, historical stack bytes, expected
allocation result or captured AFTER image is supplied. The service API reuses
`NativeTu15ArrayGrowthServices` and its typed allocation/release requests.
Current original allocator `486080`, table `45F4CC`, dispatches to already
qualified native routing owners `149DA0`/`149E00`, which use current heap DATA
and native pool owners `233250`/`233420` in the exercised domain. Unknown heap
owners, lazy default-heap construction, and pool frees above `0x2000` remain
explicit service gates in the composed proof.

The [local input audit](../analysis/physics/native-stack-arena-input-context-tu15-20261101-qualified/report.json)
checks every authenticated local CFG path. Only current ECX receiver and
ordinary formal argument DATA are semantic incoming GP inputs; incoming XMM
lanes are unused. The excluded pushes are authenticated control saves, not
physics DATA. The native backing allocator composition has no remaining CPU
service execution in the exercised router/pool path.

## Physical qualification

The unchanged English Xbox TU1.5 XBE is pinned by SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Only required retained BEFORE allocator/global/pool bytes are extracted from
the protected v1457 reference archive; their exact source members and hashes
are in the report. Each allocation is performed independently by the original
and native kernels from the same before state. Each release case first runs
actual original `2E10A0` to produce valid current arena state. No backing
pointer, free-list result or saved header is manufactured.

The 33 cases cover zero, boundary, signed-negative and large admitted requests;
actual allocate/release pairs; five read/write fault prefixes; three mutations
of current arena DATA at source store boundaries; and three actual `149E00`
null releases. Each compiler matches all retained memory banks, FP words,
allocation result, ordered reads, exact store before/written bytes and typed
allocator request order. There are 483 original ordered reads, 293 stores and
57 genuine allocator entries. The native kernels preserve host FP state.

Mutations of the saved cursor word, minimum allocation size and restored
remaining-byte word are rejected. The original minimum allocation request and
zero-byte release semantics are preserved, including ordinary wrapped
32-bit arithmetic. Arbitrary mutation of source formal byte-value argument
storage during a backing callback is outside this API's declared input domain.

| Frozen source | SHA-256 |
| --- | --- |
| `include/halo2/native_tu15_stack_arena.hpp` | `e9c4cc3bc9059f31090b5dd13e8da9c925a286fa4ffc3ff050b7f8299706e8a5` |
| `src/physics/native_tu15_stack_arena.cpp` | `de705cc899a73b4747f8c5c388a8a32a5b84cb8dbb9887b7633f1b50d9918e86` |

The new helper runs through `tools/run_test.py`, with private CMake builds
through `tools/run_tests.py`. The qualified owner was verified absent after
completion; 49,655,808 allocated bytes were removed. No generated large
artifact is retained. Required original reference archives remain preserved.

## Pair composition

The first [genuine slow-arena parent report](../analysis/physics/native-pair-consumer-slow-arena-genuine-tu15-20261101-first/report.json)
uses a declared source-owned reservation before a genuine `321510` entry:
actual `317A40` reserves the current available arena bytes into a separate
owned buffer. This leaves fewer than 16 bytes available by the real source
constructor, without changing the island's existing pair pointer, count,
capacity or allocation metadata. The pair's actual temporary reservation then
executes `2E10A0`, and its destructor executes `2E1110`. All persistent bytes,
FP and ordered parent events match with genuine remaining services. This is
valid declared preparation, not a captured positive change event or a supplied
allocation result. The temporary reservation's DATA lifetime spans the parent
comparison; the owned child processes and all their storage are retired.

The native child composition and actual live growth/service-input capture
are tracked separately; success of this component does not close those gates
or the broader full-physics goal.

The [native child composition](../analysis/physics/native-pair-consumer-native-arena-tu15-20261101-qualified/aggregate.json)
now passes nine parents plus three mutations per compiler. It includes three
real slow allocation/release pairs per compiler after genuine reservation
preparation, with native filter, arena, backing routing and pool kernels.
Each compiler compares 815,188,065 persistent bytes and 1,754 ordered parent
operand events. The original and native child store streams have separate
component qualification; the parent proof preserves typed child-call boundaries
and full persistent memory/FP. The completed owner removed 866,750,464 bytes.

The first actual positive runtime growth input is also qualified separately:
[actual02 typed BEFORE composition](../analysis/physics/native-pair-consumer-actual-growth-tu15-20261101-qualified/aggregate.json).
This real imported-MCC multiplayer capture has first count two, second count
zero and an existing one-record allocation with capacity one. Unchanged
original `321510` confirms/creates the actual pairs, calls `149E00` with
`[1007A7E0,10,12]` (hex), then allocates `30` bytes through `149DA0` and
publishes count/capacity three. Both compilers match 89,734,085 persistent
bytes, complete FP, 146 ordered parent events, the header and all six typed
callback boundaries with native filter/routing/pool/arena services. The
input old count, capacity, pointer and allocator state are never altered.

The archived input is an actual typed native ENTRY, not a captured original
CPU ABI. The isolated reference declares ordinary ECX plus four formal
arguments, an empty live x87 stack with the captured FP words, zero-defined
independent nonargument GP inputs, and separate private original FS/return
storage. All mounted source code is authenticated original TU1.5; it runs
only in the offline oracle. No GP/SIMD postimage or old control metadata is
provided to the native kernel. This qualification is a physical component
comparison of an actual positive live input, not successful visible live
activation of the updated allocator binding.

Required archive SHA-256 is
`05ce4baa6c8d8a71326a629ce93a25eca8259a5b52edd9f51fc62fd5b3cd90dd`:
`analysis/multiplayer/physics-live-tu15-20261012/mcc-native-flip-controller-original-v1465-20261101/iteration-02-native-pair-owned-release-input-vehicle-camera/required-native-pair-consumer-before/actual-native-before.tar.zst`.
Its 12,003,419 bytes remain explicitly retained as the required reference;
all 4,033 before-bank files are rehashed against the retention manifest.
The native qualification's 308,310,016-byte temporary owner was removed.
