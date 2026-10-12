# Original full reset with authored scenario reconstruction

The unchanged TU1.1 full round-reset owner now returns twice with an actual
joined player and the rebuilt Lockout world. The first call deletes the owned
biped/SMG and constructs 19 authored scenario objects. After another direct
source spawn, the second call deletes all 21 objects and reconstructs the same
19. There are 23 complete object deletions, with 15 physical components/bodies
and six looping-sound records after each reset.

This closes the earlier missing looping-sound pool and serialized-physics-tag
stops in this source API composition. It does not establish normal round
dispatch, a full match, sound/device startup, native world/scenario binding or
additional variant admission. The goal retains all seven families, 39 built-in
variants, supported editable rules and general tag/weapon/equipment behavior.

## Original owners and input delivery

[The new probe](../tools/oracle_round_reset_scenario.py) reads the earlier
owned-reset tool/report and native-reset verification as immutable inputs.
The source image is the owned disc map-pack TU1.1 executable, SHA256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
Lockout remains SHA256
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
The live compatibility worker stays alive through source execution.

Actual table row 43 constructs/resets the class-fade storage through 223960 and
2239B0. Row 46 constructs the 1024 × 24-byte object-looping-sound pool and 0x244-byte
context through 18A200, then executes complete 18A2A0 map reset. These selected
storage/reset APIs retain their ascending table order around the other selected
owners. All four calls return with no supplied gameplay/resource service.
The shared native CPU implementations already exist in
[sound_looping](MP_SOUND_LOOPING_NATIVE_INTEGRATION.md); this investigation
supplies their original storage composition to the actual reset flow.

Row 46's reset also needs the mixer context at 526474. Whole 126B20 retains its
earlier physical-protection stop at kernel 800000B6, return site 21A7A3,
arguments 0x82A47000, 0x300000, 4. An independent whole-function 2205C0 attempt uses its
actual caller argument 451220. Original instructions allocate and clear the
0x2AD8-byte mixer context, then stop before 383DD7 DirectSoundCreate with
`NULL, &mixer[2AB0], NULL`. No device return or readiness is supplied.

The later map reset and world reset are independent APIs on those source-owned
cold contexts. Neither stopped audio frame is resumed. System+78 stays 0, and
127B50 returns through its original unavailable-sound branch. Device creation,
whole sound initialization and playback remain unproven.

## Whole physics-tag postprocessing

After the looping-sound owners ran, scenario reconstruction reached 1D9994 with
the lift's serialized shape index 0x0E in EDI. Its `inc word [EDI+6]` accessed 0x14,
rather than a resident shape. The complete original 11DE10 dispatcher is the
general owner for relevant `bipd`, `crea`, `coll`, `phmo` and `vehi` tags.
The new probe executes it, including its 11DF00 and 164750 tails, rather than
selecting named props or assigning a shape pointer.

The first whole-dispatch attempt exposed a separate input-delivery problem:
the inherited `index_file_offset + metadata_magic` alias did not contain the
cache's 32-byte index header. Its bytes matched unrelated file data at F10E00,
whereas the authored index header is at 1CCE800. The wrong bounds made the
iterator run beyond the authored 4512 tags. No instruction in the dispatcher
wrote that header; it was already different before the call.

The new probe explicitly delivers the exact original 32-byte header in separate
actual mmap backing and binds that iterator input alongside the same literal
tag-table view. It imports the real authored count rather than supplying a
chosen count. The private alias remains an API delivery adaptation: it does
not establish the original map I/O/cache-loader ownership or physical layout.
Original pointer words in the header are preserved. The all-group iterator
uses the real count, while the existing retained tag table and metadata own
definition references.

Whole 11DE10 now returns after 216,376 observed instructions. All 4512 original
tag entries are traversed. Complete before/after bytes are retained for all 158
relevant definitions; 69 change through original postprocessing. Every before
definition exactly matches its original file bytes. The supplied index header
is unchanged afterward, there are no writes to its backing/global during this
call, and no gameplay/resource return is supplied.

The original whole dispatcher owns selection and postprocessing. No map, mode,
weapon or prop-name branch replaces missing tag behavior. These results concern
the TU1.1 original execution; they do not establish whole retail/TU1.5 startup.

## Repeated owned-world reset

The unchanged authored Elimination row, including its private controller glyph,
comes from `mainmenu.map` and the original variant initializer/setter contract.
The complete 304-byte configuration is delivered as an explicit input; Reset
Map Every Round remains enabled and the buffer stays unchanged through both
resets. Original 14D350 allocates player EC700000, and complete 1509C0 constructs
and binds the actual biped and attached SMG.

Each complete 15B460 executes its real player/object destruction, decal/mask,
contrail/effect/registry/light and cold sound cleanup, followed by D6B60,
C1870 and D6C40. The second call begins with the 19 source-rebuilt objects plus
the newly spawned biped/SMG. Actual recursive deletion, physical-world removal,
component/body-heap/datum retirement and looping-sound record release run.

Both resulting worlds match the exact authored placement/palette data:

| Source placement type | Original count | Result |
|---|---:|---|
| Sound scenery, type 10 | 4 | The four original sound-scenery definitions |
| Crates, type 11 | 15 | The lift and 14 power-module placements |

The verifier derives their block/palette offsets and row strides from the
original object-type table, then resolves the actual cache palette references.
It compares those counts and full definition datums to the constructed world.
All 15 crates have real components and one retained body each. Object count and
high-water return to 19, physical/component counts to 15, and looping-sound
count/high-water to 6 after both resets. The surviving player's current,
previous and watched unit references are NONE; its spawn count advances 1→2.

Every inherited supplied gameplay/resource leaf is guarded during reset.
The 14 recorded events per reset are actual 16F5E0 RNG stores, not service
returns. Their source multiply/add sequence is independently checked. The
inherited fixed RNG seed/clock, nonlocal player/session API inputs, optional
creation services, raw compatibility memory and resource-delivery boundaries
remain explicit. No new field or return forces a reset, tag postprocess,
allocation, physical insertion or scenario creation result.

Whole guest `.text` stays unchanged. The listing records observed original
addresses, with platform stops separately retained; it is not an Xbox hardware
retirement trace. Source API calls, inactive/cold contexts and repeated
direct spawning do not establish ordinary input/PVS/contact/frame progression,
damage/death or normal round preparation/replication.

## Reproduction and remaining work

```sh
.tools/python/bin/python tools/oracle_round_reset_scenario.py
.tools/python/bin/python tools/oracle_round_reset_scenario.py \
  --output analysis/multiplayer/game-variants/round-reset-scenario/reproduced/report.json
.tools/python/bin/python tools/verify_round_reset_scenario.py
```

The complete two-cycle report and executed original listing reproduce exactly
in an independent live-worker run. The verifier checks source/dependency pins,
raw hashes, original instruction bytes/disassembly, header/file/tag contracts,
original storage-table entries, exact audio stops, actual placement counts,
physical and looping ownership, player references, original RNG stores and
complete returns. This slice changes the source probe and documentation; no
fresh production build, native gameplay integration or launcher fix is claimed.

The shared native reset policy still needs complete live object/world/scenario
providers. Normal sound initialization, complete round preparation/replication
and full local/network matches with edited settings remain required. The
retained startup audit admits ordinary Slayer/Team Slayer only; 37 other built-in
rows stay gated, and the operational goal remains incomplete.
