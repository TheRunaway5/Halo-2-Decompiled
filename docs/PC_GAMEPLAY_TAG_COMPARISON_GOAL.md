# Xbox 1.5 versus PC tag comparison milestone

Status: **complete, 2026-10-11**. This is a milestone under the unfinished game-mode
operational goal. The app rejected creation of a second goal while that goal
exists; the original goal is not being marked complete or replaced.

Deliver an evidence-backed answer to which locally supplied PC tag set matches
original Xbox Halo 2 with update 1.5. Preserve original assets and unrelated
checkout work.

Acceptance:

- Inventory the original base/DLC multiplayer maps, supplied Vista maps and
  installed MCC maps, recording exact input hashes and unmatched maps/tags.
- Compare schema-readable matching tag fields, block counts, references and
  strings across the available shared multiplayer maps. Preserve duplicate
  field identities, source types and exact numeric bits; distinguish opaque or
  unavailable fields from matches. Include the gameplay groups in the results.
- Use the original Xbox 1.5 combat postprocessor's actual outputs as the
  combat baseline, and investigate the corresponding PC executable adjustment
  paths so stored raw values are not mistaken for final runtime values.
- Retain reproducible readers, complete machine-readable differences,
  independent checks and a repeat comparison. Publish a direct source
  recommendation with explicit limits on full gameplay parity.

The earlier 41-field sample is corrected: its name-only lookup confused
the two barrel fields named Error Angle. Original measured offsets distinguish
barrel +0x5C from the update's actual destinations +0x74/+0x78. Historical sample
reports remain preserved; corrected output lives under
`analysis/multiplayer/pc-gameplay-tag-comparison-20261011/`.

Delivered [findings and reproduction](PC_GAMEPLAY_TAG_COMPARISON.md),
[full comparison](../analysis/multiplayer/pc-gameplay-tag-comparison-20261011/measured/report.json),
[compact coverage](../analysis/multiplayer/pc-gameplay-tag-comparison-20261011/coverage.json),
and [passing verification](../analysis/multiplayer/pc-gameplay-tag-comparison-20261011/verification.json).

Verified: 23 original maps, 44 platform/map pairs, 111 schemas, all 712
retained canonical/repeat files identical, 43,909,316 difference records
accounted for, and 5,999 independently read numeric/count locations. All 943
MCC stored combat destinations match the original per-map Xbox 1.5 outputs.
All 23 Vista MP scenarios independently execute the unchanged original
routine, producing the same 41 destination values per map; all 668 execution
evidence files repeat exactly. Unknown schemas, opaque payloads, additional
loader transforms and complete runtime parity remain explicit limits.

The source recommendation is original Xbox tags plus original 1.5 consuming
code. Both PC sources match the measured combat adjustments through different
mechanisms; neither complete PC cache set is an exact Xbox replacement.
The original all-game-mode operational goal remains unfinished. Completion of
this comparison does not admit additional variants or prove live gameplay.
