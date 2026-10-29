# Multiplayer event-message producer audit — 2026-10-28

This is a read-only audit of the native source at the start of this work. It
covers the shared path used by all seven original multiplayer families, rather
than treating CTF as the whole feature. No game, emulator, compiler or test was
run for this note.
The audit establishes connected and missing code paths; it does not claim that
every event in every mode has been exercised during a played match.

## Implementation follow-up

The missing generic join described below is now implemented by
`src/ui/hud/source_event_messages_tu15.cpp` and
`src/ui/hud/multiplayer_event_messages_tu15.cpp`, called by the runtime's shared
engine-event consumer before its separate audio-owner gate. Local presentation
queues retain source recipient and split-screen rules and are restored with
receipt continuation. `MultiplayerHud` accepts that local queue independently
of the authority snapshot and draws queued event text while dead. The audit
below records the original starting state; authored entries without a qualified
producer are still not assertions that every catalog row has been played.

## Main finding

At the initial audit, the native game already retained source multiplayer
events, per-player literal message queues, message timing, network transport
and a message renderer. The generic event-to-text join was missing: the only
production call to `source_hud_message_submit` was the blocked-teleporter
consumer. Generic event caption/display keys were selected by the announcer,
but its caller consumed sounds rather than submitting text.
[Literal producer](../src/gameplay/multiplayer/multiplayer.cpp#L1971),
[selected keys](../src/ui/vista/native_vista_announcer.cpp#L327),
[runtime consumer](../src/gameplay/multiplayer/multiplayer_runtime.cpp#L533).

## Shared production and delivery

1. The canonical match registers an event observer before the source-owned
   ordinary gameplay frame. Completed events receive a native delivery serial
   and are kept in a 64-event journal. This is conditional on
   `source_engine_owned_`, not on being an online session.
   [Ownership predicate](../src/gameplay/multiplayer/multiplayer.cpp#L353),
   [observer registration](../src/gameplay/multiplayer/multiplayer.cpp#L629).
2. `PhysicsLiveWorldTu15::required` observes real calls to `1A8740` and
   `1A87A0`, preserves the eight event words plus final halfword, original tick,
   random state, raw world mode, game-over flag, and actual player identities,
   flags and membership. It runs the original owner before publishing the
   observation. The `1A87A0` mode-4 suppression is retained.
   [Boundary](../src/physics/physics_live_world_tu15.cpp#L1293).
3. Native snapshots carry both the generic journal and each player's four
   literal message rows. Received rows are validated. Thus the transport can
   already carry recipient-specific formatted text; adding a client-wide
   broadcast would discard an existing recipient boundary.
   [Message serialization](../src/network/multiplayer_network.cpp#L959),
   [journal serialization](../src/network/multiplayer_network.cpp#L1204),
   [message receive](../src/network/multiplayer_network.cpp#L1670).
4. The current journal drain selects audio once per unseen serial. Its late-join
   path skips historical broadcasts while retaining an actual targeted
   admission. The offline frontend may own a separate legacy Slayer/King audio
   queue. Neither policy is evidence that event text should be online-only.
   [Drain and gates](../src/gameplay/multiplayer/multiplayer_runtime.cpp#L533).

## Existing original text owner

The lifted original `1A83B0` body already contains the source event text branch.
It reads the actual player's local-view index at `player+28`, rejects a missing
local view, and suppresses post-game events except general event 19. It obtains
the response's `+0C` text key, resolves it with `1A9D90`, and formats the event
with `1A7C10`. Depending on response flags, it then calls special message
submission `25A9AF` at `1A8567`, or literal `25AAF1` at `1A857B`. A separate
response `+14` path resolves/formats text and calls `25AB76` at `1A85DF`, using
the response's `+18` value as a duration input.
[Lifted owner](../src/physics/physics_live_support_tu15_generated.cpp#L111419).

The current native literal output capture only admits `25AAF1` when the caller
is **teleporter** `1A65DB` and a teleporter notice capture is active. It does not
collect generic `1A857B`, special `25A9AF`, or timed-display `25AB76` outputs.
Do not replace this with handwritten kill strings before recovering the source
formatting/row contracts. The existing `SourceHudMessages` validator only admits
literal row flags (`+84/+85` must be zero), so the special row cannot simply be
recast as the existing literal representation.
[Restricted capture](../src/physics/physics_live_world_tu15.cpp#L1454),
[literal flags and validation](../src/ui/hud/source_hud_messages.cpp#L30).

## Authored responses are not producer triggers

The source generator's registered families are CTF (source engine 1), Slayer
(2), Oddball (3), King (4), Juggernaut (7), Territories (8), and Assault (9).
These engine selectors must not be confused with event-category numbers or
recipient roles. The announcer loads all original response-bank categories
0–10 except unused categories 5 and 7.
[Registered engines](../tools/generate_multiplayer_engines_tu15.py#L27),
[response banks](../src/audio/source_announcer_assets_tu15.cpp#L21).

The independent [response-key census](../build/test-results/mp-event-messages-20261028/source-research/response-keys.json)
establishes which recipient responses are authored. It does **not** establish
that a native gameplay trigger has fired, that the primary key is nonempty, or
that a literal feed line is appropriate. The following is an authored catalog;
the producer table below has a separate evidentiary meaning.

| Original family | Engine selector | Event category / runtime `mulg` bank | Authored event subtypes |
| --- | --- | --- | --- |
| Slayer | 2 | 2 / `+A8` | 0 start; 1 new target |
| CTF | 1 | 3 / `+B0` | 0 start; 1 taken; 3 player return; 4 timeout return; 5 captured; 6 new defensive team; 7–9 textless cues; 12 contested; 13 capture failure |
| Oddball | 3 | 4 / `+B8` | **Only 0 start, 2 pickup, 4 reset** |
| King | 4 | 6 / `+C8` | 0 start; 1 controlled; 2 contested; 4 moved; 5 team controlled; 6 team contested |
| Juggernaut | 7 | 8 / `+D8` | 0 start; 1 new Juggernaut |
| Territories | 8 | 9 / `+E0` | 0 start; 1 gained; 2 lost; 3 all controlled; 4 team gained; 5 team lost; 6 all team controlled |
| Assault | 9 | 10 / `+E8` | 0 start; 1 taken; 2 dropped; 3 player return; 4 timeout return; 5 scored; 6 new defensive team; 7–9 textless cues; 12 defused return; 13 placed; 14 arming started; 15 arming complete; 16 contested |

Shared category 0 (`+98`) contains kill 0, suicide 1, betrayal 2, victory 3/4,
score-time warnings 7–10 and 47/48, quit 11, joined 12, unknown killer 13,
remaining-match-time warnings 14–17 and 32/33, game over 19, textless cues
20–22, changed team 23, rejoined 24, lead changes 25–30, round over 31,
falling 34, collision kill 35, melee 36, sudden death 37, boot 38,
flag/bomb carrier kill 39/40, sticky 41, sniper 42, assassination 43,
boarding 44, starting-team notice 45 and telefrag 46. Shared flavor category 1
(`+A0`) contains double/triple/killtacular 0–2, spree/riot 3/4, broken spree 6,
frenzy 7, killtrocity/killimanjaro 8/9, and 15/20/25-kill awards 10–12.
These names are read from authored response keys, not inferred from incremented
statistics.

In particular, **CTF has no authored subtype-2 response**, although the actual
drop producer constructs subtype 2. Oddball likewise has spawn-1 and drop-3
producers without corresponding authored response rows. No feed line should
be invented for either gap. Empty primary keys are per-response/channel
facts: general 20–22 and CTF 7–9 have no primary text, while general 37 has an
empty primary key and a secondary `gen_sudden_death_all` key. Territories start
and Assault pickup also include a textless recipient row alongside other
authored text rows. A sound-only response and a timed secondary display are
different from an ordinary literal message.

## Source-confirmed producer paths

Here, “confirmed” means the complete source body constructs the stated packet
and reaches the genuine `1A8740`/`1A87A0` owner, with its conditions preserved.
It is static producer evidence, not a claim that every listed transition has
been observed during a played match or that the text join is accepted.

The frozen original fixture's `base_banks` independently contain the actual
registration cells, objects and 51-entry vtables. Reading those bytes gives
the following callback routes. Slot 6 receives activation; slot 30 is the
mode's death/stat/score/role callback used by shared `15F8B0`; slot 31 may
override the shared death-message subtype. The callback contract is common
across the seven families, even where a particular slot has no work.
[Fixture provenance](../tests/fixtures/multiplayer_engines_tu15.md#L8),
[generation from original vtables](../tools/generate_multiplayer_engines_tu15.py#L93).

| Family | Activation, slot 6 | Death callback, slot 30 | Mode-specific producer paths |
| --- | --- | --- | --- |
| CTF | `249600` | `24A1E0` | Pickup `24A2F0` → 1; drop `24A4B0` → 2; touch return `24A910` → 3; objective timeout/return `249B20` → 4; successful goal `2496B0` → `24C0F0` → 5; selected defense `24C540` → 6; contest `249B20` → 12; blocked goal `24C390` → 7 or 13. Category 3. |
| Slayer | `2C5D70` | `2C5DD0` | Activation produces category 2/subtype 0 at `2C5DC0`. Shared combat events and mode scoring remain separate from the authored new-target row; the latter is not qualified here. |
| Oddball | `2C61E0` | `2C6A80` | Activation 0 at `2C6230`; actual item spawn `2C6060` → 1; pickup `2C6290` → 2 at `2C6352`; drop `2C6360` → 3 at `2C640B`; object reset `2C6800` → 4 at `2C6933`. Category 4. Its subtype-5 periodic producer at `2C6A00` is also not an authored text response in this census. |
| King | `2C7070` | `2C7820` | Activation category 6/subtype 0 at `2C70BE`; hill-state transition owner `2C7270` selects controlled 1/5 or contested 2/6 and dispatches at `2C7390`/`2C7478`; hill movement owner `2C7490` dispatches subtype 4 at `2C7610`. |
| Juggernaut | `198250` | `1982B0` | Activation category 8/subtype 0 at `1982A0`; actual role-assignment owner `198980` produces subtype 1 at `1989DE`. Death owner also emits category 8/subtype 2 at `19832F`, without a matching authored row in this census. |
| Territories | `2C9240` | `8EB50` | Activation category 9/subtype 0 at `2C928E`; actual ownership-change owner `2CA5C0` compares prior/current owners, produces lost 2/5 at `2CA754` and gained 1/4 at `2CA7E7`, carrying the territory index in the final halfword. Authored all-territories rows 3/6 still require their distinct trigger trace. |
| Assault | `249600` | `24A1E0` | Shares the CTF objective class, selecting category **10** for original engine 9. In addition to common pickup/drop/return/capture/defense events, `2496B0` produces arming 14/15 at `24985B`/`2498F9`; `24BE90` produces placed 13 at `24C0B4`; `249B20` produces contest 16. Planted-bomb completion `24B950` reaches `24C0F0` for scored subtype 5. |

Sources: [activation category 3/10](../src/gameplay/multiplayer/source_ctf_activation_spawn_tu15_generated.cpp#L353),
[CTF/Assault event-category mapping](../include/halo2/source_objective_native_tu15.hpp#L84),
[objective pickup/drop](../src/gameplay/multiplayer/objective_native/item_events.hpp#L43),
[touch return](../src/gameplay/multiplayer/objective_native/touch_return.hpp#L27),
[objective timeout/contest](../src/gameplay/multiplayer/objective_native/object_tick.hpp#L24),
[arming and goal](../src/gameplay/multiplayer/objective_native/player_tick.hpp#L55),
[placed-bomb completion](../src/gameplay/multiplayer/objective_native/return_reset.hpp#L16),
[Oddball](../src/gameplay/multiplayer/source_multiplayer_engines_tu15_generated.cpp#L23537),
[King](../src/gameplay/multiplayer/source_multiplayer_engines_tu15_generated.cpp#L24799),
[Juggernaut](../src/gameplay/multiplayer/source_multiplayer_engines_tu15_generated.cpp#L12624),
[Territories](../src/gameplay/multiplayer/source_multiplayer_engines_tu15_generated.cpp#L28457).

### Actual kill, suicide and betrayal

The real attribution owner `1F2280` reaches shared multiplayer death owner
`15F8B0` at instruction `1F2740`. `15F8B0` reads the actual source player rows,
credited killer/victim/relationship and packed damage-report inputs. Its
classification branch `15FC4D`–`15FD3E` chooses ordinary kill 0, self kill 1,
relationship kill 2, falling 34, collision 35, melee 36, sticky 41, sniper 42,
or assassination 43. The special subtype does not come from a HUD-side weapon
guess. In the packed source report, the low six bits are the damage kind and
the upper two are the classification: collision 35 is kind 2 or classification
3; sticky 41 is kind `16h` with classification 2; sniper 42 is kind `0Dh` or
`0Eh`; melee 36 and assassination 43 use the source melee-kind branch, with
classification 2 selecting assassination. Earlier missing-killer, self,
relationship and suppressed-death branches take precedence.
[Attribution caller](../src/physics/physics_live_support_tu15_generated.cpp#L139017),
[packed inputs](../src/physics/physics_live_support_tu15_generated.cpp#L89565),
[classification](../src/physics/physics_live_support_tu15_generated.cpp#L89790).

Before publication it calls original vtable slot 31 at `15FD63`. In CTF and
Assault that callback is actual `24A290`, which checks the real held objective
and substitutes general subtype 39 or 40 for an eligible carrier kill. The
packet then contains category 0 at `+00`, selected subtype at `+04`, actual
killer at `+0C` and victim at `+14`, with their actual team membership at
`+10/+18`; recipient `+08` is NONE for the source recipient iterator. It
checks raw world mode !=4, sends `B1E80` at `15FDEB`, then calls the observed
`1A8740` boundary at **`15FDF4`**. Separately, slot 30 is called later in the
death owner at `1600E4` to perform mode-owned score/stat/role effects. It is not used as
a score-difference substitute for this packet.
[Event construction and real dispatch](../src/physics/physics_live_support_tu15_generated.cpp#L89864),
[carrier override](../src/gameplay/multiplayer/objective_native/carrier.hpp#L22).

Category 1 medal production is also inside this death owner: after an actual
nonself, nonrelationship credited kill, source retained counters select a
flavor subtype, `1A87F0` fills the actual cause, and `1A87A0` is called at
`1600AD`. This is an independent event from the ordinary kill packet, so a
single death may legitimately produce both channels. The authored medal list
alone does not show which threshold branch executed in a particular match.
[Source flavor publication](../src/physics/physics_live_support_tu15_generated.cpp#L89905).

### Actual CTF capture

Per-player callback `2496B0` requires an actual current unit/held objective,
the original held-item class, an active game and a valid objective team. It
queries the real goal with `24C1E0`, applies the source flag-at-home option,
and takes the blocked-capture notice path when that condition fails. After
the original authoritative-completion gate succeeds, the CTF branch calls
real score/stat owner `19BC10` at `249A06`, then `24C0F0` at **`249A11`** with
the actual capturing player and objective team. These are the point and
event paths in source order; no observer compares before/after scores.
[Goal and home checks](../src/gameplay/multiplayer/objective_native/player_tick.hpp#L81),
[score and capture call](../src/gameplay/multiplayer/objective_native/player_tick.hpp#L111).

`24C0F0` also records the original CTF/Assault capture statistic. It writes
category 3 (10 for Assault) and subtype 5, actual capturing player at `+0C`,
the player's membership at `+10`, and actual objective team at `+18`. In raw
mode !=4 it sends `B1E80` at `24C1BD`, then `1A8740` at **`24C1C6`**. That
generic boundary and the subsequent per-recipient response selection are
the correct integration point for the user's “someone scores a point”
example. Assault's planted completion reaches the same event owner from
`24B950` at `24BA31` after checking the actual planted state and scoring
player identity.
[Capture owner](../src/gameplay/multiplayer/objective_native/events.hpp#L27),
[Assault completion](../src/gameplay/multiplayer/objective_native/return_reset.hpp#L16).

### Shared phase, admission and custom-mode boundaries

Additional source-confirmed shared producers include round over 0/31
(`15E3A0` → `1A8740` at `15E414`), game over 0/19 (`15F450` → `1A87A0` at
`15F589`), sudden death 0/37 (`164EF0` → `1A8740` at `164FA6`), and original
remaining-time warnings (`15E870` → `1A87A0` at `15E99C`). The recovered
`1A9080` score-warning owner uses exact remaining values 10/30/60 for general
47/9/7, adding one for teams. Its real dispatches are `1A9130`, `1A91D7`,
and `1A927C`. The separate `source_score_changed_event` helper preserves
these semantics but is not itself proof of a callback connection.
[Round over](../src/physics/physics_live_support_tu15_generated.cpp#L89441),
[game over](../src/gameplay/multiplayer/source_multiplayer_engines_tu15_generated.cpp#L9654),
[sudden death](../src/gameplay/multiplayer/source_multiplayer_engines_tu15_generated.cpp#L10551),
[time warnings](../src/gameplay/multiplayer/source_multiplayer_engines_tu15_generated.cpp#L8798),
[score-warning owner](../src/gameplay/multiplayer/source_multiplayer_engines_tu15_generated.cpp#L13728).

Joined 0/12 is constructed by actual player admission `15A660` and sent through
`1A8740` at `15A759` only after the original elapsed-world-time test; initial
roster setup is not evidence for a joined line. Changed-team 0/23 is produced
by `15AD50` at `15AE41`, requiring the original team-mode and source-tick
conditions. The native join path constructs real players, and native departure
calls real `88C50` with the current player packet. The authored quit/rejoin/boot
rows are catalogued above; a complete departure/rejoin producer and client
visibility qualification is still distinct work. The shared death owner has
an actual departed-player flag branch selecting 0/11; an unowned departed body
must not be converted to a fabricated kill to force the notice.
[Admission condition and packet](../src/gameplay/multiplayer/source_multiplayer_engines_tu15_generated.cpp#L6910),
[team change](../src/physics/physics_live_support_tu15_generated.cpp#L87851),
[actual native departure](../src/physics/physics_live_world_tu15.cpp#L3067),
[departed flag selection](../src/physics/physics_live_support_tu15_generated.cpp#L89794).

Custom **Bomb Defusal** is separate from the seven original families. It is a
native ruleset selected by an additional parameter in frontend Assault engine
5, with its own `BombDefusalState` and status formatting. It is explicitly
excluded from automatic original System Link selection. Category-10 Assault
responses establish the original bomb behavior; they do not establish a full
custom defusal pickup/plant/defuse/round-win text specification. That custom
mode needs an explicit trigger/channel design and separate acceptance, while
ordinary combat messages can use whatever actual shared source events its
runtime legitimately produces.
[Custom selector](../src/gameplay/multiplayer/bomb_defusal.cpp#L53),
[custom state/status](../src/gameplay/multiplayer/bomb_defusal.cpp#L199),
[System Link selection](../src/gameplay/multiplayer/multiplayer_runtime.cpp#L437).

## Queue, renderer and validation boundaries

The current queue keeps four literal UTF-16 rows, uses source ticks, truncates
to the original 62-character copy contract, replaces the oldest occupied row,
and retains the original sequence ordering and nonlinear fade. HUD globals
supply hold/fade and line spacing. The current renderer displays up to four
rows in a full viewport, three in split-screen, or two split-screen rows when
an action prompt is present. Only the unit HUD owns this shared text pass;
weapon/offhand HUDs disable it to avoid duplicate messages.
[Queue](../src/ui/hud/source_hud_messages.cpp#L30),
[authored durations](../src/ui/hud/hud_content.cpp#L125),
[renderer](../src/ui/hud/multiplayer_hud.cpp#L1367),
[unit/weapon ownership](../src/gameplay/multiplayer/multiplayer_runtime.cpp#L1759).

There are two render gates requiring an independent original-behavior check
before promising death-screen message visibility: `MultiplayerHud::draw`
returns before drawing messages for dead players, and the runtime submits the
normal HUD only when the scoreboard's `normal_hud` gate is true. This audit does
not establish whether those gates match original event-message visibility.
[Dead-player gate](../src/ui/hud/multiplayer_hud.cpp#L1042),
[scoreboard gate](../src/gameplay/multiplayer/multiplayer_runtime.cpp#L7691).

The tests present at initial audit qualify literal queue/sort/fade behavior and
teleporter output delivery, not the missing generic message join. The source-call boundary test
covers actual Slayer activation in raw modes 1, 3 and 4. None should be presented
as all-mode kill/objective HUD-message acceptance.
[Queue fixture test](../tests/source_hud_messages_tu15_tests.cpp#L17),
[full-frame local/remote teleporter test](../tests/multiplayer_engine_frame_events_tu15_tests.cpp#L32),
[native activation call-boundary test](../tests/source_multiplayer_event_boundary_tu15_tests.cpp#L36).

The existing engine-combat test supplies useful upstream coverage separately:
its default cohort gives ordinary aim/fire controls to seven representative
mode families, requires actual `1F2280` attribution and `15F8B0` stores, and
checks matching live/retained killer/victim statistics. Its optional
`HALO2_COMBAT_ENGINE_EVENT_WIN_FLOW` branch explicitly supplies a declared
whole-`15F8B0` death input and therefore has a different attribution scope.
Neither branch currently establishes selected event text, per-recipient rows
or displayed message acceptance merely by proving the death counters.
[Declared scopes](../tests/multiplayer_engine_combat_tu15_tests.cpp#L40),
[actual owner checks](../tests/multiplayer_engine_combat_tu15_tests.cpp#L194).

Only this small Markdown report was written for the audit. No scratch build,
probe, emulator output, binary or capture was generated or retained.
