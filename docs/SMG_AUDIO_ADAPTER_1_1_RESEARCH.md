# Minimum source audio adapter contract for disc 1.1

This is a **separate bounded study**, following the frozen [FP sound producer study](SMG_FP_SOUND_1_1_RESEARCH.md). It establishes original pool producers, original selection kernels, and original sound-record creation up to the unavailable original cache allocator. It does **not** establish accepted ordinary scheduler playback, decoded audio, successful Xbox driver initialization, hardware mixing, or audible parity. No production file was changed. `native_gameplay_update_applied:false` remains in every report.

An SDL provider can own real host allocations, devices, streams, completion notifications, and voice handles. The native source scheduler still needs to own admission, source datums, pitch/gain, permutation state, cache pins, virtual voice arbitration, and callback ordering. Returning a made-up successful Xbox constructor or choosing the first permutation is not supported by this evidence.

## Reproducible authority and freeze

All instructions are from the user's disc update `default.xbe.03` SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`. The Lockout cache is SHA-256 `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`; sound chunks are read and hashed from the user's `assets/disc/maps/shared.map`. This is the map-pack **1.1** image; it is not a final-ever title-update claim.

- [Oracle](../tools/oracle_smg_audio_adapter_1_1.py): SHA-256 `5e0a4af31c9a7689da2d753a6bb0cb77f244ca35daa92cc702941e4f239c264d`.
- [Frozen suite](../analysis/multiplayer/map-pack/smg-audio-adapter/frozen/suite.json): SHA-256 `007109dc2ec4447c016b2c604856f9c2bbc0e4f051b7c8004475213d628ec34a`.
- [Source fixture](../analysis/multiplayer/map-pack/smg-audio-adapter/source-fixture.json): SHA-256 `525ed0246e4271a14b23c8148739010f18e1c28955889631cc35ed6781300a22`.
- [Auditor](../tools/audit_smg_audio_adapter_1_1.py), [audit result](../analysis/multiplayer/map-pack/smg-audio-adapter/audit.json).

Six cases have byte-identical independent process reexecutions. The audit verifies 624 listings / 57,324 instruction rows directly against the image, 1,578 metadata records directly against the cache, every enumerated chunk hash, and 83 original LCG stores. It also verifies eleven earlier frozen tool/report/document pins, including the entire previous FP sound freeze. No earlier tool/report/document was regenerated or changed.

To reproduce a case, retain its existing instruction directory, write the new report to a distinct filename, and compare it with the frozen original:

```sh
.tools/python/bin/python tools/oracle_smg_audio_adapter_1_1.py \
  --case ready_create \
  --instructions analysis/multiplayer/map-pack/smg-audio-adapter/frozen/instructions/ready_create \
  --output /tmp/halo2-audio-ready-create-reexecution.json
cmp analysis/multiplayer/map-pack/smg-audio-adapter/frozen/ready_create.json \
  /tmp/halo2-audio-ready-create-reexecution.json
.tools/python/bin/python tools/audit_smg_audio_adapter_1_1.py
```

The auditor exports only this new study's fixture/audit. The prior frozen sound study remains untouched.

## What executed, and what did not

| Case | Original execution | Explicit boundary |
|---|---|---|
| `pools` | Partial `126B20`/`126DE0`; source source/static-voice constructor fragment; real controller/looping/effect/shared-voice constructors; original static voice/free-bit policy fragment | Named static backing only; cache/kernel/clock stops; post-cache/post-driver fragments entered directly; availability never committed |
| `ready_select` | Sixteen actual `21B180` selections and its RNG/state helpers against real ready tag metadata | Direct call with pitch index 0; local seed initially 0; original cache runtime bytes supplied as input |
| `reload_select` | Sixteen actual `21B180` selections against real reload metadata | Same direct call/seed boundary; authored runtime byte `range+5=0` is retained, not normalized by guessing |
| `ready_create` | Real registered SMG FP ready request at tick 1; it rejects unavailable audio; separately, original listener selector and `127750` creation kernel run to missing cache allocator | Actual rejected request is copied into explicit stable storage; creation call site is supplied directly; no admitted scheduler success is supplied |
| `reload_create` | Same, real full-reload request at tick 47 | Same explicit creation boundary |
| `empty_reload_create` | Same, real empty-reload request at tick 124 | Same explicit creation boundary |

These cases execute unmodified instructions. They never set `system+78=1`. The direct creation probes do not change an earlier rejection into an accepted request. They inspect `127750` as a kernel with its original caller inputs. Its caller has not completed, its initial cache read has not completed, and no voice has played.

`system+80` and `system+84` stay at original allocator zero in these partial fixtures. They are not simulated successful platform clock values. The ordinary runtime loaded-map producer of mutable pitch-range history and forced-permutation bytes is not yet proven. Every selector row retains its exact initial/input bytes; the raw reload forced-0 result must not be generalized into a production map-init policy.

## Request producer to preserve

The existing frozen sound study proves real attached SMG `00010002`, owner/current Spartan `00010001`, registered local view 0, actual `right_hand` attachment, original FP event dispatch, and original `18AA90 -> 18AD70 -> 18DDE0 -> 18BA10 -> 21F220`. Ready selects `E5AC0437`; both primary reload variants select `E5AD0438`. The separate `E5AE0439 smg_reload_full` tag exists but is not selected by those primary channel events. See the [previous request contract](SMG_FP_SOUND_1_1_RESEARCH.md) for source-defined R58/P-A0 fields and unused stack areas.

The original owner route changes audible location type 1 to type 0 for the actual local owner relationship. Original `129450` chooses the first valid user-player slot for type 0 and returns distance 0. This study executes that helper against the actual registered source view; the result is listener 0, without supplying an active world listener or a distance result. World-positioned audio instead needs the actual listener producer `128B50`, nearest-listener/distance and transmission logic, location flags, and active camera data. This study does not generalize the type-0 result to distant shots or other players.

The callback table at `0044B14C` contains:

| Offset | Original entry | Role at verified callers |
|---|---:|---|
| `+00` | `18DDE0` | Original request/owner producer; executed in original FP requests |
| `+04` | `18E060` | Called by `129A20` after source gets a static voice; not executed by these unavailable-cache probes |
| `+08` | `18E0D0` | Called by `12AB40` during voice update; original weapon/source notification; not executed here |
| `+0C` | 0 | No callback at that table slot |
| `+10` | `18E2F0` | Called by `128A70` during source removal; not executed here |
| `+14` | 0 | No callback at that table slot |
| `+18` | `18E390` | Original marker-data equality helper; compares ten DWORDs |
| `+1C` | 0 | No equivalence callback at the slot tested by `129C50` |

The native consumer must preserve callback existence, owner, marker data, and invocation phase. A provider stream handle must not replace the original source datum handed to those callbacks. The eight-entry table bytes and original instructions are retained in the new fixture/listings.

## Source initialization and flag producers

Original policy bytes at `00451220` are `28002900010024002400010033000000`; load them from the user's image through the original-data provider.

| Original producer | Published slot / bytes | Original result in bounded execution |
|---|---|---|
| `126B20` beginning | `[4ED950]`, system `20C` | Source allocates/zeros system; sets hardware `+79=1`; initialized `+78=0` remains false |
| `126B8D..126C52` | `[4ED94C]` source pool / `[4ED948]` static voice storage | 384 source records × `BC` bytes; pool valid `+29=1` comes from original instructions; `16D220` resets the real pool; static storage `1200` bytes source-cleared |
| `21BC50` | `[526468]` / `[52646C]` | 64 controller records × `1C`, root `1C`; original pool valid/reset execute; root seed producer halts at original kernel system-time import |
| `21C1D0` | `[5098E8]` | 128 looping records × `D4`, actual original valid/reset and empty bitmap |
| `21F5A0` | `[526470]` | 16 effect records × `14`, actual original valid/reset and empty bitmap |
| `21FEB0` with actual policy pointer | `[5098EC]` | 51 shared voice records × `6C`; capacity comes from policy word `+0C` |
| `126C9D..126D35` | System bitsets `+00/+60/+70`, category spans/counts; static voices | Original policy `36,36,1` makes 73 static slots; all original source/driver/channel sentinel fields and category/free bits retained; stops before initialized commit |
| `223960`, `2239B0` | `[5098F0]`, `[5098F4]` | Explicit separate static backing; original constructor allocates/zeros `360` bytes (54 × `10`); original map reset sets its initialized flag and target/current/time to 0 |
| `126E54..126E63` | System `+1F8/+200` | Original writes both ambient indices to NONE, isolated after blocked clock operation; no clock result manufactured |
| `126DE0` before clock | `[526464]`, system `+1E4/+7A` | Actual `matg` references real `ugh!`; master fade 1 and enabled 1 are original writes |

The normal `126B20` caller checks successful controller, looping, effect, shared-voice, and driver initialization before its final initialized commit at `126D35`. The isolated fragments above do not supply those missing successes to that caller.

The driver policy's first words `40,41,1` are a separate original driver/channel policy. Do not merge them with the 73 static slots or the 51 shared records. Pool validity, occupancy, salts, free indices, raw bitmaps, and all `1200` static bytes are retained from actual producers. The controller root seed/default assignments following its clock call were **not** executed.

Class records come from the actual `sncl E1750001 sound\\sound_classes`, all 54 `5C`-byte records retained and byte-audited. `223A10` updates class fades using actual elapsed-seconds XMM1 from the normal scheduler. It either copies target/sets time 0 when remaining time <= delta, or performs separate SSE subtraction/division/multiply/add, then time subtraction. `223A90` supplies target/time by actual class-name matching; class enable/disable uses `223B30`'s source flag setter (not executed by this fixture). No class gain defaults were guessed.

## Availability, admission, and RNG ordering

The original availability function `1271B0` requires system bytes `78 && 79 && 7A`. `21F220` and `128380` obey this. The actual FP requests in these reports return NONE before `128380`; they consume no admission RNG draw. The separate direct `127750` call omits that outer admission by its explicitly supplied call site.

On an actually available system, inspection of `128380` establishes the order: availability; authored codec/encoding gate; one local LCG draw; strict `random > authored skip_fraction` comparison; pitch/codec validity (`128320`); maximum distance; actual listener selection; distance gain/class gain; -64 dB threshold/explicit location override. This successful branch was **not** executed here. The provider cannot bypass it by always returning a sound handle.

Local RNG uses original `seed = seed * 0019660D + 3C6EF35F` modulo 2^32, then high 16 bits. It is the local seed at original `[4EE9D8]+4`, not the multiplayer shared gameplay seed. Original `21BC50` also initializes a controller/root RNG from CRT `time(0) XOR rand() XOR GetTickCount()`; the original system-time call stops, so that final seed is not supplied. Ambient/controller and other sounds can consume randomness between requests; this study does not establish global audible-world RNG ordering.

Original `127750` then performs:

1. Original `1269B0` gain draw, even where authored variance is zero; optional request base gain follows P flag `80`, otherwise actual image 0.
2. Source scale/class gain conversion `126930`, exact original x87 dB/linear helpers `21B700/21B900`, class/ambient state; strict -64 dB creation threshold.
3. Original optional-effect producer and source pool allocation `16D2C0`.
4. Original delay calculation from location/listener; priority/listener fields; `126A40` pitch draw, even with authored 0 bounds. `127866 FSTP [source+90]` is the float32 store authority.
5. Source stores randomized gain at `+94`, snapshots the local seed after pitch at `+98`, source owner/callback/location fields and original sentinels.
6. Authored pitch range / promotion range (`21B060` / `21B020`), or explicit P flag `400` variant; original permutation `21B180`; `129E00` releases old pin and installs datum/range/permutation/chunk 0.
7. Source scheduling time/flag fields; `1275B0` requests the initial chunk. These probes stop at its actual cache allocation dependency.

Original gain arithmetic stores `high16 * [4650C0]` to float32 first, then uses separated SSE subtraction/multiply/add with actual playback fields `+10/+14`. The ready tag has both fields -6; reload has -3 and 0. Original pitch interpolation remains x87 until caller's float32 store and reads signed playback shorts `+18/+1A`. Do not replace these with normalized volume 1, skip zero-variance RNG draws, or rearrange floating arithmetic. The table named `constants` in the fixture is a capture of image scalar bytes: `0054FA64` is a **mutable propagation-scale image default**, not established runtime tuning. Its producer is unresolved; type-0 bounded results do not establish world propagation delay.

## Exact measured selection and creation results

`21B180` first checks range `+5` forced index, consumes it by setting NONE, and stores selected index in range `+4`, without a RNG draw. Otherwise its helpers read runtime used bits/last index subject to actual class flag `+A bit5`; `21B220` draws candidate indices, rejects used indices, resets all-used while retaining the prior selection as used, draws the permutation skip test, and limits skip retries to `count*2`. `21B670/21B3C0` commit used/last state. Persistent used bits are meaningful only for `count>1`; do not write a permanent per-sound fixed seed or first-permutation override.

[Ready selector report](../analysis/multiplayer/map-pack/smg-audio-adapter/frozen/ready_select.json): selections `0,1` repeated eight times from seed 0; draws per selection `2,2,2,3,7,2,2,2,2,4,2,6,3,2,2,2` (45). [Reload selector report](../analysis/multiplayer/map-pack/smg-audio-adapter/frozen/reload_select.json): raw range begins with forced 0, so first selection uses 0 draws; next fifteen each use 2 (30). That initial raw runtime byte is an explicit input boundary, not a recovered loaded-map reset policy.

The source-creation probes have additional gain/pitch draws, so their first ready selection differs from the metadata-only kernel:

| Direct creation kernel | Actual FP event tick / datum | `source+90` pitch | `source+94` gain bits | `source+98` snapshot | Range/permutation/chunk | Draw count |
|---|---|---|---|---|---|---:|
| [Ready](../analysis/multiplayer/map-pack/smg-audio-adapter/frozen/ready_create.json) | 1 / `E5AC0437` | `00000000` | `C0ED52AE` (-7.416342735290527) | `47502932` | 0 / 1 / 0 | 4 |
| [Full reload](../analysis/multiplayer/map-pack/smg-audio-adapter/frozen/reload_create.json) | 47 / `E5AD0438` | `00000000` | `C0400000` (-3) | `47502932` | 0 / 0 / 0 | 2 |
| [Empty reload](../analysis/multiplayer/map-pack/smg-audio-adapter/frozen/empty_reload_create.json) | 124 / `E5AD0438` | `00000000` | `C0400000` (-3) | `47502932` | 0 / 0 / 0 | 2 |

Ready draw stores: `1269D8 -> 3C6EF35F`, `126A7A -> 47502932`, `21B281 -> D1CCF6E9`, `21B349 -> AAF95334`. Both reload kernels have only the first two stores because the raw supplied range forces permutation 0. These are kernel outputs, **not** production seed initialization or accepted playback traces. The original BC records, pool headers, callback requests, and exact store instructions are frozen.

## Cache/resource producer and completion boundary

The source-selected resource is `ugh!` table `+24` pitch ranges (12 bytes), `+2C` permutations (16 bytes), `+44` chunks (12 bytes). Original selected chunk indices must be retained. `21ABD0` uses chunk `size_flags & 3FFFFFFF`. The 17 graph sound refs / 23 permutations / 84 chunks in the source fixture are enumerated and hashed; enumeration is not a scheduler selection.

`1275B0` calls `21A960(sound_datum, chunk_pointer, 2)`. When `chunk+8` is NONE, valid owner plus load flag causes actual `21AB20`: reserve original cache space with `13EAE0`; source page offset << page shift + original cache buffer; original entry datum insertion `16D390`; publish chunk's cache datum and entry's owner/chunk; set entry pin `+4=0`, reference `+5=0`, used `+3=0`; rounded read size `(size | 1FF)+1` when needed; `ECX=chunk+0` encoded location; then `19D8C0` with six stack args `(rounded_size, destination, 0, entry+2, 5, 4)`.

`19D8C0` masks location offset to 30 bits and selects original file-handle globals from the top two bits. The stock FP chunks here have location 2 and use the original shared-map resource bytes. Other file locations must be resolved by the actual file registration producer; do not assign filenames by assumption. A host file provider can complete a **real** aligned read from that verified file and publish the corresponding completion to `entry+2`. Setting `entry+2=1` before a real read would fabricate the source completion producer.

`21A960` input flags: bit0 waits for completion, bit1 initiates load, bit2 pins. Result bits: 1 pending, 2 ready, 4 pinned. Waiting uses original `SwitchToThread`. Ready changes entry used `+3=1`; pin changes `+4`. `128030` uses source flag `+4 bit1` to pin once; `1280B0` releases one pin and clears that flag before replacement/removal. `12B670` owns current/next voice chunk pins; `12BBA0` advances them from real stream consumption; `12BD20` releases pins and clears virtual voice/channel fields. Cache busy callback `21AAB0` and delete callback `21AAF0` are original instruction listings, not stubbed success. Preserve these reference/eviction conditions in the native adapter.

Source cache initialization `21A6E0` needs a real `208C` cache-entry allocation and `30FC` page-allocator allocation through the actual allocator vtable, original `13E8E0` constructor (`C0`, `E`, `200` policy), and a `300000` physical audio buffer / `XPhysicalProtect`. All creation probes stop before `13EAE0` uses `[5098E4]=0`; they do not manufacture any of these outputs.

## Ordinary scheduler order and voice lifecycle

The ordinary update is **`127290`**, inspected in the frozen [original listing](../analysis/multiplayer/map-pack/smg-audio-adapter/frozen/instructions/pools/00127290.asm). It is not executed through successful playback in this study. Preserve this order:

1. Availability and reentrancy/in-progress `+7D`; driver `220D10` polls original streams/consumption (`DirectSoundDoWork` and real status).
2. Original pause/resume paths (`+7B`, options); resume services voices/controllers, real clock origin update.
3. Driver listener/settings stage `221470`; when not paused, `128140` reads GetTickCount, updates origin `+80`, accumulated time `+84`, elapsed seconds `+1E0`, master fade and ambient timers.
4. `223A10` class fades, `128B50(1)` actual local listener/environment producer.
5. `21BDD0` occupied controller RNG advance; `21D1F0` looping update; `12A820` voice chunk/consumption update.
6. `129770` source/callback/listener validity/removal; `129A20` pending source resource checks and static voice admission.
7. Source-generated transmission arrays `12A850`, per-user `223B80`, `12AAD0`; voice gain/pitch/chunk callbacks and driver requests `12AB40`.
8. Driver `220F90` commits deferred settings and submits/polls streams; controller cleanup `21BEA0(0)`; clears `+7E/+7D`; original cache frame bookkeeping.

`1274B0` is an opportunistic service pump gated by original map/options/time and guarded by `+7C/+7D`; it runs a reduced driver/time/voice/cache sequence. **`126F00` is map teardown**, not the normal update: it waits/services, clears voices/loops/controllers, disables `+7A`, clears `ugh!` and resets hardware effects. `127C10` is a pause/reset/removal path with original source class/state policy. These listings are inspected evidence, not proof of executing those lifecycle paths in the supplied partial context.

Static voice assignment is not "one host sound handle per request": `129E50` evaluates actual class `5C` caps/flags, source/owner equivalence, same-frame ownership limits and existing slots; helpers `129C50`, `12A070`, `12A1B0` select/admit/steal with source priorities/time. `129A20` links source `+AC` to the static slot and invokes callback `+04` only at the source phase. `12AD30` then allocates actual driver/shared channels using source category/free bitsets, source/effect flags and genuine driver status; failure removes/clears through `128A70/12BD20`. These branches have original listings but no successful voice fixture here.

For pitch/gain processing after real voice assignment, original `12A3F0` combines playback pitch, authored scale pitch, range natural pitch, optional effect/Doppler and class/permutation gain; `12B900` constructs driver flags/class/effect/source fields; `2219B0/221B90` perform stream routing, volume/pitch/mix changes, and `12B670` queues/pins actual selected chunks. `221B90`'s pitch-to-driver conversion uses original rate-index base offsets `-11F5/-1F5/-95C`, bounds, rounding and minimum-change tests. `221760` converts the retained pitch back to a playback rate for elapsed-source samples. These must be ported against original instructions and a separate genuine provider fixture before claiming full voice parity. This study deliberately does not substitute standard-library rounding/mixing/loop timing for unexecuted driver behavior.

Removal `128A70`: if source has a static slot, release `12BD20`; release source chunk pin `1280B0`; invoke source callback `+10` if present; then `128AE0` releases effect/controller references and frees source datum. Callback/finished/removal outputs must be source-produced after real completion, not fabricated immediately when host playback is requested.

## Minimum host provider services and remaining gate

A minimum native implementation can port the proven scalar/pool/request kernels now and keep source availability false until real host services are ready. The provider contract below is the source dependency boundary, not an assertion that these APIs are implemented.

| Needed service | Exact original dependency | Required distinction |
|---|---|---|
| Storage | Genuine `NtAllocateVirtualMemory` at `33786F`, slot `417C34=800000B8`; physical arena/protection/cache backing | Host allocator returns real owned storage; source constructors initialize their own fields. Do not return invented original constructor handles |
| Time | `337A45` / `417CD4=8000009C` KeTickCount; `336983` / `417BC8=80000080` KeQuerySystemTime via CRT time | Real provider clock values, original wrap/conversion/order; no supplied successful zero clock; controller seed and global audio RNG initialization remain unclosed |
| Resource loading | `21AB20 -> 19D8C0 -> 1A4870`, source completion pointer entry+2 | Verified real file handle/location, aligned read, completion/error and exact queue phase; cache ready cannot be forced |
| Audio construction | `2205C0 -> DirectSoundCreate/GetCaps`, distance/rolloff/speaker/headroom/Doppler/HRTF/effect/stream/buffer constructors | Real host device/streams and status/errors; SDL-owned identifiers are private provider handles, not XBE source datums or imaginary DirectSound objects |
| Stream/voice work | `220D10`, `220F90`, `2B4B30`, source stream/buffer methods | Real consumed bytes, queue status, pause/flush/completion; source arbitration/pins/callbacks run from those outputs |
| Listening/mixing | `128B50`, `221470`, `221B90`, source effect routing and mix tables | Actual per-view/control/world listener producer and source math; owner-bound type0 does not prove spatial/world behavior |
| Decoding | Actual snd rate/encoding/compression, source-selected chunk bytes, driver stream format | Ready/reload both have rate index1/encoding1/compression1 in original metadata; no decoded output has been proven here; do not assume the ready sound is mono |

No real audio platform provider executed in this freeze. Exact immediate unavailable dependencies are frozen in the reports. The next legitimate extension is a native provider that actually allocates storage, opens/reads the verified resources, obtains real device/stream handles and reports real completion, followed by an original/native differential of admitted `21F220`/scheduler/voice phases. Mutable map-load audio history, complete controller seed initialization, global audio RNG ordering, voice stealing, stream pitch/mix/decoding, listener/world audio and pause/end/removal callbacks remain explicit parity gates.
