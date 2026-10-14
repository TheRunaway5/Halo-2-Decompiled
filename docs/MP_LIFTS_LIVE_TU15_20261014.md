# Live original TU1.5 lift integration

The visible native tests use actual original-map placed objects, original
world/body/controller integration and actual UDP peers. No source contacts,
forces, velocities or completed physics bodies are supplied. Every passing
case retains a stable executed ELF, exact same-tick authority comparisons and
a continuously verified unaltered desktop mirror with recorded video.

The test's initial location is a declared pre-tick input from authored phantom
geometry composed with the original placement. Original1F31E0's literal table
selects capsule type1 at phmo+58, stride50, and hull type4 at+70, stride100.
`lift-shape-inputs-v286/shape-dispatch.json` authenticates this against English
TU1.5. `placed-lift-geometry-v287.json` retains every relevant original shape,
all selected placement records and current map hashes.

Separate upper exit and lower intake fields exist on Cyclotron and Headlong.
The first `authored-lifts-v286` pass selected the first field; those initial
exit-field checks do not establish lower-intake transport. Its report's
Lockout-only location label is stale after the geometry generalization.
The corrected fixture selects the lowest authored geometry across all phantoms
and records the selected definition/placement explicitly.

The lift gate now requires upward displacement of at least the actual authored
player height, together with positive upward velocity and exact LAN state.
It no longer counts tiny grounded position corrections as a successful lift.
This is a bounded transport check, not a claimed full retail trajectory or
walking approach/exit proof.

| Original placed definition | Exact observed UDP states | Upward displacement | Maximum upward velocity | Authored player height gate |
| --- | ---: | ---: | ---: | ---: |
|midship|182|4.341179|3.671978|0.725000|
|midpillar|182|0.994680|2.217461|0.725000|
|warlock|181|2.050681|2.735266|0.725000|
|cyclotron-short|182|3.314285|2.716865|0.725000|
|headlong|182|6.733249|3.830126|0.725000|
|colossus-high|181|10.516462|9.152491|0.725000|
|foundation|181|4.813711|3.960793|0.725000|
|lockout|181|3.077028|3.964367|0.725000|

Midship uses `lift-render-admission-v290`; the other seven use
`contact-agent-lifts-v291`. Evidence root:
`analysis/multiplayer/physics-live-tu15-20261012/`.

Midship initially stopped at particle rendering before lift observation.
The isolated build adopts the existing signed-texture uploader narrowly,
and the production particle renderer now applies authored NV2A per-stage
alpha kill before combiners. Line-width state158 is admitted for its filled
triangle outputs; it has no line rasterization effect there. The original
English TU1.5 SDK line-width owner3FF810 emits method40380. The Cxbx enum is
navigation only; the original source listing and native output topology are
the implementation evidence. Existing captured particle draws remain exact
for both depth formats, including line-width controls. Those captured-draw
regressions do not establish TU1.5 gameplay pixel parity.

The pillar case then reaches missing original contact-agent constructor3040D0
from2F76F3. Its original registration at304B63 and eight-entry vtable41D03C
are authenticated. Sixteen complete additional native bodies and1,457 original
instruction sites close the direct dependency graph; existing actual native
owners remain responsible for allocation and all other calls. The collision
query module totals19 complete routines and1,582 sites. The actual pillar
rerun passes after integration. `contact-extension-source-v291/` retains the
read-only original listings, closure and generation evidence.

Cyclotron's main intake remains red. The earlier positive-only gate counted
about0.014 units of grounded movement; the strengthened gate correctly fails.
`cyclotron-phantom-trace-v291` retains182 exact observed UDP states and a
20-MiB read-only before/after1DA750 trace over the actual components, units and
motion banks. It confirms actual phantom velocity delivery; later phases
return the stalled actor's velocity to zero. Contact/controller admission and
an ordinary approach remain under investigation. No force or gravity tuning
is changed to make the case pass.

All other authored placements/instances, machine lifts, ordinary approach/exit,
quantitative original retail trajectories and full physics parity remain open.


## Ordinary approach and current-source integration, October14

`cyclotron-velocity-registers-v297` distinguishes the shared controller return
from the explicit zero-vector branch. The observed zero vectors are stack
inputs to original1D9080, consistent with dynamic E508B; E51D6 alone does not
prove that the explicit zeroing branch ran. The optional trace reads the
unchanged source registers/stack and detaches before captured streams retire.
The complete phantom-force generator repeats byte for byte with that observer.
The extended19-body contact-agent module also repeats byte for byte using
the same pinned lowerer as the isolated build.

`cyclotron-approach-v298` declares a BSP-grounded point outside the authored
lower field and walks into it through ordinary SDL forward input. It passes:
182 exact observed UDP states,19 forward commands, zero jumps, initial
Z -0.646760 and peak Z4.211033, with maximum upward velocity2.993616. The actor
finishes grounded at Z3.202404. The runtime stays unchanged and the live
mirror remains verified throughout. This resolves the previous endpoint-spawn
failure as a fixture limitation; neither gravity nor original force logic was
changed. All quantitative retail trajectory comparisons remain open.

The first approach matrix also passes Colossus high and Cyclotron short.
Foundation rejects the declared forward-side start because no cached BSP floor
exists there. The fixture now checks deterministic directions around the
actual field radius and admits only a clear BSP-grounded input position.
A fresh coherent current workspace snapshot, `source-snapshot-v299.json`, is
being built independently. It includes current native protocol80 and current
renderer/player lifecycle interfaces. The retained v242 isolated runtime uses
protocol78; its live results must not be described as protocol80 validation.


Further ordinary-approach runs retain their failures. Foundation and Headlong
pass after clear-floor admission, with182/183 exact UDP observations and
50/89 actual upward phantom deliveries to the actor. Midship main passes183
observations and35 upward deliveries. Lockout passes182 observations after
its zero-radius convex hull receives a separately declared approach distance.
Midship pillar walking remains red: the player stays on the lower floor below
the raised phantom. A normal jump-entry test requires actual original upward
force delivery and another authored player height of rise after first delivery;
a jump's height alone cannot pass it. Warlock's first convex-hull approach
lands on the upper lip; its17 actual force deliveries produce no full lift.
The next input fixture anchors its floor query at the lower authored placement
Z while preserving the authored horizontal field center. No source force,
contact, controller state or solved transform is supplied by these fixtures.


The corrected lower convex-field approach passes Warlock with182 exact UDP
observations,3.342016 units of rise and maximum upward velocity3.537312.
`warlock-lower-approach-v302` retains the raw trace and video. Midship pillar's
normal jump-entry run `midpillar-jump-entry-v301` passes with one jump,
67 actual upward phantom deliveries and2.809743 units of further rise after
the first delivery, against the0.725000 authored player-height gate.
Its earlier no-jump walk remains a failed case. Across the nine definitions,
eight have ordinary no-jump approach transport proof; the raised pillar has
normal jump-entry plus actual-force/post-entry-rise proof. Additional authored
placements and original retail trajectory comparisons still need validation.


The fresh current source build is protocol80. `current-protocol80-camera-v304`
passes the moving Spectre passenger/driver source cameras and UDP state.
`current-lift-approaches-v306` passes Cyclotron, Lockout and Warlock after an
explicit fixture recompilation. The first v305 attempt used the earlier test
object because copy2 preserved a source timestamp older than the build object;
that attempt and its correction are retained. Production source/ABI did not
change in that correction. Eight focused/departure checks pass for the current
source, including the528-case original force corpus and actual original-player
and UDP departure tests.

The20-placement current-source sweep initially found a short Cyclotron intake
with no source force delivery. Its actual SDL trace showed the automated
approach steering away from the target: native yaw subtracts mouse X movement,
while the fixture correction used the opposite sign. That test input is now
corrected; no original physics/control logic is modified. The v307 sweep parent
was stopped after identifying the issue, and its active Foundation placement3
case was allowed to finish/clean up normally. The incomplete sweep and every
raw run remain retained. Fresh v309 placement validation is required before
claiming all-placement coverage.


The corrected v310 sweep passes all20 authored placements, with3,633 exact
UDP observations and1,053 actual upward force deliveries to the actor. Every
case uses the same stable protocol80 executable and verifies its live mirror
throughout. See [the placement matrix and videos](MP_LIFT_PLACEMENTS_TU15_20261014.md).
Moving machinery and original retail trajectories remain separate open gates.


Current coverage expands to 28 placements across 13 map-local definitions,
with 5,088 exact UDP comparisons and 1,374 observed upward deliveries. See
[placement evidence](MP_LIFT_PLACEMENTS_TU15_20261014.md#further-authored-player-fields)
for the eight additional fields, per-build videos and retained failures. The
wider 43-placement phantom inventory includes non-player and initially absent
objects; it does not establish complete lift coverage.
