# Native TU1.5 BV-agent owners

Nine Midship variant contexts reached an unowned indirect call at `315A4C`
to `2F18D0` during the actual `14FEC0` world update. The native implementation
now contains complete original bodies for `1CDFA0`, `2F1790`, `2F18D0`,
`2FE0F0` and `30B960`: 289 original instruction sites, lowered offline into
compiled C++ arithmetic, branches and calls. Production executes no XBE blob,
opcode decoder or emulator. Other owners retain their explicit dispatch and
missing-owner failures.

## Original reference and native comparison

The opt-in entry observer captured the actual Midship scheduler input. Its
89,870,755 bytes of full-world banks are disposable recovery input, not a
retained production dependency. Offline original execution followed 90,448
instructions to reach and complete the indirect BV call; the original data
selected its target. No collision, admission or subcall result was supplied.
The resulting BV witness contains 453 executed instructions, 83 ordered stores
and 32,808 complete input/output bank bytes. It calls existing native owners
`31EDE0` three times and `312620` once.

The first native comparison exposed floating-point differences from Unicorn.
An independent libc-free Linux i386 process then executed the untouched
original instructions over the original-derived entry. Its complete path and
memory agree with the emulator, but its x87 control word is `027F`, status
`0120` and MXCSR `1FA0`, where the emulator reported `023F`, `0100` and
`1F80`. Native code agrees with the original CPU values. The authoritative
fixture therefore uses the actual original i386 exit state, with no native
expected output used. This is CPU instruction evidence, not a physical Xbox
framebuffer or audio comparison.

The strict native test compares all recorded instructions and ordered stores,
defined integer flags, GP/SSE/FS state, full x87 status and control word,
extended values, all MXCSR bits and every retained bank. A private final-EAX
bit flip fails the integer/SSE/FS gate. The ordinary strict candidate passes.

- [Original scheduler execution](../analysis/vehicles-tu15-goal-20261023/bv-original-v4.json)
- [Untouched original i386 CPU run](../analysis/vehicles-tu15-goal-20261023/bv-hardware-v1.json)
- [Reproducible native generation](../analysis/vehicles-tu15-goal-20261023/bv-native-generation-v3-roundtrip.json)
- [Strict component qualification](../analysis/vehicles-tu15-goal-20261023/bv-native-component-v5-strict/report.json)
- [Negative control](../analysis/vehicles-tu15-goal-20261023/bv-native-negative-v1/report.json)

The required original regression fixture,
`tests/fixtures/vehicle_bv_agent_original_tu15.json`, is explicitly retained:
144,493 bytes, SHA-256
`903785b8fc804a8960ef4c3b70b924fafea03405597106099479735852d67d9a`.
Its adjacent provenance identifies the source image and reference reports.
Raw world banks, compiler products and i386 executables remain inside the
owning `tools/run_test.py` lifecycle and must be removed when recovery ends.

## Gameplay integration

`physics_live_world_tu15.cpp` dispatches these entries to
`source_run_vehicle_bv_agent_tu15`, preserving the original memory, ABI,
floating-point context, callbacks and store observer. The CMake module links
the native owner into the game and registers `original_vehicle_bv_agent_tu15`.
The [scoped refresh](../analysis/vehicles-tu15-goal-20261023/native-stage-v3/refresh.json)
authenticates the source/build integration. The
[Midship regression](../analysis/vehicles-tu15-goal-20261023/native-bv-midship-v1/report.json)
passes all nine actual world contexts and the strict original component test.
The [cleanup receipt](../analysis/vehicles-tu15-goal-20261023/bv-recovery-cleanup-v1.json)
verifies removal of the 310,868,406-byte recovery owner, including raw world
banks, original CPU harness and compiler products. The required bounded
original fixture remains; the main gameplay build has its separate active owner.

One complete witness qualifies its reached path; recovering all five bodies
does not prove every branch or vehicle interaction. Constructor acceptance,
weapons, rendered output, audio, destruction, campaign behavior, replication
and launcher delivery remain governed by the full
[vehicle checklist](VEHICLES_TU15_GOAL_CHECKLIST.md).
