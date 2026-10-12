# Sound cache requests and stream construction

The production cache library builds the original sound-cache request owner and
the original stream format/construction CPU owner. This checkpoint advances the
sound path toward native playback; it does not fix live reload audio or establish
all-map audible effects.

## Original stream evidence

Whole bodies `220520`, `2215F0` and `2B4970` were paired between update 1.1 and
the supplied final update executable. Queue methods were identified from each
original queue's first vtable entry: PCM `2B4530` and ADPCM `27E760`, with the
complete original float-conversion helper `32909C`. The PCM method writes the
owning record; the ADPCM method is an original empty `ret 4`. Their final-update
counterparts match after relocation normalization. The separately paired
`222CA0` effects routine has not been ported by this checkpoint.

Two independent executions of each original executable agree for 174 cases:
ten explicit format ABI controls and 164 stream-construction prefixes covering
all 82 indices in the original startup policy for each executable. The native
comparison checks 3,860 ordered stores, 24,969,696 complete retained output bytes
and 164 exact requests to the mandatory native stream-creation service. It stops
before that service returns. Original instructions were executed unchanged;
creation results, pause results, device ownership and voice identities were not
supplied by the comparison.

The format owner writes 18 of its 20 bytes for PCM. The caller therefore supplies
explicit raw scratch backing; the last two bytes are preserved. The original
conversion helper is represented only for the authenticated immutable float
operands 1.0 and 2.0. Other operands stop rather than using a guessed conversion.
Queue storage is declared original raw ABI input, not a claim that native queue
initialization has completed. Driver backing comes from the retained original
startup allocation prefix, which also has no admitted device.

The original-data importer loads seven immutable words per executable, 56 bytes
total: the two format operands, two queue identities, two first-method identities
and completion callback identity. Mutable queue fields, driver state and scratch
storage are separate owners. The base executable has no verified mapping and
retains an absent optional value.

`source_sound_driver_stream_construct` also translates the original CPU
continuation after platform creation, including its unconditional
`Pause(stream, 0)` call even when creation returned a failure. Those instructions
have been inspected, but that continuation is outside this differential
checkpoint: no actual creation result was provided. It must not be treated as
verified driver admission or playback.

## Actual cache reads

The immutable cache-request dependency is
`analysis/multiplayer/integration/sound-cache-request-native/final-manifest.json`,
SHA-256 `2568f5ded360ee5e8d7fd9967045f7c12a11fbb462b23f0e1beaf7d69b9c07dc`.
Its production test composes three original cache requests with the actual
worker CPU path and three real reads from the owned `shared.map`. Eighteen full
phases match 8,346 ordered stores and 65,779,560 retained bytes. The original
worker writes completion; the test does not synthesize it. Source flags 0, 4
and 5 are checked before and after the actual work.

This composition uses serial phases. It does not establish safe simultaneous
game/worker memory access, cache reclamation during playback, a live driver,
packet submission or audible output. The retained native-thread dependency is
separate evidence of actual host threads and synchronization, not a substitute
for those missing source lifecycle owners.

## Production validation

Fresh production normal and ASan/UBSan builds pass the registered stream,
immutable-import and cache-request tests. The independent stream validator
also builds copied, hash-checked source snapshots with warnings as errors in
normal and sanitizer modes. Its report is
`analysis/multiplayer/integration/sound-driver-stream-native/validated/report.json`,
SHA-256 `59d736c37d7d627609eb07448fc996c23bb0f38f9d27e80eed9ce8d2b1c14cc2`.

Canonical original reports, independent repeats, fixtures, observed source,
production archives, test binaries and build flags are frozen under
`analysis/multiplayer/integration/sound-cache-stream-production-20261009/frozen`.
Exploratory failed checks remain separately retained and are not canonical
passing evidence. The current launcher was not rebuilt or replaced by this
checkpoint. Reload, grenade throwing, canister interaction and full live sound
integration remain open requirements.
