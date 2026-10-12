# Original particle rendering-input fixtures

`title_particle_render_inputs.inputs.bin` freezes 48 read-only Xemu Vulkan
observations of original `175610`, with actual cache owners, completed system,
location, emitter and particle records, clock and camera. The authored event,
system, emitter property headers, function curves, flags and required-value
masks agree with `mainmenu.map`. Native `for_each_particle_render_input` and
sprite construction reproduce all 312 observed calls in linked-chain order,
including tint, alpha, scale, cache invalidation and final owner keys.
All 69,120 expected bytes match, with no tolerance.

`tools/export_title_particle_render_inputs.py` exports these records from
`analysis/frontend/title-effects-runtime/live-render-inputs.json`.
Input SHA-256:
`5c2d8676fb202000e144a61f3b20087615ed718cf5d634a6e9c41c9a85af17a1`.
Original-output SHA-256:
`bc4b83b40597edaade802c01a759e8f033010e9d2f7fb12eedcc22e2f61ff744`.
The report is `analysis/frontend/title-particle-render-inputs.json`.

All those live systems use world coordinates. The separate
`title_particle_render_coordinates.inputs.bin` exercises unchanged original
`175610`, `1751D0`, `173BA0`/`248DF0`, `246CD0`/`246D80`, `248450`/`142EB0`
and `224550` in 96 controlled cases. Only final `23ED30` GPU submission is
intercepted. The coordinate enum is an explicit controlled definition input;
the cases cover world/local/parent coordinates, both first-person flag values,
warm and changed cache owners, and empty particle lists. All 264 handoffs and
submissions, plus final caches and owner keys, match 63,360 original bytes.

`tools/export_title_particle_render_coordinates.py` reproduces that fixture.
Input SHA-256:
`818c572ddea0136618b9a75a4af22e2f5193131a8c8bd668f79fcf3e1fd2f10a`.
Original-output SHA-256:
`26653679c6296f9fe6b7bcc1eb02dd20dd9395e291bf29ef78d1d621fef5891c`.
The report is `analysis/frontend/title-particle-render-coordinates.json`.

Both use XBE SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`
and map SHA-256
`1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b`.
Expected records contain a call count, each identity/cache/position/direction/
scale/RGBA/sprite record (208 bytes), and the final 84-byte cache with owner
keys. Raw float words are preserved.

The executable's matrix addition order differs from the recovered C:
`142EB0` sums the left/up columns' Y/Z components as `(up + left) + forward`.
The native implementation retains that order, and the coordinate comparison
is byte-exact. Tint unpacking uses the original reciprocal multiplication.

These fixtures supply completed simulation state. They do not prove live
creation, emission, lifetime, physics, camera-scale production, material
dispatch, global transparent ordering, or synchronized frontend pixels.
The camera-scale branch requires an explicit original caller result; unknown
inputs are rejected. Captured particle states are never used to populate the
live frontend. Every menu layout and navigation route remains a separate
required completion gate.
