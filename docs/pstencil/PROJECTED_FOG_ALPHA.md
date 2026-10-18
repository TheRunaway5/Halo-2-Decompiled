# Projected-light attenuation through fog alpha

The authored armory projected-light programs put operand`0x33` in final
combiner input B: unsigned inverse fog alpha. Their vertex programs write the
attenuation distance using the original light uniform bank. The existing raw
vertex epilogue and original hardware-fog parameters produce the interpolant.
This is part of the light-volume attenuation calculation.

The raw fragment adapter previously rejected register3. It now accepts an
explicitly bound alpha component. The fragment shader clamps the actual
interpolated vertex fog to`[0,1]`, matching the retained original `psh.c` pFog
construction. Fog RGB remains unavailable and is rejected by admission.

The optional pipeline specialization is part of the pipeline cache key and
must be supplied by the packet caller. The pStencil projected-light domain
binds it; the default remains disabled. The original448-byte fragment packet
layout and ordinary completed combiner-constant binding are unchanged.

## Evidence

`analysis/pstencil/opening-campaign/native-lights/attribute-v1/inspect-fog.txt`
retains the actual eight armory material programs and their inverse-alpha
operand. The retained original pixel emitter's pFog construction is in
`analysis/rendering-reference/psh.c`.

`projected_fog_alpha_tests` compares56 GPU cases across these eight authored
program structures with independently evaluated scalar combiner arithmetic.
The declared test shader switches and deterministic nonzero colors/samples
are fixture inputs. All eight attenuation controls respond to fog alpha.
They do not substitute for a live framebuffer comparison.

The existing original fragment-admission fixtures pass: eight banks, five
families and896 compact packet words. The1080 rejection controls include
unbound fog-alpha and fog-RGB reads. Campaign-v12 exposed the missing binding
before a projected-light GPU capture completed. Campaign-v13 contains this
fix; its full replay and shadow/light image controls remain runtime gates.

## Completed native acceptance slice

Campaign-v13 completed all13 main scenes with zero script/validation errors.
Its1280x720 replay retains eight same-completed-frame light/shadow controls.
All eight show projected lighting and cast-shadow darkening; none has a brighter
shadow channel. The first pair changes203404 light pixels and42904 shadow
pixels. The separate higher-resolution replay also completes, with four
positive light/shadow pairs and zero shadow brightening. Its actual drawable
is2560x1378; the desktop clamps the requested2560x1440 window height.

The screenshots are native framebuffer readbacks, retained under
`analysis/pstencil/opening-campaign/campaign-v13/native-highres/captures`.
They demonstrate the late Chief/Armorer light path, not completed visual
identity or recording acceptance. Screenshot controls and first-use work cause
large frame intervals, so these diagnostic runs are not recording artifacts.
The installed live-v14 launcher is unchanged.

The proposed Original framebuffer regression cases could not run because
`before-color.bin` is missing from each retained1335/1448/1880 fixture. That is
an unavailable comparison input, not a passing framebuffer regression. The120
existing vertex modules remain byte-identical, and the original CPU source,
catalog and fragment-admission regressions pass.
