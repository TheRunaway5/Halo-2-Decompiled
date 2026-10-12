# Original weather plate update

`source_weather_plate_simulation.hpp/.cpp` implements the complete original
`37260` routine over an explicitly owned 936-byte plate. It does not publish
resources, select a camera, advance a clock, choose random-stream history, or
submit Vulkan draws.

The original ordinary caller resolves the palette's `weat` datum through the
tag table at `36E10..36E1E`. `36E5A..36E5C` supplies that owning root, elapsed
time, and the plate to `37260`. Consequently the helper's 172-byte weather
input is the literal owning root, not a separately invented wind structure.
The consumed root fields are `+30/+34/+38`, `+90/+94/+98/+9C`, and `+A8`.
The source elapsed `509418`, intensity `4670FC`, auxiliary stream, and literal
244-byte `485600` view bank remain caller inputs. The view bank contains the
signed view slot, position, forward vector, angle, inverse transform at `+8C`,
and world transform at `+C0`. No input producer is inferred from a picture.

The routine writes only the selected view's `B8`-stride state and the original
shared plate fields. It records intensity and camera position, computes camera
movement along the forward vector in the original SSE order, optionally wraps
the depth phase through `CVTTSS2SI`, and repairs authored layer depths when
flag bit 1 is present. Invalid conversion has the original `80000000` sentinel;
there is no undefined C++ float-to-integer conversion.

Each of the three layers rebuilds its world point from the supplied source
transform. The update consumes auxiliary random draws in z/x/y order, stores
each intermediate float, and clamps the random walk through the original
ordered/unordered comparisons. It adds the owning root's wind/bend terms,
applies the authored response divisor, clips drift to the stored initial
length, and updates per-view offsets using the original inverse-transform
arithmetic. The x87 norm adds `(z*z+x*x)+y*y`; its float store precedes the SSE
reciprocal. Nine auxiliary draws occur per complete update, and the primary
stream is untouched. Mutable drift is shared between views, as in the source.

The original `FPTAN` result is multiplied before the float store used by the
optional projected size. Nonfinite camera angles and arguments outside the
instruction's argument range are explicitly rejected before native mutation.
The oracle retains their complete original executions as six separately
labeled source-only controls. It does not imitate exceptional x87 stack
behavior with a portable tangent or silently claim those inputs are supported.
Incomplete owned banks and signed view slots outside 0..3 also reject before
any output or random-stream mutation.

`tools/differential_source_weather_plate_simulation.py` executes unchanged
`37260..37A52`, with no replacement instruction, arithmetic, random service,
or callee. The mandatory native fixture compares 1,810 admitted cases and
615,400 words exactly: complete plate/view/weather banks, return AL, and the
auxiliary word. Its 1,816 original executions comprise authored Lockout and
Warlock plates initialized through unchanged `36F50`, all four view slots,
1,536 independent finite controls, floating classification/cancellation
controls, and two explicitly supplied 32-step sequences carrying the source
outputs into the next inputs. These supplied sequences are arithmetic tests,
not a reconstructed original gameplay timeline.

The frozen corpus is `tests/fixtures/retail_source_weather_plate_simulation.tsv`;
the report is `analysis/multiplayer-shading/weather-plate-simulation-original.json`.
The disassembly is `analysis/multiplayer-shading/weather-plate-37260-disassembly.txt`.
The native test requires the entire corpus and includes eleven pre-mutation
ownership/angle guards. Normal and fully instrumented sanitizer runs consume
the same original outputs. The helper and its mandatory original corpus are registered in CMake.
They add no live weather activation; resource publication, visibility, actual view/clock/RNG
chronology, and the weather GPU pipeline remain separate producers.
