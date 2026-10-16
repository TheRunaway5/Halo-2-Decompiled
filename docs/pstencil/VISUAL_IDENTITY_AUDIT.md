# May 2003 visual identity audit

The strongest additional candidates are **material-specific direct response,
bump/texture accumulation, and view-dependent environment response**, followed by
authored detail/change-color, emission and transparency. These can use the
current geometry, textures, animations and levels. A single global gloss or
contrast adjustment would discard distinctions present in the primary data.
This document is an evidence/adaptation audit; it adds no rendering behavior.

## Evidence and scope

Reference: the read-only May 3, 2003 `game/halo2debug.xbe`, SHA-256
`5a9edb4c874a782594c86020db49b0086f44ae38f53ff2276b3192a2c2383d18`,
and matching `tools/bin/halo2debug.map`. The section-resolved provenance and
function hashes are in [the material export][provenance]. The accompanying
[read-only inventory][inventory] records hashes and byte offsets for 362 loose
tags, template path fields in 537 loose `.shader` files, and resident
object → model → render-model → material links in five current Xbox caches:
Coagulation, Lockout, New Mombasa, Delta Approach and High Charity.

Evidence labels used below:

- **Known code**: an identified executable branch, call, state or texture binding.
- **Known data**: a literal serialized path/parameter name or a resolved current
  cache reference. Loose-tag path fields establish authored dependencies;
  they do not prove that a material was loaded or selected in a particular frame.
- **Adaptation**: a proposed use of current assets, explicitly separate from
  historical equivalence.
- **Unresolved**: execution/selection, parameter meaning, or identity not proved
  by the inspected evidence. No missing loose pathname proves an object absent
  from all 2003 content, and no similar object name proves a historical alias.

The current mode already has stencil volumes, authored layer-13 direct specular
and existing environment/emission paths. [The material bank][material-bank]
assembles the actual pass with the shader's selector and remaps; it does not
substitute a guessed exponent. [The current world/model binder][world-binder]
already submits independent environment and self-illumination layers. These
are existing capabilities to preserve, not additional features to count twice.

## Executable-backed stages worth extending

| Candidate | Primary evidence | Adaptation using current assets; remaining limit |
| --- | --- | --- |
| Bump and texture accumulation | `render_layer_bumpaccum` (`000E8040`) selects target 9; `render_layer_texaccum` (`000E80B0`) selects target 11. The environment material branches first try `rasterizer_shader_draw_geometry` (`0023A1B0`), then the legacy fallback: bump at `001D056F`, texture at `001D0B55`. [Bump layer][accum-layers], [texture layer][texaccum-layer], [bump branch][bump-branch], [texture branch][texture-branch]. | Feed current authored bump/base/detail passes into the matching staged architecture while retaining posed positions and material boundaries. This is a larger candidate than the delivered shadow mode: exact target encodings, blend rules, alpha ownership, normal transforms and readback must be recovered before claiming the 2003 accumulation pipeline. |
| Per-material direct light utility/response | `render_dynamic_light` (`000F7F20`) schedules shadow work, then utility and lighting submissions. `environment_dpndnt_light_shader` (`001D3090`) reads material flags/fields, binds accumulation targets 11 and 10 as textures, and constructs a conditional combiner program. [Per-light scheduler][light-scheduler], [dependent-light function][dependent-light]. | Preserve each current shader's direct-light pass, specular selector, masks, authored constants and actual light inputs. The 2003 response-tag families below are candidates for exact recovery, not permission to assign arbitrary dull/shiny multipliers. Do not equate the 2003 layer numbers with retail layer 13. |
| Environment response with separate normal/glancing tint and strength | `shader_stencilshadow_get_environment_map_properties` (`00147810`) returns one texture, two three-component vectors and two scalar strengths, with flag-controlled overrides/scaling and an admission check. `rasterizer_stencilshadow_env_dprs_model_section_draw` (`001CF010`) calls it, supplies vertex attributes 10/11 and binds the returned texture at stage 3 before the draw. [Getter][env-getter], [model draw][env-model]. | Reuse current authored environment maps and their tint/mask/remap data. The 2003 templates explicitly name `env_tint_color`, `env_glancing_tint_color`, both brightness controls, and midrange controls. Recover the pass math before adding a 2003 alternative. Do not assume the legacy getter covers every template path. |
| Detail texture and change-color composition | `metal/gun_metal` names base/detail/change-color maps; `default_simple_two_change_color` references `default_texture_two_change_color` at file offset `0x955`. [Gun-metal template][gun-metal], [two-color template][two-colors]. | Keep current base/detail textures and current object color context. Bind through authored remaps; do not recolor an entire mesh from its faction or basename. The current loader already applies per-object color data. [Current color application][colors]. |
| Independent emission and transparent surfaces | `render_layer_self_illumination` (`000E8150`) has its own target/state setup. Environment emission first tries the generic shader at `001D1AF7`; environment transparency has its own structure callback at `001D2CD0`. [Emission layer][emission-layer], [emission branch][emission-branch], [transparent callback][transparent-callback]. | Retain current emissive maps/animation and transparent materials separately from opaque point-light receivers. Screens, plasma, glass and shields must retain their own blend/alpha and sorting behavior. Authored emission alone is not evidence of a light that illuminates neighboring geometry. |

The environment code demonstrates **both a template renderer and a legacy
fallback**: material callbacks try the generic shader draw before entering
hard-coded legacy setup. A loose `.shader_stencilshadow`, `.shader`, and template
with related names cannot therefore be treated as three simultaneously active
passes. Exact selected paths remain a frame-validation requirement.

## Response and template inventory

All paths in this table are beneath
`/mnt/SSD4TB/Leaks/Halo 2/2003/tools/tags/`. Offsets are **file byte offsets**,
not in-memory struct fields. The inventory preserves every listed source's hash.

| Primary file | Known serialized dependencies and controls | Interpretation boundary |
| --- | --- | --- |
| [`shaders/light_responses/specular_dull.shader_light_response`][response-dull] | `light_utility_specular_dull` at `0x1DD`, its tangent-space variant at `0x2A1`; `default_light` and old lightmap paths. | Distinct authored response family; exact lobe/combiner behavior is not derived from the word “dull.” |
| [`shaders/light_responses/specular.shader_light_response`][response-normal] | `light_utility_specular` at `0x1DD`, tangent-space variant at `0x29C`; `default_light` and old lightmap paths. | Ordinary named response, separate from dull/shiny. No conversion to modern roughness is established. |
| [`shaders/light_responses/specular_shiny.shader_light_response`][response-shiny] | `light_utility_specular_shiny` at `0xCC`, `default_light` at `0x10E`, `old_lightmap` at `0x141`. | This file does **not** contain the shiny tangent-space path. That loose pass exists separately; its presence does not establish a link from this response tag. |
| [`metal/gun_metal.shader_template`][gun-metal] | Ordinary response `0x7EF`; `default_texture` `0x880`; `anisotropic_bump` `0x8B6`; `default_environment_map` `0x8EA`; default gun-metal reflection path `0x679`; separate tint/glancing/midrange controls. | Useful architecture for weapon materials, not a mandate to replace their current reflection texture or authored values. |
| [`metal/smooth.shader_template`][smooth] and [`metal/dirty.shader_template`][dirty] | Both link ordinary response, default texture, anisotropic bump and environment mapping. Dirty includes a metal-dirty detail map; exact references are retained in the inventory. | Distinct authored parameter sets should survive. Template names alone do not classify a current surface. |
| [`metal/covenant/simple_black_white.shader_template`][covenant] | Ordinary response `0x7C5`, anisotropic bump `0x88C`, environment map pass `0x8C0`. Its illumination sibling additionally links self-illumination. | A supported material family for comparison, not proof every Covenant vehicle uses it. |
| [`metal/covenant/shiny_self_illumination.shader_template`][covenant-illum] | Despite its name: **dull** response `0x8C9`, **default_bump** `0x9B5`, environment `0x9E5`, self-illumination `0xA27`. | Strong counterexample to selecting light response or anisotropy from a template basename. |
| [`equipment/rubber.shader_template`][rubber] | Ordinary response `0x7C5`, rubber detail path `0x3E7`, anisotropic bump `0x88C`, environment pass `0x8C0`. The scan finds one loose `.shader` user: `scenarios/shaders/human/urban/interior/int_redpartition_trim.shader`. | “Rubber” is a template family, not evidence that every tire/grip uses it. |
| [`glass/default_glass.shader_template`][glass] | Ordinary response `0x4C5`; transparent pass `0x516`; explicit environment/specular/self-illumination maps and alpha-blend map/color/opacity names. Four loose `.shader` users are recorded. | Preserve transparency and masks; do not force glass into the opaque stencil-light material path. This does not prove refraction. |
| [`bump_mapping/anisotropic_bump.shader_pass`][anisotropic] | `bump_map` at `0x18D`, default vector texture path at `0x196`, scale controls at `0x1C5/0x1D2`. Gun-metal, smooth, dirty, rubber and the ordinary Covenant template link this pass. | This is evidence for recovering a named bump pass. It does **not** yet establish a modern anisotropic specular BRDF, a tangent direction convention, or merely anisotropic texture filtering. Recover the actual shader/program and bindings before implementing it. |

An important consistency check: Warthog `tires.shader` links **metal/dirty**,
and SMG-dual `smg_rubber_grip.shader` links **metal/gun_metal**, both at `0x90`.
Ghost `ghost_shell.shader` and Phantom `shiny_shell.shader` link
**default/default_simple**, not the Covenant family. The [actual tire][old-tire],
[grip][old-grip], [Ghost][old-ghost] and [Phantom][old-phantom] files own these
facts. A renderer family classifier must follow authored data, not surface names.

## Applying the identity to current vehicles, weapons and props

The following are practical **adaptation mappings**, not reconstructed 2003
object identities. Current links and bitmap inventories are proven resident
metadata; selected live regions, LODs, passes and transparency admission still
need validation. Exact shader datums/file offsets are included to make the
examples checkable in the original current caches.

| Current object/family | Known current and 2003 evidence | Adaptation; unresolved issue |
| --- | --- | --- |
| Warthog, Scorpion, Pelican; human hardware | The 2003 Warthog render model names separate tire, metal, vinyl, plastic, glass, light and turret materials. Current Coagulation tire shader `E75D05E8` at `0x24C8EDC` uses `opaque/tex_bump_shiny`, with current tire bump/base and rubber detail dependencies. | Apply 2003 stages per current material/part; preserve current hull/tire/detail/clear-surface distinctions and damage variants. Do not assign `equipment/rubber` solely because a part is a tire. |
| Ghost, Banshee, Wraith, Phantom; Covenant vehicle shells | Actual 2003 Ghost/Phantom shader paths above prove generic-template use with environment/bump inputs. Current Phantom exterior in New Mombasa uses `tex_bump_env_no_detail`, carapace reflection and separate antigravity/illumination materials. | Reuse current shell normals/base/reflection maps; retain independent emission/transparency. Family-level styling is an adaptation, not a claim that all these objects shared a 2003 template. |
| Spectre and its shields/damage surfaces | Current Coagulation shader `F03C0EC7` at `0x270688C` uses `tex_bump_env_no_detail`, Spectre base/bump and carapace reflection; its other materials include illumination, plasma shield, alpha-tested torn shell and damage emission. The loose 2003 tree has separately named Shadow and Creep vehicle/model data. | Treat each current material using its authored inputs. A Spectre → Shadow/Creep historical alias is **unresolved**; do not copy an entire old object's materials or geometry on that assumption. |
| SMG/battle rifle/magnum/sniper and other human weapons | 2003 SMG-dual and energy-blade handle link gun-metal. Current SMG bare-metal shader `E992081D` at `0x2568328` uses `tex_bump_env` and a monochrome gun-metal reflection map, while painted metal has a distinct template. | Preserve current bare/painted/clear/plasma parts and the current normal/specular masks. An exact 2003 gun-metal response is a candidate only after shader recovery; it is not a single preset for all weapon parts. |
| Brute plasma rifle and beam rifle | Current Coagulation shaders `EE250CB0` (`0x2656B24`) and `EE780D03` (`0x2667958`) use `tex_bump_env`. Brute plasma materials explicitly share ordinary plasma-rifle bump/emission resources while retaining their own base texture; beam rifle has separate ammo/plasma materials. The 2003 ordinary plasma-rifle shader actually links the Covenant `simple_black_white` template. | This provides a defensible Covenant weapon material comparison for later/differently named content. Keep the current maps and pass boundaries; exact 2003 counterpart identities remain unresolved. |
| Current `flak_cannon`; 2003 fuel-rod naming | High Charity has separate dull (`E7C3064E`, `0x10094F0C`, `tex_bump`), shiny (`E7C60651`, `0x100952A0`, `tex_bump_dprs_env`) and glowing (`E7C70652`, `0x100954C8`, `tex_bump_illum_bloom`) shaders. The 2003 `fuel_rod_gun_metal.shader` links gun-metal at `0x90`. | Carry these distinct current material roles through the 2003-style stages. A name/gameplay resemblance does not prove exact historical asset identity or authorize replacing the shiny/glowing parts with gun-metal. |
| Energy sword/blade and assault bomb/objective props | The 2003 energy-blade handle is gun-metal, top/bottom are animated transparent-add template users. Current handle `EEB90D44` (`0x26733DC`) uses `tex_bump_env`, but the blade has separate `plasma_mask_offset` materials. Current assault bomb `EE0A0C95` (`0x2651688`) uses opaque bump plus a separate scrawl overlay. | Keep plasma, handle and overlay materials separate. No exact loose 2003 assault-bomb pathname was found in this audit; that is not proof the object/concept was absent. |
| Scarab, large props, crates, screens, lights and glass | Current New Mombasa Scarab hull shader `E56803F3` at `0xDA32F98` uses `opaque/prt_scarab`; other materials independently provide illumination. This is a concrete counterexample to treating every object as the ordinary dynamic-light family. 2003 glass/light/template data supports separate transparent/emissive roles. | Retain current baked/PRT/base contribution and material classification, then add only supported admitted direct/environment stages. The existing PRT binder has explicit bounded admission checks; do not claim complete Scarab support from a successful ordinary prop or armor test. |

Current cache sources: [Coagulation][coagulation], [New Mombasa][mombasa],
[High Charity][highcharity]. Full current shader names, bitmap references,
template passes, available-layer masks and cache hashes are in [the inventory][inventory].
These examples retain current assets and levels throughout.

## Implementation and verification gates

1. Keep the renderer contract at **actual shader datum + selected material LOD +
   authored pass/implementation + current object color/light context**, as the
   existing material bank does. Family names are audit aids, not runtime truth.
2. Recover the 2003 response/anisotropic program and remaps before proposing
   numeric material changes. Use current base/normal/detail/specular/change-color/
   emissive/environment resources only where their authored bindings establish
   their meaning; a bitmap basename does not establish an alpha-channel mask.
3. Capture one known material through the original 2003 draw path, with the
   actual selected template/legacy branch and all target/binding/state inputs.
   Compare exact stage outputs before extending coverage to current materials.
   Keep physical 2003-frame comparison separate from host functional tests.
4. Validate ordinary, inside/near-plane and first-person views; posed vehicles,
   weapon/prop parts, transparent isolation, changing colors, damaged variants,
   resizing and Original → pStencil → Original restoration. Preserve alpha
   channels used by direct specular/transparent composition.

Fog, depth of field and screen-space effects have named executable callbacks in
the exports, but their visual identity and scene-specific authored parameters
were not traced here. Leave them unresolved instead of adding a global grade,
blur, bloom amount, refraction or replacement lighting from filename evidence.

Reproduce this metadata audit without changing the reference or renderer:

```sh
python3 tools/pstencil/audit_visual_identity.py \
  --output analysis/pstencil/visual-identity-audit-new/evidence.json
```

The tool rejects an existing output and a mismatched reference XBE hash. Its
successful run validates inventory access/relationships, not framebuffer parity,
live pass admission, recovered anisotropic mathematics or complete object coverage.

[provenance]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-materials/build.json
[inventory]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/visual-identity-audit/evidence.json
[material-bank]: /home/eric/Developer/Halo-2-Decompiled/src/pstencil/material.cpp
[world-binder]: /home/eric/Developer/Halo-2-Decompiled/src/world_material.cpp
[accum-layers]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003/functions/000e8040.c
[texaccum-layer]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003/functions/000e80b0.c
[bump-branch]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-materials/disassembly/001d0510.asm
[texture-branch]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-materials/disassembly/001d0af0.asm
[light-scheduler]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003/functions/000f7f20.c
[dependent-light]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-materials/functions/001d3090.c
[env-getter]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-materials/functions/00147810.c
[env-model]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-materials/functions/001cf010.c
[colors]: /home/eric/Developer/Halo-2-Decompiled/src/appearance.cpp
[emission-layer]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003/functions/000e8150.c
[emission-branch]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-materials/disassembly/001d1a90.asm
[transparent-callback]: /home/eric/Developer/Halo-2-Decompiled/analysis/pstencil/2003-materials/disassembly/001d2cd0.asm
[response-dull]: </mnt/SSD4TB/Leaks/Halo 2/2003/tools/tags/shaders/light_responses/specular_dull.shader_light_response>
[response-normal]: </mnt/SSD4TB/Leaks/Halo 2/2003/tools/tags/shaders/light_responses/specular.shader_light_response>
[response-shiny]: </mnt/SSD4TB/Leaks/Halo 2/2003/tools/tags/shaders/light_responses/specular_shiny.shader_light_response>
[gun-metal]: </mnt/SSD4TB/Leaks/Halo 2/2003/tools/tags/shaders/shader_templates/metal/gun_metal.shader_template>
[smooth]: </mnt/SSD4TB/Leaks/Halo 2/2003/tools/tags/shaders/shader_templates/metal/smooth.shader_template>
[dirty]: </mnt/SSD4TB/Leaks/Halo 2/2003/tools/tags/shaders/shader_templates/metal/dirty.shader_template>
[two-colors]: </mnt/SSD4TB/Leaks/Halo 2/2003/tools/tags/shaders/shader_templates/default/default_simple_two_change_color.shader_template>
[covenant]: </mnt/SSD4TB/Leaks/Halo 2/2003/tools/tags/shaders/shader_templates/metal/covenant/simple_black_white.shader_template>
[covenant-illum]: </mnt/SSD4TB/Leaks/Halo 2/2003/tools/tags/shaders/shader_templates/metal/covenant/shiny_self_illumination.shader_template>
[rubber]: </mnt/SSD4TB/Leaks/Halo 2/2003/tools/tags/shaders/shader_templates/equipment/rubber.shader_template>
[glass]: </mnt/SSD4TB/Leaks/Halo 2/2003/tools/tags/shaders/shader_templates/glass/default_glass.shader_template>
[anisotropic]: </mnt/SSD4TB/Leaks/Halo 2/2003/tools/tags/shaders/shader_passes/bump_mapping/anisotropic_bump.shader_pass>
[old-tire]: </mnt/SSD4TB/Leaks/Halo 2/2003/tools/tags/objects/vehicles/warthog/shaders/tires.shader>
[old-grip]: </mnt/SSD4TB/Leaks/Halo 2/2003/tools/tags/objects/weapons/rifle/smg_dual/shaders/smg_rubber_grip.shader>
[old-ghost]: </mnt/SSD4TB/Leaks/Halo 2/2003/tools/tags/objects/vehicles/ghost/shaders/ghost_shell.shader>
[old-phantom]: </mnt/SSD4TB/Leaks/Halo 2/2003/tools/tags/objects/vehicles/phantom/shaders/shiny_shell.shader>
[coagulation]: /home/eric/Developer/Halo-2-Decompiled/assets/disc/maps/coagulation.map
[mombasa]: /home/eric/Developer/Halo-2-Decompiled/assets/disc/maps/03b_newmombasa.map
[highcharity]: /home/eric/Developer/Halo-2-Decompiled/assets/disc/maps/07a_highcharity.map
