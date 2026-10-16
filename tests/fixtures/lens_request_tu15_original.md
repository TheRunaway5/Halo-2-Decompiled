# TU1.5 lens request and frame fixtures

The generated JSON files are intentionally ignored because they contain
original executable/cache bytes. The original authority is English Xbox
TU1.5 `default.xbe`, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

From the repository root, with the owned executable and audited caches present:

```sh
.tools/python/bin/python tools/oracle_lens_request_tu15.py --fixture tests/fixtures/lens_request_tu15_original.json --report analysis/map-world-features/lens-request-20261015/original-request-regenerated.json
.tools/python/bin/python tools/oracle_lens_frame_tu15.py --fixture tests/fixtures/lens_frame_tu15_original.json --report analysis/map-world-features/lens-request-20261015/original-frame-regenerated.json
ctest --test-dir build-weapon-zoom-hud-20261014 -R '^original_tu15_lens_(requests|frame)$' --output-on-failure
```

Both generators reject existing output files to preserve evidence. The first
fixture contains 72 actual authored-definition request cases; the second has
42 frame/view/reset cases. Original instructions and reached services execute
unchanged. Native tests compare complete instruction paths, registers, defined
flags and retained memory. These fixtures do not supply GPU visibility or
establish complete native display scheduling. See
[the lens continuation](../../docs/SOURCE_MAP_LENS_REQUESTS_TU15.md).
