# Game-over session handoff and Postgame Carnage Report

The unchanged session owner has a two-step game-over handoff. A completed
countdown writes session phase5; the subsequent update queues request4. The
original reason6 return-to-menu chain constructs the separate authored
Postgame Carnage Report (screen16). The pregame lobby is screen14; the report's
exit/navigation callbacks still require recovery.
The request consumer, world teardown, report's runtime data producers and its
navigation remain required. The native frontend currently has no such complete
return path. Reopening its lobby after a zero countdown would skip these owners.

The original executable and disc cache are the authority. No phase is named as
a gameplay family, nor is a familiar network-state meaning assigned to the
unidentified session-state values. Compiled phase/request constants remain
literal original values; unknown record bytes retain their uncertainty.

## Executed source boundary

[oracle_session_game_over.py](../tools/oracle_session_game_over.py) executes
unchanged retail, TU1.1 and owned TU1.5 instructions. The complete session
owners are retail6EA20, TU6EF30 and u5 72650. Their actual preflights6DFA0,
6E480 and719D0 also execute. API inputs declare a secondary session with
state0, owner role bytes+C/+D zero, distinct game/owner identities, game-ready
and end records, session records and a frozen millisecond clock. These inputs
select the recovered quiet path; they are not produced by a live original
session, and no external network/closing/map service is supplied as successful.

The source oracle covers288 direct phase writes,3012 complete session-owner
updates,48 three-update sequences and3072 signed-mode/completion combinations.
The recorded mutations and outputs agree across all three images. All original
record bytes outside the classified fields remain unchanged, including all18
unclassified bytes per record initialized with declared nonzero API patterns. The executed
owner set covers278 distinct instruction addresses per image. There are22
complete cross-version routine pairs,832 paired instruction rows, and51
original listings with1883 rows including static return/UI owners. Actual
direct calls constrain the repeated closing predicate's identity; unexecuted
external callees retain their raw addresses and unproved identities. Both
independent runs produce identical reports, fixtures and instruction hashes.
There are zero instruction patches and zero supplied game/platform returns.

## Fields and policy

The original phase setter is retail5A220 / TU5A1F0 / u5 5AED0. Its recovered
fields use the following original widths:

| Original session offset | Field | Width |
| --- | --- | --- |
| +54 | Record iteration count | signed32 |
| +4978 / +497C | Generation / sequence increments | wrapping32 |
| +4980 | Current phase | signed32 |
| +4984 | Phase timestamp | milliseconds,32 |
| +72DD + i×14 | Record occupied byte |8 |
| +72DE + i×14 | Record changed byte |8 |
| +765C | Pending transition byte |8 |
| +7660 | Previous phase | signed32 |
| +7664 | Previous-phase timestamp | milliseconds,32 |

Offsets and strides in this table are hexadecimal. The remaining18 bytes in
each record are not assigned semantics. Changing phase marks each nonzero
occupied byte's changed byte as1, preserves other changed bytes, remembers the
previous phase, writes both timestamps, and increments both wrapping counters.
With no occupied iterated record the pending byte becomes0. Repeating the same
phase preserves every field. Negative counts execute no record iteration.
Native input counts above the classified16-record region remain unavailable.

After preflight,72650 preserves state when a request is pending or the main
session state is outside3..8. In phase4, the original closing query is true
only when both global bytes4DC040 and4DC042 are nonzero. Closing states5..8
write phase1 and return true; the state3/4 closing service is outside this
execution boundary. Without closing, states5..8 write phase5 and return true
only when game-over is active and its countdown is exactly0. Negative counts
remain incomplete. Otherwise the force-end byte can deliver actual13B3C0,
which calls the existing13B4A0 countdown start only for ready game kind2;
repeat requests preserve the already active countdown. Ready campaign kind1's
transition services remain unavailable.

On the following phase5 update, actual71990 writes node+4A=1, node+4C=4,
node+50=0 and the first node+54 payload word0. Other non-phase4 values request1.
The next pending update preserves that request. These are state-machine
requests, not a claim that a lobby has already been loaded. Original718F0
consumes them through actual state-object exit/entry callbacks. In particular,
the pinned original state vtable45C6D0 binds update72650, entry727C0 and
exit72810. Its exit callback compares actual game identities
and can call69B40 to initiate world closing. That callback has not been
supplied as successful by this oracle.

The native reusable policy is
[source_session_game_over.hpp](../include/halo2/source_session_game_over.hpp).
It preserves the recovered fields and calls the existing countdown core for
the recovered kind2 force-end route. Identity-matched map transitions, ready
campaign end services and closing state3/4 services fail before mutation.
This module is linked into the native systems library but **is not yet bound
to a live native session or the frontend return flow**. Its preflight result
is a caller-owned query input; the original oracle executes the actual quiet
preflight instead of supplying its return.

## Return/UI instruction and asset evidence

The final694E0 branch calls191FB0(6) after its earlier world/readiness owners
when13B340,13B4F0 and inverse13B320 all permit it. The two mode predicates
read a signed byte. The recovered leaf permits a completed game with raw
mode≤1 or mode≥6; the earlier world/readiness owners remain necessary.
This is a raw predicate, not a semantic assignment of local/network modes.

Static191FB0 uses existing main-menu state3 directly through14AF00, or builds
the original0x1118-byte options, selects state3 and the actual
`scenarios\ui\mainmenu\mainmenu` path, carries reason6 at options+264, builds
local-controller options through13B120, and queues191580. In191580 the
state3/reason2..6 branch preserves the existing session from its reset gate.
After a successful map load,1916A0 passes that reason to14AF00. The complete
map-load, platform timer, controller/options and UI-allocation dependencies
are not executed or supplied as successful here.

Reason6 reaches14B031, whose actual callback239D34 calls239B87 with screen
constant16. The pinned disc cache identifies wgit E5960376 at file offset
59501336 as `ui\screens\game_shell\postgame_statistics\pcr`, screen16,
with six authored panes. Separate wgit E5A80388 at59511420 is
`ui\screens\game_shell\pregame_lobby\pregame_lobby`, screen14, with three
panes. [The raw cache records](../analysis/multiplayer/game-variants/session-game-over/menu-assets.json)
pin the header words, sizes and cache hash. Report construction, its six-pane
selection/statistic producers, retained player/team results and its actual
Back/continue callbacks require further source recovery and normal-flow
validation. Static instruction/asset correspondence does not establish their
successful runtime execution.

## Validation and remaining scope

Native checking makes150254 comparisons against the independent original
fixtures. The complete new policy/countdown/sound-class test slice passes
ASAN, UBSAN and leak detection. Six focused native ending, countdown/import
and frontend/lobby regressions pass, and the production executable builds.
This turn adds no full-match, native session, LAN lifecycle, rendered Postgame
Carnage Report or hardware validation claim. A fresh strict catalogue audit
runs78 normal menu starts and still admits only2/39 variants at startup;37 remain blocked and the goal
remains active and incomplete.

```sh
.tools/python/bin/python tools/oracle_session_game_over.py
cmake --build build --target session_game_over_tests halo2_native -j2
ctest --test-dir build -R '^(original_session_game_over|original_game_over|original_game_over_import|native_game_end|retail_frontend|retail_frontend_lobby_game)$' --output-on-failure
```
