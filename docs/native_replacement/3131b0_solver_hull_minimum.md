# Actual 3131B0 solver hull facet minimum

The private typed kernel covers Primary's complete 76-site minimum-facet
owner, body SHA-256
`0f800ab67d9e63dac43f5f72584b2050f2a3afdc12325e8b319e2a0e5eb455ac`.
It walks four candidates per batch and handles the signed scalar tail.
The actual x87 comparison selects a strictly lower ordered candidate; an
equal or unordered comparison keeps the existing facet.

The raw frame preserves the exact initial and final caller words. Each
candidate spills to the saved ECX slot, and the selected value is reread
from the same slot after the prior winner is dropped. The final POP
therefore uses the live rounded candidate bits. The tail increments the
facet pointer before the nonpopping FST, and the batch shift flags are
visible before the saved DI write.

GCC and Clang sanitizers each matched 96 unchanged original calls, all
76 sites, 1,998 full pre-I/O packets, 1,248 ordered reads, 750 stores and
96 hostile provider/observer equivalents. Both rejected the three
compiled defects: reversing the comparison, omitting the visible
candidate spill, and moving the batch shift after the saved DI write.
The corpus covers batch and tail paths, signed malformed counts that
return, infinities/NaNs and candidate aliases of the saved slot.

Reports are under
`build/test-results/native-solver-hull-minimum-frame-20261110/`.
The owned physical runs are removed; no large artifacts remain.

The private public value view shares the candidate walk but retains the
existing depth-zero contract and its privately rounded scratch. Both
sanitizer compilers matched 96 complete calls, 132 stopped provider-read
prefixes and 16 untouched depth refusals, and rejected three compiled
defects. The Frame, Value and finite CTest selector checks also pass on the
R29-derived private source copy. This is not a migration of the live public
facade yet.

The retained-source view keeps its modeled flags, source PUSH publication,
and the exact current/fresh spill and C1 profiles. A private complete-source
gate passed the four current/fresh raw/prepared contexts, exact retagging,
four whole-unit syntax checks and 24 strict input controls. The gate
rejects a bridge or continuation label outside the owning function. The
final owned copy passes registered 3/3 and both sanitizer source suites:
each 6,464 comparisons, all 24 exact frontiers, 4,022 native entries,
2,036 retained continuations, 2,580 stopped prefixes, 128 winner-reload
witnesses and 19 untouched refusals. Each rejects five compiled defects.
The copy is removed. Final receipt:
`build/test-results/native-solver-hull-minimum-private-cohort-r2-20261110/report.json`.

An earlier retained gate ran all 24 frontier sites but a concurrent
source edit invalidated its input check. The first independent retry
also found a missing test-helper directory and a stale-pointer defect
whose earlier winner was hidden by a later winner. The accepted copy
includes the real helper directory and captures the actual pre-reload
pointer. The earlier receipts remain unchanged.

The proposed installation passed the five registered Frame, Value, Routes,
Generation and selector checks in an independent owned copy. The two
overlays reverse to the exact prior Primary and public C++ files. Operand
integration and the corrected same-copy catalog test pass. The first
aggregate preserves the incorrect unittest class name as a failed
invocation. Both owners are removed. See
`build/test-results/native-solver-hull-minimum-binding-private-20261110/private-installation-closure.json`.
The exact Primary and public overlays are now installed. The rebased
candidate passed the full four-context generation, operand and catalog
checks after both E0900 owners were installed. Removing only this overlay
restores the complete prior Primary and public C++ bytes. The parent
reference control is `HALO2_REFERENCE_SOLVER_HULL_MINIMUM`.
The live reversal receipt is
`build/test-results/native-solver-hull-minimum-marker-live-plan-20261110/installed-reversal.json`.
The first immutable installed cohort passes six of seven registered
checks, plus the separate GCC source check. Only the full migration
catalog times out at its explicit 3,600-second limit. The supplemental
catalog-only attempt on a newer independent snapshot ends with signal
status 143 before a test result. No catalog pass is claimed for either
attempt. Both owners are removed. The earlier exact private proposal
catalog pass remains distinct. The full `313BB0` controller remains open.

The standalone legacy facet validator initially stopped before its compiled
checks because the Minimum implementation moved out of its C++ facade.
The repaired validator compiles the same stride, comparison and record
identity defects from exact copies of the shared value header. The
original cases, driver, reference image, and Clear and Clip import hooks
are unchanged. GCC and Clang each pass all four legacy tests (1,173
original paths, 1,155 returns, 18 stopped prefixes, all 76 sites, three
compiled defects rejected). The exact owned proposal also passes the full
Clear and Clip siblings (75 and 539 paths). The earlier failure and the
new receipts are retained under `build/test-results/native-solver-hull-minimum-legacy-seam-before-20261110/`,
`build/test-results/native-solver-hull-minimum-legacy-seam-private-r2-20261110/`
and `build/test-results/native-solver-hull-minimum-legacy-seam-installed-full-20261110/`.
All generated build and scratch trees are removed.
