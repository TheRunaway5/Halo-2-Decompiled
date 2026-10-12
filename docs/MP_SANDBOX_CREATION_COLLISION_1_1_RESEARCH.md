# Original Lockout prop creation and collision, disc update 1.1

This independent experiment executes the original placed Lockout explosive box
and lift request/constructor paths. It supplies no object identity, header,
body, pose, physical component, contact, physics admission or success result.
It does not yet execute an ordinary player's lift contact or an explosion.
The earlier [authored inventory and scalar contract](MP_SANDBOX_EXPLOSIVES_LIFTS_SOURCE_RESEARCH.md)
remains unchanged.

## Owning factory and missing map postprocessing

Actual **BFFE0** takes the original scenario row in EAX and five stack arguments:
object type, row index, palette-block pointer, placement-check boolean and final
creation boolean. This fixture selects type 11, an existing row and its actual
palette at scenario+330, with both booleans true. Those booleans are declared
direct-call inputs; the entire automatic scenario placement loop is not executed.

Original **D6660** builds the C4-byte request from the row/palette, invokes
**B8820** for the real defaults/sentinels, converts the row's original Euler
rotation with **11F680**, copies placement/variant fields and checks original
BSP placement policy. Original **B8A30** then owns header/body allocation,
damage state, model nodes and region/permutation storage, type-11 **11D1C0**
and the real Havok component/body construction.

The selected explosive is **E6F80583 power_module**, scenario crate row 1. The
selected lift is **E6ED0578 lockout_lift_tube**, scenario crate row 0. These are
actual map placements; no guessed tag name or standalone asset replaces them.

The serialized power-module rigid body has **phmo E7090594, bank +38, stride90,
record+38=4**. Passing that serialized index to the original physical constructor
without its map postprocess stops at **1D9994**, `inc word [EDI+6]`, with EDI=4
and invalid address A. This is not an absent physics-constants pointer.

The source tag postprocessor **11DE10** dispatches `phmo` at **11DECA/11DECC**
to **1EE380(0)**. The new fixture executes the actual tag iterator **1243C0**
for the authored phmo and the unchanged selected dispatch fragment
**11DE40..11DED1**, stopping before the next iterator. **1EE380/1EE8F0** replace
the rigid-body record+38 with **8025CC40** and populate the real shape/vtable,
reference-count and child pointers. Every touched raw shape record is retained
before/after. The whole map postprocess is not asserted complete.

After that original selected-tag postprocess, the full power-module factory
returns **E26F0000**. Its real object header pool has count/high-water 1, its
body has original size382, physical component **E1680000**, and a genuine
allocated Havok body/motion/world insertion. No service executes during its
factory except the original shared-RNG store recorded in the inherited trace;
that store is an executed game instruction, not a supplied service.

## Collision queries on the source-created explosive

The experiment calls original **16A560** on the explicit normal projectile mask
2480000F; the actual result is **9**, used as the lower traversal flags. It then
executes **16AF90 → 1F3500/1F3900 → original BSP traversal → DA0E0/16A5C0**
against the real allocated model nodes and region/permutation state. No poses,
model mapping, surface identity, material or intersection return is supplied.

The query segments are explicit fixtures: for each world axis, origin is the
source-created root minus2 on that axis, delta is+4 on that axis, and the next
axis receives an offset of −.5, 0 or+.5. These segments exercise the authored
shape; they do not stand in for a source-produced SMG muzzle/flight request.
The 5C-byte collision accumulator is an explicit direct-consumer input, zeroed
with fraction1. The direct selected-object flag is true, so the world broadphase
and its registry candidate selection are outside this bounded query.

All nine queries return through the unchanged original instructions with zero
supplied services. Six offset segments miss and retain fraction1. The three
center segments hit the authored power-module surfaces:

| Axis | Fraction bits | Fraction | Global material |
|---|---|---|---|
| X | 3EEF9DB2 | .46799999475479126 | 86 |
| Y | 3EF62B6B | .48080000281333923 | 86 |
| Z | 3ECB923B | .3976000249385834 | 86 |

Each complete 5C-byte output, original selected surface/plane/normal/material,
source object/body/node state, arguments, ordered stores and executed address
set are retained. This establishes the object collision consumer for a real
created explosive; it does not establish moving-prop contact, native integration,
area damage or full Havok/world parity.

## Lift reaches an audio-owner dependency

The same original selected postprocess executes on the lift's **E6F3057E phmo**,
including its two phantom shapes. The factory allocates a real object and
**E1680000** physical component, then enters **BFB90 → 18A9C0** to register its
authored looping sound **E6F4057F**. The full factory has not returned yet.
Original **16D2C3** reads `[EDX+34]` with EDX=0 because owner pool **4F485C**
is absent in this world-only composition. The stop/address34 is retained.

This fixture does not replace that owner with a fabricated pool or unavailable
audio success. The source owner constructor/reset **18A200/18A2A0** are already
separately researched in the immutable audio owner-pool suite, whose reset stops
at its absent driver. Composing that real owner is a separate next step.
The lift's provisional allocated body/component is not described as a completed
active scenario lift, and its **1D5830** contact admission and **1D5E90** force
application have not executed here. The earlier512-case scalar freeze remains
explicit about its contact, mass, projected-velocity and clock inputs.

## Evidence and reproduction

Owned update image SHA256:
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
Owned Lockout SHA256:
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.

New tool: [oracle_mp_sandbox_owner_1_1.py](../tools/oracle_mp_sandbox_owner_1_1.py).
Source-owned empty-world preparation is the immutable `PlayerTrace.prepare`;
its original object/physics/allocator constructors remain unchanged. Original
options/default-Slayer delivery, raw host storage and inherited services remain
declared in every report. There is no local player/view, complete map readiness
or normal local control producer in this composition.

All three final cases replay the same **whole measured compatibility-produced
KPCR/ETHREAD/stack/process records at their actual addresses**. They do not patch
selected fields or assert retail Xbox thread/context parity. Each case is executed
twice independently and whole reports/instruction listings must match exactly.
The earlier actual live worker measurement is retained outside this final freeze;
the final evidence explicitly uses measured-context replay.

Final reports are under
`analysis/multiplayer/sandbox-source/owner-creation/final/`:
`power-serialized-{measured,repeat}`,
`power-fixed-{measured,repeat}`, and `lift-fixed-{measured,repeat}`.
The independent manifest is `owner-creation/frozen/suite.json`; the audit validates
all recorded files, unchanged prior pins, raw source image bytes, creation/stop
contracts and every complete collision output. Run:

```sh
.tools/python/bin/python tools/audit_mp_sandbox_owner_1_1.py
```

To reproduce a new independent result without overwriting a frozen path:

```sh
.tools/python/bin/python tools/oracle_mp_sandbox_owner_1_1.py \
  --fixup --collision \
  --context-replay analysis/multiplayer/sandbox-source/owner-creation/final/power-fixed-measured/raw-host-context.json \
  --output /tmp/halo2-prop-owner-repeat/report.json
```

Use `--tag lift --fixup` for the retained lift/audio stop and omit `--fixup`
for the serialized-shape control. All reports keep
`native_gameplay_update_applied:false`; there is no latest-title-update,
complete explosion, lift/player motion, audio playback or hardware parity claim.
