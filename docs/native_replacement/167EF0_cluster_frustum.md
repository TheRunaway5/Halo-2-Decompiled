# 167EF0 cluster frustum aggregation

Actual TU1.5 `167EF0` now defaults to a typed group/range loop in
`native_tu15_cluster_frustum_kernel.hpp`. The public
`source_cluster_frustum_visibility` facade uses that same loop. The raw bridge
calls the genuine, separately qualified `184690` sphere classifier; it supplies
no classification outcomes.

The sole production owner is the ParticleDraw generated unit. Its complete
original body SHA remains
`dd41d7611ab271a7e94ffa248c759b2917facd041be177cc61de9b43607023ce`.
The binder also pins the entire selected resolver and dispatcher and requires
the complete native Sphere owner. The original 47 instruction sites remain as
the reference path, controlled by `HALO2_REFERENCE_CLUSTER_FRUSTUM`.

The ABI takes the cluster-entry ordinal in EAX, the set in EDI, radius in XMM7,
and center/output pointers in the two stack arguments. It returns visibility
in AL and stores containment as one byte. The transport retains the original
saved-register and argument stores, lazy count/index/output rereads, child ABI,
AL-only return and RET8 behavior. Later groups still acquire their own counts
after a prior group establishes containment.

Admission requires TU1.5, dense masked x87 state without active MMX, and either
absent instruction callbacks or the live exact three-service cursor protocol.
Traces, cleared/rebound/invalid cursor witnesses and unqualified store observers
fall through to the retained body. Unsupported raw backends are excluded before
their headers are included. Arbitrary GP or policy mutation during a provider
call is outside the qualified provider contract.

The registered GCC sanitizer/release gate passed all eight tests, including the
three existing Sphere tests. Each parent run used 1,032 unchanged-i386 paths:
288 normal cases and 744 atomic provider/completed-observer prefixes, covering
all 47 parent sites. Each configuration made 5,340 comparisons and captured
638,835 pre-I/O packets across native and reference routes. Native packet
comparisons check all GP registers, flags, XMM lanes, MXCSR, x87 controls,
status and live 80-bit values. The total also includes reference observations
whose pre-I/O assertions check flags only, as distinguished below. Complete
normal returns, backing bytes and ordered reads/stores agree.
The independent installed Clang sanitizer gate passed the same corpus.

Coverage includes all 16 alignments, all x87 TOP positions, live depths
0/1/4/8, DF, negative/empty ranges, six groups, source index wrapping, NaNs,
and output aliases with counts, indices and saved stack words. Four compiled
semantic mutants were rejected. Raw/prepared actual source copies execute zero
retained statements on admitted paths; forced reference executes real retained
statements and returns the same complete normal state. Forty-eight live-policy
checks and two reference controls reach the untouched first retained statement
before source I/O or ABI changes.

Reference normal returns and ordered memory accesses are compared with the
original CPU. Its pre-I/O flags are also compared. Full reference pre-I/O GP
qualification is separate because legacy `State::push` publishes decremented
SP before its write callback; the native bridge stages SP atomically and uses
the original CPU as its authority. Actual MMU faults, partial provider writes
and observer preimage-read faults are outside these atomic-prefix checks.

The public DATA regression passed 512 cases, including 86 genuine format
errors, with identical FP effects. Full particle regeneration is byte-identical
and both raw/prepared units pass standalone syntax checks. The independent
Sphere facade and all other generated bodies are preserved.

Evidence is consolidated at
`build/test-results/native-cluster-frustum-closure-20261109/report.json`.
All owned scratch/build trees were removed; no binaries or raw banks are
retained. This change is later than frozen R17. Historical R14 counted 4,066,280
literal observations in this owner; those diagnostic counts imply no measured
timing gain. The `135970`/`136EA0` caller subtree and full production migration
remain open.
