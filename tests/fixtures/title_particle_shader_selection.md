# Original particle shader selection fixtures

The three input records specify original smoke `E21A00A5`, discharge
`E21F00AA`, and sparks `E22100AC`, original transparent layer 15 and pass 0
within that layer. The expected records contain the original selected template,
pass, vertex datum, classification, and complete program bytes.

`tools/export_title_particle_shader_selection_oracle.py` executes unchanged
`2243C0` and `23EA80`, including original `137BD0` and the particle object's
original virtual getter/initializer. It stops at original vertex binding
`1C590`. Six side-effecting render-setup calls are intercepted and listed in
the report. Shader dependency lookup and classification selection execute
unchanged; none of those results is substituted.

XBE SHA-256:
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
Input SHA-256:
`fd30373abf34b36b7816d0d253dd8b8b8abba7b0cdeea47a299f55efb24af467`.
All three definitions supply authored style 0 and select classification 7 of
`E20E0099`; its program SHA-256 is
`3cc3d35a41c96091239df1bd7a4f3f7d51cc674eb7d6c9ca899b29ac6e1458ba`.
Report: `analysis/frontend/title-particle-shader-selection.json`.

This establishes the original selection path for these supplied definitions
and layer/pass inputs. It does not identify the captured live emitter, execute
material side effects, establish pixel implementation choice, or render native
particles.
