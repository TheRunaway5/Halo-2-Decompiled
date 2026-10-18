# OG Xbox TU1.5 depth-of-field filter recipe oracle

The optional `retail_cinematic_dof_filter_tu15.json` stays ignored. Its native value-recipe test compiles without it and returns77 when absent; present failed, malformed or wrong-version evidence fails.

```sh
.tools/python/bin/python tools/generate_campaign_dof_filter_fixture.py \
  --xbe "/path/to/Halo 2 DLC/content/u5English/default.xbe"
```

For the complete authored request and timed completion controls, first generate
the separately authenticated state fixture and then enable campaign expansion:

```sh
.tools/python/bin/python tools/generate_campaign_dof_fixture.py \
  --xbe "/path/to/Halo 2 DLC/content/u5English/default.xbe" \
  --maps-directory "/path/to/original/disc/maps"
.tools/python/bin/python tools/generate_campaign_dof_filter_fixture.py \
  --xbe "/path/to/Halo 2 DLC/content/u5English/default.xbe" \
  --maps-directory "/path/to/original/disc/maps" \
  --campaign-state-fixture tests/fixtures/retail_cinematic_dof_tu15.json \
  --output tests/fixtures/retail_cinematic_dof_filter_tu15_authored.json
```

The expansion pins the qualified state fixture SHA256
`754c33b530454168ccf4c307f6ac21b821742c07090a4800f38f1af5cbe93df9`,
reopens all14 campaign caches and authenticates every salted9-node call/REAL
argument chain. Each of64 requests initializes a zeroed active3FC interface,
executes actual1FC10, and completes actual1FD00 at start/mid/end:192 samples.
The separate36 original timed updates are replayed from the same zeroed active
initialization and their complete interface banks must match. Only completed
output+44/+48/+40 words become the far/near/focus filter inputs; untouchedA5
setter banks are never treated as completed state. Together with the preserved
ten representative controls, the expanded fixture has238 exact command recipes.

The optional expanded JSON also stays ignored. The same compiled test accepts
either fixture path; no fixture or original asset is needed at compile time.

The generator pins the English TU1.5 executable SHA256 and executes unchanged complete21A60, shared21180/20F10,134AD0 and actual37CDE8 CRT pow arithmetic. Ten representative authored/fractional/disabled combinations record each60-word pixel-program commit, eleven34A80 draw arguments, target/stage selection and texture/render state, in source order. Only declared void native graphics state/submission calls are recorded at their real ABI; incoming scalar values are preserved rather than supplying successful returns. No target is allocated, no source instruction is patched and no pixels are supplied.

The native recipe executes statically lowered whole original source bodies using a private CPU scratch bank. It provides the command values and order needed for a renderer implementation. Resource allocation/readiness, source target dimensions, original quad coordinates, texture routing, actual near/far scene draws, Vulkan compositing, resolution scaling and matched OG Xbox TU1.5 frame proof remain separate required work. Positive blur counts above2^24 cannot advance the original binary32 pass loop and remain unadmitted by this finite recipe materializer; the source host setter retains raw words.
