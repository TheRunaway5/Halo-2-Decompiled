# Initialized scenery idle phase

The stable initialized type6 idle phase is implemented in
`source_scenery_idle_phase.hpp/.cpp`. It consumes the actual caller's clock
step and one explicitly admitted controller update. It does not infer source
visibility, activation, scheduling or elapsed time from an object's birth.

The original path is `10A2F0 →1CB710 →1C68C0 →1C66A0 →1C6560`.
`1CD4E0` updates fade/blend byte counters, rather than the main clip phase.
The phase update preserves the source binary32 operations:

```
advanced = ((clock_step * playback_rate) * 30) + frame
endpoint = frame_count - float_word(0x38D1B717)
looped_frame = advanced - endpoint + authored_loop_frame
```

The source checks delay, playback enable and status bits before advancing.
After the update it retains the loop/completion flags and computes the next
endpoint flag with the same clock/rate arithmetic. Loop subtraction occurs
once per admitted update; it must not be replaced by `fmod` or a calculation
from total elapsed seconds.

`Cache::load_source_scenery_idle_phase_definition` reads the literal selected
jmad record. The bounded implementation requires more than one frame, loop
frame zero, both sibling links NONE, and zero frame, effect and sound event
counts. Unsupported records throw. This prevents the phase helper from losing
real callbacks, weighted variation changes or random-state consumption.

All 42 supplied multi-frame automatic Slayer SCEN owners pass these gates:
34 have 100 frames and eight Gemini lilypads have 250 frames. Original
constructors produce delay/status zero, playback flags `0x003F`, rate
`0x3F800000`, phase zero and zero crossed/next event masks. The actual
`1CB3B0/1DD630` calls at loop boundaries preserve each supplied idle binding.

The untouched original executable ran 1,000 explicitly admitted updates for
each of these initialized controllers. All 42,000 updates performed zero
primary/auxiliary RNG writes and zero `BF600` callbacks. The frozen production
test compares 798 source samples across those lifetimes, including loop
boundaries and the final phase. At tick 1,000 the 100-frame phase is
`0.0009918212890625`; the 250-frame phase is `0.00042724609375`. Both differ
from resetting a native loop to zero.

A separate untouched `1C68C0` differential covers 8,296 explicit original
clock/controller banks and 58,072 output words. It exercises delay, pause,
completion, looping, rate reset, endpoint clamp and signed playback rates.
All output words match. It supplies original-layout clip metadata directly
and uses no function/service hooks. The production test retains 201 of these
independent cases.

Evidence and fixture provenance:

- `analysis/multiplayer-shading/scenery-idle-controller-original.json`
- `analysis/multiplayer-shading/scenery-idle-phase-metadata.json`
- `analysis/multiplayer-shading/scenery-idle-phase-step-original.json`
- `tests/fixtures/retail_scenery_idle_phase.tsv` and provenance
- `tests/fixtures/retail_scenery_idle_phase_step.tsv` and provenance

The controller-lifetime oracle executes original `146980` with explicit game
options tick rate 30 and input rate 1. It obtains clock step word `0x3D088889`.
That is an explicit source input, not a captured paused-session clock bank.
The original simulation admission/PVS scheduler, session time-scale producer
and any later action changes remain separate required state. A native caller
may supply its real simulation step and admitted updates while documenting
that scheduler boundary. It must retain controller state by actual full
object handle, reset on a new lifetime and sample the resulting authored
frame directly, without converting it through seconds.
