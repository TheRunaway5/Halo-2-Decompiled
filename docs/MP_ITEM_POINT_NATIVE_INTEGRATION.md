# Native item points and pickup composition

The bounded native `FBFA0` item-point producer matches **208 original calls**:
205 root outputs and three ground-marker/contact-plane projections. The native
producer also composes with `E0C20/30BD0/C57A0` and ordinary terrain queries for
all 207 recorded pickup checks. Those checks retain 187 exact normalizations,
1513 ordered casts and 21 visible/186 blocked results. Normal and focused
address/undefined/leak sanitizer comparisons pass.

This closes the point arithmetic and its visibility-consumer connection under
literal original current state. It does not produce live current nodes, contact
state, registry candidates, item settling or inventory transfer. The original
authority and its frozen scope are in [item-point research](MP_ITEM_POINT_RESEARCH.md).

## Original data and arithmetic

`Cache::load_item_point_definition` imports the actual weapon→model→render-model
chain, the first ground-point StringID group and its 36-byte marker records.
Both original BR and SMG definition/model/group/marker imports match. Metadata
addresses use the map's metadata mapping. Nothing embeds original marker data
or contact geometry in source.

`source_item_point` preserves the root fallback when the original item's
ground-contact bit is clear. A fallback return of zero still defines a usable
point. In the measured ground branch it invokes the existing source quaternion
and node-composition kernels, selects the ordinary collision surface's signed
plane and projects the composed point using the original separate scalar SSE
operations and `3D4CCCCD` shift constant. The active ordinary collision bank is
explicit: the item's contact BSP field does not select that global bank.

The comparison checks full return words, points, imported markers, quaternion
bases, local/world transforms, the selected current node, raw/emitted planes,
signed distance and projection shift. All 832 original current-node records are
retained as inputs; three projection calls reuse one observed SMG node/contact
state. They are not three separately produced physical settling states. The
2498 retained operand-read records are independently audited source evidence;
the native test does not claim to emit or compare a corresponding memory-read
stream.

## Connection to visibility

`source_item_pickup_visibility` checks the original unit/item type masks before
asking for the item point. It consumes the point regardless of the producer's
return word, as original `C5845` does. It then invokes the existing native
visibility consumer with the item's full identity and an explicit collision
query service. The query service in the comparison invokes the native ordinary
terrain outer kernel; expected geometry and collision results remain untouched
original observations.

The 207-case composition correlates original consumer IDs and full item/type
identities with the frozen item-point states. It uses 205 root paths and two
marker projections. The separate 208-case producer suite contains one additional
direct marker projection. Source type-rejection ordering is checked separately
with unavailable point state; those gate checks are instruction-backed checks,
not additional measured Xbox cases.

## Reproduce the checks

The differential tools pin the original manifests, source/audit tools, documents,
payloads, source dependencies and instruction listings. They require the frozen
original reexecution to remain byte-identical. Expected fixture values are not
calculated by native code.

```sh
cmake --build build-multiplayer --target item_point_tests pickup_visibility_tests -j4
python3 tools/differential_item_point.py
python3 tools/differential_pickup_visibility.py --item-points \
  --output analysis/multiplayer/integration/item-pipeline-differential.json
ctest --test-dir build-multiplayer --output-on-failure \
  -R '^(retail_item_point|retail_pickup_visibility|retail_pickup_pipeline)$'

ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
python3 tools/differential_item_point.py \
  --binary build-multiplayer-asan/item_point_tests \
  --output analysis/multiplayer/integration/item-point-asan-differential.json
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
python3 tools/differential_pickup_visibility.py --item-points \
  --binary build-multiplayer-asan/pickup_visibility_tests \
  --output analysis/multiplayer/integration/item-pipeline-asan-differential.json
```

Original data, literal exports and reports stay under ignored local asset and
analysis folders. Comparisons skip with code 77 when their local original inputs
are absent. The C++ modules build without those files.

The fresh asset-free source snapshot builds and runs57 checks:18 pass and39
original-data comparisons skip. Six focused normal checks and the same six
address/undefined/leak checks pass. A broader normal run completes132 checks
with130 passes and two failures: `retail_menu_haze_capture_gpu` rejects its
capture uniform's size, and `retail_source_frustum_camera` rejects a source
mode. Those failures are retained in `item-pipeline-all-tests.log`; this is
not a full-suite pass. The installed title and four-player Turf capture use
stable before/after runtime hashes and have no Vulkan validation diagnostics.
Records: `code-only-item-pipeline/report.json`, `48-item-pipeline-build/capture.json`
and `49-item-pipeline-turf/capture.json`. The first48 Turf attempt used a wrong
local file path and is retained as failed;49 is the successful retry.
The build-specific `build-multiplayer/Halo2-Title-Launcher` matches the captured
launcher bytes. The shared root launcher was refreshed by the frontend chat
after49 and before50; the LAN script directly ran the verified multiplayer
runtime. No later shared-launcher refresh is treated as this capture's launcher.

## Remaining producers and branches

All measured calls use unparented type2 items. Live physics/contact/node state,
parent attachment, owner redirection, mirror/region/remap behavior, instanced
contacts, negative-plane executed coverage and BR contact projection remain
open. Missing/empty ground-marker groups have a listed root path but no executed
case in this fixture. Unsupported live branches fail explicitly rather than
creating a pose or contact. The capsule comparison still receives original roots
and current crouch and explicitly configured FPCW`027F`/MXCSR`1F80`.

The production multiplayer match does not yet run this full live pickup path.
World initialization, unsorted registry ownership, bounds/pose producers,
admission, attachment and transfer order remain required integration work.
