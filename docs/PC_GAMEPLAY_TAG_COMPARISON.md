# Original Xbox 1.5, supplied Vista and installed MCC tags

Both supplied PC sources reproduce the **41 measured destinations of the
original Xbox 1.5 multiplayer combat adjustment**. Vista's unchanged original
executable writes those values after loading tags; MCC already stores them.
Neither PC cache set is an exact replacement for the original Xbox tag banks.
This project should continue using **original Xbox data together with original
1.5 consuming code** as its parity authority.

The earlier MCC **39/41** claim was a reader error. The weapon schema names
two different barrel fields `Error Angle`, at +0x5C and +0x74. The old reader
selected +0x5C by name; original update instructions target +0x74/+0x78.
The corrected reader uses the measured original address to disambiguate the
field. MCC matches **41/41**, while raw Vista matches **3/41** before its
original executable applies the 38 actual changes. Historical reports and the
old reader remain preserved and explicitly superseded.
[Corrected sample](../analysis/multiplayer/pc-gameplay-tag-comparison-20261011/corrected-sample/report.json),
[corrected sample explanation](PC_COMBAT_TAG_SAMPLE.md),
[original PC execution evidence](PC_COMBAT_EXECUTABLE_RESEARCH.md).

## Which inputs were compared

The inventory covers 23 canonical original multiplayer maps: 12 retail maps,
nine first map-pack maps, and Desolation/Tombstone from the later owned DLC.
Duplicate DLC copies and the distinct older Tombstone revision remain recorded
with their hashes. Original `highplains2.map` is mapped explicitly to the
`highplains` PC counterpart; its original filename and bytes remain intact.

| Source | Multiplayer maps inventoried | Original-map counterparts | Additional PC maps |
|---|---:|---:|---|
| Original Xbox retail plus owned DLC | 23 canonical | 23 | none |
| Supplied Vista | 23 | 21 | `needle`, `street_sweeper` |
| Installed Steam MCC | 25 | 23 | `needle`, `street_sweeper` |

The supplied Vista set lacks `derelict` (Desolation) and `highplains`
(Tombstone). These are missing comparisons, not matches. The broad comparison
therefore contains **44 platform/map pairs**, with hashed inventories of the
additional PC maps retained separately. The Vista distribution contains
Cartographer components; pristine retail distribution provenance is
unverified. Findings apply to these exact local files.
[Complete map inventory, input hashes and per-map results](../analysis/multiplayer/pc-gameplay-tag-comparison-20261011/measured/report.json).

## Tags and title-update code both contribute

Tags carry authored weapon, projectile, damage, vehicle, material and scenario
definitions and references. Code interprets those definitions, implements
rules and simulation, and can transform loaded metadata. The original Xbox
1.5 routine is a concrete example: it follows authored multiplayer weapon and
vehicle slots and references, then changes BR spread, damage, melee and other
combat fields. Reproducing only the raw disc values would omit these changes.

The original Vista routine at `0046EFDB` has the corresponding MP path.
Executed unchanged over all **23 supplied multiplayer scenarios**, it performs
41 stores and 38 changes per map, modifying 13 shared tags. Every ordered
destination equals the original Xbox field contract. This is **943 stores,
874 changes and 8,786 original instructions**, with no supplied callback or
service returns. Declared loader bases and resident metadata bindings are
explicit fixture inputs. The complete loaders and their preceding group
fixups were not executed.
[Source instructions, callers, all-map reports and verification](PC_COMBAT_EXECUTABLE_RESEARCH.md).

MCC's stored values match the corrected sample. The broad audit's independent
verification additionally checks all 41 destinations in every one of its
23 original-map counterparts. MCC's complete executable postprocessing has
not been recovered or executed here. Stored equality therefore establishes
these fields, without establishing final engine behavior.
[Independent tag audit verification](../analysis/multiplayer/pc-gameplay-tag-comparison-20261011/verification.json).

## What the wider comparison establishes

The reader compares every named schema-readable field in matching
group/path tag identities across the available original maps, including the
24 declared gameplay groups. It uses exact scalar bytes, block counts,
normalized target group/path references, actual string-ID tables and decoded
strings. Repeated sibling names retain occurrence identities. Numeric
differences are exact bit differences, including one-bit floating-point
rounding differences; no tolerance silently turns them into matches.

The Xbox private views contain only actual ordered stores executed by the
owned 1.5 routine for each map. Vista private views contain only the measured
stores from that map's unchanged original PC routine, checking every before
value. MCC views contain decoded stored metadata. Original inputs are
read-only. Full per-field differences, source offsets and types, unmatched
tags, schema hashes and parsing issues accompany each comparison.
[Audit implementation](../tools/compare_pc_tag_banks.py),
[complete report](../analysis/multiplayer/pc-gameplay-tag-comparison-20261011/measured/report.json).

The pass used **111 schema files**. In the gameplay groups, the Vista pairs
contain 50,225 named value differences and the MCC pairs contain 31,200;
MCC also has 16,295 field-type differences. These are repeated map/tag/field
occurrences across different counterpart map counts, including compiled
metadata and references, and do not rank gameplay balance. The gameplay
reader excludes **21,842 Vista-pair and 23,920 MCC-pair out-of-tag schema
reads**. Broader rendering/audio groups have additional explicit issues,
unknown fields and opaque payload exclusions.
[Compact coverage and issue breakdown](../analysis/multiplayer/pc-gameplay-tag-comparison-20261011/coverage.json).

Platform schemas matter. The audit uses Assembly's Xbox overrides, Vista
Halo2 layouts, and all 35 MCC overrides from the same pinned Assembly revision
`fd14ef3c46135e9422966c6a64cd926fb0bd23fb`, with its Halo2 fallback. MCC has
different compiled structures, including 64-bit fields and larger compiled
physics elements. Treating those layouts as unchanged Vista layouts would
produce false comparisons. The previously exploratory pilot outputs are not
the canonical results.
[Pinned MCC schema provenance](../analysis/multiplayer/pc-gameplay-tag-comparison-20261011/schemas/provenance.json),
[Assembly source at the pinned revision](https://github.com/XboxChaos/Assembly/tree/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins).

Concrete Ascension-visible differences demonstrate why whole PC tag banks
cannot be called identical Xbox replacements:

| Tag / named field | Original Xbox view | Vista view | MCC stored view |
|---|---|---|---|
| `globals\globals` / Materials block count | 182 | 183 | 183 |
| `energy_blade` / first Damage Pyramid Angles word | `e3dfd63e` | `e4dfd63e` | `e4dfd63e` |
| BR / Zoom-In Sound target | `sound\weapons\sniper_rifle\zoom_in` | same as Xbox | `sound\weapons\battle_rifle\zoom_in` |
| `ball` / weapon flags at +0x12C | `d8003400` | `d8003400` | `58003400` |

The words above are source bytes in little-endian file order. The angle
example differs by one least-significant bit. References and compiled flags
also differ. These are metadata observations: this audit does not infer an
unverified balance effect from them. Named fields that the schema describes
as runtime pointers or compiled physics data can differ for platform reasons.
Per-map counts include repeated shared definitions and must not be read as a
percentage of unique balance settings or a ranking of engine fidelity.
[Ascension Vista differences](../analysis/multiplayer/pc-gameplay-tag-comparison-20261011/measured/ascension-vista-differences.jsonl.gz),
[Ascension MCC differences](../analysis/multiplayer/pc-gameplay-tag-comparison-20261011/measured/ascension-mcc-differences.jsonl.gz).

Unknown/undefined schema fields, external resource and data-reference
payloads, absent schemas, invalid or out-of-tag schema reads, and unavailable
owners remain explicit exclusions or issues. An unreadable field is never
counted as equal. A schema label is a community layout description, not proof
of its complete original consumer semantics. Original instructions are the
authority for the measured combat adjustments. Other loader fixups, MCC
executable transforms, platform simulation differences and full live gameplay
remain outside this audit.

The MCC reader's later native-operand continuation removes the immutable
engine-data executable dependency and passes content loading on all 43 maps.
It also corrects negative-size raw cache chunks to retain bytes beginning at
their recorded offset. This does not close this audit's semantic exclusions:
for example, MCC Floodlab contains 37,509 script expressions versus 36,670 in
the Xbox reference. See [native immutable TU1.5 operands](MCC_GAMEPLAY_CONVERSION.md#native-immutable-tu15-operands)
for the validation boundary and retained evidence.

## Reproduction and verification

The canonical and repeat outputs are retained separately. The verifier
checks every retained file hash, current source/input/schema pins, all
difference group/reason totals, and representative actual numeric/count
locations per map/group/reason independently of the schema walker. It also
reconstructs processed views from original ordered stores and checks every
MCC combat destination. The separate PC execution verifier checks all 943
Vista raw owner offsets and reconstructs every changed tag buffer.
[Tag verification tool](../tools/verify_pc_tag_banks.py),
[tag verification report](../analysis/multiplayer/pc-gameplay-tag-comparison-20261011/verification.json),
[Vista execution verification](../analysis/multiplayer/pc-executable-postprocess-20261011/vista-allmaps-verification.json).

All checks passed: **712 canonical/repeat files match exactly**, all
**43,909,316 difference records** reconcile with their group/reason totals,
**5,999 reported numeric/count locations** were independently read, and all
**943 MCC combat destinations** match. The canonical and repeat top reports
have SHA-256 `de4f05b2a161c558380f159ea219b41acd084cb5296a8bb200af6bc69d0ab352`.
These counts describe the retained audit scope and its explicit exclusions.

```sh
.tools/python/bin/python tools/compare_pc_tag_banks.py \
  --output analysis/multiplayer/pc-gameplay-tag-comparison-20261011/measured
.tools/python/bin/python tools/compare_pc_tag_banks.py \
  --output analysis/multiplayer/pc-gameplay-tag-comparison-20261011/repeat
.tools/python/bin/python tools/verify_pc_tag_banks.py
```

The retained verification used two stages so canonical source checks could
run alongside generation of the independent repeat. The finish stage rechecks
every source/canonical hash and compares every repeat file before setting
`all_checks_passed`:

```sh
.tools/python/bin/python tools/verify_pc_tag_banks.py --source-only \
  --output analysis/multiplayer/pc-gameplay-tag-comparison-20261011/source-verification.json
.tools/python/bin/python tools/verify_pc_tag_banks.py \
  --finish-repeat analysis/multiplayer/pc-gameplay-tag-comparison-20261011/source-verification.json
```

Run the documented original Vista all-map oracles first if their retained
evidence is unavailable. Required local game files and pinned schema/reference
sources are recorded in the reports. No game installation is modified, and
this audit changes no native runtime behavior.

The PC-source comparison milestone is complete and its linked verification
passes. The broader [game-mode operational goal](GAME_MODE_PROGRESS.md)
still requires original tag, data-type, variant and weapon contracts to drive
normal runtime flows across all 39 built-in variants and supported editable
settings. This research does not establish that those variants are operational.
