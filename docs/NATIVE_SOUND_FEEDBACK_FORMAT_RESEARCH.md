# Native GP auxiliary feedback: format and time

Research date: 2026-11-01. The CPU/dataflow join is established within the
retained reference cases. Neither hardware format-2 byte placement nor the
effective VP feedback delay is established by the sources inspected here.
An alignment-independent identity round trip would require an additional
producer/consumer agreement that the current evidence does not prove.

## What the original Halo 2 evidence establishes

The unchanged TU1.5 CPU reference identifies descriptor `391410` as the shared
resource: the GP append uses scratch offset `B000`, and the VP input-page
publication selects page `7FF` and the physical address from `391414`. The
authenticated GP program writes 640 words from bins 11–30 using control
`49E2`, whose format field is 2. These are declared resource-context cases,
not observed peripheral allocation, sample consumption or hardware time.
The XBE pin is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`;
the GP program pin is
`c2527379062a138b53dbaadcb3f9ac5762409ff0e38d2f320aea47f8a3ffd8e8`.
[Original auxiliary-page join](../build/test-results/vehicle-destruction-wet-voice-20261101/original-auxiliary-join.json).

The original SDK cases produce mono PCM at 48,000 Hz, 24 sample bits and a
four-byte stride. Each wet attachment contains 32 samples / 128 bytes. Its
source offset is `(input_bin - 11) * 128`; the eight wet voices select bins
15–22. The native adapter checks the observed initialized owners, attachment,
published cursor and masked format condition
`(format & F81F0000) == E0000000`. `E0000000` is a masked format condition,
not the entire observed voice-format word. Nominally, 32 samples at 48,000 Hz
span two-thirds of a millisecond; that calculation is not a hardware-frame
completion or latency measurement.
[Original SDK/layout cases](../build/test-results/vehicle-destruction-wet-voice-20261101/original-layout.json),
[native layout checks](../src/audio/source_sound_wet_voice_layout_tu15.cpp).

`Sources::logical_view` selects the corresponding retained `Word24` plane.
The output receipt explicitly describes current retained values rather than an
immutable frame or a consumed hardware block. It therefore does not establish
the missing byte conversion or frame ownership.
[Logical source adapter](../src/audio/dsp/native_tu15_sound_wet_return_sources.cpp),
[output receipt contract](../include/halo2/native_tu15_sound_gp_output.hpp).

## What the pinned xemu model establishes

At xemu commit `fc24584ce88f0915ad7f04775bb7712c2e3f49ee`, the DMA implementation
names format 2 as MSB and format 6 as LSB, but leaves their distinction as a
commented FIXME. Both use the same four-byte write of the unshifted DSP word;
both input paths mask the lower 24 bits. That implementation cannot be used as
independent evidence that physical format 2 is lower-aligned.
[DMA owner, lines 43–51, 148–159, 194–225 and 262–270](https://github.com/xemu-project/xemu/blob/fc24584ce88f0915ad7f04775bb7712c2e3f49ee/hw/xbox/mcpx/apu/dsp/dsp_dma.c#L148).

The same model decodes sample-size 2 / container-size 3 as S24 in B32. Its
buffered PCM read loads a little-endian DWORD and calls the S24 conversion.
That conversion discards the high byte by shifting the unsigned DWORD left
eight bits before signed normalization. Thus the emulator's reader matches
its lower-aligned DMA writer. This is consistency inside one implementation.
[Voice-format definitions](https://github.com/xemu-project/xemu/blob/fc24584ce88f0915ad7f04775bb7712c2e3f49ee/hw/xbox/mcpx/apu/apu_regs.h#L215),
[VP PCM read](https://github.com/xemu-project/xemu/blob/fc24584ce88f0915ad7f04775bb7712c2e3f49ee/hw/xbox/mcpx/apu/vp/vp.c#L993),
[S24 conversion](https://github.com/xemu-project/xemu/blob/fc24584ce88f0915ad7f04775bb7712c2e3f49ee/hw/xbox/mcpx/apu/fpconv.h#L42).

The frame function calls VP before GP/DSP processing. That is an emulator
ordering fact, not proof of silicon overlap or a complete one-frame feedback
delay. The VP path also uses a sinc resampler whose own source comments state
uncertainty about hardware interpolation. Consequently, call order alone is
insufficient to qualify effective sample age or cursor advance.
[Frame ordering](https://github.com/xemu-project/xemu/blob/fc24584ce88f0915ad7f04775bb7712c2e3f49ee/hw/xbox/mcpx/apu/apu.c#L230),
[resampler owner](https://github.com/xemu-project/xemu/blob/fc24584ce88f0915ad7f04775bb7712c2e3f49ee/hw/xbox/mcpx/apu/vp/vp.c#L1095).

The subsequently inspected xemu commit
`dc10b4cea96e403d30c17ed53551fdaf023bc17f` still groups formats 2 and 6 and
retains the alignment FIXME. It additionally clears DMA RUNNING after a small
number of status reads. That policy is not a physical completion witness.
[Later DMA source, lines 166–181 and 321–330](https://github.com/xemu-project/xemu/blob/dc10b4cea96e403d30c17ed53551fdaf023bc17f/hw/xbox/mcpx/apu/dsp/dsp_dma.c#L166).

## A logical identity is still a conditional claim

For a 24-bit word `w`, matching lower-aligned encode/decode operations compose
to `w`. Matching upper-aligned operations also compose to `w`. These are
algebraic identities, not evidence that the actual two peripherals use matching
operations. For example:

| Assumed GP write | Assumed VP extraction | Result for `w = 123456` |
| --- | --- | --- |
| `00123456` | lower 24 | `123456` |
| `12345600` | upper 24 | `123456` |
| `12345600` | lower 24 | `345600` |
| `00123456` | upper 24 | `001234` |

Therefore the unknown alignment cannot be cancelled merely because both
endpoints are described as 24-bit. The original CPU descriptors prove what was
requested, while the xemu pair proves what that model implements. Neither is
an independent decoded-sample observation from MCPX. The native DMA owner
correctly keeps `unverified`, `declared_low24` and `declared_high24` distinct;
the two declarations are analysis inputs rather than hardware certification.
[Native DMA contract](../include/halo2/sound_gp_dma_tu15.hpp),
[native transfer owner](../src/audio/dsp/sound_gp_dma_tu15.cpp).

## First-party experiment route found

JayFoxRox's `xbox-tools` has a dedicated `dsp-dma` branch. Its initial commit
`f18baac3f8b40a49847ee5de5fbaa2f464b4d17a` actually selects format 2, fills DSP
memory and prints scratch readback. This is a concrete starting harness, but it
contains no retained result bytes or expected-bit assertion proving the
alignment. The author describes the PR as unfinished and reports testing with
XBDM in the XDK dashboard; that report does not supply console identification
or the missing byte observation.
[Initial format-2 experiment](https://github.com/JayFoxRox/xbox-tools/blob/f18baac3f8b40a49847ee5de5fbaa2f464b4d17a/python-scripts/dsp_dma.py#L159),
[author's PR description](https://github.com/JayFoxRox/xbox-tools/pull/60).

The branch head `c7be10bd75f81eb327117c66a8c2dd6031b671ff` programs 16-bit and
format-6 transfers and prints data. Its named format constants remain useful
inputs to an experiment, but are not themselves measurements. The author's
separate frame-start issue explicitly leaves real-hardware behavior untested.
[Later experiment](https://github.com/JayFoxRox/xbox-tools/blob/c7be10bd75f81eb327117c66a8c2dd6031b671ff/python-scripts/dsp_dma.py#L275),
[frame-start issue](https://github.com/JayFoxRox/xbox-tools/issues/61).

The published MIXBUF tracing script does not close this gap. It reads GP
MIXBUF through MMIO and then removes the padding byte for playback; it is not
a format-2 scratch DMA readback. Its pacing uses host wall time.
[Trace script at `457db540c871876ef41f76a760b81c6c8e3e9464`](https://github.com/JayFoxRox/xbox-tools/blob/457db540c871876ef41f76a760b81c6c8e3e9464/python-scripts/trace_apu_mixbuf.py#L35),
[MMIO trace implementation at `51ee241f71b2ad708e0f0c5519d5bea538867aaa`](https://github.com/XboxDev/xboxpy/blob/51ee241f71b2ad708e0f0c5519d5bea538867aaa/xboxpy/apu.py#L193),
[padding conversion](https://github.com/XboxDev/xboxpy/blob/51ee241f71b2ad708e0f0c5519d5bea538867aaa/xboxpy/dsp.py#L4).

## Next implementation and validation boundary

The following are proposed experiments, not completed tests:

1. Adapt the published DMA harness into a finite on-device capture with a
   recorded machine/backend identity, exact program/descriptor bytes and
   cleanup. Test the original auxiliary control `49E2`, not just a differently
   configured transfer. Use asymmetric words including `000001`, `123456`,
   `7FFFFF`, `800000`, `FFFFFF` and per-bin/per-sample tags. Retain actual
   four-byte scratch results, padding behavior and completion observations.
2. Independently test the original wet voice's S24/B32 interpretation. Feed
   distinguishable upper/lower candidate buffers through an actual SDK-created
   voice with observed pitch, gain, envelope, routing and filter state. Compare
   decoded/mixed samples with appropriate control formats, rather than trusting
   the emulator's enum names. A GP readback alone does not prove this reader.
3. Capture successive producer and consumer sequence tags on-device, with
   actual voice cursors and frame progression. Determine cold first-read
   behavior, partial/start/stop transitions and steady-state sample age. A host
   timer or logical copy ordinal is not a substitute for those observations.
4. Compare a native feedback consumer with those recorded bytes and phase
   observations. Both wrong alignments, wrong bin identity, stale generations,
   and wrong same/previous-frame selection must fail targeted controls before
   enabling the production four-bin route.

This research supplies no new playback receipt, source-ready state or hardware
certification. The separate GP-speaker-ring to EP acquisition investigation is
outside this note. No build, emulator or hardware probe was run for this
research, and no source download or large generated artifact was retained.

## Reproduction pins for inspected external source

Hashes below were computed from the exact retrieved raw source bytes on the
research date. They identify evidence, not successful hardware execution.

| xemu revision | File | SHA-256 |
| --- | --- | --- |
| `fc24584ce88f0915ad7f04775bb7712c2e3f49ee` | `dsp/dsp_dma.c` | `cc1d3db0a7f821cae61184a35295c6cea5ce15e9d129ce15d3329adc5029f8e1` |
| same | `fpconv.h` | `2f88c5c3bf38743d8a8a356e49e8d871408a32fbbb2f85dddea20c3236b4d943` |
| same | `vp/vp.c` | `bb9a86ba10628a036272a7df9af3f33a5438fd7134e563d7cb2473f179145ca0` |
| same | `apu.c` | `300643a11239756c05993f5651cda5dc1fdf77a17c07ab59e31dd2ccd0b44c5f` |
| same | `dsp/gp_ep.c` | `29649306d9ff03b35127d8bababab972273932b9b05175f1425f883918981bf4` |
| same | `apu_regs.h` | `389bbac71b64b8d0b91466840ff0992e05b5a442b8dc5de9d2bc8b03964e97a9` |
| `dc10b4cea96e403d30c17ed53551fdaf023bc17f` | `dsp/dsp_dma.c` | `4a19ab8e1d53a8e58d2301f777d509fc5dbf19daa04deb194b25465ec97a9369` |

