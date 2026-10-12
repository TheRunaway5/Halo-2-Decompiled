# Source scope camera and action contract

The native scalar camera implementation now matches the measured original
normal-camera FOV producer, observer adoption, remaining-time polynomial and
horizontal render FOV. The suite contains 28 frames, 112 complete scalar phase
states and four authored FOV leaves; optimized and ASan/UBSan builds pass with
every represented float compared by its bits. Runtime integration belongs to
the caller: this module does not manufacture a local player/view binding,
replace action ownership or reconstruct the other five observer channels.

## Authority and immutable evidence

The executed Xbox 1.1 image is
`assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
The independently authenticated supplied 1.5 image is
`/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe`, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Original instructions, not annotation names or Ghidra pseudocode, own behavior.

Evidence lives in `analysis/multiplayer/integration/scope-camera/`:

| Slice | Original tool | Independently identical report SHA-256 |
|---|---|---|
| Camera: `measured-frozen` / `repeat-frozen` | `tools/oracle_mp_scope_camera_1_1.py` | `61ca2404bea7d70801fc4043a1d628cf3f7cf55d9638141601cf9f357cc85860` |
| Positive jump/crouch: `action-flags-measured` / `action-flags-repeat` | `tools/oracle_mp_scope_action_flags_1_1.py` | `6a837a0bb9bea026733dba6a18303a2191934cc8593602c6fe6ce6640bfb15db` |
| Camera/action 1.1→1.5: `update-comparison-frozen` / `update-comparison-repeat` | `tools/research_mp_scope_camera_updates.py` | `ccc5e85c4355b8e51ffca18f5dd6cbe881355bc3ad4095f4828d61545ef4e5c2` |
| Scoped fire/reload: `fire-measured` / `fire-repeat` | `tools/oracle_mp_scope_fire_1_1.py` | `77110d8b832f99ba69509ffa258027430c81d63fe4333cc42c37950269b8fbba` |
| Manual callback/reset correspondence | `tools/research_mp_scope_reset_callbacks.py` | See separately frozen `reset-callback-measured/report.json` |

All original instruction listings in each paired execution/comparison agree.
`audit-frozen/audit.json` independently checks 65,347 literal original instruction
rows, 112 camera states and 32 genuine positive action states. Its SHA-256 is
`c9cb32e8b17dd896f2c5d8415014686224edae99cbdd704c0f0be75fe805e5ec`.
`reset-audit-frozen/audit.json` checks another 14,544 literal rows and all 32
firing frames; SHA-256
`560000256ff760c450c832ba9e4ad2889c321632df328c95d44a355628b89c36`.

The firing freeze contains a preserved prose error: it says local view remains
NONE, but its numeric output is local view 0 and unit owner player 0. This is the
inherited older explicitly preallocated player context, not an executed source
local-session binding. The separate reset audit records that correction. Both
original reports and their tool remain untouched. That firing suite captures
unit selector reset stores, not the retained per-view control buffer.

## Native caller and selector scheduling

`include/halo2/unit_scope_camera.hpp` and `src/unit_scope_camera.cpp` represent
one FOV channel. `tests/unit_scope_camera_tests.cpp` reads
`measured-frozen/fixtures.tsv`, SHA-256
`979f0a49446cf4b0b74bb1da93062f747cbcdff796d659f07d78c7d827e28664`.
`tools/export_mp_scope_camera_fixtures.py` exports source outputs directly;
no native function supplies expected values.

The ordinary frame runs original input production `188AD0` before simulation,
then normal camera `156C00` and observer update `170D00`. The normal camera's
persistent target starts at zero. Its supplied local view/control linkage and
the incoming selector are explicit native adapter inputs in this camera
fixture; no source session-creation claim follows from them.

For a valid unit, original input producer `1884E0` calls `C9920` at `188775`.
If AL is zero, `18877C` branches to `188801`; `188805` writes retained view
control `+2E=FFFF` and `188809` clears `+89`. Its earlier runtime/UI rejection
branches also join this clear. Admitted no-edge input retains `+2E`; original
controller tap/hold/release handling uses processed bits 4/8 and `+89/+8A/+8C`.
Those Xbox polling branches are separate from user-authorized native RMB input.

At `188A74/188A78`, the producer copies its retained signed-short `+2E` into
outgoing command `+24`. `150F00` does **not** replace that selector with unit
CURRENT or REQUESTED: `151768` only compares CURRENT with the already supplied
incoming selector for activity. `188B60` and `C9BE0` do not recheck `C9920`.
The camera uses the retained view selector, even when CURRENT differs. Hence
native valid-unit admission failure must clear the producer selector before
building the camera packet, in addition to explicit shot/reload/damage resets.

The early owner-NONE branch at `1885C4` skips the zoom section to `188931`;
it does not itself prove a selector-removal write. Owner teardown remains its
own lifecycle. Unit CURRENT=-1 alone is not a source selector-clear predicate.

## Exact scalar camera behavior

`188B60` obtains authored `bipd+CC` and magnification from `C9BE0` using the
view selector. Magnification 1 returns the authored FOV. Otherwise `DIVSS`
computes FOV/magnification and accepts it only strictly between the immutable
source limits. Invalid division/range returns authored FOV. Missing unit uses
the executed static default. Stock Spartan FOV is bits `3F9C61AA`; stock SMG
binocular magnification 3 yields bits `3ED08238`.

`23E210`, through normal `23DE70`, creates packet flags 1 and global duration
0. It multiplies FOV by the immutable packet multiplier, clamps it and writes
packet `+28`. Only a changed persistent target writes group-4 flag 1 and
duration bits `3E3851EC` at packet `+90/+A4`. An unchanged target leaves these
caller-cleared fields zero; adoption retains the original remaining motion.

Fresh source BSS and `170F30/170E40` own initial current/target FOV. This factory
is not the reset of a running observer: the original constructor can retain
derivatives. First `170C10` registration sets forced flag 8 and zero durations,
so first adoption snaps. Registered packet flags are retained separately from
adopted packet flags because `171910` skips motion when registered flags include
`0x20`.

`170FF0` adopts only packet bit 0. Group flag 1 with flag 2 clear retains a
longer prior duration, capped at source 2. Group flag 0 retains prior remaining
time when it exceeds global time and forced flag 8 is clear; otherwise it uses
global time. Thus unchanged input does not restart or cancel the transition.

`171910` FOV channel 4 uses current `+130`, target `+30`, remaining `+AC`, velocity
`+1A4`, acceleration `+1D8`, delta `+344` and coefficients at
`+20C/+240/+274/+2A8/+2DC/+310`. The native module preserves its exact separated
float arithmetic and remaining-time quintic, including interrupted transitions,
snap expiry, retained coefficients and source signed-zero behavior. Source dt
is the caller's actual frame float times the original multiplier. `172A50`
clamps the resulting horizontal FOV and writes observer/render packet `+F0`.
The existing original camera-angle projection consumes this horizontal FOV;
the scalar module does not replace original `173810` projection with a guessed
perspective or tangent interpolation.

Observer acceleration cancellation is coupled. When a channel exceeds its
source cap, the original cancels that duration and all other channel durations
exactly equal to it. The module reports FOV cap cancellation; the caller must
perform matching-duration cancellation on its other channels. Cancellation by
an earlier channel must already be reflected in supplied FOV remaining time.
The stock frozen frames never take either coupled branch. They establish the
normal FOV subset, not all six-channel motion behavior.

## Action and reset ownership

The genuine allocator/body constructor `C52C0→BD270` produces a 44-byte action
component, reached through signed unit `+346`. `C54AB..C54B5` requests descriptor
`+344`, size 44h and alignment zero; `BD317` writes its body-relative offset and
`BD331/BD333/BD33A` zeroes storage. Later sentinels overwrite only component
`+10/+14/+18/+2C`. Component `+4/+8` therefore start genuinely zero; native State
or Body.grounded is not their producer.

The independently executed full-world jump/crouch suite uses the actual
source-created player, biped `E26F0000` and body-relative component offset 1632h.
All four neutral/crouch/jump/both cases return through full original submission,
player update and world update/postpasses. Positive crouch uses `E3B20→DEE00`,
sets unit134 bit23 and crouch2C4 bits `3E2AAAAB`. Positive jump executes
`E5CB0→E5DA0→E6E10`, returns AL1 and produces velocityZ `40451EB8`. All 32
phase states retain action component words `+4/+8=0`. Parent is NONE, object
10A byte 0 and authored bipdBC bits `00448008` (bit10 clear). These are measured
ordinary source states, without a supplied contact/velocity/action success.

Manual reload `E88B0→E8740` sets its action bit only on success. The primary
SMG reload bit is 0. `E8950→104A30` retains this action while its relevant
magazine signed mode is 1..6; `E7E60` clears the bit when the callback returns
AL0. Automatic magazine reload does not manufacture this manual action bit.
The owning original callback table at 46EC68 index0 points to record46EA04
`[E88B0,E8950,0]`; authenticated1.5 table473D28 points to473AC4
`[E97D0,E9870,0]`.

**Ordering distinction:** the constructor/firing harness explicitly composes
`C7E90→1021D0→E7E60`. It is not an ordinary dispatcher-order proof. Ordinary
`BD360` calls type dispatcher `10CDF0` before recursing into carried children.
Biped table46EF18 has `C7E90` then `DE960`; `DE960` calls `E7E60` at DEDD4.
Its carried weapon's table46F170 then calls `1021D0`. Thus the ordinary attached
biped action callback observes pre-weapon-update magazine mode. If that child
update finishes reload, the bit can persist until the following biped phase,
whose scope admission occurs before its action callback clear. Preserve this
source phase distinction when integrating; do not silently promote the harness
post-weapon ordering into retail frame parity.
Separate `ordering-measured` / `ordering-repeat` reports compare complete
`BD360→BE4A0`, `10CDF0→10DD10`, `DE960→DF880` and `1021D0→1030F0` bodies;
all four primary and78 retained source pairs match. The actual1.5 type table
corresponds to original46FAD0 at474B90. These checks preserve both older suites.

Successful manual `E8740` invokes `C9A40(0)` and clears both unit selector bytes
and motion. Prepared shots invoke `107160→C9A40(1)` at1079C5, before projectile
postpass, when the actual owner is not NONE. The count57..60 stock SMG trace
shows three such resets. `C9A40(1)` retains zoom only when actual primary
`105120` succeeds: authored signed zoom count is nonzero and no authored
magazine has signed runtime mode1..6. Stock SMG count0 resets binocular scope;
an eligible optic weapon retains it. No weapon flag or guessed scope rule is
part of this predicate.

`C9A40` writes current/requested=-1 and motion=0. Its valid mapped-view branch
also writes producer `+2E=FFFF` and `+89=0`. Native view retention must consume
these reset events or it will reinsert scope next input frame. The fire fixture
deliberately supplies selector0 again on frame2, confirming that a fresh
incoming selector can reinsert zoom; this is a stress boundary, not an executed
ordinary local polling producer.

The existing original damage history evidence establishes result58h `+44` as
normalized body request and `+48` as normalized shield request. Both are full
requests, including overflow, and match native `UnitDamageResult.normalized` in
170 complete-entry differential samples. `CB410` calls `C9A40(0)` when either
is strictly greater than zero. It does not test clipped health loss or StateDead.
See `docs/MP_SMG_DAMAGE_HISTORY_RESEARCH.md` and the frozen
`analysis/multiplayer/integration/smg-damage-differential.json`.

## Executable constants and later update scope

`audit-frozen/constants.json`, SHA-256
`c1b672a35a13578dd08643179e2a6115b5d3cf02be7f0ef24c8559172fe3f733`,
lists every constant in `UnitScopeCameraConstants` order with both image
addresses, literal bytes and initializer provenance. Read the immutable input
of the six executed MOVSS static writers, not serialized zero runtime globals.
The complete six-cap table and double reciprocal numerator are separately
verified. The normal range constructors actually execute in the1.1 suite;
the1.5 literal writers and their genuine initializer-table entries are audited.

The camera/action comparison has 48 primary and154 total mapped original
routines. All 48 complete primary bodies/internal branch topologies match;
the reset callback extension adds nine complete matching primary bodies,
including `CB410→CC590`. This is bounded static1.5 equivalence, not a fresh1.5
full-game execution or proof of all runtime globals/platform services.

Reproduce into **new** output directories; preserve canonical freezes:

```sh
.tools/python/bin/python tools/oracle_mp_scope_camera_1_1.py --output /tmp/halo2-scope-camera-repeat
.tools/python/bin/python tools/oracle_mp_scope_action_flags_1_1.py --output /tmp/halo2-scope-action-repeat
.tools/python/bin/python tools/oracle_mp_scope_fire_1_1.py --output /tmp/halo2-scope-fire-repeat
g++ -std=c++20 -O2 -fno-fast-math -ffp-contract=off -Iinclude src/unit_scope_camera.cpp tests/unit_scope_camera_tests.cpp -o /tmp/halo2-scope-tests
/tmp/halo2-scope-tests analysis/multiplayer/integration/scope-camera/measured-frozen/fixtures.tsv
```

Scope HUD/weapon presentation, complete camera position/basis, other-channel
cap cancellation, native local-owner lifecycle and unsupported action owners
remain separate integration/evidence boundaries.
