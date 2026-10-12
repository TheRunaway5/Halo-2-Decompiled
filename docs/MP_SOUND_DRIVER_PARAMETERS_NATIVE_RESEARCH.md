# Original sound driver numeric parameters

The production cache now builds the original CPU numeric audio owners in
`sound_driver_parameters.cpp`. `OriginalData::sound_driver_parameter_constants`
imports all sixteen literal driver operands from the selected update executable.
The unverified base image leaves them absent. No driver, source availability,
listener, stream, format, effect or voice state is imported as a default.

The native module implements complete12C760 normalization, including its x87
epsilon comparison, separately rounded SSE sub/sub/div, original clamping and
degenerate CL comparison. It implements complete221760's numerical x87 rate
chain and source-selected stored-pitch addressing. The explicit221B90 pitch
and ordinary volume slices retain SSE multiply/add/clamp/CVTTSS2SI and
normalization/multiply/subtract/CVTTSS2SI, respectively. They are named numeric
slices, without claiming the whole driver/routing/change-gating body.

The required floating-point context preserves source rounding and precision.
The complete rate calculation executes the original fild/fmul/fldl2e/frndint/
f2xm1/fscale chain with the imported words, without substituting an ideal
`pow(2,pitch/1200)`. Host caller context is restored. The current instruction
backend is verified on GNU x86; unsupported architectures/toolchains and
unmasked exception delivery remain explicit unavailable boundaries.

The original/u5 authority is independently repeated and frozen at
`analysis/multiplayer/integration/sound-platform-voice-contract/final-manifest.json`,
SHA-256 `335e403664cce23d8ed5548bcb86cced2d276cc3c6928c5db34d0473fce5da62`.
Its provider packet SHA-256 is
`c92d5a3624b21aa7b41d36a658023e4427082aca1541dad1c7f8c684fe9e3d9d`;
scalar fixture SHA-256 is
`354b75bc0af39f21fa209330e78a1551aae4682ca501936e015c21a02f851bff`.
Expected results come from direct original instruction execution. Declared
rate-query backing is a scalar input, without an allocated source voice or
successful device startup claim.

Production CTest `original_sound_driver_parameters` passes normal 0.02 seconds
and GCC AddressSanitizer/UndefinedBehaviorSanitizer 0.18 seconds. Both imported
update operands match every expected case and intermediate value:

| Original controls per image | Count |
| --- | --- |
| Complete normalization | 3,168 |
| Explicit pitch slices | 4,096 |
| Explicit ordinary gain slices | 2,048 |
| Complete relative-rate queries | 2,048 |
| Total | 11,360 |

All 22,720 native paired checks pass, along with 128 exact imported operand
bytes, caller-context preservation, explicit missing-memory rejection,
read-only selected-driver addressing and unverified base absence. Independent
operand exports have identical fixture SHA-256
`863a5f24d5680d3f633bf54c099647c640e5ea350c172530f10708a92913c29d`
and report SHA-256
`e6bc889f9f412a3dfbc8ebb5adb21512d0e35012bab307bc07966ac14a93df1d`.

These comparisons do not establish Xbox hardware/DSP bit or latency parity.
Source routing/effects, parameter-change gates, actual driver method calls,
startup, listener/clock/RNG, admission, factory, arbitration, packet/cache
lifetime and platform playback remain to compose. No live sound consumer or
new launcher is installed. The separate platform contract defines the required
real SDL queue/completion service without source success defaults.

Sources, binaries, logs, literal exports, original inputs and earlier evidence
are preserved at
`analysis/multiplayer/integration/sound-driver-parameters-provider-20261008/frozen/manifest.json`.
Shared CMake/cache snapshots are observed in full; this work owns only the
numeric module, operand import and its registered production test.
