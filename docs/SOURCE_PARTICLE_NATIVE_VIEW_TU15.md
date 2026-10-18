# Native particle view ownership and original camera continuation

`Renderer::particle_view_inputs()` now exposes recorded native particle inputs
through an issued resource use. It reads c8..11, vertex/depth clip inputs and
depth parameters from the actual descriptor-backed mapped uniform allocation,
and retains the pipeline, descriptor, buffer offsets, commanded viewport and
scissor. It expires when prepare/record changes the bank, targets are recreated,
the renderer retires, or the borrower changes threads. Recording does not prove
GPU submission or completion.

`SourceParticleGraphicsBindingTu15::initialize_view()` admits the recorded
full-surface programmable W path. It verifies the actual per-view scissor and
source viewport constants, retains the real depth/program/target owners, and
runs whole original `13430`. The declared native ABI has explicit programmable
clip bounds; its bit2 is not inferred Xbox SDK boot state. Only the initializer
can publish the bounds and control-cache fields. Unknown readers and other
device fields remain unavailable. Nonempty queues reject initialization before
any reset. The operation must precede queue collection; it never restores
discarded counters after initialization.

The complete20-byte bounds/control packet is consumed atomically alongside the
existing program/declaration/start/constants. `consume_draw()` validates the
recorded draw inputs and publishes the original clip words to both production
shader inputs. Native tests cover two actual split-view descriptor banks,
default and boundary1 initialization, original packet values reaching the next
GPU uniform allocation, queue-reset rejection, and resource lifetime guards.
The caller's source globals are declared fixtures here; this does not prove
original `169D0` startup or a live collection/submission sequence.

The captured seven-draw world diagnostic exposed two useful rejections. Its
source constants describe the top-right320x240 split rectangle, so recording a
full-screen scissor cannot establish matching view ownership. With that rectangle
corrected, its near bound is38.3999977 rather than the initializer's255.999985.
The frozen production caller supplies the original scaled-projection factor0.15.
Those failed candidates are retained. No boundary override or guessed near/far
values are used to make them pass.

Whole original `12D30` is now lowered, authenticated through the containing
`132B0 ->12F80 ->12D30` call chain. `132D0` is an interior address and is not
used as an entry. The unchanged original factor at`44A8A0` is imported. The
module has65 whole bodies and4,922 unique instruction sites; all64 prior body
site sets are unchanged. Forty ordered CPU calls cover projection, viewport,
reset, complete camera production and restore, including scaled/unscaled and
explicit/default camera contexts. Their declared output pointer avoids the
separate unowned `401360` fallback; that branch remains an explicit required
service. The native camera/resource producer and this routine's full live
handoff still need integration.

All40 calls match the unchanged original i386 across GP/SSE, defined arithmetic
flags, x87/MXCSR, ordered paths and every144,809 retained byte per case, with
10,887 native instruction callbacks. The two actual native view initializer
calls also match all138,025 retained bytes each and their complete CPU/path
state. No game/GPU return or original instruction is patched in these checks.

Evidence and pinned private builds are under
`analysis/map-world-features/particle-native-view-20261016/`. The prior actual
seven-draw program/declaration/start/constant GPU regression remains a separate
positive check on cleared scene depth. It is not acceptance of scaled native
view integration, ordinary world-depth visibility, or an Xbox framebuffer.
The full queued parent still stops at its unowned live `3FF611` cache read.

The normal CMake interfaces and affected objects compile. This stage does not
install a whole combined runtime. Both launchers continue selecting the shared
normal runtime; their current receipt still reports native MCC gameplay
unavailable. The accepted7501144f world binary and59 pinned shaders are preserved.
All21 Xbox plus2 Vista maps, original scheduling/resource/RNG inputs,
host/client/latejoin/reset behavior, complete stream/parent ownership, combined
normal-launcher delivery and Xbox visual acceptance remain active requirements.

The [camera publisher continuation](SOURCE_PARTICLE_CAMERA_PUBLISHER_TU15.md)
adds complete`12F80` camera packets and checks them against real descriptor
inputs. Its native caller/World-history and live ordering limits remain explicit.
