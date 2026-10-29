# Native ai_living_fraction

`ai_living_fraction` now uses the existing native27DC40 traversal with its full
signed32 living and population sums. It does not reuse the truncated SHORT
living-count result or weighted mean vitality. Population<=0 produces positive
zero; otherwise the kernel converts living first, converts population second,
and divides with actual scalar SSE under the supplied original MXCSR policy.
Direct actors still perform the original unit-vitality read and return1/1;
a missing actor returns zero. Weighted vitality operations remain required for
the original FP effects even though their value is not the fraction result.

The query enum appends `living_fraction` at5. Static assertions preserve all
existing values0..4, and the existing algorithms and canonical reference files
remain intact. Name-based builtin dispatch exposes a REAL result(type6).

The original signature is TU1.5 opcode327, definition458494, evaluator2AF700,
REAL(AI). An independently read installed MCC descriptor proves opcode329,
definition180B249C0, evaluator180786B90, with the same signature. The generic
MCC signature profile now admits that single mapping. Projection changes only
the call's opcode word; function identity, context type, literal AI reference,
datum salts, links and name nodes remain unchanged. No mission-specific script
or tag edit was made. The runtime reads neither executable for this operation.

`analysis/campaign-native-ai-fraction-20261028/proof.json` joins signature
provenance, original execution and current-source component results. The new
required reference is
`tests/fixtures/native_campaign_ai_living_fraction_tu15.json.gz` (86921bytes,
SHA256 `c41edc788915e243a18338274b343d409d7986bd0f0ffdd89665aa3dd22bfad6`).
It contains138 whole direct27DC40 population-output calls paired with the real
resumed2AF700 evaluator, plus85 independently stopped read prefixes. The whole
fixture was regenerated in verify mode and matched exactly. Original212010
argument handling and211DA0 publication execute unchanged; no dependency return
or instruction is supplied. Unicorn records source reads, while actual i386
hardware execution supplies the FP status and published results. Those statuses
differ from Unicorn in91 cases, so emulator FP is not acceptance authority.

Cases include squad/group/actor/starting-location references, missing actors,
negative/zero/positive populations, negative living values, weighted NaNs and
denormals, independent population values, counts beyond16 bits and a513-squad
numerator beyond2**24. The latter exercises actual integer-to-float rounding
under all four rounding modes. Direct-actor signaling-NaN vitality verifies that
the required MOVSS read does not become an invented arithmetic/health gate.

All961 cases across the four query fixtures pass with GCC, Clang and actual
ASan/UBSan. Both production query callers compile with GCC and Clang. Five
actual private source mutations fail: truncate the numerator, substitute mean
vitality, skip the direct vitality read, omit required weighted arithmetic, or
allow negative populations through the division gate. Existing count and both
fighting-count mutation checks also reject their altered implementations.
The MCC mapping's focused normal and sanitizer tests pass, including opcode-only
projection and name/arity/salt rejection controls; generated metadata matches
its JSON profile. The first minimal layout link lacked the production Bytes
implementation; the corrected qualification compiles actual cache.cpp rather
than substitutes. Only an unrelated concurrent audio adapter header drift was
reported by the query snapshot runs; the owned query sources remained unchanged.

This qualifies the finite native query and its ready/resumed evaluator. Full
first-dispatch HS expression evaluation and global HS FP ownership/publication
remain open. The current production caller returns the query's value but does
not publish its returned sticky FP context into the shared HS owner. This work
adds no fabricated publication and makes no full campaign/rendered/co-op claim.
All owned original ELFs, input banks, native probes, mutant files and build trees
were removed; only the required reference and compact receipts remain.
