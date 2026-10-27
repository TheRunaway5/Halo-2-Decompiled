# Native TU1.5 island deactivation transition proof

Actual TU1.5 `316CD0` (39 sites) and notification owner `322740` (98 sites)
are implemented as typed native algorithms in
`native_tu15_island_transition.hpp/.cpp`. The transition appends the island to
the inactive array, swap-removes it from the active array, rewrites both
indices, clears active byte `+29`, and dispatches deactivation notifications.
Notification dispatch visits listeners in reverse order, compacts null entries
with stable shifting, and visits the changing island body cohort. No virtual
CPU, register/flags machine, or old root is executed by these algorithms.

Source reloads are explicit. Array growth can change the actual backing and
count. The transition rereads the target island index and active backing after
publishing the swapped element. World notification callbacks can replace their
listener backing/count. Body notification retains the original body's listener
header while reloading the body argument from the island before each callback;
the outer body count is reread after each body. Null compaction rereads backing
and count at the original phases. Counts/index arithmetic wraps as unsigned
32-bit values with source signed comparisons.

Actual constructor `3164F0` installs interface vtable `41D958` at object `+8`
(`3164FF`). Its slots `+4/+8` both point to actual `2E4D60`, the one-instruction
`RET4` leaf. That default notification is a direct native no-op. Other listener
owners remain explicit named callbacks with real receiver, owner, subject and
FP context. Actual `2E3250` array growth remains an explicit dependency receiving
the actual array header and stride four; no allocation outcome is invented.
These roots perform no FP instructions and transfer FP state only through real
callbacks. Their semantic result does not include volatile CPU outputs.

The required original validator executes unchanged physical i386 instructions
from English TU1.5 SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Its operand tracer retains ordered architectural reads, original stores, final
banks and full numerical FP control/status. It executes genuine default
notifications to completion and stops at actual entries for unconverted
growth/custom notification dependencies; no callback result is supplied.

`build/test-results/native-island-transition-tu15/report.json` records:

- 178 physical original cases, including 129 complete calls, five actual
  dependency entry stops, and 44 read-fault prefixes.
- 914 ordered stores, 7,680 ordered reads, and 22,682 original instructions.
- GCC 16.2.1 and Clang 23.1.4 Release passes under strict FP options.
- An unchanged-original negative control rejecting a private mutation that
  writes the inactive count rather than `count-1` as the island index.
- Mutable backing/count/index/listener model coverage, body argument/header
  lifetime behavior, genuine callback FP transfer, and default no-op closure.

Reproduction uses owned scratch:

```sh
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  .tools/python/bin/python tools/validate_native_island_transition_tu15.py \
  --output build/test-results/native-island-transition-tu15
```

The 15,132,709-byte raw fixture, original ELF/observer, object files and both
private compiler builds were removed with
`.local/test-scratch/halo2-test-m024b4x9`; that path was checked absent. Small
reports, commands and hashes remain. No large artifact was retained. Required
fixture absence is a failure; running model tests without a fixture does not
constitute original comparison.

Root integration is still required. Array growth/custom listener execution,
full composed live-world comparison, visible multiplayer and native-only
physics closure remain open. This proof does not change the separate v1457
95-byte raw-bank failure or its allocation-lifetime diagnosis.
