# Original Cairo cinematic skip and checkpoint timing

Research date: 2026-10-07. The original Cairo skip controller saves a suspended
script continuation before the cinematic, restores that continuation after
natural completion or accepted skip input, and uses a one-tick reverted-time
comparison to bypass the cinematic on replay. Correct VM scheduling and the
checkpoint clock are necessary for that replay to finish without another save
or revert. This note establishes that bounded controller behavior; it does not
establish complete campaign or save-file compatibility.

## Authority and pins

The primary executable is `assets/disc/default.xbe`, retail build
`02.09.27.09809`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The actual Cairo script nodes come from
`assets/disc/maps/01b_spacestation.map`, SHA-256
`5f79832ce3ae8035e547c5c673008802a4971fbf60551755fcc0553482843ae0`.
Both fixtures assert those hashes before execution. Addresses below are
original Xbox virtual addresses. The oracle executes original instructions;
recovered C supplies names and context, not expected values.
Sources: [original-wrapper fixture](../tools/oracle_campaign_skip.py),
[ground truth](../analysis/campaign/skip-research-fixture.json).

Local CC0 recovery is pinned to revision
`5f08630655e3ddc9ac45909090f11ae799fa0c27`; its
[function manifest](../tools/reference/halo2-decompiled/config/functions.csv)
distinguishes matched functions from provisional recovery. Builtin names and
opcodes are corroborated by Assembly revision
`fd14ef3c46135e9422966c6a64cd926fb0bd23fb`,
[Xbox scripting definitions](../tools/reference/assembly/src/Blamite/Formats/Halo2Xbox/H2X_Scripting.xml).
All research changes for this task are this new note, versionable oracle tools,
and isolated `analysis/campaign/skip-research*` evidence; production code was
not edited.

## Authored controller

Cairo static script7, `cinematic_skip_start`, boolean, root `E4090096`:

```lisp
(begin
  (cinematic_skip_start_internal)
  (game_save_cinematic_skip)
  (sleep_until (not (game_saving)) 1)
  (not (game_reverted)))
```

Cairo static script8, `cinematic_skip_stop`, void, root `E41A00A7`:

```lisp
(begin
  (cinematic_skip_stop_internal)
  (if (not (game_reverted)) (game_revert)))
```

These are decoded from the actual compiled cache, not approximated wrapper
source. Script records: file offset `0E6582D8`, 418 records, stride40;
expression nodes: `0E65EBB8`, 27,091 records, stride20;
strings: `0E64BEA4`, 50,228 bytes. The first mission body is guarded by the
boolean start result. Source: [raw nodes and decoded expressions](../analysis/campaign/skip-research-cairo-nodes.json),
[authored Cairo startup](CAMPAIGN_RESEARCH.md).

| Operation | Original behavior |
|---|---|
| `cinematic_skip_start_internal`, `2A9780` | Set interface `g510C50+6` to1; return HS void. |
| `cinematic_skip_stop_internal`, `2A97A0` | Set the same byte to0; return HS void. |
| `game_save_cinematic_skip`, `2AC3A0` | If signed checkpoint stage `g4701EC<4`, set stage4, retry counter0, request-start tick=current tick, consecutive-check short0. Stage>=4 coalesces without resetting those fields. |
| `game_saving`, `2A9D10` | Return main pending-save byte `547F70!=0` OR checkpoint stage `g4701EC!=0`. |
| `game_revert`, `2A9A10` | Set main bytes `547F6F`, `547F74`, `547F75` to1; actual restore runs later. |
| `game_reverted`, `2A9D40` | Return external timestamp `4E6090 == game_time.tick` (`g510C54+8`). |

Sources: original listings
[start](../analysis/campaign/skip-research-2a9780.txt),
[stop](../analysis/campaign/skip-research-2a97a0.txt),
[save request](../analysis/campaign/skip-research-2ac3a0.txt),
[saving](../analysis/campaign/skip-research-2a9d10.txt),
[revert](../analysis/campaign/skip-research-2a9a10.txt),
[reverted](../analysis/campaign/skip-research-2a9d40.txt);
[evaluator recovery](../tools/reference/halo2-decompiled/src/unknown_29f5b0.cpp).

## Save, clock and replay order

The ordinary simulation tick `137FE0` calls the script service **209310** at
`138046`, then checkpoint service `226050` at `13808C`, then increments
`game_time.tick` at `1380F8`. `1917B2` is a different HUD service and must not
be treated as the script scheduler. Main loop `12B450` services save/load/revert
before driving the next simulation tick; save service `12BB20` is at `12B4C8`,
while tick driving `137F70` occurs later at `12B632`.
Sources: [simulation tick](../analysis/campaign/skip-research-137fe0.txt),
[main loop](../analysis/campaign/skip-research-12b450.txt),
[tick driver](../analysis/campaign/skip-research-137f70.txt),
[script service](../analysis/campaign/skip-research-209310.txt).

The unchanged-instruction fixture supplies initial tick100 and follows this
sequence for both routes:

| Point | Saved/current state and consequence |
|---|---|
| Scripts at100 | Start sets skip-enabled, requests stage4, queues the unevaluated `(not (game_saving))` child, and sets wake101. `game_saving` has not executed. |
| Checkpoint service at100 | Cinematic stage4 clears stage and sets main pending-save. |
| End of simulation tick | Original increment changes game clock to101; child remains unevaluated, wake101. |
| Main save service | `12BB20 -> 123E20` captures the arena at clock101 before the next script update, then clears pending-save. |
| Scripts at101 | Predicate now sees saving=false; start finishes with true, allowing the cinematic. |
| Natural end / accepted input | Restore is requested at supplied later tick200. `12BA30 -> 123F60` replaces the arena with the saved state. |
| Post-restore callback | Original `124620` writes external reverted timestamp101, equal to the restored clock. |
| Restored scripts at101 | Pending predicate evaluates saving=false; start now returns false because `game_reverted` is true. Replayed stop clears skip-enabled and requests no second revert. |

Original saved expression stack, top first:
`E400008D` predicate, `E3FE008B` sleep-until, `E4090096` start wrapper.
Both fixture routes perform exactly one checkpoint write and read. The
restored child/frame state matches the saved state. Sources:
[original route traces](../analysis/campaign/skip-research-fixture.json),
[save service](../analysis/campaign/skip-research-12bb20.txt),
[post-restore callback](../analysis/campaign/skip-research-124620.txt).

Native integration must suspend/defer at the host save request and preserve
the VM continuation, capture after the end-of-tick clock increment, and run
the restored script update at that current clock before incrementing it.
Incrementing first loses the reverted equality. Capturing before the host
call completes can repeat the save; evaluating the nonliteral predicate
before its poll stores the wrong checkpoint continuation. These consequences
follow from the original trace above, not from a native expected-value model.

### `sleep_until` timing and ABI

Original `2089B0` queues a nonliteral condition through `2099F0`, sets wake to
current tick+poll, and yields because `209850` observes the future wake. The
child executes at that wake; on true, the parent completes in the same update.
Arguments are evaluated once; default poll is30, default timeout is `NONE=-1`.
Poll short and timeout long convert through float
`(delay * float(1/30)) * float(game_rate)` followed by x87 nearest-even integer
rounding. `45DC6C` is float bits `3D088889`, value
`0.03333333507180214`; game rate is the short at `g510C54+2`. Poll is clamped
to at least1. Timeout `NONE` bypasses conversion; timeout0 finishes at the
first state1 deadline gate before queueing a predicate. Deadline caps wake
at start tick+timeout. A literal/global expression (`flags&1`) is evaluated
immediately by `2099F0`; global (`flags&4`) is read when queued. Cairo's
compound condition takes the delayed nonliteral path. Sources:
[2089B0](../analysis/campaign/skip-research-2089b0.txt),
[2099F0](../analysis/campaign/skip-research-2099f0.txt),
[209850](../analysis/campaign/skip-research-209850.txt),
[HS recovery](../tools/reference/halo2-decompiled/src/unknown_209520.cpp).

Builtin evaluator ABI is stack `(opcode, thread datum, initialize)`, `RET12`.
Interpreter `209850`: `EDI=thread datum`, plain `RET`.
Expression queue `2099F0`: `EDX=thread datum`, `EBX=result pointer`,
stack expression datum, `RET4`. Completion `209AE0`:
`ECX=thread datum`, `EAX=result`, plain `RET`.
Thread record size is `418`: script index `+4`, wake `+8`, saved wake `+C`,
current frame pointer `+10`, base frame `+18`. Frames contain parent `+0`,
expression `+4`, result pointer `+8`, allocation short `+C`, payload `+E`.
All private stack payloads, wake state, globals and other threads must survive
restore. Source: [HS recovery](../tools/reference/halo2-decompiled/src/unknown_209520.cpp),
[completion instructions](../analysis/campaign/skip-research-209ae0.txt).

## Stage coalescing and player skip input

With interface cinematic-active byte `+5`, `226050` services only stage4. It
clears stage4 even when options byte `+1121` suppresses saving; otherwise it
tail-calls `12B790` to set pending-save. This path bypasses ordinary checkpoint
combat-safety checks and player healing. A forced save outside cinematics
also runs player healing. Stage1/2 safety policy is a separate dependency;
the bounded skip fixture does not substitute it for cinematic stage4.
Sources: [226050 instructions](../analysis/campaign/skip-research-226050.txt),
[checkpoint recovery](../tools/reference/halo2-decompiled/src/unknown_225f80.cpp),
[pending-save service](../analysis/campaign/skip-research-12b790.txt).

Original player-input fragment `14D715..14D760` requires admitted unit
`+134 bit0`, action `+1D bit0`, and interface skip-enabled `+6`. If interface
`+22` is set, next-map readiness `163B60` must return false. Accepted input
clears BSP-pending `547F73` and pending-save `547F70`, then sets dispatch
`547F72`. Main service `12BAD0` rechecks skip-enabled and next-map readiness,
always clears dispatch, and on acceptance sets `547F6F=1`, `547F74=1`,
`547F75=0`. Physical controller-button mapping and upstream unit/action
admission are outside this fragment proof. Sources:
[input instructions](../analysis/campaign/skip-research-state-ref-14d3a0.txt),
[dispatch instructions](../analysis/campaign/skip-research-12bad0.txt),
[dispatch recovery](../tools/reference/halo2-decompiled/src/unknown_12be90.cpp).

`163B60` returns true only if a next map exists and its location is none0,
queued1, or copying2. No next map, open3, or copied4 returns false. Its jump
table extends beyond the manifest's 26-byte function range. Source:
[complete code and jump table](../analysis/campaign/skip-research-163b60-full.txt),
[loading recovery](../tools/reference/halo2-decompiled/src/loading.cpp),
[map-location recovery](../tools/reference/halo2-decompiled/src/unknown_213d20.cpp).

`12BA30` forwards the request bits to `123F60`, then clears its pending flags.
Restore translates those bits into module callback flags: natural script
revert `0x60`, accepted input `0x20`. Full callback effects can therefore
differ between routes. The saved interface restores skip-enabled=1; the
replayed skip-stop clears it. Forcing that byte to0 globally during restore
is not the original transition. Sources:
[12BA30](../analysis/campaign/skip-research-12ba30.txt),
[123F60](../analysis/campaign/skip-research-123f60.txt),
[original route traces](../analysis/campaign/skip-research-fixture.json).

## Snapshot contents and outside state

The game-state arena is `3FE000` bytes (`3BE000` CPU plus `40000` GPU), with
header size `1288`. Thread/global pools use game-state allocator `g510C2C`;
threads have capacity `140`, stride `418`; globals capacity `C00`, stride8.
This restores script continuations along with arena-owned world state rather
than manually unwinding one cinematic thread. Runtime node lookup can point
at immutable compiled cache nodes; do not require mutable program nodes in
the snapshot merely because this fixture places its supplied nodes there.
Sources: [arena recovery](../tools/reference/halo2-decompiled/src/unknown_123b30.cpp),
[HS pool initialization](../tools/reference/halo2-decompiled/src/unknown_209580.cpp).

`123E20` rotates two checkpoint slots, executes pre-save module callbacks,
writes the arena, copies the header, and executes a post-save callback.
Restore validates checkpoint availability, header/game options, and BSP
admission, calls pre-restore modules, replaces arena memory through `2152E0`,
checks the restored header, then calls post-restore modules. Failed admission
sets main level-restart byte `547F6E`, rather than reporting a completed
restore. Source: [save instructions](../analysis/campaign/skip-research-123e20.txt),
[restore instructions](../analysis/campaign/skip-research-123f60.txt),
[callback loops](../tools/reference/halo2-decompiled/src/unknown_199520.cpp).

Main pending flags, checkpoint sequencing `g4701EC`, external checkpoint
metadata, and reverted timestamp `4E6090` are outside the saved arena.
Post-restore `124620` sets the timestamp from the restored tick and clears
game-time byte `+1`; `226030` clears checkpoint sequencing unless callback
flags contain `0x80`. New-map initialization resets the timestamp to `NONE`.
Sources: [124620](../analysis/campaign/skip-research-124620.txt),
[callback table](../tools/reference/halo2-decompiled/src/unknown_03d380.cpp),
[new-map initialization](../analysis/campaign/skip-research-123c20.txt),
[checkpoint recovery](../tools/reference/halo2-decompiled/src/unknown_225f80.cpp).

`game_saving` stops reporting true after the request service, not necessarily
after asynchronous disk I/O finishes. Original cache write may copy the arena
to a texture-cache buffer before asynchronous storage; cache read waits for
the outstanding copy/write lock before reading. Native world/VM snapshots
must be internally complete at the supplied storage seam. Full storage
format and asynchronous I/O parity remain outside this proof. Source:
[cache recovery](../tools/reference/halo2-decompiled/src/unknown_214f10.cpp),
[write instructions](../analysis/campaign/skip-research-2151f0.txt),
[read instructions](../analysis/campaign/skip-research-2152e0.txt).

## Reproduction and limits

Run `.tools/python/bin/python tools/oracle_campaign_skip.py` for
the two real Cairo-wrapper routes, and
`.tools/python/bin/python tools/oracle_campaign_skip_edges.py`
for 66 unchanged-XBE edge cases. Edge cases cover stage coalescing, cinematic
save suppression, unit/action/enabled/readiness input gates, dispatch recheck,
one-tick reverted equality, and original next-map jump-table results.
Sources: [edge fixture](../tools/oracle_campaign_skip_edges.py),
[edge ground truth](../analysis/campaign/skip-research-controller-edges.json).

The wrapper fixture runs unchanged HS interpreter, actual Cairo static7/8
nodes, save/restore controller, callback-table order, and original clock and
post-restore timestamp/stage callbacks. It supplies storage copies, valid
header/BSP admission, unrelated module/UI callbacks, and admitted input unit
and action. The later cinematic clock200 is supplied. It does not run the
complete opening mission, actual world animation/audio, physical input,
ordinary checkpoint safety, complete module restore effects, or a campaign
playthrough. Those remain required evidence for broader campaign claims.
