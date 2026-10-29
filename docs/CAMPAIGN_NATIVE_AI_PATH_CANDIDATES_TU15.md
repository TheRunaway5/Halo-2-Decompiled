# Native TU1.5 path-node lookup and segment candidates

`src/campaign/ai/native_campaign_ai_path_candidates_tu15.cpp` implements the
call-free27C410 and27C450 owners needed by actual27A460 path-cost selection.
The public header and `cmake/CampaignNativeAiPathCandidates.cmake` expose typed
retained-data APIs. No CPU packet, original private stack, instruction loop,
source binary execution or host navigation approximation is used in production.
This component supplies neither a path-query success nor complete search.

Lookup starts at `(key&1FF)*8`, reads source WORD slots at workspace120B6,
advances with originalFFF wrap, and compares the full DWORD key at the signed
node's44-byte record+B8. It returns the source signed WORD, including NONE=-1.
There is no new node-range check, low16 key comparison or loop limit. Fully
occupied nonmatching tables keep the original cycle; the reference deliberately
stops at declared read-failure boundaries after real wraparound.

For segment start A, end B and reference P, candidate math uses
`d=B-A`, `norm=(dx*dx+dy*dy)+dz*dz`, and the original numerator
`((A.y-P.y)*dy+(A.z-P.z)*dz)+(A.x-P.x)*dx`.
This sign is intentional. A nonpositive or unordered norm copies A as raw bits.
An ordered fraction below0 or above1 copies B. A NaN fraction falls through
to interpolation. Native scalar SSE and actual COMISS retain rounding, DAZ/FTZ,
NaN operand priority and status effects. Input differences are read in XYZ
order; numerator reads A.z and A.x before P.z and P.x. Interpolation publishes
X before rereading A.y and Y before rereading A.z. Overlapping outputs therefore
keep the original lazy reads. The actual one constant is read through source
memory only when its comparison branch is reached.

The scoped native SIMD owner adopts/captures MXCSR and restores unrelated host
MXCSR. Original x87 controls, status and occupied values are untouched. Masked
SIMD exceptions are the qualified profile; an unmasked trap owner is required
before admitting that separate case. The API does not reconstruct dead flags,
XMM registers, volatile GPRs or callee private pushes.

`tools/oracle_native_campaign_ai_path_candidates_tu15.py` executes unchanged
physical i386 code from English TU1.5 SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The353 cases execute all74 sites and222,714 original instructions. There are
45,813 ordered reads,519 stores,103 fault prefixes and three actual original
unmapped operand faults. The matrix covers high key bits, signed/extreme node
IDs, collisions, full4096-slot wrap and nonterminating-table prefixes; it also
covers every candidate branch, all rounding/DAZ/FTZ combinations, endpoint and
norm boundaries, raw/paired NaNs, infinities, subnormals, aliases, source read
order and partial publications. Complete source FP and caller x87 preservation
match. No dependency result, original instruction patch or resumed failure
outcome is supplied. No previous geometry/vector source was changed.

GCC normal and ASan/UBSan pass1/1 each. Clang passes8/8 with seven actual-source
mutations rejected: unsigned node offsets, lost hash wrap, truncated keys,
textbook fraction sign, reassociated norm, excluded zero norm and reordered
source reads. The independent original repeat is byte-identical. Exact current
pins, commands and verified scratch cleanup are in
`analysis/campaign-native-ai-path-candidates-20261028/proof.json`.

The required fixture is
`tests/fixtures/native_campaign_ai_path_candidates_tu15.json.gz`,142,472 bytes,
SHA-256 `73e66567ea38ecf487e5210113b076718fc08b2ffa91290d20a152496099b98e`.
All owned original ELF, raw banks and compiler/build products were removed.
Only the required reference and bounded reports remain. Numeric/live dispatch,
complete27B340/200D30 search/update, authored jump feasibility and generic native
campaign acceptance remain open.
