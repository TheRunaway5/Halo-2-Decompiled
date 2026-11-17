# TU1.5 codec quaternion product

The private `1201E0` DATA kernel and raw caller view match all 75 original
instructions and both complete source copies in Primary and DeathCycle.
`120570` calls this child directly; the parent is a separate qualification.

The routine stages an input only on exact pointer equality with the output.
Each staging branch reads Z and W before publishing W, then Z, to the real
stack scratch. It preserves lazy reads through partial overlaps and ordered
SSE operands, including distinct NaN payloads. The saved SI pop, stack-release
flags and return-address read remain observable.

Both sanitizer compilers passed 584 unchanged-i386 paths: 192 dense inputs,
all 256 opaque MMX masks and 136 read/write/completed-observer prefixes.
Each compiler made 2,336 comparisons and checked 87,208 complete pre-I/O
GP, flags, SIMD, FP and live80 packets. Five compiled defects per compiler
were rejected, including operand reversal and early staging publication.

The separate retained view passed 1,166 comparisons per compiler against
both complete source bodies, including 162 provider prefixes and 108
copy-specific checks of the 54 completed-instruction successors. The 192
dense inputs and all opaque masks preserved the authored source's complete
register, memory and host-control results. Neither compiler produced a
measured difference between the authored intrinsic expressions and the
explicit original operand order in this corpus.

The retained proof assumes read-only GP/SIMD and FP metadata/control providers.
It does not qualify mutable source callbacks, physical partial stores,
other images, production admission, generator defaults, or parent/runtime
closure. Both owned probe trees and their large fixture banks were removed;
only bounded reports and source manifests remain.
