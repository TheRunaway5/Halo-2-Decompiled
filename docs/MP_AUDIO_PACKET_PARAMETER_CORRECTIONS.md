# Ordinary native sound packet corrections,2026-10-11

The live native multiplayer adapter dropped two authored inputs before sending
PCM to SDL: caller/range pitch tuning and the selected permutation's signed
gain. These corrections improve the existing adapter. They do not connect the
complete original admission, listener, scheduler, Doppler or Xbox DSP owners.

## Original authority

The authority is the unchanged disc1.1 XBE
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`
and owned u5 XBE
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Full instruction correspondence is retained in
`analysis/multiplayer/integration/sound-platform-voice-contract/paired`.
Both new oracles pin that report and the prior unchanged caller-control fixture.
Neither patches instructions nor intercepts callees.

Original12A482..12A4A8 retains caller-adjusted pitch for ordinary output.
12A551..12A563 converts the selected range's signed base pitch to binary32,
subtracts it and stores the resulting playback pitch. The adapter previously
used adjusted pitch only to select a range, then sent random base pitch directly
to driver conversion. `tools/oracle_sound_packet_pitch.py` executes the final
conversion/subtraction/store in both images, using the prior original control
and range outputs. Authored zero random-pitch bounds allow the same cases to
run through actual native selection and SDL submission without replacing RNG.
The97 cases include material sounds and multiple sample ranges on Lockout and
Triplicate. All97 fail before correction and match afterward. Independently
repeated original fixtures and reports are identical.

Original12A603..12A650 adds the chosen permutation's signed byte+4 gain before
clamping ordinary output gain. `tools/oracle_sound_packet_gain.py` executes
that slice and original221CF1..221D14, including unchanged12C760, in both images.
The42 inputs use three actual grenade-bounce sounds on each map and seven
caller controls. They have zero authored gain variance/pitch and one sample.
The new test compares actual SDL mixed PCM against the original integer volume
request, using the real decoded sample. Before correction all42 outputs are2dB
too loud; afterward all42 match within1e-6 absolute amplitude. The amplitude
conversion is the identified native SDL service, not an Xbox waveform claim.
Original repeats produce identical fixtures and reports.

`MultiplayerAudio::State::packet` now applies each selected sample's gain before
submitting its PCM, including subsequent ambient packets. The ordinary impulse
path retains caller pitch adjustment and subtracts the selected range base
before original driver pitch/rate conversion. The observer records that actual
pitch input for regression; it does not create a source voice or admission flag.
Ambient caller-control ownership is still unresolved and retains its current
input. Global RNG order, complete loop phases and scheduler timing remain open.

## Validation and current runtime limit

An isolated copied source tree and build live under
`analysis/audio-parity-20261011/source-snapshot` and `build-stable`.
The original snapshot manifest and subsequent audio overlays are retained
separately. This avoids changing shared header/archive layouts during a build.
All12 focused audio tests pass after both corrections; the separate3,864-case
original admission suite also passes. The source distance correction is covered
by3,543 additional exact original controls.

The original frozen source snapshot builds the native game and rendered audio
test. An actual local1 rendered run reaches diffuse texture allocation and
fails with `allocate diffuse memory: Vulkan result -2`, before gameplay audio.
The report pins the binary/map/XBE and retains stdout/stderr. Earlier shared
builds failed during model preparation and two CPU tests threw `std::bad_alloc`;
the isolated audio/footstep tests pass. These failures are not counted as live
audio validation. The native launchers currently select a separate concurrently
maintained performance build; these isolated artifacts have not replaced it.

## Reproduce

Use new oracle output directories to retain the historical controls:

```sh
.tools/python/bin/python tools/oracle_sound_packet_pitch.py --output <pitch-output>
.tools/python/bin/python tools/oracle_sound_packet_gain.py --output <gain-output>
ctest --test-dir <build> --output-on-failure -R '^original_sound_packet_(pitch|gain)$'
```

The fixtures and provenance are under `tests/fixtures/sound_packet_*.original.*`.
The complete sound playback goal remains active in [AUDIO_PARITY_GOAL.md](AUDIO_PARITY_GOAL.md).
