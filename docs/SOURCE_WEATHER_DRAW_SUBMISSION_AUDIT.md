# Original weather draw submission audit

The authority is the unchanged supplied `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`,
its authored weather programs and the immutable private Lockout RAM exports.
This is a CPU/source audit. It introduces no weather renderer, guest mutation,
replacement residency result, camera, clock, random stream or GPU capture.

The reproducible bank/disassembly extractor is
`tools/audit_source_weather_draw_snapshot.py`. Its reports are
`analysis/multiplayer-shading/weather-draw-lockout-state1.json` and
`weather-draw-lockout-update-entry.json`, with exact raw input files alongside
them. Actual draw entry/return evidence is frozen in
`analysis/multiplayer-shading/weather-particle-packet/packet.json` and its
`manifest.json`, using `tools/freeze_source_weather_particle_packet.py`.
Particle/resource and complete indoor-entry execution are retained by
`tools/audit_source_weather_particle_submission.py` in
`weather-particle-draw-inspection.json` and
`weather-particle-update-entry-inspection.json`. Projection inspection is in
`weather-12fa0-producer-inspection.md/.json` and its literal disassembly.
These reports distinguish static instruction inspection from unchanged
execution. They do not replace the existing bounded arithmetic fixtures in
`WEATHER_RENDERING_PARITY.md`.

## Caller and real admission

The sole ordinary caller found for `36B50` is `2BCD0`. It calls weather after
`C3E90` and before the final `44370` submission flush, `1595D0`, `42B20` and
first-person `3FDF0`/`404B0` intervals. The preceding world/fog/shadow passes
therefore already establish the scene color and depth targets. Weather is not a
post-HUD compositor.

`132D0:132D7..132E3` copies the incoming 0x298-byte frame bank to `485600`;
view slot, camera at frame+18 and projection at frame+8C are published together.
`36B50` consumes that actual signed-short slot and the current cluster/list
produced by the clipped-visibility path. An authored weather palette does not
by itself cause drawing. The existing source admission helper requires the
actual clipped cluster list and containing cluster; a PVS upper bound is not
an interchangeable input.

On view 0, `36B50` derives elapsed from the admitted double render-time bank,
updates `509418/50941C` and calls `398D0`. For each admitted palette, the order is
`4B3D0` wind on view 0, all `37260` plate updates, all `38450` particle updates,
all `37A60` plate draws, then all `391F0` particle draws. It uses separate
short-circuit boolean chains for simulation and drawing; failure in the former
does not suppress the latter by a shared boolean. The exact frame, current
palette and resource ownership remain caller inputs.

Two actual Lockout phases make the boundary concrete:

| Immutable export | Current slot / cluster | Clipped list | Scope |
| --- | --- | --- | --- |
| `lockout-state1`, RAM SHA `e7aedaa7…b802` | -1 / 15 | 15, 9, 2 | Post-view retained data; not a weather draw entry |
| `lockout-weather-update-entry`, RAM SHA `29e490d1…16cb` | 0 / 13 | 13, 15 | Hardware-breakpoint entry at original `36B50`, unpatched instructions |
| `lockout-weather-particle-entry`, RAM SHA `f76d0569…24cd` | 0 / 13 | 13, 15, 9 | Hardware-breakpoint entry at original `391F0`; weather cluster 9 is present |
| `lockout-weather-particle-return`, RAM SHA `952562fe…7d9` | 0 / 13 | 13, 15, 9 | Actual complete call's return at `36F00`; command staging, not final presentation |

Palette 0 is authored on clusters 0, 2, 7, 8 and 9. The actual indoor entry
thus admits no palette. Complete unchanged `36B50` and actual `398D0` return
with 144 verified title instructions and 14 restored pages: all sixteen
admission flags are zero, all distances are 32767, no wind/simulation/draw call
occurs and the auxiliary stream is unchanged. This explains the absent
`391F0` breakpoint without inventing a weather visibility override.

Both snapshots contain the real root `E77D0608` at `8026E87C`, one initialized
plate and a 1,024-particle resource group. The first snapshot has 637 positive,
189 zero and 198 negative particle ages. Its render/state arrays differ from
the authored pre-`39A80` resource payloads; those assets are not current
simulation output. Complete real `12DE70` mode 3 over each actual descriptor
returns AL=1 with 19 unchanged instructions, no external service and no
manufactured readiness. The fresh entry replay refreshes the physical-row
timestamp to the actual manager clock. This is a fresh diagnostic admission,
not a claim about its historical load/completion schedule.

The later actual entry is source-ready and emits 238 points from its initialized
1,024 rows. `tools/audit_source_weather_particle_entry_original.py` executes
the complete unchanged `391F0`, including actual title and SDK callees, on an
owned clone with captured GPR/XMM/flags/empty x87 state and stack argument.
Only the incoming return address is replaced with a diagnostic stop. It verifies
1,728 distinct instructions, executes 34,021 steps, reaches no external or
uncaptured service boundary and restores/checks all 67 mapped pages. All 8,872
distinct non-stack source-written bytes, all returned GPR/control registers and
all XMM registers match the actual `36F00` export. Thirteen surrounding bytes
outside that written set differ and are retained explicitly; this is not a
claim that complete RAM histories are equal.

The appended command span `83F9EC64..83FA0A64` is 7,680 bytes / 1,920 words
in 523 packets and is byte-for-byte equal to the actual return. Its 238 logical
color rows and 238 v0 point rows match the actual initialized render/state
arrays and strict age admission. The original SDK's red/blue byte conversion
is retained separately from logical colors. The source renderer/state/pixel/
camera/binding and point-parameter commands are now real same-call evidence;
their GPU consumption, sprite coverage and final presentation remain separate.

## Particle path: `391F0`

The input is the actual 0x8C-byte system record. `12DE70` receives
ECX=system+40 and EAX=3 before any draw. Its not-ready branch returns AL=1
without drawing. Ready input uses published system+68: group+0 count,
group+4 20-byte render rows and group+C 32-byte state rows.

The routine chooses lines only for the literal system+6C WORD value 2. It
selects built-in VS22 class 0, calls `12FA0` with EAX=`48568C`,
ECX=`485618`, scaling byte zero and actual render mode `4858B8`, builds
the complete `39780` pixel program, then binds textures through `1CF80`.
For non-rain it requests the authored system+4 sprite at stage 3 through
`143C0`, with bitmap record index ECX=0 and coverage XMM2=0. Rain does not
issue that bitmap request. Current resident texture ownership is supplied by
the real `137550`/`12310`/bitmap service chain, not by a replacement all-ready
policy.

Source view count is signed player+8, clamped to 1..4. Each view receives
`count / views` consecutive rows; the remainder is discarded. Only strict
ordered age>0 is emitted. The existing submission helper proves these input
positions/colors: rain endpoints use the authored streak scale and elapsed;
points carry size*16777215 in v0.w. This w is point diameter input, not a
replacement homogeneous position.

VS22 computes four DPH projection components, viewport multiplication,
RCP for `oPts.x` and RCC for position. `oPts` is output register 6.
`oPts.x=(size*16777215)/projectedW`; it has no additional authored viewport
width multiplier. The dedicated graphics path must explicitly publish this
output as point diameter, preserve source W depth, and convert the original
pixel coordinates to the actual Vulkan target. The ordinary world shader's
triangle path does not expose it.

The rain state uses the authored line width. The actual SDK `3F5730` multiplies
width by device+964 and 8 in x87, adds 0.5, converts through `3FCF00`, and caps
the unsigned result at 0x1FF. The device scale is a projection/viewport producer
input; it cannot be replaced with an unexplained modern line width. Point
sprite/scale is enabled only on the non-rain path. The source raster state is
SRC_ALPHA / INV_SRC_ALPHA, alpha GEQUAL 0 (method 033C=0x206, reference
0340=0), depth LEQUAL, W-buffer enable 2 and
no depth writes. The method values and named cleanup are retained in the
literal `391F0` disassembly.

That cleanup writes named defaults; it does not recover arbitrary previous
values. In particular, it disables point sprite/scale, alpha/blend and the
source pass flag, while line width and W-buffer state are not restored through
their setters. `3FF920`'s enabled-scale path also consumes retained size/min/max
state at `407668/66C/684` and the actual device raster height at +EEC, and emits
the source eight-word point-parameter packet. Those states and NV2A clamping
remain part of point raster admission; authored oPts arithmetic alone does not
prove their final hardware effect.

The immediate packed-color SDK routine `3FB3E0` swaps the red/blue byte lanes
before its literal `41940+attribute*4` packet. A native shader receiving decoded
logical colors need not repeat that swap, but its input convention must match
the actual NV2A register value. The older immediate fixture hooks this SDK
boundary and therefore proves source arithmetic, not the final pushbuffer
packet or point/line raster coverage.

## Plate path: `37A60` and `3A5C0` mode 3

`37A60` receives the 0x3A8-byte plate and indexes the actual 0xB8-byte view
state using the signed slot. It uploads c21/c22/c23.xy as each retained scroll
pair at view+11C/120, +124/128 and +12C/130 multiplied by the corresponding
authored scale at plate+48/4C/50. It supplies the retained metric at
view+134/138/13C with both signs to each layer's quad UVs. The rectangle uses
the actual signed-short source edges and subtracts 0.5. No triangle-mean UV,
manual scroll phase or normalized 0..1 fullscreen rectangle is substituted.

The retained layer opacity is multiplied, in source SSE order, by
view+14C*view+148. The three clamped alpha values and clamped authored first
tint feed a 52-byte mode-3 compositor input, along with actual plate+80/+84
depth-blend limits. `3A5C0` divides those limits by source far distance,
quantizes them via floor at the literal source precision, enforces its
mode-3 span bounds and builds all sixty combiner/texture-program words.
Its optional returned depth is clamped to the actual source near value.
`37A60` uses it to compute both projected depth and W depth with the source
near/far globals. This is a depth compositor, not an ordinary alpha texture.

The exact stage-0 source is especially important. At `37F14`, EDI=3 and
EBX=0 enter `14F60`. Literal table byte `15154+3` is 2; table entry
`15114+2*4` is `15020`, which reads the depth-surface pointer at `509368`.
`1DD60` supplies its target-3 dimensions; its literal fallback is 640x480
when the optional target-3 row is absent. That fallback is source code, not
an instruction to synthesize an image. The subsequent loop binds the three
authored bitmap references at stages 1, 2 and 3. Stage 0 uses pixel-coordinate
UVs and clamp addressing; the three plate images use wrap addressing and
the literal linear filters. The final sampling image must retain the source
depth texture's actual alias/packing, not an unproved grayscale/color view.

The alias constructor is recovered directly at `14600`: it allocates a
20-byte texture header, writes Common=`40001`, Data=the original depth
surface's Data field, Lock=0, Format=`11229` and Size=`271DF27F`. Thus the
original depth storage is exposed as the source linear 32-bit ARGB texture
format (literal bitmap linear-format-11 table maps hardware format `12`).
The real Lockout original depth header uses hardware format `2E`, and its
alias uses `12`; both Data words are `03B58000` and both Size words are
`271DF27F`. This proves the byte-sharing source interface. It does not prove
that a float depth sampler is interchangeable with that alias, or supply the
NV2A D24 quantization/filter/raster result for a modern depth attachment.

Source plate raster state is ONE / INV_SRC_ALPHA, alpha GREATER 0,
RGB-only `00010101` write mask, no depth writes, stencil/cull disabled,
depth comparison LEQUAL, and W-buffer enabled only when returned depth is
greater than near. It chooses built-in VS21 class 0 and emits a four-vertex
quad-strip through immediate v0..v4. VS21 passes v0 to oPos and v1 to oT0,
then adds c21..23.xy to v2..4 for oT1..3. Its authored class-0 stream-semantic
list is empty because this path supplies immediate registers, not a model
stream declaration. VS22 likewise uses its explicit immediate v1 color.
The ordinary model typed-declaration loader is not this immediate interface.

## Actual sprite binding and point packet

The real sprite is `E77E0609`, `scenarios\\bitmaps\\solo\\sentinelhq\\snow_flake`,
record 0 at `8026EDCC`. Complete unchanged `143C0 ->137550 ->12310 ->12CCF0`
executes 152 instructions on 15 restored pages with the literal `391F0`
inputs and no replacement readiness/device result. Partition 0 handle
`E9440045` is actually ready and supplies header `7FE50B28`. The source
changes stage 3 from the preceding plate image `7FE50B00` to that sprite,
sets dimensions to 32x32, refreshes its deadline 5970 ->9565 and updates the
physical-row use time to the actual manager clock. All those writes match the
later complete return. `1CF80` and the SDK subsequently change the header's
Common word `00040001 ->000C0001`; the other four header words are unchanged.

The actual header is `{000C0001,00C43000,0,05540F29,0}`: 32x32 BC3 with
four admitted levels 32/16/8/4. All 1,408 captured physical resource bytes equal
the authored `lockout.map` partition at `0149F000`, SHA-256
`6f5c36ca1c5f8a8ab02b1472e546d1070ef8b069e58122beba4ead51bc5a1133`.
This comparison does not make a new GPU readback or supply native residency
history. A generic authored chain including smaller extra levels is not this
explicit resident-header contract. The independent binding report/tool are
`weather-sprite-particle-entry.json` and
`tools/audit_source_weather_sprite_binding.py`.

The actual `3FF920` packet at method `200A30` is
`{0,0,3C91A2B4,42800000,42800000,42800000,80000000,0}`. Point modes are
1/1 and fixed POINTSIZE method `4043C` contains 8. The fixed-size arithmetic
uses retained point size 1, rather than minimum 0: the x87 original-size value
survives the intervening ratio/parameter stores. This corrects the earlier
inspection's minimum attribution. It does not determine whether NV2A's
programmable oPts or fixed POINTSIZE controls final size in this shader mode.
The real selected VS22 class 0 is `E7E4065F`, 148 bytes, SHA-256
`3f9e4e6030626339db80d3bb5bf9e4103936041f9bdcfd75626f18ca2ff1a1ee`.
Its runtime payload equals the original cache payload. The frozen packet holds
that program, all 48 camera words, all 60 pixel words, both raw input arrays,
both logical/SDK color conventions and the complete original command span.

## Minimal native integration seam and open inputs

A dedicated weather pass can use the existing literal VS21/VS22 translator and
combiner evaluator with explicit immediate attributes. It must carry separate
point-list, line-list and quad-strip topology, distinct particle/plate raster
state and per-draw complete pixel programs. The parent renderer can supply the
already-established source scene color/depth continuation; plates additionally
need the actual sampled depth alias and its sampler/coordinate contract.
State must be per view and per draw so a second view does not overwrite queued
constants, texture bindings, point diameter or plate scrolls.

The minimum explicit point packet interface is an ordered `SourceWeatherVertex`
list, the actual VS22 class-0 bytes, a full c0..11 bank, a complete `PixelProgram`,
an admitted resident sprite/header/mip chain, stage sampler state, the eight
source point parameters, fixed-size/mode words and W-depth/raster state. A
producer supplies actual admitted view/visibility, readiness, current initialized
arrays, elapsed/view count and SDK state. It does not default those inputs from
map names or substitute initial assets for current rows. The current JSON/binary
packet is a captured diagnostic contract; no native live packet builder or
weather rendering pass is activated here.

The scene service still has to provide a real admitted frame bank, clipped
visibility, resource publication/ready state, original cold initialization,
wind/elapsed/intensity/auxiliary state and complete `38450` particle updates.
Current sprite/plate bitmap residency is also an input. Existing bitmap request,
queue, read, pending-header and CPU-image-selection proofs do not alone create
that GPU residency timeline. No weather draw is activated by this audit.
The isolated native point prototype is now described in
[SOURCE_WEATHER_POINT_GPU_PROTOTYPE.md](SOURCE_WEATHER_POINT_GPU_PROTOTYPE.md).
It consumes the actual238-point packet and original sprite/programs over an
explicit controlled background/depth target at640×480, odd641×481,1080p and4K.
Its952-case arithmetic, raster-response controls and normal/ASAN validation do
not activate runtime weather. Inherited sampler/RGB/stencil/depth cache inputs
are separately pinned in `weather-point-inherited-control-inspection.json`.

Actual retained weather raster captures are still needed to compare original
point-sprite orientation/coverage and FBO presentation, line coverage, depth
alias sampling and final scene appearance. The prior448-case analytic program
test and its historical GPU result remain labeled as such; neither those nor
the isolated controlled target establish physicalXbox appearance parity.
