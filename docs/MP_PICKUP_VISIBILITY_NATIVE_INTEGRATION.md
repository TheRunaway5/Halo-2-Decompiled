# Native capsule and pickup visibility consumer

The bounded native `E0C20/30BD0/C57A0` kernels now match **207 original capsule/
visibility calls,187 exact normalizations and1,513 ordered terrain casts**.
There are21 visible and186 blocked results, including374 clear target clips.
These kernels are implemented in [pickup_visibility.cpp](../src/pickup_visibility.cpp)
and compared with untouched original-executable outputs. They do not yet run
the live multiplayer pickup path: item points, current object/node state,
registration and Havok settling remain separate producers.
[Measured authority](MP_PICKUP_CONSUMER_MEASURED_RESEARCH.md),
[observed casts](MP_PICKUP_CONSUMER_FIXTURES.md),
[live producer contract](MP_PICKUP_NATIVE_CONTRACT.md).

## Implemented source boundaries

`Cache::load_pickup_capsule_definition` reads actual resident `bipd` metadata
at`+264..273`. It translates metadata addresses through the map's metadata
mapping and checks the tag bounds; encoded resource-file locations are a
different address space. `pickup_capsule` receives the original root-service
result and actual current crouch. It preserves the separate float32 radius,
height-interpolation and diameter operations. Neither current crouch nor root
is inferred from previously emitted capsule geometry.

The side-vector consumer preserves the original cross-product multiply/
subtract order, including signed zero. `pickup_side_normalizer` forms the
original x²+y²+z² and square root at53-bit precision, retains that double
length, spills float32, divides one by the spill, and multiplies each component
separately. The comparison checks the captured valid x87 length, spill,
reciprocal, input/output/return and source-selected side vector. The source
fixture explicitly configures FPCW`027F` and MXCSR`1F80`; this is not a newly
recovered Xbox hardware initialization or reciprocal guarantee.

`pickup_visibility` preserves original type masks and short-circuit order:
top and bottom direct tests, both item-center-to-shifted-endpoint clips, then
positive/negative top and bottom side tests. The original constants are
preserved as scalar instruction inputs. An explicit outer-query service owns
each collision result; the consumer retains unit/item ignored identities,
start/delta/mask and optional hit-driven endpoint replacement.

The test's service invokes the actual native
[ordinary terrain outer kernel](MP_TERRAIN_OUTER_NATIVE_INTEGRATION.md) for
every emitted cast. It compares original start/delta words, mask/ignored full
identities, order, return AL, fraction, point and both eight-byte locations.
Expected casts and normalization values are copied from independent original
observations; native code never generates their expected geometry.

## Verification

[Differential tool](../tools/differential_pickup_visibility.py) pins the
observed and measured manifests, source/audit tools and documents, original
dependency chain and every payload. Its conversion of a captured53-bit x87
length to a double bit pattern is a representation conversion, rather than a
new square root calculation. The separate original audit checks all9,792
instruction rows,107 source pins and both complete original baseline repeats.

The normal differential and CTest check pass. The same differential and CTest
check pass with address/undefined/leak sanitizers. Twelve prerequisite normal
checks and four collision/remote-binding sanitizer checks also pass; these
results are separate from the earlier full105-test clock checkpoint.

```sh
cmake -S . -B build-multiplayer -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-multiplayer --target pickup_visibility_tests -j4
python3 tools/differential_pickup_visibility.py
ctest --test-dir build-multiplayer --output-on-failure -R '^retail_pickup_visibility$'

python3 tools/differential_pickup_visibility.py \
  --binary build-multiplayer-asan/pickup_visibility_tests \
  --output analysis/multiplayer/integration/pickup-consumer-asan-differential.json
```

Records are under ignored `analysis/multiplayer/integration`:
`pickup-consumer-differential.json`, `pickup-consumer-asan-differential.json`,
`pickup-consumer-tests.log`, `pickup-consumer-asan-tests.log`, and the two
`pickup-consumer-prerequisite-*-tests.log` files. Original assets remain local
and excluded from the source repository. Missing original files make the
comparison skip with code77.

The initial native importer comparison failed because it used the encoded
resource path for a metadata address. That importer was corrected; the first
failed logs remain, and the unchanged original expectations now pass. No source
fixture or original report was changed to accommodate native output.

## Remaining scope

All207 measured cases use unparented type0/type2 objects, current crouch zero
and metadata flags4. Other flags/crouch/parent/type cases have instruction
evidence but no executed comparison here. All187 normalizations take the
non-small-length path; small-length fallback and unordered behavior remain
unexecuted. All374 target clips are clear; clip-hit replacement stores are
implemented from instructions but have no measured hit case. Only selected
source geometry buffers are compared directly; all2,304 exported `G` records
remain provenance, not a claim that every internal store was compared.

The original consumer-only comparison receives captured `FBFA0` outputs as
inputs. The additional `retail_pickup_pipeline` comparison now produces all 207
points natively from the literal source current state, including the two
marker/contact cases, and passes them directly into visibility and terrain.
Its type gates precede point production and its point consumption does not test
the producer's return word. See [item-point composition](MP_ITEM_POINT_NATIVE_INTEGRATION.md).
This does not produce live item settling, current
marker/node poses, full collision dispatcher contestants, registry ordering,
pickup admission/transfer, or inventory/world allocation. The native match
does not substitute a host grounding flag, guessed pickup distance or invented
contact to bypass these gates. Full sandbox parity remains unfinished.
