# TU1.5 particle effect color oracle

```sh
.tools/python/bin/python tools/oracle_particle_effect_color_tu15.py \
  --xbe '/path/to/owned/u5English/default.xbe' \
  --fixture tests/fixtures/particle_effect_color_tu15_original.json
cmake --build build --target particle_effect_color_tu15_tests
ctest --test-dir build -R '^original_tu15_particle_effect_color$' --output-on-failure
```

The oracle requires the authenticated English TU1.5 SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
It executes complete unchanged `22AD40`/`22AD50` getters, then complete
`178920` with its real `1346C0` and `1349E0` callees. It supplies declared
particle flags, prior system color and both effect colors. No callee output,
allocation or effect admission is supplied.

The 3,366 cases cover both authored bits, unrelated bits, neutral/boundary
colors and seeded random channel values. They compare the native kernel's
packed output against the original whole routine, using nearest rounding
and original x87 control `027F`. This is color arithmetic evidence, not a
matched Xbox pixel comparison.

TU1.5 `176E70` initializes system color to `FF808080`. `17BB10` and
`179EB0` supply effect body `104` and `108`; flag bit7 selects multiplication
and bit8 selects tinting. RGB packing intentionally has no alpha byte in
the tint branches, while the multiply-only branch copies the full first
color word. The native source explosion journal retains both original
effect colors through LAN transport.
