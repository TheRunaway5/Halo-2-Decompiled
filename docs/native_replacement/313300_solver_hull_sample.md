# Actual 313300 solver hull sampler

The typed kernel expresses the three-sample seed recurrence of Primary
`run_313300`, complete body SHA-256
`abb1e9c9f80b2d38224c12612dc1fbde9091798d3579728bcd998980f7264703`.
The raw view preserves the actual caller-slot updates, reloads the lower
bound and scale for each sample, and stages each x87 store and pop. It has
no child calls.

The first i386 comparison exposed a full pre-read EFLAGS discrepancy at
`313312`: the actual first immediate IMUL preserved incoming ZF for the
seed-zero case, while an unseeded host IMUL captured unrelated flags.
The raw view now seeds the actual source context before each native IMUL.
The full original packet comparison is unchanged; no flags are masked out.

GCC and Clang sanitizers each matched 80 complete unchanged original calls,
all 36 sites, 1,600 full parent pre-I/O packets, 1,120 ordered reads and
480 stores. The corpus includes aliased output and caller slots, preserved
live x87 input values, signed zeros, infinities, NaNs and denormals. Each
compiler also matched 80 hostile provider/observer/Before cases to the
same original outcome through the raw FP scope. The source-level modeled
flags and hardware partial-fault cases are separate domains.

The bounded reports are under
`build/test-results/native-solver-hull-sample-frame-20261110/`.
Both successful raw owners and the failed original diagnostic owner were
removed; no binaries or raw banks were retained. The public depth-zero
value candidate uses the same three-sample policy with ordinary DATA
addresses. Both sanitizer compilers matched 160 old public calls, including
28 cases where the output overwrites the scale constant before its next
read, 108 provider and observer stops, and 12 nonzero-depth refusals.
Three compiled public defects failed on both compilers. The exact supported
Primary route and public sampling facade now share the typed kernel.

The registered Frame, Value and real selector passed 3/3 on independent
manifest `6a4cf26cadf849d75bb9ca0ca40a71eb11b2556b483ba3fd3396c6f4750f8f10`.
The selector checked 401 exact names and 802 negatives; the owned copy was
removed. `private-frame-value-closure.json` records the reports and cleanup.

The retained SourceFrame uses the generated model's flag and software
FP helpers. It keeps the completed observer before the software pop, unlike
the separate hardware-publication view. Genuine fresh source differs from the
current body at four load C1 updates, three calls to its exact single spill
helper, and the final ST0 self-store and pop C1 update. Both private sanitizer
compilers passed 2,848 retained-source comparisons, 912 continuations,
1,024 read/write/observer stops, 19 untouched refusals and seven compiled
defects. Exact current/fresh raw/prepared admission and the 19 authored
suffix labels were authenticated before insertion. The independent
`HALO2_REFERENCE_SOLVER_HULL_SAMPLE` control forces the exact retained body.
The installed frozen cohort passed all 11 registered checks, including
the four sampler checks and full catalog. The independent installed GCC
sanitizer passed 2,848 cases, 1,744 native entries, 912 exact suffixes
and 19 untouched refusals. Its owned tree was removed. Final receipt:
`build/test-results/native-solver-hull-line-sample-bound-official-20261110/report.json`.
Physical fault domains and the rest of `313BB0` remain open.
