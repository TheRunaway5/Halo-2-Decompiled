# Retained original20 sky stencil writes

Unchanged original20 Vulkan replay establishes one actual stencil producer:
sky draw8097 changes **462 pixels from00 toFF**. Draw8102 and moon8110 preserve
that image exactly. All three leave every stored D24 integer unchanged. This
is an observed source-frame result, not a generated native stencil mask or
physical Xbox precision proof.

The input is `original-xemu/original_capture_20.rdc`, SHA256
`d99a5c97c85916c7b07e30dc1f181d6db81b3a5246686bcbeebebbc5445642ee`.
Only standard RenderDoc offline replay/readback APIs were used. No guest frame,
replacement shader/resource, fitted geometry, or title/SDK instruction change
was used. Both replay controllers and capture handles were shut down before
the hardware slot was released.

## Actual stored transitions

The six resources are immediately before and after the three draws, at
inclusive replay events8096/8097,8101/8102 and8109/8110. The attachment is
`ResourceId::8601`, D24S8,640x480, one sample.

| Draw | Stencil before | Stencil after | Changed pixels | Depth changes |
| --- | --- | --- | --- | --- |
| 8097 sky A | 307,200 zero | 306,738 zero;462FF | 462, all00→FF | 0 |
| 8102 sky B | 306,738 zero;462FF | identical | 0 | 0 |
| 8110 moon | 306,738 zero;462FF | identical | 0 | 0 |

The changed coordinates have inclusive bounds x184..381,y78..125. These are
the replay readback/`PickPixel` coordinates. The complete source-derived stencil
bytes are retained; the bounds do not substitute for the actual coverage.
Draw8102-before,8102-after,8110-before and8110-after equal8097-after byte for byte.
The lack of new writes on the latter two draws does not prove that their
fragments never pass: writing an already-FF byte also leaves it unchanged.

## Copied-byte layout and independent controls

`GetTextureData` returns1,536,000 bytes for this Vulkan D24S8 image. The first
1,228,800 bytes contain307,200 little-endian DWORDs with depth in low24 and
stencil in high8. The trailing307,200 bytes are unused, zero bytes in all six
exports; they are **not** the stencil plane. The retained primary RenderDoc
`vk_replay.cpp:4739–4746` reserves separate aspect-copy space, then4916–4933
repacks D24/stencil into the leading DWORDs.

All **54 actual `PickPixel` controls** agree exactly with binary32 normalized
depth `(word&0xffffff)/0xffffff` and stencil `(word>>24)/255`. Controls include
two changed pixels,241,85 and367,120, plus unchanged foreground/background
and image-edge points. This independently checks the packing and row indexing.
The separate Lockout OpenGL readback places depth high24 and stencil low8;
that different API layout is not used for these files.

## Actual state and geometry boundary

All three actual packets enable stencil on both faces with ALWAYS,
KEEP fail/depth-fail, REPLACE pass, and FF reference/read/write masks.
The exported RenderDoc enum names pin compare1=AlwaysTrue and pass2=Replace;
these numbers are not interpreted as Vulkan enum ordinals. The packets use
LEQUAL depth testing without depth writes, no culling, depth clamp enabled,
depth clip disabled, zero depth bias, and one sample. Their blend writes RGB
with SRC_ALPHA/INV_SRC_ALPHA and preserves alpha.

Sky A/B are layer15 sky_dome/clouds_low before the moon. The independently
decoded Zanzibar sky metadata identifies sections0/1/2, part0, raw parttype4,
material2/1/0 and74/49/4 strip indices respectively. Layer15 alone is not
an admission predicate; the title's `2BCD0→4C690` also applies source part and
view predicates. Moon8110 is **stencil-enabled** in this source packet.
No different moon stencil state is inferred from its material or blend.

8097/8102 original VS/PS banks, shader disassembly, indices, post-VS and texture
mips remain in `appearance-residual-replay/source`. The additional moon packet
and those resources are under `original20-moon-stencil`. The source clip-region
gate and W/depth behavior still control covered passing fragments. An isolated
native writer must consume that actual geometry, shader/clip/depth input and
state; a fullscreen quad, approximate coverage rectangle or fabricated mask
does not reproduce this producer.

The immediate8097-before image is all stencil zero. It does not identify the
earlier clear call. These intervals do not establish later resets, any other
view/map, the source stencil lifecycle, or which producer created Lockout's
separate6,255FF pixels. The live Renderer still has no such stencil writer;
these artifacts do not activate weather or modify production rendering.

## Reproduction and frozen paths

GPU export tools are `tools/export_original20_sky_stencil.py` and
`tools/export_original20_moon_stencil.py`; they require a coordinated replay
lease. The CPU-only `tools/analyze_original20_sky_stencil.py` verifies the
capture/export hashes and54controls, separates depth/stencil, and publishes
`analysis/multiplayer-shading/original20-sky-stencil-analysis/report.json`.
The source raw reports are `original20-sky-stencil/report.json` and
`original20-moon-stencil/report.json` in the same analysis tree. The independent
world migration audit remains `SOURCE_WORLD_D24_MIGRATION_AUDIT.md`.
