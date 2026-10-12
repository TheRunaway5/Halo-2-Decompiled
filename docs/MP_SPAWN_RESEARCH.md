# Original Xbox FFA Slayer spawn selection

The original chooser ranks every authored player-start position by the sum of
spawn influences plus one random bias, then accepts a strict new maximum only
when the player-shape query allows it. It does not maximize distance from an
opponent. The original FFA identity byte is the player's session membership
slot, subject to the host-property exceptions below; this identity is separate
from a native unit's combat/AI alliance field.

This is a bounded original-instruction contract with fixture world state. It
does not establish complete world collision, live animated marker state, global
RNG startup, complete match scheduling, teams/objective engines, or live Xbox parity.
No production spawn code is changed by this research.

## Authority and reproduction

- Base `assets/disc/default.xbe`: SHA-256
  `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
- Map Pack title update 1.1
  `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`: SHA-256
  `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
- Actual scenario/multiplayer-global records from all 12 original disc maps and
  nine Map Pack caches. Each cache's hash, start offsets, static-zone offsets,
  overrides and runtime/constant offsets are retained in the generated report.
  Lockout additionally verifies SHA-256
  `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.

Run:

```sh
.tools/python/bin/python tools/oracle_mp_spawn.py
```

The tool writes only ignored evidence under
`analysis/multiplayer/integration/spawn-source`: `report.json`,
`base-instructions.txt`, and `tu11-instructions.txt`. It maps the untouched
original executable sections; no instruction bytes are replaced. The report
records the tool hash and Python/Unicorn/Capstone versions. The instruction
listings record hashes for each exported range. The collectors use actual
original pool iterators and Slayer callbacks, rather than native code.
Authority: [oracle](../tools/oracle_mp_spawn.py),
[TU instruction evidence](../analysis/multiplayer/integration/spawn-source/tu11-instructions.txt),
[base instruction evidence](../analysis/multiplayer/integration/spawn-source/base-instructions.txt).

| Base | Update 1.1 | Responsibility |
| --- | --- | --- |
| `14EF00` | `150640` | Complete chooser, private seed, spawn-count ownership |
| `23AEA0` | `23CFA0` | Read multiplayer defaults and scenario overrides |
| `23B060` | `23D160` | Sum the influence list at a candidate position |
| `23B170` | `23D270` | Gather authored static initial/respawn zones |
| `23B3F0` | `23D4F0` | Static-zone height fallback and absolute values |
| `23B500` | `23D600` | Gather moving vehicle/projectile influences |
| `23B8E0` | `23D9E0` | Gather current players and stored death positions |
| `23BA10` | `23DB10` | Append an influence of a configured type |
| `23BA90` | `23DB90` | Score plus a random draw |
| `23BB70` | `23DC70` | Transform and query the player collision shape |
| `23BC40` | `23DD40` | Gather the full influence list in source order |

These entries and their original instructions are exported by the oracle.
The readable base reconstruction is
[unknown_23b500.cpp](../tools/reference/halo2-decompiled/src/unknown_23b500.cpp).
The TU exports are under
[analysis/map-pack-update/functions](../analysis/map-pack-update/functions).
The original instructions settle ambiguous decompiler types and calling
conventions; in particular, `23D160` is not an empty loop despite that appearance
in the current Ghidra C export.

## Data contract

Each scalar below is an original little-endian 32-bit float unless another type
is stated. The oracle reads these bytes from the cache and relocates only
pointers into fixture memory.

The scenario's player starts are `scnr +100`, count/pointer, stride `34`. Only
position floats at `+0/+4/+8` feed the score. The chooser loops through every
record in authored order; it does not inspect start team, BSP, game-selector,
spawn-selector or campaign-representation fields. A native active-BSP filter is
a separate native boundary and must not be represented as an original chooser
predicate. Authority: complete `14EF00`/`150640` and
`23BA90`/`23DB90` in the instruction evidence above;
[player-start schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/scnr.xml).

The `mulg` Runtime block has stride `568`. Dynamic heights are upper at `+150`
and lower at `+154`. Twelve influence types occupy `+180 + type*1C`: inner radius
at `+0`, outer radius at `+4`, weight at `+8`; the remaining bytes of each row are
not consumed by the recovered settings initializer. The Multiplayer Constants
block is at Runtime `+530`, stride `160`:

| Constant offset | Original consumer |
| --- | --- |
| `00` | Maximum random spawn bias |
| `08`, `0C`, `10`, `14` | Explosive projectile weight, inner radius, outer radius, lead seconds |
| `18`, `1C`, `20`, `24` | Moving-vehicle minimum speed, weight, radius cap, lead seconds |

Authority: `23AEA0`/`23CFA0`, `23B500`/`23D600`, `23BA90`/`23DB90`;
[mulg schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/mulg.xml).

Scenario Spawn Data is `scnr +318`, stride `60`. Its first row supplies lower
height at `+0` and upper height at `+4`: a nonzero value replaces the corresponding
dynamic default. Dynamic Spawn Overloads at `+48` are stride `10`, with signed
16-bit type at `+0` and raw inner/outer/weight floats at `+4/+8/+C`. The original
initializer applies them in authored order to the selected type. These values
are not guessed constants or a new tuning policy. Authority: `23CFA0` original
instructions and [spawn-data schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/scnr.xml).

Static Respawn Zones are Spawn Data `+50`; Static Initial Spawn Zones are `+58`.
Both are count/pointer blocks, stride `30`:

| Zone offset | Field |
| --- | --- |
| `00` | Name StringID, u32 |
| `04` | Relevant-team bit mask, u32 |
| `08` | Relevant-game bit mask, u32 |
| `0C` | Objective flags, u32 |
| `10`, `14`, `18` | Position X, Y, Z |
| `1C`, `20` | Lower and upper height |
| `24`, `28`, `2C` | Inner radius, outer radius, full weight |

FFA ignores the relevant-team mask. A zero relevant-game mask accepts every
engine; Slayer accepts a nonzero mask only if bit 0 is set. Slayer's zone
callback returns true and adds no condition for objective flags `+C`. For each
height independently, a nonzero zone height wins, else a nonzero scenario height,
else the runtime default; this **static** height result is passed through `fabs`.
Dynamic shared heights and override radii/weights remain their original raw
floats. Authority: `23D270` and complete `23D4F0`; original TU Slayer object
`487188`, vtable `463B48`, slot `84` points to `A1110` (`true`, `ret 8`). The engine
table initializer `37EB40` establishes that object at engine index 2; these
instructions are also exported.

## Current-state gather and score arithmetic

An influence is exactly eight floats, stride `20`:

```text
[x, y, z, inner_radius, outer_radius, upper_height, lower_height, full_weight]
```

The list has signed 32-bit count followed by up to 128 influences. Append stops
at 128; it does not reorder or replace older influences. Gather order is static
zones, moving objects, player/death-position influences, then the active engine's
extra collector. Original Slayer's extra collector adds nothing (`24DC50` base;
`2B4590` TU, `ret 8`). Authority: `23BC40`/`23DD40`, `23BA10`/`23DB10` and the
inlined append paths in the instruction evidence.

For each influence, `23D160` computes `dx`, `dy`, `dz` as float subtractions.
It computes `d2` as `float(float(dx*dx) + float(dy*dy))`, using SSE scalar
instructions. An influence contributes only if `outer*outer > d2` and
`upper >= dz >= -lower`. The horizontal outer boundary is strict; both vertical
boundaries are inclusive. Within `inner*inner > d2`, or if `inner >= outer`, it
contributes the full weight. Otherwise it computes:

```text
float((1.0 - (sqrt(double(d2)) - double(inner))
                / (double(outer) - double(inner))) * double(weight))
```

The falloff is x87 work with its source operation order and the supplied oracle
53-bit precision mode; it stores a float before adding that contribution to the
running total with SSE `addss`. The accumulation follows list order. Native
verification must preserve those float rounding boundaries and avoid replacing
the calculations with FMA or an all-float square-root/interpolation path.
The fixture CPU state is x87 control word `027F`, status word zero, empty tag
word `FFFF`, and MXCSR `1F80` (round to nearest). These are recorded fixture
inputs; execution of the complete original startup/thread floating-point setup
is outside this suite.
Authority: complete `23D160`/`23B060`; 72 named exact-bit kernel cases in
[report.json](../analysis/multiplayer/integration/spawn-source/report.json).

Current living players use the original player iterator, which walks valid pool
bitmap slots in increasing order and skips player flag bit 1 of byte `+2`.
Unit datum is player `+2C`; unit position is object `+30`. The spawning datum
itself is excluded from the live-player branch. Units with dead bit 2 of byte
`+10A` set add no live influence. Byte
`player+C0` equal to the spawning player's byte selects type 1 (friend); unequal
selects type 0 (enemy). This comparison does not itself test the team-mode flag.
Type rows and shared heights come from the initialized original settings above.
Authority: `23D9E0`/`23B8E0`, including actual `1A2BA0`/`19F240` iterator execution
in the oracle; the friend/dead/enemy cases retain their full gathered lists.

The moving-object iterator admits types 1 and 5. A type-5 projectile contributes
only when its definition's explosive bounds at `+CC` and `+D0` are both positive.
Its influence center is `position + velocity*projectile_lead`; its radii/weight
are from Multiplayer Constants. A living, detached vehicle first contributes
the configured type 4/3/2 for empty/friendly/enemy occupancy. A vehicle above the
strict squared minimum-speed threshold also contributes at
`position + velocity*vehicle_lead`, with inner radius equal to the speed capped
by the authored radius cap, outer radius `float(inner*1.2f)`, and authored danger
weight. The fixture suite executes empty vehicles and explosive projectiles;
occupied vehicle-child worlds remain outside its tested cases. Authority:
complete `23D600`/`23B500` and the oracle's projectile/empty/moving-vehicle cases.

Stored death positions select type 10 only in the iterator branch where the
player's unit datum is NONE or the iterated full player datum equals the spawning
datum. A different player's existing dead unit takes the skip branch, without
falling through to that player's history. In FFA this history branch applies to
the spawning player's own stored history too; initial/respawn bit 3 does not
change its eligibility. Team mode additionally consults the engine's friendship
callback, which is outside the FFA contract.
The state is engine globals `+558 + absolute_player_slot*18`: active byte `+0`,
position `+4/+8/+C`, signed 16-bit initial/nonzero gate `+10`, and signed remaining
countdown `+12`. FFA's collector requires active != 0 and the `+10` gate != 0; it does not use a
positive-only comparison or decay the weight itself. The zero and negative
snapshot fixtures establish that predicate. Authority: `23D9E0`; original death
writer base `15CE70`, reconstructed in
[unknown_157450.cpp](../tools/reference/halo2-decompiled/src/unknown_157450.cpp).
That writer obtains the death position from marker `04000095`, marks an absent
unit invalid, and stores the resulting respawn countdown in both shorts.
The follow-up below establishes the stored-history countdown owner and original
head-marker composition. A native port must not substitute an arbitrary
root/body position for the original marker.

## RNG and chooser contract

The original chooser takes a full u32 player datum in ECX, then three stack
arguments in order: player-definition tag datum (u32), deterministic flag (bool),
preview flag (bool). It returns the signed authored-start index in EAX. The
player datum's low 16-bit absolute slot chooses its player record and contributes
to the private seed; its high 16-bit salt remains part of datum identity for the
live-player exclusion test.

The actual spawning caller TU `1509C0` passes deterministic=true, preview=false
to `150640` (`150A6A..150A71`). The chooser builds a private u32 seed:

```text
((1u << (absolute_player_slot & 31)) XOR options_u32_10 XOR options_u32_14)
    + sign_extend(player_i16_218)             // wraps as u32
```

The option fields are inputs; the oracle supplies `13579BDF` and `2468ACE0` and
does not establish session initialization of those fields. For deterministic
calls each candidate advances this private seed; for nondeterministic calls it
advances global RNG storage `+4` instead. The weapon direction/firing stream is
at global RNG storage `+0`; these spawn paths leave it untouched. Authority:
complete chooser and `23DB90`, plus the recorded intermediate/final seeds in
each oracle row. A preview flag does not choose the RNG stream; it controls only
the successful spawn-count increment.

For **every authored start**, including one later rejected by collision:

```text
seed = seed*0019660D + 3C6EF35F               // modulo 2^32
random = seed >> 16                          // unsigned 0..65535
score = float(double(influence_sum)
              + double(random)*double(executable_float_1_over_65535)
                               * double(authored_maximum_random_bias))
```

The reciprocal is read from original executable data (`45DC50` base;
`4650C0` TU); its float bit pattern is `37800080`. It is not reconstructed from
a decimal literal. This is the original x87
multiply/multiply/add/store order. Seed progression occurs even with zero bias.
Authority: complete `23BA90`/`23DB90`, including global-stream, preview,
zero-bias and signed-count fixtures.

The best accepted score begins at original float `-1000.0` and the selected
index begins at -1. A candidate only reaches the shape predicate if its score
is **strictly greater** than that best accepted score. Failed shape queries do
not update the best score. Equal scores retain the first accepted authored
index. No accepted score above the threshold, no starts, or all candidate shape
queries blocked returns signed -1; no fallback start is invented. The chooser
increments the player's **16-bit** `+218` only on a successful non-preview
return. Signed seeding and 16-bit wrap are verified at counts -1 and 32767.
Authority: complete `14EF00`/`150640`; tie, threshold, zero-start, blocked-all
and blocked-best cases in the report.

With tag NONE (`FFFFFFFF`) the original shape helper accepts immediately. With
a valid player tag, it builds an identity transform, adds original `0.0701f` to
position Z, chooses the shape at definition `+290 +20` when `+28C` is nonzero,
otherwise `+298 +30`, and queries excluded component NONE with filter 9.
A collision-service true result rejects the candidate. The oracle's blocked
cases record the actual transformed original requests but supply that service
result at `1C8B40`; they do not execute Havok/world collision. Authority:
complete `23DC70` and its recorded shape requests.

## Player identity and lifecycle ownership

Original host property updater base `65CB0` / TU `65DD0` assigns the FFA identity
in a precise branch: session `+14 == 1`, requested property byte `+7C != FF`, and
session parameter `+48` bit 0 clear. It writes the **membership slot** (`0..15`)
to property `+7C`. Requested `FF` remains `FF`; with team mode enabled it instead
clamps the signed requested value to `0..7`; outside that session phase it
preserves the current property's team byte. Actual `65DD0`, its actual property
copy service `5AEB0`, and the original `14D600` configuration-copy prefix were
executed on noncontiguous slots 0, 3 and 15 for five phase/team/spectator cases.
The `90`-byte verified property copy to player `+44` maps property `+7C` to player
`+C0`. The copy-prefix execution stops at `14D687` before subsequent name,
notification and color work. Authority:
[unknown_054fe0.cpp](../tools/reference/halo2-decompiled/src/unknown_054fe0.cpp),
`65DD0`/`14D600` instructions and `player_property_assignment_cases` in the report.

The signed spawn count belongs to the player record, not the match or weapon
RNG. TU player creation `14D350` resets `player+218` to zero after initialization
through `14D800`. That initializer sets flag bit 3 of byte `+2` from its new-player
argument. Original round reinitialization `14D210` calls it with false, then sets
bit 3 when the source global `4D6D42` is zero. The static collector selects `+58`
when bit 3 is set and `+50` otherwise. Actual successful spawn `1509C0` clears
the bit at `150DA1`; the chooser itself leaves it intact, and the failed-spawn
return branches leave it intact. These lifecycle instructions are exported;
the oracle supplies the flag/counter at the world-fixture boundary rather than
claiming the whole original allocation/creation pipeline was executed.

The actual player respawn countdown is separately `player+164` (i32). Original
`15DD40` decrements it only after its engine/player eligibility checks; the
stored death-state shorts described above must not be assumed to be that live
counter. Reset base `15DC40` clears each active player's countdown and stored
history active byte. The follow-up establishes history lifetime and its source
tick order; complete countdown service scheduling remains outside the executed
fixture boundary. Authority:
[unknown_157450.cpp](../tools/reference/halo2-decompiled/src/unknown_157450.cpp).

## Verification result and integration boundary

The current reproducible suite passes 72 exact score-kernel cases, 84 complete
base/TU chooser comparisons across all 21 maps (empty/enemy worlds, initial and
respawn), and 17 additional TU chooser fixtures. It also verifies five original
host-property cases with three noncontiguous membership slots each. Every paired
chooser case matches the complete gathered influence list, every candidate's
float bits and private/global seed state, selected index and count progression.
The independent expected scorer imports no native game code.

Common fixture state is fully recorded: spawning datum `00010000`, options state
2, mode 0, team flag false, option words `+10/+14`, active player bitmap, signed
spawn count, input players/objects/history, initialized settings and bias. The
weapon RNG initial value is `12345678`; the separate global spawn stream starts
at `87654321`. Per-map source fields and all gathered influence bits are retained.
Ordinary map comparisons supply tag NONE; the two collision fixtures supply
explicit masks and recorded original shape requests.

For a native fixture reader, `maps[].start_positions` is the complete authored
XYZ list, and `maps[].authored` contains the source zone and setting inputs.
Each `rows[]` case contains `input_state`, exact `influence_bits`, ordered
`scores[]` (index, float bits, private seed and global spawn seed), selected
index/score, pre/post signed spawn count and final stream state. Explicit
synthetic start overrides appear only in `fixture_overrides.starts`. Player
fixture triples are `[source_identity_byte, XYZ, unit_dead]`; death triples are
`[absolute_player_slot, XYZ, signed_snapshot]`. Their test pool/object state is
constructed visibly in the oracle, rather than imported from native gameplay.

An implementation can now import the source metadata and port the chooser,
float/RNG operations, authored static zones and the currently represented live
players with a separate source FFA identity byte. It must expose its unavailable
world-shape service and omitted original object/history inputs as boundaries.
Successful original object creation, collision services, live animated node
state for history-marker capture, occupied vehicles, global RNG startup and surrounding match
scheduling require their own integration evidence. Passing this bounded selector
suite alone does not close the complete multiplayer spawn-parity gate.

## Follow-up: original history expiration and collision admission

Run the separate follow-up oracle:

```sh
.tools/python/bin/python tools/oracle_mp_spawn_lifecycle.py
```

It executes unchanged instructions from both pinned images. It passes 132
history-tick cases, 16 complete original head-marker lookup/composition cases,
18 original death-writer prefixes, 48 original final contact-callback cases and
26 spawn-shape requests. Base and TU state results match for every paired case.
It also executes the original TU floating-point control routine with the actual
active-tick arguments. Real capsule and StringID metadata are retained from all
21 caches. Evidence:
[follow-up tool](../tools/oracle_mp_spawn_lifecycle.py),
[lifecycle-report.json](../analysis/multiplayer/integration/spawn-source/lifecycle-report.json),
[base lifecycle instructions](../analysis/multiplayer/integration/spawn-source/base-lifecycle-instructions.txt),
[TU lifecycle instructions](../analysis/multiplayer/integration/spawn-source/tu11-lifecycle-instructions.txt).
This executes the whole ticker and marker helpers, but stops each death writer
immediately after storing both history shorts, before notification/event work.
The source tick call graph is inspected; the entire game tick is not executed.

### History record lifetime and source tick order

Base `162A30` / TU `1640D0` owns expiration. Its input EAX is masked to the
absolute 16-bit player slot. At engine globals `+558 + slot*18`, it executes:

```text
if initial_gate_i16_at_10 != 0:
    remaining_i16_at_12 = max(sign_extend(remaining_i16_at_12) - 1, 0)
    if remaining_i16_at_12 == 0:
        active_byte_at_0 = 0
        initial_gate_i16_at_10 = 0
        remaining_i16_at_12 = 0
```

The active byte does not gate the countdown. A zero `+10` leaves the record
unchanged, even if its active byte or remaining short is nonzero. Negative
`+10` still enables ticking; nonpositive remaining time expires on that call.
Expiration leaves XYZ untouched and never changes the separate live player
countdown at `player+164`. The history influence keeps its full configured type-10
weight until expiration; the scorer does not interpolate it by remaining time.
The unrelated state bytes `+14/+15/+16` have their own transition logic; mode 4
skips that logic but still executes the history countdown.

TU `15D7C0` calls `1640D0` once for each player yielded by the original valid-slot
iterator, after its engine player callback at slot `3C`. The iterator excludes
player flag bit 1 of byte `+2`. TU active-tick entry `139750` calls the player
spawn/update stage `14EAE0` first, then the game-engine stage `15D7C0`, and
increments game-time `+8` at the end. A false tick flag bypasses these stages.
Thus the source start-of-tick chooser sees history before that tick's expiration
update. Moving expiration ahead of spawn selection changes the source order.
Authority: `13978F`, `1397AC`, `15D8BA..15D8BC` and complete `1640D0` in the TU
instruction evidence.

The FFA death writer base `15CE70` / TU `15E4D0` initializes both shorts from the
low 16 bits of the resulting live respawn countdown. FFA uses countdown mode 2
because the team-mode flag is clear. For supplied penalty `player+168`, it sets
live `player+164` to the maximum of one second in ticks and penalty plus the
variant interval at options `+1BC` converted to ticks. Conversion uses SSE float
integer conversions/multiply followed by x87 `fistp` in the original mode. It
then clears penalty `+168`. The executed prefixes use attacker NONE, which
avoids attacker award/suicide services; they establish the supplied-penalty path.
At fixture rate 30 and interval 5 the result is 150; zero/negative intervals
produce the 30-tick minimum. A 32790-tick delay copies signed -32746 to both
shorts, which expires on the next history-tick call. These are literal source
16-bit stores, not a new saturation rule.

When the unit is NONE, the writer marks history inactive, stores XYZ `(0,0,500)`,
and still initializes both shorts. With a unit it marks history active and
copies the complete original marker helper's world XYZ; the helper's return
count is not used to cancel history capture. Active-tick entry `139750` also
calls original `__control87(9001F, FFFFF)` at `139764`. Executing that routine
confirms precision-control bits `200` (53-bit) and rounding-control bits zero
(nearest), matching the scorer oracle's precision/rounding fixture. Full
startup/thread state and MXCSR initialization remain separate boundaries.

### Original head-marker capture

The original cache StringID table resolves `04000095` to `head`. The player
masterchief multiplayer render model's authored head marker uses node 15;
the elite multiplayer model uses node 19 in the tested Lockout cache. Exact
local translations/quaternions, source file offsets and output marker bytes
are in the report. They are not a fixed offset from the unit position.

Base `B8D30` / TU `B9C20` resolves the marker object, follows its definition
`+38` model datum and the model's `+4` render-model datum, finds the named marker
group (`1D8F00` / `1DC8F0`), and composes its local marker transform with the
current object node matrix (`1D8F50` / `1DC940`). Node matrices come from the
object-relative signed short offset `+116`; region permutations use offset
`+11A`. Marker region/permutation conditions determine which entries match.
The output stores local node transform at `+4` and composed world transform at
`+38`, with world XYZ at output `+60/+64/+68`. The authored marker's extra float
at `+20` is carried to output `+6C`; it is not applied as a scale to marker XYZ.
The supplied current node's scale is applied. Identity and rotated/scaled node
fixtures execute those full original transforms and match exact float bits.

The writer requests capacity 1 and flag false, so the original marker-object
resolution applies. If no model/named marker produces a result, the helper
fills the output from the original object's first node matrix and returns zero
for the nonzero head StringID. The death writer still copies that fallback
world XYZ. Missing-model and missing-marker fixtures establish this fallback;
it is not permission to use root/body position whenever animated head data is
unavailable. Native capture requires the original current node pose or an
explicit unavailable-marker boundary.

### Source player shape and final contact predicate

The spawn helper uses the first capsule subobject at biped pill-block pointer
`+290 +20` when count `+28C` is nonzero; otherwise it uses the translated sphere
subobject at sphere-block pointer `+298 +30`. All inspected multiplayer player
bipeds have two capsules, and the spawn helper chooses the first standing one.
Its radius is float bits `3E333333`, approximately .175; endpoint Z values are
approximately .175 and .55, with X/Y zero. Complete source shape bytes, biped
standing/crouching dimensions and offsets are retained for every cache.
Authority: original `23BB70`/`23DC70` and the real
[biped shape schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/bipd.xml).

The helper constructs an identity transform, independent of the start's
authored facing, and adds executable float bits `3D8F9097` to candidate Z using
SSE `addss` (`45E290` base; `465740` TU). This is the original approximately
.0701 offset. With the capsule above, its vertical bounds are approximately
candidate Z + .0701 through candidate Z + .7951. It passes the actual shape,
excluded component NONE and filter value 9 to the original world query. The
26 helper cases execute that request and its hit-to-reject return behavior;
the world query's hit result is an explicitly supplied service.

Base world-query wrapper `1C5210` / TU `1C8B40` creates a transient transformed
shape volume, adds it to the Havok world, dispatches contact tests, removes it,
and returns the callback's `found_other`. The actual final callback is base
`1C5400` / TU `1C8D30`. For each delivered contact body, it follows parent `+C`
links to the root and only admits a root with type `+18 == 1` and nonzero entity
`+20`. It reads component property key `2001` from the entity; an absent property
uses zero in these instructions. It ignores contacts with a material-bearing
leaf shape, defined by all of:

- Leaf shape virtual type is not `18`.
- Shape metadata `+8` is nonzero and is outside integer sentinel range 1..16.
- The metadata byte `+1E` is not `FF`.

For any remaining contact, excluded component NONE admits it; another excluded
component admits it only when different from the root entity's component.
Admission sets both callback bytes `+4` and `+8` to 1 and never clears them.
The spawn caller supplies NONE, so it does not exempt its prior player body.
The 48 callback cases execute parent traversal, property search and this entire
predicate. Only shape virtual `getType` is a supplied service in these cases.
The executable callback establishes a contact-admission contract; it does not
prove which world bodies the query dispatch will deliver.

World shape relocation/construction, broadphase membership, filter 9's world
collision rules, backend contact generation/tolerance, current live node poses
and integrated tick scheduling remain outside this follow-up's execution.
An active-BSP start filter, a point ray, or an arbitrary native capsule overlap
must not be labeled full original spawn collision parity on this evidence.

## Follow-up: current-unit release, corpse references and replacement order

Run the separate ownership oracle:

```sh
.tools/python/bin/python tools/oracle_mp_spawn_ownership.py
```

The 108 base/TU cases match: 42 complete assignment calls, six body-depleted
prefixes, 12 death-cleanup prefixes, six release-to-history-consumer chains,
18 complete player object-deletion callbacks and 24 complete spawn functions.
The deletion callback's final control notification is supplied. Spawn cases
supply representation lookup, world contact result, object creation,
customization and loadout services; their original chooser, placement-record
construction, assignment and final player-state instructions execute.
The assignment calls and death-release prefixes use local user NONE, network
object datum NONE, no weapons/children and an explicitly supplied object pool.
Death-prefix biped ragdoll state 3 avoids animation services. They stop after
the original ownership release, before subsequent weapons, seats and actions.
They do not execute the whole damage path or create a live collision body.

Authority and reproducible evidence:
[ownership oracle](../tools/oracle_mp_spawn_ownership.py),
[ownership-report.json](../analysis/multiplayer/integration/spawn-source/ownership-report.json),
[base instructions](../analysis/multiplayer/integration/spawn-source/base-ownership-instructions.txt),
[TU instructions](../analysis/multiplayer/integration/spawn-source/tu11-ownership-instructions.txt).
The report records ordered source writes and entry addresses for every case.

### Ownership fields and release producer

These fields are independent of the source FFA identity at `player+C0`:

| Record | Offset | Type | Original role |
| --- | --- | --- | --- |
| Player, stride `21C` | `+2C` | 32-bit datum or NONE | Current assigned unit |
| Player | `+30` | 32-bit datum or NONE | Previous unit/corpse reference |
| Unit object | `+13C` | 32-bit player datum or NONE | Current player owner |
| Unit object | `+140` | 32-bit player datum or NONE | Previous player owner |
| Unit object | `+10A`, bit 2 | Damage flag | Body depleted; separate from assignment |

The complete binding helper base `14CAD0` / TU `14E240` takes the full player
datum on the stack and the new unit datum in EBX. Comparing only an absolute
player slot loses the original salt. The runtime pool access masks the datum
to its absolute low-16-bit slot, but stores the full supplied player datum on
the unit. If old and new unit datums are equal, the helper does nothing,
including NONE-to-NONE. Otherwise it releases an old unit, resets the player's
latched input fields, then assigns a new unit when non-NONE. Its ownership
stores do not depend on the unit's body-depleted bit. The nested original
active-state helper base `CBF60` / TU `CD3B0` separately forces unit flag bits
0/1 at `+134` clear when body-depleted is set.

The ordinary unit-death cleanup is base `CAA60` / TU `CBDC0`. It first handles
the biped death service, clears player-controlled unit state and schedules the
object for garbage collection with an interval of ten seconds in ticks. That
schedule does not itself delete the corpse. If unit `+13C` is non-NONE, it
performs this exact order (TU store/call addresses shown):

1. `CBE85`: player `+30 = player+2C`.
2. `CBE88`: call `14E240(player_datum)` with EBX = NONE.
3. `14E2B3`: old unit `+13C = NONE`.
4. `14E2B9`: old unit `+140 = full player datum`.
5. `14E2ED`: call the original unit active-state helper with false.
6. `14E2F2`: player `+2C = NONE`.
7. Clear player `+38`, `+3C`, `+17C`, `+17E`; refresh player status.

Base matching stores are `CAB25`, `14CB43`, `14CB49`, `14CB82`.
The cleanup then refreshes player status again before its remaining death work.
It preserves the previous-unit reference, the live respawn countdown and the
history record in the executed prefix. A fixture with no unit-side player owner
skips this release block even if a separately supplied player still references
the dead unit; that inconsistent fixture is a predicate case, not a claim that
such ownership mismatches occur naturally.

The readable base decompile at
[unit_object_type.cpp](../tools/reference/halo2-decompiled/src/unit_object_type.cpp)
incorrectly renders this death call as `function_14cad0(player_index,
unit_index)`. Original instructions `CAAB5` and `CAB28` show EBX was set to
NONE; both pinned images and the executed cases establish the NONE argument.
The executable is the authority for this call.

### Damage-stage order and history eligibility

The body-depleted setter base `D6800` / TU `D7E00` sets object `+10A` bit 2
before its animation notification. Executing through that store proves it
leaves player `+2C`, player `+30` and unit owner fields unchanged at that stage.
Thus setting a native unit's `alive=false` does not represent this source
ownership operation.

The inspected ordinary damage-result consumer TU `DAC40` calls death-event
handling `1ED960` at `DB0CA`, then calls unit damage aftermath `CB410` at
`DB10D`. The event handler calls the history writer `15E4D0` at `1EDE20` when
its victim player datum is valid. The aftermath calls `CBDC0` at `CBA2E` when
damage-result `+4` bit 0 is set. In this FFA path the relevant source stages are:
body depleted, death history captured while the current unit is still assigned,
unit cleanup, assignment released. These call sites are verified instructions;
the whole damage-result consumer and its event/award services are inspected,
not emulated by the ownership oracle. Other force-death, deletion, campaign or
replication entry paths must retain their own service/state conditions.

Executing the original player influence collector immediately before and after
the death-release prefix closes the eligibility boundary. For another player's
dead unit that remains current, the collector skips the live influence and does
not fall through to history. Once the original release makes current unit NONE,
that player's active, nonzero-gate history becomes a type-10 influence. Its
retained corpse datum at player `+30` does not prevent history admission. For
the spawning player's own full datum, the collector uses eligible history even
before release. Executing the original history ticker to expiration removes
the history influence in both cases. All six paired chains use the original
collector, release helper and ticker. No `alive`-to-ownership inference is used.
Authority: complete base `23B8E0` / TU `23D9E0` in the earlier selector evidence,
plus the new `history_admission` report cases.

### Corpse deletion and successful or failed respawn

The original player object-deletion callback base `152CF0` / TU `154440` walks
the valid player pool for deleted object types 0/1. If a player still owns the
object, it saves current to previous, calls the binding helper with NONE and
refreshes status. It then clears player `+174` if matching, and clears previous
`+30` if matching. Consequently deleting the retained corpse clears its datum;
it does not clear death history. Deleting a still-current unit first releases
ownership, then clears the just-retained previous datum in the same callback.
An unrelated object leaves both ownership references unchanged. The complete
callback runs in all 18 cases, with only its final control notification supplied.
Authority: `152D85..152DDD` / `1544D5..15452D`.

Normal multiplayer spawn base `14F2E0` / TU `1509C0` obtains a biped definition,
then calls the original chooser. No chosen start returns false before creation
and binding. A valid chosen start increments player `+218` inside the chooser,
before object creation is attempted. Therefore a later creation failure returns
false but retains that count increment, current NONE, previous corpse datum
and initial-spawn bit. Even a missing biped definition is tested after the
chooser call, so that failure can also retain a successful chooser increment.
The no-starts case retains count zero. These failure cases execute both complete
source spawn functions with the explicit services listed above.

With a non-NONE created unit, the spawn function binds it at TU `150C93`.
The binding helper stores new unit `+13C = full player datum`, new unit `+140 =
NONE`, calls active-state(true), then stores player `+2C = new unit` and
`+30 = NONE` (`14E345`, `14E34B`, `14E370`, `14E37C`, `14E37F`). Subsequent
multiplayer loadout work occurs after binding (`150CB8`). Final player cleanup
sets `+198 = 0`, clears bits 1/2 of byte `+3`, clears `+17C/+17E`, sets `+194`
to one second in ticks, refreshes status, then clears the initial-spawn bit
(byte `+2`, bit 3) at `150DA1` and returns true. Neither the binding helper nor
this executed spawn path clears the history record or changes live countdown
`+164`; the caller's countdown/eligibility stage owns that input. An active
history record can therefore survive a successful respawn, although another
player's live current unit takes precedence over history in the collector.
Source uses a separate preserved/reuse object path when player globals `+AC`
contains a usable unit; that path is inspected but is not executed here.

The preceding multiplayer player stage TU `14EAE0` only considers active
players (flag bit 0 set, bit 1 clear) with current unit NONE. It invokes
`15F3A0` during collection and again immediately before each queued spawn.
That original eligibility routine owns the positive `+164` decrement, displayed
seconds `+170`, phase/lives gates and a 32-tick slot gate outside the first four
engine ticks. The stage uses global RNG word `+0` to randomize multiple queued
spawn attempts. The chooser itself remains a private/global-`+4` stream user,
as established earlier. These surrounding stage instructions are retained in
the report; complete stage execution and event/network services remain open.

## Follow-up: active tick, live countdown and early respawn scheduling

Run the separate timing oracle:

```sh
.tools/python/bin/python tools/oracle_mp_spawn_timing.py
```

This follow-up executes the unchanged active-tick caller, complete early player
stage on a zero-controls input frame, complete countdown/eligibility function,
complete chooser and spawn function with the earlier explicit spawn services,
and the caller's final clock increment. The death event is supplied at the first
world-subsystem boundary after the early player stage. Its original writer runs
through live-countdown initialization and penalty clearing, stopping before
history/head-marker capture. The earlier original body-depleted and ownership
release prefixes then run. No current head pose, history XYZ or live world body
is supplied in these timing cases.

The paired base/TU report contains 84 death-to-respawn timelines, eight
identity/penalty sequences, 48 scheduling boundaries and 14 complete
active-input-flag helper calls. The timelines and scheduling boundaries execute
11,292 original caller ticks; base/TU results agree after removing instruction
addresses. Fixtures use local-user NONE, zero input controls, 30 ticks per
second, engine start tick zero, full player
datums with salt seven, absolute slots 0/3/15 and the pinned real Lockout biped
definition and spawn assets. The report records every tick's before/after timer,
ownership, initial flag and signed spawn count, both eligibility calls, original
notification records, ordered relevant stores and both RNG stream results.

Authority and reproducible evidence:
[timing oracle](../tools/oracle_mp_spawn_timing.py),
[timing-report.json](../analysis/multiplayer/integration/spawn-source/timing-report.json),
[base instructions](../analysis/multiplayer/integration/spawn-source/base-timing-instructions.txt),
[TU instructions](../analysis/multiplayer/integration/spawn-source/tu11-timing-instructions.txt).
The report pins its tool and imported oracle hashes, both executable hashes,
cache hash and emulator versions. It lists every supplied later-world call by
original call site and entry address.

### Clock and caller-owned input frame

The caller is base `137FE0` / TU `139750`. It allocates a `4048`-byte input
frame using the original stack-probe routine, executes original
`__control87(9001F, FFFFF)`, gathers the frame, then consumes it. The active-tick
condition is **frame byte `+4`**. The readable TU decompile presents that byte
as a normal `char param_1`, but original `13977D` reads `[esp+4]` after the
large frame allocation; the instruction-owned frame is the authority.
The frame producer is base `682E0` / TU `684F0`, and the original frame consumer
base `683A0` / TU `685B0` executes its zero-controls branch in this oracle.

The live producer clears the frame and calls its gathering routine. The inspected
gatherer [TU `694A0`](../analysis/map-pack-update/functions/000694a0.c)
stores the result of original helper `68460` to frame `+4`
on its ordinary gathering branch. The complete helper base `68250` / TU `68460`
returns true only when all these original conditions hold: runtime byte
`4CF770` / `4D6D38` is nonzero; options pointer is nonnull; options byte `+1120`
is nonzero; options signed short `+2` is not NONE; runtime byte
`4CF772` / `4D6D3A` is zero; and the runtime state pointed to by
`4CF77C` / `4D6D44` has dword `+18 == 4`. The 14 helper cases execute its
true path and each false gate. Recording/replay frame gathering, actual input
production and membership transitions remain service boundaries.

For an active frame the order is:

1. Complete early player stage base `14D3A0` / TU `14EAE0`.
2. Later world phases, beginning base `138DB0` / TU `13A520`; engine update
   base `15C150` / TU `15D7C0` and object update base `B6F10` / TU `B7E00`
   occur among these later calls.
3. Caller-local timing bookkeeping, then frame finish base `684E0` / TU `686F0`.
4. Increment game-time dword `+8`, base `1380F8` / TU `139868`.

Thus an early-stage spawn at game clock `S` occurs before the later world phases
of that same active tick; the caller returns with clock `S+1`. An inactive frame
runs gathering/consumption/finish but skips the early/world phases and the game
clock increment. The oracle supplies the frame producer's zero controls and
explicit true/false `+4` flag; every later world subsystem and frame finish is
an explicit no-op service. The early membership helper executes its original
runtime-flag-zero branch. This establishes caller ordering conditional on those
inputs, not a complete live network tick.

### Typed timer inputs and death initialization

| Owner | Offset | Type | Role |
| --- | --- | --- | --- |
| Game time | `+2` | signed 16-bit | Tick rate |
| Game time | `+8` | 32-bit clock | Active tick number; increment wraps as a dword |
| Engine globals | `+70` | 32-bit clock | Engine start used for signed elapsed tick |
| Player | `+164` | signed 32-bit | Live respawn countdown |
| Player | `+168` | signed 32-bit | Accumulated penalty, consumed by death writer |
| Player | `+170` | signed 32-bit | Displayed remaining seconds, clamped to 0..1023 |
| Player | `+1B4` | signed 32-bit | Death elapsed tick, clamped to nonnegative |
| Player | `+1AC` | signed 16-bit | Remaining lives; NONE skips decrement |
| Player | `+218` | signed 16-bit | Chooser-owned spawn count |
| Options | `+1BC` | signed 32-bit seconds | Respawn interval |
| Options | `+1C0` | signed 32-bit seconds | Same-full-player suicide penalty |
| Options | `+1E8` | signed 32-bit seconds | Penalty gated by original identity predicate |

The original death writer base `15CE70` / TU `15E4D0` first writes
`player+1B4 = max(signed32(game_time+8 - engine_globals+70), 0)`. An attacker
equal to the victim's **full datum**, including salt, adds options `+1C0`
converted to ticks to the victim's accumulated penalty. Attacker NONE skips
that addition. In the FFA branch with team flag clear, countdown mode is two:

```text
N = max(one_second_ticks, accumulated_penalty_ticks + interval_ticks)
player+164 = N
player+168 = 0
```

The writer converts its integer seconds/rate with `cvtsi2ss`, multiplies in
single precision, stores that float, then uses original x87 integer conversion.
The source sum and comparison operate on signed 32-bit values. The executed
integer-second fixtures cover intervals 0/1/5, suicide penalties 0/1/5 and
stored seven-tick penalties. The minimum is one second even when interval and
suicide penalty are both zero. The writer prefixes stop at base `15D163` /
TU `15E7C3`, immediately after penalty clearing, with current-unit ownership
still intact. The independent history snapshot countdown remains the separate
signed-short lifecycle established above.

For a non-NONE attacker, the writer calls engine vtable `+6C` with the two
signed identity values from player `+C0`. The actual Slayer routine base
`24DD70` / TU `253F10` returns **identity A != identity B**. The options `+1E8`
addition occurs only when that result is false, attacker/victim full datums
differ and **the victim's pre-write `+164` is zero**. The original caller tests
the victim's countdown, not the attacker's. Distinct valid FFA membership slots
therefore receive no `+1E8` addition for an opponent death. The report's
equal-identity fixtures exercise the false-predicate branch explicitly and are
marked invalid as normal FFA membership fixtures; they must not be used to
invent per-slot combat teams. In those branch fixtures, a stored seven ticks
becomes 67 after a two-second `+1E8` addition when victim countdown is zero,
then becomes a 247-tick death countdown after a five-second interval and
one-second suicide penalty. With different identities, or victim countdown one,
the stored seven remains seven and the later suicide countdown is 187.

### Early-stage countdown and successful spawn boundary

The early player stage walks the valid player bitmap in ascending absolute-slot
order and reconstructs each full player datum from the record's salt. It only
collects active flag-bit-0 players with inactive bit 1 clear and current unit
`+2C == NONE`. Body depletion by itself does not enter this queue. In ordinary
multiplayer mode, it invokes base `15DD40` / TU `15F3A0` once while collecting.
If eligible, it queues the full datum and invokes the same function again
immediately before calling the normal spawn function. Successful normal spawn
binds the replacement unit and clears the initial-spawn bit inside this early
stage; the clock has not advanced yet. Both eligibility calls are retained in
the successful tick's report row.

With an existing engine, the eligibility function first requires its original
player/lives gate base `158A50` / TU `15A090` and playing-phase gate
base `15B2F0` / TU `15C930`. The fixture uses an active valid FFA identity,
unlimited lives, engine short `+6C == 1` and engine dword `+C04 == 1`. These
complete helpers execute; other player/lives states are not established by
the timeline fixture. A blocked engine phase leaves a positive countdown
unchanged although an active caller still advances the game clock.

For a positive `+164`, eligibility emits any countdown notice **before** its
single decrement: subtype 20 at three/two/one seconds, subtype 21 at one tick.
It then decrements `+164` once and stores the post-decrement signed quotient
`+164 / tick_rate` to `+170`, clamped to 0..1023, before invoking the original
dirty-state helper. Its notification construction executes; delivery is a
supplied service, and network-player slots are NONE in these fixtures. Negative
nonzero countdowns are neither decremented nor eligible. Zero only passes when:

```text
elapsed = signed32(game_clock - engine_start_clock)
elapsed < 4                 # includes negative elapsed
or (elapsed & 31) == (full_player_datum & 31)
```

The original negative elapsed branch returns before the bit-mask comparison.
The complete function's engine-absent branch returns true without decrementing;
the timeline cases keep the original Slayer engine present. Initial players
with zero countdown and initial flag bit 3 can spawn during the first four
engine ticks without going through the death writer's minimum. Initial zero
cases cover elapsed -1/0/3/4; a successful spawn clears bit 3, while a blocked
one preserves it. The queued recheck's rare false branch can initialize an
initial player's countdown to the converted variant interval; it has no
one-second minimum. That branch is retained in source instructions and is not
reached by these stable single-player fixtures.

When multiple players are queued, the inspected original stage randomizes queue
removal with the global `+0` stream: one LCG advance while queue size exceeds
one, then index `(seed >> 16) % queue_size`. The remaining last entry consumes
no queue-order draw. The single-player timing fixtures do not execute the
multiple-entry branch. Each normal chooser retains its earlier independent
private/global-`+4` RNG contract; neither countdown nor slot-gate waiting draws
from that stream in the recorded fixtures.

This neutrality is limited to local-user NONE. On failed eligibility, the stage
calls preview base `14F0F0` / TU `150810`. The inspected
[TU preview routine](../analysis/map-pack-update/functions/00150810.c) requires
player local-user short `+28 != NONE`, throttles using Xbox `GetTickCount`, and
then calls the chooser with player NONE and preview true. Such a local preview
can consume the global `+4` stream. Its wall-clock service and preview-world
path are not executed here; a live local-player integration must preserve this
unresolved input instead of declaring all waiting ticks RNG-neutral.

For a supplied death after the early phase at clock `D=100`, the death writer
sees 100, the release occurs before the footer, and the caller returns at 101.
The first countdown decrement is in the next early phase at 101. With immediate
source ownership release, zero is reached at `100+N`. For these positive
elapsed fixtures, successful early-stage clock is:

```text
S = (100 + N) + ((absolute_slot - (100 + N)) & 31)
```

| Interval seconds | Suicide seconds | N ticks | Zero at clock | Spawn slot 0 | Spawn slot 3 | Spawn slot 15 |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | 0 | 30 | 130 | 160 | 131 | 143 |
| 1 | 0 | 30 | 130 | 160 | 131 | 143 |
| 5 | 0 | 150 | 250 | 256 | 259 | 271 |
| 5 | 5 | 300 | 400 | 416 | 419 | 431 |

The caller returns with clock one greater than each spawn clock in this table.
For the five-second, slot-zero case, notices occur at clocks 161/191/221
(subtype 20) and 250 (subtype 21); its zero countdown then waits until 256.
Retaining the depleted current unit across clocks 101 and the early phase of
102 freezes the countdown at 30. Releasing ownership after that early phase
makes the first decrement occur at 103. Inactive frames freeze both clock and
countdown; active non-playing frames freeze only the countdown. These sequences
are executed source cases, not an `alive=false` ownership shortcut.

This closes the bounded caller/early-phase scheduling contract previously left
open. A native end-of-world absolute deadline alone omits source ordering,
ownership admission, inactive/phase freezes and the 32-tick slot gate. A native
integration should retain the signed countdown and full datum, run this player
phase before later world events at the current tick, and advance its clock at
the source footer boundary. Actual frame gathering, membership updates, damage
generation, animation/head transforms, history tick scheduling inside engine
update, world collision/creation and network event delivery remain unexecuted
services in this timing oracle. The source paths for those stages must retain
their separately proved contracts; these cases do not establish full game
tick or collision parity.

## Follow-up: multiple-player queue ordering and conversion rounding

Run the separate bounded cases:

```sh
.tools/python/bin/python tools/oracle_mp_spawn_queue.py
.tools/python/bin/python tools/oracle_mp_spawn_rounding.py
```

The timing tool and report above are frozen while these separate tools add
evidence. Timing tool SHA-256 is
`b8ffecca7e05ef3c0996e8a8dbd204a731eb1325ec73016b9c67c844d4f7d653`;
timing-report SHA-256 is
`2e8cf5df6aa91c225f715190ae9b26115db79b8d195807ec9a6b689b5319102b`.

### Original queue execution with explicit no-starts world

The [queue oracle](../tools/oracle_mp_spawn_queue.py) executes 34 paired cases,
44 full active-caller ticks and 204 original normal-spawn attempts.
[queue-report.json](../analysis/multiplayer/integration/spawn-source/queue-report.json)
records the full salted player datums, every scan/recheck, complete source
attempt outcome, ordered original global-`+0` writes and before/after state.
It imports the frozen timing tool and records all dependency hashes. The complete
early-stage instructions are retained in that timing evidence.

The fixture deliberately overrides the real scenario's authored-start count
to zero. Each complete original chooser and normal spawn function therefore
fails before creation. Every pending current unit is NONE, death history is
empty, local user is NONE and controls are zero. No created unit position,
head transform, collision contact or loadout is invented. This is an attempt
ordering and retry contract with an empty-start-table fixture, not a successful
multiple-unit world spawn oracle.

The admitted queue follows ascending absolute-slot scan order. The original
queue loop is exactly:

```text
while size > 0:
    if size > 1:
        global_rng_word_0 = global_rng_word_0 * 0x19660D + 0x3C6EF35F  # u32
        chosen_index = (global_rng_word_0 >> 16) % size
    else:
        chosen_index = 0
    player = queue[chosen_index]   # full salted datum
    recheck eligibility; attempt ordinary spawn if still eligible
    queue[chosen_index] = queue[size - 1]
    size -= 1
```

Removing an entry replaces it with the last entry; it is not stable erase.
There are `size-1` queue draws when all admitted entries are processed. The
single remaining entry draws nothing. The ordinary spawn return value is not
used as a branch condition: the original stage invokes its concurrent
eligibility refresh after the spawn call, then removes the queue entry even
when the normal spawn returns false. Current unit NONE, initial bit 3 and
spawn count zero survive the source no-starts failure, so that player can be
collected again at the next eligible early phase. An eventual creation failure
has the different chooser-count ownership already proved above.

For global `+0` seed `12345678`, original attempts are:

| Admitted slots | Attempt slots | Queue draws | Final word `+0` |
| --- | --- | --- | --- |
| 0, 3 | 3, 0 | 1 | `75432777` |
| 0, 3, 15 | 3, 0, 15 | 2 | `CD305E6A` |
| 0 through 15 | 3, 13, 15, 0, 6, 1, 9, 11, 12, 2, 10, 8, 7, 5, 14, 4 | 15 | `B20611CD` |

The report also executes seeds zero/one, ordinary respawn flags, a single
pending player, negative countdowns, positive countdown admission, consecutive
failed initial attempts and startup slot-gate boundaries. For slots 0/3/15 with
initial countdowns 2/1/0 at clock zero, scan makes 0's countdown one and excludes
it; slots 3/15 queue and are attempted as 15/3. At the next clock, zeroing
0's remaining tick admits all three; their attempt order is 3/15/0. Three
failed initial players are retried at clocks 0/1/2/3, with no attempt at clock
4 because those slots do not satisfy its slot gate. Sixteen pending players
at elapsed four admit only slot 4; at elapsed 35 admit only slot 3. These
single-entry queues do not change word `+0`.

All no-starts cases leave global spawn RNG word `+4` unchanged, since the
chooser has no start to score. Normal real-start attempts retain their separate
chooser draws and source-count updates. The queue evidence does not replace
those chooser or collision contracts, and the local-user preview boundary
above still applies.

### Single-precision conversion and ties-to-even

The [rounding oracle](../tools/oracle_mp_spawn_rounding.py) executes 22 paired
calls to complete original seconds-to-ticks helper base `1469F0` /
TU `148160`. It first runs the original inactive caller to establish the actual
FP environment, then invokes the helper without the common oracle's CPU reset.
[rounding-report.json](../analysis/multiplayer/integration/spawn-source/rounding-report.json)
retains original instructions, the two product/result stores and FP controls.

Original caller FPCW is `023F`, with round-to-nearest, ties-to-even and 53-bit
x87 precision. Initial MXCSR is `1F80`, with round-to-nearest, ties-to-even SSE
rounding. Sticky inexact flags may change during conversion; the rounding-mode
bits remain the same. The helper converts signed-short tick rate to single
precision, multiplies its single-precision seconds argument with `mulss`, stores
that float, loads it with x87 `fld`, and uses `fistp` to a signed dword. Therefore
rounding is applied after the single-precision product has already rounded.

| Supplied float seconds | Stored float product at 30 Hz | Source integer ticks |
| --- | --- | --- |
| 0.05 | 1.5 | 2 |
| 0.15 | 4.5 | 4 |
| 0.25 | 7.5 | 8 |
| 0.75 | 22.5 | 22 |
| -0.05 | -1.5 | -2 |
| -0.15 | -4.5 | -4 |
| -0.25 | -7.5 | -8 |
| -0.75 | -22.5 | -22 |

These fractional values are explicit helper fixtures, not claims about authored
variant values. The integer options used by the death writer first undergo
`cvtsi2ss` as described above. The tested variant intervals 0/1/5 at 30 Hz
produce exact integer products 0/30/150; a native half-away-from-zero conversion
would nevertheless differ from the verified original helper contract.

## Follow-up: authored player placement and actual Lockout BSP construction

This is a separate, frozen experiment. It does not modify the earlier chooser,
history, ownership, timing, queue or rounding reports. Reproduce it with:

```sh
.tools/python/bin/python tools/oracle_mp_spawn_placement.py
```

The [placement oracle](../tools/oracle_mp_spawn_placement.py) is pinned by
SHA-256 `29fe396eb70ae293a2c4a3cfd0469522e294b7d1524def43da6dec0f7bdd826f`.
Its ignored [placement-report.json](../analysis/multiplayer/integration/spawn-source/placement-report.json)
is pinned by SHA-256
`11ae3316e030ce217f1aa73eaf59b74df483502aa4e6507697dba2bd51b02e34`.
It records every actual authored start, full creation record, root write,
ordered original routine entry, source-produced object and BSP location,
used service, dependency hash and separate live-body stop. The instruction
listings are under ignored `spawn-source/placement-instructions` and have
individual hashes in the report. All executable and Lockout cache pins remain
the pins at the beginning of this document.

### Completed setup and constructor boundaries

There are 66 authored Lockout player starts. For each row, the experiment
copies its actual 0x34 bytes into a one-row scenario view. The original chooser
selects that row; no chooser result or facing is fabricated. Paired base
`14F2E0` and TU `1509C0` normal multiplayer functions complete, including
original creation-record initialization, authored position/facing copy,
orientation, player binding and successful return. Representation supplies the
real `masterchief_mp` datum `E9F2086D` and variant NONE; shape contact supplies
clear; creation supplies a datum; customization supplies false and loadout is
no-op. These 132 cases prove setup and binding, not created physics or loadout.
Base and TU position/forward/up vectors agree bit for bit for every row.

The TU creation records then feed unchanged generic object creation `B8A30`
against all actual cache tags, actual masterchief model data, actual Lockout BSP
nodes, planes, leaves and clusters. The original object allocator and original
cluster registries execute using explicitly supplied empty backing storage;
heap leaves supply capacity without changing source initialization. The cluster
active mask is explicitly all-active. Animation runtime-node lookup returns
unavailable, and optional looping sound/effect allocation returns unavailable.
No head-marker pose is supplied. Only services actually called appear in each
case's `used_services`; the constructor does not call the inherited unavailable
Havok ray-test leaf in these valid cases.

All 66 generic constructors complete with physics global byte `486560=0`.
This is an explicitly suspended/deferred physics case. The physics subsystem's
live initialization sets that byte to one; zero must not be represented as the
ordinary live-world default. A second set of three cases uses byte one and the
original physics-component allocator/type dispatch, then stops before the
real shape virtual call at `1DA93E`. Those live prefixes resolve location,
register the object, allocate the component and produce its body position.
They do not build or register a working Havok body.

### Position, orientation and location order

The normal player function initializes the 0xC4 creation record with base
`B7930` / TU `B8820`. TU `11DDA0` copies the selected authored start's XYZ and
facing. The normal function executes x87 `fcos` and `fsin` on that authored
facing, producing forward `(cos(facing), sin(facing), 0)`, and copies up
`(0,0,1)`. The relevant record fields are:

| Creation offset | Original input |
| --- | --- |
| `+00` | real biped definition datum |
| `+1C..24` | exact authored root XYZ |
| `+28..30` | source cosine/sine forward vector |
| `+34..3C` | source up vector `(0,0,1)` |
| `+40/+44/+48` | zero linear velocity |
| `+4C/+50/+54` | zero angular velocity |
| `+58` | scale one |
| `+5C` | full salted player datum |
| `+64` | sign-extended player membership byte `+C0` |
| `+AC` | representation variant; fixture supplies NONE |

Original `B8A30` copies root XYZ into object `+64/+68/+6C`, forward into
`+70/+74/+78`, up into `+7C/+80/+84`, velocities into `+88..9C`, and scale into
`+A0`. Source unit creation callbacks `C52C0`, `DDB40`, `DFAF0` and `E6F60`
initialize the actual biped; they do not replace these XYZ values.

The report watches every store overlapping object root XYZ. Each constructor
first has three zeroing stores in the original allocator, then exactly three
stores copying the authored float bits at `B8B8C`, `B8B91`, `B8B97`. There are
no later root-position stores in the completed suspended-physics constructor
or the live prefix through `1DA93E`. No downward ground ray is called in these
slices, and no `+0.002` root adjustment occurs.

Creation runs `BDF10`/`BDF80` to build model-node transforms and `BEDE0` to
compute bounds from the actual object tag. Bounds are separate from root
position; tiny X/Y rounding differences can occur when transforming the local
tag offset, so replacing the bound computation with a constant Z offset is
not established. It then runs `C0400`, `11D4A0` and `14B9D0` to resolve spatial
location, followed by `B94F0`, `B97D0`, `1CE650` and `14BD00` for location and
cluster registration. Activation precedes `1C7050`/`1C6F50` physics creation.
All 66 real cases resolve a valid location across 12 cluster values, without
calling the location-repair sphere routine `1E1800`.

| Actual authored index | Exact root XYZ float bits | Source leaf / cluster / BSP |
| --- | --- | --- |
| 0 | `40E11301 BE09D990 C10476D3` | 15 / 1 / 0 |
| 32 | `C10FDFCB 3F834A5C C1223342` | 2111 / 9 / 0 |
| 65 | `BE273FEF 413EC528 C1423332` | 2260 / 15 / 0 |

`11D4A0` uses current active-BSP global short `46FB64` and the current BSP
descriptor, not the authored start's BSP short `+12`. It traverses actual
packed BSP nodes using the candidate spatial point. `C0400` can, when its
first location has cluster NONE, use a sphere query `1E1800` and leaf fallback
`11D460`; that inspected branch finds a spatial location and does not write
root XYZ. It has not been exercised by the 66 valid authored starts.

An additional paired, explicitly modified metadata fixture changes only start
`+12` to seven while preserving actual XYZ/facing. The original chooser and
normal player setup still succeed with the exact XYZ. This demonstrates that
the field is not a player-setup admission filter under this fixture. It does
not demonstrate a second BSP exists in Lockout or that the active BSP can be
arbitrarily changed. The constructor's current BSP state still owns location.

### Shape admission and live body limits

The earlier lifecycle evidence established the original helper's first authored
capsule/fallback-sphere selection and transform. This new experiment also runs
the original chooser, helper and Havok-query setup without supplying a contact
result, stopping before the query allocator pointer is dereferenced at
`1C8C34`. The resulting descriptor has filter nine, excluded component NONE,
the first real authored capsule shape, identity rotation and translated XYZ
with the helper's source single-precision `Z + 0.0701`. For actual start zero,
authored Z bits `C10476D3` become query Z bits `C10357B2`; X/Y remain
`40E11301/BE09D990`. This query offset belongs to admission, not player root
placement. The descriptor and callback bytes are retained in the report.

The full `1C8B40` world-query path allocates a Havok query worker, installs the
original callback, adds the worker to world `526234` through `314A00`, invokes
the worker/broadphase virtual interfaces, and removes it through `314B30`.
Callback `1C8D30` is separately proved only for supplied contact fixtures.
The actual static-body/contact candidates and filter-nine broadphase behavior
are unresolved. A downward BSP ray does not implement this source shape test.

The three live creation prefixes reach original `1DA6F0`. It selects the
standing shape from the real capsule block, generates identity shape rotation
and the same authored root position via `BACC0`, and reaches `1DA93E` with
physical component `00010000` and valid source location. The actual new biped
state is one in these cases and its adjustment count at `+3AA` is zero. The
inspected `E23A0` branch can subtract a count-derived amount from the **body
position** when that count is nonzero; it does not establish an unconditional
root offset. No nonzero-count body case is executed here.

At `1DA93E`, the real capsule's serialized vtable word is `00BF5950`, which
has not undergone the original tag runtime fixup. A guessed class/vtable would
invalidate an original-instruction oracle. Shape fixup, body builder `1DAAC0`,
Havok registration, the fully populated live world, successful contact admission
and later movement/settling remain named boundaries. The proved correction is
to preserve selected authored XYZ through normal setup and this valid constructor
boundary. This report does not prove full live spawn/collision parity.

### Failed attempts and normal death-stage distinction

Complete paired normal-spawn failure cases produce:

| Boundary | Source return byte | Signed spawn count from zero |
| --- | --- | --- |
| authored-start count zero | false | zero |
| contact supplied for every candidate | false | zero |
| representation NONE after a selected start | false | one |
| generic creation supplied NONE after a selected start | false | one |

Every failed case preserves current unit NONE, previous unit, initial flag bit
three, live countdown and death-history record. The chooser owns incrementing
the signed count once it has a selected start, even when later setup cannot
create a unit. No selected start causes no increment. The frozen queue report
above already executes the caller's false-return path: the attempted queue
entry is removed regardless of false return; surviving ownership/countdown
state is re-evaluated at the next eligible early phase. There is no proved
failure-specific timer reset or fixed delay.

Ordinary current-unit release belongs to the source damage aftermath, not a
generic observation that body health is depleted. In inspected TU `DAC40`,
the multiplayer death-event call to `1ED960` is at `DB0CA`; its history writer
`15E4D0` is called while the player still owns the current unit. Damage
aftermath `CB410` is then called at `DB10D`. `CB410` checks result flags bit
zero for death and calls `CBDC0` at `CBA2E` after its feedback, animation,
stun and AI/physics notifications. `CBDC0` begins biped death handling,
clears controlled-unit fields and schedules corpse garbage, stores player
`+30` from current `+2C`, then calls `14E240` with EBX=NONE. That original
binding call clears unit current-player `+13C`, writes its full previous
player datum at `+140`, and clears player current-unit `+2C`. Further weapons,
seats, actions and death-animation processing follows the release.

The earlier ownership/timing fixture executes that release prefix with an
already depleted body and explicit biped ragdoll state three, after the early
player phase. It supports a native fixture that releases ownership immediately
after lethal damage in that bounded schedule. It does not establish that
`Unit.alive=false` itself produces release, or prove the complete original
normal damage-event/marker/animation path. Successful respawn source binding
is separately executed above and clears previous-unit ownership as documented
in the ownership section.

### Additional executed aftermath producer cases

The separate [aftermath oracle](../tools/oracle_mp_spawn_aftermath.py) and ignored
[aftermath-report.json](../analysis/multiplayer/integration/spawn-source/aftermath-report.json)
extend the release evidence without changing the placement report:

```sh
.tools/python/bin/python tools/oracle_mp_spawn_aftermath.py
```

The tool SHA-256 is
`f1f0fd38dc721680442ad33d8f0492d1a6aaa5b5b3b1c6a3e5a4c017923cbbd9`;
the report SHA-256 is
`42f8649fc1ce92e40ebab4c3aa70cf33c6c0ef7633c144383f6f884ef98edbf0`.
Twelve cases execute actual `CB410` from entry with actual Lockout
`globals\\falling` damage-effect bytes and explicit post-damage result records.
Slots 0/3/15 use full salted player datums. Both body-dead-bit values are crossed
with both result-death-bit values. The input object has ragdoll state three,
local user NONE, no attacker or AI actor, zero supplied damage magnitudes and
the result's animation-suppression bit `0x20` set. No engine service is replaced
in this experiment. These are declared post-damage fixtures, not independently
produced combat results.

Every result with death bit zero returns from the complete aftermath routine
with ownership unchanged, including when the existing body-dead bit is set.
Every result with death bit one executes source calls in order
`CB410 -> CBDC0 -> E6050 -> BC840 -> 14E240` and stops after original ownership
release at `CBE95`. It preserves the existing history and live countdown. The
exact release stores are unit `+13C=NONE` at `14E2B3`, unit `+140=full player`
at `14E2B9`, and player `+2C=NONE` at `14E2F2`; player previous-unit `+30`
was stored at `CBE85`. This is direct evidence that the aftermath result's
death bit selects the ownership producer, rather than the unit dead bit.

Three additional prefixes clear the animation-suppression bit and use a
death result with the body already dead. They run ordinary `CB410` only to
damage-animation entry `115FA0`; ownership is still unchanged at that boundary.
No animation result is supplied and no normal-death completion is claimed.
The full damage consumer and its history writer precede this aftermath in the
inspected caller; these fixtures do not execute a history capture or fabricate
a head pose. They preserve the distinction between the supported immediate
native release fixture and complete original ordinary death processing.
