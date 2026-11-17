# Native TU1.5 movement resume

`native_campaign_ai_movement_resume_tu15.cpp` supplies the movement completion
branch called from `200D30` at `200D95`, `200DAA`, and `200DB7`. It calculates
source radius and arrival from retained actor/path/unit data. It does not set
unit position, synthesize movement controls, or supply an arrival result.

| Entry | Actual ABI | Native scope |
| --- | --- | --- |
| `1EBBD0` | ECX actor; XMM0.low REAL; RET | Authored radius, character/object property lookup, source defaults and minimum clamp |
| `1FB4F0` | EAX actor; stack REAL radius; AL; RET4 | Projected goal distance, final-path-segment overshoot, vehicle velocity check |
| `200A40` | EAX actor; stack action-request byte; RET4 | Ordinary movement completion and action guards; linked-action children remain named refusals |
| `218EB0` | ECX first framed point, EBX second, EAX output; RET | Same unframed or differing-frame difference; same non-NONE frame refuses at `218EFB -> 218880` |
| `218F60` | EAX framed point, EBX world point, EDI output; RET | Native projected/world difference |

The radius uses existing native `1ED700`/`1ECD00` property owners. Geometry uses
the existing native frame projection. Exact scalar SSE operations run with the
retained MXCSR and restore the host state. Publication passes the actual stack
arguments and synchronizes floating-point state on normal and exceptional
returns. The component admits masked FP exceptions and an empty x87 stack.

The unchanged English TU1.5 XBE is pinned to
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The physical original and native replay agree on 952 cases: 903 whole returns
and 49 deliberate child boundaries. Every instruction in `1EBBD0` (66) and
`1FB4F0` (130) was reached; the other reached counts are `200A40` 60/127,
`218EB0` 41/45, and `218F60` 31/31. All 602 retained stores, their source PCs,
retained DATA, defined results, saved GP/ESP and floating-point states match.
Seven independently built mutants are rejected. The owner and replay driver
pass ASan/UBSan/LSan against all 952 cases; reused child objects were not
reinstrumented. Ordered reads, dead/private child frames, arbitrary aliases,
unconsumed volatile GP/XMM and arbitrary fault continuations are not claimed.

The parent refresh replaces 24 command-movement and seven command-point
`1EBBD0` stops using identical initial bytes and actual original child execution.
One publication gate fixture required an input correction: its mode4 goal field
contained float1.0 (`3F800000`), previously unconsumed before the stop. Continuing
the original requested an undeclared zone count at `010E1A28`. The fixture now
uses legitimate absent goal `FFFFFFFF`; its original execution was rerun. The
failed comparison remains recorded. This row is not a same-input parity claim.

Seven newly composed point cases reach existing native spatial/vector owners
whose semantic stores do not expose PCs. Only the recorded PCs `BD0FA`,
`BD10D`, `14D12D`, `BD11D`, `BD148`, `BD14E`, and `BD433` are excepted for
these rows. Their byte values and order still match. All other store PCs retain
the established publication checks.

Evidence and storage receipts are under
`build/test-results/armory-native-progress-20261113/movement-resume/`.
`qualification.json` records final source/fixture hashes, parent validation,
retained original references, and verified scratch removal. The canonical
regression input is `tests/fixtures/native_campaign_ai_movement_resume_tu15.json.gz`.
The first generated reference included unused navigation workspace padding;
the canonical rerun uses zero padding, reducing the reference from 4.94 MB to
399 KB without changing any reached path or result. The superseded new capture
is removed after promotion; former canonical parent references are preserved.
