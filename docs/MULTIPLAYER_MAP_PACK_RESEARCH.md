# Multiplayer Map Pack disc: update authority and level records

Evidence collected 2026-10-07 from the user's ISO and its extracted files. This
research identifies the disc payloads; it does not claim the native engine applies
the title update, and it does not assign guessed tuning values.

## Findings

The disc contains a **Halo 2 1.1 title update**, nine DLC map caches, localized
installer/downloader resources, and map catalogue records. The disc's English
installer confirmation explicitly tells the player to check for `1.1` in the lower
right corner of the initial Halo 2 start screen. Its other seven confirmation
languages contain the same version check. The installed update requires the
language that matches the original game disc.

`patch.lvl` and `patch_v3.lvl` are **multiplayer map catalogue data**. Their original
loaders validate checksums and install localized names, descriptions, scenario
paths, flags/auxiliary menu data and map preview pixels. These files do not contain
a decoded list of weapon/grenade/melee balance adjustments. Their filenames must
not be used as evidence that numerical tuning has been extracted.

This disc does **not establish the final-ever Xbox update**. Xbox announced another
Halo 2 auto-update on July 20, 2005, after the period associated with this pack.
Additional maps, Tombstone and Desolation, were released on April 17, 2007 and are
absent from the extracted nine-map inventory. The exact last title-update version,
its executable and differences from this disc have not been verified here.
[Xbox July 20 update announcement](https://news.xbox.com/en-us/2005/07/20/halo-2-autoupdate-is-here/),
[Xbox April 17, 2007 map announcement](https://news.xbox.com/en-us/2007/04/17/halo-2-blastacular-map-pack/).

Microsoft's own Map Pack description says it includes changes to weapon, grenade
and melee combat. This establishes the intended scope of the title update, not
the numerical values or the code implementing those changes.
[Microsoft Map Pack description](https://news.xbox.com/en-us/2005/08/26/halo-triple-pack/).

## Original artifacts

| Artifact | SHA-256 / relevant evidence |
| --- | --- |
| User's Multiplayer Map Pack ISO | `e822d1da7040e9479c2cd301f9300f240daea7029a898ac6216a1b1b29f14b34` |
| Base game `assets/disc/default.xbe` | `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d` |
| Update `demos/update/Updates/default.xbe.03` | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| Updated `.text` section | `cc2f07253ca6d531ff068442df31cb91773a03ae2d6d25779f8be72ee95a5131` |
| Each of the three `patch.lvl` files | `87cd6035b6457bdf4fb5988bca00f398ebc7f3eb944b33df0b6c446722eb9b24` |
| Pack 1 `patch_v3.lvl` | `79aa649a0e9ea9f56d407ce02d6e8b5f08cf0549e33691b30051f30cc163d739` |
| Pack 2 `patch_v3.lvl` | `5ea88ad33dd6429fe633ade7adeea4d128fcc3516358ceea6fa2955d332f1a1d` |
| Pack 3 `patch_v3.lvl` | `b4518faa1d892fbf17dde710881f57e7ea36f69883f1e430a8c476fc920963c7` |

All paths in the remainder of this document are relative to the repository unless
otherwise stated. Extracted update resources are under
`assets/multiplayer-map-pack/demos/update/Updates/`; DLC resources are under
`assets/multiplayer-map-pack/demos/dlc/`.

`default.xbe.03` through `.10` share the same executable-section contents; their
certificate versions differ (`0x303` through `0x30a`). Both the base and updated
executables contain the build string `02.09.27.09809`. Therefore neither that build
string nor the filename suffix identifies a different gameplay build among the
eight disc variants. Section/certificate inventories are saved as
`analysis/multiplayer/map-pack/default.xbe.03.json` through `.10.json`.

The disc-local primary version source is
`assets/multiplayer-map-pack/demos/update/Updates/Confirmation.utf.01`, a UTF-16
English confirmation message. The unmodified ISO extraction manifest is
`assets/multiplayer-map-pack/disc-manifest.json`; full file listing is
`analysis/multiplayer/map-pack-disc-list.txt`.

## LVL loader and record layout

The base game's `0x19BDC0` checks version `2`, clears the checksum at record `+4`,
starts CRC at `0xffffffff`, processes `0x2D3FC` bytes, compares the result to the
stored checksum and restores the checksum. The matching decompile is
`tools/reference/halo2-decompiled/src/unknown_19bd50.cpp`; its original CRC body is
`tools/reference/halo2-decompiled/src/crc.cpp` at `0x163BA0`.

Updated executable `default.xbe.03` has the corresponding validator at `0x19F270`.
It checks version `3` and processes `0x2D400` bytes through its original CRC callee
`0x1656A0`. The updated file loader at `0x19F4C0` opens `patch_v3.lvl`, reads an
8-byte header containing version `3` and a positive record count, then reads and
validates that many `0x2D400`-byte records. Its registration call is `0x19F2E0`.

The exact checksum expression is:

```text
zlib.crc32(record with the little-endian uint32 at +4 set to zero) XOR 0xffffffff
```

The XOR accounts for the original routine leaving its accumulator uncomplemented.
This expression was checked against execution of the original instructions, not
against a proposed native parser.

| Record field | Version 2 offset/size | Version 3 offset/size | Evidence |
| --- | --- | --- | --- |
| Record version | `+0`, 4 bytes, value 2 | `+0`, 4 bytes, value 3 | Original validators |
| Record checksum | `+4`, 4 bytes | `+4`, 4 bytes | Original validators |
| Record type | `+8`, 4 bytes, value 1 | `+8`, 4 bytes, value 1 | Registration rejects another type |
| Scenario path | `+0xC`, 0x100 bytes | `+0xC`, 0x100 bytes | Original registration copies to map entry |
| Added field; meaning not established | Absent | `+0x10C`, 4 bytes | Byte layout and downstream field shifts |
| Map identifier | `+0x10C`, 4 bytes | `+0x110`, 4 bytes | Original registration's entry lookup/key store |
| Field before titles; meaning not established | `+0x110`, 4 bytes | `+0x114`, 4 bytes | Position immediately before titles; on-disc value `0xffffffff` |
| Localized titles | `+0x114`, 9 × 0x40 bytes | `+0x118`, 9 × 0x40 bytes | Original copy of 0x90 dwords; decoded UTF-16 text |
| Localized descriptions | `+0x354`, 9 × 0x100 bytes | `+0x358`, 9 × 0x100 bytes | Original copy of 0x240 dwords; decoded UTF-16 text |
| Auxiliary integer | `+0xC54`, 4 bytes | `+0xC58`, 4 bytes | Copied to runtime entry `+0xC4C`; meaning not established |
| Flag byte | `+0xC58`, 1 byte | `+0xC5C`, 1 byte | Copied to runtime entry `+0xC50` |
| Auxiliary map-menu data | `+0xC5C`, 16 bytes | `+0xC60`, 16 bytes | Copied to runtime entry `+0xC54`; individual meanings not established |
| Preview pixel bytes | `+0xC6C`, 0x2C790 bytes | `+0xC70`, 0x2C790 bytes | Original copy of 0xCF rows, 0xDC dwords per row |
| Total record | `0x2D3FC` = 185,340 bytes | `0x2D400` = 185,344 bytes | Validators and exact file lengths |

The preview copy is 207 rows × 220 4-byte pixels. Its original destination pitch
comes from a texture lock call; the code advances the source by `0x370` bytes per
row. Channel/format naming is not established by this copy alone, so preserve the
raw bytes until the destination texture format is independently verified.

Version 2 files contain one record with no container prefix. All three copies on
this disc are identical and describe Lockout (map identifier `50`). Version 3
files have an 8-byte container prefix and 2, 2 or 5 records, producing lengths
370,696, 370,696 and 926,728 bytes respectively.

## Authored catalogue

The English names and identifiers below are decoded from original record fields.
The file/scenario aliases are not guessed from names:

| DLC directory | Map cache | Authored scenario path | English title | Map identifier |
| --- | --- | --- | --- | --- |
| `00000001` | `containment.map` | `scenarios\multi\containment\containment` | Containment | 1300 |
| `00000001` | `warlock.map` | `scenarios\multi\halo\warlock\warlock` | Warlock | 1109 |
| `00000002` | `deltatap.map` | `scenarios\multi\deltatap\deltatap` | Sanctuary | 1302 |
| `00000002` | `turf.map` | `scenarios\multi\turf\turf` | Turf | 1000 |
| `00000003` | `backwash.map` | `scenarios\multi\backwash\backwash` | Backwash | 1201 |
| `00000003` | `dune.map` | `scenarios\multi\dune\dune` | Relic | 1200 |
| `00000003` | `elongation.map` | `scenarios\multi\elongation\elongation` | Elongation | 1001 |
| `00000003` | `gemini.map` | `scenarios\multi\marathon\gemini\gemini` | Gemini | 1002 |
| `00000003` | `triplicate.map` | `scenarios\multi\triplicate\triplicate` | Terminal | 1101 |

Xbox's April 25, 2005 announcement identifies the first two download groupings as
Bonus (Warlock/Containment) and Killtacular (Sanctuary/Turf). Its July 5, 2005
announcement identifies the last five as Maptacular. Those names agree with the
disc's grouping and authored record titles.
[Xbox first four map release](https://news.xbox.com/en-us/2005/04/25/315/),
[Xbox Maptacular release](https://news.xbox.com/en-us/2005/07/05/halo-2-maptacular-pack-now-available/).

## Reproduction and limits

### Executable and combat comparison

The separate Ghidra project `analysis/projects/halo2-map-pack-1-1` and export
`analysis/map-pack-update` analyze the pinned update `.03`. This export contains
13,812 functions, with 13,811 successfully decompiled. These are analysis
pseudocode; they are not recovered buildable game source. The base game's
function inventory was not applied to the update's shifted addresses.

`tools/compare_map_pack_combat.py` compares resident tags by group and authored
path, reads visible combat fields through the pinned Assembly schemas, and
resolves each cache's local references and StringIDs before comparing. It checks
weapon combat/aim fields and magazine/trigger/barrel arrays, projectile fields
`+BC..+183`, damage fields `+0..+6B`, and the globals damage/armor table. Each DLC
map supplies 158 or 159 common tags and 9,275 or 9,300 compared fields against
base Lockout. All compared numerical fields match. Four Maptacular caches
(Backwash, Relic, Gemini and Terminal) add the Gauss projectile's flyby sound
reference to `sound\\vehicles\\scorpion\\scorpion_projectile_up`; it is absent
in Lockout and the other five DLC caches. This finding describes raw authored
cache data, not an executable balance patch. The report is
`analysis/multiplayer/map-pack/combat-cache-comparison.json`.

The existing native armor, shield and body damage kernels were also compared
directly with the **untouched 1.1 instructions**:

| Kernel | Base entry | 1.1 entry | Cases matching exactly |
| --- | --- | --- | --- |
| Damage/armor lookup | `D5BC0` | `D71C0` | 6,000 / 6,000 |
| Shield damage and overflow | `D9110` | `DA710` | 6,000 / 6,000 |
| Body damage and region routing | `D8CB0` | `DA2B0` | 8,000 / 8,000 |

The updated material scaling (`DA620`), body-vitality helper (`D7320`), armor
lookup (`D71C0`) and region selection (`DC690`) execute unchanged in the body
oracle. Fixture boundaries still supply team scaling, maximum shield, selected
materials, region response, riders, depletion notifications and effects. Float
bits, flags, stun, overflow and callback results are compared, including missing,
duplicate and wrapping armor keys. The same 20,000 cases also pass against the
base executable after extending the tool. Reports pin the original executable
and native library hashes and list the verified call/global address mappings.
This does not test full damage-event construction, hit geometry, multiplayer
modifiers, grenade explosions, melee, or title-update installation/application.

```sh
python3 tools/compare_map_pack_combat.py
.tools/python/bin/python tools/differential_damage.py --version map-pack-1.1 --library build-multiplayer/libhalo2_kernels.so
```

The differential report is
`analysis/multiplayer/map-pack/damage-1.1-differential.json`. Original scalar
instruction listings are saved beside it as `tu-damage-*.asm`. No numerical
balance adjustment follows merely from relocated function addresses or a new
helper extracted from formerly inline code. Recovery of the specific 1.1 balance
changes remains unresolved; the native runtime has not been labeled 1.1 or
given guessed damage/radius multipliers.

Run from the repository root:

```sh
python3 tools/audit_map_pack.py
.tools/python/bin/python tools/oracle_map_pack_levels.py > analysis/multiplayer/map-pack/level-instruction-oracle.json
```

The audit independently rehashed all 148 extracted files (5,479,383,937 bytes)
against the extraction manifest, checked that the base executable remains
unchanged, compared all eight update section inventories, validated the nine
map headers and decoded/CRC-checked every catalogue record. Its result is
`analysis/multiplayer/map-pack-audit.json`. The existing native cache reader
also recognized all nine DLC caches; per-map inspection outputs are in
`analysis/multiplayer/map-pack/native-cache-inspection.json`. That check reads
headers, tag directories and BSP records; it does not render or play the maps.

The oracle pins both identified executable hashes, maps their sections unchanged,
and executes the actual record validator and its CRC callee. It supplies record
bytes and caller register/stack state. It does not replace either CRC routine.
For each of the 12 extracted records (3 identical version-2 copies and 9 version-3
records), it requires acceptance, rejection after one payload byte changes,
rejection after the version changes, and checksum-field restoration. All **36
cases** passed. The Python CRC expression is separately compared with the original
accepted value. The result is saved in
`analysis/multiplayer/map-pack/level-instruction-oracle.json`.

Relevant original instruction listings:

- `analysis/multiplayer/map-pack/v3-level-read.asm`: updated file loader;
- `analysis/multiplayer/map-pack/v3-level-record-checksum.asm`: updated validator;
- `analysis/multiplayer/map-pack/v3-level-register.asm`: updated catalogue registration;
- `analysis/multiplayer/map-pack/v3-level-read-begin.asm`: initial file-validation path, partial listing.

Still required before applying gameplay balance: locate and recover the changed
title-update gameplay routines and any tag postprocessing, execute their original
instructions against source-backed fixtures, and verify affected weapons,
grenades, melee and networking transitions. Comparing raw map tags alone cannot
prove updated runtime balance because the updated executable can alter behavior
or tag values after loading. A later/final update also requires its own original
artifact and provenance; this disc supplies the 1.1 authority only.

The accessible Xbox announcement confirms the April 18, 2005 auto-update release,
and the April 16 notice explicitly points to weapon changes. The legacy Bungie
detail URLs linked from those notices returned unavailable/404 responses during
this research. Forum comments and remembered percentage values were deliberately
not used as numerical tuning authority.
[Xbox April 18 update](https://news.xbox.com/en-us/2005/04/18/halo-2-autoupdate/),
[Xbox April 16 weapon-update notice](https://news.xbox.com/en-us/2005/04/16/at-last-halo-2-auto-update-details/).

## Verified weapon-error path recovered from disc 1.1

[Weapon error instruction research](WEAPON_ERROR_1_1_RESEARCH.md) records the
original firing/idle accumulation, opposite-hand blend, angle selection,
`147A90` table/RNG/rounding, and exact invocation boundaries. All 70 state/angle
fixtures and 13,098 additional complete random-direction cases match the native
scalar kernels bit for bit. These kernels are not wired to live firing yet:
full barrel marker, hand timer and shot-pass scheduling still need
source-backed integration. The paired base `A2BA0` / updated `A38F0` instructions
also verify a new 1.1 fired-error update before projectile dispatch in one direct
shot-application path. Its higher-level event and global timing remain unresolved.
This does not establish all title-update balance changes or their live application;
`native_gameplay_update_applied: false` remains the correct status.
