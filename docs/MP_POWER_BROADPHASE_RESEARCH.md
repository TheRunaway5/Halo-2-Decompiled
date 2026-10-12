# Power-module current-shape bounds and real broadphase pairs

The native source owner now computes the actual current convex/wrapper bounds and inserts them through complete original `2E8A00`, using retained sorted axis endpoints, signed-word packed-index repair, the real scratch allocator and actual pair construction. Forty-one original comparisons pass: **48,518,387 full source bytes and 966 ordered owning writes**. Thirty-nine are complete positive lower calls; two compare the canister's `3144D0` orchestration through its exact next pair-manager entry, with no successful solver callback supplied.

This closes the broadphase bank/pair segment after the frozen canister body/shape/grouping/slot/world membership owners. It does not yet make the screenshot's canister block the player, explode or take damage. The actual pair manager, solver/contact delivery, current-world initialization and Scene bridge remain required owners. A generated pair is not a contact, impulse, incoming-damage event or collider installed in the live runtime.

New production files are [physics_broadphase.hpp](../include/halo2/physics_broadphase.hpp), [physics_broadphase.cpp](../src/physics_broadphase.cpp) and [physics_broadphase_tests.cpp](../tests/physics_broadphase_tests.cpp), with localized CMake registration. Root also authorized the one-line live convex-bounds correction described below. Scene, runtime, Simulation, rendering, releases and all earlier frozen archives remain untouched.

## Unchanged original execution

[oracle_mp_power_broadphase_1_1.py](../tools/oracle_mp_power_broadphase_1_1.py) extends the immutable world-insertion observer read-only. It runs the same real selected startup/CRT/scratch allocator/world/cache fixup, original authored Lockout power-module constructor, actual player join/spawn, source PVS and ordinary/physics/dirty/new schedule. It introduces no guest fields, positions, velocities, contacts, clocks, PVS masks, shapes, successes, returned pairs or instruction patches.

The entire preceding world call/read/write/state report and complete power-module schedule compare equal. The original source captures four real backend calls: canister and player insertion at construction, then their source PVS rebuilds. Each constructs one pair with the already source-created BSP body's handle. The two outer native cases cover the canister's initial static path and actual mode-4 PVS dynamic path. The player backend cases independently exercise the same complete bank/pair owner with their real source-produced bound inputs; they do not prove a native biped shape callback.

Authority is the unchanged TU1.1 XBE SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` and authenticated English u5 SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`. Labels and pseudocode are navigation only.

Frozen ignored evidence is `analysis/multiplayer/sandbox-source/power-broadphase/final/`:

- `measured/report.json` and independent byte-identical `repeat/report.json`, SHA-256 **`b4abeb6fd0e2f50a832d064204d8d69bd97f5baae57123eb9911ed8fbdc167e2`**: 92 calls, 74 full states, 2,487 reads and 2,069 writes.
- Twenty-four complete source listings, 1,787 instruction rows. Every row is byte-audited; all 1,111 observed read/write instruction sites match the immutable prior actually executed listing.
- Source-only `exported-owner/fixtures.tsv` and identical exported repeat, SHA-256 **`b5f5bc8e29368234f656b2ca83ce54a27edd13124ca02a92467d3492243b7efc`**. No native code/output creates expected data.
- Full authenticated correspondence for 25 complete bodies, SHA-256 **`27e0ca004227a19010adb23f1fa8f455cbf54787d328a05ab9ccf9cf3a08f553`**, including separate absolute operand and direct-call evidence, and the actual constructed wrapper's virtual `+18` anchor.
- Audit, native validation, schema, archived source/tool/doc copies, exact CMake additions and final manifest.

The prior world manifest `a29afe0dfc8a6957e61b217cf132ec99b7d553167a4f29161cefe1b342716625` and all **135 files** remain intact; their transitive preceding freezes remain intact. Reproduction should use a fresh output directory, since frozen paths reject changed bytes.

## Complete bounds and the newly covered margin

The actual canister's initial resident authored convex is `8025CC40`, virtual `+18=303E30`. Its current transform is source body `+14` plus `20`. PVS rebuild uses wrapper `0601AD20`, whose underlying resident shape is read from `+30`; virtual `+18=2E2380` delegates the same transform, margin and output to that convex owner.

Original `314514–314533` reads `world+CC→+08`, multiplies by executable constant `46502C`, and spills the margin to binary32 before the virtual call. Both actual canister calls produce **`3C86594B`** from actual field `3D06594B`. Original `303E34/38/44` then loads that margin, adds shape radius `+0C` (actual zero here) and spills before `1D1C00`. Every resulting bound vector/write is captured and compared; the two direct convex cases and the wrapper case cover this nonzero margin independently of outer orchestration.

This exposed a live native error in [physics_body_factory.cpp](../src/physics_body_factory.cpp): `f(spill(...))` passed a floating result to `f(uint32_t)`, truncating the small margin to integer zero. Root authorized changing the expression to `spill(...)` directly. Original source and every prior archive stay unchanged. The new positive fixtures reject that prior expression at the first `1D1C66` extent store; the correction passes the complete original bounds and subsequent quantized/pair state. Earlier zero-margin constructor cases remain valid and are also rechecked.

Complete `2E2380` consumes wrapper `+34` body. If body and body `+08` world exist, it multiplies all four actual motion `body+3C→+40` words by source scalar `world+150`. Otherwise it uses zero. It expands maximum using `MAXPS(max+expansion,max)` at `2E23E8`, then minimum using `MINPS(min+expansion,min)` at `2E23F5`, preserving source operand order, signed-zero and NaN selection semantics. The actual canister PVS case follows the present-body/world branch; no velocity or elapsed value is assigned. The player's underlying wrapper shape is a different source virtual owner and remains unported by this module.

## Exact retained broadphase storage

`source_insert_physics_broadphase_body_1_1` represents complete `2E8A00`; ECX is actual backend `world+C4` (`060056D0` here), arguments are body handle **`body+1C`**, actual 32-byte shape bounds and the original 20-byte pair descriptor. This handle addresses the source retained packed-body index; it is not an object/tag datum or a host pointer to replay.

| Source backend field | Owning input/output |
| --- | --- |
| `+10`, `+20`, `+30` | Four-lane min/max offsets and scales |
| `+40/+44/+48` | Packed body-record bank, count, capacity; record stride `10` |
| `+4C/+50/+54` | X endpoint bank/count/capacity; endpoint stride 4 |
| `+58/+5C/+60`, `+64/+68/+6C` | Y and Z endpoint banks |
| `+70/+74/+78` | Optional secondary bucket mode, shift and bank |

For minima and maxima separately, the source performs `ADDPS` offset, `MULPS` scale, `MINPS` imported maximum `418C20`, `MAXPS` imported minimum `418C10`, and `ADDPS` bias `418C00`. It shifts each resulting original float word right by 7; low 16-bit minima are masked `FFFE`, maxima OR 1. No decimal threshold, generic AABB quantizer or native-derived table is substituted.

At original count/capacity equality, real body banks grow through existing complete `2DF780→148A10→22E950`, with no returned-address table. Count increments at `2E8B2E`. Complete `2E7500` inserts each min/max endpoint in source order, moving full four-byte entries and writing original 16-bit body IDs and output indices. Equal maximum values move while `maximum<=existing`; equal minimum values stop because its comparison is strict `<`. The new packed record stores Y minima/maxima at offsets `0/4`, Z at `2/6`, X at `8/A`.

Complete `2E6240` reproduces the actual MMX **signed** `PCMPGTW`, wrapping word sums/subtractions, and 64-bit stores `2E631C/6326` for every prior body record. It repairs the indices after the two new endpoints; the record's `+C` body handle remains unchanged. This is not an unsigned geometry comparison or an approximate rebasing operation. Native retained-memory behavior is represented; MMX/x87 tag/status hardware effects are not.

The source writes new record `+C=handle` (`2E8BAD`), then `*handle=original index` (`2E8BB0`). The overlap bitset is allocated by actual `2E7DA0` for `(signed count>>5)+4` words, aligned to 16 bytes; the source zeroes `(signed count>>7)+1` vectors. The actual existing adapter (`487638`) supplies its current `+08` pointer and `+0C` available bytes. Native code advances/rewinds those actual fields with the original stores, rather than allocating a guessed array or importing recorded addresses. Alternate scratch virtual `+24/+28` branches require real providers and have no success default.

X endpoint traversal toggles bits in the original ordering, then the packed Y/Z overlap subtraction and `80008000` mask decide actual candidates. Even retained other-handle values append a real pair through exact stores `2E8E45/48/4F`; odd values route to the source secondary bucket bank. The secondary `+70` path and odd-coded candidate branch are source-listed and implemented with their original fields/order, but unexecuted in these four cases. All four actual `+70` fields are zero. Axis/body/pair growth branches that these cases do not enter are not reported as new positive coverage; their existing native allocator/bank owners remain required.

All four actual pair arrays are `480DD0`, count 1. Canister pairs contain handles `0601A49C` and `0600526C`; player pairs contain `0601A53C` and `0600526C`. The latter is the source BSP body's handle (`06005250+1C`). These values are evidence identities, not production constants or callback returns.

## Outer insertion and real next manager boundary

`source_insert_physics_world_broadphase_1_1` represents complete `3144D0` ordering using explicit caller-owned aligned scratch. It constructs real `313C90` pair storage with capacity from `world+234`, computes original x87 margin/current-shape bounds, verifies actual backend virtual `+04=2E8A00`, then runs the complete native bank/pair owner. Shape/virtual identities outside represented owners fail at their actual required boundary.

The actual source pair count is positive in both canister paths:

- Initial static body byte `+40=1`: original writes temporary list vtable `419F30`, inline pointer `frame+60`, count 0, capacity `80000004`, body handle owner `body+C`, then installs list at manager `world+138→+24`. Actual next `378EB0` ECX is `06007F80`; stack is pair bank `480DD0`, count 1, 0, 0, optional source `world+D0+8=060053D8`. Native stops exactly here when this genuine service is absent. It does not reset the list or release its pair bank as if the manager succeeded.
- Actual PVS dynamic body byte `+40=0`: original next `31D980` ECX is the real retained island `0601AE20`; stack is pair bank `480DD0`, count 1, 0, 0. Native stops exactly at that required island manager. The unchanged source continues sorting (`379140`), real scratch (`313EF0`), then `378EB0`; these source calls and stores are captured, but their native solver delivery is not supplied here.

If real manager providers complete, the outer owner follows the original restoration, per-island delivery/list retirement and `313E50` pair-bank release. That orchestration is implemented, but successful positive native solver-manager completion is **not** claimed by these boundary fixtures. Typed services accept the actual memory, target records, exact arguments and store observer; none can quietly default to success or return a captured pair/contact.

For installation, compose the frozen native body/shape/factory/grouping/slot and world-membership owners with these new shape/backend owners over genuine allocator-owned records. Actual world/backend initialization and the real BSP record/endpoint banks must already exist. The source fixture executes that initialization; the native Scene bridge may not populate it from these snapshots. The smallest next live gate is actual `378EB0` pair manager admission and `31D980` island delivery with their real virtual dispatch/contact construction. The new unchanged reads identify the concrete positive path: `378FC3/FCF` resolves actual filter record `060053D8`, vtable `45C760`, virtual target **`2E5F50`**; its actual output byte read at `378FD1` is `01` in all four source calls. The subsequent `378FE7/FEA` dispatch resolves **`314020`** through static temporary-list slot `419F34`, or **`31D360`** through dynamic island-list slot `41B608`. These are real source admissions and next owning callbacks, not native successful defaults. Their complete native providers, followed by solver/world/listener/Scene ownership, remain open. Player-versus-canister positive collision and damage/destruction remain unproved.

## Validation and authenticated 1.5 scope

Normal C++20 O2 with warnings as errors and strict source FP contraction, ASan/UBSan O1 with **all used broadphase/factory/body-state/contact dependencies instrumented** and leak detection, and the production-library CTest pass. The separate production factory test also retains its earlier constructor coverage. There is no rendered/GPU test or live world collision claim.

The exporter includes full retained input/after buffers and every ordered owning site/address/before/written packet. Internal ABI/FS writes remain in the source report. Semantic stack outputs (bounds, pair/bitset descriptors and the temporary static list) compare exact ordered site/address/written bytes. Their source before bytes are retained but excluded from native store-before comparisons because the original reuses that temporary frame for earlier quantization/ABI values. Full retained allocator, endpoints, packed records, handles, pair data and output state compare without that exclusion.

All 25 represented full routine/internal-branch correspondences match authenticated u5, with separately retained absolute memory operands and direct calls. The wrapper virtual slot is resolved from its paired actual constructor, not an assumed address offset.

| TU1.1 owner | Authenticated u5 |
| --- | --- |
| `3144D0`, `303E30`, `2E2380` | `318020`, `307E30`, `2E5E80` |
| `2E8A00`, `2E7500`, `2E6240` | `2EC4B0`, `2EAFB0`, `2E9CF0` |
| `2E7DA0`, `2E8090` | `2EBA20`, `2EB850` |
| `313C90`, `313E50` | `3177E0`, `317870` |

Quantizer words are bit-identical: `418C00/10/20→41C680/90/A0`. Full mappings and every exact imported word are in `later-correspondence/complete-report.json`. This is complete static correspondence, **not u5 gameplay execution**, automatic relocation of the TU1.1 address-based memory API, Xbox reciprocal/hardware parity, a complete title frame or continuous settling. No new elapsed/current-pose/contact/admission inputs are fabricated.
