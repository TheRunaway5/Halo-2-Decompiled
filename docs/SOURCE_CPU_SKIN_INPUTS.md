# Original local and skinned vertex input adaptation

The native CPU pose path now retains the original compiled class of every
expanded model part and applies one proved pass-specific input operation:
`transparent_generic_reflection` classes3..5 normalize the transformed normal
before the reflection calculation. Its world class0 and local rigid classes1/2
do not. The six other audited first-person program families retain their
existing input values. No authored program, texture, palette, skin weight or
material color was changed.

## Original class and declaration authority

Original `1CAA0` reads the section's classification and, for skinned geometry,
selects the opaque or transparent active-influence count according to part
types4/5. `1CB20` selects the compiled class. Original `1C330` binds registers
by ordinal in that class's literal semantic list; `1C8C0` supplies the section
stream descriptors, and `35790`/`35850` resolve their format and size. Raw
local/boned semantic ordinals cannot be applied directly to already CPU-posed
world streams.

`tools/audit_first_person_vertex_routes.py` executes unchanged `1CAA0`/`1CB20`
with actual part metadata, intercepting only downstream state consumers
`1C590`/`1CB70`/`1C8C0` for observation. Unchanged `1C330` and its format
callees then execute on literal selected declarations and stream types. The
report is `analysis/multiplayer-shading/first-person-vertex-routes-original.json`:
21 caches,84 authored SMG/BR/Chief-hands/Elite-hands model banks,147 sections,
378 parts and2,016 declaration executions/129,024 output words. Original class
counts are1:21,2:189,4:147,5:21. Sections include authored LOD alternatives;
these counts do not identify currently visible geometry or an animation phase.

`tools/audit_elite_vertex_classes.py` independently executes the same original
class selector across Elite body and FP-body sections. Its retained report
`analysis/multiplayer-shading/elite-body-vertex-classes-original.json` covers
42 model banks,819 sections and1,302 parts: classes1:399,2:504,3:42,5:357.
Neither this corpus nor the84 FP model banks contains class6. No native
character-selection service is inferred from the authored availability.

Both reports retain executable SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.

## Bounded original discrepancy

The original reflection class0 payload is676 bytes, SHA-256
`a37e3ae948caee7a6b00ddeb389fca00117dfeedc5adb2da27e37224734b6207`.
The actual Elite FP glow selects class4; its996-byte payload has SHA-256
`08a40fcd35f6d24907efdfb54d90f628144c154420eda1c9baa95b8d5e4b99de`.
Class4 instruction23 performs self-DP3 on the transformed normal,
instruction25 performs RSQ and instruction27 multiplies the normal by that
inverse length. Classes3/5 contain the same operation; classes0/1/2 lack it.
All original payloads/disassemblies are retained in
`analysis/multiplayer-shading/fp-reflection-source/`.

The native source matrix blend correctly keeps its raw normal for the other
audited programs. Feeding that normal to reflection world0 omitted the local
skinned program's DP3/RSQ/MUL. The operation is now explicit after the existing
source CPU matrix blend. Admission uses exact compiled payload size and code
fingerprint, with the literal original class as a separate input. A shader
name or native category cannot activate it.

Original reflection class6 aliases class5's payload and omits a fourth skin
influence. That specific adapter request remains rejected with a precise
diagnostic; the already validated class6 behavior of other families remains
unchanged. This work does not invent an interpretation of the aliased program.

## Native implementation seam

`RenderModel::VertexOrigin::vertex_class` records the original section/part
selection. `SourceDynamicModelShaders` retains resident slots by source
material and input class only when a proved operation is needed; otherwise it
reuses existing slots. The helper in `source_cpu_skin_inputs.cpp` copies child
passes before assigning their independent input policy. Base and reflection
passes cannot share an accidental normal modifier.

`Material::world_vertex_input_transform` is explicit CPU metadata. The existing
3712-byte material UBO and all previous offsets remain unchanged: bit6 of the
packed Normal selector encodes the scoped operation. The literal authored
`SourceVertexDeclaration` and program bytes remain unchanged. CPU and GLSL
adapters execute ordered self-DP3, RSQ and component MUL, preserving the
source zero-times-infinity behavior. Rigid/world Preserve bindings retain their
existing data. Shadow material residency and plan admission require Preserve;
the independent shadow normal attenuation must not receive this adapter.

## Validation and visual boundary

`tools/generate_cpu_skin_input_fixture.py` freezes378 original part/class rows
and49 literal compiled records across seven families/classes0..6 in
`tests/fixtures/retail_cpu_skin_inputs.tsv` (SHA-256
`81b99f1bc40eea0b4dd669b77b7f5f129d98d04a07f45aae60cf783e0c99cad4`).
The source fixture and metadata report are independent of native expected
output. `source_cpu_skin_inputs_tests` checks84 model banks and470,925 expanded
class identities. Actual source selection routes276 Elite reflection face
vertices to the new binding. Eighty unique authored Elite glow vertices and
controlled class/program inputs contribute292 test cases,89 scoped normal
bindings,88 missing-operation controls and18,688 original-program output
components.

The Vulkan compute variant includes the production typed input loader without
`H2_VERTEX_REFERENCE_INPUTS`. It checks37,376 input/output words with the
authored programs;18,586 input words are bit-exact in the measured run.
The remaining RSQ-derived inputs and outputs pass the explicit relative
tolerance in the test. Normal and ASAN/UBSAN CPU/GPU runs pass with required
Vulkan synchronization validation. CPU leak checking passes. GPU ASAN/UBSAN
passes with leak checking disabled; enabling leak checking retains allocations
from unloaded GPU/DBus modules with no application allocation stack, so GPU
leak freedom is not established. Existing world-declaration and all21 dynamic
model integration controls pass unchanged.

`source_cpu_skin_render_tests` renders the real Elite FP asset using its
authored pass/images, explicit bind channels plus a45-degree left-hand local
rotation, a camera above the posed bounds and controlled ambient/primary
lighting. It checks one selected resident modifier binding, unchanged authored
program/constants/declaration, valid rendering and byte-identical fresh repeats
at640×480 and1280×720. Authored before/after RGB is neutral in this inspection.
The images do not prove visible correction, original Elite appearance or a
gameplay controller phase. No original Elite GPU capture was supplied; the
bounded correction is supported by original code/input execution and the
production-loader tests. Physical NV2A arithmetic precision and complete
first-person appearance parity remain separate validation requirements.

## Plasma shield local-position increment

The damage/shield goal adds a second exact-payload policy for original
`transparent_generic_offset`. Its classes1..6 transform position and raw
normal for geometry, while generating noise outputs `o9/o10.xyz` from the
original decompressed position before posing. Class6 has a distinct
four-influence payload; the reflection alias limitation above does not apply.
`Vertex::source_local_position` now retains this position through triangle
expansion and posing. The packed POSITION selector's bit6 selects six DPH
operations after the unchanged world program; all output W components survive.
The production Vulkan binding supplies this separate stream. This policy
requires literal declaration `[0,4,5,6]` and the exact compiled family/class;
other programs keep their existing input contract.

Eighteen controlled poses match1,152 CPU output words against all six
original local/skinned programs. The production compute path checks3,456
components within its stated2e-5 arithmetic tolerance, including missing-local
and wrong-normal controls. Real Spartan/Elite body/arms positions survive
posing, and the complete renderer produces deterministic authored-material
inspections with responding local-coordinate controls. These tests do not
establish original framebuffer precision or live shield scheduling.
[Source pass/pose evidence](SHIELD_PLASMA_PASS_RESEARCH.md),
[CPU/GPU test](../tests/source_shield_vertex_tests.cpp),
[renderer inspection](../tests/source_shield_render_tests.cpp),
[active goal and remaining owners](DAMAGE_SHIELD_COVERAGE_GOAL.md).
