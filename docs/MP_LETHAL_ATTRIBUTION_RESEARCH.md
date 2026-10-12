# Original full-datum lethal attribution

This separate TU1.1 gate executes original stock SMG amount/result production,
`DAC40`, complete `1ED960`, original Slayer `2C2350`, history recording and the
unit death-cleanup prefix through ownership release. It uses source-generated
player/object salts and source-generated FFA membership. It is a bounded native
integration contract, not a complete death-event, animation or world simulation.
All earlier suites and production files are preserved.

## Authority, artifacts and reproduction

- Original TU1.1 XBE:
  `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256
  `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
- Original Lockout cache: `assets/disc/maps/lockout.map`, SHA-256
  `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
- New oracle:
  [oracle_mp_spawn_lethal_attribution.py](../tools/oracle_mp_spawn_lethal_attribution.py).
- Ignored immutable report:
  `analysis/multiplayer/integration/spawn-source/lethal-attribution-report.json`.
- Original instruction/constant listing:
  `analysis/multiplayer/integration/spawn-source/tu11-lethal-attribution-instructions.txt`.
- Prior contracts: [datum ownership](MP_DATUM_OWNERSHIP_RESEARCH.md),
  [SMG producers](MP_SMG_EVENT_STRENGTH_RESEARCH.md),
  [SMG damage/history](MP_SMG_DAMAGE_HISTORY_RESEARCH.md), and
  [Slayer accounting](MP_SLAYER_ACCOUNTING_RESEARCH.md).

```sh
.tools/python/bin/python tools/oracle_mp_spawn_lethal_attribution.py \
  --output analysis/multiplayer/integration/spawn-source/lethal-attribution-report-repro.json
```

There are **23 cases**: seven 25-impact source sequences, eleven previous-history
cases and five engine/result gates. Every sequence records all input-event and
normalized-result bytes, full counter/callback identities, per-stage statistics,
unit ownership/dead flags, the four history entries and both random streams.
No instructions or marker/animation poses are patched or fabricated.

## Precisely supplied boundaries

The frozen first-impact producer's complete event bytes are the hit fixture.
World collision, hit finding and projectile travel are not rerun. A fresh
fixture projectile is allocated for each owned hit using the actual shared
object allocator; original `B8820`, `B8D0D..B8D2A` and `BD080` place its captured
owner triple into that event. A projectile captured before an actual leave or
attacker death/rebind is reused for the deferred final hit in those cases. Its
full player/unit fields are not recomputed from the new occupant.

The arena addresses, `0x800` body-storage length and active/type registration
remain explicit datum-suite fixtures. Complete source player/host-property pool
setup runs. Units use the real Lockout Master Chief tag, the original unit/history
constructor prefix, original membership-assignment slice and complete binding.
The source cache supplies unit damage scales; normalized full body/shield values
of one are the health fixture. The source controller/property and player default
constructors produce handicap zero and `+194 == 0`; normal impact scales and
Slayer gameplay defaults come from the frozen producer.

The 25-impact sequences supply hits at ticks `200 + 2*i`, with no recharge,
weapon/reload simulation or world tick between impacts. Self-hit and ownerless
finals are explicit attribution inputs; this report does not claim the physical
stock SMG collision producer ordinarily creates a self-hit. Eleven reader cases
seed previous history by running complete original `CBA60` with explicit varied
amount/tick requests, then supply normalized body `0.001`, shield zero before an
actual original SMG damage entry. Their history bytes are source-produced; their
seed amount requests and low-health state are declared kernel fixtures.

`15E4D0` is a **named supplied death-writer adapter**, not the complete source
writer. At that original call site it forwards the source-selected full attacker,
full victim and relationship byte to **actual unchanged Slayer `2C2350`**, with
callback cause-class `NONE`. The adapter uses the original four-argument stdcall
return to continue `1ED960` directly; it does not start nested emulation. The
ordinary default `options+22C == 2` has no cause-class scoring branch, so that
explicit fourth callback input does not change these ordinary score requests.
Original writer call `15ED04` invokes its engine vtable `+78` callback before
return; the adapter preserves this position relative to subsequent attribution
helpers while omitting marker capture, respawn timer, event/medal/profile and
loadout work. Those omitted writer effects retain their separate frozen contracts.

Original effect/feedback/impulse/animation service leaves inherited from the
frozen damage oracle are named and recorded whenever reached. Lethal execution
ends at `CBE95`, after original `CBDC0` ownership detach, before its remaining
world/type/AI/animation aftermath. The source body-depleted routine runs in the
actual damage path; the victim is dead but still owned while attribution reads
its history. The mode-four gate returns from complete `D9180` without reaching
that detach endpoint.

## Source-generated identity inputs

The actual source pool bootstrap gives players stride `21C`, capacity 16, first
salt `EC70`; the shared object headers have stride 12, capacity `800`, first salt
`E26F`. The source host-property producer assigns FFA property `+7C` from the
membership index; complete `14D350 -> 14D800 -> 14D600` copies it to player
`+C0`. Original unit constructor membership assignment writes signed unit `+138`.
These fields remain separate from the full player datum and full unit datum.

| Role | Full player | Membership `+C0` / unit `+138` | Full initial unit |
| --- | --- | --- | --- |
| attacker 0 | `EC700000` | 0 | `E26F0000` |
| victim 3 | `EC710003` | 3 | `E2700001` |
| attacker 7 | `EC720007` | 7 | `E2710002` |
| attacker 15 | `EC73000F` | 15 | `E2720003` |

These are empty-pool scripted fixtures; complete authored world startup order
is still unresolved. Salts with their high bit set are valid, even though the
32-bit signed representation is negative. `FFFFFFFF` alone is `NONE`.

An actual leave/rejoin of attacker 0 makes its new player `EC740000`. Source
`14DCB0` detaches it, frees its old player record, dispatches object player-removed
callbacks, and source `14D350` allocates the same index with the next global
player-pool salt. Source lookup rejects `EC700000`; the deferred projectile keeps
that old captured player and its old unit. An attacker death/rebind preserves
player `EC700000` while assigning a new object-pool unit generation. Both
transitions are executed; the report does not substitute a manually changed salt.
The attacker death/rebind fixture executes the source dead-state/ownership cleanup
prefix and new-unit constructor/binding. Its separate attacker death accounting,
respawn timer, generic placement/loadout and spawn-count updates are outside this
fixture; it isolates the changed current-unit identity consumed by attribution.

## Result and history input contract

The original normalized result consumed by `DAC40` contains:

| Offset | Type | Contract |
| --- | --- | --- |
| `+00` | byte | packed reporting/cause byte; ordinary SMG is 8 |
| `+04` | 32-bit flags | bit 0 lethal body result; bit `800` suppresses attribution |
| `+08` | full tag datum | original damage effect |
| `+0C` | full player datum | captured incoming attacker, including salt |
| `+10` | full object datum | captured source unit/object, including salt |
| `+14` | signed 16-bit | incoming membership/designator |
| `+44` | float32 | normalized body amount |
| `+48` | float32 | normalized shield amount |

`DAC40` passes the attacker/full victim-unit and the separate designator/cause
into `1ED960`. Its designator stack argument is the zero-extended 16-bit pattern;
relationship routines interpret it as signed short, so `0000FFFF` means -1.
Other byte arguments can have live register values in their unused high bytes.
The report records both raw stack arguments and the semantic low-byte fields;
for example writer cause `0200EF08` is cause byte **08**, not a wider cause code.

History is the original four-entry table at victim unit `2F0 + i*10` (hexadecimal
stride): timestamp, accumulated float32 amount, full source object and full
attacker player, followed by cause bytes at `330 + i`. Original `C52C0` initializes
empty timestamps/identities to `NONE` and amounts/cause to zero. Existing source
writer semantics remain unchanged: match by non-NONE player **or** non-NONE object,
accumulate and refresh time/cause while preserving original identities; otherwise
use an empty entry or protect the first strict maximum and replace the oldest
other entry. This suite feeds that exact table into actual `1ED960`.

## Exact observed stage order

For an ordinary eligible lethal result, the trace and assertions establish:

1. Original `D9180` generates amounts and the normalized result; body depletion
   sets the dead flag while player/current-unit ownership remains present.
2. `DAC40` calls `1ED960` on **previous history**, before the fatal hit is recorded.
3. `1ED960` selects and validates credit. An enemy credit runs `1EDF70` and
   generic stat helper `1ED6A0` for the credited full player, field 2. A nonenemy
   credit instead increments that player's `+1C4` and sets the writer relationship
   byte; it does not receive the enemy-kill field-2 increment.
4. `1ED960` reaches `15E4D0`. The named adapter runs actual Slayer `2C2350` here,
   after credited-kill counters and before assists or victim death counters.
   History at the callback equals history at the reader. Ordinary enemy score
   is +1, selected self score is -1, and credited `NONE` requests no score change.
5. After the writer returns, original assist helpers update qualifying recipients.
   Original `1EE130` updates victim field 3 and, when the **original incoming
   full player** equals the victim, field 4. This comparison is independent of
   the credited player selected for the score callback.
6. `DAC40` continues into `CB410`. The original float32 sum is result `+48` then
   `+44`; `CBA60` records that fatal amount with the incoming identities and cause.
   Every recorded writer amount is asserted against the result's float32 sum.
   No writer is called when both incoming player and source object are `NONE`.
7. The lethal `CBA60` tail updates its source-selected unit `+2EA/+2EC` counter.
   Feedback/cancellation leaves retain their explicit boundaries. `CBDC0` then
   stores the current unit in player `+30`, calls `14E240(player,NONE)`, clears
   player `+2C`, and changes unit `+13C` to `NONE`, `+140` to the full victim.

Source stat records start at engine `304+4`, stride `1C`. Field 0 is score, 2
credited kills, 3 deaths, 4 suicides and 6 enemy assists. Generic stat deltas use
the already frozen signed-short ±30000 clamp. All twelve raw short fields, last
kill tick, player `+1C4`, unit `+2EA/+2EC`, callback identities and exact source
write addresses are recorded. Streak/multikill and the nonenemy history helper
are recorded without inventing additional native names or semantics.

## Original credit selector

The executable source is `1ED960`, with actual `1E2F50 -> 15F680 -> Slayer
253F10` membership relationship callbacks. In ordinary FFA, the Slayer relation
is signed-short membership inequality. A valid supplied direct enemy/designator
prevents other players' history from overriding that direct attacker. Missing
or nonenemy direct input permits validated enemy history candidates. Direct
matching history is considered even for self input; it can win over smaller
other enemy history.

Candidate kill-history timestamps must be **unsigned strictly greater** than
`game_tick - nearest(6.0 * rate)`, and damage must strictly exceed the current
maximum. Equal maxima keep the first entry. The source also retains a direct
matching-entry fallback and finally checks the direct full player's salt. Full
salt validation occurs before credit is committed, even if a direct matching
stale history entry passed the earlier candidate equality test.

A valid direct attacker with no matching history can be credited by the final
fallback, but its history amount threshold remains zero. That path does not
award assists from unrelated history. A stale direct player whose separate
supplied designator identifies an enemy still closes the other-player candidate
branch: a different valid enemy history entry is not a silent replacement for
that stale direct input. With direct `NONE`, the same valid other history can be
selected after stale entries fail their actual full-salt checks.

## Assist threshold, ties and history-age distinction

After a selected valid history entry, the source forms float32 threshold
`selected_amount * 3ECCCCCD` (**0.4**). For another valid full player it admits
an assist candidate when that player is the direct incoming player **or** its
stored amount is strictly greater than this threshold. It then routes the
candidate through the original relationship/helper paths and deduplicates full
player identities before awarding counters.

**This assist loop has no timestamp cutoff.** A record excluded from the recent
kill-credit search can still receive an assist after another history entry is
credited. No expiration service or artificial history clearing is supplied.

At selected amount `3F000000` / 0.5, the threshold is `3E4CCCCD`. Candidate
`3E4CCCCD` / 0.2 does not qualify; the next float `3E4CCCCE` does. These cases use
source `CBA60` to produce the table and actual `1ED960` to decide assists.

## Executed outcomes

All ordinary rows increment victim 3's death field once. Each table states
score/counter differences in addition to that death.

| 25-impact sequence | Source selected credit and outcome |
| --- | --- |
| 25 direct enemy-0 impacts | player 0 kill +1, score +1; attacker-0 unit counter +1 |
| 25 supplied self impacts | victim score -1, suicide +1; no enemy-kill or unit kill-counter increment |
| 24 enemy-0, supplied self final | player 0 kill/score +1; victim suicide still +1; no attacker-unit counter +1 |
| 24 enemy-0, captured old shot after actual leave/rejoin | credit `NONE`; new occupant has no kill/score; old still-valid unit counter +1 |
| 24 enemy-0, captured old shot after attacker death/rebind | same full player 0 kill/score +1; its **new current unit** counter +1; old unit remains in stored history |
| first 20 enemy-0, last five enemy-15 | player 15 kill/score +1, player 0 assist +1; unit 15 counter +1 |
| 24 enemy-0, ownerless final | prior player 0 kill/score +1; no fatal current-hit history write or attacker-unit counter increment |

The unit counter comes from **current incoming identities**, not from the
credited player. `CBA60` first validates the incoming full player and uses that
player's current unit when present; otherwise it validates the captured full
source object. For stock class 2 the fixture's unit redirect `+24C` is `NONE`.
It excludes a dead resolved unit, applies its original four-second tick reset,
then updates signed short `+2EA` and timestamp `+2EC`. Thus old captured-unit
identity can remain in history while the counter goes to a respawned player's
new current unit. If the old player is stale after leave but its captured old
unit remains allocated/alive, that old unit is the fallback counter recipient;
the rejoined player's unit is not substituted.

| Previous-history fixture at tick 200 | Credit / enemy assists |
| --- | --- |
| 0:0.8, 7:0.4, 15:0.2, self:0.9; no direct | 0 credited; 7 assist |
| 0:0.5, 7:exact threshold, 15:next float, self:0.9; no direct | 0 credited; only 15 assist |
| equal recent enemy maxima 0:0.5, 7:0.5 | first entry 0 credited; 7 assist |
| 0:0.8 at exact cutoff tick 20, 7:0.4 at tick 21, 15:0.2 recent | 7 credited; **0 and 15 assist** |
| direct enemy 15, matching history amount 0.1, larger histories 0/7 | 15 credited; 0 and 7 assist |
| direct enemy 15 without matching history | 15 credited; no assists |
| direct self, matching amount 0.9 vs enemy 0.8/0.4 | self credited, score -1, suicide +1; 0 and 7 assist |
| stale direct enemy 0 after leave/rejoin, valid 7/15 history present | credit `NONE`; no assists |
| no direct after the same leave/rejoin, stale 0 and valid 7/15 history | 7 credited; 15 assist; stale player 0 excluded |
| no direct and empty history | credit `NONE`; no assists |
| no direct, tick 2 and previous tick-1 enemy history | unsigned cutoff wraps; credit `NONE`; no assists |

The four-entry self amounts are explicit attribution fixtures. They explain
why “self with enemy history always credits the enemy” is not a complete source
rule. These are not assertions that stock SMG collision creates normal self
history. Expired-for-kill-credit entries remain stored and can matter to assists.

## Executed engine and result gates

Original `DAC40` requires ordinary mode (byte `options+C != 4`), unit type in
mask 3, result lethal bit 0, result bit `800` clear, and non-NULL engine statistic
block from `1636D0` before calling attribution. `1ED960` additionally returns
immediately when that statistic block's valid byte is zero. Native ordinary FFA
must preserve these gates; they are not interchangeable with unit ownership or
an alive flag.

| Explicit varied gate | Observed source behavior |
| --- | --- |
| engine table entry absent | no attribution/death stats/score; history and ownership cleanup still run; no unit counter +1 |
| statistic valid byte zero | reader returns; no attribution/death stats/score; history and detach run; incoming unit counter +1 still occurs |
| mode byte four | actual produced flags `10A`, complete damage return, no attribution/callback/detach; current-unit ownership remains |
| generated result `10B` changed only to `90B` (bit `800`) | attribution suppressed; history, unit counter +1 and detach still run |
| generated result `10B` changed only to `10A` (lethal bit cleared) | attribution and death cleanup omitted; nondeath history still records, ownership remains |

The last two cases first execute original `D9180` up to `DAC40`, capture its
actual generated result, then explicitly vary only `+04` and execute original
consumer instructions. They are gate fixtures, not claims that ordinary stock
SMG impact produces those changed flags. The mode-four row changes both producer
and consumer behavior and is not an ordinary local FFA world claim.

## Native integration boundary

The authoritative ordinary sequence is: retain the captured full incoming
player/object/designator; generate normalized damage; use previous history for
source selection/counters; invoke the selected-credit Slayer callback at the
death writer boundary; finish assists and victim stats; record the current hit
with incoming identities; update its resolved source-unit counter; release current
ownership through the death-cleanup stage. Current incoming attacker, selected
credited player, source object, player's current unit and membership are distinct
identities. Preserve that distinction across player pool reuse and respawn.

This report supplies the source-visible inputs/outputs and ordering for that
slice. It does not close complete world startup/deletion, hit finding, travel,
reload cadence, vehicles/mount redirects, persistent profiles, medal/event delivery,
marker/pose production, later corpse animation or native networking. The default
callback cause-class input is explicitly limited to the ordinary flags used here;
additional scoring variants require their own source writer-class producer.

## Freeze hashes

| Artifact | SHA-256 |
| --- | --- |
| Oracle | `093e30f102597a2440415cadabc6bd3cfd56225dc8f025761742cc6e9d38e7cc` |
| Frozen report | `46d65d088255fc1fc357a84aa6e8d17b6065387327be9c03ba9f392ce0b1c84d` |
| Instruction listing | `73fb9a16ee378d218a8238a5cad833af2821dad8eb216b1ae523d821f21855f7` |

Independent reproduction is byte-identical. Python compilation and whitespace
checks passed. The frozen datum, producer, damage and Slayer reports and every
dependency hash recorded in them remain unchanged.
