# Live Warlock and Backwash rain

The subsequent [background weather checkpoint](SOURCE_WEATHER_LIVE_PLATES.md)
adds live Lockout/Warlock plates and preserves these snow/rain captures exactly.
Both weather test scripts now select that newer verified executable. This
document preserves the original rain delivery and its measured results.

Ordinary matches now submit the original type-2 weather rows as line pairs.
Previously the live renderer required a sprite before inspecting the primitive
type, then skipped line submissions. Both authored rain systems have a NONE
sprite, so their simulation ran without a visible draw. The retained pre-fix
on/off captures changed zero pixels on both maps.

The new `SourceWeatherLineRender` uses the original VS22/class0 program and
39780 rain combiner. It preserves the existing 391F0 immediate endpoints,
packed colors, per-view capacity partition and authored width: Warlock has
70 particles and width 2; Backwash has 150 and width 1. Rain needs no bitmap.
The match compositor records each view after late world effects and before
first person, using the full target viewport, that view's source camera
constants, and its scissor. The snow point path continues separately.

The retained xemu tree `fc24584ce88f0915ad7f04775bb7712c2e3f49ee` supplies
the line's perpendicular third vertex and perspective W-depth reconstruction.
Both retained source blobs match their Git tree hashes. This is an explicit
emulator arithmetic profile, not physical Xbox line-raster evidence. Native
Vulkan supplies line coverage and admitted device widths; the current native
depth offset/factor inputs are explicitly zero. Unsupported logical-device
features or widths fail admission instead of silently replacing the line.
RGB-only SRC_ALPHA/INV_SRC_ALPHA blending and W-depth testing remain enabled;
rain does not write world depth.

The controlled GPU test checks visible coverage, perspective W-depth occlusion,
width 1 versus 2, W clipping, zero alpha, color masks, unchanged depth/target
alpha, and preservation of valid bindings after a rejected packet. It produced
512 visible pixels and 344 pixels with the controlled intermediate depth.
The source asset tests and three device-event/world/network regressions also
pass in the same fresh build.

The native finite on/off/repeat comparisons show 2,754 changed RGB pixels on
Warlock, 1,903 on Backwash, and the existing 2,452 on Lockout. Each enabled
repeat is byte-identical. These captures use explicit finite-test cadence and
random seeds; the weather draw suppression leaves simulation running.
Commands, logs, images and subsequent ordinary local/split/LAN observations
are retained in
`analysis/map-world-features/weather-visible-rain/accepted-v1/`.
All fifteen live cases pass with zero Vulkan validation errors, unchanged
frozen source/shader hashes, and successful four-view UDP joins on each rain
map. These LAN checks exercise the native weather renderer and session flow;
they do not establish original network scheduling parity.
The frozen source/build is
`analysis/map-world-features/rain-render-snapshot-20261012/`, containing the
previous device-event checkpoint plus eleven rain-related source inputs.

Run `tools/run_rain_test.sh` for Warlock or
`tools/run_rain_test.sh backwash` for Backwash. Add `--local-players 4` for
four local views. `tools/run_lockout_snow_test.sh` selects the same verified
runtime. At this checkpoint the test scripts resolved to executable SHA-256
`f58a05ef51fd5808fd62f985995cd2001bdd387a3f1d30f598b26db4c839a186`.
The ordinary title launchers retain the separately delivered menu runtime.

The all-map goal remains active. Background plates now have the subsequent
live checkpoint linked above. Complete original platform elapsed/RNG scheduling,
original async cache chronology and physical Xbox
rendered parity still require their own live evidence. This rain checkpoint
does not complete map scripts, attached effects or other world interactions.
