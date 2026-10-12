# Canister effect owner and retained stops

The actual Lockout canister death path now reaches its authored explosion
`E70A0595` with source-created effect storage, source-owned player visibility
and the original breakable-mask storage/reset. It retains effect `E6650000`
and location `E6650000`, then advances from event0 to event1. Its next stop is
**the first required decal part**, before any particle birth. This is an owning
boundary, not a completed visible explosion or native full-effect admission.

The [new oracle](../tools/oracle_canister_particle_owner_20261010.py) runs four
independent TU1.1 cases. Each starts from the unchanged original authored
`power_module` factory, correlating its complete state, ABI and call/store
prefix to the immutable owner report. The measured and independently repeated
reports are under
`analysis/multiplayer/integration/canister-particle-owner-20261010/`.

| Case | Genuine additional roots | Exact retained result |
|---|---|---|
|`empty`|Effect constructor/reset|Effect allocated, original visibility rejects and retires it; later damage aftermath stops at `185D01`, reading absent `4F4850` backing.|
|`player`|Original join/spawn, early cluster writer, PVS update, effect constructor/reset|Visibility accepts; effect/location remain occupied. Damage aftermath still stops at `185D01`.|
|`player-mask`|Same player/PVS plus original mask constructor/reset|All seven bounded `D73E0` calls return. First `1788B0` enters event1, then stops at `180B12`, reading absent decal context `4F1F1C+380C`.|
|`decal-prefix`|Same storage preparation plus complete decal constructor entry|`17EC90` creates the real decal pool/context, then stops before `VirtualAlloc(0,12,101000,4)` returns. The stopped frame is never resumed.|

These cases retain 39 bounded original roots: 35 returns and four exact stops,
80 observed effect calls and 21,203 non-stack stores. Each case exports 49
instruction files, including complete selected bodies, all executed bytes and
store sites. Full effect/particle pool headers, bitmaps and allocated backing
are captured before/after each root; occupied records retain complete salts
and bytes. Mask and decal context backing, PVS, world/player records, incoming
registers, stack words, defined returns and pending calls are retained.
No GPU or native runtime change is part of this checkpoint.

## Source visibility and scheduling

Original `1775D0` tail-calls `175860`, creating the effect/location and
particle-system/particle/location/emitter pools. Original `177690` resets them.
The effect pool is256×400 bytes, locations512×60, systems128×84,
particles1024×64, particle locations256×76 and emitters256×52. These sizes are
read from their actual resulting headers; no pool pointer or datum is assigned.
[Constructor and reset instructions](../analysis/multiplayer/integration/canister-particle-owner-20261010/final-measured/player-mask/instructions/001775d0.asm).

`D7CD0` produces the actual canister damage-owner record. An explicitly requested
`D7E00` death and successive unchanged `D73E0` roots enter the explosion request
through `1785C9→177A10`. This is a diagnostic death request, not a source bullet
hit or a natural full frame. The request has flags4, actual effect `E70A0595`
and parentNONE. Original `179B90` allocates the effect; `17A030` allocates its
location. [Executed instructions](../analysis/multiplayer/integration/canister-particle-owner-20261010/final-measured/player-mask/instructions/executed.asm).

`17B1A0` reads the actual location cluster6. Effect tag flags `1C` select
`11D650`, which tests the source PVS at `Game+1138`. With the source-empty
player pool the mask is zero and `11D650` returns0. `17B1A0` sets runtime
visibility bit3 and `179080` retires the effect before event processing.
[Visibility owner](../analysis/multiplayer/integration/canister-particle-owner-20261010/final-measured/empty/instructions/0017b1a0.asm),
[PVS predicate](../analysis/multiplayer/integration/canister-particle-owner-20261010/final-measured/player-mask/instructions/0011d650.asm).

The existing immutable player helper executes original join/spawn,
`14EAE0..14ED0C` and `13A380`; the resulting PVS is `FFDF0000` followed by60
zero bytes. The same original `11D650` then returns1. This is a nonlocal player
fixture: no local user, camera, view/controller binding or all-active mask is
assigned. [Owning player/PVS evidence](MP_PLAYER_VISIBILITY_RESEARCH.md).

Original `17AD20` processes event0, whose authored part/system counts are zero.
After the mask owner closes the damage aftermath, the first `1788B0` reads the
retained clock's actual elapsed word and calls `17AD20` again. Source
`17AC00/179D10` advance to event1; `17BDF0` enters its first authored decal
`E70B0596`. `180980` stops at `180B12` because `4F1F1C` isNULL. The frame is
not resumed; no sound, light, bloc, collider or particle result replaces it.
[Scheduler](../analysis/multiplayer/integration/canister-particle-owner-20261010/final-measured/player-mask/instructions/0017ad20.asm),
[part owner](../analysis/multiplayer/integration/canister-particle-owner-20261010/final-measured/player-mask/instructions/0017bdf0.asm).

## Actual missing storage owners

Callback table `447428`, row34, contains `1858E0` initialization and `185960`
map reset. The original constructor requests4204 bytes from the real game
arena, installs `4F4850` and resets its static handle bank. The reset writes
4201 bytes ofFF. The actual complete allocation, stores, remaining three
backing bytes and checksum/cursor effects are recorded. This independently
agrees with the existing [round-reset storage proof](MP_ROUND_RESET_STORAGE_RESEARCH.md);
no assumed intact-breakable bitset is assigned here.
[Constructor](../analysis/multiplayer/integration/canister-particle-owner-20261010/final-measured/player-mask/instructions/001858e0.asm),
[reset](../analysis/multiplayer/integration/canister-particle-owner-20261010/final-measured/player-mask/instructions/00185960.asm).

The separate `decal-prefix` case executes original callback37 `17EC90`. Its
real allocator creates a1024×64-byte decal pool at `82EF974C`, then allocates
3C2C bytes and installs decal context `4F1F1C=82F09818`. It calls vertex-cache
initializer `43880`, which reaches `33785C` with the literal request
`(address0,size12,allocation101000,protection4)`, caller return `43894`.
Execution stops before the platform wrapper returns; no raw allocation,
NTSTATUS, success, cache pointer or device result is supplied.
[Decal constructor](../analysis/multiplayer/integration/canister-particle-owner-20261010/final-measured/decal-prefix/instructions/0017ec90.asm),
[vertex-cache initializer](../analysis/multiplayer/integration/canister-particle-owner-20261010/final-measured/decal-prefix/instructions/00043880.asm).

The real next constructor dependency is that platform allocation protocol and
its ensuing original vertex-cache construction/reset. The current partial
decal constructor must not be treated as ready storage or reused to continue
the separately stopped event1 frame. Previous round-reset probes constructed a
separate cache API object under a declared binding boundary; that does not
close this whole constructor. It is not necessary to change physics0 math to
pass this gate.

## Boundaries and reproduction

The recorded complete compatibility context, raw heap/game-arena backing,
actual selected physics postprocess and resource delivery, default variant
packet and30Hz clock inputs remain those of the immutable authored factory
and player helper. The compatibility generator has exited; this is recorded
context replay, not a live Xbox thread. Added roots are a selected bounded
composition after factory correlation, not the complete68-callback title
startup order. Each added root uses **zero supplied service returns**.
No position, contact, PVS, effect/event/phase identity, tick, allocated handle or
readiness field is assigned by the extension.

Run from the repository root with a new output path:

```sh
PYTHONPATH=tools .tools/python/bin/python tools/oracle_canister_particle_owner_20261010.py --output NEW_EMPTY/report.json
PYTHONPATH=tools .tools/python/bin/python tools/oracle_canister_particle_owner_20261010.py --with-player --output NEW_PLAYER/report.json
PYTHONPATH=tools .tools/python/bin/python tools/oracle_canister_particle_owner_20261010.py --with-player --with-mask-owner --output NEW_MASK/report.json
PYTHONPATH=tools .tools/python/bin/python tools/oracle_canister_particle_owner_20261010.py --with-player --with-mask-owner --with-decal-owner --output NEW_DECAL/report.json
```

Reports and every instruction file must compare byte for byte to a fresh
independent run. Earlier exploratory reports/tool versions are retained
separately. Original executables/cache and recorded context remain external;
no payload fixture is embedded in production.

The independently frozen [physics0 leaf](MP_PARTICLE_PHYSICS_MOVEMENT_RESEARCH.md)
still passes its paired1.1/u5 original/native full-buffer/store comparisons.
That proof uses genuine local SMG particle records plus actual canister pmov
metadata; it does not establish canister birth. Whole E70A still requires its
complete decal/light/sound/bloc parts, multi-event births/location scheduling,
collider1 on metal/sparks/panel and the class8 plasma system. None may be
silently discarded. This checkpoint provides no whole-effect admission,
visible explosion, rendering, continuous lifetime or collider parity claim.
