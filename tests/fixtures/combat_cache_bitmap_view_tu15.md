# TU1.5 cached bitmap metadata caller oracle

Generate the ignored fixture using an owned English TU1.5 executable:

```sh
.tools/python/bin/python tools/oracle_cache_bitmap_view_tu15.py \
  --xbe '/path/to/owned/u5English/default.xbe' \
  --fixture tests/fixtures/combat_cache_bitmap_view_tu15_original.json
cmake --build build --target combat_action_tu15_tests
ctest --test-dir build -R '^original_tu15_cache_bitmap_view$' --output-on-failure
```

The oracle executes the complete original `1D2D0` caller and its original
metadata/address helpers, including `407C00`, without replacing callee results.
It supplies declared texture metadata and the original API's device input.
The native boundary serves only the authenticated CPU metadata query at
`1D2EA`; it does not create or emulate an Xbox GPU device.

The 200 cases cover twenty formats, five sizes and both linear and swizzled
storage. The comparison checks every GP/SSE output, defined arithmetic flags,
the caller/helper instruction sequence outside the native API boundary, the
observed output, input texture and return arguments. The original caller never
reads GPU multisample state; the native boundary leaves that field untouched.
Dead stack locals are excluded from the comparison.

Run the generated fixture with `combat_action_tu15_tests`, or CTest
`original_tu15_cache_bitmap_view`. This is CPU metadata/ABI evidence and does
not establish final particle rendering parity.
