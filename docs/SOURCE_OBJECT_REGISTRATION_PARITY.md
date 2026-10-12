# Original object creation and spatial registration

The source-backed location, membership and visibility helpers require an actual
ordered registration history. `SceneObjects` currently supplies native lifetime
handles and creation ticks; it does not supply the complete original registration
packet or reproduce all original successful allocations. This audit separates
verified construction attempts, cached primitive inputs, registration algorithms
and the remaining live services.

The authority is `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
All independent oracles execute its unchanged instructions. No live Xbox session
was changed or read for this audit, and existing cold constructor scripts and
reports were preserved.

## Two ordered creation passes and two registration paths

`18F000` calls `137CA0`, `D5560(0)`, `C09C0`, `137E40`, then
`11C1B0(activeBSP,1)`. `D5560` visits type0 through12 ascending, and authored
placement ordinals ascending within each supported type. Descriptors with either
scenario placement or palette offsetNONE are skipped. `D59F0` admits placement
flag0 clear and a name, valid persistent definition, or derived creation policy
other than1. `BF0F0` then applies name/resource/game/authority/factory gates;
`BF380` follows each attempted factory call. An admitted attempt is not a
successful allocation or a final pool slot.

`137D40` invokes the 68-row activation table at `440DE8` in increasing `0x24`
(**36-byte**) stride. Object activation is row27, offset`3CC`, function`B6BB0`.
It enables registration, then runs `BAEB0` over occupied nonzero-generation
headers in ascending actual pool-slot order. Roots with parent`object+14==NONE`
and object flag7 clear call `BF510`, followed by `B8600` with the supplied
location packet **regardless of the return from `BF510`**. `108960` invokes the
object's ordered inheritance activation callbacks for every iterated owner,
including owners excluded from root registration. Mutating callbacks can affect
later occupied slots; a static sorted list is not the general source iterator.

Automatic BSP placement is later activation row57, offset`804`, function
`D4E30→D5640`. `D4E30` suppresses this pass only when interface pointer`510C50`
exists and both bytes`+5` and`+7` are nonzero. `D5640` again visits type0 through12,
then placement ordinal ascending. Its existing-placement bitmap is captured
before construction by actual `BAEB0`; only runtime source0/1 owners contribute,
using type and original placement ordinal. It excludes existing, named, valid
persistent/damageable definitions and placement flag0. Manual policy2 requires
the selected BSP mask bit. Other policies require originNONE/current or `D59F0`
true. It runs pressure housekeeping before each supported type and after each
factory attempt.

These later constructors run with registration enabled. `B7B40` first produces
bounds with `BD020`. It supplies the caller's valid seed location when present;
otherwise `BF510` success supplies its packet and failure passes NULL to
`B8600`. This differs from the unconditional supplied packet in `B6BB0`.

`source_scenario_initial_creation_attempts` and
`source_scenario_bsp_creation_attempts` already expose the verified type/ordinal
order and placement predicates. Their outputs still precede `BF0F0` admission.
`LevelContent` stores arrays in type order6,0,1,3,2,7,8,9,10,11. `SceneObjects`
now sorts its initial and missing-BSP constructor attempts by type and authored
placement ordinal before retaining its separate native final admission gates.
The supplied 21
multiplayer initial attempt sets happen to lack a mixed admitted lower-type and
scenery ordering conflict. Headlong's BSP raw type1 attempts precede scenery,
but those seven vehicle definitions areNONE. Existing successful-handle
expectations should not be changed solely to fit an ordering theory. The new
mixed fixture independently executes13 source cases with32 literal placement
records from `03a_oldmombasa`. Its initial25 attempted keys become25 admitted
native allocation records, including three slots subsequently removed by the
literal manual-BSP policy. Three empty-registry BSP controls admit5/6/5 native
records in source order. Content-order negative controls differ. The original
factory service returnsNONE, so these native successful allocations do not
claim captured original successful handles.

## Factory game admission is an explicit context

The first `D5060` predicate takes placement type, placement flags, scenery
`+5A` mask, source game-options type`+8` and engine`+180`:

1. Begin with `admitted = !(placement_flags & 64)`.
2. For type6, game type2, and a signed16 mask other than0 or`7F`, engine1/2/3/4/7/8/9
   **replaces** admission with low-byte mask bit0/1/2/3/4/5/6 respectively.
3. Engines5/6 or any other value retain the first predicate. Type7 and type11
   never enter the scenery mask branch.

Flag0 is handled by the earlier placement predicates; it is not consumed by this
`D5060` branch. `BF0F0` subsequently calls authority predicate`A7640` when requested
by its caller. Generic scene code needs actual game-options inputs before it can
apply this branch. Map multiplayer metadata alone is not an engine producer.

The unchanged source gate passes **2,160 isolated cases**, including game
types0/1/2. The frozen fixture also retains410 literal scenery records from all
21 supplied maps, their original factory gate outputs and their original
initial/empty-registry BSP attempted-key membership. None of these maps has a
scenery placement combining flag64 and a specific mask
other than0/`7F`, so the replacement behavior is a general correctness fact, not
a claimed current-map visual symptom.

Original `19D220` maps UI category0 (Slayer) to **source engine2**, whereas UI
category4 (CTF) maps to source engine1. The retained original20 Zanzibar
game-options bank independently corroborates game type2 at options offset0
(globals`+8`) and engine2 at options offset`178` (globals`+180`). Its unchanged
4,376-byte `zanzibar-rng20-game-options.bin` has SHA-256
`f1c63fa6005bb98685c556b951620579f25f55102412d632b28aa44281f612ed`.
Source engine ordinals must not be substituted with UI category ordinals.

Literal raw-cache fields and unchanged `D5060` execution establish392 scenery
factory candidates under the generic flag0/flag64/definition gate versus278
under explicit source game type2/engine2. The114 excess candidates occur on20
maps; Warlock's candidate set is unchanged. These are factory candidates, not
an observation of successful original allocations, registration or lifetime.

| Map | Authored scenery | Generic candidates | Source Slayer candidates | Excess |
| --- | ---: | ---: | ---: | ---: |
| ascension | 7 | 7 | 2 | 5 |
| beavercreek | 31 | 31 | 27 | 4 |
| burial_mounds | 32 | 32 | 27 | 5 |
| coagulation | 11 | 11 | 4 | 7 |
| colossus | 5 | 5 | 1 | 4 |
| cyclotron | 11 | 11 | 2 | 9 |
| foundation | 5 | 5 | 0 | 5 |
| headlong | 26 | 26 | 18 | 8 |
| lockout | 5 | 5 | 0 | 5 |
| midship | 33 | 33 | 28 | 5 |
| waterworks | 21 | 21 | 14 | 7 |
| zanzibar | 36 | 34 | 27 | 7 |
| backwash | 19 | 9 | 4 | 5 |
| containment | 19 | 19 | 14 | 5 |
| deltatap | 14 | 14 | 9 | 5 |
| dune | 18 | 12 | 6 | 6 |
| elongation | 50 | 50 | 45 | 5 |
| gemini | 26 | 26 | 23 | 3 |
| triplicate | 17 | 17 | 9 | 8 |
| turf | 7 | 7 | 1 | 6 |
| warlock | 17 | 17 | 17 | 0 |
| **Total** | **410** | **392** | **278** | **114** |

`SourceSceneGameContext` now carries the literal game type and engine through
the trailing optional `Simulation` and `SceneObjects` constructors.
`MultiplayerMatch` implements FFA Slayer and supplies `{2,2}` before
`Simulation::scene_` is initialized. This route is independent of map names.
An absent context preserves existing campaign constructor/script behavior.
An explicit context uses `source_scenario_factory_game_allowed` for initial,
missing-BSP and manual constructors, and `SceneObjects::restore` restores it
with the retained scene. A setter after construction would be too late to
repair attempted allocation order, RNG consumption or native handles.

Placement flag0 remains outside the factory game predicate: automatic
placement excludes it, while explicit script creation can pass it when the
captured factory gate admits. In particular, a specific multiplayer mask can
replace an initial flag64 rejection. A second unconditional flag64 check would
violate the unchanged branch.

## Complete initial spatial primitives

New `source_object_primitive.hpp/.cpp` implements unchanged `B88E0→16B430`.
Its `SourceObjectPrimitiveInput` uses actual header flags`+2`, object flags`+4`,
signed type byte`+AA`, retained spatial sphere`+40/+4C`, and explicit type0/type11
extension fields. It returns the existing exact 20-byte
`ObjectLightSpatialPrimitive`. It does not choose a list or register an owner.

For noncollidable object flag9 clear, primitive flags are0. Otherwise:

| Primitive bit | Actual source predicate |
| --- | --- |
| 0 | Header flags`+2` bit4 or object flags`+4` bit0 |
| 1 | Object flags bit20 |
| 2 | Object flags bit22 clear |
| 3 | Object flags bit31 clear |
| 4 | Type0 and unit byte`+10A` bit2 |
| 5 | Type0 and unit byte`+348` bit3 |
| 6 | Type11 and prop word`+12C` bit1 |

`B7B40` produces collidable bit9 from valid object model`+38` and model collision
reference`+0C`. `BF9A0` produces bit31 only with model render and collision
references and positive render-model counts`+74` and`+7C`.
`11BBF0` initializes prop`+12C` to0, then sets bit1 from crate-definition`+BC`
bit0 or a present physics model whose shapes all have motion type1/2; an empty
shape bank also passes that original loop. These are independent source fields,
not a deduction from a material name or PRT capability.

Read-only observations replay the same independently reset successful-storage
cold factory prefixes, stop before `B8890`, and execute untouched `B88E0/16B430`
afterwards. All742 fixtures have header flags`0A`, parentNONE, registration flag8
clear and locationNONE. No later runtime flag producer is inferred.

| Cold kind | Owners | Collidable | Noncollidable | Actual primitive flags/count |
| --- | ---: | ---: | ---: | --- |
| SCEN | 278 | 80 | 198 | 0:198; 12:80 |
| MACH | 49 | 38 | 11 | 0:11; 12:29; 4:9 |
| BLOC | 415 | 390 | 25 | 0:25; 12:338; 76:51; 4:1 |

Prop`+12C` bit1 is set for53 owners, including51 collidables. The helper matches
**6,630 cases / 33,150 output words bit exactly**, including all742 retained cold
owners, every relevant classification branch and signed type boundaries. The
frozen independent fixture contains1,478 cases /7,390 output words. Both
production-linked private CTests `retail_source_object_primitive`,
`retail_scene_creation_order` and `retail_scene_game_context` pass together in
regular and ASAN/UBSAN configurations with leak detection enabled and no
findings (3/3 each). The context test validates all21 actual native match scenes,
automatic BSP construction, explicit creation, retained context restoration and
campaign compatibility against the frozen source outputs.

## Registration and removal events

`B8600` with supplied location keeps the old location/header cluster if the new
cluster isNONE and sets flag18. Its NULL branch tries primitive center`+40`, then
root position`+64`; it does not invoke `BF510`'s separate sphere-leaf fallback.
A valid location copies the original8 bytes and clears flag18. Object flag21
requests an all-cluster mask over the actual authored cluster count.

The function selects collidable bank`4DE2E0` or noncollidable bank`4DE2D4` from
actual flag9, constructs the source primitive, calls `1CAC60`, and sets object
flag8/header flag6. `1CAC60` prepends each successful membership; its owner and
membership allocations are separate. Thus registration in ascending pool order
produces reverse per-cluster head order. Allocation failure can leave partial
links despite registration flags being set. Current `ObjectLightSpatialLists`
explicitly assumes successful original allocations and does not model pressure.

| Original event | Required spatial action |
| --- | --- |
| `BDEF0`, unregistered | Write selected primitive; no registration |
| `BDEF0`, registered, unchanged | Keep retained primitive and links |
| `BDEF0`, registered, squared move greater than source`.01` or radius differs | `B87B0`, write primitive, `B8600(NULL)`; reinsert changes head order |
| `B8B70`, registered collidable | Rebuild primitive flags and update20 bytes through`1CADF0`; membership/head order stays |
| `B7430` admitted transform path | Remove before transformation, update bounds when requested, then register with supplied location |
| `B87B0` | Remove full-handle memberships through`1CAE40`, clear registration flags, retain location |
| `B8460` root destruction | Deactivate/remove root, recursively destroy children, type/physics callbacks, free generation |
| `B6C50` BSP deactivation | Visit actual slots ascending; remove registered owners, callbacks, reset location/header cluster; disable registration after loop |

A snapshot of current positions does not encode these different events or the
`.01` retained-cache threshold. In-place updates must not be converted to
remove/insert operations. Script hiding is not header bit4 or source primitive
flag0. Attachment/root normalization and callback-driven children remain actual
services, not fabricated registry entries.

## Proposed live seam

A persistent adapter can compose the existing pure source services from an
ordered event packet. The packet needs:

- actual full-generation handle, native pool-slot rank, live type, parent handle,
  header flags and object flags;
- retained rendering sphere`+30/+3C`, spatial primitive`+40/+4C`, root position`+64`,
  previous8-byte location and header cluster;
- explicit type0 bytes`+10A/+348`, type11 word`+12C`, and selected BSP;
- event kind distinguishing initial-BSP supplied registration, constructor
  seeded/initial/default registration, bound-change default relink, supplied
  transform relink, in-place flags update, unregister, destruction and BSP reset;
- explicit successful owner/membership allocation policy or actual allocation
  outcomes, plus inheritance/attachment callbacks that can mutate the registry.

The adapter owns persistent collidable/noncollidable lists, full-handle
membership/location state and render-object registry. `B6BB0` traversal consumes
actual occupied slot order; registration events update lists in caller order.
Each view then feeds actual portal visibility and these lists to
`source_object_admission`, preserving the source queue association bank.

Native `SceneObjects` handles have a one-based low slot; source object headers
use zero-based slots. Preserve native full-generation identities as native
identities and supply explicit pool rank; do not relabel them as captured Xbox
handles. Native construction/event history can exercise the original algorithm
without establishing equality to the original game's successful global pool
history. Original factories can allocate children, fail, recycle slots or remove
objects through pressure housekeeping. Initial equipment/player callbacks and
intervening BSP callbacks also remain unresolved chronology inputs.

The current SCEN/MACH/BLOC services close the cold cached-sphere and primitive
classification inputs for their742 independently observed owners. They do not
close complete callbacks, post-constructor header/object flag evolution, child
allocation, physics updates, allocator pressure or full successful global
registration. A bounded successful-allocation subset needs those boundaries
explicitly stated; it must not be advertised as full original world admission.

## Artifacts

- `analysis/multiplayer-shading/object-registration-seam-audit.{py,json,log}`:
  unchanged21 initial/42 BSP passes (584 initial, 909 empty-registry BSP and874
  explicit eligible-initial-registry BSP attempts), explicit existing
  registries, game gate, complete per-map candidate ordinals and callback/type
  table inventory.
- `analysis/multiplayer-shading/scene-creation-mixed-original.{py,json,log}` and
  `tests/fixtures/retail_scene_creation_order.tsv`:13 controlled mixed cases,
  32 literal first-four source placement records from`03a_oldmombasa`, block
  counts bounded to four, reversed native input order. Outputs are original
  attempted keys. No successful allocation handle is invented.
- `tests/fixtures/retail_scene_game_context.tsv` and
  `tests/scene_game_context_tests.cpp`:2,160 unchanged factory gate observations,
  410 raw placement/game outputs, explicit native automatic/manual/restore
  controls and all21 `MultiplayerMatch` constructor-context checks.
- `analysis/multiplayer-shading/object-registration-primitives-original.py` and
  separate`{scenery,machine,prop}-registration-primitives-original.{json,log}`:
  actual cold header/parent/classification/primitive fields.
- `analysis/multiplayer-shading/object-primitive-differential.{py,json,log}` and
  `tests/fixtures/retail_source_object_primitive.tsv`: unchanged complete source
  primitive versus the native pure helper.
- `docs/SOURCE_OBJECT_ADMISSION_PARITY.md`: existing location/membership/list and
  visibility-queue contracts and independent proofs.
- `analysis/multiplayer-shading/object-registration-native-proof.json`:
  frozen original fixtures, source/executable hashes, private regular and
  sanitizer linked test logs and exact tested scope.
