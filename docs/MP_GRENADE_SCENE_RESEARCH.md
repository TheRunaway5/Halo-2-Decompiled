# Released grenade scene evidence

`SourceGrenadeScene` renders the original frag/plasma model banks for the
supported ordinary released branch. It imports the actual current map's
`matg+100/+104` grenade rows (stride `2C`, projectile at `+28`), prepares every
authored geometry/shader LOD, and keeps full salted owner identity, four-view
LOD history and ordinary lighting state independently. Two shared material
templates occupy eight resident slots and 1,366,304 texture bytes on each of the
23 supplied maps; ownership does not duplicate these texture banks 2,048 times.
The caller supplies its actual object allocator capacity and the imported
`ObjectConstructionOperands::one_bits`, rather than a separate rendering cap
or an assumed scale field.

The canonical original evidence is
`analysis/multiplayer/integration/grenade-scene-20261010/measured-final/` and its
independent `measured-repeat/`. The source images are the user-owned disc1.1 XBE
SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`
and authenticated supplied u5 XBE SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Unchanged original instructions execute against literal Lockout metadata and
the genuinely allocated/released frag record from `live-grenade-20261010`.
Explicit pool/tag locator backing supplies addresses only. No service returns,
registration results, object flags, variant, node pose or scale are patched.

| Complete body, disc1.1 | Complete u5 counterpart | Instructions |
| --- | --- | ---: |
| `BB430` | `BC570` | 22 |
| `BB480` | `BC5C0` | 88 |
| `BB580` | `BC6C0` | 33 |
| `16EC00` | `16FFE0` | 31 |
| `16ED00` | `1700E0` | 281 |
| `BDF80` | `BF0C0` | 394 |
| `16F3C0` | `1707A0` | 46 |
| `BEB30` | `BFC70` | 149 |
| `143960` | `144CF0` | 85 |
| `1441D0` | `145560` | 69 |
| `BEDE0` | `BFF20` | 202 |
| `C0490` | `C15D0` | 10 |

The eight executed controls cover two images, two definitions and two world
positions. `BB430`, `BB480` and `BDF80` return in all eight: native variant/region
selection and all 13 words of the authored default node match exactly. Four
`BEDE0` controls return completely. The translated four retain the exact regular
sphere result, then stop at the actual world-unregistration owner `B96A0` /
u5 `BA7E0`; no successful world callback is supplied. All four sphere words
match the native arithmetic. Plasma is explicitly a changed-definition direct
consumer control using its real model metadata, not a newly executed plasma
factory/allocation claim. These are original-instruction emulator comparisons,
not a separate physical Xbox floating-point measurement.

Lockout frag `E7A80623` resolves through model `E7A90624` to render model
`E63204BD`; plasma `E7B2062D` resolves through `E67F050A` to `E680050B`. Both have
object type5, no animation graph, no model variants, one region and its sole
eligible permutation0. The native constructor rejects any map definition that
violates this bounded structure, has PRT, source light attribution, or requires
unowned color/animation state. The source-created frag has node descriptor
`{34,1CC}`, region descriptor `{A,200}`, variant `FF`, scale `3F800000`, generic
object flags `00030320`, control byte `+10A=0`, and released `+C1bit0=0`.
`B8820` owns the scale via the literal one operand at disc1.1 `465030` / u5
`46A0D0`; this is the existing imported construction operand.

The new constructor extraction retains original states100,130,160,183,184,240,
their complete 522-byte bodies, raw creation descriptor, and 46 overlapping
source writes. It exposed an important held/released distinction: original
attachment `BA2A0` calls `BAA80` with `DL=1` at `BA6A5..BA6A7`, and `BAB16` sets
object `+C1bit0`. Original `B82A0` passes `DL=0` at `B82B0..B82B2`, and `BAAF8`
clears it during release. Whole `BAA80` also tests type mask `1883`, `+C0bit6`
and `+B4`; parent identity alone is not a universal frozen-state rule. Complete
paired bodies for `BAA80`, `BA2A0`, `B82A0`, `B86C0` are retained separately.
The scene therefore rejects `parent!=NONE`. Runtime filtering to released
entries is the supported native subset, **not evidence that the original hides
every attached object**. Held/local visibility and its retained flag producer
remain unimplemented.

Each accepted draw owns its copied vertices and lighting bank. The approved
`SourceObjectGeometry::select` overload moves only previous-LOD storage to the
full owner's four-slot bank; the original method delegates with its existing
member bank. Tests compare every full vertex byte, material reference, selected
LOD/transition and distance against four independent original geometry objects
for one and four views. The scene rejects NONE, out-of-capacity indices,
definition/type mismatches, duplicate full datums, and two simultaneous salts
sharing one low16 slot. Destroy/reuse and copied per-view output lifetime are
checked. The renderer must retain distinct per-owner lighting uniforms while
sharing resident images/material templates; a single shared material-range
lighting binding would overwrite earlier owners.

Final CPU validation uses the actual production archive comparator and a fresh
fully instrumented CMake `source_grenade_scene_tests` target. Both run all
23 maps, with independent repeats and ASan/UBSan/leak detection. The fixture
export is literal original data; no native pose is used to generate an expected
result. An earlier attempt linked an older Cache archive against a concurrently
changed `LevelContent` declaration and failed in its destructor. Those failed
logs remain exploratory evidence. The final narrow test consumes
`load_bsp_content` directly, the exact collision bank moved by `load_content`,
and the final production/sanitizer archives and header snapshots are retained
with precise observed-build provenance. No complete fresh-tree provenance is
inferred from an archive's timestamp.

Current position, forward/up, velocity, camera/admission and render frame are
explicit native sandbox inputs. The scene does not recover source PVS/world
registration, local held visibility, damage/permutation changes, or postrelease
angular physics. Shader programs, material ordering and all authored LOD banks
are preserved. GPU alias/pass validation and launcher delivery belong to the
separate root integration checkpoint; the CPU freeze makes no GPU/FPS claim.

The native high-refresh presentation path now samples released grenade positions
between the previous and current completed snapshots on every render frame,
using the same local fixed-step remainder or LAN snapshot fraction as player
positions. `grenade_render_position` matches the full salted datum, definition
and grenade type, and requires both endpoints to be released. New owners,
attachment transitions and recycled slots use the current position. Removed
projectiles are omitted by the current snapshot. The rendered position feeds
the existing model transform, bounds, LOD and lighting query together; retained
forward/up and all authoritative projectile state remain unchanged. This is a
requested native presentation enhancement, not a claim of original Xbox
snapshot interpolation. Simulation, collision, fuse and detonation callbacks
still run only at the existing 30 Hz clock. The asset-free
`native_grenade_presentation` test covers frag/plasma fractional samples,
endpoints, tick continuity, bounce and lifetime cuts. The actual Vulkan grenade
runtime probe also requires uniform mesh translation between renders of the
same simulation tick for each local view.

Reproduce original and native controls into new destinations:

```sh
.tools/python/bin/python tools/oracle_grenade_scene_state.py --output NEW_ORIGINAL
.tools/python/bin/python tools/extract_grenade_scene_constructor.py --output NEW_CONSTRUCTOR
.tools/python/bin/python tools/pair_grenade_scene_attach_flags.py --output NEW_ATTACH_PAIRS
.tools/python/bin/python tools/export_grenade_scene_fixtures.py NEW_ORIGINAL/report.json NEW_FIXTURES
cmake --build build-animation-event-asan-20261008 --target source_grenade_scene_tests -j2
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 .tools/python/bin/python tools/probe_source_grenade_scene_maps.py --binary build-animation-event-asan-20261008/source_grenade_scene_tests --fixtures NEW_FIXTURES/fixtures.tsv --output NEW_SAN_MAPS
```
