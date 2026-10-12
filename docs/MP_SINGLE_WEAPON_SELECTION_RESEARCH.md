# Original one-weapon switch selection

The complete original CF300 returns **slot0**, not NONE, for the genuine
allocated Spartan carrying only its starting SMG. All six measured calls
(previous0/NONE, direction-1/0/+1, modeNONE) return AX=0 and preserve the entire
source-owned unit, weapon, manager, player, clock, RNG, pose and physical state.
For currentPrimary0/currentOtherNONE this is a request for the existing slot:
there is no positive switch or invented second weapon.

It does **call a capability producer**. CF37D invokes CEAF0 with the actual SMG
datum in EDX and `(unit,mode)` on the stack. CEAF0 invokes111490 at CEBD4 with
unit in EAX, source mode07000101, class05000720 and name03000D26. Original
111490→1CFDE0→1CFCE0 executes and returns true. No supplied capability,
world/Havok result or new guest write admits this case. The isolated native
selector retains CEAF0 as a required service; it never assumes that one slot
removes the capability query. Missing capability fails before modifying state.

[New oracle](../tools/oracle_mp_single_weapon_selection_1_1.py) first reproduces
the immutable actual player/biped/SMG/PVS/world neutral command composition.
Each call restores its exact complete guest backing/CPU snapshot. Only selector
ABI arguments vary; the original188630 caller establishes modeNONE. This
experiment does not admit a live local device or change readiness. Original
Xbox reciprocal and inherited startup/resource/platform boundaries remain
explicit. The ordinary selector does not use floating-point arithmetic.

Measured original source directory:
`analysis/multiplayer/integration/inventory-source/single-weapon-selection/`.
The report SHA256 is
`0a0b7d14df711a4719a745b20ebf0b2171fae23030697769cc37a6fe15bb3ebc`;
oracle SHA256 is
`f1d2a72a918e3733840501af410f196cbc706ddf2a0571bd64108a08bd066ca1`.
The independent repeat matches the report and every instruction file exactly.
There are six complete selector calls, six genuine capability chains, 30
observed calls, 999 instruction reads, and 4,110 temporary/stack stores. The
complete source-owned state is identical before/after; temporary stores are not
inventory writes. Every listed instruction is audited against unchanged original
image and guest bytes. Original TU1.1 image remains SHA256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.

## Exact native seam

[Public module](../include/halo2/unit_weapon_selection.hpp) and
[implementation](../src/unit_weapon_selection.cpp) port the integer selector,
with explicit stable unit storage, full-datum definition and CEAF0 services.
The supported ABI is previous-1..3 and direction-1/0/+1. Other inputs fail rather
than indexing outside native storage. Only the actual one-SMG branch is executed
in this new suite; competing flags/timestamps and occupied other-hand exclusion
remain source-listed branches, not new measured coverage.

Source unit fields are full carried datums218..224, signed slot timestamps
228..234 and signed other-hand byte213. CF300 retains its initially resolved
unit pointer across capability calls. It checks capability before other-hand
exclusion, reads timestamp before definition12C bit3, and scans all four slots
including current primary. Start is0 for previousNONE, otherwise
`(previous+4+direction)%4`. Negative direction steps3 modulo4; other directions
step1. First admitted candidate wins; an already selected bit3 candidate is
retained, a new bit3 candidate beats a plain candidate, and direction0 alone
chooses a greater **signed** timestamp among plain candidates. No owning store
or clock advance is introduced.

Existing immutable complete body correspondence proves CF300→TU1.5 D0390 with
the same internal branches and integer operands. Its call target is CEAF0→CFB80,
but this note does not claim a full TU1.5 manager/runtime execution or infer
external capability correspondence from the selector body alone. The existing
[static mapping](../analysis/multiplayer/integration/inventory-source/inventory-switch-u5/report.json)
and its original listings remain unchanged.

[Exporter](../tools/export_mp_single_weapon_selection.py) uses only original
captured complete unit prefixes, actual definition bytes, capability ABI/AL and
selector AX. [Focused test](../tests/unit_weapon_selection_tests.cpp) compares
every capability argument/order and complete entry/exit unit prefix. Literal
capability outputs are supplied only by this differential. Normal and
address/undefined/leak sanitizer builds both pass six calls and six capability
boundaries. The fixture and export report independently repeat byte-identically.

Fixture:
`analysis/multiplayer/integration/inventory-source/single-weapon-selection/native/fixtures.tsv`,
SHA256 `1e5ddf344ea3839cde7dbdd5a9a18a38e2ff5ed691b7f09b966a7f81792f89d9`.
Its compact schema is C(name,unit,mode,previous,direction,AX), B/F(raw unit800),
K(unit,item,mode,AL,raw entry/exit unit800), D(full item datum,raw definition360).

Root-owned build additions are `src/unit_weapon_selection.cpp` in halo2_systems,
a unit_weapon_selection_tests executable linked to that library, and an
original_unit_weapon_selection test invoking the fixture above with missing-file
skip code77. No runtime, shared Unit, CMake, geometry, damage, animation, release
or prior evidence file changes are made by this extension. A live adapter must
preserve equality-to-current as a no-op and retain unavailable second-item,
capability/channel, physical and pickup owners explicitly.
