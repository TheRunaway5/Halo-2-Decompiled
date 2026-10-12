# Original player/unit ownership and owned-object teardown

The shared native player/unit owner now preserves original assignment,
roster-status calculation and object-reference release. Its339 original CPU
comparisons cover retail, TU1.1 and TU1.5, including24 partial service stops.
Separately, the unchanged TU1.1 executable joins actual players, constructs
their bipeds and attached SMGs, clears their ownership during deactivation,
deletes both objects and spawns again using the surviving player record.

This closes the unowned-body restriction of the earlier constructed-object
investigation. The shared native owner still needs its actual view/control,
activation, network-dirty and engine-parameter services and full object/world callback
binding. Full original round/scenario/placement reset, normal edited-setting
matches and additional variant admission remain incomplete.

## Original constructed-player flow

[The owned-object probe](../tools/oracle_owned_object_deletion.py) combines the
retained player-visibility and constructed-object storage preparation. Selected
original initializer callbacks run in table order:19 players,20 view/control,
22 engine,25 physics constants,27 objects,28 unit queue,30 trigger bitset,
39 contrails,40 effects,58 full AI and59 physics. Their selected resets and the
retained activation/mask/world sequence execute before joining.

The exact retained original-produced default Slayer buffer and host-effective
property packet are delivered as API inputs. Zero-key nonlocal identity and
sign-in/session transport remain declared boundaries. Whole title/session
startup, savegame/thread I/O and Xbox kernel/hardware parity are excluded.
Original14D350 and its constructor/property callbacks allocate and initialize
the real player; no roster, current-unit or cluster field is supplied.

Complete1509C0 selects from the unchanged authored Lockout starts, constructs
the actual biped and SMG, binds them through14E240 and executes the source
loadout/attachment path. Retained optional effect178120, looping sourceC10E0
and equip-audio170060 creation boundaries remain recorded. Actual cache/resource
bytes and requested raw platform backing retain their prior documented scope.

Six fresh source flows cover slots0,3 and15 with two source phase preparations:

- The initial zero cluster masks, before the normal player/PVS update.
- The original current-cluster update prefix, complete PVS callbacks and
  complete prephysics/world/postphysics/newflags passes.

Each flow constructs, deactivates and deletes, then repeats from the same real
player. There are12 constructed owned bipeds and24 complete object deletions,
including their attached weapons. The source spawn count advances from1 to2,
the new object handles have new salts, and the surviving player keeps its
original full datum. Current, previous and watched unit references are NONE
after every deactivation/deletion. Object, component and physical-body counts
and object high-water return to zero after each deletion.

During teardown, every inherited provided gameplay/resource service is guarded
before its hook could return. Successful deactivation and deletion use none.
Actual154440,14E240,153A90 and189490 callbacks execute, followed by actual
recursive weapon deletion, Havok removal/destruction, component retirement,
body-heap release and datum retirement. Whole guest `.text` stays unchanged.
The trace records observed source addresses, including retained host/platform
boundaries; it is not an Xbox hardware instruction-retirement trace.

The current-cluster prefix stops before the command-input/action tail. The
phase passes and repeated direct spawning are explicit source API composition;
they do not establish a complete ordinary tick, damage/death, a normal round
transition, a full match or physical contact/input parity.

## Shared native CPU contracts

| Owner | Retail | TU1.1 | TU1.5 |
|---|---|---|---|
|Player/unit assignment |`14CAD0`|`14E240`|`14F620`|
|Object/player reference callback |`152CF0`|`154440`|`155820`|
|Roster status |`152340`|`153A90`|`154E70`|

[The native owner](../src/source_player_unit.cpp) accepts the actual mutable
global slots and required raw-memory/view/control/activation/network-dirty/parameter
services. It preserves the21C player layout, signed16 user and signed8
representation, full salted handles, fixed12-byte object headers, availability
bitmap iteration, mutable global reloads and original ordered field widths.
The reference callback retains its original unit-type mask and always reaches
the required view-object callback, including the nonunit branch.

Assignment clears old current ownership, retains the previous player on the
old unit, resets the original latched input fields and installs new ownership
in the original order. Release preserves the original extra roster-status
call, clears watched/previous references across all occupied records and
executes the required view callback. Neither operation frees object storage.

Retail's local multiplayer control handoff converts representations0/1 to2/3.
TU1.1 and TU1.5 pass the signed byte directly. Retail assignment therefore has
a real additional branch; its complete instruction shape is not claimed equal
to the updates. Both reference/status owners and the two update assignment
owners have verified complete source correspondence.

[The independent CPU oracle](../tools/oracle_player_unit.py) declares raw API
banks and all eight service leaves explicitly. These leaves record exact ABI
arguments; the parameter's raw float bits are a declared return input. They
do not implement view/controller/physics/network notification behavior. The separate actual
constructed-player probe above supplies the evidence for real teardown.

The native comparison covers all339 complete/partial banks,7,809,204 bytes,
3,471 ordered writes and1,470 reached service calls. It includes sparse/full-end
roster slots, current/previous/watched references, replacement/noop/NONE
assignment, local/nonlocal users, signed representations, retail game-type
mapping, engine absence and original byte-shift type semantics.24 additional
native cases remove the required callback entirely and reproduce the original
partial field state before the unavailable service. Missing memory also stops.

## Reproduction and validation boundary

```sh
.tools/python/bin/python tools/oracle_player_unit.py
.tools/python/bin/python tools/oracle_player_unit.py \
  --output analysis/multiplayer/game-variants/player-unit/reproduced/report.json \
  --fixture analysis/multiplayer/game-variants/player-unit/reproduced/fixture.json \
  --verify-fixture tests/fixtures/player_unit_original.json
.tools/python/bin/python tools/oracle_owned_object_deletion.py
.tools/python/bin/python tools/oracle_owned_object_deletion.py \
  --output analysis/multiplayer/game-variants/owned-object-deletion/reproduced/report.json
cmake --build build --target player_unit_tests -j2
ctest --test-dir build -R '^original_player_unit$' --output-on-failure
.tools/python/bin/python tools/verify_player_unit_lifecycle.py
```

The CPU reports/fixture and complete constructed-player report reproduce
byte-for-byte in separate runs. The focused configured native check passes.
Both new C++ translation units compiled directly with ASan/UBSan and leak
detection also pass all339 records and24 absent-service checks. The new owner
also compiles with the configured `halo2_systems` flags. A fresh production
startup or whole production link is not part of this validation; the new owner
is unbound to the native match's complete teardown chain.

The verifier checks literal original bytes/disassembly, source correspondence
and the retail difference, raw blob hashes, exact repeats, actual teardown
callbacks/counts, native results, source freshness and artifact pins. Full
world/sound/scenario reconstruction, round preparation/replication and complete
normal local/network matches remain required by the game-mode goal. Admission
remains ordinary Slayer/Team Slayer startup, with37 other built-in rows gated.
