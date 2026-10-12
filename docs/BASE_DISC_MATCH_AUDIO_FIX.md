# Base-disc countdown crash

An installation prepared from the Xbox disc selects the base `default.xbe`.
Its sound driver and sound creation operands were absent from `OriginalData`;
only the map-pack and TU1.5 executables supplied them. At the end of the Vista
lobby countdown, `NativeVistaFlow::world_loaded` opens the announcer mixer.
That mixer rejected the missing operands and terminated loading with
`Authored multiplayer audio requires selected original driver constants`.

The loader now imports the base disc's 16 driver and 11 sound creation words.
These are original immutable operands. No device readiness, listener, source
pool, RNG state or platform result is substituted.

`tools/research_base_sound_parameters.py` verifies the original executable
SHA-256 pins, finds unique complete corresponding functions and checks their
instruction streams, internal branches, jump tables and literal operands.
It reads each constant through the paired original operand, including the
two immediate pointers used for silent gain and default extra gain. All 27
words equal their independently supported TU1.5 counterparts. Candidate
prefixes locate functions; they are not accepted as proof by themselves.

The ten verified TU1.5/base consumer pairs are:

| TU1.5 entry | Base-disc entry | Instructions |
|---|---|---:|
| `226490` | `21fa80` | 563 |
| `226060` | `21f650` | 38 |
| `224e20` | `21e410` | 35 |
| `12db00` | `12aff0` | 34 |
| `128b10` | `126000` | 245 |
| `127d70` | `125260` | 33 |
| `12a810` | `127d00` | 98 |
| `220000` | `2195f0` | 27 |
| `220200` | `2197f0` | 44 |
| `129b20` | `127010` | 172 |

Generate the local evidence and operand fixture using owned executables and
the existing reference-reader dependencies:

```sh
.tools/python/bin/python tools/research_base_sound_parameters.py \
  --base assets/disc/default.xbe --update /path/to/English-TU1.5/default.xbe \
  --output analysis/multiplayer/integration/base-sound-parameters-20261012
cmake --build build --target sound_base_constants_tests sound_driver_parameters_tests
ctest --test-dir build -R 'original_sound_base_(constants|parameters)$' --output-on-failure
```

The operand regression failed before the fix and passes afterward. The base
driver also passes 11,360 scalar cases from the existing original-execution
fixture, covering pitch, gain, normalization and relative rate under eight
floating-point contexts. Generated original reference payloads remain local
and ignored. Tests return 77 when their owned inputs are unavailable.

This static correspondence does not prove external helper behavior or a
physical Xbox audio waveform. The runtime regression passed the actual
two-player Vista countdown, both player activations and GPU presentation
through match tick 90 (530 multiplayer frames), with the audio mixer enabled
and SDL dummy output. It retained the same resized window through loading and
exited with status zero and no runtime errors. The run used the testing copy's
prepared assets with the newly built main-checkout runtime, in an isolated
launcher directory; it did not replace the testing copy's runtime.
