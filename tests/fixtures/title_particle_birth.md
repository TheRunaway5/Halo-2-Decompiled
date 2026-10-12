# Original particle birth-initialization fixtures

`title_particle_birth.inputs.bin` contains 128 read-only observations at
original `249170` entry in Xemu Vulkan. Each input preserves effect/event/
system/emitter identity, the actual 17 values, RNG seed and initial 64-byte
particle record. Each expected record preserves the 64-byte particle at
`249A71` return plus the final RNG seed. All 8,704 native bytes match with no
tolerance. Six observed emitter definitions have 126 sphere (shape 2) and two
ring (shape 4) calls. Their five property headers and curves agree with the
disc map. Hardware breakpoints and memory reads leave instructions unchanged.

`tools/export_title_particle_births.py` exports the fixture from
`analysis/frontend/title-effects-runtime/live-particle-initialize.json`.
Input SHA-256:
`fb0ac42345760ca3394deb575a95eddf02f60ccb3deefe6dc5589c37b92a3b63`.
Original-output SHA-256:
`ff62f8df3fa93ec2ca2dc568f0b020277df96474b05d85ad1f9c523bcdaaef32`.
Report: `analysis/frontend/title-particle-birth-initialization.json`.

`title_particle_birth_shapes.inputs.bin` executes unchanged original `249170`
and its property/RNG routines in 3,078 controlled cases. Only the shape enum
is an explicit controlled definition input, an extra word after each identity
header. Sphere/inward-sphere (2/3) each exercise all 1,026 original direction
indices. Ring (4) spreads azimuth across the same RNG upper-word range.
Authored emitter properties remain unchanged, with explicit values, initial
records and seeds. All 209,304 output bytes match. No instructions or service
calls are intercepted. Separately, original instructions reproduce all 128
live Xemu particle/RNG records with no differences.

`tools/export_title_particle_birth_shapes.py` reproduces the controlled fixture.
Input SHA-256:
`d6da312901009ff5deab904140d7675aca2b52fef5e5d2045395d0636c52d50c`.
Original-output SHA-256:
`93bea828dcc349b240093446f94d97b642c8c1c8fc4773cc2823c3cf8bfec0e9`.
Report: `analysis/frontend/title-particle-birth-shapes.json`.

Both use XBE SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`
and map SHA-256
`1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b`.
Live direction bytes match the XBE table loaded by `OriginalData`, SHA-256
`9581ebb544c806b2c88cdc2a094df6a0ac07978c9bb07dcb9f68fe2962fd2feb`.

The x87 azimuth fraction remains in double precision through multiplication
by 2*pi before its float store. Ring radial sine remains in double precision
through component products. Observed Xemu `RSQRTSS` matches a binary32 square
root then binary32 reciprocal; a direct double reciprocal square root differs
in 28 of 128 live cases. Physical Xbox approximate reciprocal-square-root
behavior remains unverified; these claims apply to the user's Xemu reference.

The initializer admits shapes 2, 3 and 4, rejecting others explicitly. It
initializes position, velocity, inverse lifetime and rotation rate. Record
allocation, random ushort initialization, value-cache completion, particle/
child-system linking, emitter/world transforms, lifetime/motion updates,
collision, emission scheduling and GPU dispatch remain separate requirements.
The live frontend still does not emit particles. These fixtures do not prove
complete title pixels or any menu layout/flow.
