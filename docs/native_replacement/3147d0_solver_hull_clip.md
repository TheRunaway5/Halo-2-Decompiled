# Actual 3147D0 solver hull facet clipping

The full 56-site Primary body has SHA-256
`fb1f686d30c823296eba36bf95bea60040458217bbde6e6c4b869dfb7d1f81fd`.
The private shared controller projects the point against the current
facet, compares and updates the live bounds, then reacquires both bounds
for the final relation and result. It preserves the order of the packed
projection and both scalar adds, including destination NaN payload
selection and the escaped upper lanes. The physical view preserves the
lower-formal read before projection and the upper-formal read and
scratch reload after the genuine saved-SI push.

GCC and Clang sanitizers each match 86 unchanged original calls, all 56
sites, 1,725 complete pre-I/O packets, 1,438 reads and 287 stores. The
matrix includes caller-frame aliases, paired packed NaNs, dense masked
incoming x87 depth through seven and 86 hostile-provider equivalents.
The original rejects three compiled defects: wrong final compare mask,
swapped packed multiply operands, and a misdirected upper-bound store.
The raw FSTP writes through the provider, then commits the pop before
the completed observer. The separate public value view keeps the old
public helper's host-FP and provider semantics. No signal, alignment
fault saved-RF, other image, or unsupported x87 claim is made.

The exact predecessor public comparison passes under both sanitizer
compilers: 1,956 complete calls, 23 provider/observer stops, 16 untouched
x87-depth refusals, 140 observer changes and 140 complete events across
both bound stores. The result, FP publication, stats, observer-visible
FP state and caller host FP restoration match. The compiled wrong dot,
inverted upper test and missing lower epsilon are rejected.

Reports: `build/test-results/native-solver-hull-clip-frame-20261110/` and
`build/test-results/native-solver-hull-clip-value-20261110/`. All owned
trees from this private stage were removed.

The retained-source view passes 174 cases under each sanitizer compiler
with the complete current and genuine generated FP profiles: all 56
sites, 3,492 pre-I/O packets, 116 stopped prefixes, two live SIMD
feedback controls and both bound publications in each source profile.
The observer sees the authored retained FSTP store before the logical
pop. The fresh spill helper publishes C1 at that store in the directed
upward-rounding case; the old source spill does not. A redundant
first-round FLD C1 control was rejected as a fixture control because
the shared push already clears it. The corrected exact upward-rounding
test rejects dropping fresh spill C1, popping before the observer, and
caching the projection lane across the last formal read. See
`build/test-results/native-solver-hull-clip-source-frozen-r3-20261110/report.json`;
all related owned source copies were removed.

The private source owner also passed the four exact current/fresh
raw/prepared combinations under both sanitizers. Per compiler, it checks
4,990 default comparisons, 4,474 native entries, 3,450 exact suffix
handoffs covering all 23 retired I/O boundaries, 516 whole-entry
refusals, and two unchanged foreign exceptions. The real Clip reference
control checks another 344 untouched entries. Both the installed-entry
and specializer APIs reject all 42 malformed variants, and the route
suite rejects four compiled owner defects. The immutable
`b38d9129782c5e320fcc586d412867d2f51bf2acac57674339f34e00dac93efd`
cohort is recorded in
`build/test-results/native-solver-hull-clip-tagged-source-frozen-r3-20261110/report.json`.
The earlier failed fixture runs remain separate. Its owned source copy
was removed; this is a private qualification, with no default installed.

The exact Primary default and public value facade were subsequently
installed. The 14-path rebase preserves all unrelated bytes; both
complete source formats and the public facade reverse to their exact
predecessors. The existing Clear source and public path remain
authenticated. The default retains the complete original fallback for
callback-bearing raw entries and unsupported provider/FP contexts.
`HALO2_REFERENCE_SOLVER_HULL_CLIP` forces that retained route.

The final installed cohort passed all five registered checks, including
whole current/fresh raw/prepared syntax and the adjacent Clear value
test, plus independent GCC routes and focused operand/catalog
validation. Both legacy compiler suites pass the original 539 Clip
cases (489 full returns) and 75 Clear cases (43 full returns), with
their three compiled defects per family. The two legacy runners copy
the real shared value header into their disposable fixtures; the Clip
defect controls target its relocated expressions. See
`build/test-results/native-solver-hull-clip-bound-official-20261110/report.json`
and `build/test-results/native-solver-hull-clip-installation-rebased-r2-20261110/live-reversal.json`.
The final snapshot and all owned builds were removed. This closes the
scoped TU1.5 owner and public wrapper; other images, the broader hull
controller and physical fault domains remain separate.
