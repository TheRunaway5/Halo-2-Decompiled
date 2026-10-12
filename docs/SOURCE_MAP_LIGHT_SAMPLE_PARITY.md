# MAP object-light sampling

The native MAP query now uses the separately proved `source_object_light_rays`
outer control and retains the authored 72-byte part type. Its contact service
is the existing original BSP/MOPP traversal, exact-leaf attribution and
authored double-sided retry. It is a MAP adapter: it does not create a live
dynamic registry, asynchronous resource manager or current PRT cache.

`BspSurfaceLighting::sample(hit, detail, initial, caller_sample)` implements
the special MAP part branch in `D1E10`. Original `13D9F0` recognizes types
4/5. Initial queries write incident `(0,0,-1)` and both packed colors
`0xFF404040`, then return status0. Retained queries return status0 without
writing any of the five caller words. A retained special-part query without
the actual cache target is rejected. Runtime callers supply the original
cache+30 target, rather than the interpolated current lighting state.

The ordinary MAP branch's diffuse FLOAT2 UV interpolation adds AB before AC
for Y. X and the lightmap SHORT2 path add AC before AB. These operations round
separately. The source CPU texture detail is always1; this is distinct from
which actual image the resource manager currently returns.

## CPU incident selection

`D1E10` passes the resident render geometry to `D31C0` for both cluster and
instance contacts. `D31C0` starts scanning at descriptor3 for type48; when no
48 is found, it uses the last descriptor. The data address is descriptor+C,
with four-byte incident indexing; descriptor+8 is ignored. With at most three
descriptors, it preserves the initialized +Z incident from original440B48.

The decoder follows this literal selection. All88 instance calls in the
retained diagnostic corpus have four descriptors `[1,24,27,31]`: their CPU
incident therefore comes from the last SHORT2 lightmap-UV bytes interpreted
as packed incident. The previous native query instead used the GPU instance
lighting bucket. GPU bucket/vertex declaration bindings remain independent
and unchanged. The fixture records the actual selected descriptors, indices
and packed words, not a fitted incident direction.

## Whole original evidence

`tools/differential_source_map_light_sample.py` executes complete unchanged
`D1E10` and its lower geometry, bitmap, resource and packing callees on the
immutable Zanzibar snapshot paused at0012B33E, tick12474. FCW023F, MXCSR1F80,
segments and captured RAM are explicit inputs. This is not frame20 or a
later shadow producer capture. Every executed instruction is checked against
the pinned retail XBE, and written pages are restored between calls. No live
guest input, advancement, writes or substituted math/residency service occurs.

The2438 actual ready inputs come from completed whole original `D2A50`
diagnostic calls. There are96 additional full-call controls:24 actual inputs
with only their two-byte authored part type changed to4/5 and each initial
mode. These control mutations occur only in the disposable memory clone.
They are not claims about the actual part type of those24 source contacts.

The frozen test compares34708 metadata, UV, incident and sample words exactly.
It also rejects a missing retained caller target, and checks the native outer
guard/ray modes against the independently proved `D2A50` contract. Mode bit0
wins over bit2, so5 is a five-ray mode; invalid banks return miss2 before
custom-ray validation. A valid mode4 call requires an explicit displacement.

Twenty-one actual current diffuse surface records/pixel banks are retained
as explicit sampler inputs. Sixteen equal decoded authored mip images;
five require separately retained software image data. Those five account for
128 calls whose diffuse sample differs from the native fully resident mip0
image. Their dimensions alone do not identify an authored hardware mip: the
source low-detail image bytes can differ from an equal-size hardware tail.
The actual original temporary CPU flags are retained; `136140` tests bit3
for byte/word swizzled addressing, independently of `1D2F0`'s bit0.

The test supplies those actual resident images to isolate geometry, CPU
sampling and orchestration. It does not silently select low detail in live
queries or claim the original resource-cache history was reproduced. The
standalone low-detail asset decoder and actual hardware/software selection
policy are separate services.

## Validation and remaining caller services

CTest `retail_source_map_light_sample` uses the frozen source inputs plus the
supplied Zanzibar cache. Normal and ASAN/UBSAN/leak checks pass alongside
`retail_source_object_light_rays` and all21-map
`retail_multiplayer_surface_lighting`. The all-map authored-spawn controls
preserve the expected Triplicate attribution miss. Six diagnostic queries
at the authored spawn positions reach types4/5: Cyclotron2, Headlong1 and
Turf3. These centers are explicit test inputs, not inferred posed player
query centers or observed scheduling.

Live MAP loading is synchronous. Its missing decoded image still uses the
existing unavailable boundary; it is not complete original status/residency
production. Dynamic object registration/current poses, asynchronous image
selection, generated status3 and PRT cache creation/refresh remain explicit
caller services. No native dynamic contact or global registry is invented by
this correction.

Evidence: `tests/fixtures/retail_source_map_light_sample.tsv` and adjacent
provenance, `analysis/multiplayer-shading/map-light-sample-original.json`,
`map-light-sample-normal.log`, `map-light-sample-asan.log`, and
`map-light-query-census.tsv`. Both production source files compile with
strict floating-point operations and contraction disabled.
