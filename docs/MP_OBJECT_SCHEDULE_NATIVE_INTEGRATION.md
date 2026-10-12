# Native ordinary, dirty and new-object dispatchers

The native `B7E00`, `B7F50` and `B8040` dispatchers match the three recorded
original phases, including 15 ordered owning stores and ten producer calls.
The comparison checks 13 literal states: complete 2048×12-byte header storage,
the 256-byte occupied bitmap, the `0x4C` pool header, full `0x1D88` unit queue,
`0x84`-byte context and every occupied object body. Normal and focused
address/undefined/leak sanitizer comparisons pass. Authority:
[independently repeated active-item schedule](MP_ITEM_ACTIVE_SCHEDULE_RESEARCH.md).

`BD360`, `BD710`, `1019F0` and `C0270` remain explicit producer services.
The differential checks their full-datum arguments, order and entry buffers,
then supplies their literal original output buffers without moving storage.
It does not implement their motion, transforms, contact, cleanup or physics
world step, and does not replace those producers in the live multiplayer path.

## Source order and storage

Ordinary and dirty passes consume the original queue first, then scan occupied
headers in ascending bitmap index. They retain the original new8 exclusion;
the general scan also checks the body's queue bit. The source high-water and
occupied bits are read during each scan rather than copied into a visit list.

The new-object pass clears already-updated20 on every occupied header. It
requires new8, active1 and update2 with delete10 clear, then clears new8 before
the ordinary producer. A resulting dirty4 bit admits the dirty producer. It
then runs dirty cleanup, a second occupied scan for pending deletion, clears
context+3 and enters the final tail cleanup. In the measured schedule the
queue count is zero and no deletion branch executes.

The fixture's six context stores are `00→01→00` for each phase, including the
clear before `C0270`. The nine header stores include unchanged writes. Alternative
ordinary exits, queue-priority callbacks and deletion callbacks are retained
from complete original listings but have no executed coverage in this suite.
Missing services fail when their source phase needs them; they never imply a
successful update or deletion. Only the original 12-byte header layout is
supported. Raw address words remain opaque; native body storage is an explicit
caller responsibility.

## Reproduction and limits

```sh
cmake -S . -B build-multiplayer
cmake --build build-multiplayer --target object_schedule_tests -j4
python3 tools/differential_object_schedule.py
ctest --test-dir build-multiplayer --output-on-failure -R '^retail_object_schedule$'

ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
python3 tools/differential_object_schedule.py \
  --binary build-multiplayer-asan/object_schedule_tests \
  --output analysis/multiplayer/integration/object-schedule-asan-differential.json
```

The exporter pins the original manifest, report, audit, research document and
tools, verifies 167 prior dependencies and all 279 instruction files, and
requires byte-identical original report/audit/listing repeats. Expected buffers,
callbacks and stores come literally from those original observations. The
exported fixture remains in ignored local analysis storage; its CTest check
skips with code 77 when absent.

Nine focused normal checks and the same nine sanitizer checks pass:
`object-schedule-focused-tests.log` and
`object-schedule-focused-asan-tests.log`. A fresh disposable source-only snapshot
builds and runs 64 checks: 18 pass and 46 original-data comparisons skip. Its
787 copied files are pinned in `code-only-object-schedule/report.json`; later
documentation edits and shared source changes remain outside that snapshot.
The earlier broader 153-check result has a retained transparency asset failure
and is historical after adding this module. Fresh captures55/56 identify the
rebuilt runtime: title startup, four-view Turf firing/reload at tick62, and an
actual loopback dedicated host with one/two-view clients. Their runtime,
launcher and shader hashes remain stable during each run, with zero Vulkan
validation diagnostics. The scheduling module is compiled into that runtime;
these captures do not exercise its missing live producer/frame route or prove
complete multiplayer parity. Captures53/54 remain historical.

Source local command ownership, live object body storage and world admission,
the actual ordinary/dirty producers, the intervening world step, continuous
item settling, other contact contestants and complete frame order remain open.
