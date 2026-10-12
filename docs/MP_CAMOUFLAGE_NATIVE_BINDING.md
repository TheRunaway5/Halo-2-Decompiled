# Shared camouflage in native gameplay

The native implementation connects the recovered signed powerup timers,
primary/secondary unit fades and weapon/damage disruption to their current
player and unit owners. Normal camouflage variants remain gated until the
visual consumers and normal match flows pass. This milestone does not establish
authored powerup pickup, intrinsic suit activation or complete match parity.

## Ownership and phase order

`MultiplayerPlayer::source_powerup_timers` owns original player+198/+19A.
Fresh allocation clears both; leave retains them; successful spawn resets both.
`Unit::source_camouflage` owns unit+2B0/+2B4/+2B8. Its cold values are zero,
and camouflage flags remain bits 3/4 of the ordinary shared unit action word.
The [original lifetime research](MP_CAMOUFLAGE_STATE_SOURCE_RESEARCH.md)
distinguishes allocation clearing from birth, which preserves poisoned timers.

The pre-world player loop refreshes camouflage before the spawn-damage
countdown and King occupancy update. It supplies the actual selected-engine
trait owner and the original normal frame's 30Hz, round-to-nearest contract.
The native ordinary unit phase fades current units, including dead units,
before weapon updates. After the world/projectile phases, the current-unit
guard admits the player timer owner. Timer expiry therefore changes the fade
direction after that frame's fade, preserving the source ordering.

`MultiplayerMatch::grant_powerup` is an explicit service input to original
1537F0. Calling it is not evidence that the authored pickup path is operational.
Index 1 validates the second timer contract and does not implement overshield
activation through a camouflage side effect.

## Authored disruption

The cache imports `weap`+27C/+280 as the accepted-shot amount and fade-speed
limit. It imports selected `jpt!`+30 as the positive-damage disruption amount,
including area-damage definitions. Neither a weapon name nor a universal
balance value chooses these inputs.

The source SMG accepted-fire activity performs disruption before its
first-person/zoom publication and before the projectile postpass. The existing
generic native projectile branch uses its declared accepted-shot boundary;
this does not establish original parity for every weapon. Positive body or
shield report requests disrupt the damaged unit before its zoom reset. A
friendly-fire rejection with no positive request does not enter that owner.

The standalone source kernel compares 6,912 unchanged whole disruption calls
across the three original executable images. Its measured branch has intrinsic
mode 0 and replication entity NONE. Native cold units use those same bounds.
Actual suit delegation and Xbox dirty-entity publication remain separate owners.

## Verification

The [frozen native verification](../analysis/multiplayer/game-variants/camouflage-native-20261011/native-gameplay-verification.json)
passes all nine focused CTests: the two original arithmetic targets, native
camouflage, King, Slayer, LAN startup, retail multiplayer, live props and
grenade delivery. Its runtime target compares two
explicit-grant native timelines against all three original image expectations,
checks accepted firing/positive damage and leave/rejoin/respawn, and compares
imported attributes to actual Lockout tag bytes. Its source expectations come
from unchanged original owners, rather than arithmetic duplicated in the test.
All 180 completed-frame comparisons across the three original expectations
pass, including the small positive binary32 residue after fade-out. All 28
imported weapon pairs and 25 selected damage attributes match their actual
Lockout tag bytes.

Fresh original executions reproduce the exact 4,968-call refresh/timer/fade,
6,912-call disruption and 546-call declared timeline fixtures. The scoped native
ASan/UBSan run instruments eight production owners and the new runtime test;
remaining dependencies use the ordinary complete build. The standalone kernel
test and both linked source owners are fully instrumented. Both sanitizer runs
pass. The manifest pins complete frozen source, original assets, actual commands,
logs and binaries, recording later live-checkout drift separately.

Reproduce with `.tools/python/bin/python tools/verify_camouflage_native.py`.
The first launch was interrupted before verification to select the repository
Python environment and four build jobs. Its partial logs are preserved; the
successful resumed run uses the unchanged frozen source and original fixtures.

## Native snapshot publication

The [snapshot verification](../analysis/multiplayer/game-variants/camouflage-native-20261011/snapshot-verification.json)
passes the same nine focused checks and a scoped ASan/UBSan run over the two
changed production owners and runtime test. Every compiled translation unit
depending on either changed header is rebuilt: 41 consumers across five
archives and the five native test targets. The immutable base, exact dependency
selection, copied headers/sources, compiler commands, rebuilt objects, archives
and binary hashes are recorded. This excludes unrelated concurrent edits.

`MultiplayerMatch::state` copies the actual unit's primary, secondary and fade
speed. Native protocol 34 carries these three binary32 values alongside the
existing action word. Clients consume authoritative snapshots and do not
simulate the two player timers. Actual UDP comparisons preserve every bit
through explicit grant, 30 fade/expiry frames, an inactive accepted shot's
negative primary, and the following clamp. The existing maximum-player
fragmented-snapshot check also passes. This is the native transport, not the
original Xbox replicated-entity/dirty protocol.

Reproduce with `.tools/python/bin/python tools/verify_camouflage_snapshots.py`.

## Render-context resolver

The [first-biped scalar kernel](../src/source_camouflage_context.cpp) now matches
all 156 original scalar results and observed primary-slot reads. It preserves
the initial checked lookup, raw parent projections, first-biped stopping and
finite clamp to [0,1]. The [frozen CMake/CTest verification](../analysis/multiplayer/game-variants/camouflage-context-native-20261011/verification.json)
freshly reproduces its original fixture and passes a fully instrumented
standalone ASan/UBSan build. The ordinary renderer now connects this kernel to
current equipped world/FP material banks before authored animation. Its
[material binding verification](MP_CAMOUFLAGE_MATERIAL_NATIVE_BINDING.md)
passes1,280 original composition comparisons,12 focused CTests, a separately
frozen corrected GPU isolation check and scoped ASan/UBSan. It does not implement
the separate biped camouflage clone or prove a complete positive camouflage match.
See [render source research](MP_CAMOUFLAGE_RENDER_SOURCE_RESEARCH.md).

The [earlier kernel-only verification](../analysis/multiplayer/game-variants/camouflage-native-20261011/kernel-verification.json)
passed the 4,968 corrected refresh/timer/fade comparisons before native binding.
The [source kernel details](MP_CAMOUFLAGE_KERNELS.md) preserve that proof boundary.

## Remaining completion requirements

Normal King and Phantom variants remain gated. Live biped/shadow
rendering, actual replicated entities, intrinsic suit control, authored
pickups and full ordinary match flows remain required by the
[all 39 variant goal](GAME_MODE_PROGRESS.md). Bounded native arithmetic and
explicit grants do not satisfy those remaining requirements.
