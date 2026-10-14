# TU1.5 line emission for map particles

`initialize_particle_birth` now implements the actual shape 9 branch of
English TU1.5 `252480`. `NativeWorldEffects` admits that shape through its
existing shared birth path. Shapes 6, 7 and 8 remain rejected pending their
independent implementation and evidence.

This removes the shape rejection from Warlock's `water_stream_2` and
`water_stream_3`, and from Deltatap's waterfall-hit/drip child graphs. The
original branch evaluates the authored radius and angle; it takes two angular
RNG draws and one Y-position draw through `259B0`. X and Z stay positive zero.
The native code preserves the original float stores, x87 double intermediates
and `0.f - radius` subtraction. There are no map-specific particle parameters.

The initial native comparison failed on the unsupported-shape guard. After
implementation, **3,072 whole original birth comparisons pass**: 1,408 from
Warlock's 11 actual shape 9 emitters and 1,664 from Deltatap's 13. All 64
particle bytes and the resulting RNG seed match. The prior live birth,
controlled shape 0–5, hard-terrain and plasma birth tests also pass.

Admission alone exposed a second live failure: Deltatap prepared and started
all nine attachment graphs but emitted zero particles. The original object
attachment branch at `C0E2E` calls `179540`, which supplies `1791C0`'s explicit
`gravity` and `up` seed packet at `BBE00`'s world object origin. These seeds
take priority over model markers. The native bridge had omitted them; the
actual `null_up` render model contains a `marker` group rather than an `up`
group. Substituting an arbitrary missing name with root would be incorrect.

`source_effect_seed_location` implements whole `17B580` / retail `1786F0`
arithmetic, including its complete `11F9C0` perpendicular-vector helper.
**2,048 original seed matrices match byte for byte** across null, zero-scale,
unit-scale and scaled node inputs. The zero-scale case initially caught a
signed-zero difference and now retains the original positive-zero position.
No instruction or service is replaced in either original helper comparison.
Completed node matrices and object world origins are declared inputs; the
complete attachment constructor and `BBE00` producer are outside that oracle.

The native scene producers now supply the object origin separately from
animated node zero. The bridge constructs the two local seed locations once
per full object generation and composes them with current nodes on updates.
After restoring these inputs and admitting the verified shape through creation,
the 21-map native bank prepares **69 of 96** graphs, starts **66** initial
attachments and observes summed particle peaks of **2,716**. Warlock prepares
and starts all 12 attachments with a 492-particle peak; Deltatap prepares and
starts all nine with a 203-particle peak. The 180-tick check covers repeated
views, immediate retirement and salted recreation with declared bind WORLD
nodes/origins. Full original eligibility, scheduling and network history remain
open. The prior whole particle-creation fixture also passes; no new whole
shape-9 allocation/creation oracle is claimed.

The [fixture instructions](../tests/fixtures/tu15_map_particle_shape9.md) pin
the owned TU1.5 executable and describe the declared inputs. Unicorn executes
the complete original initializer and its actual helpers without instruction
patches or substituted services. Its existing RSQRTSS result policy remains
explicit; these comparisons do not establish physical Xbox estimate bits,
complete value-cache production, allocation/emission timing or Xbox pixels.

`HALO2_REFERENCE_PARTICLE_SHAPE9_HIDE` is a capture diagnostic. It hides only
the final native draw submissions for particles belonging to shape 9 emitters.
All property evaluation, simulation, particle lifetimes and RNG still run.
It is unset for ordinary gameplay. The camera-only capture override also
preserves the automatic source world; no root/pose override is needed.

Evidence and the coherent source/build are retained under
`analysis/map-world-features/attachment-continuation-20261014/`.
Both normal launchers now select the tested `.local/map-world-shape9-20261014`
package. Its exact hash passes 21 ordinary rendered map cases and four
60-tick split-screen cases. Warlock's stream changes 3,572 pixels when only
shape 9 draws are hidden; a close Deltatap waterfall-volume diagnostic changes
123,784. Enabled repeats and captured simulation/RNG lines are identical.
These camera-only views preserve the automatic source world, but do not prove
ordinary-view Deltatap visibility or Xbox scene parity. Ordinary Lockout snow
also retains a 20,028-pixel difference with an identical enabled repeat.
All captures have zero Vulkan validation errors. The normal launchers' separate
MCC gameplay readiness gate still reports unavailable.
The full 23-map effects/interactions goal remains active. Named functions,
swarm/wind controllers, external bindings, nonvisual parts, full original
scheduling/resource/RNG ownership, network history and Vista resource owners
remain separate work.
