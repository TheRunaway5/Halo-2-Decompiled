# Original lobby countdown and local-user caller evidence

Untouched base-disc instructions establish the ordinary lobby caller's literal countdown/minimum arguments `10/3`, a separate automatic caller's `3/3`, and a local join count derived from valid users rather than a forced two players. This is frozen static evidence: no session state, controller ownership, profile, readiness, countdown or clock is supplied or executed. The complete original menu/lobby/countdown lifecycle and actual local launch participant count remain open.
[Independent source-only tool](../tools/oracle_mp_spawn_lobby_source.py),
[report](../analysis/multiplayer/integration/spawn-source/lobby-source/report.json),
[verification](../analysis/multiplayer/integration/spawn-source/lobby-source/independent-freeze-verification.json).

## Countdown caller and owning gates

Base `25106C` tests `19A902` and has two paths. Both call `199F34`, load the controller argument into EAX, push minimum3 then countdown10 and call `19A78E` at `251082` or `251096`. The second path requests sound2 if the helper's defined AL is false. The countdown's literal arguments are direct original bytes; they do not establish when a valid lobby admits the request.
[Complete caller](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/0025106c.asm),
[actual request helper](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/0019a78e.asm),
[caller state predicate](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/0019a902.asm).

`19A78E` reads the controller's session-user index from `54E8E0 + controller*C70 +4` and rejects NONE. State9 additionally requires `592F0`, then forwards its caller's countdown to `64900`. State10 reads `19A8D0`, compares the current seconds against the caller's minimum and requests one fewer only when greater. `64900` requires the session interface, a live session state in3..8 and the corresponding actual local-user valid byte; it forwards through `5BF10`. The original request, remote-host and countdown-store paths are listed, without replacing these gates.
[State predicate](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/0019a2ce.asm),
[session admission](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/00064900.asm),
[host/remote request](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/0005bf10.asm),
[actual countdown writer](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/0005df30.asm).

Base `24FBEA` has a different automatic branch that supplies `3/3`, under its own state and readiness predicates. These two contexts must remain distinct.
[Separate caller](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/0024fbea.asm).

## Local users and counts

Base `199C47` visits user indices0..3 ascending. It calls `647A0` with ECX=index and EDX=the current output cursor, increments EDI only when AL is true and then advances that output cursor by `C`. At `199C7F` it passes ECX=EDI to `59230`. Thus this join caller uses the valid returned-user count; four slots do not imply two participants.
[Join loop](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/00199c47.asm),
[related join path](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/00199c94.asm),
[count consumer](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/00059230.asm).

Original `647A0` tests the byte at `4CD904 + index*D0`. On true it copies exactly12 bytes from the following address and returns AL1; on false it returns AL0. The actual12-byte output/stride is the authority here, rather than the reference reconstruction's `XUID` type label.
[Exact getter and three dword copies](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/000647a0.asm).

Original `18F58C` clears the local controller table and writes NONE at each `+4`. `18FB34`, when controller flag10 is clear, obtains its12-byte identity from `14B410`, calls `64610` and stores the returned session-user index. `18FEE9` separately creates the index when `+4` is NONE. `64610` searches actual valid slots, clears its selected `D0` record, sets valid and copies the12-byte identity. Its NULL-identity path calls the same original identity producer. All are statically listed; their profile/device/platform and initialization context is not supplied here. In-game `14F190/14F270` separately own actual local-user/controller attach/detach count updates.
[Controller initializer](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/0018f58c.asm),
[profile application producer](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/0018fb34.asm),
[other controller producer](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/0018fee9.asm),
[session-user slot owner](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/00064610.asm),
[identity producer](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/0014b410.asm),
[in-game user binding](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/0014f190.asm),
[in-game controller binding](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/0014f270.asm).

## Time and scheduling boundary

Base `6E360` reads the session's active countdown. When the observed value differs, it captures the current network time into state+14 and the observed seconds into+18. For a nonnegative countdown it compares signed elapsed time against1000 ms; only when seconds are positive and that threshold is met does it capture time again, decrement by one and request `5BF10`. This is a one-decrement path, with ownership of the observed session state and time still required.
[Original periodic decrement](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/0006e360.asm),
[zero-countdown state transition](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/0006e410.asm).

Original `75870/75890` read cached `51054C` when byte `510548` is nonzero, otherwise use the original `3314B0` GetTickCount thunk. `8DF50` and `8DFC0`, under their actual `4D8BA0` network gate, set that cached flag/time around their original network/session/interface update calls and clear/update it afterward. The thunk reads through import411690. These owning stores and call order are static evidence; no kernel clock backing, cached timestamp or admitted session is provided by this suite.
[Clock read](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/00075870.asm),
[elapsed-time read](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/00075890.asm),
[interface update owner](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/0008df50.asm),
[session update owner](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/0008dfc0.asm),
[original data-import thunk](../analysis/multiplayer/integration/spawn-source/lobby-source/instructions/003314b0.asm).

## Frozen scope

The original base XBE is pinned to `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`. Both independently generated reports and all31 instruction files (1,260 static instruction rows) match byte-for-byte; every listed byte is compared with that untouched executable. There are zero executed guest instructions, patches or supplied gameplay/platform values. TU address mapping, whole menu/lobby flow, actual local profiles/users and a live countdown are not claimed.
[All source/input pins and boundaries](../analysis/multiplayer/integration/spawn-source/lobby-source/report.json),
[repeat verification](../analysis/multiplayer/integration/spawn-source/lobby-source/independent-freeze-verification.json).

```bash
.tools/python/bin/python tools/oracle_mp_spawn_lobby_source.py \
  --output analysis/multiplayer/integration/spawn-source/lobby-source/reproduction/report.json
```
