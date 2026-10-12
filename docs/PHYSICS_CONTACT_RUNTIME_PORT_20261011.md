# Original contact call state and packet generation

The native contact runtime now generates the two retained canister contact
packets through the complete original call chain. Every observed instruction,
ordered store, byte of retained memory, general register and SSE lane agrees
with the original executable. The cold empty-contact call returns completely.
The positive call stops at the actual `31F21B -> 313800` world-manager owner
(`317350` in TU1.5), after generating and retaining its packets.

This is a verified physics component. The live multiplayer mover has not been
replaced, the existing launcher has not been republished, and the whole physics
engine or multiplayer port is not complete.

## Why a full call state was necessary

The earlier semantic contact dispatcher stopped before `300400`: its 128-byte
packet includes saved registers, return addresses and temporary stack values
written by preceding callees. Replacing those bytes with recorded packets or
zeroes would give the wrong owner and lifetime to that data.

`source_run_physics_contact_runtime` compiles 70 complete authenticated function
bodies, totaling 8,386 original instructions. Its native functions share the
actual caller's general registers, SSE lanes, x87 stack, flags, FS base and guest
memory. They preserve the preceding shape bounds, child cache, BSP queries,
child-shape construction, narrowphase, GJK, allocation and packet-copy stores.
There is no production opcode fetch, decoder, emulator, executable execution,
contact-result replay or supplied successful world-manager result.

The old semantic API retains its required packet-frame boundary because its
caller does not provide the complete original call state. The new API requires
that state explicitly; it does not invent registers or restart the already
mutated old call. Unknown callees receive the real pushed return address and
full retained call state, or stop if no real owner is installed.

## Original evidence and scope

Both locally supplied executables are hash pinned:

- Map Pack 1.1: `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
- TU1.5: `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

The 70 complete paired bodies include branch topology, internal jump tables,
absolute operands and direct targets. Seven additional complete paired bodies
(878 instructions) authenticate the allocator initializer, its object/arena,
and the BSP/object vtable locations needed to bind retained inputs to TU1.5.
Virtual slots used by the tested allocator and BSP paths are checked against
the paired native entries.

The 1.1 inputs come from the unchanged original retained factory, player spawn
and world schedule. The previous complete batch measurement remains identical;
no instruction, contact, allocation result or packet is patched. Changed native
files in the shared checkout do not execute in this measurement. Historical
native-source drift is reported separately from authenticated original inputs.

The TU1.5 test deliberately rebinds those raw 1.1 input records using the
verified operand, initializer and vtable anchors, then executes the untouched
TU1.5 functions. This tests the paired component binding. It does not establish
that the entire TU1.5 world, map loader or caller has executed. A separate 1.1
re-execution reproduces the captured interval exactly.

## Validation

Four native component cases cover the positive canister and empty-contact
calls in both images: four packet copies, 29,888 observed instructions, 5,372
ordered writes and 11,154 retained bytes. They exercise 3,768 distinct original
instruction sites out of the 8,386 lowered sites. ABI stack and FS writes are
included; no store bytes are masked. Tests also compare general registers,
all SSE lanes, defined integer flags, x87 depth and comparison conditions.
Unused host/emulator x87 exception status bits are not asserted as hardware
parity.

Absent reciprocal-square-root and world-manager owners must stop with the
identical preceding instruction/store/memory state. The four CPU estimate
observations are explicitly Unicorn test inputs. There is no fallback CPU
estimate, and these observations do not establish Xbox CPU accuracy.

The isolated v6 source derives from the pinned v5 snapshot plus the listed
owned overlays. Release and AddressSanitizer/UndefinedBehaviorSanitizer checks
include these component cases and the preceding world/motion/Jacobian/solver
checks. The source manifest and validation report record the exact commands,
fixture hashes, repeat hashes, logs and compiled binaries.

Evidence: `analysis/multiplayer/sandbox-source/physics-contact-runtime-20261011/`.
These ignored local fixtures contain original input/output evidence and do not
belong in the code repository. Existing local `assets/` and `analysis/` folders
are inside this working checkout and ignored; physical relocation of every
original asset outside the checkout remains separate unfinished work.

## Remaining integration

The next source owner is world-manager `313800`, followed by constraint
membership, deferred mutations and the remaining live allocator/controller
ownership. The full contact call must then connect to a source-owned world
scheduler with its real caller ABI. Alternate body/motion classes, vehicle and
ragdoll constraints, hardware-verified CPU estimates, and all-map split-screen
and LAN gameplay validation remain required. A component test does not prove
that the reported Lockout gravity lift or other live sandbox failures are fixed.
