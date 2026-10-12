# Original frontend emblem composition

The customization appearance screen now retains its original type9 emblem widgets and shader composition. The measured boundary is source inputs, selected bitmap pages, pixel-program words, authored sampler/blend state and UI quad placement. This does not establish whole-frame pixel parity, GPU fixed-function precision parity, or every menu transition.

## Original asset and widget path

The original `mainmenu.map` SHA-256 is `1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b`; the executable is `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.

Literal `22FC08` constructs type9 widgets from pane+60 records of36 bytes and group/grid widgets from pane+68 records of24 bytes. The former native parser read pane+60 as grids, so screen47's appearance group was absent. Skin+44 contains type9 records. The repaired parser retains flags, animation, delay, layer, bitmap page, bitmap/shader references and bounds. [Pane constructor instructions](../analysis/frontend/customization/0022fc08.asm), [literal screen definitions](../analysis/frontend/customization/emblem-definitions.json), [literal skin definitions](../analysis/frontend/customization/emblem-skin-definitions.json).

`2C8D7C` supplies an appearance pointer through item flags3 to `22F042 →2AFEAE`. The latter copies the16-byte appearance bank into each type9 widget through `2B12CA`; the absent sequence override remainsNONE. Definition flag2 suppresses a widget only when that override isNONE. The current appearance group has no such flags. [Appearance binding instructions](../analysis/frontend/customization/002afeae.asm).

Screen47's grid origin is(-271,38). `2BACBC` adds `abs(bottom-top)` to both emblem Y bounds before adding the group origin. The large bounds(0,0,-128,128) therefore draw at(49,74)–(177,202); the small bounds(96,141,64,173) draw at(190,74)–(222,106). This is the source placement, not a screenshot-adjusted offset. Both placements are asserted by the production painter test. Group layout includes type9 bounds except definition flag1, consistent with the bitmap rule. [Group placement instructions](../analysis/frontend/customization/002bacbc.asm).

## Shader, textures, colors and opacity

`2B11C9` reads the widget's own+64 opacity, writes it to `g4E696C`, supplies opaque-white immediate vertex9 and calls `22A664`. It does not multiply an additional parent opacity into this scalar. The widget's own authored animation index is retained without the decrement used by some other widget classes. [Type9 drawing instructions](../analysis/frontend/customization/002b11c9.asm), [quad/pass instructions](../analysis/frontend/customization/0022a664.asm).

The large shader is `ui\hud\shaders\ui_emblem` (`E5560336`). Its single implementation0 pass is `shaders\shader_passes\hud\waypoint_emblem`, with background texture stage0 and foreground stage1. Background has32 pages; foreground has64. The small shader uses the corresponding original small texture banks. The original sampler states are retained; blending is enabled `SRC_ALPHA / INV_SRC_ALPHA`. [Shader/texture audit](../analysis/frontend/customization/emblem-shader-audit.json).

The appearance-function inputs come from literal `14CCF0`, through `22ACE7` when no gameplay player is bound:

| Input | Original value |
| --- | --- |
| `0D0005FF` |1 |
| `170005FA` |foreground byte /64 |
| `170005FB` |background byte /32 |
| `180005FC` |flags bit0 set→0, clear→1 |
| `1D0005FD` |flags bit1 set→1, clear→-1 |
| `1D0005FE` |flags bit2 set→1, clear→-1 |
| `0C0005CB` |widget opacity at `g4E696C` |

Input0 uses `22ACE7`'s original default1; production forwards the values to the authored shader overlays rather than reproducing the emblem with a new color formula. All four appearance palette colors pass through the existing original packed object-color boundary. Pixel extern color sources3,4,21,22 map to primary player, secondary player, primary emblem and secondary emblem colors. [Appearance function instructions](../analysis/frontend/customization/0014ccf0.asm), [widget function resolver](../analysis/frontend/customization/0022ace7.asm).

A real original appearance47 draw was retained at `22A803`, after authored bindings and the UI vertex override. Its appearance bytes are `080c0901001a090000`: colors8/12/9/1, foreground26, background9, flags0. The native prepared program matches all60 recorded pixel-program words exactly. The capture also retains the actual quad and opaque-white vertex9. [Original draw](../analysis/frontend/customization/original-emblem-draw.json), [retained240-byte program](../analysis/frontend/customization/frontend-emblem-live-program.bin).

Crucially, `22A664` overrides the authored emblem vertex shader with rasterizer vertex slot20. The live rasterizer block has24 entries at `[globals+5C]`; slot20 resolves to `E4710251`, `rasterizer\vertex_shaders_xbox\screen_hud`. Its fourMOV program passes v0 to position, v3 unchanged to both texture stages and v9 to color. Thus the UI draw uses rawUV0..1 on both stages. The shader's authored1.2 transform is not applied after this override. [Actual rasterizer block](../analysis/frontend/customization/original-rasterizer-globals.json), [original screen_hud program](../analysis/frontend/customization/emblem-screen-hud-vertex.bin).

Production reuses the authored register-combiner execution in `hud.frag`, enables its second sampler only for the explicit emblem marker, and selects the centered UI viewport for these widgets. Other HUD materials preserve their full-surface viewport policy. Generic world/model animation scheduling skips emblem materials because the painter supplies their actual working appearance bank.

## Verification and boundaries

`tools/oracle_frontend_emblem.py` executes unchanged original `14CCF0` and `22ACE7`, freezing682 input/result words. `frontend_emblem_tests` checks all682 words, all60 live pixel-program words, all64 foreground page choices and32 background choices, the flag0 toggle and opacity overlay, both widget definitions and both production UI quads. `retail_frontend_emblem` passes. [Fixture report](../analysis/frontend/customization/frontend-emblem-original.json).

The state producer is separately verified by `retail_frontend_appearance`: children49/233 update the working appearance on focus; B restores the child snapshot; X toggles only foreground flag bit0; acceptance retains the edit. The emblem painter consumes that working bank. Exact visual transition clocks, all type9 instances outside customization47, and framebuffer pixel differences remain separate work. Passing these source boundaries is not a claim that all customization or frontend visuals are complete.

## Emblem on the 3D player model

The 3D model has a separate source binding. `2B10A3` verifies the appearance species, passes `appearance+5` to `2B102F`, and then applies the four color banks through `2B0F76`. `2B102F` resolves each authored scene object name and copies the four bytes into `object+24` only for biped type0. The retained original appearance Spartan object contains `1a090000` at that field, matching foreground26, background9, flags0 and reserved0. [Appearance-to-object binding](../analysis/frontend/customization/002b10a3.asm), [original object write](../analysis/frontend/customization/0002b102f.asm), [retained object header](../analysis/frontend/customization/original-preview-final-implementation-spartan.json).

The shader object-function path is `BCC20 →BC970`. It reads foreground/background/flags from `object+24/+25/+26`. Its six emblem names match the UI names, but the enclosing `BCC20` clamps signed flip outputs into[0,1], and unknown names stay outside the emblem helper. There is no UI widget opacity input in this model context. `BC970` suppresses the bank when flags bit3 is clear and a present scenario has type0; the actual mainmenu scenario has authored type2, so that suppression does not apply. `frontend_model_emblem_input` preserves this bounded menu input path independently of `frontend_emblem_input`. [Object input function](../analysis/frontend/customization/000bc970.asm), [enclosing resolver](../analysis/frontend/customization/000bcc20.asm), [unchanged original replay and asset report](../analysis/frontend/customization/frontend-model-emblem-original.json).

Both `mc_emblem` (`E24B00D6`) and `elite_emblem` (`E33A018F`) use the original `emblem_overlay_simple` pass with the64-page foreground bitmap. Their authored named overlays consume foreground/64, enabled flag bit0 and constant1. These specific armor passes contain no background bitmap or background-page input; the background remains part of the separate UI emblem composition. The authored transform curves are retained, including the Elite-specific placement, rather than sharing the UI quad transform. The frontend animation scheduler binds each biped's working appearance across its complete resident body material range, then updates both program constants and bitmap frame indices at the existing render time. Held weapons keep their own source context.

`tools/oracle_frontend_model_emblem.py` executes unchanged `BCC20 →BC970` for6,144 raw input cases, including out-of-range byte values, every flags byte and absent UI-opacity/unknown names. The extended `retail_frontend_emblem` test matches those outputs, both retained60-word model pixel-program banks, every authored shader LOD, all64 foreground pages and the model enabled toggle. The original banks were retained after the actual `16F32` binding stage. Their exact match verifies the emblem program boundary; it is not a synchronized comparison of the rendered armor pixels or a claim of complete customization parity.

## Held plasma weapon extern37

The same checkpoint found two held-plasma materials rejected by unresolved scalar extern37: `plasma_rifle_gauge` and `plasma_rifle_current`. Literal `336F0` entry37 dispatches to `33850 →33670`, using the current object handle, traversing parents while object+AA is nonzero and reading the unparented unit+2B0 value clamped to[0,1]. [Scalar dispatch](../analysis/frontend/customization/000336f0.asm), [parent context producer](../analysis/frontend/customization/00033670.asm).

A read-only original appearance47 snapshot retains all ten units and their attached weapons. All ten unit+2B0 words are zero. `tools/oracle_frontend_weapon_camouflage.py` replays the unchanged producer over those exact20 object/parent inputs with host pointer relocation only: every result is exact `00000000`. This is a source context replay; an attempted steady-state producer breakpoint did not fire, so it is not labeled a live gauge shader-draw capture. [Object snapshot](../analysis/frontend/profile-regression/original-model-customization-elite-camouflage.json), [unchanged replay report](../analysis/frontend/customization/frontend-menu-weapon-extern37-original.json).

`prepare_default_object_material`, used for classification3 held weapons, admits/remaps extern37 only when the caller supplies the proved source context. Missing context still rejects these shaders. The authored late `plasma_alpha_active_camo` pass is admitted under the same explicit condition; the bound base program survives later animated masks. `retail_frontend_menu_weapon_extern` checks both materials, absence rejection, explicit0/.5/1 scalar bindings and animation retention. General gameplay camouflage updates are outside this bounded menu binding.

The first production capture is [native checkpoint3](../analysis/frontend/customization/checkpoint-3/appearance.png). A direct comparison with the restored original47 screenshot at the literal emblem bounds retains7,899/16,384 identical large-icon pixels and393/1,024 identical small-icon pixels; mean absolute RGB differences are(.504,.824,.795) and(.271,.733,.771), respectively. The background phases and animated shell bands are not synchronized, and the source icons alpha-composite the lower scene, so these numbers do not isolate emblem GPU precision or prove100%pixel parity. The shader inputs/program/quads remain the exact independently measured boundary. [Region comparison report](../analysis/frontend/customization/emblem-native-region-comparison.json), [original screenshot](../analysis/frontend/profile-regression/original-model-customization-spartan-restored.png).
