# Source37A60 plate caller profile and projection

`source_weather_plate_caller_profile` is a pure bounded helper over the actual
52byte compositor record produced by37A60, depth limits and its two independent
camera banks. It generates the complete60word pixel definition, selected depth,
source quad Z/W and source raster classification. It owns the literal original
arguments internally: mode3, center0, anchor_end0, anchor_start1, skip_state1,
and a requested optional depth. The existing source program builder, graphics
guard, shaders, fixtures and GPU result remain unchanged.

The source authority is immutable retail XBE
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`
and unchanged-original37A60/SDK execution. The new helper's fixture uses no
native arithmetic or outputs as its oracle. The source proof is retained in
`analysis/multiplayer-shading/weather-plate-program-admission/derived-caller-proof.json`.
It derives the exact37A60 entry by replaying the real captured36B50 entry's
unchanged prefix, then executes14 actual/control callers to real return36ECF.
The replay substitutes no instructions, callees, readiness, GPU words or MMIO.
This is bounded original CPU/SDK evidence, not physical Xbox or live GPU proof.

## Interface and source operation order

Inputs expose selection near/far at485658/48565C separately from projection
near/far at485AD4/485AD8. The actual entry holds1/64 and1024 in both banks;
independent controls prove that the packed program and projected quad consume
different banks. The helper does not assume those banks are equal.

The compositor profile follows literal37A60 construction: byte0 equals1;
each of the first three float3 values is`{+0,alpha,+0}`; the alpha and final tint
lanes are finite in the caller's clamped[0,1] range. The ignored padding bytes
remain ignored. The actual first record word is`4B7FFF01`; replacing this
profile check with a fullword equality to1 would reject real source input.
The helper accepts the already produced compositor record; it does not replace
the producer of the caller's layer factors or tint.

On source3A5C0 false return, the result is `NoDraw` with all optional fields
empty. Otherwise the helper returns the exact complete program/depth and
executes the projection operations from37DBE..37E45, each rounded separately
to binary32, in original order:

```
span = projection_far - projection_near
scale = projection_far / span
scaled_depth = scale * selected_depth
scaled_near = scale * projection_near
numerator = scaled_depth - scaled_near
divided_z = numerator / selected_depth
Z = divided_z * 16777215
divided_w = selected_depth / projection_far
W = divided_w * 16777215
Z and W each clamp to [0,16777215]
```

The actual caller yields Z/W`4B7F9A99`/`481FFFFF`. The equal-near and raw
selected-below-near controls yield`00000000`/`437FFFFF`; their selected depth
is clamped to the near bank before this projection. Finite operands whose
derived projection becomes Inf/NaN are explicitly rejected. This is an
unsupported arithmetic classification, not an invented original false return.
No additional authored tint, alpha or depth threshold is introduced.

The source profile is `AboveNearWDepth` when returned depth exceeds selection
near. At the original quad, depth testing and control0 Z-perspective are
enabled, LEQUAL is retained, alpha is GREATER0, blending is ONE/INV_SRC_ALPHA,
RGB is writable, and depth writes are disabled. A returned depth equal to near
is `NearDepthDisabledUnsupported`: original depth testing and control0
Z-perspective are disabled. The existing Vulkan plate path implements only
the above-near profile and must continue rejecting that near profile.

The result is a source raster classification, not a GPU admission token.
Finite source projection clamps can produce zero W for other controlled bank
values; a subsequent graphics admission still needs the existing positive-W,
finite executed-VS, exact quad/surface/clip and binding checks. Public result
fields must not be accepted as an independently certified program or image.

## Validation

The independently exported fixture has14 complete original cases:

| Original case family | Cases | Source result |
| --- | --- | --- |
| Actual derived entry, all8 above-near classes, independent projection/selection banks | 11 | Above-near W-depth |
| Equal-near and raw selected below-near | 2 | Near depth disabled, graphics unsupported |
| Collapsed depth interval | 1 | No draw; original prior program preserved |

Each fixture comparison checks65 words: presence, source profile, selected
depth, Z/W and all60 program words. The false case represents absent optional
outputs with sentinel words; the source proof independently verifies the real
retained program and absence of a quad. The sentinels are not substituted
original stack values. Both normal and full ASAN/UBSAN/LSAN builds pass all14
cases (910 compared words each) and38 classification/control checks.

Controls cover wrong compositor flags, ignored-padding acceptance, non-positive
zero X/Z lanes, alpha/tint outside the caller profile, nonfinite source inputs,
finite inputs causing nonfinite projection intermediates, singular camera
divisions, the near unsupported profile, original no draw, source arithmetic
controls, non-nearest host rounding, and host DAZ/FTZ. Rejection is checked
without modifying the original retained input; its subsequent result remains
identical. These are pure CPU controls and do not establish Vulkan descriptor
preservation, which remains covered by the separately frozen scene fixture.

The translation unit enforces `FLT_EVAL_METHOD==0`, IEEE binary32 and no
fast-math. The build script uses `-ffp-contract=off -fno-fast-math` and strict
warnings. The reused producer checks source PC53/nearest controls and actual
host nearest/gradual arithmetic; supported host control inspection is x86 SSE,
including MSVC x86/x64 macro branches. Unsupported host inspection remains an
explicit rejection. GNU x87 excess precision and fast-math compile controls
both fail as required. The present run is native Linux x86-64, with no new
Windows, non-x86 or hardware proof.

Commands:

```
python3 tools/export_source_weather_plate_caller_fixture.py
bash tools/build_source_weather_plate_caller.sh
ASAN=1 bash tools/build_source_weather_plate_caller.sh analysis/multiplayer-shading/weather-plate-caller-profile/build-asan
python3 analysis/multiplayer-shading/weather-plate-caller-profile/verify.py
```

The test executable accepts one fixture path or `--controls`. Evidence and
logs are in `analysis/multiplayer-shading/weather-plate-caller-profile/`,
including fixture provenance, validation, input and artifact manifests.
The two prior plate input manifests verify unchanged:19 CPU-program inputs
and57 scene inputs. Neither the existing pure builder nor graphics module,
shaders, existing fixtures, existing manifests, CMake or runtime is edited.

This helper closes the bounded literal caller-argument and projection seam.
A future typed graphics preparation must derive these outputs from these
inputs, validate all60 words and quad Z/W before renderer mutation, reject the
near unsupported profile, and retain the proved VS/alpha/bindings/alias state.
Representative original/native GPU comparisons across the expanded program
classes and actual live current-bank/resource producers are separate required
boundaries. See `SOURCE_WEATHER_PLATE_PROGRAM_ADMISSION.md` for that review.
