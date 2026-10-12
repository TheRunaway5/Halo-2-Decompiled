# Source-owned power-module motion-shape wrapper

The native owner now reproduces the actual Lockout `power_module` PVS rebuild's `1DACF0 → 1DADF0 → 1DAD80` path, including its resident convex-shape calculation, real small-block allocation, underlying-shape references, retained body identity and configuration stores. This is a separate immutable extension of the [body slot/world freeze](MP_POWER_BODY_SLOT_WORLD_RESEARCH.md). It does not install a live canister collider, solver callback, player contact, damage, destruction or explosion.

The implementation is `include/halo2/physics_shape_wrapper.hpp`, `src/physics_shape_wrapper.cpp` and `tests/physics_shape_wrapper_tests.cpp`. It requires actual retained `SourcePhysicsContactMemory` backing and the existing allocator. Missing backing, virtual owners and unsupported positive shape/disposal branches fail explicitly. It has no captured-address allocation callback, successful Havok default, authored sphere or AABB substitute.

## Authority and execution

TU1.1 original: `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.

Authenticated TU1.5 original: `/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe`, SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

`tools/oracle_mp_power_body_prepare_1_1.py` inherits the immutable slot/world trace, verifies its 158-file manifest and executes the same original selected phmo initialization, complete authored crate constructor, actual source player join/spawn/cluster/PVS and six recorded world phases. All preceding slot/world records and the complete preceding power schedule compare equal. It observes actual `1D95E0` roots belonging to the retained crate component; it supplies no new pose, body, shape, allocation, admission, activation, contact, clock, velocity or result. The entire resident 1,144-byte phmo image, including the original shape8 vertices and planes, is measured before/after every retained state. This includes the source-initialized resident representation, rather than assuming serialized discriminator4 already describes a constructed shape8.

The inherited context, resource residency, options, optional audio/attachment services and original 12-byte clock fixture remain explicitly bounded. In particular, reading that unchanged clock does not close its natural next-frame producer. No tick increment, repeated physics step, complete title startup or complete frame is added.

The final directory is `analysis/multiplayer/sandbox-source/power-body-prepare/final/`. Original measurement and independent repeat are byte-identical: 108 calls, 48 full states, 1,292 reads, 1,279 stores and 41 arithmetic snapshots. Whole original instruction bytes, preceding pins and the independent fixture repeat are audited. The exported native fixture contains five complete selected calls, 5,918,954 retained bytes and 207 ordered owning stores. Normal, ASan/UBSan and production-library checks are recorded separately in that directory; fixture expectations come only from original execution.

Three earlier observer drafts are preserved outside `final`. The first mislabeled the `184510` ECX/EDX scalar outputs; the final record uses literal `ecx_output`/`edx_output` and source-proven argument ordering. Later additions capture constructor input packets, all eight XMM registers and the complete resident model. No original instructions or guest writes were changed by these observer corrections.

## Complete measured owner contract

The real crate identity is object `E26F0000`, component `06200050`, body `0601A480`. Its initial mode7 body retains resident convex shape `8025CC40`. The PVS rebuild produces a new mode4 lifetime of that body and an allocated wrapper `0601AD20`. Reused addresses do not make their lifetimes interchangeable.

`1DACF0` receives body in EDI, then component and scalar word on the stack (`ret 8`). It tests byte `486577` and reads the actual motion virtual `+18`. Mode7 returns the same body with no writes. The measured mode4 path calls `1DADF0`, decrements the previous body shape reference at `1DAD2B`, writes body `+0C` at `1DAD67` and sets component `+04` bit `10` at `1DAD71`. It retains body identity. The original zero-reference deferred-delete/destructor branch is listed but unexecuted here; native code stops at that exact boundary after the decrement. Motion mode6 is source-listed; its positive virtual owner is not supplied.

`1DADF0` receives component in ECX and shape/body/caller scalar as its three arguments (`ret 0C`). It first calls `184510`, reads signed clock `*[518444]+02`, performs the exact SSE sequence and dispatches the actual shape virtual `+14`. This fixture admits only the real type8 convex path. Compound2/10/11, proxy21, existing wrapped17 and their recursive/auxiliary allocation branches are listed-only, with no successful replacement provider.

`184510` builds a temporary 0x40 matrix from the source 0x34 basis pointed to by global `46FC70`. It preserves original store order, fills each fourth lane with zero, and places the basis translation in the fourth vector. ECX is initialized to imported negative maximum float (`447020`, `FF7FFFFF`); EDX to positive maximum float (`452044`, `7F7FFFFF`). It calls `184800(shape, 0, matrix, EDX, ECX)`. Thus EDX is the minimum output and ECX is the maximum output. They are caller scratch, not contact points or persistent bounds.

The actual `184800` dispatch subtracts4 from the shape type, reads byte table `1850C4 + (8−4)` and then address table `1850A8 + 4*4`, admitting original branch `184E6B`. The branch's scalar span is twice the minimum of source shape `+10/+14/+18`, with the original COMISS/JA/JBE order. It writes the caller minimum at `184F5D` and maximum at `184F70`. Its separate XMM0 return is `sqrt((Y²+X²)+Z²)` for shape `+10`, plus the same exact-order length for shape `+20 + matrix translation`. That returned length is preserved and independently compared; the following `1DADF0` scalar consumes ECX's maximum span, not this length. This calculation does not replace the original vertices/planes or generate a collider.

The measured maximum span is `3E83126D`; `184510` returns XMM0 low word `3EE54845`. Original `1DADF0` computes, with a binary32 result after every SSE operation:

```
v = maximum_span * caller_scalar
v = v * [46502C]
v = v + [486564]
v = v * float(sign_extend(clock_word_at_+02))
```

The actual caller scalar from `1D9843` is `3F59999A`, imported `46502C` is `3F000000`, measured `486564` is `3D06594B`, and the unchanged inherited clock word is30. The actual result is `4087EF9D`. Neither this scalar nor that tick rate is hardcoded into the owner.

`1DAD80` receives underlying shape in EDI and body/scalar on the stack (`ret 8`). It allocates an actual0x40-byte block using adapter `487638` virtual `+10 → 148A10`, source allocator mode2 and real backend virtual `+10 → 22E950`. The native implementation composes the existing measured free-list/page allocator; an absent or exhausted alternate allocator remains unavailable. It writes size word40 at `1DADA0` and calls `2E2530(shape, body, 41200000)`.

`2E2530` constructs the wrapper: imported `46938C` at `+0C`, reference1 at `+06`, flags0 at `+08`, source vtable `418930`, underlying shape at `+30`, zeros only `+10..+1F`, underlying reference increment, original x87 square at `+20`, body at `+34`, initial radius word `41200000` at `+24`, word `40A00000` at `+28`, original x87 squared difference at `+2C`, and finally underlying shape `+0C` copied to wrapper `+0C`. The remaining `+38..+3F` and padding are retained; the allocator block is not zero-filled.

`2E25A0 → 2E5720 → 2E5750 → 2E5760` constructs a real0x14-byte configuration packet: `+00=0`, `+04=[46938C]`, `+08=0`, `+0C=41A00000`, `+10=40A00000`. Caller `1DAD80` then stores measured scalar at `+0C`, shape at `+08`, and `scalar * [465090]` at `+10`, where the imported word is `3E800000`. `2E2260 → 2E5780` copies configuration `+00` to wrapper `+08`, scalar to wrapper `+24`, its original x87 square to `+20`, configuration `+10` to wrapper `+28`, and the original x87 squared difference to `+2C`. Configuration `+08` is retained as written even though this admitted copy path does not consume it.

The actual matrix translation and later configuration packet share address `021F7D00`. `SourcePhysicsShapeWrapperScratch` exposes their independently measured pointers and preserves this alias. It does not allocate fictitious matrices or separate the aliased source storage. The real native memory adapter must own scratch storage and retained allocator/model/body/component backing.

## Arithmetic and correspondence limits

Actual source FPCW is `027F`; actual MXCSR is `1F80`. The owner uses literal SSE operation order and native x87 loads, subtraction, multiplication and binary32 spills. Masked floating-point status effects and Xbox instruction/hardware estimates are not represented by this API. Unmasked exception delivery and unavailable x87 platforms have explicit boundaries. No reciprocal estimate is used by this wrapper segment; preceding factory reciprocal ownership remains unchanged.

Complete original bodies and internal branch topology correspond in the authenticated later executable:

| TU1.1 | TU1.5 |
|---|---|
| `184510` | `185910` |
| `184800` | `185C00` |
| `1DACF0` | `1DF5B0` |
| `1DAD80` | `1DF640` |
| `1DADF0` | `1DF6B0` |
| `2E2260` | `2E5D60` |
| `2E2530` | `2E6030` |
| `2E25A0` | `2E60A0` |
| `2E5720` | `2E91D0` |
| `2E5750` | `2E9200` |
| `2E5760` | `2E9210` |
| `2E5780` | `2E93E0` |

The actual constructor's vtable immediate proves wrapper type23: TU1.1 `418930 +14 → A0120` and TU1.5 `41C3D0 +14 → A9520`, both complete original `mov eax,17; ret` bodies. Short configuration constructors are mapped through their genuine direct callers, rather than nonunique prefix guesses. This is full static source correspondence, not new TU1.5 guest execution. The API names remain `_1_1`: their source globals, virtual-target checks and observer instruction sites are TU1.1 storage bindings. A live TU1.5 adapter must import/map its actual addresses explicitly.

## Remaining live bridge

This segment can be installed after actual body creation and before slot commit, consuming the source body/component, actual resident shape, caller scalar, caller-owned scratch and retained game clock. It preserves typed lifetimes and the underlying convex vertex/plane ownership. It closes the measured wrapper output boundary from the prior factory freeze.

The shortest remaining chain still includes actual `1D9330` grouping/context and complete `1D95E0` selection/orchestration, full `1D3BB0` body/motion/shape/destructor/free ownership, then `1D4F30 → 1C7320 → 315820` real world/broadphase/solver delivery. Existing current-bounds and contact-record/lifecycle consumers require those actual backend/body/property/contact owners. No captured source contact may be used to admit a live native collision. Full title readiness, natural repeated tick ownership and positive player-versus-crate contact or incoming-damage/destruction remain unproved. Scene/Simulation/runtime/renderer/release files were not edited.
