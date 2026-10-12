# First-person sound event, world request and raw scheduler composition

`source_first_person_sound_raw_event` closes the raw request bridge between the existing first-person event producer and complete `source_sound_submit`. It composes the existing event/marker owner, complete `18DDE0` world request and complete `21F220` scheduler without recorded lower-owner replies. It preserves the original caller backing and ordered raw writes. It does **not** make live reload audible: the measured source scheduler returns `NONE` through its genuine cold availability gate, and current multiplayer still lacks the required retained source world/listener/startup owners.

The new files are `include/halo2/fp_sound_submit.hpp`, `src/fp_sound_submit.cpp` and `tests/fp_sound_submit_tests.cpp`. No runtime, renderer, shader, `OriginalData`, shared build-directory or CMake file was changed by this work.

The canonical original report is `analysis/multiplayer/integration/fp-sound-submit-composition/measured-final/report.json`, independently repeated byte-for-byte under `repeat-final`. SHA-256: `fb50999007a4726301aab7885b3dcc581a58655fdde02ebb6d3e2014e6db8ff0`. The canonical `fixtures-final/fixtures.tsv` independently matches `export-repeat-final` and the earlier fixture bytes: `ef33bfc4dcd32a540709a30b09ddc1d29ad238e220afe461d9cc5f7a663deae0`. Earlier attempts remain retained; the final report explicitly records the inherited ready control rather than treating it as an owned inventory transition.

Fresh original execution follows these complete, previously authenticated corresponding bodies:

| Owner | Disc 1.1 | Authenticated u5 |
|---|---:|---:|
| FP event | `1687C2` | `169BA2` |
| Marker/callback producer | `18AA90` | `18BE50` |
| Request/location producer | `18AD70` | `18C130` |
| Object/node/velocity/PVS request | `18DDE0` | `18F1A0` |
| Raw request caller | `18BA10` | `18CDD0` |
| Scheduler submit | `21F220` | `223B20` |

The dynamic evidence executes 1.1 unchanged instructions. The prior whole-body correspondences authenticate u5 structure and operands; this experiment does not claim a complete dynamic u5 world/startup schedule. The XBE hashes remain `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` and `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

Three unchanged prior schedules produce the ready event at fixture tick1, reload at tick47 and empty reload at tick124. These are that fixture's clocks, not canonical-launcher timestamps. The ready case retains the previous explicit unit `+212=FF` stimulus. Other source equipment, local/player association, world, resource, clock and initial RNG boundaries are inherited and recorded. No additional object fields, availability flags, owner result, scheduler reply, selected permutation or device result is assigned. All three actual `18DDE0` calls return `AL=1`; all three actual whole `21F220` calls return `FFFFFFFF`. The new native composition matches 30 ordered world stores, 99 ordered raw construction stores, all160 request bytes, full marker/location packets and retained hand identifier/activity. Submit itself performs no stores in these three cold cases.

`18AD70` supplies a 68-byte input prefix to `18BA10`; only its first48 bytes belong to the represented location record. The final20 bytes remain caller-owned. `18BA10` likewise leaves parts of its160-byte stack request untouched. `FirstPersonRawSoundBacking` therefore requires the full explicit caller marker, location,20-byte tail and160-byte request. It does not zero-extend `SourceSoundPlayRequest`. The bridge preserves priority/flags, seventeen ordered DWORD copies, owner/callback/interface writes, the conditional class-bit OR, ten marker DWORD copies and the data-size write. Its scope is the actual FP caller: marker40, interface`NONE`, no selected-range/permutation pointer. Other `18BA10` callers are outside this API.

The required submit callback receives mutable160 bytes. It must bind their actual source/native storage alias to the complete scheduler and its mandatory dependencies. After genuine success, conditional attention receives the **post-submit raw bytes**, including any effect-controller changes; the earlier semantic packet cannot supply those current bits. A missing submit owner throws after construction without committing a completed hand result. No semantic submit/attention callback is silently reused.

The test consumes actual owned Lockout20-byte `snd!` and92-byte `sncl` records through `SoundMetadataStore`, and actual executable operands through `OriginalData`. Lower object and local records are newly observed unchanged source backing, not native live providers. Complete `source_sound_object_request` and `source_sound_submit` run in the test; no admission, promotion, creation, controller-seed or effect-return callback is provided. The natural cold branch makes those unavailable dependencies unnecessary. This does not establish their positive composition.

Copied-source normal and GCC ASan/UBSan checks independently pass under `validation-final` and `validation-repeat-final`. They link copied observed production Cache/synchronization archives. Their provenance records the exact archive and header snapshots; it does not assert every archive member was freshly built from the observed shared source tree. Canonical test CLI:

```text
fp_sound_submit_tests <fixtures-final/fixtures.tsv> <owned Lockout.map> <owned default.xbe.03>
```

The first missing live inputs remain concrete. `MultiplayerMatch::configure_first_person_event_owner` delivers actual local clock events before replicated clocks, but no current runtime sound consumer is installed. Current `Unit` stores semantic identity, movement and action fields; it does not retain the salted header/body, signed node-bank offset/current52-byte node transforms, physical velocity record, original root location/PVS bank and actual local/player camera associations needed by `18DDE0`. Render-time matrices or `player.position` are not substitutes. The active BSP word is owned by `11D780→11E8E0`, including module callbacks/resource activation; `Simulation::bsp()` alone does not prove that admission. A positive scheduler additionally needs genuine startup/map/class/listener/controller/cache/voice state and actual clock/RNG producers.

The earliest native driver seam is the complete `2205C0` caller's `native_device_create`, corresponding to original call `220626→383DD7` (u5 `387877`). It publishes the allocated driver with initialized byte0 before device creation. A real SDL backend may own a native device and lifetime through that explicitly named platform substitution. It cannot supply the immediately following original `381BEE` sixteen-byte capability output from SDL's48kHz/stereo/float format: the retained compatibility ABI names those words free2D buffers, free3D buffers, free scatter/gather entries and allocated memory. Cxbx's two `0x200` values are explicitly TODO placeholders, not hardware authority. Saved-speaker `381C0A`, distance/rolloff, mix bins/HRTF and effects/routing remain distinct. The saved-setting predicate at `2206D4` is `(value&FFFF)==2 && (value&10000)!=0`; the actual SDL two-channel output cannot determine it. No fabricated capability/settings/plugin values or readiness commit is introduced here.
