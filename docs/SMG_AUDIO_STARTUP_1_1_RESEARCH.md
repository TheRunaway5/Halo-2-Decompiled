# Original 1.1 audio startup, clock and device-provider boundary

The separate startup oracle executes original map-pack 1.1 instructions with
actual host clocks and storage. It freezes the normal startup/map callback
locations, twelve original sound-clock updates, and original stream/buffer
requests. **It does not complete original audio initialization or playback.**
No initialized/ready flag, device handle, capability, controller seed, packet
completion or DirectSound success is supplied. Earlier sound, first-person,
variation and cache studies remain unchanged.

## Frozen evidence and reproduction

| Artifact | SHA-256 |
| --- | --- |
| `tools/oracle_smg_audio_startup_1_1.py` | `cbca5876b76195d85071aebfa7f404544f95152a832b9f038deb499d4f4ca6a3` |
| `tools/audit_smg_audio_startup_1_1.py` | `ca7cec8cbdef89a275e5f5841f87d71e265144166497ab93166ba68e23b4f67f` |
| `analysis/multiplayer/map-pack/smg-audio-startup/frozen/suite.json` | `a368b7735502f02ec0db5d164cdd1f049367dec81fc76acc5942a1453dfa2a4c` |
| `analysis/multiplayer/map-pack/smg-audio-startup/source-fixture.json` | `e02bcb3338711805ebef0c63b18c9cc807f8819a71ec6c5d117b106874764b02` |
| `analysis/multiplayer/map-pack/smg-audio-startup/audit.json` | `ccdee10aab8b1f0cfdea3a2511b4ac7b5ca9fd54b0dc88ec0ffb4e30a62f7f78` |
| Real paused SDL probe `tools/probe_sdl_audio_provider.py` | `38e2ada1c2a3505285a1a7d6c52279a1c1c43bc09d069456a5091b797ab694e6` |

Authority is the user's update `default.xbe.03`, full-image SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
The inherited cache is the original Lockout image, SHA-256
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
The audit checks every saved instruction's bytes against that image: 122
listings, 7,178 instruction rows, two independent source reexecutions, sixteen
recorded real clock calls, twelve completed original clock kernels, six
unadmitted stream requests and two unadmitted voice requests. It verifies 27
prior frozen pins. Both source runs retain initialization byte `+78=0`.
[Suite](../analysis/multiplayer/map-pack/smg-audio-startup/frozen/suite.json),
[full fixture](../analysis/multiplayer/map-pack/smg-audio-startup/source-fixture.json),
[audit](../analysis/multiplayer/map-pack/smg-audio-startup/audit.json).

Reexecution must write a new directory, never overwrite frozen evidence:

```sh
.tools/python/bin/python tools/oracle_smg_audio_startup_1_1.py \
  --output analysis/multiplayer/map-pack/smg-audio-startup/check/report.json \
  --replay analysis/multiplayer/map-pack/smg-audio-startup/measured/report.json \
  --instructions analysis/multiplayer/map-pack/smg-audio-startup/check/instructions
.tools/python/bin/python tools/oracle_smg_audio_startup_1_1.py \
  --output analysis/multiplayer/map-pack/smg-audio-startup/compat-check/report.json \
  --replay analysis/multiplayer/map-pack/smg-audio-startup/compat-measured/report.json \
  --context-replay analysis/multiplayer/map-pack/smg-audio-startup/compat-measured/raw-host-context.json \
  --instructions analysis/multiplayer/map-pack/smg-audio-startup/compat-check/instructions
```

These replay exact observed provider inputs. They are independent execution of
original instructions, not a second live clock/context measurement. The
`--compat-context` option instead reads the pinned, already built Wine32
generator and keeps its real worker alive during source consumers. It never
calls the generator's build routine.

## Normal title and map activation sites

The actual table at `447428` has 68 records of 36 bytes. `139390` dispatches
the constructors. Row43 at `447A34` contains class constructor `223960` and
map callback `2239B0`; row44 at `447A58` contains sound constructor `126B20`
and map callback `126DE0`. New-map dispatcher `139410` walks the map entries
starting at `447430`; `190A30` calls that dispatcher. `139470` resets entries
in reverse. All table bytes are retained from the XBE.
[Table and call records](../analysis/multiplayer/map-pack/smg-audio-startup/measured/report.json),
[constructor dispatcher](../analysis/multiplayer/map-pack/smg-audio-startup/measured/instructions/00139390.asm),
[map dispatcher](../analysis/multiplayer/map-pack/smg-audio-startup/measured/instructions/00139410.asm).

Actual map request `1902E0` reaches `1904D0`. Its state dispatch uses
`19E070` and table `190598`: states0/1/2 invoke `190B30`, state3 skips that
loader, state4 takes its error path. The success sequence is `190509`
calling `190650(AL=0)`, `190510` calling actual cache loader `124040`, and
only after that returns true, `190519` calling `11DE10` and `190521`
calling `190690(AL=0)`. Failure calls `190730` at `190539`.
`124040` reads/validates the selected real cache header and tags before
publishing globals `4EB114`, `4E7920` and `4E791C`.
[Request](../analysis/multiplayer/map-pack/smg-audio-startup/measured/instructions/001904d0.asm),
[actual cache loader](../analysis/multiplayer/map-pack/smg-audio-startup/measured/instructions/00124040.asm).

`190690` has actual resource/thread/game-map stages. Its `1906FA` store
sets cache-pool `5098DC+29=1`, followed by `16D220`. That field must be
produced by this stage rather than set to admit sound-cache work. These
full title/map callers are **inspected, not executed** in this bounded
study. Direct reset/update calls below are explicitly separate oracle inputs;
they do not establish normal request ownership or a completed map activation.
[Source lifecycle](../analysis/multiplayer/map-pack/smg-audio-startup/measured/instructions/00190690.asm).

## Real clocks and retained source arithmetic

Original `336983` is the `GetSystemTimeAsFileTime` import wrapper. The provider
writes real host UTC nanoseconds converted to `ns/100 + 116444736000000000`
into its output pointer. It preserves EAX because this is a void API; original
CRT `32803E` reloads and converts FILETIME to integral Unix seconds through
`327290`. Original `337A40` is `GetTickCount`; the provider returns actual
Linux `CLOCK_BOOTTIME` milliseconds modulo2^32. Each source call site,
argument, raw input and resulting output is recorded. The Windows FILETIME
epoch and API shape are documented by Microsoft; Linux epoch/granularity do
not establish Xbox timing parity.
[Microsoft FILETIME](https://learn.microsoft.com/en-us/windows/win32/sysinfo/file-times),
[Microsoft GetTickCount](https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-gettickcount),
[source time conversion](../analysis/multiplayer/map-pack/smg-audio-startup/measured/instructions/0032803e.asm).

Direct original map reset `126DE0` writes current tick to sound-system `+80`,
clears accumulated milliseconds `+84`, sets master fade1 and `+7A=1`, and
resets ambient/per-view state. It then reaches `127540 -> 2217E0 -> 2204C0`
and stops at **`2204E6 mov edx,[esi]` with ESI=NULL**. The driver/effect
object at `+2AB4` is not initialized. No effect return is substituted.
[Executed reset](../analysis/multiplayer/map-pack/smg-audio-startup/measured/instructions/00126de0.asm),
[exact effect boundary](../analysis/multiplayer/map-pack/smg-audio-startup/measured/report.json).

Six independent direct `128140` calls in each context preserve these SSE2
operations and ordering:

1. Calculate signed32 wrapped delta `new_tick - old(+80)`.
2. Store new tick to `+80`; add delta modulo2^32 to `+84`.
3. `CVTSI2SS` delta, then multiply by original float at `4650E0` (`0.001`).
4. Add that float to `+1FC`, store it to `+1E0`, add it to `+204`.
5. The inherited options mode2 takes the original multiplayer fade branch,
   leaving master fade1.

The auditor checks raw float bits for every output against the recorded input
and original constant. The measured deltas are small real elapsed times;
integer-wrap edge cases and a complete admitted scheduler are not claimed.
[Clock instructions](../analysis/multiplayer/map-pack/smg-audio-startup/measured/instructions/00128140.asm),
[checked outputs](../analysis/multiplayer/map-pack/smg-audio-startup/source-fixture.json).

## Whole compatibility context and the CRT heap stop

The no-context case stops before the first `FS:[24]` read in `32C609` or
`3804DE`. The other case copies four **whole** records from a real Wine32
worker executing pinned primary Cxbx `EmuGenerateFS<true>`: KPCR280 bytes,
ETHREAD140 bytes, its host-backed stack snapshot and the source static process
record. No pointer is relocated and no selected field is patched. Its source
output `KTHREAD+28=TLS NULL` selects the original CRT global fallback branch.
Explicit emulator descriptors route FS to the produced KPCR; the selectors
are not asserted to be Xbox selector values. The context remains a primary
compatibility host-thread branch, distinct from normal Xbox/XAPI title-thread
initialization.
[Whole-context provenance](MP_COMPLETE_COMPAT_KPCR_RESEARCH.md),
[actual snapshot](../analysis/multiplayer/map-pack/smg-audio-startup/compat-measured/raw-host-context.json).

Source controller constructor `21BC50` creates its 64-by-1C pool and root1C,
then requests `time(0) -> rand() -> GetTickCount()` before storing their XOR
at root`+8` (`21BCF6`). `rand` enters `32C609`. The real whole context gets
past FS/TLS selection and reaches original `calloc32E813(1,88)`, but the
original process heap global `566F08` is NULL. `GetProcessHeap2D7969` returns
that source value; actual `HeapAlloc2DAC7F` reads receiver`+18` at address18.
**The root seed store and cold CRT seed1 store at32C66A are not reached.**
No heap receiver, CRT seed, controller seed or local RNG seed is supplied.
[Controller constructor](../analysis/multiplayer/map-pack/smg-audio-startup/compat-measured/instructions/0021bc50.asm),
[original CRT rand](../analysis/multiplayer/map-pack/smg-audio-startup/compat-measured/instructions/00328505.asm),
[exact heap failure](../analysis/multiplayer/map-pack/smg-audio-startup/compat-measured/report.json).

The newer, separately frozen world study has now recovered original process
heap and CRT prerequisites `32CFFC` and `3274E4`. They are **not inserted
retroactively into this freeze**. `32CFFC` is the runtime lock-table initializer;
`3274E4` creates the on-exit array through original malloc. Their actual
table/caller order and their declared raw OS backing boundary belong to that
study. A further RNG startup study must execute the appropriate heap producer
and preserve those boundaries before claiming controller/local seed completion.
[Separate CRT preparation](MP_FIXED_MEMORY_WORLD_RESEARCH.md),
[separate process-heap boundary](MP_PROCESS_HEAP_WORLD_STARTUP_RESEARCH.md).

Separate original gameplay RNG constructor **`1479B0`** is callback-table
row3 (`447494`), before sound row44. It writes shared seed`78A8`, then calls
`32803E`, `328505`, `337A40` and stores `time ^ rand ^ tick` into local RNG
`+4` at `147A13`. This was established from the unchanged table and original
linear disassembly; the existing Ghidra function inventory omits this entry.
It is not `147790`. It has not been executed in this audio freeze. The
adapter's explicitly supplied local seed0 must not be presented as normal
startup output. Boot-seed recovery needs its own independently frozen trace.

## Exact original device and resource requests

Independent direct driver constructor `2205C0` consumes original policy
`451220`, reserves/zeros2AD8 and publishes `526474`; original device
field`+2AB0` stays NULL. It has not returned. It reaches
`220626 -> 383DD7 DirectSoundCreate(NULL, &driver[2AB0], NULL)`.
With whole context, `383DD7 -> 3804DE` reaches the first unavailable
`RtlEnterCriticalSection` request at `3804F2`: import slot`417BE0` contains
`80000115`, argument`38D158`, full original critical-section bytes
`010004000000000060d1380060d13800ffffffff0000000000000000`.
The provider is not replaced with success.
[Driver](../analysis/multiplayer/map-pack/smg-audio-startup/compat-measured/instructions/002205c0.asm),
[device wrapper](../analysis/multiplayer/map-pack/smg-audio-startup/compat-measured/instructions/00383dd7.asm).

Subsequent device calls are inspected only: four-DWORD `GetCaps381BEE`;
distance factor`383B46` with float bits`4043126F`; rolloff`383C0D` zero;
speaker configuration`381C0A`; 32 mix-bin headroom calls`381C77` with zero;
deferred doppler`383B6A` zero; LightHRTF4channel`38059B`. `192D30` requests
the original executable `DSPImage` with effect locations9/10 and download
flags1, then retrieves eight-byte effect records for effects4..7 at offset20.
These are required source device/effect services, not actual SDL capabilities.
[DSP initializer](../analysis/multiplayer/map-pack/smg-audio-startup/measured/instructions/00192d30.asm).

The policy selects **82 streams and 51 shared 3D buffer voices**. Stream128
and buffer64 are struct capacities, not these authored allocation counts.
The 73 static policy slots remain distinct from the authored sound-class count
as already explained in the adapter document.
[Prior policy/class distinction](SMG_AUDIO_ADAPTER_1_1_RESEARCH.md).

The independently invoked unadmitted stream constructor `2215F0` produces
actual requests through `220520 -> 2B4970 -> 383B22`:

| Input stream type/index | Original format |
| --- | --- |
| 0 / 0 | Xbox ADPCM tag69, mono44100Hz, average24804 bytes/sec, block36, bits4, extra2, 64 samples/block |
| 1 / 40 | Xbox ADPCM tag69, stereo44100Hz, average49608 bytes/sec, block72, bits4, extra2, 64 samples/block |
| 2 / 81 | PCM tag1, stereo44100Hz, average176400 bytes/sec, block4, bits16, extra0 |

Each 24-byte descriptor has flags`20000000`, max attached packets2,
its original stack WAVE pointer, callback`222840`, original index and
mix-bin NULL. The last unused two bytes captured after PCM's18-byte format
remain stack64; they are **not authored PCM extra data**. Actual device
argument isNULL and no media object or success is returned. Original
`222B80 -> 383AFE -> 383799` similarly produces first buffer descriptor
`[18h,2010h,0,0,0,0]` with aNULL device and no fabricated voice.
[Actual descriptors](../analysis/multiplayer/map-pack/smg-audio-startup/source-fixture.json),
[stream constructor](../analysis/multiplayer/map-pack/smg-audio-startup/measured/instructions/002215f0.asm),
[buffer wrapper](../analysis/multiplayer/map-pack/smg-audio-startup/measured/instructions/00383afe.asm).

The inspected source stream interface requires vtable`+0C GetStatus`,
`+10 Process(packet,NULL)`, `+14 Discontinuity` and `+18 Flush`. `2B4B30`
checks status READY bit1 before acquiring a packet. A successful Process
calls codec`+10`, failure calls codec`+14`; successful submission later
issues Discontinuity. Actual callback`222840(index,packet-context,status)`
checks the index against the driver count and calls codec`+18` for completion;
status0 may resubmit. `220D10` performs DirectSound work and polls/flushes
appropriate channels. None is executed against an admitted real stream.
An SDL adapter must report **actual** backend progress and retained packet
ownership rather than fabricate consumed bytes or completion callbacks.
[Submission](../analysis/multiplayer/map-pack/smg-audio-startup/measured/instructions/002b4b30.asm),
[completion](../analysis/multiplayer/map-pack/smg-audio-startup/measured/instructions/00222840.asm),
[polling](../analysis/multiplayer/map-pack/smg-audio-startup/measured/instructions/00220d10.asm).

## Genuine host probe and primary compatibility comparison

The separate paused SDL probe opens the actual default **PipeWire** backend:
returned deviceID2, obtained48000Hz float32 little-endian, stereo, 512 frames,
4096 bytes, then closes it. No dummy/disk backend is forced. The device remains
paused, no packets are queued and no audible playback is tested. This proves
real host device access only. SDL ID2 is not an original DirectSound object
pointer and supplies no original caps, effects, stream admission or consumption.
The probe uses NULL callback/queue mode; the current renderer callback is a
separate host consumer.
[Measured SDL probe](../analysis/multiplayer/map-pack/smg-audio-startup/sdl-provider.json),
[SDL device API](https://wiki.libsdl.org/SDL2/SDL_OpenAudioDevice),
[SDL status API](https://wiki.libsdl.org/SDL2/SDL_GetAudioDeviceStatus).

Primary Cxbx source commit `585c49a50af1255ab155099e06f24505f9c5a800` uses
actual Windows `DirectSoundCreate8`, a host primary buffer and real buffer
progress. Its `GetCaps` implementation explicitly has TODO placeholders of
`0x200` for free2D/free3D buffers; those values must never become retail
capability authority. Its stream packet manager derives completions from real
host cursor movement. These primary files guide compatibility API ownership;
they do not establish Halo2/Xbox sound hardware behavior.
[Actual Cxbx device and caps implementation](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/hle/DSOUND/DirectSound/DirectSound.cpp),
[actual packet ownership/progress](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/hle/DSOUND/DirectSound/DSStream_PacketManager.cpp),
[saved source pins](../analysis/multiplayer/map-pack/smg-audio-startup/reference/cxbx-585c49a/provenance.json).

## Next gate

The executable scalar sound/selection and cache lifecycle evidence stays usable
within its prior explicit inputs. Complete playback still needs actual source
heap/CRT preparation, admitted controller/device/effect initialization, normal
map activation and real packet/voice services. Original seed constructor1479B0
must be frozen separately. The complete normal map scheduler, device caps,
effects, packet progress, audible output and Xbox clock parity remain unproved.
`native_gameplay_update_applied:false` remains the research status.
