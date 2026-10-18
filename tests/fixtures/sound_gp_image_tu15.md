# Original TU1.5 GP image preparation

`load_sound_gp_image_tu15` reads only the authenticated owned English TU1.5
executable. It preserves its embedded GP control program, decodes the seed
table and all15 effect programs, retains the unencrypted final return and
initial data, and exposes each effect's original program/state binding. Raw
descriptor fields retain their original units. The source resource's request
word is3; image preparation preserves it without supplying readiness.

The native test compares every28768 decoded resource byte against unchanged
original CPU execution and every15684 control/effect program byte against an
authenticated original live upload. It checks the initial data,15 prologues
and scratch geometry, and rejects a different existing input file. A private
decoder with79 warmup iterations instead of80 fails before accepting an effect.
Both GCC and Clang comparisons pass. The current-root CMake target and CTest
pass separately under the existing memory limits.

Create fresh evidence directories with the existing serial memory guard:

```sh
python3 tools/tag_match_memory_budget.py --output-root NEW_ORIGINAL_GUARD --timeout 120 -- \
  /tmp/halo2-fov-reference-env/bin/python tools/oracle_sound_dsp_image_tu15.py \
  --output NEW_ORIGINAL

python3 tools/tag_match_memory_budget.py --output-root NEW_EXPORT_GUARD --timeout 60 -- \
  python3 tools/export_sound_gp_image_tu15_fixtures.py \
  --original NEW_ORIGINAL --capture ORIGINAL_LIVE_CAPTURE \
  --capture-decoder-reference PINNED_DECODER_FIXTURES --output NEW_FIXTURES
```

The live capture report pins its decoder fixture report and observer. Keep
those original inputs; do not overwrite them with regenerated reports.
`--observer` selects a capture's retained observer source when necessary.
The exporter authenticates that complete capture before retaining its uploaded
program, and compares all newly decoded effects with it. Copy the previously
absent `NEW_FIXTURES` directory to the ignored
`tests/fixtures/sound_gp_image_tu15` to enable `original_sound_gp_image_tu15`.

Current evidence is in `analysis/opening-cinematic-20261015/`:
`dsp-gp-image-original-v284`, `dsp-gp-image-reference-v286`,
`dsp-gp-image-native-v288`, GCC/negative/Clang guard jobs v289/v290/v292,
and current-root CMake/CTest v291/v293. The first comparison v287 failed
because the test incorrectly expected source request word0. Inspection of the
unchanged resource established3; the production loader needed no flag change.
Final current-root builds, CTests and native export validation are in v300/v301;
every exported byte reproduces the original bootstrap comparison input. The
oracle tool source also matches the tested original probe exactly.

This verifies immutable image preparation, not the SDK's allocation,
descriptor relocation, device activation, upload handshake, DSP execution,
voice routing, final PCM/timing or complete cutscene engine. The two analysis
instruction cores remain separate from the game; no Rust or GPL reference
emulator dependency is introduced by this native loader.

## Original GP bootstrap execution

`export_sound_gp_boot_inputs_tu15` is a separate CMake analysis target linked
only to the native image loader. Its two arguments are the owned executable
and a fresh output directory. It prepares an explicitly zero-backed offline
scratch image, with the original control/image/header bytes, plus effect/state
expectations. This is not the SDK allocator or descriptor relocation service.
The final exporter writes its word files explicitly in little-endian order.

`tools/oracle_sound_gp_boot_tu15.c` is analysis-only. Build it separately against
the pinned xemu reference sources/shims; optionally define `USE_JIT` and link
the pinned MIT instruction core for the second execution. Do not disable C
assertions. The retained compiler argument arrays are in
`dsp-gp-final-build-v300/commands.json`. Neither interpreter links into the game.
Its arguments are the exported input directory, fresh binary result path,
frame count and32-bit bus mask. Run every execution serially through the same
60/64MiB/zero-swap guard.

The probe starts at reset PC0, executes the real bootstrap and image-request
path, checks the program-loaded bytes and all15 initialized state flags, then
runs complete GP control-loop frames. It checks that the original DSP program
clears the request from3 to0 itself. It saves every32-bus mix output and final
PRAM/X/Y/scratch byte. Silence16 frames, direct-bus1024 frames and all-bus1024
frames match exactly between the two cores, including48910966 original
instructions per core and the repeated output-ring wraps. There is no captured
RAM, supplied CPU return, register patch or ready-word write.

The private P98-clear omission v299 fails the actual post-execution header
check. The earlier v298 zero-request control fails only the declared input
check and is not evidence of original-program behavior. Both failures remain
preserved. The original instruction cores still share the pinned reference
peripheral model, including its timer and DMA timing limitations. Matching
these executions does not establish hardware timing, full SDK/device ownership,
native PCM, live cutscene volume/timing or physical Xbox equivalence.
