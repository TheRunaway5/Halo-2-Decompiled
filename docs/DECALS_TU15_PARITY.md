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
| Blood | Original species/material effect routing, particle contact child effects, projection, color, overlap and lifetime | Prepared-MCC source World now births blood through the original effect parts (2026-11-11): SMG hits on a Spartan produce `blood_splat`, on an Elite `blood_elite_splat`, with the authored layer 3, 180 s lifetime and 20 s decay; the renderer consumer draws every record and UDP peers receive them. Native procedural path rejections are unchanged; pixel comparison pending |
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

## Prepared-MCC decal bitmap residency (2026-11-11)

On prepared MCC maps the source World created no dynamic decals at all: no
blood, bullet holes or scorches. An instruction census of an SMG kill on
Lockout showed every request reaching `181DA0`'s final bitmap check, where
`12F800` and the mode-2 (blocking) `12F910` both returned zero. `182BB0` and
`183C10` never ran. The canonical MCC bitmap keeps 168-byte image rows with no
texture-cache partition, but the original addresses them with the Xbox
116-byte stride. `12F4A0` then sees a zero partition size and refuses. On Xbox,
the same blocking request loads the bitmap and admits the decal.

The two calls at `182B17`/`182B26` are now `required` boundaries. The shared
generator rule is in `generate_physics_live_support_tu15.py`, and
`regenerate_decal_bitmap_residency_boundary_tu15.py` re-emits only these two
calls. An Xbox cache runs both original callees unchanged. An MCC World binds
the decal's bitmap frame as a complete prediction row, the same owner used by
`predict_mcc_bitmap_frame`. The original `12F800`/`12F910` then load the real
MCC resource through the existing `1301B0` I/O boundary. A frame with no
deliverable partition keeps the original refusal and is logged once.

Evidence: `build/test-results/blood-respawn-20261111/` (blood reports for
human, Elite and host-client; boundary generation receipt). The Xbox-cache
`native_tu15_plasma_decal_lifecycle` still births, fades and retires its scorch
and glow decals. Its separate stale grenade-count assertion predates this change.

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
