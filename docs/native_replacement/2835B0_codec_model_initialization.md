# TU1.5 codec model initialization at 2835B0

The original 301-instruction function has exact Primary and AI source copies.
Its private initialization view covers 77 instructions: the opening weight
comparison, the real short RET28 at `283A3F`, and the live frame through the
secondary-mode branch at `283718`. The branch destinations `28371E` and
`2837C0` are stops before the next instruction. The short path completes
with the original saved EDI and EBX pops and does not push EBP.

The mode byte is read before the first push. The two later formals use their
own adjusted stack pointers. In the initialization path, the converted frame
index comes from the actual scalar spill and reread, before the signed count
load. Twenty-two bitplane words clear before the first two global stores;
the remaining two clear after the register compare that controls index
clamping. Later stack formals are read after these writes. Tests back both
forms of alias: the spill can supply the count, and the plane or decoder
globals can replace a later stack formal.

The first GCC sanitizer gate matches unchanged TU1.5 on 292 paths, 1,168
comparisons, and 34,328 complete pre-I/O packets. It includes 212 atomic
provider or completed-observer interruption prefixes, 45 genuine returns,
35 live mode-branch states, and 20 backed alias paths. Its scratch is removed.
The expanded gate on an independently copied R31 source passes both
sanitizer compilers. Each matches 724 unchanged original paths, 2,896
comparisons, and 86,168 complete pre-I/O packets. This includes 288 genuine
RET28s, 224 live mode-branch states, 212 interruption prefixes, 128 backed
alias paths, and all 256 opaque MMX masks. Both compilers reject all four
compiled defects. The immutable copy and its test outputs were removed.

A separate private view continues the two live modes to the common
`283815` join. The secondary mode retains both observable ceiling
stores where the original first stores the limit and then overrides it.
It keeps the original scalar spill/reload and ordered fractional
arithmetic. The other mode reads its stack byte again after initialization
and preserves the original floor and ceiling snaps. The first GCC gate
matches 372 original paths, 1,488 comparisons and 55,932 complete
pre-I/O packets, including 279 interrupted prefixes. The full independent
gate passes both compilers on 856 original paths, 3,424 comparisons and
127,216 full pre-I/O packets per compiler. It covers 292 interruption
prefixes, all 256 opaque MMX masks, 20 floor snaps, ten ceiling snaps and
16 late mode-byte alias paths. Four compiled defects are rejected per
compiler. Both probes were removed; the bounded combined receipt is
`build/test-results/native-codec-model-initialize-mode-private-closure-20261110/report.json`.
The composed readonly input domain reaches
63 of the 65 mode instructions: after the original clamp, a primary index
that differs from the last index cannot be negative or wrap on increment.
The negative successor store at `2837D1` and jump at `2837D7` are outside
this composed proof.

The next private section covers the 83 instructions from `283815` to
the first decoder call or its bypass. It stops before the actual CALL at
`283942` or before the bypass join at `283947`. The byte-sized model
marker is read again after weight publication. The three stream offsets
use the original signed descriptor words, track reads, and base reloads.
The corpus includes real backing aliases where publishing weight changes
the later marker and publishing the stream base changes both later
track counts. The GCC smoke passes 445 unchanged paths, 1,780 comparisons
and 82,676 complete packets, with 358 interruption prefixes and all 83
new instructions reached. The full independent GCC and Clang proof
passes 960 unchanged paths, 3,840 comparisons and 186,220 complete
packets per compiler. It covers 420 interruption prefixes, 288 genuine
short returns, 158 bypass joins, 94 stops before the actual CALL, five
late-track and five late-marker aliases, and all 256 opaque MMX masks.
Both compilers reject all four defects. The owned trees were removed;
the receipt is
`build/test-results/native-codec-model-selector-setup-private-closure-20261110/report.json`.

The first CALL composition executes `283942` and pushes the original
return address `283947`. The format selector `283570` and the reached
field selector then run on that single stack entry. The proof ends before
the selected field tail owner executes. The genuine bypass reaches
`283947` with its original stack pointer and no CALL write.

The first GCC sanitizer gate passes 526 original paths across 224 parent
and 71 selector instructions, with 18 distinct tail entries and 421
interruption prefixes. The original before-write prefix preserves the
incoming stack pointer. The completed-write prefix has the committed
return word and the advanced stack pointer. Both dense x87 and opaque
MMX inputs reach those two CALL boundaries.

The frozen GCC and Clang sanitizer runs each pass 4,232 comparisons and
230,640 complete packets across 1,058 original paths. The paths include
446 interruptions, all 256 opaque MMX masks, 166 stops across the 18 tail
entries, 158 genuine bypasses, and 288 short returns. Both compilers
reject all five defects. The owned scratch trees were removed. The
receipt is
`build/test-results/native-codec-model-first-selector-private-closure-20261110/report.json`.

The selected `284D30` decoder has 141 original instructions. Its complete
retained body is also the byte-identical fall-through suffix of the
Campaign `2849F0` owner. The private decoder executes three bitmap lanes
and calls the row functions selected by the actual table at `48C33C`.
Row 8 uses `2963D0`, `296410` and `2CEC60`. Row 1 uses `296180`, `2961F0`
and `296220`. Each original child returns before the parent reloads its
lane counter and output cursor. The direct decoder tests and the
`2835B0` calls both observe the real continuations at `284DAD`, `284E2D`
and `284EAD`.

GCC and Clang each match 928 row-8 paths, including all 256 opaque MMX
masks, and 172 dense row-1 paths. They reject all five deliberate defects
in each row family. The row-1 comparison projects two retained vector
halves onto each completed original `MOVAPS`. The full receipt is
`build/test-results/native-codec-sparse-decode-private-closure-20261110/report.json`.

The zero-secondary route continues after the genuine first decoder RET
at `283947`. It reads the current descriptor and its live length, then
pops EBP, EDI and EBX and executes the parent `RET 0x28` when the length
equals EBX. In 29 backed original cases a real `296410` translation store
clears the later length word before the parent reads it.

Both sanitizer compilers pass 853 row-8 paths, including 265 complete
parent returns and all 256 opaque MMX masks, and 108 dense row-1 paths,
including eight complete parent returns. The runs cover 588 row-8 and
100 row-1 interruption prefixes, match 569,572 and 67,356 complete
pre-I/O packets per compiler respectively, and reject four compiled
defects in each family. The earlier direct decoder gate admitted an
early stack-pointer publication defect because the comparator discarded
the duplicate provider packet for an indirect CALL. The corrected
comparator first checks equality of both packets before projecting them
to the one original instruction packet. The bounded zero-secondary
receipt is
`build/test-results/native-codec-parent-ret28-private-closure-20261110/report.json`.

The nonzero descriptor route calls `283E10` at `28396C`. The original
empty and signed-negative record paths return AL=0 through distinct
epilogues and continue at `283971`. The parent then publishes all three
second stream pointers, calls `283570` again at `283A37`, and reaches the
real `284D30` decoder a second time. The complete runs return from that
decoder at `283A3C` and execute the parent `RET 0x28`.

Both sanitizer compilers pass 1,042 row-8 paths with 264 whole parent
returns and 163 dense row-1 paths with eight whole parent returns. Each
compiler matches 1,097,928 and 146,248 full pre-I/O packets respectively
across 77 parent continuation sites. The corpus independently stops
after each transform epilogue and after each of the three real child
calls on both decoder passes. The four compiled defects are rejected in
each compiler and row family. The original row-8 validator failed to
recognize the wrong-table mutant's explicit child-target refusal. The
retry accepted that exact diagnostic and removed its owned scratch. The
receipt is
`build/test-results/native-codec-model-secondary-final-20261110/closure.json`.

An admitted packed transform returns AL=1 at the genuine `283971`
continuation. The original then writes format four at `283975`, pushes
`283A3C` at the second `283A37` CALL and selects the `284290` field
routine. Its jump at `2842BE` reaches the actual `293290` entry with the
CALL's return slot still live. The 221-site decoder owns that frame
after the jump. This proof ends before its first instruction and records
zero parent returns.

GCC and Clang each match 340 unchanged original paths, 1,360 comparisons
and 545,340 full pre-I/O packets. The corpus covers 64 tail stops, 276
interruption prefixes, all 74 parent continuation sites through the
second selector CALL and 178 sites in the real transform. It includes
separate AL=1 return stops with and without an ancestry parent, plus the
before-write and completed-write boundaries at the second CALL. Each
compiler rejects three defects. The initial fixture fault at `283E8F`
came from inconsistent root data between the stack backing and the
observer's argument metadata. The corrected fixture supplies the same
root in both places. The owned trees were removed. The receipt is
`build/test-results/native-codec-model-format-four-20261110/closure.json`.

The private `293290` decoder reads each bitmap and calls the genuine
row-8 routines `2963D0`, `296410` and `2CEC60`. It resumes after each
child before reading that lane's bitplane and output cursor. The three
translation instructions at `293496`, `2934A3` and `2934B1` all write
`[EAX+0x10]`. Each publication accumulates the next authored scalar
addition at that same address. The rotation phase retains both live
sign-vector reads and the scale phase uses the original scalar product.

Primary and CombatAction each retain the complete 221-site decoder. The
CombatAction body's 127 selected operands resolve to their literal values
in TU1.5. Applying those substitutions reproduces the Primary body
byte-for-byte. That check is a source audit, with the runtime comparison
below anchored to the unchanged executable. The audit is
`build/test-results/native-codec-format-four-decode-20261111/source-owner-audit.json`.

The independent decoder gate covers all 221 decoder sites and 45 child
sites. Both sanitizer compilers match 688 physical paths, 2,752
comparisons and 519,848 complete pre-I/O packets each. The corpus includes
320 actual decoder returns, 368 interruption prefixes, all 256 opaque
MMX masks, six independent child-return stops, and 555 three-write
translation groups. Each compiler rejects five defects. The first build
failed on an ambiguous C++ `decode` lookup. The private frame now calls
the typed kernel explicitly. The proof uses aligned atomic backing,
and its owned trees were removed. The receipt is
`build/test-results/native-codec-format-four-decode-20261111/direct-closure.json`.

A separate AL=1 composition runs `293290` on the second selector's live
frame. Its real RET consumes the saved `283A3C` word. The resumed parent
then reads its three saved registers and performs the real `RET 0x28`.
The corpus independently stops at `283A3C` after the decoder consumes its
return word and at each of the decoder's three genuine row-child returns.

Each sanitizer compiler matches 484 original paths, 1,936 comparisons
and 997,868 complete pre-I/O packets. The corpus includes 64 whole parent
returns, 420 interruption prefixes and 32 repeated translation groups.
It reaches 209 decoder sites in composition. The direct gate covers the
complete 221-site decoder. Both compilers reject defects that skip the
decoder return or the parent's epilogue. The prior stop-at-entry mode
also passes its Clang regression. The owned trees were removed. The
composition receipt is
`build/test-results/native-codec-format-four-decode-20261111/parent-closure.json`.

The private `293290` source view keeps the authored `284290` caller and
its owning dispatcher. Primary runs `2963D0` and `296410` locally and
delegates `2CEC60`. CombatAction delegates all three. The source view
uses the retained stack, scalar and vector memory helpers and resumes an
exact retained suffix after a completed instruction revokes the entry
policy. The 92 suffix labels each sit immediately before the actual next
instruction. Raw source with an active instruction callback takes the
retained body. Prepared source requires the live quiet cursor protocol.

The legacy GCC source could commute the two NaN operands at `293391`.
An unchanged TU1.5 execution through `284290` records XMM3 lane 0 as
`FFC23456` immediately before `29339E`. The unmodified retained GCC body
recorded `7FD5815E` on the same input. The staged fallback and native
source view use the authored operand order at all eleven decoder SSE
arithmetic instructions. Both recorded bit patterns are preserved in
`build/test-results/native-codec-format-four-source-20261111/`.

Both owning generators reproduce the proposed overlay in disposable
copies. Raw and prepared output reverse to the complete unbound source,
and preparing each current and fresh owner a second time produces
identical bytes. A CMake check rebuilds both aliases after a change to
the isolated binder and confirms the prepared outputs are identical.
The unapplied patch and its exact source preimages are under
`build/test-results/native-codec-format-four-source-20261111/staged-wiring-r5/`.

The frozen GCC and Clang sanitizer runs each pass 18,432 comparisons
across the eight current and fresh raw and prepared source contexts.
Each default run enters the native decoder 3,328 times, observes 14,964
real native child calls and resumes 692 exact retained suffixes. The
own-reference control enters the native decoder zero times. The
unrelated control reproduces the default counts. Each context passes
complete translation unit syntax and rejects 102 source mutations.

Both compilers reject all four compiled defects. The first GCC harness
could not classify a swallowed foreign exception because the erroneous
resume reached an invalid bank. The retry rejects the first illicit
retained callback and retains the original failure receipt. Both frozen
copies have the same decoder implementation and binding helper. Their
owned scratch trees were removed. The source capsule receipt is
`build/test-results/native-codec-format-four-source-20261111/source-capsule-closure.json`.

The prepared Release build links both complete `284290` exports
against the actual world runtime and calls the exported row-eight
children when the owner delegates them. Default, own-reference and
unrelated-reference runs each exercise 288 cases, 96 real exceptions,
1,080 external child returns and 294 repeated translation store groups.
The three complete ABI, I/O, observer and bank streams are identical.
The default and unrelated controls each enter the native decoder 288
times. The own-reference control enters it zero times. The streams and
owned build were removed. The bounded linked receipt is under
`build/test-results/native-codec-format-four-source-20261111/current-linked-stage/`.

The latest private rebase takes the exact eleven-file BCB70 insertion
over the qualified linked cohort. Both genuine generators reproduce the
staged decoder with whole-source reversal and repeated preparation in
every current and fresh raw and prepared context. The independent
Primary inverses for `1046C0`, `C9070`, `BCB70` and `107140` compose
with the decoder inverse. The rebased patch remains unapplied under
`build/test-results/native-codec-format-four-source-20261111/current-bcb70-rebase-r2/`.
