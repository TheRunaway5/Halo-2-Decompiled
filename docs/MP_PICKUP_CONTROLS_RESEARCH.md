# Original TU1.1 pickup controls and player phase

Research date: 2026-10-08. This continues [inventory and targeting research](MP_INVENTORY_RESEARCH.md) and [world-item initialization research](MP_ITEM_WORLD_RESEARCH.md). Production code is untouched.

The implementable slice is a held-button request generated from authored player-control metadata, admitted through the original player phase, and applied to a stable weapon object. Ordinary BR pickup takes type 20 / mode 3 in the executed stock SMG idle, held-firing, full-reload and empty-reload cases. A separate explicitly supplied numeric trigger selector 1 or 2 takes replacement mode 4; those selectors must not be invented from a native firing/reload visual.

Complete native controller/key binding, live spatial enumeration and visibility, full unit animation timing, and Havok remain unresolved. The report identifies each supplied service and runtime fixture. The authored threshold is **233 milliseconds**, produced by the original conversion from the actual cache value; it is not a guessed tick count.

## Reproduction and evidence

Run:

```sh
.tools/python/bin/python tools/oracle_mp_inventory_controls_1_1.py
```

[The tool](../tools/oracle_mp_inventory_controls_1_1.py) writes [the ignored report](../analysis/multiplayer/integration/inventory-source/controls/report.json). It contains 28 cases, 406 complete routine/fragment rows, 47 input samples, original generated/action payload bytes, admission outputs, requests, inventory snapshots, and source instruction listings. Four cases run the existing original SMG input/tick/update oracle before generating the pickup controls. Three use original world allocations, rather than the earlier preconstructed target objects.

Primary authority is the untouched TU1.1 `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`, and untouched Lockout `assets/disc/maps/lockout.map`, SHA-256 `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`. Original instructions are executed without patching. The other 20 multiplayer cache files are read for the authored hold-threshold comparison; their paths and SHA-256 values are in `authored_player_control_maps` in the report. Control execution uses Lockout.

The frozen world and grenade reports remain unchanged. This new report extends the old target report's explicitly supplied action-flag fixtures by producing the flags through original input/control instructions.

## Device, profile and control records

The relevant source records are distinct:

| Record | Address/index | Relevant layout |
| --- | --- | --- |
| Hardware handle array | `4ED79C + controller*4` | Nonzero handle admits a controller |
| Processed device | `4ED7AC + controller*48` | Analog values `+0..7`, thresholds `+8..F`, 16 button frame counts `+10..1F`, 16 button millisecond counts `u16 +20..3F`, four stick axes `i16 +40..47` |
| Profile | `5262A8 + controller*1C` | 16 logical-to-device button bytes `+8..17` |
| Logical input | `526318 + controller*50` | Logical frame counts `+0..F`, millisecond counts `u16 +10..2F`, other stick/trigger fields |
| Player | `player_pool+44 + low16(player_datum)*21C` | Hardware controller index `i32 +24`; local user index `i16 +28`; unit datum `+2C` |
| Local control | `*(4F4854) +14 + local_user*94` | Unit datum `+0`, gameplay flags `+4`, action flags `u16 +8`, inventory sequence `u16 +28`, primary/other slot bytes `+2A/+2B`, target 12 bytes `+6C` |
| Player action | 5C-byte payload | Gameplay flags `+0`, action flags `u16 +1C`, inventory sequence `u16 +1E`, primary/other slot bytes `+20/+21`, target 12 bytes `+28` |

The executed fixture has hardware controller 0 and local user 0. The source indexes those two records separately; their indices must not be assumed interchangeable. [Full control generation](../analysis/multiplayer/integration/inventory-source/controls/instructions/001884e0.asm), [input processing](../analysis/multiplayer/integration/inventory-source/controls/instructions/00187610.asm).

Original `21A530` initializes the profile mapping to `00 04 02 03 01 05 06 07 0C 0D 0E 0F 0A 0B 00 01`. Original `192666` then applies the selected profile button-layout byte at `556548 + controller*C74`. For layouts 0, 1, 2 and 3, logical button **2** still maps to device button **2**, and logical button **3** to device button **3**. Full `219D50` copies the source device frame/millisecond counters through that mapping. These are exact original indices; this report does not assign an unverified physical button label or native key name. [Profile defaults](../analysis/multiplayer/integration/inventory-source/controls/instructions/0021a530.asm), [profile overrides](../analysis/multiplayer/integration/inventory-source/controls/instructions/00192666.asm), [logical mapping](../analysis/multiplayer/integration/inventory-source/controls/instructions/00219d50.asm).

## Original polling and hold threshold

`126350` calls original `XInputGetState` at `4101AC`. The tool supplies its 22-byte packet as a hardware-service result. Analog button `n` reads packet byte `+6+n`; the executable's analog reorder table `447288` is `00 01 02 03 04 05 06 07`. Digital buttons read packet `u16 +4` through table `447290`, `01 02 04 08 10 20 40 80`. The source record's analog down condition uses the **previous** stored analog value and threshold before replacing the value from the new packet. It then increments frame and millisecond counters, saturating at `FF` and `FFFF`, or clears them. Analog hysteresis adjusts thresholds afterward. Therefore a press starts its counter one poll later, and a release retains a counted-down state for one additional poll in the recorded zero/255 sequences. [Original device polling](../analysis/multiplayer/integration/inventory-source/controls/instructions/00126350.asm).

The surrounding `1260A0` obtains elapsed milliseconds from the original clock, clamps that interval to `0..100`, and calls `126350`. The experiment supplies the interval directly to the unchanged polling routine; its 33-ms trajectory is a labeled fixture, not a proposed native timing constant. [Polling caller](../analysis/multiplayer/integration/inventory-source/controls/instructions/001260a0.asm).

`187610` reads the first player-control record through `matg +F4`. The cache block count is at `matg +F0`; all 21 actual multiplayer caches have count 1. The hold threshold is record `f32 +7C`. In Lockout the first record is file `01D1A3E4`, and the threshold field is file **01D1A460**, bits **3E6EE632**, value `0.23330000042915344`. Original `46513C` contains float 1000, bits `447A0000`. `18841A..188433` performs single-precision multiplication and truncating `CVTTSS2SI`, yielding **233**. All 21 authored cache fields have those same threshold bits; their metadata reads are recorded separately from the execution. [Threshold instructions](../analysis/multiplayer/integration/inventory-source/controls/instructions/00187610.asm), [metadata report](../analysis/multiplayer/integration/inventory-source/controls/report.json).

The generated input structure has three adjacent fields: gameplay flags `+18`, selector/transient flags `+1C`, and action flags `u16 +20`. `1884E0` transfers generated `+20` to local-control `+8`, then player-action `+1C`. Reading generated `+1C` as the player-action flags gives the wrong button contract. [Generation and marshalling](../analysis/multiplayer/integration/inventory-source/controls/instructions/001884e0.asm).

For the pickup pair, the source action flags are:

| Bit/mask | Original generated condition |
| --- | --- |
| 0 / `0001` | Logical button 2 frame count is nonzero |
| 1 / `0002` | Logical button 2 milliseconds are at least the converted authored threshold |
| 4 / `0010` | Logical button 3 frame count is nonzero |
| 5 / `0020` | Logical button 3 milliseconds are at least the same threshold |

The threshold comparison itself does not also test the frame count. The actual authored threshold is positive and the polling path clears milliseconds on release. The executed boundary cases use original polling from an explicit processed-device duration, producing no primary pickup at 232 ms and a primary BR pickup at 233 ms. The source emits both held and threshold flags (`0003` primary, `0030` secondary) at the threshold. [Original generation](../analysis/multiplayer/integration/inventory-source/controls/instructions/00187610.asm).

## Player admission, consumption and sequence ownership

The report executes the unchanged final player-phase fragment `14ED0C..14ED4F` (exclusive stop), with a one-player pool/frame fixture. Its relevant order is:

1. Iterate the original player bitmap, require the player's action-frame mask bit, and validate its action with full `14E7E0`.
2. Require a valid player unit datum and unit `+134 bit0` for the ordinary controlled-unit path. Local input suppression may reinitialize the payload while preserving look angles.
3. Copy all 5C bytes to the phase's private payload.
4. Call full `150DD0` for button edges and consumption.
5. Call full `1EA490` to save the local action/view cache.
6. Call full `150F00` for interactions and actions.
7. Call full `151C90`, which constructs the unit controls and invokes full `C8140`.

No frame-mask bit, a malformed target action, disabled controlled-unit admission, and original local input clearing each prevent the executed ordinary pickup. The tool keeps the disabled-unit case's auxiliary local-player branch inactive explicitly; it does not recover that auxiliary branch. Full `1884E0` still scans candidates even after its input-clearing gate zeroes generated controls. [Player phase](../analysis/multiplayer/integration/inventory-source/controls/instructions/0014eae0.asm), [payload validator](../analysis/multiplayer/integration/inventory-source/controls/instructions/0014e7e0.asm).

`150DD0` retains held state in player `u16 +3C`: primary bit0, secondary bit4. It generates primary press/release bits **2/3** and secondary press/release bits **6/7**. Player flags `+2` bit `10` suppress the entire low action nibble until primary release; bit `20` suppresses the high nibble until secondary release. Those bits are set when the corresponding source interaction succeeds. They must remain player-owned state; simply issuing a pickup on every held frame loses this contract. [Edges and suppression](../analysis/multiplayer/integration/inventory-source/controls/instructions/00150dd0.asm), [interaction consumption](../analysis/multiplayer/integration/inventory-source/controls/instructions/00150f00.asm).

For primary threshold bit1, `150F00` first tries `1533E0` for the selected nonweapon interaction. If it does not consume the action, it calls weapon request `153090(primary=1)`. Success sets player bit `10` and clears action bit1. Secondary threshold bit5 calls `153090(primary=0)`; success sets player bit `20`, updates local control state, and the source clears bit1 rather than bit5. The next `150DD0` suppression clears the high nibble until release. This observed source behavior is preserved. [Primary nonweapon action](../analysis/multiplayer/integration/inventory-source/controls/instructions/001533e0.asm), [weapon request](../analysis/multiplayer/integration/inventory-source/controls/instructions/00153090.asm), [player actions](../analysis/multiplayer/integration/inventory-source/controls/instructions/00150f00.asm).

The source view cache is copied **before** `150F00` mutates the private payload. The original frame payload is also unchanged. In the continuous-hold case, first successful BR pickup occurs at counted 264 ms in the supplied 33-ms trajectory; the 231-ms sample has no threshold flag. Exactly one type-20 pickup is emitted. Later generated flags stay `0003`, but player-phase flags become zero under consumption until release. [View-cache copy](../analysis/multiplayer/integration/inventory-source/controls/instructions/001ea490.asm), [continuous-hold case](../analysis/multiplayer/integration/inventory-source/controls/report.json).

Inventory sequence prevents a later unit-control copy from undoing a same-phase pickup. Generated action `u16 +1E` is the inventory sequence associated with its selected slot bytes. Pickup increments the unit's replicated sequence `u16 +214`. `C8140` accepts its incoming sequence/slot word only when incoming sequence equals `unit+214`. In the successful BR case the incoming payload still has sequence 1 / primary slot 0, while pickup has changed the unit to sequence 2 / slot 1. `C8140` preserves the new requested slot 1. On the next control-generation call, `1884E0` refreshes local sequence/slot fields from unit `+214`; the next payload has sequence 2 / slot 1. [Unit controls](../analysis/multiplayer/integration/inventory-source/controls/instructions/000c8140.asm), [control refresh](../analysis/multiplayer/integration/inventory-source/controls/instructions/001884e0.asm).

## Actual weapon actions, allocation and replacement

The source-generated ordinary outcomes are:

| Initial carried SMG state | Original state before pickup | Primary BR result |
| --- | --- | --- |
| Idle | Trigger `+20C=0`, magazine mode 0, loaded/reserve 60/120 | Mode 3; retain SMG slot 0, BR becomes slot/current/requested 1 |
| Held firing after 45 original ticks | Trigger 0, magazine mode 0, ammo 37/120 | Same mode-3 result; preserve SMG ammo and shared RNG |
| Full reload command after those firing ticks | Trigger 0, magazine mode 1, ammo 37/120 | Same mode-3 result |
| Empty reload command after 135 firing ticks | Trigger 0, magazine mode 1, ammo 0/120 | Same mode-3 result |
| Explicit numeric trigger selector 1 or 2 | Declared selector fixture | Mode 4; drop SMG and replace slot 0 with the same BR datum |

The reload cases execute original `105640` commands and original weapon updates, including the relevant real animation timing lookup. They do not claim that the complete player/unit phase automatically generates those reload commands. Projectile allocation and other prelude service boundaries remain named in the report. Numeric selector fixtures are separate from the actual-action cases. [Original SMG input](../analysis/multiplayer/integration/inventory-source/controls/instructions/001045e0.asm), [weapon update](../analysis/multiplayer/integration/inventory-source/controls/instructions/001021d0.asm), [reload command](../analysis/multiplayer/integration/inventory-source/controls/instructions/00105640.asm), [case evidence](../analysis/multiplayer/integration/inventory-source/controls/report.json).

With a duplicate stock SMG target, primary threshold does not emit a pickup in this carried-SMG fixture. Secondary threshold admits mode 5, producing current/requested hands `[0,1]` and retaining both object identities. This is evidence of the source dual-wield path; it does not make native dual wielding or automatic ammo collection complete. A BR target rejects the corresponding secondary request. [Admission/request](../analysis/multiplayer/integration/inventory-source/controls/instructions/00153090.asm), [ownership dispatcher](../analysis/multiplayer/integration/inventory-source/controls/instructions/000ce510.asm).

The allocated cases first create the initial SMG from actual Lockout row 12 with complete `15C110→B8A30→16D2C0`, then carry it through original `CE510`. They then allocate an actual BR from row 0 or 1 through that same unchanged path. Source identities are **00010002** and **00020003**, respectively: the second salt increments. Original generated controls carry the entire second datum through targeting, request type 20, `EA660`, `CE510`, and the inventory slot. The target does not become a replacement definition. [Source allocation](../analysis/multiplayer/integration/inventory-source/controls/instructions/000b8a30.asm), [source datum allocation](../analysis/multiplayer/integration/inventory-source/controls/instructions/0016d2c0.asm), [pickup callback](../analysis/multiplayer/integration/inventory-source/controls/instructions/000ea660.asm).

The carried BR remains datum `00020003`, definition `E192001D`, ammo 36/72, heat 0, and originating row byte `+AF` 0 or 1. Its row's runtime entry still references that same datum. Original carry changes owner to the unit and world scale 1, unregistering its world position. Original mode-4 replacement drops the allocated initial SMG, preserves the allocated BR identity and ammo, and consumes the same four shared-RNG draws already recovered for the controlled drop path. The ordinary mode-3 pickups consume no additional shared RNG. [Carry/removal ownership](../analysis/multiplayer/integration/inventory-source/controls/instructions/000cfed0.asm), [drop/removal](../analysis/multiplayer/integration/inventory-source/controls/instructions/000cfd80.asm), [allocated case states](../analysis/multiplayer/integration/inventory-source/controls/report.json).

The original simulation order in `139750` runs players before netgame timers and object stages. After each allocated pickup the report executes full `15C4B0` at the same eligible row phase. The row still owns its carried datum; held-item flags start the source 45-second timer and decrement it on that visit, producing timer **44**. Pickup does not clear the map-row datum or allocate another weapon. The older [netgame research](MP_INVENTORY_RESEARCH.md#authored-netgame-weapons-timing-and-identities) owns the rest of the respawn contract. [Timer stage](../analysis/multiplayer/integration/inventory-source/controls/instructions/0015c4b0.asm), [source stage order](../analysis/map-pack-update/functions/00139750.c).

## Short secondary release and unit switch request

After carrying SMG slot 0 and BR slot 1, a short logical-button-3 press and release sets the generated selector/transient `+1C bit0`. `1884E0` feeds that event into full `CF300`, which finds the other carried slot and excludes the current other hand. The selected primary byte becomes 0. The outgoing action's inventory sequence still matches the unit, so `151C90→C8140` writes requested slot 0 while current remains slot 1. This is a slot request, not immediate slot replacement. [Slot chooser](../analysis/multiplayer/integration/inventory-source/controls/instructions/000cf300.asm), [control generation](../analysis/multiplayer/integration/inventory-source/controls/instructions/001884e0.asm), [requested slot admission](../analysis/multiplayer/integration/inventory-source/controls/instructions/000c8140.asm).

The report then executes original unit switch-request fragment `C6D6B..C6E19` (exclusive stop). The ordinary branch requires definition `+BC bit10` clear and unit `+10A bit2` clear. It updates the ready timestamp for each current hand and emits type **8** for a differing primary requested/current slot, or **18** for the other hand. The request has immediate byte `+4=0` and silent byte `+5=0`. Original `E9FE0` and its callbacks own the eventual transition. The supplied unavailable animation-transition service makes this recorded case ready slot 0 immediately; that result must not be converted into an unconditional native timing rule. [Unit request phase](../analysis/multiplayer/integration/inventory-source/controls/instructions/000c6c50.asm), [request assembly](../analysis/multiplayer/integration/inventory-source/controls/instructions/000e7ef0.asm), [switch handler](../analysis/multiplayer/integration/inventory-source/controls/instructions/000e9fe0.asm).

## Explicit limits

- Hardware packets and elapsed polling milliseconds are supplied at the original XInput service. Native key/controller bindings and the complete input-frame scheduler are not recovered.
- One local player and an enabled controlled-unit flag are runtime fixtures. The full biped constructor, local player lifecycle, client/server ownership transport, and auxiliary player phase are excluded.
- Earlier target cases retain their supplied spatial list, radius and visibility. Allocated cases use real BR world allocation/bounds and actual biped radius 0.44, with a supplied nearby unit root-node pose and registry center. Their spatial list and pickup visibility remain supplied; no live look-ray or terrain-visibility claim is added.
- The world allocator executes original registration against actual Lockout BSP data. Havok `246A50` remains supplied unavailable, and no complete settling/physics stage is claimed.
- Animation/marker availability is an explicit existing service boundary. Switch request ownership is recovered; complete switch animation timing is not.
- Replacement with an active local user needs HUD message backing storage. The tool supplies that storage and executes original zeroing fragment `1931B6..1931BF` and original message-slot clear `252F4D`; original `252CF2` then sees empty slots. This is an empty HUD lifecycle fixture, not a suppressed source callback or HUD display implementation. [HUD allocation size](../analysis/multiplayer/integration/inventory-source/controls/instructions/002529b7.asm), [source zeroing](../analysis/multiplayer/integration/inventory-source/controls/instructions/0019316e.asm), [message-slot clearing](../analysis/multiplayer/integration/inventory-source/controls/instructions/00252f4d.asm), [drop's message update](../analysis/multiplayer/integration/inventory-source/controls/instructions/00252cf2.asm).

The next unresolved admission layer is real runtime spatial enumeration and visibility tied to the player's complete world/unit phase, followed by animation resource/transition timing. The recovered held-threshold action and stable identity contract are usable independently of those unresolved services.
