# Actual 314D80 solver hull seed facets

The complete 97-site Primary body has SHA-256
`aea4f9f90a0cf403578341309ffd367b0525925dcf6992033df9fdeace7e43b0`.
The typed kernel consumes the original 12-row tetrahedron link table,
builds the first real Plane, measures the live point against it, and
conditionally reverses the winding. It then runs the four actual Plane
rebuilds using the returned registers and frame. Each original Plane is
the complete qualified private `314A30` owner.

GCC and Clang sanitizers each match 48 unchanged original calls, all
97 sites, and 240 real Plane calls. Per compiler, 12,848 full packets,
7,184 ordered reads and 5,424 original stores match, including both
windings, degenerate geometry, paired packed NaNs, and hostile
provider equivalents. The original detects the wrong link-table
stride, omitted winding write and wrong x87 compare mask. Reports:
`build/test-results/native-solver-hull-seed-facets-frame-20261110/{clang,gcc}-r3.json`.

The first fixture put the facet array across the live edge-array
header; original Plane output overwrote the pointer and the subsequent
original MOVAPS faulted. The corrected backing layout fixes that
fixture, with no kernel change. All owned scratch trees were removed.
The private raw proof does not establish retained-source faults,
physical vector atomicity, another image or a production default.
