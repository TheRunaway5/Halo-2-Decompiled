# Retained sound metadata and original first-person constants

The native source sound request owners now have a retained provider for every
resident sound definition and sound-class record in the current 23-map set.
Normal and GCC AddressSanitizer/UndefinedBehaviorSanitizer checks match all
29,780 complete 20-byte sound definitions and 1,242 complete 92-byte class
records. The provider retains its actual cache after the facade is destroyed
and releases the cache after its last callback is released.

This is metadata ownership for source requests, not live sound playback. No
event consumer, listener, sound source, scheduler or SDL voice is installed by
this change.

## Source records and boundaries

`Cache::load_source_sound_definition` resolves a real resident `snd!` tag with
the complete datum and returns its literal 20 bytes. NONE, another tag group,
or an absent resident record is rejected. `Cache::load_source_sound_classes`
imports the actual `sncl` block count and every 92-byte record through the
cache's checked metadata address translator. The interpretation and extents
were already established by `SMG_AUDIO_ADAPTER_1_1_RESEARCH.md` and
`MP_SOUND_VARIATION_NATIVE_INTEGRATION.md`; these getters do not depend on a
sound having a supported permutation count.

`SoundMetadataStore` owns a shared cache and copied resident records. Its two
callbacks return full records by value and retain their backing independently
of the facade. Unknown datums and negative/out-of-range class indices stop
explicitly. These are imported resident metadata; later source class-state
mutations still require their original runtime owners. The store does not
initialize fades, enable flags, availability, RNG, voice records or cache pins.

Independent exports use the source cache reader to copy complete raw records
from all 21 previously checked maps and the two final DLC maps. Both exported
fixtures and reports are byte-identical:

- Fixture SHA-256: `3ca2fdfa277d5494696bf97a3578bda744acdebc0a4651503e5fc3e016ad1c50`.
- Report SHA-256: `f750772e6951ff065711dd3c91b207d657e85fe69c43c89d06135eb717e072d3`.

Each map has 54 class records. The sound definition count includes repeated
resources across maps; it is not a count of distinct encoded samples. The
test also compares caller copies, missing-input rejection and backing lifetime.

## Immutable first-person inputs

`OriginalData::first_person_sound_constants` is present only for the two
authenticated update images. Their complete instruction correspondence and
immutable operand authority is
`analysis/multiplayer/integration/fp-sound-request-native/u5-final/providers.json`
(SHA-256 `b1342c75193140948eb52f37b9fc7d6a09fe9965e2d56b70633a562e3030384f`).

| Input | 1.1 source | 1.5 source | Imported extent |
| --- | --- | --- | --- |
| Position basis pointer | `46FC28 → 447144` | `474CE8 → 44ABEC` | 12 bytes |
| Forward basis pointer | `46FC48 → 447180` | `474D08 → 44AC28` | 12 bytes |
| Ordinary callback identity | `44B14C` | `44EBF4` | Complete resident 32-byte table checked |
| Weapon callback identity | `44B16C` | `44EC14` | Complete resident 32-byte table checked |

The importer dereferences the original position/forward pointers rather than
embedding replacement vectors. The test compares both vectors and both
callback identities with the independent source authority for each image.
Callback identities remain source identities; they are not callable host
pointers or SDL voice handles. Native callback behavior must be supplied by
the corresponding complete source owners. Unverified base-image constants
remain absent.

The mutable active BSP word (`46FB64 → 474C24`) is deliberately excluded.
Fresh executable bytes at that address cannot supply live BSP state. Current
weapon node/world transforms, valid object/location/velocity, camera and local
player relationships, source admission, RNG ordering, voice lifecycle and
playback remain required. No new launcher is published by this checkpoint.

## Validation record

CTest `original_sound_metadata_store` passes in the actual production cache
library: normal 0.25 seconds, sanitizer 1.85 seconds, identical full-record
counters. The independent exports, copied binaries/logs, source snapshots,
owned input hashes and preserved earlier resource checkpoints are recorded in
`analysis/multiplayer/integration/sound-metadata-provider-20261008/frozen/manifest.json`.
Only Linux native execution was checked; no physical audio or Xbox sample
comparison is claimed.
