# Original local-map pool construction and callback16 platform stop

Original `19F160` now completes before the same full `190A30→139410→139540` map-start attempt. It owns two level-handle pools of 20 and 50 eight-byte slots. Original callback13 `19F720` then completes, resets both pools and creates 15 source level handles from the retained real cache. The original ascending reset loop next reaches callback16 `125370`, which stops at its first unprovided `MmSetAddressProtect` request. Readiness, local membership, profiles, device scheduling and simulation state four remain unexecuted. No pool record, binding, readiness or game/physics success result is supplied. Production and earlier freezes are unchanged.
[Independent oracle](../tools/oracle_mp_spawn_local_map_pools.py),
[complete report](../analysis/multiplayer/integration/spawn-source/local-map-pools/report.json),
[preceding exact callback13 stop](MP_LOCAL_COMMAND_SCHEDULING_RESEARCH.md).

## Authority and independent freeze

Original TU1.1 XBE SHA-256 is `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`; original Lockout SHA-256 is `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`. The frozen complete compatibility context and selected original heap/world/control/view/camera/simulation preparation remain declared inputs. The complete compatibility worker stays alive through source execution. The two additional original RNG-constructor clock requests are replayed from measured real-host inputs; the earlier simulation clock measurements remain unchanged. Compatibility-produced unknown zeros, raw host memory and clock inputs are not recovered Xbox hardware defaults.
[Complete platform context](MP_COMPLETE_COMPAT_KPCR_RESEARCH.md),
[preceding preparation and ordinary scheduling limits](MP_LOCAL_COMMAND_SCHEDULING_RESEARCH.md),
[new measured clock input](../analysis/multiplayer/integration/spawn-source/local-map-pools/measured-extra-clock-inputs.json).

| Evidence | SHA-256 |
| --- | --- |
| Oracle | `73eaaa882c57491583f952757c652bcd6dc36ac690d06e5b44c619f9fc33bb6f` |
| Report | `2a0fac629605e5166e8f8cbaf85f8c8723b21cfbe9b83d2f3a4e255ef03e76ab` |
| Instruction manifest | `03d0dac5f693886473368756526b297bee6964d8d5c8ed4494d6ba4c07cb4990` |
| Executed original instruction listing | `f714c70edc0558d2726646e77f20211170efd87d6ab8cbb81511c5fea3f46012` |
| Observed source store listing | `b929ccc4772e6d34844cfd91d20acd5d12b7b6271f3bb650247efb5234504729` |
| Additional measured clock fixture | `1775aa43de2041caf2e6b6927b8f603f1571064dddcfa905e7d8a9136c1c6fb9` |

One independently repeated owning case audits 12,830 executed original instruction addresses against the pinned XBE and unchanged emulator image. The 65 routine listings contain 4,912 rows; the complete executed listing and 242 observed source store sites are also retained. The deterministic report and all 67 instruction files reproduce byte-for-byte. All 101 earlier/input pins, including all 50 instruction files from the preceding local-scheduling freeze, remain unchanged. Native allocation addresses and complete live host context records are separate evidence and may differ across independent runs.
[Executed bytes](../analysis/multiplayer/integration/spawn-source/local-map-pools/instructions/executed-original-instructions.asm),
[observed source stores](../analysis/multiplayer/integration/spawn-source/local-map-pools/instructions/observed-map-pool-source-store-sites.asm),
[independent verification](../analysis/multiplayer/integration/spawn-source/local-map-pools/independent-freeze-verification.json).

Reproduce from the repository root:

```sh
.tools/python/bin/python tools/oracle_mp_spawn_local_map_pools.py \
  --output analysis/multiplayer/integration/spawn-source/local-map-pools/reproduction/report.json
```

## Actual allocator and precisely supplied raw-storage boundary

The allocator is original static XBE data: `[46FBF8]=480D68`, the object's vtable is `45A480`, and its first two methods are `11E150` and `11E180`. No allocator object, pointer, table or method is assigned. Original `19F160` invokes `11E150` for literal F0 and 1E4 bytes. Original `11E150` supplies `(NULL, requested_size, 101000, 4)` to `33785C`. That complete XAPI wrapper executes unchanged and calls original kernel ordinal184 with pointer-to-base, zero-bits, pointer-to-size, flags and protection arguments.
[Original static data and actual call ABI](../analysis/multiplayer/integration/spawn-source/local-map-pools/report.json),
[pool constructor](../analysis/multiplayer/integration/spawn-source/local-map-pools/instructions/0019f160.asm),
[original allocation method](../analysis/multiplayer/integration/spawn-source/local-map-pools/instructions/0011e150.asm),
[complete original XAPI allocation wrapper](../analysis/multiplayer/integration/spawn-source/local-map-pools/instructions/0033785c.asm).

The newly supplied service is only the requested raw storage at ordinal184. Two actual owned Linux `mmap` allocations with read/write protection are mirrored into Unicorn through `mem_map_ptr` at declared guest addresses 0B200000 and 0B201000. The measured host page size is 4096; each requested raw region occupies one host page. The provider records the exact original request, actual native storage address, initial raw-storage hash, guest mirror, resulting mapped size and declared NTSTATUS zero. The original wrapper then obtains its return from the supplied kernel outputs. No constructor field is written by the provider. These guest addresses, pointer/size/status adaptation, flag101000 handling and host-produced initial zeros remain explicit provider boundaries. This does not execute Xbox `NtAllocateVirtualMemory`, prove MEM_NOZERO behavior or reproduce hardware allocation addresses. The original fixed arena is not enlarged.
[Actual requested raw allocations](../analysis/multiplayer/integration/spawn-source/local-map-pools/report.json),
[separate live native storage evidence](../analysis/multiplayer/integration/spawn-source/local-map-pools/raw-host-allocator-storage.json).

Earlier Python observer layers assume fixed raw `33785C` service returns and reject these new source-requested sizes. Only those observers are bypassed at this one wrapper entry; ordinary instruction journaling and the original wrapper body continue. Every post-preparation gameplay/Havok service hook is removed. The owning constructor and admitted map/reset path use no supplied game or physics return. Earlier frozen preparation's raw OS services and its sole excluded savegame-I/O call retain their declared scope.
[Removed service records and complete stage events](../analysis/multiplayer/integration/spawn-source/local-map-pools/report.json),
[preceding arena and startup limits](MP_FIXED_MEMORY_WORLD_RESEARCH.md).

## Source-created pools and complete callback13 reset

| Source output | First pool | Second pool |
| --- | --- | --- |
| Header name written by source | `level handles` | `multiplayer level handles` |
| Binding writer and slot | 19F1A3→4F5CBC | 19F1D0→4F5CB8 |
| Actual supplied raw allocation requested by source | F0 bytes | 1E4 bytes |
| Source capacity/stride | 20 / 8 | 50 / 8 |
| Source header address | 0B200000 | 0B201000 |
| Source data/bitmap addresses | 0B20004C / 0B2000EC | 0B20104C / 0B2011DC |
| Constructor high-water/count | 0 / 0 | 0 / 0 |
| Retained reset high-water/count | 15 / 15 | 0 / 0 |
| Retained source next salt | E57B | F56D |

Original `16D070→16D0D0` writes names, capacity, stride, alignment, allocator and data/bitmap fields in both complete headers. `19F160` applies original flag value0x04 and publishes the actual returned pointers. It then initializes 64 tracking pairs at 4F5AB8 to `(NONE,0)` through source stores. Full before/after F0 and 1E4 storage, complete 4C-byte headers, all source stores and the live allocator ABI are retained.
[Complete pool records and source stores](../analysis/multiplayer/integration/spawn-source/local-map-pools/report.json),
[original pool construction](../analysis/multiplayer/integration/spawn-source/local-map-pools/instructions/0016d070.asm),
[header field owner](../analysis/multiplayer/integration/spawn-source/local-map-pools/instructions/0016d0d0.asm).

During full map start, callback13 `19F720` sets the original +29 eligibility byte and calls whole `16D220` for both pools. Full `19F6B0`, `19FC60` and `19FD70` then execute and return. The actual delivered scenario short +10 is one, so `19F6B0` skips its type-two network enumeration branch. The actual matg170 descriptor has count one and pointer 8014FC58; its nested first level descriptor contains 20 authored 108-byte records at 8014FC60. The matg178 descriptor is empty. `19FC60` produces 15 handles in the first pool; `19FD70` leaves the second pool empty. Exact source descriptor/header/record bytes and all 15 created handle records are retained. These are level-handle outputs, not local player or view bindings. The real-cache/global binding remains inherited metadata delivery rather than a complete cache loader proof.
[Actual cache inputs and created handles](../analysis/multiplayer/integration/spawn-source/local-map-pools/report.json),
[complete reset callback](../analysis/multiplayer/integration/spawn-source/local-map-pools/instructions/0019f720.asm),
[source salt/count/data reset](../analysis/multiplayer/integration/spawn-source/local-map-pools/instructions/0016d220.asm),
[scenario gate](../analysis/multiplayer/integration/spawn-source/local-map-pools/instructions/0019f6b0.asm),
[first level producer](../analysis/multiplayer/integration/spawn-source/local-map-pools/instructions/0019fc60.asm),
[second level producer](../analysis/multiplayer/integration/spawn-source/local-map-pools/instructions/0019fd70.asm).

## Exact subsequent callback order and first unresolved dependency

The unchanged full `139410` reset loop executes nonzero targets in ascending table order: index2 `11E330`, index9 `187550`, index13 `19F720`, then index16 `125370`. The first three return. Callback16 enters `125370`, which reads the actual source arena `[4ED650]=82D47000`, supplies literal size3BE000/protection4 and calls `2D7A1A`. That original XAPI adapter tail-jumps to kernel ordinal182 `MmSetAddressProtect` through import417BA8.
[Actual callback records, order and ABI](../analysis/multiplayer/integration/spawn-source/local-map-pools/report.json),
[original reset traversal](../analysis/multiplayer/integration/spawn-source/local-map-pools/instructions/00139410.asm),
[callback16 original request](../analysis/multiplayer/integration/spawn-source/local-map-pools/instructions/00125370.asm),
[original protection adapter](../analysis/multiplayer/integration/spawn-source/local-map-pools/instructions/002d7a1a.asm).

The retained stop is the unprovided kernel service, before any permission operation or void return is supplied:

```text
kernel target:          800000B6, ordinal182
original return site:   00125387
source arguments:       82D47000 / 003BE000 / 00000004
EAX/ESI:                82D47000
EBX:                    00000000
ECX:                    021FCE4C
EDX:                    00000008
```

The inherited provider previously declared only `(82D47000,3FE000,4)` during arena preparation. This new request lies within that prior source-created backing, but extending its protection provider is separate work; this freeze performs no new protection operation. Static `125370` also shows a subsequent, unexecuted protection request `(arena+3BE000,40000,404)`, followed by its record initialization. No permission/cache behavior is inferred to bypass either request.
[Complete unresolved-service record](../analysis/multiplayer/integration/spawn-source/local-map-pools/report.json),
[full callback16 listing, including the unexecuted tail](../analysis/multiplayer/integration/spawn-source/local-map-pools/instructions/00125370.asm).

The original in-progress game byte +0 remains one. Readiness +1120 remains zero; simulation kind/state remain 1/2. `1395B0` membership creation, `11D780` BSP startup, `190A6E` readiness, local profiles/controllers/views/device submission and state-four scheduling are not reached. The prepared empty source player/object pools are preserved. The explicit-root composition still excludes complete `139390` startup, all 68 storage constructors, upstream options/lobby/sign-in/session ownership, original game-clock construction, complete `139750` and Xbox hardware execution. The completed pool/reset boundary does not close those gates.
[Complete retained state](../analysis/multiplayer/integration/spawn-source/local-map-pools/report.json),
[preceding original local ownership/caller chain](MP_LOCAL_COMMAND_SCHEDULING_RESEARCH.md).
