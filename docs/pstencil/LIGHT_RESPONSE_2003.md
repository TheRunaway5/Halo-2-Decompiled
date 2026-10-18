# Recovered 2003 light response

The original utility and dependent-light programs now have a native program
constructor in `include/halo2/pstencil/light_response_2003.hpp` and
`src/pstencil/light_response_2003.cpp`. The retained native candidates now use
these programs in separate utility/dependent render passes with a real RGBA8
intermediate target, the current full texture-mapping target and the existing
per-light stencil test. The dedicated campaign launcher selects the verified
campaign-v4 integration; the general gameplay launcher retains live-v14.

`light-response-native-v2` proves the sphere integration with actual Cairo
group and visor captures, rendered internally at 5120×3840. Chief, Arbiter
and the regular Elite use both recovered passes. Its point-shadow control
darkens 426,246 group pixels without brightening any channel; its specular
control changes 264,893 pixels with no negative contribution. Original is
byte-identical to the previous capture and restores exactly after pStencil.
The visor close-up has a separate native reflection control. These are native
current-game images, not original Xbox framebuffer comparisons.

The next candidate adds the [projected constant/gel adaptation](PROJECTED_LIGHT_RESPONSE_2003.md)
needed by all five actual opening cinematic light definitions. Static authored
2D gels and their specular masks stay attached to their real source lights.
Unsupported moving/animated/volume gels retain the existing route.

The constructor retains complete outputs of unchanged original `000964B0` for
builtins 6, 10 and 7. Its generated table covers all 560 declared oracle inputs:
64 distinct specular utility programs, 64 distinct dull utility programs and
24 dependent-light programs. Every output word matches. The utility table
coalesces geometry contexts 1..3, and coalesces quality values -1, 1 and 2.
Quality zero selects the other branch. These equivalences are checked over
the complete corpus rather than inferred from the decompiler's signed types.

The dependent constructor accepts the original caller's packed RGB colors.
It does not substitute an unverified color-packing rule. Its completed light
colors, material attributes, gel and receiver selection remain caller inputs.
Unknown selectors are rejected.

`pack_light_rgb_2003` separately implements original `0002F380` with its
`023F` x87 control word: exact double-precision RGB×255 products followed by
nearest-even integer conversion. It matches 1,023 unchanged-original cases,
including adjacent float values around all 255 channel thresholds. The
combined constant/packing oracle authenticates 548 executed instructions.

## Actual image and vertex inputs

Original texture override 9 in `00096200` loads rasterizer globals `+0C`.
The actual resident datum is `E23F00CB`, `rasterizer\distance_attenuation`.
It is a 32³ I8 volume with six XYZ-swizzled mip levels. The original padded
allocation is 37,504 bytes; the mip payload consumes 37,449 bytes. All 149,796
decoded RGBA bytes match the existing current-game attenuation image in both
Cairo Station and the opening campaign cache:

`1179229ab6bd7e9be0794da2f11520054d95b6299b22cc65f1027908342a6207`

The original normalization cube also matches the current image; see
[visor binding evidence](COMBINED_VISOR.md). No replacement lookup formula or
newly authored bitmap is needed for these inputs.

Unchanged `000981C0` executes twelve world-vertex selection fixtures, with
137 authenticated instructions. For sphere context zero it selects utility
program `36` for tangent-space materials, `44` for accumulated normals, and
dependent-light program `28`. The corresponding non-sphere selections are
`2F`, `3D` and `25`.

| Input role | Original tangent utility `36` | Current authored sphere specular program |
| --- | --- | --- |
| Bump UV | Ready UV in V3 → oT0 | Current C18/C20 UV remaps → oT2 |
| Light vector in tangent basis | oT1 | oT1 |
| Eye vector in tangent basis | oT2 | oT3 |
| Spherical attenuation coordinate | oT3 | Can be constructed from current world position, C31 and C35.w |
| Tangent basis inputs | V6, V5, V4 | V4, V3, V2 |
| Light position | C15 | C31 |
| Eye position | C4 | C7 |

Original utility36 computes its falloff coordinate as
`0.5 - (light_position - position) * C19.w`. The original sphere builder
stores `C19.w = -0.5 / radius`, matching current `C35.w`. The sign comes from
the actual original `003E22D0` value (-1), not from a guessed attenuation
formula. The current material's authored UV transforms remain an explicit
adaptation at the input boundary.

## Constant and GPU checks

The unchanged original sphere constant builder `000A3CF0`, called at
`000A4E75`, executes 256 declared input records using its real EBX/ESI and
seven-stack-argument ABI. No instruction or service is replaced. Its 457
executed instructions are authenticated against the original executable.

For the supported ordinary sphere branch, 7,424 mapped scalar outputs match
the current constant builder exactly. Three lanes keep separate ownership:
original C20.xy preserve their caller values while current C36.xy clear;
original C21.w writes its rasterizer global while current C37.w carries its
caller value. The utility36 program and final RGB color packer do not read
these lanes. This does not establish compatibility for animated or projected
gel inputs. The tests also check all four original unwritten lanes per case.

The actual authored current Chief programs at all three LODs pass 768 vertex
comparisons per cache, in Cairo and the opening: light/eye tangent vectors,
original falloff coordinates, and the explicit current bump-UV adaptation.

The new compute test evaluates utility and dependent programs in sequence
for 4,480 cases, including all 560 constructor rows. All 8,960 stage results
match the independent scalar register-combiner evaluator, with maximum
channel error `2.68221e-7`. The native Vulkan validation run emits no messages.

The intermediate fixture explicitly rounds to normalized 8-bit channels.
The original targets are linear A8R8G8B8, but this compute test does not create
the actual utility attachment, sample textures, exercise stencil receivers,
or establish original hardware conversion precision. Those remain native
renderer integration and framebuffer checks. It is not an original GPU frame
or a new in-game visual-parity claim.

Evidence, replay commands and logs are retained in
`analysis/pstencil/2003-light-inputs-v1/`. The generated program table and GPU
test are integrated into the optional pStencil CMake targets. The tested
campaign runtime and its existing 64 shaders remain pinned by campaign-v3.

## Reproduction

Use new output paths to preserve retained evidence:

```sh
.tools/python/bin/python tools/pstencil/extract_reference_light_programs.py \
  --output analysis/pstencil/new-light-programs/reference.json
.tools/python/bin/python tools/pstencil/generate_light_response_2003.py \
  --reference analysis/pstencil/new-light-programs/reference.json --check
.tools/python/bin/python tools/pstencil/extract_reference_sphere_constants.py \
  --output analysis/pstencil/new-light-inputs/sphere-constants.json
```

`pstencil_audit_light_response_inputs` retains current prepared shader banks
and attenuation images. Pass its output to
`tools/pstencil/audit_reference_light_inputs.py`; then run
`pstencil_reference_light_inputs_tests` with the audit, current directory and
sphere constant corpus. The `pstencil_original_light_response_gpu` CTest uses
`PSTENCIL_LIGHT_CONSTRUCTION_REFERENCE` when that original corpus is available,
and adds the boundary corpus from `PSTENCIL_LIGHT_RGB_REFERENCE` when supplied.

The retained native integration passes exact Original roundtrips and
same-frame light/shadow/specular controls. Remaining work includes the
accumulated-normal path, animated material/light families, complete face and
asset coverage, and comparison with a matching original GPU frame. The
constructor/CPU/compute/native-current checks establish separate facts;
they do not collectively substitute for that missing original frame.
