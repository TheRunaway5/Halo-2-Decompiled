# Original multiplayer datum allocation and ownership

This is a separate frozen TU1.1 source gate for player and shared object pool
identity. It executes the original allocator, player initialization, ownership
binding, player removal and object player-removed dispatch. It does not establish
complete world startup allocation order, full object construction/deletion or
live multiplayer parity. Production and earlier research suites are untouched.

## Authority and reproduction

- Original TU1.1 executable:
  `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256
  `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
- Original Lockout cache: `assets/disc/maps/lockout.map`, SHA-256
  `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
- Oracle: [oracle_mp_spawn_datums.py](../tools/oracle_mp_spawn_datums.py).
- Ignored report:
  `analysis/multiplayer/integration/spawn-source/datums-report.json`.
- Raw original instruction and type-descriptor listing:
  `analysis/multiplayer/integration/spawn-source/tu11-datums-instructions.txt`.

Reproduce from the repository root:

```sh
.tools/python/bin/python tools/oracle_mp_spawn_datums.py \
  --output analysis/multiplayer/integration/spawn-source/datums-report-repro.json
```

The suite has **25 cases**: 16 pool/initializer cases, three body-storage/release
cases and six ownership sequences for player indices 0, 3 and 15 with retained or
deleted corpse storage. The source executable is mapped unchanged. No executable
instructions are patched and no head or animation poses are supplied. Prior
producer, damage and Slayer reports are asserted against their frozen hashes.

## Original pool creation and representation

The executed player bootstrap `14CC00..14CC49` requests `0x2210` bytes through a
supplied arena allocator, passes literal `players` at `45AAF4`, capacity **16**,
stride **0x21C**, alignment exponent zero into original `16D070`, sets pool flag
bit 2 and publishes the pool at `4F01F4`. The later player-globals arena allocation
is outside this prefix. The complete original new-map player reset `14CCD0`
executes, including `16D220` and the real AI player-globals reset `1CB9D0`.

The executed object bootstrap `B76C2..B76E0` passes literal `object` at `45A3A4`,
capacity **0x800 (2048)** and stride **12** into original `16CFF0 -> 16D070`, sets
pool flag bit 2 and publishes the pool at `4E78D0`. Its allocator requests `0x614C`
bytes, including the header, headers and bitmap. The earlier other-object setup
calls and later dynamic body heap initialization are outside this prefix. This
is the shared object-header pool used for units, weapons, projectiles and other
object types; it is not one independent pool per type. Raw callers and descriptors
are included in the instruction listing.

| Pool field | Type | Source meaning |
| --- | --- | --- |
| `+00` | 32 bytes | NUL-terminated pool name; constructor terminates byte `+1F` |
| `+20` | signed 32-bit | maximum element count |
| `+24` | signed 32-bit | element stride |
| `+28` | byte | alignment exponent |
| `+29` | byte | valid flag |
| `+2A` | 16-bit flags | bit 2 allocated pool; bit 3 poison-fill mode |
| `+2C` | 32-bit | signature `64407440` |
| `+30` | pointer | underlying allocator |
| `+34` | signed 32-bit | automatic allocation scan cursor |
| `+38` | signed 32-bit | high-water index, exclusive |
| `+3C` | signed 32-bit | allocated element count |
| `+40` | 16-bit | next salt counter shared by the pool |
| `+44` | pointer | element storage |
| `+48` | pointer | occupied bitmap |

`16D070` puts aligned elements after its `0x4C`-byte header and the bitmap after
`capacity * stride`. The first two bytes of every element are its salt; zero
marks a free element. For objects these two bytes belong to the **12-byte object
header**, not the object body. Header `+2` is the flags byte, `+3` the type byte,
`+6` the allocated body length and `+8` the body pointer. Body `+0` is its tag
datum. See original `16D070`, `16D0D0`, `BD170`, and the raw pool/header snapshots.

A runtime datum is the full 32-bit pattern `(salt << 16) | index`. `NONE` is
exactly `FFFFFFFF`. Ordinary source salts have bit 15 set: valid player datum
`EC700000` and object datum `E26F0000` therefore have negative signed 32-bit
representations. Testing “datum > 0” or treating all negative values as absent
would reject source-valid objects. The pool's absolute index is the low 16 bits;
the full datum remains the ownership identity.

## Salt initialization, allocation and release

Original `16D220` zeroes scan cursor, high water and count. It copies the first
two name bytes into `+40`, little endian, and ORs `8000` into that word:

- `players`: bytes `70 6C` -> first salt **EC70**.
- `object`: bytes `6F 62` -> first salt **E26F**.

It clears every element's salt and the occupied bitmap. Normal mode does not
clear the remainder of free elements; bit 3 fills storage with `BA` first. A
new-map reset reseeds the counter; ordinary release does not. A datum surviving
across a whole pool reset can consequently alias a newly allocated datum. This
suite does not add an invented generation beyond the source's 16-bit salt.

Original automatic allocator `16D2C0` receives the pool in EDX. It scans clear
bitmap bits from `+34` up to `+38`, otherwise selects `+38` if below capacity.
Successful allocation sets the bitmap, increments count, moves cursor to
`index + 1` and raises high water to at least `index + 1`. It zeroes the complete
stride, stores the current pool salt into the element, then increments the
16-bit counter. If the incremented counter is `FFFF`, it replaces it with
`8000`. Executed wrap input `FFFD` allocates salts `FFFD`, `FFFE`, `8000`, ending
with counter `8001`. Normal initialized operation does not allocate zero or
`FFFF` salts. No random stream participates in this allocator.

Original requested-index allocator `16D410` receives the pool in EDX and the
signed requested index in EDI. It rejects a negative/out-of-capacity index or an
element whose salt is nonzero, returning `FFFFFFFF` without changing pool
state. On success it sets the bitmap, increments count, raises high water and
calls `16D480` for zeroing/salt assignment. **It does not move the automatic
allocation cursor.** The executed index-5 case leaves cursor zero and the next
automatic allocation takes index zero. Player creation `14D350` uses this
requested-index path with the membership index; player index is not chosen by
its own first-free scan.

Original release primitive `16D4C0` receives EDX pool/EAX datum. It masks the
low 16 bits, clears that element's salt and bitmap, decreases count, lowers the
cursor to the released index if necessary, and shrinks high water across trailing
zero-salt elements. With bit 3 set it fills the entire stride with `BA` before
clearing the salt. It does not alter the next-salt counter. Executed hole reuse
returns the released index with the next pool salt, and trailing release reduces
high water as described.

**This release primitive does not validate the incoming salt, bounds or prior
occupancy.** Its internal caller must already own a valid element. The report
executes valid releases, not double-free or invalid-pointer cases; source count
is a signed integer decremented without an added clamp. Do not mistake this
internal primitive for a checked public handle API. Other internal unit/player
field accesses also often mask the index directly.

Original checked lookup `16D560` receives ESI pool/EAX full datum. It returns
NULL for `NONE`, index outside the current high water, zero element salt, or a
salt mismatch. It compares the signed stored 16-bit salt with the full datum's
arithmetic upper half, which correctly accepts salts with bit 15 set. It does
not use the bitmap as an additional identity check. Reused indices with different
salts fail old-datum lookup. This is the check used by the separately frozen
player damage-history reader `1ED960`.

The suite also executes a deliberately bounded capacity-three fixture to prove
full-pool rejection without mutation. It does not change the recovered ordinary
player capacity of sixteen.

## Object body allocation and failure

Complete original `BD170` first allocates a header through `16D2C0`, then calls
body-storage service `18FCE0` with the address of header `+8`, requested length
and zero flag. Only that lower service is supplied by this oracle; mapped body
addresses and success/failure are explicit. On success original instructions
store header `+6` length and zero every requested body byte. Requested length
`0x800` is a **body-storage fixture**, not a claim that generic creation requests
that length for every real unit or projectile.

If body storage fails, `BD170` releases the newly allocated header with
`16D4C0` and returns `FFFFFFFF`. The occupied count/cursor/high water return to
zero in the empty-pool case, but its salt is consumed: `E26F` becomes `E270`.
Native failure cleanup must not rewind the salt counter.

Complete `BD1F0` is separately executed with an explicit zero-link body-heap
fixture. It clears header flags, returns the supplied allocation size to heap
bookkeeping, updates the zero-link heap head/tail, clears header `+8`, then calls
`16D4C0`. The fixture reclaims `0x810` bytes for its `0x800` body plus supplied
16-byte heap link header. This is proof of the primitive's bookkeeping and
header release; it does not execute every world/type/Havok deletion callback.

## Executed player and unit ownership sequence

Every sequence begins with the actual source pool bootstrap and player-map
reset above. Complete `14D350 -> 14D800` creates an ordinary nonlocal player at
requested membership index 0, 3 or 15. Source-local properties come from the
frozen controller reset/publish producer and are included as all 144 bytes in
this report. Machine registration, network transport and live sign-in are
outside this fixture: no local machine, user or controller is attached. The
initial flag is set by source initialization, current/previous unit are `NONE`,
and spawn count is zero. Complete source initialization and the actual Slayer
join callback execute; persistent profile mode and replicated entity are disabled
by their original predicates.

A unit is allocated through complete `BD170`, receives the real Lockout
multiplayer Master Chief tag and executes `C52C0..C5464`, which initializes unit
ownership and damage-history fields. Object active/type registration and the
body length are supplied. Complete `14E240` binds the actual full player and
unit datums, including original `CD3B0`, actual Slayer shield initialization and
status update `153A90`. This binding is not a successful generic spawn callback
and does not claim to increment spawn count or clear the initial flag.

| Stage | Player `+2C` current | Player `+30` previous | Unit ownership |
| --- | --- | --- | --- |
| source bind | original full unit | `NONE` | unit `+13C` full player; `+140 NONE` |
| body-depleted prefix `D7E00..D7E42` | original full unit | `NONE` | dead bit set; still owned |
| death-cleanup prefix `CBDC0..CBE95` | `NONE` | original full unit | unit `+13C NONE`; `+140` full player |
| bind another unit through `14E240` | new full unit | `NONE` | new unit owns same full player |
| complete player leave `14DCB0` | `NONE` before player record release | retained fixture has `NONE` | active unit detached before pool free |
| source rejoin and bind | newer full unit | `NONE` | new player salt, same requested index |

Death cleanup uses explicit `(unit, NONE, 0, 0, 0)` arguments and a biped
ragdoll-state-3 fixture. Body depletion stops before its animation notification;
death cleanup stops after ownership release. The delayed death caller scheduling
and animation/physics aftermath beyond those endpoints are outside this suite.
A dead bit alone must not be treated as a released current-unit datum.

The optional corpse-deletion branch executes complete player object-deleted
callback `154440` with its final control notification supplied as a no-op, then
complete `BD1F0`. `154440` clears matching player previous/corpse reference before
the pool index becomes reusable. Full generic deletion ordering outside this
player callback and storage primitive remains a boundary. The retained-corpse
branch does not release its header merely because it is dead.

## Projectile ownership across actual leave and rejoin

The shot owner triple is read from the bound unit fixture's `+13C` full player,
its full unit datum and signed `+138` designator. Original `B8820` builds the
creation descriptor. `B8D0D..B8D2A` copies descriptor `+70` to projectile `+C2`,
`+68` to `+C4`, and `+6C` to `+C8`. The final `+C8` store is at **B8D24**; the
exclusive endpoint needed to include it is **B8D2A**. Complete firing remains
the independently frozen producer suite. The designator is a separate field;
this pool oracle does not encode the player index as that field.

Complete original player leave `14DCB0` detaches the current unit with `14E240`,
cleans local controller/user state where applicable, releases the player record,
then calls `BC7E0`. The latter enumerates actual allocated objects through
`BBDA0`; `10D0F0` walks their original type-descriptor inheritance list and invokes
non-NULL player-removed callbacks at descriptor `+5C`.

The actual biped inheritance list includes unit callback **D0300**. It compares
unit body `+2A8` to the departed full player and clears it to `NONE` on equality.
The actual type-5 projectile inheritance list has **no player-removed callback**
in either descriptor. The executed leave consequently preserves projectile
`+C4` player and `+C8` unit, even though it frees that player record. It does not
replace them with `NONE`, the new unit, or a new occupant of that player index.
The report records each real callback dispatch and its full arguments; original
inheritance arrays and their `+5C` entries are included in the listing.

For the empty-pool index-0 fixture:

| Allocation/lifecycle | Retain corpse | Delete corpse before replacement |
| --- | --- | --- |
| original player | `EC700000` | `EC700000` |
| original unit | `E26F0000` | `E26F0000` |
| captured projectile | `E2700001` | `E2700001` |
| replacement unit, same player | `E2710002` | `E2710000` |
| player rejoin, same index | `EC710000` | `EC710000` |
| unit bound after rejoin | `E2720003` | `E2720002` |
| projectile player/unit after rejoin | `EC700000` / `E26F0000` | `EC700000` / `E26F0000` |
| checked lookup of departed player | NULL | NULL |
| checked lookup of original unit | original allocated corpse header | NULL; index zero now has salt `E271` |

Indices 3 and 15 execute the same generation changes with their requested player
low halves preserved. Each sequence uses one shared object counter: allocating
the projectile consumes a generation between the original and replacement unit.
All stages retain full datums, raw pool/header/record bytes and both random
streams; the pool allocator itself has no random selection.

## Native integration contract and remaining scope

This gate supports a player pool with source-generated salts and source requested
indices, and one shared object-header identity pool whose allocator is used by
supported units, projectiles and items. Keep the current-unit datum, corpse datum,
projectile-captured player datum, projectile-captured unit datum and separate
membership designator explicitly. Respawn changes the unit datum while preserving
the player's datum; leaving releases the player record; rejoining can reuse its
index with a new pool salt. A checked full-datum lookup decides whether an old
captured identity still resolves.

The frozen history producer and consumer already preserve full result attacker
player and source object fields; this gate supplies the original generation and
ownership semantics for those fields. Map those APIs to runtime full datums,
including the high-bit salts, without rewriting them to slot-plus-one or replacing
captured fields with the current occupant. A stale field can remain stored while
checked lookup returns NULL; clearing the stored field is not the source's leave
behavior proved here.

Complete authored map startup object population, every object/body allocator
size, lower arena allocation, static object creation order, world/Havok registration,
full deletion orchestration, network membership transport and departure timing
remain unresolved here. A native shared pool can reproduce source allocator and
reuse behavior for its supported allocation sequence without claiming that its
first live unit has the same full datum as the full original world. The example
first-unit values above are explicitly **empty-pool scripted fixtures**. No fixed
per-slot salt, invented salt rewind, dead-body release or departure owner clearing
is established by this report.

## Freeze hashes

The exact tool/report/listing hashes are recorded below after independent
reproduction. Earlier suites remain byte-identical and are imported read-only.

| Artifact | SHA-256 |
| --- | --- |
| Oracle | `cc9aa611cd8e482a9e29879b731835c76544e697d08acedf8c44a11a515fea11` |
| Frozen report | `9d93d609507bd352f13d6bd9c202071f26b05ee2ea4bb942a87ce3fcdf945629` |
| Instruction listing | `1bcd717925b5b8642370db4ee6c0dab9dd31af87d94ff9039ee8a5a240077d88` |

Independent `datums-report-repro.json` is byte-identical to the report. Python
compilation and whitespace checks passed. The latest producer, damage and Slayer
oracle hashes and their frozen report hashes retain the previously published
values; no earlier source suite was rewritten.
