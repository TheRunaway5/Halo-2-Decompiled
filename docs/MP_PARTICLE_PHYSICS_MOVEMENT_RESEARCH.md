# Particle physics0 movement

The complete TU1.1 `2BF130..2BF375` body independently maps to authenticated
u5 `2C2B40..2C2D85`. Both execute unchanged for **1,080 cases**, producing the
same **138,240 complete particle bytes** and **1,200 ordered stores**. The
[native owner](../src/particle_movement.cpp) matches those outputs in normal
and GCC ASan/UBSan builds. This closes the physics0 controller, not an entire
canister effect or collision controller.

The original input and executed-instruction evidence is in
`analysis/multiplayer/integration/particle-physics-movement-20261010/measured-v2/`.
Its independent `repeated-v2/` copy agrees byte for byte. The
[oracle](../tools/oracle_particle_physics_movement_20261010.py) retains complete
main-body listings, every executed lower instruction, literal controller and
function records, complete input/output particle buffers and ordered stores.
Report SHA256 is
`623a1b74e08ba4f0800e50fa7dba07b5ebb432f4828e7ef697326ca2f05e8689`;
fixture SHA256 is
`c8eaba4f52bfbb63d37175e95ad5d91740710179e941337b86e1b15a14abc379`.

## Actual canister controller inventory

Actual Lockout `E70A0595` has these six class7 systems. The seventh,
`plasma_in_beams`, has no movement tag and requires class8 rendering. Complete
authored records, including parameter curves, are retained in the existing
[inventory](MP_PROP_EXPLOSION_PARTICLE_ADMISSION.md) and
`analysis/multiplayer/sandbox-source/live-prop/explosion-definitions.json`
(SHA256 `76b2ceb78d1f191149dd9ec4df7bd7fef42676e1149dd07e7313ee4214fedcf1`).

| Event/system | Particle | pmov | Controller types | Physics0 row VA |
|---|---|---|---|---|
|1/0|E71C05A7 flash_explosion|E71E05A9|0|80260A00|
|1/1|E71F05AA metal_bits|E37301FE|0,1|801C79F4|
|1/2|E72005AB shrapnel_sparks|E1FD0088|0,1|801893C0|
|1/3|E72205AD panel_impact|E1FD0088|0,1|801893C0|
|2/0|E72505B0 white_rolling_flame|E42202AD|0|801DE494|
|3/0|E72905B4 smoke_trail_large|E72A05B5|0|80262574|

The full 20-byte physics0 rows, in that order, are:

```text
0000000003000000140a2680000000000c000000
00000000030000001c7a1c800700000000000000
0000000003000000e89318800700000000000000
0000000003000000e89318800700000000000000
0000000003000000a8e41d800700000000000000
0000000003000000882526800700000000000000
```

Row `+0` is the controller type; `+4/+8` is its parameter block; `+C` is the
constant-parameter mask; `+10` is the required-value mask. Each parameter row
is20 bytes; its function begins at `+4`. Physics0 reads the first three rows
in order, independently of their parameter-ID words.

Three systems can use physics0 alone. Metal, sparks and panel also require
original collider1 `2BF380`; their collision step cannot be discarded.
The whole E70A effect remains unavailable until all required systems,
multi-event/location scheduling and rendering classes are supported. An
admission check must inspect the complete controller list before mutating an
effect through a supported leaf.

## Complete CPU contract

Original dispatcher `2BF0C0` receives controller in EAX, system in ECX and
particle-list head in ESI, with location, elapsed time and pmov flags on the
stack. Type0 tail-jumps `2BF130` with ECX=system and stack arguments
`(controller, full_head_datum, elapsed_float)`. Original `2C06C0` walks the
controller block in authored order. `249210` has already advanced position,
age and phase before that dispatch; this new leaf must run afterward.

The source completes required system values using `mask & 107F0`. It evaluates
constant parameter bits0/1/2 once. For each particle it loads `next` first,
indexes the actual pool by low16 datum, skips flags `&9`, and skips age greater
than one or unordered age. It does not validate the salt. For a new particle
pointer, the evaluated cache is masked with `FFFF07F0`; missing required
particle values come from `mask & F80F`. Nonconstant parameters are evaluated
through complete original `248DA0→13CB00`.

With three evaluated parameters `p0,p1,p2`, source float operations are:

```text
drag = clamp_source(p1 * dt, 0, one)
angular_drag = clamp_source(p2 * dt, 0, one)
gravity_step = (current_gravity * p0) * dt
velocity.z = velocity.z - gravity_step        // first store
factor = one - drag
velocity.x = velocity.x * factor
velocity.y = velocity.y * factor
velocity.z = velocity.z * factor             // second Z store
rotation_rate = (one - angular_drag) * rotation_rate
```

Every operation is SSE float precision and preserves this evaluation order;
the native source is compiled with contraction disabled. The COMISS clamp
branches preserve unordered values. Native malformed-memory rejection adds
explicit capacity, cycle, pointer-overflow, curve-size and missing-value
bounds. These are host safety conditions, not recovered source gates.

The [API](../include/halo2/particle_movement.hpp) takes actual retained pool
storage, current gravity, selected-image one, value owners and imported shader
function tables. Its optional observer retains original store PCs. The Cache
overload imports literal pmov rows and copies their actual forwarded curves.
Flash curves forward into two `effe` payloads; other constants forward into
other `pmov` payloads. It resolves unique positive-sized actual payloads.
Zero-sized alias entries are not metadata storage owners. No copyrighted
payload fixture or tuning constant is embedded in the production source.

TU1.1 locator/call operands are particle pool `526514`, one `465030`, physics
constants `526254`, value writer `175620` and property reader `248DA0`.
Corresponding verified u5 operands are `52D714`, `46A0D0`, `52BE68`, `176A40`
and `24FFE0`. Original `1EEF40` owns the1C-byte physics-constants record and
`1EEFE0` resets gravity; the supplied record here is their actual earlier
executed output, bits `40857AF5` for gravity. It is not a native default or a
newly executed u5 constructor.
[Owning original storage proof](MP_LIVE_BIPED_WORLD_RESEARCH.md).

## Fixture and validation boundary

The fixture cross-composes actual canister controller metadata with the two
genuine source-created local SMG particles and source system/effect owners.
Those records come from the immutable local-effect report SHA256
`e30bf2ee9f87012a8778e370196679d2ee9739bbbb3409b85e238f611656dcc5`.
No canister particle birth or continuous lifetime is inferred. Five elapsed
values, three flag controls, three age controls and two angular-rate controls
are declared CPU inputs. Existing source particle positions, velocities,
salts and other bytes remain literal. The original private caller stack is
declared zero ABI storage; unrequested value-cache words are unspecified
source temporaries. Native semantic value reads require their actual producer;
unused constant-curve arguments do not establish private-stack parity.

All original helper instructions execute; there are no substituted service
returns, contacts or allocation results. x87/MXCSR inputs are explicitly
masked; output comparisons do not establish Xbox FP sticky-status behavior.
The tests compare both complete64-byte particle records and every source store,
then exercise native malformed-memory rejection. There is no GPU, collider,
Havok, continuous-motion, new particle allocation or full-effect admission claim.

Reproduction from the repository root:

```sh
PYTHONPATH=tools .tools/python/bin/python tools/oracle_particle_physics_movement_20261010.py --output NEW_EMPTY_OUTPUT_DIRECTORY
g++ -std=c++20 -O2 -ffp-contract=off -Iinclude tests/particle_movement_tests.cpp src/particle_movement.cpp build-animation-metadata-store-20261008/libhalo2_cache.a -pthread -o particle_movement_tests
./particle_movement_tests assets/disc/maps/lockout.map analysis/multiplayer/integration/particle-physics-movement-20261010/measured-v2/fixtures.bin analysis/multiplayer/integration/particle-physics-movement-20261010/measured-v2/context.bin analysis/multiplayer/integration/particle-physics-movement-20261010/measured-v2/stores.bin
```

The independent sanitizer build uses `-O1 -g -fsanitize=address,undefined
-fno-omit-frame-pointer -ffp-contract=off` and the matching actual sanitizer
Cache archive. Assets and executables remain external. Frozen copied archives
and headers are observed dependencies; uncalled archive members are not
asserted independently rebuilt or audited by this test.

The registered production target `particle_movement_tests` and CTest
`original_particle_physics_movement` also pass in the normal build. The
production source declaration applies strict floating-point evaluation to this
source. CMake registration is root-owned; no renderer/runtime or other
particle owner is changed by this checkpoint.
