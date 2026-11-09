# 1E5AF0 node-resource eligibility and tail transfer

Primary, AI-actor and AI-frame TU1.5 copies default to a typed native kernel.
It resolves the descriptor's node, tests its resource flag, obtains the resource
index and table in source order, and tests the resource's enable byte. Disabled
paths return directly. Enabled paths tail-call the genuine `126200` owner with
its exact input ABI. No result or continuation outcome is supplied for that
child, whose lifecycle closure remains open.

Wrapping node/resource address arithmetic is preserved. The final ECX retains
the resource table's upper 24 bits; setting CL to one preserves flags from the
actual enable byte. A sentinel index retains EAX=`FFFFFFFF`, ECX=0 and the
original low-page flag read. Five semantic read phases publish the source GP,
flags and cursor before faultable reads. The kernel owns no virtual registers
or instruction table. Complete body pins and full selected-resolver pins guard
all three bindings; original bodies remain available through
`HALO2_REFERENCE_NODE_RESOURCE` and service/trace admission.

The registered sanitizer gate passed 1,032 native transfers and the same 1,032
forced-reference transfers: per copy, 168 unchanged i386 normal returns, 112
genuine `126200` entry states and 64 before-read stops. All 19 source sites ran.
Complete GP/EFLAGS/XMM, MXCSR, x87 control/status, banks and explicit source
reads agree. Caller live 80-bit values are checked at normal/child-entry stops
and preserved by the native path. Twelve additional canonical backing/alias
cases include two backed-zero-page sentinel cases; these are not physical
normal-path claims. Sixteen body/resolver mutations and seven unsafe admissions
are rejected. An optimized hardware gate also passed.

Final receipt:
`build/test-results/native-node-resource-official-20261109/evidence/native-tu15-node-resource.json`
(3,680 bytes, SHA-256
`7c5498f7a353b028d8ab32d4b5a645c82d610b76a0fe119843b62d0cb1f9828b`).
Fresh complete primary/AI-actor/AI-frame generation, authenticated binding
checks and full C++ syntax checks passed. Required archived AI reference banks
were reconstructed only in scratch, hash-checked and removed. Original evidence
was unchanged; all probe/configure/CTest/generator trees were removed and no
large artifacts were retained.

The hardware observer does not describe RET's implicit stack read as an explicit
memory operand. Returned stack state is compared, but ordered implicit-RET read
and physical page-fault behavior are not qualified here. The real child outcome,
whole resource lifecycle, other images and whole-engine native closure remain
separate obligations. The r4 engine snapshot predates these bindings.
