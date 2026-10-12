# Original King recording event and lifecycle

Research snapshot: 2026-10-12. Enabled, unfrozen King recording now has bounded whole-original execution evidence in retail, TU1.1 and TU1.5. The real event, CRT time conversion, captured Xbox kernel clock, player-position sampler, unparented object-position owner and ring append execute without instruction patches or supplied returns. This supplies a native recorder contract; it does not admit King, establish an advancing original clock, execute complete startup/final-report services, or complete the all39-variant goal. [Generator](../tools/research_king_recording_event.py), [frozen report](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/report.json), [fresh reproduction](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/reproduction.json).

This note extends the disabled-path and static findings in [King engine research](MP_KING_ENGINE_SOURCE_RESEARCH.md) and [callback research](MP_KING_NATIVE_CALLBACK_RESEARCH.md). It corrects that callback note's XYZ offsets: the real record uses **`+04/+08/+0C`**, not `+03/+07/+0B`. The original sampler receives record+4, and the executed raw records confirm the aligned offsets. Earlier evidence remains unchanged. [Whole TU1.5 event](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-event-0019bca0.asm), [raw case264 in report](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/report.json).

## Original inputs and correspondence

| Input | SHA256 |
| --- | --- |
| [Retail XBE](../assets/disc/default.xbe) | `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d` |
| [TU1.1 XBE](../assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03) | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| [Owned imported TU1.5 XBE](</home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe>) | `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd` |
| [Saved physical RAM](../analysis/multiplayer-shading/original-weather-20261009/session1/lockout-weather-particle-entry/exports/physical-ram.bin) | `f76d0569dedcd2e94d3f2ee4c67a97db47cefea848105a8b24fe08342daf24cd` |

The RAM comes from the existing paused original Lockout session. Its [export manifest](../analysis/multiplayer-shading/original-weather-20261009/session1/lockout-weather-particle-entry/export.json) supplies the actual saved CPU/page-table context. The generator translates the original virtual pages and pins the captured kernel code, clock data, PE export slot and retail IAT value. Isolated game/player/object records, event flags/timestamps, ring poison and argument values are declared API inputs. No new VM or live session was operated. [Snapshot reader](../tools/differential_source_object_admission_snapshot.py), [captured kernel metadata/pages in report](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/report.json).

| Owner or field | Retail | TU1.1 | TU1.5 |
| --- | --- | --- | --- |
| Whole King scorer | `2BEEE0` | `2C5220` | `2C8CA0` |
| Recording event | `1972A0` | `198DA0` | `19BCA0` |
| Ring append | `197120` | `198C20` | `19BB20` |
| time32 | `321AAE` | `32803E` | `32BBAE` |
| Player position | `1994D0` | `19AFD0` | `19DE80` |
| Object position | `B9DD0` | `BACC0` | `BBE00` |
| Enable / event-and-stat freeze | `510CA0/+1` | `518490/+1` | `51DFA0/+1` |
| Ring index / last sample | `510CA4/+4` | `518494/+4` | `51DFA4/+4` |
| Recording-start time | `511024` | `518814` | `51E324` |
| Ring base | `515C34` | `51D424` | `522F34` |
| Retained context / completion | `510CB0/+1` | `5184A0/+1` | `51DFB0/+1` |

Twelve complete retail-to-update shape/topology comparisons match for the event, append, time32, player-position sampler, clock import thunk and unsigned64 divide. All three images produce equal records, indices, last timestamps and query counts for the same declared cases. The separate static inventory contains28 whole-owner comparisons:18 match, while startup, completion, full retained refresh and two session owners differ. Their separate complete listings are retained; a relocated address is not treated as proof of equivalent lifecycle services. [Correspondence and listings](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/report.json).

## Real clock and fixed snapshot boundary

The original XBE clock import word is `80000080`, ordinal128, in all three images. The captured retail IAT at `411588` contains `80019F60`. The actual captured kernel PE at `80010000` has ordinal base1,366 exports, function table RVA`2E8`, and ordinal128 RVA`9F60`; that resolves to the same `80019F60`. The tool derives these fields from captured bytes, then installs that actual loader binding in each image's clock IAT. TU1.1 uses IAT`417BC8`, TU1.5 `41B668`. This is an explicit loader-data input; no thunk or kernel instruction is altered. [Retail thunk](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/retail-query_thunk-003303f3.asm), [TU1.5 thunk](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-query_thunk-0033a4d3.asm), [raw export/IAT evidence](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/report.json).

The complete captured `KeQuerySystemTime` has10 static instructions. Its stable path executes9: read high1 at `8003A884`, low at `8003A880`, compare high2 at `8003A888`, retry through PAUSE on mismatch, then write the actual64-bit result through the stack output pointer and `RET4`. Captured KSYSTEM_TIME bytes are `c0a42b35cd56dd01cd56dd01`; both high words agree. The copied code/data pages are `80019000` and `8003A000`. Every reached instruction is compared with its original XBE or captured kernel bytes; the clock data remains unchanged. [Whole captured kernel](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/captured-kernel-KeQuerySystemTime-80019f60.asm), [per-query original output buffers](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/report.json).

Whole time32 takes one cdecl stack argument, an optional32-bit output pointer, and also returns seconds in EAX. It obtains the real kernel FILETIME, adds unsigned64 `FE624E212AC18000`—subtracting `116444736000000000`—then calls the original unsigned64 divide by `10,000,000`. The captured FILETIME is `134359003251320000`, producing **`1791426725` (`6AC700A5`) Unix seconds** after integer division. The three nonnull-output probes confirm the same value is written and returned. The native platform seam must preserve this epoch/whole-second/32-bit contract; game ticks or an arbitrary steady-clock epoch would change stored events. [Whole CRT owner](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-time-0032bbae.asm), [whole divide](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-divide-0032ae00.asm), [clock probes](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/report.json).

This composition uses fixed authentic platform data. It does not exercise timer interrupts, a torn-clock retry, advancing samples, complete kernel boot or a physical Xbox oscillator. The first and second event samples therefore have equal values here, but remain distinct actual calls and observations. A production clock may advance between them; the original owner uses each result at its own source position. [Kernel observations and declared limits](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/report.json).

## Event, pose and append contract

The event ABI is **EBX=player datum**, stack `(value_at14,value_at10)`, `RET8`. The King FFA scorer supplies `(4,NONE)` before awarding the point, reads the new score afterward, and calls score-change publication only if it differs. The team-award branch does not make this FFA recording call. Existing whole-scorer evidence has4,386 calls; the new note retains complete292-instruction scorer listings rather than replacing that evidence. [Whole TU1.5 scoring instructions](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-static-scoring-002c8ca0.asm), [earlier scoring report](../analysis/multiplayer/game-variants/king-engine-20261011/scoring/report.json), [native scoring seam](../src/source_king.cpp).

Disabled enable byte or nonzero event/stat freeze returns before any clock call or last-time mutation. Otherwise the first time32 result is retained. The owner continues only for literal **unsigned32 `(last-first_now)>3`**. Equal or last=now+1/+2/+3 skips; last=now+4 admits; last=now-1 wraps and admits. This is not a reconstructed conventional elapsed-time throttle. On the admitted branch a second time32 sample supplies relative record time, then the pose helper runs. Pose success calls the real append. Pose failure does not append. Both admitted pose paths store the **first** sample as last time; the skipped paths preserve last. [Whole event](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-event-0019bca0.asm), [unsigned-boundary cases](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/report.json).

**The second clock is sampled even when both current and previous unit are NONE.** Pose resolution occurs afterward. There are180 independently executed admitted no-unit cases across the three images, each with two real kernel queries, no append, and last updated to the first sample. [Exact case indices from frozen output](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/activation/complete/report.json).

| Record offset | Actual bytes/value |
| --- | --- |
| `00` | u8 type2 |
| `01` | low player byte BL |
| `02` | `FF` participant NONE |
| `03` | zero |
| `04/08/0C` | actual XYZ binary32 bits |
| `10` | second stack argument; King supplies `FFFFFFFF` |
| `14` | first stack argument; King supplies4 |
| `18/1C` | zero dwords |
| `20` | second time32 sample minus recording-start, wrapping32 |
| Total | `24` hex =36 bytes |

For report input264, source player`9ABC0000`, current unparented position `(1.25,-2.5,3.75)`, `(4,NONE)`, last=now+4 and start=now produce these exact bytes in all three images:

```text
0200ff00 0000a03f 000020c0 00007040 ffffffff 04000000 00000000 00000000 00000000
```

The index changes0→1. Case267 preserves negative-zero X and wraps index999→0, with relative-time word`80000000`. These are outputs of the original owners, independent of native formatting or score code. [Raw original records and full-ring hashes](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/report.json).

The player-position ABI is ECX=player datum, stack outputXYZ pointer, `RET4`, AL=success. It indexes the raw low16 slot, stride`21C`, player-pool data pointer`+44`. Current unit`+2C` wins; only NONE falls back to previous unit`+30`; both NONE returns AL0. It does not validate player salt/high-water itself. The actual object-position owner then indexes the unit header and, for parent`+14=NONE`, copies object`+64/+68/+6C` unchanged. The parent/node-transform branch remains in the complete listing but is not executed by this corpus. [Whole pose sampler](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-pose-0019de80.asm), [whole object-position owner](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-object_position-000bbe00.asm).

The append ABI is EDX=record pointer, plain RET. It independently checks the same enable/freeze bytes, copies9 dwords to `ring+index*36`, then computes unsigned `(index+1)%1000`. It does not publish a separate event count, fullness flag, file or network packet. Ring entries overwrite; bytes outside the selected36-byte record are preserved. Index validity is a caller/state prerequisite: this corpus declares indices0/1/998/999 and does not invent an out-of-range clamp. Completion B1 does not gate event/append; enabled/unfrozen cases with completion1/255 still append. [Whole append](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-append-0019bb20.asm), [direct append and completion cases](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/report.json).

## Lifecycle writers and distinct flags

The inventory in each report version verifies direct absolute write operands against original discovered instruction owners. It additionally records startup's REP clear. It is bounded field-owner evidence, not an exhaustive indirect-alias or full session-boot recovery. The full startup/refresh/completion bodies differ between updates and remain separately listed. [Write inventory and static comparison](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/report.json).

| Lifecycle owner | Retail | TU1.1 | TU1.5 |
| --- | --- | --- | --- |
| Recording/retained startup | `195A40` | `197450` | `19A430` |
| Stop/abort | `195E90` | `1978A0` | `19A7F0` |
| Full retained refresh | `195ED0` | `1978E0` | `19A830` |
| Set first recording-start time | `199460` | `19AF60` | `19DE10` |
| Shared round preparation | `157AE0` | `159180` | `15A560` |
| Shared phase transition | `15C550` | `15DBC0` | `15EFA0` |
| Shared post-world/end consumer | `15CA10` | `15E070` | `15F450` |
| Retained completion/publication | `196390` | `197EA0` | `19ACB0` |
| Restore retained start/state | `1973F0` | `198EF0` | `19BDF0` |

Startup first zeroes the retained context from B0, including completion, start fields and ring, writes index0, and stores a real time32 sample as last. The exact clear sizes are **retail`DCD8`, TU1.1`DD78`, TU1.5`DE78`**, derived from original REP counts`3736/375E/379E`; the earlier retained-layout note's `DE78` initialization statement applies to TU1.5. This does not change the shared event/ring offsets. Three whole null-engine startup calls confirm the clears, index0 and last=`6AC700A5`. Because this branch skips enablement, preexisting A0/A1 poison`7F/55` is preserved. [Retail startup](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/retail-static-startup-00195a40.asm), [TU1.1 startup](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/tu11-static-startup-00197450.asm), [TU1.5 startup](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-static-startup-0019a430.asm), [whole startup outputs](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/report.json).

The startup's selected-engine branch captures real settings, player/machine properties and session metadata, sets retained header+0 to1, then for options mode!=4 sets **A0=1, A1=0** and calls the actual identity-remap and team-refresh owners. That complete populated startup branch was inspected, not executed here. It does not itself set a nonzero recording-start time: startup cleared that field. The full retained refresh checks enable!=0, completion B1=0 and started flag=0 before calling the real start-time producer. The producer sets header+`370` to1 and header+`374` to a new time32 sample. It searches the retained machine rows through the actual original owner and, when found, sets that row's byte+9 to1. Nine whole calls cover no qualifying row and explicit occupied/+8 rows0/15; no machine-search or clock return is supplied. [Actual refresh caller](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-static-refresh_context-0019a830.asm), [whole timestamp-start owner](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-static-timestamp_start-0019de10.asm), [whole machine search](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-static-local_machine-0019dc90.asm), [raw start mutations](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/report.json).

Shared round preparation clears **A1** only for an enabled recording outside mode4. A changed transition to phase2 sets **A1=1** under the same mode/enable gate. The ending branch of post-world, gated by selected engine, non-mode4, game-over, unhandled end and enabled/uncompleted recording, clears A1 temporarily, runs the real final counter/retained update owners, sets A1=1, then calls retained completion. **The numeric-stat writer and event recorder share these A0/A1 bytes.** Existing native `SourceResultRecording` and its original phase producer are the correct shared gate, not a second independently invented event-freeze state. [Whole preparation](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-static-prepare-0015a560.asm), [whole phase transition](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-static-phase-0015efa0.asm), [whole ending caller](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-static-post_world-0015f450.asm), [existing native phase binding](../src/source_round_lifecycle.cpp).

Completion writes **B1=1**; it is separate from A1. It gates retained context refresh/remapping and report readiness. The TU1.5 complete owner also gathers session metadata before this write and has actual platform/PCR publication branches afterward; none is supplied a successful return here. Stop/abort disables A0 for a selected engine, then if B1 was zero marks header+2 and invokes the same completion owner. Restore, while B1=0, copies its input start word into the actual start field and can request completion from its input byte0. Two additional session owners directly disable/enable A0 (TU1.5 `6C4D0/6C590`; retail `6AE20/6AEE0`; TU1.1 `6B2F0/6B3B0`); the enable branch also clears A1 and invokes remap/refresh. Their complete static bodies are saved, but full session transitions are outside this experiment. [TU1.5 completion](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-static-completion-0019acb0.asm), [stop](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-static-stop-0019a7f0.asm), [restore](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-static-restore-0019bdf0.asm), [session disable](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-static-session_disable-0006c4d0.asm), [session enable](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-static-session_enable-0006c590.asm).

## Startup and first active tick ordering addendum

A separate static addendum preserves the already frozen event tool and corpus. Nine verified original near-CALL edges establish the actual direct callers across all three images. Twelve complete new listings retain startup's caller, active tick, pre-world engine stage and actual registered King slot18. The active-tick/pre-world/King-update bodies have complete matching shapes; the three startup-caller bodies remain independently preserved. This addendum executes original registration to resolve King slot18 but does not execute the added world or session services. [Addendum generator](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/activation/ordering_evidence.py), [static call/ordering report](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/activation/complete/report.json), [13-file byte-equal reproduction](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/activation/reproduction.json).

| Actual owner/call | Retail | TU1.1 | TU1.5 |
| --- | --- | --- | --- |
| Startup caller | `67EE0` | `680F0` | `692D0` |
| Its recording-startup call instruction | `67F50` | `68160` | `69340` |
| Immediately following flag=1 write | `67F55→4CF778` | `68165→4D6D40` | `69345→4DC048` |
| Active-tick owner | `137FE0` | `139750` | `13AAC0` |
| Pre-world engine call instruction | `13803C→15C150` | `1397AC→15D7C0` | `13AB1C→15EBA0` |
| Next call, full retained refresh | `138041→195ED0` | `1397B1→1978E0` | `13AB21→19A830` |
| Conditional timestamp-start call | `195EF6→199460` | `197906→19AF60` | `19A859→19DE10` |
| Actual King slot18 owner | `2BD680` | `2C3A10` | `2C7490` |
| Its scoring call instruction | `2BD729→2BEEE0` | `2C3AB9→2C5220` | `2C7539→2C8CA0` |

Startup's actual caller invokes recording startup after its real context/session setup calls and immediately before writing its activation byte to1. Its initial raw byte gate is retail`4ED39C`, TU1.1`4F496C`, TU1.5`4F9C6C`: nonzero skips that startup/flag-write branch; this note assigns no guessed semantic name to that gate. The cold last-time sample belongs at the recovered activation/setup boundary, rather than an arbitrary earlier C++ construction time. The native runtime already exposes `session_world_loaded` after world/local-player setup and before its loop; that is a concrete available native activation seam, whose adaptation remains a declared native phase mapping rather than proof that the original session setup services ran. Preserve the original enable/mode gates and cleared start flag; do not manufacture retained machine identity or B1 completion. [TU1.5 startup caller](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/activation/complete/u5-startup_caller-000692d0.asm), [native loaded callback position](../src/multiplayer_runtime.cpp).

The active tick prepares its actual input record and enters the world branch only when that record's byte+4 is nonzero. Inside that branch the shared pre-world stage runs player work, King occupancy via slot15, then King slot18 and its scorer. **Full retained refresh follows immediately after the pre-world stage returns**, before subsequent world-object phases. Its first eligible call samples recording-start time when enable!=0, B1=0 and started=0; A1 is not a gate there. Therefore the native start-time seam belongs after first eligible pre-world King scoring, not before the first score and not unconditionally in a recorder constructor. This distinction remains relevant even though an ordinary newly initialized timer may not award a point in that first tick. The native step can map these recovered phase boundaries without pretending that the complete original refresh's session/machine services succeeded. [Whole active tick](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/activation/complete/u5-tick-0013aac0.asm), [whole pre-world stage](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/activation/complete/u5-preworld-0015eba0.asm), [registered King update](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/activation/complete/u5-king_update-002c7490.asm), [original retained-refresh guards](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-static-refresh_context-0019a830.asm).

Only direct near-CALL references are inventoried; indirect/tail callers are not asserted absent. The addendum does not supply the input preparation's active byte or run a whole original world tick. Its initial local helper filename shadowed Python's standard `inspect` import before any added original owner ran; the failed import/source is retained and the renamed helper reproduces independently. [Preserved failure](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/activation/failed-import.json).

## Native storage and pose seam

`SourceRetainedResults.events` already provides the actual1,000×36-byte ring at retained+`4F84`. Start flag/time belong to its raw header+`370/+374`; cursor and last time are separate globals before that context. `SourceResultStatistics` alone ends at the ring boundary and is not event storage. `SourceResultRecording {enabled,frozen}` already models A0/A1, while retained header+1 models B1. The existing native match owns the stat bank and shared recording gate; this audit does not claim it already owns or publishes a complete original retained event context. [Native layouts](../include/halo2/source_result_retention.hpp), [stat/recording views](../include/halo2/source_result_statistics.hpp), [match state and constructor](../src/multiplayer.cpp).

The existing shared remapper processes all1,000 events' signed participant bytes+1/+2 by inverse retained/live identity mapping and preserves the other34 bytes. It uses enable and B1, allowing A1 to be frozen. This is already independently recovered and tested; no duplicate remap oracle was added. Native player identities/configuration, machine roster, final report and transport remain their own required owners. [Retained-result source evidence](MP_RESULT_RETENTION_RESEARCH.md), [existing original remap oracle](../tools/oracle_result_retention.py), [native remapper](../src/source_result_retention.cpp).

For currently unparented native units, the sampler can use player `source_current_object`, falling back only when NONE to `source_previous_unit`, and resolve the selected actual unit through the existing current/retained-unit store. `finish_death` preserves previous-unit identity and `retain_unit` retains full unit state; `unit_from_datum` checks the real object lifetime before resolving current or retained storage. Reading that unit's actual position preserves the source unparented position contract, including binary32 signed zero. It must not replace missing old units with player position or a fabricated pose success, and must not fall back merely because a non-NONE current identity fails resolution. Source parent/node transforms and unavailable/deleted units remain explicit boundaries. [Native player/unit identities](../include/halo2/multiplayer.hpp), [actual current/retained resolution and death/leave ownership](../src/multiplayer.cpp), [unit parent/position fields](../include/halo2/simulation.hpp), [original pose and position instructions](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-pose-0019de80.asm).

## Adjacent score-change event locator

`SourceKingScoringServices::score_changed(player,next)` corresponds exactly to retail **`19F470`**, TU1.1 **`1A2DD0`**, TU1.5 **`1A9080`**. ABI: EAX=full player datum, one stack signed updated-score argument, `RET4`. All three complete130-instruction bodies match after declared relocation. This is a bounded static locator for the parent's existing event queue; presentation internals were not expanded or newly executed. [Native callback interface](../include/halo2/source_king.hpp), [whole original score-change body](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-static-score_changed-001a9080.asm), [three-image correspondence](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/report.json).

The owner subtracts updated score from options+`18C`. Exact remaining values10/30/60 select subtypes47/9/7 for FFA, or48/10/8 when the actual selected-engine pointer is nonnull and options+`184` bit0 is set. Each threshold is checked separately, with an options reload after earlier dispatch. Options mode4 suppresses publication. The defined event prefix is eight little-endian32-bit words plus u16 tail:

```text
words = [0, subtype, FFFFFFFF, full_player, signed_player_C0, FFFFFFFF, FFFFFFFF, 0]
tail  = FFFF
size  = 34 bytes
```

Word4 contains the actual signed player team byte even in FFA; the team predicate chooses the subtype and does not replace this field with NONE. The source first calls the network publisher, then the local dispatcher, using the same event buffer. [Exact event stores and ordered calls](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/u5-static-score_changed-001a9080.asm), [native raw event type](../include/halo2/vista_announcer_events.hpp).

| Service destination | Retail | TU1.1 | TU1.5 | ABI |
| --- | --- | --- | --- | --- |
| Network publication | `A7C50` | `A8A30` | `B1E80` | EDX=event pointer |
| Local dispatch | `19EB30` | `1A2490` | `1A8740` | EDI=event pointer |

The earlier King source note establishes the network owner's actual mode5 gate and ordinary mode1 local path. This note identifies the source call sites/schema; it does not equate network return with local delivery or claim tag/audio queue admission. [Existing service findings](MP_KING_ENGINE_SOURCE_RESEARCH.md), [retail score-change calls](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/retail-static-score_changed-0019f470.asm), [TU1.1 score-change calls](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/tu11-static-score_changed-001a2dd0.asm).

## Reproduction and measured limits

From the repository root, use fresh output paths:

```sh
.tools/python/bin/python tools/research_king_recording_event.py --output analysis/multiplayer/game-variants/king-recording-event-source-20261012/new-run
```

The tool refuses to reuse an output directory. Frozen `complete/` and a separate-process `reproduction/` have **65 byte-identical files**:64 complete original listings plus the report. The report contains1,944 whole event calls,72 direct append calls,3 whole time probes,3 whole null-engine startups,9 whole timestamp-start calls,360 appended event records, and1,527 real kernel queries. It preserves every clock output buffer, record, ring hash, before/after flag/index/last bytes, lifecycle mutation and verified direct writer. [Reproduction manifest](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/reproduction.json).

The event inputs cover six enable/freeze/completion triples, nine unsigned throttle deltas, current/fallback/no-unit poses, slots0/15, ring indices0/999, raw argument words, relative-time wrapping, asymmetric coordinates and signed zero. Direct append also covers indices1/998. All reached original instructions are checked unchanged. The saved failed diagnostic reached whole startup and then hit a Python report-construction TypeError; its exact error and correction remain in [failure evidence](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/diagnostic-2/failure.json) and [failure note](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/diagnostic-2/README.md). No source service failure was substituted. [Generator](../tools/research_king_recording_event.py).

| Frozen artifact | SHA256 |
| --- | --- |
| Generator | `e41c5d1bc497c8e50bc1c02402bf7be27a62010875de662c6d784368f97673ea` |
| Complete report | `395b5d08cbcc47bf421349051cec917afc2689882239ff53a044c07c17915da8` |
| Reproduction manifest | `a2beafd70dcd94c7d7941bd4fbcc7578da8da95db17779bb30c6031940f57027` |

The supported execution is a declared composition of complete owners against a fixed authentic clock and isolated valid player/object contexts. Full enabled startup/property capture, an advancing native recording lifecycle, parented transforms, source identity/report publication, real session transport and normal King flows remain outside this evidence. No native source, test, fixture, CMake, progress or admission file was changed by this research.
