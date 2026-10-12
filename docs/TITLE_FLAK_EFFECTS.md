# Live title flak

The frontend now creates the five scenery placements in the original
`mainmenu.map` that reference
`effects\cinematics\c01_outro\human_and_cov_firing_cine`. Each has two
effect attachments: `cov_flak_explosions` and `human_flak_explosions`.
Their authored emitter definitions, positions, rotations, particles, bitmaps,
movement, lifetimes, and sprite properties feed the existing native particle
renderer. Imported Vista UI uses this same live scene.

The missing links were retained scenery attachment playback, particle death
effect dispatch, and frontend submission of the live particle draws.
The Covenant primary particle creates `cov_flak_death_explosion` when it dies.
That effect contains plasma smoke, electric bursts, white smoke, flashes,
electric discharge, and shrapnel sparks. The human primary particle supplies
its authored flak appearance directly. Playback continues at the shared
30 Hz scene clock and interpolates between ticks.

Source evidence:

- `analysis/frontend/title-particle-source-chain.json` records original tag
  references, placement data, particle definitions, and authored emitters.
- `unknown_0bc190.md` and `unknown_175bd0.cpp` in
  `tools/reference/halo2-decompiled` recover attachment creation through
  `1766B0`, its retained runtime repeat flag, and event wrap/carry behavior
  through `179190`/`1792B0`. The parent effects' authored loop indices are -1;
  the runtime attachment flag supplies repetition.
- `unknown_248340.cpp` and `unknown_246f10.cpp` recover death dispatch before
  unlinking a particle, its last position, velocity direction, and A/B scale.
  Original XBE constants supply the fixed up/down fallback vectors.
- Original `175EA0` biases each particle location's current bounds center by
  its authored sort bias times .05 along the view forward. `1E4C0`, `42850`,
  and `1E700` place those locations and light volumes in shared transparent
  depth order. The native frontend interleaves those submissions.

The GPU upload retains one mapped uniform buffer and a descriptor bank across
frames. This avoids allocating three Vulkan memory objects per particle every
frame. Shader interfaces export the values consumed by the next stage; the
translated original vertex arithmetic remains unchanged.

## Verification on 2026-10-11

Evidence is in `analysis/frontend/title-flak-runtime-20261011/`.

- `regression-before.log` and `regression-marker.log` retain the failing
  attachment/empty-particle reproductions. `native_mainmenu_flak_explosions`
  then simulates one minute with a fixed seed: ten retained attachments,
  continuing flak, all expected death textures, no unresolved components,
  and a peak of 812 particles. Drawing twice or changing resolution does not
  mutate simulation; batched and individual tick histories match.
- `final-tests.log` records nine passing focused groups, including live
  creation/emission/render inputs, sprite arithmetic, title attachment
  playback, and shared impact/grenade/plasma paths.
- `original_particle_graphics_gpu` compares all 12 retained original draw
  readbacks exactly with both D24 and D32 depth. Its added 1/65/129/1 draw
  sequence verifies buffer growth and reuse preserve color and depth.
- `captures.json` records title ticks 181, 300, and 900, 4:3/16:9/21:9 extents,
  and the main menu, with imported Vista UI and no Vulkan validation messages.
  `before.png`, `after.png`, and `diff15.png` retain an earlier paired title
  frame and an amplified difference showing the restored blue flak bursts.
- `performance-initial/report.json` and `performance-final/report.json` record
  visible 75-frame title runs at the actual 2560x1440 render/presentation size,
  with 15 warmup frames. Reusing uploads improves that bounded workload from
  2.58 to 113.10 FPS with 496 particles at the frozen camera time.
- `title-live.log` verifies an unfrozen 240-frame run of the delivered runtime
  with no validation messages, ending with 624 particles in 160 systems.
  `performance-delivered/report.json` records a subsequent 80.30 FPS sample
  at the same actual 2560x1440 extent while other builds were active. These
  short host timing samples are not a guaranteed frame rate.

Run the focused check with:

```sh
ctest --test-dir build-performance-4k-current-20261011 --output-on-failure \
  -R 'particle_(creation|emitter|render_input|sprite|graphics)|native_mainmenu_flak|native_world_(impact|plasma)'
python3 analysis/frontend/title-flak-runtime-20261011/capture_runtime.py
```

`HALO2_MENU_EFFECT_SEED` accepts a decimal uint32 for repeatable diagnostics.
Normal startup initializes the stream from the source secondary seed formula.
The retained attachment importer currently supports static, unmarked,
function-free scenery attachments with a single constant, delay-free event,
which covers all ten title flak attachments. More general attachment owners
require additional lifecycle services.

These checks establish live native playback and bounded original particle
kernel/graphics comparisons. They do not establish a matched random history
or full original Xbox/Vista framebuffer parity.
