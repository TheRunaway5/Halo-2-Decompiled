# 171070 fog membership

The Shadow and ParticleDraw owners now default to the same typed fog selector.
It checks actual scenario/BSP availability and the override bit, then resolves
a signed cluster designator through the optional plane palette and compares
the actual fog datum. There are no children or supplied outcomes. The public
`BspAtmosphere::contains_planar` method shares this decision core; its existing
instance recursion and host validation remain, including the policy used by
`source_bsp_cluster_contains_fog`.

The raw transport retains lazy global/plane/table reads, saved SI/BX stores,
AL-only ownership, interleaved pops and RET. Raw negative palette values other
than NONE preserve the original wrapped address calculation. The public view
preserves its existing safe vector bounds policy instead of imposing that
policy on the raw bank view.

Both original complete-body hashes remain unchanged:
`0c3b62f036e905dd2c7631554b0f539e28b31e1e7b81ffe1b3ec22aa2297ba1c`
(selected Shadow) and
`d54131935860f584d28698770ba1b5a60ec59705a23295f3d2d7fbb28b964a37`
(literal ParticleDraw). The installer requires the exact whole resolver and
dispatcher for each body. It reconstructs only the exported exact native
prefix and ordered fallback-shift spelling. Other bodies are unchanged: 146
in Shadow and 72 in ParticleDraw.

Unchanged i386 exposed a pre-existing retained shift defect: before the final
datum read, the translated flags were 0x246 while the original CPU had 0x256.
The bounded failure remains preserved. The native transport and the exact
TU1.5 fallback correction seed the original `CMP(index,-1)` followed by
`SHL ECX,4` in one assembly block and capture physical flags, including AF.
No flags are masked from comparison. Other images/backends retain their
previous fallback policy. The canonicalizer reverses only that exact spelling
before checking the unchanged original body pins.

The registered five-test GCC sanitizer/Release gate passed in 285.42 seconds;
independent installed Clang sanitizer also passed. Each hardware gate uses
1,008 original paths: 608 normal returns and 400 atomic provider/completed
observer prefixes, covering all 41 sites and making 8,152 comparisons. The
72,722 captured pre-I/O observations split into 59,954 fully compared native
packets and 12,768 retained flags-only observations. Native packets check full
GP/flags/XMM, MXCSR/x87 controls/status/live 80-bit values. Complete reference
normal returns, backing bytes and ordered accesses agree; legacy PUSH's
pre-write SP policy remains outside full retained pre-I/O GP qualification.

The corpus includes availability and sentinel branches, direct and negative
designators, signed plane words, wrapped negative/high palette addresses,
saved-stack/global aliases, 16 alignments, both DF values, dense x87 depths
0/1/4/8 and all 256 opaque MMX known masks. Five compiled mutants are rejected,
including lost physical shift AF. Raw/prepared source paths have zero retained
statements/callbacks when admitted; the positive reference path evaluates real
retained statements. Guard checks reach the untouched first retained statement.

Both complete renderer units regenerate identically and pass four standalone
raw/prepared syntax checks and repeated preparation. Actual public APIs agree
with pinned predecessor methods for 92,160 lookups, including 13,073 exact
FormatErrors under both compilers.

Admission requires TU1.5, supported dense or opaque-MMX FP state, the current
callable/cursor/type/executor/three-site protocol and inactive trace flags.
Observers require the qualified store-observation witness. Cleared/rebound or
stale witnesses, unsupported states/backends and
`HALO2_REFERENCE_FOG_MEMBERSHIP` enter the retained path. Actual signal/MMU
faults, partial providers, observer preimage failures and arbitrary GP/policy
mutation remain separate domains.

The source changes are later than frozen R18
`4a2bd5b5f5301402619cd0b1e00d788b27ce43f16d4feb01dae261e94d681394`.
Evidence is consolidated in
`build/test-results/native-fog-membership-closure-20261109/report.json`.
All owned code/bank/binary/build copies were removed; bounded reports and source
hashes remain. This proves the stated component, not the renderer parent,
other images, full native-only execution, timing or original-frame parity.
