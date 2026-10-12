# Keyboard action delivery and the next live inventory unit

The keyboard bindings already recognize Tab/wheel switch, F melee and Q grenade. Their queued requests are discarded when [runtime delivery](../src/multiplayer_runtime.cpp#L727) reads only `keyboard.consume(...).controls`. `Controls` and the LAN command byte represent jump/crouch/fire/reload, with no representation for these three actions. This is a real delivery gap, but forwarding the flags alone cannot implement their gameplay owners.

No native switch, melee or grenade-throw consumer exists in the inspected `src/`/`include/` files. Current live `Unit` retains one `WeaponState`; spawn constructs one starting weapon and one salted weapon datum. It has no spare carried weapon, four-slot bank, requested/current hand selectors or unit action/animation channels. Existing grenade counts and current/requested type cover selection, not throw. No new disconnected helper or production edit is made by this review.

## Available native calls and their useful boundaries

| Native function | Useful live seam | What it does not own |
|---|---|---|
| `MultiplayerKbm::consume` | Delivers one queued Tab/wheel/F/Q pulse at a simulation step. | Gameplay admission, action progression or LAN serialization. |
| `produce_source_player_control` | Exact delivered5C→7C packet, including sequence/slot bytes and action flags. | Choosing the next slot, device codec or current action state. |
| `apply_source_unit_input` | C8140 copies current-sequence-compatible requested slots to unit+216/+217. | Immediate current-slot mutation or switch acceptance. |
| `source_inventory_empty_slot` | Literal CEA70 empty-slot scan. | Selecting a switch target. |
| `set_source_inventory_slot` / `advance_source_inventory_sequence` | CFED0/CFE60 slot insertion/removal/sequence order with actual owner/world services. | Switch handling, owner/attachment/world success or second-item creation. |
| `smg_initial_equip`, `smg_input`, `smg_update` | Existing bounded single-SMG owner state; `blocking_ticks` represents weapon+17C. | Unit put-away/ready channel, generic BR state or current/requested slot lifecycle. |
| `starting_grenades`, `grenade_ready_type` | Actual starting counts and bounded CD310 direction0 selection. | Throw admission, count debit, release marker, projectile or explosion. |

These APIs are in [player_command.hpp](../include/halo2/player_command.hpp), [unit_input.hpp](../include/halo2/unit_input.hpp), [unit_inventory.hpp](../include/halo2/unit_inventory.hpp), [smg.hpp](../include/halo2/smg.hpp) and [inventory.hpp](../include/halo2/inventory.hpp). The slot orchestration's owner/world/drop leaves remain explicit services: [native slot contract](MP_UNIT_INVENTORY_NATIVE_INTEGRATION.md).

## Minimum next integrated weapon-switch unit

The next useful implementation is a **persistent source-owned inventory and action/animation state on each live unit**, connected to current match ownership and current animation updates. It is not another pure selector with no live caller. Required state must originate in the source constructor/loadout/carry producers rather than assigning an apparent ready manager or extra weapon:

| Retained owner | Required original fields |
|---|---|
| Inventory | Four full salted datums at unit+218..224, with independently retained item definition, ammo, heat and own state for every occupied slot. |
| Hand selection | Signed current+212/+213 and requested+216/+217 bytes. Current/requested remain distinct throughout an asynchronous switch. |
| Sequence | u16+210/+214. C8140's equality gate preserves a new carry selection against stale incoming command bytes. |
| Ready timestamps | Four u32 clock tick words at+228..234. These are timestamps, not duration/countdown values. C6C50 updates the current slot's timestamp before requesting a switch. |
| Unit action component | Signed unit+346 resolves the control component; active action words are component+4/+8. Switch primary bit8/other-hand bit18 belong here. |
| Primary manager | Signed unit+12A resolves the90-byte manager, including actual graph/weapon class/name/activity/current channel and clocks. |
| Secondary channel | Signed unit+33E resolves the extension; switch secondary channel is extension+9C,32 bytes. Preserve actual packed clip, flags and frame state. |
| Item transitions | Each weapon's own animation state+178, countdown+17C, own selected index+17E, input+16E and flags+16C; lower original1042E0/1040F0/weapon update and attachment ownership. |
| Admission | Actual112270 manager admission, unit+134 bit25, E8610 action state and CEAF0 target capability; actual definition+BC/object+10A update gates. |

Only the final currently equipped definition/ammo needs to remain the public HUD/render selection; the retained objects must survive switching. Mutating `Unit::weapon.definition`, calling generic `Simulation::equip` or rebuilding ammo from metadata would replace carried identity/state.

The proven source request path is `CF300 selector → command+1E sequence/slot record → 151C90 → C8140 → C6C50(type8) → E9FE0/E9B40`, then the original action/weapon/channel updates reach `E9D50`. CF300 consumes actual slot existence, CEAF0 capability, other-hand exclusion, authored weapon flag+12C bit3 and per-slot ready timestamps; a naive two-slot toggle is not its contract. The full original switch request has immediate/silent bytes+4/+5 both zero. Directly setting current slot or always forcing an immediate request omits the measured asynchronous path.

The current live starting loadout contains one SMG. Meaningful Tab/wheel switching needs an actual second carried item first. For the stock Lockout case, the source evidence is an allocated BR from authored row0/1 carried through the original pickup path, preserving its original salted datum. Native world-item creation/current state/target admission/carry and the new retained Unit state must connect before switching can visibly alternate those weapons. No second weapon is granted by this review. Its required carried-state seam can be developed separately from world settling, but that separate fixture cannot be presented as a live player acquiring the item.

## Immutable switch fixture and its first five phases

The useful existing proof is [MP_PICKUP_EQUIPPED_RESEARCH.md](MP_PICKUP_EQUIPPED_RESEARCH.md), specifically `report.json /cases/8`, name `allocated_br_short_other_button_switch_to_smg`. The source-allocated SMG is00010002 and BR00020003; unit00010001 retains both slots. After short release, current primary is1 and requested is0; both inventory sequences are2. This fixture has explicit arena/player/device/world service boundaries and supplied continuation ticks. It does not establish a complete admitted live frame.

The original bounded continuation order is `1D0ED0 manager blend → C6C50 unit control → 1021D0 SMG then BR in allocator order → E7E60 actions → 112E10 unit request/animation → BDF80 pose`. The original canonical current/current request builder supplies the animation request. No native visual shot/reload timestamp substitutes for this order.

| `/cases/8/continuation/n/after` | Tick input | Current/requested | Action word at actual component+4 | BR+17C | Secondary clip/frame bits |
|---|---:|---|---|---:|---|
|0|2|1/0|`00000100`|3|206 put_away / `00000000`|
|1|3|1/0|`00000100`|2|206 / `00000000`|
|2|4|1/0|`00000100`|1|206 / `3F800000`|
|3|5|1/0|`00000100`|0|206 / `40000000`|
|4|6|0/0|`00000000`|4|207 ready / `00000000`|

At phase4 the SMG+17C becomes15. All five phases retain both object datums and their recorded ammo. The primary manager's current weapon name changes BR→SMG at phase4. Current-slot timestamps also update; at phase4 they are6/6. These are **observed source states under the listed bounded order**, not a universal five-tick switch duration. Channel and own-weapon producers decide completion.

Use the exact fields `after.manager.unit_control_hex`, `manager_hex`, `secondary_channel_hex`, `secondary_frame_bits`, and each weapon's `raw_hex`/`weapon_ready_ticks`. **Do not use `after.action_bits` as the live component**: that older convenience capture reads fixed unit+704 and remains zero. The actual source gate resolves signed+346; in this fixture it is3B7C and its component+4 carries bit8. This clarification does not modify the immutable capture.

| Existing immutable artifact | SHA256 |
|---|---|
| [Original switch/carry oracle](../tools/oracle_mp_inventory_equipped_1_1.py) | `47f56a9236ea2efe870d1ad82af16ace214f4598294f8c826966cced94dee655` |
| [Original report](../analysis/multiplayer/integration/inventory-source/equipped-pickup/report.json) | `d6cec76de0a155f2c5817b61d8073d4555ddb70efa48e731b349fd8ff742c00b` |
| [Original audit](../analysis/multiplayer/integration/inventory-source/equipped-pickup/audit.json) | `594183300f7d0c88ea4d180a1188587848775d74de86705cb59629b86cb5f3a4` |
| [Independent reproduction](../analysis/multiplayer/integration/inventory-source/equipped-pickup/reproducibility.json) | `97ffc47f4acd0cd471a14a58769d56ba65e9537d9a90815c6ede6e2e6a5eb53e` |

The original TU1.1 action table0046EC68 maps types8/18 through descriptor0046EA24 to E9FE0/EA180/EA370/EA270 (begin/update/finish/interruption). Existing full source listings remain under `equipped-pickup/instructions/`. The table/observations above are explicitly TU1.1; the newer command producer's TU1.5 correspondence does not silently establish every switch handler's TU1.5 mapping or a TU1.5 owning animation runtime.

## F melee and Q throw

F is presently a host melee request with no native gameplay consumer. The original TU1.1 table has distinct type26 descriptor0046EAA4/EAE90 and type27 descriptor0046EAB4/EB530/EB700/EB830. EB530 reads actual current weapon/definition flags, unit+134 bit25, action bits22/27/29, and the action component's stored progress before reaching EB080 and target/effect consumers. Existing weapon error gates and imported melee balance tags are not an executed melee hit: the live `WeaponDefinition` carries firing damage, with no retained melee action/target/contact/event contract. Source target admission, authored melee action/clock/event, actual current collision matrices/contact, damage owner/region/effect delivery must connect before F damages a player. A firing ray with an invented range or immediate application of the processed strike damage is not authorized by the current evidence.

Q is presently a host throw request with no native gameplay consumer. The source table's type22 descriptor0046EA64 points to E95E0/E99B0/NULL/E9A50 (begin/update/no finish/interruption). The source selection proof establishes current/requested types and counts, plus112270's gate for known idle/fire/reload states; it does not establish throw admission or release. The required next owners are the actual throw component/channel progression, count-store timing, hand/marker release point, grenade definition/projectile allocation/velocity/current state and fuse/bounce/detonation/radius damage. Native `Simulation::Projectile` represents the existing firing path and does not provide that grenade lifecycle. Decrementing the count and spawning a generic ballistic projectile on Q would invent those producers.

Runtime must retain the whole `MultiplayerKbmInput` until simulation delivery, forward its action pulses through a per-player request contract, and serialize the corresponding command/state if used over LAN. The current LAN command stores only four action booleans and the state snapshot retains one active weapon. Those are root-owned integration edits. This note neither implements a positive action with absent fields nor changes any frozen fixture. It gives the next complete integration unit: genuine retained inventory plus the source current/requested/timestamp/component/manager state and its owning schedule, followed by a real carried second object and the ordinary switch callback chain. F/Q need their separate positive producers after that shared owner foundation.
