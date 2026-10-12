# Live TU1.5 physics integration, 2026-10-12

English TU1.5 is the sole executable acceptance target, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Full player, vehicle and object parity remains incomplete. Canonical IDs in
paired modules select TU1.5 implementations; no 1.1 execution is used here.

`PhysicsLiveWorldTu15` owns one fresh source bank, native process/arena backing,
actual constructors, source world, header/body/component and player pools,
cache resources, BSP and command delivery. Actual B9B70 biped birth,14EBE0
player construction and14F620 binding now return over that same bank.

The initial 120 neutral ticks and60 forward ticks returned without movement.
That check is RED. It revealed a missing source player-cluster producer:
1500C0..1501A8 now runs before13B6F0 PVS. Real PVS activation subsequently
reaches additional body/world registration, controller, contact and animation
owners. Missing owners are recovered from their original bytes and retained
ABI, never replaced with successful callback returns. The active-body movement
check has not yet passed.

Authored scene birth now uses source C1120/D75A0, the original scenario type
lookup, raw placement and palette. Source BA3E0/BA490 own deactivation and
retirement; scene lifetime observation occurs afterward. Scene rendering reads
the completed source root, including parent WORLD nodes and handedness.
Current scene root/salt/read-only/interpolation checks pass. The independent
TU1.5 BFC70 comparison passes256/256 exact13-word outputs under Unicorn.
These explicit presentation inputs are not solved-physics evidence.

Complete11EF70 now prepares every resident collision, biped, creature,
physical-model and vehicle definition before birth. Its original11F02C call
passes zero to1F2C70, resolving serialized shape references. Actual startup
row3/148D40 now constructs both random streams, including original CRT
time/rand conversion over explicit native UTC/current-thread inputs. The
literal13A8F2..13A8FB options-seed delivery runs before scene construction.
With these producers, the complete source factory now constructs19 Lockout
scene bodies in one fresh world. This establishes births, not lift forces.

The real151DA0 post-birth sequence B1A70/B1C30/1613B0 now runs after player
binding. Complete1613B0/1A8B50 creates and carries an actual source SMG;
the fresh-world diagnostic reaches this successful return with one weapon.
Native inventory observes its full source identity, selectors and sequences.
Admitted world-weapon births now select complete15D4F0 over the same bank,
instead of reserving a bodyless header. The general vehicle factory is exposed
through that same original owner; vehicle birth/driving remains unverified.

The TU1.5 observer callbacks now select actual11E600 location,3F4F0 visibility,
1746BE..174749 fog classification and16C620 complete segment collision over
the same bank. Native death/kill-volume consumers now follow TU1.5 physics
pose readback. Observer live execution is still pending successful world startup.

PCMPGTW and PADDW lowering passed400,392 independent native x86 instruction
comparisons in
`analysis/multiplayer/sandbox-source/physics-live-world-tu15-20261012/mmx-word-hardware`.
The original CRT32CC0C also required real FISTP64/FILD64 lowering. This does
not settle the separately retained whole-world x87/MXCSR flag difference or
Xbox reciprocal-square-root estimate-bit equivalence.

Visible attempts are retained under
`analysis/multiplayer/physics-live-tu15-20261012/live-integration-v2` through
`live-integration-v10`. All use the real multiplayer acceptance executable and
visible desktop. They currently fail before creating the SDL window; none is a
new visible gameplay pass. V6 reached a missing object-looping-sound pool in
18BD80->16E6A0. Source18B5C0 construction and the exact18B660..18B71C storage
prefix have since been wired. The separate2279E0 audio-volume reset and18BB50
profile consumers are not claimed as completed audio composition.
V10 gets through scene construction and stops at the then-unavailable CF4D0
equipment owner. The fresh diagnostic has since completed that equipment path
and now reaches active PVS/physical-world registration. No new visible
gameplay frame was produced by these first ten attempts.

V11 now completes preparation and renders one actual two-player split-screen
frame. Its unaltered window recording retains30 frames over1.1 seconds at
2560x1378; no simulation tick completes in that build. The retained runtime
stops at the then-unavailable320260 world-registration callback. Recovering
that path subsequently exposes serialized instanced-BSP collision vtables on
the first1CC8D0 tick. Complete171120 is now called with the real BSP datum
at original11EA20's boundary, after1255C0 and before BSP binding. The fresh
diagnostic now reaches the real2E9640 collision-query callback through its
resolved vtable. Player motion, lift force and whole-frame acceptance remain RED.

The fresh Lockout diagnostic now completes its first full command/object/world
physics tick with19 original scene bodies, a source-bound player and its actual
SMG. The second tick reaches original resource/texture preparation. Actual
44B308/44B310 now construct/reset the scenario-instance bitmap used by the
controller's instance update. Native FUCOMIP lowering retains extended
operands and unordered comparisons. Texture storage12EBE0 and activation
12ECF0 now run at their original startup/resource boundaries. Their literal
0x404 protection request maps to real host read/write protection under an
explicit host cache policy; Xbox GPU cache coherency is not established.

Visible v12 runs on the real desktop at960x540 retain11 movement-window
frames and18 lift-window frames. Neither completes a simulation tick in
that sealed build. Movement reaches the character-contact owner1FA510;
the lift entrance reaches an additional body-registration callback. Both
paths are now included alongside Coagulation in fresh same-bank recovery.
The development harness's cache archive and headers are sealed together;
a stale layout mismatch exposed by loading the lift metadata was corrected.
Coagulation's real15D4F0 factory reaches its third authored vehicle row,
not yet a completed six-vehicle birth or driving pass.

Required remaining acceptance includes actual moving players, jumping/crouching,
lift membership/force and upward travel, dynamic object/projectile impulses,
source-created/driven vehicles, source retirement/death, and visible multiplayer
plus real UDP authority snapshots. Vehicle inventory alone is insufficient.
