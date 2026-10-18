# Projected-light vertex attribute bank

The late opening scene exposed two missing inputs in the raw projected-light
backend: V10 and V11. The cinematic armory's `panel_mech_gunbarrel` material
supplies diffuse RGB `(0.44, 0.54, 0.59)` and specular RGB `(0.4, 0.4, 0.4)`.
The compiled shader reads RGBA. The complete existing per-pass constant bank
owns alpha; RGB remaps preserve it. This bridge copies that completed bank,
including alpha, without selecting new material values.

ABI3 appends sixteen constant attributes at byte3104 to the complete ABI2
3104-byte view/constant prefix. Its total UBO extent is3360 bytes. Programs
that read external V10/V11 require explicit component coverage. Other external
attribute registers remain rejected. The build-time SPIR-V validator checks
both layouts, fetch declarations, descriptor location, output locations and
consumed-register sets.

The optional emitter also handles the actual DPH/MOV and ARL/MOV combinations
in these projected programs. Paired address loads retain the old address until
the ILU output completes. Ordinary ABI2 source generation keeps its previous
instruction corridor. The normal catalog retains its120 existing ABI2 modules
and adds33 distinct ABI3 programs from `shaders/pstencil/native-programs`.
All120 original SPIR-V hashes are unchanged.

## Verification

`analysis/pstencil/opening-campaign/native-lights/attribute-v1` retains the
source generator, compiler commands, unchanged-original-emitter fixtures and
GPU readbacks. The explicit original-emitter maintenance tool is
`tools/pstencil/prepare_projected_vertex_reference.py`; its GPL comparison
shader text stays in analysis artifacts and is not production shader source.

The GPU comparison covers actual raw inputs for classes0 through6 with explicit
test palettes, the real material bank, and whole original light-uniform output.
All548811 captured words match the independently retained original emitter.
All14 V10/V11 zero-bank controls change the output. Vulkan validation reports
zero errors. The separate CPU arithmetic comparison covers258264 components;
it uses a relative tolerance because reciprocal arithmetic is not an Xbox
hardware-precision oracle. The existing menu source/catalog regressions pass.

These are isolated shader/input proofs. The declared test identities/palettes
are not live actor admission, animation, a matched Xbox framebuffer, or complete
material-initialization parity. The unchanged current material bank is the
bridge's input; this work does not independently establish every original
stateful alpha-bank initialization.

## Runtime acceptance

The installed live-v14 launcher remains unchanged. Campaign-v11 reached the
late scene and exposed the paired-instruction gap. Campaign-v12 contains the
new bridge and mixed ABI2/ABI3 catalog. Its replay exposed the separate raw
fragment fog-alpha binding, documented in PROJECTED_FOG_ALPHA.md. Campaign-v13
adds that binding and is undergoing full native replay and image controls. Recording, high-resolution
captures, full May2003 material/MRT presentation, and source-backed coverage of
all opening-scene lights remain acceptance gates.
