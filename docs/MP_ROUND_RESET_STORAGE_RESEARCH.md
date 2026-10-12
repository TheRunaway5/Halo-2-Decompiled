# Original storage behind the round reset

The unchanged authored Elimination next-round reset now passes the earlier
decal-pool stop in a controlled original-code probe. Original constructors
provide the player, view/control, object, mask, decal, contrail, effect and
multiplayer AI contexts. The next retained stop is the missing sound context:
retail `126405`, TU1.1 `127B55`, TU1.5 `128F15`, reading NULL+78.

This advances source research. It does not establish a complete reset, live
object deletion, a native multi-round binding or an operational Elimination
variant. The runtime admission gates remain closed.

## Original owners

The original game initializer walks 68 callback records, each 36 bytes. The
tables are retail `440DD8`, TU1.1 `447428` and TU1.5 `44AED0`. The probe reads
the initializer and map-reset pointers directly from those records and retains
their complete bytes. Addresses below are initializer/map-reset pairs.

| Callback row and context | Retail | TU1.1 | TU1.5 |
|---|---|---|---|
|19 players |`14B4B0/14B580`|`14CC00/14CCD0`|`14DFE0/14E0B0`|
|20 view/control |`185AB0/185B00`|`1874D0/187520`|`1888A0/1888F0`|
|27 objects |`B67C0/B69D0`|`B76B0/B78C0`|`B87F0/B8A00`|
|34 context at TU1.5 global4F9B50 |`183E40/183EC0`|`1858E0/185960`|`186CB0/186D30`|
|37 decals |`17D220/17D2E0`|`17EC90/17ED50`|`1800B0/180170`|
|39 contrails/points |`17B7B0/17B870`|`17D220/17D2E0`|`17E640/17E700`|
|40 effects/locations |`175B50/175C10`|`1775D0/177690`|`1789F0/178AB0`|
|44 sound, independently stopped |`1253D0/125690`|`126B20/126DE0`|`127EE0/1281A0`|
|58 AI |`1C7790/1C79E0`|`1CB180/1CB3D0`|`1CFA40/1CFC90`|

The previous TU1.5 world-weapon stop at `18A85A` reads view/control context
global4F9B54. Its actual storage owner is `1888A0`, followed by `1888F0`.
The previous current-unit stop at `154E80` needs player context global4F54F0,
created by `14DFE0`. Locating these owners does not prove the full live-object
destruction path: this probe supplies no live object bodies or current units.

The full multiplayer AI storage callback `1CFA40` also returns under the
declared game-type2 inputs. Its subsequently executed map reset is `1CFC90`.
This supplies the context needed by the complete player map-reset callback;
the earlier AI-prefix experiment is not reclassified as a full initializer.

## Declared API inputs and retained boundaries

[The probe](../tools/oracle_round_reset_storage.py) declares raw game-arena
backing, an empty cursor/tail, CRC seedFFFFFFFF and allocator-object storage.
Original arena constructors (`124750/125EA0/127260`) and allocations
(`124700/125E50/127210`) execute normally. The physical allocation/protection
producer and whole title startup are excluded. The source capacity is3FE000
bytes; the emulated mapping is rounded to400000 bytes.

The decal storage initializer creates its original1,024-entry,64-byte datum
pool and3C2C-byte context, then reaches the original vertex-cache initializer.
That initializer calls VirtualAlloc with `(0,12,101000,4)`. The probe stops
before that platform call. Its frame is never resumed and no return is
supplied. The independent original allocator and cache constructor
(`13D170/13E8E0/13FC50`) build a separate613C-byte vertex-cache API object using
the unchanged constructor arguments. Binding that object to the original
cache-global pointer is an explicit caller input. It does not prove the
complete vertex-buffer/platform initializer.

After the selected contexts exist, all eight selected original map-reset
callbacks return. Two occupied player slots are allocated through the original
indexed datum allocator, preserving its genuine salts. The declared roster is
copied into those slots with current units and replication entities NONE.
Join/profile/current-unit production and actual object construction are excluded.

The original retail variant initializer/setters generate the unchanged authored
mainmenu row7 buffer. No Reset Map Every Round edit is applied. The next-active
phase reaches the original full reset, increments the decal cache epoch from1
to2, fills all4201 bytes of the original mask context withFF, and passes the
contrail, effect, object-registry and light-registry resets. It then stops on the
missing sound context. Live deactivate/delete owners and the subsequent
scenario reset/callback/placement tail are not entered.

A separate fresh probe retains the sound storage initializer's earlier
process-arena failure: retail `125429`, TU1.1 `126B79`, TU1.5 `127F39`, writing
NULL. That failed frame is not resumed, and sound storage initialization is
excluded from the selected game-arena callback sequence. Its missing process
arena and downstream audio services require their original producers.

## Verification and remaining work

The retained report contains60 completed original API calls across the three
images, three stopped VirtualAlloc boundaries, three independently stopped
sound initializers and three stopped authored next-active phases. It includes
58 complete source-shape comparisons, relocated operands and252 literal
instruction listings. Guest `.text` remains equal to the original image.
There are no instruction patches or supplied returns.

Snapshots retain the stat/ranking/respawn/netgame banks, full16-player records,
selected full context buffers and pool headers, allocator cursor/CRC, call
arguments, registers and exact stops. These are the declared observed records,
not a claim that every byte of the entire game arena is captured.

```sh
.tools/python/bin/python tools/oracle_round_reset_storage.py
.tools/python/bin/python tools/oracle_round_reset_storage.py \
  --output analysis/multiplayer/game-variants/round-reset-storage/reproduced/report.json \
  --verify-report analysis/multiplayer/game-variants/round-reset-storage/report.json
```

The independent command compares the complete report and every retained raw
record byte for byte. The verification manifest records the report, instruction
listings, source dependencies and original assets used by those runs.

The [subsequent sound cleanup work](MP_SOUND_SOURCE_LIFECYCLE_NATIVE.md) locates
the process-arena request and supplies a native shared cleanup owner. Normal
sound initialization and its runtime binding remain needed before continuing
the unchanged reset into its scenario/placement tail. Actual
constructed object deletion and reconstruction must also be validated before
binding later native rounds. Normal local/network matches, their editable tag
settings, spawn/combat/objective/HUD flows and all39 variants remain the goal's
acceptance requirement.

Subsequent [constructed-object teardown research](MP_CONSTRUCTED_OBJECT_DELETION.md)
executes the real deactivation, physics/world deletion and final storage release
for inactive unowned TU1.1 bipeds. Original view/control and effect storage close
its retained context faults, and reconstruction after deletion succeeds.
The native final storage owner is implemented, while full native teardown and
the unchanged authored round-reset/scenario/placement binding remain incomplete.
