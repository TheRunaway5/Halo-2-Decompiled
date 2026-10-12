# Original unit position for control reach

`source_unit_center` ports the base-disc `CAFC0` decisions and arithmetic used
by `1516C0`'s control reach test. Its ordinary unit branch blends the authored
standing/crouching eye heights and scale, honors the actual `E0DC0` state gate,
applies `DF380` observer correction, then issues the two literal world queries.
The queries use the **normalized** correction: first a reverse quarter-unit
segment, then a unit-length segment. A sub-epsilon correction is discarded.
The first query's miss point and the second query's contact margin are retained.

The other branches select the actual related unit's seat marker or the parent's
camera marker. A count-zero `B8D30` result still supplies its returned matrix
translation; the caller must provide the real marker producer's result.
Parent attachment position remains the actual `B9EF0` service.

The same module implements `DE190`'s player admission, head-marker fallback,
local-frame conversion, initial publication and subsequent per-axis `.01`
offset steps. `DF380` consumes that persistent byte/vector state and the
authored pitch/correction operands. `DE080` preserves the vertical-look
fallback through actual up, and `11D090` produces the original perpendicular
up vector. The ignored `A74C0` calls only test whether the vectors are
orthonormal; they do not normalize or change them.
`10F820` now reads both actual state channels: each requires a non-NONE datum
and index, flag bit0, and neither state bit0 nor bit3. It is not a substituted
admission service. The `+13C` field is a player handle, as established by its
original lookups into the `4E8C24` player array.

`322BD8`/`322BF5` keep the angle in x87 through the subsequent subtraction.
The native pitch kernel preserves that precision boundary and restores the
caller's control word. Other scalar boundaries retain the original SSE
operand order. The API requires finite operands and nearest, non-FTZ/non-DAZ
arithmetic. The x87 angle implementation currently requires GNU/Clang x86;
unsupported implementations fail explicitly. Other platforms are unverified.

## Original comparison

`tools/differential_source_unit_center.py` executes unchanged `CAFC0`, `DF380`,
`DE190`, `DE080`, `11D090`, `10F820`, `E0DC0`, `30BF0`, `17CA10`, the orthonormal
predicates and reached original CRT math. Every executed instruction is checked
against the pinned base-disc executable. Curve5 is read from its actual
`46D148` bank and frozen with the fixture.

All **1,678 cases / 10,659 output words** match, with 1,062 distinct original
instructions visited. Outputs include each declared service's exact arguments
and invocation order. Coverage includes cold/retained head offsets, marker
fallback, per-axis step boundaries, vertical and epsilon basis fallbacks,
pitch/curve gates, crouch state, parent/related-seat paths and both contacts.
Sixty-four further inputs retain the preceding original head-offset output.
Five diagnostic calls use the retained Zanzibar unit's actual fields, posed
node0, state channels and authored definition values. Its current forward angle does not
consume a marker/contact service result. That is a retained-input comparison,
not a captured control-input execution or a simulated player beside a computer.

The immutable RAM hash remains
`ab7e518eda315ecd55e681d440c988d2814260c292bbbd69972754ac7d32bf55`.
The original executable hash is
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.

```sh
cmake --build build-map-world-20261011 --target source_unit_center_tests -j 6
.tools/python/bin/python tools/differential_source_unit_center.py \
  --binary build-map-world-20261011/source_unit_center_tests
ctest --test-dir build-map-world-20261011 --output-on-failure \
  -R '^(source_unit_center_guards|retail_source_unit_center)$'
```

The frozen fixture runs without the RAM export or original executable.
Reports and build/check logs use the `unit-center` prefix under
`analysis/map-world-features/`.

All nine focused CTests pass, including unit-center guards/fixture, the control
target and spatial query checks and the shared device world. The address/
undefined sanitizer fixture and guard checks also pass. A freshly built,
frozen native executable also passes snow on/off/repeat and ordinary Zanzibar
startup with no Vulkan validation errors. The on/off images differ at 2,452
RGB pixels and the enabled repeat is byte-identical. Source/shader observations
remain unchanged during those captures. That executable's SHA-256 is
`42aec4bfe437b8d71ba6f3c37fc2feb8e896b3f8df71acf0a6b60f527f219f18`;
the screenshots and verification are under
`analysis/map-world-features/unit-center-live/`. These are weather/startup
regressions, not evidence that the new unit-center owner runs in gameplay.

## Remaining owners

World roots/attachment positions, marker posing, `CBA50` aim clamp and `1697C0`
contact production are declared services in this
comparison. Their producer behavior is not established by replacing those
calls. A separately implemented [world-contact kernel](SOURCE_OBJECT_VECTOR_CONTACT_PARITY.md)
exists; it still requires actual registered spatial, posed collision and
breakable-state banks. The API does not supply those live banks or replace
them with render geometry.

Normal multiplayer does not yet feed this module its live posed state and
services. Control-marker posing, live registration and original player-action
dispatch remain open. The separate TU1.1 runtime also needs verified matching
routine/field bindings; these base-disc addresses cannot simply be reused.
This work does not make Zanzibar's computer interactive yet. The
[all-map goal](MAP_WORLD_FEATURES.md) remains active, with
[visible Lockout snow](SOURCE_WEATHER_LIVE_SNOW.md) as its live checkpoint.
