# English TU1.5 multiplayer phantom inventory

The original-cache audit covers all 21 supplied Xbox multiplayer maps. It
finds 21 map-local definitions with 43 authored SCEN/MACH/CTRL/BLOC placements
whose physics models contain phantom shapes. The inventory retains map hashes,
placement flags, raw shape records and raw 104-byte phantom force definitions.
It includes initially absent objects and fields that exclude players, and
does not cover every dynamically spawned object type or BSP force owner.

Evidence: [data inventory](../analysis/multiplayer/physics-live-tu15-20261012/all-map-placed-phantom-inventory-v327.json).
The source reader is `tools/audit_multiplayer_phantom_fields_tu15.py`.

Visible native player-field coverage now includes 32 placements across
16 map-local definitions. Each retained case has exact native UDP snapshot
comparison, actual original force delivery and stable executable/live mirror
proof. See [placement results](MP_LIFT_PLACEMENTS_TU15_20261014.md). These are
bounded integration results, not full original retail trajectory parity.

Dune's authored tower and pod are separate force cases. Whole original
`1DA750` resolves the contact material through phmo+44, stride12, signed
phantom-type index+8, then phmo+2C with stride104. The tower's hull4 uses
material1/force type1; hull5 uses material2/force type0. The pod hull25 uses
material1/force type0. The actual raw pod force row includes a negative axial
coefficient. That alone does not determine the sign of a live velocity
adjustment: original feedback can increase a falling body's velocity. Its
current approach result is retained as a diagnostic, not promoted to a
production physics defect.

[Dune shape/material inputs](../analysis/multiplayer/physics-live-tu15-20261012/dune-phantom-shape-bindings-v337.json)
retain exact hull bytes, AABB inputs and force rows. Original source execution
and live body membership/poses are still required to establish their behavior.
Initially absent Backwash HVAC placements also need a distinct lifecycle test.
Moving and rotating supported riders remain an independent open requirement.


## Actual live Dune force calls compared with original instructions

The v339 opt-in observer exports one actual joined-body force call's complete
accessible memory and ABI before/after the real `1DA750`. It supplies no
physical result. Each capture contains 66 regions and 60,084,310 bytes.
Untouched original instructions then execute from exactly those inputs.

The pod passes both Unicorn and host i386 checks. The host check executes
2,086 original instructions. The tower's Unicorn run matches all registers
and physical bytes but omits two sticky inexact FP flags. Actual host i386
execution confirms the native port's flags and all final memory, GP/SSE/x87
state and FP controls/status across 2,777 original instructions. These runs
replace no callee, source service or physical result and patch no original text.
The Linux user-mode harness retains the host interrupt flag at input; its
raw value and explicit comparison mask are recorded. That architectural bit
is outside this host comparison. This is host CPU evidence, not Xbox hardware
or a complete source trajectory comparison.

[Pod original hardware result](../analysis/multiplayer/physics-live-tu15-20261012/dune-pod-original-hardware-v341/report.json),
[tower original hardware result](../analysis/multiplayer/physics-live-tu15-20261012/dune-tower-original-hardware-v341/report.json).
Their actual visible capture runs retain 182 exact UDP states each, stable
ELFs and continuously verified live mirrors. Both generic height assertions
remain red from the declared in-field starts. The tower nevertheless delivers
135 upward impulses in the earlier v338 in-field run. Its normal approach
fixture issues zero jumps because a collision blocks it outside the default
jump trigger radius; an earlier ordinary jump is being tested separately.
No source force, body geometry or solver parameter is tuned to satisfy these
fixture thresholds.


The tower's earlier normal jump now passes the existing actual-force and
further-rise requirements in `dune-tower-earlier-jump-v342`, with 181 exact
UDP snapshots and 71 upward deliveries. Its first normal-entry force call also
passes full original i386 comparison across 1,263 instructions and all
60,084,310 accessible source bytes. This retains the same host IF limitation.
[Normal-entry original hardware result](../analysis/multiplayer/physics-live-tu15-20261012/dune-tower-entry-original-hardware-v342/report.json).
The native original force implementation is unchanged.


The two initially absent Backwash HVAC placements now pass their actual
explicit creation/lift/UDP/render path in v346 after a source prop geometry
preparation repair. They overlap the existing machinery fields and remain
separate from default map birth. Current passing authored placement coverage
is 31 across 15 map-local definitions; see the placement report for scope.


## Player admission classification and remaining object fields

Complete original `1D9FB0` chooses bit3 for a source object with an actual
player binding and bit(type+5) for its source object type. Biped type0 thus
checks bit5. Applying those original flag tests to the audited force rows
finds 32 player-flag candidates and 11 excluded placements. All 32 candidates
now have bounded native live force/rise evidence. This classifies flags only;
size/dead/filter conditions and every dynamic/BSP field remain separate.

The 11 excluded placements are Colossus grav_colossus(4), Midship phantom
volume(1), Elongation crate cushions(2), Gemini gravity columns(2), and Turf
hinge-door springs(2). They require actual object/body interaction tests.
They must not be judged by a player lift assertion. The initial Colossus
probe sees no field members during its tested interval; authored spawner and
actual body lifetimes are being examined before claiming object-field proof.

The normal Dune pod entry additionally passes untouched original i386
comparison across 1,100 instructions and all 60,084,310 source bytes, with
the documented host IF boundary.
[Pod normal-entry original result](../analysis/multiplayer/physics-live-tu15-20261012/dune-pod-entry-original-hardware-v351/report.json).
