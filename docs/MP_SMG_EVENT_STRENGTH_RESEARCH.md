# Stock SMG event, Slayer strength and shot-owner producers

This is a separate frozen TU1.1 source gate for the **first stock direct SMG
impact**, ordinary published local handicap, default Slayer damage multiplier,
and full shot-owner identities. It preserves the earlier
[damage/history suite](MP_SMG_DAMAGE_HISTORY_RESEARCH.md) and
[impact suite](SMG_IMPACT_1_1_RESEARCH.md). No production implementation was
changed. Original instructions execute unchanged; supplied world, transport
and allocation services remain named below. This is not a complete original
world, hit finder, lobby or network replay.

## Frozen evidence and reproduction

| Input / output | SHA-256 |
| --- | --- |
| TU1.1 `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03` | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| Lockout `assets/disc/maps/lockout.map` | `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9` |
| [New oracle](../tools/oracle_mp_spawn_smg_producers.py) | `a86cff27319ce75a838ea42bca7fd9f1907b01fbb8b5329a1ede22f16627dd82` |
| [New report](../analysis/multiplayer/integration/spawn-source/smg-producers-report.json) | `c8f66fbb8f5041acad27e2b113d0f8952f3654ec8bfd31fd626e48acfef4fcfa` |
| [Original instruction listing](../analysis/multiplayer/integration/spawn-source/tu11-smg-producers-instructions.txt) | `46fe839d0cb319337fb335d7a1ad21c6a7654038ddd7ad65097f2bcff36cb623` |
| Preserved damage oracle | `4d20d8c2e7229dd2bf78a601482d1862535c0208ab7a57b86f47192337c1a378` |
| Preserved damage report | `2b7d072e92dde4ad6af7a92f301cf5b734410df44745c988bbcf6b5b372e9aca` |
| Preserved impact oracle | `c8cd233522a66035ff0cfec56eb3781da254161f1da6692744eb46c4d05da1e6` |
| Preserved impact suite manifest | `401adedf8559b9511bda66dc38f9e1c723d3690a40fd962724e119594ddc37b4` |

The report contains **58 cases**: four freshly executed impact events matching
the earlier frozen events byte for byte; 36 controller/property/amount cases;
four full-datum firing/allocation/owner-copy cases; and 14 actual default-Slayer
and multiplier-branch cases. Four property cases enter original new-player
initialization; the multiplier cases additionally initialize both players.
The new report retains complete property, projectile, event, relevant player
state, instruction entries and supplied services. Its independent replay is
byte-identical. The damage oracle's nine earlier tool/report dependency hashes,
the damage tool itself and its instruction listing remain unchanged.

```sh
.tools/python/bin/python tools/oracle_mp_spawn_smg_producers.py --output analysis/multiplayer/integration/spawn-source/smg-producers-report-repro.json
cmp analysis/multiplayer/integration/spawn-source/smg-producers-report.json analysis/multiplayer/integration/spawn-source/smg-producers-report-repro.json
```

Addresses, offsets and sizes below are hexadecimal unless otherwise indicated.
The instruction listing is read from the pinned XBE. Ghidra sources are useful
cross-references; raw instructions and executed report bytes own these claims.

## First direct-impact event and exact amount inputs

The executed path is original firing/upper allocation/type construction, then
`FCEC0 -> FCF90 -> 100110 -> FEB40`. The collision service supplies one kind-4
object hit. For selected real Spartan surfaces, original `16AF90` builds the
collision metadata from actual `coll`/`hlmt` bytes, with lower intersection and
current node poses supplied exactly as declared by the earlier impact oracle.
At the direct-hit branch, `FEB40` initializes the event with `D7C60`, copies
ownership with `BD080`, copies hit point/direction, and calls `D9180`.

`D7C60` zeroes **88 bytes** with `REP STOSD`, installs the damage tag, default
sentinels and BSP short, then loads original executable constant `465030`
(`3F800000`) into `+54`, `+58`, `+5C`. It leaves `+60`, `+64`, `+68` zero.
`FC2E0` stores that same constant into projectile `+188` at `FC397`.
`FEB40` reads projectile `+188` at `FEC94`, and stores the resulting scalar into
event `+54` at `FEE8C`. Authored SMG projectile definition `+BC` is zero, so the
distance-scaling branch controlled by bit 4 is skipped. The first direct event
therefore has these exact source-produced inputs:

| Event field | Type used here | Original bits | Owning operation |
| --- | --- | --- | --- |
| `+54` | float32 | `3F800000` / 1 | Projectile constructor `+188`, then direct-impact copy |
| `+58` | float32 | `3F800000` / 1 | Retained initializer value |
| `+5C` | float32 | `3F800000` / 1 | Retained initializer value |
| `+60` | raw 32-bit field | `00000000` | Initializer zero; no direct-branch write |
| `+64` | raw 32-bit field | `00000000` | Initializer zero; no direct-branch write |
| `+68` | raw 32-bit field | `00000000` | Initializer zero; no direct-branch write |

The direct branch also ORs event `+04` with 8; copies collision hit point into
`+24..2C` and `+30..38`; normalizes incoming motion into `+3C..44`; retains
zero `+48..50`; copies projectile `+1A8` into event `+7E`; and writes authored
projectile `+128 & 3F` (=8) into event `+84`. The D9180 call receives the full
collision object `+40`, node `+46`, region `+44`, model-material entry `+5A`,
and collision normal `+28`. Source: `stock_direct_impact_events`, listing
`D7C60`, `FC2E0`, `FEB40`, and the preserved impact instruction inventory.

All four produced **complete 88-byte events** match the frozen original impact
reports in modes 1 and 5 for material entries 0 and 1. The older fixture uses
attacker `00000000`; this is a declared unsalted owner fixture, and is not
evidence for valid original player attribution. The separate full-datum cases
below close the owner-copy contract without reinterpreting that fixture.

The actual SMG `jpt!` is `E59D0428`, file offset `01DEEB54`. Absolute tag `+1C`
is zero; `+20` and `+24` are equal `40980000` (4.75). Original `DB160` reads
the equal upper bounds and consumes no selection RNG. Its source expression,
before multiplayer scale, is:

```text
((1 - event[54]) * tag[1C] + selected_upper * event[54]) * event[5C]
```

For the first source event this yields `40980000`. The new property cases
compose those exact event scalar bytes with explicit salted attacker/source
unit/designator fields, then execute the earlier original `DB160 -> DA710 ->
DA2B0` kernels. This is a separate source-kernel composition, not one continuous
world damage call. Earlier damage/result/history cases remain the authority
for `+44/+48` normalized outputs and their consumers.

## Local handicap producer, host assignment and player initialization

Strength at player `+C1` is property `+7D`, which comes from the local controller
handicap. It is not inferred from a generic variant option or a zeroed native
player. These original paths execute:

1. `190FB5` clears the controller storage and calls `19277D` for all four slots.
   `19277D` executes its cleanup and at `1927C8` stores zero into controller
   `+200` (`556630 + controller * C74`). The oracle poisons the entire controller
   destination before this source clear. It stops at `19100B`, before the final
   unrelated `24DBF6` UI reset tail.
2. With an explicitly supplied active local controller/user ID, complete
   `1918D0 -> 1917C6` constructs the property packet and `64900` publishes it.
   `1917C6` executes `154F40` property initialization and actual inactive-record
   `64890` branch. At `191885/19188B` it copies the controller `+200` **low byte**
   to property `+7D`, overriding the property initializer's zero. Voice runtime
   is inactive; original `54F30` executes its inactive branch.
3. The exact produced 90-byte packet is explicitly delivered to host session
   `+1138 + membership * 13C`. Complete `65DD0` and `5AEB0` copy the requested
   property into host effective `+11C8 + membership * 13C`. In phase 1 with the
   teams flag clear, host membership assigns property `+7C` from membership index
   when the request is not FF. **Property `+7D` is copied independently** in all
   these host cases; the host's local zero initializer does not own its value.
4. Original `14D600` copies all requested properties into player `+D4`, runs
   `14D3D0` sanitation, then copies the effective 90-byte packet into player
   `+44`. Consequently property `+7D` becomes player `+C1`. The original signed
   byte clamp is `<0 -> 0`, `>3 -> 3`, otherwise unchanged. The prefix stops at
   `14D687`, immediately after that effective copy; later notifications are not
   modeled. Default cases enter `14D800` first: its creation record `+1C` is the
   original property input passed to `14D600` at `14D9E9`. Its earlier writes
   clear player `+194` at `14D96A`, current/previous unit to NONE, and set initial
   flag 8 for the supplied new-player argument. The player salt is preserved.

Source authority: `source_strength_cases`, `original_default_slayer_multiplier_cases`,
new listing `190FB5/19277D/1917C6/65DD0/14D3D0/14D600/14D800`, and Ghidra
[1917C6](../analysis/map-pack-update/functions/001917c6.c),
[19277D](../analysis/map-pack-update/functions/0019277d.c),
[65DD0](../analysis/map-pack-update/functions/00065dd0.c).
The retail readable UI source also identifies controller `+200` as handicap
and its four choices, in `unknown_2b116a.cpp` around retail `2B4C45`; that UI
handler is not executed by this suite.

| Published handicap byte | Effective player `+C1` | Original enemy amount bits / value |
| --- | --- | --- |
| Source reset zero, or explicit 0 | 0 | `40980000` / 4.75 |
| 1 | 1 | `40640000` / 3.5625 |
| 2 | 2 | `40180000` / 2.375 |
| 3 | 3 | `3F980000` / 1.1875 |
| 4, 127 | 3 | `3F980000` / 1.1875 |
| 128, 255 | 0 | `40980000` / 4.75 |

The report contains each of these nine inputs for all four controllers. The
default row is backed by executed controller reset; explicit values are branch
fixtures. A remote or already modified lobby player's strength must use that
player's actual published property; local reset does not prove every remote
player or retained lobby state has handicap zero. Sign-in, controller activation
and transport remain explicit boundaries.

## Actual default Slayer and every multiplier gate

Complete original `1A0A90(options+13C, 0)` generates default Slayer gameplay
settings. Only optional localized name lookup is omitted by the original
absent-globals branch. It writes engine index 2 at options `+180` and flags
**`00000F9A` at `+184`**. Bit 7 is set; trait bits 12 and 13 are clear. This
differs from the earlier amount fixture's supplied flags zero, though its enemy
amount agrees. The actual Slayer object `487188` has vtable `463B48`; its `+8C`
trait callback is `253F40` and its `+6C` relationship callback is `253F10`.

Original `15F520` takes attacker in ECX and target in EAX. Its ordered behavior is:

1. Start with 1. Return 1 immediately if the engine pointer is absent, attacker
   equals target, or attacker is NONE. Self-hit bypasses handicap, traits and
   target `+194`; do not apply the ordinary enemy handicap to self damage.
2. For a present, different, non-NONE attacker, read signed player `+C1`.
   Values 0/1/2/3 select 1/.75/.5/.25. Other raw values retain 1; ordinarily
   source property sanitation already restricts this byte to 0..3.
3. If target is NONE, return this handicap scale. Otherwise call original
   `1615F0(attacker,2)`. Actual Slayer `253F40` returns options `+184` bit 12;
   true multiplies the scale by `3FC00000` / 1.5.
4. Call `1615F0(target,3)`. Slayer returns options bit 13; true multiplies by
   `3F000000` / .5. Next, signed target player `+194 > 0` multiplies by
   **`3DCCCCCD` / approximately .1**. Zero or negative does not multiply.
5. If options bit 7 is clear, call `15F680`, which dispatches Slayer `253F10`:
   unequal signed membership shorts permit damage; equal membership returns
   zero scale. If bit 7 is set, skip that relationship callback.

All multiplications occur in that source order as separate SSE operations.
The multiplier itself does not perform player salt validation. Caller ownership
and later attribution have their separate identity contracts.

The default case executes both new-player initializer prefixes, establishing
both `+194 = 0`, and uses original published handicap zero and memberships 0/3.
It then executes the actual default-variant producer and `15F520`: both trait
callbacks execute and return false; the relationship callback is skipped by
source bit 7. Result is exactly `3F800000` / 1. Fourteen multiplier cases cover
each shortcut, each trait, both traits, positive/negative `+194`, and bit-7
relationship cases. Perturbed trait flags, positive `+194`, same membership and
absent engine are **supplied branch inputs**, not newly recovered lifecycle
producers. Authority: `original_default_slayer_multiplier_cases` and the raw
`1A0A90/15F520/1615F0/253F40/253F10` listings.

## Shot owner and full-datum attribution API contract

For an owned firing unit, `108E83..108EB5` looks up that actual unit, copies its
**full player datum at unit `+13C`**, its membership short `+138`, and the full
unit datum into an owner triple. `B8820` copies the supplied triple into creation
descriptor `+68/+6C/+70`. Original `B8D0D..B8D24` stores those fields into
projectile **`+C4/+C8/+C2`**. Thus stock ordinary projectile `+C8` is the firing
owner's full unit datum; it is not automatically NONE, the projectile itself,
or a player-slot encoding. If no firing owner is found, the original earlier
branch initializes the owner triple to NONE instead.

Original `BD080`, called by `FEB40`, copies projectile `+C4` to event `+08`,
`+C8` to event `+0C`, and the signed membership/designator short `+C2` to event
`+10`. It does not recompute the owner from the current occupant of a player
slot. The event was zero-initialized, so the unused high half of its designator
container retains the initializer bytes.

Four cases execute actual firing, upper allocation and type constructor with
full player datums `00070000` and `00080000`, owner unit `00010001`, membership
0, in modes 1 and 5. Complete descriptor/projectile/event bytes prove preservation
of all three identities. Original `16D560` resolves the matching player salt;
after an explicitly supplied slot-salt change, it returns null for the old full
datum while a repeated original `BD080` retains the old projectile owner.
The salt change is a reuse-state fixture; departure or allocation salt evolution
is not executed here. Authority: `source_shot_identity_cases`, listings
`108E62..108EEA/B8820/B8D0D/BD080/16D560`, and preserved firing/allocation suites.

An attribution port must carry those source identities through its API:

| Runtime value | Required identity and ownership |
| --- | --- |
| Player identity | Full player pool datum, separate from membership `+C0` and native combat team |
| Current unit | Player `+2C` full unit datum or NONE; ownership follows original bind/detach lifecycle |
| Projectile attacker | Captured full player datum from firing unit `+13C` at shot creation |
| Projectile source object | Captured full firing unit datum, retained across projectile lifetime |
| Damage event/result | Independent full attacker, full source object, signed designator; no `slot+1` encoding |
| History owner | Target full unit datum/generation, with original per-unit initialization |
| Each history entry | Full stored player and full object, tick, float32 normalized amount and cause |
| Attribution lookup | Resolve full player against actual capacity/stride/nonzero matching salt; convert a valid result to display slot afterward |

This table is a port contract derived from the executed producers and earlier
damage/history/attribution routines, not a new native allocation algorithm.
The history writer deliberately records full stale identities; matching is by
non-NONE full player **or** full object and retains the original stored identities.
Validation belongs to the original attribution reader. Looking up the current
occupant by the old datum's low slot bits would change credit after departure or
reuse. Damage history belongs to the unit and resets at its constructor, not
at every hit and not merely because a player slot remains connected. The earlier
[history specification](MP_SMG_DAMAGE_HISTORY_RESEARCH.md#history-initialization-amount-and-identity)
and [Slayer attribution specification](MP_SLAYER_ACCOUNTING_RESEARCH.md) own
the actual writer/reader and stale-credit cases. This suite does not prescribe
unproved player/object salt increments or manufacture runtime datums.

## Remaining boundaries

No live collision, actual animation pose playback, remote lobby transport,
controller sign-in, full object-world registration, initial-frame RNG ordering,
corpse deletion or full multiplayer network semantics are established here.
The event producer uses the older impact suite's declared intersection/pose and
lower allocation fixtures. Its D9180 call is intercepted for event isolation;
the separate frozen damage suite executes that original consumer and history.
The new default branch establishes multiplier 1 only under the enumerated
source-produced default flags, handicap and initializer state. Later positive
`+194` lifecycle, edited traits and retained/remote properties must use their
actual source state. No frozen prior tool, report or documentation was rewritten.
