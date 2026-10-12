# Original map reset with a source-produced compatibility clock

The next two original ordinal156 clock reads now execute against literal data produced by unchanged, independently pinned compatibility timer bodies and genuine host time observations. Callback23 then stops at its next original ordinal128 `KeQuerySystemTime` request. Readiness, local membership, controller/view binding and successful map/world startup remain unproved. This separate1.1 report changes no production file or preceding freeze and makes no1.5 address claim.
[Independent tool](../tools/oracle_mp_spawn_local_map_clock.py),
[complete report](../analysis/multiplayer/integration/spawn-source/local-map-clock/report.json),
[preceding clock/callback18 owners](MP_LOCAL_MAP_STORAGE_RESEARCH.md).

## Pinned timer ownership and actual provider

Pinned Cxbx primary code declares ordinal156 `KeTickCount` as a volatile32-bit variable initialized to zero, and declares interrupt/system clocks as three32-bit words. Its startup calls `timer_init` before `KiInitSystem`. These are compatibility-produced initial values, not recovered Xbox hardware defaults.
[Primary clock declaration](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlKe.cpp#L2082-L2085),
[pinned startup source](../analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/CxbxKrnl.cpp),
[retained primary clock source](../analysis/multiplayer/integration/spawn-source/local-map-clock/primary-source/EmuKrnlKe.cpp).

The new tool extracts complete `timer_init`, `get_now`, `pit_next`, `KiInitSystem`, `KeInitializeDpc` and `KiClockIsr` bodies unchanged. It compiles them as an actual32-bit Wine process with the original extracted private types and real C++ synchronization. The provider records the requested native `QueryPerformanceFrequency`, `QueryPerformanceCounter` and `GetSystemTimeAsFileTime` calls. An independent repetition replays those exact previously observed raw API inputs through the same bodies; it does not choose a timestamp.
[Exact compiled bodies and declared transport](../analysis/multiplayer/integration/spawn-source/local-map-clock/compiled-timer.cpp),
[compiled executable, body/source pins and actual API records](../analysis/multiplayer/integration/spawn-source/local-map-clock/report.json),
[measured raw input fixture](../analysis/multiplayer/integration/spawn-source/local-map-clock/measured-native-clock-inputs.json).

`timer_init` owns frequency/start/last counters, the initial PIT time and the system clock from actual FILETIME. `get_now` converts observed counter differences to microseconds and accumulates execution time. `pit_next` calls the interrupt handler when its source1000-microsecond threshold is reached. That handler carries fractional microseconds, advances both clock records by `10000 * elapsed_ms` and adds elapsed milliseconds to the source32-bit tick counter.
[Primary initialization and performance-counter bodies](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/common/Timer.cpp#L49-L61),
[primary PIT update](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/common/Timer.cpp#L92-L103),
[primary elapsed-time producer](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/common/Timer.cpp#L117-L127),
[primary whole clock interrupt](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlKi.cpp#L149-L212).

Whole `KiInitSystem` and `KeInitializeDpc` initialize the private process, wait head, DPC, IDE device queue and all32 timer-table entries. Each entry's forward/reverse links remain self-links, with its complete original time value. The whole interrupt handler obtains its actual host mutex and checks that source-empty table. Its system-time-change flag remains source-initialized clear. Unavailable expiry/DPC/time-change services abort if entered; they supply no result and are not reached. Complete native private records, including host synchronization bytes, are retained separately from deterministic clock and source buffers.
[Primary full owning initializer](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlKi.cpp#L106-L123),
[primary DPC initializer](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlKe.cpp#L929-L953),
[complete raw native owner records](../analysis/multiplayer/integration/spawn-source/local-map-clock/raw-native-private-clock-records.json),
[per-observation complete clock words and table audit](../analysis/multiplayer/integration/spawn-source/local-map-clock/report.json).

This is a bounded compatibility provider. Its PIT producer runs on the owning observation requests; complete `system_events`, device interrupts, native thread scheduling and Xbox hardware clocks are not executed. Earlier prepared clock providers retain their separate measured fixture/time bases. This new producer begins with its source staticzero, so full-title clock continuity is not claimed. No game tick,30Hz rate or readiness value is supplied.
[Original complete event-loop boundary](../analysis/multiplayer/integration/spawn-source/local-map-clock/primary-source/Timer.cpp),
[exact provider boundary ledger](../analysis/multiplayer/integration/spawn-source/local-map-clock/report.json).

## Original reads, outputs and remaining dependency

The literal source-produced four-byte tick value is transported into the existing declared ordinal156 backing. Original `337A40` reads the import, `337A45` loads the actual backing and `337A47` returns. General registers, raw stack, before/after backing and defined EAX at both returns are recorded. The original XBE import remains verified as encoded `8000009C`; runtime backing remains the earlier explicit `0B000000` relocation.
[Unchanged original load and return](../analysis/multiplayer/integration/spawn-source/local-map-clock/instructions/00337a40.asm),
[literal two-call ABI and backing records](../analysis/multiplayer/integration/spawn-source/local-map-clock/report.json).

| Original caller return | Backing before, raw four bytes | Source-produced after | Actual return EAX |
|---|---|---|---|
| `68E87` | `3260b802` | `78a80000` | `0000A878` (43128ms) |
| `68FA5` | `78a80000` | `7ba80000` | `0000A87B` (43131ms) |

The same full `190A30→139410` reset preserves actual callbacks2/9/13/16/17/18/19/20/21/22/23. The first ten return. Callback23 progresses through both tick reads, then original `197450→32803E→336983` reaches ordinal128 at `80000080`. That new request remains unserviced: return address=`32804C`, output pointer=`021FCFB0`, complete eight-byte entry buffer=`0000000000000000`. Full registers/stack and all owning states are retained. Its source `KeQuerySystemTime` body is independently identified, but is not executed or substituted in this freeze.
[Interrupted callback23](../analysis/multiplayer/integration/spawn-source/local-map-clock/instructions/000680f0.asm),
[original197450 caller](../analysis/multiplayer/integration/spawn-source/local-map-clock/instructions/00197450.asm),
[time(NULL) and original XAPI thunk](../analysis/multiplayer/integration/spawn-source/local-map-clock/instructions/0032803e.asm),
[exact ordinal128 stop](../analysis/multiplayer/integration/spawn-source/local-map-clock/report.json),
[primary next service body](../analysis/multiplayer/integration/spawn-source/local-map-clock/primary-source/EmuKrnlKe.cpp).

Full map reset remains interrupted before `1395B0` membership, `11D780` BSP startup and `190A6E` readiness. Full literal local, pool, metadata, clock and callback18 records remain in each owning before/after snapshot. No current participant, profile, local-user mapping, admitted command or successful world result is inserted.
[Map-start owner](../analysis/multiplayer/integration/spawn-source/local-map-clock/instructions/00190a30.asm),
[complete retained state and callback returns](../analysis/multiplayer/integration/spawn-source/local-map-clock/report.json).

## Separate source Game+0E producer

Original TU139880 initializes the options record. It clears `1118` bytes, reads signed-short48CFE0 and uses it when positive, otherwise literal60. It computes signed division by2 and writes the resulting short to options+06 at1398C1. Original `139540` copies those options to `Game+08`; options+06 therefore becomes `Game+0E`. Constructor139880 then calls original7B540 to generate its eight-byte random ID, so the whole options constructor cannot be claimed complete by copying only its rate output.
[Complete options initializer](../analysis/multiplayer/integration/spawn-source/local-map-clock/instructions/00139880.asm),
[actual options-to-Game copy](../analysis/multiplayer/integration/spawn-source/local-map-clock/instructions/00139540.asm),
[random-ID producer and its original platform calls](../analysis/multiplayer/integration/spawn-source/local-map-clock/instructions/0007b540.asm).

The original XBE's48CFE0 begins zero. Source169D0 writes60 only when that short is zero. Source123B0 calls original3368A0/3368C9 and, when the first returned value is3, writes50 or60 according to the returned flag40. Those platform video/configuration observations have not been supplied or executed here. The default options result is a source literal branch, not a measured hardware rate.
[Original zero/60 owner](../analysis/multiplayer/integration/spawn-source/local-map-clock/instructions/000169d0.asm),
[video/configuration owner](../analysis/multiplayer/integration/spawn-source/local-map-clock/instructions/000123b0.asm),
[original platform callers](../analysis/multiplayer/integration/spawn-source/local-map-clock/instructions/003368a0.asm),
[original flag reader](../analysis/multiplayer/integration/spawn-source/local-map-clock/instructions/003368c9.asm).

Actual callers190BA0,190BF0,23855A and6F190 own options construction in their respective launch/session paths. Their full listings are retained; no caller is direct-entered with assigned member/profile/lobby state to produce a successful multiplayer launch. Existing prepared options still have ratezero, and reset17 still produces positive-infinite dt. Genuine local options, participants and launch/countdown ownership remain the next separate gate.
[190BA0](../analysis/multiplayer/integration/spawn-source/local-map-clock/instructions/00190ba0.asm),
[190BF0](../analysis/multiplayer/integration/spawn-source/local-map-clock/instructions/00190bf0.asm),
[23855A](../analysis/multiplayer/integration/spawn-source/local-map-clock/instructions/0023855a.asm),
[6F190 session owner](../analysis/multiplayer/integration/spawn-source/local-map-clock/instructions/0006f190.asm),
[separate lobby/count/time evidence](MP_LOBBY_SOURCE_RESEARCH.md).

## Reproduction and freeze

One bounded case verifies12,986 original executed instruction addresses and90 instruction files, including88 bounded entry listings, the complete executed listing and1,757 observed original store sites. Independent execution replays the measured native API inputs and reproduces the report and every listing byte-for-byte. All349 preceding artifact/type/listing pins,11 untouched primary files and the compiled compatibility source/executable remain verified. Native addresses and complete private host records stay in separate files. No original instruction or production file changes.

```bash
.tools/python/bin/python tools/oracle_mp_spawn_local_map_clock.py \
  --clock-replay analysis/multiplayer/integration/spawn-source/local-map-clock/measured-native-clock-inputs.json \
  --output analysis/multiplayer/integration/spawn-source/local-map-clock/reproduction/report.json
```

[Complete original executed bytes](../analysis/multiplayer/integration/spawn-source/local-map-clock/instructions/executed-original-instructions.asm),
[original observed store sites](../analysis/multiplayer/integration/spawn-source/local-map-clock/instructions/observed-map-clock-source-store-sites.asm),
[independent verification and final pins](../analysis/multiplayer/integration/spawn-source/local-map-clock/independent-freeze-verification.json).
