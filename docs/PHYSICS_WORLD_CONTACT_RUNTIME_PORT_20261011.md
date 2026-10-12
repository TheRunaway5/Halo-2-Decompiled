# Contact insertion into the original physics world

This extends the sealed v6 contact runtime. It is a component port, not a claim
that multiplayer, the whole physics engine, or the published launcher is done.
The live `Simulation::move_unit` path still uses the host `CollisionWorld` mover.

## Original authority and native ownership

The authenticated Map Pack 1.1 executable SHA-256 is
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`;
the user-owned TU 1.5 executable SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

`research_physics_world_contact_runtime_20261011.py` independently checks the
complete bodies against both images, using the previously pinned source owners.
The added contact-point owners are bound from their actual paired direct callers.
The generated runtime now contains 82 complete native bodies and 8,926 lowered
instructions. Twelve bodies were added beyond v6:

- `313800`, `31D2B0`, `322320`, `31C520`, `31E7D0`: world/island selection,
  constraint insertion, requirements, accumulation, and listeners.
- `3136B0`, `3131C0`: deferred merge records and island activation.
- `2DF710`, `2DF780`, `2DF360`: original bank growth and byte/word copying.
- `322410`, `3223B0`: contact-point insertion and contact identifier allocation.

All calls share their actual GP, SSE, x87, flags, FS and stack state. The generated
code compiles directly to arithmetic, memory operations, branches and native
calls. Production does not decode instructions, execute the original binary, or
replay expected packets. Missing owners still stop with the actual call state.
Original CPU estimates remain required services; emulator estimates used by
fixtures are not Xbox hardware proof.

## Floating-point caller state

Physical x87 TOP is now retained independently from logical stack depth. Before
this change an empty stack implicitly reset TOP to zero. The contact runtime's
private stack now preserves a caller's actual physical index while pushing and
popping logical values. The ABI explicitly represents contiguous active logical
ST values. Sparse tag stacks and stack faults remain unsupported owners.

Each executable is checked at all eight TOP values with depths zero, one and
three, for both the positive contact call and the empty call: 48 declared input
perturbations per image. Their expected branches, stores, retained memory and
active 80-bit values come only from untouched original execution. These are FP
state stress tests, not 96 additional gameplay scenarios. Comparison flags and
TOP are checked; this does not prove every unused x87 exception/status bit.

## Retained schedule and TU input binding

The original-only oracle reruns the unchanged retained factory/player/world
schedule and requires the previous complete batch measurement to stay identical.
For the positive contact interval the native implementation now passes world
insertion, requirements and point-bank/identifier updates. Its next required
owner is listener notification `31E8B0` (TU `322440`). The empty interval returns.

TU inputs are an explicit rebinding of those 1.1 raw retained records, not a
complete TU 1.5 world/caller execution. Twelve complete initializer/constructor
bodies (1,043 instructions) authenticate mutable object/arena and vtable anchors.
Eleven actual virtual slots are checked, including island requirements and the
body point-velocity methods. Early draft inputs with stale body vtable addresses
are retained as drafts and excluded from final validation.

Only first observed input and first-before-write bytes populate initial banks.
Outputs are never inserted as initial inputs. All ordered writes, including stack
and FS writes, and all retained bytes are compared without masking. The old v6
fixtures retain their exact prefix through a test-only instruction observer; the
expanded fixtures run to the actual next unimplemented owner.

## Reproducible cohort

`analysis/multiplayer/integration/physics-engine-20261010/source-v7` derives from
immutable v6 with only the listed owned overlays. It inherits the v6 top-level
CMake file unchanged. No concurrent frontend, shaders, audio or gameplay edits
are imported into this cohort. The source manifest, complete correspondence,
repeat outputs, generated manifest, original fixtures, binaries, compiler flags
and Release/ASan/UBSan logs are checked by
`tools/validate_physics_world_contact_runtime_20261011.py`.

The evidence directory is
`analysis/multiplayer/sandbox-source/physics-world-contact-runtime-20261011`.
The original assets and extracted analysis data are ignored local inputs in this
working tree; they have not yet all been physically relocated outside it. This
cohort excludes those inputs from the code snapshot and links local analysis for
tests. The active multiplayer goal still includes asset-location/import work.

Remaining work includes the listener/game callback chain, live world scheduling
and allocator/controller ownership, alternate motions and vehicle/ragdoll
constraints, actual live mover integration, and all-map split-screen/LAN checks.
No updated launcher is published for these component-only changes.
