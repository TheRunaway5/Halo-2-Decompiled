# Original item node, bounds, location and registry producers

This separate TU1.1 execution closes original heap-backed initialization of the
authored Lockout BR rows 0/1 and SMG row 12. Their initial current nodes, bounds,
location and unsorted registry links now come from original allocation and pose
instructions. It does **not** close their active frame, physical settling,
current moving contact, animated player pose or live pickup admission.

The source-created empty player pool produces a zero visibility/activation
mask. All three weapons retain inactive header `4A`; original ordinary, dirty
and new-object passes return without executing an item tick. This is the
measured limit, not a reason to supply a nearby player, active bit or contact.

## Frozen evidence

New files only:

- `tools/oracle_mp_item_current_producers_1_1.py`, SHA-256
  `cc04b51cd4ce882c25323ce534c264373356930725cf015a93b2908e8058bb81`.
- `tools/oracle_mp_item_current_producers_audit_1_1.py`, SHA-256
  `a20200230373f9e2e077af989c6dae4bbcfee8934221c72db1e5c8e5eeebbe8e`.
- Ignored `analysis/multiplayer/integration/inventory-source/item-current-producers/`:
  `report.json`, independent `reexecution/report.json`, `audit.json`,
  `manifest.json`, `schema.json`, `reproducibility.json`, and instruction files.
- Original execution report SHA-256
  `8e0351b19fbae952a6dd895f77489fed31cb9f04723ce2b5814fa0f656181a76`.

The report has two cases, 18 selected root stages, 153 measured calls, 57
interned source states, 22,866 observed stores and 111 stored object states.
The audit checks 248 instruction files, 36,780 opcode rows and 516 observed
store sites against original XBE bytes. It preserves 149 prior pins, including
the entire measured transfer freeze. Original source is TU1.1
`default.xbe.03`, SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`;
Lockout is SHA-256
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
No production or earlier frozen evidence is edited.

Reproduce with `.tools/python/bin/python` and the new oracle, passing
`--output` into a fresh directory, then run the new auditor with
`--output-dir` against the canonical evidence. `manifest.json` records final
pins and independent report/listing equality. Earlier exploratory drafts in
the directory are excluded from the freeze.

## Original storage preparation and its actual lifecycle owner

The imported, immutable `FixedTrace(data)` keeps the real Wine32 compatibility
worker alive and executes the original CRT scratch setup and complete empty
`1C5FF0` world construction. See `MP_FIXED_MEMORY_WORLD_RESEARCH.md`. Its game
arena starts at `82D47000`, cursor `9318`, after original empty 2048 × 12 header
pool and BSP table initialization. Earlier raw OS backing, exact compiled
compatibility context/locks, selected CRT registrations and excluded startup
calls remain explicit boundaries.

The first new case executes untouched `B76E0..B77A6` (exclusive). It produces
the original 1 MB heap, 84-byte world record, A00-byte scratch and both cluster
registries, advancing the cursor to `13735C`. Calling full `B78C0` then faults
at `D5E98`, writing the valid flag of the absent widget pool `4E78F0+29`. That
failure identifies an owning prerequisite instead of supplying its result.

The second case executes full original `B76B0`, including `D5E30` widget
initialization, `10CA70` type-chain initialization and `C0E50` light storage.
Full `B78C0` map reset and `B7AA0` source-empty activation then return. No
supplied service is used in these three stages. Their resulting arena cursor
is `174048`, heap `82D8D004`, world record `82E8D054`, scratch `82E8D0D8`.
These addresses describe this fixture; they are not engine constants.

The original storage chain is:

1. `B76EA → 125520 → 18FC80`: source arena request 1 MB plus the original
   descriptor/alignment overhead, pool signature/name/backing/capacity and free
   state. `B76F5` owns global `4E58BC`.
2. `B76FF..B7750`: source 84-byte world record, original 21-dword clear and
   `4E58C4` assignment. `B7749` also clears the source static object arrays.
3. `B7756..B778A`: source A00-byte scratch and `4E58A0` assignment.
4. `B7790` and `B77A1 → 1CE5B0 → 29C520 → 16CFF0`: two original triplets at
   `4E58B0` (collidable) and `4E58A4` (noncollidable), each with 800-byte heads,
   an original 2048 × 32 forward pool and 2048 × 12 reverse pool. Their datum
   salts, alignment and backing are original constructor outputs.
5. Full `B78C0` initializes widget/type/light state, clears the header and four
   reference pools through `16D220`, fills scratch/cluster heads with `NONE`
   and resets source world counters/flags. Full `B7AA0` owns world-active byte
   `*4E58C4=1`; its object iterator is source-empty in this case.

Source evidence: the new `instructions/000b76b0.asm`, `00125520.asm`,
`0018fc80.asm`, `001ce5b0.asm`, `0029c520.asm`, `000b78c0.asm`,
`000b7aa0.asm` and measured before/after states.

This execution deliberately prepares the preceding frozen empty world before
the new object initializer. It is a bounded producer experiment. It is **not**
the full authored startup sequence. The actual lifecycle owners are recovered
independently from original `139390`, `139410`, `1394B0` and callback table
`447428`, stride 24. They visit records in ascending order, with init at +0,
map reset at +8 and activation at +10 within each record:

| Record | Source init | Source map reset | Source activation |
|---|---|---|---|
| 19, `4476D4` | `14CC00` players | `14CCD0` | `187550` |
| 25, `4477AC` | `1EEF40` item constants | `1EEFE0` | zero |
| 27, `4477F4` | `B76B0` objects | `B78C0` | `B7AA0` |
| 58, `447C50` | `1CB180` AI | `1CB3D0` | `1CB5A0` |
| 59, `447C74` | `1C5F00` physics | `1C5FD0` | `1C5FF0` |

The report exports literal table words/bytes, not a proposed reordered startup.
Other records and complete game/AI/player lifecycle remain separate work owned
by the spawn investigation.

## Activation mask and item constants

The first complete `13A380` attempt executes both original `13A2A0 → 151EC0`
queries but faults at original `1CD56D` reading absent AI global `4FCDA8+1`.
The fixture then executes original `1CB180..1CB1F4` exclusive: actual 374-byte
AI storage, 38-byte auxiliary storage and complete `1CB990` reset. Later AI
initializers are excluded. Full `14CC00` creates an actual empty player pool.
The second full `13A380` returns with both 64-byte masks zero. The earlier
inherited FF mask is overwritten by original owning stores, and is not used
as the weapon admission input.

At `13A3EB`, `1CD540` supplies an actor cluster mask into the caller's local
buffer. At `13A414`, `13FB50` **ORs two masks into the output**. It does not
unconditionally enable every cluster: exact ABI is ECX cluster count, EAX
first mask, EDX second mask, stack output. In the measured source-empty case
both masks are zero, so the final mask remains zero. The literal instructions
`13FB70..13FB75` read/OR/store each word. Connected player/view/PVS state is
still required for a normal FFA result; this case makes no such claim.

Full `1EEF40` also returns and owns 1C bytes at arena `82EBD734`, cursor
`176734 → 176750`, assigning `526254`. It writes original gravity/scale/air
constants and zero vector from original static bytes. This supersedes the
older supplied constant pointer. Source evidence: `001eef40.asm`, mask-stage
calls/stores and full stored before/after state. The existing physics world's
empty/static state does not prove item motion or contact.

## Authored allocation: measured initial node and registry state

Full `15C320` and `15C370` reset the original netgame row table. Selected
`15C110` calls use actual unchanged scenario equipment rows, offset `120/124`,
stride 90, in order 0, 1, 12. The original entry emits its descriptor from the
row's collection/variant and pose, then original `B8A30 → BD170 → 16D2C0 →
18FCE0` allocates the header/body. `BD270 → 18FD70` grows the actual body in
the original 1 MB heap. No allocator result, heap pointer, growth result,
existing datum, body size, node offset, transform, location or registry entry
is supplied.

| Authored row | Definition | Datum | Body bytes | Location8 | Loaded/reserve |
|---|---|---|---:|---|---|
| 0 BR | `E192001D` | `E26F0000` | 830 | `e106000009000000` | 36 / 72 |
| 1 BR | `E192001D` | `E2700001` | 830 | `4805000009000000` | 36 / 72 |
| 12 SMG | `E57B0406` | `E2710002` | 1260 | `c30800000f000000` | 60 / 120 |

These datum values depend on the actual source-empty pool, constructor salt
and selected allocation order. Integration must retain pool-produced datums;
the values in this table are not assigned identities for a populated match.
All owners/parents and physical-component datums are `NONE`, heat is zero,
and shared RNG remains `12345678`. `1C6F50` executes unchanged and type 2 does
not enter its `1883` type-mask physical-component constructor branch. The
NONE component is therefore observed source behavior in this item case,
not a supplied constructor failure.

Both BR bodies have a zero-length block at `110` with offset FFFF. The SMG
has 128 bytes at offset 832. All three have an actual current node bank of
208 bytes at offset 612 (`114` length / `116` relative offset): four source
52-byte matrices. `118/11A` owns the separate observed 10-byte block at 820;
this note does not infer another role from its shape.

BR `weap → hlmt → mode` resolves to original render model `E194001F`, address
`8015FEC4`, four actual mode node records, and no animation graph. SMG resolves
to `E57D0408`, address `802167D0`, four mode nodes and graph `E5860411`.
The only new resource delivery is its literal 20-byte descriptor `80218900`,
shared file offset `089EEC00`, 200 bytes, SHA-256
`7114e2d37aa691e20225ceb9b25e6265294552a39612bb461cc052231d35d534`.
The bytes are appended inside the existing mapped resident arena; no arena
enlargement or fabricated animation data occurs. Original `124F30` resource
delivery remains an explicitly supplied cache-residency service. World unit
poses and current contact planes are not supplied.

The shortest measured producer chain is:

1. First `BDF10`, return `B861C`, invokes `BDF80 → BEDE0` during original
   `B8570` scale initialization. `BDF80` changes the actual current node bank;
   its return leaves bounds/location unchanged. `BEDE0` then changes bounds
   while retaining the node bank. No contact or location has been created.
2. Second `BDF10`, return `B915A`, refreshes the same nodes/bounds during the
   final allocator initialization. These banks/bounds are already equal in
   the measured initial state.
3. `C0400 → 11D4A0`, return `C0435`, queries the source-selected registration
   sphere center (`object+40`) into the original descriptor's location8.
   This is not a locator query at an arbitrarily chosen authored root.
4. `B94F0`, return `B919D`, copies that location8 to `object+28` and cluster to
   header+4. `1CE650`, return `B9605`, allocates the actual forward/reverse
   links and retains original insertion order. `B94F0` sets object flag 100
   and header registered flag 40; then `B7C50` reads the actual zero mask and
   returns AL 0. No active-bit fallback is introduced.
5. Allocator postinitialization retains header update flag 2 and new flag 8.
   `15C110` stores the same datum in the row entry and writes object byte AF
   to the authored row index. Initial full salted identity is preserved.

The original cluster 9 chain becomes **row 1 → row 0**:
`EC640001(E2700001) → EC630000(E26F0000) → NONE`; cluster 15 contains
`EC650002(E2710002) → NONE`. These are original unsorted linked lists, not
distance or index sorting. The report preserves raw 32-byte links, salt,
next, object, all heads and pool descriptors at each owning stage.

## Field contract for a minimum initial-current-state port

The source state is exported literally; native results are not used as
expected data. Required represented fields and owners are:

| Source field | Ownership / requirement |
|---|---|
| Header stride C: +0 salt, +2 flags, +3 type, +4 cluster, +6 body bytes, +8 pointer | `16D2C0`, `BD170`, `BD270`; resolve salted identity through actual occupied bitmap/pool |
| Object +0 definition, +14 parent | Original allocator; never replace carried/world identity with a definition |
| Object +64 root, +70 forward, +7C up, +88/+94 velocity, +A0 scale | Descriptor/constructor and later original motion; initial fixture uses actual authored records |
| Object +110/+112 optional local bank, +114/+116 current bank | `BD270`, `BDF80`; relative signed-short offsets and literal 52-byte current matrices |
| Object +30/+3C bounds, +50/+5C render bounds, +40/+4C registry sphere | `BEDE0`; transformed definition center/radius, render bound selection and definition bit1000 must retain original arithmetic/selection |
| Object +28 location8 and header+4 cluster | `C0400/11D4A0`, then `B94F0`; query the original selected center |
| Object flag100, header40, collidable bit9 and triplet selection | `B94F0/B96A0`, original `1CE650/1CE7E0` forward/reverse membership; retain source unsorted order |
| Header active1/update2/new8/dirty4/already-updated20/delete10 | Original activation and object scheduling; registry membership does not mean tick admission |
| Object +12C item flags, +130 contact record | Constructor initializes them; later original `F9B10/F9EA0` owns motion/contact. No initial root or node matrix supplies a contact plane |
| Object AF row index; row datum/timer/counter | Original `15C110` and prior frozen timer/transfer contract |
| Object +22A/+22C ammo, +184 heat, +154 owner | Original weapon constructor/transfer; retained datum/slot stores remain separately frozen |

Minimum admitted scope is heap-backed **initial** authored item allocation,
source initial nodes/bounds/location and unsorted registration in the stated
empty-player world. The existing FBFA0/C57A0 consumers can consume genuine
states, but this fixture supplies no source-produced player capsule/marker or
live near-target admission. It also does not replace the later movement state
with the initial cached node bank.

## Original frame schedule and the blocking live boundary

Original `139750` orders the relevant phases:

`14EAE0` player/held input → other title callbacks → `15D7C0` equipment
allocation → `13A380` activation masks → `B7E00` ordinary objects → `1C7660`
world physics → `B7F50` dirty objects → `B8040` new objects/cleanup.

Thus a new weapon allocated after the player phase first becomes eligible at
the same frame's new-object pass, and a subsequent held-input selection sees
the preceding frame's current object state. The new suite invokes the three
object passes in their original relative order after the selected allocation;
it does not invoke the entire title frame or world step.

The original guards are material:

- `B7E00` and `B7F50` skip header-new8 items. General occupied visits preserve
  original ascending bitmap order, after the original special-datum list.
- `B8040` clears already-updated20 for occupied objects. It requires original
  new8, active1, update2 and !delete10 before clearing new8 and calling
  `BD360`, followed by `BD710` if dirty4 is set.
- In this measured state every weapon has header `4A`: registered40, new8,
  update2, **active1 clear**. All three passes return with exact object and
  registry states unchanged; new8 remains set. There are zero `BD360`,
  `BD710`, `F9B10`, `F9EA0`, `16B240` or `246A50` calls.

For an admitted moving item, the remaining source chain is statically owned
by `BD360 → 10CDF0 → F9B10/F9EA0`, which can update root/contact and mark
dirty, then `BD710` (physical readback if present) → `BDF80` → type dirty
callbacks → `BEDE0` → changed registration through `B96A0/B94F0`. Physical
world stepping/readback is the intervening original `1C7660` stage. See exact
new listings `000bd360.asm`, `000bd710.asm`, `000f9b10.asm`, `000f9ea0.asm`,
`000bede0.asm`, and `001c7660.asm`; these active branches are **not executed
by this freeze**. Existing earlier conditional contact traces retain their
explicit unavailable-Havok boundary and are not promoted to live proof.

The next live requirement is source-owned populated player/view/PVS state and
its activation scheduling, followed by admitted item motion/terrain/object/
Havok queries and resulting dirty-node/registry refresh. Spawn investigation
owns the complete dynamic biped/world construction. Full player animated
current nodes, pose resource loading, Havok motion/contact/readback, original
Xbox reciprocal behavior and full authored callback startup are still explicit
limits. No source success, gravity/friction replacement, active bit, contact,
nearby root or current pose is chosen to bypass them.

Two inherited Python observer hazards were removed in this new tool only:
the old `WorldTrace` observers at `18FCE0/18FD70` write supplied heap addresses
even after service removal, and the old `InventoryTrace` B8A30 observer shares
`allocations` with `FixedTrace`'s Havok measurement records. The new suite
bypasses only those Python observer effects and conflicting queue
interpretation. All original guest instructions remain unchanged and run;
the independent byte audit and repeat are mandatory. The only actually used
explicit engine leaf during the new stages is unavailable looping sound
allocation `C10E0`, which neither supplies node/physics state nor pickup
success.
