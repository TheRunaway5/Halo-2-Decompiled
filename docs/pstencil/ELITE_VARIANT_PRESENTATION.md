# Elite variants and the neutral 2003 detail response

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
completion. Original's shaders and the on-disk maps are unchanged. The code,
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
