# 1179C0 / 10B840: device update callbacks

Native replacements of the two device update callbacks of the scene device
constructor unit (`src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp`).
10DD10 (the object type updates, native) runs them through the type chain's
`+0x40` callbacks: DEVI binds 10B840 and MACH binds 1179C0, so a machine runs
both, a control or light fixture runs 10B840.

## Files

| Part | File |
|---|---|
| Logic (game systems, named layouts) | `src/physics/runtime/native_tu15_device_update_systems.hpp` |
| Boundary adapters (ABI, x87 stack, XMM, frames, cursor) | `src/physics/runtime/native_tu15_device_update.hpp` |
| Binder (pins, children, dispatch routes, operand identity) | `tools/native_device_update_replacements.py` |
| Harness | `tests/native_tu15_device_update_tests.cpp` |
| Validator | `tools/validate_native_device_update_tu15.py` (CTest `native_tu15_device_update`) |

The binder is registered in `spatial_owner_bindings`
(`tools/specialize_native_operands.py`, marker `static void run_1179c0(`) and
in the specialization helper list (`cmake/NativeOperandSpecialization.cmake`).
It installs only into the build-local specialized copy of the scene device
unit.

## What the routines do

**10B840, device update** (`device_update::run`), the channels of every device:

1. Clears the `moving` flag (device `+0x12C` bit 0x100); this update sets it again.
2. Position (`+0x140`, velocity `+0x144`, group `+0x13C`). A scripted position
   (flag 0x8) follows its script (10CD70) until done (flag 0x20); its velocity
   is the change divided by the tick length. Otherwise a position with a group
   approaches the group's value when the value, the velocity or the
   initializing flag (0x80) asks for it:
   - the animation follows the position (1CE8D0) when the device has an
     animation block (`+0x12A`) and a position channel (`+0x18C`);
   - acceleration and speed are the powered ones weighted by the power
     (`+0x134`) plus the unpowered ones weighted by 1 - power (definition
     `+0xC8..+0xD4`); `moving` is set when both are positive;
   - a closed device waits its delay (definition `+0x10C`), counting ticks in
     `+0x148`; the delay effect (`+0x114`) plays on the first tick (10CA30);
   - the velocity is clamped to the speed (x87 |v| > speed) keeping its
     direction, and the channel steps (121890, which loops when definition
     `+0xBC` bit 0 is set);
   - arriving plays the opened or closed effect (`+0xF0`/`+0xF8`) by the
     direction before the step; a velocity that turns plays the opening or
     closing effect (`+0xE0`/`+0xE8`);
   - a moved position, or the first update, is reported (10C5D0) and the
     animation advanced (1CEBD0).
3. Power (`+0x134`, velocity `+0x138`, group `+0x130`): a scripted power (0x10)
   follows its script; otherwise, unless the position is scripted, the power
   approaches its group's value the same way, without delay or effects (B8760
   checks the power channel `+0x1AC`).
4. The first update of a device with a position: a machine notifies (BF050,
   1181F0(1)); the initializing flag clears unless the object still counts
   its awake ticks (`+0xB3`). Returns 1, otherwise the changed byte.

**1179C0, machine update** (`machine_update::run`):

1. A machine that drives its physics cluster (object flags `+0xC0` bit 0x1000)
   passes its power, at least 1 with bit 0x400, to the cluster record
   (1DA750), unless the power is within epsilon (0.0001) of 0 and the floor is
   off (x87 compare).
2. With an animation block and a position animation channel, the channel
   follows the position.
3. A gear (machine type `+0x11C` = 2) turns: its rate is (1 - power) *
   unpowered rate + powered rate * power, at extended precision (FLD/FSUB/
   FMUL/FADDP, spilled to the frame with FST), clears its velocity, advances
   its position by rate * tick length and wraps at 1 (the animation channel
   is advanced to 1 and restarted from 0). A turning gear marks its network
   entity (`+0xD4`, B7E50 with 0x400), reports the position (10C5D0) and
   publishes it to its position group.
4. An unlocked door (type 0, machine flags `+0x1CC` bit 1 clear) queries the
   units around it every fourth tick outside game mode 4 (BD080 with the
   activation radius `+0x118`, or the bounding radius when that is below
   epsilon). A unit opens the door unless it is a vehicle that does not open
   doors (unit definition `+0x1EC` bit 20), its own flag (`+0x10A` bit 2) or
   its definition (`+0xBC` bit 14) says so, or the door is closed and
   one-sided (machine flag 4, or flag 8 for player units) and the unit is in
   front of the door's marker 0x7000686 (BAD60, else the object's center and
   forward vector, BBFF0). An opened door sets its group to 1 (10C290) and its
   open ticks (`+0x1D0`) to -3.
5. A door's machine portals (`+0x1DA` first, `+0x1DC` count, the BSP's
   `+0x220`/`+0xC` portal list): each portal's bit in the portal mask
   (`[505E7C]`) follows whether the door is open (its group's value while
   `moving`, or its position), and its openness word (`[4F9B58]+0x28`) is the
   position between the closed and open openness (`+0x124`/`+0x128`), clamped
   to [0, 1] and scaled to 65535 (FISTP in the live rounding mode).
6. A fully open door (position exactly 1, flag 2 clear, not game mode 4) counts
   its open ticks and sets its group to 0 after its open time (`+0x120`).
7. A machine that moved (device flag 4, or a moved parent in the parent chain)
   notifies (BF050, then 1181F0(0) and BDA50) and returns 1; otherwise 118350
   runs and the changed byte is returned.

## Exactness and the adapters

- Memory accesses are in the original order and width, including the
  constants the original reloads into registers after child calls.
- SSE work is the original instruction with the original operands in the
  original order on the live MXCSR (asm statements; COMISS versus UCOMISS as
  the original chose them). x87 work (the |value| tests and the gear rate)
  runs on the host x87 under its live control word.
- Values in callee-saved registers across child calls are the logic's kept
  struct fields; the adapters hand back what each child returned.
- The adapters own the ABI only: per-role register effects (including
  partial-register writes and LAHF's AH), frames and slots, the XMM register
  file (loads put a scalar in lane 0; register-only arithmetic and the
  flag-deciding compares are replayed on the register copy with the MXCSR
  restored), and the x87 stack. The x87 sequences are balanced and contain no
  child call, so only C1 is published: each sequence ends with C1 clear; the
  portal loop's FISTP leaves its rounding bit. The two spills to the frame
  (FST of the rate, FLD/FISTP of an openness) are in the adapter.
- Every derived value (record addresses, cursors, stored floats, pushed
  arguments) comes from the logic; the adapters do not recompute them.

Children stay genuine: B7E50, BBFF0, BF050 (1179C0) and B8760, BF050 (10B840)
through the unit's dispatch (they have no body in the unit, so dispatch
reaches the required owner, unchanged); the others as the unit's direct bodies
(BD080 is natively bound there).

Admission: the TU1.5 image, the established cursor/census admission
(`native_object_tick_detail::admitted`), two free x87 slots without an active
MMX alias, and masked x87 and MXCSR exceptions.

Switches: `HALO2_REFERENCE_DEVICE_UPDATE=1` runs the translated bodies;
`HALO2_VERIFY_DEVICE_UPDATE=1` records each admitted native call and replays
the unmodified translated body against it (every read and store in order,
every child entry ABI, final registers, flags, XMM, x87 and MXCSR), printing
`Device update verify: ...` at exit.

## Proof

See the progress section "Device update callbacks 1179C0/10B840" in
[REMAINING_TRANSLATED_20261115.md](REMAINING_TRANSLATED_20261115.md) for the
figures and the live results.
