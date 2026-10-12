# Source shader detail and instance fade

Shader detail is separate from geometry detail. The original `41040` submits
each selected part to `1BF50` with its authored shader bias, the parallel
collector coverage byte, the part's authored detail scale, and the actual view
state. The former native highest-detail-only policy is not the general source
policy. The new independent module exposes the literal arithmetic and cache
metadata; the caller still owns visibility, geometry and actual draw admission.

## Authority and tests

The oracle executes the supplied original `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
`tools/differential_source_shader_lod.py` executes complete `1BEB0`, ordinary
`shad`-path `1BF50`, `2C490`, `16AE0`, `322B0C`, `1C1B0` and `1C290`, plus
bounded unmodified arithmetic slices in `41040`, `2CBF0`, `133390`, `133520`,
`133790` and `4D640`. No called routine is replaced with a native result.
Every source memory, collector, camera and auxiliary input is explicit.

The final differential matches **50,016 cases / 226,066 words bit for bit**:

- 11,616 contexts for 968 distinct authored part-used map-local shader banks;
- 7,701 coverage contexts using literal cluster bounds and instance spheres
  from the 21 original caches;
- complete source alpha conversion and all 256 source stipple selectors and
  32-word patterns;
- the complete 25-entry source layer-mode table under 12 frame-type inputs
  and the actual gameplay frame-descriptor producer;
- threshold equality, signed zeros, NaNs, infinities, invalid integer
  conversions and adjacent instance-admission thresholds.

The source CPU configuration is explicit x87 control `027F` and MXCSR `1F80`.
The paired quiet/signaling NaN cases retain the original execution's measured
class, significand and sign selection before native arithmetic; this avoids
compiler-dependent operand reordering in Debug versus optimized builds.
This arithmetic proof does not establish framebuffer or physical NV2A parity.
The frozen regression is `tests/fixtures/retail_source_shader_lod.tsv`; the
full differential report is
`analysis/multiplayer-shading/source-shader-lod-original.json`. The separate
asset regression verifies all **6,450** ordinary shader bias banks and
**11,628** thresholds across all 21 caches, retaining every raw float bit.

## View and coverage producers

`source_shader_coverage_camera` consumes the actual `SourceViewProjectionBank`
from `2FD90`, whose original address is `4B9E14`. Its inverse-view Z row is
`4B9E20 / 4B9E2C / 4B9E38 / 4B9E44`; the coverage projection scalar is
`4B9ED0`, bank offset `BC`, `viewport_half_metrics.y`. A raw camera direction,
estimated projected radius or GLM projection is not an equivalent input.

| Caller | Literal geometry input | Coverage multiplication order |
| --- | --- | --- |
| `133390` cluster | cluster `+54..68` min/max axis pairs | `(height / depth) * radius * 2` |
| `133520` instance | instance `+3C` world center, `+48` radius | `(radius / depth) * height * 2` |
| `133790` object | actual cached object `+30` center, `+3C` radius | `(height / depth) * radius * 2 * 4BA028` |

The three inverse-view dot products have different source operation orders;
the helper preserves each one. The source absolute-depth branch clamps to
the literal `0.1` value, including its unordered comparison behavior. The
cluster radius comes from rounded SSE half-extents followed by the original
x87 PC53 square root. Instances below the source `0.13` radius threshold
multiply coverage by `1.75`.

The actual `2CBF0` coverage argument is its tenth stack argument, entry
`ESP+28`. `133F70` supplies the model collector's `+1B4` coverage field; that
local collector is distinct from the unrelated effect context at global
`510C4C`. `2CBF0` multiplies coverage by `0.25`, floors with the original
CVTT/comparison/wrapping arithmetic and clamps the result to `0..255`, storing
the parallel byte in `4C0B84`. The packed command's top byte is a different
input. `source_shader_coverage_byte` retains the source exceptional integer
behavior, including negative overflow wrapping; a generic floor-and-clamp
replacement changes those cases.

## Part metric and shader selection

`410F8..41116` computes the part metric as
`((float(collector_byte) * resolved_part_scale) * 4670E4) * 4`.
The scale is the exact `45CE0` result, including its authored nonpositive
fallback, exposed by the transparent-part metadata module. `4670E4` comes
from complete `2C490`: `0.85` for the ordinary one-player/default branch,
`0.3` for two/three players and `0.25` for four players. It is distinct from
the object coverage multiplier `4BA028`, respectively `1 / 0.8 / 0.7`.

`1BEB0` uses the actual signed-short outer camera height, bottom minus top:

| Raw shader bias | Source multiplier |
| --- | --- |
| 0 | `1 / height` |
| 1 | `0 / height` |
| 2 | `0 / height` |
| 3 | `2 / height` |
| 4 | `4 / height` |
| 5 | `0 / height`, scan bypassed |
| 6 | `1 / height` |

An actual auxiliary pointer at `510C50` with nonzero byte `+5` doubles only
bias 6 after bias 0 is stored. The raw source values for biases 1 and 2 are
zero; their editor names do not justify inventing a different multiplier.

`1BF50` reads the raw selected shader `+3C` bias, resolves its first `shad`
postprocess record and first template postprocess record, and scans the
template's 10-byte LOD records at threshold `+6`. Actual view modifier
`4BA019` doubles the metric before its bias multiplier. The first ordered
`coverage >= threshold` comparison wins. Bias 5 directly selects index 0.
An exhausted scan clamps to the final index. The literal empty-bank return
is index `-1` with AL true; this is retained as the source return value and
does not authorize a native draw from an absent bank.

The caller must supply the same source rectangle and projection banks to
the height and coverage producers. Independent resolution, split-view and
FOV heuristics are not substitutes. The ordinary source selection therefore
remains reviewable independently of native image size and resize scheduling.

## Instance fade is source stippling

`13362D..13368F` computes
`truncate(saturate((coverage - 25) * literal(1/7)) * 255)`.
Its SSE intermediate is spilled before x87 PC53 multiplication and complete
`322B0C` truncation. Zero skips the instance command. Nonzero AL becomes
command byte `+1A`, global `4BA152`, through `2CBF0`.

Before drawing, `4D0B0` calls `4D640`. An alpha other than `FF` is converted
with the literal reciprocal of 255; `1C290` rounds the resulting scalar times
16 to nearest-even and selects one of 17 patterns. `3F8CC0` uploads that
32-by-32 pattern using the source `STIPPLE_PATTERN` command. `4D720` restores
the all-one pattern afterward. The fade is not a shader-opacity parameter
or an instruction to change the authored blend state.

`source_shader_instance_stipple` returns the exact level and 32 DWORD rows
generated by complete `1C1B0` from the literal 16 ordered positions at
`43F188`. All 256 alpha-byte conversions and source patterns are independently
verified. Native draw-state application must preserve source fragment/window
coordinate addressing and per-draw isolation; that renderer integration is
separate from this CPU proof.

Pattern upload is separate from enabling it. `15680` consumes the scheduler's
AX mode, and `44370` selects that mode from the literal 25-entry table at
`43FA34`, stride `24` hexadecimal. `source_shader_layer_stipple` returns the
actual enabled layers **1, 2, 3, 15 and 18**; all other layer entries disable
stippling. Frame descriptor types 4 through 7 force it off for every layer.
The special direct calls in `2BCD0` and `423C0` agree with those modes for
their submitted layers. Thus emissive, environment, PRT and bloom layers do
not automatically inherit a base pass's partial-instance pattern.

The producer of global `485898` is `12A50`, which copies frame descriptor
word zero. Ordinary gameplay `2234E0` sets ECX to 3 at `22370F`, and `2B5D0`
retains that value in ESI and writes the descriptor at `2B676` before calling
`12A50`. `source_shader_gameplay_frame_type` retains that literal **3**. This
field is not an antialiasing sample-count setting. An independent read-only
virtual memory export from the unchanged paused original session confirms
the retained value 3 in
`original-xemu/retained-shader-frame-type.json`; status remained paused before
and after the read. The source `15680` mode-2 SAMPLEALPHA path exists, but
none of this table's 25 layer entries selects mode 2.

## Scope and admission boundaries

The read-only inventory
`analysis/multiplayer-shading/shader-lod-authored-inventory.json` covers all
ordinary map-local `shad` tags and the authored part-use superset from BSPs
and 925 placed rendered models across all six geometry levels. Of the 968
distinct part-used shader banks, 740 have multiple levels and **731** have
multiple levels without the bias-5 scan bypass. These are metadata counts,
not counts of visible or submitted draws. The four unresolved Zanzibar
pillar model selections remain explicit fixture inputs in that superset.

The infinite water grid has an independent scheduler: `3C3C0` directly
selects the first template LOD and calls `1BBF0 / 16B90` with LOD 0, layer 3,
implementation 0 and coverage `100000`. It does not call `1BF50`. Ordinary
authored BSP water-pool/edge parts require their actual command/scheduler
path; infinite water must not be routed through this ordinary selector.

The helper supplies ordinary `shad` resolution only. Dynamic `MTPR` / `prt3`
virtual-template services, override/group contexts, exact native pass
admission and rendering remain separate caller responsibilities. The cache
loader rejects missing tags/templates and out-of-range bias rather than
substituting the highest detail level.
