# Stock SMG effect request, actual storage and visibility

The original TU1.1 stock SMG firing path now executes real effect storage, allocation, its current world muzzle marker, effect-location allocation and visibility retirement on the independently repeated allocated player/item world. The separate native module ports the exact request/object/visibility consumers. It does **not** turn a shot notification into a flash: generic effect allocation, event/particle ownership and rendering remain explicit services.

The owning source is the unchanged [TU1.1 executable](../assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03), SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`, and [Lockout cache](../assets/disc/maps/lockout.map), SHA-256 `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`. The [oracle](../tools/oracle_smg_effect_owner_1_1.py) verifies every field of the immutable [active-item report](../analysis/multiplayer/integration/inventory-source/item-active-schedule/report.json) before extending it. Only two already-delivered command words request firing; local-device/session/readiness admission is not asserted.

## Actual preparation and owning requests

[Original107160](../analysis/multiplayer/integration/inventory-source/smg-effect-owner/instructions/00107160.asm) calls `107B50` during successful prepared-shot handling, before the projectile postpass. Stock barrel flags do not select its alternative projected-marker branch. The unchanged authored `weap E57B0406` normal firing entry selects `effe E59E0429`, `effects\objects\weapons\rifle\smg\fire_bullet`. Its one effect-location StringID is `0F0000DB`, `primary_trigger`; the world and first-person marker group bytes are preserved in the [measured report](../analysis/multiplayer/integration/inventory-source/smg-effect-owner/report.json).

The effect has one event with zero delay and duration words `3D088889/3D088889`, one SMG sound part, and four authored particle systems: first-person `fire_burst_center`, third-person `fire_burst_long`, third-person `muzzle_smoke`, and third-person `fire_burst_center`. These are authored system camera-mode values, not a native choice of particles. The event duration does not establish an individual particle's lifetime. All six emitters and their complete property/curve bytes are retained.

[Original106070](../analysis/multiplayer/integration/inventory-source/smg-effect-owner/instructions/00106070.asm) chooses the actual weapon unless header+02 bit4, body+04 bit0 or definition+38 NONE selects its retained body+14 parent. [Original107B50](../analysis/multiplayer/integration/inventory-source/smg-effect-owner/instructions/00107b50.asm) separately reads the original weapon's body+12C bit0 and body+154 owner for the sound branch. Effect NONE returns NONE; an actual `effe` calls `17CBD0` on the old handle, then constructs a new request if that refresh fails. `17CBD0` with old NONE returns false without requiring a fabricated pool.

[Original1781F0](../analysis/multiplayer/integration/inventory-source/smg-effect-owner/instructions/001781f0.asm) clears its whole60-byte packet and stores:

| Packet field | Actual source value/owner |
| --- | --- |
| +00 | 5 |
| +04/+08 | effect datum/selected full object datum |
| +0C/+10 | NONE/NONE |
| +14/+18 | ushort FFFF/FFFF; surrounding bytes remain zero |
| +1C/+20 | original direction-entry address/count2 |
| +40/+44 | caller scalar words from XMM0/stack |
| +54/+58 | FF808080/FF808080 |

[Original177DA0](../analysis/multiplayer/integration/inventory-source/smg-effect-owner/instructions/00177da0.asm) fills two1C-byte entries from the actual `BACC0` object center, original46FC5C down vector and46FC50 up vector, with marker IDs `070000C0`/`020000CA`. The source center is a required current-matrix service, not a projectile origin or guessed muzzle position.

## Real storage, marker and negative visibility

Full `1775D0→175860` creates the effect/location/particle-system/particle/emitter/particle-location pools using the existing source game arena/default allocator; full `177690` resets them. No pool or allocation success is supplied. Generic `179B90` creates actual `E6650000`; world `B9C20→1DC8F0/1DC940` finds `primary_trigger`, and `17A030` allocates its actual location. The observer records all occupied records, pool headers/bitmaps, source arithmetic inputs, instructions, reads and ordered stores.

[Whole17B1A0](../analysis/multiplayer/integration/inventory-source/smg-effect-owner/instructions/0017b1a0.asm) uses effect+20 signed cluster, optional `3EB10`/518440+5 bypasses, then definition bit2 to choose game+1138 or game+1178 PVS. Stock effect flags2 have bit2 clear, so `11D620` uses the **local-view** mask1178. The genuine nonlocal fixture has player+28=-1 and empty1178. The source sets effect+02 bit3, executes real `179080/178F60` retirement, and returns NONE from creation. No event/birth/render success is inferred. The new local-view extension preserves this complete negative case rather than changing its masks.

## Native consumer and validation boundary

[weapon_effect.hpp](../include/halo2/weapon_effect.hpp), [weapon_effect.cpp](../src/weapon_effect.cpp), and [focused tests](../tests/weapon_effect_tests.cpp) expose actual object/tag/center/retained-effect refresh/create/sound services. Missing required owners throw. The complete visibility consumer mutates literal190-byte effect storage and requires real retirement when admitted. Valid source clusters0–511/NONE are supported; arbitrary corrupt storage is not promoted to a source path.

The [fixture exporter](../tools/export_smg_effect_fixtures.py) emits only original packet/body/PVS/callback inputs and returns. The measured create/retire results are explicit lower-service boundaries in the comparison harness. The two measured native roots compare all60 request bytes,38 direction bytes,190 effect exit bytes, source retirement entry and return. Normal and ASan/UBSan runs are in [validation](../analysis/multiplayer/integration/inventory-source/smg-effect-owner/validation/report.json). This validates these consumers, not a native allocator or visible flash.

[Complete-body correspondence](../analysis/multiplayer/integration/inventory-source/smg-effect-owner/u5-correspondence/report.json) independently establishes TU1.5 counterparts: `107B50→108A70`, `106070→106F90`, `17CBD0→17DFF0`, `1781F0→179610`, `177DA0→1791C0`, `17B1A0→17C5C0`. It compares every normalized instruction/internal branch and records full listings and literal operands. Calls outside those six owners and actual TU1.5 effect execution remain separate boundaries.

The source report and every instruction file reproduce byte-identically in [independent-repeat](../analysis/multiplayer/integration/inventory-source/smg-effect-owner/independent-repeat/report.json). [Manifest](../analysis/multiplayer/integration/inventory-source/smg-effect-owner/manifest.json) pins this freeze and all prior evidence. No prior suite or shared runtime/CMake/render/shader file changes.

Root registration additions are source `src/weapon_effect.cpp`, executable `halo2_weapon_effect_tests` from `tests/weapon_effect_tests.cpp` linked to `halo2_native`, and CTest fixture argument `analysis/multiplayer/integration/inventory-source/smg-effect-owner/native-fixtures/fixtures.txt`. Live callers must retain authored effect handle, actual source attachment/current marker state, local/all-player visibility and generic event/particle lifecycle. Existing `scene_particles` has no multiplayer caller; the module deliberately does not supply those owners.
