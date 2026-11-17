# TU1.5 codec transform chain

`283E10` has 196 original instructions and two complete source copies in
Primary and AIFrame. It calls `120570` at `283EC7` and `283F76`, and calls
`120740` at `283EE4`. The private proof separates the ancestry prefix,
the packed decode and returned second child, and the later output publication.

The ancestry prefix preserves the signed row and optional remap tests, the
actual REP stage, and the repeated transform calls. Empty and rejected rows
take their original RET20 paths. An admitted row stops at `283EE9` after the
real inverse returns. A backed reverse-REP alias makes the original `120570`
restore a different BP and SI; its parent consumes those actual values in
the later root and ancestry reads. No child result is supplied.

The frozen ancestry proof passed 1,004 unchanged-i386 paths with both
sanitizer compilers: 208 dense, all 256 opaque MMX masks and 540 interruption
prefixes. Each compiler covered 94 parent and 270 descendant instructions,
4,016 comparisons, 347,608 complete pre-I/O packets, 32 genuine changed
BP/SI returns, and five rejected compiled defects. The earlier observer
omitted the actual `283E6B` memory TEST because Capstone marked no access;
the correction describes only that original operand. An expanded live run
passed its numerical checks but was rejected because the imported common
generator changed while it ran. The frozen result supplies acceptance.

The second private view begins at `283EE9`, reuses the packed norm policy,
and stops after the real second `120570` return, before `283F7B`. It
preserves both original upper XMM pairs through the retirement of EMMS at
`283F1B`, then applies MOVLHPS, the native RSQRTPS, and the aligned store.
The expanded frozen proof passed 516 paths with both sanitizer compilers,
including 112 dense inputs, all 256 opaque masks, 92 exact post-EMMS
witnesses and 56 provider/observer prefixes. Each compiler covered all 42
packed-phase sites, checked 2,064 comparisons and 573,084 complete packets,
observed 3,232 genuine child returns, and rejected five compiled defects.
The first expanded mutant fixture used an all-zero upper-pair witness that
could not detect lost lanes. Its separate failed receipt remains, and the
retry selects a discriminating packet from actual original execution.

The complete raw view also qualifies the final 60 output sites. It
reacquires the signed output ancestry and each of the three source flag
bytes, then publishes the success marker after the bitplanes. Both
sanitizer compilers passed 582 unchanged-i386 paths, covering all 196
parent and 270 genuine descendant instructions. Each checked 2,328
comparisons, 538,768 complete pre-I/O packets and five compiled defects.
The original corpus includes 28 double-decode rows, 28 cases where REP
overwrites the row flag, and 56 where an earlier bitplane changes the flag
read by the next plane.

Raw OR flags retire with a successful store, and the raw REP observer sees
the updated cursors. The authored retained helper instead computes OR
flags before its preimage/write and advances REP cursors after its
observer. A separate source suffix comparison starts at authentic
`283F7B` states after the original child returns. The frozen literal-source
gate compares 176 such inputs against the exact Primary and AI suffixes
through the transfer before `28404A`. Both sanitizer compilers pass 1,536
comparisons, including 74 provider prefixes, 38 ordinary and 16 REP
retirements, and 810 observer feedback cases. Four compiled defects are
rejected by each compiler. The raw before-write test does not stand in for
a provider exception after a combined instruction's successful read.

Operand preparation retags the braced outer REP callback. The authored
callback after each iteration remains direct, including after the final
iteration decrements ECX to zero. The final source suffix test covers the
exact raw and prepared Primary and AI bodies, with prepared operands proven
to be identity selections. Each sanitizer compiler passes 5,344
comparisons on 176 actual input states, with 148 provider prefixes, 76
ordinary and 32 REP retirements, 160 pure numeric callback stops, and 350
quiet prepared REP witnesses. The test also observes 1,400 live trace or
callback changes and 1,620 store-observer changes. Six compiled defects
are rejected by each compiler, including changing the trailing call to
prepared delivery and dropping a pure numeric callback.

The final immutable cohort also reruns the complete physical comparison
after the source callback changes. Both compilers match all 582 original
paths, all 196 parent and 270 genuine descendant sites, and 538,768
complete pre-I/O packets per compiler. Each rejects the five raw defects.
The consolidated receipt is
`build/test-results/native-codec-transform-chain-private-closure-20261110/report.json`.

These proofs do not qualify intermediate MMX payloads during the pure
decode interval, arbitrary mutating providers, physical partial writes,
a source/default binding, or engine delivery. Large generated banks and
completed probe owners were removed; only bounded reports and frozen
source manifests remain.
