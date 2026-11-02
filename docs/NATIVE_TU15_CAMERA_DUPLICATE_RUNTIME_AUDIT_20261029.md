# Remaining native camera helper copies

This is a read-only inventory, not new runtime qualification. The current
shadow module contains only two lifted copies from the qualified packet,
BSP, rectangle, layout, projection, default and policy groups: `2C470` and
`144090`. It has149 local bodies. None of `2F7E0`, `2B700`, `14CDB0`, `2F950`,
`2F5E0`, `2F620`, `2F690`, `2FB50`, `2FD70`, `380450` or `25940` is local there.
The exact file/body hashes and primary calls are retained in
`analysis/campaign-native-new-game-camera-20261028/shadow-camera-duplicate-primary.json`
and `camera-helper-global-copy-inventory-v2.json`. The shadow file SHA is
`c16e116e8d9d979eda638e363b0e6731de432bfa99834417eb614b1ba294750e`.

| Shadow copy | Runtime/local callers | What remains before retirement |
| --- | --- | --- |
| `2C470`,35 sites, `source_shadow_frame_tu15_generated.cpp:1006` | No local call; it remains public through driver:19705 and owns-entry:19847. The actual producer caller `229B9E` has separately been retired to the qualified native policy. | Separate the direct legacy diagnostic contract from production ownership; do not recreate dead registers or instruction counts in the native policy. |
| `144090`,82 sites, same file:15046 | One local call: `C3DC0:C4879`, real return `C487E`, inline at:10432. This is not the qualified projection caller `2FFE9`. | Qualify this caller/continuation and a narrow transport, then replace the real call with a required-owner bridge and remove the local copy. |

At `C4879`, EAX is `record+4` and ECX is `record+38`: adjacent, disjoint
52-byte destination/source frames. After return the source reloads EAX at
`C487E`, ECX before `166C90`, and EDX after that call. `166C90` overwrites EDX
before its normal-path use, but its early rejection and the next `C3DC0` loop
iteration still need explicit volatile/SIMD liveness coverage. The qualified
typed inverse algorithm is reusable; the existing numeric binder admits only
the isolated `2FFE9` inverse transport and deliberately does not own all
`144090` calls. These are new caller gates, not a missing affine formula.
Sources: shadow:10425–10439; `native_tu15_camera_projection_binding.cpp:16–29`;
the unchanged primary CALL bytes `E8 12 F8 07 00` and continuation are in
`shadow-camera-duplicate-primary.json`.

The shadow entry fixes its image to TU1.5 at:19688; its1420 selected-address
entries are identities. The generic `MapPack11` branch in the selected helper
does not make this public API image-polymorphic. Original authority is the
TU1.5 image SHA `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`;
the generator checks that image and builds its correspondence from those
bytes (`tools/generate_shadow_frame_tu15.py:145–196`). No regeneration is
needed or proposed.

The optional main-view reference test is a material compatibility constraint.
It directly invokes this shadow `2C470` entry, comparing full returned GP,
defined flags, exact instruction count, ordered stores and postimages for12
cases (`tests/native_tu15_main_view_data_tests.cpp:31–36`). Its three main
prefix cases replay original store extents to check backing only; they are
not a second projection algorithm (:39–44). The reference path defaults to
an empty optional fixture with skip77; the cold storage test remains separate
(`cmake/PhysicsLiveWorldTu15.cmake:1121–1127`). A semantic native policy cannot
truthfully satisfy dead-CPU/instruction-count assertions by inventing them.

To remove this production copy while preserving every original fixture and
check, first move its exact historical35-site diagnostic body to a test-only
frozen helper, as the existing shadow prefix reference does. Keep the full
GP/flags/count assertions on that explicitly diagnostic path, and run the
native typed policy over independently restored inputs to compare actual
policy stores/postimages. Then remove the production body, declaration,
dispatch/public admission and generator reintroduction path. Do not silently
skip the12 cases or make the new native kernel emit fake instruction events.
The existing test-only precedent is
`tests/helpers/source_shadow_prefix_2e6810_reference_20261023.hpp:1–9`.

The wider inventory finds nine remaining implementation copies of these
named helpers, across seven generated files, after the producer's narrow
policy retirement to17 bodies/14 public entries:

| Helper | Remaining copies and local real caller sites |
| --- | --- |
| `2C470` | Shadow only; public diagnostic entry, no local call. |
| `25940` | Texture producer public copy, no local call; particle-target copy at `14D67`, `14D9A`. |
| `144090` | Shadow `C4879`; map-world `1E088B`; AI-frame `16C06F`; dual-image physics canonical `142D00`, TU1.5 sites `22EBFD`, `22ED07`. |
| `14CDB0` | TU1.5 live-support at11 sites and AI-frame at6 sites, with4 overlapping original sites. These callers are outside the camera selector's numeric admission. |

The three fixed-TU1.5 inverse bodies have identical body SHA
`69a1ac07734132b1faa22fc1905209d8dafd12de15e2258f02bae67f38e0ee64`.
All exact file/body hashes, source lines and call lists are in
`camera-helper-global-copy-inventory-v2.json`; the shared physics alias and
callers are in `camera-inverse-dual-image-copy.json`. The native BSP leaf
already exposes tree/point/node typed arguments, but its numeric binding
owns only `2B700` at `2B7E6` (`native_tu15_camera_bsp_selection_binding.hpp:4–9`).
Do not treat those17 residual local calls as qualified native transports.

The particle-target `25940` copy is a retained-row parity candidate, not an
immediately safe native GPU route. Its two local callers match the sealed
`14D67/14D9A` ABI profiles, and `198A8` is also an authenticated query caller.
That establishes the original row-selection algorithm and caller transport;
it does not reconcile those rows with current host targets18/20. Root keeps
`25940` unrouted until selection, dimensions, attachment and frame-lease
authority agree. See
`analysis/campaign-native-new-game-20261028/frame-policy/target-ownership-plan.json`,
especially `component_scope`, `callers`, `original_resource_lifecycle`,
`native_owners` and `required_proof`.

The `14BA0` consumer also needs actual attachment resolution and surface
dimensions. The extern18 path through `198A8` reaches `14F40`/`1DD40`, whose
logical dimensions still come from source records. Resource allocation
through `1DC20` and the camera's18 lifecycle do not by themselves reconcile
the separate `13B70` screen-history18 owner. The bounded original direct
creation graph has not resolved target20's trigger; an explicit host GPU20
control is not proof of original campaign activation. The original dimensions,
prepared attachment, produced content, retirement and resource epoch must
share one genuine authority before any live query can claim readiness.

The actual Renderer-owned `258F7` capture path must remain first-owned: it
checks a current target capability, genuine call/RET frame and completed scene
request (`source_texture_camera_frame_capture_tu15.cpp:40–59`). A future
particle retirement must preserve that path, compose the genuine authority in
the standalone83-case harness, and retain its SDK stop boundaries and checks
(`tests/source_particle_target_tu15_tests.cpp:200–279`). Its generator should
authenticate both calls, not just `14D9A`
(`tools/generate_particle_target_tu15.py:13–18`). Raw retained-row parity alone
does not authorize an early World route or supply a GPU result.

The physics `142D00` copy must retain its MapPack contract. Its public runtime
takes an explicit image and offers canonical versus selected observation PCs
(`physics_world_runtime_generated.cpp:9–17,38176–38179,38668–38677`). TU1.5
`144090` is reached locally at selected `22EBFD/22ED07`, while canonical
observations use `22A2FD/22A407`. Any TU1.5 retirement must bind image and caller
before translating that boundary; do not globally replace the canonical body
or reinterpret MapPack memory as TU1.5. Existing generic physics tests compare
complete GP/XMM/flags/trace/store/FP state for the selected fixture image
(`tests/physics_world_runtime_tests.cpp:68–100`). Keep their original authority
and diagnostic transport separate from native gameplay admission.

Direct object/vehicle-motion reference glue also calls `run_actual(144090)`
and consumes its matrix (`tests/native_tu15_object_motion_tu15_tests.cpp:252–257`,
`native_tu15_vehicle_motion_tu15_tests.cpp:484–492`). Migrate those matrix checks
to the independently physically qualified typed inverse before withdrawing
the public legacy diagnostic route; they do not justify fabricating volatile
outputs. The146 shadow admission cases cover other entry gates and the10
native shadow-query cases cover `2E79A0`; neither fixture reaches `C4879` or
`144090`. No tests, binaries, generated source or original fixtures were changed
or executed for this inventory. Full shadow-frame native closure remains open.

The next concrete independent qualification is `C4879` against the already
qualified native inverse core. A proposed `SourceShadowProfileDirect` must be
bound to the genuine TU1.5 shadow caller, source memory owner, actual call site
and ordinary DF=0/masked-FP profile; it is not an existing general capability.
Recover actual unmodified original caller frames and both `166C90` continuation
branches, including the next `C3DC0` iteration, before deciding which volatile
outputs are dead. Verify the adjacent52-byte frame layout, persistent bytes,
ordered reads/stores, MXCSR at all relevant fault cuts, preserved older x87
values/nonvolatile state, and real end-of-call RET consumption. Do not read
or validate RET early in a way that moves its failure before completed stores.

Keep that proof separate from the original `2FFE9` profile and all MapPack
`142D00` diagnostics. Only after the new profile passes should a narrow native
binding admit `144090` at `C4879`, the one shadow call become an authenticated
required-owner bridge, and the shadow copy be retired. Unknown callers retain
an explicit required-owner boundary; no whole `C3DC0`, `166C90`, shared inverse
or shadow-frame completion follows from this bounded step.
