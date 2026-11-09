# Native MOPP AABB traversal (`309940` / canonical `305E40`)

The authored-MOPP collision-tree traversal now takes the native route by default
in both production owners: `ObservedWorldState` in
`src/physics/runtime/physics_world_runtime_generated.cpp` and `State` in
`src/physics/runtime/physics_contact_runtime_generated.cpp`. Both TU1.5 and
MapPack11 images are admitted. World supports selected and canonical observation
PCs; data pointers, real child targets and return addresses keep their actual
image identities. Unknown images and observers without the required
observation-only store witness are refused before native execution.

The typed controller in
`include/halo2/native_tu15_mopp_aabb_traversal_kernel.hpp` is shared by the public
value-frame API and the retained-caller adapter. It decodes authored geometry
DATA and makes geometric decisions without a CPU register file, instruction
interpreter or emulated flags. The thirteen projected directions share
`native_tu15_mopp_aabb_projection_kernel.hpp`. The separate ABI adapter in
`src/physics/runtime/native_tu15_mopp_aabb_traversal.hpp` publishes observable
registers/flags and retains actual stack, property, recursion and child-owner
contracts. Immutable executable dispatch tables become compiled DATA-format
classification; executable instructions are never interpreted by this route.

The adapter uses lazy fourteen-word caller copies, actual property slots and
real recursive, growth and diagnostic handoffs. PUSH commits SP after a
successful provider write; RMW flags and REP progress commit before the
completed-write observer. Parent control never consumes fabricated child
results. Both DF domains and zero, one, four and eight incoming x87 values are
covered by the hardware corpus.

Complete original bodies, selected resolvers and dispatchers are authenticated
as exact raw/specialized pairs. World body SHA-256 before its native prefix is
`e305c6cd745e83016af45747938f2b2101cbcc768920a429fed51857ea2112cb`;
the separate State body is
`ffd0d98959366dea0236a09a56c6c0573c338efbd942241d01756ddba0968da7`.
Both contain 467 source lines / 464 original sites. The State body equals the
World body after only type and observation-PC normalization. A source hash or
resolver/table drift is a hard authentication failure.

## Qualified component gate

The final official GCC sanitizer/optimized cohort passes 6/6 checks: TU1.5 and
MapPack11 ABI comparisons in both configurations, operand specialization and
World observation regeneration. Each image/configuration checks the same
2,365 physical fixtures against unchanged original i386 execution: 781 normal
or real-child-entry cases, 907 stopped reads, 265 pure-store failures and 412
completed-write observer exceptions. It reaches 454/464 original parent sites.

Each receipt records 21,285 native and 15,192 reference comparisons, 180,582
complete direct-native pre-I/O packets, 429,516 reference packets, 26 exact
authentication mutations and eighteen admission refusals across the two owning
types. Direct, production raw-bound and specialized entry copies are compared.
Production copies compare complete final/prefix GP, flags, XMM, FP/live 80-bit
state, whole banks and ordered I/O. The native routes execute no retained
parent continuation or literal callback. The copied reference dispatcher uses
the correct image-selected recursive target in both images.

The closure receipt is
`build/test-results/native-mopp-aabb-all-images-closure-20261109/report.json`;
its four exact evidence reports are in
`build/test-results/native-mopp-aabb-all-images-official-20261109/evidence/`.
Before/after proof-input hashes are identical. All owned builds, dependency
copies, banks and binaries were removed; only bounded reports remain.
Earlier independent Clang sanitizer proofs qualify the same numerical/ABI
controller before the final additional observer-refusal control; they are not
misreported as a rerun of the latest refusal corpus.

The State 82-function / 8,926-instruction generator reproduces installed source
byte for byte, with raw/specialized whole-unit Clang syntax passing. The World
477-function / 35,694-instruction generator also reproduces its source exactly
while preserving all separately authenticated native overlays.

## Remaining closure

Ten diagnostic continuation sites beyond the first real diagnostic entry,
actual allocator/backend returns, descendant exceptions, wider FP/MMX domains
and physical memory fault semantics remain separate gates. The bounded parent
proof does not establish whole-query native ownership, campaign/all-map native
execution, original full-frame parity, performance or launcher delivery.

Frozen r9 includes the TU1.5 parent bindings and public shared controller; its
79 component checks, engine build and eight Coagulation/Zanzibar accounting
runs pass. The later MapPack admission and final observer guard are outside
that freeze. The selected launcher runtime is unchanged. Both image variants
remain within the overall goal until all their dependencies are native.
