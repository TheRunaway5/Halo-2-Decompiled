# Original 1.1 SMG first-person sound request and platform boundary

The original first-person ready/full-reload/empty-reload events now execute through the previously unavailable `18AD70` leaf. Their actual owner callback, request builder, tag/class lookup and sound-effect dispatch execute without supplying a playback return. All three requests return `FFFFFFFF` through the original unavailable-system predicates. Genuine successful initialization and playback remain unproved: the original setup reaches Xbox kernel allocation and kernel clock dependencies for which this fixture supplies no result.

This is a separate research freeze. No production file or earlier frozen suite was changed. `native_gameplay_update_applied` remains `false`.

## Authority and frozen evidence

The authority is the user's untouched disc 1.1 executable `.03`, SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`, and Lockout, SHA-256 `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`. The corresponding original shared-map chunk bytes are read from the user's `shared.map`; 84 independently hashed chunk records are retained. This work does not identify a later/final title update.

- [Oracle](/home/eric/Developer/Halo-2-Decompiled/tools/oracle_smg_fp_sound_1_1.py), SHA-256 `44777fe55191fe36ccdef4cca53afa3ab3b5720057bc18229ce8e9698111e4a6`.
- [Seven-case freeze](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/suite.json), SHA-256 `f290a4fafe336efa899518e460ec7cbcaf6331e017e8c964067727924ae96641`.
- [Source request/resource fixture](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/source-fixture.json), SHA-256 `52b5571c3763402c0624688a44a73c51777e5e3541987a5d02d7005b9905360d`.
- [Audit](/home/eric/Developer/Halo-2-Decompiled/tools/audit_smg_fp_sound_1_1.py) verifies original instruction bytes, cache records, chunk hashes, earlier dependency pins, actual request fields and independent repeats; its [report](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/audit.json) retains counts and validation limits.

Each of the seven cases has a byte-identical independent reexecution. The audit verifies 308 instruction listings containing 19,565 original instruction rows, 212 cache metadata records, 17 authored graph sound references, 23 original permutation metadata query pairs and 84 source chunk records. The three completed request cases cover 175 first-person after-phase states; every one equals the preceding frozen clock suite's corresponding state with its unavailable audio leaf. This equality proves the bounded replacement's unavailable return behavior, not available audio behavior.

The reusable fixture inherits the frozen real SMG allocation, hand attachment, registered view, original multiplayer profile conversion, actual camera initialization, original unit/weapon/first-person clock and real animation resources from [the clock oracle](/home/eric/Developer/Halo-2-Decompiled/tools/oracle_smg_fp_clock_1_1.py). Its player/unit/world heap/resource residency, clear-world and failed post-loadout projectile boundaries remain explicitly recorded. The prior oracle SHA-256 remains `7d5548e26d3503ea1057aef3202a9285d7e9fa431d0a943c372788b64282b5d6`. Original Unicorn reciprocal/matrix instructions still do not establish Xbox hardware reciprocal bit parity.

## Executed original initialization and exact stops

The `startup` case executes only the actual static-arena allocation argument fragment at `12008`, with `EBX=0` explicitly matching the preceding original `12001` XOR. It does not execute the unrelated preceding disk-cache probe or claim full startup. Original `VirtualAlloc` receives `(0, 23000h, 1000h, 4)` and prepares the kernel arguments. Execution stops before `33786F` calls import `[417C34]=800000B8` (`NtAllocateVirtualMemory`). No arena pointer or kernel return is produced. [Startup report](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/startup.json), [original argument instructions](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/instructions/startup/00012000.asm), [original VirtualAlloc instructions](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/instructions/startup/0033785c.asm).

The `cache_init` case separately supplies static backing memory at `07C00000` with capacity `23000h`, initial used count zero, through globals `517FF0/517FF4`. This is a named storage boundary, not a returned platform allocation or manufactured audio handle. Original `126B20`:

1. Allocates its `20Ch` sound-system record with original alignment/CRC bookkeeping.
2. Zeros all 83 DWORDs, publishes `[4ED950]`, sets byte `+79=1` and `+7D=0`; byte `+78` remains source-zero.
3. Calls original `21A6E0`. Its allocator vtable resolves through `[46FBF8] → 480D68 → 45A480 → 11E150`.
4. Actual `11E150 → VirtualAlloc` prepares `(0,208Ch,101000h,4)`. It reaches the same `NtAllocateVirtualMemory` import stop; no sound-cache pool allocation succeeds.

The source system record, arena use `20Ch`, CRC, attempted allocation arguments and lack of pool publication are independently retained. The original initializer has not returned; reports mark every interrupted call accordingly. [Initialization report](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/cache_init.json), [126B20](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/instructions/cache_init/00126b20.asm), [21A6E0](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/instructions/cache_init/0021a6e0.asm), [11E150](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/instructions/cache_init/0011e150.asm).

The separate original map-reset entry `126DE0` runs against that partial source-produced storage. It reads `[4E791C]`, checks the authored matg sound-global block count at `+C0`, loads the authored block pointer `+C4` and sound-gestalt datum at block `+20`, and resolves it through the actual tag table. It publishes `[526464]=804EEE90`, the actual Lockout `ugh!` datum `F324119F`; it also writes master fade bits `3F800000` at sound-system `+1E4` and `enabled +7A=1`. Original `GetTickCount` loads import `[417CD4]=8000009C` (`KeTickCount`) and stops before the kernel data read at `337A45`. No clock value is supplied, and the later origin `+80`, time `+84`, listener reset/mix and driver updates are not executed. [126DE0 instructions](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/instructions/cache_init/00126de0.asm), [GetTickCount](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/instructions/cache_init/00337a40.asm).

Running map reset after an interrupted initializer is an explicitly separate dependency probe, not the complete normal initialization sequence. The request continuation uses only the resulting unavailable state: `initialized=0`, `hardware_available=1`, `enabled=1`, with original-zero origin/time and no listener manufactured.

## Actual first-person event requests

The source unit is `10001`; its actual allocated attached primary SMG is `10002`, right hand/node 20, with registered view zero. Ready uses the actual normal unit wrapper: the only changed input is the same explicit current-weapon-index `UNIT+212=FF` boundary used by the frozen ready-producer evidence. No direct invented first-person sound event is submitted.

| Original case | Tick of sound event | FP clip | Exact sound datum | Retained hand activity |
| --- | ---: | ---: | --- | --- |
| Ready | 1 | 29 | `E5AC0437`, `sound\weapons\smg\smg_ready` | `0024` |
| Manual full reload | 47 | 31 | `E5AD0438`, `sound\weapons\smg\smg_reload` | `0065` |
| Empty reload | 124 | 30 | `E5AD0438`, `sound\weapons\smg\smg_reload` | `0064` |

Graph `E5AA0435` also contains `E5AE0439` named `smg_reload_full`, but that is not the event datum emitted by either tested primary reload clip. Do not substitute it by matching a host filename to the clip label. [Ready](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/ready.json), [manual reload](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/reload.json), [empty reload](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/empty_reload.json).

Original `1687C2` receives the actual 20-byte type-1 animation callback record. The camera/reference gate and existing callback behavior are preserved by the unchanged source. It calls `18AA90` with `EBX=sound datum`, arguments `(owner SMG10002,NONE,3F800000)`. Original `18ACD0` constructs the marker; `18AA40` chooses callback table `44B14C` in these authored cases. Original `18AD70` receives six stack arguments `(sound datum,1,marker pointer,3F800000,NONE,0)`, `ESI=10002`, `EDI=44B14C`. Its actual `RET18h` performs 24 bytes of argument cleanup. [Request instructions](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/instructions/ready/0018ad70.asm), [caller](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/instructions/ready/0018aa90.asm).

Actual `18EF20` changes request type `1→0`. It scans the original four user-player slots, admits camera modes 0/1, requires a live player's current unit, and tests the owner/current-unit parent relationship in either direction through original `BBEA0`. `BBEA0` traverses actual object parent `+14`, rather than comparing invented listener identities. The current attached SMG passes from the registered player's real current unit. The optional output-view pointer is null in this request; no listener index or listener record is supplied. [18EF20](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/instructions/ready/0018ef20.asm), [parent traversal](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/instructions/ready/000bbea0.asm).

The actual owner callback `18DDE0` executes and returns `AL=1`; its position, compressed forward, velocity and location fields come from the existing source-created world/object context. Original `18BA10` then constructs the play-state request and invokes `21F220`. For these cases, the sound metadata custom-playback index `+0D` is `FF`, and both `18EAC0/18EBA0` return zero through actual tag-group and metadata checks. Original `21F220` tests sound-system bytes `+78/+79/+7A`; `+78=0` takes its actual return-`NONE` path, without calling `128380`, `127750`, choosing a permutation, allocating a voice, or calling DirectSound. Original `18BA10/18AD70/18AA90` return; the first-person hand retains identifier `FFFFFFFF` and stores the low-16 activity shown above. [Owner callback](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/instructions/ready/0018dde0.asm), [play-state builder](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/instructions/ready/0018ba10.asm), [availability branch](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/instructions/ready/0021f220.asm).

The independent `uninitialized_ready` control removes `18AD70`'s old supplied leaf without creating even partial audio storage. It reproduces a real context fault at original `21F366`, reading address `00000078` from `[4ED950]=0`. The request has reached `18AD70 → 18DDE0 → 18BA10 → 21F220`, but has not returned. This establishes the missing system dependency; it is not classified as a successful playback rejection. [Exact stop](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/uninitialized_ready.json).

## Request contract and stack validity

The frozen source fixture exports the admitted, source-defined fields for a later bounded native event consumer. The raw reports retain complete original stack requests and their original stack writes for independent inspection; incidental unused stack bytes must not become default values or tuning.

| `18BA10` input request offset | Verified source field in these cases |
| --- | --- |
| `+00` | Location flags/type nibbles, DWORD zero after actual type resolution |
| `+04` | Input scale bits `3F800000` |
| `+08` | Original explicitly written zero DWORD |
| `+0C/+10/+14` | Actual source-produced position bits |
| `+18` | Original compressed forward DWORD |
| `+1C/+20/+24` | Actual source-produced velocity bits |
| `+28..+2F` | Original owner/world location output |
| `+44` | Platform-playback argument `FFFFFFFF` |
| `+48` | Actual owner datum `10002` |
| `+4C` | Original owner callback table `44B14C` |
| `+50` | Original pointer to the 40-byte marker record |
| `+54` | Null variant input |

Request bytes `+30..+43` include unused original stack contents and are not exported as a valid native field contract. In the actual `21F220` input, the verified request flags are `2D`, priority DWORD `1`, owner datum at `+50`, callbacks at `+58`, copied 40-byte marker at `+5C`, source-data-size ushort `28h` at `+8C`, and platform-playback DWORD `FFFFFFFF` at `+98`. Other optional fields remain gated by original flags; uninitialized padding/unused gain/variant fields are not authored defaults. The constructor scale value is an input to original gain/spatialization processing, not proof that the final mixer gain is one. [Original stores and copy order](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/instructions/ready/0018ba10.asm).

## Real resources retained without invented selection

The actual map-reset-produced sound gestalt binds the real FP graph's 17 sound references. Every authored pitch range and permutation is enumerated without treating an enumeration index as a selected playback result. Original `21AD70` is independently executed for all 23 authored `(pitch,permutation)` pairs and reads the original permutation word `+6`; original `21ABD0` sums actual chunk DWORD `+4 & 3FFFFFFFh` for the actual authored chunk count. All outputs match the original cached records and all 84 chunk hashes are retained. This does not decode the original platform codec or execute streaming/cache residency. [Resource report](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/resources.json), [21AD70](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/instructions/resources/0021ad70.asm), [21ABD0](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/instructions/resources/0021abd0.asm).

`smg_ready` has two authored permutations, each with 53,712 source chunk bytes; `smg_reload` has one, with 75,888 bytes; the separately inventoried `smg_reload_full` has one, with 133,992 bytes. Ready's runtime permutation has not been selected. Metadata format names use the existing local Assembly plugins only as navigation; original cache bytes and executed operands are the authority. [snd! schema](/home/eric/Developer/Halo-2-Decompiled/tools/reference/assembly/src/Assembly/Plugins/Halo2/snd!.xml), [ugh! schema](/home/eric/Developer/Halo-2-Decompiled/tools/reference/assembly/src/Assembly/Plugins/Halo2/ugh!.xml), [matg schema](/home/eric/Developer/Halo-2-Decompiled/tools/reference/assembly/src/Assembly/Plugins/Halo2/matg.xml).

## Inspected subsequent dependencies, not executed successes

The retained original instruction listings establish what full setup still requires. After `21A6E0`'s two actual virtual-memory pool allocations, it constructs the sound-cache allocator and reserves/protects the original physical sound buffer. `126B20` then allocates original sound sources/static voices and invokes `21BC50`, `21C1D0`, `21F5A0`, `21FEB0`, and `2205C0`; only their successful original completion allows it to set `initialized +78=1`. These later successes have not been executed in this freeze.

`21BC50` initializes its controller seed from actual `32803E(0) XOR rand() XOR GetTickCount()`. `2205C0` calls original DirectSoundCreate at `220626 → 383DD7`, then obtains real caps/handles, configures distance factor bits `4043126F`, rolloff zero, speaker configuration, mix-bin headroom, Doppler, HRTF and actual buffers/streams. No provider return, driver handle, callback or clock for these paths is manufactured here. [21BC50](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/instructions/cache_init/0021bc50.asm), [driver initialization](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/instructions/cache_init/002205c0.asm).

Original `128380` also advances the local random seed at `[4EE9D8]+4` only after system availability and codec admission. Its skip/gain/listener/voice admission paths are not reached by these unavailable-system requests. Therefore the previous clock suite's local RNG consumption cannot be generalized to successful audio playback, and an eventual native sound scheduler must preserve this additional source order. [Admission instructions](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/map-pack/smg-fp-sound/frozen/instructions/cache_init/00128380.asm).

The proven integration boundary is the exact animation event datum/tick/activity, actual owner request, resolved local audible type and unavailable return/hand bookkeeping. Actual available playback, listener update lifecycle, resource/cache streaming, codec/platform clock, permutation and gain selection, voice ownership/retirement and local audio RNG consumption remain outstanding.

## Reproduction

Run new source traces into a separate destination, preserving the frozen evidence:

```sh
.tools/python/bin/python tools/oracle_smg_fp_sound_1_1.py --case ready --output /tmp/halo2-fp-sound-ready.json --instructions /tmp/halo2-fp-sound-instructions
.tools/python/bin/python tools/oracle_smg_fp_sound_1_1.py --case resources --output /tmp/halo2-fp-sound-resources.json
.tools/python/bin/python tools/audit_smg_fp_sound_1_1.py
```

The first command's instruction inventory paths differ from the frozen paths; compare its original calls, stores, contexts and tick states independently of that host output-path field. The audit reads the existing seven frozen repeat pairs and verifies their byte identity/pins, then deterministically regenerates only this new sound fixture/audit. It does not modify prior suites or production.
