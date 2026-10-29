# Campaign audio provider and retirement frontier

The next closed native capability is the actual final phase of `129E30`:
`129EA0` source retirement, `220690` controller-reference release and generic
`16E8A0` salted data-array deletion. Source/controller policy is typed code in
`native_campaign_audio_retirement.hpp`; deletion reuses the shared semantic
`native_tu15_data_array_delete` module owned jointly by audio and script-clump
consumers. No second generic deletion implementation was added.

The capability has no source-provider callback, platform call, supplied return
or lifted dependency. It preserves the upper controller-reference flag and
seven-bit decrement/wrap, conditional side-reference publication, rereads the
source's controller datum after that publication, and retains/reloads pool
owners exactly where the original does. Shared deletion performs actual
DWORD/tail-byte BA poison, bitmap clearing, salt invalidation, free cursor,
trailing extent contraction and live-count decrement. ABI adapters require the
original forward string direction, DF=0. No virtual CPU or virtual stack occurs.

## Remaining provider boundary

`12AAD0` reads the source flags, descriptor, callback, deadline and audio clock.
Flags bit0/5, a missing descriptor/callback, or the unexpired deadline with
audio+7B clear return the original default allow result. Otherwise `12AA20`
calls the real descriptor+0 predicate with context, sound datum, marker and
location backing. A false provider result is **not** itself the final deny:
the original reloads the binding table after the callback, tests source type,
then class policy through `228220` and `127AA0`, and publishes flag20. Native
conversion must retain those callback effects and later policy branches.

The owned image and existing `OriginalData` importer authenticate these complete
32-byte descriptors (they are identities, not executable host function pointers):

| Descriptor | Predicate at +0 | Stop notification at +10 |
|---|---|---|
| Ordinary FP `44EBF4` | `18F1A0` | `18F6B0` |
| Weapon FP `44EC14` | `18F1A0` | `18F6B0` |
| Effect `455C18` | `224400` | `2244A0` |

`18F1A0` has 184 sites and 14 direct dependency owners (object validity/parents,
class, nodes/transforms, physics velocity, location/PVS and audibility). The
existing semantic `sound_object_request.cpp` represents its TU1.1 counterpart
`18DDE0`, requiring actual retained object, transform, velocity and point-query
services. It is a useful native implementation, but there is no qualified
TU1.5 campaign descriptor-dispatch binding to claim here. The ordinary/weapon
tables themselves are retained by `native_tu15_world_memory.cpp`.

`18F6B0` has 22 sites and calls `BCDF0` and `D3300`. Effect predicate `224400`
has 20 sites and calls `224590`; effect stop `2244A0` has 51 sites, calls a dynamic
effect callback and `224040`. Each reached owner still needs its genuine native
dependency graph. Other descriptors may be supplied dynamically: this recovery
authenticates the three imported families, not an exhaustive runtime inventory.
No provider success is synthesized and these callbacks are not registered as
qualified by this work.

## Remaining stream/resource boundary

`129E30` first releases a present signed static-voice index through `12D0E0`,
then clears the source index, unpins through `129470`, invokes a present
descriptor+10 stop callback with actual context/source/reason, and finally calls
`129EA0`. Callback mutations therefore precede the new module's source-pool
lookup. This work closes that final lookup/release/delete phase only.

`12D0E0` retains static-voice, cache and gestalt owners before its first clear.
It releases the +20/+1C cache references with signed selectors, clears their
pointers, and handles a retained driver-stream index through `2B81C0`. After
the driver returns it rereads the stream index, clears the system+60 bit,
releases the shared voice through `224A60`, then clears voice fields in order.
`224A60` decrements a 16-bit shared reference, conditionally clears a system+70
bit and uses the shared `16E8A0` deletion owner at zero.

When driver flags+1 have any low-three bit set, `2B81C0` requires:

1. Actual retained buffer status from vtable+0C, with its DWORD output.
2. Conditional buffer Stop when status bit20000 is set (`385902` → `384E7C`).
3. Actual StopEx at time0 with flags3 (`385907` → `384ECD`).
4. The retained stream's vtable+1C flush with its original this convention.
5. Only after those calls, publication of stream flag+3 bit2.

The SDK wrappers enter real SDK/device exclusion and import dependencies;
transcribing their COM instructions is not a native backend. Existing semantic
`sound_source_lifecycle.cpp` already models the ten cross-image cleanup CPU
owners, but its documented positive callback/platform paths remain unqualified
and the implementation has no production call binding. Existing SDL output
provides actual retained packets, status and flush for native voice receipts.
It does not itself establish the source32 buffer/stream identity mapping or
this exact status/Stop/StopEx/flush contract. Those are explicit unresolved
native service boundaries, not a claim that audio output is absent.

Authenticated listings and identities are retained under
`analysis/campaign-native-eligibility-20261028/`: `owner-graph.json`,
`dependency-graph.json`, `provider-tables.json` and `platform-thunks.json`.

## Independent source/controller proof

The unchanged owned TU1.5 image is executed offline on physical i386; SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
There are no instruction patches, dependency returns, provider substitutes,
unknown-owner resumptions or emulated binaries in this proof.

116 whole original calls execute 16,197 instructions and all 114 sites (30 source,
22 controller, 62 pool deletion). The native implementation matches 3,377 reads
with store-count interleaving, 1,863 ordered writes, complete retained banks,
stack cleanup, actual preserved registers/SIMD, retained 80-bit operands and
unchanged FP controls/status/MXCSR. Portable shared-core writes are compared by
address, width, before/after bytes and ordering; source instruction PCs are
retained for reference diagnostics, not invented for portable deletion writes.

Cases include count and seven-bit wrap, upper flags, null/NONE references,
conditional side references, sparse/trailing pool slots through 33, changing
free cursors, zero/short/ordinary strides, DWORD/tail-byte poison, and an actual
alias where side-reference publication changes the source's controller datum
before its reread. GCC and Clang sanitizer evidence covers the closed chain.
Mutations reject dropping the controller upper flag, skipping the post-publication
datum reread and moving the category read across the controller-pool lookup.

The production shared helper has one acyclic static-library owner,
`halo2_native_tu15_data_array_delete`, linked by both systems and physics/runtime
consumers. Isolated proof executables compile their own private helper copy.
Whole campaign audio, positive provider/COM paths, arbitrary unowned memory and
live gameplay acceptance remain outside this bounded evidence. The campaign
gate remains closed at four qualified required addresses of forty.

Final reports, qualification metadata and cleanup/source hashes are under
`analysis/campaign-native-retirement-20261028/`. Earlier completed fade,
listener, ducking and channel algorithm source files remain unchanged. All raw
banks, ELF files, vectors and private builds use the official disposable wrappers;
no large artifact is intentionally retained.
