# Original particle emitter-update fixtures

`title_particle_emitters.inputs.bin` contains 256 read-only Xemu observations
at original `247470`, with 472 ordered `247950` creation requests. The original
emitter, owner cache, optional frame, timing and authored definition are input.
Observed creation callback head/count writes are supplied explicitly. Native
request words, pre-callback emitter states and final emitter match all 63,904
expected bytes. No original instructions were intercepted in the live capture.

Input SHA-256: `6184da7693530c5a83170af0b5527f0e4693ab79dce8bd33292950a3a43e0b88`.
Expected SHA-256: `d56b7d85a38230680ce305d98f56956d483c1577f351b34dc985a553c41fd407`.
Exporter: `tools/export_title_particle_emitters.py`.
Report: `analysis/frontend/title-particle-emitter-updates.json`.

`title_particle_emitter_oracle.inputs.bin` contains 256 controlled original
executable runs and 1,109 requests, matching all 122,508 output bytes. Only
`247950` is intercepted to supply explicit particle head/count writes. Burst,
spread and camera-scale flags, frames/null frames, offsets, orientation,
remainders and timing thresholds are controlled inputs. The original matrix,
property, accumulation and emission code runs unchanged.

Input SHA-256: `a69f0350a8b55168b7206f5ec631ec7fd979d9fd095fbeae0093cdc341348f4c`.
Expected SHA-256: `454a8610d55da27d0b4fb0e1fea954128d5956f0acd0bfd78b037b5fe9124377`.
Exporter: `tools/export_title_particle_emitter_oracle.py`.
Report: `analysis/frontend/title-particle-emitter-oracle.json`.

Both use original XBE SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`
and map SHA-256
`1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b`.
These prove isolated emitter placement/rate/request boundaries with explicit
callback outputs. Native pool allocation, complete child/effect creation,
simulation history, live frontend integration and complete menu parity remain
unverified. The frontend still supplies no live particle draws.
