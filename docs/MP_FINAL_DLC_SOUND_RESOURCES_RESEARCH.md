# Final DLC sound-resource coverage

This extends the resource checks in `MP_REPORTED_GAMEPLAY_FAILURES_RESEARCH.md`
to the two maps in the user-owned final DLC content bank. It verifies available
sound bytes and the native PC decoder. It does not establish when those sounds
play in a live match.

## Owned inputs

The existing external import manifest authenticates these files against
`/home/eric/Downloads/Halo2DLC.zip` (SHA-256
`927a4b262a57713915e6c473d54d626dc6a77e821f7d19884748d455bc6f2a59`):

| Authored map | Cache | Encoded permutations | SHA-256 |
| --- | --- | ---: | --- |
| Desolation | `content/c/4d53006400000004/derelict.map` | 4,243 | `85c9d98556a0cd8dd6bcce94992425518f8a5753c0adb0c353fddf83696f7efd` |
| Tombstone | `content/c/4d53006400000004/highplains2.map` | 4,378 | `b14a0debdb2e279c7bd7c7cda2b3e2a5cf69ca85f457923c1cfb9de6ca0b1bb1` |

These names and revisions come from the previously authenticated DLC catalog
recorded in `MP_TU_1_5_EXECUTABLE_IMPORT.md`. The older Tombstone cache remains
separate. This work does not replace it or change the launcher map list. The two
map caches remain in the user's external import storage.

## Implementation and checks

`tools/export_mp_dlc_sound_resources.py` requires the explicit content bank,
existing import manifest and shared-cache directory. It checks each map's size
and SHA-256, then exports every resident sound tag's pitch ranges, permutations
and original chunk ranges. The resource namespace is explicitly the native
cache loader's namespace; this is not an executed Xbox file-registry startup.

The existing native resource test now accepts quoted paths, which are necessary
for this external bank's directory names. Unquoted paths from the original
21-map fixture remain supported. CTest registers
`owned_multiplayer_latest_dlc_sound_resources` separately from the original
21-map coverage.

Independent exports produced identical fixtures and reports:

- Fixture SHA-256: `b05422b138f4b95c12b36ee4fe6e29387c96f35cfaf8ad97a1f85b36ab54d970`.
- Report SHA-256: `9efb4b122e375f8bd879ccbe8f93c89ae8d4a109e717c92b30401f0ec3cd2d25`.

Normal and GCC AddressSanitizer/UndefinedBehaviorSanitizer tests both pass with
the same results: 8,621 permutations, 361,503,126 exact encoded bytes, 6,482
original chunk ranges, 4,011 distinct decoded payload/format/class combinations,
zero decode failures, one genuinely silent resource, and 331,040,469 decoded
stereo frames. The normal test also rechecked the original 21-map fixture and
retained its previous results. Earlier sanitizer coverage of that fixture is
preserved in its original checkpoint.

Together, the two fixtures cover 96,875 permutations across the current 23-map
set. Distinct decoded-resource counts cannot be added because maps share
resources. The frozen manifest records independent outputs, copied test binaries
and logs, source snapshots, external inputs, and the unchanged earlier checkpoint.

## Live boundary

This proves resource availability and successful PC decoding. It does not prove
the original Xbox codec's sample output, original sound selection or shared RNG
order, source voice scheduling, listener ownership, attenuation, occlusion,
spatial playback, or live event coverage on any of these maps. The live reload,
grenade and object sound owners must supply those behaviors before all-map sound
parity can be claimed. No new launcher or gameplay fix is published by this
checkpoint.
