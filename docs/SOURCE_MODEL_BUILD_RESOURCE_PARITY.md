# Original model inputs, resources and render cache

`source_model_resources.hpp/.cpp` owns original model/node input resolution,
section resource fallback, render admission and render-cache acquisition over current owned
memory banks. Cache allocation/rebind joins `SourceRenderCachePool`. These
functions are not yet called by the multiplayer gameplay collector. The native
whole `4B5D0` now joins these owners and raw node construction in reference
tests. Live `4B5D0/133790`, resource publication, deep pose/lighting services
and matched original gameplay frames remain required work.

Authority: unchanged `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The oracles use retained RAM SHA-256
`68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e`,
and unchanged `2CBC0/133790/4B5D0` with main context`00547F88` produces six
actual inputs to each model routine. Controlled inputs are individually
identified. No original instruction or callee return is replaced.

## Native owners

| Original | Retained behavior |
| --- | --- |
| `4BCC0`, `B9CE0`, `3D3E0`, `4C0D0` | Parent visibility, actual first-person identity/count, PRT request ordering, LOD threshold short-circuit loads and native prior-cache acquisition, camouflage/shield/function overlays, signed variant/region sort bank and untouched output bytes |
| `3D4F0` | Actual normal object/model/node fields, or current first-person entry with stride`D0C`; cached low byte, signed entry low word, unsigned64-node cap, intermediate count and alias-sensitive output order |
| `137650` | Compact flag, signed LOD group, model/compound count, each non-`FF` section's unsigned node-map byte, wrapped32-bit addition and signed region loop |
| `137800`, `1376C0` | Raw node/skin palette, compact and compound banks, source float order, actual section node maps, resource-null gate and preserved output guards; deep `BD610` pose update remains explicit |
| `4B5D0` | Native admission, per-view cache/LOD reuse, model inputs/resources/nodes, shared arena allocation, first-person chain, lighting-cache join, context copy and owner render stamp; deep pose/sky/PRT/query/conversion remain explicit |
| `3DB00`, `BAF80`, `3DFE0`, `3E4E0`, `BADC0` | Default/probe/root/child routing, native cache acquisitions, query-frame/frozen/movement cadence, validity writes, current/target inheritance, RGB interpolation and typed-owner gating; query/conversion/velocity/incident results remain explicit |
| `4C2B0` | All initial requests before ordered cached fallback,16-bit success-mask truncation, low5 shift count, signed section/permutation addresses and preserved selected bytes on failure |
| `4C190` | Current body regions or first-person temporary bank, initial/fallback LOD, exact cache-LOD write order; successful sections feed native`137650` in the joined comparison |
| `3D430`, `4C640` | View0..3 acquisition before region/LOD access; invalid view performs neither acquisition nor write |
| `3DDD0` | Full-owner fast lookup, shared pool allocation/pressure rebind, current region copies, object`+CC` publication and forced native color refresh |
| `3DD10`, `BAD50`, `131ED0` | Existing cache only, force low byte or definition`+1C` flag gate, current-RGB address arithmetic, packing/count and preserved unused channels |

Addresses are32-bit caller-owned tokens. The reader/writer resolves the same
mutable banks, including aliased output bytes and publication writes. Reference
RAM is test input, never gameplay ownership. `cache_acquired` only observes a
completed native acquisition; it supplies no results or writes.

Matching-owner lookup ignores generation/occupancy and returns before reading
region/RGB extents. A miss consumes the actual256-record pool, occupancy,
cursor/highwater/count and salt. Allocation/rebind publishes those same banks
and object`+CC`; the salt's upper16 bits survive. The exhausted empty pool
returns`NONE` before body payload reads. New/rebound region extents must fit
the original16 slots. Current RGBs are read after cache publication through
native`3DD10/BAD50`; fast cache reuse does not refresh colors.

The first-pass `MOVZX` permutation comparison does not exclude byte`FF` by
its subsequent comparison with`-1`; signed permutation count is the actual
gate. A used second-pass permutation is sign-extended. Declared duplicated
region tables above16 exercise literal mask truncation/shift wrap, not authored
models or `4B5D0`'s16-slot layout.

## Verification

Evidence is under `analysis/multiplayer-shading/world-effects-goal-20261011`.

| Comparison | Cases | Exact retained words | Additional checks |
| --- | ---: | ---: | --- |
| `model-color-refresh-node-original-native-v1` | 259 | 4,255,403 | 476 ordered`12DE70` requests,213 native acquisitions, all256 records/pool/occupancy/body token/output guards; successful resource outputs feed native`137650` |
| `model-render-admission-original-native-v4` | 367 | 6,068,710 | 4 actual ordered resource returns,267 native acquisitions,30 scalar calls; main inputs, four views, allocation/hole/pressure rebind, FP/force low bytes, signed caps/counts, NaN/Inf metrics and overlay/sort controls |
| `model-build-input-original-native-v3` | 182 | 4,027 | Actual object inputs, four declared first-person entries, output aliasing, signed extents/offsets, unsigned selected-section bytes, compact flags and wrapped counts |
| `model-node-bank-original-native-v3` | 170 | 525,130 | Two actual main node calls, six LODs, FP low bytes, compact/compound controls, signed counts and current node maps; 281 resource and12 deep pose events |
| `model-build-join-original-native-v8` | 63 | 3,557,610 | Five actual main builds, four views, cache allocation/hole/pressure, token reuse, FP chain, low bytes and arena capacity; 619 ordered owner events |
| `model-build-lighting-original-native-v10` | 63 | 3,557,610 | Same whole-build inputs joined to native lighting-cache owners;672 ordered events,397 native acquisitions,12 query samples and12 conversions;zero deep-owned write patches |
| `model-lighting-cache-original-native-v5` | 160 | 2,632,960 | Default/probe/root/child routing, initial/frozen/repeated/future query stamps, distance thresholds, interpolation/velocity and allocation failure;360 ordered events |

The joined build executes7,521 unchanged instruction addresses. Its deep
services in historical v8 explicitly supplied420 consumed lighting-cache write
words. The native `3DB00/3DFE0` join removes all420 in v9; zero deep write patches
remain in those63 cases. Query/converter sample/lighting results remain explicit
leaf outputs, not native collision/publication or converted lighting production.
The comparison replays no `4B5D0` output records. Its63 cases observe29 deep
pose and29 native lighting calls,193 resource requests and397 native cache
acquisitions. Sky and PRT
callback branches have zero observations in this fixture. Controlled LOD cases
declare repeated genuinely published resource headers/pointers/node counts
within each model; they do not establish authored LOD payload residency.
The emulator restores captured FS base`80035BDC` using an explicitly declared
GDT routing table; original thread bytes and all executed instructions survive.
The earlier failing cold-query runs are retained alongside the passing run.

The standalone lighting comparison executes8,167 unchanged instruction
addresses. Its retained main has five model inputs and zero lighting calls
because geometry is reused. Five declared fresh-geometry replays supply the
lighting inputs; they are not actual paused-frame lighting calls. Further
controls declare the cache flags/stamps, copied child and probe banks. Original
`D1850`, `D4080`, `BA1D0` and `3E5A0` execute unchanged and supply only their
consumed leaf results to native code. Native owns the full256-record cache,
pool/occupancy/body tokens, routing and chronology. It retains the original
53-bit x87 precision before the float rate store and SSE order for movement/RGB.

The original displacement suffix also retains its subsequent `rate>0` check.
Movement alone never starts interpolation. The prior gameplay helper's
`displacement>.25` admission could modify an original preserved sample. The
original-controlled zero-velocity movement case fails that helper in
`gameplay-lighting-interpolation-red-v1.log`; the correction keeps only positive
rate or the original typed-MACH branch. All five original preserved-sample
cases pass in `gameplay-lighting-interpolation-green-v2.log`. Final v5/v10
reports reuse the hash-verified unchanged v4/v9 original fixtures and rerun
the corrected native binaries; they do not claim new source executions. This is a cache behavior correction,
not evidence of matched gameplay illumination.

The resource/input oracles execute3,091 and2,626 unchanged instruction addresses respectively. The admission oracle executes3,154, with five actual main inputs. Its additional PRT cases declare resource headers copied from an actual published model resource; they do not claim authored PRT payload residency. Its native prior-LOD acquisitions retain the complete pool, occupancy and body token. Deep `BAB40` scalar results remain explicit ordered inputs.
Reports retain executed-bank hashes. Native input is independent pre-call data;
cache return values/write patches are no longer replayed. Actual`12DE70` returns
remain explicit ordered platform inputs. The original executes those callees
unchanged; native IO/publication is not supplied by this comparison.

Controlled readiness copies an actual published status/cache token into resource
headers before section/LOD failure controls. This isolates selection from
unresolved kernel-backed loading; it is not authored live residency. Successful
distinct authored SMG fallback pairs are`0↔3`,`1↔4`,`2↔5`. Other cases cover
free slots, pressure rebind, age/tie order, salt wrap, owner-only lookup, all four
views, invalid views, color force/definition/token gates and aliased outputs.

Durable fixtures:

- `tests/fixtures/source_model_build.original.bin`, schema2, SHA-256
  `cff5938f45975fc027397a9df698cd4618d1a43a9e13bb681015850d9af5a544`.
- `tests/fixtures/source_model_lighting.original.bin`, schema1, SHA-256
  `e3472559952296c54d1d8870aa280ef0b006884efc3d1396691afdb99182965c`.
- `tests/fixtures/source_model_node_bank.original.bin`, schema1, SHA-256
  `cd6d76371486e76754023d7674bb64d5f61e464fe0078e3af349e95a2dff5d0e`.
- `tests/fixtures/source_model_admission.original.bin`, schema1, SHA-256
  `8c03967e2211bc7db206a235e2f50ecc9919c0b4c53365e0a9cf87769899307a`.

- `tests/fixtures/source_model_resources.original.bin`, schema3, SHA-256
  `d49065f5454ca48f5d55b9d9930d3a1a348a9bdc2d6fda0db56e6d6645c64be2`.
- `tests/fixtures/source_model_build_inputs.original.bin`, schema1, SHA-256
  `b63206c1b52bdb3f7f7ebc8ab06867a83370531ab56a2099ef5c78cd1791f56c`.

CTest names: `original_model_resource_selection`, `original_model_build_inputs`, `original_model_render_admission`. The17 focused regressions, including all three geometry LOD checks, pass in `model-admission-source-regressions-v1.log`. The admission load fix reads only `hlmt+34..48` through the original short-circuit gates and reads `+28` only for a valid threshold bank; unused `+2C/+30` are not native inputs.
Earlier checkpoints retain their historical explicit-boundary scopes.

The subsequent25 focused regressions pass in
`model-lighting-source-regressions-v1.log`. The immutable118-file
`model-build-lighting-owner-checkpoint-v1/manifest.json` freezes the source,
tools, original fixtures/provenance, executed binaries and regression evidence
for that checkpoint. It does not freeze a new installed effects delivery.

`source_object_build_collector` now binds a typed133790/133AB0 collector to
the same native4B5D0 banks. Owner+0 survives, only collector+4 onward is copied,
and native cache/node/lighting output is read back from current owned memory.
`model-build-typed-collector-original-native-v11` executes the unchanged
original again and matches all63 calls/3,557,610 words/672 deep-owner events
through this adapter. Its fixture hash remains
`cff5938f45975fc027397a9df698cd4618d1a43a9e13bb681015850d9af5a544`.
This verifies the binding, not live object/publication ownership. The separate
110-case133790 fixture uses native metric/coverage/fade/flag owners but keeps
4B5D0 and133AB0 as explicit record boundaries; those fixtures must not be
reported as a joined whole main frame.

All13 focused source regressions pass in
`model-cache-joined-source-regressions-v1.log`, including the shared allocator,
object collector/presentation, both new owners, sphere rectangles, opaque
selection/ranges, raw binding, scissor, BSP commands/collector and standalone
render-cache fixture. A concurrent CMake addition briefly referenced an absent
vitality test; the failed regeneration log is retained. The already configured
manifest was frozen for the bounded build without editing that other work.
Current CMake configuration subsequently succeeds in
`model-cache-current-configure-v1.log`. The source, reports, fixtures, executed
binaries and regression evidence are retained in
`model-cache-owner-checkpoint-v1/manifest.json`.

## Remaining gameplay gate

The current capture checkpoint still submits zero raw/packed geometry-light
packets for its selected frag light. These CPU owners do not change that result
or establish runtime delivery. Complete live `4B5D0/133790`, live pose/node and
main/dynamic query ownership, resource publication, class2/3 raw consumers and
ordered light draws remain open. Matched frames and installation follow those
runtime gates. The complete effects goal remains active.
