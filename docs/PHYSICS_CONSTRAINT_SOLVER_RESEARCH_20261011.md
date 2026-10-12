# Original constraint solver research

This records the original-only research preceding the compiled native solver.
The implementation and its later original/native comparisons are documented
in [PHYSICS_SOLVER_PORT_20261011.md](PHYSICS_SOLVER_PORT_20261011.md). The live
multiplayer mover is still the host `CollisionWorld`.

`tools/research_physics_solver_dispatch_20261011.py` executes fresh copies of
both supplied Xbox executables. The full paired bodies and internal branch
topology are authenticated by
`tools/research_physics_solver_motion_20261011.py`. The solver is `322ED0` in
update1.1 and `326A40` in TU1.5.

The supplied executable's actual initialization succeeds through `322D60`
and its original checksum helper. The research tool never forces the initialized
flag. Each fresh run changes it from `00` to `01` by executing original code.
No instruction, license result, clock result or solved body is patched in.

The original dispatch tables contain 17 schema indexes. They must be ported
from their complete branches; the table below identifies code, without assigning
unverified gameplay meanings to the indexes.

| Index | Update1.1 iteration entry | Update1.1 readback entry |
| --- | --- | --- |
| 0 | `324650` | `324C26` |
| 1 | `322F30` | `324A87` |
| 2 | `323527` | `324AF7` |
| 3 | `322F56` | `324ABA` |
| 4 | `323F12` | `324B3A` |
| 5 | `3244CE` | `324AF7` |
| 6 | `3241CF` | `324AF7` |
| 7 | `323AC2` | `324B7D` |
| 8 | `32378B` | `324BC1` |
| 9 | `323D26` | `324B3A` |
| 10 | `323E1E` | `324B3A` |
| 11 | `324310` | `324AF7` |
| 12 | `323FE5` | `324B3A` |
| 13 | `3240B1` | `324AF7` |
| 14 | `324636` | `324C1B` |
| 15 | `3245F8` | `324C1B` |
| 16 | `32461D` | `324C1B` |

The source-only lifecycle controls use the actual original dynamic-motion
constructor and preparation. A declared schema header plus end record exercises
zero constraint rows with 0, 1, 2 and 8 iterations. All eight executions across
the two images complete through the real source solver and repeat byte for byte.
They are diagnostic arithmetic controls, not physical contacts or native solver
validation. The source's first iteration copies instantaneous velocities into
the averaging fields; subsequent iterations accumulate them. Its final schema
traversal produces output records separately from that accumulation.

The full traces, paired table destinations, ordered persistent writes, source
pins and full stored states are retained in
`analysis/multiplayer/sandbox-source/physics-solver-motion-20261011/solver-lifecycle-verified-v3/report.json`.
The byte-identical repeat is `solver-lifecycle-repeat-v3`. Earlier incomplete
research attempts are retained separately. These eight original-only controls
are excluded from the 387 native comparison cases.

The subsequent native translation includes initialization and all solver schema
branches. Real Jacobian production, contact-packet production and live source-world
ownership remain required dependencies before replacing the live mover. The
completed wrapper comparisons use declared existing schema data and do not
validate a game-produced contact.
