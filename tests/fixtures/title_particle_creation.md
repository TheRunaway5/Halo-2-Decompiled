# Original successful particle-creation prefix

`title_particle_creation.inputs.bin` contains 256 read-only successful original
`247950` calls in Xemu Vulkan, ending at `247E2A` before child/effect dispatch.
Each input includes the actual cleared allocated slot/datum, emitter, system,
cache, optional location/effect record, clock/camera, request and RNG seed.
Expected output is the 64-byte particle, 84-byte cache, 76-byte emitter and
4-byte seed: all 58,368 native bytes match without tolerance.

Eight authored system/emitter references were observed. Definitions and their
curves match disc assets. Allocated records have a salt plus 62 cleared bytes.
The creation prefix covers slot linking, random ushort fields, complete value
cache, bounded shape initialization, local/world placement, inherited velocity,
color and count. No original instructions were intercepted in this capture.

Input SHA-256: `d8d968bbea48e513786df826a42431e07cb28bef72608157f4e17acaa370895e`.
Expected SHA-256: `58c089093d44dd7170f6c0ccc6d39e71bd83b380688738a8f1e43931ccdaddc5`.
Capture SHA-256: `5fafa6bdb1e0e83c8645e510008e539300429f06230ccd100606f1e08cddd956`.
Exporter: `tools/export_title_particle_creation.py`.
Report: `analysis/frontend/title-particle-creation.json`.
XBE SHA-256: `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
Map SHA-256: `1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b`.

The allocated slot is supplied; native pool allocation is not validated.
Subsequent child-system/birth-effect dispatch, simulation lifetime/physics,
live frontend particles and complete scene/menu pixels remain unfinished.
