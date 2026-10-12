# Driver constructor CPU caller and native dependency contract

`source_sound_driver_startup` in `sound_driver_startup.hpp/.cpp` represents the complete original `2205C0` caller, `220BF0` cleanup caller and `12D7B0` allocation/CRC scalar. It preserves their storage writes, call order, signed count gates and final initialized-byte commit. It does **not** implement the transitive environment, listener, stream, static buffer, effect mapping, feedback, flush or hardware-queue helper bodies. Those are required source-owner callbacks, distinct from required real platform services. Neither callback category has a default success result.

The independent source execution proves the allocation prefix through the actual `DirectSoundCreate` entry, with no device return supplied. Successful whole driver construction, audio admission and live launcher playback remain unresolved.

## Authority and independent execution

The checkpoint is `analysis/multiplayer/integration/sound-driver-startup-native/`. Its `paired/report.json`, `paired/providers.json` and 24 complete instruction listings bind twelve bodies against the user-owned authenticated images:

| Body | Disc 1.1 | Authenticated u5 | Rows per image |
|---|---:|---:|---:|
| Driver caller | `2205C0` | `224EC0` | 420 |
| Cleanup caller | `220BF0` | `2254F0` | 74 |
| Queue storage allocation | `12D7B0` | `12EB50` | 24 |
| CRC scalar owner | `1656A0` | `166A80` | 29 |
| DSP/environment constructor | `192D30` | `1972C0` | 94 |
| Listener installation | `2228A0` | `2271A0` | 229 |
| Stream construction | `2215F0` | `225EF0` | 38 |
| Static buffer construction | `222B80` | `227480` | 85 |
| Effect buffer mapping | `192E50` | `1973E0` | 63 |
| Feedback buffer construction | `2235A0` | `227EA0` | 92 |
| Stream flush | `2213A0` | `225CA0` | 75 |
| Static hardware queue shutdown | `2B4470` | `2B7E80` | 33 |

All 2,512 listing rows are checked against the original image bytes. Complete normalized correspondence is recorded by the pairing tool; addresses are not inferred from a constant relocation delta. The image SHA-256 values are `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` and `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

`oracle_sound_driver_startup_native.py` executes unchanged instructions in each image on the complete 143,360-byte raw arena from the prior frozen 1.1 startup owner. The base, used cursor and CRC are explicit inherited source inputs. Three separately labeled cursor controls cover the other alignment residues. The u5 runs use this same declared prior-1.1 arena boundary; they do not establish ordinary u5 title startup.

The eight cases produce 21,968 represented ordered stores, complete before/after arenas and global state, and the actual three-argument device request. Each stops at `383DD7` or `387877`, before the platform call returns. The driver initialized byte and device identity remain zero. Each source run also executes and retains 257 cold CRC-cache writes; the native polynomial computation matches the CRC scalar and represented stores, but does not reproduce that guest lookup-table cache.

The normal and GCC ASan/UBSan standalone snapshots independently pass the same eight cases, with real Linux `mmap` backing. An absent native constructor stops with the exact already-produced prefix; it supplies neither HRESULT nor identity. The audit verifies 38 byte-identical independently repeated files, 354 previous pins, both original images, both instruction sets and both test outputs. No unrelated shared-archive freshness or physical-device claim follows from these tests.

## Storage and commit order

The caller aligns the current raw arena address to four bytes, accounts for the alignment plus `0x2AD8`, updates the original CRC, clears `0xAB6` DWORDs, publishes the driver and writes its first byte zero. The request is null GUID, driver `+2AB0` output and null outer identity. `memory` must return genuine owned backing for exact source addresses and extents. Native overflow/backing checks are preconditions for executing these operations, not original allocation-failure results.

After the required device, saved-setting and environment operations, the source creates the 56-byte zeroed listener packet and copies the imported forward/up vectors at `+0C/+18`. It then allocates `0xCC` bytes with 16-byte alignment through `12D7B0`, without clearing this block, publishes the queue pointer and clears its flag. The static queue counters are cleared and the source budget is `0x10000`.

The original 16-byte policy supplies three signed-short stream-category counts at `+0/+2/+4` and a signed-short static buffer count at `+0C`. The driver records category starts/ends at `+1A0C/+1A12`, total streams at `+4`, and static counts at `+6/+8`. A stream/static constructor failure prevents subsequent such constructors; the original caller still performs the later effect/plugin/feedback operations before cleanup. These count and failure semantics remain in the module.

The caller writes its original environment parameters, constructs three paired effect buffers, installs the selected plugin, constructs eight auxiliary effect buffers and two feedback buffers, then writes the four imported unity words at `+2ABC..+2AC8`, integer 8000 at `+2ACC` and zero at `+2AD0`. It commits byte zero to one only after the successful original helper sequence. The outer portable startup's separate `+78` commit is outside this module.

Cleanup invokes the actual stream flush owner first, conditionally releases and clears each stream/static-buffer identity, releases and clears the device, invokes the distinct static queue shutdown owner at `4865D8`, clears global queue `51842C` and clears the driver initialized byte. The static `4865D8` owner is not the `12D7B0`-allocated queue. Missing cleanup ownership remains a stop; it is not replaced with a no-op.

## Required platform requests

| Ordered dependency | Original request and native contract |
|---|---|
| Device create | Null GUID/null outer; real provider owns returned identity, its backing/lifetime and the driver `+2AB0` output. A genuine signed negative status enters source cleanup; absence stops. |
| Capabilities | Actual device, driver `+2AA0`, exact 16-byte output. SDL channel count is not authority for Xbox capability fields. Unsupported translation remains unavailable. |
| Distance/rolloff | Original raw word `4043126F`, apply zero; then raw zero rolloff, apply zero. Their negative statuses enter cleanup. |
| Saved setting | Actual device speaker/saved-audio owner returns signed status plus a real setting DWORD. Only `(setting & FFFF) == 2 && (setting & 10000) != 0` sets driver `+1`. Nonnegative status without an actual value stops. No inferred stereo/default EEPROM setting. |
| Headroom/Doppler/HRTF | All 32 mix-bin indices with headroom zero, then Doppler zero/apply one and the zero-argument light four-channel HRTF operation. Source ignores these operation returns. |
| Effect buffers | Exact 24-byte descriptors `[18, flags, 0, 0, 0, inputMixBin]`, null outer and actual output slot. Primary flags `100000`, paired flags `100010`; source bins 23–25. Paired buffers use max/min raw `7F7FFFFF`, apply one, then play `[0,0,0]`. Constructor failures follow the source conditional path. |
| Plugin | Required mutable selected-plugin owner, release/initialize/environment operations and actual 64-byte constructor parameter packet plus separate runtime 44-byte environment packet. No source-image mutable state is promoted as current state. |
| Auxiliary buffers | Eight descriptors with bins 15–22, flags `100000`; volume zero, one actual mix-bin pair cycling 6/7/8/9 with gain zero, then play `[0,0,0]`. |
| Release | Real stream, buffer, device and plugin ownership methods; source cleanup order and identity-slot writes must remain intact. |

These service structs describe the source request. The existing SDL platform's genuine open/queue/completion operations do not establish compatible mix bins, HRTF, effects, capabilities or saved policy. It may supply a supported genuine operation through an explicit adapter; it may not fabricate the other operations to make the source caller succeed.

## Required source CPU helpers

The paired bodies are retained for the next implementation seam. `192D30` obtains the actual DSP image resource, allocates its source environment storage and issues effects-image/download and effect-data requests. `2228A0` owns the listener and environment state writes. `2215F0` constructs the original stream format, pending-packet fields and callback/channel owner; `222B80` owns the original 3D static-buffer construction. `192E50` consumes actual effect-image output descriptors and effect data. `2235A0` owns the feedback format, source sample allocation, buffer construction and playback requests. `2213A0` owns flush calls. `2B4470` owns the separate static queue shutdown. These bodies are not implemented by their named callback declarations, and no original positive execution past device creation has been claimed.

The smallest next complete source seam is the environment constructor with a genuine effects/resource provider, or the listener/stream helper CPU owner with its platform operations unavailable at their real boundaries. A chosen helper `true`, fabricated device capability, invented routing result or initialized flag would bypass this contract.

## Immutable inputs and reproduction

`SoundDriverStartupConstants` retains the exact word and two vectors. The `routing_plugin_identity` identifies the separately required mutable owner:

| Field | 1.1 source | u5 source | Authority |
|---|---:|---:|---|
| `one_bits` | `465030` | `46A0D0` | Exact four bytes `0000803F` |
| `basis_forward` | pointer `46FC48`, value `447180` | pointer `474D08`, value `44AC28` | Exact 12 bytes `0000803F0000000000000000` |
| `basis_up` | pointer `46FC50`, value `447198` | pointer `474D10`, value `44AC40` | Exact 12 bytes `00000000000000000000803F` |
| Selected plugin identity | `486590` | `48B8A0` | Literal encoded MOV; retained mutable owner required |
| Runtime environment packet | `477548` | `47C77C` | Explicit live owner; excluded from immutable defaults |

The production test CLI is `sound_driver_startup_tests <fixtures.tsv>`; the canonical fixture path is `analysis/multiplayer/integration/sound-driver-startup-native/fixtures.tsv`. The test uses Linux `mmap`; the production caller uses only its explicit portable memory/service contract. No CMake/runtime/renderer/audio/OriginalData files were changed for this checkpoint.

`reproduction.json`, `native/commands.json` and `native-repeat/commands.json` retain exact commands, compiler version, environments, source snapshots and binary hashes. To independently reproduce, use the specified new output directories: the evidence tools refuse to overwrite differing prior outputs. `final-manifest.json` binds the owned files, observed dependencies, both original inputs and all retained evidence. Earlier freezes remain unchanged.

Key pins: original report `12751ad5e29ef686a654884baa716d6b9446cf489ccf5194d91d7d3eb5845f24`; paired report `bc1425ffb0696f9c5b1c6bf889ba37d3ccdd619ddc41064cf631c263d271fa23`; providers `ca5fb91813a9691ef4cf1907082cb3a1b6324c05364f2124ebe64ba6795851b3`; fixture `f2568ee3474ac3699d849953c80abf7fc6ce26a5253ad86a4586d958bb1bfe37`; audit `028bd4eb1db0c9e0f39f27c4355dc35a0ce4b5e435931f39488f593099868be2`.
