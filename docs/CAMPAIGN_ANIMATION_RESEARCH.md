# Cairo cinematic animation and supporting host services

Research only, 2026-10-07. The authority is local `assets/disc/default.xbe`,
SHA-256 `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`,
and the compiled expressions/assets in `01b_spacestation.map`. Recovery
references are pinned at `5f08630655e3ddc9ac45909090f11ae799fa0c27`;
Assembly schemas are pinned at `fd14ef3c46135e9422966c6a64cd926fb0bd23fb`.
No production source was changed by this research. Existing startup/control
flow is described in [CAMPAIGN_RESEARCH.md](CAMPAIGN_RESEARCH.md).

## Player stashing and damage state

The retail builtin table at `004744E0` confirms these signatures and bindings;
the extracted definitions are in
[animation-research-builtin-definitions.json](../analysis/campaign/animation-research-builtin-definitions.json).

| Builtin | Opcode / evaluator | Actual behavior |
| --- | --- | --- |
| `object_hide(object,bool)` | `039 / 2A1610` | `NONE` is ignored. A live player unit is a valid object. `B9C60` sets/clears object `+4` bit0. On a change it removes/adds light attachments when the root is connected and refreshes the partition record. It does not itself zero velocity or make the object damage immune. |
| `object_cannot_take_damage(object_list)` | `05A / 2A20C0` | `D87E0(list,false)` sets damage-flags word `+10A` bit7. It applies to every object in the list, plus direct attached children carrying object `+4` bit26, the permutation-child flag. |
| `object_can_take_damage(object_list)` | `05B / 2A2100` | Same iteration, clearing that damage bit. It is not a boolean overload of `05A`. |
| `unit_impervious(object_list,bool)` | `0F8 / 2A4520` | `11A570` validates live handles, filters biped/vehicle type mask3, and sets/clears unit `+134` bit19. This flag suppresses damage impulses and reactions; it must not be substituted for the separate vitality immunity bit above. |
| `player_disable_movement(bool)` | `1DB / 2A8F30` | Writes player globals `g_4E8C20+7`. It is independent of `player_enable_input`, which writes the inverted boolean at `+6`. |

An empty or `NONE` object list makes both list setters do nothing. Cairo's
stash wrapper uses `object_cannot_take_damage(players)`, rather than
`unit_impervious`. `D9020` forces its damage resistance multiplier to zero
when `+10A` bit7 is set. In contrast, unit bit19 is tested by the impulse
branch in `D9640` and the reaction handler `C9E70`.

Evidence: original instruction dumps
[B9C60](../analysis/campaign/animation-research-b9c60.txt),
[D87E0](../analysis/campaign/animation-research-d87e0.txt),
[D9020](../analysis/campaign/animation-research-d9020.txt),
[11A570](../analysis/campaign/animation-research-11a570.txt), and
[2A8F30](../analysis/campaign/animation-research-2a8f30.txt).
Supporting recoveries:
[damage.cpp](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/damage.cpp),
[unit_object_type.cpp](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unit_object_type.cpp),
and [object hide analysis](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/docs/unknown_0b8bd0.md).
A native visibility flag alone does not establish light/partition, attached
child visibility, AI, or Havok parity.

## Authored first-scene animation data

The local compiled expression dump and asset sample are in
[animation-research-assets.txt](../analysis/campaign/animation-research-assets.txt),
with its [reproducible C++ probe](../analysis/campaign/animation-research-assets.cpp).
Every listed clip has **516 frames**, with loop-frame metadata0. That loop
metadata does not turn a non-looping script command into looping playback.

| Object / scenario name index | Graph datum | Clip StringID / name | Skeleton nodes |
| --- | --- | --- | --- |
| Chief337 | `F3F511BD` | `08001C7D / chief_01` | 33 |
| Johnson336 | `F3F111B9` | `0A001C50 / johnson_01` | 53 |
| Marine351 | `F3F111B9` | `0B001C5D / marine02_01` | 53 |
| Marine352 | `F3F111B9` | `0B001C65 / marine04_01` | 53 |
| Hat59 | `F3F611BE` | `0C001C85 / dress_cap_01` | 1 |
| Drone57 | `F3F711BF` | `0F001C8C / news_drone01_01` | 6 |
| Drone58 | `F3F711BF` | `0F001C8D / news_drone02_01` | 6 |
| Camera, flag17 | `F3F411BC` | `06001C72 / x02_01` | 1 |

The four unit calls pass **false interpolation**, and all seven objects use
live named object180 `anchor_x02_01`. Its authored position is approximately
`(54.1543,0,7)`. The individual actor placements must not be added again to
the animation root. For example, Chief's sampled root at frame0 is
`(6.86652,-12.0708,.387898)`; adding his authored scenario placement as well
would double the scene translation.

`custom_animation_relative` (`CB / 2A3600`) calls
`11B520(graph,unit,SID,interpolate,anchor,false-loop)`. Successful playback
marks the scripted animation active, requests `.267` seconds of object
interpolation when true or clears interpolation when false, records the
anchor, clears ordinary parenting, resets canonical origin/forward/up, and
zeros linear/angular velocity. The `CC` variant selects loop true.
`scenery_animation_start_relative` (`BA / 2A3070`) starts channel0 with
interpolation true and loop false, seeks `frame/30` seconds, stores the anchor,
and resets canonical placement. Unlike the unit helper, its placement reset
does not zero velocity. A missing graph/clip or unavailable animation state
fails the helper; the scenery script evaluator still returns void.

For scenery, retail `10A390` directly composes the live anchor's **node0
matrix** with the animated matrix. Using the anchor's original placement
instead of its live node matrix omits anchor animation and hierarchy.
The unit helper's anchor storage/default reset is verified; complete unit
root/attachment-node update ordering remains a separate recovery task.
Sources:
[unit animation](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_11a4d0.cpp),
[scenery starter](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_10a660.cpp),
[default placement](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/include/object_default_placement.h),
and [10A390 instructions](../analysis/campaign/animation-research-10a390.txt).

The duration queries include a **two-frame subtraction**:

* `unit_get_custom_animation_time` (`C7 / 2A3450`) returns
  `max(round_x87((duration_seconds-elapsed_seconds)*30)-2,0)`, narrowed to
  short. Its helper returns zero unless the unit has a valid custom channel
  in state `0E0000C2`.
* `scenery_get_animation_time` (`BE / 2A31B0`) uses
  `max(int16(round_x87((frame_count/30-frame_position/30)*30))-2,0)` and
  returns zero for `NONE`, wrong object type, or unavailable animation state.

Thus these clips initially report514 ticks for unit/scenery queries, whereas
the camera service uses its separate515-tick timeline. Preserve float32
operation order and x87 rounding mode rather than guessing from file duration.
Evidence: [2A3450](../analysis/campaign/animation-research-2a3450.txt),
[11B6B0](../analysis/campaign/animation-research-11b6b0.txt),
[10F690](../analysis/campaign/animation-research-10f690.txt), and
[10A460](../analysis/campaign/animation-research-10a460.txt).
The native `pose_model` currently wraps every clip; a cinematic playback
controller must supply non-looping completion/hold behavior explicitly.

## Audio gain, ambience, residency and bridge machinery

`x02` calls `sound_class_set_gain("device_machinery",0,15)` before the first
scene and restores gain1 over15 ticks after the intro. The class name is
**device_machinery**, not ambient_machinery. Retail `2AA2C0` converts linear
gain to dB with `2197F0` (clamped between -64 and0dB), then `221980` selects
every nonempty one of54 class names containing the requested substring.
The empty string therefore selects every nonempty class. `221900` interpolates
in **dB**, using remaining seconds; it does not interpolate linear gain.
Time is script-short ticks multiplied by `1/30`; negative time is clamped0.
The db variant `268 / 2AA330` passes the dB value directly. Evidence:
[2AA2C0](../analysis/campaign/animation-research-2aa2c0.txt),
[221980](../analysis/campaign/animation-research-221980.txt),
[221900](../analysis/campaign/animation-research-221900.txt), and
[gain recovery](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_2197f0.cpp).

`cinematic_enable_ambience_details` (`372 / 2AC400`) writes looping sound
globals `g_4ED288+240`. Cinematic start resets it true. Ambient portal-path
target audibility permits the first choice outside a cinematic, or while
that flag is true. It is not a global mute of scripted music/dialogue/loops.
Sources: [2AC400](../analysis/campaign/animation-research-2ac400.txt),
[cinematic initialization](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_13ca80.cpp),
and [ambient paths](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/sound_looping_paths.cpp).

`texture_cache_flush` and `geometry_cache_flush` are confirmed retail-null
operations: their definitions point to matched `2AB130`, whose14 bytes simply return HS value0. This admission is justified for these exact
bindings, not for other unimplemented resource functions. In contrast,
`sound_impulse_predict` requests the sound's first chunk and
`sound_looping_predict` requests its first chunks. A native residency service
should validate/load the requested assets; treating prediction as an arbitrary
success hides missing resources. Evidence: [definitions](../analysis/campaign/animation-research-builtin-definitions.json),
[2AB130](../analysis/campaign/animation-research-2ab130.txt),
[2A9D70](../analysis/campaign/animation-research-2a9d70.txt), and
[2AA0B0](../analysis/campaign/animation-research-2aa0b0.txt).

The first scene wakes `open_bridge_door` (script57): after417 script ticks it
calls `device_set_position(x02_bridge_door,1)`. Device name474 is machine
placement147. Its real group motion, visual animation and collision must
advance together; a scalar-only host does not open a traversable door. The
precise timing/group/private-group fields are already recorded in
[CAMPAIGN_RESEARCH.md](CAMPAIGN_RESEARCH.md#machines-and-device-groups).

No complete original audio revert lifecycle was established here. A native
checkpoint must reconcile active scripted voices with the saved state; this
research does not prove an original stop-all-voices operation. Completing
the first scene also requires real permutations, authored crowd playback,
resource requests, bridge motion, timed dialogue/music/foley, and later BSP
transitions. Parsing these clips and admitting the stashing hosts does not
prove that Cairo, or the whole campaign, is playable to completion.
