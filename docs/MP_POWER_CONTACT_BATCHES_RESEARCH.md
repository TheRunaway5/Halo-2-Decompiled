# Original power-module contact packet retention and delivery

This independent extension observes the same genuine Lockout canister construction, source player spawn/PVS and selected ordinary/physics/dirty/new-object schedule as the immutable [GJK owners](MP_POWER_GJK_OWNERS_RESEARCH.md). It adds no guest fields, register results, contacts, geometry, allocator success, clock increment, admission or instruction patch. The entire preceding GJK call/read/write/state report and power schedule compare identically before the new report is written.

The original canister path creates two child packets, admits the second child, calls the real manager, retains its index and emits one contact into the original solver output descriptor. The isolated native module now matches the actual packet helpers, point transforms/velocities, empty pruning range and both empty/positive emission calls. Native retained-contact creation stops at the exact missing constraint/world-insertion owner `313800`. This is **not live canister collision, completed native solver ownership, continuous settling, incoming damage or explosion**.

## Files and verification scope

- Original observer: `tools/oracle_mp_power_contact_batches_1_1.py`.
- Original-only exporter: `tools/export_mp_power_contact_batches.py`.
- Complete-body authenticated update correspondence: `tools/research_mp_power_contact_batches_correspondence.py`.
- Independent audit: `tools/audit_mp_power_contact_batches.py`.
- Native interface/implementation/test: `include/halo2/physics_contact_batches.hpp`, `src/physics_contact_batches.cpp`, `tests/physics_contact_batches_tests.cpp`.
- Ignored immutable evidence: `analysis/multiplayer/sandbox-source/power-contact-batches/final/`; `manifest.json` pins exact report, listings, exports, tools, native sources, binaries and validation. Earlier failed/draft attempts remain outside that freeze.

The source report has 28 observed complete calls, 18 deduplicated full states, 6,718 reads and 3,431 stores. Its 11 complete root listings cover 1,472 original instructions. Source observer call and prior scheduler call-tree order counters have separate domains: exported stop correlation uses the exact owning routine/caller/phase/stack and its first actual source read, not numeric order equality between those domains. ABI saves/pushes/SEH are preserved in the raw report and excluded from native owning-write expectations. Every owning temporary and retained field write is retained; packet holes remain untouched.

The native export has 26 cases: **24 complete calls and two exact `313800` prefix stops**, comparing 30,772,774 bytes and 179 ordered owning writes. One prefix composes `309210` into the new native `31F1B0`; the other begins at `31F1B0` itself. Two additional tests remove every provider and compare the exact earlier original prefixes, ensuring no successful callback default exists. No callback supplies recorded output or returns a chosen success. The manager tail after that stop is translated from the complete original body but **has not completed or been differentially verified natively**. Full original `300870` is measured and paired; its native orchestration is not implemented in this slice. Positive `300650` geometry/pruning, contact-match reuse, five-point reduction, emission retirement, bank growth/free and alternate manager branches are source-listed unless separately identified below.

Normal optimized, ASan+UBSan with leak checks, independent source/export repeats and fresh production-library CTest results are recorded in `validation.json` and `audit.json`. These checks are CPU evidence over explicit retained original storage, not Xbox hardware or full-game evidence.

## Exact packet and retained-row ownership

The complete unchanged listings in `final/measured/` are the authority; addresses below are TU1.1 instruction addresses. Caller raw stack tails are provenance, not additional ABI arguments.

| Buffer | Fields actually consumed/owned |
|---|---|
| Packet, stride `80` | `00` narrowphase normal, `10` A-local point, `20` distance, `24` blend weight, `30` B-local point, `40` world normal, `50` B-local point for pruning, `60` B-local normal with distance at `6C`, `70` child agent, `74` child shape, `78` original child key, `7C` pruning eligibility, `7D` lower narrowphase result. |
| Packet copy `300100` | Stores `00/10`, `20/24`, `30/40/50/60`, `70/74/78`, bytes `7C/7D`, in that order (`300109..30015B`). Leaves `28..2F` and `7E..7F` unchanged. Loads/stores retain original alias order. |
| Retained row, stride `30` | A-local point `00`, weight `0C`, B-local point `10`, encoded manager index `1C`, world normal `20`, distance `2C`. Index assignment is `3F000000 | zero_extended_AX` (`3093DB..3093EF`). |
| Emitted solver row, stride `30` | Weighted world point `00`, projection at `0C`, normal `10`, low16 manager index at `20`. Other row bytes remain untouched (`309627..309639`). |
| Descriptor | Pointer `00`, count `04`, signed capacity/ownership word `08`. Scratch descriptors also retain original block `0C` and original element count `10`. |

`300400` creates a packet by copying its genuine retained stack buffer. It initializes only the two flag bytes before the copy (`300414/30041B`), increments descriptor count (`30044E`), then calls `300100`. It does not clear all packet fields. A native adapter must provide actual retained caller backing; missing bytes fail, never become fabricated zero packet fields. `300870` writes child identity/key/result fields after this copy; it fills geometric result fields only for the lower positive result (`300A1D..300B55`).

`3001D0` initializes the scratch descriptor and requests `(count << 8 + F) & ~F` bytes. If the original adapter has sufficient signed available capacity, its cursor/available fields are updated at `300231/300234`; otherwise actual virtual `+24` allocation is required. `300260` returns the same extent through the original scratch owner or virtual `+28`, and dispatches virtual `+14` free only for nonnegative owned capacity. The measured genuine counts are two and zero; neither allocates alternate backing. These functions do not choose an arena or replace the allocator.

`300650` accepts ECX parent and five stack arguments `{candidate20, packet_descriptor, start, total, context}` (`ret14`). Its actual source call has `start=total=2`, returns two and makes no `2FFD60` geometry call. The complete translated positive body retains original swaps, untouched holes, exact stack aliases and required `2FFD60` output ownership; those positive branches are **unexecuted here**. No native geometry predicate fills that boundary.

## Contact retention and positive emission

`309210` is cdecl with seven arguments `{collidableA, collidableB, step, packet80, manager, retained_rows, retained_count_pointer}`. Frame backing is `((entry_esp - 4) & ~F) - B0`.

It squares `*(step+0C+04)` on x87 and searches existing rows for strictly smaller A-local position distance, then strictly smaller normal difference than the executable constant at `4654C0` (raw word `3C23D70B`; paired update address `46A560`). Position/normal squared sums are scalar SSE `(Y+X)+Z`, with original spills at `309273/3092AC`. An existing match preserves its encoded key through the original x87 load/store while replacing the point/normal/weight/distance fields. That match branch is listed, not executed by the cold genuine positive case.

Without a match, it writes the retained row, executes actual `2E0BA0` transforms for both local points, computes the weighted world point with the original all-ones vector at `418A60`, writes its exact temporary packet, and invokes manager virtual `+4` (`3093CF`). The actual virtual is `31F1B0`. Only returned AX different from `FFFF` stores the encoded index and increments retained count. A fifth row invokes required actual `308FB0` reduction; no guessed replacement/reduction is available.

`309450` is cdecl with eight arguments `{collidableA, collidableB, step, minimum_distance_word, retained_rows, retained_count_pointer, output_packet, manager}`. Frame backing is `((entry_esp - 4) & ~F) - 30`.

It reserves output count before scanning (`3094B6`), requiring real `2DF710` growth if necessary. For each row it transforms both points with source columns, preserving **`(X+Y)+(Z+translation)`** SSE ordering. It stores projection at `309582/309594` and tangent distance squared at `3095D9`. Acceptance requires all original strict ordered comparisons: projection `< *(step+08)`, projection `> minimum_distance`, tangent distance squared `< squared(*(step+0C+08))`. Unordered/equal comparisons reject. Positive output stores the weighted world point, normal, projection and index, then advances accepted count. Rejection requires actual manager virtual `+8` (`30965A`), decrements retained count, swaps the last retained row into the rejected row, and decrements the original output descriptor count. Retirement was not entered by these two measured calls.

The actual first child has count zero; its emission is empty. For child key `2000081D`, retained count is one and remains one, output count changes zero→one, and the emitted row begins:

```
57ddf1409f7d03c1323d06c1fcff5f37
206f8cb5c00b0436ffff7f3f00606037
00000000000000000000000000000000
```

These are original emitted bytes. They are an isolated stored-input comparison, **never a live contact callback or a borrowed scene contact**.

## Manager, allocation and solver boundaries

`31F1B0` has ECX manager, four stack arguments `{collidableA, collidableB, step, world_contact20}`, `ret10`. Frame backing is `((entry_esp - 4) & ~F) - 80`. Native return type is the semantically consumed low16 manager index; caller scratch high EAX bits are not invented.

The unchanged complete body follows collidable parent `+0C` chains, reads each root property `+20`, rejects manager count `+A4 > FA`, and, for zero count, calls actual world `313800(world, manager+10)` before slot allocation. The genuine native prefix ends there with exact source-owned frame fields and ABI:

```
call instruction31F21B, entry313800
ECX06005470, entryESP021FCA78
stack argument0601AFB0
```

This is a required **world/constraint insertion** provider, not a missing guessed hit test. The following original owners also execute in the original report, but are not fabricated by the native service:

| Owner | Required real ownership |
|---|---|
| `313800` | Retained world/constraint insertion; actual `31D2B0`, `322320`, `31C520`, `31E7D0` downstream world/island state. |
| `322410` | Manager point/scalar slot allocation and index-bank ownership. Actual source requests include real `2DF710`/allocator backing. Must return actual index and write both output pointers. |
| Motion virtual `+58` | Actual `31FEB0` point velocity on the retained motion. This leaf is fully compared here; other virtual implementations require their actual owner. |
| `31E8B0` | Real world creation listener dispatch. Its callbacks may modify the packet and manager state; no default continuation. |
| `31C2E0` | Actual property/entity contact-creation listeners where root property `+64` is positive. This differs from the existing output-refresh listener `31C3C0`. |
| Manager virtual `+8` | Actual veto/retirement owner if creation packet `+20 == 1` or emitted contact rejects. No index/free default. |

The original creates point-bank slot `0601A980`, scalar slot `0601B04C`, index zero in manager `0601AFA0`; those addresses are captured identities, **not native allocation constants**. Manager index/point/scalar counts all change zero→one through the real source allocation chain. Later packet/contact fields are not enough to replace these owning banks.

The complete source manager also computes velocity dot, material products/square roots and bias/impulse scalars with original x87 store/pop order and executable words. Its native tail remains uncompleted pending the real providers above. The complete source body and authenticated correspondence are retained so the next owner can close it without a replayed finished state.

## Actual `300870` composition and live gate

The genuine scheduler calls `300870` first for canister parent `0601ADE0`, then for player parent `0601AD60`; both receive actual world step `060055B0` and output packet `021FCF60`. The first path constructs two real BSP child shapes/agents; the second child query is cold/empty. The full original call tree retains allocator, terrain, GJK, manager, listener and solver delivery order.

The complete `300870` body executes `2EFC30` child cache first; original child-shape virtual `+34`; shape-type/table capability; actual child motion narrowphase/GJK; packet append/identity stores; source distance ordering and pruning; existing contact scan; `309210` retained ownership; then `309450` emission for every packet. It disposes scratch and owned packet storage last. The original profiling-positive branch requires RDTSC ownership; actual profiling capacity is zero. Source negative packets retain their untouched stored fields; they are not converted into guessed geometry or a cleared packet.

The next native work is complete `300870` orchestration plus genuine `313800`/`322410`/creation-listener ownership. Existing GJK may be composed with an actual platform reciprocal-square-root estimate provider. **Unicorn RSQRTSS is emulator evidence, not Xbox/Pentium III hardware evidence.** Prior measured emulator estimates remain test-only; this module adds no estimate injection, and no live callback consumes a completed captured workspace. The frozen actual-host estimate diagnostic differs in the admitted canister vector. Hardware/contact-decision parity therefore remains explicitly unproved.

Source raw object/header/body/property/shape/motion/world/pair/agent/row identities and source-owned banks must stay distinct in the live adapter. Existing SourcePropScene has no completed owner bridge across these constraints, current broadphase, child agents and solver delivery. No AABB, sphere/capsule, SAT contact or successful callback substitutes for it. Player-versus-crate contact, actual solver motion and incoming damage/destruction remain separate genuine gates; current freeze does not make the screenshot's canister collision or explosion work.

## Authenticated TU1.5 correspondence

All 11 complete bodies match normalized original instructions/internal branch topology against the independently authenticated owned English update, with paired executable data reads and direct-call targets preserved. The report retains exact instruction bytes from both files; annotation names are not evidence. This is static correspondence, not TU1.5 gameplay execution.

| TU1.1 | Authenticated TU1.5 |
|---|---|
| `2E0BA0` | `2E4690` |
| `300100` | `303BA0` |
| `3001D0` | `303C70` |
| `300260` | `303D00` |
| `300400` | `303EE0` |
| `300650` | `304130` |
| `300870` | `304350` |
| `309210` | `30CD00` |
| `309450` | `30CF40` |
| `31F1B0` | `322D20` |
| `31FEB0` | `323A10` |

The native raw-storage API remains in the explicit TU1.1 address namespace. A TU1.5 integration needs the verified address/data binding, including corresponding constant/global/vtable producers; it must not reinterpret TU1.1 trace addresses as TU1.5 addresses.
