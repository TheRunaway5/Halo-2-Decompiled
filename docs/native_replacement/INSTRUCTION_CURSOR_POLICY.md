# Live instruction-cursor policy

`State::instruction()` and `State::instruction_literal()` now revalidate the
live service callback and cursor witness before suppressing callback delivery.
The source State stores services by reference; its initial cached witness can
outlive a cleared or replaced callable, a changed executor/type, or a changed
cursor position. The literal fast path additionally checks the current exact
three-site protocol. Current trace flags still preserve full callback delivery.
A State whose initial witness was absent keeps the generic callback route even
if services subsequently acquire a valid descriptor.

The old source failed the authentic cached-clear regression: it wrote the old
cursor after `services.instruction` became empty. The bounded failure is in
`build/test-results/native-instruction-cursor-old-20261109/`. That sandboxed
failure also encountered LeakSanitizer's ptrace restriction; the repaired run
was executed outside ptrace and passed both GCC and Clang ASan/UBSan/LSan.

The repaired receipt is
`build/test-results/native-instruction-cursor-fixed-20261109/report.json`.
It checks 68 live-policy cases per compiler across base State and WorldState,
using both generic and literal dispatch. Cases include cleared/rebound
callbacks, live service-site changes, current trace flags, null or replaced
positions, missing/wrong callable types, wrong executors and initially missing
witnesses. The test forbids every memory access. Both owned scratch trees were
removed and verified absent; no generated large artifacts were retained.
The registered project gate also passes with pinned offline dependencies:
`build/test-results/native-instruction-cursor-official-20261109/`. Its complete
CMake/dependency/build owner is removed, with only bounded reports retained.

This repairs shared retained/prepared dispatch. Each native bridge separately
owns admission and must refuse stale witnesses before bypassing its retained
body. The sphere classifier's narrow admission repair and full physical
requalification have their own receipts. The fix is later than frozen r14;
it does not establish native-only runtime, frame, timing or delivery closure.
