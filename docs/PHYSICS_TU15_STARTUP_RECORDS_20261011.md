# Fresh TU1.5 startup record ownership

English TU1.5 is the sole parity target. The new
`source_run_tu15_process_startup_records` entry composes the existing compiled
process-heap, native storage/lock and shutdown-registry owners with the two
record operations reached before heap creation. It selects TU1.5 exclusively.
Original call/import/return words and arguments authenticate directly against
the owned image, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Retained older address IDs identify existing compiled bodies internally; no
older executable runs in the new recovery or acceptance checks.

| TU1.5 call | Imported owner | Actual arguments | Native operation |
| --- | --- | --- | --- |
| `2E00C1`, return `2E00C7` | `107`, KeInitializeDpc | `56EDC0`, callback `2E009D`, context zero | Write Type16, Inserted byte, routine and context only. |
| `2E00CE`, return `2E00D4` | `113`, KeInitializeTimerEx | `56EDE0`, type zero | Initialize the defined dispatcher header, self-linked wait head, timer list fields, due time and period only. |

The native void ABI validates the complete actual frame and writable record
extent before mutation, consumes the original stdcall frame, and preserves
other live CPU state. Unknown callers remain unowned. Wrong return/argument
words, tail delivery, read-only records and missing extents reject before
changing the record or CPU state. DPC importance/list/system arguments and
timer Absolute/Dpc fields retain their incoming bytes. Initializing these
records neither inserts them into a scheduler nor executes their callbacks.

Record definitions and expressions come from pinned primary compatibility
source, Cxbx-Reloaded commit `585c49a50af1255ab155099e06f24505f9c5a800`,
already retained under `analysis/`. The independent oracle extracts its actual
DPC/timer bodies, structures, enum and list helper, excluding logging and the
non-null assertion for these non-null inputs. An i386 compilation without
SSE/MMX produces every expected field write. The native implementation is not
used to produce those results. The timer's chained wait-list assignment writes
Blink before Flink; its 64-bit zero is observed as low/high DWORD stores. Native
write order matches that declared compiled primary reference, not an invented
Xbox kernel instruction trace. The existing independently compiled shutdown
list expressions provide ordinal47 in the full startup-entry cases.

24 fresh original TU1.5 procedure-entry prefixes start from literal image
header/sections and loader-zeroed virtual storage, with explicitly declared
patterned stack/GP/XMM state, two flag patterns, three stack alignments and
empty x87 TOP0/3. They enter full startup `2DBA3E` or setup `2E00AF` directly;
they do not execute the XBE entry point or restore any old world checkpoint.
All 169 instructions in the three retained startup/setup/timer-arm bodies are
byte-authenticated; the prefixes execute 28 distinct sites. They compare 504
source instruction observations, 348 ordered source stores, 360 record stores
and 48 shutdown-list stores, including their interleaving, GP/XMM/defined
flags/FS/FP state and final retained banks. 36 separate patterned record cases
compare 270 independent primary writes and untouched bytes. Original recovery,
primary compilation and measurements repeat byte-identically.

The prefixes stop before TU1.5 `2E00E7` reads unresolved imported hardware
data322 through cell `41B66C`. The encoded import is not a hardware record.
Original measurement stops before consuming data; native execution refuses
the missing bank while retaining the matching preceding state. No hardware
bit, settings result, timer outcome, heap pointer or successful default is
supplied. Native process-heap installation has not occurred. This is fresh
bounded TU1.5 procedure evidence, not completed TU1.5 startup or world creation.

Evidence and sealed source/build records are under
`analysis/multiplayer/sandbox-source/tu15-startup-records-20261011/`.
Standalone checks compile every reachable startup/platform dependency from
source, with linker section collection dropping the unused world-wrapper
function. No replacement world implementation or stub is linked. The normal
project target retains its complete world dependency. Release and ASan/UBSan
checks compare the independent fixtures; their exact completion is recorded
by the validator. No original executable bytes run in production.

Remaining startup owners include the real hardware data, conditional settings
query/conversion, timer clock/queue/dispatch lifecycle and later resource/error
branches. World/BSP/body/controller construction, contact/solver ownership and
live player/vehicle/object integration remain required. These native platform
contracts do not prove original Xbox kernel clobbers, IRQL or hardware behavior,
and full physics parity remains incomplete.
