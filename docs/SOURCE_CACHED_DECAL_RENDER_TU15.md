# Original cached decal renderer

The cached decal GPU consumer now renders in ordinary Zanzibar with one and
four local views. At 60 source ticks, hiding only its draws changes **2,680
pixels** with one view and **605 pixels** with four views; enabled repeats are
byte-identical and all six runs have zero Vulkan validation errors. The earlier
impact-sound and collision stops are passed. This remains bounded native GPU
evidence, not complete Xbox rendering or all-map interaction acceptance.

The production path is `SourceDecalScene` in `multiplayer_runtime.cpp`, followed
by `SourceDecalRenderer` in `renderer.cpp`. It reads completed, source-owned
cache pages and actual cluster heads. It neither projects substitute polygons
nor loads the captured test fixture into gameplay. The existing authenticated
native CPU cache view supplies real physical backing at original `183E17`.
It does not manufacture an Xbox graphics command buffer.

## Original contracts

The authority is the owned English Xbox TU1.5 executable, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`, and
original version-8 map resources.

* Whole `44030` traverses the layer/cluster lists, reads the retained bitmap
  image index and packed color, and stamps the primary record to suppress
  repeated submissions across clusters. Its recorded original fixture produces
  **20 draws / 22 quads** from Zanzibar's **28 records / 32 stored quads**.
  Native draw planning matches that exact draw order and primary identity.
* Whole `43A30` supplies all six literal 60-word pixel programs, RGB-only
  writes, original blend factors/equations, CCW culling, LEQUAL without depth
  writes, linear/clamp sampling, and integer W-depth bias **-200**. The source
  programs and states match the executable fixture.
* Rasterizer-global vertex shader index3/class0 is the actual decal program:
  13 instructions, 212 bytes, declaration `{0,3}`. Build-time translation is
  authenticated again against the actual loaded program. Original packed
  signed 11/11/10 UV/brightness, immediate RGBA attribute10, and zero attribute11
  feed this program. No expected vertex bank is embedded.
* Vulkan uploads actual cached XYZ/packed projection vertices and authored
  bitmap mip chains. Its geometry/fragment stages preserve the source screen
  vertices for the existing original W-depth reconstruction and D24 rounding.

`tools/oracle_cached_decal_draw_tu15.py` executes the whole original `44030`
against the completed captured cache. Four declared SDK sinks record graphics
arguments and return at their stated ABI; no instruction is patched and the
recorded Xbox command buffer is never submitted. Complete admission of all
512 head slots is a fixture input, not a camera/PVS claim.

`tools/oracle_decal_pass_tu15.py` executes six whole original raster/program
producers, 537 color cases and 2,025 fade cases. `source_decal_draw_tests`
checks the six programs/states and exact retained draw sequence. The original
cache export and 180-case native CPU vertex-view checks also pass.

## Live continuation work

`analysis/map-world-features/decal-consumer-20261014/` contains the frozen
source manifest, explicit fixes manifest, original disassemblies/registrations,
build logs, original comparison fixtures, failed live receipts, and the cold
21-map source-tick bank. The module currently contains **186 complete original
bodies / 14,749 instructions**, compared with the installed 108-body module.
The new 1,038-case bank compares full GP/XMM/flags, ordered instructions/writes,
and retained fixture banks for locale conversion, UTF lookup/copy, non-template
formatting, queue admission, constructors, and scalar property callbacks.
This bounded bank does not independently cover every new body or call path.

The current native interface is English. Its explicit XGetLanguage boundary
returns raw English code1 only to authenticated original callers; original
`11F440` converts it and `1A9D90` owns the cached language field. This is a
native platform input, not discovery of the Xbox console's language setting.
Loaded locale resources and template-token expansion remain separate owners.
The fresh test copy also incorporates the shared source's original player
history/feedback constructors and map resets, retaining genuine allocations.

## Remaining acceptance

The `capture.py` test uses ordinary source startup, neutral controls, a declared
auxiliary seed and tick cadence. No source geometry, object, camera or driver
is replaced. Its eight-tick enabled/hidden/enabled pixel comparison succeeds
with 25 live draws / 41 quads admitted to the ordinary view. The old captured
fixture's 20/22 count describes different declared complete head admission and
cache state; it is not substituted into this live run.
`HALO2_REFERENCE_CACHE_DECALS_HIDE` suppresses only native draw submission;
source draw planning and its stamp side effects still execute. The retained
receipts include `captures/zanzibar-1-verification-8.json` and the 60-tick
comparisons; its PNG and difference
image show the authored caution markings and wall sign producing pixels.

The latest completed cold bank passes **all 21 Xbox maps**, improving the
installed build's 13 passes. Gemini's null collision-instance mask now has its
actual 219080 constructor and 2190C0 BSP-load lifecycle. The original 64-byte
allocation and complete authored bit3 exclusion logic execute; no all-visible
mask is injected. Constructor row 44B7AC and BSP-load row 44B7BC authenticate the
phase distinction. Its original leaf mask and random-direction lookup are
included in the 1,038-case bank. All 21 successes still describe one source tick.

The final rendered continuation preserves Terminal's 1,200 ticks with one view
and 240 with four views. Lockout snow changes 20,078 pixels with identical
repeats. Foundation's lift attachment fixture changes 38,829 pixels with
identical repeats at its declared root/camera, with ordinary startup tracked
separately. The consumer admits original Xbox caches; the two Vista maps still
require their own live rendering path.

Per-view serial increments reproduce the recovered low16 primary stamp
contract; complete original render-context chronology remains unestablished.
The native scene scheduler also needs exact interleaving of texture-mapping
layer2 between original decal layers1 and2. Unsupported bitmap fallback,
complete original material/lighting chronology, Xbox frame comparison,
long-lived contacts/effects, network authority, late joins and resets remain
open. Metadata, a CPU fixture or an unsubmitted upload does not close them.

Both installed normal launchers select `.local/map-world-20261014-v3`,
SHA-256 `c57be1fc4e0520e03090733f31d55429ffe8d63ab86f8017b113d860c76a3abc`.
Both installed `--launcher-info` outputs verify that exact runtime. The final
packaged binary passes Zanzibar 60 ticks with one/four views, Lockout 60,
Gemini 15 and Turf 15 with zero Vulkan validation errors.
Their MCC-only readiness gate remains active and reports native MCC gameplay
unavailable. Direct original-XBE tests do not establish normal MCC gameplay.
The all-map world effects/interactions goal remains active.
