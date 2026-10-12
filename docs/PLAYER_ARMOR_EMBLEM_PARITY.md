# Player armor emblem source and captured framebuffer parity

The multiplayer body animation path previously supplied only ammunition shader
inputs. It never bound the player's selected emblem page, enabled toggle or
emblem colors, so the authored armor pass received zero for its emblem inputs.
The repaired path binds each resident body material to its actual player's
sanitized live configuration. The menu preview uses the same recovered object
input resolver with its authored scenario type2.

The complete-model GPU reproduction also exposed a separate ordering defect:
standalone layer2 parts were drawn before armor lighting/environment, which
painted over them. Original `2C052` submits layer2 after those passes and before
fog. `44370` reads its target from `43FA1C + layer*24`: layer1 uses target1;
layer2 uses the current final target (`FFFFFFFF`), not the intermediate color
surface. Standalone and child layer2 passes now follow that original schedule.

An original-XBE framebuffer comparison subsequently exposed a one-pixel depth
edge defect. Interpolating W through a hardware varying rounded across an
integer boundary and rejected a Spartan emblem pixel that the original drew.
The emblem pipeline now carries the ordered triangle's original window
positions through a geometry stage and reconstructs W before D24 quantization.
This applies to `emblem_overlay_simple` in W-depth views; ordinary Z-depth views
retain their existing comparison.

## Original authority

Retail `14BFC0` copies player+89..8C, corresponding to live configuration+45..48,
into object+24..27. `BCC20 -> BC970` reads those bytes. Scenario type0 suppresses
the emblem unless flags bit3 is set. A missing scenario, multiplayer type1 and
menu type2 retain the bank. Recognized, active and scalar outputs are distinct:
a suppressed recognized input returns recognized1, active0 and scalar0; the
foreground toggle can return recognized1, active1 and scalar0.

The color producer `7F790 ->1A06A0` reads signed appearance color indices2/3
from the actual matg+160 palette, with the original white fallback. Team and
neutral overrides affect only body color channels0/1. The emblem binding
preserves the body's independently owned primary and secondary colors and
passes its two selected emblem colors through the existing original packed
object-color boundary.

Both Spartan `mc_emblem` and Elite `elite_emblem` retain their original
`emblem_overlay_simple` passes, foreground bitmap pages, vertex programs,
texture coordinates, sampler states and blending. These armor passes consume
the foreground alone. A UI background is not added to the armor. Resident
child passes are copied before per-player animation or color writes.

Original instruction references: [profile copy](../analysis/retail/functions/0014bfc0.c),
[object input](../analysis/frontend/customization/000bc970.asm),
[enclosing resolver](../analysis/frontend/customization/000bcc20.asm),
[palette producer](../analysis/retail/functions/0007f790.c),
[render schedule](../analysis/damage-shields-goal-20261011/overlay/0002bcd0-retail.asm),
[target selection](../analysis/retail/functions/00044370.c),
[literal target table](../analysis/armor-emblem/original-layer-targets.json).

## Reproducible checks

`tools/oracle_player_armor_emblem.py` runs unchanged original instructions with
host object/player pools and the literal Lockout palette. It retains 24,576
input cases across absent/type0/type1/type2 scenarios, 768 profile dword copies,
and 256 palette cases. The same 24,576 input cases are also replayed against
map-pack TU1.1 `BDB10 ->BD860` and the launcher's owned u5 `BEC50 ->BE9A0`.
Executable/map hashes and scope are recorded in
`analysis/armor-emblem/original.json`; the durable fixture is
`tests/fixtures/player_armor_emblem_original.tsv`.

`retail_player_armor_emblem` compares every retained result, all 64 foreground
pages for both species in Lockout/mainmenu, every authored emblem shader LOD,
both retained original 60-word menu model pixel programs, toggles, colors,
unchanged placement/sampler/blend bindings and independent shared child banks.

`player_armor_emblem_gpu` inspects the complete posed armor assets using declared
cameras aimed at the largest connected authored emblem patch. It compares
disabled, page26, changed page/colors and repeat frames, retaining the surrounding
armor and environment passes. Changed opaque foreground pixels must retain
their exact original packed palette RGB; that assertion rejects the incorrect
intermediate-surface routing even though it also makes an emblem visible.
`player_armor_emblem_runtime` observes
actual native four-view gameplay materials immediately before GPU upload,
using four selected profiles and both species. These checks establish native
delivery; their inspection cameras are not original gameplay camera captures.

Use a Release build and run the focused checks with the bundled validation layer:

```sh
cmake -S . -B build-armor-emblem-20261011 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-armor-emblem-20261011 --target halo2_native player_armor_emblem_tests player_armor_emblem_gpu_tests player_armor_emblem_runtime_tests frontend_emblem_tests native_player_profile_tests -j 4
VK_LAYER_PATH="$PWD/.tools/validation/usr/share/vulkan/explicit_layer.d" \
LD_LIBRARY_PATH="$PWD/.tools/validation/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
ctest --test-dir build-armor-emblem-20261011 --output-on-failure -R '^(native_player_profile|retail_frontend_emblem|retail_player_armor_emblem|player_armor_emblem_gpu|player_armor_emblem_runtime)$'
```

The Release verification passed all eight focused checks, including source
render policy, dynamic model shader integration and material-frame selection,
in 30.85 seconds. The strengthened exact-color GPU check then passed in 5.08
seconds. The Spartan and Elite captures retained 3,109 and 2,035 changed pixels
respectively at exact primary RGB(54,116,122). Gameplay observed 16 actual armor
banks across four profiles and both species. No validation-layer error was
reported. [Focused results](../analysis/armor-emblem/verified-tests.log),
[full results](../analysis/armor-emblem/verified-tests-full.log),
[stronger color check](../analysis/armor-emblem/verified-color-test-full.log).

Both root launchers were installed against
`build-armor-emblem-20261011/halo2_native` and verified with `--launcher-info`.
The delivered binary hashes are in `analysis/armor-emblem/delivered-hashes.json`;
the tested source hashes are retained in `analysis/armor-emblem/source-hashes.json`.
Concurrent subsequent workspace edits are not part of that tested binary.

## Original-XBE framebuffer comparison

Two new captures run the unchanged original disc `default.xbe` and
`mainmenu.map` in xemu 0.8.136, commit
`fc24584ce88f0915ad7f04775bb7712c2e3f49ee`, using its OpenGL renderer on an
NVIDIA RTX 5080. These are emulated original-game captures, not physical Xbox
framebuffer readbacks. The exact executable, map, RenderDoc captures and replay
inputs have hashes in
[`tests/fixtures/player_armor_emblem_framebuffer/provenance.json`](../tests/fixtures/player_armor_emblem_framebuffer/provenance.json).

The captured state is foreground 26 (Thor), emblem colors 9/1, body colors 8/12,
flags 0. All eight original uploaded foreground mips decode byte-identically to
the independently captured native GPU texture. The actual native material's
first 60 words equal both retained original live emblem pixel programs.

`retail_player_armor_emblem_framebuffer_gpu` executes the production
`world_emblem.geom` and `world_emblem.frag`, with a small vertex adapter supplying
the captured original post-transform positions and texture coordinates. It uses
the original pre-pass color framebuffer and real D24S8 depth attachment, the
captured native material UBO, authored mips, original culling, trilinear border
sampling and RGB-only alpha blending. It compares every RGBA byte in the complete
640x480 post-pass framebuffer. There is no image registration, fitted color or
placement adjustment.

| Original draw | Triangles | Original changed pixels | Old differing pixels | Corrected differing channels |
| --- | ---: | ---: | ---: | ---: |
| Spartan 26003 | 44 | 362 | 1 | 0 |
| Elite 26482 | 26 | 227 | 0 | 0 |

At Spartan pixel (514,124), original RGBA changes from (93,186,171,0) to
(57,98,93,0). The old fragment shader leaves the pre-pass pixel unchanged:
interpolated W crosses the next integer depth boundary, while original ordered
reconstruction passes the comparison against stored depth 23994. The regression's
uncorrected production `world.frag` control fails with three differing channels,
maximum error 88 and summed error 202. The corrected production fragment passes
both complete buffers exactly, with Vulkan validation enabled.

[Corrected production replay](../analysis/armor-emblem/framebuffer-20261012/production-fragment-green.log),
[uncorrected production control](../analysis/armor-emblem/framebuffer-20261012/production-fragment-red.log),
[pixel details](../analysis/armor-emblem/framebuffer-20261012/pixel-comparison.json).

```sh
cmake --build build-armor-framebuffer-20261012 --target player_armor_emblem_framebuffer_gpu_tests
VK_LAYER_PATH="$PWD/.tools/validation/usr/share/vulkan/explicit_layer.d" \
LD_LIBRARY_PATH="$PWD/.tools/validation/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation \
ctest --test-dir build-armor-framebuffer-20261012 --output-on-failure \
  -R '^retail_player_armor_emblem_framebuffer_gpu$'
```

The final Release verification passed all 12 focused checks in 50.78 seconds,
including the complete armor inspection, four-player gameplay delivery and the
haze, popup-blur and bloom tests that share the offscreen harness. The full log
contains no Vulkan validation error. The CTest-produced Spartan and Elite
framebuffer hashes equal their original reference hashes exactly.
[Test log](../analysis/armor-emblem/framebuffer-20261012/verified-tests-full.log),
[byte comparison](../analysis/armor-emblem/framebuffer-20261012/verified-framebuffer-comparison.json).

A new capture of the rebuilt native menu preview confirms its actual 132-vertex
Spartan emblem draw 6247 uses the production geometry stage, the unchanged
original 60-word emblem program and all three flat triangle-window inputs in the
fragment shader. The preview rendered three 640x480 frames with validation enabled.
[Native pipeline proof](../analysis/armor-emblem/framebuffer-20261012/delivered-pipeline.json).

The armor-only delivery selected `build-armor-framebuffer-20261012/halo2_native`.
Its SHA256 is `daf79bcb5c31588ace836dbe147b0cb3d0a01dfa08d33f7263b51a57062b1d43`.
The build uses a retained source snapshot; subsequent concurrent workspace
changes are not part of this verified binary.
[Delivery hashes and launcher checks](../analysis/armor-emblem/framebuffer-20261012/delivery.json),
[matched emblem crops](../analysis/armor-emblem/framebuffer-20261012/matched-emblem-passes.png).

The subsequent [scoreboard and frontend emblem delivery](PLAYER_UI_EMBLEM_PARITY.md)
selects `build-scoreboard-emblems-20261012/halo2_native` and reran both original
armor framebuffer fixtures with byte-equal results.

## Remaining evidence boundary

Byte equality is established for these two captured original-game emblem
passes with identical geometry, depth, background and shader inputs. A complete
native gameplay or menu frame has not been synchronized against the original
camera and animation. These two captures also do not cover every foreground,
palette pair, LOD, pose, resolution or physical Xbox GPU behavior. The broader
source fixtures cover all 64 foreground pages and authored emblem shader LODs,
but that is separate from framebuffer evidence. Complete campaign, corpse/profile
replacement, physical LAN and all frontend rendering parity are not claimed.
Callers without a source profile retain their existing absent-profile behavior.
