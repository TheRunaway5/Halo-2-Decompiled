# PC combat tag sample against original Xbox 1.5

Correction, 2026-10-11: installed MCC matches **41 of 41 stored destinations**
of the measured original Xbox 1.5 combat postprocessor. Supplied Vista matches
**3 of 41 raw destinations**, but its original executable produces **all 41
Xbox values**. The previous 39/41 result selected the wrong two BR fields: the
schema repeats `Error Angle` at barrel +0x5C and +0x74. Actual update stores
target +0x74/+0x78. Raw counts do not rank effective balance or whole tag sets.
[Corrected report](../analysis/multiplayer/pc-gameplay-tag-comparison-20261011/corrected-sample/report.json),
[original Vista executable execution](PC_COMBAT_EXECUTABLE_RESEARCH.md).

The sample is exactly the destinations of the original routine measured in
[the Xbox combat postprocessor audit](MP_COMBAT_TAG_POSTPROCESS_UPDATES_RESEARCH.md).
It includes three stores whose values already matched the original raw map;
the denominator is 41 destinations, not 41 distinct balance changes. Each PC
tag is located by group/name in Lockout, then resolved through its actual shared
tag index with matching datum/group/name. All 41 destinations in both PC inputs
are owned by their respective `shared.map`. Input hashes, resolved entries,
decoded offsets, original/processed Xbox bytes and raw PC bytes are retained in
[the corrected comparison report](../analysis/multiplayer/pc-gameplay-tag-comparison-20261011/corrected-sample/report.json).

| Sampled field | Xbox 1.5 processed | Supplied Vista raw | Installed MCC raw |
|---|---:|---:|---:|
| SMG damage upper endpoints | 4.625 | 4.75 | 4.625 |
| Magnum damage bounds | 5.5 | 8 | 5.5 |
| Frag explosion damage upper endpoints | 160 | 150 | 160 |
| Plasma explosion damage upper endpoints | 200 | 120 | 200 |
| BR first-barrel +0x74/+0x78, radians | 0.001745329238474369 at both | 0.005235987715 / 0.01047197543 | 0.001745329238474369 at both |

There are no sampled MCC mismatches. The raw Vista sample differs at 38
destinations; its original MP routine adjusts those values. Values here are tag fields, not claims
about final damage after engine multipliers or complete weapon behavior.

The installed Steam manifest identifies build `19905945`; the maps use MCC
cache version 13. The supplied Vista cache identifies
`11081.07.04.30.0934.main`, cache version 8. The supplied Vista distribution
includes Cartographer components, so unmodified retail provenance is unverified.
These findings apply to the hashed local files, not every historical release.

The read-only reader follows the checked-in Reclaimer cache-header, address
translation, shared-index and MCC zlib-chunk implementation. Field paths use
the checked-in Assembly Halo2 schemas; the MCC engine descriptor names Halo2
as its fallback schema bank. The sampled fields have matching offsets in the
platform-specific schemas; the broader audit uses the MCC overrides. Those source/schema hashes are included in the
report. No installed game file is changed, and no gameplay source is replaced.

Reproduce with:

```sh
python3 tools/inspect_pc_combat_tag_sample.py \
  --output analysis/multiplayer/pc-gameplay-tag-comparison-20261011/corrected-sample/report.json
```

The original erroneous reports and reader remain preserved under
`analysis/multiplayer/pc-combat-tag-sample-20261010/`; they are historical
evidence of the reader error, not current findings. Do not regenerate them
with the corrected reader. The corrected repeat output is retained separately.
PC executable postprocessing is covered by the linked execution research;
live gameplay and visual equivalence remain outside these fixtures.
Original Xbox data plus original 1.5 executable behavior remain the parity
authority for this project.
