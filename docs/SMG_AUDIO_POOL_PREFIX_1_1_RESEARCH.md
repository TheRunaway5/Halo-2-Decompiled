# Original 1.1 statistics owners and host-backed device pool prefix

This independent extension executes the original owner of allocation-statistics
pointer **38D804**, then resumes the actual126B20 sound-constructor frame with
genuine host-backed pool allocations. Original383ECC updates the statistics
and clears each returned block;383F92 writes its marker;380772 constructs the
first DirectSound record. The next request stops before **kernel24,
ExQueryNonVolatileSetting(XC_AUDIO, ...)**. No settings value, DirectSoundCreate
success, device capability, initialized flag or playback result is supplied.

The initial device-record producer completes, but the device initialization and
parameter constructor remain incomplete. The driver output and audio
initialized byte stay zero. These are bounded source/compatibility results;
they do not establish retail Xbox pool rounding, full startup or audible SDL
playback. All prior freezes and production remain unchanged.

## Frozen artifacts and reproduction

| Artifact | SHA-256 |
| --- | --- |
| `tools/oracle_smg_audio_pool_prefix_1_1.py` | `381b1763cdffb9cbc7b73c25c983697597284d29ac7e4847bd2f092f339906db` |
| `tools/audit_smg_audio_pool_prefix_1_1.py` | `4456ca628d3978ed7dfe65b4975a723ba73b5c8adad87fe951aa3fc8c9b84964` |
| Measured report | `b5e6cb66198d911200bf1a4abd943f1182619c9276250a6fe1a2bfec310fb945` |
| Independent replay report | `d941b50918f1b5751271018cf2ada66574d3c67d302a4cac5fff9219a5fcd8f2` |
| Frozen suite | `2e2c0844a4538f2b818e95f5179b2b8f4abb8af7c9928f10885cb7dcfee2959a` |
| Source fixture | `26efeabc982e22a8975842267cc0a936c35e3c0d6832b49b1619f8773c4bf140` |
| Audit | `ba75bdc8679340a55b20813c6386bc109c9b9840e6d390275e7cbdb46e4cf6ed` |

The source is map-pack1.1 default.xbe.03, full-image SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
The auditor verifies 26 original instruction listings/2894 rows, including all
1857 executed original addresses from the preparation and resumed prefix.
It verifies four original initializer stores, two actual host allocations,
two source size queries, all2048 original zeroing DWORD stores, both markers,
device/partial-parameter fields, the exact new request and all68 prior pins.
A fresh original machine with actual fresh host storage repeats the result
with recorded whole context and measured clock inputs. This repetition is not
a second live clock/thread measurement.
[Suite](../analysis/multiplayer/map-pack/smg-audio-pool-prefix/frozen/suite.json),
[source fixture](../analysis/multiplayer/map-pack/smg-audio-pool-prefix/source-fixture.json),
[audit](../analysis/multiplayer/map-pack/smg-audio-pool-prefix/audit.json),
[measured report](../analysis/multiplayer/map-pack/smg-audio-pool-prefix/frozen-measurement/report.json),
[independent report](../analysis/multiplayer/map-pack/smg-audio-pool-prefix/frozen-reexecution/report.json).

Write to a new directory to independently repeat the source slice:

```sh
.tools/python/bin/python tools/oracle_smg_audio_pool_prefix_1_1.py \
  --output analysis/multiplayer/map-pack/smg-audio-pool-prefix/check/report.json \
  --replay analysis/multiplayer/map-pack/smg-audio-pool-prefix/frozen-measurement/report.json \
  --context-replay analysis/multiplayer/map-pack/smg-audio-pool-prefix/frozen-measurement/raw-host-context.json
```

The frozen suite is audited with:

```sh
.tools/python/bin/python tools/audit_smg_audio_pool_prefix_1_1.py
```

## Actual statistics owner and bounded initialization order

These four original11-byte routines were present in the executable, despite
being absent from the Ghidra exported function inventory. Each consists of a
literal original `mov dword ptr [owner],target` and `ret`:

| Original C++ table word | Original routine | Written owner | Produced storage |
| --- | --- | --- | --- |
| 468724 | 38410A | 38D804 | 38D120 |
| 468728 | 384115 | 38D800 | 38D124 |
| 46872C | 386EE8 | 38D8B0 | 38D128 |
| 468730 | 386EF3 | 38D8AC | 38D12C |

Original38D120..38D12F are16 zero bytes. The routines produce pointers into
that original storage; the provider never writes a statistics pointer or
counter. Original383ECC uses38D804 for pool-byte accounting, while the listed
383F2B contiguous-allocation routine uses38D800. The two remaining owner
stores are retained without asserting their wider runtime meaning.
[Original38410A](../analysis/multiplayer/map-pack/smg-audio-pool-prefix/frozen-measurement/instructions/0038410a.asm),
[384115](../analysis/multiplayer/map-pack/smg-audio-pool-prefix/frozen-measurement/instructions/00384115.asm),
[386EE8](../analysis/multiplayer/map-pack/smg-audio-pool-prefix/frozen-measurement/instructions/00386ee8.asm),
[386EF3](../analysis/multiplayer/map-pack/smg-audio-pool-prefix/frozen-measurement/instructions/00386ef3.asm).

Original cinit2D80FD first processes the C initializer table
468CBC..468CD4, then the C++ initializer table468630..468CB8; both endpoints
are exclusive. Each table is visited in ascending address order, skipping
NULL andFFFFFFFF. The actual C++ dispatch begins at2D813E and calls the table
word at2D8149. The extension executes precisely2D813E..2D814B separately for
the four adjacent words above, with ESI selecting the actual table word and
EDI retaining the original table end468CB8. Each call executes the unchanged
owner routine. The stop is before source increment2D814B.
[Original dispatcher](../analysis/multiplayer/map-pack/smg-audio-pool-prefix/frozen-measurement/instructions/002d80fd.asm),
[original table bytes and per-owner transitions](../analysis/multiplayer/map-pack/smg-audio-pool-prefix/source-fixture.json).

This is a bounded table-dispatch fixture, not full cinit. The inherited original
heap writer,32CFFC CRT-lock initializer and3274E4 C onexit initializer execute
before these four C++ entries. The inherited source game-arena fragments,
1479B0 and actual126B20 follow. Omitted C/C++ entries, the excluded game-arena
savegame call and ordinary full-title allocation/RNG history remain unproved.
The complete compatibility worker/KPCR/ETHREAD bytes are transferred unchanged;
its Wine worker stays alive during live measurement. The compatibility branch
is distinct from normal Xbox thread/TLS/hardware initialization.
[Separate exact RNG/startup boundaries](SMG_AUDIO_RNG_STARTUP_1_1_RESEARCH.md),
[whole compatibility-context provenance](MP_COMPLETE_COMPAT_KPCR_RESEARCH.md).

## Genuine raw storage provider and original consumers

The provider handles only original ExAllocatePoolWithTag and
ExQueryPoolBlockSize requests under the recovered two-argument and one-argument
kernel ABIs. For each source allocation it creates an actual anonymous private
read/write host mmap, stores the returned real host address, maps it into the
oracle through an explicitly declared guest alias, and retains the live block.
It records the source allocation tag without interpreting it. The query
reports that block's actual mapped capacity. No heap/device record, pointer
owner, statistics value, flag or ready state is written by this provider.

The host mapping uses actual4096-byte page granularity. The original requests
are48 and184 bytes; both mappings have real capacity4096. This is a legitimate
raw compatibility storage service whose capacity is measured, **not a claim
that retail Xbox ExQueryPoolBlockSize rounds either request to4096**. The
provider's guest aliases0B000000 and0B001000 are fixture address routing;
the actual native addresses are independently measured on each execution.
Initial raw zero contents are the OS's anonymous-mapping output, retained by
hash; no supplied poison/constructor pattern is inserted.
[Measured provider requests](../analysis/multiplayer/map-pack/smg-audio-pool-prefix/frozen-measurement/report.json),
[primary nxdk kernel declarations](https://github.com/XboxDev/nxdk/blob/14d5ee97e73347c973f1f57b68b79ec08c9e77f2/lib/xboxkrnl/xboxkrnl.h).

The primary header's saved SHA-256 is
`988e679c3e042bb9f0c3836a77d3aee24117e83683da9cf839ba2a84c7fe32fb`.
It establishes the declaration/ABI and XC_AUDIO index; it does not establish
retail allocation policy. The original source consumers execute as follows:

1. Original3802BF requests2C bytes through383F92; the actual private pool
   38D74C isNULL, so source383F92 takes its fallback and adds4 bytes.
2. Original383ECC calls kernel15 from383ED5 with size30h and tag626F5344.
   The provider returns actual live raw block alias0B000000.
3. At383EE2, original383ECC calls kernel23 to query that exact block.
   It receives actual host capacity4096. At383EEE it adds that amount through
   original owner38D804=38D120, producing counter0→4096.
4. With the original zero flag1, original383F03 issues1024 DWORD stores over
   the entire queried capacity. The executable owns this clearing despite
   the raw OS mapping already being zero. The zero residual byte loop has
   count0 for this measured capacity.
5. Original383FEB writes marker6C6F6F70, raw little-endian bytes `pool`, at
   the allocation base. Original383FF1 advances4 bytes and returns0B000004.
6. Original380772 completes the2C device-record constructor and publishes
   global38D7D8. Original383C31 then requestsB4 bytes for its parameter record;
   source fallback again adds4, requestingB8h. Actual raw alias0B001000 is
   queried/cleared in the same source path, and source counter becomes8192.
   Source client pointer0B001004 enters3832AA.

[Original allocation/query/zeroing](../analysis/multiplayer/map-pack/smg-audio-pool-prefix/frozen-measurement/instructions/00383ecc.asm),
[original marker/fallback](../analysis/multiplayer/map-pack/smg-audio-pool-prefix/frozen-measurement/instructions/00383f92.asm),
[complete source store stream and raw retained blocks](../analysis/multiplayer/map-pack/smg-audio-pool-prefix/source-fixture.json).

No original ExFreePool, contiguous-allocation service, private-pool constructor
or cleanup executes here. Their listed routines have zero executed rows and
remain separate lifecycle/hardware gates. The actual counter total8192 is
derived from this host provider's capacity, not a retail byte-count constant
to import into native gameplay.

## Source-produced records and unfinished caller

Original380772 writes the device record at client pointer0B000004:

| Field relative to client pointer | Original produced value |
| --- | --- |
| +0 | vtable41D760 |
| +4 | 2 |
| +10 | self pointer0B000014 |
| +14 | self pointer0B000014 |
| shared global38D7D8 | client pointer0B000004 |

All other bytes of the retained2C remain zero. These are source constructor
outputs, not an SDL device handle or a successful DirectSoundCreate result.
Original DirectSoundCreate383DD7,383D6A,383C31 and the actual2205C0/126B20
caller frames have not returned. Driver2AB0 remainsNULL; its entire2AD8 record
is still zero, at source sound-arena pointer0A01B7BC.
[Completed original record producer](../analysis/multiplayer/map-pack/smg-audio-pool-prefix/frozen-measurement/instructions/00380772.asm),
[unfinished source initialization caller](../analysis/multiplayer/map-pack/smg-audio-pool-prefix/frozen-measurement/instructions/00383c31.asm).

Partial3832AA writes parameter vtable41D724 and field+4=1, copies original
15-DWORD table38BE2C to+38 and12-DWORD table38BDF8 to+84, and writes+30=3F
and+80=FFF. It then reads original override38D148=FFFFFFFF. Its exact branch
tests low16>2 or `(override & FFFC0000)!=0`; this actual image input requires
original379FEA. No override is supplied to bypass the settings request.
Effective flags38D144 remain their initial image value0 because the source
store383310 has not executed. Later pattern writes and380750 are unexecuted.
[Partial parameter constructor](../analysis/multiplayer/map-pack/smg-audio-pool-prefix/frozen-measurement/instructions/003832aa.asm).

The earlier frozen primary compatibility lock body completes the source guard
3804DE in the same way as the predecessor. Its two compiled-body owner/recursion
stores are explicitly separated from original Xbox instructions. The critical
section remains owned at the new stop, with lockcount0/recursion1 and the whole
produced ETHREAD pointer00AD0000. No source release/unwind/cleanup result is
substituted. Sound initialized+78 is0.
[Exact preserved guard provenance](SMG_AUDIO_DEVICE_PREFIX_1_1_RESEARCH.md).

## Exact next unsupported request

Original379FEA creates its own8-byte local frame, then calls source thunk
2DD56C from **379FFD**, with return **37A002**. The thunk jumps through
417C50 to kernel ordinal24. The oracle's conventional80000018 address denotes
that decoded import ordinal, not a retail kernel virtual address.

| Request operand | Retained actual value |
| --- | --- |
| ValueIndex | 9 = primary header XC_AUDIO |
| Type output pointer | 021FCE94 |
| Type bytes before service | FFFF0000, raw little-endian bytes |
| Value output pointer | 021FCE98 |
| Value bytes before service | FFFF0000, raw little-endian bytes |
| ValueLength | 4 |
| ResultLength output pointer | NULL |
| Actual call bytes | E86A35F6FF |

These output locals retain previous stack bytes because no settings service
has run. They are **not audio settings values**. Execution stops before the
kernel, with no return or output write. The primary declaration is
`NTSTATUS ExQueryNonVolatileSetting(ULONG,PULONG,PVOID,ULONG,PULONG)`.
[Original request and conditional continuation](../analysis/multiplayer/map-pack/smg-audio-pool-prefix/frozen-measurement/instructions/00379fea.asm),
[original ordinal thunk](../analysis/multiplayer/map-pack/smg-audio-pool-prefix/frozen-measurement/instructions/002dd56c.asm),
[primary saved kernel header](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/XboxDev-xboxkrnl.h),
[primary nxdk declaration/setting indices](https://github.com/XboxDev/nxdk/blob/14d5ee97e73347c973f1f57b68b79ec08c9e77f2/lib/xboxkrnl/xboxkrnl.h).

Inspection shows that after a negative returned status the original code clears
its local audio value; after either outcome it calls33874D before returning
the local value. Neither branch nor that further service executes here. A
future extension must supply a genuine settings provider or independently
measured compatibility contract, rather than fabricate an EEPROM value or
failure to advance. Later NVAPU/interrupt/MMIO/DSP/device capability and actual
voice/packet consumption remain unproved.

The completed contract is the original statistics owner and raw-allocation
consumer slice under explicitly real host capacity/backing. The new settings
request and retained partial state are frozen. Research status remains
`native_gameplay_update_applied:false`.
