# Original TU1.5 speech curves

Use the owned English TU1.5 executable with SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`
and the original opening map with SHA-256
`5093e2f8ef7f0df41c11c6621bac5785a36f78a1cff3b8b2bc6c14b561c02a28`.
The generated JSON and binary payloads remain ignored under fixture policy.

Build `export_opening_facial_curves`, then run it serially through
`tools/tag_match_memory_budget.py`, supplying `TU15_XBE OPENING_MAP TIMING_TSV
NEW_OUTPUT_DIRECTORY`. The timing TSV is generated as described in
`opening_cinematic_audio_tu15.md`. The exporter creates actual campaign objects,
calls the original `130980` resource owner, and reads the genuine `22A680`
pointer publication. All38 authored extra-info entries yield nonempty curves.
It does not assign a readiness flag or an Xbox sound handle.

Run `tools/oracle_unit_speech_curves_tu15.py --curves` with that directory and
`--output` pointing to a new JSON file. Use the Python environment with Unicorn
and Capstone, through the same unchanged60/64MiB,zero-swap source-test guard.
Copy the JSON to `tests/fixtures/unit_speech_curves_tu15.json` for the native
test. Original `380DD0` initializes the real normalization operand; unchanged
whole `110520` executes2,166 cases across38 actual curves. Inputs cover key
boundaries, between-key interpolation, previous signed-byte controls, blending
and head suppression. No instructions are patched and no callee is intercepted.

`unit_speech_curves_tu15_tests` compares the entire48-byte retained sampler
record, normalization and caller stack. `--no-update` is a failing control for
frozen speech weights. This test proves this sampler contract, including its
original smoothing, at-most12 retained controls and quantization.

`campaign_scripted_speech_tu15_tests` separately creates the actual Prophet
and authored graph, consumes actual dialogue PCM through the native SDL
adapter, and feeds its real consumption progress to the original speech owner.
The original unit facial phase and model builder produce different rendered
vertices; `--no-speech` leaves them frozen. A mismatched sound tag cannot retire
current speech, matching-tag completion clears it, and original controls decay.
The test declares view0,solo/unmodified,zero distance metrics and explicit unit
facial phases; it does not establish whole-cutscene timing or GPU mouth pixels.

The separate `--armorer` case uses the actual human placement, anchor and
`c01_intro_scene_02` graph, advancing173 original object ticks before consuming
his actual `c01_1040_qtm` cue (`ea2c08b7`, object-name53). It compares
source-selected rendered WORLD nodes;
expanding this human's full CPU vertex/material data exceeds the unchanged
60MiB hard guard and is not required by this bounded test. Full native graphics
still supplies that separate expansion path. This case and the Prophet case
pass; Armorer `--no-speech` is a failing control.

The final cue `c01_1050_mas` (`ea2d08b8`, delay396) belongs to Chief, object-name52.
The `--chief` test uses his actual placement, graph and clip, which retain NONE
for the facial speech channel. Actual PCM retains its sound tag but does not
create controls. Full v71 playback confirms that same source state. All20
facial-capable bindings have observed controls; the21st is Chief's original
absent channel. This remains source-state evidence, not all-character GPU proof.

`tools/generate_unit_speech_tu15.py` reproduces the compiled original bodies.
The host playback bridge uses actual unit/tag/permutation identities and native
PCM progress. It never narrows a native voice id into an Xbox sound datum.
Source facial smoothing runs on game ticks, not repeatedly on render-only
frames. Complete interpolation and every character's rendered mouth remain
separate full-playback acceptance gates.

The module includes the complete original `1D4400` animation lookup required
by the live final-Armorer model build. The original full v67 playback exposed
this missing owner; v69 completes after its addition. The focused Armorer test
also passes with the prior module and therefore is not a failing control for
that particular lookup path.
