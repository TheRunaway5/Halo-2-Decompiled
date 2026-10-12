# Original sound creation CPU owner

`source_sound_create` implements the complete `127750` CPU body and its gain,
pitch, volume/class-fade, source-pool, range/permutation and source-record
helpers. Its resource, nonempty-effect and positive-controller dependencies
are mandatory named services. The measured positive paths stop at the actual
cache request; they do **not** establish a returned creation, original
`128380` admission, completed map/device startup, a voice, or audible reloads.

The new immutable checkpoint is
`analysis/multiplayer/integration/sound-create-native/final-manifest.json`.
Prior promotion, submit, audio and release freezes are preserved. No runtime,
launcher, renderer, shader, shared metadata provider or CMake file was edited.

## Source authority and correspondence

The source images are the owned map-pack `default.xbe.03` SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`
and authenticated supplied u5 English image SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
`paired-v2/report.json` and its byte listings independently establish 39 unique
whole-body correspondences. The creator is `127750 -> 128B10`, 245
instructions. Other directly implemented owners include:

| 1.1 | 1.5 | Owner |
|---|---|---|
| 1269B0 | 127D70 | Gain RNG and authored playback gain |
| 126A40 | 127E00 | Pitch RNG and authored short bounds |
| 126930 | 127CF0 | Pitch-controlled gain and class-fade combination |
| 128760 | 129B20 | Class fade and ambient gain |
| 21B700 / 21B900 | 220000 / 220200 | Original x87 linear/log conversion |
| 129570 | 12A930 | Location-dependent distance calculation |
| 2247C0 | 2290C0 | Request effect metadata |
| 16D2C0 | 16E6A0 | Raw source-pool allocation |
| 21B060 / 21B180 | 21F960 / 21FA80 | Range and permutation selection |
| 129E00 | 12B1C0 | Source sound/range/permutation/chunk installation |
| 1275B0 | 128970 | Selected-chunk resource handoff |
| 1280B0 / 128030 | 129470 / 1293F0 | Prior unpin and requested pin |
| 32909C | 32CC0C | Original x87 integer-conversion low word |

The source metadata lookup owners `18EAC0/18EBA0` compose the already frozen
`source_sound_custom_playback` / `source_sound_default_playback`, with the same
retained memory and map roots. No source instructions are patched.

## Genuine preparation and explicit raw ABI

The preparation executes the unchanged prior heap/CRT/game-arena/RNG and
`126B20` sound constructors on the pinned whole compatibility context and
replayed prior measured clock inputs. The prior game-arena producer excludes
`217090` savegame/thread I/O with its actual frame retained. The existing raw
static audio backing remains the declared `0A000000/23000` protocol; complete
ordinary title startup is not claimed.

New whole `223960 -> 2239B0` class-fade allocation/reset executes on the actual
source-created game arena. It produces `82D48294`, advances the real cursor to
5620 and records 382 ordered stores. No class pointer, class row or initialized
flag is supplied. The source RNG remains the produced eight-byte record at
`82D4828C`; sound advances its `+4` word.

Every map executes the whole `126DE0/1281A0` prefix using its owned metadata and
a measured host boot-clock observation through the original `GetTickCount`
data thunk. It stops at the genuine absent-driver access `2217E8/2260E8`.
Source `+78` stays zero. Literal map roots are an explicit ABI composition,
not the complete 88-callback map activation or an active-BSP/listener producer.

The three raw requests are unchanged actual original FP outputs for ready,
reload and empty reload from the prior immutable
`smg-audio-adapter/source-fixture.json`. Their original `21F220` calls were
rejected. Their earlier measured nonpositional local-association result zero
is retained as an explicit raw-ABI input, **not** new successful admission or
a newly produced local player. Each actual map resolves the same sound name
to its own datum. The original 160 bytes, including extra location/callback
backing absent from the semantic FP request, remain literal. Their old owner
identity/callback bytes are copied, not promoted to current live ownership.
Each independent call restores whole constructor/map-prefix buffers; no RNG,
permutation, source count or individual selected field is assigned.

## Implementation contract

`SoundCreateState` contains explicit live roots: system, literal tag table,
current matg/ugh/scenario, source/controller pools, class-fade storage, the
actual eight-byte RNG record, environment, cache pool and the mutable delay
scale operand. Serialized roots or zeroes from the XBE are not live defaults.
The existing retained mutable-tag store can own the ugh bank, but its copy
must persist across selection/clock/request calls. It does not create any of
the other owners.

`SoundCreateConstants` retains eleven literal DWORDs in header field order:

| Field | 1.1 VA | 1.5 VA |
|---|---|---|
| default_extra_gain_bits | 447298 | 44AD40 |
| minimum_gain_bits | 45120C | 455B0C |
| maximum_float_bits | 452044 | 45695C |
| rng_scale_bits | 4650C0 | 46A160 |
| unsigned_conversion_correction_bits | 465028 | 46A0C8 |
| one_bits | 465030 | 46A0D0 |
| upper_gain_bits | 465048 | 46A0E8 |
| db_exponent_scale_bits | 46508C | 46A12C |
| db_exponent_log_bits | 451214 | 455B14 |
| linear_db_scale_bits | 465078 | 46A118 |
| fade_epsilon_bits | 4650E0 | 46A180 |

`measured/providers.json` verifies all 44 bytes independently in each image.
The default-extra-gain pointer is the immediate operand at
`127783/128B43`, independently dereferenced. Mutable `54FA64` is excluded from
this packet: the bounded cold run reads its retained source zero, which is
not an active-map policy or a constant import.

The explicit floating-point context supplies original CW `027F` and MXCSR
`1F80` for these fixtures. The x87 conversion chains preserve extended
intermediates and source rounding; they use original operations rather than
`pow`/`log` substitutions. Caller control state is restored on dependency
exceptions. Exact alternative floating-point/hardware behavior beyond this
measured context is not established.

Source order matters:

- Gain RNG is consumed before the combined gain rejection and before pool
  allocation. Pitch RNG follows successful allocation and distance conversion.
- Source `+90` stores the original pitch value, `+94` stores random gain plus
  optional request extra gain, and `+98` captures the RNG **after pitch and
  before permutation**. It is not the final RNG after selection.
- Range/permutation selection mutates the same owned ugh rows/used bytes and
  advances the same RNG in original order. All authored range counts in the
  23 owned banks are 1..31; larger/invalid stack-local extents fail explicitly.
- The source copies 17 original location DWORDs and the source-sized callback
  payload with exact DWORD/byte store granularity. Ownership and numeric
  promotion selection are distinct fields.
- `129E00` performs the original unpin before installing the new sound/range/
  permutation/chunk. `1275B0` requests the real selected chunk with flags 2;
  an actual returning resource result alone controls subsequent pin flags 4.

Missing memory fails explicitly. `cache_request` must provide complete
`21A960` ownership, not a chosen success/NONE/ready response. The nonempty
`192F20` effect lookup and a positive `21BA00(controller,1) -> 21C070` are also
mandatory named dependencies at their original phases. The observed requests
have no effect work requiring these services and cold availability causes
no positive controller retain. Alternate ambient/fade/effect/controller,
explicit-range, delayed-location, exhausted-pool and successful-return paths
are statically represented but are not falsely described as covered by these
138 measured requests.

## Original resource continuation and remaining live gate

The independently retained 1.1 Lockout empty-reload source frame continues
from `21A960 -> 21AB20 -> 13EAE0 -> 19D8C0`. The source allocates cache backing,
writes the real handle/entry and requests 8000 aligned bytes at source raw
alias `82A47000`; 37 further ordered stores and complete cache buffers are
retained. It then reaches `2DC8B2` through the original wait wrapper and stops
at kernel ordinal 234, `NtWaitForSingleObject`, call `2DC8CF`, return site
`2DC8D5`. Its four arguments are handle zero, UserMode 1, alertable zero,
timeout NULL. No wait status or exclusive ownership is supplied. The earlier
progress message calling this IoCreateFile was incorrect; the raw import and
actual async-owner evidence identify a wait. No file read/completion or
returned `127750` is claimed.

A live connection still requires genuine scheduler/cache synchronization and
queue/file completion ownership, original admission/local association/world
inputs and the downstream voice owner. The real SDL device backend is a
separate platform provider; it cannot supply these source fields or interpret
an unavailable callback as successful admission.

## Validation and reproduction

The original suite covers 138 direct raw-ABI creation prefixes across 23 maps
and both images, 14,858 ordered stores, 65,947,824 complete retained bytes and
138 exact resource requests. Original old/u5 output records, mutable ugh,
RNG, full source arena and request bytes match. The extra cache continuation
is original-only and is not included in the native prefix success count.

Canonical native CLI:

```
sound_create_tests /absolute/path/to/sound-create-native/fixtures.tsv
```

`reproduce_sound_create_native.py --directory .../sound-create-native`
independently repeats whole original construction/clock inputs, all paired
bodies and exporter bytes, then builds/runs fresh standalone normal and GCC
ASan/UBSan source snapshots. No shared archive or launcher freshness claim is
made. `audit_sound_create_native.py` checks every retained instruction against
the owned images, all prior pins, repeated byte outputs and complete banks.
The final manifest pins the observed source/header/test and exact standalone
binary dependencies separately from other chats' ongoing work.
