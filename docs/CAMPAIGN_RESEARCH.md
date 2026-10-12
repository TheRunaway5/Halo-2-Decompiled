# Original Xbox Cairo campaign services

Research date: 2026-10-07. This report describes the local retail build
`02.09.27.09809` and the opening of `01b_spacestation`, rather than assuming
that a camera-point viewer or independently spawned actors execute its campaign.
The first Cairo cinematic uses an animated `jmad` camera, named authored object
placements, concurrent script threads, actor/scenery animation, sound, and a
save/revert controller. Cairo contains **zero cutscene camera points**.

## Sources and evidence labels

All references are pinned primary repositories, their recovered source, or
direct reads of the user's local disc. Layout information and formulas are
reported here; no third-party implementation body was copied into the product.

* [Assembly original Xbox scenario layout](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2Xbox/scnr.xml)
  and [original Xbox scripting definitions](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Blamite/Formats/Halo2Xbox/H2X_Scripting.xml),
  revision `fd14ef3c46135e9422966c6a64cd926fb0bd23fb`, GPL-3.0.
* [Original Xbox recovery](https://github.com/kirklandsig/halo2-decompiled/tree/5f08630655e3ddc9ac45909090f11ae799fa0c27),
  revision `5f08630655e3ddc9ac45909090f11ae799fa0c27`, CC0.
  Its [function manifest](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/config/functions.csv)
  distinguishes `matched`, `near`, and `todo`. A `@retail` comment alone does
  **not** establish an instruction-matched implementation.
* [Reclaimer animation-layout source](https://github.com/Gravemind2401/Reclaimer/tree/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Blam/Halo2),
  revision `36224d7a4581f2a1f255099bf706aa56be9f6e21`, GPL-3.0.
  The exact decoded raw-animation layout is documented in `PLAYABLE_RESEARCH.md`.

`Retail verified` below means the indicated original XBE instructions or data
were independently inspected here; it does not imply the complete recovered
function compiles byte-for-byte. `Recovered` means source reconstruction, with
manifest status stated where material. The matching local executable is
`assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
All `scnr` offsets are hexadecimal offsets relative to its metadata; all file
offsets below are absolute offsets in `01b_spacestation.map` unless stated.
The scenario datum is `E1780003`, metadata file offset `0E5F43E0`.

## Authored named objects

The scenario's Object Names block is at `scnr+48`, record stride `24`:
ASCII name[32] at `+00`, signed i16 object type at `+20`, signed i16 placement
index at `+22`. The name index selects a record; it is not a live object datum.
The type selects a placement/palette block. Palette records have stride `28`,
with the tag reference at `+00` and its salted datum at `+04`.
These fields are present in the [Xbox scenario XML](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2Xbox/scnr.xml).

The following block mapping was also verified from **retail** XBE table
`00468630`: it contains pointers to object type definitions; signed i16 fields
`definition+A/C/E` give placement offset, palette offset, and placement stride.

| Type | Placement block | Palette block | Placement stride |
|---|---:|---:|---:|
| 0 biped | `60` | `68` | `54` |
| 1 vehicle | `70` | `78` | `54` |
| 2 weapon | `90` | `98` | `54` |
| 3 equipment | `80` | `88` | `38` |
| 6 scenery | `50` | `58` | `5C` |
| 7 machine | `A8` | `B0` | `48` |
| 8 control | `B8` | `C0` | `44` |
| 9 light fixture | `C8` | `D0` | `54` |
| 10 sound scenery | `D8` | `E0` | `50` |
| 11 crate | `328` | `330` | `4C` |
| 12 creature | `368` | `370` | `34` |

Types 4 garbage and 5 projectile have all three fields `-1`, with no authored
placement blocks. [Placement recovery](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_0b7300.cpp)
reads this same definition table.

All placement kinds share this prefix:

| Field | Offset/type |
|---|---|
| Palette index; name index | `00/02`, i16 |
| Placement flags | `04`, u32 |
| Position | `08`, float3 |
| Euler orientation | `14`, float3, **radians** |
| Scale | `20`, float |
| Transform flags; manual BSP mask | `24/26`, u16 |
| Authored unique ID | `28`, u32 |
| Origin BSP | `2C`, i16 |
| Object type; source; BSP policy | `2E/2F/30`, bytes |
| Editor folder | `32`, i16 |

Placement flag bits: 0 not automatic, 1/2/3 exclude Easy/Normal/Hard, 4/5 lock
type/transform to environment object, 6 never placed, 7 lock name, 8 create at
rest. Source: 0 structure, 1 editor, 2 dynamic, 3 legacy. Authored BSP policy:
0 default, 1 always placed, 2 manual mask. A named explicit creation bypasses
the automatic-placement bit, but does not make an invalid palette or a never
placed entry valid. `D5060` normalizes nonpositive scale to **1**. Cairo's
cinematic actor placements store scale **0**, so literal zero scaling would
erase every actor. Original instructions `D5156..D5177` independently verify
the zero comparison, fallback1, and output scale write. Original 512-byte
`D5060` SHA-256 is
`395f37c4895258d4815f9ab0842ba109014914ff31ff6dbd95111cd84ef1921d`.
This normalization is recovered in
[placement source](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_0b7300.cpp);
the raw zero values were independently validated. `D5060` itself remains
manifest `todo` despite this verified local behavior.

Scenery/biped/vehicle/weapon/crate permutation data: variant StringID `+34`,
active colors u32 `+38`, four packed colors `+3C/40/44/48`. Biped/vehicle
body vitality float is `+4C`, unit flags u32 `+50` (dead, closed, not enterable
by player). For the object tag itself, default variant StringID is `+30` and
the `hlmt` model reference's datum is `+38`. The actual `hlmt` variant mapping
and inheritance are documented in `PLAYABLE_RESEARCH.md`. Source:
[scenario XML](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2Xbox/scnr.xml),
[common object header](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2/mach.xml).

### Script object-name resolution and creation

Retail-matched `20A290` converts authored object-name types 56..61 to runtime
object types 50..55 by passing the signed low 16 bits through `BB760`. Name
table storage is `g_4DE2D0`, 640 runtime datum slots. `BB760` returns `NONE`
for indices outside `[0,639]`. Thus Cairo's literal `FFFF0151` is authored
name index 337, not a live entity or tag. Calls typed `object_name` retain the
name index; calls typed `unit` or `object` must resolve it. Source:
[HS cast recovery](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_209520.cpp),
[name lookup recovery](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_0bb760.cpp).

* `object_create`, opcode `02E`, matched evaluator `2A12F0`, calls
  `BB670(name,false)`. Original `BB670` is a recovery stub, but its **236
  retail bytes were inspected**: if already bound, false returns `NONE`
  without creating. Otherwise it looks up type/placement/palette through the
  definition table above and calls `BF0F0(placement,type,index,palette,1,0)`.
* `object_create_clone`, opcode `02F`, evaluator `2A1340`, calls
  `BB670(name,true)`. An existing named object is detached by `BF090`:
  it loses its name and scenario index, becomes source 2, and receives a
  dynamic unique ID; then a newly named object is created.
* `object_create_anew`, opcode `030`, evaluator `2A1390`, matched helper
  `29FD80`, destroys an existing unprotected named object before creation.
* `object_destroy`, opcode `034`, matched evaluator `2A14D0`, operates on the
  **resolved runtime datum** and skips `NONE` or `BEB30` protected objects;
  destruction service is `B8540`.

Sources: [script evaluators](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_29f5b0.cpp),
[upstream retail inspection of named creation](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/docs/unknown_0b8bd0.md),
and the local original `BB670` instruction inspection. Name bindings need
generation-aware live datums so destroy/recreate cannot retain an old actor.

### Cairo placement fixtures

| Name / authored index | Type / placement | Palette tag | Selected variant |
|---|---|---|---|
| `chief_x02_01` / 337 | biped / 37 | `FB66192E`, masterchief | empty authored SID |
| `johnson_x02_01` / 336 | biped / 38 | `EB580925`, marine | `0D00087C`, `johnson_dress` |
| `marine_door_01` / 351 | biped / 81 | `EB580925`, marine | `0900087A`, `dress_hat` |
| `anchor_x02_01` / 180 | scenery / 271 | `F85E1626`, cinematic_anchor | empty |
| `x02_bridge_door` / 474 | machine / 147 | `803A1E01`, ss_door_int_hab_large | not permutation data |
| `macgun` / 412 | machine / 0 | `80261DED`, ss_exterior_gun_hinge | not permutation data |
| `gun_loader` / 471 | machine / 8 | `807A1E41`, ss_gun_loader | not permutation data |
| `malta` / 10 | scenery / 10 | `F80715CF`, ss_prop | empty |
| `athens` / 11 | scenery / 11 | `F80715CF`, ss_prop | empty |

Chief placement file `0E607CE8`: position
`(61.1647453308,-13.3352966309,7.00026559830)`, Euler
`(1.57079637051,0,0)`, flags 1, manual mask 1, origin 0, source 1,
policy 2, stored scale 0. Johnson file `0E607D3C`: position
`(60.6613769531,-13.3665342331,7.00026559830)`, the same orientation,
flags/BSP policy/scale. Anchor file `0E5FED48`: position
`(54.1543350220,0,6.99999809265)`, Euler
`(0,-0,-0.000000953600022058)`, flags `30`, origin 1, source 0.
These are **local byte fixtures**; regular gameplay Marine default selection
does not substitute for this cinematic's `johnson_dress` placement.

## Cutscene cameras and player control

Cutscene Flags block `scnr+1E0`, stride `38`: unknown u32 `+00`,
ASCII name[32] `+04`, position float3 `+24`, facing yaw/pitch float2 `+30`.
Cutscene Camera Points block `scnr+1E8`, stride `40`: flags u16 `+00`,
type i16 `+02`, ASCII name[32] `+04`, unknown four bytes `+24`, position
float3 `+28`, Euler float3 `+34`. These orientations are radians.
The unknown `+24` field is **not established as FOV**; retail camera setup
reads orientation/position and the script FOV service separately. Flags bit0
means edit as relative; types are 0 normal, 1 ignore target orientation,
2 dolly, 3 ignore updates. A restricted point implementation should reject
unsupported target-relative/dolly behavior explicitly. Source:
[scenario XML](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2Xbox/scnr.xml),
[camera reconstruction](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_16bd10.cpp).

These services are distinct:

| Builtin | Opcode | Evaluator | Behavior |
|---|---:|---:|---|
| `camera_control(bool)` | `1C1` | `2A8970` | Select scripted or player/vehicle camera director through `155A30` |
| `player_enable_input(bool)` | `1DA` | `2A8EE0` | Sets player input-disabled flag `g_4E8C20+6` to inverse argument |
| `player_camera_control(bool)` | `1DE` | `2A8FC0` | Sets `g_4ED284.flags10` bit0 to inverse argument; returns supplied bool |
| `camera_set(point,short)` | `1C2` | `2A89B0` | Point service `16C2F0`, void |
| `camera_set_animation_relative(jmad,SID,unit,flag)` | `1C5` | `2A8AA0` | Animated service `16BF70`, **void**, not bool |
| `camera_time()` | `1CB` | `2A8BD0` | Returns signed short remaining script ticks |
| `camera_set_field_of_view(real,short)` | `1CC` | `2A8C20` | Degrees and script-tick transition, service `16C740` |

The signatures/evaluator addresses were verified directly from original
builtin table `004744E0` and are corroborated by the
[script definitions](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Blamite/Formats/Halo2Xbox/H2X_Scripting.xml)
and [evaluators](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_29f5b0.cpp).
Input evaluators and camera-time evaluator are manifest-matched. Director
`155A30` is reconstructed; its selector behavior is also present in original
instructions. `cinematic_start` does not select the scripted camera director.

### Animated camera timing and axes

The scripted camera state is `g_510C6C`, size `78`: mode i16 `+02`
(0 none, 1 point, 2 pan, 3 animation, 4 first person), FOV time/duration/start/
target float `+04/08/0C/10`, remaining simulation ticks i32 `+14`, position
`+18`, forward `+24`, up `+30`, optional synchronization unit datum `+3C`.
Animation state: start tick `+40`, graph datum `+44`, animation SID `+48`,
cutscene flag i16 `+4C`. Source:
[camera reconstruction](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_16bd10.cpp).

Retail-verified `16BF70` selects clip by SID (`1DD0B0`), enters mode3, sets
start tick, and initializes remaining time to
`round((frameCount-1)/30 * simulationTickRate)`. An absent graph/clip leaves
state unchanged. It does not return a script boolean. Its 309 original bytes
were inspected; the recovered C++ still has manifest status `todo`.

Retail-verified `16CFA0`, instructions `16D078..16D0B1`, computes elapsed
seconds `(game_time-start_tick)*simulation_dt`; when a valid synchronized
unit is supplied, its animation frame/30 is used instead (`16D069..16D075`).
It samples animation node0 through `1CBF50`. Anchor flag facing builds
forward `(cos(yaw)cos(pitch),sin(yaw)cos(pitch),sin(pitch))`, then an up vector
through `11D090` and a transform through `1420F0`. `16D0E6..16D145` composes
**flag transform × sampled animation transform**. Original `16C840` at
`16CDB2..16CE17` copies the resulting forward, up, and position directly to
the observer command, flags `|=19`, distance/timer 0, object datum `NONE`.
There is no extra camera-local axis conversion in this handoff. Original
473-byte `16CFA0` SHA-256:
`23b88f0ab992e8518f615538a21e3b2ba2bea0bbc9dd9394a48c3ba6d74b7ca1`.

Recovered `1CBEC0` passes `seconds*30` into `2798A0`. Its frame index is
clamped to `[0,frameCount-1]`, next frame to `min(current+1,last)`;
subframe interpolation snaps fractions below `.0001` or above `.9999`, and
holds the final frame. This is a non-looping camera sample, independent of
an actor idle loop. Source:
[animation-state sampling](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_1cafc0.cpp),
[sampler](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_279d80.cpp).
The sampler recovery is `todo`; integer clamp behavior is also visible in
the original bytes. Codec-specific quaternion interpolation is described in
`PLAYABLE_RESEARCH.md`.

Retail tick function `137FE0` decrements remaining ticks with saturation at
zero at `1380B4..1380C8`, then increments FOV elapsed time by simulation dt
at `1380CB..1380DB`. Its script execution call `20B660` occurs earlier at
`138032`; a port must define equivalent scheduling instead of decrementing
camera time multiple times per rendered frame. Matched `2A8BD0` returns
`short(round(remainingSimulationTicks * simulation_dt * 30))`.
FOV `16C740` converts script degrees to radians, saves the current value,
and linearly interpolates for `shortTicks/30` seconds.
[FOV source](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_16bc40.cpp)
is manifest-matched.

The reconstructed observer setup `171D90` treats this FOV as horizontal:
`vertical = 2*atan2(tan(horizontal/2)/aspect,1)`, then applies observer
view offsets separately. It is `todo` in the matching manifest, so this is
primary-source reconstruction rather than instruction-oracle proof. Source:
[observer projection setup](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_170d70.cpp).

### Cairo's first animated camera fixture

`jmad F3F411BC`, metadata file `0E88BA9C`, path
`objects\characters\cinematic_camera\x02\x02`; one node `camera01`, eleven
clips. Clip0 `x02_01`, SID `06001C72`, **516 frames**, no movement data.
Graph raw group0 pointer `09818C00` selects the current map, group size26104;
clip-relative offset0. Clip bytes10408: static32, animated10352, static/animated
flags12/12. SHA-256
`c24d144425c8dcd9948fc22cc969d2f95b5b9da0654a34220bae7c989c7ed569`.
The static codec1 has counts0/0/0; animated codec3 has counts1/1/0.
Animated translation offset `1040`, scale offset `2870`, rotation track stride
`1020`, translation stride `1830`, scale stride `810`; masks are static
all0, animated rotation/translation bit0, scale0. Layout sources:
[Xbox jmad XML](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2Xbox/jmad.xml)
and the primary sources cited in `PLAYABLE_RESEARCH.md`; these samples and
resource hash were independently decoded locally.

Frame0 signed quaternion `(-262,-2174,-4150,32429)`, normalized xyzw
`(-.00799591959,-.06634782134,-.12665292482,.98969342147)`, local position
`(6.24428415298,-11.7625789642,.489282310009)`.
Frame257 position `(5.71848392487,-14.8855047226,.86708337069)`;
frame515 position `(6.77743721008,-9.42803478241,.486002683640)`.
Cutscene flag17 `anchor_flag_x02_01`, file `0E65E4F0`, position
`(54.1543350220,0,6.99999809265)`, yaw/pitch `(0,-0)`.
Therefore first world camera position is
`(60.3986191750,-11.7625789642,7.48928040266)`, and initial `camera_time`
is **515** at a 30Hz simulation rate. A synthetic point path would not match
this authored camera.

## Cinematic state, fades, and letterbox

`cinematic_start` opcode `22A`, evaluator `2A9740`, calls matched `13BFF0`:
sets object-state flags `+80/+81`, disables input, suppresses the AI global
at `+20`, enables interface letterbox and cinematic flags, and sets cinematic
options. `cinematic_stop` opcode `22B`, evaluator `2A9760`, calls reconstructed
`13CA80` to reverse those flags and clear cinematic effects/FOV state.
Their separate `camera_control` calls remain necessary. Source:
[cinematic start/stop](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_13ca80.cpp)
and [evaluators](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_29f5b0.cpp).

Fade-in/out opcodes `228/229`, evaluators `2A9640/2A9690`, store RGB at
`g_4E8C28`, duration `round(shortScriptTicks/30 * simulationTickRate)`,
and start game tick. Their matched `154310` evaluation clamps elapsed/duration
to `[0,1]`, applies **curve5**, then returns fade-out amount `curve(t)` or
fade-in amount `1-curve(t)`. Duration0 uses `t=1`, so fade-out0 is fully
opaque and fade-in0 fully transparent. This is not a linear fade. Sources:
[matched fades](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_153950.cpp),
[curve source](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_17ca10.cpp).

The original curve5 table is **1024 byte data**, XBE VA `0046D148`, XBE file
offset `00468FC8`, SHA-256
`ed6ff8c88811bc9aa7bb610401d9d6005a35463d8be0bca9c68e117874c68ec9`.
Samples at indices `0,128,256,512,768,1023` are `0,9,37,127,217,255`.
Retail-verified `17CA10` computes `s=clamp(x,0,1)*1023`,
`f=fmod(s,1)`, index via x87 `fistp(s-float(.1))`; for index1023 it uses
`table[1023]/255`, otherwise it interpolates `table[i]/255` and
`table[i+1]/255` with fraction `f`, then clamps the result. Under the usual
nearest-even FPU control this is nearest-even rounding after subtraction of
`.10000000149011612`; it is not simply floor(s). The recovery is `near`, but
these constants/data flow were directly verified in original instructions
`17CA34..17CAE0`. Load the small table from the user's executable to preserve
its quantization without generating an approximate easing curve.

`cinematic_show_letterbox`, opcode `22E`, evaluator `2A97C0`, sets interface
flag4. The immediate variant `22F`, matched `2A9810`, also sets fraction0/1.
Retail `13C680` instructions `13C6BA..13C708` adjust the fraction by **±dt
per simulation tick**, clamped0..1: a full transition takes **one second**,
30 ticks at30Hz. Retail `13CCF0`, `13CD21..13CD33`, uses
`fraction*.125*viewportHeight` for **each** top/bottom bar, subject to its
display-mode suppression condition; final rectangle coordinates use x87
rounding. Sources:
[letterbox state and rectangles](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_13bf00.cpp).
Both recoveries are `todo`; the rate and `.125` constant were independently
verified. Recovered `13C780` also assigns black bar alpha from the fraction
quantized to a byte, so a transition both grows and fades its bars. This draw
routine is `todo`; final full-screen fade versus UI-bar compositing order was
not fully established here. Original view routine `2B790` calls fade evaluation
`154310` before `2BA10`, while UI routine `2C790` calls `13C780`; these are
different per-view branches in `2B5D0`, so the existence of both does not alone
prove their order in every output mode. Source:
[cinematic UI drawing](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_13cf00.cpp).
`hud_cinematic_fade` opcode `270`, evaluator `2AA590`, is a separate
HUD gain/seconds transition via `13B306`, not the RGB full-screen fade.

External global literal `FFFF83E3` is `cinematic_letterbox_style`, external
slot `03E3`, type **long8**. Retail external table `00473468+3E3*4` points
to definition `00473294`, bytes `0800000000000000`: its backing address is
**NULL**. Matched `20A490` writes backing memory only when nonnull. Therefore
the scene's `(set cinematic_letterbox_style 1)` has no retail graphics effect;
it can retain a typed VM global cell without claiming an unimplemented
letterbox style renderer. Sources:
[external global name/type](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Blamite/Formats/Halo2Xbox/H2X_Scripting.xml),
[external global access](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_209bc0.cpp),
and independently inspected retail definition data.

## Machines and device groups

Scenario device groups: block `A0`, stride `28`, ASCII32name at0, initial
float `20`, flags u32 `24` bit0 only changes once. Machines: block `A8`,
stride `48`, palette `B0`, common prefix above followed by power group i16
`34`, position group i16 `36`, device flags u32 `38`, machine flags u32
`3C`, pathfinding block `40`. Device flags bits0 initially open,1 initially
off,2 change once,3 reverse position,4 not usable from any side. Machine
flags bits0 not automatic,1 one-sided,2 never locked,3 melee opens,
4 one-sided for player,5 not automatically closed. Controls use block `B8`,
stride `44`, palette `C0`. Source:
[scenario XML](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2Xbox/scnr.xml).

**Recovered devices.cpp field names are reversed relative to the verified
script builtin names.** Treat runtime `+130/+134/+138` as power group/value/
velocity and `+13C/+140/+144` as position group/value/velocity. The source
calls the former "position" and the latter "power". Matched builtin bindings
and retail scripting definitions establish this mapping:

| Service | Opcode/evaluator | Matched behavior/service |
|---|---|---|
| `device_set_power` | `FE / 2A4730` | `107190`: reset motion, write first value and change its group target |
| `device_get_power` | `FF / 2A4780` | Reads first value |
| `device_set_position` | `100 / 2A47F0` | `107140`: update second group's target, returns bool |
| `device_get_position` | `101 / 2A4850` | Reads second value |
| `device_set_position_immediate` | `102 / 2A48C0` | `107370`: second group through `107430`, mark flags80 |
| `device_group_set` | `104 / 2A4970` | `1071E0`: clamp0..1; changed + once-only gate; update group target, return bool |
| `device_group_set_immediate` | `105 / 2A49D0` | `107430`: clamp0..1; write group and all linked device values; zero linked velocity |
| `device_operates_automatically_set` | `107 / 2A4AA0` | `107590`: machine automation flag |

Source:
[device reconstruction and matched group setters](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/devices.cpp),
[immediate position setter with misleading recovery title](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_107370.cpp),
[actual builtin names](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Blamite/Formats/Halo2Xbox/H2X_Scripting.xml).
Unassigned authored group `-1` is not proof a device has no runtime group:
recovered `1076E0` allocates private groups. Power defaults to1 unless the
placement initially-off bit1 is set; position defaults to0 unless initially
open bit0 is set. Private group flag4 marks ownership; position also retains
the placement change-once bit2. Simply rejecting all such authored machines
would reject Cairo's macgun, loader, and bridge door. These rules are visible
in the [device placement callback](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/devices.cpp),
again with its misleading power/position labels reversed.

Machine tag timing: flags `BC` u32, power transition/acceleration float
`C0/C4`, position transition/acceleration `C8/CC`, depowered position
transition/acceleration `D0/D4`. Open/close/opened/closed/depowered/repowered
refs `DC/E4/EC/F4/FC/104`; delay float `10C`, delay effect `110`, activation
radius `118`, machine type i16 `11C`, door-open time float `120`.
The [common Halo2 machine XML](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2/mach.xml)
is used where the Xbox override directory has no machine XML; the offsets
were validated against Cairo tags and the original device field accesses.

Cairo bridge door name474/type7/placement147, file `0E60E9FC`: groups `-1/-1`,
device flags0, machine flags1 (not automatic), position
`(60.9517669678,-7.38645696640,6.97004079819)`. Tag `803A1E01`, metadata
`0EB94258`, timing floats `(0,900,.333333343268,4,0,900)`, machine type0,
door-open time1. `open_bridge_door` script57 sleeps **417 script ticks**, then
sets position1. Real door animation/collision movement is required; changing
a scalar alone does not move its visual mesh or collision.

The six Cairo authored groups are index0 `elv_elevator_position`,1
`outro_door_01`,2 `outro_door_02`,3 `outro_door_03`,4 `outro_elevator`,5
`outro_lever`. All initial values0; indices1..5 have once-only flag1.

## Trigger volumes

The normal trigger volume block is **scnr+108**, stride `44`; the XML label
"Kill Trigger Volumes" does not mean this is a different kill-only array.
Name SID `00`, object-name index i16 `04`, node index i16 `06`, node SID `08`,
forward float3 `0C`, up float3 `18`, corner position float3 `24`, positive
extents float3 `30`, radius float `3C`, kill-trigger index i16 `40`.
Source: [scenario XML](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2Xbox/scnr.xml),
[trigger recovery](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_11c380.cpp).

This is a **corner plus full positive extents**, not centered half-extents.
World matrix: left=`up×forward`, authored corner translation. An attached
volume resolves object name through `BB760`, finds the model node by SID,
and composes the live node transform with this local volume. Unresolved
object/node returns false (`11C380`, reconstructed). Retail original
`11C470`, 318 bytes, independently inspected at `11C533..11C5A2`, tests
local coordinates after inverse scale with **strict**
`0<x<extent.x && 0<y<extent.y && 0<z<extent.z`. Boundary faces are excluded.
It is `todo` in the manifest, not byte-matched recovery. Matched `11C5B0`
tests runtime object **center at +30**, not origin/pelvis/radius overlap.
`volume_test_object` opcode020/evaluator `2A0E40` therefore needs the object's
actual center. Radius is used by broader collection queries, not this point
test. Transform basis source:
[matrix math](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_141590.cpp).

Local trigger0 `bsp_0,2`, file `0E6119F8`, unattached:
forward `(.710723996162,.703471124172,0)`, up `(0,0,1.00000035763)`,
corner `(63.8227119446,-38.3589782715,2.79881501198)`,
extents `(4.38268947601,2.43199992180,3.60525798798)`, radius3.08708906174.
Constructed interior center
`(64.5247320683,-35.9531898646,4.60144465064)` is inside; the authored
corner is on the excluded boundary. Do not normalize away authored basis
values when comparing original float arithmetic.

## Relative actor and scenery animation services

`custom_animation_relative` opcode `CB`, evaluator `2A3600`, returns
**boolean** and accepts `(unit,jmad,StringID,boolean interpolate,object
anchor)`. It calls `11B520(graph,unit,SID,interpolate,anchor,false)`, where the
last argument selects non-looping playback. Opcode `CC` instead selects loop
true. The boolean argument is **interpolation**, not loop. Recovered `11B520`
starts the unit channel, enables its scripted animation, requests a `.267`
second blend when interpolation is true or clears that blend when false,
and records the anchor. Failure to resolve/start a real animation yields
false. Source:
[unit animation recovery](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_11a4d0.cpp),
[evaluators](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_29f5b0.cpp).
These helper/evaluator recoveries are `todo`; the script signatures were
checked against the original definition table and
[primary builtin definitions](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Blamite/Formats/Halo2Xbox/H2X_Scripting.xml).

`scenery_animation_start_relative` opcode `BA`, evaluator `2A3070`, returns
**void** and accepts `(scenery,jmad,StringID,object anchor)`. It calls
`10A660(graph,object,SID,frame0,anchor,true interpolate,false loop)`; loop
variant `BB` changes the last flag. The helper initializes the supplied graph
against the object's `hlmt`, finds the clip by SID, plays it, seeks to
`frame/30` seconds, and stores the anchor. The helpers use normal animation
state/codec sampling, not an independent guessed duration. Sources:
[scenery animation recovery](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_10a660.cpp),
[primary signatures](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Blamite/Formats/Halo2Xbox/H2X_Scripting.xml).

When an anchor is present both helpers invoke
[default-placement reset](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/include/object_default_placement.h):
clear ordinary parenting, place the object at canonical origin/forward/up,
and record scripted attachment; unit reset also clears velocity. The authored
initial actor position is **not** an extra transform to multiply on top of
the anchor and animation root. The attachment/nodes update applies the live
anchor later; preserving the full ordinary hierarchy is necessary to make
this faithful. A simplified renderer should explicitly document its scope
instead of silently double-translating or detaching cinematic actors.

## Actual Cairo startup and first scene

This trace is decoded from **local compiled expression nodes**, not a
recreated mission script. Source for node flags/types/casts:
[HS recovery](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_209520.cpp).
The node layout/argument-chain traversal and salted validation are in
`SYSTEMS_RESEARCH.md`. Script indices below are Cairo-local.

Startup script387 `mission_spacestation`, root `ACE0496C`:

1. If `player_count==0`, `sleep_forever`.
2. Static16 `cinematic_snap_to_white`, root `E510019D`, in order:
   disable player input; disable player camera control; fade-out white0;
   HUD cinematic fade0/0; cinematic start; immediate letterbox true;
   scripted camera control true.
3. Static301 `save_cutscene`, root `91372DC3`, only calls
   `data_mine_set_mission_segment("01b_cutscene")`. This is **telemetry**,
   not the checkpoint operation despite its name.
4. Static7 `cinematic_skip_start`, root `E4090096`: internal skip-start
   builtin22C; `game_save_cinematic_skip` builtin370; sleep-until
   `!game_saving` builtin250 at poll1; return `!game_reverted` builtin251.
5. If true, static17 stashes players (hide player0/player1 and invulnerability),
   static181 `x02`, root `803F1CCB`, runs, then static18 unstashes.
6. Static8 `cinematic_skip_stop`, root `E41A00A7`: internal stop22D;
   if `!game_reverted`, **game_revert** builtin238.
7. Predict tram object; camera control false; sleep1; cache block one frame;
   sleep1; static385 `mission_start`; static386 `mission_setup`; static381
   `load_bsp0a`; wake flavor/1st/atrium2/tram mission threads; sleep1;
   static10 `cinematic_fade_from_white`.

The two internal skip builtins only toggle interface flag6 via matched
`2A9780/2A97A0`. The static wrapper's returned boolean comes from actual
save/reverted state. A fresh entry is unreverted, but permanently returning
false for `game_reverted` makes skip-stop repeatedly revert. A native
checkpoint must restore script/world state and preserve the controller's
reverted signal; reporting a save as successful without a recoverable
snapshot does not execute this authored control flow. Sources:
[skip evaluators](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_29f5b0.cpp)
and [builtin names](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Blamite/Formats/Halo2Xbox/H2X_Scripting.xml).

`x02` first calls texture/geometry cache flush, ambience details false,
machinery sound-class gain0 over15ticks, switches BSP by name `01_bsp_0`
(index0), sleeps1, then calls static60 `x02_scene_01`, root `EA9D072A`:

1. White fade-out0, scripted camera true, cinematic start; set external
   letterbox-style1; FOV60degrees/0ticks.
2. Predictions for scene resources/music/foley; sleep **prediction_offset**.
   Local global initializer is **45**, and `sound_offset` initializer is15.
3. Static58 `x02_01_setup`, root `EA030690`: destroy type mask2052 (`804`,
   weapons + crates), destroy offending_light name300; anew Chief337,
   Johnson336, hat59; Johnson helmet permutation to empty; anew Marines351/
   352, choose Smith/Perez heads; anew crowd Marines338..346 as authored;
   anew drones57/58; cinematic LOD flags; wake score/foley/dialog/crowd/DOF/
   lighting/bridge-door threads.
4. `camera_set_animation_relative(F3F411BC,06001C72,NONE,flag17)`.
5. `custom_animation_relative` Chief337 graph`F3F511BD`, `chief_01`
   SID`08001C7D`; Johnson336 graph`F3F111B9`, `johnson_01` SID`0A001C50`;
   Marines351/352 with that graph and `marine02_01`/`marine04_01`, all relative
   to named anchor180. Scenery animation relative for hat and two drones.
6. Sleep20; fade-in white15; sleep `camera_time-sound_offset`; predict foley;
   sleep `camera_time-15`; create next-scene problem actors; fade-out white15;
   sleep15; cleanup destroys this scene's actors and clears screen effects.

`x02` subsequently switches BSPs0→5(`high_0`)→1→5→1→5→1 through scenes02..10,
with five-tick waits at switches; it restores machinery gain1 and bloom
override false. One loaded BSP does not contain the complete introduction.
Static385 `mission_start`, root `AC0C4898`, only prints three designer strings.
Static386 `mission_setup`, root `AC3A48C6`, clears objectives, disables
flashlights, sets player/human and covenant/prophet allegiances, sets macgun412
and gun_loader471 positions immediately0 and power0, creates Malta10/Athens11.
Static381 `load_bsp0a` creates named rack weapons and attaches them to authored
markers. It is not an AI wave or replacement spawn algorithm.

Static10 `cinematic_fade_from_white`, root `E45800E5`, restores HUD gain1/.5sec,
cinematic-stop, camera-control false, white fade-in15, sleep15, player input
true, player camera control true. Static9 fade-to-white instead disables
input/camera input, HUD fades0/.5sec, cinematic-start, white fade-out30,
sleep30, camera-control true. These are the actual transition services needed
before a later unsupported actor/scene operation can be reported honestly.

## Local validation hashes and remaining boundaries

| Array | Count/bytes | File offset | SHA-256 |
|---|---:|---:|---|
| Object names | 481×36 | `0E5F4840` | `36392ed03d91dd9881fa7613fb4aee6a702729064537a5f8292e3ff60aa8f072` |
| Cutscene flags | 34×56 | `0E65E138` | `5f4db007170c80754a14b272d83f290bc7bbcb6ceb3677a7aa93886a6e92c2dc` |
| Camera points | 0 | — | empty block |
| Device groups | 6×40 | `0E60BFB4` | `943e1a14d0226295c0216e5af4ef5e07ce644c9a5f3be66b2c792b8dfc1ddbf4` |
| Machine placements | 160×72 | `0E60C0A4` | `dde2fd487230ef29f98d8f3a9f85f394afd354842a76dd311bfe1e18e2c6f031` |
| Trigger volumes | 52×68 | `0E6119F8` | `ba710711ab2e2a356cc880f5ea1f963df1122f98907281de1dd31bb13ed2e990` |
| Script records | 418×40 | `0E6582D8` | `34d32f0e372ffc1e2ad98ffe3e74fdc0841ae264e2488dc99b2838d193ca6448` |
| Expression nodes | 27091×20 | `0E65EBB8` | `b52e8cebc73a7ef095ef3307be8a66ef1ef07ee49ba28b8a6ba160d2e77d23bb` |
| Script strings | 50228 bytes | `0E64BEA4` | `7d3c3b5275b64fbbb3107182992abf0e4be094bcda097a3ac50478114dc8a427` |

Primary source/layout inspection and independent numeric decoding establish
these data contracts, original evaluator signatures, and the cited retail
instruction behaviors. They do not establish a visually matching cinematic,
whole mission completion, or recovered native AI/physics parity. In
particular: full checkpoint restore semantics, relative character/scenery
animation, animated doors and dynamic collision, actor permutation changes,
HUD fade, cinematic sound/DOF/lighting, marker attachments, BSP switching,
object node attachments, and all later campaign builtins still require
service-level implementation and runtime verification. Documented source
`todo` status must not be promoted to byte-matched parity merely because an
authored script advances in a port.
