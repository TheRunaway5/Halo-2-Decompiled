# Authenticated later-update lobby, local-count and options owners

The owned `u5English/default.xbe` retains the ordinary lobby request's literal countdown/minimum `10/3`, the distinct automatic request's `3/3`, and a join count derived from valid local users across four slots. Its join consumer has a different calling convention. These are independently recovered original instruction contracts; this suite does not execute a ready lobby, join participants or run a countdown.
[Original-byte comparison tool](../tools/research_mp_lobby_later_update.py),
[complete report](../analysis/multiplayer/title-update-1.5/lobby-owners/report.json),
[independent verification](../analysis/multiplayer/title-update-1.5/lobby-owners/independent-freeze-verification.json).

The later executable is the external owned file `/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe`, SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`; its `.text` is `d9c6fd7e7475862b1f19c2da80d7f2f6dc41da53619991002e9546e5f9e629fe`. The pinned authentication artifact reports a valid original retail header signature and all 33 original section hashes. The regenerated decompiler's names and imported annotations are not used to establish any correspondence.
[Authentication evidence](../analysis/multiplayer/title-update-1.5/dlc-authentication.json),
[retained input identities](../analysis/multiplayer/title-update-1.5/lobby-owners/report.json).

## Countdown and local-user count

Old entries below are from the original base disc, with literal bytes verified against its frozen source listings. Later addresses come from complete later bodies and actual owning call destinations, rather than reuse of the old address or its label.

| Owning slice | Base entry | Later entry | Evidence and difference |
| --- | --- | --- | --- |
| Ordinary countdown caller | `25106C` | `25737A` | Complete instruction/branch correspondence; both request sites retain `10/3`. |
| Request helper | `19A78E` | `194F81` | Both actual ordinary call sites identify it; controller stride changes `C70` to `C74`. |
| Caller predicate | `19A902` | `1950F5` | Complete body correspondence; actual state-helper target is changed body `194ABC`. |
| Automatic caller | `24FBEA` | `255E2A` | Actual callee context identifies this changed body; retains the separate `3/3` request. |
| Valid-user getter | `647A0` | `65820` | Complete body correspondence; `D0` stride, valid-byte gate, 12-byte copy. |
| Local join loop | `199C47` | `194449` | Ascending0..3, returned AL controls the count; argument delivery changes to stack. |
| Count forwarding | `59230` | `59E50` | Actual join destination; three stack arguments and `ret C` replace the old register/one-stack-argument ABI. |
| Session slot writer | `64610` | `65690` | Same selected-record operations, proved shared-global relocation and actual identity callee; array-end address changes. |
| Session countdown admission | `64900` | `65980` | Actual helper destination; unchanged gates with the relocated local-user array address. |
| Periodic decrement | `6E360` | `71D70` | Complete instruction/branch correspondence, signed 1000 ms threshold and one decrement. |

At `257388..257390` and `25739C..2573A4`, the ordinary caller pushes minimum 3 then countdown 10 and calls `194F81`. The helper reads `[55D774 + controller*C74]`, rejects NONE, calls `194ABC`, admits state 9 only after `59F90`, and for state 10 reads `1950C3` and requests one fewer only when the current value exceeds the caller's minimum. Its original admission call is `65980`, which requires byte`535F00`, the session at`535F34`, session+`741C` in 3..8, and the relevant valid local-user byte before `5CBD0`. No gate result is supplied here.
[Ordinary caller](../analysis/multiplayer/title-update-1.5/lobby-owners/instructions/base-0025106c-later-0025737a.asm),
[request helper](../analysis/multiplayer/title-update-1.5/lobby-owners/instructions/base-0019a78e-later-00194f81.asm),
[changed state predicate](../analysis/multiplayer/title-update-1.5/lobby-owners/instructions/base-0019a2ce-later-00194abc.asm),
[admission](../analysis/multiplayer/title-update-1.5/lobby-owners/instructions/base-00064900-later-00065980.asm).

The automatic caller `255E2A` retains `3/3` at `255E61/255E63`; its surrounding policy is changed. In particular, the later body moves the preceding EAX result into EBX, calls `64F10`, tests EAX and continues on zero, whereas the base body uses its old stack argument and AL predicate. The report retains every changed instruction. The shared countdown literals do not prove identical automatic eligibility.
[Complete automatic caller](../analysis/multiplayer/title-update-1.5/lobby-owners/instructions/base-0024fbea-later-00255e2a.asm).

`194449` visits indices 0..3 and calls `65820` with ECX=index and EDX=output cursor. True AL increments EDI and advances EDX by 12. It then pushes the output pointer, EDI count and EBX identity/result before calling `59E50`; entry stack+4/+8/+C respectively contain EBX/count/pointer. `59E50` forwards those actual arguments to `72FD0` and returns with `ret C`. `65820` tests `4DA128 + index*D0` and copies 12 bytes beginning one byte later. Thus this original caller does not force two participants. The valid-byte producer, profile/device ownership, successful session membership and subsequent game-player creation remain separate unexecuted requirements.
[Join loop](../analysis/multiplayer/title-update-1.5/lobby-owners/instructions/base-00199c47-later-00194449.asm),
[actual count forwarding](../analysis/multiplayer/title-update-1.5/lobby-owners/instructions/base-00059230-later-00059e50.asm),
[getter](../analysis/multiplayer/title-update-1.5/lobby-owners/instructions/base-000647a0-later-00065820.asm),
[selected-slot writer](../analysis/multiplayer/title-update-1.5/lobby-owners/instructions/base-00064610-later-00065690.asm).

`71D70` preserves the observed-count/timestamp fields+18/+14. It reads cached network time at`51D04C` when flag`51D048` is set, otherwise calls `33B590`; signed elapsed time must reach 1000 ms, and seconds must remain positive, before the single decrement and `5CBD0` request. `79690/796B0` are the corresponding cached-time getters. The original thunk `33B590` loads through `41B774`, whose original encoded import bytes identify ordinal 156; no kernel data backing, host time or cached timestamp is supplied. Runtime clock cadence and zero-countdown completion are not proved by this comparison.
[Periodic original body](../analysis/multiplayer/title-update-1.5/lobby-owners/instructions/base-0006e360-later-00071d70.asm),
[time getter](../analysis/multiplayer/title-update-1.5/lobby-owners/instructions/base-00075870-later-00079690.asm),
[original data-import thunk](../analysis/multiplayer/title-update-1.5/lobby-owners/instructions/base-003314b0-later-0033b590.asm).

## Options and map/control readiness

These entries use frozen TU1.1 original bytes as their old source. The table identifies owning later routines; it does not import the earlier suites' emulated game/platform state into this executable.

| TU1.1 entry | Later entry | Owning slice |
| --- | --- | --- |
| `139880` | `13ABF0` | Options initializer |
| `139540` | `13A8B0` | Options copy into game+8 |
| `190A30` | `191DF0` | Whole bounded map-start body |
| `139410` / `1395B0` / `11D780` | `13A780` / `13A920` / `11E8E0` | Reset/options, membership production and BSP startup |
| `68300` / `69170` / `69E90` | `694E0` / `6A350` / `6B070` | Readiness, simulation-phase dispatch and transition |
| `188AD0` / `1884E0` / `187610` | `189E90` / `1898A0` / `1889D0` | Local-command owning call chain |
| `69280` / `852A0` | `6A460` / `884F0` | Command scheduling and delivery |
| `147CC0` / `147D20` | `149050` / `1490B0` | Clock storage constructor and reset |

`13ABF0` clears 0x1118 bytes, reads signed-short config`4922E0`, uses 60 when it is nonpositive and divides the positive/fallback value by two into options+6. `13A8B0` copies the 0x1118 bytes to game+8 through global`4F3218`, placing that value at game+0E. This is the original producer relationship; no runtime config value, game rate or readiness is assigned. The random-ID callee's full later behavior is not established by this initializer match.
[Options initializer](../analysis/multiplayer/title-update-1.5/lobby-owners/instructions/tu1_1-00139880-later-0013abf0.asm),
[options copy](../analysis/multiplayer/title-update-1.5/lobby-owners/instructions/tu1_1-00139540-later-0013a8b0.asm).

`191DF0` calls original reset, membership and BSP-startup owners in the retained order. It writes game+1120=1 only on the defined true-AL return from `11E8E0`; the false path calls `13A7E0` and returns false AL. `694E0` retains the flag/pointer/game+1120 gates and its subsequent actual predicate calls. Neither the successful BSP return nor those predicates are supplied or executed. `6A350` retains state dispatch including state 4 at simulation+18; no state 4 byte is inserted to admit local commands.
[Map-start body](../analysis/multiplayer/title-update-1.5/lobby-owners/instructions/tu1_1-00190a30-later-00191df0.asm),
[readiness body](../analysis/multiplayer/title-update-1.5/lobby-owners/instructions/tu1_1-00068300-later-000694e0.asm),
[simulation-phase dispatcher](../analysis/multiplayer/title-update-1.5/lobby-owners/instructions/tu1_1-00069170-later-0006a350.asm).

## Freeze and limits

The comparison contains 59 requested routines: 42 complete normalized instruction/branch correspondences, 10 changed bodies identified through original owning contexts, and 7 unresolved owners. All 52 identified later bodies and every old body are retained; the two available unadmitted prefix candidates are also retained. All 113 listings, 8,039 literal instruction rows and both independently produced reports repeat byte-for-byte. All 60 paired calls whose targets have identified owners agree, with zero mismatches. The 490 prior immutable input pins are verified before and after; there are zero guest instructions executed, patches, supplied platform/gameplay values or production changes.
[Complete differences, operands, calls, data bytes and pins](../analysis/multiplayer/title-update-1.5/lobby-owners/report.json),
[repeat and original-byte audit](../analysis/multiplayer/title-update-1.5/lobby-owners/independent-freeze-verification.json).

Arithmetic/count/timing literals stay exact during candidate matching. Only MOV/PUSH image-address candidates and image memory operands normalize. The changed slot writer's array-end comparison is independently related to the already proved user-array global with the same byte offset; its literal change remains recorded. Every static reference includes original bytes and whether it falls in a raw-backed or zero-filled virtual section. Those virtual zeros do not establish initialized runtime state.

Unresolved requested base owners are `6E410`, `14F190`, `18FB34`, `18F58C`, `18FEE9`, `190D4B` and `64690`. Candidate later bodies `71E20` and `151C90` remain explicitly unadmitted. Full later call-closure behavior, actual profile/controller/local-user initialization, admitted membership, countdown lifecycle, clock and world/control readiness remain open. Xbox source instructions also do not establish SDL desktop window ownership. The native one-window fix is a separate host implementation concern.

```bash
.tools/python/bin/python tools/research_mp_lobby_later_update.py \
  --output analysis/multiplayer/title-update-1.5/lobby-owners/reproduction/report.json
```
