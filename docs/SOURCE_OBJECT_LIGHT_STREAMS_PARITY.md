# Original resident object lighting streams

`source_object_light_radiance` and `source_object_light_incident` implement the
complete `D33A0` and `D31C0` CPU data services from explicit resident geometry
descriptors, backing bytes and original triangle indices. Current geometry
residency, object attribution and lighting-cache production remain caller inputs.

## Literal service behavior

Both services return false when geometry `+38` has at most three descriptors.
This occurs before any index/data read. Otherwise they begin at descriptor3
in the ordered `+3C` array (stride32). `D33A0` stops at the first declaration46
or47; `D31C0` stops at the first48. If the scan exhausts the array, the original
uses its last descriptor anyway. The native helper preserves that behavior
instead of treating a missing semantic as a source miss.

Each service reads directly from descriptor `+C`, ignoring descriptor `+8`.
The explicit native bank therefore starts its `data` at that pointer. Its
`descriptor_offset` retains the source field but does not affect these two
functions. Callers must supply sufficient actual backing bytes even for the
fallback interpretation; an incomplete native bank throws.

Indices come from geometry `+24` at `lightmap_triangle * 6`, as three consecutive
unsigned shorts. This is the attribution packet's lightmap-triangle field,
independently of its strip field.

Declaration47 radiance is consecutive R/G/B bytes multiplied by the literal
float `0x3B808081` (1/255), reusing `decode_bsp_radiance`. Declaration46 and the
fallback path read packed DWORDs through original `131BB0`: R from bits16..23,
G from8..15, B from0..7; alpha is ignored. The source interpolates
`(C-A)*weight_c + (B-A)*weight_b + A`, with separate single-precision operations.

Incident reads packed11/11/10 signed components through the CPU midpoint
decoder, reusing `decode_cpu_lightmap_incident`. Each component is effectively
`(2*signed_code+1)/(2047 or1023)`, with the original rounded constants. It does
not use GPU NORMPACKED3 conversion, normalize directional strength or rotate the
authored world-space incident. Source X interpolation starts with B; Y/Z start
with C. Extrapolating weights and out-of-range colors are retained; `D1E10`
clamping and PRT/baked combination occur afterward in their separate services.

## Untouched original comparison

`tools/differential_source_object_light_streams.py` executes complete unchanged
`D33A0`, `D31C0` and their `131BB0` callee. It checks287 executed instruction
sites against the pinned retail XBE; no service is replaced.

The252 explicit data banks cover:

- All42 extra-stream sections identified by the all21-cache raw census: seven
  models in Containment, Turf and Triplicate, with15,388 original index triangles.
  The actual resources, declaration order, raw index order and all stream bytes
  are retained. These are relocated inputs to the CPU services; they do not
  assert a live object/pose/residency producer for every authored section.
- Two actual paused resident geometry banks and the eight successful diagnostic
  `14A8E0` outputs. Both banks have only three descriptors, so both data services
  correctly return false. The parent complete `D1E10` replay likewise reaches
  `D33A0` but skips `D31C0` for those contacts, while current PRT succeeds.
- 208 explicit valid diagnostic banks/1,664 cases covering initial-three
  exclusion, first-match selection, absent-semantic fallback, nonzero ignored
  offsets, ARGB alpha/channel order, PBYTE RGB, signed incident endpoints,
  duplicate semantics, permuted indices and extrapolating/signed-zero weights.

All17,060 cases match both source success bytes. All100,008 defined RGB/incident
float words match exactly. The controlled corpus includes512 radiance fallback
and640 incident fallback cases, and464 descriptors with nonzero ignored offsets.
Incomplete-index/data guards are also checked. Private normal and
ASAN/UBSAN/leak CTest runs pass.

The retained snapshot is paused at `12B33E`, tick12474; RAM/manifest hashes are
recorded. Its contacts are separately declared diagnostic original calls, not
captured original query scheduling. FCW023F/MXCSR1F80 are explicit arithmetic
inputs to the CPU emulator. This proof makes no physical-Xbox or GPU-fetch
claim and performs no guest I/O/advancement.

## Native seam and files

The public geometry bank accepts an ordered descriptor list, bytes at each
actual data pointer and raw indices. There is no fallback to expanded mesh
colors/incident, default posed vertices or a fabricated current PRT cache.
`SourceObjectLightMix` can consume a baked input only after both services
succeed; its status/residency and diffuse-texture sampling remain separate.
Availability of a decoded baked color does not establish `D1E10` status0.

API/source: `include/halo2/source_object_light_streams.hpp` and
`src/source_object_light_streams.cpp`. Frozen source inputs/outputs and hashes:
`tests/fixtures/retail_source_object_light_streams.tsv` and provenance JSON.
CTest: `retail_source_object_light_streams`. Source report:
`analysis/multiplayer-shading/object-light-streams-original.json`; validation
hashes/logs: `object-light-streams-validation.json` in the same directory.
No native runtime or dynamic-contact activation is added here.
