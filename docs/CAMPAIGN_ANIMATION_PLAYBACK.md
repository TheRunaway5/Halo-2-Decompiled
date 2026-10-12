# Retail campaign animation playback

Research only, 2026-10-07. Authority: local `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
Recovery commit `5f08630655e3ddc9ac45909090f11ae799fa0c27`; Assembly commit
`fd14ef3c46135e9422966c6a64cd926fb0bd23fb`. This extends the first-scene asset
findings in [CAMPAIGN_ANIMATION_RESEARCH.md](CAMPAIGN_ANIMATION_RESEARCH.md).
No production source or earlier documentation was changed.

## Script signatures and failed starts

The retail table `004744E0`, rather than source guesses, gives these bindings.
`G` means animation graph, `SID` means StringID, `short` is signed 16-bit.
Scenery type is55, unit51, object50, graph36, SID11, boolean5, short7, void4.
All these definitions have flags0. The complete raw definitions are in
[animation-playback-retail.json](../analysis/campaign/animation-playback-retail.json).

| Opcode | Signature | Evaluator |
| --- | --- | --- |
| `0B8` | `void scenery_animation_start(scenery,G,SID)` | `2A2FD0` |
| `0B9` | `void scenery_animation_start_loop(scenery,G,SID)` | `2A3020` |
| `0BA` | `void scenery_animation_start_relative(scenery,G,SID,object)` | `2A3070` |
| `0BB` | `void scenery_animation_start_relative_loop(scenery,G,SID,object)` | `2A30C0` |
| `0BC` | `void scenery_animation_start_at_frame(scenery,G,SID,short)` | `2A3110` |
| `0BD` | `void scenery_animation_idle(scenery)` | `2A3170` |
| `0BE` | `short scenery_get_animation_time(scenery)` | `2A31B0` |
| `0C7` | `short unit_get_custom_animation_time(unit)` | `2A3450` |
| `0C8` | `void unit_stop_custom_animation(unit)` | `2A34D0` |
| `0C9` | `bool custom_animation(unit,G,SID,bool interpolate)` | `2A3520` |
| `0CA` | `bool custom_animation_loop(unit,G,SID,bool interpolate)` | `2A3590` |
| `0CB` | `bool custom_animation_relative(unit,G,SID,bool interpolate,object)` | `2A3600` |
| `0CC` | `bool custom_animation_relative_loop(unit,G,SID,bool interpolate,object)` | `2A3670` |
| `0CD` | `bool custom_animation_list(object_list,G,SID,bool interpolate)` | `2A36E0` |
| `0CE` | `bool unit_custom_animation_at_frame(unit,G,SID,bool interpolate,short)` | `2A3740` |
| `0CF` | `bool unit_is_playing_custom_animation(unit)` | `2A37A0` |
| `0D0` | `void object_set_custom_animations_hold_on_last_frame(bool)` | `2A37F0` |
| `0D1` | `void object_set_custom_animations_prevent_lipsync_head_movement(bool)` | `2A3840` |

`11B520` rejects a `NONE` unit or graph, then `1101E0` requires an animation
state and a matching clip. Clip lookup occurs **before stopping the existing
custom animation**. A failed name lookup therefore preserves the old channel,
anchor and scripted flags. A later graph-initialization failure can occur
after the old animation has been stopped; failure is not universally
transactional. The bounded original fixture confirms `NONE`, missing state,
missing clip and nonexistent SID0 failures preserve the existing state when
the requested graph is already installed. A zero SID is not a special start
frame. `10A660` additionally rejects SID0 explicitly; it reports false to its
caller, but the scenery script evaluator returns void regardless of success.
Both helpers use engine-owned live object handles; the `NONE` guards are not
evidence that arbitrary stale handles are safe.

`custom_animation_list` visits unit type-mask3 objects; an empty list succeeds.
Its recovered `result && start(...)` means a failure short-circuits later
starts even though iteration continues. The at-frame unit helper starts a
non-looping, non-relative clip first, then seeks its primary channel if valid;
the boolean reports the start outcome. Scenery idle `10A3F0` clears its idle
flag and starts SID `0400000C` as a loop at frame0 with no anchor; it is not a
freeze command. Sources: [11B520](../analysis/campaign/animation-playback-11b520.txt),
[1101E0](../analysis/campaign/animation-playback-1101e0.txt),
[10A660](../analysis/campaign/animation-playback-10a660.txt),
[11B4A0](../analysis/campaign/animation-playback-11b4a0.txt), and
[10A3F0](../analysis/campaign/animation-playback-10a3f0.txt).

## Channel clock, seek and completion

A channel stores graph/clip IDs, float32 `rate` at+18, float32 frame position
at+1C, flags at+12, delayed-start count at+10 and completion flags at+11.
Start `1C6470` sets rate1, position0, clears completion/events, and sets a
two-update delay only if channel flag `8000` is supplied. Custom unit starts
use `4001`, or `4003` when looping. Public scenery starts also use `4001/4003`.

`1C68C0` advances only an active channel (flags bit0) without completion byte
bits0/3. A positive delay is decremented instead. Advancement is ordered
float32 `tick_seconds * channel_rate * 30 + current_frame`, using simulation
globals `g_510C54+4`. `1CB5B0` also suppresses advancement while the parent
animation state's+6C bit0 is set. Render time and audio wall time are separate.

`1C6560` seeks to `clamp(frame,0,float32(N)-0.0001f)`. It updates upcoming
events and sets completion bit1 (`2`) when the next simulation step would
reach that upper boundary. At-frame scripts convert signed short frame to
float32 seconds using `1/30`, then multiply by30 in `1CCB80` or `11B4A0`.
This round trip is visible: original frame7 becomes `7.000000476837158`,
frame515 becomes `515.0000610351562`. Negative values, including−32768 and−1,
clamp to frame0; zero starts frame0; values above the end clamp to the end.

`1C66A0` clears completion bits1/2/3, dispatches crossed events, and checks
that the callback did not replace the animation before changing its clock.
At the boundary a non-looping channel sets bit3 (`8`) and holds `N−0.0001f`.
A loop sets bit2 (`4`) and carries the residual into its authored loop frame:
`next = float32(next−upper_boundary) + loop_frame`. It does one wrap, followed
by the ordinary seek clamp, rather than an unrestricted modulo operation.
Variant selection occurs only when channel flag20 is set and loop frame0;
custom flags4003 do not select that path. For516 frames at30Hz:

| Situation | Original frame / completion byte |
| --- | --- |
| update513→514 | `514 / 0` |
| update514→515 | `515 / 2` (next-step completion) |
| non-loop update515→end | `515.9998779296875 / 8` |
| further non-loop updates | same position /8 |
| loop update515→loop0 | `.0001220703125 / 4` |
| loop update515→loop40 | `40.0001220703125 / 4` |

Duration is `N*(1/30)` seconds, independent of rate. The script queries then
subtract elapsed seconds, multiply by30, x87-round to an integer, subtract2
and clamp at0. Unit narrows to short **after** the subtraction/clamp; scenery
narrows **before** them. Preserve float32 order and x87 round-to-nearest-even;
`round(N−frame)` is not an equivalent implementation. The516-frame clips
report514 at frame0,1 at frame513, and0 by frame514 while playback continues.
Unit query returns0 unless its main channel is valid in custom state
`0E0000C2`; scenery returns0 for `NONE`, wrong object type, missing state,
graph or channel. An overlay custom animation need not appear in that unit
duration query. A zero duration query is not a reliable pose-completion flag.

Evidence: [1C6470](../analysis/campaign/animation-playback-1c6470.txt),
[1C6560](../analysis/campaign/animation-playback-1c6560.txt),
[1C68C0](../analysis/campaign/animation-playback-1c68c0.txt),
[1C66A0](../analysis/campaign/animation-playback-1c66a0.txt),
[1CB5B0](../analysis/campaign/animation-playback-1cb5b0.txt),
[unit query](../analysis/campaign/animation-playback-2a3450.txt),
[unit seconds](../analysis/campaign/animation-playback-11b6b0.txt), and
[scenery query](../analysis/campaign/animation-playback-10a460.txt).

## Unit hold, stop and sampling

The main unit controller has an additional completion rule. `1101E0` copies
`!objectGlobals.flag80` into per-unit control bit100 **at start**; later
changes of the global do not retroactively update that bit. Opcode0D0 sets
the global. `cinematic_start` (`13BFF0`) sets it true, while map/reset
`13CA80` clears it. This is the hold-on-last-frame policy.

After its channel update, `111650` sees the anticipatory bit2. A scripted
loop4003 continues; a non-loop4001 with control bit100 clear sets parent state
pause bit1. At normal30Hz this holds the516-frame cinematic unit at frame515,
the last actual sample, before the raw channel reaches515.9998779. With
control bit100 set, the custom-state branch calls `11B710` and returns to
ordinary animation. Animation type!=0 uses control overlay channel+9C;
the overlay completion branch can stop it independently. Treating all unit
custom animations as an indefinitely held primary channel omits that route.

Explicit stop ignores `NONE` or an inactive custom state. Otherwise it clears
active bit10, control bit100, saved anchor and object scripted flag2; resets
custom control and returns to default/weapon idle, including secondary weapon
state. If the **old** control bit100 was set, the old anchor was non-NONE and
the unit has no ordinary parent, it commits evaluated root node0 multiplied
by the model's stored default-root matrix to position/forward/up, zeros linear
and angular velocity, and enables Havok via `B8890`. For a cinematic-held
clip (old bit100 clear), explicit stop does not perform that root commit.
`B8890` is physics activation, correcting the earlier note's node-update
description. These effects cannot be represented by clearing a clip name.

The unit script's interpolate boolean requests `.267` seconds of **object
pose-transition interpolation** (`BA350`), or clears it (`BA3D0`). It does not
change custom channel4001/4003. Channel sampler `1C6F60` enables frame
interpolation if flag40 is set, or if an active, nonzero-rate channel runs at
a rate other than1 or a simulation frequency other than30Hz. Therefore the
false parameter is not a blanket prohibition on fractional-frame sampling.

Sampler setup `2798A0` truncates/clamps the current frame index to0..N−1.
With interpolation enabled it selects at most the next index N−1, uses the
fraction, snaps fractions below `.0001` or above `.9999`, and disables
interpolation at the final index. Thus515.5 and515.9998779 both sample frame515
without blending toward frame0. The clock's upper clamp and sampler's final
index are distinct. A native pose evaluator that wraps its next-frame sample
at the final index produces a visible jump even with a non-looping clock.

Evidence: [unit controller](../analysis/campaign/animation-playback-111650.txt),
[hold setter](../analysis/campaign/animation-playback-2a37f0.txt),
[cinematic start](../analysis/campaign/animation-playback-13bff0.txt),
[stop](../analysis/campaign/animation-playback-11b710.txt),
[channel sampling](../analysis/campaign/animation-playback-1c6f60.txt),
[sampler setup](../analysis/campaign/animation-playback-2798a0.txt), and
[physics activation](../analysis/campaign/animation-playback-b8890.txt).

## Live anchor composition

Unit callback `112DC0` at object-type matrix slot `467A20` reads control+28.
Scenery callback `10A390` at slot `468060` reads object+138. Both validate the
anchor through `BADC0`, fetch its **current node0 matrix** (object+116 offset),
then execute `142A60(anchor_node0,animated_matrix,animated_matrix)`. A destroyed
or NONE anchor leaves the supplied animated matrix unchanged; it does not
reuse a cached scenario anchor.

The transform includes uniform scale, basis vectors and translation. World
position is `anchor.position + anchor.scale * anchor.basis * local.position`;
scale is multiplied, and basis vectors are rotated. A fixture local position
`(3,4,5)` with live identity anchor `(54,0,7)` yields `(57,4,12)`. Moving the
anchor to `(60,−4,9)`, rotating+90° about Z and scaling2 yields `(52,2,19)`
for **both** callbacks. This closes the earlier unit anchoring uncertainty.
Do not add the actor's scenario placement a second time. Relative starts
reset canonical object placement and ordinary parenting; units also zero
velocity. Object traversal must evaluate a referenced anchor's current nodes
before composing dependent objects, including when the anchor itself animates.
Evidence: [112DC0](../analysis/campaign/animation-playback-112dc0.txt),
[10A390](../analysis/campaign/animation-playback-10a390.txt), and
[142A60](../analysis/campaign/animation-playback-142a60.txt).

## Reproduction and limits

Run `.tools/python/bin/python tools/inspect_campaign_animation.py`
and `.tools/python/bin/python tools/oracle_campaign_animation.py`.
The latter executes the original channel, lookup, event, query, failed-start,
sampler-index/fraction and matrix instructions, with a synthetic resident graph,
object arena and empty event/codec data. Only HS argument/result entry points
are fixture adapters. The x87 control word is027F and MXCSR1F80. Results are
in [animation-playback-golden.json](../analysis/campaign/animation-playback-golden.json);
the [fixture source](../tools/oracle_campaign_animation.py) records
its exact boundary. No unit controller, successful full-object start/stop,
compressed skeleton, Havok, sound event or complete cinematic was replayed.

A native controller needs saved per-object graph/clip, channel frame/rate,
loop/hold/completion/pause state and live anchor handle; it must preserve the
old clip on failed lookup and return to real idle/physics when required.
Separate pose sampling must clamp final indices instead of wrapping. These
requirements support the first Cairo scene; they do not establish complete
campaign animation, machine motion, authored AI or full-playthrough parity.
