# Original object lightmap attribution

`source_object_light_attribution` implements the complete admitted `14A8E0`
path from an already selected object contact to its resident render triangle.
It consumes explicit current owner, active-permutation and WORLD-node banks,
literal render-model metadata and raw CPU geometry. It does not construct a
native registry, choose an object pose, schedule a query or sample its lighting.

## Source fields and operation order

The input contact is the 96-byte `1697C0` result. `+40` identifies the owner;
signed shorts `+44/+46` identify model region/node; `+48` selects the packed
collision region/permutation/BSP; `+4C` is the actual collision leaf; `+54`
contains the signed plane. Its already rounded world point is at `+8`.

`14A8E0` reads current active bytes through body `+11A`, then selects the
render-region permutation's literal `+C` highest section from `mode+20`.
It always uses geometry LOD5 here, independently of the current drawing LOD.
Active byte NONE or section255 returns a source miss. Geometry residency is
the actual `12DE70` mode3 result. A synchronous native decoder can declare its
decoded resident bank; this does not prove equality of asynchronous residency
or query scheduling.

For a nonempty current WORLD-node bank, the source invokes `142700` twice:
first with that node's body `+116` WORLD transform, then with the literal
render-model node inverse transform (`mode+4C`, stride96, offset40).
The implementation preserves subtract, reciprocal-scale and transposed-dot
order. Building an inverse matrix and applying a generic point transform would
round differently.

`1EFB40/1EFD80` resolve the actual collision bank. The existing
`source_object_light_render_reference` supplies the ordered node-map/leaf scan:
the resident geometry node bytes choose a map; its leaf count must match the
selected collision BSP; NONE references stop the scan; plane matches precede
triangle testing. The helper consumes the literal strip indices, without
triangle-strip winding repair or collision-fan substitution.

`23AC30` fetches CPU positions from the first resident stream descriptor.
Formats1/2/3/4 are float3, signed-short3, float3 with a node byte, and
signed-short3 with a node byte, respectively. The compressed axis is:

```
normalized = (float(short) * 2 + 1) * float_bits(0x37000080) + 0.5
position = normalized * (high - low) + low
```

Each operation rounds separately, without FMA. This is distinct from the
NV2A/GPU SHORT position decoder in the model loader; expanded rendered mesh
positions cannot replace these CPU bytes. Formats outside1..4 require the
original default-vector producer and are explicitly rejected. Current retained
sections all have format2. The final node hint is written by the third fetch.

The reused `original_render_barycentrics` implements `11E800`. Successful
outputs preserve `14A8E0`'s part0 behavior: its part search reads output `+24`
while that field is still NONE, before storing the accepted strip. The helper
does not change this into an intended strip-range part lookup.

## Immutable source evidence

`tools/audit_dynamic_object_contacts_snapshot.py` executed complete original
`1697C0` against the restored snapshot's 202 current owners and both raw spatial
pool states. Its rays are explicit diagnostics through retained primitive
spheres, rather than captured original query scheduling. Across 3,792 dynamic
queries, 157 positive object contacts differed from their static-only control.
The original `14AF40` action gates classify those contacts as 80 scenery
rejections, 12 static-map fallbacks and 65 model-attribution attempts. Initial
`0x4480000D` and retained `0x04800C0D` query branches remain distinct.

`tools/audit_dynamic_object_attribution_snapshot.py` then executes entire
unchanged `14A8E0` for those65 admitted contacts. Current owner/active/WORLD,
tag, collision, resident raw geometry and residency services are actual
immutable snapshot inputs. No instruction or service is replaced. The original
returns 8 successes and57 misses; a source miss is not a missing-input success.

The frozen native fixture retains three complete required model banks and five
current owners. Each successful native result matches all14 output words
exactly, and all65 success/miss results match. A further512 unchanged `23AC30`
cases cover every admitted format, signed-short edges, asymmetric bounds,
raw float/signed-zero copying and node bytes. Another142 unchanged `142700`
cases cover all retained WORLD/inverse transforms and explicit zero/unit/
negative/nonunit-scale controls. Total defined outputs are2,651 exact words.

The replay pins the original XBE hash and snapshot RAM/manifest hashes. It
verifies705 executed instruction sites and preserves223 owner/spatial banks.
FCW023F and MXCSR1F80 are the literal paused snapshot configuration; this is
CPU-emulator/source arithmetic evidence, not a physical Xbox arithmetic claim.
The snapshot is paused at `12B33E`, tick12474. These diagnostic calls are not
the later retained shadow phase or original frame20. No guest I/O or advancement
is performed.

## Native integration boundary

The caller must provide:

- The already source-admitted current contact and its actual owner identity.
- Current active region bytes and current WORLD transforms.
- Literal highest-section, bounds, inverse-bind and collision metadata.
- Actual resident first-stream CPU bytes, raw strip indices and leaf maps.
- The current collision bank resolved for that owner, or its actual failure.

Missing or malformed supplied banks throw; proved source rejection/miss returns
`nullopt`. `D1E10`'s current PRT cache, authored incident/radiance streams,
interpolation and bitmap sampling are a separate service. No live dynamic
contact activation, successful registration chronology, current PRT-cache
producer or final lighting-color parity is claimed by this module.

## Files and verification

API/source are `include/halo2/source_object_light_attribution.hpp` and
`src/source_object_light_attribution.cpp`. Frozen inputs/outputs and hashes are
`tests/fixtures/retail_source_object_light_attribution.tsv` and its provenance
JSON. CTest `retail_source_object_light_attribution` runs the CPU comparison and
explicit missing-bank guards. The source report is
`analysis/multiplayer-shading/dynamic-object-attribution-snapshot-original.json`.
Private normal and ASAN/UBSAN/leak runs pass; file hashes and exact counts are
recorded in `analysis/multiplayer-shading/object-light-attribution-validation.json`.
