# Fresh native TU1.5 physics frame

`PhysicsLiveWorldTu15::step_physics` now connects the complete original outer
physics frame to the same freshly constructed native world and resource owners.
Its canonical entry is `1C7660`, authenticated as English TU1.5 `1CC8D0`.
It requires both a constructed world and an admitted session. Missing original
callees propagate their actual failing state; this entry supplies no contact,
force, solver result or host movement fallback.

The source frame consumes the configured clock's `seconds_per_tick` at `+4`.
It runs the original world step, callbacks, object synchronization, deferred
release and floating-point control changes. The returned ABI retains the
source register and FP state while the native caller's FP environment is
restored. This API does not advance the game clock, deliver commands, construct
players or replace the encompassing object/game scheduler.

## Checked behavior

The frozen Release and address/undefined/leak sanitizer builds each pass both
map tests, without skips. Lockout and Coagulation each run 120 complete physics
frames and observe 20,880 ordered source writes. The tests also exercise the
missing-world/session gates and check world ownership and host FP restoration.
The world is fresh, with no player or dynamic-object population. These are
physics frames; the test does not claim 120 complete multiplayer game ticks.

Four selected native frames—the first two from each map—compare against the
unchanged TU1.5 instructions running on the host i386 CPU through the existing
test-only Linux ptrace bridge. Each comparison matches:

- 987 original instructions and all 43 ordered non-stack source writes;
- all 58,581,070 retained non-stack bytes, including untouched bytes;
- GP/XMM, defined integer flags, empty x87 depth, x87 status/control and MXCSR.

Integer flag comparison uses the declared `CD5` mask; Linux host control flags
are not Xbox interrupt-state evidence. Stack scratch banks are excluded from
the memory/store comparison. Every original memory read must lie within a
retained input range. No BIOS, original instruction patch, callee substitution
or native expected output is supplied to the original execution. The original
receives the exported native input bank; this does not independently prove the
preceding constructors or cache conversion, and is not Xbox hardware evidence.

The initial Unicorn comparison is retained as a failure. Its stores and final
banks match, but its x87 control word is `023F` instead of native `027F`.
The separate host CPU comparisons produce `027F` and pass the exact control-word
check. No x87 control bit was masked to make the emulator comparison pass.

The second frozen cohort recompiles all 23 linked physics/platform/heap source
files, along with the fresh frame and native-owner sources. Its four selected
input banks, final banks, store traces and exit ABIs are byte-identical to the
original CPU comparisons. All 44 compilation units use strict FP options and
are instrumented in the sanitizer build. The pinned cache archive is reused
and is **not** sanitizer-instrumented; these checks do not cover that decoder's
memory accesses. Other later edits in the shared checkout are explicitly
excluded from the frozen audit.

An actual `strace -f -e trace=%file` run of the second Release build completes
with no BIOS, EEPROM, captured kernel-table or reference-kernel file access.
Owned game executable data and map assets remain inputs. The BIOS remains
reference-only and is not a runtime or distribution dependency.

The separate [session-before-world audit](../analysis/multiplayer/sandbox-source/physics-native-frame-integration-20261012/session-first-v1/validation.json)
checks the initialization order used by the current TU1.5 match adapter. Only
the diagnostic test order changes; it links the same 43 pinned native runtime
objects and uses the same frozen headers. Each map passes 120 frames in both
Release and address/undefined/leak builds. The selected first frame from each
map also matches 987 original instructions, 43 non-stack stores and all
58,581,070 retained non-stack bytes. Its own file trace confirms no BIOS or
EEPROM access. This checks the source API order, not the entire Match
constructor or player birth.

## Evidence and reproduction

The immutable [frame audit](../analysis/multiplayer/sandbox-source/physics-native-frame-integration-20261012/validation-frozen-v2.json)
pins the two builds, compilation commands, test results, four original CPU
comparisons, native byte reconciliation, source/data/tool manifests and native
file trace. It separately records that live multiplayer parity and a new
playable executable are not established.

```sh
python3 tools/validate_physics_native_frame_tu15_20261012.py

# Actual root CMake target, after building its current dependencies:
cmake --build build-multiplayer --target physics_native_frame_tu15_tests -j2
ctest --test-dir build-multiplayer --output-on-failure \
  -R '^native_physics_frame_tu15_(lockout|coagulation)$'

# Repeat one original CPU comparison into a new output directory:
.tools/python/bin/python tools/differential_physics_native_frame_hardware_20261012.py \
  --case analysis/multiplayer/sandbox-source/physics-native-frame-integration-20261012/lockout-frames-v2/0 \
  --output analysis/multiplayer/sandbox-source/physics-native-frame-integration-20261012/lockout-hardware-repeat

# Repeat the session-before-world diagnostic into a new directory:
python3 tools/test_physics_native_session_order_tu15_20261012.py \
  --output analysis/multiplayer/sandbox-source/physics-native-frame-integration-20261012/session-first-repeat
```

The original comparison requires the privately owned English TU1.5 executable,
Capstone, the Linux i386 execution/ptrace backend and the existing complete-body
correspondence. It does not require a BIOS. Private bank exports and original
test inputs are validation artifacts, not distributable game assets.

## Remaining integration

This component closes the fresh-world outer-frame connection. It does not
resolve the red [visible lift and vehicle acceptance cases](PHYSICS_MULTIPLAYER_VISIBLE_TU15_20261012.md).
The live biped-construction prototype still reaches missing original callees;
its diagnostic is retained in
`analysis/multiplayer/sandbox-source/physics-native-frame-integration-20261012/live-biped-red-v2.log`.
That moving prototype is not the frozen frame cohort.

The required next gates are completed source body construction, actual player
command delivery, populated-world stepping and solved-state publication into
the match. The primary fixture constructs the world before admitting its
session; the separate diagnostic verifies the opposite source API order used
by the match adapter. Complete Match initialization remains unverified. Lifts,
teleporters, every vehicle/controller class, dynamic props, projectiles,
ragdolls/deaths, continuous contacts and multiplayer authority still require
source-led live acceptance across the requested maps and split-screen/LAN
paths. No updated launcher is delivered from this diagnostic cohort. The full
physics and multiplayer goal remains unfinished.
