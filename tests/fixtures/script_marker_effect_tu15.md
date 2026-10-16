# TU1.5 script marker-effect requests

The fixture is generated from the owned English TU1.5 executable, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Its192 cases execute complete unchanged `2AA360`, `179700` and `1791C0`.
`BAD60` supplies explicit completed WORLD records/counts. `178E30` captures
the96-byte parameter packet and two28-byte seeds instead of allocating.

Generate into a new path, then copy the resulting JSON to
`tests/fixtures/script_marker_effect_tu15.json`:

```sh
/tmp/halo2-fov-reference-env/bin/python tools/oracle_script_marker_effect_tu15.py \
  --xbe /path/to/owned/TU15/default.xbe --output /path/to/new-original.json
ctest --test-dir BUILD -R '^original_script_marker_effect_tu15$' --output-on-failure
```

The generator refuses existing output. Tests compare both lookup calls,
NONE/count admission, every parameter word and every seed word. The source
backing address is caller-owned and normalized to zero for comparison.
The192 cases include72 actual submissions. The final damage-owner field is a
signed short with zeroed padding; the constructor uses its second independent
marker lookup's output even when that lookup returns count zero.

This fixture does not execute the marker producer, effect allocator, PVS,
random streams, particle/light/lens consumers or GPU/audio playback. The actual
opening tag audit separately retains its two requests and their complete
static part inventories through `tools/audit_campaign_marker_effects.py`.

`original_script_marker_effect_kernel_tu15` runs the same192 cases through the
complete authenticated compiled original bodies. It uses the same two declared
boundaries; it does not expand the fixture's proof to allocation or rendering.
`tools/generate_script_effect_tu15.py` regenerates those three complete bodies
into a fresh output/manifest directory.

`native_opening_script_effect_allocation` separately exercises the actual
retained world, opening scenery constructor, marker lookup and allocator. It
also checks original cinematic controls/LOD, deferred particle retirement and
checkpoint restoration across480 source game/particle phases. It does not
cover custom controllers, per-view collection, lenses, full playback or
Xbox pixels/audio. The scoped results are recorded in
`analysis/opening-cinematic-20261015/script-effect-runtime-integration-v7/RESULTS.md`.
