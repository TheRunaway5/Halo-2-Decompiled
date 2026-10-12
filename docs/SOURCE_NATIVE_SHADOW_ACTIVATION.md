# Native multiplayer shadow activation inputs

The fixed shadow renderer is available, but ordinary multiplayer does not
produce a `SourceShadowPlan`. `runMultiplayer` constructs `Renderer` without a
shadow material residency table and never assigns `ModelView::source_shadows`.
This audit identifies the actual source services needed to connect them. It is
a read-only caller/control-flow review, not another original execution oracle.
No production activation or new input defaults are introduced.

The authority is the supplied Xbox `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`, its
`analysis/retail/functions` listings, the existing unchanged-instruction
oracles, and immutable captured banks named below. A native render vector is
not an original object collector.

## Source entry and ordering

Ordinary `2BCD0` submits world/environment layers8/20 before `335E0`; shadows
precede dynamic lights, atmospheric fog, late transparency and first person.
The optional Renderer hook already occupies this point and preserves the
scene color and W-depth through its LOAD continuation.

`335E0` requires both `226D0` and `335C0`. `226D0` checks literal pointer
`4E0344`, positive count at its+80, pointer `4E0348`, the first record's+1C
against NONE, and that record's+4 against `4E0348+8`. `335C0` admits an absent
`510C50` bank or a zero bank+5; otherwise it returns byte `509415`. These are
explicit active-bank/cinematic inputs, not inferred from a map name.

`223080→2C490` independently writes feature byte `4BA020`. The existing native
`source_geometry_lod_policy(actual_local_players).feature_bytes[0]` reproduces
this producer: the ordinary one-player/default branch writes1, and counts2,
3 or4 write0. `32350` rejects zero. Consequently fixed ordinary object shadows
must remain disabled for those split-screen player counts under this policy.
This does not suppress or establish the separate dynamic-light/type12 paths.

The original main collector is produced before this phase. `1320F0` resets
queues, executes `134390`, then `132220`. The later `2CBC0` collectors visit
clusters, instances and owners. `133720→134240` performs complete PRT-prefix
row swaps, and `133790` can add lowbit0 using the *first* complete `4BAF0` byte.
`33400` reads this processed owner queue in its literal order, not a material
grouping or a list sorted by full handle. The second fade byte feeds
`3D7F0`, a separate consumer. The complete source queue reconstruction is in
[SOURCE_OBJECT_RENDER_QUEUE_PARITY.md](SOURCE_OBJECT_RENDER_QUEUE_PARITY.md).

## Verified services and present native callers

| Input/service | Existing source proof | Native multiplayer status |
|---|---|---|
| Player-count feature bank | Complete `2C490` | Actual player count is supplied; the shadow-enable byte is available |
| Main camera and world depth | `2F970/2FB70→2FD90→12FA0`, `441B0` | Actual source projection/frustum and world c0..6 are submitted with W-depth |
| Full camera c0..11 | `source_view_shader_bank`, actual GPU camera fixture | Helper exists; current `ModelView` retains only c0..6, so a future plan must preserve the complete result with actual `RenderViewport` inputs |
| Main BSP visibility/admission | `173130→134390`, actual authored portal/instance/fog banks | Per-view source banks are produced and used for BSP geometry |
| Ordinary spatial lists and admission | Registration/location/pool services; `132220` over literal pools | Helpers exist, but the live runtime does not maintain those source pools or call the ordinary owner admission service |
| Processed owner queue | `134240`, complete `4BAF0`, flag marker; actual41-owner reconstruction | Helpers exist; no live source owner queue is produced |
| Current regular bounds and poses | Constructor/pose/bounds, native source-model skinning | Source scenes and living native players supply bounded regular spheres; actual links, death/camouflage/header state and type11 physics overrides remain separate |
| Lighting direction/opacity arithmetic | `D37F0/D4080`, `3DB00/3DFE0`, source query services | Native per-view caches carry `shadow_direction` and `shadow_opacity`; their existence does not establish `3DC90` owner-cache validity or source collector/reuse query cadence |
| Root lighting-cache selection | Complete `3DC90/BAF80`; 2,090 cases / 10,450 words, including 42 actual same-phase existing-cache cases | Explicit object-slot/cache-callback/validity-byte helper exists; actual native lifetime and post-query record state remain callers |
| Fade/priority/candidate/history | Complete fade and priority helpers; `3D7F0/33400`, history selection | CPU services are available; actual runtime banks/history/observer producer are not wired |
| Shadow frame/root uniforms | `31F50/32350/C1930/C1A80`, `4A1E0/48EE0` | Pure helpers exist; actual hierarchy, cache-validity result and later geometry admission remain callers |
| Shadow view volume/secondary camera | Complete `320B0→1652E0` and single-camera `C1D90`; 4,710 cases / 443,456 exact words, including 111 directly retained camera words | Explicit-bank helpers exist; actual receiver box/light/projection and secondary admission remain callers; mode0 is rejected |
| Secondary row classification | Complete `165A20`; 5,181 cases / 305,627 exact words, including five independently retained root row/count words | Explicit queue/current-metadata helper exists; actual secondary admission queues remain callers |
| Secondary draw-unit sphere culling | Complete `44720` sphere branch and linear `45560`; 1,258 cases / 41,015 exact words | Explicit resident subpart/bound/pose/mask helper exists; actual pre-cull collectors and accelerated/frustum services remain callers |
| Generate/Filter/Apply | Authored layer10/11, actual target replay and strict Renderer hook | Available only for explicitly supplied resident materials and ordered draw records |

Native player lighting is now evaluated inside the real view loop. That fixes
the former first-view-only call, but does not turn every queried/rendered native
owner into an admitted source collector row. In the original successful
nonreused `4B5D0` path, `3DD10` and context-present `3DB00` run independently of
ordinary versus PRT shader usage. Geometry reuse skips that tail. The separate
256-record cache, geometry token/stamp and shared frame arena are proven in
[SOURCE_RENDER_CACHE_PARITY.md](SOURCE_RENDER_CACHE_PARITY.md), but their
full live collector/resource caller is still required.

The native `SceneObjects` API retains current placement lifetime, birth ticks
and native generation handles. Its handle is `generation<<16 | (slot+1)`, and
freed vector slots can be reused. Current vector order cannot recover source
successful registration chronology. The source event journal can consume real
ordered native birth/registration/bound-update/removal events, provided actual
source primitive/header/link/location inputs and activation callbacks are
supplied. It must not reconstruct them by sorting the current scene.
[SOURCE_OBJECT_LIVE_REGISTRATION_SEAM.md](SOURCE_OBJECT_LIVE_REGISTRATION_SEAM.md)
records that contract and the bounded cold/root observations.

## Minimum activation packet still needed

The future runtime caller needs the following complete packet for each actual
submitting view. These describe data requirements, not a new public ABI.

1. **Frame/view and entry state:** active `226D0` banks, `335C0` cinematic
   inputs, actual render frame/view/local-slot indices, clock, complete
   submitting-camera c0..11 and source inverse-view depth/half-pixel metric.
   The player-count feature bank is already available.
2. **Owner registry and ordinary queue:** actual full handles, current object
   type/header flags/kind, flags+4, parent+14, sibling+C, child+10, cached
   render sphere+30/+3C, dying+10A, death tick+BC, and camouflage+2B0 where
   read. Supply both physical spatial pools and literal membership/head order,
   or an actual already-admitted complete queue. Compute PRT-prefix swaps and
   both per-view fades from current/root model metadata, original metric and
   `4BA019`; keep associations and all four queue columns.
3. **Priority and selection lifetime:** actual observer callbacks/scalar
   bits, local-user/player mapping, root player datum+13C, current local slot,
   retained per-view `5246F8` history and uncleared candidate flag-byte3 inputs.
   `33400` preserves that byte; a zero-filled replacement is not the captured
   source bank. History belongs to its source frame/view/reset lifecycle.
4. **Root-cache admission:** the actual `3DC90` result, valid owner-cache state
   and current84-byte lighting record after the source resource/reuse/query
   route. A bound ordinary material or nonempty native geometry is not this
   validity producer. Supply the literal root hierarchy, no-parent and
   `3D7C0/B9CE0` checks; the cached opacity multiplied by current owner fade
   must exceed the source `.09` threshold.
   [SOURCE_SHADOW_LIGHTING_SELECTION_PARITY.md](SOURCE_SHADOW_LIGHTING_SELECTION_PARITY.md)
   now proves the complete selection over explicit current slots and actual
   cache acquisition/validity inputs; it does not fabricate those live banks.
5. **Secondary camera/visibility and row classification:** actual `320B0`
   view-volume admission, `C1D90` generated shadow camera bank(s), root+2C
   containing cluster and `1320F0` secondary context. Then apply `165A20`'s
   actual root/type/flag classification to clusters, instances and owners.
   Current main-mode0/auxNULL helpers alone do not implement every required
   secondary branch.
6. **Ordered draw units and completed bindings:** `2CB30→133390→133520→133720`
   and `44720` must retain the main accepted prefix and the secondary range,
   resource-admitted geometry/regions, compact palettes/pose, exact part and
   subpart identity, plane/OBB tests, shader LOD, available layer masks and
   geometry shadow modifiers. Produce both admitted layer10 caster and layer11
   receiver groups, actual winding/depth-clamp controls and material externs,
   object colors, animation and pixel bindings in original order. Create their
   resident `SourceShadowPlanMaterial` slots before constructing Renderer.

`32350` checks a nonzero **secondary cluster queue**, not a final GPU draw
count. `165A20` clears bit1000 on the root row when its second argument is0;
other owner rows clear bit2000 and can clear both1000/2000 using their actual
type and flag31. Cluster/instance flags also change. Thus choosing every
currently visible world triangle as a receiver bypasses real source policy.
`44720` further consumes context+2A78/+2A7C, individual draw-unit flags,
transforms and exact part/subpart bounds; whole-model sphere membership is not
its replacement.

## Bounded activation recommendation

No normal-gameplay shadow activation is currently fully evidenced. Keep the
strict optional Renderer plan and its W-depth/camera/material guards. Pure
`320B0` and the single-camera `C1D90` branch are now recovered over explicit
main-view/frame banks in [SOURCE_SHADOW_VIEW_PARITY.md](SOURCE_SHADOW_VIEW_PARITY.md).
Complete `165A20` classification is also available over explicit queues in
[SOURCE_SHADOW_SECONDARY_PARITY.md](SOURCE_SHADOW_SECONDARY_PARITY.md).
The sphere/linear-subpart draw-unit service is available in
[SOURCE_DRAW_UNIT_CULL_PARITY.md](SOURCE_DRAW_UNIT_CULL_PARITY.md). Its retained
32893 control replays only the post-state: it cannot establish the earlier
part calls. The next source implementation must recover actual secondary
context/resource banks and accelerated/frustum draw-unit services. In parallel, connect the source registration
journal only to actual native lifecycle events once its required primitive,
activation and current owner fields are produced. A SCEN/MACH/BLOC-only
collector must remain labeled as an explicit subset; it cannot stand in for
the complete gameplay collector or silently remove players, vehicles,
weapons, attachments and allocator-pressure interactions.

The seven retained original generation draws have an independent decoded
WORLD suffix proof: 392 sampled output words match. One fully corroborated
Chief section19 pose covers 672 referenced vertices, with 16,128 nonposition
GPU words exact and measured bounds of 0.06251 target pixel / `8e-6` depth.
The actual compiled variants were local/boned/skinned classes1/2/4/5; this does
not establish seven live native class-0 pose producers. See
[SOURCE_SHADOW_CASTER_WORLD_PARITY.md](SOURCE_SHADOW_CASTER_WORLD_PARITY.md).

The separate live shadow phase is version3: 135 independent checked words,
including seven computed/corroborated fade words and three
computed/corroborated priority words. Its original OpenGL/llvmpipe phase and
later3290A metadata retain their temporal distinction; they are not the
original20 Vulkan GPU frame. See
[source-shadow-live-phase.md](source-shadow-live-phase.md). These original
bank reconstructions do not supply new native lifetime or geometry producers.

This review performs no GPU work, guest I/O, resume/input, production code
change, new fixture execution or public ABI change. Existing proof counts
remain those of their named independent tests. The adjacent audit inventory is
`analysis/multiplayer-shading/native-shadow-activation-audit.json`.
