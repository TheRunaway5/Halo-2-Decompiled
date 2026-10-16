# Opening guard cloth construction

Actual x01_10_problem_actors creates scenario names8 and9 near the end of scene09.
Their real source births call original119060 with captain-flag tagE56103EC.
The test declares source seed12345678 and preserves the original default variants.
Both actual calls return NONE through the no-marker/free branch. Requiring a
successful flag allocation for these inputs was an incorrect test assumption.

Two untouched original replays each execute1690 instructions/1287 stores and
match native registers, floating-point state and all original-written or
native-changed bytes. Captured source backing remains complete; CPU pages load
its exact bytes on demand, and every game read/write must remain within backing.

The six whole original cloth construction/initial-geometry bodies contain699
instructions. This check covers the declared no-marker/free branch, not initial
relaxation success, cloth animation, cloth pixels or complete opening playback.
The final evidence is
analysis/opening-cinematic-20261015/cloth-construction-integration-v6/RESULTS.md.
Its three serial checks pass in2.260s under the unchanged60MiB hard/64MiB measured
ceiling and zero swap. The generic read-only entry capture helper compares large
source mappings in64KiB chunks instead of allocating a duplicate49MiB mapping.
