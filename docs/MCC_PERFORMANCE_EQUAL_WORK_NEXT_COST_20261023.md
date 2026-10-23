# Equal-work CPU costs and next owners — 2026-10-23

Read-only source assessment of [gpu-fixed-cohort.json](../build/test-results/presentation-offload-20261022/virtual-ordinary-runtime/gpu-fixed-cohort.json), SHA256 `9df993425a43232fa50fe5170e93fa2322ba3d65dfbd8cef0ef6e45ddc11c2e3`. The profile helper is pinned to `c849b1f9650228d29f853e3180f9bd9b18757903171a4640281c0e347f82afdf`; its sibling native runtime is `b1b6a50f77ed2e45d4a1cfd56d73871dafe11da54d18d6b206df53d836bb04e6`. These measurements precede the canonical planning/fallback edits below; current source pointers identify owner boundaries, not an assertion of exact profile/source identity.

The report passes its declared equal-command/RNG/sampled-state gates and detects no concurrent repository build interference. It runs60 real commands and submitted frames after60 preparation ticks, excludes10 warmup frames and the final capture frame, and averages49 frames. Its scope explicitly excludes ordinary FPS acceptance, physical presentation latency and complete gameplay/event parity; frame equality is qualified separately. Both source RNG words match within each CPU/GPU pair. GPU work is real:620,640 deformed vertices for1p and2,883,156 for4p, with60 GPU dispatch frames each.

Mean milliseconds below are **wall durations for named owner phases**, except the final row, which is measured owner-thread CPU. Camera/objects/first-person values sum the actual local views. Child worker CPU is not included in that owner-thread total.

| Phase | 1p CPU | 1p GPU | 4p CPU | 4p GPU |
| --- | ---: | ---: | ---: | ---: |
| Input/simulation | 7.446 | 7.672 | 15.379 | 15.897 |
| Camera/lighting owner interval | 5.445 | 5.412 | 20.063 | 20.447 |
| Objects | 1.475 | 1.510 | 5.335 | 5.444 |
| First-person | 0.455 | 0.567 | 1.708 | 2.277 |
| Renderer global-constants interval | 0.874 | 0.970 | 6.095 | 5.850 |
| Renderer geometry planning | 0.093 | 1.273 | 0.820 | 11.549 |
| Renderer geometry binding | 0.321 | 0.370 | 1.614 | 1.713 |
| Complete timed wall interval | 16.900 | 18.775 | 56.201 | 70.032 |
| Complete timed owner-thread CPU | 16.839 | 18.711 | 55.860 | 69.743 |

Planning increases by1.180ms/10.730ms while the complete wall interval increases by1.875ms/13.831ms. That arithmetic locates most of this paired regression in the planning interval; it does **not** assign all of it to reserve, map copying, validation or any individual leaf. One ordered CPU/GPU pair per player count is not a repeated throughput estimate. Small fence waits (0.002–0.005ms) and command recording (0.192–0.786ms) also do not support prioritizing a command-recording thread over the larger CPU owners.

Two measurement distinctions matter:

* The fixed cohort forces one simulation tick per render iteration. Ordinary high-refresh rendering can execute zero or one30Hz tick depending on its accumulator; the7–16ms input interval here is a per-command diagnostic cost, not an unavoidable cost in every120Hz render. [cadence](../src/gameplay/multiplayer/multiplayer_runtime.cpp:3215), [tick delivery](../src/gameplay/multiplayer/multiplayer_runtime.cpp:3702)
* Timed source work includes finite-capture diagnostics. Capture-only pStencil point lifetime formatting/output occurs at [2121](../src/gameplay/multiplayer/multiplayer_runtime.cpp:2121); finite shadow selection at [4921](../src/gameplay/multiplayer/multiplayer_runtime.cpp:4921), owner summaries at [5092](../src/gameplay/multiplayer/multiplayer_runtime.cpp:5092), and particle/lens summaries at [5157](../src/gameplay/multiplayer/multiplayer_runtime.cpp:5157). The retained logs confirm those paths execute. Their cost has not been isolated, so these intervals cannot be called pure ordinary gameplay costs.

The names cover these actual owners:

* **Input:** event/controller handling, authoritative `live_match().step(controls)`, source snapshot/camera completion, audio/effect delivery, scene/body pose capture and sensors, ending at `measure(input_ms)`. This is much more than reading buttons. Tick writers, RNG consumption, allocator/device callbacks and ordered observers remain on their current owner. [step/tail](../src/gameplay/multiplayer/multiplayer_runtime.cpp:3801), [phase end](../src/gameplay/multiplayer/multiplayer_runtime.cpp:3945)
* **Camera/lighting:** per-view camera/projection/fog and visibility; source model admission/collection; original main/shadow `44710` work; completed node/section extraction; source particle and lens production; object-light cache queries and bindings, ending at5252. It cannot be parallelized as a single camera function. `render_model_view` enters the same TU1.5 world, while model building writes shared scratch/cache/arena state and invokes PRT/color owners. [main source call](../src/gameplay/multiplayer/multiplayer_runtime.cpp:4860), [particle/lens source calls](../src/gameplay/multiplayer/multiplayer_runtime.cpp:5143), [world entry](../src/gameplay/multiplayer/multiplayer_object_runtime.cpp:1502), [mutable model builder](../src/rendering/models/source_model_build.cpp:43)
* **Objects:** source-selected vehicle/PRT sections, already reused per-owner skins, machine/prop/scenery/dynamic/projectile packets, world-weapon shader/lighting owners, then5641. Several of these legitimately consume posed CPU vertices. Existing frame-pose reuse must not be counted as a new optimization. [vehicle path](../src/gameplay/multiplayer/multiplayer_runtime.cpp:5367), [other owners](../src/gameplay/multiplayer/multiplayer_runtime.cpp:5500), [phase end](../src/gameplay/multiplayer/multiplayer_runtime.cpp:5641)
* **Global constants:** the interval after the frame fence through5189 also contains camouflage/lens resource preparation, particle/decal/contrail/weather preparation, post-effect updates and lazy resources. It is not just a UBO memcpy. `prepare_menu_world` exits for ordinary full-surface gameplay, so its title-only per-frame raw descriptors/UBOs are not established as this cohort's6ms cause. [phase start](../src/rendering/renderer/renderer.cpp:4625), [particles](../src/rendering/renderer/renderer.cpp:4763), [phase end](../src/rendering/renderer/renderer.cpp:5189), [title exclusion](../src/rendering/renderer/renderer.cpp:3733)

The strongest immediate improvement was a concrete planning algorithm issue, now fixed canonically by the coordinator, **with new runtime measurement pending**: per-vertex pending/set bookkeeping and repeated exact `records_.reserve(size+append)` were replaced by destination runs, interval occupancy and geometric growth; repeated immutable geometry checks now share identity. This audit additionally found `add_bank` cloned the full palette map even for empty submissions, and source/palette vectors still reserved each exact new extent. Canonical code now returns for empty submissions **after** late-registration/address rejection, and grows source/palette capacity geometrically with width guards. The existing sorted destination order, disjoint geometry owners and source ordinals remain required. The coordinator reports a red capacity-growth symptom test; no isolated millisecond saving is assigned to these individual fixes. [current plan](../src/rendering/renderer/frame_model_skin_plan.hpp:98)

Remaining bounded directions, in priority order:

1. Re-run the same declared workload after the planning fix, then repeat ordinary-clock unsampled FPS/quality gates. If planning remains material, examine the remaining palette-map clone per **nonempty** bank and provenance scans with actual counts first. Do not remove complete source/binding/numeric validation or change draw order to gain time.
2. Resolve leaf costs inside input and camera/model/shadow/particle owners before another large rewrite. MCC's observed native direct data accesses are a concrete architectural contrast to checked translated execution; they do not authorize replacing a source algorithm or skipping faults, observers, mutation or cursor effects. The current phases establish owner priority, not which translated leaf is now dominant.
3. Split renderer global preparation into its real service owners before claiming uniform-buffer or descriptor bottlenecks. A narrow actual repeated computation exists: `ParticleRenderer::program` decodes the vertex program, `material()` hashes its bytes even on a cache hit, then `program()` hashes them again; prepared-view validation requests this per particle. Passing the already validated hash through the cache-hit path could remove the second integer scan without dropping a rejection gate. Larger decode/metadata caching needs explicit immutable material/program/renderer lifetime and failed-initialization proof; pointer/size or hash alone is not full byte identity. This is an unmeasured small candidate, **not an explanation of the6ms interval**. [double hash](../src/rendering/effects/particle_renderer.cpp:235), [program metadata](../src/rendering/effects/particle_renderer.cpp:266), [per-particle request](../src/rendering/renderer/renderer.cpp:4828)
4. For a second core, retain source admission, node/IK/history/lighting/shader callbacks and cache publication on the original owner. Only completed immutable presentation/packing jobs or the final CPU-only skin kernel with disjoint owned output can be candidates. Join before the original selected-section/shader/overlay consumer; preserve exact SSE graph, FP/errno admission, all-zero behavior, owner/pose/LOD identity, errors and lifetimes. The current byte-copy worker and ordinary GPU skins already exist. Moving source visibility/shadow writers or whole views concurrently is not admitted by these timings.

The other exact `reserve(size+1)` hits found in the bounded renderer search are CPU bitmap backing's sparse-row/frame staging and lens material construction. Projection invokes backing stage only when `(bitmap,frame)` is newly admitted; warm known frames refresh through the existing owner/observer path. These are not established as repeated hot per-frame growth. `ParticleRenderer::reserve_draws` already doubles its retained uniform/descriptor arena; proposing that reuse again would duplicate existing work. [bitmap cold staging](../src/rendering/mcc_cpu_bitmap_projection.cpp:178), [backing reserves](../src/rendering/mcc_cpu_bitmap_backing.cpp:165), [particle arena reuse](../src/rendering/effects/particle_renderer.cpp:285)

The pinned [MCC audit](MCC_CLASSIC_PERFORMANCE_REFERENCE_20261022.md) still establishes AMD64 native object loads, retained immutable/default buffers/upload rotation, binding-state suppression, indexed instancing and deferred-command machinery in `halo2.dll`1.3528.0.0, SHA256 `de65b4f4fdbf3f0a5eab7431fe530da17dd815599182dfd6ae9b7e21cf171946`. It does not establish MCC's active worker schedule, ordinary skin placement, per-pass costs or a matched4K120 comparison. This diagnostic's GPU-on regression is not GPU-default acceptance, and a canonical planning fix is not measured performance acceptance.

Current static source pins (SHA256):

```text
95e9bbf0bbc732cf58c85ac7a7e500dc39d3a6c6cfc697b871dab7898de489fe  src/rendering/renderer/frame_model_skin_plan.hpp
a5bb26cbd6a82e7c78feeb632a788e3a207c0fd63f23feec93410ee8a4668b28  src/gameplay/multiplayer/multiplayer_runtime.cpp
8d747c02d3331baa7abb302af88776e7e646020865aa0f7c016e88a67d66008c  src/rendering/renderer/renderer.cpp
2c5cbe99d9fb1580fdf7f1541eaaa56cc6302808e3c267ba88bec2307dd24ca7  src/rendering/effects/particle_renderer.cpp
2a4914aac5e2607f5c41d2d0cc4674a6c80a0333fc7b061a1fab7ed3c137448a  src/ui/menu_world_native_draw.cpp
db219652afdcdade1868594264d7a29ea487e48ec4c1df1494b0d10c3d90b781  src/rendering/mcc_cpu_bitmap_backing.cpp
27ab015f5f71e0b77ea6888945aec54e5618aec83d05ec0d2013ea35a131a165  build/test-results/mcc-classic-performance-reference-20261022/binary-evidence.json
```

Only this note was written by this audit. No compiler, build, test, probe, runtime, frozen refresh or generated scratch artifact was started.

## Subsequent measured candidate

The optimized plan and typed resource fallback now build and pass the twelve
normal actual4K CPU/GPU frame/readback/source/RNG pairs, plus four separately
classified forced CPU fallback controls. The following results supersede the
pending measurement status above. They do not replace its historical source pins.

| Measured current candidate | 1p CPU | 1p GPU | 4p CPU | 4p GPU |
| --- | ---: | ---: | ---: | ---: |
| Median ordinary FPS, three interleaved pairs | 71.109 | 65.319 | 19.287 | 17.828 |
| Equal-work iteration owner CPU, ms | 17.068 | 17.970 | 55.235 | 59.287 |
| Equal-work geometry planning, ms | 0.088 | 0.317 | 0.793 | 2.082 |
| Equal-work total geometry packing, ms | 0.410 | 0.731 | 2.364 | 3.937 |

GPU skinning regresses ordinary FPS by8.14%/7.56% at the median; it remains
opt-in. Both120/60 targets are missed. All measured samples have actual4K
render/present extents, the RTX5080, separate-thread upload work, zero CPU
resource fallback, zero verification readback, unchanged source/shader/binary
pins and no detected repository compiler interference. Equal-work CPU and FPS
remain distinct measurements with the diagnostic limits described above.
See [compact performance summary](../build/test-results/presentation-offload-20261022/virtual-resource-runtime/compact-performance-summary.json).

The next experiment summarizes each palette row once in the exact integer
admission predicate, comparing against the complete source-pinned old predicate;
it does not widen the numeric domain. Actual CPU stack collection on the
unchanged measured candidate is underway to distinguish admission, provenance,
allocation/copy and driver costs before choosing further changes. The first
gprofng compatibility pilot exited with SIGSEGV before any native output; its
cause is unresolved and provides no evidence of a GPU implementation fault.
Its raw experiment owner was removed; the existing unprivileged CPU-clock
sampler is the bounded fallback.
