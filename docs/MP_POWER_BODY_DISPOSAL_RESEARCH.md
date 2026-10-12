# Power-module body disposal and retained rebuild storage

The complete original `1D3BB0` lifecycle owner is now available as `source_dispose_physics_component_bodies_1_1`. It consumes retained component/body/shape/property/allocator storage, retires the actual body and resets the component for rebuild. Five original comparisons pass with 5,916,704 complete backing bytes, 90 ordered owning writes and ten complete allocator-call packets. This closes the measured mode-7 teardown before the existing grouping owner rebuilds the same crate. World removal/insertion, player-versus-crate collision, damage and explosion remain separate owners.

The new implementation is [physics_body_disposal.hpp](../include/halo2/physics_body_disposal.hpp), [physics_body_disposal.cpp](../src/physics_body_disposal.cpp) and [physics_body_disposal_tests.cpp](../tests/physics_body_disposal_tests.cpp). CMake changes register only this source and its original differential test. Scene, Simulation, runtime, renderer and prior source/native freezes are unchanged.

## Authority and reproduction

Original instructions are from TU1.1 `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`. The authenticated English u5 image has SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`. Annotation names and decompiler comments are not behavioral authority.

[oracle_mp_power_body_disposal_1_1.py](../tools/oracle_mp_power_body_disposal_1_1.py) subclasses the frozen grouping trace read-only. It independently observes disposal calls/reads/writes/backing buffers during the same original selected initialization, authored `power_module` factory, source-created player, actual PVS and ordinary/physics/dirty/new schedule. It supplies no new guest fields, returned admission, contacts, pose, clock, allocation result or instruction patch. Its complete inherited grouping calls/reads/writes/states/math/factory packets and the original power-module schedule compare equal to their prior freezes.

The ignored frozen evidence is `analysis/multiplayer/sandbox-source/power-body-disposal/final/`:

- `measured/report.json` and byte-identical `repeat/report.json`: SHA-256 `d43e2d610e0075209419aed5b38e023f9f03280bd8af80e458e92c620839e513`; 29 calls, 18 full states, 264 reads and 198 writes.
- Twenty complete original instruction listings, 796 rows; all bytes match the executable. The byte audit also identifies all 290 observed read/write instruction sites in the prior actually executed source listing.
- `exported-owner/fixtures.tsv` and byte-identical repeat: SHA-256 `cfa8d3d5f1754b1eebed87fc8ec186046677b184e4f0a7f8c33ce7fb933a8799`. Expected output is exported only from original execution.
- `later-correspondence/complete-report.json`: SHA-256 `f157066a31e87425342b240a6c82fb2e1a2d1f5bf956bbef288ad6957199126f`; complete body/branch topology pairing for 21 routines, with paired raw memory operands, direct calls and actual constructor-owned virtual slots.
- `audit.json`, `native-validation.json`, source/tool/doc copies and `manifest.json` retain the final pins and validation commands.

Reproduce in a new output directory with `.tools/python/bin/python tools/oracle_mp_power_body_disposal_1_1.py --output <new-directory>/report.json`. The prior grouping manifest `8d14894ae02d9486f89b882eb855b69d052fe5b9665055a9b0d95f2d45b05d7d` and all 206 files must remain intact. Existing frozen paths reject different bytes. The original real compatibility-worker/backing prerequisites are inherited unchanged; this is not complete title or platform startup.

## Actual lifetime and ABI

The owning order is the frozen source `1C70D0` PVS transition: `1D4C50` removes the existing world body, `1D3BB0` disposes it, `1D9330` rebuilds through the frozen grouping/wrapper/factory/slot owners, and `1D4F30` reinserts the rebuilt body. The new oracle observes the original `1D3BB0` call from `1C7120`, return `1C7125`, component `06200050`. It does not invoke disposal on an invented body or modify PVS membership.

The actual component belongs to the source-created power-module datum `E26F0000`, object body `82D87FC0`. Before disposal: component flags `+04=00000A01`, body bank `+70=0601A6C0`, count `+74=1`, capacity `+78=1`; constraint count `+80=0`; optional references `+98/+9C=NULL`. Its mode-7 body is `0601A480`, motion `0601A5C0`, referenced auxiliary `0601A9E0`, exact resident convex shape `8025CC40`. These addresses describe the oracle fixture; production must use actual retained allocator-owned identities.

Original `1D3BB0` takes the component at stack argument 0 and returns it in EAX, `RET 4`. Its original 0x18-byte frame is captured after `1D3BC5`. `318900` output arguments resolve to `021FCEDC` and `021FCEE0`, corresponding to this frame's `+10/+14`. The native scratch interface requires two real writable output words; it does not make their pointer values authoritative inventory or world identities.

## Disposal order and exact stores

The complete original source listing is `final/measured/001d3bb0.asm`; children have their own complete listings. The native implementation keeps the following order:

1. Constraint rows are 0x0C bytes at component `+7C`, count `+80`, referenced object at row `+08`. The source decrements reference `+06`, then checks residency or calls its actual virtual destructor with flag 1, then zeros row `+08` (`1D3BD9` through `1D3C05`). Count zero is the executed case; positive constraints are listed, with unresolved actual virtual owners required.
2. Body rows are 0x60 bytes at component `+70`, count `+74`; body at slot `+40`. Each existing property `2001` is removed with original `318900`, then added as `FFFFFFFF` through complete original `318A90`. Property `2002` follows. Existing properties are searched before removal; an absent key still receives the original add operation. No existing-key overwrite is substituted.
3. `318900` decreases count (`318953`), shifts each following key/value pair (`31896A/31896F`) and writes the removed value (`31897E`); the listed absent-key branch writes zero (`318933`). The retained actual property bank has count/capacity 4. The two ordinary removals each cause four owning writes and preserve trailing untouched storage.
4. The body reference at `+06` decreases at `1D3CBD`. On zero, the original signed residency-range check uses `80061000`, byte global `54F248` and dword global `54F26C`. Both globals are observed zero; the actual body is outside the resident range, so its real vtable slot 0 is `317AD0`. This is a signed 32-bit range calculation, not an unsigned address test. Native `source_physics_body_reference_is_resident_1_1` also represents complete `27F9F0` and compares its actual AL.
5. `317AD0` runs the complete rigid-body destruction below and frees the body through its real allocator when flag bit 0 is set. Only after it returns does `1D3D13` zero slot `+40`. Slot index bank `+48`, count `+4C`, capacity `+50` are preserved in the actual inline-capacity `80000004` case. The listed nonnegative capacity branch frees that bank through the required allocator.
6. Component stores remain ordered: flags `+04=0` at `1D3D57`, byte `+1C=0` at `1D3D5A`, selectors `+18/+19=FF` at `1D3D5D/61`, body count `+74=0` at `1D3D65`, constraint count `+80=0` at `1D3D68`. Body/constraint bank pointers and capacities remain retained. Non-null optional `+98/+9C` references are released and cleared afterwards; neither is present in the measured case.

At return, body bank `0601A6C0` and capacity 1 survive, its slot body pointer is NULL, component count is zero and both selectors are NONE. The same original PVS root subsequently rebuilds using those retained bank bytes; it does not allocate a fabricated replacement slot or copy a fixture address into production.

## Complete body, motion, reference and free owners

The real virtual chain is:

`317AD0 → 317970 → {motion kind, motion destructor, 3124B0 → 31C1C0, auxiliary reference/destructor, 311910 → shape reference, property free, DD970} → body free`.

`317970` first writes body vtable `41A950` (`31798E`). It calls motion vtable `+18` twice; the actual `31B250` returns kind 7 both times. The `3126E0` owner reads motion `+C0`; it is NULL, so no saved-body destruction is supplied. Motion slot 0 is actual `320D20 → 320D50 → 2E00F0`; this stores vtable `459D94`, then really frees the motion.

`3124B0` stores entity vtable `419EA4`, invokes complete `31C1C0` listener ordering, releases body `+54`, considers dynamic bank frees, then calls `311910`. `31C1C0` enumerates `+7C/+80` backwards, then `+88/+8C` and `+94/+98` forwards. All three actual listener counts are zero. Positive callback slots require an actual provider; there is no successful empty listener default.

Body `+54` holds actual auxiliary `0601A9E0`, reference 1. Its zero-reference residency check returns false. Slot 0 of its source-constructed vtable `45CAD0` is `1D53C0`, which writes `459D98`, then `459D94`, then frees the auxiliary. The high header reference-flag branch is unexecuted. Native stops at required original `2DD7F0` if it is needed; that service cannot infer or supply flag-manager success.

`311910` writes base vtable `419DD0`, decrements exact resident shape reference `+06` from `0081` to `0080` (`311943`), frees the real property bank and runs `DD970`, whose actual stores are `459D98` then `459D94`. Shape-zero virtual destruction is unexecuted and retains its required source owner boundary.

The actual frees through adapter `480DB0`, virtual `+14=148A70`, backend `06016040`, backend virtual `+14=22EB20`, are:

| Call instruction | Retained block | Byte count | Original allocation class |
| --- | --- | --- | --- |
| `320D3F` | motion `0601A5C0` | `D0` | `29` |
| `1D5400` | auxiliary `0601A9E0` | `70` | `26` |
| `3125B8` | body `+70` bank `0601A478` | `04` | `12` |
| `31198E` | property bank `0601A8E0` | `20` | `12` |
| `317AEF` | body `0601A480` | `A0` | `28` |

Native free operations validate real adapter/backend virtual identities and the original signed backend range before using the existing complete `22EB20` owner. It changes the original class free count, live allocation count, block next pointer and freelist head at `22EB58/63/6E/70`. There is no address-replay free callback. Missing alternate allocators or virtual owners stop after preceding source stores. A read-only observer retains the complete allocator instruction/adapter/block/byte-count/class packet.

## Native comparison scope and limits

The five independent calls are whole `1D3BB0`, its actual complete `317AD0`, two `318900` removals and actual `27F9F0`. The test compares complete before/after retained backing, every owning store's instruction/address/written bytes and retained store-before bytes, return component/body/output/AL and ten complete original allocator packets. Internal Xbox stack argument pushes/exception-registration FS plumbing are not native storage owners. The two semantic `318900` output words remain compared; only their temporary stack store-before bytes are excluded after source ABI stack reuse. All raw original frames, packet before/after bytes and excluded ABI/FS stores remain in the report.

Normal C++20 O2 with warnings as errors and strict scalar FP, ASan/UBSan O1 with leak detection, and production-library CTest pass. The test also removes actual positive-case allocator identity backing and verifies a hard stop rather than a successful free. No new positive contact, destructor class, constraint or reference-flag stimulus is manufactured.

Full original instructions represent the unexecuted branches, but their positive lower owners are not claimed as executed coverage. Motion kind other than the actual `31B250`/7, saved-motion body destruction, positive entity listeners, constraint classes, zero-shape-reference destruction and reference-flag manager need real retained providers. Source-clock, real Xbox reciprocal hardware, full title/session initialization and complete natural frame scheduling boundaries inherited from the prior suites remain open. No continuous settling, player blocking or explosion is proved by disposal.

## Authenticated 1.5 correspondence

The complete instruction and internal branch-index comparisons, including genuine virtual-constructor slots, establish:

| TU1.1 owner | Authenticated u5 owner |
| --- | --- |
| `1D3BB0` | `1D8470` |
| `318900`, `318A90` | `31C450`, `31C5E0` |
| `317AD0`, `317970` | `31B610`, `31B4B0` |
| `320D20`, `320D50`, `2E00F0` | `324870`, `3248A0`, `2F7A10` |
| `3126E0`, `31B250` | `3161F0`, `31EDE0` |
| `3124B0`, `31C1C0`, `311910`, `DD970` | `315FA0`, `31FD50`, `315400`, `B4150` |
| `1D53C0`, source auxiliary constructor `1D5210` | `1D9C80`, `1D9AD0` |
| `27F9F0`, `148A70`, `22EB20`, `2DF780`, `2DD7F0` | `283450`, `149E00`, `233420`, `2E3250`, `2E12A0` |

Actual paired constructor vtable stores identify body slot `41A950→41E3C0`, motion `41B7F0→41F260`, auxiliary `45CAD0→461970`; the destructor wrappers cannot be selected uniquely from generic prefix text alone. Original absolute operands and direct-call addresses remain recorded separately. This is complete static correspondence, not a u5 execution or automatic relocation of the TU1.1 native source-address API.

## Smallest remaining true world bridge

The next complete owner should be `1D4F30` with `1C7320→315820` and their actual lower world storage producers. Complete original static bodies are saved separately in `final/next-world/`; this addition does not claim a new execution or world admission.

`1D4F30` uses retained component/body slots, source object header/body, global source world `526234`, contact listener `526538`, object `+C1` admission byte and actual source model/material scaling. It inserts the listener with `312360` and, when its own flags admit, the offset listener with `3123A0`. It calls `1C7320` with the real body and object admission bit, then source linear/angular velocity delivery `1D47C0/1D4840`, creates real 0x70-byte auxiliary through `1D5210`, installs body `+54` reference ownership, and updates component flags at `1D51FA`. Its constraints use the actual world `313800` owner; actual constraint count remains zero.

`1C7320` uses source capacity counter `486558`; at threshold `352` it consumes actual property `2001` through `1C8AF0`, increments the counter (`1C736A`) and calls `315820` on global world. Its later admission condition may call `3121F0`; `27F190` is the real deferred-removal owner, not world insertion itself.

`315820` temporarily sets world byte `+12C`, changes static/dynamic count (`+134` or `+130`), writes body `+20` identity, calls actual `312DD0` world/island bank insertion, then `3144D0` broadphase/backend insertion, restores the world byte, increments body reference and delivers `31E610` listeners. `312DD0` owns real allocation/island/bank growth; `3144D0` consumes actual body/motion kinds, shape virtual bounds, broadphase backend and pair state. These owners must be ported with real world/island/shape/backend storage, not a callback supplying captured body/world addresses or a successful AABB surrogate.

The installed factory/grouping/disposal pieces can now construct, retain, commit and retire the measured exact convex body in actual CPU-owned allocator storage. They cannot make `SourcePropScene` collidable until the genuine world/broadphase/solver owners and scene delivery exist. Player-versus-crate collision and incoming canister damage/destruction remain explicit next admitted scenarios; no borrowed contact or approximate collider is introduced here.
