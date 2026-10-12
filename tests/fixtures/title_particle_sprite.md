# Original title particle sprite fixtures

`title_particle_sprite.inputs.bin` contains 64 controlled cases for each of
the three original title particle tags: smoke `E21A00A5`, discharge
`E21F00AA`, and sparks `E22100AC`. Each case supplies the completed 17-value
cache, seed fields, position/direction, emitter scale/color, and caller-selected
bitmap context to the original and native sprite calculation.

`title_particle_sprite.retail.bin` contains only the corresponding original
XBE's `224550` output intercepted at `23ED30`. It was generated with
`tools/export_title_particle_sprite_oracle.py --cases 64`, using original
executable SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The intercept observes parameters without replacing property evaluation.

Retail `23ED30` receives the geometry pointer in EBX and eight stack
arguments, and cleans those arguments up on return. Retail `color4f` is
alpha/R/G/B; the native input convention is R/G/B/alpha. The exporter converts
this input layout explicitly. Neither convention was inferred from pixels.

CTest compares every output byte with these independent original records.
These fixtures establish sprite calculation for supplied values, not
emission, simulation, particle shader or framebuffer parity.
