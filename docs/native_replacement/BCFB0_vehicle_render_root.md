# TU1.5 VehicleUpdate root lookup: BBE70 / BCFB0

The VehicleUpdate copy of canonical `BBE70` (actual TU1.5 `BCFB0`) now
uses the typed root lookup with the source's direct or literal callback
format. Calls with no instruction callback retain the existing renderer
bridge. Calls with callbacks preserve all 13 authored sites, including
the actual Phantom remap and the prepared quiet-cursor path. The reference
control `HALO2_REFERENCE_VEHICLE_RENDER_ROOT=1` uses the complete retained
body. The older World and ModelRender copies and MapPack 1.1 are unchanged.

An exact recovered archive exposed this issue without skipping the registered
VehicleEnvironment CTest. In original case 2478, `BC934` calls the root
lookup and the next caller site is `BC939`. The old unconditional native
branch omitted the 13 child observations, leaving 73 instead of 86 events.
The failure was identical with `HALO2_REFERENCE_VEHICLE_OBJECT_LOOKUP=1`.
On the same owned sanitizer build, removing only the older BBE70 prefix
restored the sealed body and passed all 16,381 cases, 3,651,499 instructions,
613,055 ordered writes and 2,082,340 full-state bytes. This is a diagnostic
control, independent of the new installed default.

The diagnosis receipt is
`native-vehicle-legacy-root-trace-diagnosis-closure-20261110/report.json`.
Private proof compared both source formats under GCC and Clang sanitizers,
with six archived callers and 42 virtual-read stops per format. The actual
bound private bridge passed the default and its separate forced-reference
control. Six unchanged-original hardware paths also checked the selected
identity, escaped GP, explicit ordered reads, committed banks and executed
site sets/counts across depths 0, 1, 2, 3, 8 and 64.

The source installation receipt is
`native-vehicle-render-root-installation-20261110/report.json`. The installed
archived default and its own forced-reference test each passed all 16,381
cases without skipping in the same immutable GCC sanitizer build, manifest
`83a10a5c553397e008afa8134393ef479e010bb8b9b9aaccf085aa22044e37d0`.
Registered operand specialization, migration inventory and original renderer
ownership also passed without skipping; the latter compared 423 original
calls, including 39 read-fault prefixes. Four full current and fresh raw and
prepared source units passed standalone Clang syntax. See the verified
receipts and hashes in `native-vehicle-render-root-closure-20261110/report.json`.

All owned builds, the copied source and the temporarily recovered fixture
were removed. Only bounded logs, reports and the compressed source manifest
were retained. The archive and persistent fixture path are unchanged. This
entry does not claim a replacement for the sibling owner copies, an engine
promotion, broader physical-fault semantics or full runtime closure.
