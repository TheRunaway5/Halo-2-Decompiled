# Retained canister resident BSP child query and triangle production

This slice executes the unchanged original Lockout power-module factory, player/PVS and ordinary world schedule from the frozen current-broadphase suite. It ports the actual resident BSP query and terrain child construction consumed by `300870 → 2EFC30`. It does not supply contacts, child keys, geometry, admitted pairs, poses or a solver return. The native world/contact/solver bridge remains incomplete; this does not claim that the live canister collides or explodes.

All new evidence is under ignored `analysis/multiplayer/sandbox-source/power-bsp-children/final/`. Earlier evidence, including the current-broadphase manifest `b82760c074e4658932bf9e628b2a7fd1f1b1cd52d71e1c8a994e5b04980ccd9c`, remains unchanged. `tools/oracle_mp_power_bsp_children_1_1.py` checks all 428 prior manifest entries and compares the entire prior current-broadphase and power schedule outputs exactly. Guest instructions and guest fields/results are unchanged. The new observer checks both the return address and original callee stack-pop ownership when correlating recursive `305E40` calls.

## Original ownership and order

The retained canister agent calls `2EFC30` at `3008FB`. That owner calls `2E0A60` at `2EFC75`, current wrapper bounds `2E2380` at `2EFCA6`, and resident BSP shape query `1F2420` at `2EFD5E`. The existing real current-broadphase/scheduler produces the collidable, world, step, shape, matrix and output buffers. This extension observes the same original source preparation; it does not replay prior contacts in a native callback.

`1F2420 → 2E5BC0 → 3066E0 → 305E40` produces a real encoded child-key bank. `1F26B0 → 1F2DB0` removes disabled keys in place without sorting. The canister's world query emits **`2000081D`, then `20000813`**. The subsequent original `2EFC30` calls `2EF8F0` at `2F0039` before child construction; that separate owner presents `20000813`, then `2000081D`. The MOPP output order must not be confused with the later sorted child-agent order. The current player query returns zero keys naturally.

Original `1F3320 → 1F2DB0 → 1F16F0` constructs the real terrain child from the resident collision BSP. Original child filter `1C6C30 → 2E5E20`, child-agent factory `2EF680 → 2E13B0 → 2F48A0` and subsequent support/narrowphase/contact owners remain distinct. Existing source execution reaches them and the transient crate contact, but this native slice does not substitute their outputs.

Source listings and per-call packets are `final/measured/*.asm` and `final/measured/report.json`. `final/audit.json` correlates original query order, triangle packets and source opcode visits. `final/later-correspondence/complete-report.json` pairs complete original bodies with authenticated TU1.5; this is static correspondence, not TU1.5 gameplay execution.

## Native API and exact stored inputs

The new interface is `include/halo2/physics_bsp_children.hpp`, implementation `src/physics_bsp_children.cpp`. It uses the existing literal source-memory/ordered-store interface. Missing backing fails in the adapter. All source-address buffers must have genuine retained owners. Neither the API nor its tests install captured geometry in live Scene state.

| Original owner | Native function | Required state and produced state |
| --- | --- | --- |
| `2E0A60` | `source_physics_relative_transform_1_1` | Two actual 64-byte transforms and output backing. Exact four-lane transpose/multiply/add/subtract operations; reloads the left transform after the four output stores, preserving source aliasing. |
| `305E40` | `source_query_physics_mopp_1_1` | Original context, 14-word classifier, resident bytecode, real output bank, actual temporary backing/entry ESP. Original recursive traversal, signed integer classification, shifts, key-base arithmetic and ordered writes. |
| `3066E0` | `source_initialize_physics_mopp_query_1_1` | Actual MOPP header, 32-byte current bounds, query context, output descriptor and source stack backing. Original x87 subtract/multiply/float spill/FISTP under caller FP control. |
| `2E5BC0` | `source_query_physics_bsp_mopp_1_1` | Shape `+10` supplies resident MOPP header. Original aligned query-context/temporary placement. |
| `1F2DB0` | `source_physics_bsp_child_enabled_1_1` | Exact encoded key and original switch table. Class1 reads collision-BSP surface flags and optional switch bank. Object classes read retained datum/header/remap banks. Instanced switch `185AA0` requires its actual provider. Only AL is represented. |
| `1F26B0` | `source_filter_physics_bsp_children_1_1` | ESI is the source output descriptor. Original ascending scan, count decrement and ascending shift stores. No chosen filter success or sorting. |
| `1F16F0` | `source_construct_physics_bsp_triangle_1_1` | Actual collision BSP, type byte, owner word, surface index, encoded key, optional transform and output storage. Exact source surface-edge cycle and vertex bank, header/refcount/margin/metadata and vertex ordinal words. |
| `1F3320` | `source_physics_bsp_child_shape_1_1` | Measured class1 terrain path and source disabled/null paths. Other classes stop at the unported `1F337E` branch. |
| `1F2420` | `source_query_physics_bsp_children_1_1` | Measured complete ordinary-terrain path, composing real MOPP query/filter. Instanced class2 stops before the `1F2482` branch stores. |

The actual source FP controls are x87 `023F` and MXCSR `00001F80`. The module retains explicit caller controls and source arithmetic order. Unmasked FP effects and architectural exception/status parity are unavailable. Finite authored inputs and represented masked outputs are measured; this does not claim universal floating-point or Xbox hardware parity.

`3066E0` writes context `+00` output descriptor and `+30` resident MOPP header. Its source quantized maxima are `+10/+14/+18`; minima are `+20/+24/+28`. Each coordinate computes `(bounds - header-origin) * header+0C`, spills to binary32, reloads, FISTP-converts under caller rounding and subtracts one. Initial classifier `[0..2]` are signed high words of maxima plus one, `[4..6]` signed high words of minima, and `[8..13]` are zero. Unwritten words 3 and 7 retain actual backing. The recursive traversal owns the subsequent classifier changes. The output descriptor is `{bank,count,capacityFlags}`; capacity masks bit31. At capacity, original `2DF780` bank growth uses the existing real allocator owner, never an allocated-address fixture. No measured query grows its 128-entry bank.

`1F16F0` reads the collision BSP surface bank at `+2C` (8-byte rows), edges at `+34` (12-byte rows), and vertices at `+3C` (16-byte rows). Surface `+02` is the first edge. Edge signed `+0A == surfaceIndex` selects side1, otherwise side0; side chooses the vertex at `+side*2` and next edge at `+04+side*2`. It follows the original cycle until the first edge. Output `+14` is the vertex count; vertices start `+20`, stride16. The constructor writes zero to each vertex's fourth lane, then overwrites it with `3F000000 | ordinal`. Source margin comes from original global `486570`; surface material/flags are stored at `+18/+1A`; encoded key at `+08`. Output untouched bytes remain retained. Actual terrain constructors use EDI=NULL; non-null transform arithmetic is source-listed only and is not claimed as positive execution coverage.

## Reproduction and validation boundary

The source oracle and its independent process repeat must produce byte-identical reports and all nine instruction files. The exporter consumes only their original packets, original read/store timelines, original before/after buffers and original XBE table bytes; it uses no native-derived expected geometry. ABI pushes/saved registers are excluded, while owning classifier, transform, triangle, descriptor and arithmetic spill stores remain literal. Every represented full backing span and ordered owning store is compared by `tests/physics_bsp_children_tests.cpp`.

Run the independently frozen oracle into a new output directory, export it with `tools/export_mp_power_bsp_children.py`, then run `physics_bsp_children_tests` with the exported `fixtures.tsv`. The exporter supports `--call`/`--phase` for bounded diagnostics. Complete validation is registered as CTest `original_physics_bsp_children`; the missing evidence fixture skip code is 77. The final manifest pins source tools, original/repeat evidence, correspondence, tests, source files, validation logs and freshly linked binaries.

The initial development observer closed multiple recursive frames sharing one return address. Its report, initial fixture export and failing native logs are retained separately under `draft/observer-return-initial` and `draft/export-initial`. The corrected observer additionally matches original entry ESP plus the exact return-pop amount. No guest result changed. The final independently repeated report is the authority.

The completed comparison covers 209 original calls: 111 MOPP traversal calls, 11 ordinary BSP queries with matching wrappers/initializers/filters, 16 eligibility calls, 9 child-shape calls, 9 triangle constructors and 20 relative transforms. Original source reports and all nine listings repeat byte-identically. Native optimized and independent-fixture repeat, ASan/UBSan with leak detection, and the freshly linked production CTest all pass: **247,364,134 full retained bytes and 11,645 ordered owning writes**. Nested owner fixtures intentionally compare overlapping original call intervals. The source instruction audit checks 1,201 listing rows and all 393 observed read/store instruction sites.

## Authenticated TU1.5 correspondence

All nine complete bodies match normalized instructions, internal branches and switch topology against the independently authenticated owned TU1.5 image. Original code/data operands and instruction listings are pinned individually; no annotation label supplies semantic evidence.

| TU1.1 | TU1.5 |
| --- | --- |
| `1F2420` | `1F6D10` |
| `2E5BC0` | `2E9670` |
| `3066E0` | `30A1E0` |
| `305E40` | `309940` |
| `1F26B0` | `1F6FA0` |
| `1F2DB0` | `1F76A0` |
| `1F3320` | `1F7C10` |
| `1F16F0` | `1F5FE0` |
| `2E0A60` | `2E4550` |

The native API retains TU1.1 source-address storage/vtable protocol. TU1.5 correspondence alone does not construct a TU1.5 live memory provider, relocate retained pointers or prove a TU1.5 dynamic query execution.

## Minimum remaining live bridge

This slice can replace the actual ordinary resident BSP query, encoded child eligibility/filter, relative-transform and terrain triangle construction boundaries. It needs current matrices/bounds from the genuine existing body/world producers and resident collision/MOPP banks from actual cache import. The output is source geometry, not a contact.

The next native gate is the complete `300870/2EFC30` orchestration over these real producers: current query bounds and key sorting/cache lifetime, per-child filtering, actual child-agent allocation/retention and narrowphase virtual dispatch. Those must then produce the actual contact manager/solver buffers, retained constraint/island delivery and current body scheduling before Scene/Simulation may claim live canister collision. Incoming damage, destruction/explosion and their effect owners remain additional required source owners. No AABB, capsule/SAT fallback, borrowed contact snapshot or success callback closes those gates.
