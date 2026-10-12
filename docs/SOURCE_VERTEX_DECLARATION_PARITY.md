# Authored world vertex declarations

The native world adapter now binds vertex registers from the selected original
`vrtx` classification's literal semantic list. Original `1C330` assigns the
register by the semantic's ordinal in `class+8` (count at `class+4`, class stride
`0x1C`). `1C8C0` supplies the actual section stream descriptors; immutable XBE
table `43F408`, stride21, contains semantic/format pairs. Original `35790` and
`35850` provide their D3D format and byte sizes. Shader names and native pass
categories are not register declarations.

The all21 original cache inventory is
`analysis/multiplayer-shading/world-vertex-declarations.json`, produced by
`tools/audit_world_vertex_declarations.py`. Its unchanged `1C330` executions
cover34 literal declaration lists and2,176 output words. The complete asset
inventory includes7,605 compiled class records and1,480 class0 records; these
counts include non-world effects, unavailable programs and repeated cache
copies. They are authored availability, not executed draws.

The selected ordinary corpus uses only semantic IDs0 (position),3 (UV),4
(normal),5 (tangent),6 (binormal),8 (incident),9 (lightmap UV),14 (radiance),19
(PRT cluster) and20 (PRT weights). The ten original player/body/hands/weapon
families contribute777 shader roots and33,285 pass references with no consumed
register differences in the previous adapter. Among968 selected BSP/model shader
roots, only Triplicate's two train materials have a consumed-input difference:
`prt_lightmap` expects `[0,3,4,5,6,14,19,20]`. It reads radiance at v5, cluster at
v6 and PCA weights at v7. Plain `prt` uses `[0,3,4,5,6,19,20]`, with cluster and
weights at v5/v6. Original channel3 type48 is packed incident, and channel4
type47 is three-byte radiance. Both train radiance streams are entirely zero
over8,138 source vertices; substituting absent-stream white changes their input.

`transparent_generic_offset` has `[0,4,5,6]`, so v1/v2/v3 are normal/tangent/
binormal. It is a latent adapter correction in the supplied selected corpus;
this work does not admit its currently unsupported plasma-shield pass. Existing
source layer, target and runtime admission remain unchanged.

`SourceVertexDeclaration` retains the original class ordinal and semantic list.
`Cache::load_source_vertex_declaration` validates the original tag version,
class/count and metadata pointers. `MaterialPass` retains class0 metadata, and
each existing pass binder transfers it to the prepared `Material`. Unknown
semantics or local/skinned classes cannot be packed as ordinary world streams.
The supported decoded inputs retain their original format component defaults:
three-component values have w1, UVs have z0/w1, the cluster has y0/z0/w1 and PCA
weights retain all four components. UVs/positions/bases have already passed
through the existing source decompression and pose/placement producers; this
adapter does not replace those transformations or their source evidence.

`tools/audit_model_incident_coordinates.py` retains the original instruction
dataflow in `analysis/multiplayer-shading/model-incident-coordinate-proof`.
Eighty class0/class1 records cover twenty distinct compiled payloads from
Lockout, Containment, Turf and Triplicate. The local classes transform the
normal, tangent and binormal through c50..52, while incident is copied or
normalized without that matrix. Each first node/incident combination is DP3
against an already transformed geometric basis. The world classes perform the
same projection against their already transformed input bases. Incident stays
in its authored world coordinates; posing it would introduce an unsupported
rotation.

One appended `uvec4` in the material uniform stores16 semantic selectors;
previous offsets are unchanged (3696→3712 bytes). Unlisted registers retain
the material's constant vertex attributes. The default/procedural adapter is
explicit and unchanged; actual source world materials use the authored mapping.
`source_vertex_inputs.glsl` supplies the production vertex interpreter. PRT
semantics require its separate actual PRT vertex bank, and animation cannot
silently change a resident declaration. Shadow world0 declarations independently
remain generation `[0,3]` and application `[0,4]`.

`tools/generate_source_vertex_declaration_fixture.py` freezes46 original class0
payloads and10 declaration layouts, with the complete unchanged `1C330` output
words. Forty-five payloads come from referenced ordinary asset families; one is
the labeled latent offset control. The fixture is
`tests/fixtures/retail_world_vertex_declarations.tsv`, SHA-256
`472c5dad3607f52e4f0925fd92141d06c0b7ea5e0aeb750d640b093f1251633e`.
The test derives expected register values independently from original stream
index/byte-offset/format descriptors, verifies native raw cache metadata/code,
and exercises malformed version/class/count/pointer and unsupported semantic
controls. Its Vulkan compute variant includes the production input loader,
without the reference-input macro. Controlled decoded streams and constant
banks are explicit test services, not retained original gameplay banks.

Normal and ASAN/UBSAN CPU runs check11,776 input words exactly. The required-validation GPU
run checks12,288 register words exactly, including eight wrong-register controls
for PRT-lightmap and latent offset shaders. Of12,288 authored-program output
words,12,257 are exact; remaining finite outputs differ by at most2 ULP in the
measured NVIDIA run. This proves declaration binding and the bounded lowering;
it does not prove physical NV2A arithmetic precision, live original class
selection or complete map appearance parity. Separate original stream fixtures
retain the model data provenance and unchanged absent-stream defaults.
Both normal and ASAN/UBSAN GPU runs pass with required Vulkan and synchronization
validation. The actual Renderer shadow fixture and its declaration-admission
controls also pass in both builds, preserving the phase-specific source maps.

The separate local/skinned audit in `SOURCE_CPU_SKIN_INPUTS.md` preserves these
raw world declarations. A literal original part class and exact reflection
payload identity can request NormalizeSkinNormal for proved classes3..5.
Only the packed Normal selector receives bit6; the material UBO remains3712
bytes, with unchanged offsets. Rigid/world and other program families retain
Preserve. No local bone-index or skin-weight semantic is admitted as an
ordinary world register by this modifier.
