# Multiplayer shading validation history

These reports describe their recorded executable/shader snapshots. Later
changes require their own validation. Current source contracts, map coverage
and outstanding acceptance requirements are in
[MULTIPLAYER_SHADING_PARITY.md](MULTIPLAYER_SHADING_PARITY.md).

The source-shadow hook and scenery per-view cache snapshot passes all 216 normal
tests (`source-shadow-hook-full-suite.log`, 514.61 seconds). Its native binary
and matching shaders are preserved under `train-declaration-before`; binary
SHA256 is
`6b3f9f5fb23d5d4f70598e9eb94ee108c589df103a4e67829a525e0d47f99892`.
This verification precedes the later ordinary-declaration/train-stream changes.
The fresh retained Zanzibar comparison is
`live-source-shadow-hook-reference/comparison.png`; its world and first-person
region mean absolute channel differences remain 0.873614583 and 0.667061192 out
of 255. Timer and uncaptured simulation/history differences remain outside those
diagnostic regions; full-frame equality is not established.

The earlier root-order/player-cache production sweep passes 78/78
captures (`live-source-player-view-cache-all-maps/summary.json`), with unchanged
binary/shader hashes and no Vulkan or synchronization diagnostics. It covers
all 21 maps at one/two/four players plus five effect maps at 480p/1080p/4K.
The binary SHA256 is
`038fa5408d33f61b4207dca06af825e9d7e99cff1df27adc871a0cd26f1c56e8`.
Five focused ASan/UBSan/leak checks also pass for actual asset/first-person root
banks, render-cache allocation, per-view query cadence and shadow vertex
arithmetic (`source-root-cache-shadow-asan-tests.log`). The independently
validated shadow submission, plan and Renderer hook are included in the 216-test
snapshot above and retain their separate focused normal and sanitizer checks.

`live-source-player-view-reference/comparison.png` compares the same binary
against the retained original Zanzibar frame 20, using the version3 explicit
camera/player/device fixture. The world region's mean absolute channel
difference is 0.873614583/255 (1,438 channels differ by more than 8); the
first-person region is 0.667061192/255 (340 channels differ by more than 8).
These regions are diagnostic selections, not a complete-frame equality test.
The original process is not modified, and full match/HUD/controller, visibility,
clock and history state remains unrestored. Provenance records all fixture,
source image, executable, cache, shader and native capture hashes.

The `source-frustum-all-maps/summary.json` sweep records 78 passing production
captures: every supplied map at 1/2/4 local players, plus selected atmosphere
maps at 480p, 1080p and 4K. Shader/binary hashes remained unchanged throughout,
and Vulkan synchronization validation reported no diagnostics. This proves
execution and admission, not original/native framebuffer equality.

Independent original emulation evidence is under `original-xemu`; its private
HDD/EEPROM, retained game state, executable/cache hashes, RenderDoc captures
and raw camera/root lighting records preserve the comparison inputs.
`frames/ivory-original-native-stage-offsets.png` compares the source camera,
frustum, profile colors, SMG idle frame32 and retained original lighting. The
bounded fixture is diagnostic input: body controller phase, match/HUD state
and complete runtime lighting history are not restored by it. Authored material
brightness remains active; the paused final context brightness is diagnostic
only. This is not a passing complete-frame parity result.

The original/native GPU draw comparison exposed a renderer metadata loss:
reconstructing `PixelProgram` from its 60 GPU words discarded fragment offsets,
so player lighting wrote ambient RGB and alpha into the wrong combiner stages.
The renderer now retains the active offsets through initial upload and animated
updates, and supplies them during object-light, camera and fog remaps. The fresh
SMG paint draw matches all 18 original packed pixel constants. At two retained
pixels, alpha is identical and the maximum RGB difference is 0.000933, less
than one quarter of an 8-bit step; sampled filtering/BC differences remain
outside a hardware-equality claim. `object_lighting_view_isolation` now also
compares actual per-view rendering with independently CPU-bound source contexts
at three resize states, preventing a shared wrong stage mapping from passing
the isolation test.

The later `scenery-source-all-maps/summary.json` sweep also records 78/78
production captures with unchanged hashes and no validation diagnostics after
the scenery gate/probe integration. This snapshot precedes the stricter
transparent-effect admission and retained-player fixture.

The current `live-scenery-policy-all-maps/summary.json` sweep records 78/78
production captures after the resolved scenery collector/bounds integration
and original player-count layer gates. It covers all21 maps at one, two and
four players, plus the five effect maps at480p,1080p and4K. Binary/shader
hashes remain unchanged throughout and Vulkan synchronization validation
reports no diagnostics. A preceding18-capture targeted sweep covers Headlong,
Gemini, Elongation, Ascension and Zanzibar, including three extra Gemini
resolutions. These are native execution and resolution checks; no original
framebuffer comparison was performed by either sweep.

The corrected fragment offsets also pass a fresh 78-capture production sweep
(`source-stage-offset-all-maps/summary.json`) across all 21 maps and selected
480p, 1080p and 4K views. A subsequent 15-capture check on Ivory Tower, Lockout,
Coagulation, Backwash and Gemini covers the held-weapon attachment integration
at one, two and four players (`held-weapon-captures/summary.json`). Both retain
unchanged binary/shader hashes throughout their runs and report no validation
diagnostics. These snapshots precede the packed model-input decoding correction.

Third-person guns now follow the posed body hand marker through the original
world alignment, parent-local conversion and object-transform sequence. Their
own initialized idle controller is selected by the source lookup; graph-NONE
and source idle-NONE cases retain the bind pose. Authored gun colors remain
independent of player colors, and the attached gun receives its owner's lighting
bank. Actual body controller transitions, gun firing/dual state and mirror flags
are still separate runtime requirements.

The coherent-view NVIDIA texture failure is resolved by static unroll hints
on the bounded four-texture/eight-combiner loops. Original implicit texture
lookups, mip/filter selection, sampler state and program words are unchanged.
The earlier normal production NVIDIA matrix passes all 21 maps and 289,317,042
channels: direct/one-view results are exact, and two/four-view matching
interiors differ by at most one byte level. Earlier correct direct/one-view
RGB baselines remain byte-identical. The tested world SPIR-V is 49,380 bytes,
SHA256 `ed77b1cc912d87d2873b46b9e4d83a062f91e0694b7168c407eea43758a6c840`.
Evidence is in `matrix-production-nv21.log` and
`unrolled-original-baselines.json`. Failed synchronization/compiler/mip
experiments remain research evidence and are absent from production.

The subsequent live-machine matrix also passes all 21 maps with the source
machine geometry and section banks included (`live-machine-matrix-resize.log`).
Direct/one-view images remain exact, and matching two/four-view interiors remain
within one byte level. The shared-renderer resize checks include machine
material allocation and lighting bindings and pass without Vulkan diagnostics.
Independent original PRT fixtures pass both CPU and Vulkan checks across 118
vertices in nine original draws. Diffuse RGB, snapped XY and authored UVs are
exact; the separately measured projection/reciprocal precision bounds are in
`original-xemu/prt-output-audit/README.md`. These bounds do not establish physical
NV2A arithmetic precision.

The `source-uv-bounds-all-maps/summary.json` production sweep passes 78/78
captures after the node0 query-origin, transparent scenery and environment-pass
integration, with unchanged binary/shader hashes and no validation diagnostics.
It covers all 21 maps at one/two/four players plus selected 480p, 1080p and 4K
views. These execution checks do not establish original framebuffer equality.

The subsequent packed-input/query-admission build also passes all 78 captures
(`source-model-input-all-maps/summary.json`), with unchanged binary/shader hashes
and no validation diagnostics. Its fresh original/native GPU replay confirms
bit-exact active SMG UV outputs after the SHORT decoding correction. The separate
hand skin-weight difference was then recovered from the original compiled
influence variants; this sweep precedes that correction.

The subsequent `native-skin-weights_capture.rdc` production replay confirms the
blend correction on the GPU: active arm oD1 differs by at most 2.38419e-7,
environment oT1/oT2/oT3 by 1.43051e-6/1.90735e-6/4.32134e-7, and all active
weapon/hand UV outputs are bit exact. The 44 packed pixel constants and active
vertex constants match the original draws. Two base-pass c20.x differences
address a disabled texture stage. At five retained material pixels the native
scalar combiner reproduces native GPU RGB exactly; substituting only the
captured original base sample reproduces the original hand RGB and paint
within one float ULP. Original BC2 blocks remain unchanged: the emulator's
expanded texture rounding is not treated as a physical Xbox filter contract.
Raw off-screen projections, clipping and full match-state restoration are
separate open boundaries. Evidence is under `original-xemu/fp-output-audit`.

The skin-weight production snapshot passes all 74 normal checks, including the
full NVIDIA coherent-view matrix, effect resize, per-view object lighting,
retained player bindings, special textures and source content validation
(`skin-weights-full-suite.log`). Its production sweep passes all 78 map,
one/two/four-player and selected 480p/1080p/4K captures with unchanged hashes
and no Vulkan diagnostics (`source-skin-weights-all-maps/summary.json`, binary
SHA256 `aeb1448a555ca63dacc859daca82faa637b1f896e043c3eeed7d39714528cd34`).
Six focused ASan/UBSan checks pass for original-player bindings, the first-person
pose/IK path, object sky/contact helpers, device overlays and vertex-program
admission (`skin-weights-asan-suite.log`). This is the validated snapshot before
the new PRT GPU bank/stream implementation. Earlier 66-check and six-check
reports remain historical evidence, not coverage of later changes.

