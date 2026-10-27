# Decal TU1.5 acceptance

The requested scope is all decals in offline and online play, with blood
matching the original Xbox TU1.5 owners and assets. TU1.5 is the behavioral
and visual authority; prepared MCC data is the runtime input. A native fixture,
nonempty pool or submitted draw alone does not establish matching appearance.

This audit is in progress. No complete decal parity claim is made.

## Acceptance matrix

| Area | Required evidence | Current status |
| --- | --- | --- |
| Asset census | Every available map, decal family, chain, layer, flag, bitmap image, mip and sprite selection | 171 normalized decal contracts and 96 blood-producer contracts found in both banks; per-map admission remains separate |
| Static map decals | Actual scenario placements, clipped source cache, cluster/PVS admission and rendered pixels | Original cached consumer has bounded Xbox-cache evidence; MCC consumer explicitly disabled |
| Scenario decorators | Floating/projected decorator groups and their own material/geometry consumers | Separate census and renderer audit pending |
| Blood | Original species/material effect routing, particle contact child effects, projection, color, overlap and lifetime | All 24 blood decal atlas/mip inputs match; directional smears and particle collision child effects hit explicit procedural rejections; runtime comparison pending |
| Bullet, energy, scorch and compound decals | Original effect requests and cache births, correct texture/layer/chain, no duplicate native/source projection | Runtime comparison pending |
| Projection | Original surface traversal, clipping, packed UV/brightness, orientation, scale and moving receiver behavior | Native static triangle adapter is a separate implementation |
| Admission and storage | Original recent overlap rules, allocation, salts, cache capacity and retirement | Full comparison pending |
| Six draw layers | Original programs, blend factors, RGB mask, sampling, W-depth bias and texture-mapping interleave | Literal state fixtures exist; full live order remains open |
| Time and reset | Original birth, fade, expiry, compound epoch, map/round reset and frame-rate independence | Bounded original lifetime fixtures exist; full live acceptance pending |
| Offline multiplayer | Actual prepared-MCC source world and one/four local rendered views | New source-cache binding regression pending |
| Offline campaign | Launcher-selected campaign entry, actual births and renderer consumption | Entry and ownership audit pending |
| Online authority and replicas | Actual event delivery and visible source output on both peers; duplicate/loss/reset handling | Joining peers do not own the source simulation decal cache |
| Late joins and migration | Original expected local/replicated behavior, then corresponding lifecycle checks | Original semantics require verification; historical decal replay is not assumed |
| Delivered runtime | Focused checks, native GPU evidence, online/offline checks and guarded launcher qualification | Pending fixes and qualification |
| Original appearance | Matched TU1.5/native frames with pinned map, camera, timing and effect state | Open |

## Established source owners

TU1.5 `1800B0/180170` own decal construction/reset, `17D730` the effect-part
request, `181500` its source/collision admission, `181CB0` projection,
`183AF0/183C10` packing/cache writes, `17FB60/180520` lifetime and compound
updates, `43A30` the six layer programs/states, and `44030` cached draw
traversal. Their exact original executable, fixture and evidence boundaries
are documented in `SOURCE_DECAL_VERTEX_CACHE_TU15.md` and
`SOURCE_CACHED_DECAL_RENDER_TU15.md`.

`180980` is inside projection mathematics in the authenticated TU1.5 image,
not its creation entry. Older retail address labels must not be carried over
without pairing. The direct TU1.5 byte receipt is
`build/test-results/decals-20261025/authority/tu15-owner-bytes.json`.

Investigation reports are under `build/test-results/decals-20261025`.
Builds and captures use the owned storage lifecycle in `TEST_STORAGE.md`.
