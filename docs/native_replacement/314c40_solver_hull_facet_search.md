# Actual 314C40 solver hull seed facet search

The complete 40-site Primary body has SHA-256
`cf064e4ddf5197668cf787167332cdae868d3559178c9b348cdb509ddbb3f3d0`.
The shared kernel initializes both output words and the live clipping
interval, calls actual `3147D0`, writes the returned status byte, and
calls actual `3148E0` only on the intersecting branch. Both child
returns use the live frame; the two epilogues retain the exact saved
register order and result.

GCC and Clang sanitizers each match 48 unchanged original cases, all
40 sites, 7,680 full packets, 4,844 reads and 2,944 original stores.
The calls include 72 genuine Visit entries and 92 genuine Clip entries
across the whole composition, including recursive descendants. Each
compiler rejects a wrong initial bound, a missing seed-status write and
a wrong nonintersecting result. Reports:
`build/test-results/native-solver-hull-facet-search-frame-20261110/{clang,gcc}-r2.json`.

The independent complete current retained-source comparison includes
both parents and their actual Clip child, with exact return-word,
pre-I/O, completed observer and provider-stop evidence. See
`docs/native_replacement/3148e0_solver_hull_facet_visit.md` for that
96-case combined scope. All owned trees were removed. No production
binding or full hull or other-image closure is claimed.
