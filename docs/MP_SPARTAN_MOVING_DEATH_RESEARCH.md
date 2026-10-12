# Source moving Spartan death and retained momentum

The independently repeated original ordinary death composition preserves a
genuinely moving Spartan's object+88 velocity through the physical body rebuild.
The native ordinary damage path now retains that velocity. This does not deliver
the requested ten-body ragdoll, death animation, or continued corpse physics.
The kill-trigger velocity clear is unchanged because its owning transition is
not measured here.

## Original execution and precise scope

The authority is the unchanged TU1.1 executable
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`
and actual Lockout/shared cache bytes. The separate oracle is
[oracle_mp_spartan_death_physical_1_1.py](../tools/oracle_mp_spartan_death_physical_1_1.py).
Its [measured report](../analysis/multiplayer/integration/spartan-death-physical-20261010/moving-effect-fixed/report.json)
and independent fresh-process repeat have SHA256
`a409f68196a35a4a81dd282c37da397874bac391ba557c32153e912bd0ed9526`.
All326 report/instruction files compare identically. There are685 observed
original calls,48,765 ordered stores and92,645 reads.

Preparation reproduces the complete frozen neutral command fixture first,
restores its actual output backing/CPU state, and repeats the frozen
`authored_heading_forward` case. Every before/after state in that case compares
byte-identically. External input is the declared command axis pair1/0 and
original11DDA0 delivery of the actual chosen authored start37 heading. Neither
live device admission nor an active whole-title frame is established. The
original command/control/physics path produces velocity; no unit velocity,
root, physical mode, clip, pose, contact, PVS membership, clock or success is
assigned. [Prior movement authority](MP_COMMAND_LOCOMOTION_POSE_RESEARCH.md).

The new explicit composition then executes selected original Spartan phmo
postprocessing, complete1775D0→175860 effect storage construction,177690 reset,
and D7CD0 attribution for the actual unit. It calls diagnostic original
D7E00(unit,owner,1,0), CBDC0(unit,NONE,NULL,0,0), followed by B7E00→1C7660→B7F50→B8040.
These are declared owning roots, not a newly recovered incoming fatal-hit
producer or natural repeated frame scheduler. The inherited30Hz timestep and
root-call FP/ABI adapter remain explicit. No clock increment or second full
game tick is manufactured.

The retained negative predecessor also remains available: without original
effect construction, D7E00 stops at16D2C3 readingNULL+34 through179BE0. Its
[report and executed tool snapshot](../analysis/multiplayer/integration/spartan-death-physical-20261010/unfixed/report.json)
are preserved separately. Pool pointers, bitmaps or allocation success were
not filled to bypass it.

## Measured transition

| Owning stage | Object+88 three binary32 words | State+3DC | Mode+34B | Physical bodies |
| --- | --- | ---: | ---: | ---: |
| Original movement output | `3C33EFD9 3EA3BE55 BE6F9DB2` | 1 | 0 | 1 |
| D7E00 return | same | 1 | 0 | 1 |
| CBDC0 return | same | 3 | 0 | 1 |
| Following B7E00 return | `3C869D9F 3EF59DAF BE6F9DB2` | 3 | 0 | 1 |
| Following B7F50/B8040 return | `3C869D9F 3EF59DAF BEBEFF5C` | 3 | 0 | 1 |

The entry velocity is approximately(0.0109824771,0.319811493,-0.233999997).
The cleanup's actual unchanged E6050→E6DC0→1E8E60 transition calls
**1C70D0→1D4C50→1D3BB0→1D2B10→1DA570→1DA6F0→1D4F30** as admitted by
the stored component flags. Original1C70D0 remembers component+4 bit5, removes
world membership when set, disposes current body storage, replaces component+4
with caller0, rebuilds, and restores world membership when previously set.
Its two observed complete calls return with ESP delta12. Object+88 survives
that owning chain exactly. The following ordinary phases change actual velocity
and root through their original physics consumers; these outputs are evidence,
not newly installed native corpse movement.

This is a **controller rebuild**. Complete1DA570 reads dead state3 but observes
unit+34B0 and the actual bipd+1F0 bit11 clear, so it calls1DA6F0 with source
mode10. It does not call1D9330. E2520 subsequently returns0 with actual
unit+3FD/+3FE=0/0; no ragdoll admission is inferred from the dead bit.

Original1F0E90 owns physical-state+21, corresponding to unit+3FD, from its
qualifying retained component contact bank at+88/count+8C. The selected row's
counterpart+14==NONE identifies the branch that sets this byte. The measured
bank does not provide that positive row. E2520 also has the separately listed
1F08E0 admission; its actual input byte+11 is zero in this execution. Recovering
genuine contact delivery/admission remains the next live transition gate.
Neither a ground ray nor native grounded establishes these bytes.

The three supplied170060 audio playback returns and one suppliedEFE30 action
notification remain inherited explicit boundaries during cleanup. Original
physics/body callbacks are not replaced. EFE30 is the first-person notification
wrapper, and this experiment does not claim complete successful audiovisual
death playback. Recorded RNG consumers, weapon/grenade drops, marker/resource
adapters and actual full object datums remain in their original order.

## Independent audit and native regression

[audit_mp_spartan_death_physical_1_1.py](../tools/audit_mp_spartan_death_physical_1_1.py)
checks65,245 listing rows against original executable bytes, verifies the
entire report/listing repeat, checks all recorded read/store instruction
addresses, and replays125 complete retained-buffer intervals against12,509
overlapping reads and5,182 stores. Full raw unit, component, Havok backing,
player, clock, world and queue inputs/outputs remain in the report.
[Compact audit](../analysis/multiplayer/integration/spartan-death-physical-20261010/audit.json).
Static authenticated1.5 correspondence for E6050/E6DC0/1E8E60/1C70D0/CBDC0
remains the separately frozen [death audit](MP_DEATH_RAGDOLL_INTEGRATION_RESEARCH.md);
this extension dynamically executes1.1 and does not relabel its addresses1.5.

The localized production change removes only the unconditional velocity clear
inside Simulation::damage after ordinary vitality depletion.
[multiplayer_death_momentum_tests.cpp](../tests/multiplayer_death_momentum_tests.cpp)
uses actual Lockout starts, authored stock-SMG inventory/ammunition and normal
damage/vitality. Only the shooter uses public debug teleport/noclip setup; the
victim moves through Controls and its velocity/health are never injected.
A second unhit production Match measures the actual current movement before
the projectile postpass. Fatal cleanup and the retained old full unit datum
after respawn must preserve those exact words, previous player and death tick.
The disposable source copy restoring the old clear fails at the fatal-momentum
assertion. No native dead-body movement, pose selection or solver claim is
added by this correction.

Reproduce original evidence:

```sh
.tools/python/bin/python tools/oracle_mp_spartan_death_physical_1_1.py \
  --moving --effect-storage --phmo-fixup --output NEW_DIRECTORY/report.json
.tools/python/bin/python tools/audit_mp_spartan_death_physical_1_1.py \
  --directory analysis/multiplayer/integration/spartan-death-physical-20261010/moving-effect-fixed \
  --repeat NEW_DIRECTORY --output NEW_DIRECTORY/audit.json
```

The registered production target `multiplayer_death_momentum_tests` now passes
`original_multiplayer_death_momentum` in both the normal physics build and the
actual ASan/UBSan build with halt-on-error and leak detection. The normal test
takes 0.04 seconds and the instrumented test 0.75 seconds. Earlier stale-target
and mixed-header build failures are retained alongside the fresh successful
build/test logs; they are not counted as successful validation. The archived
binaries are newly linked against the completed current headers. No release or
presentation code changes are included.
