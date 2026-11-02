# Native TU1.5 gameplay sleep policy

The native `1D9B20/1D9B70` wrappers now compose the complete
`1D9BC0/F20C0/E7790/E65A0` eligibility chain with the previously qualified
native short/long body policy. The kernels read actual object/component/slot
and authored controller data. They preserve original lookup order, early
returns, SIMD unordered/zero comparisons, x87 integer conversion and result
publication. They accept no CPU frame, captured outcome or original-execution
service.

Both GCC and Clang pass422 unchanged English TU1.5 physical cases:347 full
returns and75 external-operand/store fault prefixes. The retained operand
bytes, read/store order, result and complete FP words match. Three mutations
(wrong attachment, treating unordered as zero and wrong wrapper period) fail.
[Component report](../build/test-results/native-gameplay-deactivation-policy-tu15-20261101-read-ledger/report.json).
The authenticated image SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

The first harness incorrectly required unchanged x87 status even though these
original bodies perform x87 arithmetic. Its failed result is retained. The
corrected harness compares the actual resulting FP words and verifies balanced
incoming live-value ownership on full return. A subsequent strict read ledger
found Capstone omitted the read-access flag for the authenticated
`E65C7: 84 90 48 03 00 00` (`TEST byte [EAX+348],DL`). The harness now records
that real byte read. Neither correction changes the original instructions or
weakens the persistent-byte or final-FP comparison.

The world binding selects these kernels for the two actual virtual owners.
Its two result bytes stay above the genuine residual callback stack. Four
model compositions exercise both wrappers with blocked vehicle controls and
the native body-policy path; no old wrapper is called. The full110 original
world cases,96 force compositions and rejecting world mutation also pass.
[Binding report](../build/test-results/native-world-gameplay-deactivation-binding-20261101/report.json).
[Frozen source](../analysis/physics/native-gameplay-deactivation-live-binding-20261101/source-pins.json).

v1464/02 passes its live policy gate while driving:92 native calls reconcile
to5 blocked and87 body-policy calls, with31 biped and61 vehicle eligibility
tests. Neither old wrapper occurs in residual accounting. All eight official
regressions and the normal/grenade/rocket strict original frames pass; the
driving sample compares89,729,945 bytes. The recording uses the actual vehicle
camera, and the rocket case additionally exercises genuine target death.
[Live results](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-contact-entity-recovery-original-v1464-20261101/iteration-02-summary.json).
Private control-stack aliases, unmasked FP exceptions and the remaining
world/controller migration are open. Disposable
compiler/original banks were removed; reports, commands and source pins remain.
