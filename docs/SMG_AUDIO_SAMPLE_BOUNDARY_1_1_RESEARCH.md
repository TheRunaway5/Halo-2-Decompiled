# Original SMG compressed samples and the decoder/device boundary, disc 1.1

Untouched original cache loading produces the complete selected ready and
reload compressed resources. Original `21AA60` returns their actual cache-page
addresses, and original `220520` builds the Xbox ADPCM format used for these
sounds: stereo, 44,100 Hz, 4 bits, 72 bytes per block and 64 samples per block.
Original linked DirectSound format conversion `3840C7 → 384057` also returns.

The original stream constructor reaches a genuine unavailable driver-owned
channel store, `2B4975`. It is stopped **before** that store. No driver,
channel backing, voice, successful stream creation, readiness, status,
consumption or device result is supplied. No original CPU decoder, packet
submission, hardware execution, decoded PCM, audible output or playback is
proved. Inspection shows that the stock packet codec forwards the compressed
bytes to DirectSound; the inspected linked driver selects its ADPCM hardware
format rather than decoding those bytes in the packet wrapper.

This is a separate research freeze. Production is unchanged, all 114 earlier
pins remain unchanged, and every report retains
`native_gameplay_update_applied:false`.

## Authority and measured scope

The authority is the user's disc 1.1 executable,
`assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
Lockout is `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`;
shared.map is `084c817fa82bb9bc416d9ba847cec05be431abc419e3a1087cdf211fbf47f5af`.
The later/final title-update state is not inferred.

The separate [oracle](../tools/oracle_smg_audio_sample_boundary_1_1.py)
subclasses the unchanged [source-created owner experiment](SMG_AUDIO_VALID_OWNER_1_1_RESEARCH.md).
It repeats that experiment's original world allocation, join, loadout, audio
owner constructors, real rejection, explicitly called unvoiced source kernel
and complete unvoiced removal. It does not copy an object registry, synthesize
an owner, assign a local view, or substitute the older composed FP owner.
The genuine allocated carried SMG is `E2700001`, parent `E26F0000`, player
`EC700000` in these runs. Addresses are measured compatibility outputs.

The sound callback root is still an **explicit composition** of the actual
allocated weapon and authored graph reference with the direct `18AA90` ABI.
It is not an executed ordinary FP event. Original `21F220` naturally rejects
because the original initialized flag remains zero. The actual newly produced
rejected A0-byte request is copied unchanged into declared raw caller storage,
and original `127750` is called explicitly; it creates an unvoiced source and
the real cache request. Original `128A70 → 18E2F0` then completes its existing
valid-weapon/type-mask rejection cleanup. The unchanged
[ordinary nonlocal FP gate](SMG_AUDIO_FP_OWNER_GATE_1_1_RESEARCH.md) still returns
NONE for all four real view queries and does not run an owning FP sound caller.

Each new run replays the previous measured **whole** compatibility context and
the exact five previously measured host clock inputs per case. No context
pointer, selected ETHREAD byte, clock result, seed, admission or game field is
changed to force progress. Compatibility KPCR/ETHREAD/TLS output remains
distinct from retail Xbox thread authority. Real raw allocation, lock,
semaphore and verified file-read providers retain their earlier explicit
limits. The actual file handle bound at `55F798` is still an external provider
binding; the original file-registry owner is not executed.

The [measurement](../analysis/multiplayer/map-pack/smg-audio-sample-boundary/measured/report.json)
and [independent fresh-process repeat](../analysis/multiplayer/map-pack/smg-audio-sample-boundary/reexecution/report.json)
retain whole source/world/cache state, every request and provider boundary,
sample calls, ordered writes, static original instructions and executed
original instructions. `.text` and linked `DSOUND` instructions are retained
from their actual XBE sections. Stop instructions and replaced service entries
are distinguished from executed instructions. The
[auditor](../tools/audit_smg_audio_sample_boundary_1_1.py) compares the complete
source outputs using the existing named provider-address/mode exclusions,
checks every retained instruction against the XBE, and compares the exported
samples byte-for-byte to shared.map. Exact hashes and counts are in the
[suite](../analysis/multiplayer/map-pack/smg-audio-sample-boundary/frozen/suite.json)
and [audit](../analysis/multiplayer/map-pack/smg-audio-sample-boundary/audit.json).

The final retained set has 28 instruction files, 35314 instruction rows and
33687 unique executed original addresses. The auditor also verifies the 212
artifact members of the earlier manifest-based freezes, in addition to the
114 direct prior pins. The earlier `pre-freeze-text-only-listing-probe`
directory is exploratory and is excluded from this final manifest.

## Authored sound and resource metadata

The selected sound definitions are actual references of FP graph `E5AA0435`.
These are ready/reload resource cases, not all SMG firing, impact, movement or
looping audio. `reload_full` is listed only as original definition inspection;
its sample selection/loading is not part of this execution.

| Sound | Datum | Raw 20-byte `snd!` definition |
| --- | --- | --- |
| ready | `E5AC0437` | `021c0501010130009c000100ffffffff3a040000` |
| reload | `E5AD0438` | `021c0601010111009d000100fffffffff9050000` |
| reload_full, inspection only | `E5AE0439` | `021c0601010111009e000100ffffffff8c0a0000` |

Definition bytes `+3/+4/+5` are `1/1/1` in these resources. Original driver
format construction uses **encoding byte `+4`**, value 1. The source sound
cache `ugh!` is `F324119F`, Lockout file offset `34350736`, size `169068`.
Its original table count/pointer fields and element sizes are:

| Table | Count / pointer offsets | Element bytes |
| --- | --- | --- |
| Pitch ranges | `ugh! +20/+24` | 12 |
| Permutations | `ugh! +28/+2C` | 16 |
| Chunks | `ugh! +40/+44` | 12 |

The original created source stores selected pitch at `+9C`, permutation at
`+9D`, and chunk index at `+9E` (word). Those outputs are all zero in these
two measured cases; they are not substituted selection inputs. Original
`1275B0/129E00` selection and ordered RNG production remain those of the
unchanged source-creation experiment.

| Selected metadata | ready | reload |
| --- | --- | --- |
| Pitch range raw 12 bytes | `77010000ffffd003e6020200` | `770100000000ffffe8020100` |
| Permutation raw 16 bytes | `2302000000000000d0d100006a030100` | `2502000000000000702801006c030200` |
| Permutation chunk count `+E` | 1 | 2 |
| Permutation byte total `+8` | 53712 | 75888 |
| Permutation word `+6` | 0 | 0 |

Bounded original `21ABD0` is called with ECX=the actual authored permutation
pointer and EDX=its actual chunk count. It sign-extends permutation `+C`, reads
the original chunk table, masks each chunk size with `3FFFFFFF` and sums those
**bytes** into EAX. It returns 53712/75888. That field is not labelled a decoded
sample count. Bounded `21AD70` receives EAX=sound datum and stack pitch,
permutation; it resolves the real tables, reads unsigned permutation word
`+6` into EAX and converts it to XMM0. Both words are zero here; no unproved
time unit or decoded duration is assigned to the word.

## Real cache bytes and pointer/reference lifecycle

The inherited source request loads ready chunk 0 and reload chunk 0. Their
earlier owning removal is completed before these new metadata/data queries.
For reload chunk 1, this experiment explicitly calls original
`21A960(sound, chunk, flags=2)` with the **actual authored next chunk**, then
invokes one original worker iteration. It observes pending bit 1 and no ready
bit 2 before completion. This is a declared direct cache request, not an
executed normal stream-prefetch or async scheduling order.

Original `122360` reads each real aligned region through the verified
shared.map provider. The provider writes actual bytes and read length;
original `1223AC` publishes completion. No ready field is prefilled. Original
`21A960(..., flags=0)` confirms ready bit 2 before the new pointer query.

| Resource / ordinal | shared.map offset | Authored bytes | Actual aligned read | Raw chunk metadata |
| --- | ---: | ---: | ---: | --- |
| ready / 0 | 79925760 | 53712 | 53760 | `0092c384d0d10000ffffffff` |
| reload / 0 | 80033280 | 32760 | 32768 | `0036c584f87f0000ffffffff` |
| reload / 1 | 80066048 | 43128 | 43520 | `00b6c58478a80000ffffffff` |

All three original chunk locations are 2, referring to shared.map. The audit
checks all 130048 aligned bytes. It exports the exact authored byte extents
separately from the real rounded read buffers; padding is not passed off as
authored compressed audio.

`21AA60` takes EDX=the actual runtime chunk pointer. It uses chunk `+8`'s
low-16 datum index in the cache pool at `[5098DC]+44`, stride 16. It increments
entry reference byte `+5` if below `FF` (`21AA82`). The observed byte is 0→1
for each pointer call. It resolves the corresponding 24-byte page record from
`[5098E4]+64`, reads the page dword at `+8`, and returns:

`[5098E0] + (page_dword << page_allocator[+34])`.

Measured ready/reload-first pointer is `82A47000`; reload-second is
`82A4F000`. The owning constructors, cache entry datum, page record, buffer and
reference changes are retained as full buffers. The provider does not supply
those pointers or a successful cache return. These are bounded data-pointer
queries after the completed unvoiced source removal; this experiment does
not claim normal voiced packet lifetime or release of these extra query refs.

| Authored compressed bytes | SHA-256 |
| --- | --- |
| ready complete selected permutation | `3cf70cd06a33bf1c6f9da537b69a8feb7c9d51e1c0f285ed1b4f2ab271e47013` |
| reload chunk 0 | `571021ea891011efa2466576c7b9986df3d90b1a8a1361a1456c067a17b12c7a` |
| reload chunk 1 | `30432691e0f2760bb17f75f0e87002c512dd5c50eb483ead73f0c4bb50845330` |
| reload complete selected permutation | `a49ab2b9a932d3ddcb9f34c3fee1183c92d498aa6e8403189d01350837f88046` |

## Executed original format contract

`220520` receives EAX=actual encoding 1, ESI=declared raw caller format output,
EDI=declared codec-pointer output. Raw caller output backing is an explicit
mapped 4096-byte area at `0C300000`, not a supplied driver or constructed voice.
The original instructions write the complete 20-byte format:

`6900020044ac0000c8c100004800040002004000`.

| Offset | Source output |
| --- | --- |
| `+0`, word | Format tag `0069` |
| `+2`, word | Channels 2 |
| `+4`, dword | Samples/sec 44100 |
| `+8`, dword | Average bytes/sec 49608 |
| `+C`, word | Block alignment 72 |
| `+E`, word | Bits/sample 4 |
| `+10`, word | Extra bytes 2 |
| `+12`, word | Samples/block 64 |

The original sequence chooses float 2 from `46507C`, calls original x87
conversion helper `32909C`, writes the low word channels, computes
`channels*36`, truncates to the low word block alignment, then multiplies that
unsigned word by `689` for average bytes/sec. It writes the linked codec
instance `4865F8`; this is original executable data, not a fabricated codec
constructor result. The linked reconstructed base function
[21E410](../tools/reference/halo2-decompiled/src/unknown_21e330.cpp) labels this
format `WAVE_FORMAT_XBOX_ADPCM`; the executed 1.1 bytes establish its values.

`3840C7(out, input)`, `RET 8`, calls original `384057` and builds compact
12-byte format `6900020444ac000048000000`. It writes literal tag `69`, byte
channels, byte bits, dword rate and dword alignment from the original format
operands. This is a bounded linked-format kernel, not a voice initializer.

Each authored chunk is divisible by the proved 72-byte block size: ready 746
blocks; reload 455 and 599 blocks. Multiplying block counts by the authored
64-sample block field is a derived capacity calculation, not executed PCM
decoding, duration, a proof of hardware predictor/nibble behavior, or audible
parity. No ADPCM step/index table, predictor, nibble layout, rounding or
saturation rule is invented or claimed here.

## Genuine stream-constructor stop

Bounded `2215F0` receives EBX=actual encoding 1. Its stack channel input is
explicitly selected as 40, the sum of original preceding policy counts in
`451220`; this is not a claimed scheduler-selected free voice. Original
`2215F0` calculates channel `[526474] + index*34 + C`, writes its own format
and 24-byte descriptor, and calls `2B4970` with EAX=channel, ECX=original codec,
EDI=descriptor.

At the genuine stop, `[526474]` is still NULL. The calculated channel is
`0000082C`; source instruction `2B4975 894e30` would write codec pointer
`004865F8` to `0000085C`. The hook stops **before** that original store. It
does not map a channel there, assign the driver, call a constructor stub, or
return success. This instruction is present in the inspected listing and
absent from the executed instruction set.

The original stack descriptor at this stop has six dwords:

| Offset | Original descriptor output |
| --- | --- |
| `+0` | Flags 0 |
| `+4` | Maximum attached packets 2 |
| `+8` | Pointer to actual source-built 20-byte format |
| `+C` | Callback `222840` |
| `+10` | Explicit channel context 40 |
| `+14` | Mix-bin pointer NULL |

The later, unexecuted `2B4970` instructions would OR descriptor flags with
`20000000`, use driver `+2AB0`'s real DirectSound owner and call `383B22` to
construct a stream. Neither that call nor its handle, return, headroom, voice
count/state or later codec/chunk initialization is executed in this experiment.
The older device experiment retains its own distinct partial static-backing
boundary; it is not promoted to genuine driver production here.

## Inspected packet/upload/hardware ownership, not executed playback

The original static codec pointer `4865F8` has vtable `45E2B8` and nine entries:
`27E760,27E760,2B4C50,2B4CE0,2B4D60,2B4D90,2B4DD0,187550,2B4E10`.
The decompiled class is named `c_pcm_codec`, but the actual selected format
and packet operands above remain authority; the class name does not change
the compressed sample type.

Inspection of [original packet builder 2B4CE0](../analysis/multiplayer/map-pack/smg-audio-sample-boundary/measured/instructions/002b4ce0.asm)
shows `RET 8`, stack stream/packet. It selects chunk
`stream[+1C + 4*started_count]` using the signed started-count bitfield from
`stream+2`, calls `21AA60`, obtains chunk size `+4 & 3FFFFFFF`, and computes
`offset=min(stream[+2C], max(size-48hex,0))`. Its 24-byte packet fields are:

| Packet offset | Original write |
| --- | --- |
| `+0` | Real compressed cache pointer plus offset |
| `+4` | Compressed size minus offset |
| `+8` | Completed-size pointer NULL |
| `+C` | Status pointer NULL |
| `+10` | Actual chunk pointer as completion context |
| `+14` | Timestamp pointer NULL |

It returns low byte 1. No PCM allocation or ADPCM decode occurs in that
inspected packet routine. The packet routine is **not executed** by this new
experiment: its source-owned stream/voice admission has not completed.
`2B4D60` updates submitted-count/offset; `2B4D90` failure and `2B4DD0`
completion manage the chunk/reference lifecycle. `2B4E10` checks chunk count
against submitted count. These static routines are retained for ownership
inspection only; they receive no fabricated stream state.

Original [stream update 2B4B30](../analysis/multiplayer/map-pack/smg-audio-sample-boundary/measured/instructions/002b4b30.asm)
admits channel states 2/3, calls genuine stream vtable `+C` GetStatus, requires
nonnegative return and READY bit 1, checks codec `+20`, builds through codec
`+C`, then calls stream `+10` Process(packet,NULL). It selects codec submitted
or failure callbacks and calls stream `+14` Discontinuity after submission.
Original `222840` checks channel ownership and forwards completion through
codec `+18`. No such state, status, returned device or completion is supplied.

Inspected linked DirectSound owner `381365` checks its real lock/device and
stream-ready context before calling `3865E6`. `3865E6` copies all 24 packet
bytes into a real packet-pool record, initializes pending status `8000000A`,
and passes packet address/size/zero to kernel import slot `417D20` (original
import word `800000AF`, ordinal 175). It links the real packet and can call
`38632E` for hardware submission. These pool/device/kernel/MMIO dependencies
are not supplied or executed here.

The linked hardware-format owner
[386F8B](../analysis/multiplayer/map-pack/smg-audio-sample-boundary/measured/instructions/00386f8b.asm)
reads the compact format from the real voice configuration. For tag `69`,
`386FD1..386FDE` writes `(old_format & 0FFFFFFF) | A0000000`. It derives
channel bits from the actual channel byte. When the real voice-active flag is
set, it waits on MMIO `FE820010`, then writes actual voice identity to
`FE8202F8` and format to `FE820304`. This identifies the unchanged linked
hardware handoff inspected for ADPCM; it does not prove the hardware decoder,
its PCM result, packet consumption, DSP effects or audio output.

The separate WMA codec instance/vtable is not selected by these encoding-1
definitions. Its CPU/media-object decoder calls are a different path and are
not executed or substituted for stock SMG ADPCM.

## Reproduction and supported native adapter boundary

Run the new experiment in two fresh output directories; its existing input
context and clocks remain pinned to the earlier measured owner experiment:

```sh
audio_sample_repro=$(mktemp -d)
.tools/python/bin/python tools/oracle_smg_audio_sample_boundary_1_1.py \
  --output "$audio_sample_repro/measured/report.json"
.tools/python/bin/python tools/oracle_smg_audio_sample_boundary_1_1.py \
  --output "$audio_sample_repro/reexecution/report.json"
.tools/python/bin/python tools/audit_smg_audio_sample_boundary_1_1.py
```

The final auditor verifies the retained frozen pair without rewriting their
reports, instruction files or sample bytes. Earlier frozen dependencies are
checked before and after original execution.

A native adapter can use the verified source-selected compressed byte extent,
the original format fields, genuine cache/pin outputs and explicit unavailable
stream boundary. Host audio allocation/decoding/output must remain a declared
provider until separately measured/proved; SDL handles are host provider
results, not original DirectSound constructor outputs. This evidence does not
justify enabling the original initialized flag, copying a channel/voice
record, fabricating READY/consumed packets, or claiming a generic host ADPCM
decoder has Xbox PCM bit parity. Ordinary local binding, full FP-to-sound
ownership, complete map/audio activation, source-created driver/device and
real stream submission remain separate open gates.
