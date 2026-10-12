# Live initial inventory and retained weapon identity

`MultiplayerMatch::spawn` now initializes a persistent source inventory for the
single actual allocated starting SMG. It retains the complete weapon datum,
the current/requested hand selectors, both inventory sequences and all four
slot timestamps. The public `Unit.weapon` remains the sole active primary
weapon state. Its ammo and SMG clocks are not copied to another active bank.
The new inactive bank is empty at spawn, because the original loadout supplies
one SMG. No spare weapon, positive pickup, switch action or delay is supplied.

The read-only match interface exposes `source_inventory(player)`,
`inventory_weapon(player, slot)` and `weapon_from_datum(full_datum)`.
`inventory_weapon` resolves the current primary slot directly to `Unit.weapon`;
other occupied slots must have their own retained inactive state before they
can resolve. Datum lookup checks the original native shared pool's full salt
and weapon type, then the actual slot identity. When a body is retained during
leave/rejoin or respawn, its inventory and weapon state remain together in the
existing retained-unit storage. The new player's starting weapon has a distinct
allocated identity. This does not implement the original physical drop or final
body/object deletion lifecycle.

Implementation: [SourceInventory](../include/halo2/simulation.hpp),
[read-only match declaration](../include/halo2/multiplayer.hpp),
[live spawn and retained lookup](../src/multiplayer.cpp),
[actual Match test](../tests/multiplayer_inventory_tests.cpp).
Runtime action delivery, CMake, live geometry, physics, damage and KBM are not
changed by this work.

## Exact initial owners

| Represented state | Original source owner and result |
|---|---|
| Four full salted item datums, `slots` | C52C0 C530E..C5316 sets all NONE. Actual CFED0 CFF0F inserts the original starting weapon in slot0. |
| `current[2]` | C5329/C532F sets NONE. Actual initial E9D50 E9EC1 writes primary0; secondary remains-1. |
| `requested[2]` | C5335/C533B sets NONE. Initial CE510 requests primary0; secondary remains-1. |
| `inventory_sequence`, `replicated_sequence` | C531B/C5322 sets both words0. CFE60 CFE9B then CFEA2 writes1/1 after the initial insertion in the measured options-byte1 path. |
| `ready_ticks[4]` | Actual zeroed allocation and CFED0 CFF16 clear the selected timestamp. E9D50 E9ECB reads original game clock518444+8 and writes that tick to the selected slot. They are timestamps, not countdown durations. |
| Active weapon ammo/ready | Full original constructor/equip produces60 loaded,120 reserve, own state9 and blocking ticks0. Existing `smg_initial_equip` retains that actual instant branch. |
| Inactive state storage | Empty while all other slots are NONE. It is native owning storage for future genuinely carried objects, not an additional source constructor or success result. |

The native spawn applies only this known initial output. It does not claim a
full live native invocation of CE510/CFED0, their world/attachment callbacks,
or the source manager/capability graph. General source slot and sequence
orchestration remains separately verified in
[MP_UNIT_INVENTORY_NATIVE_INTEGRATION.md](MP_UNIT_INVENTORY_NATIVE_INTEGRATION.md).
That kernel's actual mode4/network/drop paths must not be bypassed when a future
general carry/remove path uses it. The initial source FFA byte1 and the known
zero→1 insertion result are retained here, without inventing a new options
initializer or a general mode conversion.

The independently frozen genuine original player spawn supplies the stronger
initial output: unit E26F0000, item E2700001, slot0 points to that same item;
current/requested are `[0,-1]`; both sequences are1; all timestamps are0 at
that fixture's actual initial clock. The original source arena/pool has different
datum salts/backing from the native pool. Tests translate only this explicit
unit→slot→weapon relationship and check native full-datum validity; they do not
assert that a native allocation must have the oracle's fixture salt/address.
[Original full player spawn](MP_PLAYER_VISIBILITY_RESEARCH.md),
[allocated/equipped weapon](SMG_CONSTRUCTOR_EQUIPPED_1_1_RESEARCH.md).

## Original-only fixture and native checks

[Exporter](../tools/export_multiplayer_inventory_spawn.py) reads immutable
original reports and emits the two complete biped constructor outputs plus the
two complete player-spawn outputs. It preserves raw unit bytes210..237, raw
weapon magazine bytes, original salted datums and exact fields; no expected
value comes from the native implementation. It also checks all nine older
equipped-pickup initial snapshots against the newer real-player field results.
Both export files independently repeat byte-identically.

- Fixture:
  `analysis/multiplayer/integration/inventory-source/native-retained-inventory/fixtures.tsv`,
  SHA256 `32f8087fdebbb3a9cb0abb9149e52c7b20a7cccab77b46137cbe6f0ab577e6e7`.
- Original-only export report SHA256
  `0112fa8b0ce1667bcc16fc2d95cb7b9654549e18d07aeea459b06ae9d2b3a619`.
- Export reproduction SHA256
  `cc73b152ffa6000625e948dffc4d150ff257b874fc984c1edd77f9eb41c78bae`.

The focused test loads actual original Lockout content, checks constructor and
initial carry fields, full-datum/type rejection and exact active-state pointer
identity, fires through real Match steps, then leaves/rejoins and verifies that
the old carried datum/ammo/clock state survives in its retained body. The new
spawn gets fresh identity and its selected timestamp from the current match
tick. Another live player receives a distinct starting weapon. Kill-volume
delivery is explicitly excluded in this test so inventory retention can be
measured independently of death triggers; production geometry is untouched.

Required root-owned CMake registration is a `multiplayer_inventory_tests`
executable linked to `halo2_systems`; `retail_multiplayer_inventory` receives
the original Lockout map and the fixture above, with skip return77. No runtime
action integration or menu/KBM/UI change is required to run this check.

The focused original-map test passes normally and with address/undefined/leak
sanitizers. Four rebuilt current regressions also pass: the original bare SMG
Match/local-UDP traces, allocated/equipped SMG trace, and local/actual-UDP player
identity lifecycle. Inputs and all three owning source files stay byte-identical
during these checks. `native-validation.json` under the export directory pins
the source files, binaries/libraries, complete outputs and logs; its SHA256 is
`144fcdeacec8889fbaa8d5f7d1a1f45245c9aa3cde071577f0037af139b62fea`.
These are focused live storage/ownership checks, not positive switching or a
complete playable source-frame verification.

## Focused TU1.5 full-body correspondence

[New read-only correspondence tool](../tools/research_mp_inventory_switch_u5.py)
checks unchanged owned TU1.1 and authenticated TU1.5 bytes. All12 focused
owners have unique complete normalized instruction/internal-branch matches;
the24 complete encoded listings contain2,888 rows. Direct call targets,
original image memory operands, memory-destination moves and jump tables are
retained, including external callees whose bodies remain outside this report.
The new report and all24 listings independently repeat byte-identically.

| TU1.1 entry | Authenticated TU1.5 entry | Owner |
|---|---|---|
| C52C0 | C6440 | Unit inventory/type constructor |
| CE510 | CF5A0 | Initial carry/request/equip orchestration |
| CFED0 | D0F60 | Full-datum slot insertion/removal |
| CFE60 | D0EF0 | Inventory sequence stores |
| CF300 | D0390 | Source next-slot selector |
| E9FE0 | EAF00 | Ordinary switch begin |
| E9B40 | EAA60 | Switch admission/request helper |
| E9D50 | EAC70 | Current-slot/manager/weapon transition |
| EA180 | EB0A0 | Switch update |
| EA370 | EB290 | Switch finish |
| EA270 | EB190 | Switch interruption |
| E7E60 | E8D80 | Action update dispatcher |

Original dispatcher E7EAA/E8DCA uses type table46EC68/473D28. Types8 and18
both reference the exact ordinary switch descriptor46EA24/473AE4. Its new
begin/update/finish/interruption words are
`EAF00/EB0A0/EB290/EB190`, each a separately proven complete handler here.
The descriptor is uniquely found in the new raw file by all four verified
function pointers, and both original type-table words are checked directly.
Annotation labels/comments are not used as source authority.

- Separate report:
  `analysis/multiplayer/integration/inventory-source/inventory-switch-u5/report.json`,
  SHA256 `358511090ee3be1cce652923afc1979b759d2d48b63b45287ca25ff48d3251d2`.
- Separate reproduction SHA256
  `1dbca14f484ec3eea3ef967a60f332f9a33cdfc067def8f1423f7aea91bc530e`.

This is static original complete-body/table correspondence, not a new TU1.5
gameplay execution. The immutable TU1.1 switch fixture remains the dynamic
authority for its bounded supplied ticks and source-produced action/channel
progress. Its actual component resolves unit+346; its fixed+704 convenience
capture must not be treated as the component. No old instruction address is
relabeled as a TU1.5 executed address.
[Exact measured phases and missing F/Q producers](MP_KBM_ACTIONS_LIVE_INTEGRATION.md).

## Remaining live ownership

This closes actual persistent initial inventory storage and read-only retained
item identity, not visible switching. The native unit still needs the real
primary/secondary action components, managers/channels and owning clocks before
the asynchronous switch callback chain can run. Generic retained weapon heat,
generic own-weapon state and attachment/current-world pose remain unavailable
outside the existing single-SMG state. A meaningful alternate selection also
needs a real second carried object from the actual authored world/pickup path;
this change grants none. No full Havok/contact/settling, positive carry/drop,
melee/throw, complete updated frame or Xbox hardware parity is claimed.
