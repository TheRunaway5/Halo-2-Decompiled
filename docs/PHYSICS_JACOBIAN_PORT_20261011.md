# Native original constraint production

The native constraint step can now invoke the three verified virtual Jacobian
producers and pass their actual output into the compiled solver. The playable
multiplayer world still needs the remaining contact/world ownership and live
integration gates. This extension does not establish a working Lockout lift,
vehicle controller, ragdoll or complete physics engine.

## Source and implementation

Complete-body correspondence authenticates 36 routines against the owned map
pack 1.1 executable and supplied TU1.5 executable. The translated runtime has
3,808 original instructions across 16 complete entries, extending the previous
five solver owners with:

- `31BBE0`, `31BD00`, `322380`: the actual verified virtual producers.
- `325BB0`: complete point-record Jacobian/schema generation, including coupled
  rows, friction-basis selection, radius handling and both accumulator transforms.
- `324D00`, `325B70`, `326820`: actual header and callback schema emission.
- `31B810`, `31B880`, `31B9D0`: actual mass-scaling/restoration and row callbacks.
- `1D2D70`: property lookup for the producer's `2003` record.

`tools/generate_physics_constraint_runtime_20261011.py` generates direct C++
branches, integer operations, SSE intrinsics, x87 arithmetic and bank writes.
It rejects unsupported operations during generation. Production contains no
original executable code, opcode fetch, instruction decoder or emulator.
The generated solver invokes these three known callback pointers through
statically bound native functions sharing its ABI, SIMD and logical x87 state.
Unknown callbacks, alternate initialization owners and unowned CPU estimates
remain explicit required dependencies.

`SourcePhysicsConstraintStepServices::jacobians` binds the verified virtual
entries in the original `322690` wrapper. It preserves the actual guest return
address and nonvolatile registers for each producer. Unknown vtable entries
retain their required-owner boundary. `solver` connects the existing complete
solver, so the chain produces, solves and reads back real output rather than
supplied schemas or solved-body values. The wrapper binding currently targets
the 1.1 mutable layout; the producer/solver bodies themselves bind both images.

`SourcePhysicsSolverServices::reciprocal_sqrt` is a required platform estimate
provider for the two original producer RSQRTSS sites. Emulator observations
are used only in differential tests. They are not a production table or proof
of Xbox hardware estimates. Strict floating-point compilation and the existing
supported GCC/Clang x86 x87 backend remain required.

## Differential scope

`tools/oracle_physics_constraint_runtime_20261011.py` executes untouched
original instructions. Its 156 controls per image exercise every new complete
entry, zero/one/two/four point counts, accumulator transform flags, coupled
rows, friction-basis and radius branches, property lookup and four rounding
modes. Original constructor/preparation owners create the accumulators; raw
contact records are explicitly diagnostic inputs, not gameplay captures.

Each image compares 91,049 instruction observations, 16,235 ordered stores and
20,451,296 retained state bytes, including the entire guest ABI stack. All CPU
estimate inputs and their sites must match, and all observed estimates must be
consumed. Removing the required estimate provider must stop at the first
original RSQRTSS site with the same preceding instruction/store sequence.

Defined EAX results are compared for `326820` (payload pointer) and `1D2D70`
(property lookup), where actual callers consume them. Other producers/callbacks
are invoked for their bank outputs and their callers discard EAX. An initial
diagnostic found that Unicorn and the native x87 FPU differ in low exception
status bits left in discarded EAX: the zero-point control returned `FFFF7D04`
versus `FFFF7D05`, and a positive-point control returned `7800` versus `7820`.
This discrepancy is retained as a validation limit; unused register equality
and Xbox FPU exception behavior are not claimed. Every ordered store and full
bank, including zero-point controls, remains compared without masking.

`tools/oracle_physics_constraint_produced_20261011.py` executes the whole
original island wrapper with two dynamic bodies, each actual producer,
one/two/four raw points and four raw record variants through all four scratch
allocation branches. The 144 cases supply no existing schema, generated
Jacobian or solver result. They exercise actual initialization, iterative
solving, known callbacks, motion readback, profiling and scratch release.
The checks compare all persistent writes/banks and the complete translated
producer/solver/callback instruction sequence. The handwritten outer wrapper's
unused ABI stack and full instruction sequence are outside this chain check;
the separate component tests retain full guest ABI stacks.

Evidence is retained under
`analysis/multiplayer/sandbox-source/physics-constraint-runtime-20261011/`.
`correspondence-verified/complete-report.json`, `generated-manifest-v1.json`,
the original-only fixtures, independent repeat fixtures and build/test logs
record provenance. The isolated v5 build derives from immutable v4 with owned
overlays only. Original binaries and extracted banks remain external to the
versioned source deliverable.

All 12 selected suites pass in both the isolated release and fully
AddressSanitizer/UndefinedBehaviorSanitizer builds, without skips. The new
comparisons total 456 cases, 88,997 ordered writes and 50,359,274 retained state
bytes. The earlier solver/world/motion suites also pass against the extended
runtime. Independent original-only reruns are byte-identical. The pinned
`validation.json` records the final source manifest, binaries, fixture and log
hashes; it keeps the immutable v4 provenance separate from this v5 extension.

## Remaining integration gates

Contact/manifold creation from the current broadphase and narrowphase still
needs the retained `300400` packet-frame owner and its real lower dependencies.
Deferred world mutation/contact delivery, live allocators and controller
ownership, alternate motion/body classes, vehicle/ragdoll constraints, complete
TU1.5 world bindings and production Xbox-compatible estimate behavior remain
unverified. The live `Simulation::move_unit` host mover is not replaced by this
extension. Complete all-map split-screen and native LAN gameplay validation is
still required before declaring the multiplayer goal finished or publishing
these components as a gameplay fix.
