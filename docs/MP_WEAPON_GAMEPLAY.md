# Native multiplayer weapons and hands

The native multiplayer adapter now carries map weapons into a four-slot inventory
without replacing their full object datums or ammunition. Primary and secondary
hands own separate weapon states, trigger input, reload accounting, event clocks,
first-person poses and HUD ammunition. Native UDP commands and snapshots carry
the same ownership and world-item changes.

Keyboard/mouse controls: holding E picks up or swaps a weapon (or takes compatible
spare ammunition), holding X picks up a dual-compatible offhand, wheel/Tab selects a carried
weapon, LMB fires the primary, RMB fires the offhand while dual wielding, R reloads
both hands, and Q drops the offhand and requests a grenade throw. With one weapon,
RMB keeps the existing scope control. Controller input supplies the same commands:
X pickup/reload, Y switch, left shoulder dual pickup, right shoulder grenade, and
left/right triggers for the two hands.

## Authored animation and source inputs

`Cache::load_first_person_binding` resolves each weapon's actual Spartan/Elite
record and action table. Idle, ready, fire_1, reload_full, reload_empty,
throw_grenade and melee_strike_1 use their authored graph indices and frame/event
records where present. Dual hands select the real `any/dual/any` action table.
Missing optional actions do not disable an otherwise valid weapon binding.
The primary and secondary clocks advance at simulation frequency; rendering
samples their retained intervals. The existing source SMG trigger/reload kernels
continue to run for each SMG independently.

Pickup and dual pickup require the authored player-control hold threshold:
`matg` control+7C, multiplied by 1000 and truncated, gives 233ms in Lockout.
The original player edge/consumption leaf suppresses a successful interaction
until its button is released. SDL and native UDP retain separate elapsed times
for both requests. SDL elapsed-time production is a native device adapter;
the original Xbox analog hysteresis and one-poll latency are separate owners.

The secondary weapon root now follows the original graph-node hand flags and
the opposite authored hand parent. Only that root's local Y position and
forward/up Y components are negated; its left basis is rebuilt in original
arithmetic order. The full mesh is no longer reflected around the camera.
Secondary arms retain the original left-arm node mask, both hands share the
primary camera-control adjustment, and event options are 103F/203F for the
primary/secondary channels. Authored triangle winding and tangent bases survive.

The `weap+12C` dual flag (bit22), barrel+64 damage scale, magazine constructor
fields and model/animation references come from map metadata. Candidate distance
uses the `152E90` authored object bound radius plus .3 (and .1 for special class
`weap+290`), with a strict squared-distance comparison. Ranking follows `152A30`:
greater priority, then nearer root, retaining equal ties. Camera direction does
not gate weapon pickup. The recovered `C57A0` capsule/visibility consumer runs
against the native collision service. Four-slot identities and low-nibble sequence
advancement follow the recovered inventory layout. The original equipment timer
kernel supplies row respawn timing independently of the carried/dropped item.

Source references:

- [Inventory target, bounds, admission and row timer research](MP_INVENTORY_RESEARCH.md)
- [Measured carry/transfer and attachment ordering](MP_PICKUP_TRANSFER_MEASURED_RESEARCH.md)
- [Pickup visibility consumer](MP_PICKUP_VISIBILITY_NATIVE_INTEGRATION.md)
- [First-person clock and presentation](MP_ANIMATION_PRESENTATION_INTEGRATION.md)
- [Grenade action/marker integration](MP_GRENADE_OWNED_PREPARATION_RESEARCH.md)
- [Original pickup input and offhand transform comparisons](MP_WEAPON_INPUT_DUAL_SOURCE_PARITY.md)
- [Source admission and live player carry limits](MP_PICKUP_ADMISSION_SOURCE_PARITY.md)

## Validation and remaining source scope

`weapon_actions_tests MAP` loads actual world/first-person models, resolves both
species, checks single and dual bindings, samples actual weapon/arm geometry and
advances every available selected action back to idle. Lockout validation covers
15 weapon definitions, 352 selected actions, 1,056 finite sampled poses and 156
actual offhand poses across both species' dual-compatible weapons.
`weapon_compound_tests` compares 40 cases (4,416 float words) against untouched
original 137800/1376C0 execution, including the beam rifle's six authored compound
nodes. `native_weapon_gameplay_tests` checks native allocation and
ownership, non-dual rejection, independent firing/reload/ammo, carry selection,
mixed compatible weapons with different magazine capacities, dropping, row
respawn, SDL short taps/focus reset and real loopback UDP delivery.
`native_weapon_runtime_tests MAP SHARED XBE` supplies only diagnostic initial
placement near an actual authored SMG row, then uses real SDL requests and
observes authoritative state and submitted poses through pickup, two triggers,
reload and grenade throw. An optional fourth argument captures the dual view.

Eighteen focused CTest regressions pass. Original player edge/consumption
comparisons cover 4,096 complete player/action buffers. Original dual WORLD
comparisons cover 160 cases and 87,360 exact float words; both suites reproduce
identically in retail, TU1.1 and TU1.5. The actual SDL dual-SMG sequence confirms
the held pickup threshold, two distinct firing/reloading weapon datums, four
submitted first-person clips, offhand dropping and the grenade animation.
One recorded validation run submits 904 dual weapon draws. A separate four-player runtime test
confirms one Q pulse, one inventory debit, 35 first-person and 90 remote-body throw
submissions, grenade world draws in every view, release and detonation. GPU HUD
checks cover four-view shield alpha and independent radar updates; the existing
volume/scope texture regression also passes.
The actual SDL/native-UDP fire/reload client test submits 215 poses, including
213 fractional first-person samples. Its host fixture waits for actual participant
admission before starting the original active-engine query, as the dedicated host
does; this does not establish graphical empty-host lifecycle parity.

Unit HUD resources and motion-sensor history are shared across weapon selections.
Weapon HUDs and material constants retain independent hand ownership. Local games
allocate body material slots for their actual participants; native network games
retain the full player capacity. Byte-identical HUD images and immutable empty
texture-stage images originally shared GPU allocations; identical authored
weapon/stage/animated images now share them too. Samplers and material constants
remain independent. A dynamic 2D image update takes private image ownership
before changing pixels, preserving other views. The residency regression checks
16 non-HUD material banks, all four stages, animated frames, separate volume
typing and independent visible updates. This removes the duplicate-allocation
failure seen when the native UDP client retained its full-capacity weapon bank.

This is a native gameplay adapter, not complete original sandbox parity. The
existing generic non-SMG combat adapter still lacks complete charge/heat/lunge
managers. The live inventory now applies original `152080` player restrictions
using complete authored carry flags. The broader `CEC00`/`EA660`/`CE510` source
port retains mandatory owners and is not yet the live admission path. Full
original inventory action admission, Xbox device polling,
put-away/draw blend scheduling, animated bound/contact producers and registry
ordering remain separate source work. Native world weapons retain authored row
poses; dropped items use the current native unit root and do not yet run original
Havok settling or throw dynamics. The visibility backend is the existing native
triangle world, not the full original collision dispatcher. Weighted item
collections require their original RNG producer and are not admitted by this
adapter. Original third-person firing/reload layer composition, primary/secondary
blend layers, feedback and complete IK
admission remain separate owners; framebuffer equivalence across actions and
weapons remains unverified. Exact dual hierarchy arithmetic on explicit local
inputs does not establish the missing animation managers or full rendered parity.
