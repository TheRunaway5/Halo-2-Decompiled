# Native TU1.5 built-in custom constraint schemas

The native custom-schema module owns only the three authenticated retail
TU1.5 callbacks `31F3A0`, `31F410` and `31F560`. They respectively scale the
two bodies' retained damping vectors, restore those eight scalar values and
add two projected SIMD contributions to retained Jacobian biases. Their true
cdecl arguments are `(row, payload)`; these three built-ins use only payload.
Unknown registered callback addresses require a separate owner and remain
explicitly unqualified.

An independent source audit authenticates the three actual producer immediates
and calls to schema emitter `32A390`. All 34, 26 and 60 callback instructions
are accounted for. No callback reads an incoming GP/XMM value, calls another
owner or consumes an incoming live x87 value. The two floating callbacks use
at most one private x87 value and return balanced. The solver's private x87
depth is zero at the callback call site. This permits a typed native callback
without inventing implicit virtual register inputs.

The physical proof executes unchanged instructions from the pinned English
TU1.5 executable. Both GCC and Clang pass 336 cases: 245 complete calls,
55 exact read-fault prefixes, 18 rejected destination stores and 18 exceptions
delivered after an actual store. The cases include all rounding modes,
24/53/64-bit x87 precision, FTZ/DAZ, aliases, paired quiet/signaling NaNs,
signed zero, infinities and subnormals. Ordered reads/stores, retained bank
bytes and complete FP control/status are compared. Two negative controls
reject altered source operand order and a prematurely committed FSTP pop/C1.

For an FSTP destination rejection, the module restores the real pre-instruction
x87 state, including its live value and C1/TOP. Successful memory publication
commits the actual pop and rounding status before observer delivery, so an
observer exception preserves the already completed instruction.

The bounded authority, source hashes, commands and compiler reports are in
`build/test-results/native-constraint-custom-schema-final-tu15/`; the static
ABI/registration audit is in
`build/test-results/native-constraint-custom-schema-abi-tu15/`.
The final standalone proof's 6,085,709-byte fixture and all private executables,
raw banks and compiler products were removed. Its cleanup receipt confirms
that the owned scratch directory no longer exists.

Full native solver/initializer/custom-callback composition and visible
multiplayer execution are separate gates and must cite their own current
source-pinned results.
