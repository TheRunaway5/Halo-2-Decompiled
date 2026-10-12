# Model shader detail producer

`SourceModelShaderView` and the two pure helpers in `source_model_shader.hpp`
connect the original cached owner sphere to the per-part shader selector. They
do not choose geometry, synthesize an object owner, initialize source objects,
or substitute native projection/bounds estimates. The same arithmetic serves
SCEN, MACH, BLOC, body and first-person collectors when their actual caller
inputs are supplied.

## Original path

`133790` reads the collector owner's cached object sphere at `+30 / +3C`.
It computes the coverage from the actual inverse-camera Z row
`4B9E20 / 2C / 38 / 44`, viewport scalar `4B9ED0`, and `4BA028`. These are
separate from the normalized distance used to choose geometry detail.
`133F70` forwards the literal whole-collector `+1B4` word to `2CBF0` entry
`ESP+28`. The quantizer stores the resulting coverage byte in parallel bank
`4C0B84`; the packed command's top byte is a different input.

`133AB0` copies the parent collector's entire `1E4`-byte bank when rebuilding
an attached child's geometry. Its copied `+1B4` coverage remains unchanged;
`4B5D0` receives that same word as entry stack argument `+C`. Thus computing
coverage independently from the rendered weapon's bounds would change this
path. The native caller must retain the actual collector owner explicitly.

`41040` forms each part's metric from the coverage byte, the already-resolved
`45CE0` part detail scale, `4670E4`, and the literal factor four. Complete
`1BF50` chooses the authored shader LOD with the selected shader's raw `+3C`
bias and actual view multiplier table. The later `40E30` wrapper invokes
`1BF50` with the same inputs and forwards its chosen index to `3B9D0` in EBX.
Shader overrides must be resolved before supplying the shader definition;
using the original material ID after an actual override is not equivalent.

Complete `2C490` supplies two different globals:

| Local players | `4BA028`, owner coverage | `4670E4`, part shader metric |
| --- | --- | --- |
| ordinary/default | 1 | 0.85 |
| 2 or 3 | 0.8 | 0.3 |
| 4 | 0.7 | 0.25 |

## First-person rule

`4BCC0` forces first-person **geometry** LOD 4. It does not force shader
LOD 0. The first-person dispatchers `44AC0` and `402B0` read the actual
parallel `4C0B84` byte, then invoke ordinary `41040`. The `402B0` slice
`403C1..403DD` copies every possible byte unchanged; there is no literal
255 replacement. `404B0` changes first-person drawing state after command
selection, without replacing the shader selector.

`3D2C0` owns the original selected first-person unit identity and `3D4F0`
supplies its authored first-person render-model/node banks. That selection is
a separate caller service. The helper accepts the actual collector owner's
cached sphere; a camera eye, hand bounds, or weapon origin is not a fallback.
Attached models retain the copied owner coverage. No new object registration
or attachment-lifetime chronology is inferred from this arithmetic proof.

The first-person material can therefore select a nonzero shader LOD when
its literal coverage, part scale, rectangle and player multipliers require
one. Authored bias 5 still bypasses the threshold scan. An empty template
bank retains the original `-1 / AL=true` result and does not authorize a draw.

## Material preparation seam

Production model shader selection now retains the resident shader records
independently of geometry detail. `SourceObjectGeometry` supplies the shared
MACH/BLOC geometry union, `SourceSceneryScene` supplies the selected-section
baked/dynamic SCEN union, and `SourceDynamicModelShaders` supplies the ordinary
body/attached/first-person material union. Each uses the retained section/part
identity and the literal `45CE0` scale. The caller supplies the actual owner
sphere and render-view shader bank; a triangle center or child weapon sphere
does not replace those inputs.

The minimum ordinary resident key is source material and selected shader LOD.
PRT additionally retains geometry LOD and section, because the node palette,
PCA stream and coefficient bank belong to that selected section. A shader LOD
index is not a PRT stream LOD. `SourceObjectGeometry::view(index)` retains the
selected vertices/material slots for each player independently. Its lighting
flag describes whether those selected parts need an ordinary shader binding;
it does not choose admission or cadence for the owner's source lighting cache.
The legacy
shader-absent diagnostic still copies the highest-shader record.

The existing pass producer already honors `Material::selected_shader_lod`:
`world_material.cpp::layer_passes` indexes that selected record, rather than
`lods.front()`. A resident variant must start from the raw authored material,
set the selected shader index, then prepare the source implementation:

- baked SCEN: implementation 1 via `prepare_world_material(..., true)`, with
  the existing exact selected-section incident/RGB buckets;
- actual dynamic SCEN/MACH/BLOC/body/FP: implementation 2 via
  `prepare_dynamic_material`;
- admitted PRT: independently scheduled layer 5 via `prepare_prt_material`
  and that section's geometry-LOD coefficient bank. The current bounded
  producer explicitly rejects mixed layer 3/5 materials.

Raw BE240 object colors must be retained **after** the selected passes exist,
using the shared original pack/unpack draw boundary. Complete `16B90` binds
authored constants (`17170`), vertex externs (`19BE0`) and pixel externs
(`19CA0`) before `17420` applies animated pixel (`176A0`) and vertex (`17960`)
parameter masks. Animated values therefore win overlapping components; object
externs survive in untouched components. Resetting each frame to `pass.program`
without restoring the retained object bank loses those untouched values.
Reapplying colors after all animation reverses the original overlap order.
Per-view lighting/camera externs must also preserve the pass's actual source
binding rules. Copying a previously prepared highest-detail material's bound
constants or texture stages cannot prepare a different authored shader LOD.

Actual visibility, collected sections, part identities, source overrides and
lighting policy remain caller inputs. Four unresolved Zanzibar pillar region
selections are not resolved by this module. No generic bake/dynamic fallback
is introduced.

The admitted all-map census contains no shader with highest-record layer 5
and a lower record. The eleven ordinary lower records on PRT-capable models
belong to parts that already use layer 3 at highest detail. They must not be
described as a same-part PRT-to-dynamic transition. Literal masks still control
each resident record's implementation independently.

The live owner sphere is supplied by the existing source cached-bound service.
Reference-player format 3 retains the original owner sphere explicitly.
Ordinary native player bounds use the current native body pose; equivalence of
that pose's phase to original gameplay remains a separate boundary. This
shader arithmetic and material union do not establish original object
registration, live controller chronology or current Havok broadphase bounds.

The MACH/BLOC union allocates 1,735 root material slots across the 21 maps;
the largest per-map union is Zanzibar's 291 (36 MACH and 255 BLOC).
This allocation census excludes BSP, SCEN, bodies and first-person objects,
and is not a count of submitted draws. The full table is
`analysis/multiplayer-shading/source-object-shader-resident-inventory.txt`.
The renderer's resident limit must account for the complete level and all
player-specific banks, rather than comparing this partial union to 2,048.

## Independent validation

`tools/differential_source_model_shader.py` executes the supplied unchanged
retail executable SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
It chains the original coverage slice, quantizer and complete `16AE0`,
complete `1BEB0`, original part metric slices and complete `1BF50`. It also
executes complete `40E30` through its actual `1BF50` call, stopping before
the separate `3B9D0` draw-list service. No callee/result is replaced.

The full chain matches **6,793 cases / 33,965 words bit for bit**, including
**1,758 literal model-part shader/scale contexts** from the 21 original
caches. These are authored metadata contexts, not a count of visible parts.
Twelve complete `133F70` argument fixtures retain the exact coverage word,
including signed zero and invalid values. Twelve complete `133AB0` child
copy fixtures retain it both in the copied bank and the `4B5D0` argument.
All 256 first-person parallel bytes are copied unchanged by the original
`402B0` instructions. Source execution control is explicit x87 `027F` and
MXCSR `1F80`; this does not establish physical Xbox arithmetic or pixel parity.

The report is `analysis/multiplayer-shading/source-model-shader-original.json`;
the frozen native cases are `tests/fixtures/retail_source_model_shader.tsv`.
The arithmetic seam is separate from resident-material admission and GPU
scheduling tests, which must validate their own actual submitted variant banks.

`source_object_shader_geometry_tests` checks 80,712 actual expanded-part routes
from the Headlong bus, Zanzibar crane and big wheel. It selects 28,872 lower
shader records and checks 22,968 PRT vertices using explicit near/far owner
spheres, 1/2/4-player multiplier banks and source rectangles of height 480/720.
Forced source flag 19 keeps geometry fixed, so a changed stream cannot conceal
shader-detail selection. The test checks retained quantized object colors,
independent player material slots, source section/part identity, selected
ordinary shader bindings, pure-PRT binding omission and a highest-only negative
control. These are resident routing tests with explicit caller inputs, not a
claim that those synthetic spheres are original gameplay state or that pixels
match Xbox output.

The focused routing test also passes with address/undefined sanitizers enabled.
The older machine, prop and geometry-detail scene tests remain separate legacy
coverage; their shader-absent diagnostic does not validate per-view shader
selection.

## Owner lighting cache admission

The original lighting cache is prepared before part shader selection.
`133790` invokes `4B5D0`; its geometry rebuild path performs optional PRT
`23690`, then `3DD10` and, when the collected model's render-cache context is
present, `3DB00`. The resulting 84-byte state is copied into that context.
The later `133F70` / `2CBF0` / `41040` command path selects the part shaders.
Even a failed `23690` result reaches this lighting tail before the subsequent
draw-success test. A material with only layer 5 therefore still participates
in owner-cache admission. Omitting its ordinary shader uniform binding is
different from omitting its owner-cache update.

This is not an unconditional query on every frame. `4B5D0` can reuse geometry
when its actual context offset, frame/view stamp, retained per-view geometry
LOD and `4C190` result permit reuse; that branch bypasses both `3DD10` and
`3DB00`. Failed section/residency/collector admission and absent render-cache
context are earlier gates. Native resident geometry does not itself reproduce
the original context allocator or geometry-cache reuse history.

Within `3DB00`, object `+7` bit 0 selects the global default and object `+D0`
selects an authored BSP probe. Otherwise `BAF80` follows the actual parent
chain to the root. The requested root runs `3DFE0`; an attached child inherits
the root state. `3DFE0` skips a repeated query frame and a valid frozen
(`object+C1` bit 0) cache. An initial invalid cache still attempts its query
when frozen. Other retained states use the original distance cadence,
movement threshold, last-render stamp and interpolation inputs. The source
cache-allocation failure branch has a separate direct-query fallback.

`tools/audit_prt_object_light_admission.py` executes unchanged post-PRT
`4B5D0` instructions or its actual reuse branch through complete `3DD10`,
`3DB00`, `BAF80`, valid-existing `3DDD0` and `3DFE0`. Its 27 fixtures clone
the actual retained big-wheel, blades and main-crane object/definition bytes,
with explicitly supplied initial or valid cache states. Twenty-one fixtures
reach `3DB00`; twelve reach the real `D1850` query entry. It proves the initial,
frozen, repeated-frame, absent-context, reuse and failed-PRT-result gates
without replacing a callee or query result. The report is
`analysis/multiplayer-shading/prt-object-light-admission-original.json`.
The earlier geometry/PRT services, original cache allocator and live reuse
history remain caller boundaries; these fixture cache states are not claimed
to be the literal paused owners' cache payloads.

Three paired-view fixtures also execute complete `D4080` with the original
Zanzibar `gldf` records/curves and explicit interpolation-disabled state. A
valid nonfrozen cache at render frame 4, last query frame 0 and a moved owner
sphere skips the first view's query at distance 1 (interval 60). That complete
first call leaves the query stamp at 0. The next view in the same render frame
uses distance 401 (interval 3), reaches the real `D1850` entry and stamps 4.
Thus a blanket per-render-frame cache-call guard is broader than the original
last-query-frame guard. Each admitted, non-reused view must evaluate the
actual cache gate; the native cadence/conversion helper already retains that
last-query-frame distinction.

No expanded-vertex or accepted-part-shader count gates this original call.
With explicit resident resources, `4C2B0` can succeed even when all selected
regions are `NONE`. `4B5D0` still prepares its node/collector bank and reaches
the lighting tail. A native resident collector must therefore keep its cache
admission separate from both its geometry and shader-selected vertex counts.

Production cache advancement must remain separate from `ViewDraw`'s ordinary
binding flag. The native MACH/BLOC caller supplies its represented resident
collector eligibility; original registration and allocator/reuse history remain
separate. No shader mask supplies those missing owner-state inputs.

Cold BLOC frozen state has a literal constructor producer. `D5560` pushes its
force argument 1, `BF0F0` forwards it to `D5060`, and `D5060`'s mask `0x1883`
includes type 11. This sets request `+18` bit 3 regardless of placement bit 8.
`B7B40` clears the object `+C0` word and copies request bit 3 into object `+C1`
bit 0. `tools/audit_constructor_frozen_bit.py` executes that unchanged suffix
and binder for 1,024 input cases, with no errors. The native constructor cache
retains `frozen=true` alongside its initial sphere and velocity; it rejects
later physics advancement or root/scale changes. The report is
`analysis/multiplayer-shading/constructor-frozen-bit-original.json`.

Live MACH frozen state additionally depends on `B7060` / `B7150` header/tick
admission of `BC820`, which calls `BC5E0` for an existing physics component.
Active physics reads all body groups through `1D1C10` / `30BC40`; `30BC40`
reads the actual engine object's island pointer at `+44` and its activity byte
at `+29`. The final frozen bit is the inverse of any active group. With no
active physics, a root is frozen and a child inherits its parent's frozen bit.
Gear pose arithmetic marks the source pose/header dirty but cannot supply
that backend activity byte. The native MACH cadence currently evaluates with
`frozen=false`; that literal is not evidence of the source body's current
state. Its later frozen/physics producer remains unresolved; the device type
or PRT mask is not a substitute.

`source_object_light_views_tests` exercises the production prop scene with
the real Headlong bus and real source collision/lightmap samples. Its later
nonfrozen physics/cache origin and velocity are explicit controlled inputs.
At frame 4 the near view retains the old bank and the far view binds the
newly sampled bank; a first-view-only negative control differs. This validates
the native view/cache integration, not a live Havok motion transition.

## Per-view extern ordering audit

Original `16B90` binds the lighting/camera externs before `17420` applies the
animated parameter masks. The current renderer clones an already animated
material bank and applies the per-view lighting/camera externs afterward.
`tools/audit_shader_view_animation.py` checks the literal selected shader LOD,
layer, pass, implementation, default fragment permutation/offset and component
masks from the original caches, without using a native program as its expected
mask.

The conservative MODEL pass superset covers 1,071 shader/implementation
contexts and 11,644 pass contexts. Of these, 2,004 contain vertex/pixel lighting
externs and 235 contain the RGB camera extern; none of those 2,239 contexts has
an active animated vertex/pixel write. The BSP superset covers both authored
static implementations in 1,314 shader contexts and 11,752 pass contexts. Its
four camera-extern contexts also have no animated vertex/pixel writes. There
are therefore zero overlaps in these supplied asset contexts, and the current
per-view write order yields the same bank for them. That result does not
authorize reversing the general original order for another asset or fragment
configuration. The earlier object-color overlap oracle proves why that order
matters when masks overlap.

The complete raw mask records, cache/fixture checksums and explicit scope are
retained in `analysis/multiplayer-shading/shader-view-animation-inventory.json`.
Every independent layer is conservatively included, so the reported counts
are authored metadata contexts rather than visible draw counts. The player/FP
declared material tables are a superset, and the four unresolved Zanzibar
pillar selections remain excluded.
