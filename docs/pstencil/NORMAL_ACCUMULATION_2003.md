# Normal accumulation and world-space lights

The installed opening cohort under `analysis/pstencil/normal-accumulation-native-v8/`
adds the recovered May 2003 target9 producer and its normal-space light consumer.
Current levels, textures, posed meshes and authored bump UVs remain the inputs.
The normal target is generated once per view before the point/projected lights.
Each light retains its independent stencil counter and target10 utility pass;
the dependent-light pass adds the completed diffuse/specular response.

## Primary evidence

- `2003-presentation/game/functions/000e8040.c` selects target9 for layer2.
  `2003-shadow-lighting/functions/00051160.c` schedules the layered scene path.
- `2003-presentation/accumulation-construction-v1/reference.json` retains 101
  unchanged original constructor fixtures. Builtin1 has the exact 60-word
  normal program SHA `de4b7f1fc503474ccbc61cb1c3c9d6f0dac8ce45c08c463a63ce980205042cb1`.
  Its texture stages transform signed bump RGB through the posed basis, sample
  the normalization cube, and retain that cube's RGB with the bump image alpha.
- `2003-presentation/accumulation-bindings-v1/reference.json` checks resident
  original bindings for 17 shaders. Sixteen have the layer2 builtin1 producer;
  plasma grenade decal does not. Their layer16 normal utilities bind target9
  at stage0, normalization cubes at stages1/2 and attenuation at stage3.
- The original stage0 target9 sampler is nearest/clamped. The normalization
  cubes are linear/clamped. The normal bump pass uses anisotropy4 except the
  ceiling's `default_bump`, which uses linear filtering.
- Original programs1B/15/16 supply the three rows of `(binormal,tangent,normal)`.
  Original44/3D supply world-space light and eye vectors for the sphere and
  projected normal utilities. The checked original utility/dependent pixel
  constructors remain the lighting programs; no new specular formula is added.

The native source test checks the exact constructor, all 608 relevant resident
sampler fields and 256 independently evaluated final-combiner RGB/alpha cases.

## Current-asset adaptation

Exact original material names retain their recovered material constants and
sampler distinction. Later assets use the same recovered producer/consumer
with their own authored bump images, material colors and posed basis. This is
an explicit adaptation, rather than evidence that those assets existed in 2003.
Admission still requires the supported authored tangent-sphere interface, a
normalized 2D bump image, normalization cube, texture-mapping pass and a supported
light context. Unsupported light/material families retain the existing route.
The staged scene exercises Chief armor and visor, Elite head/helmet/arms/torso/
legs, and Arbiter flesh/cloth/armor/helmet/teeth. Existing metallic Arbiter
companions remain separate from ordinary Elite armor.

The private target9 and target10 images are linear RGBA8 and share the private
D24S8 attachment. Target9 writes are depth-equal with no depth writes or stencil
test. Target10 keeps each light's stencil-equal128 test. The new shaders retain
the actual current clip/W-depth/stipple outputs. All companion phases share one
posed stream per receiver instead of rebuilding it for each phase.

`HALO2_PSTENCIL_TANGENT_LIGHT_RESPONSE=1` is a diagnostic comparison that selects
the previous tangent utility path when materials are prepared. It is not a new
user-facing graphics mode.

## Verification and limits

The initial v7 source-material integration has an actual RenderDoc capture and
replay: five normal producers precede five normal consumers, followed by the
scene's dependent passes. The exported target has 192,140 written RGB pixels
and retains bump alpha from 0 to 255. Native and replay inputs/shaders are retained;
the instrumented capture tool only surrounds a real renderer frame.
The original v7 injection emitted loader messages and its harness returned 1;
the capture and replay completed. This is distinct from the ordinary native
photo runs, which returned0 without validation diagnostics.

The expanded v8 GPU capture/replay also returns 0, using the implicit capture
layer without the older loader messages. Its native image matches the ordinary
photo exactly. Fifteen normal producers precede fifteen normal utilities;
target9 contains 1,041,781 written normal pixels, its alpha spans 0..255, and the
normal utility's actual stage0 descriptor reads that resource. The producer's
stencil test is disabled and the consumer's is enabled. All 72 existing GPU
stencil/cap cases pass when linked against the new production render.o.
The diagnostic tangent route reproduces the v5 pStencil group image exactly.

The expanded v8 group and visor runs return 0. Both use 5120x3840 internal
stencil/color/depth targets and save 2560x1920 images. Original is byte-identical
to the v5 photo and to the restored Original frame. Group point-shadow controls
darken 426,390 pixels, with no brightened channels. Specular controls change
278,861 pixels; removing the Arbiter metallic companions changes 41,216 pixels.
Full opening verification and current runtime selection are recorded in
[the opening campaign document](OPENING_CAMPAIGN.md).

This is not a matched May 2003 GPU-frame certification. Original C10/C11 screen
coordinate ownership, live lighting-model selection and layer-begin clear
semantics are not yet recovered completely. The implementation uses a same-pixel
current-target retarget and initializes uncovered private target pixels to zero;
that clear is not claimed as recovered original behavior. The current base has
already written depth, so equal/no-write normal replay is a current-renderer
scheduling adaptation. Animated UV/light families, other projected shapes and
remaining presentation families keep separate unfinished gates.
