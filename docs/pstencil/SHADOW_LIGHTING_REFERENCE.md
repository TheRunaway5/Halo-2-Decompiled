# May 2003 shadow lighting reference

**The May 2003 renderer has two separate shadow operations.** Ordinary point-light
stencil rejects direct light. A separate **lightmap-shadow** stage builds a mask
from eligible models and their cached incident lighting, then multiplies the
already rendered color once. That multiplication precedes environment mapping,
self-illumination and dynamic lights. Repeating it for every grenade would not
match the inspected source schedule.

This explains why removing all added point light can still leave a bright
current level: the delivered point-light stencil operation does not, by itself,
alter the existing baked/base contribution. It does not establish that every
May 2003 shadow was black, or that the current levels should receive replacement
baked lighting.

## Primary provenance and recovered scope

Reference: read-only `/mnt/SSD4TB/Leaks/Halo 2/2003/game/halo2debug.xbe`, SHA-256
`5a9edb4c874a782594c86020db49b0086f44ae38f53ff2276b3192a2c2383d18`,
with its matching `tools/bin/halo2debug.map`. [The focused export manifest][manifest]
retains section-resolved function addresses/hashes, raw globals, layer dispatch
entries and part masks. [The extraction script][extract] reads these literal
bytes; Ghidra exports were performed against the existing project with
`-readOnly -noanalysis`. Disassembly owns calling-convention/FPU details where
pseudocode drops register inputs or intermediate precision.

## Schedule: one lightmap mask, then one darkening operation

`render_scene` (`00051160`), normal scene type 1, schedules the following. Its
default layered-renderer path is selected when `0074455C == 0`. The matching
primary symbol and HS binding name this Boolean `render_old_rasterizer`; its
image-initialized value is zero. The nonzero branch selects the old rasterizer.
The dedicated ISQ stencil algorithm examined below is distinct from that
old-rasterizer branch; exported old functions do not establish that it ran
in a particular frame. [The caster audit](CASTER_FACE_PARITY.md#layered-renderer-branch-selection)
records the variable binding and initial-value evidence. The names come from
`render_layer_begin`'s literal string/dispatch tables.
[Scene schedule][scene], [begin dispatch][begin].

| Order | 2003 layer | Owning behavior |
| --- | --- | --- |
| 1 | 5, lightmap (indirect) | Scheduled at `00051697`; environment/model baked or cached lighting contribution. |
| 2 | 6, lightmap (shadow) | Scheduled at `0005174A`; stencil clear `0x80` once at layer entry, eligible models build the mask. |
| 3 | 7, lightmap (diffuse) | Scheduled at `00051807`; `000E8350` draws the screen rectangle with setup `000E81A0`, testing stencil **NOTEQUAL `0x80`**. |
| 4 | 12, env map | Scheduled at `000518C4`, after the multiplier. |
| 5 | 10, self illumination | Scheduled at `00051977`, after the multiplier. |
| 6 | dynamic lights | `render_lights` called at `000519F7`; each admitted light has its own shadow/utility/light sequence. |

The lightmap mask is cleared by [layer shadow entry `000E8A40`][shadow-layer],
not by each model's fake-light builder. Each model can have a different source
direction while contributing to this common mask. Overlapping volumes do not
cause repeated full-screen multiplication. `000E8350` does have a debug repeat
count at `0048F428`; its XBE initial value is zero, so the normal operation draws
once. This is an explicit debug control, not a per-light loop.

The separate ambient-pass function `001CCBD0` is a **return-only function** in
this executable. Do not invent an ambient replacement from that name.
[Ambient function][ambient]. The model lightmap fallback in `001CFCD0` consumes
cached render lighting when a usable authored lightmap section is unavailable.
[Model lightmaps][model-lightmaps].

## Exact multiplier, blend and alpha behavior

The [setup disassembly `000E81A0`][multiply] is authoritative:

| State/program input | Literal behavior |
| --- | --- |
| Target | Current main target `00743894`, through `rasterizer_set_target(target, 0, true)`. |
| Color write | `0x1010101`, all four channels. |
| Blend | Enabled; source factor `0`; destination factor `0x300` (source color); ADD `0x8006`. Thus `destination × shader_output`. |
| Depth | ALWAYS `0x207`; depth writes disabled. The screen operation is admitted by stencil rather than receiver geometry depth. |
| Stencil | Enabled; KEEP on fail/depth-fail/pass; comparison NOTEQUAL `0x205`, reference `0x80`. |
| Pixel program | Final ABCD `0x00000001`, final EFG `0x00001100`; final constant 0 is `0xFFbbbbbb`. Its output is constant RGB `b/255` and alpha 1. |
| Amount | `b = __ftol2((1.0 − ssc_lightmap_shadow_darkening) × 255.0)`, evaluated by the original x87 instructions, not a replacement host float expression. |

The exact script-global entry at XBE `004C2DFC` is
`38 75 44 00 06 00 00 00 44 F4 48 00`: name pointer `00447538` is
`ssc_lightmap_shadow_darkening`, value pointer is `0048F444`. The initial value
there is `CD CC CC 3E`, float32 `0.4000000059604645`; the other literal operands
are `003DC844 = 1.0` and `003E23C4 = 255.0`. These are executable data, not chosen
material parameters. They and the zero debug-repeat count are retained in
[the manifest][manifest].

Precision matters. `real_math_initialize` (`0003F640`) and
`real_math_reset_precision` (`0003BB10`) call `__control87(0x9001F, 0xFFFFF)`;
reset is called in game initialization, new-map initialization and each
`game_tick` (at `000698F3`). CRT default precision also calls
`__controlfp(0x10000, 0x30000)`. Under the source's 53-bit precision contract,
original setup plus original `__ftol2` yields **byte 152, `0xFF989898`**, hence
RGB multiplier `152/255`, about `0.596078`. Rounding the intermediate expression
to float32 can produce byte 153 and is not the same contract.
[Initialization][math-init], [reset][math-reset], [tick caller][tick],
[CRT precision][crt-precision].

The retained [original-CPU oracle result][oracle] compares all 14 setup state
writes and records RGB 152 / packed `0xFF989898` after original setup and
`__ftol2` execution. This is CPU/program-construction evidence; it is not an
original Xbox framebuffer capture.

Alpha is preserved by the blend: shader alpha is 1, source factor is zero and
destination factor is source color, so destination alpha is multiplied by 1.
The pass is not an alpha overlay, additive black, or replacement direct-light
opacity. Its receiver scope is **screen pixels whose common lightmap-shadow
stencil differs from `0x80`**. There is no material-by-material receiver test
inside this full-screen operation.

## How the lightmap-shadow mask is generated

`rasterizer_model_draw` (`001C5EE0`) recognizes layer 6 and requires the model's
eligibility byte at `+0xD0` and nonzero presentation opacity byte at `+0xD`.
`rasterizer_model_compute_fake_lighting` (`001C60D0`) initializes eligibility
false and sets it true only when the actual object's type short at `+0x68` is
**0 or 1** and `render_model_check_shadow_manifold` (`001606B0`) succeeds.
These are specific source gates; current authored caster-pass availability is
an adaptation, not proof that the 2003 manifold gate was executed.
[Model draw gate][model-draw], [eligibility producer][model-eligibility].

The selected model carries a cached `render_lighting` pointer at `+0xCC`.
The [model-begin dispatch][model-begin] sends layer 6 to `001CE670`, which passes
**that lighting record + `0x1C`** into `001CC280`. The builder:

- multiplies this three-component vector by literal **−20000.0** at `0044A750`
  to form a far light origin;
- builds original light geometry/state, resets the active-light bookkeeping,
  and calls `rasterizer_light_begin` with extrusion argument **2.0**;
- enables the special lightmap-shadow flag `007439F0` in the caller, then uses
  the model shadow path. Ordinary dynamic lights instead pass extrusion 1024.0.

[Fake-light caller][fake-caller], [builder][fake-builder],
[ordinary-light schedule][dynamic-schedule]. The fake light is a separate
construction; a nearby point light with a modified opacity is not equivalent.

The vector's producer is identified. `build_distant_lights` (`000E2690`)
normalizes the sampled incident vector and writes its negative to
`render_lighting` floats **7..9**, exactly `+0x1C`. It also computes a distinct
clamped projected-shadow direction at floats 16..18 (`+0x40`); that is **not**
the vector used by this fake-light caller. [Direction producer][direction].

The current TU1.5 [object-lighting state][current-lighting] uses a different
layout and clamps `shadow_direction.z` to at most `−0.75`; its `primary_direction`
also goes through an authored angle limiter. Neither field is automatically the
2003 `+0x1C` value. A bounded current-asset adapter can obtain the raw current
`ObjectLightmapSample.incident`, normalize it and negate it, then retain the
literal far-origin/extrusion construction. The availability, generation and
validity of that actual sample remain necessary inputs.

## Caster faces and receiver limits

The original part predicates resolve to this table; numeric types are from this
2003 executable and must not be reused as retail enums without checking.
[Opaque predicate][opaque], [casting predicate][casting],
[shadow-only predicate][shadow-only], [transparent predicate][transparent].

| 2003 part type | Opaque | Shadow casting | Shadow only | Transparent |
| --- | --- | --- | --- | --- |
| 1 | yes | yes | yes | no |
| 2 | yes | yes | no | no |
| 3 | yes | no | no | no |
| 4 | no | no | no | yes |

The literal `g_valid_part_types_from_render_layer` table (`004A90C0`) gives
layers 6 and 13 mask `0x4`, i.e. bit/type 2, in the generic
`rasterize_model_part` admission path. But the dedicated ISQ model route examined here
dispatches complete resident sections through `000E9B40` to `0017B4D0`.
That function consumes cached triangle/face-facing bitvectors, shadow caps,
ISQ adjacency and rigid-group/skinning admission, with special handling when
`007439F0` is set. A blanket triangle filter based on one of the predicates is
not the complete ISQ volume algorithm.
[Generic part admission][part-admission], [section dispatch][sections],
[caps/adjacency consumer][caps].

Layer 6 submits models; the separately named environment-shadow function is
used by ordinary point-light layer 13. Static world geometry therefore receives
the resulting full-screen mask through the visible depth/stencil surface, but
this inspected layer-6 route does not establish a second set of BSP fake-light
casters. Future reflection/emission/transparent contributions are scheduled
after darkening; multiplying the final composed framebuffer would darken
contributions the source adds later.

## Point lights, including placed/static inputs

`render_lights` (`000F8050`) traverses the admitted runtime light candidates,
loads their `ligh` definition/state, performs visibility and shadow selection,
submits the lit/casting scene, and invokes `render_dynamic_light`. That function
clears/builds its own ordinary stencil mask and uses utility/light passes which
test **EQUAL `0x80`**. It does not invoke the lightmap full-screen multiplier.
[Runtime light loop][runtime-lights], [direct receiver state][direct-receiver].

The 2003 executable has an actual **scenario-light creation path**:
`light_place` (`000E4DC0`) resolves the authored scenario/palette entries and
calls `light_new_from_scenario` (`000E4AD0`); that constructor writes the real
scenario index, definition and placed transform into a runtime light datum.
`light_get_geometry_from_scenario` (`000DF6E0`) supplies scenario-specific
geometry. This is stronger evidence than a loose light-tag filename, but still
does not prove every fixture emitted a runtime point light in a captured frame.
[Placement][light-place], [constructor][scenario-light],
[scenario geometry][scenario-geometry].

The current pStencil [direct-light adapter][current-points] discovers actual
constant untimed `ligh` **object attachments**, resolves current roots/markers,
and supplements the existing effect-light producer. It does not reconstruct
2003 scenario-light placements from emissive textures, import the 2003 level's
lights into current levels, or recover a current level's baked lights as runtime
points. Any static point-light extension must use actual current authored
placement/attachment data plus the source admission/lifetime/geometry rules.

## Actionable adaptation and validation boundary

The supported missing feature is a **separate pre-reflection lightmap-shadow
mask and one source-quantized color multiply**, with the current baked/base
lighting retained. Keep ordinary per-point stencil rejection unchanged. Use
actual sampled incident lighting and admitted current posed caster geometry;
explicitly record where current caster availability replaces the old manifold/
ISQ gate. Preserve depth, alpha, view isolation and Original-mode restoration.

Do not apply the multiplier per point light, after reflections/emission, or with
a guessed darkening amount. Do not claim exact 2003 world-shadow coverage from
the current closed-prism geometry adapter: the special source cap/adjacency
path, all original admission inputs and Xbox framebuffer comparison remain
separate validation requirements. The recovered code establishes why baked
color can be darkened and its exact ordering; it does not prove a particular
current level will match a 2003 screenshot's absolute brightness.

[manifest]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-shadow-lighting/build.json
[extract]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-shadow-lighting/extract.py
[scene]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-shadow-lighting/disassembly/00051160.asm
[begin]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-shadow-lighting/disassembly/000e8e20.asm
[shadow-layer]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003/functions/000e8a40.c
[ambient]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003/functions/001ccbd0.c
[model-lightmaps]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003/functions/001cfcd0.c
[multiply]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003/disassembly/000e81a0.asm
[math-init]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-shadow-lighting/disassembly/0003f640.asm
[math-reset]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-shadow-lighting/disassembly/0003bb10.asm
[tick]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-shadow-lighting/disassembly/00069860.asm
[crt-precision]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-shadow-lighting/disassembly/00309fe9.asm
[model-draw]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-shadow-lighting/disassembly/001c5ee0.asm
[model-eligibility]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-shadow-lighting/disassembly/001c60d0.asm
[model-begin]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-shadow-lighting/functions/000e9970.c
[fake-caller]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003/disassembly/001ce670.asm
[fake-builder]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003/disassembly/001cc280.asm
[dynamic-schedule]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003/functions/000f7f20.c
[direction]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-shadow-lighting/functions/000e2690.c
[current-lighting]: /home/eric/Developer/Halo-2-Decompiled/src/object_lighting.cpp
[opaque]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-shadow-lighting/disassembly/001caf90.asm
[casting]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-shadow-lighting/disassembly/001cb000.asm
[shadow-only]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-shadow-lighting/disassembly/001cb070.asm
[transparent]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-shadow-lighting/disassembly/001cb0d0.asm
[part-admission]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003/functions/001cd8a0.c
[sections]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-shadow-lighting/functions/000e9b40.c
[caps]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003/functions/0017b4d0.c
[runtime-lights]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003/functions/000f8050.c
[direct-receiver]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003/functions/000e8ca0.c
[light-place]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-shadow-lighting/disassembly/000e4dc0.asm
[scenario-light]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-shadow-lighting/disassembly/000e4ad0.asm
[scenario-geometry]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-shadow-lighting/disassembly/000df6e0.asm
[current-points]: /home/eric/Developer/Halo-2-Decompiled/src/pstencil/point_lights.cpp
[oracle]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/live-v3/build/pstencil/original-shadow.json
