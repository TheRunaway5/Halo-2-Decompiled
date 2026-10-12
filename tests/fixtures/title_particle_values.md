# Original particle value-cache and live sprite fixtures

`title_particle_values.inputs.bin` supplies 1,024 controlled owner states to
the unchanged original `173BA0` masked writer, `248DF0` source getter, and
`4EFB0` normalization routine. No instructions are intercepted. The output
contains 17 float words and the completed mask for each case, 72 bytes each.
All 73,728 native output bytes match the original. Null owners, missing
effects, existing ready bits, random endpoints, clock values, negative
fractions, and coincident particle/camera positions are covered.

These records use XBE SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
`tools/export_title_particle_values_oracle.py` reproduces them. Input SHA-256
is `5d7eb1ab063676cbfe1c277804e494631b78b95858a32fa1c11788e91d12bcc9`;
original-output SHA-256 is
`df7181a291b2ed89cf25851effea80222f61a53b760936dca2023ba741052720`.

`title_particle_live_sprite.inputs.bin` freezes 512 read-only GDB observations
at original `224550` entry and `23ED30` submission in Xemu Vulkan. Each input
contains a particle datum, the initial cache and its actual owners, clock and
camera state, and the sprite call's seed/context/position/direction/scale/tint.
Float words are preserved as bytes, including untouched cache slots. Sequence
indices are derived from the actual live bitmap header and context pointer.
Each expected record is the completed 72-byte cache followed by the original
88-byte sprite submission. All 81,920 native output bytes match exactly.

The live records cover nine definitions: `E20A0095`, `E22C00B7`, `E23100BC`,
`E22B00B6`, `E22600B1`, `E22900B4`, `E21F00AA`, `E22100AC`, and `E21A00A5`.
Their required-value masks and live bitmap headers agree with the original
`mainmenu.map` asset definitions. `E22900B4` has both a bitmap parameter and a
numeric parameter; the numeric entry's `bitm` reference group does not make it
a texture. The native loader reads the original 16-bit parameter type.

`tools/export_title_particle_live_values.py` exports this fixture from
`analysis/frontend/title-effects-runtime/live-sprites.json`, recorded by
`capture-live-sprites.py` in that directory. Input SHA-256 is
`c327f5094d312d423be57b018f7167ab36a2899ab0d5d062d2a7d72c39df4d0a`;
original-output SHA-256 is
`297939503bf7895e12aaec001da8c400ce7c922c6e83ca5beeca31ec5a88e3f3`.
The old `live-sprites-cache-only-v1.json` has an incorrect submission pointer
order and is not this fixture's authority.

These observations span live calls, not a synchronized original/native frame.
They establish value-cache production and CPU sprite construction for supplied
owner state. They do not establish particle creation, physics, lifetime,
emission scheduling, camera history, material dispatch, transparent ordering,
additional GPU billboard styles, or complete title-screen pixels. Captured
particle state is not used to generate particles in the live frontend.
