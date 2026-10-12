# Original equipped death cleanup after CBF21

This independent TU1.1 gate executes complete `D7E00` and `CBDC0` with the
original allocated SMG inventory, actual Spartan graph and source-created
unit extension, extra and control blocks. It closes the earlier missing
component boundary at `CBF21` for the tested ordinary equipment states. It
does not claim complete world spawn, biped Havok, impact attribution or Xbox
pose-bit parity. Production and earlier frozen evidence are untouched.

## Authority and reproduction

- TU1.1 executable:
  `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256
  `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
- Lockout cache: `assets/disc/maps/lockout.map`, SHA-256
  `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
- New oracle: [oracle_mp_spawn_death_tail.py](../tools/oracle_mp_spawn_death_tail.py).
- Ignored evidence: `analysis/multiplayer/integration/spawn-source/death-tail/`
  contains `report.json` and a separate `instructions/` directory.

```sh
.tools/python/bin/python tools/oracle_mp_spawn_death_tail.py \
  --output analysis/multiplayer/integration/spawn-source/death-tail-repro/report.json
```

Seven cases execute the complete cleanup: initial idle SMG, held firing,
early reload, late reload, source-picked-up dual SMGs, holstered SMG/current BR,
and actual original pool exhaustion. The report pins the preceding owner-counter,
corpse and attachment reports and every imported source dependency. It records
component construction, before/depleted/after state, complete call order,
selected stores, action requests/tables, original marker outputs, collision
queries, resource delivery and explicit service calls. New listed instruction
bytes and action table pointers/records are compared directly with the original
executable.

## Source-produced components and bounded caller inputs

The new tool inherits the immutable
[attachment fixture](MP_WEAPON_ATTACHMENT_RESEARCH.md). Original `1CE8C0`
constructs the manager; `1CEAC0` binds actual Spartan graph `E9180793`; complete
`C52C0/BD270` allocate and initialize extension `+33E=3A00`, extra `+342=3B24`
and controls `+346=3B7C`. Original `14E240` assigns the preallocated player to
unit `00010001`. Full `1A28A0 -> B8A30 -> 101BB0 -> CE510 -> E9D50` creates
actual starting SMG **`00010002`**, definition `E57B0406`, and selects actual
combat idle clip 185. It is never replaced with the dormant index-0 weapon.
`FB560/B9C20/1DC8F0/1DC940` execute actual cache marker/parent attachment.

The reserved player/unit arenas, manager offset `3800`, initial object/header
extent, 64-header pool backing and active-cluster mask remain inherited explicit
inputs. Full biped datum allocation, source player salt producer and complete
unit physics/world registration are outside this fixture. Those pool/ownership
rules retain their separate frozen contracts. This suite must not be promoted
to a source-complete startup allocation order. Initial biped state `+3DC=0`,
physical component `+B4=NONE` and inactive physical flags are inherited body-arena
inputs. The complete generic unit constructor creates the animation components;
the full biped/Havok constructor and contact producer are not executed.

Idle/firing/reload cases use authored player start 0's root and facing. Pickup
cases retain the earlier explicit near-target root, then execute original
233-ms primary/secondary pickup admission. Original `BDF80/BDF10`, cache
animation sampling, bounds and marker routines produce all current matrices;
no absent component offset, fabricated head/hand/drop pose or chosen animation
clip is supplied. Exact descriptor-selected Lockout/shared resource bytes are
delivered at the named residency boundary. See
[MP_PICKUP_EQUIPPED_RESEARCH.md](MP_PICKUP_EQUIPPED_RESEARCH.md) and
[SMG_CONSTRUCTOR_EQUIPPED_1_1_RESEARCH.md](SMG_CONSTRUCTOR_EQUIPPED_1_1_RESEARCH.md).

Firing and reload preparation use the frozen bounded source root order:
original player input conversion, manager blend `1D0ED0`, unit wrapper `C7E90`,
actual allocated weapon `1021D0`, action dispatch `E7E60`, animation `112E10`,
pose `BDF80`, projectile postpass `1019F0`. Projectile creation is supplied
failed and the muzzle segment supplied clear **during preparation only**;
both services are removed before death cleanup so real grenade allocation and
terrain queries execute. Supplied input flags and phase/tick scheduling remain
explicit; this is not a complete engine tick or successful world projectile.

The death roots run at tick 200. `D7E00(unit, NONE, 1, 0)` executes fully,
sets dead bit `+10A bit2` and returns while still bound. `CBDC0` then receives
`(unit, NONE, nullVector, knocked0, force0)`. Class `NONE`, knocked zero and
force zero match the preceding frozen actual SMG fatal cleanup. The optional
vector pointer is explicitly null here; class `NONE` makes original `E6050`
skip its optional vector-copy branch. The full prior `D9180`/attribution/death
writer is not replayed by this new fixture. Its real synchronous caller order
remains the separate [corpse contract](MP_CORPSE_LIFECYCLE_RESEARCH.md).

Original `B7972..B7995` garbage initialization stores execute before the death
roots, with the original prefix's garbage-pointer/zero register values supplied
to this slice. They establish head `NONE` and count zero. Earlier `B78C0` world
callbacks and pool-reset prefix are excluded; no complete new-map initialization
is claimed.

## Complete cleanup order

The complete original [CBDC0](../analysis/map-pack-update/functions/000cbdc0.c)
and its new listing establish the following order. The earlier prefix still
performs garbage admission, player previous-unit save, original detach/status,
death-tick stores and `D22B0` before `CBF21`.

1. At `CBF21`, resolve the **source-allocated extra block** through signed unit
   `+342`. If its signed mode `+C` is strictly between 0 and 15, invoke `116640`:
   it stops non-`NONE` extra handle `+1C` through `213890`, then clears mode.
   Every tested source-created ordinary extra mode is zero, so that conditional
   branch is inspected/listed but unexecuted. No nonzero mode is fabricated.
2. Visit selected primary then selected secondary, according to signed bytes
   `+212/+213`. Each non-`-1` slot resolves its full `+218` datum and calls
   `1045E0(weapon, 0, 0)`. This clears low-short weapon input `+16E` and source
   input byte `+170`; it is not an inferred complete magazine reset.
3. Clear unit `+134 bit21` at `CBFAC`. An attached unit with seat `+1FC=-1`
   calls `CD9E0`; a seated attached unit builds action type 30 through `E7F30`.
   The tested source roots are unparented, so those vehicle/attachment branches
   are listed, not executed.
4. `D0140` visits slots 0..3, skips current primary/secondary slot indices,
   and calls `CFD80(slot, mode0, chooseOther0)` for each other slot. Nonempty
   unselected items therefore leave inventory and drop **before grenades**.
5. If extra weapon reference `+238 != NONE` and options mode byte is not 4,
   call `CF8C0` and clear that reference. The original constructor leaves this
   reference `NONE` in all tested cases; no synthetic extra item is introduced.
6. `CFF90` processes the two source grenade types, consumes positive signed-byte
   counts, and attempts actual item creation/drop in type order.
7. If source `+1F5` is zero, or force byte is nonzero, call original action-table
   request callbacks for **type 19 first, type 9 second**, with zeroed 32-byte
   requests. Both point directly to `EA440`, which drops secondary for type 19
   and primary for type 9. This is immediate direct callback execution, not an
   assumed later action queue. Source `+1F5=0` and force0 are observed here.
8. Resolve the source control block through signed `+346`; call the type-26
   interruption callback if nonnull, then clear bit26; do the same for type27,
   then type22. Actual type26's callback is null; type27 calls `EB830`, which
   clears control reference `+2C`; type22 calls `E9A50 -> E8F30`, then clears
   control byte `+C`. These callbacks execute even when the corresponding
   action bit was already clear.
9. `115A00` clears both secondary animation extension channels and returns.
   Complete `CBDC0` reaches original `CC254 ret 0x14` in every case.

The direct selected-drop `EA440` path itself interrupts action types
**8, 18, 0, 10, 27, 22**, in that order, via `E7FF0`, before `E9A90`, slot
removal/drop and `E9D50`. Both selected weapons repeat that sequence in the
dual case. `E7FF0` receives full unit in ECX and action index in EDI; its action
index is not a stack argument. The outer 26/27/22 interruption sequence occurs
after those selected-drop callbacks. Full original pointer records at
`46EC8C`, `46ECB4`, `46ECC0`, `46ECD0`, `46ECD4` and `46ECE0` are recorded
and byte-checked in the new report.

## Weapon stop, reload and dual ammunition effects

Selected-drop `E9A90` executes `105750`, `1042E0(weapon,1)`, zero input,
`108230`, original physical holster and selected-index clear. `105750` owns
a reload-dependent branch: for each magazine in mode1, if signed remaining
transfer ticks times two are strictly less than total transfer ticks, complete
the source reload transfer before resetting it. `108230/109C30` reset trigger
and magazine operation state. The new early/late reload cases preserve this
distinction instead of treating every death as an unconditional ammo cancellation.
Immediate snapshots and individual weapon stores are included in the report.

The first late-reload transfer actually occurs earlier, during selected-drop
`EA440`'s type10 interruption: original `E7FF0 -> E89F0 -> 105750 -> 106D40`.
The later `E9A90` call finds that magazine already reset. Original player detach
`14E240` in the preceding prefix invokes `FB360`; its store at `FB3B2` changes
SMG item flags `+12C` from `0D` to `05`. `FB360` derives bit3 from a carried
item's owner unit `+13C != NONE`, so detached player ownership clears bit3
before either stop/transfer path. This is an executed producer, not a supplied
ammo flag. [Original FB360](../analysis/map-pack-update/functions/000fb360.c)
and the new listing/store sequence establish this dependency.

In the ordinary idle case, the dropped SMG preserves 60 loaded / 120 reserve;
the held case preserves 55 / 120. Early reload starts cleanup at 37 / 120,
remaining49/total50; it resets magazine mode1 to0 and both transfer ticks to
`-1`, preserving ammo. Late reload starts at 37 / 120, remaining24/total50;
strict `24*2 < 50` completes loaded ammo to60 while reserve remains120, then
resets mode/ticks. Original `106D40` only invokes reserve subtraction `104D30`
when item `+12C bit3` is set. Here the preceding source detach cleared it;
`104D30` is listed but not called. Do not insert an inferred23-round reserve
deduction. [Original 106D40](../analysis/map-pack-update/functions/00106d40.c)
owns the guarded subtraction and loaded/timer stores.
Actual `1042E0/108010` and its weapon graph change weapon state to10 and barrel
mode to2. These are source weapon state values, not native animation guesses.

The dual same-definition case also executes original `1058F0` **before dropping
secondary**. Primary reserve fills from120 to its authored cap180, taking60
from secondary reserve. Both retain60 loaded; secondary reserve becomes60.
The subsequent primary drop keeps180 reserve. The source routine first transfers
other reserve, then other loaded ammo if remaining room exists, and compares
signed shorts against authored magazine `+C`; the tested full-magazine state
only exercises reserve transfer. Do not drop two copies with unchanged120/120
reserves. Heat-swap and loaded-ammo transfer branches are listed, unexecuted here.

## Grenade allocation and physical drop

The real biped starting fields `+1B4/+1B6` and actual `matg+100` grenade block
are included as exact cache records. The full unit constructor/loadout produces
counts **4 frag / 0 plasma** in this bounded configuration. This value is not
installed by a direct count write or generalized to every lobby/variant.

`CFF90` processes positive **signed-byte** count for each actual grenade record.
Outside mode byte4, it calls `B8820` with the authored grenade item definition,
then original `B8A30`. Each success goes through `FB120(unit)`, `FB3F0`,
`FB490`, `FB120(NONE)`, and either deletion admission or `CFB00` according to
the source model/flags. Actual frag item `E63004BB` takes physical drop here.
The count decrements whether allocation succeeds or fails. Four actual source
allocations produce datums `00030004..00060007`; complete allocation/type
construction, identity, bounds and marker operations run. Capacity/backing
storage and resident delivery remain explicit services.

The exhausted-pool case fills the remaining original 64-header fixture with
complete `16D2C0` calls and verifies its next return is `NONE`. These body-less
reservations are explicit pressure inputs, not alleged world-created objects.
All four original `B8A30` grenade requests return `NONE`; both grenade counts
still finish zero. Existing SMG still drops, with no source grenade objects
created. No allocator-failure return is supplied.

`CFB00` uses full parent unit in ECX, item/full datum in its first stack word,
float multiplier1.0 in the next, and marker override in EAX. It selects the
actual drop/hand marker, attempts original temporary marker attachment,
detaches, evaluates source velocity/spin and performs the terrain query. The
selected weapon was physically holstered by `E9A90` first; its temporary drop
attachment is therefore distinct from the prior direct drop fixture's
already-parented rejection. Exact marker records and output matrices are
captured. Positions/velocity must not be replaced with a guessed head offset.

Each exercised successful physical drop consumes **four shared LCG writes**;
failed grenade allocations consume no drop RNG. From shared seed `12345678`,
the five idle drops (four frags, SMG) produce `54B5447C`; six dual/holstered
drops produce `F10D0170`; pool exhaustion's sole SMG drop produces `4B5C952C`.
Held/reload preparation already changed the stream, and their exact before/after
seeds are retained separately. No extra cosmetic RNG is inserted.

## Immediate retention and remaining boundaries

Every case returns with dead unit header/body allocated, source biped state3,
owner `NONE`, previous owner retained, player current `NONE`, previous unit
retained, no inventory/current/requested weapon references, grenade counts zero,
outer action bits26/27/22 clear and both secondary channels reset. Original
`E6050 -> E6DC0 -> 1E8E60` transitions the inherited initial biped state0 to3;
the preceding corpse suite's supplied state3 shortcut is unnecessary here.
The original transition runs with the fixture's inactive physical component;
this does not prove the live biped/Havok initialization or ragdoll pose.
The retained primary animation identifier is unchanged by this cleanup.

Original marker/pose, terrain plane/surface, inventory, allocation and action
instructions execute. The following boundaries still apply:

- `18FCE0/18FD70` supply heap backing/capacity; `124F30` supplies exact cache
  residency. They do not establish complete storage/world startup.
- `246A50` returns supplied false for the BSP Havok shape branch in every
  successful physical drop. Actual terrain calls execute, but complete Havok,
  active unit/object broadphase, vehicle/seat behavior, physical corpse motion
  and later dropped-item settling are unproved.
- Audio `170060`, effects/sound allocation and `16A356` first-person notification
  keep their named inherited boundaries. Complete FPS death presentation is
  outside this fixture even though actual weapon/unit graph components run.
- No nonzero extra dialogue/animation mode, extra `+238` weapon, vehicle parent,
  held grenade or nonzero force state is fabricated. Those branches remain
  inspected/unexecuted. The original type22 callback's no-held-reference path
  executes; a held live grenade's interruption requires its own producer gate.
- Source reciprocal/reciprocal-square-root instructions execute unpatched in
  Unicorn. Their established Xbox reciprocal-bit boundary remains; reproducible
  emulator matrices are not asserted to be exact Xbox matrices.
- This suite performs no garbage collection or world deletion. It composes
  with the prior synchronous attribution/history/ownership/retention contracts;
  it does not replace them or demonstrate an integrated original game tick.

The implementable ordinary equipment contract is the precise stop/drop/interrupt
order, stable full weapon/item identities, source ammo effects, source grenade
failure consumption and retained component state. Physical transforms/RNG and
registration must retain the named world-service and startup boundaries.

## Frozen evidence

- Oracle SHA-256:
  `e728636553ffb34db875c6a253326467b8db2a66a621035e1aaffdb180a2c553`.
- Report SHA-256:
  `7a467658d889a562b9d353f8378a7aba0f3bedff67f6f72b26075793248a4384`.
- Canonical instruction-listing manifest SHA-256:
  `cc7ad5c02f238068874a8238a3ff5667d11d9516bb7523cf11e5bd7c91a21f72`.
  This hashes `json.dumps(report['instruction_listings'], sort_keys=True,
  separators=(',', ':')).encode()`; individual file hashes are in the report.

All seven cases complete the unchanged original return. The 59 listings contain
5,697 instruction rows, including 3,956 exercised listed addresses. Every listed
instruction and each captured action pointer/record matches the original XBE.
The original TU/Lockout/shared-cache pins, 18 imported dependency hashes and three
preceding frozen report pins pass. Python syntax and whitespace checks pass.
An independent rerun reproduces the complete report and all 59 listing files
byte-for-byte. Production and all preceding frozen suites remain untouched.
