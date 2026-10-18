# Separate material masks for direct specular

Live-v11 adapts current independent texture passes into a separate pStencil
specular mask. It retains current levels, models, textures, shader LOD selection,
normal maps, light inputs, highlight programs and the existing Arbiter polish.
Original keeps its current rendering behavior.

## Native failure and reference boundary

The native Banshee fixture selects geometry LOD4 and shader LOD2. That shader
LOD combines its texture/environment response and does not populate the
existing texture-accumulation surface. Its final scene alpha is zero, so the
previous destination-alpha direct-specular blend contributes no highlights.
A diagnostic changing only that blend exposes a nonzero authored highlight;
removing stencil shadows alone does not. The material's base bitmap contains a
nonzero authored alpha mask, so a constant gloss value would discard useful
current asset data.

The May 2003 resident `default_light` layer17 adds RGB with ONE/ONE and reads
separate texture/utility targets. Its render-state43 is `00010101`. An unchanged
original builtin7 construction fixture also demonstrates material-alpha
multiplication in selector1. These facts support separating the material mask
from the light sum. They do not establish the complete original MRT scheduling,
all utility selectors, or the current-to-original per-material crosswalk.

Primary receipts: [resident light states](../../analysis/pstencil/elite-response-audit/light-alpha-reference.json),
[original light constructions](../../analysis/pstencil/elite-response-audit/original-light-programs.json),
[selector fixture](../../analysis/pstencil/live-v11/light_mask_probe.cpp).
The original build and maps are read-only references.

## Current-asset implementation

The pStencil material bank admits an independent current `stencil_texture`
pass when its actual LOD0 dynamic binding provides one. It retains that pass
as mask residency. The renderer pairs each combined base material with an
independent texture pass from the **same shader datum and CPU input transform**.
It draws that pass with the existing posed geometry, camera, native owner colors
and object-function inputs into texture accumulation. This supplies a mask even
when the visible current shader LOD combines textures and lighting. Ordinary
visible shader LOD selection remains native.

Supported direct-specular draws sample the matching accumulation pixel's alpha
and multiply the final authored RGB by it before additive blending. Their
seven-combiner program remains byte-identical: texture modes, scratch registers,
normal/gel samples, lobe, tints and completed light constants survive. The
fragment-stage multiplication does not overwrite T0 or append a combiner.
The draw retains the same stencil test and depth behavior.

Materials without an admitted independent texture pass keep the previous
current destination-alpha path. Spectre is an explicit control for this case.
Current diffuse accumulation state is retained: imposing the 2003 RGB-only
state on unsupported current combined materials suppressed Spectre specular.
That state mapping was rejected rather than generalized outside its original
separate-target context.

Code stays in [specular_mask.cpp](../../src/pstencil/specular_mask.cpp),
[material.cpp](../../src/pstencil/material.cpp) and
[specular_mask.glsl](../../shaders/pstencil/specular_mask.glsl), with small
renderer/fragment seams for residency, owner bindings, accumulation and sampling.

## Verification and limits

The focused suite exercises all four actual Banshee shader LODs, rejects a
foreign shader mask, and checks 3,584 independent material-alpha cases against
the unchanged authored scalar program on the GPU. The armor adaptation test
contains 9,216 total GPU cases. Native captures isolate specular, independent
casters, point shadows, baked shadows, visor reflection and Original restoration.
Vehicle controls use the same pStencil frame with casters removed; they do not
confound shadowing with an Original-versus-pStencil environment difference.

An isolated Banshee camera/light fixture produces direct specular while its
Original diffuse contribution is zero. Both results are reported separately;
the specular and shadow controls must still make visible contributions.
Banshee, Warthog, Ghost, Spectre, Scorpion and Wraith pass native controls.
Battle Rifle, Plasma Rifle, Needler and Brute Shot also pass at 1920x1440.
The smaller 960x720 Battle Rifle/Plasma Rifle fixtures had positive specular
contributions below the diagnostic's 100-channel threshold. Resolution was
increased with the source query/raster viewport; the threshold and materials
were retained. Native netgame rows construct the pickups. Their actual item
flags, weapon flags, power, heat and signed magazine fields feed the same
`SourceWorldWeaponShaderInput` used by the game, rather than unit vitality.

Vehicle and weapon pictures are native controlled owner diagnostics. Armor photos are
actual staged owners in the loaded Cairo level. Neither is a matched 2003
framebuffer capture.

The existing thin horizontal floor lines also appear in the Original Cairo
control and are not introduced by the pStencil shadow masks. This change does
not establish full MRT/utility/combined-response/ISQ/manifold parity or original
object admission. Those remain separate reference work.

The ordinary launcher also reports existing direct-attachment light boundaries:
named object-function-driven weapon lights and timed attachment updates are
not supplied by the constant-light pStencil owner. These concern emitted
attachment lights; the receiving material controls above pass. Constant object
point lights and grenade glow retain their existing live path.
