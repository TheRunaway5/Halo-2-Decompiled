# Camouflage target3 depth/color alias contract

Target3 retains the original main color surface format and points it at the
actual depth surface's backing. With the recovered formats, the alias's blue
channel occupies the stencil byte. Layer19's blue-only color write therefore
has a stencil-storage consequence even though hardware stencil testing is
disabled at layer entry. This is a source-derived storage contract. Exact
camouflage raster output, blend quantization and simultaneous color/depth alias
behavior have not been verified on physical Xbox hardware or a faithful
emulator implementation.

The separate [pass/resource research](MP_CAMOUFLAGE_PASS_SOURCE_RESEARCH.md)
establishes scene copying, layer18/19 scheduling, authored programs and states.
This note resolves the alias channel layout and records the emulator boundary.
No implementation, CMake, progress or variant admission is changed.
[Reproducible inspector](../tools/inspect_camouflage_depth_alias.py),
[report and source pins](../analysis/multiplayer/game-variants/camouflage-depth-alias-20261011/report.json)

## Actual headers and original format owner

The immutable original Lockout RAM export at weather particle entry contains
these six-DWORD surface headers. This is an actual retained source context,
not a captured camouflage draw. Pointer global509370 is the target3 color
alias; global509364 is the original depth surface.

| Surface | Header address | Data/backing | Format | Size |
| --- | --- | --- | --- | --- |
| Main color50935C | `00406A04` | `03DB0000` | `00011229` | `271DF27F` |
| Other color509360 | `00406A1C` | `03C84000` | `00011229` | `271DF27F` |
| Depth509364 | `00406A4C` | `03B58000` | `00012E29` | `271DF27F` |
| Alias color509370 | `7FF10000` | `03B58000` | `00011229` | `271DF27F` |

The complete original14600 instructions copy all24 bytes of the main color header to509370,
then replaces only its Data word+4 with depth header+4. The captured alias
also has a changed Lock word+8, which does not change its format or backing.
Original14BC0 target3 selects this alias and the actual depth surface.
Original1CD30 chooses the full XDK bind3F7870 or equal-format bind3F7B30.
Both XDK owners emit the color and depth offset methods; their instructions
do not substitute a separate color allocation for target3.
[Original alias owner](../analysis/multiplayer/game-variants/camouflage-depth-alias-20261011/surface_aliases-00014600.asm),
[target binding](../analysis/multiplayer/game-variants/camouflage-depth-alias-20261011/target_bind-00014bc0.asm),
[cache/bind owner](../analysis/multiplayer/game-variants/camouflage-depth-alias-20261011/surface_bind-0001cd30.asm),
[full XDK binding](../analysis/multiplayer/game-variants/camouflage-depth-alias-20261011/xdk_full_bind-003f7870.asm),
[equal-format XDK binding](../analysis/multiplayer/game-variants/camouflage-depth-alias-20261011/xdk_equal_format_bind-003f7b30.asm)

The inspector executes unchanged **whole3FD860** with the actual copied
headers. Main/other/alias color with the actual depth header, and two declared
depth-NONE controls, all return`00000128`; their headers remain unchanged.
The source's original XDK table/jump instructions yield color8, zeta2 and
linear layout1. Original3F6520 emits this value through method208.
The pinned NV2A definitions identify color8 as`LE_A8R8G8B8`, zeta2
as`Z24S8`, and method208 as`SET_SURFACE_FORMAT`.
[Whole format owner and jump tables](../analysis/multiplayer/game-variants/camouflage-depth-alias-20261011/xdk_surface_format-003fd860.asm),
[original format packet owner](../analysis/multiplayer/game-variants/camouflage-depth-alias-20261011/xdk_format_packet-003f6520.asm),
[NV2A format definitions](../tools/reference/xemu-0.8.136/hw/xbox/nv2a/nv2a_regs.h:866)

## Byte mapping and the blue write

Let N be the24-bit stored depth payload and S the8-bit stencil byte. The
NV2A packed backing word is`W=(N<<8)|S`, with little-endian memory bytes
`[S,N_low,N_middle,N_high]`. In the original color format those bytes are
`[B,G,R,A]`. The pinned xemu OpenGL format table uses BGRA/REV for this
color format and24_8 for packed depth/stencil. Its Vulkan conversion explicitly
packs depth in high24 and stencil in low8, and reverses that operation on
upload. The host Vulkan depth-aspect copy layout is a separate representation.
[OpenGL format table](../tools/reference/xemu-0.8.136/hw/xbox/nv2a/pgraph/gl/constants.h:286),
[pinned conversion source](https://github.com/xemu-project/xemu/blob/fc24584ce88f0915ad7f04775bb7712c2e3f49ee/hw/xbox/nv2a/pgraph/vk/surface-compute.c#L29-L70),
[Vulkan zeta-to-texture conversion](../tools/reference/xemu-0.8.136/hw/xbox/nv2a/pgraph/vk/texture.c:597)

| Alias color channel | Backing bits | Depth/stencil interpretation |
| --- | --- | --- |
| B | 7..0 | Stencil S |
| G | 15..8 | Depth N bits7..0 |
| R | 23..16 | Depth N bits15..8 |
| A | 31..24 | Depth N bits23..16 |

Layer19's actual compiled state record`4301000000` means source render
state67,value1. The unchanged original table at43E6F0+67*4 yields packet
`00040358`. Whole original3F5CF0, executed against a declared sufficient
push buffer, emits exactly`[00040358,00000001]`. All seven authored
layer19 state records are independently emitted without replacement services.
NV097 method358 is`SET_COLOR_MASK`; its bit0 enables blue, with green/red/alpha
at bits8/16/24. This is not an RGBA-order interpretation of integer1.
[Authored raw layer19 records](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/assets.json),
[whole packet writer](../analysis/multiplayer/game-variants/camouflage-depth-alias-20261011/xdk_state_packet-003f5cf0.asm),
[original packet results](../analysis/multiplayer/game-variants/camouflage-depth-alias-20261011/report.json),
[NV2A mask definition](../tools/reference/xemu-0.8.136/hw/xbox/nv2a/nv2a_regs.h:1000)

The same authored records enable blending with source factor306
(`DEST_COLOR`), destination factor0, operation8006(`ADD`), depth
comparison202(`EQUAL`) and depth writes0. Therefore the recovered color
operation's blue component is`B_source*B_destination`, before target
quantization. Given this alias,`B_destination=S/255`. Only the resulting
quantized blue byte replaces S; the three stored depth bytes are preserved
by the color mask. The262,144 logical storage controls verify that byte
projection for every input/replacement stencil byte at four declared depth
payloads. **These are logical byte controls, not original GPU executions or
an NV2A blend-rounding model.** The shader's source blue output and passing
fragment coverage must still come from the actual program and geometry.
[Authored states and externs](MP_CAMOUFLAGE_PASS_SOURCE_RESEARCH.md),
[NV2A blend/mask consumers](../tools/reference/xemu-0.8.136/hw/xbox/nv2a/pgraph/pgraph.c:1302)

Original44370 invokes15370 with BX0 for layer18 and layer19, disabling
hardware stencil testing at entry. That does not protect the stencil byte
from the separately enabled color-alias write above. No unconditional
stencil clear or replacement operation is inferred from the pass name.
[Layer setup](../analysis/multiplayer/game-variants/camouflage-depth-alias-20261011/layer_select-00044370.asm),
[stencil control](../analysis/multiplayer/game-variants/camouflage-depth-alias-20261011/stencil_control-00015370.asm)

## What the existing captures establish

The saved original software-OpenGL weather event215778 has an actual sampled
depth-as-color texture. Rechecking all307,200 pixels gives captured RGBA
bytes`raw_depth[2,1,0,3]`, exactly the channel mapping above. Its before/after
depth-stencil bytes are identical. This establishes a sampled alias layout;
it does not contain a verified layer19 color-alias write. Raw files and their
existing hashes remain unchanged.
[Original surfaces and hashes](../analysis/multiplayer-shading/weather-plate-original-reference/event-215778/surfaces.json),
[sampled-alias result](../analysis/multiplayer/game-variants/camouflage-depth-alias-20261011/report.json),
[separate native sampled-alias proof](SOURCE_WEATHER_DEPTH_ALIAS_PROTOTYPE.md)

Original20 sky event8097 separately establishes462 actual00→FF stencil
writes with depth unchanged. Its RenderDoc Vulkan readback repacks the
leading DWORDs as depth-low24/stencil-high8, unlike the Lockout OpenGL raw
layout. That capture proves an ordinary sky stencil producer, not camouflage
alias feedback or layer19 coverage. Applying its API readback representation
directly as the NV2A alias backing would reverse the relevant interpretation.
[Sky capture and independent readback controls](SOURCE_ORIGINAL20_SKY_STENCIL.md)

The new report inventories20 saved original-xemu capture metadata files
with capture references. No named camouflage pass is present in those
metadata files, which primarily list actions/resources. This is explicitly
**not** an exhaustive shader/state inspection of every raw RDC. No existing
capture examined here establishes the complete biped camouflage effect.

## Pinned emulator limitation

The local xemu source tree is pinned to
`fc24584ce88f0915ad7f04775bb7712c2e3f49ee` (0.8.136). Every inspected file's
Git blob SHA1 is recomputed against the pinned tree, with SHA256 also recorded.
Both backend surface-update owners explicitly diagnose simultaneous color
and zeta targets at the same offset as unsupported and unbind the opposite
target. They cannot be treated as a faithful oracle for target3's concurrent
depth-EQUAL/color-alias draw. The logical layout remains useful; actual fragment
tests, writeback visibility and framebuffer history need additional evidence.
[Pinned OpenGL owner](https://github.com/xemu-project/xemu/blob/fc24584ce88f0915ad7f04775bb7712c2e3f49ee/hw/xbox/nv2a/pgraph/gl/surface.c#L1117-L1130),
[pinned Vulkan owner](https://github.com/xemu-project/xemu/blob/fc24584ce88f0915ad7f04775bb7712c2e3f49ee/hw/xbox/nv2a/pgraph/vk/surface.c#L1424-L1436),
[verified file/blob manifest](../analysis/multiplayer/game-variants/camouflage-depth-alias-20261011/report.json)

## Reproduction and remaining boundary

Run without a graphics context or replay lease:

```sh
.tools/python/bin/python tools/inspect_camouflage_depth_alias.py
.tools/python/bin/python tools/inspect_camouflage_depth_alias.py \
  --output /tmp/camouflage-depth-alias-reproduced
```

The report preserves actual surface headers, five whole format calls, seven
whole command-writer calls,27 byte-verified original instructions,10 complete
indexed static-owner listings, pinned xemu sources, capture input hashes and
the declared logical controls. No instruction is patched and no game return
is supplied. It does not submit generated packets to hardware. The whole
allocation/binding/render owners, authored biped coverage, exact blend
quantization and concurrent depth/color alias effect remain outside this
executed corpus. Normal King/camouflage admission remains gated.
