# Original singleton netgame weapon construction and controls

`tools/differential_source_netgame_weapon.py` executes the supplied retail
`default.xbe` instructions, checked against SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
Each executed instruction is checked against the unchanged executable bytes.
The frozen fixture is `tests/fixtures/retail_source_netgame_weapon.tsv`; its
provenance file records the executable, all 21 cache hashes, scaffold hash,
fixture hash, executed instruction digest, counts, and service boundaries.
`analysis/multiplayer-shading/source-netgame-weapon-original.json` also retains
each literal 0x90-byte scenario row, singleton item record, descriptor, weapon
metadata, constructor admission, complete node0, and cold item state.

The native CPU test compares 3,014 cases and 30,993 original output words:

| Kind | Cases | Original authority and checked outputs |
| --- | ---: | --- |
| B | 465 | `15AAD0` singleton stock-class0 selection through `B7930` and original `B7B40` entry: all 49 descriptor words and `10DAD0` scale |
| B complete subset | 31 | Complete untouched `15AAD0` constructor return: 13 node0 words and 12 cold state fields |
| S | 1,386 | `10DAD0` held/unheld, game-type, finite authored scale, positive and clamp boundaries |
| U | 155 | Complete `10B4D0` / `10B850` calls, including registered/unregistered and absent/present collision controls; result and item fields |
| T | 1,008 | `15AE70` timer controls through the original allocation-request entry: request, actual row handle, timer, and empty counter |

All 465 singleton descriptors across the 21 caches are retained. There are 45
authored levitated singleton rows: 31 have no animation graph and no source
spin flag; 14 have an animation graph whose full constructor remains outside
this proof. Ground-item settling is also outside this proof. The native helper
rejects weighted collections, nonstock options, vehicle classification,
unrecovered constructor graph/spin/ground paths, and unsupported update owners.

## Literal constructor state and root

The complete subset contains energy blades, battle rifles, and covenant
carbines. Original `B7B40` sets bit9 when the authored `hlmt+0xC` collision model
is present. `10B2C0` sets bit12. The final cold flags are therefore `0x1000`
for the 12 energy blades and `0x1200` for the other 19 constructors. This
definition-derived bit is independent of runtime item collision field `+112`.

| Field | Original cold value in all 31 constructors |
| --- | --- |
| Parent `+14`, ignored object `+14C`, item owner `+154`, object owner `+C8` | NONE |
| Frozen control `+C1` | 1 |
| Control `+10A` | 0 |
| Runtime collision model `+112` | -1 |
| Item flags `+12C`, remaining ticks `+12E` | 0 |
| Weapon flags `+16C`, power fraction `+184` | 0 |

The shader fields are original constructor outputs. In the recovered
`FF5F0` / `10CF50` predicate, these zero fields yield `turned_on=1`. No material
name or invented sword-power default supplies this result.

`10DAD0` uses the original positive authored scale and clamps to **0.5 through
3.0**. Nonpositive values and held items use 1. The energy blade's authored
multiplayer scale is 1.25. Node0 is the source object frame composed with the
actual model's default node0 local transform. Its local transform is identity
for the sword. The three admitted carbine rows (Headlong 6, Containment 12 and
13) have nonzero local root translation; the direct object frame alone differs
from the original constructor node0.

## Explicit services and validation limits

The inherited cold constructor scaffold supplies successful zeroed object
storage at `BC280`, successful nonrelocating extension storage at `18E340`,
and no allocation pressure at `BF380`. It does not replace constructor,
selector, variant, graph, math, transform, bounds, shader-input, scale, or timer
instructions. The engine-mode callback is an explicit timer input.

The actual source pools at `4EA93C` (`effe`), `4ED28C` (`lsnd`), and `4E030C`
(`ligh`) have explicit zero capacity. Their original allocation routines run
and return NONE. `BECA0` uses 24-byte widget rows; `lsnd` dispatches to
`188F90`, while `ligh` dispatches to `C0230`. These are the only nonempty widget
groups in the complete subset. `cont` uses a different service and remains
rejected. Empty widget pools establish this cold constructor branch; they do
not prove live effect, sound, or widget-light production. Physics storage is
also explicitly empty, the cold object registry is disabled, and no BSP or
global activation chronology is claimed.

For U controls, registered bit8 is explicitly supplied to the actual source
item. The first original update preserves the entire constructed item body.
The additional controls verify that the only possible write in this branch is
bit29 when registered with a valid collision field. Every byte outside the
flags word is compared against the original body, including pose and velocity.
Owner `+C8` with an authored `+104` function block requires `16EB20`; the native
helper rejects that unported service.

Timer fixtures stop at original `15AAD0` entry and preserve its unbind and
allocation services as separate caller work. The test also verifies the
native caller contract: unbind precedes allocation, a failed allocation leaves
NONE, and row timer/empty counters reset. No allocation-success return is
substituted in the original timer controls.

The test is a CPU comparison against frozen original outputs. It does not
prove GPU rendering, Xbox behavior, a live multiplayer match, general item
settling, animation-graph item poses, or successful native world activation.

Run the focused native test with CTest's `retail_source_netgame_weapon` name.
Regenerate and compare the fixture with the repository's Unicorn environment:
`.tools/python/bin/python tools/differential_source_netgame_weapon.py --native
<path-to-source_netgame_weapon_tests>`.

## Native lifetime and rendering integration

`SourceNetgameWeaponScene` prepares the admitted singleton, levitated,
no-graph/no-spin rows before Renderer allocation. It retains all six exact
geometry LOD collectors and every authored shader LOD. The model's actual
default node0 contributes to both its WORLD geometry and its regular cached
query sphere; the authored item scale remains separate from that node bank.
Source initial change colors are applied before animation. Ordinary per-view
lighting uses the existing source cache/query helpers with the actual cold
frozen bit. No captured live pose or pixel tint is inserted.

The native Match pool owns actual full salted object handles. Each row enters
the original `15AE70` ordinal-modulo-rate phase at the actual simulation tick.
Allocation retains the 196-byte source descriptor and records that tick. The
bounded source item update preserves the constructed levitated pose. Local and
host views submit these actual objects; LAN client world-item lifetimes are
not yet replicated. Inventory pickup, moving/settling items, graph/spin owners,
and live effect/light/widget activation remain separate unported services.

The all21 CPU integration exercises **31 source rows, 399 resident material
slots and 89,232 selected vertices**, with exact birth phases, salted-owner
isolation, row associations, same-frame selection and actual `turned_on`
bindings. These resident counts include the complete LOD union and are not
counts of simultaneous submitted passes.

The three carbines require an explicit owner camouflage scalar. The unchanged
`336F0 -> 33850 -> 33670` producer terminates with zero for the actual type2
owner whose parent is NONE. Attached owners are rejected by the bounded helper.
This explicit input admits the authored `meter_active_camo` display pass; the
existing scalar remapper and animation baseline retain its source destination.
The carbine's lower environment variant is the original
`stencil_env_super_fast`, with declaration `[0,4,5,6]`, its own cubemap and
original additive state. It is executed with those literal inputs rather than
replaced by the bumped environment variant. The display's separate custom
`ammo` input now uses the [proved constructor magazine and authored function](#cold-magazine-state-and-carbine-authored-ammo-input).

`tools/verify_multiplayer_shading.py --capture-tick 6` supplies an explicit
no-action input file and captures at the actual native clock. This reaches the
Zanzibar row5 birth at tick5; the earlier frame3 default could finish before
that source phase. The input, executable and SPV hashes are recorded. This
tick6 reference does not reach every row: Headlong's carbine is row6 and
Containment's are rows12/13. The full all-map lifetime capture uses
`--capture-tick 31`, after every admitted row's ordinal-modulo30 birth phase.

`analysis/multiplayer-shading/live-netgame-sword-final/provenance.json` records
a fresh retained-camera/player/device diagnostic at tick6. Its immutable
executable and matching shaders are in that directory's `snapshot`, executable
SHA256 `d5162def8ef49dbd5dcd26d66b56a59daf4a1c60a24b5e11ea96fde86236f4f9`.
The sword is visibly present from its source row. World-region MAE is
0.721526042 RGB bytes out of255 (782 channels above8); first-person MAE is
0.619677939 (32 above8). Original pixel(206,168) RGB(173,129,217) compares with
native(122,125,183). The original shader clock and coverage-driven mip
residency are not restored, so this is neither exact sword color nor full-frame
equality. The fresh Headlong/Containment/Zanzibar one/two/four-player run at
1280x720 tick6 passes9/9 with zero SDK/synchronization diagnostics; literal
images and immutable hashes are recorded in `live-netgame-weapons-targeted`.

## Unattached weapon camouflage and the carbine display

`tools/oracle_source_unattached_weapon_camouflage.py` independently reexecutes
the 31 complete cold constructors and calls the unchanged
`336F0` index37 → `33850` → `33670` owner consumer on each actual source body.
Every body has type2 at `+AA` and parent NONE at `+14`; every returned word is
positive zero. The complete source body remains unchanged. The report retains
each body hash and actual object-header bytes, and ties the construction to
the preceding constructor proof hash and literal cache hashes.

The source begins with `BADC0` handle validation. A nonunit follows its actual
parent chain until it reaches type0, whose `+2B0` scalar is clamped to 0–1.
With parent NONE it returns zero without reading a unit-layout scalar from
the weapon. Thirteen declared controls prove the dependency: a type0 parent
at 0, 0.5, or 1 produces that value; a nonunit parent whose own parent is NONE
produces zero; a null parent body produces zero; a direct type0 owner reads
its own scalar. A weapon's unrelated `+2B0` word cannot supply camouflage.
The native helper admits only an unattached weapon. An attached weapon
requires the actual source parent producer, even when an isolated source
control happens to return zero.

The literal Headlong and Containment `carbine_display` shaders each have a
single `meter_active_camo` pass. Its packed extern range is `0x0200` and its
only remap is `0x25030480`: source37, scalar alpha, destination bank1 slot2.
Its fragment range is empty. The unchanged `19CA0` binder is executed on a
declared poisoned 60-word program bank at camouflage 0, 0.5, and 1. The full
six resulting banks are frozen; the alpha bytes are respectively 0, 128,
and 255, with all other declared bank fields preserved.

The existing CPU test reads
`tests/fixtures/retail_source_unattached_weapon_camouflage.tsv`: 31 original
weapon-owner results, 13 parent/type controls, and six full alpha banks total
404 source output words. It checks the literal cache remaps, native packing,
explicit carbine material admission, rejection of absent camouflage context,
and alpha retention through ordinary animation reset. This proves the owner
input and this authored binding. It does not prove a live parent chain,
complete pass assembly, GPU execution, or a framebuffer result.

## Cold magazine state and carbine authored ammo input

The carbine display also reads an authored objectfunction, rather than an
FF5F0 built-in export. The actual Headlong export is `0x04000B3F`; the actual
Containment export is `0x04000B35`. The literal objectfunction bank is at
weap `+64/+68`, with 32-byte records. Both carbines have a single flags0
record importing built-in `0x1200056A` and exporting their cache-specific
StringID. Its authored 20-byte curve is the identity0–1 scalar curve, with
no enable, threshold, or scale input. This bank is distinct from the raw
weap `+104` predicted-resource block consumed by the item C8 service.

The full unchanged cold constructor supplies the ammunition. `FDAD0` reads
signed shorts at each magazine record `+6` (initial rounds), `+A` (loaded
capacity), and `+C` (reserve capacity), using the actual weap `+2C0/+2C4`
bank and 0x5C-byte record stride. `FDB9A` selects the signed minimum of initial
rounds and loaded capacity, and `FDB9D` stores it at object `+22C` with a
16-byte magazine stride. `FDBAE/FDBB3` stores the signed minimum of initial
minus loaded and reserve capacity at object `+22A`; the low16bits are stored
even if the signed difference exceeds a short. Actual BR metadata108/36/108
produces loaded36/reserve72. Actual carbine metadata36/18/72 produces
loaded18/reserve18. The12 admitted swords have no magazines and retain the
zeroed allocator storage. All19 magazine-bearing constructor stores are
observed at original `FDB9D`.

`FF5F0` selector `0x1200056A`, at `FFAA9`, selects magazine0, divides its
signed loaded short by signed loaded capacity, and clamps the SSE result to
0–1. Missing magazines or capacity0 return zero. Its active byte is true
exactly when the result exceeds zero. The three actual cold carbines return
1.0/active1. A direct `FF5F0` call with their authored export returns false
without writing either poisoned output; the complete `BAB40` resolver finds
the authored function and invokes `BA8C0` through the original type2
`108D90` dispatch. The resulting export is 1.0/active1. Defaulting these
world-item exports to zero is therefore outside the proved behavior.

Both actual carbine display overlays are executed through complete original
`1B230`, with the literal original `BE8B0` caller adapter bound to the actual
source owner. The adapter calls `BAB40`; neither callback code nor a computed
function result is substituted. Both cold overlay fractions are1.0. Their
literal input/range StringIDs and curve bytes are retained. The explicit
context binding proves this bounded shader call composition and does not
claim the complete original scene's callback-registration lifecycle.

The standalone oracle now writes two further frozen fixtures. The magazine
fixture contains31 complete cold constructor banks and9 declared unchanged
`FDAD0` controls, totaling200 output words. The carbine fixture contains3 cold
owners and33 declared magazine-count/capacity/loaded controls, including both
authored overlays for every case, totaling360 output words. Declared controls
modify only disposable emulated metadata/body inputs; the disc caches remain
unchanged. Native tests compare the exact loaded/reserve shorts, full built-in
and generic results, literal function metadata, both authored overlay
fractions, and explicit rejection of seven unproved function branches. The
held-weapon input API remains unchanged. This adds560 exact CPU output words
to the404 camouflage/binding words and30993 birth/update/timer words.
