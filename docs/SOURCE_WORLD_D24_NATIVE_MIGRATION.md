# Native integer-depth resources and source encoding

The live renderer now selects the original integer D24S8 depth/stencil format,
requires its actual image capabilities, and preserves stencil through main
LOAD continuations. Fog retains a separate depth-only sampled view. The main
image can supply real depth/stencil aspect copies for the exact weather alias.
World and light-volume fragments now use the original power-of-two/next-float
integer encoding. Source sky stencil writes, conditional clear scheduling and
live weather producers remain separate work.

Literal original12490 selects D24S8. The independently captured original
Lockout caller retains the integer setting for128C0. The alternative floating
depth setting is not silently represented by this native integer path.
See [the original resource audit](SOURCE_WORLD_D24_MIGRATION_AUDIT.md) and
[the original sky chronology](SOURCE_SKY_STENCIL_CHRONOLOGY_AUDIT.md).

## Resource changes

- Device selection requires a present graphics+compute queue and actual D24S8
  attachment/sample/transfer-source support. The combined image-usage query
  checks single-sample, mip and layer availability; allocation checks the real
  drawable extent. An unsupported GPU receives an explicit failure rather
  than a D32/D16 substitution for packed original depth.
- The current-size main image adds transfer-source usage. Framebuffers use a
  combined DEPTH/STENCIL view; fog and patchy descriptors keep the DEPTH-only
  view. Independent water depth uses a combined attachment view too.
- The initial main pass keeps existing native `{depth1,stencil0}` initialization,
  then stores both aspects. LOAD continuations load stencil. In-pass depth-only
  clears keep their existing masks.
- The external-to-subpass dependency includes depth/stencil attachment READ
  as well as WRITE, so continuation LOADs participate in its visibility scope.
  This changes no authored clear, comparator or stencil operation.
- An opt-in `HALO2_DEPTH_DIAGNOSTICS` capture reads the actual two aspects after
  rendering and restores the attachment layout. The waited frame fence and
  transfer-to-host barrier cover both readbacks. One depth DWORD carries low24
  depth bits; the upper byte is unspecified copy padding and is ignored in
  comparisons. Stencil is a separate tightly packed byte plane.
- Swapchain recreation destroys both views after dependent services and
  framebuffers, under the existing device-idle lifetime boundary.

Independent source review found no current actionable resource/lifetime defect.
It explicitly identified accumulation's copied initial CLEAR/UNDEFINED stencil
policy: that pass still clears the shared image within its render area. This is
consistent with the current zero/no-writer native stage, but needs the actual
source ordering before a live stencil writer is enabled. A blanket preservation
or clear rule is not inferred from the background geometry.

## Coherent native build and retained comparison

`analysis/multiplayer-shading/world-d24-native-migration/project` preserves a
coherent4551-file current source copy. Its initial compilation encountered a
duplicate local `forward` identifier in `original_data.cpp`; the current checkout
independently already contained the exact behavior-preserving variable rename.
Only that name fix was applied to the private baseline, with original bytes and
the amendment recorded. The full baseline native executable then built.

The D32 executable is separately preserved as `halo2_native-d32-baseline`.
The source renderer before migration is preserved independently; the changed
renderer was rebuilt and linked against the same coherent cache and other
native objects. That first resource-only stage did not change GLSL. A separate
build then connects the proved source encoder, with its pre-encoding shaders
and compiled modules preserved. The later LOAD-read dependency amendment also
preserves its preceding renderer and binaries. Private compilation and each
explicit source amendment are recorded beside the build.

Both binaries use the same retained Zanzibar camera/player inputs at native
tick6,640x480. Vulkan SDK1.4.363 synchronization validation reports no warnings
or errors in the successful captures. The actual GPU is NVIDIA GeForce RTX5080.
The first validation setup emitted a deprecated-layer-settings warning; its log
is preserved separately. The successful runs use `VK_LAYER_VALIDATE_SYNC=1`.

| Diagnostic retained region | D32 baseline | D24 resource only | D24 source encoding and LOAD dependency |
| --- | ---: | ---: | ---: |
| World mean absolute RGB byte difference | .7226302083 | .7197239583 | .7217708333 |
| World channels differing by more than8 | 789 | 769 | 785 |
| First-person mean absolute RGB byte difference | .6196779388 | .6196779388 | .6196779388 |
| First-person channels differing by more than8 | 32 | 32 | 32 |

D32 versus D24 differs at60 RGB pixels, maximum40 and total absolute653 RGB
bytes. These are actual measured images, not the CPU ideal-conversion diagnostic
from the earlier audit. The D24 readback has307,200 actual integers in120..FFFFFF
and307,200 zero stencil bytes. That zero stencil is native initialized state,
not the original source's462-FF-pixel sky result.

These are sampled image diagnostics, not isolated measurements of a depth
change's effect. Native integer tick6 does not fix fractional interpolation:
`multiplayer_runtime.cpp` derives `blend` from the actual host accumulator and
uses it in `shader_time`. That fractional shader clock is not recorded/restored
by this camera/player fixture. The before/after LOAD-dependency captures differ
at136 RGB pixels, maximum12 and total absolute324, while all307,200 D24 and S8
values agree. Small sky RGB differences therefore cannot be attributed solely
to a resource, encoder or dependency change.

The comparison also does not restore complete original simulation, visibility,
HUD or first-person history. It is not full framebuffer equality, physical
Xbox precision, or an all-map matched-original comparison.
The original/native phone image and exact metrics are beside the build in
`phone-original-native-d24.png` and `retained-d24-comparison.json`.

## Source integer encoding correction

An isolated actual-GPU diagnostic confirms that the former fragment expression
`floor(W)/16777215.0` stores N+1 for6,291,455 of all16,777,216 integer inputs on
the RTX5080. The original captured fragment and pinned xemu source instead use
`uintBitsToFloat(floatBitsToUint(floor(W)/16777216.0)+1u)`; that expression stores
every tested integer exactly. Its separate float-word output also agrees with
the strict CPU expression at every input. An EQUAL-against-max-depth control
demonstrates an actual comparator difference at input FFFFFE.

`world.frag` and `light_volume.frag` now call the existing `worldD24Depth`
helper. The world shader keeps its previous clamp, integer-W selector and
`gl_FragCoord.z` alternative. No source reconstruction function, clip gate,
geometry stage or stencil writer is activated by this encoder correction.
The shader dependency list includes the helper so edits rebuild both users.
Independent review found no blocking discrepancy in that connection.

Normal and address/UB runs of the diagnostic produce identical measurements
and binary outputs with zero SDK synchronization errors. This is exhaustive
post-admission integer encoding evidence, not full production interpolation or
another driver's lowering guarantee. Details, finite fractional controls,
historical shader hashes and the separate graphics leak boundary are in
[the encoding proof](SOURCE_DEPTH_ENCODING_GPU_DIAGNOSTIC.md).

## Map and resize validation

The resource-only all21-map1080p capture pass succeeds at native tick6 with actual
depth/stencil readbacks and no Vulkan warning/error or source admission fallback.
Its43,545,600 readback texels have matching depth/stencil extents; all stencil
values remain zero in this native resource-only stage. Input binary, shader and
map hashes remain unchanged during the pass. These are native execution and
resource checks; individual original frame comparisons were not performed.

The resource-only all21-map three-player1281x721 pass also succeeds at native tick6,
with no SDK warnings/errors and19,395,621 actual depth/stencil texels checked.
The intentionally odd size exercises partial split-view and tightly packed
aspect-copy rows. Both capture runs preserve their recorded binary/SPV/map
hashes. Its two focused renderer checks also pass:36 persistent resize cases
across Backwash/Beaver Creek/Gemini,1/2/4 players and480p/720p/1080p/4K, plus
fresh/reused/resized scenery,12 lighting perturbations, LOD and other-view
isolation. These use explicit motion inputs instead of the retained capture's
unfixed fractional host clock.

The source-encoding stage separately completes21/21 single-player1080p captures
before the LOAD dependency amendment. The final coherent build after that
amendment passes42/42 native captures: all21 maps at1920x1080 with one player,
and all21 at1281x721 with three players, at actual native tick6. Both retain
binary/SPV/map identity and report no SDK diagnostics. Actual depth/stencil
readbacks cover43,545,600 and19,395,621 texels respectively; all S8 bytes are
still native initialized zero. Neither run compares matched original frames.

The same final build passes both focused checks in81.37 seconds:36 persistent
map/player/resolution cases with exact CPU motion/RNG and synchronized
geometry/fog/water/postprocess, then fresh/reused/resized Headlong scenery,
12 LOD/baked-light perturbations and exact other-view isolation. Their actual
color and D24/S8 captures are retained separately from the resource-only tests.
The source, binaries, shaders, logs and image/readback hashes are frozen by
`tools/freeze_native_depth_migration.py` in
`native-depth-migration-report.json` and its artifact manifest. Historical
measurements and input manifests are never reassigned to the newer binary.

## Next source boundaries

The retained original sky A/B/moon all use stencil ALWAYS/REPLACE FF. Exact
admission needs the dedicated4C690 owner phase, raw parttype4 and admitted
layer15/pass/implementation, plus actual source caller/selection gates. The
original frame's first sky draw writes462 pixels, while the next two preserve
those bytes. Current background spans do not carry all that ownership data.

The original geometry stage supplies flat per-triangle W reconstruction inputs;
retained vertex output alone must not be treated as those inputs. The actual
GS, remapped strip indices and both VSOut/GSOut are now independently exported.
A bounded stencil-only producer matches every307,200 original D24/S8 value
for each A/B/moon draw and their single-fence chain on both geometry paths,
in normal and address/UB runs with zero SDK synchronization errors. Its17
comparisons/controls and58 input/ownership rejections pass. A changes exactly
462 stencil bytes00→FF; B/moon retain them, with no depth writes.

Those462 samples are exactly the retained attachment's max-depth pixels. This
endpoint match does not establish every possible sky edge under another depth
field, authored native vertex production or full sky appearance. Source
scheduling, owner/raw-part admission and authored apply/cleanup states must
still control the live writer and clear/load order. See
[the retained-geometry proof](SOURCE_SKY_STENCIL_PROTOTYPE.md).

Weather producer activation still requires actual current simulation, visibility,
clock/RNG and resident resource ownership. The isolated connected cloud/snow
proof supplies no substitute for those banks. The independently verified
current-D24 alias can consume this integer resource once those ownership and
submission boundaries are integrated.
