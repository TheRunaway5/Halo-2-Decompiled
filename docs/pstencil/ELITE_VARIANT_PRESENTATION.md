# Elite variants and the neutral 2003 detail response

## November 10, 2026: native shiny response correction

The current source now preserves original builtin **9** (shiny) separately
from builtin **6** (ordinary) and **10** (dull). Previously the constructor's
boolean selector represented only ordinary/dull, and its 16-material reference
bank contained no Elite entries. Consequently Elite body materials used the
ordinary response and retained later direct-light material colors.

The new [extractor](../../tools/pstencil/extract_elite_light_materials_2003.py)
authenticates six exact Elite shader paths across 18 original caches. All 108
resident records agree: normal builtin1, utility builtin9, dependent builtin7.
Head, helmet, arms, torso and legs have V10 `(0.98,0.81,1,0)` and V11
`(0,0.19,0.33,0)`; teeth use white for both. The generated bank preserves
float32 bits, exact names and original selector/color field offsets in
[reference-materials.json](../../analysis/pstencil/elite-light-response-20261110/reference-materials.json).

Fresh unchanged original execution supplies 816 complete constructor fixtures,
including all 256 shiny selector combinations and 524 authenticated original
instructions. Production now has 64 shiny identities in each of the tangent
and accumulated-normal families. All 128 Vulkan modules compile and pass the
instruction-bank-read audit. Across 16,384 native GPU/scalar fixtures the maximum
channel error is `1.01328e-6`; the complete utility/dependent suite passes 6,528
two-stage fixtures with maximum error `3.57628e-7`.

The prepared MCC opening-cache test passes 56 actual armor material-bank
completions across all five body surfaces, sphere/projected inputs and contrast
controls. It also preserves 20 authored unlit LOD cases and explicitly tests
the separate unsupported teeth boundary. A read-only cross-cache check confirms
3,420 normal-producer/shiny-utility sampler fields across 90 body-material
records in the same 18 original caches.

The first complete frozen engine and launchers built successfully. Its staged 503
pStencil/world modules pass the native instruction-bank-read audit, and its
pStencil Lockout startup returns zero with three submitted frames under Vulkan
validation. Overall engine qualification passes **124/125** tests but fails
`native_mcc_custom_mechanics_tu15`: camouflage controls and Infection pickup
controls report false. The candidate was not installed and its disposable
build/runtime were removed. The previous selection was retained at that stage.
[Engine result](../../analysis/pstencil/elite-light-response-20261110/engine-candidate/report.json),
[new-engine pStencil startup](../../analysis/pstencil/elite-light-response-20261110/native-candidate-smoke.json).

The repair run reproduces those controls against the old adapter, then passes
both focused controls and the full **125/125** engine suite with the native
adapter repairs. Its later map sweep passes 26/27 cases and exposes a separate
Needle overshield metadata admission failure. The complete literal overshield
setup is now added to the authenticated shared conversion profiles, preserving
the existing 94 profiles and fixtures. Its focused regression passes every
authored LOD with exact programs, constants, images, samplers and raster state;
an unknown extern is still rejected. [Repair evidence](../../analysis/pstencil/elite-repair-20261110/engine/report.json),
[overshield regression](../../analysis/pstencil/elite-powerup-delivery-20261110/iteration-0/report.json).

Build `d5c08bdaf2f24d7088d0c36611120498` passes **126/126** tests and **27/27**
map/player launches, including Needle, plus frontend qualification. Both
launchers select the installed engine. The selected source inventory contains
the Elite material bank and shiny identities, the native adapter repairs, and
the authenticated overshield profile. All 3,360 staged native world, fragment
and pStencil modules match the installed package and pass the instruction-bank
audit. The installed shortcut passes its Vulkan startup check after removal of
the disposable build. [Qualification](../../analysis/pstencil/elite-powerup-delivery-20261110/engine/report.json),
[installed verification](../../analysis/pstencil/elite-powerup-delivery-20261110/installed-pstencil.json).

Current teeth still lack the supported alpha-tested texture-mask producer; authored distant LODs
without layer13 retain no direct-specular contribution. Current MCC geometry,
bump maps and other images also differ from the 2003 assets. This change does
not establish full original framebuffer parity or qualified campaign delivery.
The accepted criterion for this delivery is recovered source logic; an original
framebuffer comparison is not required for installing this correction.

Reproduce the material-bank test with the owned prepared MCC opening map:

```sh
python3 tools/run_tests.py --target pstencil_elite_material_bank_2003_tests \
  --filter '^pstencil_elite_material_bank$' \
  --cmake-arg=-DPSTENCIL_ELITE_MCC_MAP=/path/to/prepared/halo2/h2_maps_win64_dx11/00a_introduction.map
```

## Earlier neutral-detail delivery

Live-v10 removes the dark charred detail sample from the supported pStencil
Elite texture passes. It preserves the current base, bump, change-color,
reflection and illumination assets, actual model variants and authored specular
passes. Original retains its existing texture response. No Elite-wide gloss
multiplier or armor recoloring was added.

## Reference evidence

The May 3 ai_test cache's resident `head`, `helmet`, `arms`, `torso` and `legs`
shader paths all bind `shaders\default_bitmaps\bitmaps\default_detail` at texture
stage 1 of their layer-4 texture pass. The read-only original loose bitmap
contains RGB(128,128,128), alpha 255: its compressed 4x4 source and the 4x4,
2x2 and 1x1 processed images agree. Hashes, exact cache offsets and all five
bindings are in [neutral-detail-reference.json](../../analysis/pstencil/elite-response-audit/neutral-detail-reference.json),
reproduced by [the verifier](../../analysis/pstencil/elite-response-audit/verify_neutral_detail_reference.py).

Current Cairo's standard Elite shaders instead bind
`objects\cinematics\covenant\elite_helmet_charred\bitmaps\bubbled_burn_detail_plate`.
Its RGB ranges from 0 to 74 and its alpha is uniformly 128. The isolated current
texture multiply doubles base RGB times detail RGB and multiplies base alpha
by detail alpha. Consequently this sample darkens the armor and halves the
alpha subsequently used by destination-alpha specular/environment blending.
[Current inputs](../../analysis/pstencil/elite-response-audit/texture-inputs/bindings.json),
[texture inspection](../../analysis/pstencil/elite-response-audit/texture-inputs/texture-comparison.png).

## Bounded implementation

[detail.cpp](../../src/pstencil/detail.cpp) admits only those five exact shader
paths, their recognized independent two/four-change-color texture passes,
texture modes 8021, stage-local constants and the isolated literal T0*T1 RGB
and alpha multiply. It substitutes the proven neutral sample through that
stage's otherwise unused C1. The current base sample, change colors, other
combiners, normal maps, output scaling and base alpha mask remain intact.
This covers ten current LOD passes. Higher combined texture/environment passes
that omit the detail texture need no such substitution. Unknown operations
and later shader paths are rejected.

The renderer applies this response only in pStencil after owner uniform
completion. Original's texture response and the on-disk maps are unchanged. The code,
tests and photo helpers remain under pStencil subfolders.

## Actual rank comparisons and capture corrections

The same loaded Cairo floor, camera, pose and declared white point light were
used for Minor, Major, Ultra, Spec Ops, Zealot, Honor Guard, Councilor and
Ranger. Rank selection goes through the native biped request and constructor;
the resulting native model-variant byte is checked against the authored
variant. Ranger uses its separate actual biped definition. The rank colors
come from the completed native owner, rather than hand-painted presets.

The comparison narrowed the dark response to shared standard body materials.
Ranger uses different materials; Honor Guard and Councilor have separate
helmet materials. The neutral policy therefore also cleans the shared body
armor of those ranks without imposing the standard armor response on their
special helmets or Ranger equipment.

The photo helpers now borrow native owner coverage to choose shader LOD
separately from the declared high-detail geometry LOD. They also evaluate the
fresh actor's authored object functions from its actual vitality/damage bank.
The object-input-only control did **not** remove the charred pattern; it was
not treated as a confirmed cause. Photo input corrections change the earlier
photo baseline, so response-only controls use the same corrected tool with
only `neutral_detail_response` bypassed at link time. The historical runtime
baseline remains live-v9 for ordinary gameplay comparisons.

[Rank receipts and controls](../../analysis/pstencil/live-v10/extra-status.json),
[canonical captures](../../analysis/pstencil/live-v10/capture-status.json),
[focused test results](../../analysis/pstencil/live-v10/tests.log).

## Remaining presentation boundary

The exact neutral-detail response is supported by primary bytes and resident
bindings; it is not a reconstruction of the complete 2003 MRT pipeline.
The new read-only utility export executes 576 unchanged original builtin
program constructions. Its zero-selector fixture distinguishes ordinary
builtin 6 (half-vector power 8), shiny builtin 9 (power 12), and dull builtin
10 (power 3). This does not yet prove every material's utility selector or the
complete normal/utility/texture target crosswalk. Those responses were not
converted into guessed modern roughness or additional gloss multipliers.
[Utility export](../../analysis/pstencil/elite-response-audit/original-utility-programs.json),
[reproducer](../../analysis/pstencil/elite-response-audit/extract_original_utility.py).

Full utility/MRT scheduling, other combined reflection families, original
manifold compatibility, and an original-build framebuffer comparison remain
open as described in the presentation and caster reference documents.
