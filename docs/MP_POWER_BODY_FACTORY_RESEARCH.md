# Power-module retained rigid-body and convex-shape factory

This slice implements the actual crate's initial **1DAAC0→318100→320CC0 mode7**
construction and its source-PVS **1DAAC0→318100→317EA0→321320 mode4** rebuild.
It also implements the selected original physical-model initializer's actual
convex shape, vertex-bank and plane-bank stores. It creates neither a substitute
box collider nor a successful world/contact service. The immutable earlier
body-state, contact lifecycle and power-module schedules remain unchanged.

The isolated native module is
[physics_body_factory.hpp](../include/halo2/physics_body_factory.hpp),
[physics_body_factory.cpp](../src/physics_body_factory.cpp), with
[original comparisons](../tests/physics_body_factory_tests.cpp). CMake adds
only that source and `physics_body_factory_tests` / `original_physics_body_factory`.
`SourcePhysicsContactMemory` must address actual retained allocator/cache/body
storage. Missing backing fails; an adapter cannot return a captured body address.

## Authority and measured scope

Original TU1.1 executable SHA-256 is
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
The newly supplied authenticated u5English executable is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Untouched instruction bytes are authority; annotation labels/pseudocode are
navigation only. No guest pose, body, admission, contact, successful allocator,
clock or queue result is added. This uses the frozen source preparation and
actual authored Lockout power-module row1 and selected physical tag. It retains
the original bounded clock and platform preparation limits; it does not run
full title startup or repeated physical settling.

The new [factory oracle](../tools/oracle_mp_power_body_factory_1_1.py) selects
only the actual returned crate physical-component pointer's two1DAAC0 calls.
The source allocates/reuses its actual A0 body, constructs initial D0 motion,
and reconstructs active110 motion. The prior factory outputs, **all404 earlier
body-state calls/reads/stores/states**, and the six original power-world phases
compare exactly to their freezes. The independent new repetition has identical
61calls,39whole retained states,695reads and754stores. All35complete factory
listings repeat byte-identically. The separate
[shape oracle](../tools/oracle_mp_power_shape_factory_1_1.py) has41reads,
23stores, one complete initializer and two complete listings, also independently
identical. Its original return destination is the selected parent fragment's
exclusive11DED1 stop; observation at that stopped destination supplies no guest
return or execution. These reports are
[factory measured](../analysis/multiplayer/sandbox-source/power-body-factory/final/measured/report.json),
[factory repeated](../analysis/multiplayer/sandbox-source/power-body-factory/final/repeat/report.json),
[shape measured](../analysis/multiplayer/sandbox-source/power-body-factory/final/shape-measured/report.json),
[shape repeated](../analysis/multiplayer/sandbox-source/power-body-factory/final/shape-repeat/report.json).

The [audit](../tools/audit_mp_power_body_factory.py) checks **2022 original
instruction rows**, every complete later-image body, source repetitions,
fixture repetition, and all115 prior manifest pins. No new source-success
service is inserted. [Audit output](../analysis/multiplayer/sandbox-source/power-body-factory/final/audit.json).

## Actual resident convex shape

Original selected1EE380 resolves physical-model datumE7090594 through the
actual4EB114 tag table; its current model1144bytes are retained whole.
The actual cold model has one rigid row at`+38`, one convex row at`+70`,
four48-byte packed vertex rows at`+78`, and fourteen16-byte planes at`+80`.
The unsupported sphere/capsule/other convex/compound banks have zero counts.
Native admission checks that actual supported model scope; another positive
shape bank or a different rigid-row selector requires its owning initializer.

The rigid row's packed selector`00000004` means discriminator4/index0.
Original1EE8F0 resolves it through model`+74`, stride100, offset20; original
1EE3CF stores the resident pointer into rigid row`+38`. It preserves all
vertex/plane bytes and all untouched physical-model bytes.

| Original site | Actual ownership |
|---|---|
|1EE630|resident shape`+8` = its containing authored convex row|
|1EE633|shape`+30` = actual packed vertex bank; stride30, count/capacity at`+34/+38`|
|1EE636|shape`+D4` = actual plane-vector bank; stride10, count/capacity at`+D8/+DC`|
|1EE63C|shape`+0` = resident VT45D188|
|1EE642|shape`+6` = persistent reference-count word80|

The actual vertex capacity4 selects the external bank. The source-listed
inline-bank path for capacity≤3 is ported but unexecuted here. The API is
selected **cold initialization**, not general map reset/disposal or rerunning
an already converted rigid selector. It reads the real resolved tag pointers;
it neither copies frozen addresses into a live tag table nor generates a hull.
[Complete initializer](../analysis/multiplayer/sandbox-source/power-body-factory/final/shape-measured/001ee380.asm),
[selector](../analysis/multiplayer/sandbox-source/power-body-factory/final/shape-measured/001ee8f0.asm).

Actual VT`+14→9D8E0` returns type8. Actual VT`+18→303E30→1D1C00` consumes
shape`+10/+20`, the exact source transform, margin plus shape`+C`, imported
absolute-value/sign masks, and emits its32-byte local shape AABB. This is the
convex shape's original bounds query, **not a box collision substitute**.
`1D1C00` preserves full four-lane source MULPS/ANDPS/ADDPS/XORPS ordering:
extent = (abs(X product)+abs(Y product))+(abs(Z product)+margin);
center = (X product+Y product)+(Z product+translation). It stores positive
extent, negative extent, then adds the actual center to each. The source
identity transform469340 is imported original data, not a fabricated pose.
[Wrapper](../analysis/multiplayer/sandbox-source/power-body-factory/final/measured/00303e30.asm),
[full query](../analysis/multiplayer/sandbox-source/power-body-factory/final/measured/001d1c00.asm).

## 1DAAC0 packet and 318100 ownership

`source_create_physics_rigid_body_1_1` consumes the real physical-component
record and **14original caller words**, retaining byte-field semantics.
The component`+8` datum selects its actual object-header row and body pointer;
object`+C0`bit5 can force motion7. Filter identity uses low16 datum index+1,
or802 for caller kind10. Caller flag low8 chooses filter kind2; otherwise the
original caller kind is retained. This is independent of native grounded state.

| Caller index | Owning output/consumer |
|---|---|
|0|actual resident shape→info`+4`|
|1/2|low8 gates original transformed/wrapped-shape queries; actual type8 has no unwrap|
|3|four-word vector→info`+90`; mode4 consumes it through31FD50|
|4/5|literal caller words→info`+AC/+B0`, then body`+4C/+50`|
|6|actual48-byte matrix→info`+60/+70/+80`;321390 reads its three diagonal words|
|7|mass word→info`+A0`;321050→321220 owns its reciprocal|
|8|motion low8→info`+B4`, subject to original object flag gate|
|9|actual40-byte source matrix;2E0860→1DC750 quaternion, matrix`+30`→info`+20`|
|10/11|filter kind and low8 source flag|
|12/13|info`+A4/+A8` then mode4 motion`+14/+18`|

Body-info initializer317890 and entity312410 reuse the previously frozen
native owners. ActualA0 allocation goes through original mode2 allocator and
small-block class tables/freelists; the allocation result is the identity.
`318100` installs VT41A950 and then creates the selected motion. It does not
zero unowned padding. Wrapped/compound shape classes, positive temporary
property-bank cleanup, and other motion selection branches are explicit missing
owners rather than successful defaults. No such branch executes in these cases.
[Complete1DAAC0](../analysis/multiplayer/sandbox-source/power-body-factory/final/measured/001daac0.asm),
[complete318100](../analysis/multiplayer/sandbox-source/power-body-factory/final/measured/00318100.asm).

Both actual motion constructors execute317A30: zero velocities and source
fields, copy actual position to`+60/+70/+B0`, copy quaternion to`+30`, construct
its rotation matrix at`+80` with378AB0, and preserve all unowned allocation
bytes. Mode7 runs31F970 then320CC0, preserves their ordered VT/field stores,
and binds body`+14` to actual motion`+60`. Mode4 runs321320, initializes its
four reciprocal diagonal fields, extra velocity/history banks and signed scalar,
then317EA0 dispatches actual inertia, center and mass owners:
321390→31FD50→321050→321220. The center update calls exact2E0BA0 with its
source-aliasing output at motion`+70`, preserving actual`+7C`word. There is no
world-velocity-to-mode substitute or chosen shape/current pose.

The direct317EA0 native allocator-selector API admits **only its measured
mode4** branch. Mode7 is the independently measured **318100 direct** allocation
path; the unmeasured direct317EA0 mode7 tail is not supplied as a successful
shortcut. Source-listed other modes remain unavailable.
[Mode7](../analysis/multiplayer/sandbox-source/power-body-factory/final/measured/00320cc0.asm),
[mode4](../analysis/multiplayer/sandbox-source/power-body-factory/final/measured/00321320.asm),
[selector](../analysis/multiplayer/sandbox-source/power-body-factory/final/measured/00317ea0.asm).

Mode4 applies the actual info linear/angular packets through31FE90/31FEA0,
then constructs its real60-byte auxiliary via317BF0→31AC90→31ABD0. That owner
uses the exact shape bounds to produce its squared half-extent scalar.1DC7E0
increments the new auxiliary reference, stores actual body`+54`, and the caller
drops its temporary reference. Actual cold body`+44/+54` are zero. Positive
wakeup, old auxiliary destruction, shared auxiliary mode1, zero-ref deletion and
other positive lifetime branches remain explicit unavailable owners.
Finally317D10 computes the motion radius from the genuine local AABB and actual
stored vector, with original SSE sums/square roots followed by x87 half-product
and addition. This radius is not the backend's current quantized world bounds.
[Auxiliary](../analysis/multiplayer/sandbox-source/power-body-factory/final/measured/0031ac90.asm),
[shape scalar](../analysis/multiplayer/sandbox-source/power-body-factory/final/measured/0031abd0.asm),
[radius](../analysis/multiplayer/sandbox-source/power-body-factory/final/measured/00317d10.asm).

## Floating point and fixture boundaries

Actual captured constructor math uses x87CW027F and MXCSR1F80.378AB0 retains
x/y doubled values extended, spills doubledz and its products to binary32,
retains y*w/z*w extended until their owning output stores, and preserves exact
subtraction order.2E0860 positive-trace branch and its original normalization
are measured; the fully source-listed negative-trace matrix branch is ported
but unexecuted. No negative matrix/quaternion pose is invented to increase
coverage. Masked FP status and unmasked exception delivery are not represented.

1DC750 executes original RSQRTSS then exact original Newton operation order.
The required estimate callback has **no success default**. The first comparison
uses the actual measured original instruction operands/results only in its
**test adapter**, not a live body/shape/allocator callback. Four further
normalizer/full1DAAC0 comparisons use the real `source_host_x86_rsqrtss_estimate`
provider and reproduce every final source buffer/store without a captured
estimate. For actual input3F800000 the measured emulator estimate is3F800000;
this host's real instruction estimate is3F7FF800. Both source refinement paths
produce3F800000 here. This bounded agreement **does not establish Xbox estimate
parity or unmeasured inputs**. [Actual host probe](../analysis/multiplayer/sandbox-source/power-body-factory/final/host-rsqrt-boundary.json).

The [exporter](../tools/export_mp_power_body_factory.py) uses original measured
inputs/reads/stores and imported executable virtual-table bytes. It emits no
native-derived expected geometry. Its16cases compare **17,725,312 retained
bytes and865 ordered portable stores**, plus4real-host compositions. Internal
call-stack/FS-chain stores are excluded. Source-generated info, quaternion and
AABB outputs remain included. AABB scratch aliases earlier internal call frames:
its uninitialized entry bytes are delivered from the **actual owning query's
entry**, not asserted to equal the enclosing1DAAC0 frame's earlier stack bytes.
Raw reports preserve all original stack/read/store observations. The portable
fixture does not reproduce exception/call-frame plumbing or claim those garbage
bytes are geometry or backend state. Export/re-export is byte-identical.
[Fixture/schema records](../analysis/multiplayer/sandbox-source/power-body-factory/final/exported-owner/fixtures.tsv),
[export report](../analysis/multiplayer/sandbox-source/power-body-factory/final/exported-owner/report.json).
Normal O2/Wall/Wextra/Werror, ASan+UBSan and the production-library CTest pass.
[Normal](../analysis/multiplayer/sandbox-source/power-body-factory/final/validated-native-normal.txt),
[sanitizers](../analysis/multiplayer/sandbox-source/power-body-factory/final/validated-native-sanitizer.txt),
[CTest](../analysis/multiplayer/sandbox-source/power-body-factory/final/ctest-normal.txt).

## Authenticated1.5 correspondence

A fresh address-free instruction scan followed by complete instruction/internal
branch-topology comparisons establishes **all37actual owner bodies**, including
short leaves resolved by the actual paired callers/constructed virtual tables.
Every listing's bytes come directly from the original authenticated images;
user annotation names are never authority. The paired switch topology/data and
absolute memory operands remain in the original report.
[Tool](../tools/research_mp_power_body_factory_correspondence.py),
[complete report](../analysis/multiplayer/sandbox-source/power-body-factory/final/later-correspondence/complete-report.json).

| TU1.1 | Authenticated1.5 |
|---|---|
|1DAAC0 /318100|1DF380 /31BC40|
|1EE380 /1EE8F0|1F2C70 /1F31E0|
|320CC0 /31F970 /317A30|324810 /3234F0 /31B570|
|317EA0 /321320|31B9E0 /324E90|
|321390 /31FD50 /321050 /321220|324F00 /3238B0 /324BC0 /324D90|
|2E0860 /1DC750 /378AB0|2E4350 /1E1010 /37C5D0|
|303E30 /1D1C00 /9D8E0|307E30 /1D64C0 /AD390|
|31AC90 /31ABD0 /317D10|31E820 /31E760 /31B850|

Actual constructed VT pairs are41B8F0→41F360,41B770→41F1E0,
41B7F0→41F260,45D188→461FE0; corresponding slots are retained in the report.
The native API is deliberately **TU1.1-addressed storage**, reusing the frozen
1.1 entity/allocator module. It has no implicit1.5 virtual/global address remap.
This static correspondence is not later-image gameplay execution or proof that
1.5 pointers may be passed directly to a1.1 API. Image-bound provider/address
ownership must be implemented before that use.

## Concrete live bridge after this slice

A real Scene/Simulation bridge can now install the **resident convex-model
import, body-info builder, retained A0 rigid body, initial mode7 motion, PVS
mode4 motion/auxiliary, shape-query/matrix/velocity stores** using actual owned
storage and identities. It must consume actual1D95E0-produced arguments and
real allocator state; it cannot replay these fixture packets or fixed addresses.
The slot/world wrapper still owns shape temporary-reference release, body-slot
commit, PVS rebuild/disposal, property installation and world insertion. These
are outside this module; the same old source world schedule remains evidence.

The next exact native ownership gate is **1D95E0**'s complete argument and
body-slot result/shape-temporary lifecycle, composed with the enclosing
physical component/body-slot lifecycle and genuine1C7050 world insertion.
Existing native contact-row/lifecycle consumers require actual backend entity,
encoded property keys, contact callback admission and manager packets. This
slice supplies none of those as a successful default. A real world/broadphase
and solver implementation/provider must deliver them from these same retained
convex vertices/planes/bodies. Current quantized bounds must use the existing
backend owner, never the local shape AABB as a replacement. The source-authored
crate root, unsorted registry membership and ordinary/world/dirty/new-object
ordering remain unchanged.

**Positive player-versus-crate blocking, admitted incoming damage/destruction,
explosion and continuous settling remain open.** This is a verified factory
stage that can feed a genuine backend; it is not a claim that the screenshot's
missing collision/explosion is fixed or that the factory is live in Scene yet.

The independent freeze manifest is
[manifest.json](../analysis/multiplayer/sandbox-source/power-body-factory/final/manifest.json).
Earlier freezes and playable releases are untouched.
