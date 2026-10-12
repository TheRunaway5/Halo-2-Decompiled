# Power-module retained child cache and support-state owners

The new native owner constructs the actual canister's two retained terrain child agents from real resident convex/BSP geometry, actual layer banks, the source scratch allocator and the source heap. It also reproduces all five observed capsule support-state initializations during player spawn. It does not provide a contact result, solve a collision or install a collider in `SceneObjects`/`SourcePropScene`. The next required chain is complete `300870` per-child processing, `2E1430` type admission, `2E14E0` shape-order dispatch and `2F4BD0 → 30AE40` narrowphase followed by actual contact-manager/solver delivery.

This is an independent extension of [the immutable resident BSP-child freeze](MP_POWER_BSP_CHILDREN_RESEARCH.md). That freeze, its native source and all earlier archives remain unchanged.

## Original execution and repeat

`tools/oracle_mp_power_child_agents_1_1.py` inherits the frozen BSP `ChildTrace`, verifies its 72-file manifest and all ancestor pins, then executes the identical original selected physical-tag initialization, authored Lockout power-module factory, genuine player creation/PVS and recorded ordinary/world/dirty schedule. No new game tick, pose, location, body, shape, activation bit, pair, contact, filter result or successful service is assigned. Its new observer records 38 calls, 21 complete retained states, 7,275 reads and 3,775 writes. The entire preceding BSP-child report and preceding power schedule compare equal before publication. A second independent execution repeats the full report and twelve complete routine listings byte-for-byte.

Source authority remains TU1.1 `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`, and the original resident Lockout cache data. The prior fixture's declared platform/backing, selected initialization, fixed stored game clock and source scheduling boundaries remain in force. This is not a new complete title startup or successive natural frame execution.

The final original report is `analysis/multiplayer/sandbox-source/power-child-agents/final/measured/report.json`, SHA-256 `4e8af24f7dce3311efe71dce6db1bc12cbfee547fc002de5b39d2fe8d8ea3570`. `final/repeat/report.json` is identical. Full packets and state correlation remain in the report; `final/audit.json` additionally exposes the resulting retained rows and the following genuine narrowphase calls.

## Cache ownership and order

Original `2EFC30` receives `ECX = parent agent`, then four stack arguments: own collidable, other collidable, source step/context, child factory. The actual factory is `2EF680`. Its aligned work frame is `((entryESP - 4) & ~15) - 0x3E0`.

| Owner field or temporary | Source ownership |
| --- | --- |
| Agent `+08` | Existing contact-manager identity; passed unchanged to new child agents |
| Agent `+0C/+10/+14` | Retained row pointer/count/capacity word |
| Agent `+20/+30` | Source current query minimum/maximum, written at `2EFD23/2EFD27` |
| Row stride `0C` | `+00` encoded child key, `+04` retained middle word, `+08` child-agent identity |
| Frame `+390` | Actual relative transform from complete `2E0A60` |
| Frame `+40` | Actual 32-byte bounds from the own shape's bounds virtual |
| Frame `+180` | Child-key descriptor, initially backed by frame `+18C`, capacity word `80000080` |
| Frame `+2C` | Original temporary retained-row bank, constructed by `2EF970` |
| Frame `+64` | Child collidable: actual child shape, key, other transform pointer, parent collidable |
| Frame `+80` | Original BSP triangle construction output |

`2EFC7D..2EFCA6` reads the source step scalar at `+08`, multiplies it by imported `46502C`, spills the product, adds the original scalar and passes that actual word to the shape bounds method. The native owner preserves the original x87 spill and SSE operation order. It neither chooses a world margin nor creates a transform.

The actual source `4878B8` byte is zero. This admits the sorted merge path at `2F0017`: original `2EF8F0` unsigned quicksort, temporary bank, key comparison/retention, actual child construction and layer filtering, child allocation, retained-bank allocation/copy, temporary-bank disposal and any original grown-key disposal. The new native cache represents this mode. Nonzero `4878B8` stops at the unmeasured incremental branch `2EFD80`; it is not silently treated as sorted mode. Warm retained-row merge/release and capacity-growth branches are source-listed but have no positive execution in this cold suite. Their real virtual/free dependencies remain required.

The original MOPP emits `2000081D,20000813`; quicksort produces `20000813,2000081D`. The actual canister parent agent `0601ADE0` changes from zero retained rows to:

| Encoded key | Actual child identity |
| --- | --- |
| `20000813` | `0601AAE0` |
| `2000081D` | `0601A5C0` |

The real player parent agent `0601AD60` has an empty query and retains zero rows. The native comparisons execute both complete `2EFC30` calls. Row `+04` happens to be zero in these actual buffers; the owner does not initialize it or infer that future allocations are zero-filled.

## Filter, allocator and child constructor

`1C6C30` follows real collidable parent links. Its represented complete body retains the special root/property/shape branch and its required capability virtuals, then passes actual root layer words to `2E5E20` with receiver adjusted by `-0C`. The complete layer leaf rejects matching nonzero upper 16-bit groups or reads the real 32-entry bit bank at filter `+1C`. The native code does not select a positive filter result.

`2EF970/2EFA10` retain the exact 20-byte temporary descriptor and original 16-byte-aligned scratch allocation/release stores. Overflow and nonstandard scratch virtuals are required owners. The real mode-2 `148A10 → 22E950` small-block allocator and `148A70 → 22EB20` free path are composed from the earlier frozen native allocator; larger/unavailable allocation owners remain explicit.

The complete dispatch/constructor chain is:

1. `2EF680` reads both real shape type virtuals, indexes the original table at `step[0] +18C + ((typeA << 5) + typeB) *4`, and invokes the actual entry.
2. Actual `2E13B0` allocates `100` bytes with category `1A`, stores size word `+04=0100`, and calls `2F48A0` with **own collidable, child collidable, contact manager**. It deliberately omits the step argument.
3. `2F48A0` stores the real manager at `+08`, installs initial vtable `419288`, produces the actual relative transform and initializes support state at `+0C`. Its type-5 `305650` branch remains a required external owner.
4. It writes `+18=0` and `+2C=BF800000`. `2E13B0` then installs vtable `4188D0` and writes `+F0=0`.

The remaining bytes of the 256-byte agent are retained. The native constructor does not clear the whole allocation or copy a recorded agent. Complete `307FE0` returns the source null-agent singleton `487AC4` when the original caller actually chooses that branch; it is not a contact-success default.

## Actual support arithmetic

Complete `305560` uses the source shape center/support methods and the original `2E0BA0/2E0CC0` transform arithmetic. The represented source methods are wrapper center/support `2E22A0/2E22B0`, convex SoA center/support `3037C0/3037F0`, capsule support `2E42A0`, and BSP triangle center/support `2E4360/1F1D20`. The source wrapper adds its real offset only after the actual dot test and restores the support vector's fourth word. Convex selection uses the original compressed jump table, strict x87 comparisons and original tie ordering. Triangle selection preserves its source scan and strict comparison. No normalization, approximate support point or replacement shape is used.

Capsule `2E42A0` spills the dot of `(second - first)` with the actual direction and performs original `FCOMP; TEST AH,5; JP`. Only ordered less selects the first stored endpoint; greater, equal and unordered select the second. Its original encoded fourth word is `3F000000` or `3F000010`. Both endpoint branches execute in the five actual spawn cases. Masked FP status effects are not claimed as Xbox hardware parity.

`305560` writes the two source support halfwords, `FFFF/FFFF`, bytes `01/01`, and the actual shape `+3C` returns. It leaves support-state bytes `+0C..+0F` untouched. Actual canister support records begin `08000600FFFFFFFF0101FFFF` and `0D000300FFFFFFFF0101FFFF`; these halfwords come from the actual returned point words, not an assumed triangle ordinal.

## Native validation and import seam

New files are `include/halo2/physics_child_agents.hpp`, `src/physics_child_agents.cpp` and `tests/physics_child_agents_tests.cpp`, with only localized Systems source/strict-FP/CTest registration. The module consumes `SourcePhysicsContactMemory`, actual retained pointers/banks, source FP inputs and explicitly required original services. It can replace the `2EFC30` owner once the caller possesses those live banks and real current body/shape/transform state. It is not a snapshot replay adapter.

`tools/export_mp_power_child_agents.py` exports 36 complete original calls, full before/after buffers and 1,453 ordered owning writes. Its expected bytes come solely from original reads/stores and retained snapshots. The independently exported fixture files are identical, SHA-256 `06fc747f744a388500587f31424b1384f7957c9219361d4e93f389492fa14c18`. The compact format retains `C` ABI rows, `M/A` complete memory spans and `W/WS` literal source writes; ABI saves/SEH bookkeeping are excluded, while actual owning stack outputs are retained.

Optimized execution, independent fixture repeat, ASan/UBSan with leak checking, and a newly compiled/linked production `original_physics_child_agents` test pass. They compare 42,608,247 bytes and all 1,453 writes. The production binary SHA-256 is `df1d9971dffa1e827c009796852293a7fa0d5c2df0fca7a9e565c018cfbe6993`. The sanitizer instruments the new module and all actually used native BSP/current-bounds/allocator producers. `final/native-artifacts` retains exact sources, binaries, production library, flags/link command and localized CMake patch.

`tools/research_mp_power_child_agents_correspondence.py` proves 26 complete original bodies against authenticated TU1.5. Nonunique sort/bank prefixes are resolved through their actual paired direct callers. Short shape virtuals are anchored through original paired wrapper/triangle/physical-tag initialization vtables. The complete report SHA-256 is `b8d9560f13f4f92c307f59ac57c0f3a591e82f09f0110f3d7210e461f2c23237`. This is static correspondence, not new TU1.5 gameplay execution. Native `_1_1` global/virtual/instruction bindings still require an explicit TU1.5 storage/address adapter.

The real next original path is `300870 → 2E1430` at `3009C9`, then actual child virtual `+20 → 2E14E0` at `3009FD`, then `2F4BD0` at `2E17E8` and `30AE40` at `2F4D05`. The source report retains this genuine call tree. Those contact/solver outputs are not exported as native callback results or used as live contacts. Live player-versus-canister collision, motion response, incoming damage, destruction and explosion remain open until their actual owners are composed.
