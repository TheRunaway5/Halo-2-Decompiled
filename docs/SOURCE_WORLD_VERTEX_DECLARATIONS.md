# Original ordinary world vertex declarations

`tools/audit_world_vertex_declarations.py` reads all 21 supplied Xbox caches.
It records 7,605 compiled classification records, 1,480 class-0 records,
79 distinct class-0 payloads and 164,648 literal shader/template/pass references.
The artifact is `analysis/multiplayer-shading/world-vertex-declarations.json`.
These counts include resident programs outside ordinary world rendering.

## Declaration authority

The unchanged retail `1C330` reads the selected `vrtx` classification's
ushort semantic list at classification `+8`, with count at `+4` and stride
`0x1C`. The list ordinal is the NV2A input register. `1C8C0` obtains each
stream's literal declaration at `43F408 + 21 * stream_type`; that table holds
semantic/format byte pairs, `FF` terminates and `FE` inserts padding.
`35790` supplies the D3D format and `35850` its byte size.

The audit executes those original instructions unchanged for 34 distinct
literal semantic lists, producing 2,176 exact declaration words. The supplied
metadata and stream bindings are explicit relocated inputs. This proves the
mapping arithmetic; it does not establish a live original classification,
visibility or successful registration event.

The native ordinary path decodes/skins geometry on the CPU and loads the first
compiled payload, class 0. The original packed model classes also include
blend-index/weight registers; their different register numbering must not be
copied onto the native class-0 payload. Retained first-person packed inputs
provide independent source-stream evidence, including the shifted packed
position, UV and NORMPACKED3 registers.

## Consumed ordinary layouts

| Program family | Class-0 semantic sequence |
| --- | --- |
| `stencil_texture`, `stencil_emissive` | position, UV |
| `lightmap_dynamic*`, `stencil_env_bumped_tangentspace` | position, UV, normal, tangent, binormal |
| `lightmap_perpixel*` | position, UV, normal, tangent, binormal, incident, lightmap UV |
| `lightmap_pervertex*` | position, UV, normal, tangent, binormal, incident, radiance |
| `transparent_generic_reflection` | position, UV, normal |
| `transparent_generic_offset` | position, normal, tangent, binormal |
| `prt` | position, UV, normal, tangent, binormal, PRT cluster, PRT weights |
| `prt_lightmap` | position, UV, normal, tangent, binormal, radiance, PRT cluster, PRT weights |
| shadow generation | position, UV |
| shadow application | position, normal |

The complete ordinary subset contains 531 class-0 records and 33 program
names. Its semantic IDs are exactly `0,3,4,5,6,8,9,14,19,20`.
Inline shader attributes outside each declaration remain separate authored
bindings, including object-color registers 10/11.

All 777 literal shader roots / 33,285 pass references from the ten tested
Chief/Elite body, hands, SMG/battle-rifle/sniper held and first-person roles
per map matched the former ordinary register adapter. The BSP/selected-model
part census contains two differing material references: Triplicate's
`ze_train` and `ze_train_west`, layer 5 implementation 0, use `prt_lightmap`.
The existing generic PRT override supplied cluster/weights in registers 5/6;
this program actually reads radiance 5, cluster 6 and weights 7.

The resident `transparent_generic_offset` reads normal register 1, while the
former generic adapter supplied UV there. Its current plasma-shield pass
names do not pass the ordinary late-pass admission guard, and none occurs in
the selected material census. `stencil_env_super_fast` and decorator families
also have different resident declarations but are outside that admitted path.
They must retain their own declaration when their original submission path
is implemented.

## Authored model streams restored

`tools/audit_model_extra_streams.py` scans every resident `mode` in all 21
caches. Exactly 42 sections / 7 model instances / 23,459 source vertices have
extra streams. Every one declares channel 3 as type 48, stride 4 (semantic 8,
NORMPACKED3) and channel 4 as type 47, stride 3 (semantic 14, PBYTE3).
They belong to four Containment shield/gate models, the unplaced Turf scarab
and the two Triplicate trains.

Both train radiance streams are entirely black: 4,071 and 4,067 vertices.
The former decoder ignored these streams and retained white vertex color.
`model.cpp` now validates their literal descriptors and preserves incident
and radiance before triangle expansion. Sections without these streams keep
their previous defaults. Incident remains authored world-space data; pose
does not rotate it through the model root, just as the original lighting
program consumes it against the transformed basis.

`retail_model_extra_streams` checks frozen original stream identities and
format-reference conversions through native expansion and rooted posing,
including black train radiance and six plain-PRT controls. The incident
conversion retains signed 11/11/10-bit division without endpoint clamping;
PBYTE3 retains component byte order and normalization by 255. These are the
pinned supplied GPU-reference semantics and original declaration formats,
not new physical Xbox raster or live original vertex-output evidence.

The unchanged original `35790` format-6 case at `357BB` returns `0x34`.
The unchanged `3F7440` uploads this exact low byte into the NV097 vertex-array
format method `1760`; its arithmetic at `3F75BA..3F75C4` adds only the stream
stride. The two literal stream layouts in this census produce 68 exact
original push-buffer words, including radiance format `0x334` (stride 3,
component count 3, type 4 `UB_OGL`). The retained Xemu OpenGL fetch sets
unsigned-byte normalization; its Vulkan fetch uses `R8G8B8_UNORM`.
This identifies the supplied fetch conversion independently of packed CPU
lightmap RGB or brightness conversions. The physical Xbox vertex-fetch
precision boundary remains explicit.

The four Containment models and both trains have only zero radiance bytes.
The unplaced Turf scarab contributes 5,841 nonzero, nonendpoint components,
with 107 distinct byte values across the full census. For example, section 1
vertex 52 has literal bytes `(12,23,35)`, and vertex 53 has `(32,59,90)`.
The frozen test checks their normalized binary32 values as well as all-zero
train data: 23,459 source vertices / 42 sections are covered by all six
authored detail-level collectors. It verifies 217,008 expanded and 217,008
rooted posed vertices, 189,000 black-train checks and 8,790 unchanged
plain-PRT control vertices. Normal and ASAN/UBSAN checks pass.

No original guest advancement, input, asset modification or inferred
registration/append order was used for this audit.
