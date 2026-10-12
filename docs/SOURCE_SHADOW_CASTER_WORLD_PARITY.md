# Original shadow casters through the decoded WORLD variant

The existing `prepare_shadow_material(..., Generate, true, ...)` selects the
literal authored class-0 `shadow_buffer_generation` program. The seven retained
original20 caster draws can use that program **when their actual posed WORLD
positions and decompressed UVs are supplied**. No preparation helper was missing,
and this proof does not relax `SourceShadowPlan` admission or activate native
shadow submission.

`original_shadow_caster_world_cpu` verifies the bridge through the actual native
material preparation, completed shadow bindings, camera/global bank packer and
vertex interpreter. It uses original DebugVertex intermediates and original
outputs as independent expectations. It also exercises the real native model
loader and skinner for one fully corroborated captured section.

## Literal original identities

`tools/generate_shadow_caster_world_fixture.py` identifies each draw uniquely
from its complete retained position-stream prefix. Every used raw attribute
component (57,170 bytes in total) and each original index segment matches the
literal Xbox Zanzibar cache. The original `1C330` semantic lists and immutable
format-size table supply the component offsets; native model names, decoded
positions and expected native outputs are not used to select an identity.

| Original event | Render model | Section / part | Executed compiled class | Literal declaration |
|---|---|---|---|---|
| 7713 | SMG `E51603A1` | 2 / 0, painted metal | 2 (class 3 has identical bytes) | position, indices, UV |
| 7721 | SMG `E51603A1` | 2 / 1, bare metal | 2 (class 3 has identical bytes) | position, indices, UV |
| 7729 | Chief `E9B90844` | 18 / 0, Chief | 1 | position, UV |
| 7737 | Chief `E9B90844` | 19 / 0, Chief | 4, two influences | position, indices, weights, UV |
| 7742 | Chief `E9B90844` | 16 / 0, Chief | 4, two influences | position, indices, weights, UV |
| 7750 | Chief `E9B90844` | 17 / 0, Chief | 5, three influences | position, indices, weights, UV |
| 7758 | Chief `E9B90844` | 18 / 1, visor | 1 | position, UV |

These are compiled **class ordinals**. The metadata classification word is zero
for all seven authored variants. The older exported filenames include
`-class0-` from that word and must not be read as the executed class ordinal.

The decoded WORLD class-0 payload is 228 bytes / 14 instructions, SHA-256
`1bb03f8703d3a9f89df087333999f960684ff3e7b9a4f045d1787d2f2e74d6dd`.
Its declaration is `[0,3]` (position and UV); it does not consume normals,
bone indices, bone weights or the high-register palette.

## Actual intermediate and completed-bank proof

The original translated shaders initialize private variables in a recorded
order. The immutable DebugVertex exporter retains first-change insertion order.
The generator checks that initialization order, scalar address register and
all twelve vector temporaries before mapping the retained private IDs to `R0`
and to the register written by the original `c172` UV-decompression instruction.
It freezes the resulting **actual original words**, rather than deriving them
with a native pose function. The original raw attributes, all 192 constant
vectors and seven output vectors are frozen beside them.

The local/boned/skinned programs first produce WORLD xyz in `R0`, and decompress
UV with `c172`. Their shared shadow suffix has the same arithmetic as class 0.
The bridge supplies WORLD position with `w=1`, decoded UV with `z=0,w=1`, and
identity `c172=(1,1,0,0)`, as the existing decoded-world production path does.
Actual completed material `c18..33`, shadow-root `c31..36/c40..48` and camera
`c0..11` are supplied explicitly through `source_shadow_plan_bindings` and
`pack_vertex_program_bank`; no host camera, root, material color or palette is
invented.

All **392 output words** from fourteen actual original DebugVertex samples
across the seven draws match exactly through both the actual raw compiled
variant and the existing prepared class-0 path. The comparisons retain the
original xemu position wrapper and its actual surface/clip conversion words.
Forty-two negative controls respond: raw local positions, raw compressed UVs,
and applying the compression bank a second time each change all fourteen
results. A raw local/skinned program is still rejected by the normal plan;
it is not relabeled as an admitted class-0 draw.

## One actual native pose slice

The separately retained paused original20 unit record contains 33 WORLD
transforms. This record was exported **after** the capture interval. It is not
assumed to be the pose state of every draw in the capture.

For Chief section19, literal compact node map
`[0,1,2,4,5,7,8,12,13]`, all **108 consumed original palette words** match the
result of the real `source_model_skin_palette` using those retained WORLD
transforms and the literal model inverse binds. This independently corroborates
that section's consumed pose bank despite the later export. Other captured
sections do not all pass that comparison, and are not promoted to matched native
pose fixtures. Section17's first three matrices match, but that partial match is
not used to establish its complete pose.

The real native model loader selects the authored Chief section19/part0,
preserves source vertex identity and per-part weights, and
`skin_model_world_nodes` consumes the corroborated WORLD bank. The native
positions/UVs match both independent original `R0`/UV samples exactly, and their
**56 output words** match the actual original DebugVertex outputs exactly.

The same path covers all **672 actually referenced vertices** of that part.
Against the complete independently frozen packed original inputs and original
class-4 microcode, all **18,816 output words** match exactly. This exercises the
native loader, original complement-weight producer, actual inverse binds,
compact-node mapping, ordered matrix blending, skinning and class-0 suffix.
Expected values are not generated by the native skinner.

Against the actual retained GPU postVS buffer, all **16,128 nonposition words**
(`oD0/oD1/oT0..3`) match exactly. The position comparison is measured separately:
928 component words differ, maximum absolute depth difference is
`7.6443e-6`, and maximum difference on the actual 128-pixel shadow target is
`0.0625005` pixel. Maximum ULP distance is 184,374 near small depth values; a small
ULP claim would be incorrect. The regression bounds are 0.06251 target pixel and
`8e-6` depth. The captured translated GPU uses `OpDot`, while the ordered CPU
arithmetic and the original DebugVertex reference agree on this fixture. This
is an explicitly bounded xemu/native arithmetic comparison, not proof of
physical retail NV2A rounding or identical silhouette coverage.

## Validation and remaining caller services

Focused normal and ASAN/UBSAN/leak-enabled CPU CTests pass. This work runs no GPU
submission and makes no guest I/O, resume, input or snapshot changes. Its new
files are the test, generator, frozen TSV/provenance and this document; production
renderer, scene, shader, pose and public ABI files remain unchanged.

Before real source shadow submission, the caller still needs the actual owner
collector/child order, source fade/priority inputs, complete current root/frame,
geometry/part admission, shader selection, source visibility and completed pixel
bindings. This fixture supplies explicit original draw inputs; it does not
produce those live services. For the other six draws, a matched current native
pose producer is not established merely by the sampled class-0 suffix proof.
The captured generation draws used classes1/2/4/5; no executed generation
class-0 draw is claimed. No new native GPU class-0 caster capture or full-frame
appearance equivalence is claimed here.
