# Original mode3 weather plate graphics admission review

The complete proved mode3 CPU-builder domain cannot be admitted by the current
`SourceWeatherPlateRender` unchanged. Its fragment combiner covers the emitted
instruction vocabulary, but the complete original caller has a second depth
profile which the fixed Vulkan pipeline does not implement. The existing exact
captured60word guard, shaders, scene fixture and result remain frozen. This is a
source evidence and interface proposal, with no graphics or runtime activation.
The separately added pure caller-profile helper is documented in
`SOURCE_WEATHER_PLATE_CALLER_PARITY.md`; it matches the14 original caller cases
and does not relax the graphics guard.

Evidence is retained in
`analysis/multiplayer-shading/weather-plate-program-admission/`. Run the new
`audit.py` and `derive-caller.py` with `.tools/python/bin/python`, then run
`validate-admission.py` with Python3. The tools write only that evidence directory.

## Program family and combiner coverage

The frozen2440case fixture contains1780 successful definitions and660 original
false returns. The successful definitions contain1651 distinct complete60word
programs and16 structural shapes: eight reciprocal classes, each with the
source compositor's alpha-channel or RGB-channel flag. Only14 words vary:

| Index | Field | Distinct corpus values |
| --- | --- | --- |
| 4,5 | Alpha inputs4,5 | 2,2 |
| 11,12 | Constant0 stages1,2 | 85,78 |
| 14,15 | Constant0 stages4,5 | 1416,1404 |
| 22,24 | Constant1 stages4,6 | 1404,25 |
| 30,31,32 | Alpha outputs4,5,6 | 3,3,2 |
| 38,39 | RGB inputs4,5 | 2,2 |
| 43 | Final constant0/tint | 1418 |

The other46 words are invariant. The entire inventory, including every distinct
value, is in `program-family.json`; the values above are corpus counts rather
than an inferred finite list of all possible source colors.

Word53 remains`00011108`: eight stages, MSB mux selection, unique C0 and C1 for
each stage. Word54 remains`00008421`: each of the four five-bit texture-mode
fields is PROJECT2D. Compare mode42, dot mapping55, dependent input56 and
descriptor mapping/control57..59 are zero. Final inputs8/9 remain
`1D010000`/`00001D00`.

The union uses registers0,1,2,4,5,8..13; input mappings0,1,2,4,6,7;
output mappings0,2,4; output destinations0,4,5,12,13; RGB dot bits0,`1000`,`3000`;
and sum/mux selection. No blue-to-alpha output, fog register, final EF product
or final color-sum operand is consumed. `combiner.glsl` implements all of this
vocabulary. Both RGB and alpha read the prior stage before either result is
committed, matching the retained xemu generator. The constants upload maps
all18 packed C0/C1/final words to ARGB bytes divided by255, so the variable
constants are already represented by the generic uniform path.

This is structural coverage, not a new GPU arithmetic comparison for1651
programs. The existing source/native plate image verifies one captured
definition with a measured maximum one-byte RGB residual against the original
OpenGL llvmpipe scene. It does not verify the other classes or RGB-flag profiles
on hardware or establish physical NV2A precision.

## Complete original SDK and caller execution

`original-raster-proof.json` executes the complete immutable3A5C0 with
`skip_state=0` for26 controlled cases over actual captured near/far, SDK and
command banks: eight classes with above-near, equal-near and raw selected depth
below-near inputs, plus Lockout10..30 and authored Warlock18..30 depth limits.
The latter uses the captured Lockout compositor and is explicitly a depth-limit
control, not an observed Warlock draw. All26 calls return without a service
replacement or memory/MMIO boundary:1847 distinct original instructions and
41 captured pages are restored. All26 full60word programs and optional depths
agree with the frozen native helper (1638 output words including status).

Standalone3A5C0 selects temporary shader/raster settings. Its source blend
command is`0344=00008001` (CONSTANT_COLOR), and alpha testing is disabled. It
retains`8421` in the SDK texture-mode bank but does not emit1E70 in that interval.
These temporary settings are not the final37A60 plate draw profile.

`derived-caller-proof.json` starts at the actual captured36B50 entry, restoring
the captured register/segment/XMM/x87 context and lazily restored RAM. The
unchanged source reaches37A60 after511751 instructions,1688 distinct
instructions and49 pages, with no external boundary. Its exact derived entry
pages, CPU values and named plate/camera banks are recorded before the call.
The14 cloned cases execute complete37A60 and its real SDK callees to the actual
caller return`00036ECF`. They comprise the unchanged actual entry, equal-near,
raw selected below-near, collapsed range, independent projection/selection
bank controls, and all eight above-near reciprocal classes. No instructions,
callee returns, readiness, GPU command words or MMIO values are supplied.

All14 original calls complete. Thirteen emit four positions; the collapsed
range leaves the prior60word program unchanged and emits no quad. It still
emits84 bytes of earlier scroll-constant commands, so this is not a claim that
the complete caller performs no writes. The native pure helper matches all63
status/depth/program output words for each of the14 cases (882 words). Each
successful call selects VS21 and its57 hardware-emitted pixel-program words
match the source definition at the quad. The last three descriptor words are
zero. The method-to-word correspondence is validated independently in
`validate-admission.py`, including deferred`1E70=8421` flushed by SDK begin.

The proof derives37A60 entry/return from original CPU execution of a captured
36B50 entry. It is not a separately captured physical37A60 breakpoint or a GPU
replay of those controls. The parent whole36B50 proof later encounters a real
particle SDK MMIO boundary; completing the bounded37A60 call does not erase
that whole-caller boundary.

## Depth and projection dependencies

At the actual derived caller entry the selection bank485658/48565C and
projection bank485AD4/485AD8 both hold near`3C800000` (1/64) and far`44800000`
(1024). They are separate source banks. Modifying only the projection bank
changes quad Z/W while leaving the full pixel definition and selected depth
unchanged. Modifying only the selection bank changes the packed definition
while keeping the projection unchanged when the selected depth remains10.
Thus observed equality in this one source entry is not a license to merge
their ownership or assume producer equality in every camera state.

After the returned optional depth, source37DBE..37E45 executes these binary32
operations individually, in this order, using the projection bank:

```
span = far - near
scale = far / span
numerator = (scale * depth) - (scale * near)
Z = (numerator / depth) * 16777215
W = (depth / far) * 16777215
Z and W each clamp to [0,16777215]
```

The parenthesized products are separately rounded before subtraction; the
division and final multiplication are separately rounded too. The new
validator reproduces exact Z/W words at all four positions in all13 successful
cases. The actual entry yields`4B7F9A99`/`481FFFFF` (16751257/163839.984375).
Equal-near and raw below-near cases return the near value and emit
`00000000`/`437FFFFF` (0/255.99998474121094).

At37A60's quad, alpha test0300 is enabled,033C is GREATER`204`,0340 is zero;
blend0304 is enabled with0344 ONE`1`,0348 INV_SRC_ALPHA`303`,0350 ADD`8006`;
0358 is RGB`00010101`, and035C disables depth writes. VS ordinal21 is selected
after3A5C0. Source depth behavior depends on the returned depth compared with
the **selection** near bank, independently of the projection bank:

| At the quad | Returned depth > selection near | Returned depth == selection near |
| --- | --- | --- |
| 030C depth-test enable | 1 | 0 |
| 0290 control0 | `00110001` | `00100001` |
| 0290 Z-perspective bit16 | 1 | 0 |
| 0354 depth function | LEQUAL`203` | LEQUAL retained, testing disabled |

The source clamps optional depth to at least near, so a finite admitted return
below near is impossible; the below-near control describes its raw selection
before that clamp. The caller restores depth/alpha settings after drawing.
The report retains the emitted command prefix before the first immediate
position, avoiding a mistaken interpretation of those later restored values.

The frozen native plate pipeline always enables W-depth LEQUAL, and its fragment
always applies W clipping and writes the corresponding fragment depth. It
therefore cannot admit the near branch by merely widening the pixel-word
guard. That branch needs its own source-backed shader/pipeline profile and
matched appearance controls, or must be rejected explicitly.

## Textures, addressing and alpha

The four PROJECT2D words do not determine a complete texture path. The retained
xemu shader generator additionally inspects texture dimensionality, cube/rect
views, shadow-map format, border adjustments, convolution, signed components,
color key and alpha kill. The existing native fragment has four plain
`sampler2D` projected reads. It normalizes the packed depth alias's source
pixel coordinates by the actual alias extent; layer UVs remain normalized.

The actual derived37A60 hardware packet retains the captured family: stage0
format`00011229`, address`00030303`, filter`01012000`; layers1..3 format
`08870F29`, filter`02062000`; U/V repeat for each layer. Stage0 is a linear
A8R8G8B8 color alias, not a depth-comparison sampler; the source layers are
DXT45 and the isolated fixture feeds their actual decoded RGBA mip exports.
The real source depth alias's byte relation was independently established in
the retained GL capture. Cached fields absent from the emitted interval are
left absent in the report rather than assigned invented register values.

The current opaque Vulkan sampler/view handles cannot demonstrate these
properties. A generalized caller must carry proven binding metadata or own
the sampler/view creation. For the existing captured resource profile the
preconditions are one full-surface RGBA8 alias mip, nearest filtering and clamp;
three actual RGBA8 layer mip chains with linear min/mag/mipmap filtering and
repeat U/V; matching extents/row order and source view component mapping; no
shadow, convolution, signed component, color-key or alpha-kill specialization.
All alias bytes must come from the actual current packed D24S8 producer. A
valid60word program cannot certify current texture readiness, image contents,
the alias's stencil/alpha byte, decoding or resource lifetime.

The compositor's flag chooses RGB versus alpha-channel layer operands. It does
not provide final raster alpha enable/ref/function or a different texture
service. Final alpha depends on the sampled alias and layer channels plus the
register-combiner result. The shader's byte-rounded GREATER0 test agrees with
the retained original generator, but the additional classes/flags still need
matched fragment, discard and blend validation before a larger GPU claim.

## Proposed narrow interface

Keep the existing reference-packet preparation unchanged. Add a separately
reviewed caller-profile admission factory only after the additional GPU
comparison. It should accept source inputs rather than arbitrary packed words:

1. The actual52byte compositor plus source selection near/far, depth start/end,
   arithmetic controls and separately owned projection near/far. Own the
   original37A60 arguments internally: mode3, center0, anchor_end0,
   anchor_start1, skip_state1 and requested optional depth.
2. Require the actual37A60 compositor profile: flag byte1; six X/Z lanes +0;
   finite alpha lanes and tint in the source caller's clamped[0,1] range.
   Ignore the three padding bytes. The actual first word is`4B7FFF01`, not
   `00000001`; rejecting the padding would reject real caller input. Other
   RGB-flag or wrapping-color inputs remain legitimate CPU-helper controls,
   outside this narrow graphics profile.
3. Invoke the strict proved pure builder. On its original false return produce
   no new draw packet; retain the owned bindings/program and skip this draw.
   Require a returned depth strictly above the selection near for the existing
   W-depth pipeline. Unsupported arithmetic/finite classifications reject
   before any renderer mutation; they are not fabricated original false returns.
4. Derive the full60word definition internally. Validate its invariant fields
   and the exact class/flag structural family as a redundant check; compare
   all60 words when checking an existing source packet. A mask allowing the14
   variable words, or membership in a finite corpus, is insufficient because
   it disconnects depth/color source parameters from the emitted definition.
5. Derive/check quad Z/W with the separate source projection bank and exact
   operation order. Retain the exact VS21 token guard, source four-vertex strip,
   immediate UV0/1 components, scroll, clip ranges/regions, full-surface
   viewport and derived-output finite checks. Reject an inconsistent provided
   quad before replacing retained bindings.
6. Require the proved actual binding/alias/sampler profile described above,
   with ownership and readiness established by the relevant producer. Keep
   the current alpha/blend/RGB/depth-write state fixed for this above-near
   profile. A private admission token may carry the generated definition,
   selected depth and Z/W; its constructor must not be publicly forgeable.

This proposal admits source parameter math within one proved caller profile;
it does not claim that arbitrary mode3 helper inputs are complete37A60 draws.
Modes0/1/2, the near-depth raster profile, RGB compositor caller ownership,
alternative texture specialization and live producer integration remain
separate work. The14 original caller executions and26 skip0 executions close
the bounded CPU/SDK projection/state evidence gap. Representative original
versus native GPU results across the additional class/color/alpha boundaries,
plus actual live current-bank/resource producers, remain required for broader
appearance and runtime claims.
