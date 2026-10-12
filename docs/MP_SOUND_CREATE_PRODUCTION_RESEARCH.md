# Sound creation production integration

The cache library now builds the complete `source_sound_create` CPU owner.
Its owned-data importer loads the eleven immutable arithmetic words from each
verified update executable. The base executable has no verified mapping and
retains an absent optional value. The import contains 88 bytes across the two
updates. Mutable delay scale, source/controller pools, class fades, local RNG,
map clock and cache loading are separate owners.

The upstream original/native checkpoint is
`analysis/multiplayer/integration/sound-create-native/final-manifest.json`,
SHA-256 `6483852f56ef577383d3fd80edb7edec16a942f0067d654742e9370c95026318`.
The eleven-word provider packet is
`5be92a49dfafe878d86f5aa9481471541c3cf40939c5633f91a8d039126054bb`.
The production importer compares the actual words against that packet;
it does not import executable zero-initialized mutable state as runtime state.

Fresh production normal and ASan/UBSan builds pass both registered tests:

- 138 actual original creation prefixes across 23 maps and both updates,
  14,858 ordered writes and 65,947,824 complete retained bytes.
- All eleven imported words per update, with the unverified base mapping absent.

The creation comparison reaches 138 exact mandatory cache requests. Its
callback stops before supplying a result. This verifies the creation CPU
prefix and its request arguments, not a returned sound, admission, voice,
streaming completion or audible playback. The independently executed original
continuation reaches a real synchronization dependency; no wait result is
provided here. Reload and all-map sound effects remain live-integration work.

Validation logs, production archive, test binaries and observed source
snapshots are retained under
`analysis/multiplayer/integration/sound-create-production-20261009/frozen`.
The archive contains unrelated cache modules; these focused checks do not
claim an independent audit of those modules or a fresh launcher build.
