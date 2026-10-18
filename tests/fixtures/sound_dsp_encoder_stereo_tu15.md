# Original TU1.5 encoder stereo calculation

The native helper ports the EP stereo overlay's coefficient setup, optional
scale, convergent rounding, 24-bit limiting and FIFO 16-bit conversion. Input
ring order is FL, C, FR, RL, RR, LFE. The original stereo overlay reads the
first five rings. This helper does not replace GP effects, encoder startup,
metering, routing, packet completion or output timing.

The fixtures come from executing the complete original EP boot with retained
scratch bytes and the pinned analysis emulator. Each case declares six input
channel blocks and the two original scale words, then observes the first real
FIFO0 packet and the DSP's left/right 24-bit buffers. Expected native outputs
are never supplied to that execution. The reference's shared emulator model
is not a physical-console or native playback proof.

Generate into a fresh directory under the required memory guard:

```bash
python3 tools/tag_match_memory_budget.py \
  --output-root analysis/ep-stereo-fixtures-NEW-guard --timeout 120 -- \
  python3 tools/export_sound_dsp_encoder_stereo_tu15_fixtures.py \
  --capture analysis/opening-cinematic-20261015/dsp-live-original-v266/capture \
  --oracle analysis/opening-cinematic-20261015/dsp-encoder-fixture-v272/original \
  --output analysis/ep-stereo-fixtures-NEW
```

The oracle source is `tools/oracle_sound_dsp_encoder_stereo_tu15.c` (identical
to the retained source beside that executable). It links the
unchanged xemu DSP/DMA sources pinned in `dsp-installed-xemu-source-v222` and
loads the owned capture; it is not a game dependency. The capture's bootstrap,
scratch table, logical image and shared GP/EP pages are checked by
`tools/audit_uploaded_sound_dsp_tu15.py`.

Copy the complete output to the previously absent ignored directory
`tests/fixtures/sound_dsp_encoder_stereo_tu15` to enable CTest
`original_sound_dsp_encoder_stereo_tu15`. The 100 cases cover isolated
channels, silence, gain enabled/disabled, signed scale values, half boundaries,
rails and deterministic random input. Every raw output word and interleaved
16-bit sample is compared (102400 values). Each input is 1538 little-endian
u32 words: enabled, scale, then six planar 256-sample channels. Each expected
file is left/right 256-word u32 planes followed by 512 interleaved i16 samples.

The private control that omits the original rounding fails the isolated FL
case by one raw bit (`1048575` versus `1048576`), before FIFO conversion.
