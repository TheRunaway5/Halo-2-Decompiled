# Opening cinematic audio CPU references

The local generated files `opening_dialogue_timing_tu15.tsv` and
`opening_audio_gains_tu15.tsv` contain original map/executable-derived inputs.
They remain ignored by Git, following the repository's fixture policy. The
generators and native comparisons are checked in.

Use the owned English TU1.5 executable with SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`
and the original `00a_introduction.map` with SHA-256
`5093e2f8ef7f0df41c11c6621bac5785a36f78a1cff3b8b2bc6c14b561c02a28`.
The Python interpreter needs Unicorn and Capstone. Run each generator serially
through `tools/tag_match_memory_budget.py`; retain its unchanged 60MiB RAM,
independently measured64MiB ceiling and zero-swap controls.

Generate timing first with `tools/oracle_opening_dialogue_timing_tu15.py --output`
pointing to a new directory. Its optional `--xbe` selects the owned TU1.5 file.
Copy its `fixtures.tsv` to `tests/fixtures/opening_dialogue_timing_tu15.tsv`.
Generate gains with `tools/oracle_opening_audio_gains_tu15.py --output` pointing
to another new directory, then copy its `fixtures.tsv` to
`tests/fixtures/opening_audio_gains_tu15.tsv`. The gain oracle also requires the
retained original sound-platform paired instruction report used by
`tools/oracle_sound_packet_gain.py`; it verifies that report's SHA-256.

The timing oracle executes unchanged TU1.5 wake, due-runner, frame-evaluator,
sleep and result-return instructions against all49 actual dormant cue threads.
Its active30Hz clock and dormant thread banks are declared inputs. Non-timing
void evaluators have literal-only arguments; they are observed and return void
through the original result routine. No voice, object-admission or rendering
result is supplied. The result is wake-relative request timing: Pillar100,
Council279 and Truth490; the Prophet request gap is211 ticks.

The gain oracle executes unchanged control and packet/driver instructions with
neutral class-fade inputs and caller control1. It retains each cue's exact20-byte
sound,56-byte playback,92-byte class and20-byte scale bank. All49 authored cues
have zero gain variance and a single permutation. The native test compares
those bytes, exact caller-gain bits and driver-gain outputs without decoding
large PCM banks.47 cues exceed the admission floor;2 are authored silent.

These tests do not prove complete scene wake ordering, source voice admission,
moving listener/emitter state, Xbox DSP or final mixing, PCM equality, or sample
delivery/physical audibility. Native frame-end draining is a separate measured
timing defect; do not change authored sleeps to compensate for it.

`sdl_sound_timeline_tests` additionally checks the native mixer's explicit
sample-deadline contract against all49 original request delays, across three
render partitions. It observes actual first PCM consumption, including a
zero-valued first sample, and covers the real SDL dummy callback, global/per-voice
pause, cancellation and fractional source progress. It also runs without the
original fixture to exercise the generic native timeline service.
`campaign_scripted_audio_timeline_tests` uses the actual decoded Council/Truth
PCM through `MultiplayerAudio`. Explicitly scheduled consumption preserves the
337600-sample presentation gap, with a separate4800-sample pause. The immediate
start path fails this same check. All source comparisons retain their existing
memory limits. These are native platform/adapter checks: the full cinematic
request producer is not yet wired to supply those future deadlines, and its
live playback still fails timing parity. No authored delay has been changed.
