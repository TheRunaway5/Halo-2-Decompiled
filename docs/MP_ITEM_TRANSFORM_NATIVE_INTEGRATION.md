# Native item matrices, current nodes and bounds

The native matrix operations match **46 original calls**: sixteen `143960`
node conversions, twenty-six `1441D0` compositions and four `BEB30` root
matrices. Complete pointed buffers, source aliases and **446 ordered writes**
agree in normal and address/undefined/leak sanitizer builds. Seventeen
compositions overwrite their left or right input. Source authority:
[independently repeated original producers](MP_ITEM_TRANSFORM_PRODUCERS_RESEARCH.md).

The port preserves separate SSE arithmetic, shuffle/sign operations, intermediate
overwrites and signed zero. Composition reads the left basis before writing and
reads the right columns in the original reverse order. The original no-parent
root path retains scale composition and the actual low-byte handedness test;
the SMG's upper handedness word bits do not select the negative basis branch.

The sixteen original `RCPSS` results remain an explicit instruction boundary.
Their actual inputs are checked before delivering measured original results;
subsequent refinement and matrix formation are native. Those observations came
from unchanged original execution in Unicorn, and do not establish Xbox
reciprocal hardware parity. Degenerate quaternion, missing lazy initialization,
parent and negative-handed root branches are not measured by this comparison.

`BDF80` current-node production and `BEDE0` bounds separately match **eight
original calls** against complete captured raw state, with **384 ordered
body/fallback writes**. The native nodes use source descriptor sizes/offsets,
literal weapon/model/render records and original render-node parents. Both
measured render tags have parents `[-1,0,0,0]`. The BR's default local records
are produced natively in the actual fallback bank; the SMG consumes the local
records previously produced by the original animation path. Native node math
then fills the complete current bank in source order. The actual three NULL
type notification entries per call are explicit inputs; positive callbacks
remain unavailable.

Native bounds retain original primary-center operation order and scale, radius,
alternate-sphere copies and registration-sphere choice. The two already
registered dirty calls return AL0 while retaining their actual no-op sphere
stores; the two unregistered transforms return AL1. Original physical components
are NONE. Changed registered spheres require the separate unregister/location/
registry producers; those branches fail explicitly here.

These producers are compiled native components, not yet replacements for all
of the item-motion consumer's explicit world/contact services. Registration,
location, terrain/Havok admission, positive animation/physical callbacks,
continuous settling and the live original frame route remain open. Complete
original listings provide static information beyond the measured branches,
without manufacturing expected outputs for them.

## Reproduction

```sh
cmake -S . -B build-multiplayer
cmake --build build-multiplayer --target item_transform_math_tests item_node_bounds_tests -j4
python3 tools/differential_item_transform_math.py
python3 tools/differential_item_node_bounds.py
ctest --test-dir build-multiplayer --output-on-failure \
  -R '^(retail_item_transform_math|retail_item_node_bounds)$'

ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
python3 tools/differential_item_transform_math.py \
  --binary build-multiplayer-asan/item_transform_math_tests \
  --output analysis/multiplayer/integration/item-transform-math-asan-differential.json
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
python3 tools/differential_item_node_bounds.py \
  --binary build-multiplayer-asan/item_node_bounds_tests \
  --output analysis/multiplayer/integration/item-node-bounds-asan-differential.json
```

The exporters pin the frozen source oracle/auditor/document/report/audit/manifest,
all179 earlier dependencies and all65 byte-identically repeated instruction
files. They require unchanged original reports and explicit absence of new
guest inputs, patches or supplied success outputs. Expected buffers/stores and
all cache/scalar/reciprocal inputs come from these observations. The two fixture
exports stay in ignored analysis storage and their CTests skip77 when absent.
