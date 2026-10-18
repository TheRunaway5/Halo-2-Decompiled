# TU1.5 cinematic far-clip oracle

`retail_cinematic_far_clip_tu15.inc` is an optional generated oracle. It is
ignored with the other original-derived fixture data. The normal test target
builds without it and returns skip code 77; original bytes are not required
for an ordinary checkout to compile.

Generate it from the original Xbox TU1.5 executable and the original campaign
cache, using a Python environment containing Capstone and Unicorn:

```sh
.tools/python/bin/python tools/generate_campaign_far_clip_fixture.py \
  --xbe "/path/to/TU1.5/default.xbe" \
  --map assets/disc/maps/06b_floodzone.map
```

The executable must have SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The generator authenticates the three original Floodzone nodes for
`c06_intra2_02_setup` and its REAL 2048 argument. It executes unchanged
original instructions for 16 complete HS-dispatch/store cases, 72 configured
near/far selection cases and two screen-stop resets. No instruction patch,
callee stub or supplied function result is used. `--report PATH` optionally
saves the detailed ignored JSON evidence. An existing differing output is
preserved and rejected.

The native test uses a declared isolated VM envelope around those authenticated
nodes: unrelated pool rows have free salts and the outer sibling link is
terminated. It also checks exact source-word publication, native-cold factory
backing without a world-XBE path, lifecycle resets, checkpoint/replay and the
separate configured W distance and clipped camera far inputs.

These are source and CPU consumer controls. A changed `ModelView` ABI requires
a complete coherent caller/renderer rebuild. Campaign GPU submission and
matched original-frame appearance remain separate checks.
