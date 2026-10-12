# Original physical-contact record construction

The new isolated native module executes the complete measured **TU1.1 1D2DF0 /
TU1.5 1D76B0** contact-row constructor against caller-owned source storage. Both
unchanged original bodies reproduce the actual Lockout power-module/BSP contact
row byte for byte. The native comparison matches both originals' complete
72-byte row, both body/property buffers, and all14 ordered writes per call.
Normal and ASan/UBSan checks pass. No live collision, contact allocation, solver,
point/normal update, damage or destruction consumer is connected by this change.

[Native interface](../include/halo2/physics_contact_record.hpp),
[implementation](../src/physics_contact_record.cpp),
[differential test](../tests/physics_contact_record_tests.cpp),
[original replay/exporter](../tools/export_mp_physics_contact_record_1_1_to_1_5.py),
[auditor](../tools/audit_mp_physics_contact_record.py).

## Original inputs and executable authority

The caller input is the complete actual **1D3DF0→1D2DF0** entry from the immutable
[power-module source-world/contact freeze](MP_POWER_MODULE_CONTACT_SCHEDULE_RESEARCH.md),
manifest SHA256
`948723b3077688265b4392ef130c679deb4744fdb41c6558f260520150d9185a`.
That source-created row is0601A720, the actual box body is0601A480, and the
original Lockout BSP body is06005250. No collision point or success flag was
written to admit that preceding source execution. This new replay extracts only
its literal constructor-stage row/body/property buffers and exact caller ABI;
it does not claim a fresh world contact in either replay instance.

Original TU1.1 XBE SHA256 is
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
The separately authenticated owned1.5 English XBE SHA256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The exporter locates a unique exact16-instruction prefix in the new original
text, then verifies all98 instructions, internal branch topology, and every
instruction offset. The only instruction-byte difference is the absolute
encoded-key lookup global **52625C→52BE70** at **1D2E62→1D7722**. The new
entry **1D76B0** is independently verified as an actual direct-CALL target.
No user annotation labels/comments establish the mapping.

Both original constructors run from their real image bytes with the same
captured whole source buffers and registers. Their replay stack address is an
explicit ABI container, not source game/world storage. There are no supplied
services. Defined EAX remains the row address, and original stack cleanup is40
bytes including the return address (`ret0x24`). Whole output buffers and all
source store instructions agree; excess original stack words are excluded from
the nine-argument constructor ABI. [Original measured report](../analysis/multiplayer/sandbox-source/contact-record/measured/report.json).

| Owning input | Actual original value |
|---|---|
| EAX output row / ECX contact-index word / EDX key | `0601A720 / 00000000 / 0000081D` |
| Stack argument0, encoded counterpart key | `2000081D` |
| Arguments1/2, own/counterpart body pointers | `0601A480 / 06005250` |
| Argument3, fallback component datum | `FFFFFFFF` |
| Argument4, flag word (source consumes low8) | `3F800000` |
| Arguments5/6, opaque copied scalar words | `3F5FE897 / 3EFAD3E8` |
| Arguments7/8, material words (low16) | `B7850056 / 3F800068` |

The actual box body properties are ordered
`2003→0,2001→E1680000,2002→0`; the BSP body has only `2001→FFFFFFFF`.
These are complete captured original property arrays. Body`+30` is their
pointer; body`+34` is the signed count. Each entry is eight bytes, with a32-bit
key and32-bit value. The source scans in order and retains the first matching
key. A genuine exhausted scan or signed nonpositive count yields the original
zero value. Missing native storage is an error, never an empty property set.
Body pointers, encoded counterpart keys, component datums and native scene
handles remain separate identities.

## Complete owning stores

The native API preserves the initial row bytes and performs only the original
stores in source order. Scalar words are copied as bits: no float arithmetic,
normalization, guessed friction/mass or contact threshold exists in this body.

| Row offset | Original constructor owner |
|---|---|
| `+0C`, first store | EDX source key |
| `+00`, two bytes | ECX contact-index low16 |
| `+12`, two bytes | Argument8 material low16 |
| `+04` | Argument0 encoded counterpart key |
| `+08` | Source constant `FFFFFFFF` retained impact reference |
| `+10`, two bytes | Argument7 material low16 |
| `+14` | Counterpart body property2001 value |
| `+38` | Source positive-zero word |
| `+18` | Component resolution described below |
| `+3C` | Argument5 scalar word |
| `+44`, one byte | Argument4 flag low8 |
| `+40` | Argument6 scalar word |
| `+45`, one byte | Own body property2002 low8 |
| `+46`, one byte | Counterpart body property2002 low8 |

Bytes`+02..03`, `+1C..37` and`+47` remain literal caller storage. In particular,
the later contact point/normal/distance slots are not fabricated or cleared by
this constructor. Their actual producer remains **1D2F10** and its real contact
manager. Full-row comparisons verify those untouched bytes as well.

For argument0=`FFFFFFFF`, `+18` receives argument3. For other keys, source
bits29..31 select the branch: classes3/4 read the image-specific lookup pointer
and indexed record; other classes store source `FFFFFFFF`. The real measured
key is class1, so that lookup is unexecuted. The native module **throws at the
class3/4 boundary** after the preceding original stores, retaining partial-write
semantics. It supplies no callback result, component or success default. The
remaining constant/fallback and property-present branches are source-listed;
the original differential covers the actual class1 contact only. Native tests
for rejection are explicitly interface checks, not additional original contact
fixtures.

## Integration contract and remaining owners

`source_construct_physics_contact_record` accepts actual EAX/ECX/EDX and nine
stack words, an explicit image, and a mandatory raw-storage reader/writer.
The optional observer reports exact source instruction, absolute destination,
before bytes and written bytes. The source constructor does not allocate or
register its row. Callers must retain the real physical body/component keys,
property arrays and allocated row bank before invoking it.

The original source sequence is real world collision callback
**27DFE6→1D3DF0**, actual contact-bank allocation/count update, this constructor,
then actual contact manager/point/normal/impact refresh and eventual removal.
The prior freeze measures that complete bounded sequence with the genuine
source-created world. The new native module replaces only the constructor.
It does not borrow those measured contacts for gameplay. A live adapter still
needs the actual world/body/property producer, real solver callback delivery,
bank growth/free, manager virtual outputs and later impact/reference lifecycle.
**SourcePropScene/SceneObjects** do not currently own those producers.
Positive destruction additionally requires actual incoming damage and response
ownership, as documented by the prior source freeze.

Necessary CMake registration adds the single source to`halo2_systems` and the
`physics_contact_record_tests` executable/`original_physics_contact_record`
CTest using the ignored measured fixture. No Simulation/runtime/renderer/scene
consumer, immutable release or existing build directory is changed. No complete
1.5 world execution, continuous settling or Xbox hardware parity is asserted.

## Reproduction and validation

The two independent original runs produce identical reports, both full98-row
instruction listings and the fixture. The auditor verifies all1,254 prior freeze
artifacts, original opcode bytes, whole row/body/property outputs and ordered
store replay. [Audit](../analysis/multiplayer/sandbox-source/contact-record/audit.json),
[validation](../analysis/multiplayer/sandbox-source/contact-record/validation/results.json),
[manifest](../analysis/multiplayer/sandbox-source/contact-record/manifest.json).

```sh
.tools/python/bin/python tools/export_mp_physics_contact_record_1_1_to_1_5.py
.tools/python/bin/python tools/export_mp_physics_contact_record_1_1_to_1_5.py \
  --output analysis/multiplayer/sandbox-source/contact-record/repeat/report.json
.tools/python/bin/python tools/audit_mp_physics_contact_record.py
c++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Iinclude \
  src/physics_contact_record.cpp tests/physics_contact_record_tests.cpp -o /tmp/contact-record-test
/tmp/contact-record-test analysis/multiplayer/sandbox-source/contact-record/measured/fixtures.tsv
```

The separate sanitizer build adds`-fsanitize=address,undefined` and
`-fno-omit-frame-pointer`. Both native executions report **two original
constructors,720 complete bytes and28 ordered stores matched**. Fixture SHA256:
`a172738ed8d37a351b07e77ccb41844cee78db6b36a0b3df23c4c0b96af9f8ee`.
Original measured/repeat report SHA256:
`23ea555b599336f937b75f472a260e59db86ccd38040babd2fe23e1980eff377`.
