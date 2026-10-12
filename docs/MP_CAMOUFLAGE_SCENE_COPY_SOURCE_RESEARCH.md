# Original camouflage scene copy over captured Xbox state

The unchanged retail `352E0(NONE,1) → 34A90 → 34420` chain completes over the
owned saved Lockout weather-entry RAM. It uses the actual captured current
target, camera, destination target19, program/texture headers, device banks and
command capacity. No readiness, target allocation, instruction or function
return is supplied. The whole call emits 1,672 command bytes, including four
vertices with four distinct source sampling coordinates, and reaches its real
`RET 8`. This establishes CPU preparation and command emission. No GPU consumes
the generated commands in this oracle.
[Source-only oracle](../tools/oracle_camouflage_scene_copy.py),
[raw inputs, outputs, resources, events, writes and executed bytes](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/report.json)

The input is the same immutable original capture used by the separate pass
service research: normal menu/Lockout weather entry at `391F0`, RAM SHA-256
`f76d0569dedcd2e94d3f2ee4c67a97db47cefea848105a8b24fe08342daf24cd`.
The actual CPU page tables resolve captured virtual memory. The retail XBE is
pinned to `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
Only a fresh external call stack `(NONE,1)`, return sentinel and FP-control
setup are declared. Actual current target and resource fields are retained.
This is a fresh original call over an authentic environment, not a historical
capture of camouflage copying.
[Pinned capture and declarations](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/report.json),
[previous immutable pass-service evidence](MP_CAMOUFLAGE_PASS_SERVICES_SOURCE_RESEARCH.md)

The captured current target is 0, the scale word `4B8494` is 1, and both the
camera rectangle at `4B9DD0` and raster rectangle at `485648` contain signed
shorts `[0,0,480,640]` (top, left, bottom, right). The original scale test takes
its already-unit branch. `352E0` resolves destinationNONE to19 and binds the
current source target through whole `14F60` into stages 0..3. All four select
the same actual texture header `7FF20000`, dimensions 640×480, and physical
backing `03DB0000`. Captured source bytes have SHA-256
`033210b0ce56e025654316cd1db0c44f84e276b1e346ba1c9089637f07bfa741`.
The before/after report retains frame-counter words `[133138,0]`; the original
source-target branch performs its own frame-index selection.
[Whole copy owner](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/scene_copy-000352e0.asm),
[actual source texture selection](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/target_texture-00014f60.asm),
[captured headers, dimensions and backing hashes](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/report.json)

The actual target19 record `4B56A0` is ready: its backing/readiness word is
nonzero and unavailable byte is 0. It supplies dimensions 320×240, texture
header `4B56A4`, color-surface descriptor `4B56B8`, and physical backing
`02B48000`. Its captured 307,200 bytes hash to
`974fbc5b61d007e8b7f281075a61b81a226d8f9c147db20e17f2124b9bb62806`.
Whole `14BC0(19,0)` supplies that actual color surface and NULL depth to
`1CD30`. The two reached `3F8100` calls first receive SDK intermediate viewport
`[0,0,INT_MAX,INT_MAX,0,1]`, then the copy viewport `[0,0,320,240,0,1]`.
Both are preserved in order; the first is not replaced with an invented final
viewport. The global current-target identifier remains 0 after the call, while
the actual device surface/viewport state changes through the reached owners.
[Target selection and viewport sequence](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/target_select-00014bc0.asm),
[raw surface and viewport events](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/report.json)

`352E0` installs the actual four-stage pixel program and calls
`34A90(19,10,01010101,0,0,1,.5,1,4,1,1)`. The report preserves its twelve-byte
device program header and all sixty pixel-program DWORDs at the genuine
`3FA3D0` entry. Whole original SDK publication completes. The copy also selects
actual datum `E2980123`, `rasterizer\vertex_shaders_xbox\screen`, index 0 through
`1C590 → 1C2E0`. The actual compiled program at `801A194C` is 116 bytes, with
seven vertex instructions. Its original header/count and full bytes are
preserved, and whole `3F6C60` consumes them. Later `3F7440` receives NULL with
zero patch count while preparing the layout; this is recorded as the actual
input, rather than presented as a second successful non-NULL program lookup.
[Pixel program producer](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/scene_copy-000352e0.asm),
[original vertex-program lookup](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/vertex_program_lookup-0001c2e0.asm),
[original vertex-program upload](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/vertex_program_select-003f6c60.asm),
[actual selected programs and SDK events](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/report.json)

The final two true flags in the `34A90` call have distinct jobs. Its destination
rectangle is the full target `[0,320,0,240]` in `[xmin,xmax,ymin,ymax]` order;
its source origins/spans come from the camera rectangle. The actual 104-byte
context passed to `34420` is recorded as raw bytes:

| Context offset | Actual value / role |
| --- | --- |
| `+0` |1, depth argument consumed by `350E0`|
| `+4` |.5, tap amplitude|
| `+8..+24` |Four source-origin XY pairs, all0|
| `+28..+44` |Four source/destination span ratios, all `(2,2)`|
| `+48..+54` |Four zero radial weights|
| `+58 / +5C` |Zero radial bounds|
| `+60` |1, final position XY scale|
| `+64` |4, active source coordinate attributes|

Whole `34420` invokes unchanged `34100` for attributes 15 down to 0 at each of
four corners. The original callbacks return true for attributes 0..5 and false
for 6..15. The complete report records all 64 calls, their before/after output
words and actual returnedAL. Successful attributes are emitted in order 5..0
at each corner. The corner order is `(0,0),(0,1),(1,1),(1,0)`.
[Rectangle/context construction](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/copy_quad-00034a90.asm),
[quad and command producer](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/quad_builder-00034420.asm),
[whole attribute owner](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/quad_attribute-00034100.asm)

| Attribute | Actual XY at the first corner | Actual XY at `(1,1)` |
| --- | --- | --- |
|5|`(0,0)`|`(1,1)`|
|4|`(.5,.5)`|`(640.5,480.5)`|
|3|`(-.5,.5)`|`(639.5,480.5)`|
|2|`(.5,-.5)`|`(640.5,479.5)`|
|1|`(-.5,-.5)`|`(639.5,479.5)`|
|0 position|`(0,0)`|`(320,240)`|

The four position vertices are `(0,0),(0,240),(320,240),(320,0)`.
Their actual Z/W words are `4B7C00FB / 467FFFFF`, produced by whole `350E0`
with captured near/far values 1/64 and 1024 and the original 16777215 constant.
All four tap attributes retain Z0/W1. The report stores these words at the
callback returns and again in the emitted method packets; it does not replace
the original position/depth projection with host clip-space assumptions.
[Original position/depth owner](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/position_attribute-000350e0.asm),
[raw callback outputs and emitted packets](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/report.json)

The tap contract is quarter **destination** pixel transformed through the
source/destination ratio. `34100` multiplies the `.5` context amplitude by
signed `.5`, adds it to the destination-coordinate interpolation, multiplies
by the context ratio, then adds the source origin. Here the ratio is 2, so the
four tap offsets are half a source pixel in each axis. Normalized against the
source 640×480 dimensions, that is `±.5/640, ±.5/480`, equivalently
`±.25/320, ±.25/240`. The report proves the source-pixel coordinates directly;
this normalized projection does not establish Vulkan sampling or raster-center
equivalence. Offset/split camera rectangles are a separate next corpus.
[Exact SSE tap operations](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/quad_attribute-00034100.asm),
[actual rectangle ratios](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/report.json)

The inspected native `water_copy_program()` matches all 60 actual original
pixel-program words. Its documented `water_copy_taps(source_width,source_height)`
instead computes `±.25/source_width, ±.25/source_height`. With the observed
source 640×480, those normalized offsets are half the original target19 offsets.
The header's description as quarter *source* pixel therefore does not describe
this complete original call. The water shader fixes its destination divisor to
512×256, and the inspected renderer computes taps from the host source extent.
It also supplies a normalized source rectangle, but has no generic destination
rectangle or the original camera-span/destination ratio. The literal program
is reusable; the existing water-specific tap/destination interface does not
by itself cover target19 or the complete `34A90` camera contract. This is a
static native-source comparison, not a compiled-helper or water-framebuffer test.
[Captured native source and exact comparison](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/native-water-comparison.json),
[comparison inspector](../tools/inspect_camouflage_scene_copy_native.py),
[inspected native functions](../src/water.cpp),
[inspected shader](../shaders/water_copy.frag)

That comparison preserves the pre-fix native source in its JSON. The later
[destination corpus](MP_SCENE_COPY_DESTINATIONS_SOURCE_RESEARCH.md) executes
the genuine512×256 water targets, and the verified
[native destination/water binding](MP_SCENE_COPY_DESTINATIONS_NATIVE_BINDING.md)
corrects normalized tap spacing. The historical inspector/report describes
the inspected pre-fix source; it is not a claim about the current helper.

The actual device is `404FE0`; its command pointer starts at `83F9EC64` with
limit `83FA0FB0`, leaving 9,036 bytes. The original call emits 1,672 bytes within
that capacity, decoded as 119 complete method packets, including begin value 7
and end value 0 at method `17FC`. The full emitted byte span hashes to
`287464d4e06e091e1e7cee9c5e2e99bb13a1f8d227488a9c10b61b420e0142ad`.
All 2,581 distinct reached instructions match the original image. All 83
instrumented service/callback calls complete. Capacity waits, resource
readiness and platform success are never replaced; this input does not reach
an unresolved service boundary.
[Raw commands, ordered packet decoder, writes and original instruction bytes](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/report.json),
[fresh byte-identical reproduction](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/reproduction.json)

Captured backing hashes identify the input surfaces and do not show the newly
copied frame. The oracle executes no NV2A commands, framebuffer reads after GPU
completion, transparent-group schedule, layer18/19 draw or depth/color alias.
Only the captured retail source/target and full camera rectangle are executed;
TU scene-copy services and other viewport contexts remain separate. Existing
fog, pass/state/native evidence is untouched and no gamemode admission changes.
[Execution boundary and declarations](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/report.json),
[separate alias limitation](MP_CAMOUFLAGE_DEPTH_ALIAS_SOURCE_RESEARCH.md)
