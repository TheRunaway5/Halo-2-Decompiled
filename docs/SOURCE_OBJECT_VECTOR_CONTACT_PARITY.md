# Dynamic object contact traversal

`source_object_vector_contact` ports retail `169510` and its descendant filter
`168A10`. It accepts actual owner bodies, header bytes, attachment links,
primitive spheres, forwarded query flags, separate lower collision flags,
excluded handles, and the current closest-contact fraction. It does not
construct registration lists, animation poses, active permutations or a
dynamic lighting sample.

## Literal traversal and replacement

The original reads each body through the requested handle's low 16-bit slot.
It does not validate or replace the requested salt. The native helper retains
that requested handle in the callback and selected contact while separately
providing the actual slot body. Exclusions compare the requested full handle.
`B67C0` at `B67E1` supplies literal maximum `0x800` to `16B570`; the retained
object pool header independently records 2,048 slots with 12-byte records.
The native slot and cycle banks therefore contain 2,048 entries, and reject
out-of-range low slots explicitly. Tests admit slot 2,047 and reject slot 2,048.

An already admitted initial chain bypasses direct filtering and the primitive
sphere test for every sibling in that chain. Descendants use literal header
type/flags, current body fields and original `11E5E0` sphere admission. The
source visits the child at body `+10` before the sibling at `+0C`. Query flag
`20000` disables children. A target replaces the shared contact only when its
fraction is strictly smaller; equality retains the previous packet. Query flag
`10000000` returns after the first strict improvement.

`168A10` reads header flags `+2`, header type `+3`, body flags `+4`, type-0
fields `+10A`/`+348`, and type-11 field `+12C`. These are current fields, unlike
`1697C0`'s stored spatial primitive flags. No source type or flags are inferred
from a tag or the object's rendering sphere. Descendant broad-phase geometry
uses body `+40`/`+4C`, not cached rendering bounds `+30`/`+3C`.

The target callback represents `1EFB40`/`1EFF40` and `168B40`. It must return the
complete contact in world coordinates from actual collision metadata, active
region bytes and posed WORLD nodes. It receives the whole original query and
must not shorten the segment to the shared closest fraction. The helper copies
the supplied packet without changing its geometry fields. Missing native
metadata, duplicate slots and cyclic attachment banks raise an explicit error;
these guards do not model original malformed-memory behavior.

## Independent original evidence

`tools/audit_object_vector_contact_snapshot.py` reads immutable exported RAM
from `registration-snapshot-restore-20261008`. It runs unchanged `169510`,
`168A10`, `11E5E0` and the complete original posed geometry routines. Every
executed instruction is checked against the supplied XBE SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
Read-only observers record admitted target order and original target results;
no original lookup, geometry or arithmetic service is replaced.

The fixture retains all 202 live owners, all nine actual attachment roots,
and 34 selected diagnostic roots. Its 5,066 cases contain 3,782 original
target outcomes, 1,531 strict replacements and 18,507 exact defined output
words. There are 116 multiple-target cases, 68 low-slot salt aliases and 102
controls with the shared maximum exactly equal to an original target fraction
or its adjacent float. All equal/below controls retain the previous packet;
the adjacent greater maximum admits the same unchanged target.

The frozen CTest supplies recorded original fraction/packed-BSP outcomes at
the native target boundary, then checks exact traversal, replacement and owner
identity. Other contact-packet fields are separately labeled native copy
controls. This test does not establish a native live pose producer.

The snapshot was paused at `12B33E`, tick 12,474. Replay uses its literal GDT,
segment selectors, x87 control word `023F` and MXCSR `1F80`. The original
owner/spatial export banks remain unchanged in the clone. The rays and masks
are declared diagnostic inputs; they are not captured game calls or proof of
successful creation/registration chronology. No guest I/O or advancement is
performed.

## Concrete production gap

`tools/audit_dynamic_object_contacts_snapshot.py` separately executes complete
unchanged `1697C0` over the same actual raw spatial pools, owners, poses,
permutations and tag data. Across 3,792 dynamic diagnostic rays plus 948 static
controls, it finds 157 positive type-4 contacts that differ from the static-only
path: 36 with initial light-query flags `4480000D`, and 121 with retained flags
`04800C0D`. Its 223 protected owner/spatial banks are unchanged. This establishes
that dynamic contact admission is a real source path rather than a harmless
omission, without claiming those diagnostic rays are scheduled light queries.

Current `query_object_lightmap` still calls static BSP/instance contacts only.
A future live caller must compose the exact leaf/cluster order, actual raw
collidable pool scan, the attachment helper and posed target service using
one shared contact packet. It must retain instance-before-object ordering in
each newly visited cluster. Successful native registration, actual current
attachments/WORLD nodes/active region state, and `14A8E0` collision-to-render
attribution plus resident object lightmap sampling remain explicit producers.
No generic host `CollisionWorld` triangles or nearest-shape substitute is
admitted. This module does not activate native dynamic-object lighting.

## Files and validation

- API: `include/halo2/source_object_vector_contact.hpp`.
- Implementation: `src/source_object_vector_contact.cpp`.
- Frozen cases: `tests/fixtures/retail_source_object_vector_contact.tsv` and its
  provenance JSON.
- CPU test: `tests/source_object_vector_contact_tests.cpp`;
  CTest `retail_source_object_vector_contact`.
- Original replay reports: `object-vector-contact-snapshot-original.json` and
  `dynamic-object-contact-snapshot/report.json` under
  `analysis/multiplayer-shading`.

Private normal and ASAN/UBSAN/leak CTest runs pass all 5,066 cases and 18,507
defined source output words. Validation is CPU-only. The original session,
evidence disks, foreign processes and production renderer/runtime are untouched.
