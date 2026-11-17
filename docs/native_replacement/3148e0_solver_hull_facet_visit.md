# Actual 3148E0 recursive solver hull facet walk

The full 74-site Primary body has SHA-256
`1784902fe0c372d89787a718641f780a71e377f7e1ccd902babcd2cc87890c58`.
The shared controller reads each of the three live neighbor links,
clips a zero-status neighbor through actual `3147D0`, and recursively
visits intersecting facets. It checks the returned status, collects
eligible links, and publishes the live free-list update before restoring
the saved registers. It retains the source and raw x86 differences in
PUSH and read/modify/write publication.

GCC and Clang sanitizers each match 48 unchanged original cases across
all 74 sites, including 48 additional recursive calls and 66 genuine
clip calls, with 6,840 parent/child packets, 4,464 ordered reads and
2,496 original stores. Hostile provider equivalents match within the
declared ordinary-backed contract. Both compilers reject wrong link
stride, omission of the collected count increment and a wrong stop
relation. The first fixture compile used a value-returning callback on
the void raw FP I/O API; the corrected adapter returns through ordinary
captured storage. Reports:
`build/test-results/native-solver-hull-facet-visit-frame-20261110/{clang,gcc}-r3.json`.

The combined retained comparison against the complete current seed
search, recursive visit, and Clip bodies passes 96 complete calls per
compiler, all 74 visit sites and all 40 seed-search sites. It compares
14,352 pre-I/O packets, 1,613 stopped provider or observer prefixes,
120 actual Visit calls and 158 actual Clip calls. Child entry, real
return word, returned ABI and resumed memory are compared independently.
Reports: `build/test-results/native-solver-hull-facet-walk-source-20261110/{clang,gcc}-r1.json`.

All related owned scratch trees were removed. These private scopes do
not establish a production default, other image coverage, arbitrary
provider mutation, whole physical-fault coverage or full hull closure.
