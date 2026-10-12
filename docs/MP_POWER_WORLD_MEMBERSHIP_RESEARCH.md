# Actual power-module world membership and the broadphase boundary

The complete original `312DD0→31D730/31D250` world/island/body storage owner is now native. It uses the real allocator, preserves actual body/shape identities and links the source-created canister into the owning world and island banks. Nine original comparisons pass: 10,649,815 full source bytes and 228 ordered owning writes. Seven cases are complete positive membership/constructor/notification calls; two compare the original `315820` prefix through its precise next `3144D0` broadphase call.

This is a real membership segment that can be installed after the frozen body factory/grouping/slot/disposal providers. Live canister collision still requires the actual broadphase/pair/solver and scene delivery. The native outer insertion owner exposes that required broadphase service and stops there when it is absent; it never supplies a successful backend result, contact or substitute collider.

New production files are [physics_world_membership.hpp](../include/halo2/physics_world_membership.hpp), [physics_world_membership.cpp](../src/physics_world_membership.cpp) and [physics_world_membership_tests.cpp](../tests/physics_world_membership_tests.cpp). Only necessary CMake source/test registration is added. Scene, Simulation, renderer, runtime, releases and all previous freezes are untouched.

## Original execution and pins

[oracle_mp_power_world_insertion_1_1.py](../tools/oracle_mp_power_world_insertion_1_1.py) extends the immutable disposal trace read-only. The original selected startup/CRT/allocator/world/cache-fixup, authored `power_module` allocation, actual player join/spawn/PVS and ordinary/physics/dirty/new schedule remain identical. The original owning `1D4F30→1C7320→315820` calls execute at initial construction and at PVS rebuild. There are no new guest fields, successful callback outputs, positions, velocities, clocks, world admission, PVS bits, shapes, contacts or instruction patches.

TU1.1 XBE SHA-256 is `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`. Authenticated English u5 SHA-256 is `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`. Original bytes, not annotation labels or decompiler comments, own behavior.

The frozen ignored evidence is `analysis/multiplayer/sandbox-source/power-world-insertion/final/`:

- Original `measured/report.json` and independent byte-identical `repeat/report.json`, SHA-256 `6462c9fc1fac4ecdae51412d7b0ce63986e1f0f9ab85cc06087ea0141d470045`: 72 calls, 64 full states, 2,052 reads and 1,643 writes.
- Twenty-six complete source listings, 1,609 original rows; original executable-byte audit also covers all 1,562 observed read/write instruction sites against the prior actually executed listing.
- Source-only exported fixture `exported-owner/fixtures.tsv`, SHA-256 `4181c54d59992633e4b3cd31beaf1004b58dd018dbfb7429f369169472a703e8`, and identical exported repeat. No native output contributes to expected state.
- Complete authenticated u5 correspondence for all 26 represented owner bodies, SHA-256 `81fc63bb3eacd865d376e130d441f58e650b6ed55451692a60c8479f8f75ad79`.
- `audit.json`, native validation, schema, source/tool/doc copies and final manifest retain the exact pins and checks.

The preceding disposal manifest `77b9e7917a017bbe23f56b55bd233ca179202c93562750d923628a754f2660d8` and all 119 files remain intact; it transitively preserves the earlier grouping/wrapper/factory/slot/world proofs. Entire prior disposal observations and power-module schedule compare equal, not just selected outputs. Reproduce the oracle into a fresh directory; frozen paths reject changed bytes.

## Two actual paths, retained identities

The source-created component is `06200050`, canister datum `E26F0000`, body `0601A480`, world `06005470` from global `526234`. These are captured identities in the bounded original fixture, never addresses for a production callback to replay.

Initial construction uses source mode-7 motion `0601A5C0`, resident authored convex shape `8025CC40` and actual body byte `+40=1`. It follows the static membership branch and joins existing BSP/static island `0600CEE0`. That island's body bank grows through the real allocator. Body `+08` becomes world `06005470`, `+44` becomes island `0600CEE0`, and `+58` becomes body-bank index 2. No new dynamic island is constructed in this first path.

The actual PVS sequence removes/disposes that body, rebuilds it through the immutable grouping/factory/slot owners, then calls world insertion again. Mode-4 motion is `0601ABE0`, the body uses the real wrapper shape and byte `+40=0`. The source world separate-island byte `+23C=1` admits allocation of actual island `0601AE20`, size `B0`, allocation class `2D`. World island bank `+08=0601A470`, `+0C=1`, `+10=1`; the same retained body `0601A480` gets world `+08=06005470`, island `+44=0601AE20`, and index `+58=0`.

The native owner consumes these source-owned flags. It does not infer static/dynamic status from groundedness, velocity, tag name or a hand-selected physical mode.

## Complete native storage owner

`source_commit_physics_world_membership_1_1` represents full `312DD0`; original ECX is world and stack argument 0 is body. Original body `+08=world` is written first at `312DF4`.

For nonzero body byte `+40`, it appends to world `+2C` static island. Otherwise it consumes world byte `+23C`: a separate island is created when nonzero; the shared branch creates the first island only when world `+0C` count is zero, otherwise uses the first retained world island pointer. The shared positive branch is source-listed but not separately stimulated here.

Dynamic island allocation uses real adapter global `487638`, virtual `+10=148A10`, source allocator mode `480D98=2`, backend global `480DA0` with virtual `+10=22E950`. It delegates to the already verified complete small-block allocator and requires actual backing. A zero return or unprovided alternate allocator stops before writing an invented island.

After allocation, the source writes size word `+04=B0`, calls complete `31D730`, writes island `+24=world island count`, appends the body for the separate-island branch, grows the real world bank through `2DF780` only at count/capacity equality, and appends/increments the world bank at `312E76/79`. Shared construction uses its original corresponding `312EF7/FA` sites and later body append. Bank capacities retain the original high-bit ownership flags; no vector-size or borrowed snapshot replaces them.

`source_construct_physics_world_island_1_1` represents all original `31D730` stores. It preserves unowned header/padding bytes, writes primary vtable `41B610`, secondary vtable `41B604` after the original `419EBC` store, world `+20`, original byte flags and all exact bank descriptors. The body bank at `+3C/+40/+44` starts at inline `island+48`, count 0, capacity `80000001`; another inline bank starts at `island+64` with capacity `80000001`. Other owned empty capacities are `80000000`. These are original constructor constants and pointer expressions, not fabricated runtime defaults.

`source_append_physics_world_island_body_1_1` represents full `31D250`. It writes body `+44=island` first (`31D25C`), grows island body bank only at source equality, appends the body (`31D27E`), increments count (`31D281`), writes body `+58=count-1` (`31D28B`), then executes actual motion virtual `+14` and island virtual `+10` ownership. Both actual mode-7/mode-4 motion vtables resolve to complete `31F8A0`, whose original return is `80`; source island vtable resolves to `31C5A0`, adding that exact word to island `+14` at `31C5A9`. Other virtual targets remain hard boundaries. The word `80` is an original routine constant, not a guessed mass, radius, budget or threshold.

## Outer insertion, source backend and exact stop

`source_insert_physics_world_body_1_1` represents complete `315820` store/call ordering with required actual lower services:

1. Save world byte `+12C`, then write 1 (`315830`).
2. Body byte `+40` selects original static counter `world+134` decrement (`315847`) or dynamic `world+130` increment (`315858`). Write the previous counter word to body `+20` (`315861`). Source unsigned wrap is preserved. The actual initial static counter is `0FFFFFFD→0FFFFFFC`, body identity `0FFFFFFD`; PVS dynamic counter `10→11`, identity `10`. These unusual literal static values are not normalized or repaired.
3. Execute the complete native `312DD0` membership owner above.
4. Require complete original `3144D0` current-shape/backend broadphase owner. With no provider, native stops here after exactly the original preceding buffers and writes; it does not restore/finish as if insertion succeeded.
5. Only when that real owner completes, restore `+12C` at `315872`, increment body reference at `315878`, and call actual `31E610` world listeners.

`source_notify_physics_world_insertion_1_1` represents complete `31E610`. It traverses world `+48/+4C` backwards, calls each actual non-null listener's virtual `+0C` with retained body, then traverses backwards to remove NULL entries with ordered shift/count stores. Both actual original calls have source listener count zero; complete empty notification states compare. Positive virtual listeners require an explicit provider, not an empty successful callback.

The native tests use the actual original `3144D0` entry snapshot for the two outer prefix cases, including original world guard/counter/body identity/island/allocator backing and every preceding owning write. They do not import `3144D0` results or replay its return. Thus native complete membership is proved; successful full native broadphase insertion is deliberately not claimed.

The unchanged original `3144D0` itself completes both measured paths. Its observed real shape virtual `+18` targets are `303E30` for the initial resident convex and `2E2380` for the PVS wrapper. Both actual backend virtual `+04` calls resolve to `2E8A00`, backend record `060056D0`, vtable slot at `418C44`. Complete original static next-owner bodies are retained in `final/next-broadphase/`; no new backend ABI execution or native port is inferred from those listings.

The source backend delivers a batch of one pair to the owning manager path. Initial static insertion follows its `378EB0` delivery; dynamic rebuild follows `31D980` with actual pair array `480DD0`, count 1, then source sorting/scratch/manager owners. A pair is not proof of player blocking, impulse response or explosion. Complete solver iteration, contact callback admission/entity keys and Scene delivery remain separate required owners.

## Validation and 1.5 correspondence

Normal C++20 O2 with warnings as errors, ASan/UBSan O1 with all used body-state/contact dependencies instrumented and leak detection, and production-library CTest pass. Fixture expectations come only from unchanged original reads/writes/full snapshots. All 228 store instruction/address/before/written packets are exact. Internal ABI stack and FS exception-registration plumbing are retained in original reports and excluded from native storage comparisons; there is no temporary-pose or native-derived expected buffer.

All 26 complete body/internal-branch-index correspondences are retained, with original absolute memory operands, direct-call pairs and actual constructor-owned virtual anchors. Principal mappings are:

| TU1.1 | Authenticated u5 |
| --- | --- |
| `1D4F30`, `1C7320`, `315820` | `1D97F0`, `1CC560`, `319370` |
| `312DD0`, `31D730`, `31D250` | `316950`, `3212C0`, `320DE0` |
| `31F8A0`, `31C5A0`, `31E610` | `323420`, `320130`, `3221A0` |
| `3144D0`, `31D980` | `318020`, `321510` |
| `312360`, `3123A0` | `315E50`, `315E90` |
| `148A10`, `22E950`, `2DF780` | `149DA0`, `233250`, `2E3250` |

This is full static correspondence, not u5 execution, Xbox hardware parity or automatic source-address relocation of the native TU1.1 API. The source clock/real compatibility backing, optional audio/resource setup, selected startup subset and one original ordinary schedule remain explicit inherited boundaries. No continuous frame progression, player-versus-crate positive collision, damage, destruction, explosion, readiness or live rendered state is fabricated.

The next production-capable slice is actual `3144D0` with wrapper bounds `2E2380` and backend `2E8A00`/its real index, packed-bound and pair producers. The initial convex bounds owner already exists natively, but neither its result nor a generic AABB may substitute for the backend and pair state. The new retained world/island/body owner can feed that real continuation; Scene integration should follow only when current backend/solver ownership is available.
