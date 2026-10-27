# TU1.5 native world collision and post-collision listeners

`native_tu15_collide_physics_world` is the typed native implementation of
English TU1.5 `319D50`. It retains the original step/configuration copies, x87
scaling order, phase flags, mutable forward island scheduling, deferred-operation
checks, listener phases and profiling records. Its remaining island-collision
and deferred-work owners are explicit services.

The separate native listener module implements actual `3228C0` world and
`3229E0` island post-collision notifications: reverse callback iteration, live
array rereads, the second count read, stable null compaction, and source count
rereads during copying. Custom callback owners remain explicit typed services.
The complete authenticated `08FDB0` RET8 callback executes as its native empty
void operation; its original instruction is `c2 08 00`, also reached through the
actual read-only method slot at `45FD04`. No callback result is supplied.

These owners use named physics objects and arithmetic. They do not contain a
virtual instruction pointer, guest general-purpose register machine, decoder or
an original-code fallback.

## Physical original qualification

The offline oracle executes unchanged i386 instructions from the pinned English
TU1.5 XBE, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Authenticated complete listings contain 157/46/49/1 instruction sites for
`319D50`/`3228C0`/`3229E0`/`08FDB0`. Those listing counts describe source bodies,
not a claim that every path was executed.

The final GCC and Clang qualification passed:

- 300 physical-original cases: 173 complete world/listener calls, 10 stops at
  genuine remaining owner entries, 89 operand-read fault prefixes and 28 integer
  store-fault prefixes.
- 7,443 ordered reads, 3,431 ordered stores, 22,621 original instructions and all
  retained input/output banks. Both compilers matched complete CW/SW/MXCSR,
  including status/TOP/condition bits, rounding and precision controls.
- Finite values, signed zero, denormals, infinities, signaling/quiet NaNs, eight
  paired-NaN controls, signed counters and source packet/configuration aliases.
- Actual complete empty/null/default-listener paths. The original island
  collision, deferred-work and custom callback owners stop at their real entry;
  no fake return or expected collision/listener outcome advances the oracle.
- A private mutation clearing the collision flag instead of setting it was
  rejected by the unchanged original store comparison on both compilers.

Independent continuation models cover callbacks that replace the active-island
array, change the island count, enable listeners, queue deferred work, replace a
listener array and remove members. They also check source profile-label order
using actual host timestamps. These models are not original execution proof for
those remaining callback owners.

The full original timestamp sequence, remaining island/deferred/custom callback
execution and live multiplayer composition are separate gates. Unmasked traps
and the live x87 machine postimage of an x87 store fault remain unsupported.
The integer store guards intentionally exclude FST/FSTP traps; operand-read
prefixes retain preceding stores and numeric status. No whole-frame raw-bank
exclusion was added or changed.

## Evidence and storage

The final report and bounded GCC/Clang build/test/negative-control logs are in
`analysis/physics/native-world-collision-tu15-20261026-final/`. Exact source and
listing hashes, commands and the original fixture hash are in `report.json`.
The small authenticated listing is retained in
`analysis/physics/native-world-collision-tu15-20261026/original-listing.txt`.

The earlier attempt reports preserve a test-only string comparison compile
failure and a corrected Capstone x87 operand classification. Capstone23 labels
FST/FSTP destinations as reads; the validator uses the same authenticated
architectural-store correction as the existing pose oracle. Neither failure
required a physics-kernel change or a relaxed comparison.

Both compiler builds and all four enclosing scratch owners were verified
removed. Immediately before the final owner ended, its remaining temporary
files occupied 36,839,424 allocated bytes (36,805,270 logical bytes), including
the 25,399,211-byte original fixture; those files were removed. Compiler builds
had already been deleted by `tools/run_tests.py`. The final bounded reports
occupied 12,657 logical bytes before the cleanup receipt was added. No large
artifact is retained. `cleanup.json` records removal checks for every attempt.

Source pins for live integration:

| Source | SHA-256 |
| --- | --- |
| `native_tu15_world_collision.cpp` | `7917af26c3cf56a5433dbf54ebeddd9be90d54593ae710b7a9d867e70259bd01` |
| `native_tu15_world_collision_listeners.cpp` | `dedb7224c0b3f9c13e21bc17ba2411d77dc8653dee2573cd9f230c9cfc3764bb` |
| `native_tu15_world_collision.hpp` | `23924b13f0227cfbe1df9f459e0893dac9547ebd8925eb30016643a7c1e814d6` |
| `native_tu15_world_collision_listeners.hpp` | `3e3299b0961e07d30c47c6868813905f589602475e61146540ac362f49616ab7` |
