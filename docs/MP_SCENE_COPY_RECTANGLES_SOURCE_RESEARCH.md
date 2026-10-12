# Original scene-copy camera and viewport contract

The unchanged retail `352E0(NONE,1) → 34A90 → 34420` chain returns in all 34
bounded rectangle contexts. It keeps the genuine captured source 640×480,
destination target19 320×240, resource descriptors/readiness, programs, device
state and command capacity. One case retains the actual captured rectangles;
33 explicitly declared caller-input cases change only the world camera and/or
raster camera's eight rectangle bytes. No instruction, positive readiness or
function return is substituted. This proves original CPU preparation and
command emission, without an NV2A/framebuffer result.
[Oracle](../tools/oracle_scene_copy_rectangles.py),
[raw declarations, execution and output](../analysis/multiplayer/game-variants/scene-copy-rectangles-20261011/report.json),
[comparison fixture](../tests/fixtures/scene_copy_geometry_original.json)

This is a separate extension of the immutable captured-call proof. It uses
that report's pinned XBE, original saved RAM/page tables, actual current target0,
target19 and headers. The original capture remains a weather-entry environment;
these fresh calls are not a historical capture of camouflage drawing. Initial
FPCW `023F`, MXCSR `1FA0`, near `3C800000` (1/64), and far `44800000` (1024)
are the same declared/retained inputs as that baseline. No other depth range,
rounding mode, target dimension or copy flag combination is covered here.
[Immutable captured-call proof](MP_CAMOUFLAGE_SCENE_COPY_SOURCE_RESEARCH.md),
[pinned prior report](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/report.json)

The fixture has root `inputs` and aligned `versions.retail.results`. Each input
records its provenance and camera/raster signed shorts in `(top,left,bottom,right)`
order. Each completed result preserves four `vertices`, each with `corner_bits`
and 16 attributes `{index,returned,value_bits[4]}`. It also preserves the raw
destination/context DWORDs, all 60 pixel-program words, source/destination
dimensions, near/far, both SDK viewport inputs/effective outputs, and all three
whole `3F6060` input/output records. False attributes retain their actual output
buffer contents and must not be treated as emitted vertices.
[Fixture schema and whole-call projection](../tools/oracle_scene_copy_rectangles.py),
[raw before/after buffers, returnedAL and command packets](../analysis/multiplayer/game-variants/scene-copy-rectangles-20261011/report.json)

The input inventory is captured full `[0,0,480,640]`, then eleven rectangles
each applied camera-only, raster-only and to both: top/bottom halves, left/right
halves, all four quarters, inset `[20,40,300,440]`, odd-sized inset
`[13,17,254,338]` (321×241), and one pixel `[479,639,480,640]`. All are in-bounds
positive rectangles. The half/quarter names describe manufactured input shapes;
this corpus does not execute the live split-screen rectangle producer. The
report records each changed address and its captured/declared raw bytes.
[Explicit inventory and writes](../analysis/multiplayer/game-variants/scene-copy-rectangles-20261011/report.json)

## Rectangle ownership

The two fields have distinct owners. Retail `2BA10` copies the EAX camera's
`1D` DWORDs into `4B9DA0` at `2BAB8..2BACB`; its outer rectangle at camera `+30`
is therefore `4B9DD0`. It separately copies the caller's second camera into
local raster descriptor `+18` at `2BAEB..2BB0F`, queues the `298`-byte descriptor,
and calls `132D0`. Alternative owner `2C790` copies its input descriptor `+C`
camera to `4B9DA0` and its `+80` camera to local raster descriptor `+18`.
These are static complete-owner listings, not whole executions in this corpus.
[World camera owner](../analysis/multiplayer/game-variants/scene-copy-rectangles-20261011/world_camera_copy-0002ba10.asm),
[alternative camera owner](../analysis/multiplayer/game-variants/scene-copy-rectangles-20261011/alternate_camera_copy-0002c790.asm)

`132D0` copies all `298` bytes of the incoming raster descriptor into `485600`.
Its camera `+18` becomes `485618`, and that camera's rectangle `+30` becomes
`485648`. It passes descriptor camera `+18` and projection `+8C` to `12FA0`,
selects the actual raster target and invokes `14BC0`. Thus editing the two
global fields declares two different upstream caller inputs. It does not
pretend to execute those upstream producers.
[Raster descriptor owner](../analysis/multiplayer/game-variants/scene-copy-rectangles-20261011/raster_descriptor_copy-000132d0.asm)

## Whole geometry and tap output

Mode1's fixed `34A90` flags request a full destination rectangle and the world
camera as source rectangle. Every case retains destination
`[xmin,xmax,ymin,ymax]=[0,320,0,240]` and corner order
`(0,0),(0,1),(1,1),(1,0)`. Raster-only cases are bit-identical to the captured
baseline's complete compact result; paired cases are bit-identical to their
camera-only counterparts. These assertions include the program, context,
attribute words and effective viewport/transform outputs, not just XY values.
[Original construction](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/copy_quad-00034a90.asm),
[executed comparisons](../tools/oracle_scene_copy_rectangles.py)

For source camera `(T,L,B,R)` and actual destination `(Dw,Dh)`, `34A90` computes
the four repeated source origins `(L,T)` and ratios using separate single
precision instructions:

```text
reciprocalX = DIVSS(1, float(Dw))
reciprocalY = DIVSS(1, float(Dh))
ratioX = MULSS(float(R-L), reciprocalX)
ratioY = MULSS(float(B-T), reciprocalY)
```

The odd rectangle demonstrates the rounded reciprocal/product distinction:
its ratio words are `3F806667 / 3F808889`, approximately
`1.0031250715 / 1.0041667223`. Replacing the operation pair with a single
span/dimension division need not preserve these words.
[Raw context bits](../tests/fixtures/scene_copy_geometry_original.json),
[original reciprocal/product instructions](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/copy_quad-00034a90.asm)

At each corner, whole `34100` returns true for attributes0..5 and false for6..15.
Attribute0 calls whole `350E0` for destination interpolation/depth then applies
context final XY scale1. Position XY is full-target geometry in every case;
Z/W stays `4B7C00FB / 467FFFFF` for the retained near/far and depth argument1.
Attribute5 copies cornerXY, retaining initial Z0/W1. For attributes1..4, let
`k=attribute-1`, `sx=-.5` when `(k&1)==0` else `+.5`, and `sy=-.5` when
`(k&~1)==0` else `+.5`. Zero radial weights select the original fast path:

```text
baseX = ADDSS(MULSS(SUBSS(xmax,xmin), cornerX), xmin)
baseY = ADDSS(MULSS(SUBSS(ymax,ymin), cornerY), ymin)
tapX = ADDSS(originX, MULSS(ADDSS(MULSS(.5,sx),baseX),ratioX))
tapY = ADDSS(originY, MULSS(ADDSS(MULSS(.5,sy),baseY),ratioY))
```

Each instruction rounds separately; the final ADDSS has the source origin as
the destination operand. Tap Z/W is0/1. All 816 successful attribute returns
are raw original outputs, with all 2,176 callback results retained in the
fixture/report. Radial weighting and other copy-stage counts are not admitted
by these cases.
[Whole attribute owner](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/quad_attribute-00034100.asm),
[whole position/depth owner](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/position_attribute-000350e0.asm),
[raw original outputs](../tests/fixtures/scene_copy_geometry_original.json)

The four stages still bind the genuine source header with dimensions640×480.
TapXY is in source pixels, not normalized against camera span or destination.
For example top-half input has first-corner taps `±.5,±.25`; bottom-half shifts
Y to `240±.25`; the offset inset starts around `(40,20)` with offsets
`±.3125,±.2916667`. Any host normalized-coordinate projection must use the
actual source header dimensions640×480. The callback itself does no such
division. This identifies the CPU interface required for source/destination
geometry; it does not establish host interpolation/filter/sample parity.
[Original stage binding/dimensions and contexts](../analysis/multiplayer/game-variants/scene-copy-rectangles-20261011/report.json)

## SDK viewport and retained program

Both whole `3F8100` calls finish with actual cached viewport
`[0,0,320,240,0,1]`, despite the first input's `INT_MAX` dimensions. Its unchanged
instructions clamp to the bound surface header before applying device
resolution scales and size limits. This is observed output, not a replacement
of the first intermediate viewport with an assumed final one.
[Whole viewport owner](../analysis/multiplayer/game-variants/scene-copy-rectangles-20261011/viewport-003f8100.asm),
[input and effective viewport records](../tests/fixtures/scene_copy_geometry_original.json)

Three whole `3F6060` calls per case retain scales1, flags`202`, depth scale
`4B7FFFFF`, and actual device `+EF8/+EFC=3F080000` (.53125). The `8000` flag
condition for subtracting .5 is false, although global `4076F8` is1. All calls
produce offset `[43208800,42F11000,0,0]` = `(160.53125,120.53125,0,0)` and scale
`[43200000,C2F00000,4B7FFFFF,0]` = `(160,-120,16777215,0)`. The raw input
fields and observed FPCW/MXCSR are saved for every complete call.
[Whole transform owner](../analysis/multiplayer/game-variants/scene-copy-rectangles-20261011/viewport_transform-003f6060.asm),
[whole publication owner](../analysis/multiplayer/game-variants/scene-copy-rectangles-20261011/viewport_publication-003f6190.asm),
[actual transform results](../tests/fixtures/scene_copy_geometry_original.json)

Static original SDK initialization at `3FBBB3..3FBBDE`, inside `3FBA30`, loads
literal `3F080000` and stores it to both device offsets. Thus the captured
.53125 has a concrete original writer. That initialization is not executed
here. The separate fixed-function program owner `3F6F40` reads those offsets
only on its actual program-flags bit2 path; its existence does not establish
that this copy's custom passthrough program adds the same bias.
[Original literal writer](../analysis/multiplayer/game-variants/scene-copy-rectangles-20261011/viewport_defaults-003fba30.asm),
[separate fixed-function consumer](../analysis/multiplayer/game-variants/scene-copy-rectangles-20261011/fixed_function_program-003f6f40.asm)

The retained authored screen program is the same actual 116-byte, seven-MOV
program as the captured baseline. Static token decoding gives
`v0→oPos`, `v1..v4→oT0..oT3`, `v5→oD0`, `v6→oD1`, full XYZW masks, with final
bit only on the last instruction. The quad supplies attributes0..5; its false
attribute6 callback does not prove that NV2A's retained v6 is zero. This is
program-byte decoding, not original GPU execution.
[Actual program bytes](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/report.json),
[decoded literal fields](../analysis/multiplayer/game-variants/scene-copy-rectangles-20261011/report.json)

The separately pinned xemu backend converts programmable output screenXY with
`(2*xy-surfaceSize)/surfaceSize`, then multiplies XYZ byW; its conversion contains
no extra half-pixel addition. Its `roundScreenCoords` truncates XY to1/16.
These are emulator implementation facts, not independent NV2A raster-center
measurements and not permission to add/subtract an SDK offset in the native
copy shader. No framebuffer parity is inferred from them.
[Pinned xemu program conversion](../tools/reference/xemu-0.8.136/hw/xbox/nv2a/pgraph/glsl/vsh-prog.c),
[pinned xemu screen-coordinate conversion](../tools/reference/xemu-0.8.136/hw/xbox/nv2a/pgraph/glsl/vsh.c)

All 34 complete calls execute 2,581 distinct byte-checked original instructions,
preserving real command capacity and emitting1,672 bytes each. The fixture and
ten report/listing outputs reproduce byte-identically. The prior captured-call,
fog, state and vertex proofs remain immutable. TU correspondence for this full
scene-copy chain, original GPU command acceptance, pixel centers, framebuffer
copy/sampling and complete camouflage draw scheduling remain unproved; this
work changes no native implementation or gamemode admission.
[Fresh reproduction manifest](../analysis/multiplayer/game-variants/scene-copy-rectangles-20261011/reproduction.json)
