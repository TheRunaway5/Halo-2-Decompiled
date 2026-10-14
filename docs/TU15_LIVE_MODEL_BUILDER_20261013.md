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

Still required: finish live render-entry/presentation/draw collection and
shared BSP/model light commands; validate current owner/palette lifetimes;
obtain a visibly active matched original model-light case. The current complete
28-draw grenade segment remains at164 differing pixels, so strict pixel parity
is still open. No invisible draw or passing CPU fixture closes that gate.
