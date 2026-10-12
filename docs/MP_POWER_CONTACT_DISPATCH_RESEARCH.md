# Original canister contact dispatcher and retained packet-frame boundary

This independent native extension composes complete original `300870` with the existing actual child-cache/BSP, narrowphase, GJK and contact-packet owners. It uses the unchanged original observations in the immutable [contact-batch freeze](MP_POWER_CONTACT_BATCHES_RESEARCH.md), including its independent original repeat. It adds no source stimulus, instruction patch, contact, shape, clock, admission, pose or successful callback. No finished GJK workspace or packet is replayed into native composition.

The actual canister prefix and the **complete player cold-empty query** match **2,370,278 bytes and 1,047 ordered owning writes**. The canister stops at an additional real dependency: the raw stack input copied by `300400`. That boundary is required before packet count/allocation changes. The remaining positive dispatcher tail is translated from the entire original body, but **has not completed or been differentially verified**. This is not live collision, solving, settling, damage or explosion.

## Files and verified scope

- Native interface/implementation/test: `include/halo2/physics_contact_dispatch.hpp`, `src/physics_contact_dispatch.cpp`, `tests/physics_contact_dispatch_tests.cpp`.
- Original-only export: `tools/export_mp_power_contact_dispatch.py`.
- Source, repeat, byte-ancestry and instruction audit: `tools/audit_mp_power_contact_dispatch.py`.
- Ignored immutable evidence: `analysis/multiplayer/sandbox-source/power-contact-dispatch/final/`; its manifest pins sources, original-only exports, audit, normal/sanitizer/production binaries and logs. Failed whole-composition evidence remains in `draft/first-native-test.log` and is also retained in the freeze.

The exporter checks all preceding 92 GJK pins and all 82 contact-batch pins. It exports directly from the unchanged original read/write timelines and actual complete stored states. Independent exports from original `measured/` and `repeat/` are byte-identical. It does not read native outputs when forming expected bytes. No new original gameplay execution is claimed by this extension: the two source calls were already independently executed and frozen in the contact-batch suite.

Normal optimized, ASan+UBSan/leak and newly linked actual Systems production tests pass. The sanitizer build instruments this new dispatcher and the composed contact-batch, GJK, narrowphase, child-agent, BSP-child and body-factory sources. One additional test removes the packet-frame provider entirely and compares the same complete prefix state and 802 writes, establishing that absent ownership cannot become a successful packet append. The empty query completes without requesting a packet-frame provider or an estimate.

## Original schedule and exact ABI

Authority is the entire unchanged `00300870.asm` in the contact-batch freeze: **534 instructions**. `300870` is a thiscall with ECX parent agent and four stack arguments `{collidableA, collidableB, world_step, output_packet}`; `ret10`. Its frame is `((entry_esp - 4) & ~F) - 330`.

It executes child-cache update `2EFC30` before scratch/packet setup, then iterates the original unsorted child rows (stride `0C`). It obtains each actual BSP child shape, calls shape-type/capability owners, executes actual `2E14E0` narrowphase and `30AE40` GJK, then attempts packet append at `300A07`. It assigns agent/shape/key/result only after append. Source-listed later operations sort by distance, prune, inspect retained contacts, call `309210` and `309450`, dispose scratch and finally free owned packet storage. Those later positive dispatcher branches are not verified by this extension. Profiling-positive branches require actual RDTSC input; actual profiling capacity is zero.

| Actual original call | Verified native extent |
|---|---|
| Canister parent `0601ADE0`, call7, entryESP `021FCF3C` | 802 owning writes through first separated child, then exact required packet-frame stop. |
| Player parent `0601AD60`, call25, same entryESP | Complete cold-empty call, 245 owning writes, original scratch disposal and retained output state. |

Both original calls use actual world step `060055B0` and output descriptor `021FCF60`. These addresses are observed source identities, not native allocation constants. Current source pools/world/bodies/shapes/child agents and allocator storage remain separate retained identities.

## Why the raw packet requires another owner

Original `300400` initializes only packet-input bytes `7C/7D` at `300414/30041B`, increments descriptor count at `30044E`, and calls `300100` to copy the actual retained stack buffer. It does not construct or clear all 128 bytes. Source `300100` leaves holes `28..2F` and `7E..7F` untouched in the destination.

The preceding native lower owners preserve semantic data and arithmetic stores, excluding guest return addresses/register saves/SEH writes. That distinction becomes observable here because the original copy consumes those earlier stack slots. The first attempted whole native composition matched its first 805 owning writes, including append's flag/count writes, and then differed at the first copy (`300109`, destination `021FCD20`). Actual original first words were child-row `0601A9A0` and child-shape `004819D0`; the incomplete native frame contained zero. This failed comparison is retained; expected bytes were not weakened and recorded words were not injected to finish it.

The new interface exposes the dependency **before** entering `300400`:

```
instruction300A07, entry300400
entryESP021FCBFC, descriptor021FCD10
packet_input021FCB70, bytes80
packet_input = ((entryESP - 4) & ~F) - 80
```

`SourcePhysicsContactDispatchServices::packet_frame_owner` must execute the actual owning frame producer, or fail. It has no result/success default. The native kernel does not use a fixture packet as that callback. With no provider it throws before append count or allocation changes.

`final/audit.json` retains **every original input byte and its last actual writer**, for both original child packets, including bytes retained from before the dispatcher. Selected actual owners are:

| Input bytes | Last actual source owner |
|---|---|
| `00..03` | `2E14FE push EDI`: current child-row identity. |
| `04..07` | `2E14FD push ESI`: current actual child shape. |
| `08..0B` | `2E14EC push EBX`: current child-agent identity. |
| `1C..1F` | `2E14F0 mov [esp+14],ECX`: same agent, a semantic store already produced natively. |
| `20..23` | `2E1508 mov [esp+20],0`: actual initial distance storage, already produced natively. |
| Further scalar/identity slots | Actual `1F3320` argument pushes/call, `1F16F0` saves and its real terrain face/edge stores, preceding relative-matrix stores, `3009FD` return address and `2E14E0` frame. Complete byte ancestry remains authoritative. |

The next packet-owner work must represent actual saved-register/argument/previous-frame ownership and its exact lifetime or explicitly retain the boundary. Copying this audit's recorded words or zeroing a temporary would fabricate that producer. The native implementation currently cannot install the positive whole dispatcher in SceneObjects.

## Estimate and downstream boundaries

Fixture `Q` records are actual Unicorn RSQRTSS observations used **only by this test**. They are emulator evidence, not Xbox/Pentium III hardware evidence. A separate fresh actual-host diagnostic runs the composed prefix with `source_host_x86_rsqrtss_estimate`:

```
input3FBEC686, Unicorn3F51B161, host3F51B800
canister prefix: one differing estimate-spill store, same802-store extent
player empty call: no differing stores, same245-store extent
```

That host path reaches the same required packet boundary without any finished-workspace callback. It does not establish general contact-decision or Xbox bit parity. The prior admitted second-child diagnostic and its hardware limit remain unchanged; this extension stops before that child.

Downstream actual `313800` constraint/world insertion, `322410` point/scalar/index allocation, creation listeners `31E8B0/31C2E0`, retirement/reduction and genuine solver delivery remain required. The [contact-batch note](MP_POWER_CONTACT_BATCHES_RESEARCH.md) retains their exact observed call order and the native manager prefix. No borrowed contact, AABB/SAT/capsule substitute or successful backend default is added here.

## Authenticated update correspondence

The preceding complete contact-batch correspondence proves entire `300870` → authenticated TU1.5 `304350`, including internal branch topology, direct-call pairing and executable data operands. This extension audits that pinned complete mapping and original bytes again. It is static correspondence, not new TU1.5 gameplay execution. The raw native storage API retains explicit TU1.1 address names; live TU1.5 needs the verified address/data/vtable bindings.
