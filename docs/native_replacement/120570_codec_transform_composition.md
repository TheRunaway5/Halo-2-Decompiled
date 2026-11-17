# TU1.5 codec transform composition

The private `120570` kernel matches 114 original instructions and both complete
retained bodies in Primary and DeathCycle. It calls the genuine `1201E0`
quaternion product through the actual pushed return address and uses its
returned registers, SIMD and stack state. No child result is supplied.

Exact input/output equality selects one of two eight-word REP staging
loops, both using the original stack scratch. The later rotation preserves
ordered scalar operands and fresh reads around output writes. The scale
test keeps the UCOMISS, LAHF and byte TEST policy, including the unordered
branch. The final DI/SI and BP pops occur before the corresponding Y/Z
stores, and RET reads the real return address.

The physical view passed 752 unchanged-i386 paths under GCC and Clang
ASan/UBSan/LSan: 192 dense inputs, all 256 opaque MMX masks and 304
read/write/completed-observer prefixes. Both compilers covered all 114
parent and 75 child sites, 3,008 comparisons, 242,624 complete pre-I/O
packets, and 2,336 returned-child observations. Five compiled defects per
compiler were rejected. A physical before-write stop for a dual-operand
REP element remains distinct from a provider write exception after its
read has completed.

The separate retained-provider view passed 1,424 comparisons per compiler
against both complete source bodies and their real child implementations.
Each compiler checked 370 provider prefixes, 126 non-REP retirements,
and all 32 copy-specific REP retirements, including both final count-zero
callbacks. The ordered view and unchanged authored sources agreed across
all tested inputs. The deliberately unordered Clang view differed 566
times; its first divergent packet, before `12065C`, changed XMM6 from
`ffc23456` to `7fc12345`. The source bodies were not rewritten.

The proofs assume read-only GP/SIMD/FP metadata and host FP controls in the
retained provider path. They do not qualify arbitrary mutable source
callbacks, physical partial writes, other images, generator defaults,
or parent/runtime closure. Generated scratch owners and large input
banks were removed. Only bounded receipts, diagnostics and the frozen
physical proof source manifest remain.
