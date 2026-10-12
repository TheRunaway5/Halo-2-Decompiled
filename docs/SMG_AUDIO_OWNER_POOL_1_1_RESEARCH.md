# Original 1.1 sound-owner allocator and unvoiced removal

This separate experiment executes the original owner-pool constructors and the
complete original `128A70 → 18E2F0 → 128AE0 → 16D4C0` removal path for two
cache-created, unvoiced sources. It supplies real raw memory, clock, lock,
semaphore, and file services at declared provider boundaries. It supplies no
allocator object, owner validity, driver, voice, callback return, initialized
flag, or resource-ready result to these new consumers.

**The owner lookup is the invalid-datum branch.** The frozen original FP
requests name `10002`, while the inherited object-header fixture has high-water
2. `BBCB0` rejects index 2 before reading any selected header, salt, object type,
or object pointer. `D2270` does not execute. A valid weapon-owner or unit-owner
cleanup remains a separate gate. The earlier exploratory shorthand about a
weapon type-mask rejection is corrected by the captured high-water predicate.

The original driver and thread boundaries remain stopped. This experiment
does not establish complete sound-map initialization, ordinary scheduler
admission, playback, packet consumption, or audible output. Production and all
82 prior frozen artifacts are unchanged. Every new report retains
`native_gameplay_update_applied:false`.

## Authority and retained inputs

The source is the user's map-pack-disc 1.1 executable
`assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
Lockout is `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`;
shared.map is `084c817fa82bb9bc416d9ba847cec05be431abc419e3a1087cdf211fbf47f5af`.
No later title-update version is inferred.

The independent oracle is `tools/oracle_smg_audio_owner_pool_1_1.py`. It imports
the unchanged `RngTrace` and cache-provider methods. The original heap/CRT/game
arena preparation is retained from [the RNG freeze](SMG_AUDIO_RNG_STARTUP_1_1_RESEARCH.md)
and [the fixed-memory freeze](MP_FIXED_MEMORY_WORLD_RESEARCH.md). The source
startup callback table is inspected rather than executed as a full dispatcher.
Row 46 at `447AA0`, stride 36, contains constructor `18A200`, disposal `18A280`,
map reset `18A2A0`, and subsequent lifecycle callbacks. The new entry calls are
bounded caller inputs in the recorded order, not proof of all normal startup
callbacks or their complete ordering.

Whole KPCR, ETHREAD, native stack, and static process records come unchanged
from the pinned Wine32 compatibility producer. Their pointers and fields are
not relocated or patched. This is compatibility-produced context, not ordinary
Xbox thread/TLS initialization. Measured UTC and boot-time clocks are host
service inputs; independent execution replays those exact inputs and whole
context. Both cases consume five clock requests: FILETIME/tick twice in startup,
then a real tick request in the sound-map reset. There is no invented RNG seed.

The inherited object/pose/game fixtures and unavailable animation, transport,
object-storage, effects, and audio services are retained and listed verbatim in
each case. They are not upgraded to ordinary live world producers. The newly
consumed allocator is overwritten by the owning original constructor before
use. Inherited audio-request bytes are the unchanged, originally rejected
ready/reload requests from the prior [host/ownership freeze](SMG_AUDIO_HOST_CONTRACT_1_1_RESEARCH.md).

## Exact allocator owner

`125280` runs with the source-created frame. The original call to `217090`
savegame/thread I/O at `1252BA..1252BF` remains excluded; execution resumes the
unaltered frame at `1252BF` and reaches its original return. No callee success
is inserted. The source clears the arena and publishes its allocation state.
Full startup's unrelated allocation order remains unproved.

| Instruction / field | Executed owner and meaning |
| --- | --- |
| `125328`, `[51841C]` | Publishes the original four-byte allocator object |
| `125330`, object `+0` | Stores original vtable `45A610` |
| `45A610 +0` | Original method `125E50` |
| `125E50` | Rounds `(request+3)&FFFFFFFC`, returns arena+used, advances used, invokes `1656A0` with rounded size |
| `[4ED650]`, `[4ED654]`, `[4ED65C]` | Source arena pointer, used-byte counter, allocation checksum |

In this recorded backing/layout, arena is `82D47000`, allocator object
`82D48288`, and source RNG `82D4828C`. Startup finishes with used `1294`.
Those are recorded addresses from the explicit compatibility backing, not Xbox
RAM addresses or native constants to copy. The exact owning stores and later
virtual call receiver are checked by the auditor.

## Pool and map-reset prefix

After the genuine sound-map prefix and class-fade constructor/reset, used is
`15F4`. `18A200` calls the source allocator virtual method with **`60CC`** bytes.
It invokes untouched `16D070` with name `"object looping sounds"`, capacity
`400` (1024), stride `18` (24), alignment exponent 0, and the actual allocator.
It ORs bit 2 into header flags `+2A`, publishes `[4F485C]`, then allocates `244`
state bytes directly from the same arena with original checksum update and
publishes `[4F4858]`.

| Retained field | Measured source result |
| --- | --- |
| Owner pool | `82D485F4`, complete `60CC` bytes retained |
| State | `82D4E6C0`, complete `244` bytes retained |
| Arena used after constructor | `7904` |
| Pool `+20/+24` | Capacity 1024 / stride 24 |
| Pool `+30` | Actual allocator pointer `82D48288` |
| Pool `+44/+48` | Data at pool+`4C`; bitmap at pool+`604C` |
| Pool bitmap | 128 bytes, zero |
| Pool valid `+29` | Constructor leaves 0; original `18A2B5` writes 1 |
| Pool `+34/+38/+3C` | Reset first-free/high-water/count all 0 |

Original `18A2A0` executes `16D220` and then writes its state:

- `+000..+01F`: eight DWORD `FFFFFFFF` entries.
- `+020/+024`: zero; `+028..+127`: 64 zero DWORDs.
- `+128..+227`: 64 DWORD `FFFFFFFF` entries.
- `+228/+22C/+230/+234`: float `3F800000` from original `465030`.
- `+238`: `1F40` (8000); `+23C`: float zero.

The whole raw state also preserves `+240` from the prior original arena clear;
this function does not assign a meaning or store to that final word.

The map reset stops **before** the absent driver store in `2230E0`. Its
`18A790` tail has not executed. In particular, the later preference access
through `[4F10B8]+C14` is not bypassed or asserted available.

## Genuine stops and services

| Reached boundary | Retained dependency |
| --- | --- |
| `126B20 → 2205C0` | Original audio-device constructor remains unprovided |
| `126DE0 → 2217E8` | Driver `[526474]==NULL`; original pending store targets driver `+2A0C` |
| `1221B0 → 2D8606` | Original asynchronous CreateThread is unavailable; no source thread handle/return |
| `18A2A0 → 22310C` | Driver remains NULL; original gain store targets `+2ABC` |

The first sound-map attempt executes its real clock request and original cache
and sound metadata initialization before reaching the driver stop. The owner
map prefix is independently entered and stops at its genuine later dependency.
These partial prefixes are not normal map activation return paths.

Raw storage uses actual host allocations and access changes. File service opens
verified shared.map; its real provider handle is bound to `55F798` as an
explicit file-registry boundary. The original file-registry producer is not
executed. Source async initialization creates lock/semaphore requests backed by
actual host synchronization objects, and stops before its thread request. A
single original `122360` worker iteration is explicitly invoked when needed;
its second semaphore wait ends the slice. This does not prove asynchronous
thread scheduling or Xbox I/O latency.

The saved `XC_AUDIO` request, boot AV-pack state, MMIO, driver capabilities,
DirectSound/SDL playback, voice ownership, and packet consumption remain
separate unavailable boundaries from the prior immutable freezes. No partial
device pointer or preference is copied into this trace to admit it.

## Complete unvoiced removal under the retained owner boundary

The actual rejected FP request is copied byte-for-byte into separately declared
raw caller storage `0B100000` (the old request address overlaps this inherited
resident resource arena). Original `129450` computes the listener, returning
0 in these cases. Original `127750` is explicitly invoked at a supplied kernel
call site with that listener and original request bytes. It creates actual
source datum `EF730000` and requests the real cache chunk through `21A960`.
Ordinary available-sound admission did not execute or become successful.

Both newly created sources retain voice `+AC=FFFF`, controller `+A8=FFFFFFFF`,
callback table `+14=44B14C`, and owner `+10=10002`. The table's `+10` entry is
original `18E2F0`. Ready is removed while its resource is pending; reload is
removed after the original worker completes an actual aligned read.

The exact removal order in both traces is:

1. `128A70`, source in EAX, explicit reason 3. Voice-closure branch is skipped
   because original source `+AC` is `FFFF`.
2. `1280B0` resolves the source's selected authored chunk, decrements the actual
   cache pin byte from 1 to 0, and clears source flag bit 1.
3. Callback `18E2F0(owner=10002, source=EF730000, reason=3)` reads the newly
   source-produced valid owner pool.
4. `BBCB0` sees owner index 2 and inherited object-header high-water 2, takes
   `BBCED`, and returns NULL. It never reaches salt/type/pointer validation.
5. Callback returns normally; `128AE0` sees no controller and tail-calls
   `16D4C0`. The source pool's first-free/high-water/count become zero, its
   bitmap clears, and the freed record's salt becomes zero. `128A70` returns.

The owner pool/state and source-owned RNG bytes are unchanged by removal.
No voice/device method or callback result is supplied. The freed record's
remaining bytes are retained exactly rather than assumed cleared.

The ready request's pending read is then serviced through original `122360`
after full removal. Actual shared.map read is 53,760 bytes at offset 79,925,760.
Reload's read is 32,768 bytes at offset 80,033,280 before removal. Both are
512-byte aligned and byte hashes match the file. The provider writes file
bytes and its real read length; original `1223AC` publishes completion. Source
cache ownership remains separate from source-record lifetime.

For a valid owner, untouched `BBCB0` additionally requires non-NONE datum,
in-range index, nonzero matching signed salt, and `(mask & (1 << (type&31)))`.
Callback mask is 3. Its non-NULL path would call `D2270`, which compares a unit
sound-slot definition with source `+C` and conditionally clears it. Those
branches are inspection only here. **They are not covered by these two complete
invalid-owner removals.**

## Independent reproduction and native scope

Run the measurement with the pinned already-built compatibility generator; it
does not rebuild or modify that frozen executable. Then repeat in a fresh
process with the exact captured clocks and context. Use a new directory so the
frozen measurement and reports stay unchanged:

```sh
audio_repro=$(mktemp -d)
.tools/python/bin/python tools/oracle_smg_audio_owner_pool_1_1.py \
  --output "$audio_repro/measured/report.json"
.tools/python/bin/python tools/oracle_smg_audio_owner_pool_1_1.py \
  --output "$audio_repro/reexecution/report.json" \
  --replay "$audio_repro/measured/report.json" \
  --context-replay "$audio_repro/measured/raw-host-context.json"
# Verify the retained frozen evidence; this does not rewrite its measurement.
.tools/python/bin/python tools/audit_smg_audio_owner_pool_1_1.py
```

The audit compares complete source outputs, all constructor/reset/removal
buffers, clocks, original call/store order, actual file bytes, and unchanged
instruction bytes in both runs. Only explicitly identified provider/context
mode labels and actual host allocation addresses are excluded from the source
comparison. Dependency stops and replaced provider entry instructions are
excluded from the executed-instruction listing; separate static listings retain
those instructions for inspection.

A native consumer may use the verified original allocator/pool layout, reset
stores, unpin/finalize ordering, and **invalid-owner** callback branch within
their declared input boundaries. Valid-owner unit-slot clearing, actual looping
owner records, controller/voice ownership, successful full map activation and
ordinary audio scheduling remain unproved. This does not authorize setting a
driver/ready flag or replacing the source scheduler with generic playback.
