# Complete static instance contact fixtures

`tools/audit_instance_spawn_contacts_original.py` executes unchanged retail
`1697C0` over all 21 supplied multiplayer caches. Every original authored spawn
is queried in all six source lighting directions, with the original `.001f`
start offset, and the 17 unique retained Headlong/Triplicate tied rays are added.
The source call uses explicit `04800005` (static BSP and instances), source
FPCW `027F`/MXCSR `1F80`, and a cold all-one breakable bank. It supplies no dynamic
registration, scene lifetime or guessed Havok state.

The whole main BSP traversal and the actual cluster accelerator path execute:
`1DE630`, `244980 -> 2DF760 -> 2DF720 -> 301800 -> 3009E0 -> 244BB0 -> 244C10 ->
1691A0`. Two observers read cluster and callback inputs without replacing any
instruction, register, memory, contact or math service.

The frozen output includes 13,229 original queries:

- 13,212 authored spawn rays from 2,202 placements, plus 17 tied rays.
- 11,420 main-BSP hits, 562 instance hits and 1,247 misses.
- 2,641 original callbacks with cluster, literal key, collector fraction and
  prior owner/type in the recorded order.

`tests/fixtures/retail_instance_accelerator_contacts.tsv` records hit/type,
fraction, actual owner and definition, leaf/surface/oriented plane, flags,
breakable/material and world point. Its provenance contains the pinned XBE and
all map hashes. Full original 96-byte outputs and callback records are retained
in `analysis/multiplayer-shading/instance-spawn-contacts-original/*.jsonl`.
The companion `retail_instance_accelerator_callbacks.tsv` freezes actual
callback order/state separately from the resulting contact.

The production `BspSurfaceLighting::cluster_accelerators` bank decodes literal
cluster `+A8/+AC` data. `original_map_vector_contact` now traverses this bank,
feeding each collision fraction back into the visitor and preserving original
deduplication. A read-only optional observer records the `244C10` entry state
before deduplication. The standalone grammar/visitor tests independently cover
382 authored banks and all 60 reachable opcodes.

The preceding membership-order implementation matched every authored spawn
contact, but produced different owner records for eight of the tied rays.
Its corrected boundary flagged all 17 tied rays unresolved. The actual original
MOPP callbacks select Headlong owner27 for the previously falsely admitted ray,
and Triplicate owner49 for its retained tie; other Headlong rays change order.
These are source-executed outputs, not an assumed global instance order.

`tools/audit_instance_main_ties.cpp` also checks the same 53,334-ray authored
spawn/face census for exact main-BSP/instance equality. It found none. This is
finite coverage and does not prove such ties impossible. The separate explicit
static-equality control in `retail_instance_contact_ties.tsv` remains labeled
controlled state. The known Triplicate116 normal-ray contact still fails
source render attribution; the full-contact regression checks that existing
boundary independently of contact success.

`tests/instance_accelerator_contact_tests.cpp`, CTest
`retail_multiplayer_instance_accelerator_contacts`, compares the production
query directly with every frozen contact and ordered callback. It checks
168,995 contact words and 26,434 callback words (including per-query counts),
with no unresolved tied owner. The callback census includes 135 duplicate
callbacks, 1,826 with an existing main-BSP contact and 121 with an existing
instance contact. Missing source map assets return skip77.

The separate tie regression checks all 17 actual owners over the three admitted
map-query flag paths, plus original endpoint and explicit ordered collector
controls. Controlled copied/moved instances are never inserted into an authored
MOPP by assumption. These CPU tests do not establish live dynamic registration,
breakable changes, runtime lighting-cache scheduling, or physical Xbox execution.
No original guest operation, gameplay advancement or GPU work is needed for
this audit.

Coherent private normal and ASAN/UBSAN/leak-check builds pass all three focused
CTest cases: complete contacts/callbacks, retained ties/endpoint controls and
the existing all-map surface-lighting regression. Validation logs are
`analysis/multiplayer-shading/instance-accelerator-integrated-{normal,asan}.log`;
the hashed build/test summary is `instance-accelerator-integrated-validation.json`
in the same directory. The complete contact fixture SHA256 is
`6da241747851306c728653fbc475a5a742d24553bf289a198e59592a13393d5a`;
callback fixture SHA256 is
`7b9afd7c009c3dd74ade5c2c4970a471d5cbf33ddaafa251270de4e59c2edfd6`.
