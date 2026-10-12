# Source geometry resource publication

`source_geometry_resource_publish` implements whole retail223C70, paired with
TU1.1 225D80 by the unchanged-image correspondence in
[the live model-build handoff](SHIELD_LIVE_MODEL_BUILD_BINDING_HANDOFF.md).
The input resource header, descriptor bank and ready allocation are actual
caller-owned32-bit banks. Read and write adapters must share their backing;
published addresses are source tokens, never host vector pointers.

The owner writes the signed header+1C root, then iterates actual header+10/+14
descriptors in order. OffsetNONE skips a descriptor. Type0 publishes a signed
size/unsigned-stride array count and pointer. Type1 publishes its raw byte count
and pointer. Every other type follows the original packed vertex-buffer branch,
including theFFFF direct selector, indexed32-byte buffers, signed division,
16-bit count store, relative pointer stores and final0FFFFFFF mask. Descriptor
pointer/count are reloaded each iteration. Only original-written bytes change;
all other buffer/header/guard bytes survive. Zero divisors reject at the native
division boundary after preceding source stores.

`geometry-resource-fixup-original-native-v2` compares114 whole calls and
1,607,057 retained resource words.16 calls use actual ready allocations from
the unchanged main2CBC0/12DE70 replay: eight BSP, one lightmap and seven model
headers. All21 observed requested headers are ready; the per-group sample cap
does not claim every source allocation was replayed.98 declared controls cover
all descriptor branches, absent offsets, direct/indexed vertex buffers, signed
counts/division, different strides, randomized retained storage and pointer
masking. Every original instruction executes unchanged, with zero interception.
The earlier v1 report mistakenly counted the appended control cases as actual
allocations; v2 corrects that report alias and retains the same output fixture.

The live weather allocation now calls this complete shared owner after its
existing type0/extent/pair validation. It keeps its actual private descriptor
and raw allocation backing, linked-state owner and age policy. The shared
function replaces the duplicate weather-only pointer stores; it does not
expand weather's accepted resource types or create an original asynchronous
cache. `weather-shared-resource-original-native-v2.json` verifies this binding:
1,020 unchanged original12DE70/223C70/13D230 comparisons,49,980 exact words,
unchanged particle payloads and no supplied service results. Its completed
private native allocation is an explicit input; original asynchronous loading
and eviction chronology remain outside that proof.

## Complete request owner

`source_geometry_resource_request` implements whole retail12DE70 / TU1.1
12F5E0 over current shared banks. Its global-cell address map is explicit for
each image; pointer values are reread at their original boundaries. Linked
resources touch physical-cache time. Pending resources consume request flags,
signed sizes and tag index, policy selection, original priorities, promotion,
completion reads and blocking yield/pump order. Completed banks join native
223C70 before publishing linked+1E. Failure flag and special-tag counter stores
retain their source gates and wrapping arithmetic. Reached missing services
stop after the original preceding mutations, rather than returning readiness.

`geometry-request-original-native-v2` compares1,283 whole unchanged original
calls with native CMake target `source_geometry_request_tests`:20,001,676 exact
retained words and3,720 ordered service-record words.63 calls use the21 actual
linked headers observed in unchanged main2CBC0, at three request-flag values.
The other1,220 declared controls cover policy/priority selection, all four
consumed flag bits, signed sizes/tag indices, pending/ready/absent slots,
completion during promotion or repeated pumping, retained guards and joins to
actual223C70 writes. Original code bytes are unchanged. Acquisition12E3A0,
promotion120CE0, yield2D2084 and pump125D60 remain explicit recorded service
boundaries in the controls; these are not original asynchronous execution.

The frozen runtime builds in `build-effects-resource-proof-20261012` and all33
selected source/model/light/resource/weather checks pass. The first snapshot
build retained a title-preview camera declaration failure; revision2 moves
that initialization after its actual camera declaration. The first32-check
run lacked the retained raw-binding fixture and Vista map bindings; revision3
supplies those external inputs, after which all32 pass. Revision4 adds the
request owner/test and passes33. The failing logs, original source manifest,
exact camera repair, subsequent manifests and external input hashes remain
retained. These are consistent native source/runtime build checks, not a
matched-original framebuffer or installed-launcher delivery.

## Acquisition and physical-cache lifecycle

The following additional owners are implemented in native C++. Their memory
adapters share the same current header, pool, allocator and backing banks.

| Owner | Retail / TU1.1 | Verified behavior |
| --- | --- | --- |
| `source_geometry_resource_acquire` | 12E3A0 / 12FB10 | Physical admission, full-identity inflight allocation, rounded read request, actual job and priority stores |
| `source_raw_datum_allocate_identity` | 16B910 / 16D390 | Capacity/bitmap/high-water/count/salt stores and caller-supplied full identity |
| `source_physical_cache_allocate` | 13D370 / 13EAE0 | Page-window selection, age/priority protection, ordered overlap eviction and raw allocation |
| `source_physical_cache_release` | 13D830 / 13EFA0 | Actual deletion callback, list unlink and raw-pool retirement |
| `source_geometry_cache_busy` | 12E210 / 12F980 | Current-frame protection, completion byte and signed five-frame GPU-use gate |
| `source_geometry_cache_deleted` | 12E290 / 12FA00 | Queue traversal/count, real completion wait loop, resource wait, pointer unpublication and inflight retirement |
| `source_geometry_resource_in_use` / `source_geometry_resource_wait` | 223D80/223E00 / 225E90/225F10 | Type2 direct/indexed descriptor walks and actual resource query/wait dispatch |

Acquisition rereads the physical root after admission, binds the admitted full
handle through16D390, then submits the original sector-rounded I/O ABI. It does
not manufacture a completion byte. Deletion rereads its resource/async owners
at the original boundaries. Missing reached services throw after preceding
source stores. GPU-use queries consume signed-positive results; an absent
query owner never makes a resource idle.

Fresh proofs under `world-effects-goal-20261011` retain unchanged executable
instructions, complete compared banks, ordered service records and the exact
native CMake binaries/tools:

| Proof | Calls | Compared words | Explicit lower boundaries |
| --- | ---: | ---: | --- |
| `geometry-acquire-original-native-v2` | 264 | 4,329,824 | Physical admission and routed I/O |
| `geometry-request-joined-original-native-v3` | 1,283 | 20,009,075 | Physical admission, routed I/O, promotion, yield and pump |
| `physical-cache-original-native-v2` | 1,158 | 19,038,031 | Busy/deletion callback records |
| `geometry-retirement-original-native-v3` | 1,138 | 18,648,641 | D3D resource query/wait and platform mutex/completion records |
| `geometry-cache-joined-original-native-v2` | 1,319 | 20,599,799 | Routed I/O, promotion, yield and pump |

The final join executes native request → acquisition → physical allocator →
actual cache callbacks → raw pools → fixups on one bank. It supplies no
intermediate admission handle, page selection or publication output.63 cases
use actual linked headers from unchanged main2CBC0; the remaining cases are
declared initial-bank controls, including cache pressure. These controls do
not prove original asynchronous chronology or native live cache binding.

Snapshot revision6 adds the missing unchanged shared datum test fixture; all40
selected checks then pass without skips. Revision7 adds retirement and passes41;
revision8 adds the whole-cache join and passes42. Revision10 includes the
particle-death presentation fix and passes45 resource/model/light/weather and
effects checks. The source manifests, previous sources and failed logs remain
retained. Revision9 failed because concurrent root tag API changes had been
copied into an older frozen Cache interface; revision10 reapplies only the
three particle-death hunks to that snapshot's existing source.

Only the shared fixup owner is currently bound into live weather publication.
The new geometry cache owners and their joined proofs do not yet populate
live `ModelView::raw_effect_lights`. Live I/O/queue binding, dynamic draw-bank
production, ordered geometry-light packets and matched explosion frames remain
required. The effects goal remains incomplete.
