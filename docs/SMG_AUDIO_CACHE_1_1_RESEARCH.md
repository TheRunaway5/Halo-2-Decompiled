# Original SMG sound cache with real host providers, disc 1.1

This separate study executes the original cache constructors, page allocation, resource-read queue, worker callback, completion store, source pin guards, busy callback, and one-entry eviction against real authored Lockout SMG sound metadata. Host providers own actual memory, mutexes, a semaphore, and an opened, verified `shared.map` file. No original cache/page fields or completion flags are set by the fixture to force admission. No production file was changed.

This does **not** establish successful ordinary sound availability, the original asynchronous thread's timing, complete Xbox driver initialization, decoding, stream consumption, or audible parity. Actual original FP requests still reject unavailable sound. A separate direct `127750` call is explicitly supplied to inspect its cache-producing kernel; it is not a fabricated successful return from the unavailable request. `system+78` remains 0 and `[526474]` remains 0 throughout.

The prior [sound producer study](SMG_FP_SOUND_1_1_RESEARCH.md) and [minimum audio adapter study](SMG_AUDIO_ADAPTER_1_1_RESEARCH.md) remain byte-identical. This new evidence advances their cache boundary only.

## Authority and independent freeze

All instructions come from the user's map-pack update `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`. This is disc **1.1**, not a final-ever update claim.

- Lockout: SHA-256 `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
- Shared resources: SHA-256 `084c817fa82bb9bc416d9ba847cec05be431abc419e3a1087cdf211fbf47f5af`.
- [Oracle](../tools/oracle_smg_audio_cache_1_1.py): SHA-256 `9c546c136b0056a741b1d5c1c519cc476f4cbdaacd6c346bc3661d63fdf262a1`.
- [Frozen suite](../analysis/multiplayer/map-pack/smg-audio-cache/frozen/suite.json): SHA-256 `69baec9c569ba241cdf4fa5f911658d284f9e3241cd606ef659b1898d16eee83`.
- [Source fixture](../analysis/multiplayer/map-pack/smg-audio-cache/source-fixture.json): SHA-256 `c1256f79374fed79416b3e4ee89c8e83fd863ae1d25067f10302b16d3e840586`.
- [Auditor](../tools/audit_smg_audio_cache_1_1.py): SHA-256 `74d69c6b3cda4d0a569e8291747184ef9c2f6869c50de75387ad6a0a3c987b74`.
- [Audit result](../analysis/multiplayer/map-pack/smg-audio-cache/audit.json): SHA-256 `9754cfefa8c3e5e334f07e3400ebb11c1853e7287b184d2bc032534af2045853`.

Six cases have byte-identical independent process reexecutions. The auditor verifies 486 listings / 24,534 original instruction rows against the image, 1,254 authored metadata records against Lockout, every enumerated sound-chunk hash, seven actual aligned file callbacks / 313,344 bytes, seven original completion stores, 36 explicit lifecycle probes, and 26 original LCG stores. Seventeen earlier tool/report/document pins are unchanged. `native_gameplay_update_applied:false` remains in every report.

| Frozen case | Measured original behavior | Explicit supplied boundary |
|---|---|---|
| [ready](../analysis/multiplayer/map-pack/smg-audio-cache/frozen/ready.json) | Original FP ready request; direct creation kernel; selected resource allocation, actual read, original completion, original source unpin | Direct creation and one-worker invocation; genuine host services |
| [reload](../analysis/multiplayer/map-pack/smg-audio-cache/frozen/reload.json) | Same, original full-reload FP request | Same boundaries |
| [ready_lifecycle](../analysis/multiplayer/map-pack/smg-audio-cache/frozen/ready_lifecycle.json) | Pending/loaded query, pin guard, busy checks, source unpin/repin, frame bookkeeping, rejected/accepted capacity allocation and actual eviction | Kernel probe call sites and `300000` capacity request are explicit test inputs |
| [reload_lifecycle](../analysis/multiplayer/map-pack/smg-audio-cache/frozen/reload_lifecycle.json) | Same lifecycle with actual reload resource size | Same boundaries |
| [ready_queued](../analysis/multiplayer/map-pack/smg-audio-cache/frozen/ready_queued.json) | Two real source datums select different ready permutations, queue two reads and complete/dequeue separately | Second direct kernel invocation reuses the actual rejected request; it is not a second claimed FP event |
| [reload_shared](../analysis/multiplayer/map-pack/smg-audio-cache/frozen/reload_shared.json) | Two real source datums share one cache entry/read; source pins go 2, 1, 0 | Same explicit repeated-kernel boundary |

Reproduce into a distinct report. The original instruction directory is deterministic; using the same directory retains identical report paths:

```sh
.tools/python/bin/python tools/oracle_smg_audio_cache_1_1.py \
  --case ready_lifecycle \
  --instructions analysis/multiplayer/map-pack/smg-audio-cache/frozen/instructions/ready_lifecycle \
  --output /tmp/halo2-audio-cache-ready-reexecution.json
cmp analysis/multiplayer/map-pack/smg-audio-cache/frozen/ready_lifecycle.json \
  /tmp/halo2-audio-cache-ready-reexecution.json
.tools/python/bin/python tools/audit_smg_audio_cache_1_1.py
```

The auditor only exports this study's source fixture/audit. It reads prior pins without regenerating prior suites.

## Real host ownership and retained boundaries

The provider returns deterministic x86 **aliases for actual owned host objects**. Those aliases are fixture inputs; they are not claimed Xbox addresses/handles. Anonymous host `mmap` storage is mapped into Unicorn with `mem_map_ptr`; original source constructors write the headers, bitmaps, pool entries, and page state themselves. Host allocation failure, file failure/short reads, cancellation, blocking wait concurrency, and multiple worker scheduling are not exercised by these successful-read cases.

| Original platform entry | Real provider operation executed | Scope |
|---|---|---|
| `33785C VirtualAlloc` | Actual anonymous `mmap`; requested sizes `208C` and `30FC`, source flags `101000`, protection 4 | Host storage owns aliases `09000000` and `09003000`; original XAPI wrapper/kernel instructions are substituted at this entry |
| `2D79AA XPhysicalAlloc` | Actual host storage for the original request `(30E4000,61000,0,4)` | Explicit alias `80061000`; actual `12CB70` still writes its physical-arena limits; hardware physical allocation is not claimed |
| `2D7A1A XPhysicalProtect` | Actual host `mprotect` and Unicorn protection for `(82E45000,300000,4)` | Protection 4 is translated to writable/readable host backing; no DirectSound object is supplied |
| `2DC83A CreateMutexA` | Actual `threading.RLock` for each original `(0,0,0)` request | Provider registry retains real objects; original queue/free-list locking calls acquire/release them |
| `2DC7B9 CreateSemaphoreA` | Actual `threading.Semaphore(0)` for original `(0,0,150,0)` | Real release/acquire/count behavior for available work |
| `2DC9C8`, `2DC892`, `2DC814` | Actual wait/acquire, mutex release, semaphore release; actual previous count written when requested | No unavailable wait is converted into success; this single-worker fixture stops before the next wait |
| `2D88C8 SetFilePointer` | Actual seek of the opened shared map; actual resulting high/low pointer values | Original callback supplies a non-null high-word pointer containing 0 |
| `2D8681 ReadFile` | Actual aligned read, copy of real bytes to destination, actual byte-count output | Provider never writes the cache completion flag; original worker does |

`12CB70` requests physical bounds `61000..3145000`. The existing supplied original-cache metadata mapping overlaps part of the physical alias range. The fixture preserves those inherited input bytes while replacing their backing with the actual host allocation, recording the preserved region hash. This does not prove the original whole-process startup/map-memory lifecycle.

Static audio backing `7C00000/23000` and separate class backing `7C23000/1000` remain explicit inputs inherited from the prior study. Their original static startup allocator is not claimed here. Real `223960/2239B0` class construction/reset and the isolated original ambient NONE stores still execute; no unavailable clock result is supplied.

The original file registry producer is also not executed. The provider actually opens and verifies the named `shared.map`, then supplies its private file handle through original slot `55F798`, which the actual encoded location-2 dispatch consumes. No filename is guessed for other resource locations.

## Constructors that now execute

The actual original sequence is:

1. `12CB70` publishes physical-arena stage 0, lower limits `61000`, upper/current limit `3145000`, then calls the real host storage provider with the original allocation request.
2. `126B20` allocates/clears the real source system and invokes `21A6E0`.
3. `21A6E0` requests `208C` and `30FC` bytes through the original allocator vtable `[46FBF8]=480D68 -> 11E150 -> 33785C`.
4. Original `16D070` constructs the cache-entry pool, capacity 512 / stride 16. Original code sets pool `+2A bit2`. **Pool valid byte `+29` remains 0 in this constructor-only slice.** The separately [inspected map/cache lifecycle function `190690`](../analysis/multiplayer/map-pack/smg-audio-cache/inspected/00190690.asm) writes valid 1 and resets the pool in its AL==0 branch (`1906FA/1906FE`); that caller/input path is not executed here. No valid bit is forced. Original `16D390` insertion in this bounded cache path still executes.
5. `13E8E0` constructs the actual page allocator and its embedded 512-entry / 24-byte pool, marks the embedded pool valid and resets it. `21A6E0` selects allocation policy 2.
6. Original `21A6E0` reserves `300000` bytes from the original upper arena limit: `3145000 - 300000 = 2E45000`, converts it to the explicit mapped physical alias `82E45000`, requests real protection, and publishes the three original cache globals.
7. The rest of `126B20` constructs actual source/static-voice/controller pools and stops at the genuine unresolved controller `KeQuerySystemTime` import `336983`. It has not returned successful whole-audio initialization.
8. Actual `126DE0` installs the real `ugh!` reference, enabled/master-fade fields, then stops at genuine `KeTickCount` `337A45`.
9. Actual `1221B0` clears/links all 150 original 64-byte async nodes, publishes free/work lists, and obtains real mutex/semaphore objects. It then stops **before** unavailable `2D8606 CreateThread(0,4000,122350,0,0,0)`. No original thread handle, priority success, or clock result is supplied.

All partial caller stops remain in the reports, with original arguments/import slots. No fragment resumes an unavailable constructor by setting its successful return.

The page allocator's `70` bytes are retained verbatim:

| Offset | Source-produced meaning/value |
|---|---|
| `00..1F` | Original `xbox sound cache` name |
| `20`, `24` | Delete callback `21AAF0`; busy callback `21AAB0` |
| `28` | Original supplied constructor value 0 |
| `2C` | Policy 2 written by `21A6E0` |
| `30` | Available page count `C0` = 192, totaling `300000` bytes |
| `34` | Page shift `E` = 16 KiB |
| `38` | Initial source frame 1 |
| `3C`, `40` | Linked allocation head/tail NONE |
| `44..60` | Eight source request-cost slots initially `7FFFFFFF` |
| `64` | Actual embedded pool pointer, allocator+70 |
| `68` | Original bytes `65 65 65 77` |
| `6C` | Original allocator vtable pointer `480D68` |

The embedded pool capacity is 512 records. The **available physical pages are 192**; do not merge those counts. Cache-entry pool valid 0 and embedded-page pool valid 1 are separately captured original results.

## Request, allocation and original queue contract

Actual equipped/registered primary SMG FP ready/full-reload events select `E5AC0437` / `E5AD0438`. Their unavailable `21F220` requests return NONE. Each copied actual A0 request remains an explicit input to the separate creation kernel. Original `129450` executes listener selection and returns local listener 0 / distance 0. No listener return is fabricated.

Original `127750(sound,listener,request,promotion_NONE)` creates source datum `EF730000` in this fixture, retaining real gain/pitch/permutation/RNG outputs. `129E00` installs the selected range/permutation/chunk. `1275B0` calls `21A960(sound,chunk,2)`:

1. If chunk `+8` is NONE and load bit1 is present with a valid owner, actual `21AB20` calls `13EAE0(allocator,size_flags&3FFFFFFF,0)`.
2. The original allocator rounds size to its page count using shift `E`; all stock sizes in these cases are positive authored values. It returns actual page-pool datum `E2780000` for the first allocation.
3. `21AB20` calculates destination `buffer + (page.offset << E)`, inserts the cache entry with matching datum via `16D390`, and publishes chunk `+8` and cache owner/chunk fields. It sets pin/reference/used to 0.
4. Original read size rounds to 512-byte alignment. `ECX=chunk.encoded_offset`; `19D8C0` receives six stack words `(aligned_size,destination,0,completion_pointer,5,4)`.
5. Original `19D8C0` selects location-2 file slot `55F798`, masks the offset to 30 bits, and calls `1A4870` with real provider file handle, size, offset, task field 5, priority 4 and byte-count-output NONE. Destination is in EDX and completion pointer in EBX at that helper.
6. `1A4870` initializes the original 40-byte task packet and increments its source accounting global `4E9EC0`. **It writes `*completion=0` itself at `1A48CD`**, before obtaining/enqueuing the real original node. That global is not an instantaneous in-flight count: the completed single-read cases retain value 1 and the two-read case retains 2 after work list becomes empty.
7. Original `121FD0` acquires the real free-list mutex, removes a node and releases the lock. `1A4870` copies its own task packet into node+8, installs callback/completion fields, and calls `1220E0`.
8. `1220E0` acquires the work-list mutex, invokes original `122020` insertion, produces task ID `(original_counter<<8)|node_index`, increments the counter, releases the lock and releases the real semaphore once. The two queued stock requests have equal priority 4 and retain insertion order.

The executed task node layout is:

| Node offset | Verified field in these original requests |
|---|---|
| `00` | Queue priority 4, read by original insertion |
| `04` | Original produced job identifier, first 0; second `101` |
| `08` | Real provider file handle |
| `0C` | Actual cache destination |
| `10` | Aligned requested bytes |
| `14` | Masked file offset, advanced by actual read |
| `18` | Optional cumulative byte-count output pointer, NONE here |
| `1C` | Cumulative bytes read, initially 0 |
| `20..2F` | Original task-packet zeros here |
| `30` | Original task field value 5 |
| `34` | Original callback `1A47D0` |
| `38` | Completion pointer, actual cache-entry+2 |
| `3C` | Work/free-list next pointer, source-produced |

The frozen cases do not cover the insertion helper's general multi-category arbitration or all resource-location branches. They preserve the actual queue inputs/outputs instead of inferring additional policy.

## Actual read and original completion

The original async thread is not started. The fixture directly invokes actual `122360` for **one iteration**, using real lock/semaphore services. It stops before the second semaphore wait, retaining the original infinite worker structure and exact explicit invocation boundary.

The worker acquires work, calls actual `node+34` with node+8, and actual `1A47D0`:

1. Reads remaining bytes = requested size minus actual accumulated count; limits the next read by current `[475970]` (image/default `40000` = 262,144 bytes in these cases).
2. Initializes the actual high seek word to 0 and calls the real seek provider.
3. Calls the real aligned file read with destination plus already-read bytes.
4. Advances actual task cumulative count and offset from the actual byte count, and writes its optional cumulative output if present.
5. Returns 1 once the request is complete, or on a short read; otherwise returns 0 for original worker retry. These stock cases complete within one callback. Retry/short-read/error behavior has an original listing but is not exercised by this freeze.

Only after callback return 1 does **original instruction `1223AC: mov byte ptr [eax],1`** write cache completion. No provider writes this flag. Original `122150` then removes work; original worker returns the node to the free list under its real mutex. Two ready resources use two explicit worker invocations and two real reads; shared reload uses one.

| Kernel-selected sound/permutation | Original chunk VA | Shared-map offset | Authored bytes | Aligned bytes | Pages | Aligned-read SHA-256 |
|---|---|---|---:|---:|---:|---|
| Ready / 1 | `8050A60C` | `04C46400` | 53,712 (`D1D0`) | 53,760 (`D200`) | 4 | `e2fe51cdaea5325bd4b96b5cf2509de11c52140278f6b6428dab1b0e097d9027` |
| Repeated ready / 0 | `8050A600` | `04C39200` | 53,712 (`D1D0`) | 53,760 (`D200`) | 4 | `3b0eee4804bf43eca4f109f2a1fb3b5f34bc7cb7b81cdba7c3ea3d50e3ceb99c` |
| Full reload / 0 | `8050A618` | `04C53600` | 32,760 (`7FF8`) | 32,768 (`8000`) | 2 | `3977d794e728cd0498774047b912ae84acaa9b1d2bcee49b05aa1fff36dc98e9` |

The first destination is `82E45000`; the second ready allocation is `82E55000`. All authored and padded read hashes are verified from the actual file. Codec bytes have not been decoded. A completed cache entry is not a playing voice.

## Pin, ready and eviction behavior measured

Original cache entries are 16 bytes: `+0` salt, `+2` completion byte, `+3` used byte, `+4` pin count, `+5` reference count, `+8` owner sound datum, `+C` original chunk pointer. Page records are 24 bytes: `+0` salt, `+4` page count, `+8` page offset, `+C/+10` links, `+14` last touch frame.

`21A960` flags: bit0 waits for read completion, bit1 initiates load, bit2 increments pin. Result bits: 1 pending, 2 ready, 4 pinned. These cases execute nonwaiting queries; the waiting `SwitchToThread` branch is not substituted with success. Every resident query updates the original page touch frame. A ready query sets used `+3=1`; the worker's completion alone leaves used 0.

`128030` examines source **byte** `+4 bit1`. If clear, source sets it and calls `21A960(NONE,chunk,4)` once. If already set, it leaves the pin unchanged. `1280B0` decrements the actual selected entry's pin and clears only that source bit. Other source flag bits remain unchanged. Do not treat a repeated source pin guard's full EAX as a defined return value.

The original busy callback `21AAB0` returns **AL** 1 if pin `+4` is nonzero, reference `+5` is nonzero, or completion `+2` is zero; otherwise AL 0. High EAX bits are not a defined boolean. Consequently:

- Pending and pinned: busy 1.
- Pending after original unpin: still busy 1 because actual read is incomplete.
- Loaded and pinned: busy 1.
- Loaded after original unpin, no references: busy 0.
- Two shared reload sources: pin 2 after creation; releasing the first leaves pin 1/busy 1; releasing the second leaves pin 0/busy 0. Only one real read is queued.

`21A920` advances original page frame and resets request-cost slots; it needs no platform clock result. Both lifecycle cases exercise an explicitly supplied `300000` full-capacity allocation at priority 0. Pending and loaded-but-pinned cases return NONE. After actual completion, original unpin, and source frame advancement, actual `13EAE0` invokes `13EFA0 -> 21AAF0` to remove the eligible old entry. Original `21AAF0` writes the **original chunk `+8=NONE`**, frees the cache-entry datum via `16D4C0`, and the allocator repairs its page list/free state before creating the full-capacity page record. Result is actual `E2790000`, page count `C0`, offset 0, touch frame 4.

The capacity request is an explicit allocator boundary probe, not an authored sound request. General policy-2 candidate scoring, other request priorities, multiple-entry eviction ties, frame-counter overflow, references/decoder consumers, and teardown remain separate coverage needs. Directly calling the deletion callback would skip the busy/admission checks; this freeze exercises it through the original allocator.

## Minimum supported native handoff and next gate

A bounded native cache consumer can now compare source-produced constructors, salted cache/page entries, real selected resource sizes, queue descriptors, actual file completion, pin guards, busy checks and these allocation/eviction transitions against this fixture. Its host provider must own real storage/file/queue objects; only actual completed reads can enter the ready branch. Source datums and callback objects remain separate from host handles.

Retain these inputs and limits:

- Actual original source request and direct kernel/worker/probe invocation boundaries; original ordinary unavailable request is still NONE.
- Existing raw `ugh!` mutable range/permutation input bytes and local RNG seed 0 remain the prior explicit fixture inputs, not a recovered full loaded-map initialization policy.
- Static arena/class backing and inherited world/control/pose/service boundaries are recorded; no full original process startup is claimed.
- Verified shared-map binding is supplied; ordinary original file-registry/map activation producers remain separate.
- Genuine source clock stops, unavailable original async thread creation, driver pointer 0 and initialized 0 remain intact.

Successful source virtual voice admission, owner assignment/removal callbacks, decoder/cache reference lifecycle, real stream consumption, pause/flush/error outputs, original gain/pitch driver translation, and final availability commit are still required before audible parity. The [prior scheduler contract](SMG_AUDIO_ADAPTER_1_1_RESEARCH.md) retains that source ordering. Nothing in this cache freeze replaces it with generic native play-first-permutation behavior.
