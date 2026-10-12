# Driver flush, queue, packet and static-buffer CPU owners

The new `sound_driver_owners.hpp/.cpp` implements complete original CPU bodies `2213A0` (stream flush), `2B4470` (static queue shutdown), `2B4BF0` (release pending cache references), `2B48A0` (complete a queued packet), and `222B80` (static buffer construction). Real virtual methods and buffer operations are mandatory services. The naturally empty flush and queue helpers now return without a fabricated voice, device, cache-pin result or callback success.

This checkpoint does not initialize the audio driver or play sound. Nonempty ownership and platform operations remain required. The earlier `192D30` environment dependency is still the original section loader/DSP path: the actual `DSPImage` section exists in both owned images, but earlier evidence stops at `XeLoadSection` ordinal 327 without complete VM/page/reference ownership. This work neither repeats that bootstrap nor treats preloaded bytes as a successful load.

## Original authority and measured coverage

Evidence is in `analysis/multiplayer/integration/sound-driver-owners-native/`. Full normalized bodies and branch/call/operand correspondence are uniquely matched against the authenticated 1.1 and u5 executables:

| Owner | 1.1 | u5 | Rows per image |
|---|---:|---:|---:|
| Flush | `2213A0` | `225CA0` | 75 |
| Queue shutdown | `2B4470` | `2B7E80` | 33 |
| Release pending packets | `2B4BF0` | `2B85F0` | 32 |
| Complete packet | `2B48A0` | `2B82A0` | 76 |
| Static buffer constructor | `222B80` | `227480` | 85 |
| Whole cleanup witness | `220BF0` | `2254F0` | 74 |

All 750 instruction rows and the actually executed listings are checked against original image bytes. The original inputs are the same authenticated executables as the preceding driver freeze (`aaea4c9c…` and `21b207e7…`). No original instruction is modified and no platform result is supplied.

The oracle retains the complete 143,360-byte prior driver-prefix arena, the complete original `21A6E0` entry-pool constructor output, and each image's literal 16-byte static queue record. The current driver/cache pointer bindings reproduce the source-producer outputs as explicit raw ABI inputs. The u5 execution uses the same declared prior-1.1 cache layout; ordinary full u5 startup and later static queue construction are not claimed.

There are 1,140 original cases. Two are independent whole `220BF0` empty-cleanup witnesses: the actual flush and queue helpers return, and cleanup performs its three original stores without any service stub. The new module differentially checks the other 1,138 cases:

| Result | Cases | Scope |
|---|---:|---|
| Natural/raw-control return | 328 | Real empty owners, negative stream-count controls and nonpositive signed pending-count controls |
| Unallocated storage stop | 708 | Explicit positive count/flag/invalid identity controls, with actual voices and packet pointers still absent |
| Real buffer-creation entry | 102 | All indices 0–50 from the exact authored policy `+0C` count 51, on the unvoiced driver-prefix input |

The native code matches all 514 represented ordered stores and complete source regions using the production `NativeSoundStorage` source and actual OS mappings. Expected full region bytes are independently reconstructed from original controls/stores and checked against each original output SHA-256 before fixture export. Normal and GCC ASan/UBSan checks and independent reruns pass. Twenty separately repeated evidence/export files are byte-identical. The initial test-declaration compiler diagnostic is preserved in `native-initial-compile-diagnostic`; the final snapshot builds without diagnostics.

## Exact retained state and platform contracts

`SoundDriverOwnersState` has explicit current `driver_address`, `cache_address`, and `static_queue_address`. The last is the source-selected static owner `4865D8`/`48B8E8`, not the separately allocated global queue `51842C`. No fresh XBE zero is promoted as a later constructed voice or active queue. There are no new immutable operands to import into `OriginalData`.

`source_sound_driver_stream_flush` reads the signed-short stream count at driver `+4`. Each 0x34-byte record begins at driver `+0C`. It sets flag `record+3` bit 2 before requesting the retained methods; its original first missing-voice stop therefore preserves that flag write. The first identity is `record+24`: vtable slot `+0C` with a DWORD output, then slot `+18` with no output, both stack-this ABI. The second identity is `record+30`, slot `+1C`, ECX-this ABI. The request contains the actual identity, vtable address, method address, slot, ABI and output extent; missing backing or an unsupported method stops. The status value is owned by the real method and is not manufactured or used as an admission value.

The remaining original flush writes/branches are retained: clear the low three `record+1` bits, release pending cache references, clear flag bit 2, write raw `C2800000` at `record+0C`, execute the original possible second flush sequence, and clear both pending bit groups at `record+2`. The source re-reads the current count and advances by 0x34. These post-method phases are statically ported; no completed nonempty virtual-method execution is inferred from the unavailable-state controls.

Queue shutdown checks static owner `+8`. Null returns naturally. Non-null requires its real vtable slot `+4` release, clears `+8`, copies the low three packet-count bits into bits 3–5 of the retained record's `+2` byte, then invokes the complete packet owner until its signed low-three-bit count is nonpositive. It preserves original record/queue re-reads; no default method return or artificial packet completion is supplied.

Both packet owners use the original signed three-bit count. Cache reference debit uses the actual packet DWORD at `+8`, low16 index, actual cache pointer at `+44`, 0x10-byte entry stride and byte `+4`, including original byte wrapping. Packet completion first decrements both packed count fields, then debits the actual cache reference, performs the original overlapping move, clears remaining pointer slots, and selects original low mode 1/2 when the remaining count is zero/one. The native `memmove` represents its byte output, not individual original CRT instructions. Actual positive packet/cache-pin ownership and post-debit execution remain unproved live seams; the measured controls stop before the invalid packet's `+8` read. Wrapped negative extents are not clamped to a fabricated zero operation.

Static buffer construction emits the actual 24-byte descriptor `[18, 2010, 0, 0, 0, 0]`, current device, output `driver+1A4C+index*3C`, and null outer identity. The original request-builder experiment stops before `IDirectSound_CreateSoundBuffer` returns, with the actual device still absent. A real negative provider result follows the source `+38` clear/false path. A real nonnegative result requires the original ordered operations: five literal mix-bin/gain pairs `(6,0),(8,0),(7,0),(9,0),(10,0)`; flags `1F`; max/min raw distance `7F7FFFFF` with apply 1; rolloff zero/apply 1; Doppler zero/apply 1; cone outside volume zero/apply 1; then `+38` zero/true. Neither branch is exercised by assigning a HRESULT or buffer identity. Existing SDL stereo output is not a substitute for these mix-bin/3D semantics.

## Integration and next gate

The earlier startup caller can bind its named flush/queue callbacks to the complete new functions when their actual driver/cache/static owner backing is retained. This closes its empty-owner cleanup dependencies. Nonempty methods remain explicit platform services; resources, plugin/DSP construction, saved settings, capabilities, scheduler admission, selected permutations and world/listener ownership remain separate prerequisites.

The next live reload-sound task is the complete original submit `21F220` and owning `21FE40`/controller paths. The raw A0 caller record must come from its complete producer; the existing semantic first-person request omits unowned stack bytes and cannot be zero-extended to manufacture that record.

Test CLI: `sound_driver_owners_tests <analysis/multiplayer/integration/sound-driver-owners-native/fixtures.tsv>`. `native/commands.json`, `native-repeat/commands.json` and `reproduction.json` retain exact commands, compiler/environment, directly observed sources and binaries. The real storage dependency matches its separately frozen parent source. No shared CMake/runtime/renderer/audio/OriginalData files were edited, no shared archive freshness is asserted and physical hardware is untested by this checkpoint.

Pins: original report `b106eae5bc39633b181fffb838cf088cdf286de57987687a0067a51fe91a0993`; paired report `314b994fcc7ab20b7ca9efa888fd253187beb5e93536967efc9044e6f5e3f3b9`; fixture `4d7d9ff993be4989e6aa01bd82ca8d240421b46737a68d66b104428a770fc94b`; audit `05470c332ddfbef2a632a92d00b4c6909284460e1b90f001dfcabd8a9168dd90`. The final manifest binds all owned files, current evidence, the exact old dependencies and both owned images.
