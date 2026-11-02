# Native TU1.5 object publication

The actual native frame now selects semantic object publication directly.
`BDA50`, `BCFB0` and `BE7E0` are implemented in
`native_tu15_object_publication.cpp`, composing the existing native `B9390`
ancestor marker. The four original owners contain102 authenticated sites.
The interface receives object data, storage and optional observation; it has
no CPU state, instruction decoder, stored outcome or original fallback.

The authority is English OG Xbox TU1.5, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The [physical component report](../build/test-results/native-object-publication-tu15-20261101/report.json)
passes372 cases under GCC and Clang:255 full calls and117 failure prefixes,
9,524 ordered reads and402 stores. It checks root-parent chains, recursive
children/siblings, dirty and attachment flags, type-shift masking, empty root
selection, complete retained bytes and unchanged FP state. Wrong root selection,
update flag and tree-type mask mutations fail against unchanged instructions.

The implementation retains the outer header-bank identity across root selection
and the child's header identity across recursive publication. Source data reads
and rereads occur in their original order. Three atomic RMW prewrite-observer
prefixes remain unqualified: the source stop precedes the entire instruction,
whereas the storage API splits its operand read and write. Observer-mutated
bank/alias domains and platform page-fault delivery also remain separate gaps.
These limits are recorded in the report and are not waived bytes.

The [frame binding report](../build/test-results/native-frame-object-publication-binding-20261101/report.json)
passes65 original frame prefix/tail cases,248 ordered stores and both frame
mutation controls. Its model executes two actual native publications, two tree
visits, four ancestor-wake calls and genuine header stores. `BDA50` is removed
from the frame's residual dependency sequence. This combines component and
binding evidence; a complete live child-return comparison remains required.

Per-world reports expose publication attempts, completed calls, refusals, root
queries, tree calls, ancestor wakes and marked headers. The optional
`HALO2_REQUIRE_NATIVE_OBJECT_PUBLICATION` gate requires positive completed
publication/tree/wake activity, reconciled attempts and zero refusals. Visible
multiplayer and strict original-frame qualification remain pending.

The component owner is verified absent after79,478,784 allocated bytes were
removed. CMake and frame test trees use the owned runners and are removed on
completion. Small reports, commands and source hashes are retained; this
component retains no new binary, frame or raw bank.
