# Campaign camera smoothness between ticks (2026-11-22)

The report: in campaign, the camera jitters while moving, and while riding the
Armory tram (the lift out of the armory).

Campaign simulates at 30 Hz and draws as fast as the display allows. The
presentation layer already drew the eye position, actors, weapons and scenery
between ticks. Three parts of the presentation did not use that same
between-tick time.

## Causes and fixes

| Cause | Effect | Fix (all in `src/app/main.cpp`) | Restore old behaviour |
| --- | --- | --- | --- |
| The gameplay view used the latest tick's processed look (`hud_player` yaw/pitch). | Turning advanced only on tick frames, so at 120 FPS three of every four frames did not rotate. | The processed look is presented between the previous and current tick at the eye's fraction (`render_angle`/`lerp`), as `multiplayer_presentation` does. The camera is cut on unit, seat, death or a jump of at least 5 wu. | `HALO2_REFERENCE_CAMPAIGN_TICK_LOOK=1` |
| Machines (doors, lifts, the tram) drew their completed rendered WORLD nodes. Only machines skipped interpolation. | A rider's interpolated eye moved against a floor that stepped at 30 Hz. | `SourceMachineScene::collect_campaign_view` takes an optional node presenter. The primary view passes one that samples `ModelNodeInterpolation` (`capture_render`/`sample_render`), the same history actors use. Static machines keep their exact nodes, so pose-cache hits are unchanged. The secondary texture-camera view still draws completed nodes. | `HALO2_REFERENCE_CAMPAIGN_MACHINE_TICK_POSE=1` |
| With render overlap, the input-latency guard waits before the next block. It waited longer before render-only blocks than before ticking ones, and the block read its frame clock after the wait. | Ticking frames showed the world several ms "older" than render-only frames, so motion advanced unevenly at 30 Hz. | The block's frame clock is read when the draw starts. Input is still read after the wait. The guard's forecast loop is skipped because the wait can no longer change the tick count. | `HALO2_REFERENCE_OVERLAP_SAMPLE_CLOCK=1` |

Tick endpoints keep their exact source words: a fraction of 1 presents the
current tick. Simulation, tick order and input delivery are unchanged.

The trade-off: the view's rotation is now presented at the same time as the
eye position, which is up to one tick behind the newest processed look. This is
the same as local multiplayer players.

## Measurement

`tools/test_campaign_presentation_smoothness.py` (run it under
`tools/run_test.py`) does the following:

- It replays `tests/fixtures/campaign_armory_tram_inputs.json` with a
  continuous turn added at ticks 2000–2299.
- It traces with `HALO2_CAMPAIGN_PRESENTATION_TRACE=path`, which writes JSON
  lines for each gameplay camera, each moving machine root and each draw
  begin/end.
- It runs once with the three reference switches and once without.

Results are from one binary, hidden Xvfb, Original mode, immediate present,
about 35–55 FPS:

| Metric | Reference | Fixed |
| --- | --- | --- |
| Frames that did not rotate during the turn | 32–68% | 0% |
| Turn-rate coefficient of variation | 0.77–1.55 | 0.00001 |
| Camera-to-tram jump per frame, RMS (ride of 44 wu) | 0.019 wu | 0.00027 wu |
| Camera-to-tram jump per frame, max | 0.047 wu | 0.0037 wu |
| Frame clock to draw end, ticking minus render-only frames | 5.7–6.1 ms | 1.8–2.2 ms |

Run the full route (`--ticks 12000`, about 7 minutes per mode) for the tram
check. `--ticks 3300` covers only the turn.
