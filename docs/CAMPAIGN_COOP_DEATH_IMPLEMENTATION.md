# Campaign cooperative death and spectating

The shared death camera now has the original campaign target, fallback and
watch policy. The cooperative respawn scheduler is also compiled, with its
nested safety, player-birth and placement owners explicit. **Playable campaign
co-op, for either split-screen or network play, remains unfinished.** These
components are not a campaign session implementation.

## Original authority

The owned English Xbox TU1.5 executable is SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The owned Map Pack1.1 executable used for independent component execution is
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
Complete normalized instruction and internal branch topology agree for all
13 selected owners, totaling851 original instructions. Direct calls and image
operands are retained separately in the
[correspondence report](../analysis/campaign/coop-death-20261013/correspondence-final/report.json).
The short1CE060 predicate was located through the authenticated14E510 caller,
then compared in full; no address delta was assumed.

| Owner | Map Pack1.1 | English TU1.5 |
|---|---:|---:|
| Non-multiplayer teammate restriction | `23FCA0` | `246E90` |
| Last eligible type6 fallback | `23FE30` | `247020` |
| Occupied object iterator | `BBDA0` | `BCEE0` |
| Cooperative-mode query | `139F90` | `13B300` |
| Cooperative respawn permission | `13A010` | `13B380` |
| Campaign failure-state transition | `13A0D0` | `13B440` |
| Campaign failure update | `13A5B0` | `13B920` |
| Begin cooperative respawn request | `14E4C0` | `14F8A0` |
| Cooperative respawn scheduler | `14E510` | `14F8F0` |
| Place new player near survivor | `1500B0` | `151490` |
| Create/restore player unit | `1509C0` | `151DA0` |
| Animation/action safety predicate | `CD5C0` | `CE650` |
| AI combat safety predicate | `1CE060` | `1D2920` |

## Implemented components

`23FCA0` enables team restriction whenever another occupied player shares the
owner's team. That other player need not have a current unit. The separate
`23FD40` target selection still requires a current unit. Thus a dead teammate
can keep a living enemy from becoming the campaign spectator target.

Campaign construction and replacement use the original three-second watch
operand; multiplayer retains fifteen seconds. Jump cycling retains the
original one-second admission and the initialized/paused clock gate. When
participant replacement still yields no target, campaign invokes `23FE30`.
Its actual object-pool traversal selects the last occupied type6 object with
no parent and a cluster other thanNONE. Missing/retired targets still go
through the real full-datum lookup; no root or eye position replaces WORLD
node0.

`NativeDeathCamera` now reads the current source game-options owner to select
the policy. Campaign also observes player flag8 in the death/ordinary camera
gate. A new current unit restores ordinary camera ownership. The camera does
not choose a policy from the transport or number of local views.

`source_campaign_coop.cpp` ports the complete13A010,14E4C0 and14E510 control
flow and ordered retained-state writes. A new request sets pending, clears
bypass and converts the source tick rate times the original delay through its
SSE/x87 arithmetic. Existing requests retain their timer. The update preserves
cinematic timer suppression, the last countdown tick, reversion and the
source status values2/3/4/5/6/8. It retains a pending request without a survivor
or after unsuccessful birth/placement. The scheduler uses the original last
eligible survivor and its actual position;1500B0 must own the final placement.

The permission predicate rejects independent campaign respawns at difficulty3
or when original override4FCDBF is set. The producer of that override remains
external and is not named or inferred from a debug toggle. The authenticated
13A5B0 caller selects the any-dead flag when cooperative respawns are disabled,
and otherwise the all-dead flag. Its failure timer/checkpoint handoff is not
yet connected to a native cooperative session.

## Validation and boundaries

The independently executed originals supply125 lifecycle cases with930
ordered stores,7 campaign constructors and39 packets with complete retained controller state,
and45 respawn-permission/request/scheduler cases with52 ordered stores. The
original safety predicates execute on declared raw action/AI records without
instruction patches or supplied returns. The native scheduler comparison
treats its19 named safety calls as explicit component service boundaries.
Birth and safe placement are not supplied by those tests.

The native TU1.5 camera policy probe uses actually constructed multiplayer
objects and players, then delivers campaign type as a declared camera-only
input. It checks flag8, the three-second timer, a living target and restoration.
It does not establish campaign object birth, a playable campaign session or
network co-op. Camera packet experiments explicitly supply zero-initialized
output storage each frame; they do not establish the original caller's
unwritten stack bytes.

All14 selected private checks pass, including the existing local/UDP death,
spectating and GPU-render regressions. The final component recheck includes
the additional seven constructor comparisons. A private negative control
that forces the previous multiplayer policy fails the campaign flag8 check;
the source-options implementation passes. Logs and source manifests are under
[`analysis/campaign/coop-death-20261013`](../analysis/campaign/coop-death-20261013/).

## Remaining live integration

The active [campaign AI/co-op goal checklist](CAMPAIGN_AI_COOP_GOAL_CHECKLIST.md)
tracks the shared-world, script, participant, safe-respawn and network work.

- `Simulation` still owns one campaign player. The October20 source-backed
  `players` query now snapshots every occupied source player's non-NONE current
  unit in original list order, and `game_is_cooperative` reads the actual source
  campaign/options byte. Normal setup still creates a solo session; the query
  increment does not create additional players or cooperative readiness.
- Campaign input and rendering in `main.cpp` still consume one local player
  and one view. Frontend co-op presentation does not launch a native session.
- A cooperative owner must retain both players, their detached corpses,
  controls, script ownership, BSP streaming and checkpoints through death,
  birth, level changes and restoration.
- The scheduler requires live animation/action, AI safety, player-birth and
  placement services. They throw when unavailable; no successful fallback
  allows a guessed respawn location or unsafe spawn.
- Network co-op needs authoritative campaign state and checkpoint/BSP
  synchronization. Multiplayer camera snapshots alone do not supply those
  owners. Split-screen and network must feed the same campaign rules.

No installed launcher is promoted as a co-op build by this work. Validation
uses private objects linked with the previously validated native libraries,
preserving the shared checkout's concurrent work and its existing delivery.
