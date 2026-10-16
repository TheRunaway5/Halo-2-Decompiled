# Original TU1.5 unit emotional-state controls

Generate a fresh capture with `tools/oracle_unit_cinematic_emotion_tu15.py`
using `--xbe` for the owned English TU1.5 executable and a new `--output`.
The image SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

The 192 cases execute unchanged whole HS wrapper `2AD950` (registered opcode
`0DC`) and whole `111080`/`1110D0`. Only parsed argument delivery `212010` and
HS result publication `211DA0` are declared boundaries. No instructions are
patched. Each case compares the complete declared 0x600-byte unit body and
unchanged 0x24-byte clock. Duration conversion is compared at the actual
`111080` call boundary. Inputs include all 13 emotional names, unknown/None
names, signed-short durations, 15/30/60 Hz, invalid facial references, None
objects, negative/out-of-range weights and nonfinite weights.

Body, header, stack, clock and facial-reference storage are explicit inputs;
this fixture does not establish original object birth or rendered pixels.

The separate `campaign_unit_cinematic_runtime_tu15_tests` creates the actual
authored prophets and anchor, starts their scene05 tracks, exercises original
graph resets and advances 30 real source phases. It restores the same authority
checkpoint and compares emotional versus neutral poses from the actual model
builder, including deferred render owner `BF640`. The builder's view0, solo,
unmodified view and zero distance metrics are explicit test caller inputs.
This caught both the missing registered facial normalization initializer
`380DF0` and a missing weighted animation composition owner `290710`.

The test proves a changed rendered WORLD pose and checkpoint repeatability.
The campaign renderer still needs to consume that deferred pose. Facial GPU
pixels, all emotional states, full opening playback and converted MCC data
remain unaccepted.
