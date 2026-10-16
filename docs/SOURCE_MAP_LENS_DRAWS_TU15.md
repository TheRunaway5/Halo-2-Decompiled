# Original final lens draw and GPU consumer

The final-flare kernel now runs in ordinary source matches and produces visible
Elongation flares with one and four local players. On/off/repeated-input checks
pass with identical query and RNG histories. The tested build is installed at
the normal runtime path and both launchers select it. The all-map goal remains
active.

Authority is the owned English OG Xbox TU1.5 XBE, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`,
and the actual Xbox map assets. No Vista shader or replacement artwork is used.

## Implemented

`generate_lens_draw_tu15.py` lowers 52 complete original bodies containing
5,161 distinct instructions into compiled C++. These include whole `2E9C0`,
the payload, color, orientation and primitive producers, bitmap binding,
the original `1301E0` texture-header constructor, and `1CCD0`/`4044B0` pixel
program setup. Required absent owners still fail with their actual ABI.
The existing original vehicle CRT owner supplies complete `fmod` execution;
its instruction observations are translated through the authenticated TU
correspondence in the comparison test.

`166A80` is CRC32 over retained request identity. It is **not an RNG call**.
The original seed/table and source request address are preserved.

`generate_lens_program_tu15.py` authenticates the parent chain
`2BCB0/2C302 -> 13850/13B5E -> 2DEE0`, the 60-word pixel-program writes,
and the actual rasterizer vertex-reference slot10. Its authored program has
four MOV instructions. The GPU consumer retains the complete original
register-combiner program, including its nonlinear term.

`SourceLensDrawGpu` consumes strict 136-byte original `48090` quad packets,
the actual attribute8 secondary alpha, and the caller's depth mode. It borrows
real resident renderer images and samplers. The pipeline uses original
SRC_ALPHA/ONE addition, RGB-only writes, no depth writes, and the source
optional LEQUAL W-depth test. `ModelView::lens_draws` places those explicit
submissions after foreground and bloom, before HUD drawing. Empty spans do not
create flare requests or allocate a final-flare GPU owner.

`SourceLensDrawFrameTu15` retains actual authored bitmap metadata before
renderer construction and publishes original texture headers only after
verifying their genuine resident image leases. It supplies each view's actual
camera/projection and retains the source game clock and request identities.
Whole `13660` reaches its unchanged `2BCB0` world-pass boundary with the actual
view/mode/BL inputs; its original enable argument gates final drawing. After
query completion, original `20B90` selects requests with the final callback and
class mask. The adapter consumes the complete emitted FIFO, including bitmap
bindings, attribute8, depth state and strict original primitive packets.
Unowned secondary `46E90` output fails explicitly.

## Verification

Evidence is retained under
`analysis/map-world-features/lens-reflection-20261015/`.

- 71 geometry/orientation/CRC ABI cases pass with the real source-memory
  owner. The original hardware run contains 62,901 instructions.
- 45 complete callback cases pass full native GP/SSE/defined flags,
  instruction path, retained memory, x87 and MXCSR comparisons. They use nine
  authored definitions from Headlong, Backwash, Elongation and Terminal.
  The unchanged host-i386 execution contains 615,636 instructions.
- 54 constructor/program cases pass the same comparisons: 45 actual bitmap
  metadata rows and nine pixel-program publications, with 12,906 unchanged
  original instructions.
- 108 parent-gate controls pass original/native ABI comparisons with 6,680
  unchanged original instructions. These declare a cold postprocess pointer;
  active motion-blur histories are not established.
- The final GPU test submits all 45 callback packets, including 25 emitted
  quads, using 46 actual resident bitmap-image records. It admits the exact
  authored vertex program from each map's real rasterizer slot. The separate
  visible, occluded, zero-alpha, depth-disabled and repeated-input controls
  pass; visible input changes 16,415 pixels. Depth/stencil bytes are unchanged
  and Vulkan validation reports zero errors on the RTX5080.

The GPU test uses declared camera, visibility and ready-texture inputs.
Bitmap headers are produced by unchanged original code. These controls do
not establish the original resource-I/O history or a live match. The source
callbacks' GPU packets are diagnostic fixture inputs, never a production
packet replay. The kernel itself is compared independently against original
execution. This is host CPU/GPU evidence, not an original Xbox framebuffer
comparison.

The preceding reflection stage's focused binaries, source copies, libraries and
shaders are pinned in `focused-v1/` and `acceptance.json`. Its first CMake 71-case test completed;
a later broad rebuild was interrupted while concurrent shared-header edits
were invalidating dependencies. The final focused checks use frozen libraries
and sources. Shared renderer/BSP/header files advanced during these checks;
the receipt lists their differences from that frozen candidate. The later live
stage uses a separate successful full build and retains its own source pins.

## Next live boundary

The fresh frozen full build and ordinary acceptance evidence are under
`analysis/map-world-features/lens-live-20261015/`. Elongation uses its ordinary
spawn and a recorded native look input, without camera or root overrides.
At tick 150 the real GPU query reports 16 samples and original smoothing yields
alpha 200. Hiding only final lens submissions changes 397 pixels with one view
and 87 pixels in the first of four viewports. Both enabled repeats are exact;
capture state, request/query history and RNG results match the hidden controls.
The other three views retain their actual zero-visibility results.

The ordinary render path also required complete original `45A70` subpart-mask
selection. It is now retained in the existing world/shadow module without
dropping its other 140 bodies. All 193 sites are exercised by 54 unchanged
original/native integer selection controls, covering its three metadata paths
and both selection forms. Those controls do not claim scene/tag admission.

Preserve queued `46E90` secondary submissions where authored; do not silently
drop them. Direct-object lens requests remain separate unfinished work.

The fresh runtime passes 32 candidate captures across all 21 Xbox maps. Four
post-installation Elongation/Lockout captures with one/four views match those
candidate images and query/RNG histories exactly. Both normal launchers select
the installed `84f600c9` binary. Its compiled private shader directory is
retained, pinned and copied identically to the normal shader bank; see the
[delivery receipt](SOURCE_WORLD_LAUNCHER_DELIVERY.md). The prior query receipt
is historical evidence. Both launchers still report the MCC-only
native-gameplay readiness boundary.

The remaining scope across all 21 Xbox maps and two Vista maps, full original resource histories,
network/late-join/reset behavior and Xbox visual acceptance remain open.
