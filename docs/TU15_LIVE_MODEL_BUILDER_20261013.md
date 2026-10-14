# TU1.5 live model and light receiver checkpoint

Authority: owned OG Xbox TU1.5 executable SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`
and original Xbox cache assets. Evidence is in
`analysis/shader-parity-20261013`. The full shader parity goal remains active.

`PhysicsLiveWorldTu15` now loads model sections through original1717B0/130980,
real native cache-sector I/O and source pointer publication. Its model builder
uses the same retained world, object cache, deferred pose and lighting sample
owners. Master Chief exercises20 authored sections,80 resources and28,206
exact resource bytes. Stale tag/object handles are rejected.

The original render-context callback is version-specific: retailBE8B0,
TU1.1 BF7A0 and TU1.5 C08E0. The earlier shared cache initializer retained the
retail callback. The current adapter supplies the original image's callback;
existing retail cache/builder regressions pass.

The four declared model calls use one genuinely created biped, views0..3 and
one/two/three/four-player LOD policies. Their requested metrics10/0/30/1 select
source LOD1/3/0/2. Each original comparison starts from native **input** banks.
The original executable runs unchanged on the host i386 CPU; native output is
read only after original execution. No instruction or callee result is replaced.

| View / local players | Original instructions | Exact retained bytes | Artifact |
| --- | ---: | ---: | --- |
| 0 / 1 | 14,969 | 58,966,102 | `live-model-hardware-view0-v2/comparison.json` |
| 1 / 2 | 16,783 | 58,970,198 | `live-model-hardware-view1-v1/comparison.json` |
| 2 / 3 | 6,657 | 58,974,294 | `live-model-hardware-view2-v1/comparison.json` |
| 3 / 4 | 6,837 | 58,978,390 | `live-model-hardware-view3-v1/comparison.json` |

The oracle supplies a declared diagnostic D3D target only for unused multisample
metadata. Caller stack and native temporary scratch are excluded; all retained
model/cache/resource outputs are compared. This is original CPU-path evidence,
not Xbox hardware or framebuffer proof. Unicorn's view1 run differs in257
bytes around reciprocal estimates; unchanged host hardware matches. Production
arithmetic was not changed to match the emulator. The first hardware view0
attempt rejected a nullable PREFETCHT0 pointer as a real memory read. Metadata
now excludes address-only cache hints from required input-byte ranges, while
the original instruction still executes unchanged.

`source_object_admission` now implements the original light-query branch:
object+50/+5C lighting bounds, context+2A78, sphere-mode SSE overlap/x87
containment, full-handle auxiliary main-queue lookup, original body-flag16
suppression, fog/plane flags, association masks and source queue ordering.
`source_object_admission_memory` reads the actual current object and spatial
data arrays. It does not reconstruct registration order from scene objects.

The unchanged TU1.5 C35D0/31C70 caller supplies actual snapshot banks for
whole134D30. The original case plus explicitly varied sphere/flag/auxiliary
inputs produce337 queries,1,650 records and8,622 exact output words over51
actual objects. Both the direct pool projection and production memory adapter
match. The fixture includes complete consumed raw spatial/object inputs.
A fresh native world also finds two genuinely created bipeds through their
actual registration lists. Four existing admission/resource/builder regression
families pass; details are in`admission-regressions-v2.log`.

The source freeze adopts pre-existing native cache I/O, original render-pool
construction/reset and the CPU bitmap-view helper from concurrent work. These
are dependencies, not newly authored substitutes or shader fixes. The frozen
tree remains separate from the current combined launcher delivery.

The native live collector now joins original render-entry, presentation and
resource owners, producing eight draw units from two genuinely created bipeds.
The shared BSP/model light owner uses current native object admission, draw
units, palettes, commands, opaque selection and indexed ranges without an
intermediate expected-result fixture. Its controlled two-biped query produces
10 raw model packets / 1,896 indices with two distinct full object owners.

`live-collection-v3/collect/original-commands.json` compares the unchanged
TU1.5 `40E80` and `3BAA0` against these native outputs. All three command banks,
ordered calls, selected opaque records and 58,994,774 retained bytes agree;
14,820 original instructions / 1,429 distinct instructions were authenticated.
Eight unchanged `4D820` / `15280` executions reach their actual `401410` upload
boundary with exactly the native c50 words. No instruction or callee result is
substituted. The declared caller inputs and excluded temporary ranges are
recorded by the oracle. This is CPU-path evidence, not an original image.

This test exposed and fixed an actual geometry-header error: original `44930`
returns mode section+4. Reading section+0 incorrectly classified skinned
sections as rigid in opaque selection. The original opaque selector now
confirms the corrected class. Seven focused receiver/range/cull regressions
pass (`live-light-regressions-v1.log`).

Attachment queue traversal uses the compiled original `41C70` / `41C10` /
`D7230` / `41970` entries. The current fresh-biped cases consume empty
attachment lists; nonempty antenna/cloth services and deep attachment context
remain open. The partial-sphere cull adapter also still requires a live case.

The controlled Vulkan capture `live-light-gpu-v3` now shows visible grenade
illumination on a freshly created Chief using current native pose, cache
lighting and raw model packets:5 packets /33,022 changed RGB channels at960x720.
The original depth EQUAL contract is retained and validation reports no errors.
`comparison-crop.png` is a labeled native diagnostic, not a matched Xbox frame.

Six genuine smaller-sphere queries and three explicitly declared subpart-cull
requests agree with unchanged44710/45550 (`live-collection-v5/collect/
original-cull.json`). All58,994,774 retained bytes per case also agree. However,
the actual cold Chief has no mode+74/+7C attribution, and its selected subparts
are unconditional. The24 declared subpart calls perform zeroBAC30 node reads.
The oracle therefore rejects its required posed-bound coverage gate; this
cannot establish visible partial-body culling. No production flags or bounds
were changed to manufacture that coverage.

Still required: join the shared owner into
normal gameplay with matching base-pass pose/geometry, validate owner/palette
lifetimes, and obtain matched original model-light frames. The current complete
28-draw grenade segment remains at164 differing pixels, so strict pixel parity
is still open. No invisible draw or passing CPU fixture closes that gate.
