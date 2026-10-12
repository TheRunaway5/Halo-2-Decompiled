# Original 1.1 multiplayer inventory

This note recovers an initial loadout and bounded weapon ownership/action path
from the untouched Xbox 1.1 executable. It establishes the carried-slot state,
ammunition transfer, switch ordering and a shared-RNG drop path. The fixture
supplies animation and world services; it does not establish their real timing,
marker transforms, object allocation, spatial enumeration or network delivery.
Those boundaries must remain explicit in a native implementation.

The later equipment trace proves source timers, allocation request descriptors
and stable world/carried identities for actual Lockout SMG/BR rows. The target
trace establishes weapon proximity, candidate ranking, source radius ownership,
salted identity admission and pickup request assembly. Physical allocation,
settling, spatial enumeration and collision results remain supplied services.

## Authority and reproduction

The executable is the user's `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`,
SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
The executed metadata is the user's `assets/disc/maps/lockout.map`, SHA-256
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
[`oracle_mp_inventory_1_1.py`](../tools/oracle_mp_inventory_1_1.py) maps original
section and cache bytes, pins both hashes, and executes the original instructions
without patching their bodies. The saved instruction listings include source
byte hashes and distinguish listed instructions from instructions actually run.

```sh
for part in loadout loadout-created slots pickup actions ammo maps grenades choices grenade-selection player-cap grenade-gate variant-fields; do
  .tools/python/bin/python tools/oracle_mp_inventory_1_1.py --experiment "$part" --output "analysis/multiplayer/integration/inventory-source/$part.json"
done
```

The reports reside in the ignored
[`inventory-source`](../analysis/multiplayer/integration/inventory-source) directory.
Original images, map data and extracted instruction listings remain local.
The thirteen reports contain 370 direct routine/fragment rows, 48 grenade-option
cases, 99 grenade-gate cases, 20 roster-cap cases, four ammunition-transfer cases,
149 weapon-selector cases and authored placement records for all 21 maps. These
counts describe bounded evidence; they are not complete match coverage.
The original 48-case `grenades.json` is frozen at its recorded tool hash; later
reports record the extended tool's hash. All offsets below are hexadecimal.

## State ownership

The original handler dispatch table at `0046EC68` maps types 8/18 to
`000E9FE0` (switch), `000EA180` (update), `000EA370` (finished) and `000EA270`
(interrupted). Types 9/19 use `000EA440` (drop); type 20 uses `000EA660`
(pickup). This was read directly from the mapped executable table, rather than
assuming the base executable's addresses survived the update.
[`dispatch`](../analysis/multiplayer/integration/inventory-source/instructions/000e7f30.asm)

| Original location | Representation and demonstrated use |
| --- | --- |
| unit `+212/+213` | Signed bytes: current primary/other hand slots; `-1` is absent |
| unit `+216/+217` | Signed bytes: requested primary/other hand slots |
| unit `+218` | Four object datums, stride 4; absent datum is `FFFFFFFF` |
| unit `+228` | Four per-slot timestamps, stride 4; written from clock tick on ready |
| unit `+210/+214` | u16 inventory sequence/current replicated sequence |
| weapon `+154` | Owner unit datum; `FFFFFFFF` when unowned |
| item/weapon `+12C` | Item flags: bit 0 is in inventory; other demonstrated mutations below |
| weapon `+224` | Runtime magazine records, stride `10` |
| magazine `+6/+8` | Signed shorts: reserve/loaded rounds |
| unit `+23C/+23D` | Signed bytes: current/requested grenade type; `-1` is absent |
| unit `+23E/+23F` | Byte frag/plasma counts |

These are operands in original
[`slot setter`](../analysis/multiplayer/integration/inventory-source/instructions/000cfed0.asm),
[`ownership setter`](../analysis/multiplayer/integration/inventory-source/instructions/000fb120.asm),
[`ready transition`](../analysis/multiplayer/integration/inventory-source/instructions/000e9d50.asm),
[`creation`](../analysis/multiplayer/integration/inventory-source/instructions/00101bb0.asm),
and [`grenade initialization`](../analysis/multiplayer/integration/inventory-source/instructions/0015ffd0.asm).
The current/requested distinction is observable in the executed switch trace;
a native enum naming one current weapon cannot represent this state.

`000CEA70` returns the first empty slot by increasing index, or signed short
`-1` when all four are occupied. All five occupancy cases execute that routine.
`000CFED0` first sets weapon ownership through `000FB120`, then puts the weapon
away through `000FB3F0`, then writes the chosen slot and zeroes its ready tick.
It finally updates inventory sequence through `000CFE60`. Outside option mode 4,
the latter increments modulo 16 and writes the same value at `+210` and `+214`.
The tested attachment gives flags `7` and the actual owner datum.
[`slots.json`](../analysis/multiplayer/integration/inventory-source/slots.json),
[`pickup.json`](../analysis/multiplayer/integration/inventory-source/pickup.json),
[`sequence`](../analysis/multiplayer/integration/inventory-source/instructions/000cfe60.asm)

## Initial loadout and map placements

The metadata reader contract is:

| Data | Serialized layout/read |
| --- | --- |
| scenario starting equipment | block `scnr+128`: count u32 `+0`, original pointer u32 `+4`; record stride `9C` |
| starting equipment flags | u32 record `+0`; bit 0 No Grenades, bit 1 Plasma Grenades |
| starting equipment mode selectors | four i16 values record `+4/+6/+8/+A` |
| starting collections | six tag references, datums record `+40/+48/+50/+58/+60/+68`; original routine scans first five |
| item collection entries | block `itmc+0`, stride `10`: f32 weight `+0`, item datum `+8`, item variant StringID `+C` |
| item collection spawn time | i16 `itmc+8` |
| authored multiplayer globals | `matg+16C` datum → `mulg+8` runtime block, stride `568` |
| multiplayer weapon references | runtime `+68` block, stride 8; each datum `+4` |
| primary/secondary variants | serialized parameters `59/5E`; variant i8 `+D6/+D7`; options i8 `+212/+213` |
| starting grenades variant | serialized parameter `58`; variant u32 flags `+48` bit 11; options u32 `+184` bit 11 |

Resolve cache pointers with the cache's actual magic and datums through its tag
table. A NONE datum is `FFFFFFFF`; neither a collection tag nor its list index
is a weapon object identity. Preserve the serialized item variant separately.
The grenade cap input is runtime game-engine state, described below; it is not
a field in authored `mulg` metadata.
[`scenario schema`](../tools/reference/assembly/src/Assembly/Plugins/Halo2/scnr.xml),
[`collection schema`](../tools/reference/assembly/src/Assembly/Plugins/Halo2/itmc.xml),
[`multiplayer schema`](../tools/reference/assembly/src/Assembly/Plugins/Halo2/mulg.xml),
[`serialized parameter names`](../tools/reference/assembly/src/Assembly/Plugins/Halo2/sily.xml)

The TU descriptor table `00477D10` directly proves the variant storage contract:
ID `58` invokes `00239436` and returns type 1, offset `48`, bit 11;
IDs `59/5E` invoke `00239457/0023947E` and return type 3, offsets `D6/D7`,
scale 1. All three complete descriptor bodies execute without supplied services.
[`variant-fields.json`](../analysis/multiplayer/integration/inventory-source/variant-fields.json)

TU `001A28A0` is the base `0019EF40` loadout routine. It finds the first matching
scenario starting-equipment record, empties existing inventory through `000CE440`,
scans the first five of six serialized collection references for up to two
distinct definitions, resolves variant choices through `001A2700`, then requests
objects in primary/secondary order. Each successful allocation is checked for
an already carried definition by `000CEA10`; a duplicate is deleted, otherwise
`000CE510` receives mode 1. The collection chooser `001A2B00` selects a single
item without drawing random numbers. Multiple-item weighting and random variant
choices remain separate consumers; they must not be replaced by picking a first
item generally.
[`initializer`](../analysis/multiplayer/integration/inventory-source/instructions/001a28a0.asm),
[`choice resolver`](../analysis/multiplayer/integration/inventory-source/instructions/001a2700.asm),
[`collection chooser`](../analysis/multiplayer/integration/inventory-source/instructions/001a2b00.asm)

`0015B6D0` compares the four selectors in order and the initializer uses the
first matching record. With a game-engine object, a selector matches the engine
mode, modes 1 and 9 are interchangeable here, selector `C` always matches, and
selectors `D/E` match modes other than 1/9. With no engine object, only selector
zero matches. No match returns before inventory removal or count-input changes.
All authored multiplayer starts contain `[C,0,0,0]`.

Variant primary choice 2 resolves secondary first, then random primary excluding
that definition. Other primary choices resolve primary first, then secondary
excluding it. Choice 0 keeps the selected map definition; choice 1 returns NONE;
choice 2 advances the shared LCG and selects `3 + (new_seed >> 28)`, retrying
NONE or the supplied excluded definition. Choices `3` through `14` address the
following `mulg` weapon-reference indices, rather than an enum cast:

| Variant choice | Runtime index | Lockout referenced weapon |
| --- | --- | --- |
| 3 | 3 | Battle Rifle |
| 4 | 0 | Magnum |
| 5 | 1 | SMG |
| 6 | 6 | Plasma Pistol |
| 7 | 7 | Plasma Rifle |
| 8 | 5 | Rocket Launcher |
| 9 | 2 | Shotgun |
| A | 4 | Sniper Rifle |
| B | C | Brute Shot |
| C | 9 | Needler |
| D | A | Carbine |
| E | D | Beam Rifle |
| F | B | NONE |
| 10 | F | Rocket Launcher |
| 11 | E | Energy Sword |
| 12 | 8 | Brute Plasma Rifle |
| 13 | 10 | Sentinel Beam |
| 14 | 11 | NONE |

The random pool ends at choice `12`; it excludes the last two choices. The
21 direct choices and 128 random choices excluding SMG execute the original
resolver. The random cases contain 151 actual seed stores because retries
consume the shared sequence too.
[`choices.json`](../analysis/multiplayer/integration/inventory-source/choices.json)

`loadout.json` records the original Lockout SMG request with allocation supplied
as failure. `loadout-created.json` supplies an object previously initialized by
the complete original weapon callback `00101BB0`, then executes the initializer's
actual ownership/attachment branch. Result: slot 0, current/requested primary 0,
no other hand, 60 loaded/120 reserve and grenade inputs 2/0. Both keep the supplied
shared seed `12345678`. The allocator itself is excluded from this evidence.
The same original callback initializes Lockout's Battle Rifle to 36 loaded/72
reserve in the action report.
[`loadout-created.json`](../analysis/multiplayer/integration/inventory-source/loadout-created.json),
[`actions.json`](../analysis/multiplayer/integration/inventory-source/actions.json)

All 21 multiplayer caches have one starting-equipment record with selectors
`[C,0,0,0]`, one positive-weight item in its first collection and no secondary
collection. Midship and Waterworks start with a plasma rifle; the other 19 start
with an SMG. Starting-equipment flags differ: Midship and Gemini have bit 1
(Plasma Grenades); Backwash has bit 0 (No Grenades); all other maps have zero.
With ordinary starting-grenade options and per-type maximum 2 this means Gemini
gets 0 frag/2 plasma, Backwash 0/0, Midship 0/2, and the other maps 2/0. These
are authored metadata plus the demonstrated initializer operands, not a live
execution of all maps' object-spawn services.
[`maps.json`](../analysis/multiplayer/integration/inventory-source/maps.json),
[`scenario schema`](../tools/reference/assembly/src/Assembly/Plugins/Halo2/scnr.xml)

TU `0015FFD0` initializes the grenade inputs to 2 frag/0 plasma, or 1/0 when
runtime game-engine globals byte 0 bit 3 is set, invokes `001A28A0`, then applies
starting grenades option `options+184` bit 11. Weapon-set option `options+210`
decimal values 15/17 move counts to plasma, while value 16 moves them to frag. It caps each type to
the selected maximum and writes the actual unit count bytes. All 48 combinations
of scenario flags 0/1/2, enabled/disabled, cap 1/2 and set 0/15/16/17 execute the
full original routine and satisfy the checked count rule. Modified flags/options
are labeled fixture inputs, not edits to original game files.
[`grenades.json`](../analysis/multiplayer/integration/inventory-source/grenades.json),
[`0015FFD0`](../analysis/multiplayer/integration/inventory-source/instructions/0015ffd0.asm)

The runtime pointer is `004F10B8`. Original `0015A460` counts entries through
`001A2BA0`/`0016D680`: only allocated player-pool bitmap entries are visited,
and entries with player byte `+2` bit 1 set are skipped. It ORs flag bit 1 for
eligible count greater than 6, bit 2 for at least 5, and bit 3 for at least 9.
It never clears these bits. Twenty executed cases include all counts 0..16,
excluded records and a preexisting bit 3 with only two eligible entries.
[`player-cap.json`](../analysis/multiplayer/integration/inventory-source/player-cap.json)

This is an engine-initialization cache. Original `00159BD0` ends with tail jump
`00159DE3 → 0015A460`, after the engine's virtual callback `+30`. Its direct
caller is `00190A30` at `00190A8E`, on the successful lifecycle-start path; the
callers of `00190A30` are `00190130` and `001902E0`. Lifecycle table
`00447430 + 16*24` (entry 22 decimal) contains `00158C10`; that callback first zeros all `C9C`
bytes of game-engine globals. The reset prefix is executed in the report.
Supply the cached runtime bit as `maximum_one_grenade`; recomputing it from
the current connected roster on every spawn changes the original timing.
[`lifecycle reset`](../analysis/multiplayer/integration/inventory-source/instructions/00158c10.asm),
[`engine init`](../analysis/multiplayer/integration/inventory-source/instructions/00159bd0.asm),
[`lifecycle start`](../analysis/multiplayer/integration/inventory-source/instructions/00190a30.asm)

Netgame equipment is scenario `+120`, stride `90`; its item-collection datum is
`+5C`, position `+40`, orientation `+4C`, mode selectors `+4..+A`, flags `+0`,
spawn-time `+E`, empty-respawn time `+10` and timer-start selector `+12`.
Every row, collection item, weight, datum and original file offset is recorded.
Lockout's SMG is authored row 12; its Battle Rifle rows are 0/1. No item placement
or spawn timer has been fabricated. The source semantics of spawn/respawn
lifecycle and pickup-proximity selection are not established by decoding these
rows.
[`maps.json`](../analysis/multiplayer/integration/inventory-source/maps.json),
[`scenario schema`](../tools/reference/assembly/src/Assembly/Plugins/Halo2/scnr.xml)

## Grenade selection phases

Unit creation `000C52C0` writes both current/requested type to NONE at
`+23C/+23D`. Grenade-count initialization does not select a type. The report
executes that constructor prefix, then the complete count initializer, then
original player-control selection `00188676..0018871D` (exclusive stop).
Starting with control short `+2C = NONE`, the latter scans grenade type 0 then 1
for positive counts. No-cycle input is explicitly zero in this fixture.
Original marshalling `00188A4D..00188A94` copies selected control `+2C` to output
short `+22`. Full `00151C90` converts it to the unit-control payload short `+A`
and invokes `000C8140`, which writes requested `+23D` only when the value is
not NONE. Unit-update fragment `000C6E19..000C6E54` applies current `+23C` only
when requested differs, original gate `00112270` returns false, and original
`000CD310` returns a type with positive count.

| Authored starting flags | After count init: frag/plasma; current/requested | First control selection | After unblocked unit update |
| --- | --- | --- | --- |
| Lockout `0` | `2/0; -1/-1` | `0` | current/requested `0/0` |
| Gemini `2` | `0/2; -1/-1` | `1` | current/requested `1/1` |
| Backwash `1` | `0/0; -1/-1` | `-1` | current/requested `-1/-1` |

The map flags are read from the actual named caches and supplied into the mapped
Lockout starting record; allocation is supplied failure. These are bounded
original phase traces, not complete map startup or network delivery. No phase
draws shared RNG in these cases.
[`grenade-selection.json`](../analysis/multiplayer/integration/inventory-source/grenade-selection.json)

The supported stock SMG phases do not require a fabricated busy result. The
original weapon selector fragment `00109AD5..00109B2A` is executed: state 0
selects idle `0400000C`, state 1 fire_1 `06000006`, and state 5 (magazine 0,
either partial or empty reload) reload_1 `08000002`. Original `00110800` resets
the unit overlay name to NONE. `00112270` then returns false for all 96 tested
combinations of those names, controlling-player present/absent, weapon `+17C`
signed status -1/0/1, animation interruptible bit off/on, and overlay NONE/same
name. Each original unit-update fragment applies requested type 1. Positive
controls enter, melee and melee overlay return true and defer current at NONE.
[`grenade-gate.json`](../analysis/multiplayer/integration/inventory-source/grenade-gate.json)

The gate reads the base animation name at `unit + i16(unit+12A) + 7C`, overlay
name at `unit + i16(unit+33E) + A4`, controlling-player datum `+13C`, current
hands and held weapons' signed `+17C` status, and animation flags `+14` bit 4.
Its sets contain enter/exit, melee, grenade throw and other actions, rather than
fire/reload. Do not generalize the demonstrated false result to those actions.
The full untouched predicate and helper tables are published; animation graph
population, frame events and transitions remain outside this oracle.
[`gate`](../analysis/multiplayer/integration/inventory-source/instructions/00112270.asm),
[`forced-state helper`](../analysis/multiplayer/integration/inventory-source/instructions/000c8410.asm),
[`name table helper`](../analysis/multiplayer/integration/inventory-source/instructions/000c83d0.asm)

## Pickup, switching and dropping

The pickup request record is type i32 `+0`, weapon datum i32 `+4`, mode i16 `+8`.
`000EA660` refuses an owned weapon (`weapon+154 != NONE`), asks original
`000CEC00` for four admission bytes, validates the mode, interrupts actions
8/18/0/10/27/22 in that order, then invokes `000CE510`. Handler modes 0/1 require
byte 0 and no current primary; mode 3 requires byte 0; mode 4 requires byte 2;
mode 5 requires byte 1; mode 6 requires byte 3. Other modes are rejected here,
including mode 2 although it exists in the lower dispatcher. The admission
routine reads actual carried definitions, authored flags/ammunition and current
hands; its four booleans must not be replaced by a generic two-weapon limit.
[`pickup handler`](../analysis/multiplayer/integration/inventory-source/instructions/000ea660.asm),
[`admission`](../analysis/multiplayer/integration/inventory-source/instructions/000cec00.asm)

The executed actions carry an SMG in slot 0 and Battle Rifle in slot 1. The
ordinary switch request is type 8, with immediate byte `+4` and silent byte
`+5` both zero, and requested primary already set to 1. With the supplied
put-away animation accepted, `000E9FE0` leaves current slot 0 and marks action
bit 8. When that supplied gate clears, `000EA180` reaches the original ready
transition: current slot becomes 1 and ready timestamp 47. Its return is false
(finished); the outer unit action updater owns clearing that active bit. The
fixture deliberately does not invent that outer updater after a direct callback.
The current slot cannot change merely because the request was accepted.
[`actions.json`](../analysis/multiplayer/integration/inventory-source/actions.json),
[`switch`](../analysis/multiplayer/integration/inventory-source/instructions/000e9fe0.asm),
[`switch update`](../analysis/multiplayer/integration/inventory-source/instructions/000ea180.asm)

Primary drop type 9 interrupts actions, puts the current weapon away, removes its
slot through `000CFD80`, and readies the next requested weapon. The executed
Battle Rifle drop restores current/requested slot 0, clears slot 1, writes owner
`NONE`, retains all 36 loaded/72 reserve rounds and increments inventory sequence
to 3. The actual type-20/mode-3 handler then admits and re-picks that dropped
object into slot 1. This trace executes the original ownership, admission and
drop-toss math under explicitly supplied marker/animation/collision services.
[`actions.json`](../analysis/multiplayer/integration/inventory-source/actions.json),
[`drop`](../analysis/multiplayer/integration/inventory-source/instructions/000ea440.asm),
[`removal`](../analysis/multiplayer/integration/inventory-source/instructions/000cfd80.asm)

The drop advances the *same global seed* used by firing, through stores at
`000CFB9E`, `00147AC8`, `00147B9E`, then `000FB8C6`:
`12345678 → 75432777 → CD305E6A → 25DBFAC1 → 4B5C952C`.
The first is toss magnitude selection, the middle pair the existing original
random-direction routine, and the last the item angular-velocity path. Report
rows retain exact velocity bits and seed writes. Four draws are specific to this
nondegenerate, clear-service fixture; branches and successful world services can
consume differently. Use the shared seed in original action/object order.
[`toss`](../analysis/multiplayer/integration/inventory-source/instructions/000cfb00.asm),
[`item velocity`](../analysis/multiplayer/integration/inventory-source/instructions/000fb620.asm),
[`actions.json`](../analysis/multiplayer/integration/inventory-source/actions.json)

## Same-definition ammunition transfer

`001058F0` receives the recipient in EAX and donor in ECX. If definitions differ
it does nothing. For each actual magazine it fills recipient reserve toward the
authored magazine `+C` cap, consuming donor reserve first, then donor loaded
rounds. Recipient loaded rounds are unchanged. It also orders the two weapon
`+184` values so the recipient retains the smaller one when donor is lower; that
field is named heat in the recovered weapon view. No random seed is read or
written by this transfer. The four executed SMG cases cover partial filling,
capacity, an already full recipient and donor-loaded-only supply; conserved ammo
and unchanged recipient loaded rounds are asserted.
[`ammo.json`](../analysis/multiplayer/integration/inventory-source/ammo.json),
[`transfer instructions`](../analysis/multiplayer/integration/inventory-source/instructions/001058f0.asm),
[`recovered weapon view`](../tools/reference/halo2-decompiled/src/weapons.cpp)

## Authored netgame weapons, timing and identities

[`oracle_mp_inventory_netgame_1_1.py`](../tools/oracle_mp_inventory_netgame_1_1.py)
executes complete untouched `0015C320`, `0015C370`, `0015C4B0`, `0015C110`,
`0015C290` and `0015C820` paths on the original Lockout rows. Its independent
[`report`](../analysis/multiplayer/integration/inventory-source/netgame/report.json)
contains 17 cases, 693 routine rows and 615 timer visits, including unchanged
non-row-phase visits, all four weapon timer options, stationary/moved items,
missing row objects, carried objects and allocation failure. Reproduce with:

```sh
.tools/python/bin/python tools/oracle_mp_inventory_netgame_1_1.py
```

The netgame block is scenario `+120` count, `+124` pointer, record stride `90`.
These authored fields are consumed by the recovered paths:

| Record offset | Representation and consumer |
| --- | --- |
| `+0` | u32 flags; bit 0 levitates unchanged collection choice on spawn; bit 1 may delete old non-weapon on respawn |
| `+4/+6/+8/+A` | Four signed mode selectors, checked by original `0015B6D0` |
| `+C` | Authored signed team field; the traced weapon timer/spawn path does not read it |
| `+E` | Signed seconds to respawn; nonzero overrides collection/default delay |
| `+10` | Signed empty-vehicle respawn seconds; vehicle branch only |
| `+12` | Signed timer-start selector: 0 carried/moved; 1 object `+10A` bit 2 |
| `+14` | Byte classification: 0 weapon; 1..7 vehicle override; 8/9 special vehicle mapping |
| `+40/+44/+48` | f32 position copied into original allocation descriptor `+1C/+20/+24` |
| `+4C/+50/+54` | f32 Euler angles converted by original `0011F680→00143450` into descriptor forward `+28` and up `+34` |
| `+58/+5C` | Collection group and datum (`itmc` or `vehc`); original `001A2B00` selects definition and variant |

Mode eligibility is the exact helper used by loadouts: selector equal to engine
mode, 1/9 interchangeable, C always eligible, D/E eligible outside modes 1/9.
With no engine, only selector 0 matches. `0015C4B0` skips options mode 4 entirely.
Each original row index is retained; filtering native-supported definitions
must not compact indices because the index controls cadence and object `+AF`.
[`eligibility`](../analysis/multiplayer/integration/inventory-source/netgame/instructions/0015b6d0.asm),
[`update`](../analysis/multiplayer/integration/inventory-source/netgame/instructions/0015c4b0.asm)

| Lockout row | File offset | Weapon definition | Delay | Flags | Authored position |
| --- | --- | --- | --- | --- | --- |
| 0 | `01D27E04` | BR `E192001D` | 45 | 1 | `[-3.4717299938201904,6.434720039367676,-7.949639797210693]` |
| 1 | `01D27E94` | BR `E192001D` | 45 | 0 | `[6.7680487632751465,-2.5917277336120605,-5.83750057220459]` |
| 12 | `01D284C4` | SMG `E57B0406` | 30 | 0 | `[-1.5056300163269043,10.647600173950195,-12.199000358581543]` |

All three have selectors `[C,0,0,0]`, classification 0, timer-start 0, empty
delay 0, and a one-item positive-weight collection whose root delay is 0.
BR rows use collection `E191001C`, item file offset `01D32DF0`; SMG row uses
`E57A0405`, item offset `01DE97D4`. Both weight 100 and variant SID 0. Exact
orientation floats/raw record bytes and allocation descriptor vectors/bits are
preserved in the report. These single-item selections and default weapon set
do not advance the shared RNG. The broader weighted/random collection path
does; preserve original row and game-stage order rather than supplying a
separate inventory seed.

Runtime globals `+7DC` hold 100 entries of **8 bytes**, not weapon definitions:
object datum i32 `+0`, signed countdown seconds i16 `+4`, empty-vehicle counter
i16 `+6`. `0015C320` zeroes 800 bytes and sets each actual authored row datum to
NONE outside options mode 4. `0015C370` runs outside mode 4 when rounds-reset-map
options `+184` bit 3 is set or runtime globals signed round `+6E` is 0. It deletes
an old bound object if present, sets datum NONE and countdown 1, empty counter 0.
The executed reset fixtures have no old bound object; deletion semantics are a
statically read boundary. Round initializer `00159180` calls this reset after
`0015F2A0`; engine initializer `00159BD0` calls `00159180` before setting running
state `+6C=1,+C04=1`. `0015C3F0` statically reconstructs row bindings from
existing object `+AF` after `0015C320`, unbinding duplicate/invalid rows.
[`reset`](../analysis/multiplayer/integration/inventory-source/netgame/instructions/0015c370.asm),
[`round initializer`](../analysis/multiplayer/integration/inventory-source/netgame/instructions/00159180.asm),
[`engine initializer`](../analysis/multiplayer/integration/inventory-source/netgame/instructions/00159bd0.asm)

`0015C4B0` is called every original game tick but visits row i only when
`i % signed_tick_rate == signed_game_tick % signed_tick_rate`. At 30 Hz, each row
therefore receives one visit each second. For a match starting at tick 0 the
executed row-0/1/12 initial timers expire at ticks 0/1/12, respectively; this is
row cadence, not a new universal spawn-delay constant. A bound, unmoved unowned
weapon retains countdown 0. Timer-start 0 starts a delay when item type bits
match mask `1C` and item `+12C` bits 0 or 1 are set, or position displacement
squared from the authored position is greater than original `00465030=1.0`.
Exactly 1.0 does not start it. A missing datum also starts delay. Timer-start 1
checks object `+10A` bit 2 instead. The latter is statically read and has not yet
been executed in this report.

`0015C290` chooses nonzero authored row seconds first, then valid itmc/vehc root
`+8` nonzero seconds, otherwise original `0046516C=30.0`, using original x87
FISTP conversion. Classification 0 applies options byte `+211`: 1 sets -1
(never), 2 shifts delay right one, 3 shifts left one; other values retain delay.
Classes 1..7 apply options byte `+208` instead. A positive countdown is decremented
**on the same visit that starts it**, then every subsequent row visit. For
45/30-second source delays, allocation occurs after 44/29 further second visits
following that first timer visit. A native millisecond timer started with the
unmodified full delay would miss this original ordering.
[`delay`](../analysis/multiplayer/integration/inventory-source/netgame/instructions/0015c290.asm)

The serialized map-weapon parameter is `53`, respawn-time parameter `57`.
Untouched descriptor entries `00239385`/`0023940F`, found in TU table `00477D10`,
return type 3, byte offsets `D4`/`D5`, scale bits `3F800000`; options variant base
`+13C` makes them options `+210/+211`. Both descriptors execute in the report.
Actual mainmenu `sily` records identify parameter `57` as Weapon Respawn Time:
0 Map Default, 1 No Respawn, 2 Half Time, 3 Double Time. Menu hash, record offsets
and localized choices are preserved alongside the descriptors.
[`descriptor 57`](../analysis/multiplayer/integration/inventory-source/netgame/instructions/0023940f.asm)

Expiry calls `0015C110`: collection choice, class/weapon-set mapping, original
`000B8820` descriptor initialization, authored position, original orientation
conversion, then unbind an old row via `0015C820`, then allocator `000B8A30`.
Default weapon set options `+210=0` retains actual BR/SMG definitions through
original `0015BC80`. `0015C820` clears the old object's signed row byte `+AF` to
NONE and row datum to NONE; it does not delete it, clear its owner, change slots,
or refill it. Allocation result replaces row datum and zeros both counters even
when allocation fails. Success sets the new object's `+AF` to the same row i;
flags bit 0 invokes original `000BAA80` with DL=1 when the mapped definition still
equals the collection choice, setting object `+C1` bit 0 in this fixture. The
subsequent physical effects of levitation are outside the trace.
[`spawn`](../analysis/multiplayer/integration/inventory-source/netgame/instructions/0015c110.asm),
[`unbind`](../analysis/multiplayer/integration/inventory-source/netgame/instructions/0015c820.asm)

The carried-identity cases attach the original row object through full `000CE510`.
Respawn preserves its datum, unit owner, carried slot and ammunition, while
binding a different newly constructed, unowned object to the row. Allocation
failure leaves row NONE/countdown 0; the next row visit starts the full delay and
decrements it rather than retrying immediately. For weapons (object type 2),
delete-on-respawn never destroys the old bound weapon. Vehicle-empty timing,
nondefault weapon sets, destruction and distributed allocation/event transport
are unresolved active integration boundaries; the complete stock weapon slice
can expose the original descriptor to explicit allocation/settling services.

The stage order is read from untouched original instructions: `00139750` calls
player update `0014EAE0`, then after other stages game-engine update `0015D7C0`,
then object pass `000B7E00`, `001C7660`, `000B7F50`, `000B8040`, and eventually
increments the game tick. Inside `0015D7C0`, equipment timers run after
`0015D490→00241100→0015B100`, before `00164480` and the per-player engine loop.
Thus original equipment timers are after the early player stage and before the
unit/weapon/projectile object callbacks and projectile postpasses. These order
claims are instruction reads; the report executes individual full timer calls,
not the complete global frame.
[`game tick`](../analysis/multiplayer/integration/inventory-source/netgame/instructions/00139750.asm),
[`engine tick`](../analysis/multiplayer/integration/inventory-source/netgame/instructions/0015d7c0.asm),
[`SMG/projectile stage evidence`](SMG_FIRING_1_1_RESEARCH.md)

## Weapon targeting and request admission

[`oracle_mp_inventory_target_1_1.py`](../tools/oracle_mp_inventory_target_1_1.py)
executes original `001527C0→00152E90→000CEC00→00152A30`, then
`00153090→000E7F30→000EA660→000CE510`, using actual SMG/BR definitions, a
source-initialized SMG and player 0's unit. Its independent
[`report`](../analysis/multiplayer/integration/inventory-source/target/report.json)
contains 42 cases and 279 routine rows. These include actual player admission
adjustment `00152080`, full original mode-2 engine pickup gate, strict distance
boundaries, candidate tie rules, stale/invalid object rejection, same-definition
dual admission, radius ownership and three original collision-visibility paths.

```sh
.tools/python/bin/python tools/oracle_mp_inventory_target_1_1.py
```

`001527C0` initializes the 12-byte candidate storage to priority 0, object NONE,
last word NONE. The candidate itself occupies 8 bytes: signed priority i16 `+0`,
mode flags byte `+2`, zero byte `+3`, object datum `+4`. With a unit and parent
`+14=NONE`, it requests two nearby-object sets from `000BBF40`: weapon mask 4,
radius `unit+3C + original004651A0` (bits `3ECCCCCD`, .4f), then the filtered
nonweapon mask, radius plus original `00465044` (bits `40400000`, 3.0f). Each
buffer holds 64 object datums. The trace supplies enumeration results explicitly;
the spatial index's candidate ordering and parent/child traversal are not yet
native integration evidence. Preserve source enumeration order when available.
[`scan`](../analysis/multiplayer/integration/inventory-source/target/instructions/001527c0.asm)

The weapon branch `00152E90` rejects item `+12C` bit 0, then item `+14C` equal to
the player's unit, then requires squared distance between runtime bound centers
`+30/+34/+38` to be **strictly less** than the following source radius squared:

```
radius = unit runtime +3C
       + (weapon definition i16 +290 != 0 ? original004650D8 : 0)
       + original00452000
```

The constants are `3DCCCCCD` (.1f) and `3E99999A` (.3f). Actual stock SMG and BR
definition `+290` are both 0. The next gate is complete original `000CEC00`,
including player `00152080` and engine admission. Priority is 1 for `+290=0`,
7 otherwise. Flags use modes `[0,2,1,3]`: first/second primary admission sets
1/2; first/second other-hand admission sets 4/8. Viewing aim reversal produces
the same eligible weapon candidate in the executed fixture. The viewing ray
used by device interaction is not part of the weapon branch.
[`weapon candidate`](../analysis/multiplayer/integration/inventory-source/target/instructions/00152e90.asm),
[`admission`](../analysis/multiplayer/integration/inventory-source/target/instructions/000cec00.asm)

`00152A30` prefers greater signed priority, then a strictly nearer point on equal
priority. Its points come from original `000BACC0`: object root position
`+64/+68/+6C` when unparented, otherwise that position transformed through the
parent's referenced node matrix. These ranking points are distinct from the
bounding centers `+30` used in the admission distance. Equal distances preserve
the prior candidate. The trace deliberately gives the second BR a farther
bounding center but nearer root position and confirms that it replaces the
first candidate; an equal root position preserves the first.
[`rank`](../analysis/multiplayer/integration/inventory-source/target/instructions/00152a30.asm),
[`root position`](../analysis/multiplayer/integration/inventory-source/target/instructions/000bacc0.asm)

Source bound ownership is established by full `000BEDE0`: runtime bound radius
`+3C = object scale+A0 * definition f32+4`; center `+30` is the definition's
bounding offset `+8/+C/+10` transformed through the first runtime node matrix
referenced by object signed offset `+116`. Original `000B8A30` copies spawn
descriptor position `+1C` into object root position `+64`; allocation does not
write that position directly into bound center `+30`. `000BDF10` orders root-node
update `000BDF80` before bounds update `000BEDE0`. Real animation/physics node
population remains an explicit service. The executed bound cases build a supplied
simple root pose using original `000BEB30` math; they do not assert that pose is a
live character animation frame.

| Actual definition | Authored radius bits/value | Authored bounding offset |
| --- | --- | --- |
| Lockout masterchief_mp `E9F2086D` | `3EE147AE`, .4399999976158142 | `[-.019999999552965164,0,0]` |
| SMG `E57B0406` | `3DF5C28F`, .11999999731779099 | `[0,0,0]` |
| BR `E192001D` | `3E19999A`, .15000000596046448 | `[.04399999976158142,0,.05000000074505806]` |

The source radius for a stock scale-1 Spartan plus the nonspecial pickup margin
has bits `3F3D70A4` (.7400000095367432). This derives from actual object metadata,
not the biped movement capsule field `+270`. The report also uses an explicit
.5 runtime-radius fixture for generic gate tests; that test input must not
become a native radius constant. Stock-scale inside/equal/outside cases use the
original-computed radius. Scaling the unit by .5 halves the source bound radius.
[`bounds`](../analysis/multiplayer/integration/inventory-source/target/instructions/000bede0.asm)

When options state `+8==2`, a newly winning weapon candidate must pass original
`000C57A0`, or the prior candidate is restored. This is collision visibility,
not camera direction. Complete `000C57A0` is executed in the three later cases:
unit capsule geometry comes through original `000E0C20` (actual biped
`+264/+268/+26C/+270`, runtime crouch `+2C4`), weapon point through `000FBFA0`,
and collision segments through `0016B240`/`0016BAC0`. Supplied first-segment clear
accepts after one query; first blocked/second clear accepts after two; all blocked
rejects after eight. Query masks, start/delta, ignored unit/weapon datums and
supplied hit/point are recorded. Those collision results are fixture inputs;
native terrain/object visibility must come from the real collision service.
[`visibility`](../analysis/multiplayer/integration/inventory-source/target/instructions/000c57a0.asm)

`00153090` consumes the selected candidate only when priority is 1 or 7. It then
executes `000BBD10`: datum is not NONE, low index is below pool high-water `+38`,
header stride `+24`, header signed salt is nonzero and matches datum high half,
and requested type mask matches header type `+3`. It requires weapon type 2,
owner `+154=NONE`, then reexecutes full `000CEC00` before choosing the mode. A
primary request chooses 3 if mode byte 0, else 4 if byte 2; the other hand chooses
5 if byte 1, else 6 if byte 3. Otherwise it returns false. Outside options mode 4
it zeros a 32-byte request, writes type 20 `+0`, the **same object datum** `+4`,
i16 mode `+8`, then calls original dispatcher `000E7F30`. No definition replacement
stands in for object identity. Stale salt, salt zero, high-water exclusion, wrong
type, owned object and invalid priority are all executed rejection cases that
emit no request.
[`request`](../analysis/multiplayer/integration/inventory-source/target/instructions/00153090.asm),
[`datum validation`](../analysis/multiplayer/integration/inventory-source/target/instructions/000bbd10.asm)

The actual mode-2 engine vtable `00463B48` has original mode query `0009D5A0`
(returns 2) and pickup slot `+40→000A1110` (MOV AL,1; RET 8). The target fixture
uses that table and untouched leaf rather than supplying pickup permission. The
table and identifying engine initializer `002C22A0` are preserved in its report.

With an original-held SMG trigger state byte `+20C=0`, a nearby BR produces flags
1, primary mode 3 and actual BR identity in carried slot 1. Trigger state bytes
1/2 produce flags 2 and primary mode 4: original dispatch drops the SMG, then
attaches that BR to slot 0; the drop's four shared-RNG stores match the earlier
action fixture. These are exact numeric trigger states, not an inferred mapping
from native visual reload/firing phases. A same-definition second SMG produces
flags 4: primary pickup rejects, other-hand mode 5 enters dual wield. Dual combat
and real animation transitions remain outside active native coverage. Successful
action results use the explicitly supplied animation/world services recorded in
the report and are not permission to fabricate their completion times.

Four additional cases execute original player-action input ownership. Full
`0014E780` initializes the `5C`-byte action record. Original marshalling fragment
`00188A94..00188AAE` copies all 12 bytes of the selected target from controller
storage `+6C` into action record `+28`; target datum therefore reaches action
`+2C`. Full `00150F00` reads action flags u16 `+1C`: bit 1 first offers the target
to the nonweapon handler `001533E0`, then requests primary weapon pickup through
`00153090(...,1)` if unconsumed. Bit 5 calls `00153090(...,0)` for the other hand.
With nearby BR and current SMG state 0, flags 2 emit mode 3 for the actual BR datum
and clear bit 1 on success; flags `20` emit no weapon request. A second SMG reverses
that admission: flags 2 remain unconsumed, flags `20` emit mode 5. The original
other-hand success path ANDs flags with `FFFD`, which clears bit 1 rather than
bit 5; the report retains this observed source behavior. Physical controller
button mapping, native keyboard bindings, action-bit generation and HUD prompt
text are not established by these records.
[`player action`](../analysis/multiplayer/integration/inventory-source/target/instructions/00150f00.asm),
[`initializer`](../analysis/multiplayer/integration/inventory-source/target/instructions/0014e780.asm),
[`marshalling owner`](../analysis/multiplayer/integration/inventory-source/target/instructions/001884e0.asm)

## Implementable boundary and remaining work

The source supports a native inventory record with stable object datums, four
carried slots, separate current/requested hands, per-slot ready timestamps,
sequence tracking, original creation ammo, grenade initialization and exact
same-definition ammo transfer. Those operations need no invented duration or
capacity. The reports provide a bounded action differential for later pickup,
switch and drop integration.

The fixture does **not** supply authority for spatial-query enumeration,
actual world collision results, interaction input and HUD prompts, animation duration/frame events/interruption,
actual allocation and parenting, dropped-object collision/settling, vehicle
spawn/respawn branches, death inventory disposal, dual-wield actions, energy-weapon
combat, successful multiplayer event transport or complete global-RNG ordering.
The apparent ready fallback when an animation service returns unavailable is an
original branch, but that supplied unavailable result is not proof the real
Lockout resources are unavailable. It must not be used to replace the unresolved
real animation timeline.
