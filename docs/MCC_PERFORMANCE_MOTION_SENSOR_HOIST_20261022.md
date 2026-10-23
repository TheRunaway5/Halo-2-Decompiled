# Motion-sensor invariant computation and private output storage

2026-10-22. Read-only research; canonical production code was unchanged by this investigation. The one bounded component experiment supports preparing the existing texture sample metadata once per blip/pass and borrowing the new output image's writable bytes once. At the ordinary MCC radar size128, the combined variant used22.52% less process CPU time than the unchanged rasterizer in this synthetic component cohort. This is sufficient evidence for a narrow production patch with direct regressions; it is not an integrated FPS result or acceptance of the4K runtime goal.

The complete candidate and test-driver sources, compiler commands, hashes, all timing samples, and reference/combined disassembly are retained in [evidence.json](/home/eric/Developer/Halo-2-Decompiled/build/test-results/mcc-motion-sensor-hoist-20261022/evidence.json). The reviewable [candidate.patch](/home/eric/Developer/Halo-2-Decompiled/build/test-results/mcc-motion-sensor-hoist-20261022/candidate.patch) applies only to `motion_sensor.cpp`. No production reference switch or alternate runtime was added.

## Exact scope and priority

The existing720p sampled runtime profiles identify the four-argument composite leaf at1.9593%,3.4066%, and4.6705% for1/2/4 local players respectively: [1p](/home/eric/Developer/Halo-2-Decompiled/build/test-results/mcc-performance-goal-20261022/runtime-attempt3/profiles/1p-profile.json), [2p](/home/eric/Developer/Halo-2-Decompiled/build/test-results/mcc-performance-goal-20261022/runtime-attempt3/profiles/2p-profile.json), [4p](/home/eric/Developer/Halo-2-Decompiled/build/test-results/mcc-performance-goal-20261022/runtime-attempt3/profiles/4p-profile.json). Those profiles identify runtime SHA-256 `efe69297809e687bb928dba551934e5a4be2aaeffebfc14e24b759463df5bcee`; they are priority evidence, not measurements of this candidate. Component speedup cannot be multiplied into a certified FPS result. A controlled runtime A/B remains required.

The actual caller already avoids duplicate work for the same tick/unit/view at [multiplayer_hud.cpp:761](/home/eric/Developer/Halo-2-Decompiled/src/ui/hud/multiplayer_hud.cpp:761). Each view owns its history and material. Missing source ticks age history without inventing contacts; candidates retain source datum order; source history, pulse, and representation-specific color feed the rasterizer at [multiplayer_hud.cpp:766](/home/eric/Developer/Halo-2-Decompiled/src/ui/hud/multiplayer_hud.cpp:766) and [multiplayer_hud.cpp:807](/home/eric/Developer/Halo-2-Decompiled/src/ui/hud/multiplayer_hud.cpp:807). MCC materials use128×128 at [multiplayer_hud.cpp:691](/home/eric/Developer/Halo-2-Decompiled/src/ui/hud/multiplayer_hud.cpp:691). No cross-tick/view cache or change to30Hz source history is proposed.

The component reference is the unchanged current four-argument rasterizer, with these source SHA-256 values:

| Source | SHA-256 |
| --- | --- |
| `src/ui/hud/motion_sensor.cpp` | `5618997563b135f586b500918cd7427fa97e3bc6f1603d824d80281137291f8d` |
| `src/ui/hud/multiplayer_hud.cpp` | `280fa79eaa0f40a523640b2c66e56b94be10ceea970f3dd9d2d21ee0e4855d6c` |
| `include/halo2/texture_pixels.hpp` | `08784641b70a9817662a1d98c2616beab159bd821ad7af18ea8b7474af39a7d4` |
| `include/halo2/cache.hpp` | `d68153411b3e4d5b2f8b03cd85719258d28e7842004a8f0d7f53317001ae6bd7` |

These hashes were rechecked before the probe finished. Line references describe that reference version; subsequent integration by the parent task can change its lines/hash. Applying the retained candidate patch produces motion-source SHA-256 `262dde0f087dc2fcb31289feb2f10ab55b5160eb15fa55314affc8019ab5046a`.

## Two independent seams

**Prepared texture metadata.** The current sample lambda repeats nearest-mip selection, width/height selection, and mip offset lookup at [motion_sensor.cpp:120](/home/eric/Developer/Halo-2-Decompiled/src/ui/hud/motion_sensor.cpp:120). The blip's logarithm depends only on texture dimensions, snapped edges, and raster density, yet appears inside both pixel loops at [motion_sensor.cpp:167](/home/eric/Developer/Halo-2-Decompiled/src/ui/hud/motion_sensor.cpp:167). The sweep logarithm likewise depends only on this call's pulse, sweep width, and raster size at [motion_sensor.cpp:177](/home/eric/Developer/Halo-2-Decompiled/src/ui/hud/motion_sensor.cpp:177). Mask LOD is always zero.

The tested candidate splits metadata preparation from bilinear sampling. It retains a const `Texture*`, selected width/height, and byte offset; it continues reading input bytes through the same const `TexturePixels` API. It prepares each blip only after both snapped-extent and clipped-rectangle emptiness checks. In particular, positive extents can clip to an empty rectangle: the added `left>=right || top>=bottom` guard must precede the hoisted `log2`, because the reference executes no samples/logarithms for that rectangle. Sweep and mask preparation remain inside `plan.visible`, after the blip passes and original allocations.

**One private output writer.** The final export at [motion_sensor.cpp:194](/home/eric/Developer/Halo-2-Decompiled/src/ui/hud/motion_sensor.cpp:194) calls the mutable pixel accessor four times per output pixel. Each reaches `TexturePixels::write()`, which checks allocation/sharing, clears fingerprint readiness, and marks exposure at [texture_pixels.hpp:33](/home/eric/Developer/Halo-2-Decompiled/include/halo2/texture_pixels.hpp:33). There is no revision counter. This function's `out` is newly resized and remains private/unshared until return; sampling/quantization neither publishes it nor takes copies. Borrowing `out.pixels.data()` once just before export preserves the final invalidated/exposed state and writes the same bytes in the same channel order. This argument applies to this private output, not to arbitrary shared textures or source-runtime memory stores.

Both variants retain edge snapping, coverage, UV divisions, original arithmetic order, bilinear tap order, the `70/255` border alpha, additive-history/sweep/mask order, per-pass quantization, RGB-only destination writes, and zero destination alpha. No reciprocal replacement, logarithm approximation, texture format change, global compiler flag change, or parallel scheduling is involved.

## Compiler evidence

The probe compiled the unchanged reference and three separate candidate translation units using `g++ (GCC)16.2.1 20260810`, `-O3 -DNDEBUG -std=c++20 -ffp-contract=off`, with no LTO or fast-math. This preserves the existing motion source's GCC contraction setting at [CMakeLists.txt:663](/home/eric/Developer/Halo-2-Decompiled/CMakeLists.txt:663); MSVC uses `/fp:strict` at [CMakeLists.txt:648](/home/eric/Developer/Halo-2-Decompiled/CMakeLists.txt:648). It is a private component compile, not proof of identical code generation in the admitted runtime.

In the reference object's four-argument composite, whose `.text` starts at`0xD60`, `log2f` remains at`0x15A9` inside the blip pixel backedge `0x1BF3→0x1560`, and at`0x1D25` inside the sweep pixel backedge `0x28DD→0x1CF0`. The export calls `TexturePixels::write()` at`0x1065` inside the four-channel backedge `0x1079→0x1042`. These are directly observed missed hoists; the compiler has not already removed them.

In the combined object's composite, starting at`0`, sweep preparation calls `log2f` at`0x66A` before the sweep pixel body, while blip preparation calls it at`0x16AF` before blip pixel loops `0x1DF1→0x1920` and`0x1E1F→0x1820`. The export calls `write()` once at`0x12FC`, loads the output pointer at`0x1309`, then exports pixels at`0x1330` with backedge `0x13B6→0x1330`. The original `roundf` calls remain in export. Full disassembly and symbol sizes are in `evidence.json` under `assembly.reference` and `assembly.sensor_combined`.

## One bounded differential and component experiment

The authorized probe ran once after the unsampled runtime matrix ended. An earlier attempt stopped at a driver-only undeclared `tick_seconds` compiler error before execution; the corrected driver passes the explicit source interval `1.f/30.f`. No candidate result came from that failed compilation.

The successful execution completed339 cases and1,017 reference/candidate comparisons. Each case runs reference, pointer-borrow-only, prepared-sampler-only, and combined variants. It compares every RGBA byte, fingerprint, width/height/format/layers/depth, and all mip metadata. It then performs5,424 individual copy/exposure/isolation assertions across the1,356 returned images: copies of the exposed output must own different storage; mutations before and after a retained writer cannot change earlier/later snapshots; mutating a copy cannot change its source.

Coverage is deliberately broader than a single visible radar:

- All eight allowed raster multiples64,128,192,256,320,384,448,512; four valid synthetic artwork cohorts; eight plans per combination:256 cases.
- Artwork:32×32 without mips,64×64 mipped,48×31 and96×48 mipped with transposed mask/custom dimensions. Each level has distinct gradients/alpha, valid offsets and sizes, a complete chain to1×1, and13 padding bytes between levels.
- Plans include hidden and visible-empty output, regular/custom contacts, overlapping colors/history scales, borders and fully offscreen contacts, fully clipped positive rectangles, positive extents collapsing after1/16-unit snapping, negative skipped intensity, and near-edge/fractional coordinates.
- Every one of63 source pulse phases with a genuine ten-slot source history; four160-blip capacity histories; and16 control cases across nearest/downward/upward/toward-zero rounding, four plans per mode. These are additional cases, not the full Cartesian product of every pulse/rounding mode/artwork/raster.

For every function call, the driver seeds `errno`, clears FP exceptions, pre-seeds divide-by-zero on alternating cases, sets the requested rounding mode, and compares terminal `errno`, `FE_ALL_EXCEPT`, `fegetround()`, full MXCSR, x87 control word, and x87 status exception/TOP bits (`0x383F`). Every comparison matched. The driver restores the saved environment after each call. x87 condition codes and FP register contents are not part of the comparison.

The tests admit finite source plans with positive finite pulse, valid texture/mip dimensions and ranges, and finite pixel coordinate/conversion intermediates. They do not prove behavior for NaNs/infinities, malformed mip descriptors, out-of-range float-to-integer conversions, allocator failures, unmasked traps, interrupted/asynchronous FP observations, alternate libc/compiler/architecture, or every finite float bit pattern. Actual imported artwork and original capture fixtures were not loaded by this synthetic component probe.

The floating-point boundary matters even though the arithmetic is invariant. Mathematical functions can change exception flags and `errno`, as documented by [glibc's math error reporting](https://sourceware.org/glibc/manual/latest/html_node/Math-Error-Reporting.html). Reducing repeated calls changes operation/trap timing outside the tested terminal-state contract. GCC documents default math-errno behavior and the broader semantic relaxations of fast-math in [Optimize Options](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html). No such relaxation was enabled. The nondefault-rounding cases establish equality of this compiler's generated variants for those cases; they do not turn GCC's default `-fno-rounding-math` build into a universal dynamic-rounding guarantee.

Timings use `CLOCK_PROCESS_CPUTIME_ID`: median of five rotating-order samples, two full composites per sample, per raster and variant. Fingerprinting, whole-image comparison, and copy/isolation checks occur outside the timed region. The benchmark is single-threaded and bounded; the parent was performing image-only validation on the same host. Values below are CPU milliseconds per call; speedups describe this synthetic eight-contact plan only.

| Raster | Reference | Borrow | Prepared | Combined | Combined CPU reduction |
| --- | ---: | ---: | ---: | ---: | ---: |
|64|0.560|0.526|0.462|0.444|20.68%|
|128|2.203|2.114|1.787|1.707|22.52%|
|192|4.972|4.691|4.021|3.856|22.44%|
|256|8.796|8.306|7.065|6.747|23.29%|
|320|13.794|13.151|11.157|10.600|23.16%|
|384|19.791|18.788|15.913|15.227|23.06%|
|448|26.602|25.387|21.547|20.555|22.73%|
|512|34.298|32.737|27.231|26.115|23.86%|

At128, the separate improvements are4.01% for the borrowed output and18.86% for prepared sampling. The combined comparison is approximately1.291× faster, avoiding about0.496 CPU ms in this component case. Neither those absolute times nor their percentage are a measurement of the live game's sensor cost or FPS gain.

## Integration and remaining acceptance

The observed unchanged output, measured component gain, and actual assembly loop placement justify the narrow candidate. Keep a persistent exact-output regression for the admitted finite domain, including clipped-empty rectangles, mips,64..512 raster bounds, returned exposure/copy behavior, and terminal FP/errno checks. Validate the actual production compiler and imported artwork before admitting an optimized runtime.

Existing direct regressions worth retaining/running during integration are:

- [hud_sensor_tests.cpp:20](/home/eric/Developer/Halo-2-Decompiled/tests/hud_sensor_tests.cpp:20): exact RGB-only alpha, intensity-based extent, and sweep border alpha. Its `live()` checks actual artwork across all63 phases and four independent views; CTests are `native_hud_motion_sensor` and `native_hud_motion_sensor_viewports` at [CMakeLists.txt:4220](/home/eric/Developer/Halo-2-Decompiled/CMakeLists.txt:4220).
- [hud_sensor_tests.cpp:125](/home/eric/Developer/Halo-2-Decompiled/tests/hud_sensor_tests.cpp:125): exact original xemu target comparison. This helper writes `native-sweep.rgba` into its capture directory at line154; run it only against an owned scratch capture context, preserving original references.
- [hud_4k_tests.cpp:131](/home/eric/Developer/Halo-2-Decompiled/tests/hud_4k_tests.cpp:131): actual-artwork default-raster byte identity across63 phases/both appearances, higher-density RGB-only alpha/corner checks,1..4 view positions, and invalid raster65 rejection.
- [motion_sensor_tests.cpp:94](/home/eric/Developer/Halo-2-Decompiled/tests/motion_sensor_tests.cpp:94): original movement/visibility, quantized sample, pulse and history ABI fixtures. These validate source producers; they do not independently certify composite pixels.
- [hud_render_tests.cpp:247](/home/eric/Developer/Halo-2-Decompiled/tests/hud_render_tests.cpp:247): visible Vulkan contact/pulse changes and protected sensor corners across1..4 views. This is a separate integration boundary.

None of these existing CMake suites was run by this read-only subtask; the parent owns integrated validation. A falsifiable follow-up is an alternating, hash-pinned ordinary-runtime A/B using identical prepared MCC inputs, vector path, view count, raster and30Hz history, with separate exact framebuffer evidence and unsampled FPS/CPU medians. The component result alone does not establish4K120/4K60.

## Reproduction and storage

The probe used `python3 tools/run_test.py -- python3 -` as coordinator. `evidence.json.driver_source` and `variant_sources` contain the complete private C++ inputs; `compile_commands` records all object and link commands. The unchanged reference was the pinned canonical source above, compiled as its own translation unit. Scratch compiler outputs, executable and source copies were under `HALO2_TEST_SCRATCH`; no source tree, full build, runtime build, asset copy, or capture bank was created.

The failed compile owner `/tmp/halo2-test-u0muirou` and successful owner `/tmp/halo2-test-3rhfxjwj` were removed by the wrapper. [storage-receipt.json](/home/eric/Developer/Halo-2-Decompiled/build/test-results/mcc-motion-sensor-hoist-20261022/storage-receipt.json) verifies removal and records retained file sizes/hashes. Retention is bounded text evidence and the small candidate diff only; no large generated artifacts remain.
