# Canonical mainmenu BSP frame producer

`MainmenuBspFrameProducer` constructs the CPU input banks for the original
standard-video, single-player, normal-main-mode0 BSP collector. Its immutable
authored inputs come from the actual `Cache` and selected BSP. Construction
requires scenario type2. It does not identify the title by filename, a UI
screen ID, captured geometry IDs or final draw packets.

The original authority and unresolved caller services are retained in
`analysis/frontend/title-bsp-admission-audit/SOURCE_AUDIT.md`,
`original-input-producers.asm` and `original-input-results.json`. The existing
camera/clip lifecycle authority is
`analysis/frontend/title-camera-matrix-audit/WORLD_DEPTH_CALLER_LIFECYCLE.md`.

## Caller contract

The caller supplies these current values in `MainmenuBspFrameInput`:

| Input | Source authority |
| --- | --- |
| Complete canonical `menu_render_camera` result | Actual authored observer/cinematic camera; original2F800 angle |
| Previous leaf and cluster | Caller-owned per-view4BA054/4BA064 history |
| Persistent `SourcePortalMaskState` | Actual512bit4F93A4 bank, initialized by the source-owned title initializer |
| Auxiliary presence and cinematic byte5 | Actual510C50 pointer presence and script-driven state; byte5 is ignored when absent |
| Optional `flags_4e64c8` | Actual original fog override word, if known; absence invokes the equal-branch guard |

The supported collector context is the source-proved ordinary title path:
outer `(0,0,480,640)`, safe `(36,48,444,592)`, near `.015625`, far1024,
one view, radius1024, flags0, mode0, NULL filter and no split plane. Those
values are source-owned properties of this bounded entry point, not runtime
arguments guessed from a screenshot. Changed clip fields, missing render
angle, an unoriented camera and non-finite camera inputs are rejected by
`menu_render_view`. Other collectors, scaled rendering, alternate split
contexts and native aspect extension are outside this API's contract.

`prepare()` uses the actual eye/forward/up to construct two distinct banks:
original2F970/2FB70/2FD90 render state and original441B0 clipping state. The
clipping basis is never copied from the render bank's reconstructed up. The
existing render factory now returns its actual2F970 `view_modifier`, along
with the bank, and retains the old factory's inputs through delegation.

The current leaf/cluster comes from the literal14A280 root query. The actual
aggregate fog sample comes from `BspAtmosphere::sample` at the same eye and
forward, and must resolve the same current cluster. The actual clipping
camera, containing cluster and caller portal mask feed173130. The resulting
visibility and exact16E210 fog membership feed134390 with the literal131FF0
queue capacities128/384. No coarse all-geometry/PVS replacement enters this
chain.

## Location and unknown override boundaries

`mainmenu_camera_location` exposes both original2B720 local output and2B790
persistent history. A successful root query updates both, including a valid
leaf mapped to clusterNONE. On failure a range-valid prior leaf/cluster pair
becomes the local output; an invalid pair clears only that local output.
Persistent history remains unchanged on every failed query. The helper does
not require a valid pair's cluster to equal its leaf's cluster mapping,
because the original validates only the two ranges.

The frame producer requires a successful current query and a nonnegative
current cluster. It rejects outside frames explicitly; the helper's exact
outside location behavior does not establish outside fog or queue parity.
The caller commits a successful returned `retained_location` at the source
camera-state boundary, before and independently of presentation success.
The source initialization value of the history is leafNONE/clusterNONE; the
retained audit executes the original initializer prefix, while its full
startup invocation chronology remains a separate question.

An absent `flags_4e64c8` does not mean the flag word is zero. The producer
evaluates both possible bit4 branches of16E210 with the actual aggregate
`planar_datum`. Equal membership banks establish an output invariant. A
changed/modded bank whose branches differ is rejected until the caller
supplies the actual word. The native boundary checks include a changed datum
that distinguishes these branches.

## Owned handoff and remaining rendering gates

`MainmenuBspAdmissionFrame` owns the render camera, clipping camera, aggregate
fog, visibility, BSP admission and actual cinematic state. `shader_view()`
returns references to those owned banks with the canonical outer0…480,
one-player and actual modifier inputs. It is unavailable on an rvalue. Keep
the frame alive while passing that view and `frame.cinematic` to
`SourceBspResidentPlan::select`.

This producer creates no GPU state and activates no raw draw. It does not
close original partial-subpart visibility, raw base texture integration,
same-owner base/layer16 depth history, transparent collector ordering or
whole-frame parity. In particular, its canonical clipping rectangle must
not cull a wider native world; a separate proved expansion is required.
The canonical aggregate fog sample may be consumed independently of those
canonical visibility/admission outputs when rendering a wider world.

## Independent joined-original validation

`tests/mainmenu_bsp_frame_tests.cpp` accepts:

```
mainmenu_bsp_frame_tests assets/disc/maps/mainmenu.map tests/fixtures/mainmenu_bsp_frame_original.bin
```

The independent fixture producer executes the original location/history,
fog, render-camera, clipping-camera, visibility and admission chain for the
inside cases. It is separate from the production implementation and retains
full original banks for those cases.
Its schema begins `H2MBF001`, then a little-endian case count. Each record is
length-prefixed in32bit words:32 input words,6 location/modifier output words,
72 raw fog words,48 render-camera words,111 clipping-camera words, then
count-prefixed visibility and BSP-admission vectors. The source sidecar
retains case labels, instruction/source identities and initialization policy.
The four outside controls contain only original-produced location/history
outputs. Their unused fog/camera/queue slots are explicitly unsupplied zero
or empty padding; the test rejects those producer calls before comparing
such banks. They establish no outside fog, view or queue parity.

The native test requires all2,996 retained authored camera/height cases,
outside history controls, and auxiliary absent/present-byte5false/present-
byte5true controls. It compares complete render and clipping banks, all
serialized visibility/admission words, source-exposed fog offsets and exact
current/history outputs. It also checks the actual source portal initializer,
resident-view references, camera rejection boundaries and changed-datum
override ambiguity. Source163DB0 does not write visibility frustum word3;
that word is omitted consistently with the earlier independent traversal
serializer.

The original black-color service pointer at4686D4 must retain its XBE value
440A00; replacing it with a viewport pointer corrupts secondary RGB. The
independent oracle retains that original service and does not zero or repair
fog output words after execution. Scratch initialization is recorded only
for retained/unwritten storage stability. Native unexposed scratch/retained
fog words are not compared as reconstructed outputs. The complete original
288byte fog bank is retained for inspection even where the native API exposes
fewer fields.

The joined oracle extends2B790 through2B888, including actual14B360 sky
availability and the untouched caller of12E5E0. Its actual availability and
caller flags are `(1,1)`. The earlier retained standalone fog oracle supplied
`(0,1)`. The independent2,996-case fog-only diagnostic identifies differences
only in raw metadata word+10:497 actual outputs are `00000101` and2,499 are
`00000001`, versus standalone `00000000`. All other71 words agree. Tick292
is the first actual00000001 output. Original12E6AB…12E6CA sets byte+11
according to sky opacity>0; that opacity is already tested at raw+68.
The actual original words are retained
unchanged. `BspFogSample` does not expose that metadata word, and the native
test does not claim to reconstruct it. No new flag field or value is inferred
for the native consumer.

Registration, compilation and execution are owned by the integrating task.
The implementation and test source alone do not establish a passing joined
gate or live Vulkan fog integration; report those results separately.
