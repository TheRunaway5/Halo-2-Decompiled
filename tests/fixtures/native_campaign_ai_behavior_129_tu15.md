# Native behavior129 leaves

The required compressed reference contains378 unchanged physical TU1.5 calls:
1CACB0 sentinel selection,1CACD0 state initialization, and1CAE70 property-facing
publication. All7+4+22 source instruction sites execute. The image hash is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
No instruction or child result is supplied or patched.

The native comparison checks complete GP/XMM/FP outputs,270 ordered retained
stores with exact source PCs, and all retained bytes.1CACD0 also preserves exact
EFLAGS. The other leaves' arithmetic flags and dead private frames are outside
the contract. The15,660 register/FP assertions include preserving pointer high
bytes when the original writes only AL or AX.

Caller-owned storage remains retained even when its address is in the source
stack. Cases reproduce the live caller's state=SP+48h/output=SP+54h relation and
safe stores aliasing argument words atSP+4 andSP+8. The state pointer is loaded
before its DWORD clear; the unused actor argument is varied independently.
Other cases cover unaligned state, all flag-byte classes, signed property-kind
boundaries, missing property and multiple actor slots.

Ten source mutants were rejected. ASan/UBSan passed for both production source
and replay driver; an independent original run was byte-identical. A separate
read-only source review found no actionable mismatch.

Fixture:`native_campaign_ai_behavior_129_tu15.json.gz`,126,783 bytes,
SHA-256`c9c1590bfdc9c8ddbaf9fd5b8afa3f75793f96a3897d15093b758dfa40825972`.
Receipt:`build/test-results/armory-native-progress-20261113/behavior-backing/leaves-closure.json`.
Owned original/probe binaries, raw banks and mutation files were removed.
The larger1CACE0 perform owner and live campaign qualification are separate.
