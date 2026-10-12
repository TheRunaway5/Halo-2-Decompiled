# Power-module entity, property bank, current bounds and manager bridge

The native port now reproduces **50 complete original owner calls**, including
real small-block allocation/bank growth, entity initialization, current
quantized bounds and manager delivery into the already validated contact
refresh. It matches **59,059,316 retained bytes, all1,006 ordered stores and one
literal EAX/ECX/four-stack-argument delivery**. Normal, ASan/UBSan and the
production-library CTest pass. This is useful body-state ownership, **not a
completed native collider**: full rigid-body motion/shape construction, world
insertion and solver admission remain required before the scene can interact
with a power module.

The current `SceneObjects::Object` owns transform/lifetime fields; it has no
Havok entity/body/component. `SourcePropScene` owns presentation and requires
an external current-state lighting query. Its constructor lighting sphere is
not a physical collider. The bridge below identifies the actual state to
retain and the next concrete factory owner. No Scene, Simulation, runtime,
renderer, release or shader was changed. Only new source/header/test/tools and
their CMake registration were added. [Current scene](../include/halo2/scene.hpp),
[prop presentation contract](../include/halo2/source_prop_scene.hpp).

## Authority and unchanged composition

TU1.1 original executable SHA256 is
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`;
Lockout is
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
The authenticated owned TU1.5 English executable is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Annotations are not authority. Every listed instruction is checked against
the actual image. [New original observer](../tools/oracle_mp_power_body_bridge_1_1.py),
[complete original report](../analysis/multiplayer/sandbox-source/power-body-bridge/final/measured/report.json).

The extension imports the immutable real
[power-module schedule](MP_POWER_MODULE_CONTACT_SCHEDULE_RESEARCH.md) and
[contact lifecycle](MP_PHYSICS_CONTACT_LIFECYCLE_RESEARCH.md). Source preparation,
selected `phmo` fixup, full authored crate factory, real player join/spawn,
PVS and the five ordered object/world phases are unchanged. It executes
`13A380→B7E00→1C7660→B7F50→B8040`, then the same separately declared
`BEDE0` current-bounds diagnostic. The full prior factory outputs, phase calls,
stores, callbacks, states and broadphase outputs correlate byte for byte.

No body identity, root, local node, velocity, contact, solver result, clock,
queue, active mask, damage, allocation result or pose is supplied by this
extension. Existing native compatibility/raw backing, selected prerequisite
callbacks, resource services and the declared original clock fixture retain
their earlier boundaries. This is not complete title startup or a natural
repeated-frame clock. The independent new report has **404 calls,328 full
states,4,900 reads and3,834 stores**, with **23 complete original listings /
1,317 instruction rows**. All reports/listings repeat byte identically.

## What the actual factory establishes

Authored Lockout crate row1 retains BLOC `E6F80583`, model `E6F90584`, collision
definition `E7080593` and physical definition `E7090594`. Original fixup changes
the serialized rigid-body shape field `+38=4` into the real resident shape
pointer. Passing4 still has the separately preserved original failure; it is
not an address or an AABB. The fixture's resulting object datum `E26F0000`,
physical datum `E1680000`, body pointer `0601A480` and shape pointer `8025CC40`
are **observed identities, not values to copy into a live allocator**.
[Original construction](../analysis/multiplayer/sandbox-source/owner-creation/final/power-fixed-measured/report.json).

Complete `1DAAC0` produces its actual **B8-byte body-info packet**, allocates
an **A0-byte entity/rigid body**, stores allocation size at`+4`, and calls
`318100`. Initial crate construction chooses motion type7. Source PVS
activation reconstructs it as type4. The actual authored Spartan comparison
also follows7→5. No native mode is inferred from grounded or visibility alone.
All four actual B8 inputs and A0 before/after packets are retained under
`calls[entry=00318100]`, including untouched/padding bytes. The current shape's
actual virtual table is`45D188`: source type query`+14→9D8E0` and shape-AABB
query`+18→303E30` are real required owners, not a guessed box primitive.
[Full1DAAC0](../analysis/multiplayer/sandbox-source/power-body-bridge/final/measured/001daac0.asm),
[full318100](../analysis/multiplayer/sandbox-source/power-body-bridge/final/measured/00318100.asm).

The new native module ports the complete **312410 entity constructor**, not
the enclosing318100 rigid-body constructor. It must not be presented as a
ready rigid body:318100 still owns the final rigid-body VT, motion pointer,
current transform, velocities, auxiliary solver state and shape-derived
motion radius. [Native module](../src/physics_body_state.cpp),
[required storage/API](../include/halo2/physics_body_state.hpp).

| Entity field | Exact constructor ownership |
|---|---|
|`+0/+6/+8/+C`|base VT, reference count1, zero word, actual info`+4`shape|
|`+10/+14/+18/+1C/+20`|`FFFFFFFF`, zeros; current transform is subsequently owned by318100|
|`+24/+28/+2C`|signed info`+8`byte, info`+0`filter word, self pointer|
|`+30/+34/+38`|property data/count/capacity; initial0/0/`80000000`|
|`+40/+44/+48`|zero byte/word, actual info`+18`byte|
|`+4C/+50`|source literal`3F000000`/`3ECCCCCD`;318100 later uses authored info`+AC/+B0`|
|`+54/+58/+5C`|zero auxiliary pointer, `FFFFFFFF`, actual info`+1A`word|
|`+60/+64/+68`|self+`6C`, zero, `80000001`|
|`+70..9C`|four exact empty three-word bank descriptors; preserve all untouched bytes|

`318AF0` increments the **actual shape+6 reference count** before importing
properties. `312410` then writes its complete entity fields in original order.
It reads the actual descriptor bytes; it does not manufacture a source shape
or clear the whole A0 allocation. [Base constructor](../analysis/multiplayer/sandbox-source/power-body-bridge/final/measured/00318af0.asm),
[entity constructor](../analysis/multiplayer/sandbox-source/power-body-bridge/final/measured/00312410.asm).

`317890→31C1A0→31C110` is ported as the exact body-info initializer, including
SIMD zeros and instruction-owned scalar literals. All observed initializer
stores are exported. That stack packet's untouched padding was **not captured
whole at317890 entry/return**; the export does not reconstruct it. Later full
318100 input packets are separately genuine source observations.
[Initializer](../analysis/multiplayer/sandbox-source/power-body-bridge/final/measured/00317890.asm).

## Retained property and allocator ownership

`318A90` searches actual eight-byte key/value pairs. An existing key returns
without overwriting its value. A full bank calls`2DF780`, whose capacity is
count*2, or1 for an empty bank. Source allocation precedes forward copy and
free; descriptor data/capacity stores happen afterward. Positive old-bank
copy/free is measured, not replaced by a cold-only rejection.

Actual crate keys are allocated in source order:
**`2003→0`, `2001→physical component E1680000`, `2002→body slot0`**. Both cold
creation and activation reconstruction are exported, as are naturally
encountered existing-key no-write calls. The component datum, body pointer,
object datum and slot are distinct namespaces. [Property append](../analysis/multiplayer/sandbox-source/power-body-bridge/final/measured/00318a90.asm),
[bank growth/copy](../analysis/multiplayer/sandbox-source/power-body-bridge/final/measured/002df780.asm).

The required allocator is actual`487638` adapter, original VT method
`+10→148A10`/`+14→148A70`, active backing`480DA0` and measured mode2. The module
uses the actual class table at allocator`+AC+bytes`, stride table`+68`,
freelist`+24`, live count`+2E8`, page/cursor state`+18/+1C/+20`, page count
`+F0340`, allocated count`+F0344` and free count`+F0388`.

The new `22E950` small-class port covers existing heads **and actual empty-list
batch creation**. It writes real nodes inside existing owned backing. Original
page offsets/limits (`4340`, stride`2000`, payload`1FC0`, maximum`76`) come from
the instructions, not tuning. Page exhaustion returns the original zero;
the adapter then requires its real alternate allocator owner. `22EB20` free
preserves count/head/node stores. No allocation callback returns a fixture
address. The live adapter must first initialize and own genuine storage;
reading absent memory fails. This fixture executes existing-head allocation,
six empty-list batches and twelve frees across nested/direct comparisons.
New-page and page-exhaustion branches are ported from the complete unchanged
instructions but remain **listed-only, unexecuted coverage** here. No fake
exhaustion/cursor case is added. Medium/large/alternate allocator branches
remain explicit boundaries. The old contact lifecycle module is unchanged.
[Complete allocator](../analysis/multiplayer/sandbox-source/power-body-bridge/final/measured/0022e950.asm),
[free](../analysis/multiplayer/sandbox-source/power-body-bridge/final/measured/0022eb20.asm).

## Actual current bounds and manager delivery

`2E7610` reads the **broadphase index at the actual body-handle pointer** and
selects a16-byte row from backend`+40`, then tail-jumps to`2E65A0`. The packed
indices are not an object datum or authored bounds. Axis table pointers are
backend`+4C/+58/+64`, with four-byte entries whose consumed values are low16.
X min/max indices are packed`+8/+A`, Y`+0/+4`, Z`+2/+6`. Axis scales are
backend`+30/+34/+38`; the final source SUBPS subtracts backend`+10`vector.

X/Y keep their x87 reciprocals on the stack across multiplication. **Z spills
its reciprocal to binary32 first**. The measured bounds call has actual
CW`027F` and MXCSR`1F80`; the native port accepts those fields explicitly and
uses x87/SSE, without recomputed geometry or assumed53-bit precision. Masked
floating-point status flags and exception delivery are outside the API.
The actual emitted32 bytes are
`e015e940c01b07c1109006c100000000e083f240f05902c140aff1c000000000`.
They are current backend bounds, not the shape's local AABB or the lighting
sphere. The diagnostic's separate-root scope is preserved.
[Wrapper](../analysis/multiplayer/sandbox-source/power-body-bridge/final/measured/002e7610.asm),
[decoder](../analysis/multiplayer/sandbox-source/power-body-bridge/final/measured/002e65a0.asm).

Complete`1D4160` consumes ESI's actual14-byte callback context and five stack
words. It walks manager rows in original30-byte order and contact rows in
48-byte order; matches contact index, counterpart component and signed slot
bytes; checks context flag record`+18==1` and nonnull`+20`; then derives the
flip low byte by pointer identity. It delivers EAX=actual manager,
ECX=actual row and four source words to the existing1D2F10 port. The actual
refresh word`64E39500` retains unrelated upper24 stack bits; only low8 is a
semantic flip flag. The input API preserves those bits for literal comparison,
without interpreting them as a32-bit mode. This original delivery matches the
independently frozen lifecycle packet and all ten nested stores. The second
actual component has an empty manager and makes no delivery. It creates no
contact, admits no solver and substitutes no body identity.
[Complete delivery](../analysis/multiplayer/sandbox-source/power-body-bridge/final/measured/001d4160.asm).

## Minimum live bridge and next complete owner

The scene integration belongs to the simulation-owned object generation and
physics phase, not a rendering callback. Proposed retained bank must bind each
live scene generation to its **actual source object datum, physical component,
body slot, rigid-body pointer, motion pointer, resident shape and broadphase
handle**. Values in these fixtures must never be copied as live identities.
Scene presentation can then consume actual source-produced object/node root;
collision delivery must use the same actual backend bodies and current
quantized tables. Retain unsorted source registry order and the existing
ordinary/world/dirty/new-object phase order.

The next factory segment is **1DAAC0→318100**, using actual cache/runtime shape
and descriptor producers, followed by its actual insertion/activation owner.
The measured initial motion type7 calls`320CC0` on aD0 allocation. Activated
crate type4 calls`317EA0→321320` on110 bytes; this owns motion base math and the
actual quaternion/current-transform fields. `318100` also owns velocity
virtuals`+50/+54`, authored damping/coefficient inputs, `317BF0` solver state
and `317D10`'s real shape query/radius arithmetic. These must be ported or
provided by a real source-faithful backend, rather than a captured-output
callback. Old-motion release/reconstruction must also precede replacement.

Only after that retained body/world ownership exists should root add a scene
factory consumer and simulation collision hook. The small native owner here
is not installed as a permanent reject or fallback collider. Full native
world/broadphase/solver delivery, admitted moving-player-versus-crate contact,
continuous clock/settling and incoming damage/destruction are still open. The
source-created crate/BSP contact is genuine; it cannot be borrowed as a player
contact. The actual damage-info pending words remain zero and source D73E0
returns AL0. No hit, health delta, explosion radius or effect is invented.

## Later-image correspondence and validation

Static unseeded original comparison finds **all23 complete bodies** with equal
normalized instructions/internal branch topology in authenticated TU1.5.
The short wrapper is additionally resolved by its exact address-free six-
instruction prefix and real tail target. The report retains paired memory
operands, direct targets, relocated immediates and jump-table topology. Direct
targets outside the measured set are retained as unverified callees. This is
not TU1.5 gameplay execution or proof of the full transitive solver.
[Correspondence tool](../tools/research_mp_power_body_bridge_correspondence.py),
[complete report/listings](../analysis/multiplayer/sandbox-source/power-body-bridge/final/later-complete/complete-report.json).

| TU1.1 owner | TU1.5 owner |
|---|---|
|`1DAAC0 / 318100`|`1DF380 / 31BC40`|
|`317890 / 312410 / 318AF0`|`31B3E0 / 315F00 / 31C640`|
|`318A20 / 318A90`|`31C570 / 31C5E0`|
|`2DF710 / 2DF780 / 2DF360`|`2E31E0 / 2E3250 / 2E2E30`|
|`22E950 / 22EB20`|`233250 / 233420`|
|`2E7610 / 2E65A0`|`2EB0C0 / 2EA050`|
|`1D4160`|`1D8A20`|

The present native API is explicitly `_1_1`. TU1.5 allocator globals, VT
addresses and executable constant locations differ; they require an explicit
provider/image adaptation. For example, bounds one is`465030→46A0D0`, adapter
pointer`487638→48C938`, and active allocator`480DA0→48606C`. The TU1.5 delivery
calls`1D77D0`; this extension does not claim a native TU1.5 refresh/lifecycle
port. The old validated lifecycle and constructor freezes remain unchanged.

[Original exporter](../tools/export_mp_power_body_bridge.py) uses only source
before/after/read/store data. The50 fixtures preserve full backing plus actual
read closure. **654 internal call-stack/FS exception-chain stores are excluded
from portable outputs**, while actual stack-resident descriptor output stores
remain. No expected native geometry or result is used. [Fixture schema and
audit](../analysis/multiplayer/sandbox-source/power-body-bridge/final/exported-owner/report.json),
[source/export auditor](../tools/audit_mp_power_body_bridge.py).

Reproduction:

```sh
.tools/python/bin/python tools/oracle_mp_power_body_bridge_1_1.py --output /new/body/report.json
.tools/python/bin/python tools/export_mp_power_body_bridge.py --output /new/body/export
.tools/python/bin/python tools/audit_mp_power_body_bridge.py
cmake --build build-physics-body-state-20261008 --target physics_body_state_tests -j 2
ctest --test-dir build-physics-body-state-20261008 -R '^original_physics_body_state$' --output-on-failure
```

The final manifest pins source/repeat, export/repeat, both image listings,
auditor/docs/tools and immutable copies of the validated native files. It pins
only the new CMake registration snippet, not concurrently owned shared CMake.
The report SHA256 is
`6414ec9078212075c2fb0ea29d582688f00fd0404eb2ee5f06826679010ab05c`;
fixture is
`2c8312169bd862993665313f9e0c82ecc1dcc4be2625299ba9edfb01957bcedc`.
[Final manifest](../analysis/multiplayer/sandbox-source/power-body-bridge/final/manifest.json).
