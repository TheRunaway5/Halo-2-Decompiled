# Original 1.1 audio settings, host boundary, and unvoiced ownership

The original device prefix remains stopped at the real `XC_AUDIO` saved-setting
request. This research identifies its consumers, measures a genuine paused SDL
provider, and independently executes source-record cleanup kernels using the
original cache and real file services. No EEPROM value, AV-pack value, device
success, initialized flag, voice, or playback completion was supplied. This is
evidence for bounded consumers and provider interfaces, not working original
audio initialization or playback.

The authority is the user's map-pack-disc 1.1 executable
`assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
Actual Lockout and shared.map retain the prior frozen SHA-256 identities. The
new audit verifies all 74 earlier pins without modifying their files. All
reports retain `native_gameplay_update_applied:false`; production was untouched.

## Saved-setting request and source consumers

The unchanged executed pool prefix reaches `379FFD`, calling `2DD56C`, whose
import is `[417C50]`. Its original raw import is `18000080`: kernel ordinal 24,
`ExQueryNonVolatileSetting`. The exact prepared request is:

| Argument | Retained source value |
| --- | --- |
| ValueIndex | `9`, `XC_AUDIO` |
| Type output | `021FCE94`, existing bytes `FFFF0000` |
| Value output | `021FCE98`, existing bytes `FFFF0000` |
| ValueLength | `4` |
| ResultLength output | NULL |
| Source return address | `37A002` |

Those existing stack bytes are not a setting. The kernel query has no returned
status or output. The platform ABI independently confirms this five-argument
request and index. [Pinned nxdk kernel header](https://github.com/XboxDev/nxdk/blob/14d5ee97e73347c973f1f57b68b79ec08c9e77f2/lib/xboxkrnl/xboxkrnl.h).

The following is inspection of untouched instructions, **not execution past
the frozen stop**:

1. `379FEA` calls the query. A negative returned status would clear its local
   value at `37A006`; no such status was fabricated here.
2. `37A00A` calls `33874D`. That helper loads `[417CE4]`, then dereferences it.
   The raw import is `64010080`: ordinal **356, HalBootSMCVideoMode**. This is
   not `XboxHardwareInfo` (ordinal 322). If that actual boot value equals 3,
   source replaces the audio flags with 1. If it equals 6, source clears the
   upper 16 bits and leaves the lower bits. Other values retain the query result.
   No AV-pack value was supplied and the literal 3/6 predicates are not assigned
   unverified physical-pack names.
3. Parameter constructor `3832AA` first reads override `38D148`. It accepts it
   directly only when its low 16 bits are at most 2 and `(value & FFFC0000)==0`.
   Otherwise it calls `379FEA`. The original override is `FFFFFFFF`, so the
   actual prefix takes the query. Store `383310` would publish produced flags to
   `38D144`, but has not executed. Its current cold zero is not a produced
   preference.
4. `381C0A` adapts the non-NULL public interface pointer by subtracting eight,
   then calls `38087C`. After guard `3804DE`, fault global `38D14C!=0` returns
   `80004005`. Otherwise `3808A3..3808B5` writes `38D144 & 7FFFFFFF` to the
   caller's output and returns zero, releasing the critical section when its
   guard acquired it.
5. Original driver constructor `2205C0` would call that speaker-config wrapper
   at `2206A0`. Its instructions at `2206AD..2206D4` store driver byte `+1=1`
   only when the low 16 speaker bits equal 2 **and** bit `10000` is set;
   otherwise they store zero. This state has not been produced in this research.

Pinned primary compatibility definitions identify low values 0/1/2 as
stereo/mono/surround and bits `10000`/`20000` as AC3/DTS preferences. Its
`XC_AUDIO` query obtains the user-setting bytes from the configured emulated
EEPROM and validates the corresponding data before copying them; invalid,
missing, or too-small requests have real error paths. Its EEPROM creator can
initialize a new compatibility EEPROM to stereo, and its boot AV global is a
constant 1 with a TODO. **Those defaults are not the user's Xbox settings and
were not used as a result here.**
[Cxbx saved-setting service](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlEx.cpp),
[EEPROM definitions](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/common/EmuEEPROM.h),
[EEPROM constructor](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/common/EmuEEPROM.cpp),
[boot AV export](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlHal.cpp).

The frozen inspection also retains downstream `3824AC`, `387920`, `38A840`, and
`38A9E4`. They use the same surround/AC3 predicates for source buffer flags,
hardware submission, and output setup. Some touch original APU MMIO addresses.
No MMIO state, completion, or those constructors were emulated here. A host
channel count alone cannot substitute for these preference and hardware paths.

## GetCaps owns different fields from SDL output format

Original `381BEE` adapts the public interface pointer and calls `380810`.
After the same guard/fault predicate, its successful branch copies:

| Original output offset | Exact source owner/read | Driver destination from `220646` |
| --- | --- | --- |
| `+0` | `38D130`, free 2D buffer count | `+2AA0` |
| `+4` | `38D134`, free 3D buffer count | `+2AA4` |
| `+8` | `38D138`, free buffer SGE count | `+2AA8` |
| `+C` | 32-bit sum `38D120 + 38D124`, allocated memory | `+2AAC` |

The four-word layout agrees with primary `X_DSCAPS` declarations.
[Pinned DirectSound types](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/hle/DSOUND/XbDSoundTypes.h).

Original NVAPU constructor `384C7C` installs pointers to `38D130/38D134` at
object `+58/+5C`, then its `384CFE/384D07` stores would initialize them to
192/64. This constructor has not executed. Child constructor `38689A` installs
a pointer to `38D138` at its `+1C`, without initializing that counter in this
function. Its complete SGE capacity/allocation producer is still open. The
earlier genuine pool prefix did execute the memory-statistics owners and
allocation increments, but that partial allocation total is not successful
device capability output.

Thus fixed source buffer policy, source allocation accounting, and host output
format are separate inputs. SDL does not query free Xbox 2D/3D buffers or SGEs.
Cxbx's GetCaps wrapper includes TODO buffer counts of `200`, and its speaker
wrapper has a fixed stereo/TODO path. Those compatibility placeholders are not
retail capacity or settings authority and were not supplied.
[Pinned compatibility GetCaps/speaker implementation](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/hle/DSOUND/DirectSound/DirectSound.cpp).

## Minimum native SDL provider boundary

`tools/probe_sdl_audio_capabilities.py` independently queried the actual provider
twice, without forcing a backend. Both measurements used SDL 2.32.74 through
PipeWire. Six playback endpoints were enumerated; the default returned
preferred 48 kHz, signed 16-bit little-endian, two channels. Several separately
enumerated HDMI endpoints report eight preferred channels; the default is not
assumed to identify any one of them.

The default was opened twice per measurement, with declared application request
48 kHz/F32LE/stereo/512 samples: first `allowed_changes=0`, then `F`. All four
actual opens returned nonzero handles, remained paused with zero queued bytes,
and closed. Both obtained application formats matched the request. There was no
queueing, unpause, audible test, source-device success, or original driver result.

| Provider operation/input | What can be supplied from actual host operation | What remains separate |
| --- | --- | --- |
| Enumerate/preferred query | Actual device names and preferred frequency/format/channels, or actual error | Xbox saved preferences, boot AV-pack state, AC3/DTS passthrough |
| Open/close | Real SDL device handle, actual result/error, obtained application format | Xbox COM pointers, NVAPU constructors, source initialized byte |
| Raw cache backing/read | Actual allocated capacity, actual aligned file bytes, real completion of read; original cache code owns page/pin/ready state | Manufactured source-ready state or source file-registry ownership |
| Voice policy/bookkeeping | Must preserve independently recovered source policies and real allocated native voice resources | Deriving Xbox free-buffer/SGE counts from host channels or inserting TODO capacities |
| Consumption | Future measured submission/consumption at a declared provider boundary | Claiming queued-byte count as final DAC completion or source packet completion |
| Clock | Genuine measured host clock at its declared service boundary | Xbox hardware clock/startup history or an invented scheduler clock |

SDL's preferred queries return frequency/format/channels; they do not establish
physical DAC format. With `allowed_changes=0`, SDL can convert internally while
retaining the requested application format. This also narrows the interpretation
of the earlier paused-provider probe: it proved device access and accepted
application format, not physical hardware format. No earlier file was edited.
[SDL_GetAudioDeviceSpec](https://wiki.libsdl.org/SDL2/SDL_GetAudioDeviceSpec),
[SDL_GetDefaultAudioInfo](https://wiki.libsdl.org/SDL2/SDL_GetDefaultAudioInfo),
[SDL_OpenAudioDevice](https://wiki.libsdl.org/SDL2/SDL_OpenAudioDevice).
Queued-size reports concern bytes not yet sent to hardware; bytes already sent
can still be playing. [SDL_GetQueuedAudioSize](https://wiki.libsdl.org/SDL2/SDL_GetQueuedAudioSize).

A native PC preference may be an explicitly identified host configuration. It
must not be described as recovered `XC_AUDIO` or injected into the original
query to force source admission. Likewise a successful host open supplies its
own real host handle, not a fabricated original driver/DirectSound constructor
return. The recovered source request, permutation/RNG, class, controller and
cache kernels remain authoritative within their declared boundaries; the
provider contract does not authorize replacing the scheduler with a generic
first-permutation player.

## Independently executed unvoiced source ownership

The separate oracle imports the immutable cache fixture and preserves its
boundaries: supplied static arena/file-registry binding, unavailable original
constructor/map clocks and async-thread creation, inherited world/animation
services and explicit seed inputs. It runs the original FP event producing a
rejected `21F220` request, then explicitly passes those actual request bytes to
the original `127750` creation kernel. **That supplied creation call site is
not normal admitted playback.** Genuine cache/page constructors and real host
storage/file providers produce its cache entry and original source datum
`EF730000`. Source `+A8=FFFFFFFF` has no controller and word `+AC=FFFF` has no
voice. Original source callback-table pointer `+14=44B14C` remains intact.

Four cases were rerun in an independent process and match byte-for-byte:

| Case | Source cache pin | Source pool occupancy | Result |
| --- | --- | --- | --- |
| Ready, pending, full `128A70(reason=3)` | `1→0` | remains 1 | Stops before owner-pool dereference `18E2F5` |
| Ready, pending, direct unpin/finalizer | `1→0` | `1→0` | Kernels return; pending real read subsequently completes |
| Reload, loaded, full `128A70(reason=3)` | `1→0` | remains 1 | Same owner callback stop |
| Reload, loaded, direct unpin/finalizer | `1→0` | `1→0` | Kernels return; loaded cache entry remains owned by cache |

The full-remove reason 3 is an explicit probe input. It matches the inspected
normal `129770` call at `1298AD`, after `129710` returns false, but this fixture
does not execute that normal scheduler/validity producer or infer a source
lifetime.

Exact ordered source effects:

1. `128A70` resolves the BC-stride source record. Because its real voice word is
   NONE, it skips `12BD20`. Original `1280B0` tests source byte `+4` bit 1,
   resolves the current sound/pitch/permutation/chunk through real metadata,
   decrements the cache entry's byte `+4` at `12812D`, then clears the source pin
   bit at `128130`. Both pending and loaded cases preserve this order.
2. It reads callback `source[+14][+10]`, which is actual `18E2F0`, and calls it
   with owner datum `00010002`, source datum `EF730000`, and reason 3. The callback
   loads `[4F485C]=NULL`. Execution stops **before** `18E2F5` dereferences `+29`.
   No owner pool, validity flag, callback return, or removal result is supplied.
   It has not reached `D2270` or `128AE0`; the source pool remains occupied.
3. A separate direct-kernel case calls original `1280B0` then `128AE0` with the
   actual source datum. It explicitly omits that owner callback; it is not
   complete `128A70` removal. The actual controller NONE field skips controller
   release `21BD90`. `128AE0` tail-calls `16D4C0`, which clears the bitmap and
   record salt, updates first-free/high-water from 1 to 0, and decrements occupied
   count from 1 to 0. It retains the pool's next salt.
4. In the pending direct-kernel case, an explicitly invoked original async
   worker iteration still reads the real 53,760 aligned ready-resource bytes
   after the source record is freed. The provider writes actual bytes and read
   length; original `1223AC` publishes cache completion. The cache entry becomes
   loaded with zero pins and the request queue becomes empty. This proves the
   bounded separate queue/cache ownership; it proves no audio consumption or
   asynchronous-thread timing.

The loaded reload resource is 32,768 aligned bytes. Across four cases, three
actual reads total 119,296 bytes and are checked against shared.map. Cleanup
leaves both RNG words unchanged from each case's retained pre-cleanup input.
Audio initialized byte remains zero and driver pointer remains NULL throughout.
The audit checks 31 listings/1,977 original instruction rows, including 232
unique executed ownership instructions. Provider-entry replacements and the
unexecuted stop instruction are excluded from the executed listing.

## Remaining owner and integration gates

Original `18A200` is the identified producer of owner pool `4F485C`: it calls
allocator receiver `[51841C]` for `60CC` raw bytes, executes `16D070` with stride
`18` and capacity `400`, sets pool flag bit 2, then publishes the pool at
`18A243`. It allocates its separate `244`-byte state from the game arena and
publishes `4F4858`. Its raw callback reference exists at `447AA0`; complete
dispatcher execution is not claimed. Map producer `18A2A0` would set pool
validity `+29=1` and execute `16D220` before its own state reset.

The inherited cache context has `[51841C]=NULL`; neither that allocator's full
owner nor this constructor/map producer has executed in this new suite. Source
owner callback completion therefore remains a separate gate. Also open are the
actual saved-settings/boot import, device/MMIO initialization, complete SGE
producer, voiced `12BD20` and non-NONE controller release contexts, normal
scheduler admission/lifetime, and genuine audio submission/completion. None is
inferred from the verified unvoiced kernels or the paused SDL queries.

## Frozen artifacts and reproduction

- `analysis/multiplayer/map-pack/smg-audio-host-contract/frozen/suite.json` pins
  both ownership runs, source inspection, actual SDL runs, provenance and tools.
- `source-fixture.json` contains complete before/phase/after ownership states,
  actual requests/services/stores, unchanged saved-setting stop and inspection.
- `audit.json` records repeat/source-byte/file/provider verification.
- `measured/instructions` and `instructions` retain the independently executed
  slice and inspected original consumers, with their distinct provenance.

Run new outputs in fresh directories, retaining frozen paths:

```sh
.tools/python/bin/python tools/oracle_smg_audio_ownership_1_1.py --output /tmp/halo2-audio-ownership/one/ownership.json
.tools/python/bin/python tools/oracle_smg_audio_ownership_1_1.py --output /tmp/halo2-audio-ownership/two/ownership.json
.tools/python/bin/python tools/inspect_smg_audio_host_contract_1_1.py --output /tmp/halo2-audio-contract/source-inspection.json
.tools/python/bin/python tools/probe_sdl_audio_capabilities.py --output /tmp/halo2-audio-contract/sdl.json
.tools/python/bin/python tools/audit_smg_audio_host_contract_1_1.py
```

The auditor exports only this new fixture/audit; it reads every earlier freeze
without modifying it. Source settings/device execution remains stopped even
when a genuine SDL provider is present.
