# User-supplied later title-update executables and DLC

The user supplied `/home/eric/Downloads/Halo2DLC.zip`, SHA256
`927a4b262a57713915e6c473d54d626dc6a77e821f7d19884748d455bc6f2a59`.
Its 127 members expand to 818,115,872 bytes. Paths, duplicate names and symlink
modes were checked before extraction. All extracted member sizes and SHA256
hashes were checked against the import manifest. No included installer,
executable or annotation script was executed.

The extracted copyrighted reference files remain outside the repository at
`/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/`.
The source repository contains the audit tool and these notes; ignored analysis
contains manifests, source findings and the local static decompilation.
[Import manifest](../analysis/multiplayer/title-update-1.5/dlc-import-manifest.json).

## Actual executable authority

The archive contains game executables under `content/u1English` through
`content/u5English`, additional update-5 language certificates, downloaders and
a modified third-party installer. The installer is not gameplay authority.
The selected reference is:

| Field | Verified value |
| --- | --- |
| File | `Halo 2 DLC/content/u5English/default.xbe` |
| Full SHA256 | `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd` |
| `.text` SHA256 | `d9c6fd7e7475862b1f19c2da80d7f2f6dc41da53619991002e9546e5f9e629fe` |
| Title / ID | `Halo 2` / `4D530064` |
| Certificate version at +AC | `00000803` |
| Original decoded entry | `002DA9DE` |
| Retail RSA signed header | Valid |
| Every section's original Xbox digest | Valid |

[Audit tool](../tools/audit_dlc_archive.py) checks the signed header against the
retail public modulus/exponent, the length-prefixed SHA1, original digest
padding and every section digest. The exact format/key authority is pinned
[XboxDev/xbedump `xboxlib.c`](https://github.com/XboxDev/xbedump/blob/b8cd5cd0f8b1cbc4e64f41daf925a873d34dacf1/xboxlib.c),
SHA256 `3eb176d28aac3688ebc1356ad2be0e1beac8de5e9f14fb1cbd887acc4ba6cac2`.
The local source is read as data, and its hash must match before use.
All 27 XBEs were audited: 24 authenticate with the retail key and section
digests; the three installer/reboot XBEs do not fully authenticate. Disposable
certificate and section byte changes are rejected for every executable.
[Full authentication report](../analysis/multiplayer/title-update-1.5/dlc-authentication.json).
An independent compiled probe also runs the pinned source's unchanged
`load_rsa`, `VerifySignaturex` and `shax` bodies over the selected file. It
returns `signature=1 sections=1`, agreeing with the separate Python audit.
The probe reads the XBE as data and does not execute game instructions.
[Independent probe provenance/result](../analysis/multiplayer/title-update-1.5/primary-source/xbedump/probe-result.json).

The original `build_number` callback and complete private-marker replacement
path now independently execute and emit UTF-16 **`1.5`**. The corresponding
older executable emits **`1.1`**. Both original formatter/CRT paths run without
stubs or supplied formatting results. Four calls reproduce independently,
including 2,426 source reads and 1,336 ordered stores.
[Numerical display proof](MP_TITLE_UPDATE_DISPLAY_RESEARCH.md). This identifies
the supplied executable as 1.5 independently of its folder/readme; certificate
values are not interpreted as decimal patch labels. The bounded text producer
does not itself establish an entire authored title-screen render.
Retail authentication establishes the supplied signed bytes, not a claim that
all native gameplay matches those bytes or that this is the last possible
release for every regional disc.

## Annotation alignment and static code export

The user also supplied `/home/eric/Downloads/functions.json`, SHA256
`4a0aff40054c65f8c9f6b0ca7b07a1044d817fc87a66e9fa84f537e061512e5d`.
It is byte-identical to the earlier [annotation archive](MP_TU_1_5_ANNOTATION_ARCHIVE.md).
All 16,094 exported ranges fit entirely in matching named raw-backed sections
of the selected executable, without overlaps. Its real entry matches the
exported `start`. Entry-root recursive instruction decoding starts from the
executable itself: 71 of 79 reachable code targets coincide with exported
starts; eight lie within parent ranges. One alleged function range contains
only 16 `CC` bytes, demonstrating why annotations are navigation hypotheses
rather than validated behavior.
[Independent alignment report](../analysis/multiplayer/integration/inventory-source/attached-functions-code-alignment/report.json).

A separate fresh Ghidra project, `halo2-title-update-1-5`, imports the verified
English executable with the existing pinned loader/analyzer. It does not seed
the prior update 1.1 inventory or execute the attached annotation scripts.
The export completed with 13,904 identified functions, 13,903 pseudocode
exports and one 60-second decompiler timeout at `0040A2D0`. It is not a claim
that every original function boundary or path is identified. The outputs are
pinned in [the decompilation manifest](../analysis/multiplayer/title-update-1.5/decompilation-manifest.json).
Generated pseudocode remains a local inspection aid; original instruction
bytes, owning data and measured execution remain the behavioral authority.
[Decompilation provenance](../analysis/multiplayer/title-update-1.5/decompiled/provenance.json).

## Authored multiplayer map data

The nine first-generation DLC maps are byte-identical to the already owned Map
Pack ISO maps. The new authored catalogs name:

| Authored name | Current file | Earlier supplied revision |
| --- | --- | --- |
| Desolation | `content/c/4d53006400000004/derelict.map` | Identical duplicate in `00000004old` |
| Tombstone | `content/c/4d53006400000004/highplains2.map` | Distinct `00000004old/highplains.map` |

Every supplied map has an original version-8 header, matching declared file
length and footer. Original `patch_v3.lvl` records provide the display names
and scenario paths; they are map-menu records, not balance settings. Both
Tombstone revisions are preserved as distinct inputs. Native support and
sandbox behavior are not inferred from header/catalog validity.
[Map hashes and catalogs](../analysis/multiplayer/title-update-1.5/dlc-map-inventory.json).

A [raw resident combat-field comparison](../analysis/multiplayer/title-update-1.5/raw-dlc-combat-comparison.json)
checks 120,650 field pairs across all 13 supplied map files, including the old
duplicates/revision. The only eight differing matched fields are the Gauss
projectile's flyby-sound reference. This excludes executable postprocessing,
opaque fields, unmatched or nonresident tags and other groups; it cannot
establish final weapon balance. Executable balance postprocessing must be
investigated separately.

The [later sandbox comparison](MP_SANDBOX_LATER_UPDATE_SOURCE_RESEARCH.md)
maps 35 complete selected instruction sequences and four switch tables from
1.1 to the new executable. Original `1DA530` also executes 512 explicit scalar
lift-force cases, matching the old outputs, ratios and stack cleanup exactly
and reproducing independently. Those inputs are declared consumer fixtures;
the player's live lift-contact producer is not supplied or certified.

## Integration boundary

Previously frozen update 1.1 executions and native comparisons stay unchanged.
The delivered `build-test-20261008-140508` remains the existing one-window menu
fix, not a 1.5 gameplay release. The new executable is available for fresh
routine mapping and differential execution. Player damage/device admission,
animation ownership, explosive world collision/damage, lift contacts and the
complete original lobby countdown still require live integration and end-to-end
validation; no changed behavior is inferred from annotation comments.

Reproduce authentication with:

```sh
.tools/python/bin/python tools/audit_dlc_archive.py \
  --root /home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57 \
  --crypto-source analysis/multiplayer/title-update-1.5/primary-source/xbedump/xboxlib.c \
  --output analysis/multiplayer/title-update-1.5/dlc-authentication-repeat.json
```
