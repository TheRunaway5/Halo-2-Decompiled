# Superseding particle-pool label correction

The prose pool list in the frozen
[canister owner note](MP_CANISTER_PARTICLE_OWNER_RESEARCH.md), line38, reverses
the emitter and particle-location names. Preserve that frozen note and its raw
evidence. The correct original ownership is:

| Original global | Literal allocated pool name | Capacity | Record bytes |
| --- | --- | --- | --- |
| `526518` | `particle_emitter` |256 |76 (`4C`) |
| `52651C` | `particle_location` |256 |52 (`34`) |

Unchanged original `175860` loads emitter name pointer460FE4, sets EDX=4C at
1758FB and capacity100 at175900, calls16D070, then publishes526518 at17591B.
It loads location name pointer460FF8, sets EDX=34 at175934 and capacity100 at
175939, calls16D070, then publishes52651C at175948.
[Complete original constructor](../analysis/multiplayer/integration/canister-vertex-cache-20261010/final-measured/instructions/00175860.asm).

Both the old canister report and the independently repeated vertex-cache
report retain these literal names and correct raw header strides. The error is
confined to the old prose label order. Existing native particle inputs use
location52/emitter76, consistent with the
[SMG owner proof](MP_SMG_PARTICLE_OWNER_RESEARCH.md). No pool, runtime or code is
changed. The correction packet retains literal headers, names, old/new report
pins and constructor listing pin.
[Correction packet](../analysis/multiplayer/integration/particle-pool-label-correction-20261010/report.json).
