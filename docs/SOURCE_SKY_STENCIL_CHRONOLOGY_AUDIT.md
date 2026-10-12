# Original sky stencil chronology and proposed native draw profile

The retained Zanzibar sky A8097, B8102 and moon8110 all enable the same original
stencil state: ALWAYS, KEEP on stencil/depth failure, REPLACE on pass, reference
and both masks FF. The source enables that state around the dedicated
`3EDE0 → 4C690` sky submission, then disables stencil. A native stencil writer
must select that **owning phase and admitted authored part/pass**, rather than
all background geometry or every material in layer15. This document proposes
an interface and phase plan; it implements no native writer.

## Authority and bounded observations

The authority is the unchanged `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The new [CPU report](../analysis/multiplayer-shading/sky-stencil-chronology/report.json)
pins the original instruction blocks, original Zanzibar map, native cache
inventory and existing weather proof inputs. The complete
[disassembly](../analysis/multiplayer-shading/sky-stencil-chronology/source-disassembly.txt)
comes directly from that executable, not a reconstructed sky routine. The
readable matched `13D9F0` and readable `15370`, `4D640`, `4D720`, `14B360`
provide cross-checks; unknown field meanings remain identified by their actual
source offsets.

The independently exported [original20 stencil report](../analysis/multiplayer-shading/original20-sky-stencil-analysis/report.json)
and [moon packet](../analysis/multiplayer-shading/original20-moon-stencil/8110-pipeline.json)
resolve the actual moon state. The CPU audit independently compares all six
307,200-pixel decoded snapshots: A changes462 stencil bytes from00 toFF;
B and moon change none. Every D24 integer is unchanged. A-after, B-before,
B-after, moon-before and moon-after are byte-identical. The FF pixels occupy
inclusive bounds `[184,78,381,125]` in the export's raw-row coordinates.
Zero changes by B/moon does not mean their stencil test was disabled: already
FF samples can remain FF, and depth/fragment coverage determines writes.

These exports are the original20 **Vulkan replay** D24-low24/S8-high8 packing,
verified separately against54 PickPixel controls and retained RenderDoc
`vk_replay.cpp`. They are distinct from the Lockout GL depth-word packing in
the frozen weather fixtures. No original20 histogram is substituted for
Lockout's6255 FF pixels or for an all-map stencil producer.

## Literal caller chronology

| Source boundary | State effect and admission |
|---|---|
| `132D0:13306..13308` | Selects `15370(0)` while preparing a render view. This disables stencil testing; it does not clear stencil storage. |
| `2BCD0:2C09B..2C0B0` | Dedicated sky region is reached only when caller EBP is not2, byte48575C is nonzero and the saved caller input at stack+34 is not2. Names/meanings for those mode values are not invented here. |
| `2C0B2..2C0BC` | Selects `15370(2)`, then calls `3EDE0`. |
| `3EDE0` | Requires actual render-sky admission byte4B9EE9, selected scenario sky4B9EEC and retained pose readiness. Uses the selected sky's model, source `3EBD0` transform and active target before `4C690`. Valid source-owned model/selection is a prerequisite; the native single-visible-sky preview is not the complete producer of those runtime gates. |
| `4C690:4C83C..4C84E`, `4C8C0..4C8CA` | Rejects raw part types0,1,5. The later literal `13D9F0` accepts only4/5. Their intersection is **raw type4**. Also validates model/section/part shader admission and the selected source section stream. |
| `4C88A..4C8B8`, `4C943..4C94F` | Requires selected template layer15 availability and binds each authored layer15 implementation in part order. Layer15 by itself does not identify the owning caller. |
| `4C6AC..4C6BA`, `4C954..4C956` | Enables the original depth-clamp control and overrides culling to NONE after each material binding. These states affect actual stencil coverage along with source vertex/pixel execution, W-depth, window/stipple/demotion and depth comparison. |
| `4C9DC..4C9E3` | Resets the dedicated sky clamp control to0. This is a state reset, not an image clear. |
| `2C0C1..2C0C3` | Selects `15370(0)` immediately after dedicated sky return. Later draws must receive their own effective stencil profile. |

`4C690` does **not** call the generic entry wrappers `4D640/4D720`. For the
ordinary scheduled collector, `44370:44513..4451E` instead selects mode1 for
layers1/3 and mode0 for other layers, including scheduled layer15. Its per-entry
`4D640` override applies only in modes1/3: either explicit byte485A75/485A76
selects mode0, otherwise entry DWORD+0 flag4000 selects mode2. Without those
inputs the baseline remains. `4D720` selects mode1 afterward when any of the
flag/override inputs was present. That replace-zero **state** only affects
subsequent surviving/depth-passing fragments; it is not a blanket zero fill.

## Exact authored data and current native metadata

[Zanzibar metadata](../analysis/multiplayer-shading/sky-stencil-chronology/zanzibar-source-sky.json)
contains literal72-byte raw part records and apply/cleanup state sequences.
The CPU-only [native inventory](../analysis/multiplayer-shading/sky-stencil-chronology/native-sky-inventory.tsv)
uses the current `source_material_layer` implementation and frozen cache
archive; it makes no Vulkan calls.

| Source diagnostic identity | Section/part | Raw type/flags | Material | Strip indices | Native expanded vertices | Selected source layer |
|---|---|---|---|---|---|---|
| A8097 | 0/0 | 4/0003 | 2 | 74 | 180 | 15 |
| B8102 | 1/0 | 4/0003 | 1 | 49 | 96 | 15 |
| Moon8110 | 2/0 | 4/0003 | 0 | 4 | 6 | 15 |

All three selected implementations have no authored stencil apply or cleanup
commands. A/B cleanup restores cull-CW only; moon cleanup is empty. Thus these
three draws inherit the dedicated mode2 state. Their material names, alpha
blend factors and texture identities are diagnostic labels, never predicates.

The current data already offers useful source identity:

- `src/model.cpp:425` reads the raw part type; `:476` retains section/part in
  each expanded vertex. The raw type itself is not retained in `Vertex` or
  `RenderModel::VertexOrigin`.
- `src/source_render_policy.cpp:10` resolves the selected material pass to its
  authored template layer using shader LOD and packed layer/pass ranges. It
  does not resolve the owning submission phase.
- `MaterialPass::states`, shader datum, shader LOD, template pass and
  implementation retain useful selected-pass evidence. `ForwardRasterState`
  omits stencil; `src/frontend_material.cpp:80` does not apply stencil states.
  The native `MaterialPass` currently omits the literal implementation cleanup
  range at+11C, which this audit decodes independently.
- `src/multiplayer_runtime.cpp:1246` selects a sky through the per-view
  `fog.visible_sky` and appends its posed model to background/reflection
  background streams. `Cache::load_visible_sky` is a single-visible-sky
  preview and explicitly rejects multiple visible skies; it must not replace
  the per-view camera-cluster selector in a live stencil plan.
- The renderer's existing draw key retains material/lightmap/geometry/section/
  part and background grouping preserves contiguous authored runs. Its
  background span and `sky_raster` flag do not encode the source phase owner,
  raw part type or original caller gates.

The smallest addition for a future writer is a **sidecar per ordered draw
run**, not a new name-based material classification or a required vertex ABI
change: retain selected sky/model owner plus section/part, raw part type,
selected layer/pass/implementation and an explicit dedicated/scheduled owner
phase. Most pass identity is already available through the material binding;
the irreducible missing inputs are the owner phase/admission and raw part type.
Merge runs only when the effective stencil profile and those source identities
agree. Keep the existing authored alpha-layer order.

## Proposed `NativeSkyStencilProfile` and phase plan

The following is an interface specification only. All source fields are
explicit inputs; the planner cannot recover them from image pixels or names.

```cpp
enum class SourceSkyOwnerPhase { Dedicated4C690, Scheduled44370 };
struct SourceSkyDrawIdentity {
    SourceSkyOwnerPhase owner_phase;
    std::int16_t scenario_sky;
    std::uint32_t model_datum, section, part;
    std::uint16_t raw_part_type;
    unsigned shader_lod, layer, template_pass, implementation;
};
struct NativeSkyStencilProfile {
    bool test_enabled;
    // Effective source state, translated by named semantics to Vulkan.
    // Front/back identical for15370; no direct cast of capture enum1.
    StencilCompare compare;
    StencilOperation fail, depth_fail, pass;
    std::uint8_t reference, compare_mask, write_mask;
};
```

The stencil semantic types denote named KEEP/REPLACE and ALWAYS/EQUAL/NOTEQUAL
states rather than unverified numeric enum casts. Disabled mode0 changes the
enable flag only; the source state machine retains the other registers. An
effective Vulkan disabled profile ignores those retained operations without
erasing attachment bytes. Mode2 enables front/back ALWAYS, KEEP/KEEP/REPLACE,
reference FF and both masks FF. Mode1 differs only in reference00. Other
`15370` low-bit modes remain explicit source state, never inferred from a sky
layer or silently collapsed to mode2.

A `SourceSkyPhasePlan` should associate ordered draw identities with effective
profiles and the caller-owned attachment generation/view rectangle. It needs
three separate operations: admitted target initialization/load, per-draw
effective state, and phase exit state. A dedicated profile is admissible only
with validated caller gates, selected sky/model/section, raw type4, authored
layer15 and the selected implementation. Replay its apply/cleanup states in
source order before computing the effective profile. A scheduled draw follows
its own layer baseline and explicit entry flags/override bytes. Missing or
ambiguous phase/pass/selection is an unresolved packet, not permission to
enable stencil on arbitrary background runs.

The implementation phase should first validate a retained-packet fixture using
the actual original20 D24 attachment, exact source programs/streams, all three
states and six depth/stencil snapshots. Preserve depth and target alpha and
compare every stencil byte; also run unknown-owner, missing-type, non-layer15,
type5 and exit-state controls. Matching462 pixels in this fixture would prove
the bounded raster path, not the live collector or all-map lifecycle. Only
after source admission is available should the per-view native caller carry
the sidecar profile into the renderer. The independent D24 resource migration
can proceed before this writer exists.

## Clear, reset and storage ownership

Literal `132D0` proves an actual conditional initialization clear. It calls
`14B60` only when the copied render-view short+8 is nonzero. If byte4670CD is
nonzero, semantic flags1 clear color only with packed color0. If that byte is
zero, semantic flags61 clear color, depth and stencil, with a source131ED0
packed color, depth float1 and stencil0. `14B60` translates semantic bit0 to
all color lanes and bits5/6 to the hardware depth/stencil clear bits. The
[25-entry layer table](../analysis/multiplayer-shading/sky-stencil-chronology/source-blocks-and-layer-table.json)
and `44370` also carry conditional target/clear policy; layer15's clear flags
are0. The dedicated `4C690` region itself contains no attachment clear.

`16B10`/`1BBF0` material reset, `18900` implementation cleanup and `15370(0/1)`
are state operations. None is an instruction to zero the stencil image.
Native initial CLEAR0 may be labeled **native initialized state** while actual
source render-view inputs, clear rectangle/target ownership and all relevant
runtime branches remain unavailable. The all-zero image before8097 is an
observation, not proof that the sky entry cleared it.

An actual D24S8 migration needs a combined depth/stencil attachment view, a
separate depth-only sampled view, stencil STORE and stencil LOAD across every
continuation sharing that image. Existing in-pass depth-only clears should
retain their DEPTH aspect until an actual source stencil clear authorizes a
change. The current-depth alias can pack the resulting current stencil exactly;
it cannot create missing coverage, caller chronology or clear history.

This audit used only CPU disassembly/metadata/inventory and independent reads
of completed exports. No GPU, guest or original execution was performed here.
No shared renderer/CMake file was edited, and the frozen88-input connected
weather proof and prior native readbacks remain unchanged. It does not claim
physical Xbox precision, complete original-frame appearance or live weather
activation. See [the D24 migration audit](SOURCE_WORLD_D24_MIGRATION_AUDIT.md)
for the independent resource/encoding boundaries.
