# Power-module retained body slots and world lifecycle

This separate freeze executes the unchanged source-created Lockout power
module's complete **1C7050 initial physical construction** and
**1C70D0 PVS remove/dispose/rebuild/reinsert** paths. It adds a native port of
the complete admitted **1D3A70 → 1DC0C0 / 1D29A0** retained-slot ownership
segment. It does not install a substitute collider or assert live solver,
player blocking, incoming damage or explosion. Earlier factory/contact/world
freezes and playable releases are unchanged.

## Authority and reproduction

The [new observer](../tools/oracle_mp_power_body_slots_1_1.py) imports the
immutable [body factory](MP_POWER_BODY_FACTORY_RESEARCH.md) read-only and
verifies all **181 prior manifest files**. The same real original preparation,
selected authored crate factory, source player/cluster/PVS and six selected
ordinary/world/dirty/new/bounds roots execute. Every earlier factory, body
bridge and full power schedule record must remain identical before this new
report can be written. No guest field, contact, pose, allocator success,
admission or result is added.

The original source image is TU1.1 SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
The [measured report](../analysis/multiplayer/sandbox-source/power-body-slots/final/measured/report.json)
and [independent repeat](../analysis/multiplayer/sandbox-source/power-body-slots/final/repeat/report.json)
are byte-identical, SHA-256
`d3564d22b7b5046c400ab038c3e14f26df0ebd6d61049876d0a0eecc14553e2c`:
**94 calls, 62 full states, 4,966 reads and 4,163 ordered stores**.
All **39 complete instruction listings / 5,225 rows** repeat identically.
Each of the **4,224 observed read/store instruction sites** is also checked
against the original image and the immutable whole setup/factory/world
executed listing. [Audit](../tools/audit_mp_power_body_slots.py),
[audit result](../analysis/multiplayer/sandbox-source/power-body-slots/final/audit.json).

Reproduce into a fresh ignored directory:

```sh
.tools/python/bin/python tools/oracle_mp_power_body_slots_1_1.py --output analysis/multiplayer/sandbox-source/power-body-slots/new-repeat/report.json
.tools/python/bin/python tools/export_mp_power_body_slots.py --report analysis/multiplayer/sandbox-source/power-body-slots/new-repeat/report.json --output analysis/multiplayer/sandbox-source/power-body-slots/new-export
```

The original fixed backing/platform context, actual tag residency and older
explicit optional services remain inherited boundaries. The clock is the
unchanged declared original fixture; this is one bounded original schedule,
not natural repeated frames or continuous settling. Raw stack bytes and excess
caller words are retained as provenance, not additional ABI arguments.

## Actual 1D95E0 result and shape lifetime

Original **1D9330** owns the runtime context, per-body mode bytes, grouping
packets and temporary result bank. Its two actual 1D95E0 calls in each phase
are retained: the first admits the one authored rigid body; the second rejects
the empty secondary group. The complete rejecting call returns AL zero and
leaves its result packet unchanged. The success indicator is **AL**, not all
of EAX: successful measured EAX words retain caller upper bits.
[1D9330](../analysis/multiplayer/sandbox-source/power-body-slots/final/measured/001d9330.asm),
[1D95E0](../analysis/multiplayer/sandbox-source/power-body-slots/final/measured/001d95e0.asm).

The exact 1D95E0 ABI is EAX = source-owned per-body mode byte bank and eight
stack words; the original returns with `ret 0x20`. The report keeps all eight
consumed words, full registers and literal entry/exit packets. Stack word0 is
the physical component, word1 its source runtime context, word7 the0x4C-byte
output packet. The positive word2 low byte causes the original 1DACF0 call;
it must not be silently omitted. The other words remain opaque source-owned
inputs until their enclosing producers are ported.

The result packet stores are:

| Offset | Actual owner |
|---|---|
|+00|body pointer, store1D9857|
|+04|selected rigid-body index bytes,1D986A/1D9871|
|+44|selected-byte count,1D9880|
|+48|low byte of source selector flag,1D9877|

Other output bytes are preserved. Runtime context `+48` owns the resident
physics-model pointer and `+50` the current node bank. A rigid row node of-1
uses the context transform at+08; otherwise the original selects the node at
`node_index * 0x34`. The source produces its four-lane matrix and position
packets, including explicit zero fourth lanes, at1D96C4..1D9768. No pose or
authored-root substitute is supplied.

The actual resident convex shape remains **8025CC40**, with the exact authored
vertex and plane banks from the earlier freeze. 1D98B0 temporarily increments
its reference at1D9994. The body constructor retains it, then1D9812 releases
the temporary reference. The source-created crate has one retained body slot.
This is source lifetime observation, not a live rule to reuse these captured
addresses. [1D98B0](../analysis/multiplayer/sandbox-source/power-body-slots/final/measured/001d98b0.asm).

The PVS rebuild also executes **1DACF0 → 1DADF0**. The original computes its
scalar using the actual shape bounds owner184510, parameter3F59999A, imported
constant46502C, current global486564 and signed original clock+02; the exact
SSE operation order appears at1DAE1D..1DAE44. It returns a real new0x40-byte
wrapper **0601AD20**, vtable418930, retaining the original convex shape at+30
and body identity at+34. The source stores it into body+0C at1DAD67 and sets
component flag10 at1DAD71. This wrapper producer is **measured but not ported
by the slot module**. Its complete raw state, inputs and owning stores are in
call62 and the enclosing admitted1D95E0 call53.
[1DACF0](../analysis/multiplayer/sandbox-source/power-body-slots/final/measured/001dacf0.asm),
[1DADF0](../analysis/multiplayer/sandbox-source/power-body-slots/final/measured/001dadf0.asm).

## Retained slot commit: native segment

Actual identities are object **E26F0000**, physical datum **E1680000**,
component **06200050**, body **0601A480**, slot **0601A6C0**. The original
allocator owns all of them. The PVS rebuild genuinely destroys and reconstructs
the body at the same address; equality of addresses does not preserve the old
body's lifetime. The original slot bank allocation is retained across disposal.
[Before/after states and calls20/41/67](../analysis/multiplayer/sandbox-source/power-body-slots/final/measured/report.json).

The component owns `{+70 pointer,+74 count,+78 capacity_word}` at stride0x60.
Its separate constraint bank is `{+7C pointer,+80 count,+84 capacity_word}`.
The actual slot's important stores are:

| Offset | Source-owned state |
|---|---|
|+00..08|current motion+ B0/B4/B8 root copied by1D4260 → 1D29A0|
|+0C..14|current linear vector, motion+40/44/48 when body+40 is zero|
|+18..20|current angular vector, motion+50/54/58 when body+40 is zero|
|+30..3F|explicit zero vector,1DC13C|
|+40|retained body pointer,1DC0CC|
|+44 / +45|zeroed bytes; +45 bit0 receives the caller flag|
|+48 / +4C / +50|rigid-index byte-bank pointer/count/capacity_word|
|+54|inline four-byte bank backing when capacity_word80000004|

When body+40 is nonzero, the original copies the imported vector at global
46FC44 instead of current velocities. Component flag40 selects a different
position owner1D42D0; the new native port stops explicitly there because this
positive branch is unmeasured. Bytes+24..2F,+46..47 and unused trailing bytes
are **preserved**, not cleared by a fabricated whole-slot initialization.
[1DC0C0](../analysis/multiplayer/sandbox-source/power-body-slots/final/measured/001dc0c0.asm),
[1D29A0](../analysis/multiplayer/sandbox-source/power-body-slots/final/measured/001d29a0.asm),
[1D4260](../analysis/multiplayer/sandbox-source/power-body-slots/final/measured/001d4260.asm).

1D3A70 preserves the original ordered ownership:

1. Grow the actual component slot bank if count equals capacity.
2. Increment component+74 and run1DC0C0 on the actual new slot.
3. Initialize body property2003 to0, allocate the index byte bank only if its
   original inline capacity is insufficient, then copy the actual index bytes.
4. Copy current root and vectors through1D29A0.
5. Replace body property2001 with actual object+ B4, then property2002 with
   the previous slot count. Existing keys are removed with318900 before318A90.
6. Call the actual motion vtable+18 getter. Mode7 leaves the component flag;
   the actual PVS mode4 sets component flag8000 at1D3B24.

The conditional at1D3ACF uses the slot-address ADD's flags across LEA/MOV;
it does not test the incremented count. Original constant-return virtuals
31B250/9D900 return7/4. Unknown virtual motion classes remain required owners.
[1D3A70](../analysis/multiplayer/sandbox-source/power-body-slots/final/measured/001d3a70.asm),
[318900](../analysis/multiplayer/sandbox-source/power-body-slots/final/measured/00318900.asm),
[virtual getters](../analysis/multiplayer/sandbox-source/power-body-slots/final/measured/0031b250.asm).

Native API: [physics_body_slots.hpp](../include/halo2/physics_body_slots.hpp),
[implementation](../src/physics_body_slots.cpp). The memory adapter must own
the actual banks, object header/body, current motion and imported constants.
No allocator returns a captured address and no callback supplies a successful
body kind, contact or world membership. It composes the earlier real small
allocator/entity property-bank native owner. External index growth and
replacement of pre-existing keys are source-listed branches, not extra
executed positive coverage in these six calls.

## Disposal and world delivery: measured, still required

The exact observed initial enclosing chain is
**1C7050 → 1C6F50 → 1D2AA0 → 1D2B10 → 1D9090 → 1D9330 →
1D95E0 → 1DAAC0 → 1DACF0 → 1D3A70**. The real component is stored in the
object, and the physical membership counter/flag are owned by the source.
The original27F190 calls are deferred invalid-object/removal processing;
they are not themselves the world-insertion owner.
[1C7050](../analysis/multiplayer/sandbox-source/power-body-slots/final/measured/001c7050.asm),
[1C6F50](../analysis/multiplayer/sandbox-source/power-body-slots/final/measured/001c6f50.asm),
[27F190](../analysis/multiplayer/sandbox-source/power-body-slots/final/measured/0027f190.asm).

Initial mode7 world membership already exists by the new1C70D0 entry. Its
first insertion between the captured1C7050 return and that entry is outside
this new observer's root scope; the unchanged earlier full factory/schedule
remains its evidence. This new slice must not be described as a new complete
measurement or native port of every initial world-insertion callback.

Actual PVS delivery enters **1C70D0**, removes the old body's world membership
through1D4C50, then runs complete1D3BB0. It resets/replaces keys2001/2002,
decrements the body reference, calls the real body/motion/entity destructors
and allocator frees, zeros slot+40, and resets component flags/selectors/counts.
It retains component+70 and capacity. 1C7129 writes the actual requested flags;
1D2B10 rebuilds and commits; 1D4F30 then delivers the new body through
**1C7320 → 315820** and installs its source callback/property record.
The new report retains full entry/exit backing and every ordered store in these
enclosing calls, including real broadphase/world/allocator changes.
[1C70D0](../analysis/multiplayer/sandbox-source/power-body-slots/final/measured/001c70d0.asm),
[1D3BB0](../analysis/multiplayer/sandbox-source/power-body-slots/final/measured/001d3bb0.asm),
[body destructor](../analysis/multiplayer/sandbox-source/power-body-slots/final/measured/00317970.asm),
[1D4F30](../analysis/multiplayer/sandbox-source/power-body-slots/final/measured/001d4f30.asm).

**Minimum remaining live bridge:** port the actual1D9330/runtime grouping
producer and admitted1D95E0 selection/wrapper lifecycle, complete body-slot
disposal/destructors, and real world insert/remove/broadphase/solver owners.
The new native commit can then retain those same actual identities and feed
the already verified contact-row/refresh/manager consumers. A caller must not
borrow these captured world records or use shape-local AABB bounds as a box
collider. The backend must consume the retained authored vertices/planes and
deliver its own encoded entity/property keys, current quantized bounds,
contacts and solver callbacks. Positive player-versus-crate blocking,
incoming damage/destruction and explosion are still unproved.

## Comparison and authenticated1.5 correspondence

The [exporter](../tools/export_mp_power_body_slots.py) uses actual original
buffers/reads/stores only. Six independently repeated native fixtures compare
**7,093,906 full retained bytes and345 ordered stores**. Internal stack/FS
plumbing is excluded; all actual retained object/component/allocator/body/
motion/property/slot buffers remain. Normal O2/Wall/Wextra/Werror,
ASan+UBSan/detect_leaks and production-library CTest pass.
[Fixtures](../analysis/multiplayer/sandbox-source/power-body-slots/final/exported-owner/fixtures.tsv),
[normal](../analysis/multiplayer/sandbox-source/power-body-slots/final/validated-native-normal.txt),
[sanitizers](../analysis/multiplayer/sandbox-source/power-body-slots/final/validated-native-sanitizer.txt),
[CTest](../analysis/multiplayer/sandbox-source/power-body-slots/final/ctest-normal.txt).

The [correspondence tool](../tools/research_mp_power_body_slots_correspondence.py)
proves all27 complete bodies/internal branch topology against authenticated
1.5 SHA21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd.
Short virtual getters are resolved from prior original constructed vtables,
then fully compared, rather than selected by an ambiguous prefix.
[Complete paired report/listings](../analysis/multiplayer/sandbox-source/power-body-slots/final/later-correspondence/complete-report.json).

| TU1.1 | Authenticated1.5 |
|---|---|
|1C7050 /1C70D0|1CC290 /1CC310|
|1D9090 /1D9330 /1D95E0|1DD950 /1DDBF0 /1DDEA0|
|1D98B0 /1DA3C0|1DE170 /1DEC80|
|1DACF0 /1DADF0|1DF5B0 /1DF6B0|
|1D3A70 /1DC0C0 /1D29A0|1D8330 /1E0980 /1D7260|
|1D3BB0 /317970 /318900|1D8470 /31B4B0 /31C450|
|31B250 /9D900|31EDE0 /2E8CF0|

This is static full-body correspondence, not1.5 gameplay execution. Native
APIs intentionally retain the1.1 storage namespace; actual image-bound
globals/vtables/constants must be mapped/imported before1.5 live use.

The independent [freeze manifest](../analysis/multiplayer/sandbox-source/power-body-slots/final/manifest.json)
pins this slice. No Scene, Simulation, runtime, renderer, damage or release
file is edited. Earlier inner-only observer/repeat and failed observer-return
attempts are retained outside the final freeze and are not its authority.
Those failed attempts incorrectly treated an interior constructor instruction
as an extra function entry; the observer was corrected, with no guest change.
