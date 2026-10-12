# Original object render-cache allocation and reuse

`source_render_cache.hpp/.cpp` implements the persistent 256-record owner
pool, its reset/allocation/pressure-rebind behavior, original render-context
initialization, per-view geometry LOD writes, geometry reuse/arena arithmetic,
and the bounded post-resource control decisions. This is an independently
tested service seam. It is not production renderer activation or a claim that
native object/child allocation and collector history match an original frame.

The authority is the supplied Xbox executable, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`,
and its unchanged instruction bytes. The source addresses below refer to that
executable. No generic replacement cache, inferred draw count, or source
callee-return stub is used by the oracle.

## Pool and record lifetime

`3D270` requests `0x1006C` bytes from the actual allocator at `510C2C`, then
initializes a pool of 256 records with stride256, alignment exponent0 and
name `cached object render states` through `16B5F0/16B650`. The records start
at storage+`0x4C`; eight occupancy words follow the 64KiB record array.
Pool `+2A` receives flag4; `509434` becomes its pointer. The allocation
service itself is an explicit boundary, rather than a simulated success.

`2B540` activates the pool and calls complete `16B7A0`. That resets cursor,
highwater and active count, zeros occupancy and record salts, and starts the
generation at `E163`: the first two name bytes OR `8000`. It preserves all
other record bytes unless the original debug-fill flag3 is supplied; that
branch fills records with `BABA` before clearing salts. `2B5A0` deactivates
the pool. A per-owner deletion/free producer is not established here.

| Record offset | Original field |
| --- | --- |
| `00` | 16-bit generation; query-valid byte at02, converted-valid byte at03 |
| `04` | Full owner handle |
| `08`, `0C` | Last query frame, last rendered frame |
| `10` | Three-float cached query anchor |
| `1C`, `30` | Previous/current raw 20-byte lightmap samples |
| `44` | 0x78-byte render context |
| `BC` | Four signed per-view geometry LOD bytes |
| `C0` | Four permutation arrays, stride16 |

| Context-relative offset | Original field |
| --- | --- |
| `00`, `04` | Full owner, callback `BE8B0` |
| `08`, `0C` | Transient geometry offset, frame/view geometry stamp |
| `10` | Converted 84-byte lighting state |
| `64` | Four packed object colors |
| `74` | Color count byte |
| `76`, `77` | Additional alpha bytes |

`3DDD0` first tests object+CC's slot against the complete owner handle at
record+4. It does **not** check the token's generation or occupancy bit on
this path. A matching owner returns the existing token without resetting the
record or colors. The native helper preserves this rule; out-of-allocation
indices are rejected as unsupported input rather than dereferenced.

If lookup misses, `16B840` scans unoccupied slots from the cursor to the
highwater, otherwise uses the highwater when it is below256. It zeros the
whole allocated record, sets its salt, increments the next salt, and skips
`FFFF` when the increment reaches that value. The token is `salt<<16 | slot`.

When allocation has no slot, `3DDD0/16BC00` scans occupied slots in ascending
order. Age is the wrapped signed32 subtraction of render frame minus record
last-render-frame, converted to float. Negative float ages become **1000**.
The maximum age wins, with the first ascending slot retained on ties. This
is literal source arithmetic, including the raw `NONE` frame value; `NONE`
is not assigned a new infinite-age meaning. A rebind keeps the selected
record's salt, occupancy and unwritten bytes.

Rebinding resets owner/query stamps and valid bytes, calls `2C3B0`, sets the
callback, resets all four cached LODs, and copies the actual object
permutations to each view's 16-byte array. Unused permutation bytes survive
on pressure rebind. `3DD10(owner,1)` then writes actual `BAD50` current RGBs
through the independently proven `131ED0` packing function. It sets the
color count but does not overwrite channels past that count.

`2C3B0` also leaves bytes untouched: context+0C, the four color words, +75,
and the upper16 bits of +60. Its native initializer retains those bytes.
The fixed lighting direction/color words are literal results of the complete
unchanged initializer with the original static constants and FPCW027F.

## Geometry service decisions and shared frame arena

Ordinary `2B5D0` increments the render frame `4BA034` and resets arena cursor
`487B18` once before the view loop. `2B790` supplies view index `4BA050` from
view-descriptor+4. This index must come from that producer; physical viewport
ordinal is not an interchangeable replacement. Auxiliary `2C8A0` also
increments the frame but has no arena reset in its literal body. Its other
render-global services remain outside this helper.

The geometry stamp is `(frame & 3FFFFFFF) | (view << 30)`. At `4B775`, reuse
requires a prior per-view LOD other than `FF`, a context, a geometry token
other than `NONE`, and an exact stamp match. That candidate substitutes the
prior LOD before the actual resource selection call.

`4C190` selects actual object permutations (`3D430`) for normal models, or
the authored MODE region count and a zero permutation bank for first-person
models. The real `4C2B0/12DE70` residency/section service is not replaced.
The earlier helper below implemented its literal control after explicit service
results. The subsequent native join now implements whole
`4C190/4C2B0/3D430/4C640/3DDD0/3DD10` over current owned banks, including
allocation/rebind, and passes259 cases/4,255,403 words. Successful selected
sections also feed native`137650`. Actual`12DE70` publication and the live
collector/node producer remain separate; see
[the current model-resource evidence](SOURCE_MODEL_BUILD_RESOURCE_PARITY.md).

The original control sequence remains:

| Initial result / prior | Fallback request | Actual LOD on success | Ordered `4C640` writes |
| --- | --- | --- | --- |
| Initial succeeds | None | Requested | Requested |
| Initial fails, prior exists, fallback succeeds | Prior with request0 | Prior | None |
| Initial fails, prior exists, fallback fails | Prior with request0 | Requested retained | `NONE` |
| Initial fails, prior is `NONE` | None | Requested retained | Requested, then `NONE` |

Complete `4C640` calls `3DDD0` before writing a LOD byte for views0..3. Other
view indices perform neither acquisition nor byte write. The helper takes
the actual permutations, current RGBs and frame needed for that acquisition.

After resource selection, `4B801` cancels reuse if regions changed or actual
LOD differs from the original requested LOD. A subsequent source model
ordinal greater than0 is omitted on a LOD mismatch; the first model may
survive a successful fallback. Failed resource admission omits the model.
Actual node count is supplied from `137650`; the byte stored in the collector
is its low8 bits.

If geometry is not reused, or this is first person, the original allocates
`nodeCount*48+68` bytes for a positive count, otherwise zero. The 32-bit
wrapped end is admitted using a **signed** comparison against `27000`;
the token is the old cursor masked to28 bits. Arena state is shared by all
views, not reconstructed for each owner or material. Context token/stamp are
written only when a context exists, this is not first person, regions did
not change, and actual/requested LOD agree.

The continuation explicitly identifies a pending `137800` node build,
reused tail, or omission. Node building and actual resource admission are
required source services, not successful outputs manufactured by the helper.
The first-person branch can allocate even when its reuse flag remains true;
the subsequent reused-lighting tail still skips PRT/colors/query. That
combination is preserved rather than normalized to a preferred policy.

The ordinary nonreused successful tail calls `23690` when collector flag40
requests PRT, then `3DD10(owner,0)` and context-present `3DB00` even when
the PRT return is false. Reused geometry skips those calls. Only final
successful admission increments the accepted-model count and writes the
owner record's last-render-frame after its full-owner check. There is no
drawn-vertex-count or ordinary-shader gate for that owner cache.

The separate source query/conversion admission proof remains
`tools/audit_prt_object_light_admission.py`. A first view can leave a query
stamp unchanged while a farther second view in the same render frame admits
the query. The renderer must consult the actual query history for each
ordered view; a blanket once-per-render-frame call guard changes this result.

## Collectors and transparent token history

This read-only inventory identifies the inputs that a later integration must
retain. It does not claim complete live collector admission.

Main `3F2C0` builds actual visibility/admission through `1320F0`, optionally
appends its explicit forced owner, then calls `2CB30`, `133390` (BSP),
`133520` (instances), `133720` (owner collectors), and `44720`. `2CB30`
resets draw count, main accepted prefix count, override count and sort-section
state for context mode0. Other modes restore draw count from that main prefix
and retain the other state. Thus a later collector cannot use a private new
draw-unit numbering range without reproducing the original prefix contract.

`133790` obtains the full owner from the actual admitted owner list, consumes
its cached render sphere, computes actual coverage/distance, and passes a
collector to `4B5D0`. The complete collector is `1E4` bytes; `4B5D0` receives
its +4 bank. Source geometry ordinal, compacted accepted-model index, owner,
actual node bank, region selections, FP mode, PRT flag and lighting context
are distinct fields and must remain distinct.

`133AB0` processes actual collector/effect flags and traverses owner+10 child
head, then child+0C. It copies the complete parent collector for each child,
rebuilds that child's geometry context with `4B5D0`, and retains the inherited
sphere/coverage/distance inputs. It also recurses for special collector flags.
An authored placement sort cannot substitute for this live linkage/order.

`133F70` submits accepted models and their actual selected regions in source
order to `2CBF0`. That routine allocates a 32-byte draw unit at `4BA138`.
Admission requires draw count<=`351` and a non-`NONE` geometry input, allowing
850 entries numbered0..849. The count is incremented before later resource
checks; failed final `449E0` admission decrements it, so a later collector can
reuse that ID. The 192-entry extra-override table advances separately and is
not rolled back by that final draw-unit decrement. The section state highwater
is max(actual section+1); section31 is the `NONE` sentinel. Main-context
accepted submissions increment `4C0B7C`, the preserved prefix count.

`41040` draw keys compose the source draw-unit ID, distance byte and actual
part ordinal. Native material IDs, Vulkan draw numbers and RenderDoc event
IDs do not identify those original tokens. To reproduce the complete keys,
the caller needs the actual ordered owner/child/effect/BSP/instance
collectors, their resource results, all preceding allocations, and the
context-mode reset/prefix history. This task ports the owner pool and
geometry decision arithmetic, not that unresolved complete queue history.

## Retained source state and validation

`render-cache-retained-state.json` inventories immutable previously exported
postinterval original20 RAM. Wheel, blades and main crane have tokens
`E1640001`, `E1630000`, and `E1650002`. Their non-`NONE` caches corroborate
that pure PRT does not suppress owner cache admission.

The complete retained player record has owner `E2E60077`, callback `BE8B0`,
cache `E16C0009`, geometry offset `3978`, stamp `0001B276`, and LOD bytes
`[4,-1,-1,-1]`. Last-render-frame is also `0001B276`; last-query-frame is
`000181A9`. These are literal observed words, not values regenerated from
native code. The global default context at `5021D0` has owner/geometry token
`NONE` and must not be mislabeled as a persistent owner record.

The full pool header/all256 records were not previously retained for frame20.
The authorized new read-only export could not run because the private QMP
socket was absent; the other chat's session was left untouched. Retained
token associations therefore do not establish complete pool allocation/
rebind/child chronology. Postinterval CPU stamps alone also do not identify
the last gameplay draw in RDC20.

`analysis/multiplayer-shading/render-cache-original.py` executes the full
unchanged pool/reset/context/view-write functions. Its geometry/resource/
tail probes enter explicitly documented post-service instruction seams and
stop at real source call boundaries. No original instruction is patched and
no callee return is synthesized. Complete `3DDD0` probes compare selected
record bytes, headers, occupancy and a digest of every256 record to catch
changes outside the selected slot.

The final differential passes **5,506 cases / 115,520 exact output words**.
The durable fixture freezes **1,666 cases / 34,112 words**, SHA-256
`4bd99d79eb89164493b600166da9c06cf05317ea89742476a163e8b820a67206`.
It covers full/fragmented pools, salt wrap, strict tie order, signed frame
wrap/future stamps, generation/occupancy-insensitive full-owner lookup,
partial initializer/reset/rebind writes, actual per-view acquire/write,
resource fallback/write sequences, main frame reset, geometry omission,
FP reuse/allocation, zero node banks, arena boundaries and post-PRT/query
control. Unrealistic signed-overflow inputs test bounded arithmetic only;
they do not claim valid original node/resource memory access.

Fresh production-linked private CMake CTest passes in normal and
AddressSanitizer/UndefinedBehaviorSanitizer builds, with leak detection
enabled. Logs: `render-cache-build.log`, `render-cache-ctest.log`,
`render-cache-asan-build.log`, `render-cache-ctest-asan.log`.

## Required runtime service contract

A caller must supply an allocated pool lifecycle; literal full native owner
handle and object+CC history; actual live region/current-RGB banks; ordinary
frame/view entry events; explicit ordered collectors and original requested
geometry LOD/FP/PRT flags; actual `4C190` section/residency/change results;
actual `137650` node count/`137800` bank; actual `23690` result; and per-owner
`3DB00` query/conversion history. Persistent cache state, transient arena
state, collector identity and material/pass identity cannot share one token.
Successful global factories, children, physics registration and all opaque
draw prefixes remain separate required producers. Production integration
must preserve these boundaries instead of treating authored metadata or
successful expanded geometry as equivalent collector history.
