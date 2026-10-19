# MCC-only native gameplay goal checklist

Updated: 2026-10-18. This checklist owns the remaining scope; detailed evidence
stays in [MCC_GAMEPLAY_CONVERSION.md](MCC_GAMEPLAY_CONVERSION.md).
Check a box only after its stated acceptance test passes. Component proofs do
not complete an integration, gameplay, visual or audio acceptance gate.

Goal status: **active**. Keep this checklist current at each verified milestone
and before a handoff. Preserve unchecked work across sessions; the goal is
complete only when all required acceptance and delivery gates below pass.

**Required goal rule, part of the active objective:** Whenever a task or
acceptance gate is completed and verified, immediately update its checkbox and
supporting evidence in this checklist before starting the next task or handing
off. Record changed blockers and next steps at the same time. Leave unverified
items unchecked.
This requirement applies to work completed by the primary agent and subagents.
Saving that checklist update is a required part of completing each task; do
not defer it until the end of a session. For subagent work, the primary agent
records the verified result here before starting dependent work.

## Working rules

- Reuse the working development systems and ABI-compatible compiled objects.
  Compile actual compatibility changes and their affected consumers only; avoid
  broad engine/generated-module rebuilds. The selected old Main allocates a
  17,032-byte Simulation. The refreshed Main and current consumers agree on
  17,072 bytes, including current scene and presentation fields. Verify the
  actual allocator and complete support-object layouts before linking.
- Preserve Xbox TU1.5 gameplay, visuals, timing, precision and existing shaders.
- Keep serialized handles/tokens at their required widths; use real 64-bit
  host pointers and ownership for native resources.
- Treat converted gameplay definitions as normal vanilla internal data.
- MCC is the only required asset source. Xbox ISO/XBE/TU1.5, Vista banks and
  MCPX material are offline references, never onboarding/runtime dependencies.
- Reconstruct Xbox hardware or startup initializers only for demonstrated
  dependencies of the reused gameplay path. Preserve unrelated dirty work.

## Native gameplay frame compatibility

Compatibility checks below distinguish the private native frame cohort from
promoted product modules. Launcher delivery and full gameplay acceptance have
their own gates; passing a module test does not complete those gates.

- [x] Reproduce the MCC/Xbox bitmap layout failure in the actual native model path.
- [x] Supply owned CPU texture rows without rewriting canonical MCC source rows.
- [x] Handle both the ordinary bitmap accessor and the inline D5610 reader.
- [x] Compare the retained native DXT5 constructor/sampler against original
  execution: 40 cases, plus negative controls and sanitizer checks.
- [x] Complete the public native player/model test: 484-byte collector,
  33 rendered nodes, no reference-file opens, unchanged MCC map.
- [x] Supply the verified lighting constants, draw banks, collision lookup
  tables, cold query count, shadow history and fixed shadow bases reached so far.
- [x] Supply the complete authored shadow light/projection template and its
  proven publication storage: exact authored bytes, original execution controls
  and preservation checks pass.
- [x] Supply the one-byte scoped visibility-query flag required by `134C00`;
  original write/clear ordering and complete immediate static-owner coverage verified.
- [x] Promote the reviewed startup-data compatibility through V24 into product
  source; compile with current headers and preserve all prior factory state.
  The V24 promotion preserves the full 2,420-region/478,041-byte factory;
  current-header compilation, fog policy, overlap and warm-state controls pass.
  [Startup product evidence](../.local/mcc-angle-crt-20261018/product-promotion-v1/validation.json).
- [x] Verify and supply all six missing numeric constants in the retained
  `2CBA0` static draw closure (39 source bodies; 24 bytes, no mutable-state substitutions).
- [x] Supply the complete bounded `2E6ED0` selector and target tables:
  256 original dispatch controls pass; no fallback state or code backing supplied.
- [x] Verify and supply the full nested `4CBE0` sort tables and four repeat-model
  constants in V18 (109 bytes). The conditional child table supports only its
  four authenticated entries; broader shader-property admission remains unproved.
- [x] Supply the authenticated `object_light_interpolate` script boolean with
  original true default and warm script-write preservation (V19).
- [x] Validate the private sparse lens CPU-cache candidate with 54 setup and
  45 callback execution fixtures plus rejection of an unsupported coordinate
  lookup; retain the real renderer lease, images and original producer kernels.
  [Lens candidate evidence](../.local/mcc-lens-sdk-ownership-20261018/v2/validation.json).
- [x] Supply and validate the three authored texture-header lookup tables in
  the private lens adapter: prior sparse state is identical, 99 execution
  fixtures pass, including 21 format10 headers; supported formats are unchanged.
  [Table adapter evidence](../.local/mcc-lens-sdk-ownership-20261018/v3/validation.json).
- [x] Promote the validated sparse lens/table adapter into ordinary product
  sources while preserving current camera/clip-plane support. All 99 fixtures,
  shape and rejection checks pass; CMake source-specific floating-point flags
  match the accepted compile, with no original-runtime file opens.
  [Lens product evidence](../.local/mcc-lens-sdk-ownership-20261018/product-promotion-v1/work-status.json).
- [x] Supply the proved 20-byte shared renderer-mask cold field in private
  startup helper V20; original/native five-OR stores, observers, overlap
  rejection and warm-state preservation pass. Constructor completion and the
  full renderer-state lifecycle are separate from this cold backing proof.
  [Mask field evidence](../.local/mcc-model-bitmap-selector-20261018/v20/validation.json).
- [x] Supply the single missing authored `135970` view scalar in private V21:
  the original immediate closure is verified, prior startup owners are exact,
  and read-only/overlap/warm-mask controls pass.
  [View scalar evidence](../.local/mcc-model-bitmap-selector-20261018/v21/validation.json).
- [x] Supply and validate the missing fog suppression byte in private V22:
  81 original CPU gate cases and 81 actual C++ policy cases pass, with overlap
  and warm-state controls. The existing atmospheric-fog field and its whole
  frame-record owner remain unchanged. This does not prove a complete render pass.
  [Fog byte evidence](../.local/mcc-model-bitmap-selector-20261018/v22/validation-provenance-v2.json).
  The initial capture generator and its later complete-owner qualification are
  now explicitly separated and reproducible; the helper and raw behavior
  evidence are unchanged.
  [Provenance qualification](../.local/mcc-fog-control-storage-20261018/provenance-v2/validation.json).
- [x] Authenticate the next first-person finish dependency: original `16A2B0`
  reads exactly four bytes at `44A498`, authored FLOAT9 (`00001041`). It matches
  an existing development data definition. All 19 immediate owned bodies and
  their 82 fixed reads are reviewed; external, indexed and pointer-derived
  accesses remain separate.
  [Independent constant review](../.local/mcc-model-cpu-bitmap-20261018/first-person-finish-review-v1/root-primary-review.json).
- [x] Supply the exact read-only four-byte constant in private V23, preserving
  all 2,418 prior owner tuples. Factory, late-conflict, read-only, native-pointer
  and warm-state controls pass; Frame42 passes this scalar and reaches the
  separately recorded angle-wrap dependency.
  [V23 evidence](../.local/mcc-model-bitmap-selector-20261018/v23/validation.json).
- [x] Authenticate the next angle-wrap dependency: both complete startup
  producers, the 88-byte original dispatcher and its 418-row forward table
  prove callbacks `380D50` then `380D30`. Their two inputs already have exact
  owners; the two writable destinations need genuine initialization.
  [CRT admission review](../.local/mcc-model-cpu-bitmap-20261018/angle-crt-review-v1/qualified-review.json).
- [x] Validate both real callbacks in the private current-header World:
  24 original/native ABI, FP, memory and ordered-write cases pass; six actual
  World groups cover both constructor writes, eight angle queries, warm
  cancellation, same/fresh restore and store-denial fault recovery. The cold
  factory adds only eight bytes and preserves all prior 2,419 owners; no
  reference files are opened during native execution.
  [Private callback/lifecycle evidence](../.local/mcc-angle-crt-20261018/current-v1/validation.json).
- [x] Promote the reviewed angle callback integration into three ordinary
  product modules. All allocated sections, symbolic relocations, strong exports
  and actual dependencies match the accepted private candidate, so its complete
  CPU/World evidence applies. No Main, shaders or generated engine rebuild.
  [Product callback evidence](../.local/mcc-angle-crt-20261018/product-promotion-v1/validation.json).
- [x] Validate the promoted callbacks through ordinary native Main's tick3
  route: V7 submits two frames and exits successfully. This short route clears
  the demonstrated callback dependency; broader gameplay acceptance stays open.
  [Ordinary callback runtime](../.local/mcc-native-client-20261018/client-current-v7/run-v1-real-mcc/validation.json).
- [x] Compile and link the refreshed ordinary Main client with its required
  current-layout providers: 65 handwritten files plus the schema3 codec;
  successful private link and all 308 inputs verified.
  [Client link evidence](../.local/mcc-native-client-20261018/client-current-v4/cohort.json).
- [x] Verify the actual ordinary-client provider and COMDAT selection: all 65
  refreshed owners are loaded, no stale same-source by-value owner is selected,
  and 262 critical text sections in the audited type set have compatible owners.
  This bounded audit does not cover the newly identified nested clock-clip
  types below. Both linked Main
  allocation sites use 17,072 bytes. Actual runtime validation remains open.
  [Client selection evidence](../.local/mcc-native-client-20261018/client-current-v4/validation.json).
  The subsequent V22 helper-only client link preserves all 307 other inputs,
  all selected archive members and their load order.
  [Helper-only client evidence](../.local/mcc-native-client-20261018/client-current-v5/provider-preservation.json).
  Independent review rehashes all inputs and confirms that the sensitive
  by-value COMDATs select current owners; the stale model-pose return owner
  is absent.
  [Independent selection review](../.local/mcc-native-client-20261018/prefix-reader-review-v1/client-v4-selected-provider-review.json).
- [x] Refresh the demonstrated nested first-person clock-clip consumers.
  The retained content loader expects 40-byte clips and 752-byte bindings;
  current producers return 48-byte clips and 776-byte bindings. The actual
  crash was a 40-byte traversal of the current 48-byte vector. Expanded selected
  consumer checks and the actual V7 tick3 runtime now pass. This does not
  certify first-person geometry or every weapon's behavior.
  [Crash layout evidence](../.local/mcc-native-client-20261018/client-current-v5/diagnosis-v1/clock-clip-layouts.json).
- [x] Compile the four demonstrated stale clock-clip consumers with current
  headers and qualify all nine direct source users at 48-byte clips/776-byte
  bindings. Containing-type selection and the later V7 runtime pass.
  [Consumer compile evidence](../.local/mcc-native-client-20261018/client-current-v5/diagnosis-v1/clock-consumer-qualification.json).
- [x] Verify the exact cache-loader crash changes from red to green through
  ordinary Main: V5 reproduces SIGSEGV in 0.884 seconds; V6 returns from the
  same content-loading boundary in 1.595 seconds. Full native-world/render
  runtime remains open.
  [Loader regression evidence](../.local/mcc-native-client-20261018/client-current-v5/diagnosis-v1/minimized-green-v6.json).
- [x] Link the 69-source coherent V6 client and verify its expanded selected
  owners: the clock-clip traversal now steps 48 bytes; all 17 affected FP and
  19 containing-content COMDATs select fresh compatible owners. The narrower
  earlier audit is superseded for these nested types. Full runtime remains open.
  [Expanded client evidence](../.local/mcc-native-client-20261018/client-current-v6/validation.json).
- [x] Compile the two demonstrated nested ScriptVm consumers with current
  headers and pass the small VM fixture at 1/5/8/16/31 thread rows, including
  authority capture/restore and ticking. The retained-owner negative fixture
  aborts with SIGABRT; the full client's original failure is SIGSEGV in tick,
  so exact full-runtime validation remains open.
  [VM fixture evidence](../.local/mcc-native-client-20261018/client-current-v6/diagnosis-v2/vm-fixture-green.json).
- [x] Link and independently review the coherent 71-source V7 client with
  the promoted V24 World/CRT/data bundle: all 312 inputs and 307 unchanged V6
  identities are pinned. All 39 VM-family sections, 19 thread/container helpers
  and five SavedState helpers select current compatible owners. The exact
  full-client tick regression is closed by the successful run below.
  [V7 client evidence](../.local/mcc-native-client-20261018/client-current-v7/validation.json).
- [x] Close both exact ordinary-Main crash regressions: V7 returns from content
  loading, ticks the repaired VM and captures at simulation tick3; exit0 in
  94.999 seconds, two submitted frames, one local player. All 312 linked inputs,
  97 shaders and MCC maps remain unchanged; zero original-reference opens.
  Scratch is removed. This muted run does not prove audible output or parity.
  [Actual crash closure](../.local/mcc-native-client-20261018/client-current-v7/runtime-crash-regression-closure-v1.json).
- [x] Capture and inspect ordinary Main after native simulation updates.
  The actual tick3 image shows textured Lockout, HUD and the SMG. It also shows
  severe stretched first-person arm geometry and oversized HUD elements, which
  require source-led diagnosis before visual acceptance.
  [Native tick3 image](../.local/mcc-native-client-20261018/client-current-v7/run-v1-real-mcc/lockout-view.png).
- [x] Capture and inspect one actual native initial frame: Frame44 submits one
  view/frame at simulation tick0 and exits successfully. Textured Lockout
  geometry and HUD are visible. All 97 shaders/206 inputs/maps are unchanged;
  no original-reference opens. The original PPM capture is preserved and its
  PNG view has identical decoded pixels. No weapon is visible at tick0 and
  there are zero audio packets; this does not complete the gates below.
  [Initial frame](../.local/mcc-lens-bridge-20261017/frame-v44/run-v1-real-mcc/lockout-view.png),
  [Runtime evidence](../.local/mcc-lens-bridge-20261017/frame-v44/run-v1-real-mcc/validation.json).
- [x] Correct the demonstrated MCC first-person CPU vertex compression and
  local-to-global skin-map decoding. The actual 4,980-vertex Xbox/MCC arm pair
  now has zero wrong active indices (previously all 6,648 were wrong), with
  position and UV errors inside the original quantization bounds. This proves
  source geometry only; the ordinary Main capture and animation-pose parity
  remain separate gates.
  [CPU geometry regression](../.local/mcc-native-fp-geometry-20261018/product-v1/regression.json).
- [x] Verify the arm-geometry correction in ordinary Main at native tick3.
  V8-FP replaces only the proved model object and captures successfully after
  two frames/three ticks. Inspection confirms the giant stretched arm triangles
  are gone. All 312 input providers, 97 shader files and maps remain unchanged;
  there are no original-reference opens. HUD scale/placement and full Xbox
  animation/pixel parity remain open.
  [V8-FP image](../.local/mcc-native-client-20261018/client-current-v8-fp/run-v1-real-mcc/lockout-view.png),
  [Runtime validation](../.local/mcc-native-client-20261018/client-current-v8-fp/run-v1-real-mcc/validation.json).
- [x] Restore original logical bitmap sizes for the canonical MCC HUD banks
  without changing physical texture dimensions, pixels or the geometry kernel.
  The actual shared and Lockout metadata each pass all 70 banks/328 original
  frames and 708 altered-tuple controls. The extra MCC backpack page has no
  invented original owner; unknown/edited metadata is left unchanged. This is
  logical-size evidence; widget placement and the combined GPU capture remain open.
  [Logical-extent validation](../.local/mcc-hud-bitmap-geometry-20261018/v1/validation.json).
  The independent source/data review and actual CTest generator check also pass.
  [Independent review](../.local/mcc-hud-bitmap-geometry-20261018/independent-review-v1/review.json),
  [CTest manifest check](../.local/mcc-hud-bitmap-geometry-20261018/v1/ctest/report.json).
- [x] Convert the demonstrated canonical HUD placement differences through
  normal owned Cache metadata: 17 owners/259 stored-field patches, preserving
  the existing scoreboard path. Complete bitmap-geometry guards reject edited
  layouts while permitting texture-only replacements. Portable, actual
  shared/Lockout/Cairo, sanitizer and generator checks pass; original fields,
  screen references and shared curves are preserved. Whole-frame acceptance
  remains separate.
  [HUD metadata publication](../.local/mcc-hud-logical-extent-audit-20261018/v2/validation.json).
- [ ] Complete a visually valid native gameplay frame after source updates,
  including first-person geometry and HUD presentation. The bounded tick30
  capture below now passes after the class1 correction; remaining complete
  material/scene validation stays open. Coherent V13 adoption is verified below.
- [x] Resolve the combined V9 namespace admission failure through the normal
  Cache publication order. Stage the new HUD owners until after the existing
  shader/globals bootstrap, preserving the authenticated sound-class namespace.
  The exact V9 failure remains recorded; minimized regression, portable and
  actual-cache checks, and sanitizer checks pass. Ordinary Main V10 now exits
  successfully after two frames/three ticks instead of failing before rendering.
  [Namespace regression validation](../.local/mcc-hud-namespace-integration-20261018/v3/validation.json),
  [Combined V10 run](../.local/mcc-native-client-20261018/client-current-v10-hud-mcc/run-v1-real-mcc/validation.json).
- [x] Capture and inspect the combined arm/HUD corrections through ordinary
  Main. V10 shows normal arm geometry and the restored logical sizes and
  placements of the reticle, radar, ammunition and grenade widgets. All 314
  providers, 97 shaders and maps are unchanged during execution. Its PNG view
  preserves every decoded source pixel. This bounded tick3 image does not
  establish complete materials, animation, gameplay or TU1.5 framebuffer parity.
  [Combined native image](../.local/mcc-native-client-20261018/client-current-v10-hud-mcc/run-v1-real-mcc/lockout-view.png),
  [Lossless image conversion](../.local/mcc-native-client-20261018/client-current-v10-hud-mcc/run-v1-real-mcc/image-conversion.json).
- [x] Independently audit the complete V10 file trace against known Xbox,
  Vista, MCPX and original-reference roots, beyond the initial filename filter.
  All 107,928 open attempts are parsed; the only four map opens are validated
  MCC maps. No reference-bank path is accessed. Two Vista-named compiled shader
  files belong to the preserved shader bank and are explicitly distinguished
  from asset banks. This covers observed pathname operations, not unidentified
  aliases or inherited descriptors.
  [Independent file audit](../.local/mcc-native-client-20261018/client-current-v10-hud-mcc/run-v1-real-mcc/file-audit.json).
- [ ] Inspect the captured image and resolve material rendering defects.
- [x] Repeat the bounded capture through ordinary compiled Main/client, reusing
  ABI-compatible objects; V7 reaches tick3 and returns successfully.
  Checkpoint schema2→3 grows from 824 to 872 bytes; match every by-value consumer
  before combining cohorts. See the [ABI audit](../.local/mcc-native-client-20261018/authority-abi-audit-v1/validation.json).

Latest GPU capture: coherent V13 completes ordinary unmuted tick30 with the
class1 correction and reviewed counter/response data. Root inspected its image
and the version8 packaged image: the oversized left edge is gone, with arms,
SMG and HUD retained. All 97 shaders remain unchanged. V13 is now selected by
both installed launchers and is shipped in the validated version8 ZIP; the
actual installed eight-frame route also passes. Further firing-table work,
complete materials, audio timing and full TU1.5 acceptance remain open.
[Current native frame](../.local/mcc-native-client-20261018/client-current-v13-class1-fire-mcc/run-v1-real-mcc/lockout-view.png),
[Actual combined run](../.local/mcc-native-client-20261018/client-current-v13-class1-fire-mcc/run-v1-real-mcc/validation.json).

Frame44 records the bounded initial frame above.
Its one-frame harness uses the renderer's existing default shader directory;
all 97 files match the pinned copies exactly and product capture guards are
unchanged. It advances zero simulation ticks and is separate from ordinary Main.

Latest multi-tick attempt: Frame42 passes the sparse lens, texture-header
tables and shared mask dependencies. The actual lens query route completes
with zero issued queries/flares and stamp=1; this does not test visible flares.
It passes the `135970` view scalar, first-person particle setup and the V22 fog
control read and the exact `44A498` finish constant. The next actual root
`168CA1` stops at `C1F96` while accessing `5564C8`; its constructor/lifetime
must be verified before supplying backing. Table presence does not
admit unsupported format/flag combinations.
The selected-object shadow route still completes with queue=1, roots=1 and
history=1. No image is produced; all 97 shaders and 206 linked inputs are
unchanged, with no original-reference opens. Evidence:
[Frame42 result](../.local/mcc-lens-bridge-20261017/frame-v42/run-v2-real-mcc/validation.json).
The diagnostic requests simulation tick3. A later update failure before that
capture does not prove that earlier draws were never submitted; Frame44 records
the separate initial-frame boundary directly.
The current-header Main client links privately and passes the bounded final
provider/COMDAT audit and independent helper-only transfer review. Its first
actual V5 launch exits with SIGSEGV after 0.665 seconds, before producing a
frame or stdout. GDB reproduces the content-loader crash and exact selected
object layouts establish the nested clock-clip/binding mismatch described above.
All linked inputs, shaders and MCC maps remain unchanged;
no original-reference files are opened. V6 fixes this specific loader crash;
the later native tick failure remains a separate acceptance gate.
[Ordinary-client run](../.local/mcc-native-client-20261018/client-current-v5/run-v1-real-mcc/validation.json).
After the proved loader fix, V6 enters native-world/effect setup, then exits
with a later SIGSEGV after 71.664 seconds, before its requested capture.
The reproduced backtrace is `ScriptVm::tick`: retained nested thread records
are 64 bytes versus current 88 bytes, despite the same 408-byte outer VM.
V7 passes the recursive nested-owner and continuation/deque selection review
and the exact full-runtime rerun. All 311 V6 inputs, 97 shaders and
MCC maps remain unchanged, with no original-reference opens.
[V6 native run](../.local/mcc-native-client-20261018/client-current-v6/run-v1-real-mcc/validation.json).

Component evidence:
[V13 shadow template](../.local/mcc-model-bitmap-selector-20261018/v13/validation.json),
[V24 product startup-data promotion](../.local/mcc-angle-crt-20261018/product-promotion-v1/validation.json).

## MCC-only onboarding and asset ownership

- [x] Inventory the existing launcher/cache/asset handoffs and their concrete
  remaining hooks. This read-only source/manifest audit does not verify a fresh
  install, decoded UI delivery or cache-only gameplay. The next work is honest
  native-test routing and MCC-owned loading/menu inputs.
  [Onboarding inventory](../.local/mcc-onboarding-delivery-audit-20261018/read-only-v1/report.json).
- [x] Detect/import MCC content into the port-owned cache in the existing test setup.
- [x] Assemble the copied MCC menu image/font resources transactionally without
  changing their source cache. The actual-cache scratch check covered 648 image
  owners, 561 unique DDS pages and 40 font files; all scratch output was removed.
  This does not admit menu widgets, localization or the complete loading presentation.
  [Actual asset assembly](../.local/mcc-loading-production-20261018/v1/actual-assets.json).
- [x] Replace the production Main loading hooks with the owned five-input MCC
  factory and remove automatic Vista loading/settings discovery for MCC.
  Two current-header syntax checks, 22 preparation controls and the actual MCC
  factory check pass. The modified Main/frontend are now linked in V10; its
  multiplayer capture does not execute the menu/loading presentation. The
  missing mask, ambience and menu metadata remain open.
  [Production source handoff](../.local/mcc-loading-production-20261018/v1/validation.json).
- [x] Compile the three necessary MCC loading consumers against the coherent
  client headers: Main, frontend and the newly referenced owned loading factory.
  Their source/header identities and retained object sizes are recorded; all
  compiler/test scratch is removed. Actual menu/loading rendering remains open.
  [Loading consumer build](../.local/mcc-native-client-20261018/mcc-loading-consumers-v1/build.json).
- [x] Reproduce the remaining loading-presentation failure with the actual
  five owned MCC inputs and unchanged presentation constructor. Input preparation
  succeeds; construction rejects the unresolved required slot11 mask. All 314
  cohort inputs/dependencies and source maps remain unchanged, with no reference
  opens or retained test binaries. This CPU-only expected failure does not
  complete a loading frame or establish an ambience producer.
  [Actual missing-role reproduction](../.local/mcc-loading-production-20261018/investigation-v2/repro/actual-v1/validation.json).
- [x] Trace the narrow MCC loading-asset candidates and menu-music ownership.
  All 1,557 mainmenu owners, 16 candidate bitmap frames and 577 installed
  prebuild/preload filenames are accounted for. The verified `wigl`/`lsnd`
  references identify menu music; they do not establish loading ambience.
  None of the tested images authenticates slot11. All three scratch trees are
  removed. The actual loading producer/role mapping remains the next dependency.
  [Asset-role investigation](../.local/mcc-loading-production-20261018/investigation-v2/assets/status.json).
- [x] Reauthenticate the installed MCC DLL's complete legacy loading-bank
  consumer: all four chained-unwind fragments cover 1,045 bytes and 224
  instructions, with a real six-offset image loop for `loading_dx11.bin`.
  Its loading-audio call targets a literal `RET 0` leaf. The named bank is
  absent from the bounded 577-file inventory. These static facts do not prove
  the active outer frontend path, image roles, or a replacement audio policy;
  those bindings remain open. No product files or shaders change, and scratch
  is removed.
  [Current MCC loader authority](../.local/mcc-loading-production-20261018/investigation-v2/source/mcc-legacy-loader-primary.json).
- [x] Inspect the actual outer loading GFX and installed pack indexes without
  importing replacement content. The 6,352-byte GFX has 302 framed nested
  records and two external image names; all 17 pack indexes identify the
  small loading UIF/WLIB/SSL entries. These metadata references guide the
  next producer investigation; they do not yet supply the mask or ambience.
  [Outer loading metadata](../.local/mcc-loading-production-20261018/investigation-v2/source/adjacent-metadata.json).
- [x] Identify the sixth legacy MCC image's actual masking role. The complete
  loading thread/renderer and four child owners bind image5 to the destination
  color/zero blend draw. Its genuine source is the sixth DDS extent in
  `loading_dx11.bin`; no equivalent tag or available payload is established.
  This identifies the required role without admitting a substitute. The mask,
  independent ambience and production draw binding remain open.
  [Producer and role seal](../.local/mcc-loading-production-20261018/investigation-v2/source/producer-role-seal.json).
- [x] Qualify the installed MCC PCK block reader from its actual DLL consumers.
  Its header, 32 KiB logical blocks, physical offsets and mode0 raw-DEFLATE
  framing decode all 929 `cache.pck` blocks with matching checksums. The
  30,439,696-byte logical stream contains the exact loading UIF/WLIB/SSL
  resource names; no decoded stream is retained. Named resource extents and
  roles still need their source-derived lookup schema. A real
  `loading_screen_music` request is identified separately from the retired
  legacy audio hook; its sound/event binding is not yet admitted.
  [PCK reader authority](../.local/mcc-loading-production-20261018/pck-lookup-v1/first-block.json),
  [Actual logical resource metadata](../.local/mcc-loading-production-20261018/pck-lookup-v1/cache-logical-metadata.json).
- [x] Qualify the outer PCK resource header and its two actual resource extents
  from the installed reader and registered classes. The header ends at logical
  byte217; its cache block and SSL block extents are exact. Later loading-name
  metadata lies outside those rows, so its neighboring integers are not
  admitted as payload offsets. The inner-cache lookup remains the next step.
  [Source-derived PCK header lookup](../.local/mcc-loading-production-20261018/pck-lookup-v1/pak-header-lookup.json).
- [x] Extract the three named loading resources from the authenticated nested
  cache lookup. All 13,661 entries resolve; the exact CSV lengths and hashes
  identify 163 bytes across UIF/WLIB/SSL, using 27 physical blocks. Their
  contents bind `hcex_loading_screen` and native `UiLoadingScreen`; they do
  not contain a mask or music payload. The native implementation is the next
  owner to trace. Scratch is removed and no decoded pack stream is retained.
  [Actual named loading resources](../.local/mcc-loading-production-20261018/pck-lookup-v1/named-loading-resources.json).
- [x] Qualify the actual shared-pack codec and metadata lookup. The installed
  DLL selects a complete copy decoder for codec4; the source-derived lookup
  resolves all 7,247 shared-pack rows without a loading-name or nested cache
  match. Only metadata is read; no loading image or audio role is inferred.
  [Codec4 authority](../.local/mcc-loading-production-20261018/pck-lookup-v1/codec4-qualified.json),
  [Actual shared-pack lookup](../.local/mcc-loading-production-20261018/pck-lookup-v1/shared-loading-lookup-v2.json).
- [x] Inventory the four specifically named outer UI texture packs by their
  actual chunk metadata. The loading pack has 268 definitions; the other
  packs have 306, 758 and 997. Neither external GFX image name is found.
  Map pictures and named UI textures remain candidates until their real
  rendering consumers establish a role; no pixel payload is copied.
  [Outer UI texture metadata](../.local/mcc-loading-production-20261018/pck-lookup-v1/loading-texture-metadata.json).
- [x] Qualify the actual outer MCC image-name lookup kernel and its source
  registration. The complete conditional callback and name-hash consumer pass
  101 genuine x64 CPU controls against all 256 CRC words. The two exact
  external GFX name keys have no match in the four inventoried packs. The
  specific tag-to-descriptor ingress, active GFx binding, mask and independent
  ambience remain unproved; no image or audio substitute is admitted. Source,
  metadata and CPU receipts are sealed with scratch removed and no retained
  binaries, pixel banks or captures.
  [Loading lookup source and CPU handoff](../.local/mcc-loading-production-20261018/pck-lookup-v1/handoff.json).
- [ ] Verify fresh-machine detection and onboarding with only an installed MCC copy.
- [ ] Verify complete required assets: textures, bump maps, models, audio,
  gameplay definitions, UI and loading presentation.
- [ ] Verify MCC internal paths and mod-folder tags/data paths are preserved.
- [ ] Reconstruct the requested loading presentation from MCC and retained
  custom assets; remove dependence on the Vista loading bank.
- [ ] Verify cache-only startup after import without access to the MCC source files.
- [x] Establish a process-local read/execute confinement fixture for the cache
  test. Six actual MCC/Original/XBE files open before restriction and return
  `EACCES` after exec; an owned cache map and shader stay readable. Landlock
  ABI10, no-new-privileges and closed inherited descriptors are verified;
  allowlisted roots include only system paths and owned scratch. No global
  permissions or directories change. This is the confinement primitive only,
  not a game run or a filesystem-write/IPC security claim.
  [Confinement controls](../.local/mcc-cache-only-containment-20261018/v1/primitive-validation.json).
- [x] Run the actual portable version8 native Lockout test with original MCC
  and Xbox inputs unreadable. It exits0 at 17 frames/30 neutral ticks after
  133.73 seconds; the real frame is inspected. Six negative opens return
  `EACCES` after inherited exec. All 107,868 runtime open attempts are audited:
  no successful reads outside allowed roots and no original-source matches.
  All map/resource containers come from the owned cache. Package checksums,
  97 shaders, 25,004 cache source identities and original identities stay
  unchanged. All processes and three scratch roots are gone; temporary cache
  links, saves, binaries, trace and frame are removed. This proves only the
  neutral test route; the full cache-only/campaign/UI/onboarding gate stays open.
  [Contained actual gameplay](../.local/mcc-cache-only-containment-20261018/v1/validation.json),
  [Cleanup and terminal process audit](../.local/mcc-cache-only-containment-20261018/v1/cleanup.json).
- [ ] Verify the full launcher-to-gameplay path opens no Xbox, Vista or MCPX references.

## Converted vanilla gameplay and 64-bit integration

- [ ] Finish the gameplay-tag conversion audit; convert every actual TU1.5 difference.
- [ ] Verify converted definitions load as vanilla internal data, including
  default campaign and multiplayer paths and mod interoperability.
- [ ] Finish native 64-bit host-pointer/ownership integration across gameplay.
- [ ] Verify required serialized widths, arithmetic, precision and timing remain unchanged.
- [x] Refresh the demonstrated by-value consumers of current checkpoint and scene
  layouts, then validate the coherent private client. Preserve current public fields;
  do not mix old and current object layouts. Main, campaign runtime, Simulation
  and multiplayer agree on 17,072 bytes. The current campaign-view, 288-byte
  AnimationClip, optional/containing presentation, model-pose and particle
  consumers are refreshed; reviewed sky/model/audio support remains compatible.
  Missing API providers are resolved in the linked 69-source V6 client plus
  the schema3 codec. The expanded FP/Content/Weapon selection checks and the
  88-byte nested ScriptVm thread/deque review pass. V7 combines all 71 owners
  with the V24 World/CRT/data bundle and passes ordinary Main at tick3;
  V8-FP also passes with the corrected model owner. Detailed receipts are above.
- [ ] Check final delivery compatibility with subsequent additive receiver and
  sound-I/O API/body work. The pinned earlier receiver/World bodies establish
  the private validation cohort; other active work remains preserved.
- [x] Inventory current source/header drift beyond the narrow V13 boundaries.
  All 314 coherent inputs and 44 narrow header entries still match. Of the
  1,191 frozen include files, 13 differ, including one comment-only change;
  nine changed sources from the 71-source ledger still supply selected objects.
  Static review identifies voice-frame/packet, interpolation, sound-DMA and
  voice-device virtual-interface risks. This inventory does not certify the
  current bodies; paired layout checks and the minimal integration set remain open.
  [Current compatibility inventory](../.local/mcc-current-compatibility-audit-20261018/v1/inventory.json).
- [x] Measure the demonstrated current/frozen public layouts with paired tiny
  probes. `GameVoiceFrame` grows 48→56 bytes, `MatchVoicePacket` 56→64,
  `ModelNodeInterpolation` 176→184 and `SourceRuntimeSoundDmaResources` 240→264.
  Pimpl owner sizes stay unchanged; additive methods alone do not establish
  a layout break. These measurements identify actual mixing risks and do not
  invalidate the internally consistent frozen V13 baseline. Minimal affected
  consumer/provider integration remains open.
  [Paired layout measurements](../.local/mcc-current-compatibility-audit-20261018/v1/layouts.json).
- [x] Check actual selected providers against current-only APIs and layouts.
  The coherent ELF lacks all 17 inspected new APIs while older positive
  controls are present. The selected multiplayer object confirms the frozen
  48/56/176-byte voice/packet/interpolation layouts. This establishes a current
  feature/body delivery gap; compatible rebuild groups remain required before
  adopting the new providers. The frozen test cohort remains internally consistent.
  [Selected provider and layout evidence](../.local/mcc-current-compatibility-audit-20261018/v1/selected-providers.json).
- [x] Seal the minimal current/frozen compatibility matrix. Six owners must
  refresh together for the demonstrated public-layout changes: Main,
  multiplayer runtime, game voice, online voice, online session and render
  interpolation. A separate 16-file handwritten-body set includes current
  World, whose eleven required sound providers are absent from the selected
  ELF. Five already-current provider snapshots can be reused. All 314 frozen
  inputs and 44 narrow pins remain exact; later additive header changes are
  separately pinned. This identifies the integration work and does not close
  the current-product compatibility gate. Tiny probes and audit scratch are removed.
  [Compatibility matrix](../.local/mcc-current-compatibility-audit-20261018/v1/matrix.json),
  [Audit cleanup](../.local/mcc-current-compatibility-audit-20261018/v1/cleanup.json).
- [x] Validate schema3 authority encoding/decoding and rejection of unsupported
  schemas and malformed archives: 25 focused checks pass; archive framing is unchanged.
  [Codec evidence](../.local/mcc-authority-codec-schema3-20261018/v1/validation.json).
- [ ] Verify real attachment admission, restoration and continued gameplay in
  the coherent client; the codec fixture alone does not establish those behaviors.
- [x] Promote the proved texture compatibility modules into current product
  source and verify current-header compilation and CMake registration.
- [x] Integrate authenticated texture-cache export/restore lifecycle handling in
  current product World; the promoted code passes the same real lifecycle suite.
- [x] Verify current-World cold same/fresh restore, cancellation, rejection, denied
  and successful swaps, fault recovery and stale-owner teardown. The corrected
  suite passes 12 groups/15 rejection controls and nine public model calls with
  484-byte collectors/33 nodes. It proves actual resource republication after
  genuine cold restores and poisoned/partial/rollback-denied resource failures;
  no original-reference files are opened. Broader gameplay/parity gates remain open.
  [Product promotion evidence](../.local/mcc-model-cpu-bitmap-20261018/product-promotion-v1/native/validation.json).
- [ ] Verify live controls, player lifecycle, weapons, vehicles, AI, scripts,
  objectives and relevant map transitions against TU1.5.
- [ ] Complete representative campaign and multiplayer playthrough acceptance.

## Graphics and audio

- [x] Preserve the existing 97 shader files in the current native rendering tests.
- [ ] Finish graphical corrections using Reformation as a reference and Xbox
  TU1.5 as authority, including relevant bitmap/bump-map inconsistencies.
- [x] Compare five additional locally resident bitmap pairs through the
  coherent native decoder, compiling only the inspector entry point and
  reusing 313 providers. Metropolis's `matte_grey_edge_dif`, `metal_slats_bump`
  and `metal_slats_dif` have matching descriptors but different complete
  seven/nine/seven-mip bytes. Its `ec_ocean_alpha` is already byte-identical;
  no guessed pixel replacement is admitted. Sacred Icon's gradient has changed
  dimensions/mip counts and needs a separate descriptor proof. All 314 inputs,
  source maps, resource banks and shaders remain unchanged; temporary tool,
  link map and pixels are removed. This is original retail-map source evidence,
  not captured TU1.5 visual parity or a complete shared-resource inventory.
  [Native bitmap comparison](../.local/mcc-graphics-restoration-audit-20261018/v1/validation.json),
  [Bounded audit handoff and cleanup](../.local/mcc-graphics-restoration-audit-20261018/v1/handoff.json).
- [ ] Validate complete rendered scenes against authenticated TU1.5 captures.
- [x] Verify the reused MCC sound decoder and native SDL mixer produce genuine
  PCM on the pinned client providers. The actual SMG sound yields one 46,080-frame
  packet, nonzero output, completion and silence before/after; no original-reference
  opens and no provider changes. This manual-delivery fixture does not prove the
  live gameplay sound scheduler, physical audibility or video synchronization.
  [MCC PCM validation](../.local/mcc-native-audio-capture-audit-20261018/manual-v1/validation.json).
- [x] Verify genuine SDL device-callback capture through the existing disk
  backend. The real SMG output matches the manual mixer sample-for-sample
  after the production float32 WAV quantization; raw callback scratch was removed.
  [Device capture validation](../.local/mcc-native-audio-capture-audit-20261018/device-v1/validation.json).
- [x] Capture genuine nonzero PCM during ordinary unmuted MCC gameplay,
  without manually submitting a sound. V10 produces 74,752 stereo callback
  frames (1.557 seconds), peak 0.02663 and RMS 0.002109. The corrected looping
  loader no longer reports its previous allocation error. The requested WAV
  is preserved; source identification, timing, a successful longer run and
  synchronized video remain separate gates. The run fails later in rendering.
  [Live neutral audio result](../.local/mcc-native-client-20261018/client-current-v10-hud-mcc/audio-neutral-v1-real-mcc/validation.json),
  [Actual gameplay audio](../.local/mcc-native-client-20261018/client-current-v10-hud-mcc/audio-neutral-v1-real-mcc/native-gameplay-audio.wav).
- [x] Authenticate the later neutral-render failure's complete `2C390`
  constructor: 214 original bytes/56 instructions, no calls or branches, all
  generated instruction identities exact. All five immutable input tuples
  are reviewed; only `46A4BC` (four bytes) and `46A4C8` (eight bytes) lack
  backing. Nine complete original executions cover three precision controls
  and three destination sentinels. No constructor output is supplied as data.
  [Complete constructor authority](../.local/mcc-native-render-neutral-20261018/constructor2c390-v1/primary.json).
- [x] Supply the authenticated twelve constructor-input bytes through the
  ordinary data module. All nine complete native executions match Original
  GP/defined flags, ordered reads/stores, stack, instruction trace and all 128
  destination bytes; 25 negative controls pass. All 2,423 prior factory owners
  are exact, and the new native backings are above 4 GiB. Independent source
  and reconstructed-factory review passes.
  [Constructor product validation](../.local/mcc-native-render-neutral-20261018/product2c390-v1/validation.json),
  [Independent review](../.local/mcc-native-render-neutral-20261018/root-review-v1/review.json).
- [x] Qualify the constructor fixture's emulator status mismatch without
  changing engine arithmetic. The exact original floating-point instruction
  sequence on the physical host produces `FPSW=0220`, matching native, while
  Unicorn reports zero. All original stored float words still match. Preserve
  both initial failed checks and the frozen emulator report; compare status to
  the separate host instruction witness. This is not physical Xbox certification.
  [Host instruction qualification](../.local/mcc-native-render-neutral-20261018/host-x87-v1/validation.json),
  [Preserved failures and cleanup](../.local/mcc-native-render-neutral-20261018/product2c390-v1/handoff.json).
- [x] Pass the actual later-frame neutral rendering regression with the
  reviewed constructor inputs. Combined V12 exits successfully after 26 frames
  and 30 simulation ticks, with a real gameplay capture and 107,520 nonzero
  stereo callback frames (2.24 seconds). All 314 providers, 97 shaders and maps
  stay unchanged; the initial reference filename filter finds no opens.
  The prior missing-input failure is closed. This is a bounded runtime test,
  not complete visual, audio or gameplay acceptance.
  [Longer unmuted runtime](../.local/mcc-native-client-20261018/client-current-v12-neutral-mcc/audio-neutral-v1-real-mcc/validation.json).
- [x] Preserve a reproducible image check for the left-edge geometry symptom.
  The pinned V10 image has zero matching pixels in the declared region;
  V12 tick30 has 4,639. This checks the visible symptom only; different capture
  ticks do not identify its cause. Diagnostic scratch is removed. Next, join
  actual accepted draw spans to the same submitted camera and model identity.
  [Bounded symptom replay](../.local/mcc-left-edge-geometry-20261018/v1/capture-symptom.json).
- [x] Diagnose and correct the oversized left-edge world-weapon geometry.
  Actual accepted Needler geometry used normalized class1 MCC positions/UVs
  whose compression flags the CPU loader skipped. The bounded source fix
  below restores authored expansion. The actual model-only tick30 image
  removes the symptom; root inspected the lossless result. Whole-scene
  materials and full TU1.5 framebuffer parity remain separate gates.
  [Original symptom](../.local/mcc-native-client-20261018/client-current-v12-neutral-mcc/audio-neutral-v1-real-mcc/lockout-view.png),
  [Corrected image](../.local/mcc-left-edge-geometry-20261018/class1-fix-v1/capture/lockout-class1-fixed.png).
- [x] Reproduce the geometry symptom in one observer-only ordinary tick30 run.
  It retains the same 4,639 matching pixels and accepts a Needler draw whose
  verified model/vertex join projects into the affected region. Its accepted
  world bounds span about two units. All 314 V12 providers, maps and 97 shaders
  stay pinned, no reference files open, and generated binaries/captures/traces
  are removed. Projected overlap alone does not prove pixel ownership or the
  decoding cause; product geometry, clipping and visibility are unchanged.
  [Actual diagnostic run](../.local/mcc-left-edge-geometry-20261018/v1/actual-v1/validation.json),
  [Accepted draw identities](../.local/mcc-left-edge-geometry-20261018/v1/actual-v1/edge-geometry.json).
- [x] Inspect the actual paired MCC/Original world-weapon resource data.
  Needler's 1,992 and Energy Blade's 2,148 selected vertices preserve counterpart
  counts, section/part order, bounds and node maps. Their class1 MCC streams
  retain normalized FLOAT positions/UVs with compression flags3; the current
  CPU path skips that class's expansion. Applying authored compression only
  in a diagnostic copy reduces XYZ differences to 3.94e-6/9.24e-6 and UV
  differences to about 2.3e-5, within the observed Xbox quantization.
  Existing selected human-weapon controls remain unchanged. No product fix
  or new gameplay image is admitted by this census. The full-section gate
  below extends this evidence; the actual decoding correction is still pending.
  [Actual resource counterparts](../.local/mcc-left-edge-geometry-20261018/v1/intrinsics-v1/report.json),
  [Probe provenance and cleanup](../.local/mcc-left-edge-geometry-20261018/v1/intrinsics-v1/validation.json).
- [x] Complete all-section counterpart and Original class1 consumption review.
  All 22 sections and 18,219 expanded vertices retain counterpart counts,
  section/part order, bounds and node maps. Diagnostic flagged expansion
  reduces maximum differences to 9.24e-6 XYZ and 2.27e-5 UV. The authenticated
  original class1 blade program consumes `c170..172` for XYZ/UV before palette
  transforms. Independent review supports adding class1 to the existing MCC
  flag gates; class0, Xbox parsing, formats, normals, origins, matrices and
  shaders stay outside that change. Offline Original inspection is reference
  work, not a runtime independence test. Actual decoding and gameplay image
  acceptance remain next; no bit-identical or full-parity claim is made.
  [Full-section counterpart census](../.local/mcc-left-edge-geometry-20261018/v1/intrinsics-v2-summary.json),
  [Independent Original draw and pair review](../.local/mcc-left-edge-geometry-20261018/v1/intrinsics-v1/independent-review.json).
- [x] Apply the bounded class1 MCC decoding correction and verify all paired
  sections and unchanged controls. All 22 sections/18,219 expanded vertices
  pass the real Cache regression; Original reports and every unrelated vertex,
  skin, origin and bone field remain exact. Only the existing class gate and
  its comment change. The 3,312,952-byte model object uses the exact frozen V12
  dependencies, and compiler/link scratch is removed. Coordinate residuals
  remain qualified as quantization differences. Source promotion matches the
  tested candidate; installed runtimes and releases remain unchanged.
  [Class1 regression](../.local/mcc-left-edge-geometry-20261018/class1-fix-v1/controls/validation.json),
  [Guarded source promotion](../.local/mcc-left-edge-geometry-20261018/class1-fix-v1/promotion.json).
- [x] Inspect the model-object-only V12 tick30 capture after the class1 fix.
  Actual ordinary Main exits successfully after 31 frames/30 ticks; the
  left-edge discriminator drops from 4,639 pixels to zero. Root visually
  confirms the corrected edge and retained arms/SMG/HUD. All prior 314 inputs,
  maps and 97 shaders are pinned; generated ELF/audio/trace scratch is removed.
  Animation fractions vary with the ordinary clock, so this is a geometry
  regression rather than a whole-image or phase-parity comparison.
  [Causal native run](../.local/mcc-left-edge-geometry-20261018/class1-fix-v1/capture/validation.json),
  [Verified image and retention handoff](../.local/mcc-left-edge-geometry-20261018/class1-fix-v1/handoff.json).
- [x] Adopt the verified model fix and reviewed counter/response data into the
  coherent V13 client, validate its combined runtime, and update installed
  native testing plus compiled version8. Actual local and relocated launchers
  pass. Subsequent classifier/firing work and full readiness remain separate.
- [x] Link the coherent V13 candidate with only the reviewed class1 model and
  latest ordinary data object replacing V12 inputs. All 312 other inputs,
  43 MCC maps and 97 shaders stay pinned; source/header compatibility checks
  pass and link scratch is removed. The 104,675,632-byte runtime and
  20,465,553-byte provider map are retained for required current integration.
  The separate ordinary unmuted tick30 validation below passes; launcher and
  ZIP adoption remain separate from this link-only milestone.
  [Coherent V13 cohort](../.local/mcc-native-client-20261018/client-current-v13-class1-fire-mcc/cohort.json),
  [Provider and compatibility manifest](../.local/mcc-native-client-20261018/client-current-v13-class1-fire-mcc/link-manifest.json).
- [x] Validate ordinary unmuted V13 gameplay through tick30 after combining
  the class1 correction and reviewed firing data. It exits successfully after
  17 frames; root inspected the clean left edge and retained arms/SMG/HUD.
  Real callbacks produce 117,760 finite stereo frames, peak 0.05824 and RMS
  0.007582. The broader 108,120-open audit finds four observed MCC map paths
  and zero legacy asset references. All 314 providers, 43 maps and 97 shaders
  stay pinned; raw audio/trace/frame scratch is removed. This closes the
  combined neutral-runtime gate; installed/package adoption and full firing,
  gameplay and audiovisual parity remain open.
  [Combined V13 runtime](../.local/mcc-native-client-20261018/client-current-v13-class1-fire-mcc/run-v1-real-mcc/validation.json),
  [Full file audit](../.local/mcc-native-client-20261018/client-current-v13-class1-fire-mcc/run-v1-real-mcc/file-audit.json),
  [Retention handoff](../.local/mcc-native-client-20261018/client-current-v13-class1-fire-mcc/handoff.json).
- [ ] Complete ordinary Main's real firing path and verify shot/audio output.
  Actual step3 fire originally failed at `F0D38`; the data-only rerun clears
  that exact boundary and reaches the separate `1694A2` read at `16957F`.
  The next table-only run clears that boundary too, then fails at `CC26A`
  reading `CC30C`. The reviewed hand tables clear that dependency; the fourth
  firing run now reaches `E999F` reading `E9BB8`. The fifth run clears those
  tables and reaches evaluator `17F8A0`, reading missing response row3 at
  `478EA8`. The sixth run clears row3 and reaches the separate `15F77A`
  read at `48C7A4`, under root `B9090`. The seventh clears the counter and
  reaches `19B8FE` reading classifier row8 at `19BA18`. The eighth clears
  that leaf and reaches `FFF44` reading scalar `41C4CC`, under root `B9180`.
  All eight isolated
  firing runs have silent device output and no
  captured shot. Actual firing remains
  a runtime gate.
  [Actual firing run](../.local/mcc-native-client-20261018/client-current-v8-fp/audio-fire-v1-real-mcc/validation.json).
- [x] Authenticate and supply the demonstrated `F0BE0` dispatch data: one
  31-byte read-only owner containing four targets and fifteen selectors.
  All 2,420 prior factory owners are unchanged. Twenty-six Original/native
  prefix cases and 40 negative controls pass; independent direct-XBE review
  confirms the bytes, unsigned guard, exact read operands and ownership.
  Descendant calls and the actual firing/audio rerun remain separate.
  [Dispatch data validation](../.local/mcc-native-audio-capture-audit-20261018/fire-dispatch-product-v1/validation.json),
  [Independent primary review](../.local/mcc-native-audio-capture-audit-20261018/fire-dispatch-root-review-v1/review.json).
- [x] Clear the exact `F0D38` firing dependency in ordinary Main using only
  the reviewed 31-byte data owner. The next failure is the separate `16957F`
  table read; other providers and shaders are unchanged. No audible shot is
  claimed from this failed run.
  [Data-only runtime regression](../.local/mcc-native-client-20261018/client-current-v8-fire-data-v1/audio-fire-v2-real-mcc/validation.json).
- [x] Authenticate and supply the complete 164-byte `169493` read-only table.
  All 65,536 low-word selectors plus eight upper-word aliases match unchanged
  original execution; 173 negative controls pass and all 2,421 prior factory
  owners are preserved. Independent direct-XBE/source/factory review passes.
  A separate corrected illustrative report qualifies a mutable-list reporting
  defect; the complete Original/native row digest and frozen product proof are
  unchanged. Actual descendant firing/audio remains unchecked.
  [Table product validation](../.local/mcc-native-audio-capture-audit-20261018/fire-symbol-product-v1/validation.json),
  [Independent review](../.local/mcc-native-audio-capture-audit-20261018/fire-symbol-independent-review-v1/review.json),
  [Corrected illustrations](../.local/mcc-native-audio-capture-audit-20261018/fire-symbol-proof-v2/primary.json).
- [x] Clear the exact `16957F` dependency through ordinary Main using only the
  reviewed 164-byte table. The run advances to the separate `CC30C` dependency
  before failing after 98.02 seconds. All 312 providers, maps and 97 shaders
  stay pinned; zero reference opens. Its 82,432 callback frames are silent,
  so this closes only the prior dependency, not shot or audio acceptance.
  [Table-only runtime regression](../.local/mcc-native-client-20261018/client-current-v8-fire-data-v2/audio-fire-v3-real-mcc/validation.json).
- [x] Authenticate and supply both complete `CC1E0` hand-selection tables in
  one 64-byte read-only owner. All 51 Original/native prefix cases and 73
  negative controls pass; all 2,422 prior factory owners remain exact. Root's
  independent direct-XBE review verifies the entire 297-byte/88-instruction
  owner, both unsigned selector guards, all generated instruction identities,
  fresh Original witnesses and the reconstructed full factory digest. This
  admits the data only; actual descendant firing/audio remains a runtime gate.
  [Hand-table product validation](../.local/mcc-native-audio-capture-audit-20261018/fire-cc2-product-v1/validation.json),
  [Independent root review](../.local/mcc-native-audio-capture-audit-20261018/fire-cc2-root-review-v1/review.json).
- [x] Clear the exact `CC30C` dependency in ordinary Main with only the
  reviewed hand-table data. The fourth run advances to `E999F`/`E9BB8`, then
  fails after 74.54 seconds. All 312 providers, maps and 97 shaders stay
  pinned; zero reference opens and removed scratch. Its 63,488 callback
  frames are silent; no completed shot or firing audio is claimed.
  [Hand-table runtime regression](../.local/mcc-native-client-20261018/client-current-v8-fire-data-v3/audio-fire-v4-real-mcc/validation.json).
- [x] Authenticate and supply the `E9990` dispatch data as four exact read-only
  spans totaling 204 bytes, excluding padding and overlapping default words.
  The real 60-entry authored catalog selects this owner for 18 types; it is
  not a new guard for arbitrary requests. All 104 Original/native prefixes,
  3,030 instructions and 227 negative controls pass, including 60 genuine
  `B9390` calls. Independent whole-body/domain/source and full-factory review
  preserves all 2,425 prior owners exactly. The actual firing rerun and
  `116570` descendant completion remain separate gates.
  [E99 product validation](../.local/mcc-native-audio-capture-audit-20261018/fire-e99-product-v1/validation.json),
  [Independent review](../.local/mcc-native-audio-capture-audit-20261018/fire-e99-independent-review-v1/review.json).
- [x] Clear the exact E99 table dependency through ordinary Main. The fifth
  firing run reaches `17F946`/`478EA8`, then fails after 72.97 seconds. All
  312 providers, maps and 97 shaders remain unchanged; zero reference opens.
  Its 57,344 callback frames are silent. The reached function-table provider
  and real type domain need verification; no curve or shot is invented.
  [E99 runtime regression](../.local/mcc-native-client-20261018/client-current-v8-fire-data-v4/audio-fire-v5-real-mcc/validation.json).
- [x] Authenticate the reached response row3 against the existing canonical
  material-table provider. All 1,024 bytes equal Original `478EA8`; the full
  unchanged `17F8A0` evaluator and its genuine remainder helper execute in
  2,059 dense cases plus 36 complete state witnesses at three x87 controls.
  Every row byte is read, and observed writes stay in the declared stack.
  Original stores this authored table in a writable `.data` section; this
  proof does not establish arbitrary types, NaN behavior or a whole-bank
  lifetime contract. Native projection/pairing and actual firing remain next.
  [Existing-provider identity](../.local/mcc-native-audio-capture-audit-20261018/fire-curve3-proof-v1/inspection.json),
  [Complete Original evaluator proof](../.local/mcc-native-audio-capture-audit-20261018/fire-curve3-proof-v1/primary.json),
  [Independent raw-XBE and fresh replay review](../.local/mcc-native-audio-capture-audit-20261018/fire-curve3-root-review-v1/primary-review.json).
- [x] Project the reached response row through the ordinary data module,
  and verify complete native evaluator behavior and prior-owner preservation.
  Only the exact 1,024-byte row is added; all 2,429 prior owners remain exact.
  The 36 complete and 2,059 dense cases pass 269,990 instructions and 1,032
  negative controls with no supplied child return. Strict non-FP behavior
  matches Original; CW, FPSW, MXCSR and empty x87 stack/tags match the
  physical-host witness below.
  The required 1,125,904-byte ordinary data object is retained; compiler/link
  scratch is removed. Independent review confirms the exact source change,
  reconstructed complete factory digest and unchanged dependencies/providers.
  The actual firing rerun is next; shot/audio output remain a runtime gate.
  [Response-row product validation](../.local/mcc-native-audio-capture-audit-20261018/fire-curve3-product-v1/validation.json),
  [Independent product review](../.local/mcc-native-audio-capture-audit-20261018/fire-curve3-root-review-v1/product-review.json).
- [x] Clear the exact `17F946` response-row dependency in ordinary Main.
  The sixth firing run advances to `15F77A` reading `48C7A4`, then fails after
  47.99 seconds. All 312 providers, maps and 97 shaders stay pinned, zero
  reference opens are reported, and scratch is removed. Its 45,056 callback
  frames are silent; no completed shot, firing audio or capture is claimed.
  The next owner and its existing development provider need verification.
  [Response-row runtime regression](../.local/mcc-native-client-20261018/client-current-v8-fire-data-v5/audio-fire-v6-real-mcc/validation.json).
- [x] Authenticate the next counter/slot-publication prefix in Original
  `15F6E0`: 67 cases and 6,359 instructions cover all eight slot choices,
  expired/oldest selection, stable ties, early returns and low-word wrap.
  The read at `48C7A4` spans a low16 counter, a separately mutable adjacent
  policy byte and one high byte; raw cold bytes are `2A000100`. Only the
  low16 counter is incremented, while the separate original policy owner can
  clear its byte. No child execution/return or normal Xbox post-boot state is
  supplied or claimed. Independent review supports process-cold fields with
  warm/snapshot preservation; full boot and map-lifetime parity stay open.
  [Counter publication Original proof](../.local/mcc-native-audio-capture-audit-20261018/fire-counter-proof-v1/primary.json),
  [Independent owner/domain review](../.local/mcc-native-audio-capture-audit-20261018/counter-independent-review-v1/review.json).
- [x] Verify a private native three-owner overlay against all 67 Original
  counter prefixes and 6,359 instructions. GP, defined flags, XMM/FPU state,
  ordered reads/stores/PCs and complete stack/player/counter bytes match.
  Seven controls verify cross-owner DWORD reads, independent policy mutation,
  atomic write rejection at the RO byte and checkpoint retention. No child
  return or complete policy execution is supplied. Both private compile/test
  scratch trees are removed; no binaries are retained. Ordinary product
  mounting and actual firing are still separate gates.
  [Native counter proof](../.local/mcc-native-audio-capture-audit-20261018/fire-counter-proof-v1/native-validation.json).
- [x] Admit the three process-cold fields through the ordinary data module.
  Only four bytes in three exact owners are added; all 2,430 prior owners
  remain exact. All 67 native prefixes/6,359 instructions and 15 ownership,
  atomic preflight, policy, checkpoint and warm-preservation controls pass.
  The required 1,152,640-byte ordinary object is retained; compile/link scratch
  is removed. Independent source, complete reconstructed factory, object,
  dependency and control review passes. Actual firing is next; full process/map
  lifetime parity remains separate. No per-shot reset is added. The 15 controls
  comprise ten rejection and five positive ownership/state checks.
  [Counter product validation](../.local/mcc-native-audio-capture-audit-20261018/fire-counter-product-v1/validation.json),
  [Independent product review](../.local/mcc-native-audio-capture-audit-20261018/counter-product-independent-review-v1/review.json).
- [x] Clear the exact counter dependency through ordinary Main. The seventh
  firing run reaches `19B8FE`/`19BA18`, then fails after 51.75 seconds. The
  actual consumed DWORD is `0001002A`, preserving the adjacent policy bit.
  All 312 providers, maps and 97 shaders stay pinned; no reference opens.
  Its 58,368 callback frames are silent. No completed shot or firing image
  is claimed. The classifier's demonstrated row8 is the next dependency.
  [Counter runtime regression](../.local/mcc-native-client-20261018/client-current-v8-fire-data-v6/audio-fire-v7-real-mcc/validation.json).
- [x] Prove the complete unchanged `19B8F0` classifier with a private native
  table overlay. All 136 complete Original/native RET cases pass 856
  instructions and 88 table reads, matching GP, defined flags, XMM/FPU,
  ordered reads/PCs and complete memory. Its low6 mask and unsigned guard
  bound the reachable table to 41 targets; no writes or children are supplied.
  The overlay leaves product source unchanged and retains no large artifacts.
  Independent domain, raw-XBE, whole generated body and actual-reach review
  pass. Ordinary product publication and firing are next.
  [Original classifier proof](../.local/mcc-native-audio-capture-audit-20261018/fire-classifier-proof-v1/primary.json),
  [Private native proof](../.local/mcc-native-audio-capture-audit-20261018/fire-classifier-proof-v1/native-validation.json),
  [Independent classifier review](../.local/mcc-native-audio-capture-audit-20261018/classifier-independent-review-v1/review.json).
- [x] Publish the demonstrated classifier table through ordinary native data.
  Its only new owner is 164 bytes RO; all 2,433 prior owners remain exact.
  All 136 complete Original/native RET cases, 856 instructions, 88 table reads
  and 171 ownership controls pass (170 rejections and one positive native
  pointer check). Independent source, reconstructed full-factory, dependency,
  object and actual control-source review passes. The required 1,161,088-byte
  ordinary object is retained and scratch is removed. The actual eighth firing result follows below;
  successful firing/audio remains open.
  [Classifier product](../.local/mcc-native-audio-capture-audit-20261018/fire-classifier-product-v1/validation.json),
  [Independent product review](../.local/mcc-native-audio-capture-audit-20261018/classifier-product-independent-review-v1/review.json).
- [x] Clear the exact classifier dependency through ordinary Main. The eighth
  firing run advances to root `B9180`/`FFF44` reading scalar `41C4CC`, then
  fails after 79.53 seconds. All 312 providers, maps and 97 shaders stay
  pinned; zero reference opens and removed scratch. Its 54,784 callback frames
  are silent; no completed shot, firing image or sound is claimed. Authenticate
  the new immediate owner and existing scalar provider before any publication.
  [Classifier runtime regression](../.local/mcc-native-client-20261018/client-current-v8-fire-data-v7/audio-fire-v8-real-mcc/validation.json).
- [x] Authenticate the newly reached angle-offset scalar and complete immediate
  owner independently. A separate XBE parser and recursive decoder reproduce
  all 1,194 `FFA60` instruction sites, including both guarded seven-entry
  switches and all 26 direct numeric reads. `41C4CC` is exactly the read-only
  binary32 word `3FC90FDB`, already used by the development projectile path;
  its three-instruction consumer follows the genuine `11F7E0` call. This is
  static authority only. The bounded native execution pairing and ordinary
  publication follow below; an actual successful firing run remains open.
  `46A7D4` is inventory-only.
  [Immediate-owner inventory](../.local/mcc-native-audio-capture-audit-20261018/fire-angle-offset-proof-v1/inspection.json),
  [Independent raw-owner review](../.local/mcc-native-audio-capture-audit-20261018/angle-offset-independent-review-v1/authority-review.json).
- [x] Pair the unchanged three-instruction angle-offset consumer with Original
  execution using a private four-byte read-only overlay. All 104 finite-domain
  cases pass 312 instructions, both input flag seeds and four SSE rounding
  modes, with exact GP, all XMM lanes, ordered reads/store, full stack, MXCSR
  and measured x87 control/status. Flags are unchanged by the three operations;
  empty x87 tags follow initialization with no x87 operation in this slice.
  All 312 cohort inputs remain unchanged, no reference opens occur and scratch
  is removed. This does not execute the complete owner, arbitrary float inputs
  or a shot; ordinary publication follows below and actual firing integration
  remains open.
  [Original scalar consumer](../.local/mcc-native-audio-capture-audit-20261018/fire-angle-offset-proof-v1/primary.json),
  [Private native pairing](../.local/mcc-native-audio-capture-audit-20261018/fire-angle-offset-proof-v1/native-validation.json).
- [x] Publish the exact four-byte `41C4CC` operand through the ordinary data
  factory. All 2,434 prior owners remain exact; the final factory has 2,435
  regions and 479,712 bytes. All 104 finite-domain consumer cases and ten
  ownership rejections plus a native pointer above 4 GiB pass. Independent
  source/factory/object/dependency review passes, with no `46A7D4`, new math,
  ABI, generated code or shader changes. The required 1,170,328-byte leaf is
  retained; compiler/test scratch is removed. Actual coherent firing remains
  the next gate.
  [Ordinary scalar publication](../.local/mcc-native-audio-capture-audit-20261018/fire-angle-offset-product-v1/validation.json),
  [Independent publication review](../.local/mcc-native-audio-capture-audit-20261018/angle-offset-independent-review-v1/product-review.json),
  [Existing-provider qualification and cleanup](../.local/mcc-native-audio-capture-audit-20261018/fire-angle-offset-proof-v1/handoff.json).
- [x] Link the reviewed data leaf into the current coherent V13 client. Only
  the startup-data object changes; its other 313 providers, 43 MCC map files,
  97 shaders and ordinary header dependencies stay pinned. The private ELF
  and map are retained for actual firing validation; link scratch is removed.
  The installed V13 and published version8 stay unchanged. Actual firing and
  sound acceptance remain open.
  [Coherent firing cohort](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v1/cohort.json),
  [One-leaf link evidence](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v1/link-manifest.json).
- [x] Verify the angle-offset read clears in the actual coherent firing run.
  The ninth run advances through it to `100105`, reading `100D08` in
  `FFA60`'s first guarded seven-entry dispatch table, then exits1 after
  72.16 seconds. All 313 other providers, 43 map files and 97 shaders stay
  pinned, with zero reference opens and removed scratch. The genuine callback
  supplies 67,072 nonzero ambience frames; no completed shot, firing sound or
  screenshot is claimed. Authenticate both full-owner dispatch tables and
  the remaining effect-scale operand before the next runtime attempt.
  [Coherent ninth firing result](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v1/audio-fire-v9-real-mcc/validation.json).
- [x] Execute the complete Original local effect-scale selector and clamp
  (`10070C..100756`): 504 finite-domain cases cover all signed16 mode classes,
  threshold angles, preceding damage factors and both flag seeds in 3,648
  instructions. No child or supplied result is used. The exact `46A7D4` word
  already exists in the development effect-scale routine. Native pairing,
  ordinary publication and full firing remain open.
  [Original effect-scale proof](../.local/mcc-native-audio-capture-audit-20261018/fire-effect-scale-proof-v1/primary.json).
- [x] Pair the complete bounded effect-scale segment with unchanged native
  code and the selected development routine: all 504 cases/3,648 instructions,
  84 scalar reads and the existing 924-case contact fixture pass their result,
  GP/XMM/defined-flag/read/store/memory checks. Thirty-six strict emulator
  MXCSR discrepancies remain preserved as RED evidence; exact execution of
  the Original eight-byte `MULSS` on the host qualifies the missing sticky
  precision flags for all 504 native cases. The test return lies outside those
  eight bytes; no complete host segment or Xbox hardware claim is made. All
  test scratch is removed. Independent raw-code and integer nearest-even FP
  review passes. Original mode0 is unclamped; agreement with the development
  helper is limited to the declared damage factors `[0,.25,1]`. Ordinary
  operand publication remains open.
  [Qualified native effect scale](../.local/mcc-native-audio-capture-audit-20261018/fire-effect-scale-proof-v1/qualified-validation.json),
  [Independent effect-scale review](../.local/mcc-native-audio-capture-audit-20261018/effect-scale-independent-review-v1/review.json),
  [Physical multiplication authority](../.local/mcc-native-audio-capture-audit-20261018/fire-effect-scale-proof-v1/host-multiply-validation.json).
- [x] Prove both complete bounded `FFA60` dispatch prefixes against unchanged
  native generated instructions and production WorldState/flags/memory helpers.
  All 68 cases/244 instructions match GP, all XMM lanes, flags, target PC,
  ordered reads and complete memory. Signed16 extension and unsigned `<=6`
  guards bound both tables to seven 32-bit targets/28 bytes each; valid modes,
  defaults and upper aliases are covered. Stops precede the genuine target
  instructions; no target body, child or guest return is supplied. Only private
  read-only overlays are used. All compiler/link scratch is removed, with no
  retained binaries. Independent raw-XBE/guard/helper review reconstructs all
  68 witnesses algebraically and passes; ordinary publication is next.
  [Original dispatch authority](../.local/mcc-native-audio-capture-audit-20261018/fire-dispatch-owner-proof-v1/primary.json),
  [Private native dispatch pairing](../.local/mcc-native-audio-capture-audit-20261018/fire-dispatch-owner-proof-v1/validation.json),
  [Independent dispatch review](../.local/mcc-native-audio-capture-audit-20261018/fire-dispatch-owner-independent-review-v1/review.json),
  [Dispatch proof cleanup](../.local/mcc-native-audio-capture-audit-20261018/fire-dispatch-owner-proof-v1/handoff.json).
- [x] Publish and independently review the two dispatch tables and effect-scale
  operand through the ordinary factory. Only three read-only owners/60 bytes
  are added; all 2,435 previous tuples stay exact, yielding 2,438 owners and
  479,772 bytes. Ordinary-provider checks pass 68 dispatch cases, 504 scale
  cases, the prior 924-case fixture and 81 ownership controls: 77 rejections,
  three native pointers above 4 GiB and one exact cross-owner read. All source
  words match Original; dependencies, 314 prior inputs and strong exports stay
  pinned. No arithmetic, ABI, generated code or shader changes. The required
  1,194,984-byte data object is retained and all product/review scratch is gone.
  Actual coherent firing and sound acceptance remain open.
  [Combined ordinary publication](../.local/mcc-native-audio-capture-audit-20261018/fire-impact-closure-product-v1/validation.json),
  [Independent combined review](../.local/mcc-native-audio-capture-audit-20261018/impact-closure-product-independent-review-v1/review.json),
  [Publication handoff and cleanup](../.local/mcc-native-audio-capture-audit-20261018/fire-impact-closure-product-v1/handoff.json).
- [x] Link the combined data closure into the next coherent V13 client.
  Only the data leaf changes from the ninth-run cohort; 313 other providers,
  all 43 map files and 97 shaders stay pinned. The required private ELF/map
  are retained and link scratch is removed. Installed V13 and packaged
  version8 remain unchanged; the actual tenth firing run is the next gate.
  [Combined firing cohort](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v2/cohort.json),
  [Combined one-leaf link](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v2/link-manifest.json).
- [x] Verify the first dispatch table clears its actual firing boundary.
  The tenth run consumes it and advances through genuine `1027D0`/`18B1D0`
  effects to `18AEC9`, reading `18AF08` in the next bounded table, then exits1
  after 52.20 seconds. The later second-table and effect-scale branches have
  component proofs and remain unobserved in this runtime. All other providers,
  maps, shaders and dependencies stay pinned, with zero reference opens and
  removed scratch. Its 57,344 nonzero callback frames are unidentified ambience;
  no completed shot, firing sound or screenshot is claimed.
  [Actual tenth firing result](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v2/audio-fire-v10-real-mcc/validation.json).
- [x] Execute the complete Original `18AE80` category/fallback leaf and exact
  eight-entry table. All 336 whole-RET cases pass 10,343 instructions, covering
  both category fields, empty/populated metadata, presence patterns, every
  fallback selector, early failure and upper handle aliases. The 123-byte body
  excludes its padding byte and has no child calls. Surviving development
  oracle contracts supply input layouts; missing historical fixture files are
  not claimed as rerun evidence. Publication and actual firing remain open.
  [Original complete fallback leaf](../.local/mcc-native-audio-capture-audit-20261018/fire-effect-fallback-proof-v1/primary.json).
- [x] Pair all 336 cases with the existing public native `18AE80` module and
  the exact private 32-byte table. GP/XMM, defined flags, ordered reads/stores,
  stack, registry, row data and floating-point state match. Only explicitly
  proven observer preimage reads are normalized; the raw failures and diagnostic
  are preserved. No child result is supplied, all 314 providers stay pinned,
  no reference file opens and no large probe artifact remains. Independent
  review and ordinary publication are the next gates.
  [Public native fallback pairing](../.local/mcc-native-audio-capture-audit-20261018/fire-effect-fallback-proof-v1/native-validation.json),
  [Qualified proof and cleanup](../.local/mcc-native-audio-capture-audit-20261018/fire-effect-fallback-proof-v1/handoff.json).
- [x] Inventory the complete direct/tail effect-helper closure rooted at
  `1027D0` and `18B1D0` in its two selected generated modules. All eight whole
  functions and 526 instruction PCs are covered with no external or indirect
  call boundary. Full byte-extent checks against the 2,438-owner factory find
  only the missing `18AEFC` table's 32 immutable bytes. This is a static input
  inventory, not execution or full firing acceptance; both inventory scratch
  directories are verified absent.
  [Bounded effect-helper inventory](../.local/mcc-native-audio-capture-audit-20261018/effect-helper-inventory-v1/inventory.json).
- [x] Independently accept the whole fallback-leaf proof and helper inventory.
  Raw Original bytes and an independent reconstruction match all 336 cases,
  10,343 instruction PCs, 173 table reads and 945 register PUSH stores. Every
  owned site and table index is covered; observer normalization is limited
  to callback-proven preimage reads with the raw failures retained. The
  eight-function closure and every prior factory tuple also match. The
  exact 32-byte read-only table is eligible for ordinary publication;
  product-provider tests and actual firing remain separate open gates.
  [Independent fallback and closure review](../.local/mcc-native-audio-capture-audit-20261018/effect-fallback-independent-review-v1/review.json).
- [x] Publish the exact 32-byte fallback table through the ordinary factory
  and pass its provider fixtures. All 2,438 previous tuples remain exact,
  yielding 2,439 regions/479,804 bytes. The same 336 whole-RET cases and 40
  rejecting controls plus one native pointer above 4 GiB pass, including every
  overlap byte, read-only/bounds guards and warm-state preservation. Only the
  data leaf and bounded preflight change; no generated body, ABI or shader
  changes. The required 1,206,112-byte object is retained and both scratch
  directories are removed. Independent product acceptance and the next actual
  firing run remain open; installed V13 and packaged version8 stay unchanged.
  [Ordinary fallback provider](../.local/mcc-native-audio-capture-audit-20261018/fire-effect-fallback-product-v1/validation.json),
  [Product handoff and cleanup](../.local/mcc-native-audio-capture-audit-20261018/fire-effect-fallback-product-v1/handoff.json).
- [x] Independently accept the ordinary fallback-table publication. Source
  normalization, all eight Original words, all 2,438 previous factory tuples,
  the 2,439-region snapshot, object, dependencies, 314 prior inputs, exports
  and exact control links match. The complete accepted execution/observer
  loop is unchanged except ordinary-provider checks and labels. All 40
  rejecting controls plus the native pointer case are accounted for; actual
  firing remains unverified beyond the tenth run.
  [Independent ordinary fallback review](../.local/mcc-native-audio-capture-audit-20261018/effect-fallback-product-independent-review-v1/review.json).
- [x] Link the accepted fallback data leaf into coherent firing candidate v3.
  The link succeeds with all 313 other providers, 43 maps and 97 shaders exact.
  The required private ELF is 104,675,632 bytes and its map is 20,465,770 bytes;
  installed V13 and published version8 remain unchanged. Actual eleventh firing
  validation is the next gate.
  [Coherent fallback-table candidate](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v3/cohort.json),
  [Single-leaf link receipt](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v3/link-manifest.json).
- [x] Verify the fallback table clears its actual firing boundary. The eleventh
  coherent run advances beyond `18AEFC` to native root `179DF0`, then stops at
  `2C2AD8` reading the next table at `2C2B30`, exiting1 after 53.60 seconds.
  All 313 other providers, 43 maps, 97 shaders and dependencies remain exact,
  with zero reference opens. Its 59,904 nonzero callback frames are unidentified
  ambience; no completed shot, firing sound, screenshot or full run is claimed.
  The complete new table owner and surrounding particle-frame dependencies
  are the next compatibility work.
  Link/runtime/sealer scratch, raw PCM, full syscall trace and temporary save
  are removed; the requested 239,660-byte WAV and small diagnostics are retained.
  [Actual eleventh firing result](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v3/audio-fire-v11-real-mcc/validation.json),
  [Actual run handoff and cleanup](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v3/audio-fire-v11-real-mcc/handoff.json).
- [x] Inventory the new particle-module direct/tail dependency closure from
  `179DF0` and `2C2AD0`. All 50 reached whole functions and 3,686 instruction
  sites match the unchanged Original CFGs. Its 62 external/indirect boundaries
  are explicit and unexpanded. The only unowned fixed inputs are the 16-byte
  read-only table at `2C2B30`, a separate four-byte read-only scalar at `46A784`,
  and a mutable virtual-tail global at `56E0E8`. The latter is not admitted
  as zero or any other runtime default. The 2,439-owner factory stays exact;
  this scan admits no data and proves no new execution. Scratch is removed.
  [Whole particle-module static inventory](../.local/mcc-native-audio-capture-audit-20261018/particle-frame-inventory-v1/inventory.json),
  [Qualified inventory handoff](../.local/mcc-native-audio-capture-audit-20261018/particle-frame-inventory-v1/handoff.json).
- [x] Execute untouched Original `2C2AD0` dispatch and declared no-active-instance
  paths. All four table indices and unsigned rejection controls pass 32 prefix
  cases; 52 whole-RET cases execute type0 with the real `176A40` child or reject
  an out-of-range type. Together they execute 3,314 instructions with complete
  input/output, reads, stores, stack and FP receipts. The full owned CFGs match
  current generated sites. Historical particle-motion reports are reused as
  context only: their absent memory banks prevent claiming a rerun. Native
  pairing, active-instance/collision branches and ordinary publication remain open.
  [Original particle dispatch and declared whole paths](../.local/mcc-native-audio-capture-audit-20261018/particle-dispatch-proof-v1/primary.json).
- [x] Pair those 32 dispatch controls and 52 whole-RET paths with native
  execution, reusing the existing public particle module for every whole path.
  All 3,314 executed instructions, GP/XMM, defined flags, ordered reads/stores,
  complete 1,024-byte stacks, input banks and FP state match Original. Every
  dispatch index is read; only exact observer-proven preimage reads are
  normalized. The private 16-byte table has a native pointer above 4 GiB.
  All 314 prior providers stay exact, zero reference files open, and scratch
  is removed. This proves the declared paths, not active collision or firing.
  [Native particle dispatch and declared whole paths](../.local/mcc-native-audio-capture-audit-20261018/particle-dispatch-proof-v1/native-validation.json).
- [x] Review the table with a separate raw-file and unsigned-algebra method.
  All 32 prefix states/targets/flags/reads and 16 complete rejected paths are
  independently reconstructed, and every target belongs to the owned CFG.
  The 16 bytes are fully file-backed and read-only. The other 36 whole paths
  retain their Original/native pairing and 1,008 exactly qualified observer
  preimages. All providers/dependencies remain pinned. This is a second method
  by the primary agent, not a second-agent review; active-particle/collision
  and firing claims remain open. Exact ordinary table publication is eligible.
  [Raw-file and algebraic table review](../.local/mcc-native-audio-capture-audit-20261018/particle-dispatch-proof-v1/review.json).
- [x] Publish the exact 16-byte particle dispatch table through the ordinary
  factory. A real old/new factory pair confirms every previous tuple is exact:
  2,439 regions/479,804 bytes become 2,440/479,820 with one read-only owner.
  The 32 dispatch controls, 52 whole paths and 24 rejection controls plus one
  native pointer check pass, including all 16 overlap bytes, endpoints, exact
  bounds, excluded padding/code and warm-state preservation. Only data and its
  bounded preflight change. The required 1,214,872-byte data object is retained;
  scratch executables, maps, raw snapshots and traces are removed.
  [Ordinary particle dispatch provider](../.local/mcc-native-audio-capture-audit-20261018/particle-dispatch-product-v1/validation.json).
- [x] Review the ordinary 16-byte projection with a separate inverse-source
  and raw-file method. Removing only the new mount/preflight reproduces the
  exact prior source; its four words match file-backed Original bytes. The
  accepted execution/observer loops are otherwise byte-identical, all prior
  providers/dependencies and exports match, and both native addresses and
  factory bounds stay exact. Proof/product/review scratch is removed. This
  primary-agent second-method review admits the bounded table; the next actual
  firing run and full active-instance/collision behavior remain open.
  [Ordinary particle product review and cleanup](../.local/mcc-native-audio-capture-audit-20261018/particle-dispatch-product-v1/review.json).
- [x] Link the accepted 16-byte table into coherent firing candidate v4.
  Only the data leaf changes; all 313 other providers, 43 maps, 97 shaders
  and prior compatibility records stay pinned. The required private ELF is
  104,679,728 bytes and its link map is 20,465,765 bytes. Link scratch is
  removed; the installed V13 runtime and version8 ZIP remain unchanged.
  Actual twelfth firing validation is the next gate.
  [Coherent particle-table candidate](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v4/cohort.json),
  [Single-leaf link receipt](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v4/link-manifest.json).
- [x] Verify the particle table clears its actual firing boundary. The twelfth
  run advances through tick markers 0/3/6/8/10 and records up to five world
  and two first-person particle draw submissions. It then exits1 at `179DF0`
  /`2C3098`, reading the unowned mutable `56E0E8` field, after 46.48 seconds.
  This is submission evidence, not a captured firing frame or complete run.
  The 51,200 finite nonzero callback frames remain unidentified audio. All
  other providers, maps, shaders and dependencies stay exact; zero reference
  files open. Raw PCM, trace, temporary save and link/runtime scratch are
  removed, with the requested 204,844-byte WAV and small receipts retained.
  [Actual twelfth firing result](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v4/audio-fire-v12-real-mcc/validation.json),
  [Run progress and cleanup handoff](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v4/audio-fire-v12-real-mcc/handoff.json).
- [x] Identify and execute the real collision-threshold initializer. Complete
  `3829C0` is 17 bytes/three instructions and copies the existing read-only
  `46A1B8` value `3E2AAAAB` into writable `56E0E8`; its genuine constructor-table
  entry is `46DD64`. Sixteen Original whole-RET controls preserve the expected
  GP/XMM/flags/stack/FP state and confirm the unconditional store. Four more
  controls qualify the actual `46A784` scalar load. Both the exact `1/6`
  threshold and `.005` collision offset already occur in the working development
  collision implementation. The missing field is a cold-construction memory
  bridge; its virtual loader zero is not its runtime default. Native ownership,
  warm preservation and actual collision acceptance remain open.
  [Original constructor and existing development constants](../.local/mcc-native-audio-capture-audit-20261018/collision-constructor-proof-v1/primary.json).
- [x] Supply the authenticated collision defaults in the ordinary factory.
  The four-byte mutable field reproduces the complete initializer's persistent
  `1/6` memory effect; the four-byte `.005` word stays read-only. Existing
  `46A1B8` is mounted later in the full factory, so its exact immutable bits
  are materialized without moving, aliasing or changing that previous owner.
  All 2,440 prior tuples stay exact, yielding 2,442 regions/479,828 bytes.
  Eight foreign overlap bytes reject before writes; native pointers exceed
  4 GiB, mutable writes preserve the shared source, and memory-subowner restore
  plus warm counter/policy controls pass. Physics calculations and shaders are
  unchanged. Required data object: 1,232,000 bytes; temporary probes are removed.
  Product review and actual firing remain open; no original CRT caller CPU
  context or whole-world restore claim is made.
  [Ordinary collision-default compatibility](../.local/mcc-native-audio-capture-audit-20261018/collision-defaults-product-v1/validation.json).
- [x] Review the eight-byte collision bridge using direct Original bytes,
  inverse source normalization and the actual compiled control receipts.
  Only the two mounts/preflight pairs change; all 2,440 prior tuples, existing
  development collision source, 314 providers, dependencies and strong exports
  stay exact. Twenty rejecting controls, two high native pointers, four mutable
  write patterns and the quiescent memory-subowner restore pass. Proof/product
  scratch is removed. This primary-agent second-method review admits the
  persistent data bridge, not full CRT CPU context, whole-world restore or
  complete collision/firing parity.
  [Collision bridge review and cleanup](../.local/mcc-native-audio-capture-audit-20261018/collision-defaults-product-v1/review.json).
- [x] Link the accepted collision defaults into coherent firing candidate v5.
  All 313 other providers, 43 maps, 97 shaders and compatibility records stay
  exact. The required private ELF is 104,679,728 bytes and its map is
  20,465,798 bytes; link scratch is removed. Installed V13 and the published
  version8 package remain unchanged. Actual thirteenth firing is the next gate.
  [Coherent collision-default candidate](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v5/cohort.json),
  [Single-leaf collision link receipt](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v5/link-manifest.json).
- [x] Complete the actual bounded SMG firing run in coherent candidate v5.
  Genuine fire-at-3/release-at-4 input completes tick60 with exit0 and 74
  rendered frames. Ammo decreases from the neutral baseline's 60 to 59,
  reserve stays 120, and two detonations are recorded. World/first-person
  particle draw submissions reach seven/two. The captured image was viewed:
  normal arms, SMG, HUD and clear left edge are present. The actual callback
  stream contains 142,336 finite nonzero stereo frames; identifying the firing
  sound and physical audibility remain unverified. All 313 other providers,
  43 maps and 97 shaders stay exact, with zero original-reference opens.
  Link, runtime, conversion and sealing scratch is removed; requested image
  and audio evidence and the required coherent candidate remain. This proves
  one bounded shot, not sustained firing, all weapons or complete parity.
  Installed V13 and the version8 ZIP remain unchanged; refreshing the interim
  launcher/package is next.
  [Actual firing validation](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v5/audio-fire-v13-real-mcc/validation.json),
  [Verified handoff and cleanup](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v5/audio-fire-v13-real-mcc/handoff.json),
  [Actual firing image](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v5/audio-fire-v13-real-mcc/lockout-fire-tick60.png),
  [Actual callback audio](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v5/audio-fire-v13-real-mcc/native-gameplay-audio.wav).
- [x] Execute the entire unchanged Original response evaluator and its real
  CRT helper on the physical host to qualify emulator floating-point reports.
  A disposable static 32-bit ELF retains all Original section bytes at their
  exact addresses; all 36 cases match Original emulator GP, XMM, defined flags
  and every final stack byte. The host supplies separate status/control/tag
  and MXCSR witnesses. All 36 native status/control states match these host
  witnesses exactly, including the emulator's omitted precision/condition
  flags. FIP/FDP/last-opcode fields are outside this comparison. No emulator,
  patched instruction or product dependency is used, and
  all generated ELF/image/capture scratch is removed. This is current-host
  evidence, not Xbox hardware certification.
  [Whole Original physical-host qualification](../.local/mcc-native-audio-capture-audit-20261018/fire-curve3-host-v1/validation.json),
  [Exact native comparison](../.local/mcc-native-audio-capture-audit-20261018/fire-curve3-host-v1/native-comparison.json),
  [Independent capture and authority review](../.local/mcc-native-audio-capture-audit-20261018/fire-curve3-host-v1/independent-review.json),
  [Scratch cleanup](../.local/mcc-native-audio-capture-audit-20261018/fire-curve3-host-v1/cleanup.json).
- [ ] Wire proved native sound-query/playback components into Main and multiplayer.
- [x] Correct the demonstrated MCC looping-sound root layout in the reused
  content loader: its extra eight-byte prefix no longer becomes an Xbox track
  count. All 40 actual MCC maps pass 6,825 definitions, 7,454 tracks, 12,625 sound
  references and 240 negative controls. The original Xbox parser and complete
  resident source bytes remain unchanged. The separate unmuted V12 runtime
  check above captures nonzero ambience; source timing and lifecycle remain open.
  [Looping-sound layout validation](../.local/mcc-looping-sound-ambience-20261018/v1/validation.json).
- [ ] Verify voice/loop lifecycle, timing, RNG, restoration, mute and output behavior.
- [ ] Verify audible gameplay output and synchronization during real play.

## Launcher and delivery

- [x] Keep Play enabled for the clearly labeled interim MCC content test.
- [x] Retain the existing packaged MCC content test build (v6).
- [x] Implement and test the separate native Lockout test capability. Runtime,
  selected map and all 97 shader hashes must match; missing or stale validation
  retains the preview route. The launcher-only checks passed 111 cases across
  three launcher variants, the capability reader and Tk controls. The later
  installed-route and relocatable-package gates below also pass; complete
  gameplay readiness remains false.
  [Launcher capability handoff](../.local/mcc-launcher-native-test-20261018/v1/handoff.json).
- [x] Register the native-test capability and MCC menu-asset controls in CTest.
  An actual disposable CMake/CTest run passed both tests (14 cases), without
  compiling engine targets; all generated build/test scratch was removed.
  [CTest registration validation](../.local/mcc-launcher-native-test-20261018/ctest-registration-v2/validation.json).
- [x] Verify the combined CMake registrations for both HUD manifests, native
  capability, portable shader contract and MCC menu assets. All five focused
  CTests pass; no engine targets are rebuilt and the private build is removed.
  [Combined CTest validation](../.local/mcc-hud-bitmap-geometry-20261018/cmake-final-v1/results/report.json).
- [x] Implement and verify the portable compiled-default shader contract:
  exact read-only path-field relocation, canonical wrapper/loader binding and
  full 97-file inventory. Twenty focused cases and moved-folder launcher/resource
  controls pass, with independent review. Executable instructions and shader
  bytes are unchanged. The actual packaged-execution and publication results
  are recorded separately below.
  [Portable package contract](../.local/mcc-packaged-shader-contract-20261018/v1/handoff.json).
- [x] Build the two portable release launcher binaries for the requested ZIP.
  Both retained release inputs pass 58 routing cases and actual wrapper/relocation
  info checks. These portable binaries are shipped in version7; the installed
  checkout uses the separate local launcher pair below.
  [Release launcher validation](../.local/mcc-native-launcher-delivery-20261018/v1/validation.json).
- [x] Build the two current local launcher binaries with the installed
  checkout's existing absolute project/runtime/Python settings. Both actual
  `--launcher-info` routes select `build-multiplayer/halo2_native` and report
  capability state honestly. Only these two small consumers were compiled;
  their later installation and actual native route are verified below.
  [Local launcher build](../.local/mcc-native-local-launchers-20261018/v1/validation.json).
- [x] Connect the validated interim native build to both installed launchers.
  Both current launcher-info routes now select the exact V12 runtime and report
  the native Lockout test available with full readiness false. The actual
  installed launcher route renders eight frames successfully. Existing shaders
  and source-cache provenance stay unchanged; exact pre-install runtime and
  launcher copies are retained for rollback. The 8,478,720-byte default native
  test save/cache is explicitly retained for user Play.
  [Installed route validation](../.local/mcc-native-install-20261018/v1/validation.json).
- [x] Publish the validated native-test capability in a relocatable compiled
  ZIP after actual packaged execution. The real relocated payload passes
  tick30, both bundled launcher-info routes admit it, the actual launcher
  renders eight frames, and unmuted packaged tick30 produces nonzero PCM.
  Captured file opens show no original-reference or external checkout
  dependency. Missing capability falls back honestly; full readiness stays false.
  [Actual packaged validation](../.local/mcc-native-package-delivery-20261018/v1/run-v1/validation.json).
- [ ] Verify an ordinary user can import, launch and test through the launcher.
- [x] Package a fresh compiled native-test ZIP and verify its extracted contents.
  Version7 contains the current V12 runtime, existing 97 shaders, launchers and
  bundled onboarding dependencies. Every extracted checksum passes and both
  extracted launchers run. Test-only MCC cache/selection/saves are removed;
  no game banks are distributed. README records the verified firing-termination
  limitation and unfinished parity; this is an interim test release.
  [Native test ZIP](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261018-v7.zip),
  [SHA256](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261018-v7.zip.sha256),
  [Actual capture](../dist/Halo2-MCC-Native-Test-Lockout-20261018-v7.png),
  [Actual callback audio](../dist/Halo2-MCC-Native-Test-Ambience-20261018-v7.wav).
- [x] Publish compiled version8 with the verified class1 geometry correction
  and coherent V13 data. The real relocated payload passes tick30, both
  portable launcher-info checks, the actual eight-frame launcher route and
  unmuted tick30 with 113,664 callback frames. ZIP extraction/checksums,
  fallback admission and reference/external-checkout path audits pass.
  All 97 shaders are preserved, test-only cache/selection/saves are removed,
  and original source-cache identities stay unchanged. This remains an interim
  native Lockout test; firing/full readiness, fresh-import acceptance and full
  audiovisual parity are unfinished. Requested ZIP, PNG and WAV are retained.
  [Version8 ZIP](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261018-v8.zip),
  [SHA256](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261018-v8.zip.sha256),
  [Actual package validation](../.local/mcc-native-package-delivery-20261018/v2/run-v1/validation.json),
  [Corrected image](../dist/Halo2-MCC-Native-Test-Lockout-20261018-v8.png),
  [Actual callback audio](../dist/Halo2-MCC-Native-Test-Ambience-20261018-v8.wav).
- [x] Publish compiled version9 with the accepted collision defaults and
  passing bounded SMG firing path. The actual relocated package passes neutral
  tick30, both launcher-info routes, the eight-frame launcher route, and an
  unmuted fire-at-3/release-at-4 run through tick60: 74 frames, ammo 60 to 59,
  two detonations and 156,672 finite nonzero callback frames. Every ZIP
  checksum and extracted launcher check passes; no game data is distributed.
  Captured opens contain no original-reference or external checkout dependency.
  All 97 shaders, 314 providers, 43 maps and 25,004 source-cache identities are
  preserved. Actual PNG was viewed; generated staging, trace, PCM and sealing
  scratch is removed. Previous version8 artifacts stay unchanged. The README
  records the bounded result and unfinished full readiness accurately. Local
  installation of this candidate is next.
  [Version9 compiled ZIP](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261018-v9.zip),
  [SHA256](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261018-v9.zip.sha256),
  [Actual package validation](../.local/mcc-native-package-delivery-20261018/v3/run-v1/validation.json),
  [Verified handoff and cleanup](../.local/mcc-native-package-delivery-20261018/v3/handoff.json),
  [Actual image](../dist/Halo2-MCC-Native-Test-Lockout-20261018-v9.png),
  [Actual callback audio](../dist/Halo2-MCC-Native-Test-Firing-20261018-v9.wav).
- [x] Finish the installed V13 launcher-route test and preserve rollback
  runtime, capability and existing save files. Both actual launcher-info routes
  select runtime `2151bd97...`, native test available and full readiness false;
  the real installed launcher renders eight frames successfully. Shader and
  source-cache provenance are unchanged. Prior runtime/launchers/capability and
  both save files are backed up; installed tests preserve existing save bytes.
  Generated capture/trace scratch is removed, with rollback copies retained.
  [Installed V13 validation](../.local/mcc-native-install-20261018/v2/validation.json).
- [x] Install the exact accepted firing candidate behind both existing Play
  routes. Both actual launcher-info checks select runtime `915a647c...`, admit
  the native Lockout test and keep full readiness false; the actual installed
  launcher renders eight frames with exit0. Both 4,239,360-byte existing save
  files preserve hash `1292a582...`. All 314 providers, 43 maps, 97 shaders
  and source-cache provenance remain exact, with zero reference opens.
  Runtime, launchers, capability and save rollback copies total 113,601,200
  bytes and are explicitly retained. Test/trace/capture and sealing scratch
  is removed. This completes interim installation, not full save/lifecycle,
  sustained firing, campaign or TU1.5 audiovisual acceptance.
  [Installed firing-build validation](../.local/mcc-native-install-20261018/v3/validation.json),
  [Verified selection, rollback and cleanup](../.local/mcc-native-install-20261018/v3/handoff.json).
- [ ] Record and deliver the requested video with sound.
- [ ] Report remaining limitations honestly; do not label partial tests full TU1.5 parity.

## Final cleanup, after validation/delivery

- [ ] Inventory confirmed unused, reproducible test executables and temporary outputs.
- [ ] Check active processes, mapped/open files, current build inputs and launcher selection.
- [ ] Delete obsolete test/build artifacts; preserve useful test source,
  compiled engine/current coherent objects, validation evidence, reference
  assets, installed runtimes and delivered releases.
- [ ] Record exact removed paths and recovered bytes; verify current builds
  and launcher selections still work.

The preliminary cleanup inventory currently identifies approximately 1.26 GiB
of superseded private diagnostic executables. Nothing in that inventory has
been deleted. See
[cleanup plan](../.local/mcc-model-cpu-bitmap-20261018/world-v2-inline/cleanup-after-delivery.json).
Separately, the newly generated V10 syscall trace was summarized by the full
independent audit, then removed with its exact hash and byte count recorded.
This recovered 22,242,541 bytes while preserving the audit, frozen validation,
logs and requested images; it does not complete the final cleanup gates.
[Bounded trace cleanup](../.local/mcc-native-client-20261018/client-current-v10-hud-mcc/run-v1-real-mcc/trace-cleanup.json).

## Next steps, in order

1. Implement and validate the three authenticated Metropolis bitmap
   restorations as a separate V3 bundle, composing the immutable two-entry V2
   Zanzibar bundle. Resolve all five local identities and complete mip/delta
   inputs before any staged cache write. Prove exact native readback and
   rejection before writes for malformed inputs, while preserving all other
   map/DLC bindings and all existing shaders. The byte-identical ocean texture
   needs no pixel correction; the Sacred Icon gradient needs separate
   descriptor/resource evidence. Keep the now-installed firing candidate and
   published version9 available as the interim test. Sound identity,
   sustained/all-weapon firing and full audiovisual parity remain open.
2. Wire the honest native-test launcher capability and MCC-only loading/menu
   asset handoffs beyond the now-published interim Lockout route. Verify a fresh
   MCC-only import, remaining loading/menu roles and mod-folder interoperability.
   The loading-name kernel is sealed; tag1009-to-descriptor ingress, active
   GFx image binding and the genuine `loading_screen_music` event/resource
   binding remain open. The three new Metropolis corrections have source/mip
   evidence and a V3 staging design, but no V3 product or delta is published.
3. Validate the ordinary client and complete safe product/lifecycle integration.
4. Continue the remaining onboarding, conversion, parity and delivery gates above.
5. Perform the final cleanup and record the recovered storage.
