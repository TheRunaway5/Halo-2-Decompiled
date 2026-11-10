# 136E10 visibility associations

Shadow and MapWorld now default to one typed association allocator. It reserves
the current unsigned 16-bit index, saturates at 512, clears the four mask words,
then marks the caller's signed cluster ordinal. The two public BSP/object
admission allocation blocks share this DATA algorithm and retain their existing
validation, queue policy, duplicate handling and error behavior. There are no
children or supplied outcomes.

The raw view retains the intermediate 16-bit count publication, reverse mask
clear order, delayed ordinal read, wrapped negative bit-word addresses, saved
SI/DI stores, DI pop before the membership store and RET4. Host ordinals remain
validated below 128; that policy is not imposed on raw banks. Raw counts above
511 still make the actual temporary low-word publication before saturation.

The original selected complete-body pin remains
`dca96b0bb38177d553c8e2fa9e4810925db950704beed846367adc81207d623a`.
The complete literal identity projection is separately pinned as
`5ea1d1ac4638934c2bbeaf0ac8c2f8953f961acbba19c80990894a760342f1bc`.
Both whole canonical resolver/dispatcher pairs and the exact fresh MapWorld
literal context are required. Only the exported exact prefix and two known
ordered shift spellings are reversed before checking these original pins.
All other 146 Shadow and 192 MapWorld bodies were preserved during installation.
MapWorld's checked-in helper is called locally and has no public dispatch case;
its source-qualified qualification does not create a missing public route.

An unchanged-i386 comparison exposed a pre-existing retained AF mismatch before
the fourth mask clear (0x206 versus 0x216). The bounded failure is preserved.
The native view and exact supported TU1.5 fallback helpers seed the original
flag producers and capture physical shift flags, including undefined AF.
No flags are masked from comparison. Other images/backends preserve their
existing retained policy.

The registered five-test GCC sanitizer/Release gate passed in 272.37 seconds;
independent installed Clang sanitizer passed too. Each raw gate uses 982
original paths: 576 normal returns and 406 atomic provider/completed-observer
prefixes, all 40 sites and 7,734 comparisons. There are 62,448 fully compared
native pre-I/O packets plus 13,056 retained flags-only packets. Full native
GP/flags/XMM/FP/live80 values, backing bytes and ordered accesses agree.
Reference full returns/accesses agree; the old translated PUSH pre-write SP
policy remains outside full retained pre-I/O GP qualification.

The corpus includes counter overflow/saturation, negative/high ordinals, saved
register/argument/counter/mask aliases, all 16 alignments, both DF values, dense
x87 depths 0/1/4/8 and every opaque-MMX known mask. Five compiled semantic
mutants are rejected. Native source routes execute zero translated statements
and instruction callbacks; positive reference routes execute 33,408 retained
statements. Refused guards reach the untouched first retained statement.

Actual public BSP and object-list admission exports match their pinned whole
predecessors in 1,332 comparisons per compiler, including 364 exact errors and
42 explicit saturation witnesses under ASan/UBSan/LSan. The shared object
template also serves the pool overload; its pool ownership is not independently
requalified by this list-export corpus.

Fresh Shadow regenerates byte-identically (147 functions). Fresh MapWorld
retains all 193 function identities and the exact association literal
projection. Its 36 unrelated historical body differences are recorded and are
not normalized or qualified here. Both complete fresh units pass raw/prepared
standalone syntax, exact prepared association pins and repeated specialization.
Six fresh body/resolver/dispatcher mutations are rejected; seven binder tests
also preserve the full public-owner pins.

Admission requires TU1.5, a supported dense or opaque-MMX state, the current
live callable/cursor/type/executor/exact three-site protocol and inactive trace
flags. Observers require the qualified store witness. Stale/cleared/rebound
witnesses, unsupported states/backends and
`HALO2_REFERENCE_VISIBILITY_ASSOCIATION` retain the original path. Actual MMU
or signal faults, partial providers, observer-preimage faults and arbitrary
mid-call GP/policy mutations remain separate domains.

This change is later than frozen R18
`4a2bd5b5f5301402619cd0b1e00d788b27ce43f16d4feb01dae261e94d681394`.
The bounded receipt is
`build/test-results/native-visibility-association-closure-20261109/report.json`.
All owned code/bank/binary/build copies were removed; no large artifacts remain.
Renderer parents 135970/136EA0 and other image variants remain open. No timing,
whole-game parity or global native-only execution claim follows from this gate.
