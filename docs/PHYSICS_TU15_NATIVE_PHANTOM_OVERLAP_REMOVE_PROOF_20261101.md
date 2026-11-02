# Native TU1.5 phantom overlap removal

`315A70` (54 sites) and `314FE0` (26 sites) are now semantic native kernels.
The [declared-model qualification](../analysis/physics/native-phantom-overlap-remove-tu15-20261101-qualified/report.json)
passes 39 cases with GCC and Clang, exercises all 80 sites plus the real
one-site `2E4D60` callback, and rejects three semantic mutants. The
[retained actual-world qualification](../analysis/physics/native-phantom-overlap-remove-actual-earlier-tu15-20261101-qualified/report.json)
also passes both compilers on the actual earlier02 container and the
separately measured live05 receiver/other identities. This is not a captured
live05 entry or successful visible live activation.

The live05 refusal identified owner `315A70`, receiver `1007BBC0`, other
collidable `1008392C`. The required02 BEFORE archive already contains that
receiver with overlap count four, the same other at index three, and zero
listeners. Unchanged original `315A70` calls `314FE0`, then publishes count
three and writes the old last pointer to the matching slot. Native code
matches every one of the 89,795,481 captured bytes, an additional nine-byte
initialized DATA packet, all 13 ordered memory events, all five exact store
before/written values, FP, and the original RET4 contract. No old count,
capacity, pointer or source allocation metadata is changed to force the path.

Only ordinary typed receiver/other and actual FP are marshaled into an isolated
original ABI. Its new private control stack is distinct from every retained
runtime bank. The native kernel consumes no GP/SIMD file or return metadata.
The nine event bytes are caller-owned DATA, initialized from current phantom,
other and was-present inputs before a listener can observe them. No saved
register, old frame pointer or expected AFTER value is serialized.

## Source behavior and listener boundary

A null shape word in the other collidable returns immediately. Otherwise the
source searches backward through `phantom+C0/C4`. A match notifies first,
then rereads the current count and backing pointer, decrements the count and
swaps the last element into the original matching index. It leaves the unused
tail unchanged. A miss still notifies listeners with was-present false.

Both notify paths publish other, phantom and the one-byte flag in their
source order, including when there are no listeners. Reverse listener dispatch
keeps the initial index and rereads the current listener array each iteration.
Null entries are skipped; nonnull entries call current vtable slot `+4` with
the initialized event address and ordinary RET4. Unknown owners remain explicit
counted refusals. Only authenticated native owners may be admitted; the
qualified `2E4D60` RET4 no-op is distinct from any effectful callback.

The model covers first/middle/last/missing entries, null shapes, empty and
reverse listener lists with null holes, direct helper flags zero/one/FF,
13 read/write fault prefixes, and four independent callback-return DATA
controls for current overlap count/backing and listener count/backing. These
controls do not claim that the source no-op mutates those fields. Both compilers
match 465 ordered reads, 130 exact stores, 44 genuine callback entries, complete
DATA banks and FP. Wrong replacement index, wrong was-present flag and forward
listener iteration are rejected. The
[all-local-path audit](../analysis/physics/native-phantom-overlap-remove-input-context-tu15-20261101-qualified/report.json)
finds no incoming GP dependency beyond typed ECX/formal DATA and no XMM use;
listener calls remain explicit typed boundaries.

| Frozen source | SHA-256 |
| --- | --- |
| `include/halo2/native_tu15_phantom_overlap_remove.hpp` | `128e64b0c98c56a2d2bbda0fe608b20119f3b89411067c474defd3a1781887ba` |
| `src/physics/native_tu15_phantom_overlap_remove.cpp` | `40747324b1283c62237bb019fef31548a304ceca1bc6b13a5beafe21f0ee4ce7` |

The model owner removed 37,957,632 allocated bytes; the actual-world owner
removed 308,441,088 bytes. Both owners are verified absent. No new large
artifact is retained. The required02 reference archive remains protected at
its documented 12,003,419-byte size and SHA-256
`05ce4baa6c8d8a71326a629ce93a25eca8259a5b52edd9f51fc62fd5b3cd90dd`.
The new add-overlap/create prototypes are separate work and do not change
these frozen removal sources.
