# Original crate contact bank, delivery, refresh and removal

The native contact storage owner now matches the genuine source-created Lockout
power-module contact: 11 measured calls, 8,494,579 full retained buffer bytes,
90 ordered stores, six actual virtual deliveries and 12 arithmetic snapshots.
This is a TU1.1 stored-state integration segment. It creates neither a native
world/body nor a solver contact, and does not establish live crate collision,
continuous support, player contact, damage or explosion.

## Authority and immutable inputs

The executable is the original TU1.1 `default.xbe.03`, SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
The placement, component, body, world, PVS and contact all come from the
unchanged original preparation and schedule in
[MP_POWER_MODULE_CONTACT_SCHEDULE_RESEARCH.md](MP_POWER_MODULE_CONTACT_SCHEDULE_RESEARCH.md).
That freeze's manifest remains
`948723b3077688265b4392ef130c679deb4744fdb41c6558f260520150d9185a`
(1,254 artifacts). The separate constructor freeze
[MP_PHYSICS_CONTACT_RECORD_RESEARCH.md](MP_PHYSICS_CONTACT_RECORD_RESEARCH.md)
remains `53a3ed036f9a412d879cc34ce6fba72c5c6d1aafb7bf56037913c32f4184b626`
(18 artifacts). Neither is edited or regenerated here.

The new observer executes the same authored crate factory/fixup and real
player/PVS preparation, followed by exactly the same six declared original
roots. All prior stage, call, store, callback, state and bounds records compare
identically. No added guest writes, instruction patches, source return,
contact, body/key, node, damage, timing or admission input exists. The inherited
12-byte game-clock fixture is still declared; this is one original schedule,
not a natural repeated-frame or continuous-settling proof.

The final source tool is
`tools/oracle_mp_physics_contact_lifecycle_1_1.py`, SHA-256
`547f5461b1b4bdbe233fb2f6a98f1eeec8f779c0047608ca8d3836a6f1f10016`.
Its measured and independently repeated reports are byte-identical, SHA-256
`68221efe9154aa5c958cf30a8847576d1195abfada5df54c26736884f13dd8b2`.
They retain 88 calls, 460 reads, 232 stores, 95 full states and 16 math snapshots.
All 15 complete routine listings repeat identically and every instruction byte
is checked against the original executable. `22E980` is an internal jump block
of `22E950`, not a callable routine; its actual block state is retained separately.
The earlier observer-only classification failure and draft output are preserved.

Canonical new evidence is under
`analysis/multiplayer/sandbox-source/contact-lifecycle/final/`:

* `measured/` and `repeat/`: original executions and complete listings.
* `exported-math/` and `exported-math-repeat/`: independent literal exports.
* `audit.json`: source bytes, repeats, old pins and export verification.
* `frozen-native/`: validated native source/header/test copies.
* `manifest.json`: immutable artifact hashes and dependency pins.

## Actual append and allocator ownership

Source callback `27DFE6` calls full `1D3DF0` with these 13 stack words:

```
06200050 00000000 3F800000 2000081D 0000081D FFFFFFFF
3F800000 0601A480 06005250 3F5FE897 3EFAD3E8 B7850056 3F800068
```

The component is `06200050`. Its descriptor is `+88` pointer, `+8C` signed
count and `+90` capacity word with a high-bit ownership flag. The actual initial
descriptor is `[0,0,80000000]`. Below the signed 28-row limit, `1D3DF0` requests
`max(2 * masked_capacity, count + 1)` records of stride `48` hexadecimal.
`2DF710` dispatches through global `487638`'s actual adapter virtual `+10`.
The actual target is `148A10`; source adapter mode at `480D98` is 2, so it reads
the current backend at `480DA0`. Its actual virtual `+10` is `22E950`, receiving
72 bytes and opaque allocator tag `12` hexadecimal. No returned address is
supplied to this experiment or to the native differential.

The admitted backend is `06016040`. The original signed class byte at
`allocator + AC + requested_bytes` is 6. The actual nonempty freelist branch
returns its current head `0601A720` with this order:

| Owning instruction | Actual field | Before → after |
|---|---|---|
| `22E98F` | outstanding count `06016340` | `2 → 3` |
| `22E99A` | class head `0601607C` | `0601A720 → 0` |
| `22E9A7` | free count `061063E0` | `1 → 0` |
| `2DF768` | descriptor pointer `062000D8` | `0 → 0601A720` |
| `2DF76B` | descriptor capacity `062000E0` | `80000000 → 1` |
| `1D3F29` | descriptor count `062000DC` | `0 → 1` |

The allocator reads the actual free node's next pointer. The cold vector copies
zero bytes and the initial negative capacity word skips the free dispatch.
The original constructor `1D2DF0` then owns its 14 stores within the allocated
72-byte row, using the previously verified constructor module. Untouched bytes
retain actual allocator content; no bank or row is zero-filled by the port.
`1D3DF0` returns the original row index 0 and cleans 13 stack arguments.

The native adapter reads actual source memory and checks the reached virtual
targets. Unsupported adapter mode, allocator size/empty-list/growth, nonempty
old-bank copying, old-bank free, full-bank eviction and positive component
`1DC1A0` dispatch remain explicit hard boundaries. Some broader allocator
branches execute elsewhere in the complete source-world report; they are not
claimed by this narrowly exported crate-bank fast branch.

## Real manager packets and refresh

Full `1D2F10` receives EAX manager `0601AFA0`, ECX row `0601A720`, then:

| Call | Four raw stack words |
|---|---|
| Initial | `00000001 00000000 E1680000 00000000` |
| Later | `00000000 64E39500 E1680000 00000000` |

The first two arguments consume their low bytes; `64E39500` is not a selected
32-bit mode. The third is the actual physical component datum; the fourth
selects the current body slot. Source pool `526248` owns the A0-stride component
record. Component `+70` owns 60-stride body slots; slot `+40` supplies the body,
whose `+3C` supplies its current motion object.

The actual manager virtual calls are `1D2F2A→31F4B0` for the point packet and
`1D2F39→31F4D0` for the scalar packet. Both consume the contact index's low16,
look up the actual byte index map at manager `+30`, then return the mapped
32-stride packet from `+44` or 28-stride packet from `+A0`. The report retains
the whole bounded 120-byte manager prefix, descriptor words and full capacities
of all three actual banks, plus every returned full packet. No normal or point
is reconstructed by the exporter.

The actual packet is `0601A980`; scalar packet is `0601B04C`.

| Packet field | Actual source words |
|---|---|
| Point `+0/+4/+8` | `40F1DD57 C1037D9F C1063D32` |
| Normal `+10/+14/+18` | `B58C6F20 36040BC0 3F7FFFFF` |
| Initial / later distance `+0C` | `37606000 / 375FFFFC` |
| Scalar packet `+0` | `00000000` |

Each refresh first stores distance at row `+34`, point at `+1C/+20/+24`, normal
at `+28/+2C/+30`, and computed force at `+38`, preserving original store sites,
prefetch order and literal signed bits. Motion virtual `1D2FB0` actually reaches
the complete six-byte `9D900` leaf (`mov eax,4; ret`); the caller ignores that
return. It is not an invented admission or readiness success.

Actual motion `+2C` is `3B4DBB88`. Source scalar division produces denominator
`439F4656`; the native math observer compares that intermediate independently
of the final zero force. Source executable reads remain raw imported words:
one at `465030`, threshold at `4651A0`, and clock multiplier at `465198`.
The original negative-one operand at `46503C` belongs to the unexecuted
normal-flip branch. The actual noncharacter route `1D32A0` copies row
`+3C/+40` (`3F5FE897/3EFAD3E8`) into scalar packet `+8/+C`, using stores
`1D332B/1D3333`. They are caller/constructor-owned values, not selected native
friction parameters.

The actual x87 control word is `023F`, MXCSR is `00001F80`. Original clock rate
30 multiplied by the imported `3EB33333` yields the actual `41280000` float
spill. Original `FISTP` yields integer `0000000A` under the actual nearest-even
rounding mode. The observed clock/component timestamp difference is also 10,
so the dirty-wakeup store is skipped. The native module receives the recorded
control inputs, uses scalar SSE and x87 FISTP, and restores the host FP
environment. SIMD/x87 status flags and unmasked exception side effects are
outside its stored-output contract; an unavailable x87 backend rejects.

Both refreshes finish with exactly ten nonstack stores. Source-positive flipped
normal, character coefficient, object wakeup and impulse branches are listed
in the complete original routine, but unexecuted by these refresh fixtures;
the native API rejects at their owning boundary after preceding source stores.
It does not promote the void routine's incidental EAX into an admission result.

## Callback removal and retained lifetime

Actual `27E353→1D3FC0` receives BSP body `06005250`, crate body `0601A480`,
component `06200050`, and callback packet `021FCF08`. The extension measures
the literal two-byte callback index both at entry and immediately before
`1D404D`: `0000`. It is not inferred from the current row.

The full source property scans identify BSP key `2001→FFFFFFFF`, crate body
slot `2002→0`, and exhausted BSP slot lookup yielding 0. The row match requires
low16 contact index, row `+14` counterpart component key, and sign-extended
row `+45/+46` slot bytes. The matching original branch tail-jumps into
`1D40B0` using the same original caller frame, replacing only the slot-index
stack word with 0. Both owning call records retain that alias and cleanup.

The actual row impact datum is `ED690000`. Pool `52648C` and its A0-stride
backing supply the real impact record `0930004C`; reference count is 1 and type
byte `+10` is 0. Source `2298B0` skips the type1 last-reference effect path and
decrements the 16-bit reference to zero at `229901`. Then:

1. `1D410C` writes `FFFFFFFF` to row `+8`.
2. `1D4125` changes component count 1→0.
3. `1D4135` executes 18 forward `REP MOVSD` stores, including literal self-copy.

Capacity remains 1 and the retired row bytes remain allocated. Nothing frees
the bank or synthesizes a persistent terrain-support contact. Positive impact
effect delivery and the post-removal `1DC1A0` component path are explicit
unavailable boundaries. The native matcher and remover retain exact original
order, before/written bytes, typed impact identity and allocation lifetime.

## Native seam, validation and remaining live bridge

New files are `include/halo2/physics_contact_lifecycle.hpp`,
`src/physics_contact_lifecycle.cpp`, and
`tests/physics_contact_lifecycle_tests.cpp`. They consume
`SourcePhysicsContactMemory` retained storage; they do not own a replacement
collision world. The required getter, allocator and motion target checks make
an absent backend a hard failure. The four source Boolean/slot/datum words,
clock, x87/MXCSR, properties, packets and bank backing remain required inputs.

Only necessary CMake source/test registration is changed. The target is
`physics_contact_lifecycle_tests`; CTest is `original_physics_contact_lifecycle`.
Expected data comes entirely from independently repeated original snapshots,
reads and ordered writes; every changed buffer correlates to original full exit
states. Normal and ASan/UBSan comparisons cover all 11 calls, 90 stores, six
virtual deliveries and 12 intermediate arithmetic snapshots. The separate
constructor's unchanged standalone suite is not repeated.

The minimum live bridge is still source-owned crate phmo fixup and object/body
allocation/world insertion, actual current transform/bounds and body properties,
genuine PVS/world scheduling, and `1C7660→317650` solver callbacks producing the
real manager/index/point/scalar packets. The separate bounds evidence needs
the actual quantized-bounds virtual producer; an approximate mesh or sphere
cannot stand in for it. Native `SourcePropScene` does not currently supply these
Havok body/manager/callback owners. Captured fixture contacts cannot be fed to a
live crate as if newly produced.

Positive player-versus-crate collision, pushing, hit delivery, damage,
destruction and explosion remain separate. The frozen crate's `D73E0` pending
damage input is zero and rejects; no positive explosion packet is measured.
No new crate interaction action is inferred. Therefore this storage module
must not be presented as repairing the screenshot's collision/explosion symptom.
The existing constructor has independently proved TU1.5 correspondence; this
new lifecycle suite does not claim a full TU1.5 world or lifecycle mapping.

Reproduce the unchanged original execution, literal export and audit with:

```
.tools/python/bin/python tools/oracle_mp_physics_contact_lifecycle_1_1.py
.tools/python/bin/python tools/oracle_mp_physics_contact_lifecycle_1_1.py --output analysis/multiplayer/sandbox-source/contact-lifecycle/final/repeat/report.json
.tools/python/bin/python tools/export_mp_physics_contact_lifecycle.py
.tools/python/bin/python tools/export_mp_physics_contact_lifecycle.py --output analysis/multiplayer/sandbox-source/contact-lifecycle/final/exported-math-repeat
.tools/python/bin/python tools/audit_mp_physics_contact_lifecycle.py
```

Existing frozen-write helpers require identical content for existing artifacts.
