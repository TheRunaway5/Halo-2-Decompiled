# Native direct attachment lights

pStencil's optional point-light bank can read completed native TU1.5 direct
`ligh` attachments. This includes named object functions and attachment lights
whose definitions have a duration. These attachments keep their native
allocation, function evaluation, change color, position and retirement owners.
The reader never writes the native bank. Original rendering retains its path.

The local May 2003 `light_new_attached` (`E46F0`) clears its standalone timed
flag. Its `light_update_state` (`E4E70`) samples the object's function and color
bank before interpolating light color, size and brightness. TU1.5 has the same
ownership split at `C2260` and `C2CB0`. A definition's duration must not become
a standalone fade for this branch. Attachment row `+16` is the function SID;
`+12` is the signed color selection. Row `+20` is not a second light function.

The readback validates occupied object/light slots, full salted generations,
definition, authored slot, function and color binding. It copies completed
`C4/D0/D4/E0/EC` light state and the native `84/AC/B8` transform. In particular,
it does not reconstruct a weapon marker or a first-person light adjustment.
The optional bank registers those completed values for its existing receiver
and stencil-caster queries. Inactive native lights stay inactive. A missing
native snapshot cannot turn a named/function-driven attachment permanently on.

The native Plasma Rifle regression creates actual Coagulation equipment row7.
It supplies explicit heat inputs `0, .25, .5, 1, .5, 0` at the original weapon
field, then executes complete native `C2CB0`, including `BCB70/1046C0`. pStencil
matches the resulting light values and transform exactly. The test also checks
off/on spatial registration, stable identity, retirement, salted replacement,
and equality of the complete native-memory checkpoints across readback.
This is an explicit heat fixture, not evidence of firing controls.

A second regression exposed a missing scheduler phase: explicit `C2CB0`
updates passed, but a normal native tick left a sleeping weapon's light off.
The original frame calls the global light updater at `13AB4E -> C3430`, after
the effect phase. That whole134-instruction routine is now compiled directly
from the authenticated TU1.5 image and called in its original position. The
normal-tick activation regression then passes. This is shared native light
lifetime work, so Original pixel and game-state controls are required alongside
the pStencil presentation checks. Generation evidence is retained in
[native-tick-generation](../../analysis/pstencil/attachment-light-audit/native-tick-generation/).

The capture tool's `native-attachment` option uses that completed native light
without a replacement position or intensity. The ordinary external-light
fixture separately requires positive specular highlights. A real attachment
may produce zero specular on its own body at a particular angle; that measured
result is retained. Both paths check visible stencil occlusion and exact
Original round trips.

Primary disassemblies and the original-state probe are retained in
[attachment-light-audit](../../analysis/pstencil/attachment-light-audit/).
Implementation stays in [attached_lights.cpp](../../src/pstencil/attached_lights.cpp)
and [point_lights.cpp](../../src/pstencil/point_lights.cpp).

This does not add unresolved frustum/HSV/light-definition animation consumers,
or prove historical frame equivalence. Campaign submission of these lights is
a separate integration requirement.
