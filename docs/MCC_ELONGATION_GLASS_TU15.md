# Elongation's prepared MCC window material

Elongation's BSP uses material slots24,27,30,35,52,55,68 and75 for its glass.
The original cache references `glass_interior`; the imported cache references
the distinct authored `glass_inte_dlc` shader. Its unsupported MCC metadata
previously selected the diagnostic opaque fallback, which wrote pane depth
and blocked the later dedicated sky draw. Giving the canonical shader native
programs did not repair the actual BSP reference.

## Broader renderer validation, 2026-10-26

Fresh [complete material comparisons](../analysis/native-renderer-all-20261025/material-state-difference-triage-result-v3/execution.json)
confirm that the eight original/prepared glass occurrences have identical
rendering inputs despite their authored name difference. The renderer source
refresh and all-map validation are still in progress. A fresh full-scene and
actual-window GPU run against the final source, cache and executable is required;
the historical results below do not qualify the new cohort.

## Historical native acceptance, 2026-10-25

The [integrated validation](../analysis/sky-exact-20261025/integrated-source-validation/README.md)
passes all 23 CTests. All 99 original/prepared sky input pairs and 594 paired
native-renderer image directions match exactly. The complete 86-entry inventory
includes all 43 prepared stock maps and two custom maps; 201 rendered BSP/sky
contexts produce 1,206 direction controls with 1,333 native material bindings
and zero uploaded instruction words. Program/fallback negative controls reject
all three deliberate mutations. PC/custom cases without original Xbox map
counterparts retain that explicit scope in the
[acceptance matrix](../analysis/sky-exact-20261025/integrated-source-validation/all-map-matrix.csv).

Elongation retains its actual eight glass slots, original blend/depth/sampler
state, the exact V5 glass image and V6 Earth BC3 chain. The remaining model
differences are now corrected by a finite authenticated geometry catalogue:
79 profiles preserve preview and all 474 source LOD states, every literal vertex
attribute and the canonical source material/part ordering. Lockout's imported
extra mountain section is excluded from its canonical original model; Uplift
and District now have authenticated native material closure. V7 also restores
five native images for eleven owners, completing 132 prepared mip owners.
Unknown or changed inputs remain rejected. Rendering uses fixed native Vulkan
kernels; it does not execute a runtime shader interpreter.

District's geometry is literal data from the authenticated original PC11081
installer map; its native shaders, constants, textures/mips and raster/sampler
semantics come independently from original Xbox Old Mombasa. The PC four-section
model is preserved rather than replaced by the Xbox three-section model.
[The authority and source hashes](../analysis/sky-water-tu15-20261025/district-pc-geometry-projection/README.md)
record this distinction. No Vista D3D shader is parsed or admitted.

Both original and prepared Elongation window cameras pass sky on/off, actual
glass omission, independently completed transparent-queue and exact repeat
controls without Vulkan validation diagnostics. Retained native Renderer
[positive-window](../analysis/sky-exact-20261025/full-native-validation-v79/window-captures/elongation-window-positive-native.png)
and [negative-window](../analysis/sky-exact-20261025/full-native-validation-v79/window-captures/elongation-window-negative-native.png)
references show the authored sky through the glass. Paired images use the same
native renderer; the all-map sweep isolates backgrounds. These results do not
establish full-scene equivalence or physical Xbox/NV2A framebuffer precision.

The [published-and-proven runtime](../analysis/sky-exact-20261025/native-runtime-publication-v79.json)
is `d85a3da7eac74077a9c1cb87e501e266` with cache
`mcc-sky-water-tu15-20261025-v7`. Its thirteen ordinary multiplayer cases,
four frontend screens and normal admission readers pass. Concurrent published
source revisions and the two-player/Coagulation defaults were preserved.
The [actual root-launcher route](../analysis/sky-exact-20261025/normal-launcher-native-proof-v79.json)
observes this executable and cache, then submits four frames and eleven ticks
with two local players in a joined session without changing the five publication
files.

`project_mcc_transparent_two_alpha_env_illum` projects the complete audited
MCC material tuple into the independently decoded original cooked contract.
It does not choose a donor by material name or suffix. The verified native
`transparent\two_alpha_env_illum` template is312 bytes, SHA256
`4072549c86ac5fd6d3b52326d5d9677141c149a30a5c15c3ad740d1c95393b58`.
Its existing authenticated pass/program closure supplies original blend,
depth-write, sampler and shader behavior.

The projection authenticates the root, all bank shapes/ranges, the five exact
bitmap identities/frame/log words, all ten vectors, runtime data, predictions
and auxiliary banks. It retains their actual MCC bitmap handles and authored
data. The cooked original pixel words are
`008d8567 80000000 00ffffff 00ffffff`; MCC stores those same values as
`008d8567 00ffffff 80000000 00ffffff`. Only this complete paired input tuple
admits the independently observed ordering conversion. Cooked parameter-name
tables and the raw MCC template program are unavailable; a general transparent
parameter reordering is not established. Changed inputs remain unsupported.
The existing opaque projector cannot admit this transparent family.

The repeatable exporter is `tools/export_mcc_elongation_glass_tu15.py`; its small
paired fixture is `tests/fixtures/mcc_elongation_glass_tu15.json`. The fixture's
nonresident bitmap owner addresses and native template address are explicitly
declared test bindings, not evidence of raw MCC residency. The actual-cache
regression independently validates live ownership, programs, constants,
texture pixels/mips and raster state for all eight BSP slots.

The exact pixel check exposed a separate imported `glass_smudged_grey`
discrepancy: both decoded images are256x256 RGBA8 with nine mips and349,524
bytes. Original decoded SHA256 is
`9c11e22435fa1139de981cdbbe33f08076ed4f92ebe3373acdebd9136e350c70`;
the previously selected prepared image is
`1b811f460bc482850465cc6fe069328b03d5c98df8ee2ad1fca28bacf3a94e6e`.
Material conversion preserves the donor pixels; correcting those pixels has
its own authenticated preparation/candidate acceptance gate. No material
test is weakened to accept that mismatch.

The separate V5 bitmap correction in
`tools/data/native-bitmap-corrections-v5.json(.zlib)` restores the exact chain
through the existing authenticated XOR preparation path. The delta is1123
bytes:381 RGB pixels in mip1 and2 RGB pixels in mip2 differ by one byte; alpha
and all other mip levels already agree. `tools/prepare_mcc_elongation_glass.py`
creates a new COW candidate, preserving the selected pack and texture prefix.
The earlier V3 candidate at `assets/imported/mcc-sky-water-tu15-20261024-v3` also
applies the already authenticated Sacred Icon V4 sky gradient and the Earth
restoration below, completing the then-current97-mip preparation profile and
onboarding freshness checks. It passes the strict
actual-cache test for all eight window slots, including every texture/mip
byte. Candidate creation and component proof alone do not establish launcher
publication or a matched original framebuffer.

The native source input audit found that all three space-sky materials already
had identical original programs, constants, transforms, sampler and raster
state. Their remaining substantial image difference was the actual
`spacestation\bitmaps\earth_matte` input: MCC supplied1024x1024 RGBA8 with
eleven mips, while the original uses512x512 BC3 with eight mips through4x4.
V6 independently authenticates the complete MCC donor and reconstructs the
unchanged349,520 original compressed bytes, SHA256
`b80a32d6384cbfa338297df337f8e66ebaf63c33c02e2f8283d0ff37aa5e80c8`.
There is no resampling or recompression. All45 prepared maps were inventoried;
the same eligible donor occurs in Introduction, Tutorial, Cairo, High Charity
and Elongation, and every corresponding original target was corroborated.
The grouped dispatcher accumulates V5 and V6 in one Elongation cache/write;
it does not overwrite one correction with a stale second map copy.

Before the finite geometry restoration above, original and MCC space models
had non-bit-exact positions, UVs, basis and some triangle/part ordering.
The earlier all-map GPU report retained those input and pixel differences as
failures, including when programs and texture bytes agreed. Its used-material
gates require an admitted forward pass,
decoded vertex program, supported texture operations and authored layer15.
Native paired-image controls do not establish physical Xbox precision.

The full-world diagnostic cameras are `(14,7,1.2)` facing+Y and
`(-14,-7,1.2)` facing-Y. Literal original BSP traversal selects clusters28
and27, respectively; original pane geometry at y=+/-8.4 determines the views.
All42 original clusters select sky0. Native sky on/off, glass omission and
transparent-queue controls distinguish material occlusion from sky omission.
Original-assets versus prepared-assets through the same native renderer does
not establish original NV2A framebuffer or physical Xbox precision parity.

The window material now also selects a precompiled native Vulkan kernel.
The source-owned world registry includes all eight original and eight prepared
window bindings as one complete program/declaration/control tuple. Together
with sky and water it compiles15 tuples into7 vertex and14 fragment modules.
Executable SPIR-V checks reject instruction-bank, declaration-selector,
instruction-count and fragment-control-word reads. The runtime compares the
complete identity and rejects incompatible pipeline or animation changes;
this flow does not execute a shader instruction interpreter.

Adding the glass and vector-cube native metadata moves the cloned globals
publication. Four exact additive sound namespace contexts preserve startup
admission without changing existing audio operands or weakening its guards.
The native Cache planners and ordinary Elongation four-frame startup pass;
original hardware image equality remains a separate acceptance requirement.

## Earlier qualified delivery and rendered window proof, 2026-10-25

The [five-file publication receipt](../analysis/sky-water-tu15-20261025/native-runtime-publication.json)
records the earlier root launcher selection of native runtime
`87ba9ab9a08d40d4a8f2a264eea7ac79` with prepared cache
`mcc-sky-water-tu15-20261024-v3`. All thirteen ordinary multiplayer smoke cases
and the independent frontend qualification pass. Publication retained the
two-player/Coagulation defaults, authenticated the unchanged original
publication identities, and passed the normal gameplay/frontend/cache readers.
The immutable candidate qualification is separate from this publication receipt.

The [normal root-launcher proof](../analysis/sky-water-tu15-20261025/normal-launcher-native-proof.json)
also observes the actual selected native executable and its injected prepared
cache/Coagulation/two-player arguments. The ordinary `--multiplayer --frames 4`
route completes four native frames and six simulation ticks with two local
players in a joined session, without changing the publication files.

Both authored window cameras passed original-map-pack and prepared-MCC GPU
controls: sky remains visible with glass present, with an independently empty
transparent queue, and with glass omitted; the actual glass itself changes the
rendered image. Repeated views are exact and produce no Vulkan validation
diagnostics. The actual selected native executable also produced inspected
640x480 [positive-window](../analysis/sky-water-tu15-20261025/final-native-window-captures/elongation-window-positive-native.png)
and [negative-window](../analysis/sky-water-tu15-20261025/final-native-window-captures/elongation-window-negative-native.png)
references, including the authored stars/Earth sky through the corrected glass.
The [capture report](../analysis/sky-water-tu15-20261025/final-native-window-captures/report.json)
records the diagnostic cameras, executable/cache identity, one submitted native
gameplay frame per camera and retained PNG hashes. This is rendered native
regression proof; original hardware framebuffer equality is still open.

The [earlier all-map report](../analysis/sky-water-tu15-20261025/final-native-validation-v2/README.md)
and [matrix](../analysis/sky-water-tu15-20261025/final-native-validation-v2/all-map-matrix.csv)
cover 83 map inputs and 79 sky-bearing maps. All 196 admitted sky configurations
pass 1,176 direction controls with zero runtime instruction-word uploads. The
strict all-map CTest was red for the three unadmitted prepared-MCC owners
in Lockout, Uplift and District, and for original/prepared-MCC input and pixel
differences (96/96 input pairs; 536/576 pixel pairs). Complete per-ray/material
evidence is retained [losslessly](../analysis/sky-water-tu15-20261025/final-native-validation-v2/full-sky-report.json.xz).
The other eighteen focused CTests passed. The current native acceptance above
supersedes those imported geometry/material/image failures. Original NV2A
rasterization/filtering and physical Xbox precision remain unverified.
