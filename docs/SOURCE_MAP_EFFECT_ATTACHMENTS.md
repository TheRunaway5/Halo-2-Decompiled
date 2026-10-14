# Authored map effect attachments

`SourceMapEffects` imports the actual `effe` attachment rows on scenery,
machines and props, including resources for script-created placements. The
ordinary multiplayer runtime now feeds it the full salted object handle,
creation tick, selected permutations and current WORLD nodes from the same
scene producers used by geometry. It resolves authored markers, starts each
attachment once per lifetime, updates moving frames and retires the attached
systems when the owner disappears. Repeated view preparation does not restart
the effect. Clients wait for an accepted host snapshot and apply device state
and prop tombstones before this synchronization.

The original absent-function inputs are A=0 and B=1. Four salted original
allocator/attachment cases establish those values; the object lookup and
eligibility services are explicit fixture inputs. Named functions remain
unavailable. Particle death-effect definitions are recursively checked before
an attachment is admitted, including cyclic graphs. Missing child systems do
not silently turn a partial graph into a prepared attachment.

The Xbox 21-map cache bank contains 96 relevant authored attachments. The
current native test prepares 58 and starts 55 initial attachments. It checks
180 simulation ticks, duplicate synchronization, repeated views, immediate
retirement and salted recreation, using declared bind WORLD inputs. The summed
particle peaks are 1,994. Foundation's four actual HVAC attachments produce a
197-particle peak. This test does not establish current original animation,
spatial/PVS eligibility, factory scheduling or network history.

The retained Foundation on/off/repeat Vulkan captures show 38,829 changed
pixels and byte-identical enabled repeats. They use declared native root and
camera diagnostic inputs, which disable the automatic original world. Ordinary
Foundation separately passes one/four-player startup and rendering through
that world. Ordinary Lockout also passes one/four-player rendering; its
60-tick snow on/off/repeat captures show 20,076 changed pixels with an identical
enabled repeat and no root, camera or replay override. Capture receipts pin
the runtime, original executable and actual shader files.

Evidence is under `analysis/map-world-features/attachments-20261013/latest/`:
`all-map-attachments.log`, `foundation-live/verification.json`,
`snow-live/verification.json`, `ordinary/`, and immutable `versions/` snapshots.
Earlier captures in the parent directory remain separate older-build evidence.

## Original routines and delivery boundary

`source_map_world_support_tu15_generated.cpp` retains complete original TU1.5
bodies reached during startup and updates. Original function pointers or direct
call instructions authenticate their registration. The new input storage,
static defaults, CRC and projection-shape work is compiled native C++; production
does not decode or execute original opcodes. The x87 FPTAN lowering executes
the actual hardware instruction under the retained control word and preserves
its successful extra stack value and range-failure behavior.

The independent original banks compare 768 device/machine cold callback cases,
80 input-storage/static-default cases with real CRC callees, 64 complete
projection-shape cases under both `0x27F` and `0x37F` control words, and 64 complete
signed vector-packing cases including unordered inputs. The vector routine
clamps to `[-1,1]` and packs 11/11/10 bits using literal scales 1023.5/511.5.
They retain full banks, general/SIMD registers, flags,
instructions and writes. These cases do not validate every newly reached
routine. The cold reset bank does not execute active-controller sampling.
See `analysis/map-world-features/map-world-support-20261013/`.

The staged runtime preserves the published death/spectating runtime as its
baseline: its copied original link inputs reproduce every allocated ELF
section before the map changes. The support dispatcher preserves the original
support object's allocated sections and delegates its existing entries.
It also includes the current original device-constructor module and corrected
BC200 velocity caller; the published caller incorrectly passed register
arguments on the stack. Source/object/runtime hashes record the combination.
The original decal constructor at startup row `44B404` now precedes the effect
storage constructor at `44B44C`. It constructs the real decal pool and `3C2C`
global bank; an earlier update correctly failed on the absent bank. Production
calls it directly. The isolated published-baseline build inserts that same
constructor before its retained effect-storage entry, verifies both literal
registration words, preserves the caller ABI and requires actual published
pool/global pointers. This establishes that dependency, not whole original
startup ordering. Its original `180170` new-map reset now precedes `17E700`,
matching rows `44B40C` and `44B454`. The actual reset marks the pool active and
sets the cluster heads to NONE; those values are not host-supplied defaults.
The real Zanzibar preparation regression checks this state before rendering.

The current module contains **82 complete bodies / 8,338 instructions**.
The independent 976-case banks above cover their stated routines, not every
body. New retained routines include the decal constructor/reset, projection,
lighting helpers, and registered cache release/retention callbacks.

Zanzibar's original decal update reaches
an absent original D3D driver command owner. `439C0` reads the driver pointer
at `411568`, reserves command words, writes `0x41710` and zero, and grows the
command space through `404BC0` in the executable's `D3D` section. The current
failure is the read at `439F4`. This needs actual retained graphics ownership
and native renderer consumption. A dummy driver bank or an omitted update
would not establish acceptance. A separate CPU capture with declared,
unsubmitted command-recording input and validated native sector I/O completes
one original Zanzibar player tick. It produces 28 records and 32 clipped quads,
all in lit layer1. The new read-only cache decoder retains full salts, ranges,
packed UV/brightness and original world vertices. Its fixture checks stale
cache identities and oversized ranges. This does not close the graphics owner.
See [the cache implementation and render boundary](SOURCE_DECAL_VERTEX_CACHE_TU15.md).

The 21-map first-tick comparison against the installed death/spectating
baseline has no regressions and six improvements: Ascension, Colossus,
Foundation, Waterworks, Containment and Backwash. Ten maps complete that bounded
CPU check; eleven still fail on missing graphics, physical memory or other
source owners. Lockout/Foundation ordinary one/four-view rendering and the
976 original comparisons pass on the clean continuation runtime. Launcher
packaging and its exact capture receipts are recorded separately in
[normal launcher delivery](SOURCE_WORLD_LAUNCHER_DELIVERY.md). Historical
gate/traversal receipts do not accept this automatic-original-world build.

The full goal remains open: 38 authored attachments are not prepared; named
functions, swarm/wind controllers, shape9, external material bindings,
nonvisual parts, full original scheduling/RNG/resource ownership, late-join
particle history, resets, moving/crushing machinery, the Vista-only texture
guards and matched original visual acceptance are unresolved. Current scene
poses and repeated-view invariance are bounded native evidence, not full Xbox
parity.
