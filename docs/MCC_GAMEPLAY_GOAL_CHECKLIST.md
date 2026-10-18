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

## Current priority: first complete native gameplay frame

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
  material/scene validation and current-client adoption stay open.
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
- [ ] Verify fresh-machine detection and onboarding with only an installed MCC copy.
- [ ] Verify complete required assets: textures, bump maps, models, audio,
  gameplay definitions, UI and loading presentation.
- [ ] Verify MCC internal paths and mod-folder tags/data paths are preserved.
- [ ] Reconstruct the requested loading presentation from MCC and retained
  custom assets; remove dependence on the Vista loading bank.
- [ ] Verify cache-only startup after import without access to the MCC source files.
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
- [x] Finish the installed V13 launcher-route test and preserve rollback
  runtime, capability and existing save files. Both actual launcher-info routes
  select runtime `2151bd97...`, native test available and full readiness false;
  the real installed launcher renders eight frames successfully. Shader and
  source-cache provenance are unchanged. Prior runtime/launchers/capability and
  both save files are backed up; installed tests preserve existing save bytes.
  Generated capture/trace scratch is removed, with rollback copies retained.
  [Installed V13 validation](../.local/mcc-native-install-20261018/v2/validation.json).
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

1. Adopt and verify the bounded class1 correction in the coherent current
   client and installed/package routes. Source promotion, all-section Cache
   checks and both inspected tick30 images pass. V13 is installed and compiled
   version8 is published. Verify the new firing scalar at `FFF44`/`41C4CC`
   against complete owner `FFA60` and existing data providers. The reviewed
   classifier and counter clear their actual runtime boundaries.
   The reviewed response row3 clears its real
   runtime boundary; existing-provider identity, complete Original/native
   evaluator pairing, physical-host FP qualification and owner preservation pass.
   Preserve earlier corrections
   and combine the passing firing path with the
   arm/HUD/namespace and looping-sound fixes. Verify audible output and remaining
   material defects. Preserve the native source path and shaders.
2. Wire the honest native-test launcher capability and MCC-only loading/menu
   asset handoffs beyond the now-published interim Lockout route. Verify a fresh
   MCC-only import, remaining loading/menu roles and mod-folder interoperability.
3. Validate the ordinary client and complete safe product/lifecycle integration.
4. Continue the remaining onboarding, conversion, parity and delivery gates above.
5. Perform the final cleanup and record the recovered storage.
