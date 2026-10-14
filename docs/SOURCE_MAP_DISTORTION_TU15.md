# Ascension and Colossus authored distortion continuation

Both maps' authored lift particle uses `effects\generic\distortion\distortion_ripple`,
the `effect_shader_alphablend_distortion` pass, texture modes `[1,6,0,0]`,
signed RGB on stage0, and external23 on stage1. The original classification7
vertex program has44 instructions, SHA-256
`8087762549a70544d2d07a2120dd052fbaf4c34594a040007fc06a80ad52aaf9`.
It differs from the ordinary particle program. The preserved GLSL is translated
from the owned Xbox cache's tokens and consumes caller attributes/constants;
it embeds no captured vertices, camera or expected output.

The production particle renderer now selects this separate authenticated
program, decodes mixed signed RGB/unsigned alpha before filtering through the
existing texture-storage owner, and implements stage1 BUMPENVMAP. The live caller now supplies the completed bump matrix and coordinate
normalization, binding external23 to the ordered current-surface copy. Missing
state, a missing second binding, inactive dependencies, unknown programs and
unsupported texture modes remain errors.

The shader-source tests build the shipped distortion program without game
files and check its distinct identity. The production graphics fixture loads
the actual distortion program from each map, supplies declared texture-stage
inputs, and compares independently calculated filtered pixels. Sixteen cases
cover both maps, D24/D32, signed/unsigned bump inputs and zero/nonzero matrices.
The initial passing bank has maximum one-byte color error and unchanged depth.
This is a bounded renderer test, not an original Xbox pixel comparison or a
live lift capture. The existing captured particle graphics test also passes.

## Xbox framebuffer binding evidence

The owned TU1.5 executable has SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Untouched entry19510 selects external23 at199D4, which calls14F40.
The target23 switch byte at1514B is12, selecting1506C. That branch publishes
`515E34[((counter64 - 1) signed-remainder 2)]` to the requested stage's52DFA8
slot. It does **not** bind the texture header in target23's descriptor.
The descriptor supplies320×240 dimensions when ready and zero dimensions
when busy/absent; the common failure epilogue19AF5 overwrites the initial1×1.
The binder does not advance the counter or overwrite another texture stage.

At145E0, the fixed two texture headers alias the initial physical addresses
from the SDK surfaces at device+1A14/+1A18. SDK403B50 rotates the surface
addresses while those texture headers retain their initial addresses. The
counter starts at1 in169D0 and13EF0 advances it after presentation. In the
declared two-surface rotation, external23 selects the **current physical render
surface**, so a previous-frame image would be the wrong substitute.

`tools/oracle_particle_distortion_binding_tu15.py` executes120 complete original
external23 calls across four stages, readiness states and counter values,
including carry into the high word. It also executes eight complete original
SDK surface rotations with declared device/command-buffer inputs. All pass;
198 unique untouched instruction addresses are retained. No game service or
instruction patch is supplied. The declared device state and counter updates
do not establish full original presentation scheduling or GPU memory coherency.

## Live stage handoff and current-surface consumer

Untouched18540 consumes actual shader mappings, overlays and curves for both
maps. Six complete original calls with different prior stage states write raw
SDK22=5,23=0,25=0, then25=5. Duplicate25 and untouched24 are retained. The native
decoder admits the matching constant scalar overlay shape; other functions and
transformed producers require their completed original owner. The retained raw
state bank applies ordered writes and preserves unrelated lanes/stages.
SDK method order00/01/11/10 becomes NV2A uniform order00/01/10/11. The actual
water draw bank contributes raw state before later particles. Other world
sampler producers and complete graphics scheduling remain open.

The initial raw bank is zero, as in the SDK image and a retained live TU1.5 RAM
bank whose whole text section matches the owned executable. This is not execution
of the complete original graphics constructor. Nonzero declared prior banks
independently verify the stage24 retention contract.

ParticleRenderer borrows the renderer-owned scene view and requires each
external draw to resolve its unique uniform slot. The frame renderer applies
authored writes, ends the pass, copies current color, restores layouts, resumes
a color/depth/stencil continuation pass and records the dependent particle.
Each subsequent dependent draw gets its ordered copy. Completed vertex surface
dimensions normalize screen coordinates; split-screen uses the existing
full-surface camera/viewport bank. Original retail batching, physical Xbox
memory coherency and pixels remain unproven.

The declared GPU bank has32 cases:16 static bindings and16 borrowed-view feedback
cases, across both maps, D24/D32, signed/unsigned inputs and zero/nonzero matrices.
Maximum color error is one byte level, with unchanged depth. The existing18
original particle draw/depth comparisons and six original sampler CPU cases
still pass. Earlier original binder evidence retains120 external23 calls and
eight SDK rotations.

Live one-view close-camera on/off/repeat banks change32,176 Ascension pixels and
91,151 Colossus pixels. Enabled repeats are exact and source simulation/RNG
lines agree. Camera-only diagnostics retain actual source objects/PVS and their
effects. The protected base passes21 ordinary Xbox renders and nine four-view
map renders. The final kernel extension preserves byte-identical enabled
close-ups and source lines. Four-player diagnostic camera replay is rejected;
the ordinary four-view Ascension comparison changes zero pixels, so visible
split-screen lift acceptance remains open. No pixel threshold was relaxed.

Resource admission is93/96. Elongation lens1, Backwash controller2 rendering,
full sampler/frame scheduling, all interactions/network/reset behavior,
Vista-only resources and Xbox framebuffer acceptance remain unfinished.
The all-map goal stays active.

Evidence and coherent source/builds are preserved under
`analysis/map-world-features/texture-continuation-20261014/`, including red
reproductions, authored bindings, complete relevant source/SDK listings,
external23-oracle-v5, bump-owner-original-v3 and production GPU logs. Earlier failed harness runs are
retained separately; the final passing run establishes the stated behavior.

Live implementation and receipts: analysis/map-world-features/distortion-live-20261014/
and analysis/map-world-features/swarm-frame-20261014/launcher-extension/.
