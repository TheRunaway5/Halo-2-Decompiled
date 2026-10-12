# Original 1.1 device prefix after the compatibility lock gate

The independently frozen extension resumes the **actual `126B20` source
frame** at its `2205C0` device-constructor request. Original `3804DE` completes
its lock gate and returns1 using the previously measured primary compatibility
critical-section service. The next actual request is **kernel ordinal15,
`ExAllocatePoolWithTag(48,626F5344)`**, at original call site383ED5. Execution
stops before that service executes; no pool/device/capability result is supplied.

The original driver is allocated/zeroed, the original DirectSound global is
stillNULL, the sound initialized byte stays0, and the acquired lock remains
owned at the stop. This is progress in the original constructor prefix, **not
a completed DirectSound or audio initialization**. Earlier startup and RNG
freezes remain byte-identical; no production files are edited.

## Frozen evidence and independent repeat

| Artifact | SHA-256 |
| --- | --- |
| `tools/oracle_smg_audio_device_prefix_1_1.py` | `5313c91bd67a63aaf4aa3807dfd51fad01460e7dafa5503713cf107f17a81513` |
| `tools/audit_smg_audio_device_prefix_1_1.py` | `26853266f72db9e169b8d9f020fb1ee7d27661809ce5a8f4543e8f6983221f08` |
| `analysis/multiplayer/map-pack/smg-audio-device-prefix/frozen/suite.json` | `d81a75b09f83a72b41a1caad2fac4a4855525c1ea63c98fd0a31c344cb1ef348` |
| `analysis/multiplayer/map-pack/smg-audio-device-prefix/source-fixture.json` | `571503ddd26d2d0fa0afae23b0f5328964e3d460f7c8bef7eda36e29b8e6fa40` |
| `analysis/multiplayer/map-pack/smg-audio-device-prefix/audit.json` | `acf1b4a72afd6b37e30a080c27383bcdc816072fb56e9d7015c784d42acbbcf1` |
| Measured report | `bc8bf91e1004bb6a36e23fc1c3413148b68d5316c3ae903d098b2e5c3432fc21` |
| Independent replay report | `f6087bb54848f5dd975d78ba02ea1af39cb59b6892dac034c45a4c53c0570018` |

The original map-pack 1.1 `default.xbe.03` full-image SHA-256 is
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
The auditor verifies 14 listings/998 original instruction rows, including all
106 original addresses executed after resumption, against unchanged original
image bytes. It also verifies all62 prior pins. A fresh source instance
reexecutes the same preparation and original frame with the recorded whole
context/clocks, matching the source fields, request, ownership and executed
instruction listings. Replay is not a second live clock/thread measurement.
[Full fixture](../analysis/multiplayer/map-pack/smg-audio-device-prefix/source-fixture.json),
[audit](../analysis/multiplayer/map-pack/smg-audio-device-prefix/audit.json),
[suite](../analysis/multiplayer/map-pack/smg-audio-device-prefix/frozen/suite.json).

For another independent execution, write a new directory:

```sh
.tools/python/bin/python tools/oracle_smg_audio_device_prefix_1_1.py \
  --output analysis/multiplayer/map-pack/smg-audio-device-prefix/check/report.json \
  --replay analysis/multiplayer/map-pack/smg-audio-device-prefix/frozen-measurement/report.json \
  --context-replay analysis/multiplayer/map-pack/smg-audio-device-prefix/frozen-measurement/raw-host-context.json
```

## Produced context and preserved caller order

The oracle inherits and verifies the frozen original heap writer, CRT32CFFC and
3274E4, physical/game-arena fragments,1479B0 game RNG and actual126B20 sound
constructor. Raw host mmap/mprotect and measured UTC/boot clocks are provider
services; original source instructions create the heap, CRT/cache/source pools
and seed state. The pinned compatibility generator's complete records are
transferred without relocation or selected-field patches. In live measurement,
its genuine Wine32 worker remains alive. The context remains a compatibility
host-thread branch, distinct from ordinary Xbox/title-thread TLS/hardware.
[Separate exact preparation](SMG_AUDIO_RNG_STARTUP_1_1_RESEARCH.md),
[whole context provenance](MP_COMPLETE_COMPAT_KPCR_RESEARCH.md).

Actual126B20 first runs cache21A6E0 and source-pool construction, then
controller21BC50,21C1D0,21F5A0 and21FEB0. It reaches source call126C90 to
2205C0, with original return126C95 already on its actual stack. The earlier
RNG study stopped at this entry without supplying a return. This extension
removes only that observer and resumes **the same original frame**. It does
not replace the actual caller with a direct independent2205C0 invocation.
The source policy pointer451220 is already on the original stack.
[Retained frame and original requests](../analysis/multiplayer/map-pack/smg-audio-device-prefix/frozen-measurement/report.json),
[original driver producer](../analysis/multiplayer/map-pack/smg-audio-device-prefix/frozen-measurement/instructions/002205c0.asm).

Original2205C0 allocates driver2AD8 from the source sound arena and zeroes it,
publishing `526474 = 0A01B7BC` at220615. That actual position includes all
preceding sound constructors. It differs from the earlier *independent* driver
probe after an incomplete controller constructor. Its output field2AB0 isNULL;
every byte of the retained2AD8 is still zero at the current stop.

## Completed source guard, with explicit compatibility operations

Actual220626 calls383DD7 with device arguments
`DirectSoundCreate(NULL,&driver[2AB0],NULL)`. Original383DD7 calls3804DE.
Its `FS:[24]` read observes the complete produced compatibility IRQL0 and takes
the source branch to `RtlEnterCriticalSection(38D158)` at3804F2, through import
417BE0 (`80000115`, ordinal277). The full static original28-byte critical
section is:

```text
010004000000000060d1380060d13800ffffffff0000000000000000
```

This record comes unchanged from the executable; no new initializer writes
its fields. The previously pinned primary Cxbx Rtl body executes using actual
host-serialized atomic operations and the complete source-produced ETHREAD
ownership pointer. The helper's legacy log name `opaque_current_real_host_thread`
does not mean an invented token: inherited ContextTrace.token reads the actual
produced KPCR/ETHREAD pointer. It is distinct from the separately recorded host
thread ID and the original source GetCurrentThreadID field.
[Original wrapper and guard](../analysis/multiplayer/map-pack/smg-audio-device-prefix/frozen-measurement/instructions/00383dd7.asm),
[guard instructions](../analysis/multiplayer/map-pack/smg-audio-device-prefix/frozen-measurement/instructions/003804de.asm),
[pinned primary compatibility lock implementation](MP_HOST_THREAD_LOCK_ORDER_RESEARCH.md).

Actual atomic increment changes lockcount`FFFFFFFF -> 0`. The compiled
compatibility body stores owner`00AD0000` (the measured whole ETHREAD pointer)
and recursion1. The original3804F8/3804FA instructions then return EAX1. This
**source guard result** is produced by original instructions after actual
compatibility lock completion; it is not a supplied DirectSound success.
The after-record is:

```text
010004000000000060d1380060d1380000000000010000000000ad00
```

The raw device store log also records the two compiled compatibility stores at
0940004B/0940004E. The independent fixture explicitly splits them into
`compatibility_body_stores`, separate from `source_original_stores`. They are
not counted as original Xbox instructions. The retail kernel Rtl implementation
and its hardware timing remain outside this compatibility measurement.
[Exact classified writes](../analysis/multiplayer/map-pack/smg-audio-device-prefix/source-fixture.json).

## Next request and retained stop

The executed original call sequence after resumption is:

```text
126B20 caller frame
  -> 2205C0 source driver constructor
  -> 383DD7 DirectSoundCreate wrapper
  -> 3804DE guard, returns1
  -> 383D6A source device allocator
  -> 3802BF request2C through383F92
  -> 383ECC pool request
  -> [417CFC] = 8000000F: STOP before kernel service
```

Source383D6A reads shared DirectSound global38D7D8=NULL and requests2C bytes
with original allocation helper3802BF. Source383F92 finds private pool38D74C
NULL and takes its unchanged fallback: add4 bytes for its pool marker, then
call383ECC with requested30h, tag626F5344, zero flag1. At **383ED5**, the
actual kernel request is:

| Operand | Captured value |
| --- | --- |
| Target | `8000000F`, ordinal15 |
| Return site | `383EDB` |
| Size argument | `00000030` =48 bytes |
| Tag argument | `626F5344` |

Primary nxdk's kernel header gives this two-argument allocation ABI and nullable
pointer result; its exact saved bytes are pinned at SHA-256
`988e679c3e042bb9f0c3836a77d3aee24117e83683da9cf839ba2a84c7fe32fb`.
It is a primary compatibility declaration, not proof of an Xbox pool allocator's
capacity/rounding behavior.
[Original pool request](../analysis/multiplayer/map-pack/smg-audio-device-prefix/frozen-measurement/instructions/00383ecc.asm),
[original fallback/marker producer](../analysis/multiplayer/map-pack/smg-audio-device-prefix/frozen-measurement/instructions/00383f92.asm),
[primary nxdk ABI](https://github.com/XboxDev/nxdk/blob/14d5ee97e73347c973f1f57b68b79ec08c9e77f2/lib/xboxkrnl/xboxkrnl.h).

No allocation result is supplied. Consequently original380772 device record
construction,383C31 device initialization, driver caps/effects/voices/streams
and guard release are not reached. Shared device38D7D8 and driver2AB0 stayNULL;
the source sound initialized+78 remains0. Source allocation-statistics pointer
38D804 is alsoNULL in this retained prefix. Even after a legitimate future raw
pool provider, its actual owner/initializer must be recovered before assuming
later383EEE statistics writes are valid. It is never filled to force admission.
[Full retained source state](../analysis/multiplayer/map-pack/smg-audio-device-prefix/frozen-measurement/report.json).

The current stop retains actual acquired lock ownership, source frame and
partial driver allocation. No unwind/release/cleanup is simulated. The repeat
starts from a fresh original instance with the same recorded input context,
so it does not reuse a held lock from the first stopped invocation.

## Minimum next provider boundary

A native storage provider may own actual allocated raw bytes and report its
real allocation and size under the recovered ABI. It must let original source
produce the device-record fields; an SDL deviceID is not that source output
pointer. Original383ECC subsequently requests allocation size through417D4C,
ordinal23, updates the source statistics owner and zeroes the returned block;
383F92 writes pool marker6C6F6F70 and returns pointer+4. Those post-return
operations are listed but **not executed** here. Real pool ownership/size and
the statistics/private-pool lifecycle are the next gates.

Later original device initialization requires original hardware/interrupt/DSP
services, including inspected writes to Xbox audio MMIO such asFE801000. They
cannot be replaced by invented success to declare SDL compatible. A genuine SDL
adapter must retain supported source scheduling/resource ownership separately
from its actual host device, capabilities and consumption. No current evidence
establishes those later services, audible playback or full initialization.
[Original later device initializer, zero executed rows](../analysis/multiplayer/map-pack/smg-audio-device-prefix/frozen-measurement/instructions/003858d8.asm),
[earlier exact stream/voice/SDL interface boundary](SMG_AUDIO_STARTUP_1_1_RESEARCH.md).

The completed boundary is specifically the original source guard using the
stated compatibility context/service. The next unsupported original kernel
request, retained producer fields and matching independent execution are
frozen. `native_gameplay_update_applied:false` remains the research status.
