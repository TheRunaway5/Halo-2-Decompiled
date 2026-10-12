# Sandbox owners in the newly owned later executable

This is a separate byte-level comparison and scalar execution freeze. The
immutable [1.1 creation/collision fixture](MP_SANDBOX_CREATION_COLLISION_1_1_RESEARCH.md)
and [authored inventory/contact-scalar evidence](MP_SANDBOX_EXPLOSIVES_LIFTS_SOURCE_RESEARCH.md)
remain unchanged. No 1.1 address is assumed to be valid in the new image.

The new image is the user's supplied `Halo2DLC.zip` extraction,
`Halo 2 DLC/content/u5English/default.xbe`, SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
It stays outside the repository in the user-owned import directory. This
research pins those actual instruction bytes and does not infer a latest-ever
release or numeric gameplay version from the `u5English` name. Retail signature
and version identification are separate import/audit work.

The earlier `decomp1.5TU.zip` has only annotation arrays: 16094 function entries
and120593 item entries, addresses/names/comments/ranges/flags. Its separate
functions/items files duplicate the combined arrays. No executable, instruction
bytes, assembly bodies, pseudocode bodies or source-image hash was present.
Those annotation names/comments and its `Halo 2 1.5TU` author path are not used
as behavior authority here; its scripts were not executed.

## Directly recovered correspondence

The tool searches genuine instruction prefixes, then traces each candidate from
the new bytes using real direct-call targets as boundaries. Every selected
routine has a unique candidate. All **35 complete normalized instruction
sequences** and their direct internal branch destinations match the 1.1 owner.
All four traced indirect switch tables retain their original entry counts and
target instruction indices. Every absolute image-memory operand and direct call
pair is recorded separately. All direct calls whose target is among the35
recovered entries point to the expected new target.

| Owner | Disc 1.1 | Later owned image |
|---|---|---|
| Authored crate factory | BFFE0 | C1120 |
| Authored request builder | D6660 | D75A0 |
| Generic request defaults | B8820 | B9960 |
| Object allocator/constructor | B8A30 | B9B70 |
| Type11 constructor | 11D1C0 | 11E320 |
| Type11 update | 11D2B0 | 11E410 |
| Phantom contact admission | 1D5830 | 1DA0F0 |
| Phantom contact removal | 1D5A60 | 1DA320 |
| Phantom scalar helper | 1D5C70 | 1DA530 |
| Phantom full updater | 1D5E90 | 1DA750 |
| Phantom filter | 1D56F0 | 1D9FB0 |
| Physics shape selection | 1D98B0 | 1DE170 |
| Model damage-info resolution | D7160 | D80A0 |
| Damage response admission | DC0A0 | DCFF0 |
| Per-object response timer | D73E0 | D8330 |
| Destruction response | DB710 | DC660 |
| Destruction area-event producer | DB470 | DC3C0 |
| Area-damage consumer | D8280 | D91D0 |
| Direct object-damage consumer | D9180 | DA0D0 |
| Damage event defaults | D7C60 | D8BB0 |
| Physics-model postprocess | 1EE380 | 1F2C70 |
| Serialized shape-index resolver | 1EE8F0 | 1F31E0 |
| Posed object query | 16AF90 | 16C370 |
| Model query context | 1F3500 | 1F7E00 |
| Posed collision BSP traversal | 1F3900 | 1F8200 |
| Model-material selection | DA0E0 | DB030 |
| Projectile mask conversion | 16A560 | 16B940 |
| Ambient-sound object producer | BFB90 | C0CD0 |
| Ambient-sound owner registration | 18A9C0 | 18BD80 |

The full report contains all35 entries and original/later instruction listings.
There are14 differing paired memory reads, involving relocated pointers or
switch-table addresses; their exact old/new bytes are retained. Matching
normalized control flow alone does not prove those tables' callees, source
runtime global initialization, full contact behavior or explosion execution.
The report deliberately makes no blanket gameplay equivalence assertion.

## Independently executed lift scalar

Original later **1DA530** has134 traced instructions, matching disc1.1
**1D5C70**, including the original floating operation order and branches.
Its epsilon moves **4650E0 →46A180** and remains bits **3A83126F**. Its source
time-pointer slot moves **518444 →51DF54**. The source scalar constants read
by the helper match byte-for-byte, including the sign mask and finite sentinels.

The new oracle runs the unchanged later instructions for each of the512 inputs
from the immutable 1.1 scalar fixture. The older outputs are comparisons only;
they are never used to supply a later return. Every later result and output
ratio matches the older float bits, and every stack cleanup is44 bytes.
The entire later report and instruction listing repeat identically.

These remain explicit contact-distance, projected-velocity, mass, BL permission
and30Hz clock fixtures from the unchanged Lockout/other authored phantom rules.
No original contact membership, source clock producer, live player, complete
vector force, Havok contact, audio registration or device result is supplied or
claimed executed. Thus this freeze supports the bounded scalar kernel in the
owned later executable; the lift's actual world/contact producer stays open.

## Reproduction and boundaries

- [Correspondence tool](../tools/research_mp_sandbox_update_functions.py).
- [Later original scalar oracle](../tools/oracle_mp_lift_later_update.py).
- [Audit](../tools/audit_mp_sandbox_later_update.py).
- Reports/listings: `analysis/multiplayer/sandbox-source/later-update-functions/`.
- Independent manifest/audit: `later-update-functions/frozen/`.

```sh
.tools/python/bin/python tools/audit_mp_sandbox_later_update.py
```

The two function-correspondence reports and two512-case scalar reports are
independently reproduced. The audit checks their actual source-image bytes,
all function/branch/switch correspondences, explicit input/output bits and the
unchanged earlier1.1 creation/inventory/scalar manifests. All new reports keep
`native_gameplay_update_applied:false`; no production file has been edited by
this research.
