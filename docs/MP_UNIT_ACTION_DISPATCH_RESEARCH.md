# Retained unit action allocation and dispatch

The new `unit_action_dispatch` module implements the complete original direct
owners `BD170`, `B8250`, `E7F30`, `E7EF0`, `E7FF0`, `E7E60`, `1EB150` and
`1EA310`. Its differential passes 71 complete-buffer cases, ten explicitly
supplied original lower callbacks and 463 owning stores. Normal, GCC/Clang
ASan/UBSan and the registered CTest pass. This closes the allocation/dispatch
prefix around [grenade begin](MP_GRENADE_BEGIN_BODY_RESEARCH.md); it does not
make native Q throw a grenade.

The immutable evidence is
`analysis/multiplayer/integration/spawn-source/unit-action-dispatch/manifest.json`.
`REPRODUCE.md` in that directory records exact commands and dependencies.
Execution uses the unchanged TU1.1 XBE (`aaea4c9c…a5377`) and actual Lockout
(`14c54046…987a9`). Independent complete-body correspondence maps all eight
owners into authenticated u5 (`21b207e7…9c4fd`), retaining relocated data reads,
call targets and internal branches. It is static correspondence, not u5
owning execution or original console/device admission.

## Allocation and retained backing

`allocate_source_object_body()` ports all of `BD170`: obtain the original full
header datum, pass the sign-extended requested size to the required body
allocator, release the header if allocation fails, or write header word `+6`
then clear exactly the requested extent with the original DWORD/remainder-byte
stores. The source run executes actual `16D2C0` and `18FCE0`, rather than
supplying allocation success, contents or a reserved unit body.

The two measured calls allocate Spartan `E26F0000` with 1124 bytes (`464h`) and
SMG `E2700001` with 604 bytes (`25Ch`). The original tag-type record's signed
word `+8` owns each size. The later 5750-byte unit is assembled by further
source append/constructor calls; this prefix does not authorize clearing or
installing its whole final body.

`SourceObjectBodyAllocationServices` requires genuine header allocation,
body allocation and failed-header release providers. Host-owned storage can
back their requested bytes, but no provider may return success without owning
that storage. The existing `UnitConstructorObjectPool` supplies literal
12-byte headers and stable mutable body spans. Native bounds reject malformed
successful extents; they do not replace an original allocation failure.

`mark_source_object_for_update()` ports `B8250`: a set initial header bit2
returns immediately. Otherwise it sets that bit on the object and each parent
read from generic body `+14` through `NONE`. The loop does not retest ancestor
flags. The 38 natural calls include two first writes `08→0A`, made by caller
`BB2B8` for the unit and weapon. A bounded host cycle guard stops corrupt
parent chains. A `NONE` initial header is an invalid original access, not a
fabricated harmless call.

## Complete dispatcher contract

The API is [unit_action_dispatch.hpp](../include/halo2/unit_action_dispatch.hpp).
It retains the following source order:

| Owner | Complete direct behavior |
| --- | --- |
| `E7F30` | Read request type, obtain its descriptor/begin target, mark the object, invoke the actual begin callback, then notify the bound player with the defined low AL. |
| `E7EF0` | Clear all eight DWORDs of its `20h` local request, store the type, then call `E7F30`. |
| `E7FF0` | Retain the signed `unit+346` component pointer, invoke descriptor `+C` if present, then clear that type's live bit after the callback. |
| `E7E60` | Walk types0–59 ascending, checking the current bit each time. A nonzero descriptor `+4` invokes its actual update; true AL retains the aggregate, false clears the live bit after the callback, and a null update leaves the bit intact. |
| `1EB150` | Read the constructed player's signed local-user word `+28`; `-1` returns. Otherwise reach that user's real view record and `1EA310`. |
| `1EA310` | Select the oldest of four signed timestamps with strict `<` and earliest-slot ties. Store actual clock `+8`, low16 action type and zero-extended low8 result into its eight-byte notification. |

`source_unit_action_component()` follows the produced signed word `+346` and
checks the complete `44h` span. It does not create or reset that component.
The original dispatcher indexes objects and players by low16 internally;
valid full salted ownership is the caller's separate admission requirement.
Callbacks must preserve the retained backing addresses while those pointers
are live. The native API preserves raw defined AL rather than normalizing a
begin result to Boolean.

All 60 literal descriptor records come from the selected original executable.
`original-data-owned.json` contains the complete provider packet:

| Image | Action pointer bank | Type pointer bank |
| --- | --- | --- |
| TU1.1 | `46EC68` | `46FAD0` |
| authenticated u5 | `473D28` | `474B90` |

The import seam is `array<array<byte,16>,60>` plus actual signed type-record
extent words for supported object types0/2/5. Four callback words remain opaque
original identities, never host function pointers. The packet includes actual
indexed reader instructions and the prior complete `B8A30→B9B70` proof for
the type table. It provides no callback implementation, resource admission,
body construction or player/view binding.

## Owning cases and exact validation boundary

The observer adds no guest patch, gameplay field or successful return to the
unchanged admitted Q composition. Complete before/after states and returned
CPU reproduce the frozen grenade-begin composition. It captures the entire
`4Ch` object-pool record, all 2048 headers, actual body allocations, packets,
callback identities and stores. The 71 native cases comprise 27 dispatcher
calls, two allocations, 38 update-mark calls and four local notifications.

The ten lower callbacks in the native differential validate complete source
before-buffers, apply the captured complete source after-buffers and deliver
their captured defined returns. They are explicit test services, not completed
native grenade/animation/effect/world implementations. The direct owner's
stores are compared separately from recursive callback stores. `E7EF0` has
no externally supplied native stack history: only its request-store prior
bytes are excluded; instruction, identity, order, width, written value and
complete resulting request bytes remain exact.

The local notifier cases execute unchanged `1EB150→1EA310` on literal complete
records produced by the prior `1874D0/1EA3D0/156A30` constructors/resets and
the actual `1508B0` local setter. Recorded type19/false and type22/true callback
operands are delivered twice without advancing the recorded source clock.
All timestamp ties therefore choose slot0. Mapping these source-created
records into owned fixture backing is declared; natural controller/machine
membership is not claimed. Upper result-word padding is retained in the
original operand, but the owner consumes only low8.

Independent execution repeats every nested runtime report and instruction
file byte-for-byte. The outer observer report differs only in its recorded
`owning_report` destination path. The native fixture differs only in the
comment pinning that outer report. The audit permits exactly those two
identified provenance differences; it compares all remaining bytes/fields.
Local notification, u5 correspondence and original-data packets repeat
byte-for-byte. Ten additional host bounds/provider tests are explicitly not
additional original cases.

Initial host observer/exporter errors are retained under `observer-failure`,
`observed-v2` and `native-draft*`. They are not guest faults. No old freeze,
runtime, renderer, shader or release file is changed. The observed linked
archives are pinned dependencies, without a claim that every member was
rebuilt from a captured complete source tree.

## Smallest live connection still required

Current `Unit` retains typed inventory/zoom, counts, grenade selectors and a
separate scalar action word. It has no retained complete source body or the
constructor-owned managers/components addressed by signed `+12A/+33E/+346`.
Current object allocation also writes the type byte before the original
`B8A30` placement of that store. A native allocated player-pool slot's zeros
are not a produced player `+28=0` local-user assignment. These must not be
treated as completed source construction.

The live composition must first run the complete `B8A30` generic body/append
path, `BD270` and `C52C0` against retained owned body storage. Existing direct
constructor ports still require actual `110A20` extension initialization,
`116680` model/variant selection and `110BF0` default selection providers.
Their full owning output cannot be supplied by replaying the measured final
unit. Existing graph/definition/resolution/variant modules do not close
`1D0790/1D06C0`, root transform `1D1A00` or the resource-lifetime owners.
Those obligations must remain explicit until ported.

Once constructed, the same actual component `+4/+8` must own the action bits;
the typed scalar must not become a second independently mutable authority.
Generic parent `+14` and unit-specific `+12C` are different source fields.
Bind the real full player at unit `+13C`, its constructor-produced local-user
record and real view/clock owner where the notifier branch requires them.
Then dispatch Q through the actual incoming flag `04000000` and ordinary
`150F00→E7F30→type22/E95E0` gates, retaining actual grenade selectors/counts.

The source ordinary biped phase reaches `DEDD4→E7E60→E99B0`. The first admitted
update advances counter0→1 without decrementing counts. The next advances
1→2 and calls `E8D60`, decrementing2→1 before changed notification, marker
`09000100` query and original projectile creation. The frozen sequence stops
inside that creation before attachment/phase2 at the missing looping-sound
pool (`18A9C0→16D2C3`, global `4F485C`). This task does not replay NULL audio
driver boot or supply success. Complete attachment, held/release transition,
projectile motion/fuse/explosion and native authority/replication remain
required. A genuine backend result can be delivered only through its actual
request/metadata owner; begin admission alone does not establish a throw.
