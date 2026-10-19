# MCC-only native gameplay goal checklist

Updated: 2026-10-19. This checklist owns the remaining scope; detailed evidence
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
- The user's explicit October19 exception is the repository's bundled intro
  and credits movies. Import and use those port-supplied files, with recorded
  hashes/provenance, while preserving the original MCC directory structure.
- The authored SVG button prompts and their exported bitmap atlases are also
  required port-supplied artwork. Preserve the existing custom drawings/pixels;
  use imported MCC logical fonts, metrics and active input bindings. Do not
  introduce a separate Vista cache/font installation to display those prompts.
- Reconstruct Xbox hardware or startup initializers only for demonstrated
  dependencies of the reused gameplay path. Preserve unrelated dirty work.

## Current priority: MCC frontend and binary updates (October19)

Play now opens the validated title/profile/main/Settings route. The next delivery
milestone is the compiled portable ZIP with the movies and custom prompt bank.
Keep the accepted native Lockout test as a separate action. V19's accepted
gameplay conversion/build is preserved; its remaining delivery work follows
this frontend priority.

The user's current sequence is prompt size/placement first, then default Play
opening the title and menus. Keep the MCC cache and existing shaders selected.

- [x] Measure the menu prompt mismatch against the authenticated custom-art
  source. Actual profile/main/Settings quads use selector 1 controls at 44x50
  logical pixels instead of 32x36. At the existing 0.4 canvas scale they render
  17.6 pixels wide and clip vertically; the authored ink should be 12.8x14.4.
  Ordinary ASCII metrics and font ascent/descent match. Source tag bounds and
  text alignment determine placement; the canvas scale needs no adjustment.
  Actual quads and pointer targets are recorded at 640x480 and 1280x720.
  [Independent menu expectations](../.local/mcc-frontend-custom-prompts-20261019/menu-expected-geometry-v1/expected.json),
  [Emitted menu quads](../.local/mcc-frontend-custom-prompts-20261019/native-geometry-v1/menu-quad-validation.json).
- [x] Correct only the qualified menu control metrics, preserving ordinary
  text, source anchors, baseline, clipping rules and existing shader samples.
  Verify complete custom glyphs, expected sizes/positions and matching click
  targets in profile, main and Settings across standard and widescreen views.
  The corrected V23 runtime passes actual title/profile/main/Settings/back
  keyboard and mouse navigation with finite, nonzero menu audio. Both required
  native reference images were viewed; complete SELECT/BACK artwork and text
  fit their panes/footer. The bounded Lockout firing/look/movement regression
  also exits cleanly. All 325 parent inputs and 97 shaders are preserved; the
  successor substitutes only the two admitted helper/painter leaves. Scratch is removed;
  retain the requested coherent candidate (118,332,732 bytes) and two required
  comparison PNGs (180,872 bytes). Full menu-tree and framebuffer parity remain
  separate. Actual installed Play is the next gate.
  [V23 native handoff](../.local/mcc-frontend-custom-prompts-20261019/native-menu-metric-v1/handoff.json),
  [Root review](../.local/mcc-frontend-custom-prompts-20261019/native-menu-metric-v1/root-closure.json).
- [x] Implement and component-test the menu metric correction for exact
  selector 1/7 owners. All 80 menu/HUD profile rows and atomic/idempotence guards
  pass the focused CMake test. Repeated selector 7 independently matches the
  same source font and samples. The painter applies this only to the default
  imported authored bank; ordinary text and the 0.4 canvas remain unchanged.
  The private build is removed. Actual emitted positions and native Play
  acceptance follow separately.
  [Menu component checks](../.local/mcc-frontend-custom-prompts-20261019/menu-metric-implementation-v1/regular/report.json).
- [x] Verify corrected emitted menu geometry against independent source-derived
  expectations. All 14 foreground controls and 14 shadows at 640x480 and
  1280x720 retain complete custom samples, baselines, alignment and four-edge
  Accept/Back click bounds. Maximum comparison error is 0.00011 pixels. The
  profile caption now fits its 245-unit pane (227 rather than 251 units),
  restoring the previously clipped BACK text without changing source anchors.
  CPU and review scratch are removed. Native rendering acceptance follows.
  [Corrected CPU witness](../.local/mcc-frontend-custom-prompts-20261019/native-menu-metric-v1/cpu-handoff.json),
  [Independent geometry review](../.local/mcc-frontend-custom-prompts-20261019/menu-expected-geometry-v1/corrected-review.json).
- [x] After prompt acceptance, bind the passed native title/menu runtime to
  the installed Play action and test the actual button-to-title-to-menu route.
  Preserve the separate native gameplay test and prior runtime for rollback.
  V2 actual direct-host menu/Lockout and both no-argument Tk Play routes all
  pass, including cached intro/skip, finite nonzero audio and a real Settings
  B/BACK click. Actual default-window reference images were viewed. Its final source
  guard rejected concurrent diagnostic-only C++ and stale-selection UI edits;
  the original RED receipt is preserved and the owned stage/scratch removed.
  Source review qualifies the absent-registry context. Adoption will reconstruct
  the exact tested native bytes, reuse those native results and verify fresh
  current-UI Play routes; unrelated live source edits must not trigger repeated
  engine builds. The subsequent actual installed normal and title-first Tk Play
  clicks both pass title/profile/create/confirm/main/Settings/back with finite
  nonzero audio and clean exits. Normal Play uses the cached intro and a real
  Settings B/BACK click. The independently selected V24 frontend preserves the
  corrected prompt leaves and the existing gameplay runtime/106 shader files;
  its own 97 shaders, cache, saves and selections remain unchanged. Both current
  default-window images were reviewed. Scratch is removed; retain two required
  reference PNGs totaling 319,740 bytes. This accepts the stated route, not the
  full menu tree or complete gameplay/framebuffer parity.
  [Installed Play acceptance](../.local/mcc-frontend-custom-prompts-20261019/installed-play-validation-v1/closure.json),
  [Actual V2 routes and cleanup](../.local/mcc-frontend-custom-prompts-20261019/root-play-installation-v2/actual-attempt-closure.json),
  [Reviewed source deltas](../.local/mcc-frontend-custom-prompts-20261019/root-play-installation-v2/root-drift-review.json).
- [x] Restore the missing bundled movie bank in the user's SSD checkout.
  The exact reported metadata error was reproduced: the entire
  `resources/movies` directory was absent. Both original movies and the bank
  manifest are now copied with full payload hashes verified and an atomic
  no-replace directory publication. The tracked outer manifest is unchanged;
  the actual target source-manifest check passes. Retain 2,538,779,177 bytes of
  required assets; owned copy scratch is removed. Full fresh MCC import remains
  a separate gate.
  [Movie-bank repair](../.local/mcc-movie-clone-repair-20261019/repair.json).
- [x] Verify the complete SSD presentation source preflight using that
  checkout's actual importer helpers. Both movie manifests and extents pass;
  the previous full movie hashes remain authoritative. All 91 custom prompt
  files (19,865,846 bytes), including 82 SVG masters, authenticate successfully.
  No import/cache or index changes occur; scratch and the one-off helper are
  removed. This accepts presentation inputs, not a complete fresh MCC import.
  [SSD presentation preflight](../.local/mcc-movie-clone-repair-20261019/presentation-preflight.json).
- [x] Track both bundled movies with Git LFS and both manifests in normal Git;
  verify small index pointers and authentic local payloads in both source
  checkouts so the required movie bank can be pushed and fetched by clones.
  Both ROOT and SSD checkouts stage exactly the two MP4s, bank manifest,
  `.gitattributes`, `.gitignore` and release-movie instructions. Each MP4 index
  blob is a 135-byte LFS pointer with the manifest's exact SHA/size; all four
  local LFS payload objects pass full hashes. The already tracked outer
  manifest stays unchanged. Local hooks/filters are installed without altering
  unrelated index entries or non-LFS config. No commit or remote push occurs.
  Retain 2,538,778,741 bytes of required LFS payloads per checkout, in addition
  to the working assets; runner scratch is removed.
  [LFS staging verification](../.local/mcc-movie-lfs-20261019/validation.json).
- [ ] Publish the compiled portable V23 ZIP with both real movies, paired
  manifests and the authenticated custom prompts. The first V3 attempt stopped
  before native execution when replacing an inherited read-only helper file;
  no ZIP was published and scratch is removed. Its RED receipt is retained.
  V4 replaces only owned extracted helper files, preserving frozen inputs.
  The actual read-only failure/replacement control and four publication controls
  pass, with independent source review. Actual relocated native/Tk acceptance
  and verified ZIP publication remain open.
  [V3 failure closure](../.local/mcc-frontend-custom-prompts-20261019/delivery-v23-preparation-v3/failure-closure.json),
  [V4 controls](../.local/mcc-frontend-custom-prompts-20261019/delivery-v23-preparation-v4/publication-validation.json).
- [x] Adopt the exact tested runtime after that reviewed source drift. V3
  reconstructs the same native SHA and unchanged 97-shader bank, retains the
  actual V2 native acceptance, and passes fresh current-UI normal/title-first
  Tk Play routes with sound. Normal Play uses the cached intro and a real
  Settings B/BACK click; title-first skips the intro. Original runtime, saves,
  cache, selectors and renderer workbench remain intact. Root verified the
  inventory, viewed the fresh Settings image and confirmed scratch removal.
  Retain the requested staged runtime/bank/sidecars (107,043,373 bytes) and
  required reference PNGs (316,781 bytes). Final root launcher replacement is
  authorized separately after this completed acceptance milestone.
  [Accepted V3 adoption](../.local/mcc-frontend-custom-prompts-20261019/root-play-installation-v3/staged-validation.json).
- [x] Qualify the final launch-test lifecycle before running that integration.
  The shared flow inherits the test runner's process group and isolates HOME,
  XDG, saves, GPU caches and raw audio. Exact descendant cleanup and forced
  owner SIGKILL controls pass: traced children and Xvfb stop, and all scratch
  is removed. The source routing review also confirms default title screen 9,
  normal intro playback, title-first intro skip and separate Lockout admission.
  These checks prepare actual Play acceptance; they do not claim installation.
  [Flow lifecycle controls](../.local/mcc-frontend-custom-prompts-20261019/root-play-installation-v1/lifecycle-validation.json),
  [Independent launcher routing review](../.local/mcc-frontend-custom-prompts-20261019/menu-expected-geometry-v1/root-play-routing-review.json).
- [x] Recheck concurrent launcher selector changes before installing Play.
  The current optional frontend selector preserves single-runtime admission
  when no independent selection exists and rejects invalid explicit owners.
  Four focused CMake groups pass: frontend capability, selector, compiled
  launcher routing and Tk home UI. Source hashes remain stable and scratch is
  removed. An initial validation command included the default engine dependency;
  it was cancelled and cleaned, then corrected to disable that dependency.
  Refresh the small compiled launchers to these matching helper semantics;
  no native engine rebuild or new frontend registry is required.
  [Current launcher checks](../.local/mcc-frontend-custom-prompts-20261019/concurrent-route-check-v3/root-closure.json).
- [x] Refresh the small root and portable launcher inputs to the current
  selector-aware source. All four compile and configuration checks pass with
  pinned source/compiler/headers; prior inputs and concurrent source edits are
  preserved. The two root binaries total 7,342,200 bytes. Private compilation
  scratch is removed. Actual title/menu adoption uses these V2 inputs and the
  accepted V23 native runtime; it remains the next acceptance gate.
  [Current root launcher inputs](../.local/mcc-frontend-custom-prompts-20261019/root-play-launchers-v2/handoff.json).
  The designated Linux output folder now also contains these two current
  launcher binaries; the V23 native and onboarding output hashes are preserved.
  All four root/portable launcher inputs total 14,675,568 bytes; compile and
  restaging scratch are removed.
  [Current input/output closure](../.local/mcc-frontend-custom-prompts-20261019/root-play-launchers-v2/closure.json).
- [x] Prepare an isolated local shader-bank binding for that launcher runtime.
  The existing ELF relocation helper can now bind a canonical absolute owner
  within the same read-only, fixed-width path field, preserving instructions
  and shader files. This avoids replacing the active workbench's shader bank.
  All three focused relocation/gameplay/frontend admission groups pass; both
  private builds are removed. This is preparation only: no native runtime has
  been relocated or installed by this helper check.
  [Local binding checks](../.local/mcc-local-frontend-owner-20261019/checks-v2/report.json).
  The host's 156 linked libraries also match the accepted native test library
  bytes exactly; no extra loader bundle is needed for this machine. That
  read-only check leaves no scratch and does not substitute for actual Play.
  [Host dependency identities](../.local/mcc-local-frontend-owner-20261019/host-dependencies.json).
- [x] Compile the two root launcher inputs for that isolated runtime owner.
  Normal and title-first binaries use the current verified routing source,
  host Python and `.local/f22`; their configuration checks pass. Retain
  7,224,640 bytes of requested compiled launcher inputs plus frozen source.
  Scratch is removed and installed launchers/runtime/selection remain intact.
  Actual Play adoption still waits for corrected native menu prompt acceptance.
  [Compiled root launcher inputs](../.local/mcc-frontend-custom-prompts-20261019/root-play-launchers-v1/handoff.json).
- [x] Refresh the designated local Linux x86_64 outputs with the accepted V23
  runtime and both root launcher inputs. The existing staging helper publishes
  exact source hashes in `dist/builds/linux-x86_64/build-output.json`; the three
  binaries total 112,860,152 bytes. The onboarding entry and other platform
  outputs are preserved. Scratch is removed. These are local compiled outputs;
  installed Play adoption and the relocatable ZIP remain separate gates.
  [V23 local output receipt](../.local/mcc-frontend-custom-prompts-20261019/designated-output-v23/handoff.json).

- [x] Load the title screen, menu graphics, text/fonts and audio from the
  port-owned imported MCC cache. Reuse existing frontend code and shaders;
  do not require a separate Vista asset bank or read an Xbox ISO at runtime.
- [x] Locate and authenticate the existing authored SVG button-prompt masters
  and exported bitmaps, including their selector/codepoint provenance. The
  earlier82-master/331-binding/six-atlas pack is described in
  [VISTA_PROMPTS_4K.md](VISTA_PROMPTS_4K.md). Its preserved relocated resources
  match the original174 artwork files and175 sealed bundled nonreference
  files. The reconstruction metadata differs only by the documented bundler
  path rewrite and matches that sealed bundle. All12 MCC font selector names
  match the authored selectors exactly. The runtime manifest/six atlases total
  19,354,278bytes; no original fonts or regeneration are needed. All discovery
  scratch is removed, with no asset copies made by that audit.
  [Authenticated custom prompt inventory](../.local/mcc-frontend-custom-prompts-20261019/inventory-v1.json).
- [x] Restore the exact authored source bundle to portable
  `resources/presentation/prompts/`. Its91 files retain19,865,846bytes: the
  seven runtime files,82 SVG masters, reconstruction provenance and the new
  sealed source manifest4255eceb. All90 payload files authenticate against the
  delivered bank; no reference fonts are copied or artwork regenerated. These
  are required retained custom assets. The selected cache is untouched by this
  source restoration; import and actual display still need separate checks.
  [Custom artwork restoration](../.local/mcc-frontend-custom-prompts-20261019/source-restoration-v1.json).
- [ ] Import those custom prompt bitmaps and ownership metadata into the
  selected cache's port-supplied presentation tree, preserve the MCC tree,
  and include the required authored pack in the compiled distribution.
- [x] Repair the actual selected root cache's authored prompt bank. All nine
  files/19,372,296bytes match their complete source hashes, source manifest
  4255eceb and self-relative binding. Only `port_authored_prompts` is added to
  provenance; cached/live MCC tree metadata, both movie identities, selected
  runtimes/launchers, shaders, registries and saves remain unchanged. Actual
  traced writes stay in the prompt bank, provenance update and shared lock.
  No31GB rehash/reimport or P-cache copy occurs. Scratch is removed; these nine
  required cache assets are explicitly retained. Native rendering follows.
  [Actual prompt import](../.local/mcc-frontend-custom-prompts-20261019/actual-root-import-v1/validation.json).
- [x] Implement and verify the fresh/reused prompt import contract. The
  source bank has82 exact SVG masters plus authored atlases and provenance;
  only nine runtime files enter `presentation/prompts/`, using a portable
  self-relative binding. All53 focused prompt/onboarding/preparation controls
  pass. An actual authenticated bank copy into owned scratch verifies all
  19,372,296bytes against source/destination hashes. The importer shares the
  presentation lock, resumes matching partial copies and preserves custom or
  conflicting data. All four owned scratch trees are removed. Actual selected
  cache repair, distribution and visible rendering remain separate gates.
  [Prompt importer handoff](../.local/mcc-frontend-custom-prompts-20261019/handoff-v1.json),
  [Actual private bank copy](../.local/mcc-frontend-custom-prompts-20261019/validation-real-bank-v1.json).
- [ ] Apply the custom prompts in the native title/menus and gameplay HUD
  through the existing sampling path, preserving source glyph metrics and
  active keyboard/controller bindings. Verify actual selected atlas pixels,
  then visible menu/gameplay prompts using the MCC cache; record any remaining
  coverage limits before publishing the frontend capability.
- [x] Qualify actual MCC glyph ownership before applying the earlier redraws.
  All331 selector/name/codepoint/source-record bindings match MCC. Of those,
  178 have different raw dimensions across eight selectors; for example,
  Conduit12 E100 is44x50 in MCC versus the historical32x36 source and128x144
  custom raster. The pack's fourfold density is relative to its authenticated
  historical source, rather than every raw MCC glyph. A too-strict test caught
  that assumption; no artwork or logical metrics were changed. Revised component
  checks must verify historical density, exact bitmap samples and unchanged MCC
  metrics/lookup/clipping. Native display remains an open gate.
  [Measured prompt dimensions](../.local/mcc-frontend-custom-prompts-20261019/cpp-integration-v1/actual-dimensions.json).
- [x] Independently review the frozen prompt consumer changes. The default
  cache bank is selected independently of optional UI/font replacement;
  relative bindings resolve from their bank, and the sampler retains logical
  metrics and input bindings. Repeated Conduit selectors share identical
  atlas hashes, rectangles and vectors, so existing material deduplication
  remains valid. The new native button branch reads MCC fonts, with no new
  Vista font dependency. This is source/artifact review, not a rendered proof;
  its scratch is removed and no additional decoding or compile is performed.
  [Independent prompt consumer review](../.local/mcc-frontend-custom-prompts-20261019/independent-cpp-review-v1/review.json).
- [x] Validate the compatible prompt sampler components against actual MCC
  fonts and the authenticated custom rasters. All331 controls across12 raw
  selectors and three projected HUD owners preserve metrics, lookup, kerning
  and34,152,448 ordinary atlas bytes while selecting exact custom bitmap bytes.
  Relative/absolute bindings, changed CWD, aliases and malformed inputs are
  covered. Regular and ASan/UBSan probes pass. All five focused CMake groups
  pass, including the three fresh/reused import registrations. Scratch is
  removed and no large test outputs are retained. Actual native consumer
  constructors, button rectangles/X-height and GPU display remain separate.
  The selected-compatible HUD adoption must exclude only the unrelated
  pre-existing `reset_lifetimes` method privately, preserving product source.
  [Prompt component checkpoint](../.local/mcc-frontend-custom-prompts-20261019/cpp-integration-v1/handoff.json).
- [x] Construct actual selected-compatible MCC prompt consumers on CPU. The
  default Painter, full41-widget/624-material HUD and Scoreboard select exact
  custom textures/UVs from a private full056 cache view. Parent/patched menu
  vertex positions and title/main/Settings pointer/back behavior match; both
  processes exit0. Four controlled action-renderer inputs use the real
  `training_reload` row, rather than an actual SMG pickup. The real stock SMG
  row is exercised separately and has the same pre-existing token gap in both
  versions. All324 parent inputs and cache metadata remain exact; private
  compilers/binaries/cache views are removed. GPU display is still unverified.
  [Native CPU consumer checkpoint](../.local/mcc-frontend-custom-prompts-20261019/selected-consumers-v1/handoff.json).
- [x] Run the native title/menu path with imported custom prompt atlases and
  the cached bundled intro. The frozen 325-input candidate exits cleanly after
  actual X11 focus, intro skip, title/profile/keyboard/main/Settings/back input
  and 259 rendered frames. Title-only music and overall native mixer samples
  are finite and nonzero. The 97 shaders and parent inputs stay unchanged;
  owned runtime scratch is removed. This validates the basic frontend only:
  dedicated gameplay control metrics, stock pickup tokens, complete menus and
  Xbox framebuffer/audio parity remain separate. Cold window readiness took
  80.9 seconds, so this is not a startup-performance fix. Final video review
  and retention are recorded below; launcher/package adoption remains separate.
  [Native custom-prompt frontend run](../.local/mcc-frontend-custom-prompts-20261019/native-frontend-v1/validation.json).
- [x] Seal the basic frontend candidate and requested video with sound. The
  final cohort pins the immutable passed receipt; independent root checks
  verify all handoff hashes and that acceptance link. The compatible 20-second
  H.264/AAC video has 240 frames and finite nonzero stereo audio. Visual review
  confirms the bundled intro, title, profile chooser and main menu; Settings is
  proven by the input trace. Retain 125,604,679 bytes of coherent compiled inputs
  and 3,053,915 bytes of requested video/source visual evidence. Three owned
  capture/QA scratch trees are removed. No capability, installation or ZIP is
  published by this checkpoint.
  [Sealed frontend/video handoff](../.local/mcc-frontend-custom-prompts-20261019/native-frontend-v1/handoff.json),
  [Independent closure](../.local/mcc-frontend-custom-prompts-20261019/native-frontend-v1/root-closure.json).
- [x] Build and test the dedicated-control metric helper against actual MCC
  font inputs. The focused CMake test passes the 40-row profile, selector 5
  no-op, selector 6 corrections, idempotence and malformed/partial-owner guards,
  while preserving ordinary metrics, samples, atlas pixels, lookup and kerning.
  The helper is called only for private dedicated action-button fonts after
  authored sampling. Its private build is removed. Selected-compatible HUD
  geometry and native adoption remain unchecked.
  [Metric component validation](../.local/mcc-frontend-custom-prompts-20261019/geometry-implementation-v1/regular/report.json).
- [x] Preserve the prior authored dedicated-button geometry with an exact
  source-qualified logical-metric projection where MCC differs. The helper
  qualifies all 40 control records and authored samples before changing the
  16 differing selector 6 tuples; selector 5 and repeated projection are no-ops.
  Six selected-compatible MCC Cache/HUD cases confirm controller X, F and Space
  geometry against the independent source profile. Ordinary fonts, uploaded
  texture bytes, active input policy and shaders remain unchanged; 27 malformed
  or partial-state controls reject before mutation. No reference fonts ship.
  All owned scratch is removed, with no probe binaries retained. This is CPU
  geometry acceptance; native cohort adoption, visible gameplay prompts and
  Xbox framebuffer parity remain separate gates.
  [Dedicated HUD metric handoff](../.local/mcc-frontend-custom-prompts-20261019/geometry-implementation-v1/handoff.json).
- [x] Adopt the exact helper and HUD provider together in a coherent native
  runtime. V22 passes actual title/profile/main/Settings/back with unmuted audio,
  and real Lockout focus/click/held-fire/look/movement through 319 frames and
  366 ticks with a clean exit. All 325 parent inputs, 97 shaders and selected
  cache are preserved. Retain 114,415,463 bytes of coherent compiled inputs;
  test scratch is removed. The independently measured menu-control sizing
  issue still blocks prompt acceptance and Play/capability/package publication.
  [Native HUD adoption](../.local/mcc-frontend-custom-prompts-20261019/native-geometry-v1/handoff.json).
- [x] Extract and qualify the dedicated-control metric profile before changing
  native sizing. All 40 control records match the authenticated artwork
  owners. Selector 5 needs no metric changes; selector 6 has 16 differing
  tuples. The 95 printable ASCII glyphs and complete kerning tables match
  both historical owners, so a control-only projection can preserve the
  existing keyboard-label sizing policy. This is offline metadata evidence,
  with no product edits or reference fonts shipped. Implementation and native
  rendering remain unchecked; owned scratch is removed.
  [Qualified metric profile](../.local/mcc-frontend-custom-prompts-20261019/geometry-profile-v1/handoff.json).
- [ ] Normalize actual MCC gameplay action templates into the original
  native token namespace using verified names/weapon-field roles/raw hashes.
  The five same-named SMG pickup/dual templates are paired with original
  Lockout and TU1.5 Backwash. Original primary/secondary use E448/E449; MCC
  templates use E47C/E45A and E47D/E46A. Original E45A means an animated right
  thumbstick, so global codepoint replacement would conflate namespaces.
  General MCC callback semantics and other weapons/languages remain open.
  Keep this gameplay compatibility gate separate from basic menu availability.
  [Qualified action-template evidence](../.local/mcc-action-locale-token-audit-20261019/comparison-v1.json).
- [x] Require the custom prompt bank in frontend capability admission. All
  nine consumed manifest/binding/atlas inputs and any extra bank files must
  match the passed inventory under `presentation/prompts/`; omitted, changed,
  deleted and unqualified extra atlas controls reject. Seven frontend controls,
  all34 compiled launcher routing controls and22 updater controls pass through
  three focused CMake groups. Scratch is removed. These metadata checks do not
  prove actual prompt import or visible rendering.
  [Custom prompt admission checks](../.local/mcc-frontend-launcher-20261019/custom-prompt-resource-v7/closure.json).
  An independent current-cache size control also passes unchanged:52 consumed
  resources plus97 shaders need about15–16KiB, within the64KiB sidecar bound.
  The copied561 DDS pages remain inactive without `ui/pack.tsv`; the launcher
  already guards that optional overlay correctly. A hypothetical oversized
  future pack does not justify changing the current bound. Both audit scratch
  trees are removed.
  [Current receipt-size check](../.local/mcc-frontend-custom-prompts-20261019/sidecar-size-review-v2/review.json).
- [ ] Validate complete fresh and reused-cache imports before changing the
  final data-path layout. Cover copied MCC files, required converted vanilla
  gameplay tags, mod `tags`/`data` inputs and bundled custom presentation assets,
  including interruption/resume and actual native consumption. A helper fixture
  alone does not pass this import gate.
- [x] Audit current path preservation before proposing that later change.
  Raw copying already preserves selected Halo2-relative names, case, nested
  `tags`/`data`/mod subtrees and empty directories under `cache/halo2`.
  Current offline map conversion only enumerates top-level `.map` files;
  copied nested mod maps/loose tags and mod-owned shared-resource contexts need
  separate conversion/lookup coverage. External Workshop roots are not yet
  enumerated. Derived hash-named bitmap/UI catalogues are separate port caches,
  rather than a flattened replacement for the donor tree. This read-only audit
  changes no data paths and leaves the full import gate open; scratch is removed.
  [Canonical path audit](../.local/mcc-canonical-path-audit-20261019/read-only-v1/validation.json).
- [x] Identify the remaining complete-import acceptance boundaries. Current
  cache metadata covers 883 copied files and 43 maps, with no nested mod maps
  or loose tag inputs. Small controlled copies preserve nested/case-sensitive
  paths, but conversion still uses basenames and one standard resource context.
  Reuse verification authenticates copied outputs without proving a complete
  required-conversion set. This read-only audit does not rehash the real cache
  or run gameplay; owned scratch is removed and no data paths change.
  [Import acceptance audit](../.local/mcc-import-gate-audit-20261019/read-only-v1/review.json).
- [x] Make the CLI/main-thread fresh-import transaction cancellation-safe,
  including stage creation, copying, map conversion and nested presentation
  preparation. Actual SIGTERM controls now remove incomplete stages, preserve
  source/registries/prior caches and permit a clean retry. A completed published
  cache survives termination after its rename. All 21 preparation controls and
  four focused CMake groups pass; source pins remain stable across validation.
  Seven owned scratch trees are removed and no large artifacts are retained.
  Concurrent multiplayer-catalog work is preserved but is outside this fix's
  acceptance scope. The GUI worker lifetime is a separate unchecked gate.
  [CLI cancellation handoff](../.local/mcc-import-cancellation-20261019/handoff.json).
- [x] Give Linux GUI onboarding an equivalent cancellation/cleanup owner.
  Default imports now run in an owned CLI child with bounded streamed progress;
  its main-thread cancellation covers setup/select, including reuse repairs.
  Nineteen focused controls pass: real GUI termination and owner-pipe EOF,
  interruption during process creation, callback/window teardown, clean retry,
  portable Python paths and exact handler restoration. Existing controls and
  injected-function tests remain valid. Both owned scratch trees are removed;
  no real cache, registry or runtime changes occur in these tiny fixtures.
  Complete fresh/reused import and packaged GUI acceptance remain open.
  [GUI import lifetime handoff](../.local/mcc-setup-gui-lifetime-20261019/validation-v1/handoff.json).
- [ ] Qualify relative map identities and each map's shared, texture and audio
  dependencies before widening conversion enumeration. Cover duplicate mod
  basenames and nested paths without conflating resource owners. Define required
  conversion coverage and loose-tag/data producer admission; use that coverage
  in fresh/reused verification and actual native-consumption acceptance.
- [ ] After that import gate passes, update importer output and runtime lookup
  paths to follow Halo2 MCC's relative directory structure. Use MCC's pattern
  as the canonical layout for this MCC-only port, preserving original internal
  tag names, relative `tags`/`data` paths, case and mod-folder subtrees. Convert
  required definitions before publishing them at those normal vanilla paths;
  keep provenance separate from logical tag ownership. Preserve Xbox-compatible
  logical tag paths where shared, without adding an ISO import dependency.
  Compiled maps retain their donor-relative `.map` paths and internal tag
  names; loose assets retain their donor-relative `tags/` and `data/` paths.
  Existing working caches need a verified migration or compatibility lookup.
- [ ] Verify path-equivalent native loading and mod-path resolution after the
  layout update. Record representative internal tags and loose/mod `tags` and
  `data` paths, confirming converted definitions are the vanilla baseline.
  This prepares the data layout for future Steam Workshop overlays; Workshop
  discovery, downloads and subscription support remain a later implementation.
- [x] Copy the actual bundled repo intro and credits movies during cache
  import and bind their verified playback roles. Record them as port-supplied
  movie inputs, preserve MCC's copied BK2 tree and the bundled originals, and
  avoid assigning intro/credits roles to unverified MCC video filenames. Verify
  both fresh import and an existing-cache handoff without repeating31GB import.
  Fresh/reused importer controls, actual selected-cache copy and current C++
  cache-role/first-frame decoding now pass as recorded below.
- [x] Implement the fresh/reused-import and no-reimport movie handoff. The
  exact two source MP4 hashes verify2,538,778,741bytes. Four focused CMake
  groups and12 Python controls pass, including interrupted-copy cleanup.
  Runtime resolution selects the pinned cache manifest, exact roles and file
  extents; it streams video without a2.5GB startup payload hash. Full payload
  verification remains in import/repair. The bundled sources are unchanged;
  test scratch is removed. Actual selected-cache copy/playback follows.
  The self-contained next package must include the two bundled movies and
  metadata; the older portable ZIP does not acquire them from a dev checkout.
  [Bundled movie handoff](../.local/mcc-bundled-movies-20261019/handoff.json).
- [x] Preserve both required movie metadata paths in CMake build and install
  outputs. The movie target previously emitted only the bank manifest; it now
  also emits and installs `resources/movie-manifest.json`. Bounded single- and
  multi-configuration CMake builds/installations pass, and each installed
  four-file bundle supports fresh and reused movie preparation. These use tiny
  fixtures to verify packaging and importer compatibility, rather than claiming
  movie playback or a complete engine build. All five test/fixture scratch
  trees are removed; no movie copies or binaries are retained by this check.
  [Movie install validation](../.local/mcc-movie-cmake-install-20261019/v2/validation.json),
  [Cleanup](../.local/mcc-movie-cmake-install-20261019/v2/cleanup.json).
- [x] Prepare the exact successor portable-package input inventory without
  publishing it. The V18 delta needs the updated six existing Python tools,
  new movie helper, both MP4s and both metadata files, plus separately accepted
  runtime/capability evidence. Portable launchers must be compiled with
  `python3`/`./bin`, rather than copying absolute-path development launchers.
  ZIP64 is required; movie assets belong in the full ZIP rather than the
  binaries-only updater feed. This is a pinned preparation, not a new package
  or native frontend acceptance. No archive or large copy is made.
  [Portable input preparation](../.local/mcc-bundled-movies-20261019/portable-preparation-v1/preparation.json).
- [x] Make the public compiled-preview packager carry both bundled movies
  and both exact metadata files, verify complete source/copied hashes, record
  provenance and explicitly enable ZIP64. Sixteen focused controls pass,
  including real tiny archive writing, invalid sources, interruption cleanup
  and no-replace publication. Only the final requested ZIP persists during
  real packaging; private stage copies are removed. These were synthetic
  fixtures, with no new release ZIP or large movie copy. Source pins and all
  private scratch removal are recorded. Custom prompt packaging follows
  separately; this does not publish the accepted native frontend.
  [Public movie packager closure](../.local/mcc-bundled-movies-20261019/public-packager-v1/validation.json),
  [Packager cleanup](../.local/mcc-bundled-movies-20261019/public-packager-v1/cleanup.json).
- [x] Extend the public packager to include all91 authored prompt source
  files alongside both movies and metadata. Full prompt hashes are verified
  before staging and after copying, with source provenance in package metadata;
  no reference fonts are bundled for this bank. All21 focused controls pass,
  including combined synthetic archive contents and existing ZIP64/cancellation
  behavior. Scratch and fixture ZIPs are removed. This updates the packager,
  rather than producing the next real compiled distribution.
  [Prompt package closure](../.local/mcc-bundled-movies-20261019/public-prompt-packager-v1/validation.json),
  [Prompt packager cleanup](../.local/mcc-bundled-movies-20261019/public-prompt-packager-v1/cleanup.json).
- [x] Copy both bundled movies into the actual selected
  `mcc-056309e12f444a3a8992c94f6fe32f0b/presentation/movies/` bank and verify
  complete source/destination SHA-256 values. The only provenance change is
  `port_bundled_movies`; MCC tree metadata, selection, current workbench/95
  shaders and saves remain exact. No MCC tree is recopied. Retained requested
  data is2,538,778,741bytes plus the721-byte manifest and empty lock. The
  6.73-second repair's actual writes stay within that bank, lock and provenance;
  its private scratch is removed. Cached playback decode follows separately.
  [Actual selected-cache movie import](../.local/mcc-bundled-movies-20261019/cache-copy-v1/validation.json).
- [x] Resolve both actual cached movie roles through current C++ and decode
  their video/audio. Each produces a3840x2160 frame; the standalone decoder
  also checks three frames per role and finite nonzero48kHz stereo audio.
  Intro/credits report866,624/18,212,224 decoded audio frames. Attract remains
  unassigned. Private compilers, frames/audio buffers and all four recorded
  scratch trees are removed. This is bounded decode, not complete playback,
  speaker output or a displayed full video.
  [Movie copy/decode closure](../.local/mcc-bundled-movies-20261019/cache-copy-v1/closure.json).
  The prospective frontend's exact compatible Movie provider also resolves the
  selected cache bank first and decodes each role through frame3. Its existing
  audio decoder produces866,624/18,212,224 finite/nonzero frames with peaks
  0.91483/0.737033. Attract stays empty. This additional provider check runs in
  the owning frontend test's private hardlink view. That full test now passes
  and its scratch is removed; custom prompt/launcher admission remain separate.
  [Prospective provider movie check](../.local/mcc-frontend-runtime-20261019/unmuted-xinput-v2/movie-role-validation.json).
- [x] Keep the launcher's normal frontend route eligible for the bundled
  startup intro, while the title-first variant explicitly skips it. The
  relocated onboarding launcher passes all34 routing tests with this split.
  Movie import and actual decode/playback acceptance remain separate. Its
  private build is removed.
  [Intro/title route check](../.local/mcc-frontend-launcher-20261019/bundled-intro-routing-v5/report.json).
- [x] Audit the actual imported frontend inputs: both prepared caches retain
  all40 MCC fonts and their12-selector table; the selected menu contains131
  screens and275 UI bitmap banks/929 frames. The newer cache also has561
  decoded DDS assets. All57 copied BK2 files remain available, but classic
  intro/attract role bindings are not established by their presence. No
  reimport or separate Vista bank is needed for the title/font inputs.
  [Imported asset audit](../.local/mcc-frontend-asset-contract-20261019/read-only-v1/validation.json).
- [x] Get the compatible native frontend through MCC metadata/image decoding
  and into a real title frame using the full imported-cache hierarchy.
  Preserve three authored empty hidden MCC lists, use the existing MCC bitmap
  reader, load the authenticated MCC font path and read animation bytes from
  their checked owned graph. The original-file raw-data guard remains intact.
  The640x480 title frame reveals cropped PC-canvas artwork; layout/navigation
  and unmuted acceptance remain open. Both audited runs open no Xbox/Vista
  reference files, preserve97 shaders and remove scratch. The153,319-byte PNG
  is retained as required comparison evidence, not an accepted title image.
  [Metadata acceptance](../.local/mcc-frontend-runtime-20261019/content-fix-v3/validation.json),
  [Initial title frame](../.local/mcc-frontend-runtime-20261019/title-frame-v3/validation.json).
- [ ] Validate title-to-main-menu, submenu navigation and return/back through
  the selected runtime and launcher using that MCC cache. Record the exact
  imported cache and opened files, rejecting Xbox/Vista cache dependencies.
  The first bounded V19 MCC menu audit stopped with
  `invalid UI list definition`. Metadata, a first title frame and the later
  CPU navigation stage now pass as recorded below. Corrected muted rendered
  navigation also passes; the launcher and unmuted/input gates remain open.
  [Initial MCC-only audit](../.local/mcc-frontend-runtime-20261019/initial-v1/validation.json).
- [x] Verify the corrected native MCC title/profile/main/Settings/back flow
  with four actual640x480 rendered cases. Creating the private profile through
  keyboard167 reaches main6; selecting Settings at319,357 reaches19; Back
  returns through confirmation7 to title9. All four processes exit0 and open
  no Xbox/Vista reference files. The existing97 shaders are unchanged. Four
  required comparison PNGs total550,453bytes; raw frames, linked test binaries
  and scratch are removed. These muted cases do not prove unmuted startup,
  physical desktop input, every submenu or Xbox framebuffer parity.
  The compatible link uses322 physical inputs: the316-input parent, one
  replaced frontend input and seven explicit overrides; the original archive
  remains intact with member-ownership witnesses.
  [Rendered navigation acceptance](../.local/mcc-frontend-runtime-20261019/navigation-v3/acceptance-summary.json).
- [x] Identify the profile-picker navigation failure by exact identifier names.
  A bounded offline original name-table reference and the imported MCC table
  give6,873 unique common names, three explicitly absent names and no ambiguous
  matches. All requested navigation/keyboard IDs are corroborated by actual
  MCC locale ID words. Independent reconstruction verifies every generated
  row; for example, `create_new_profile` maps12000195 to1200019d. Runtime
  inputs remain MCC-only. Localized text and authored MCC IDs are preserved;
  only compiled original identifier lookups may use this qualified mapping.
  Product integration/navigation acceptance remains open; scratch is removed.
  [Named identifier inventory](../.local/mcc-frontend-string-inventory-20261019/common-unique-v1/validation.json),
  [Independent mapping review](../.local/mcc-frontend-string-inventory-20261019/common-unique-v1/root-review.json).
- [ ] Make default Play open the validated MCC frontend, while preserving a
  separate native gameplay test and honest incomplete full-game readiness.
- [x] Resolve the actual unmuted frontend failure before publishing that
  capability. The first eight-leaf/323-input candidate compiles and links but
  aborts before `Menu ready`, after a wmav2 warning and stack-smashing report.
  No real OS input, accepted candidate, video or sidecar is produced. Its
  scratch is removed; muted four-screen acceptance remains valid. Diagnose the
  exact software decoder/provider contract; no hardware initializer work is
  required without demonstrated dependency.
  [Unmuted failure](../.local/mcc-frontend-runtime-20261019/unmuted-xinput-v1/validation.json).
  The matching-header caller refresh now passes the full actual unmuted native
  X11 test: title9/profile10/keyboard167/profile38/main6, real pointer selection
  to Settings19 and Back6,482 rendered frames and exit0 in70.57 seconds.
  Actual SDL output contains848,384 finite/nonzero stereo frames at48kHz,
  peak0.362493. The sole opened music/UI bank is imported MCC
  `sounds_neutral.dat`; no Xbox/Vista runtime inputs are opened. All97 shaders,
  original316 inputs, user saves and runtime selections remain unchanged.
  The accepted324-input candidate and required coherent inputs retain
  150,790,930bytes. Its requested12-second video with native audio is1,647,535
  bytes. Raw captures/PCM/maps and private scratch are removed. Custom prompt
  integration, launcher admission and full parity remain separate open gates.
  [Unmuted/input acceptance](../.local/mcc-frontend-runtime-20261019/unmuted-xinput-v2/validation.json),
  [Coherent candidate retention](../.local/mcc-frontend-runtime-20261019/unmuted-xinput-v2/retention.json).
  The video independently decodes as144 H264640x480 frames and576,000 finite,
  nonzero stereo audio frames. It starts at profile entry, then shows the main
  menu and Settings transition; it does not include the accepted title frame.
  A212,855-byte contact image is retained for video QA, whose scratch is removed.
  The final custom-prompt capture must wait for recording readiness before input.
  [Video QA](../.local/mcc-frontend-runtime-20261019/unmuted-xinput-v2/video-qa.json).
- [x] Minimize the unmuted failure to the actual MCC MenuMusic constructor
  and identify a selected caller/producer layout mismatch. The old music
  caller allocates280 bytes for `SoundVariationDefinition`; the selected
  variation producer returns288 bytes, including `sample_rate_index` at280.
  Actual DWARF and the failing lambda's stack-canary epilogue corroborate the
  boundary. Both callers agree on88-byte `PcmSound`; the packaged channel-layout
  functions touch only the expected24 bytes. This diagnoses a concrete ABI
  mismatch, not a passed fix. Refresh only the unchanged music caller against
  the accepted header, then repeat the tight CPU and unmuted frontend tests.
  All three independent ABI probe scratch trees are removed.
  [Selected sound variation layouts](../.local/mcc-frontend-audio-abi-20261019/sound-variation-types-v1.json),
  [Actual minimized failure](../.local/mcc-frontend-runtime-20261019/audio-cpu-v3/validation.json).
- [x] Verify the narrow MenuMusic caller refresh against the actual MCC audio.
  Compiling the unchanged authenticated source against the selected288-byte
  header changes the minimized constructor from abort to exit0 in1.285 seconds.
  All12 UI sounds and four authored WMA loops decode finite/nonzero at48kHz;
  MenuMusic plays one voice through the existing mixer for48,000 frames, with
  peak0.047278. The companion debugger also exits normally. No music algorithm,
  decoder, gameplay, cache, shader or save bytes change; original316 link inputs
  remain exact. The private compiler, objects and PCM buffers are removed.
  Actual unmuted native/window acceptance follows as a separate gate.
  [Caller refresh acceptance](../.local/mcc-frontend-runtime-20261019/audio-cpu-v4/acceptance-summary.json).
  A later narrow DWARF closure verifies the other selected by-value consumers:
  multiplayer audio, the native provider, script-world access and Cache all
  agree on288 bytes, appended field offset280 and the same ten field offsets.
  Their exact four cohort object hashes match. No additional mismatch or
  rebuild is needed; audit scratch is removed.
  [Remaining sound caller layouts](../.local/mcc-frontend-audio-abi-20261019/v20-caller-closure-v1/review.json).
- [x] Bind frontend admission to the actual classic audio bank as well as
  mainmenu/shared/textures/fonts. An independent metadata control reproduced
  admission after an unpinned `sounds_neutral.dat` was changed/deleted. The
  reader now requires its exact hash; omission, mutation and missing-bank
  controls reject, while a complete receipt still admits. All34 compiled
  launcher routing controls, six frontend controls and22 updater controls pass
  through three focused CMake groups. Its private build is removed; no game
  binaries or raw data are retained by these checks.
  [Independent audio-bank finding](../.local/mcc-frontend-launcher-20261019/independent-resource-review-v1/review.json),
  [Audio-resource admission checks](../.local/mcc-frontend-launcher-20261019/audio-resource-routing-v6/closure.json).
  The independent exact control also passes: correct bank admits; omitted hash,
  changed bank and deleted bank reject; full restoration readmits. Its own
  scratch is removed.
  [Independent post-fix control](../.local/mcc-frontend-launcher-20261019/independent-resource-review-v2/review.json).
- [x] Exercise the named-identifier and PC-canvas integration through actual
  MCC menu state and pointer hit testing: title9 to profile10, keyboard167,
  profile38 and main6 succeeds; each of the five main entries owns3,840 pixels
  in its distinct hit region. The actual Settings hit at319,357 opens19.
  This CPU stage uses the imported MCC cache. Its owning run still has GPU
  cases in progress; no frontend sidecar or accepted binary is published from
  this stage alone. Final scratch cleanup follows the owning run's exit.
  [Actual MCC pointer/navigation stage](../.local/mcc-frontend-runtime-20261019/navigation-v3/hit-result.json).
- [ ] Make Update fetch the latest published prebuilt binaries for the host
  platform and replace only files requiring updates. Preserve imported content,
  shaders, saves, local source changes and the previous working runtime on
  failure. Do not implement Update as Git pull plus a source rebuild.
- [x] Compile the relocated onboarding launcher and verify binary-selection
  precedence, stale/escaping selection fallback and packaged `--update`
  routing without importing game data or compiling the native engine. The
  focused launcher routing suite passes; its private build is removed.
  This does not prove a published release or a frontend frame.
  [Launcher routing check](../.local/mcc-frontend-launcher-20261019/launcher-routing-v1/report.json).
- [ ] Verify the real update route against its actual release source and
  exercise changed/current, wrong-platform, corrupt and interrupted downloads.
  A local updater fixture alone does not prove a published build is available.
- [x] Implement official GitHub prebuilt selection, file-hash verification,
  changed-file downloads, staged activation/rollback and launcher forwarding.
  The integrated eight focused CTest groups pass, including the real Tk
  controls, separate frontend/native admission, binary updater and build entry
  point. The official latest-release endpoint returns404 with no installation
  writes; no published binary is available to accept end-to-end yet. Existing
  builds remain playable after a failed download. Both validation build trees
  are removed; no test binaries are retained.
  [Integration validation](../.local/mcc-frontend-launcher-20261019/integration-v2/report.json),
  [Updater receipt](../.local/prebuilt-update-audit-20261019/validation.json).
- [x] Pin the frontend texture bank as well as menu/shared/font inputs, and
  reject foreign or duplicate menu-map/UI overrides. The new duplicate control
  exposed the older first-value-only check before the correction. The stable
  corrected launcher passes all34 routing tests and frontend admission
  controls. Safe-path direct release-slot CLI admission also passes; changing
  its synthetic texture bank expires frontend admission. These are input and
  routing proofs, not rendered-menu acceptance. Private builds are removed.
  [Corrected routing](../.local/mcc-frontend-launcher-20261019/frontend-routing-v4/report.json).
  The independent review and22 updater/release-admission controls also pass;
  the release-slot CLI is exercised in a fresh safe-path Python process.
  [Current integration review](../.local/prebuilt-update-audit-20261019/integration-review-v1.json).
- [x] Stage successful builds into the designated platform output directory.
  Linux x86 means the current Linux x86_64 target. Reserve Linux arm64,
  Windows x86_64, Windows arm64 and macOS arm64 platform identifiers, but skip
  their builds and outputs until their support is implemented. Keep local
  build staging distinct from publishing downloadable release assets.
  A real isolated CMake integration builds four genuine ELF targets and
  verifies their distinct outputs and hashes; a second private build verifies
  staging defaults off under test scratch. Both trees and all eight binaries
  are removed. No unsupported-platform output is produced.
  [CMake staging acceptance](../.local/build-output-staging-20261019/cmake-integration-v1.json).
- [x] Compile and stage the three actual current launcher binaries into those
  designated outputs. The regular and title variants preserve the currently
  selected camera-performance workbench; onboarding keeps its normal relative
  first-run configuration. The second build includes the bundled-intro/title
  route split and current source6b55a7ad. Retained outputs total10,830,184bytes plus
  the1,184-byte manifest. The private compiler/build files are removed. Root
  launchers, the selected workbench and its95 shaders, caches and selectors
  remain unchanged. The gameplay binary will be staged after its separate
  frontend candidate acceptance; this does not install or publish a release.
  [Actual latest launcher outputs](../.local/launcher-designated-output-20261019/execution-v2/validation.json),
  [Build cleanup](../.local/launcher-designated-output-20261019/execution-v2/cleanup.json).
- [x] Compile the two separate portable release launchers from frozen
  source6b55a7ad using relative `python3`/`./bin` configuration. Both real
  ELF64 binaries pass bounded relocated configuration checks through the
  existing packaged loader, libraries and Python. Regular startup retains the
  intro; the title variant skips it. Required package inputs retain7,253,922
  bytes, including both ELFs, source and provenance. Root/P/designated builds,
  caches, saves, selectors and recursive shader inventories remain exact.
  Private build/fixture scratch is removed. No game, install, package or
  capability sidecar is produced; full extracted-package acceptance follows.
  [Portable launcher inputs](../.local/mcc-portable-launchers-20261019/inputs-v1/inputs.json),
  [Portable build cleanup](../.local/mcc-portable-launchers-20261019/build-v3/cleanup.json).
- [x] Add executable root `build-linux.sh` with clone-relative paths, quoted
  directories and configurable Python/build directory/job count. The focused
  entry-point checks cover another working directory, spaces and overrides.
  This is script/routing validation, not a fresh-clone full engine build.
  The bounded clone-input audit confirms the inspected generated renderer
  inputs have tracked sources and CMake producers, and adds the required
  `spirv-val` dependency to README. An untracked AI descriptor include belongs
  to unrelated dirty work and is preserved with its owner; the new source
  files must accompany their corresponding source changes. No clean-clone
  full engine compilation is claimed.
  [Clone input audit](../.local/build-output-staging-20261019/clone-input-audit-v1.json).


## Weapon runtime cache requirement (October19)

The weapon goal's remaining work and per-weapon completion entries live in
[WEAPONS_TU15_GOAL_CHECKLIST.md](WEAPONS_TU15_GOAL_CHECKLIST.md). Maintain that
checklist and its linked goal state when completing weapon milestones.

- [x] Require the prepared MCC cache in weapon snapshot validation. Explicit
  `--mcc-root` admission checks version13 headers, records file hashes and rejects
  ISO/Vista controls. Runtime cases use native TU1.5 startup without an Xbox
  image. The historical ISO-camera case remains reference-only. Probe scratch
  is removed; no large artifacts are retained.
  [Input admission](../analysis/weapons-tu15-goal-20261013/mcc-cache-input-admission-v1.json).
- [x] Inventory the actual prepared MCC directories against the retained Xbox
  weapon scope. All43 cache files decode, with1,479 authored weapon rows and66
  distinct paths. All63 Xbox scope paths are present. The three additional MCC
  paths are absent from all 41 owned original map/shared/menu directories;
  their resident definitions belong to both MCC shared banks. They remain
  preserved additions outside the original 63-weapon catalog. This is identity
  and scope coverage, with no
  conversion, gameplay or visual certification. Scratch is removed.
  [MCC weapon inventory](../analysis/weapons-tu15-goal-20261013/mcc-authored-weapon-scope-v1.json).
- [ ] Verify the campaign weapon/HUD and application cases against these MCC
  inputs, then complete feature and visual coverage through the real import and
  runtime path. Tag conversion belongs to import; no reference-tag substitution
  may be used to pass validation.
- [x] Verify the bounded native MCC HUD/map startup and704 shared-copy cases
  without an Xbox image constructor. The actual selected HUD address belongs to
  the resident MCC `hudg` directory. One/four-player MCC CLI simulations pass.
  The original47D6C0 operand restoration clears1E7F18 in the MCC rerun. Actual
  campaign placement next fails at1B0D0F on descriptor slot48AD78; actor/weapon
  submission and application acceptance remain open.
  Both private source/build trees and probe scratch are removed.
  [MCC startup checkpoint](../analysis/weapons-tu15-goal-20261013/checkpoint-weapon-mcc-native-startup-20261019.json).

The [weapon checklist](WEAPONS_TU15_GOAL_CHECKLIST.md) records nine passing
checks in the v9 native MCC snapshot:115-descriptor composition, actual
actor/held-weapon CPU submission, HUD/704 copies, both local-player simulations,
two retained arithmetic comparisons, node-map projection and complete canonical
metadata delivery. All three original campaign tables admit public HS placement;
the objective-locale correction clears actual startup. The RTX 5080 renderer
initializes and frame-zero simulation finishes. The actual node-map publisher
and render consumer clear `23644`/address `0x2`, and v9 also clears the PRT
`22D78`/`46A680` failure. The application reaches actual turret-item, BR and
SMG source models, then stops on MCC `17000578` (`primary_ammunition_tens`).
Its Xbox ID is `1700056C`; paired tables also expose other builtin namespace
differences. Correct name binding and the original digit producer remain open.
The current cold factory verifies all95 fixed inputs, two axis-index sites and
the complete24-format bit-depth table. The physics literal exporter preserves
the existing relocated tick-count import; its ownership regression passes.
Actual application revalidation follows with11 required checks; no MCC frame
has submitted. Completed owned source/build/probe scratch is removed, with no
new large artifacts retained. These results do not close full weapon/visual gates.

## Account usage instruction during this work

The user authorized one existing Codex usage reset at1% or less remaining.
On2026-10-19, a fresh check moved from2% remaining/two available reset credits
to100%/one credit, with the account's reset time advancing. This task did not
call `consume_usage_reset`; all three children also confirmed no reset attempt.
The account reset therefore occurred elsewhere. Treat the single-reset request
as fulfilled and do not spend another credit under it. The next fresh check at
16:26:26UTC shows99% remaining/one available credit. Null/missing usage windows
remain unavailable, never zero. No credit purchase or additional reset is
authorized. [Observed account reset](../.local/account-usage-reset-20261019/observed-reset.json).

## Immediate user-reported priority: interactive launch

- [x] Sample the installed V14 startup after the upload-hash correction. Four
  parent-owned debugger samples find MCC sound-timing relocation at4seconds,
  original combat startup memory validation at12seconds, repeated gameplay-font
  source validation at20seconds and texture bucket hashing at28seconds. These
  are bounded stack observations, not measured phase costs or normal boot timing.
  The diagnostic intentionally stops its private child; scratch is removed and
  saves, shaders, installed runtimes and host security settings remain unchanged.
  [V14 startup samples](../.local/mcc-native-click-crash-20261018/startup-profile-v3/validation.json).
- [x] Measure ordinary gameplay-font validation before adding a cache. A source
  load takes6.03ms;32 repeated complete eight-file authentications take52.69ms.
  The current API sequence for three HUD hands and scoreboard takes0.151943s,
  with175 digest calls/84,702,617bytes. This CPU-only measurement does not explain
  a multi-second boot delay or prove installed-provider cost. No persistent
  font/authentication cache is added; source mutation rejection stays unchanged.
  Disposable probe scratch is removed, with no large artifacts retained.
  [Font cost measurement](../.local/mcc-native-click-crash-20261018/font-startup-v1/validation.json).
- [x] Measure actual MCC Lockout relocation lookup with the coherent V14
  providers. All41,793 boundary/extent controls match an indexed candidate over
  6,965 admitted ranges.100,000 lookup queries take0.226813s with the existing
  scan and0.00258037s with the candidate. Whole sound-timing construction takes
  9.02226s, which is not yet attributed wholly to lookup. Index and metadata
  hashes are retained; no source/provider/map bytes change and scratch is
  removed. Product semantics review and phase comparison remain open.
  [Actual relocation profile](../.local/mcc-native-click-crash-20261018/runtime-relocation-profile-v1/validation.json).
- [x] Attribute the sound-timing startup cost with a complete paired build of
  the same plan.4,184,679 relocation calls take the existing construction from
  9.23038s to0.503806s with indexed lookup. All exposed fields of4,503 bindings,
  3,429 gaps,two owners,1,359 stored references and4,506 permutation references
  match; index/metadata hashes remain exact. This CPU phase result is not a
  native-launch guarantee. Scratch is removed; no product bytes change yet.
  [Paired timing construction](../.local/mcc-native-click-crash-20261018/runtime-relocation-profile-v2/validation.json).
- [x] Independently review sorted lookup against unchanged range admission.
  Across112,956 bounded tables,8,016 valid/104,940 rejected,960,872 comparisons
  cover empty aliases, duplicate starts, adjacent allocations, reverse mapping,
  wrap and maximum-size queries. The disjoint-owner proof passes. Product must
  sort every finalized plan and preserve validation/error behavior; generic
  relocation and reverse translation remain unchanged. Scratch is removed.
  [Independent lookup review](../.local/mcc-native-click-crash-20261018/runtime-relocation-review-v1/validation.json).
- [x] Implement and independently review the exact product lookup. Only the
  new sorted-source helper, finalization-time sort and `runtime_address`
  dispatch change. Last append225, unchanged admission228 and final sort423
  precede the sole returned plan.960,872 extracted-product controls preserve
  values, acceptance, exact errors and reverse mapping; no product consumer
  depends on relocation iteration order. Source/header layout and serialized
  widths stay unchanged. Scratch is removed; compiled/native acceptance remains
  separate and the installed V14 runtime is still unchanged.
  [Exact product review](../.local/mcc-native-click-crash-20261018/runtime-relocation-review-v2/validation.json).
- [x] Compile the exact new relocation source against ordinary current headers
  and normal cache-module flags. Compilation is warning-free in2.038s with322
  dependency pins unchanged. Inverting precisely the helper/sort/dispatch edits
  reproduces the original source hash. The2,451,704-byte object and scratch are
  removed. This verifies ordinary compilation, not a current-client ABI link,
  installed runtime or full64-bit migration.
  [Current-header compile](../.local/mcc-native-click-crash-20261018/runtime-relocation-source-v1/current-compile.json).
- [x] Run the new coherent provider through the existing129 address controls
  and complete owned Lockout/Cairo/Metropolis metadata checks. They verify
  64,290/112,779/104,548 pointer fields and33,525,954/69,887,237/60,340,131 owned
  bytes, preserving every unpatched authored byte and required sound admission.
  All four runs exit0. A subsequent extra profile compile lacks a header in the
  older frozen include tree; this probe setup issue is recorded separately and
  being repaired without changing product semantics. Its scratch is removed.
  [Actual metadata controls](../.local/mcc-native-click-crash-20261018/runtime-relocation-product-v1/metadata-controls.json).
- [x] Complete coherent-provider compilation and the repaired timing probe.
  The repeated contract/three-map controls pass; the new public lookup builds
  the complete timing plan in0.441249s, with the exact prior index/metadata hashes,
  4,184,679 calls and all exposed timing fields preserved. All315 parent inputs,
  three maps and provider dependencies remain unchanged. Temporary binaries/maps
  and scratch are removed. Native acceptance is underway; an initial native
  comparison checker used the wrong capture-row prefix after a successful V14
  tick60 baseline, and its retained failure is corrected for retry. No installed
  runtime is replaced by either probe.
  [Compiled provider validation](../.local/mcc-native-click-crash-20261018/runtime-relocation-product-v1/validation.json).
- [x] Compare real V14 and indexed-lookup native firing through fixed tick60.
  Fire at3/release at4 yields identical decoded image, save outputs and full
  capture state, including loaded59/reserve120/two detonations. Both exit0;
  callback audio is finite/nonzero. All315 prior providers,97 shaders, four
  user saves and installed reference runtime remain exact. Private runtime,
  link map, raw audio/frame/cache/save scratch is removed; the two small PNGs
  are retained as actual comparison references. These checks do not certify
  full audio identity, gameplay coverage or physical-desktop boot speed.
  [Native firing comparison](../.local/mcc-native-click-crash-20261018/runtime-relocation-firing-v1/validation.json).
- [x] Retain the accepted coherent V15 candidate for interactive validation and
  delivery, replacing exactly one leaf while preserving314 providers/97 shaders.
  Extended actual-map regression checks now verify every declared full forward
  range and its final payload byte in addition to reverse ownership; all three
  maps and generic controls pass. Required retained inputs are the2,328,600-byte
  provider,104,712,264-byte runtime and20,570,809-byte owner map. Generated test
  binaries and scratch are removed. Installation/full readiness remain separate.
  [V15 coherent candidate](../.local/mcc-native-client-20261018/client-current-v15-runtime-address-index-v1/cohort.json).
- [x] Validate V15 with real private-display pointer/focus/click, held fire,
  mouse look and movement. All nine phases survive15.17s after rendered-frame
  readiness; live first-person particles reach four draws and no errors occur.
  Frame5 is observed at20.94s, window at3.27s with copied warm GPU cache and
  saves. The separate fixed-tick firing pair takes32.55s(V14)/23.78s(V15).
  These are bounded observations, not guaranteed desktop timing or full parity.
  The test deliberately terminates only its private child after the sequence;
  all user saves remain exact and scratch is removed. Installation is next.
  [V15 real input](../.local/mcc-native-click-crash-20261018/candidate-v9/validation.json).
- [x] Install the accepted V15 lookup into both active native-test runtimes.
  All four actual launcher-info routes admit the interim test and retain full
  readiness false; the user's actual launcher completes eight frames with exit0.
  Four user save files,97 shaders and315 inputs stay exact. Prior runtimes and
  capability files are preserved for rollback; generated capture/cache/test
  scratch is removed. This delivers the measured startup improvement through
  Play while full map conversion/current-client64-bit/parity remain unfinished.
  [Installed V15 validation](../.local/mcc-native-click-crash-20261018/indexed-installed-v1/validation.json).
- [x] Publish the requested compiled V15 ZIP with the installed lookup change.
  All4,249 archive entries/checksums and both relocated launcher-info routes
  pass. The real relocated launcher renders eight frames with exit0 in27.35s
  under tracing;108,603 opens contain no reference/development dependency.
  All97 bundled shaders and unchanged V14 tools/data remain exact. The43
  unchanged importer controls are inherited, not repeated. No game cache or
  saves are distributed. Requested ZIP183,597,745bytes and118-byte checksum
  are retained; extraction,31,098,642-byte trace, raw frame and scratch are
  removed. New map-qualified conversions remain outside V15 and full readiness
  stays false.
  [V15 package validation](../.local/mcc-native-click-crash-20261018/package-v15/validation.json)
  · [Compiled V15 ZIP](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261019-v15.zip).
- [x] Synchronize the user's existing extracted folder with all4,249 V15
  archive files. Only five small metadata files change; both launcher-info
  routes pass and retain native-test admission/full readiness false. Runtime
  hashes,97 shaders,four saves and45,231 cache-tree stat entries stay exact,
  with no asset reimport. Rollback metadata totals505,646bytes and is explicitly
  retained; extraction/staging scratch is removed.
  [Extracted V15 synchronization](../.local/mcc-native-click-crash-20261018/sync-extracted-v15/validation.json).
- [x] Identify concrete authored conversion gaps against the retained original
  comparison and current owned MCC directories. Relic's flag-reset stop distance
  and Gauss flyby differ by original map generation; tag-name-only changes would
  damage other maps or mods. Two classic flyby dependencies are absent from all43
  imported directories. This is an inventory milestone; conversion, original
  consumer verification and sound reconstruction remain open.
  [Current tag-gap observations](../.local/mcc-native-click-crash-20261018/cleanup-v14/gameplay-tag-gaps-v14.json).
- [x] Authenticate the first three map-qualified conversion contexts before
  implementation: original Lockout uses CTF distance6/Gauss null, Relic uses
  distance4/Gauss Scorpion flyby, Backwash uses distance6/Gauss Scorpion flyby.
  Current scenario, mulg and Gauss donor/target banks are hash-pinned against
  fresh offline original reads. The original `24D5F0` consumer follows
  globals→mulg→runtime constants, squares field`D8` and compares squared3D
  distance. Sixteen genuine instruction-block controls preserve the strict
  near-distance boundary; full caller/team/iterator execution is not claimed.
  Scratch is removed. Pure planning, atomic integration and broader map
  contexts remain open; references remain offline only.
  [Context evidence](../.local/mcc-map-qualified-conversion-20261019/profile-evidence-v1.json)
  · [Original distance consumer](../.local/mcc-map-qualified-conversion-20261019/original-ctf-radius-v1.json).
- [x] Implement the separate pure map-qualified planner with guarded before/
  after writes for those three contexts. Exact scenario/header identity and
  original primary-globals→salted mulg binding qualify complete owner tuples.
  Strict compilation and real-context fixtures pass, including idempotence,
  changed/modded donors, identity/salt/group, absent/ambiguous dependencies,
  short/stale views, overlapping writes and invalid-width rejection. Scratch
  is removed and no binaries are retained. This source is not wired into Cache
  or V15; independent review, sanitizer/actual integration and broader coverage
  remain open. The current Cache source and active runtimes stay unchanged.
  [Pure planner controls](../.local/mcc-map-qualified-conversion-20261019/planner-validation-attempt2.json).
- [x] Run the pure three-context planner under AddressSanitizer and
  UndefinedBehaviorSanitizer. The same authenticated context and rejection
  controls pass with strict warnings and no sanitizer findings. Source pins
  remain unchanged; all generated fixtures/executables/scratch are removed.
  Independent review and actual Cache integration remain separate gates.
  [Planner sanitizer controls](../.local/mcc-map-qualified-conversion-20261019/planner-sanitizer-v1.json).
- [x] Independently review the exact pure planner and extend sanitizer controls.
  All qualification map-ID reads lie inside the captured scenario bank; target
  tuples reproduce independently. Forty-six added controls cover custom owner
  boundaries, individual stale salts, changed guards and overlapping/malformed
  multi-write publication. No blocking flaw is found within the three-context
  scope. Integration must retain immutable directory/storage ownership, resolve
  all destinations before preflight/copies, precede current donor conversions
  and preserve unknown contexts. All scratch is removed, with no large artifacts.
  [Independent planner review](../.local/mcc-map-qualified-conversion-20261019/review-v2/validation.json).
- [x] Integrate the reviewed map planner into normal Cache loading and verify
  exact intended vanilla writes, custom preservation and current/frozen builds.
- [x] Deliver the authenticated integrated map conversions through a coherent
  native runtime after bounded input/render/callback-audio acceptance. V16
  extends Gauss/Relic coverage to 28 contexts; V18 additionally ships Floodzone's
  two material indices with fixed-tick, interactive and relocated launcher
  validation. Further gameplay-tag coverage remains a separate open gate.
- [x] Wire the pure planner into ordinary build configuration and run its real
  authenticated three-context fixture through disposable CMake/CTest. The test
  passes in0.09s; it is not skipped. The public lifetime comment now explicitly
  requires unchanged directory/storage ownership through publication. Both the
  generated fixture and private CMake build are removed. The Cache hook is now
  in source before generic converters; compilation/actual normal Cache readback
  and coherent native delivery are still open.
  [CMake planner validation](../.local/mcc-map-qualified-conversion-20261019/cmake-v1/validation.json).
- [x] Authenticate the active Cache provider before coherent integration. The
  unchanged44,650-byte preimage plus106 pinned dependencies reproduces all68
  allocated sections,2,255 semantic relocations and190 nonlocal symbol contracts
  of the active V15 provider, warning-free. Generated4,472,304-byte object,
  projected headers and depfile scratch are removed; the small source preimage
  and reproduction receipts remain. This baseline proof does not admit the
  new hook or certify the whole current-header client.
  [Cache baseline compatibility](../.local/mcc-native-click-crash-20261018/map-cache-baseline-v1/validation.json).
- [x] Independently review and compile the integrated Cache hook with ordinary
  current headers. Exact source compiles warning-free in5.47s with451 dependency
  pins unchanged. Full-salt/unique lookup, immutable ownership, destination
  resolution before all guard checks, custom preservation and bounded copies
  precede generic conversion. Invalid metadata throws before any map write.
  The4,781,568-byte generated object and scratch are removed. This proves the
  source hook/ordinary compilation; actual-map execution and full client ABI
  acceptance remain separate.
  [Integrated hook review](../.local/mcc-map-qualified-conversion-20261019/review-v3/validation.json).
- [x] Execute normal Cache loading with the coherent integrated provider and
  compare each complete owned backing against V15. Lockout changes exactly its
  eight flyby-reference bytes; Relic changes exactly the required6.0→4.0 binary32
  word (one differing byte); Backwash remains byte-identical. The complete
  70,788,456/75,886,884/75,749,476-byte backings otherwise match, and every tag
  directory identity stays exact. All host backings exceed4GiB; source maps and
  315 parent inputs remain unchanged. Snapshot/binary/projected-header scratch
  is removed. Normal mod/fault controls, coherent native acceptance and broader
  map conversion remain open.
  [Normal Cache vanilla readback](../.local/mcc-map-qualified-conversion-20261019/cache-integration-v1/validation.json).
- [x] Verify normal Cache admission using a renamed stock map and two modified
  contexts. The renamed stock file receives exactly the intended eight-byte
  null flyby restoration, proving filename-independent qualification. Modified
  map-ID and header-path cases remain completely byte-identical to V15 with
  zero new writes. All backings exceed4GiB and full directory identities match.
  Source maps/315 parent inputs stay unchanged. The initial probe used a missing
  reader method; its setup failure is retained, then corrected to the checked
  pointer API. All generated map copies/snapshots/binaries/scratch are removed.
  This is focused mod preservation, not full mod-folder/render compatibility.
  [Normal Cache mod controls](../.local/mcc-map-qualified-conversion-20261019/cache-mod-controls-v1/validation.json).
- [x] Run the coherent map-conversion candidate through actual native SMG
  firing beside installed V15. Both complete60frames/ticks with identical
  complete capture state, decoded image, PNG and save outputs; loaded59,
  reserve120 and two detonations at tick4 match. Both PCM callbacks are finite/
  nonzero; sample identity is not claimed. All315 parent providers,106 projection
  origins,97 shaders,four saves, P libraries/maps and source pins remain exact.
  Private providers/ELF/link map/header/cache/save/raw-frame/audio scratch is
  removed. Two340,383-byte actual PNG references are explicitly retained.
  This accepts the bounded native shot path; installed V15/ZIP are unchanged
  and expanded map context/current-client/full parity gates remain open.
  [Native map-conversion firing](../.local/mcc-map-qualified-conversion-20261019/native-firing-v1/validation.json).
- [x] Authenticate broader Gauss context coverage before expansion. Fresh43
  MCC directory parses and38 pinned offline Xbox reads find28 paired contexts:
  five campaign and23 multiplayer, with22 null targets/six retained. All23
  multiplayer maps share exact mulg/Gauss donor owners, but distinct scenario
  banks/paths/IDs; only Relic's original CTF distance is4.0, the other22 use6.0.
  Campaign Gauss definitions have distinct salted handles/root addresses and
  bank digests, type0, and no resident mulg; do not require multiplayer owners
  or IDs for those profiles. MCC-only/resource contexts remain unqualified.
  A superseded audit Type offset is corrected to original schema`+10`. All
  source ownership pins stay exact; no banks/maps/binaries remain and both
  audit scratch trees are removed. Product expansion is a separate gate.
  [All-context authoring inventory](../.local/mcc-map-qualified-conversion-20261019/all-contexts-v2/inventory.json).
- [x] Expand the pure planner to all28 authenticated paired Gauss contexts.
  Warning-free controls pass all28 fresh owned fixtures and the prior three
  retained fixtures, covering exact original targets, all other owner bytes,
  idempotence, custom preservation, full identity rejection and guard atomicity.
  Five campaign profiles use exact resident scenario/Gauss identities without
  reading their zero-extent mulg owners. All23 multiplayer profiles keep full
  mulg and primary-globals-binding guards. Public source layout/serialized widths
  stay unchanged; the Cache hook remains exact. The4,289,689-byte private fixture,
  215,664-byte test ELF and generator/test scratch are removed. Only the13,215
  byte generated identity/hash/scalar table enters source. Independent expanded
  review, sanitizer/normal Cache/native acceptance remain open.
  [Expanded planner controls](../.local/mcc-map-qualified-conversion-20261019/expansion-v1/validation-regular.json).
- [x] Run the expanded28-context planner and retained three fixtures under
  AddressSanitizer/UndefinedBehaviorSanitizer, warning-free with no findings.
  The preflight function remains byte-identical to the reviewed version and
  the public header's noncomment lines remain exact. Source/table/test pins
  stay unchanged. The9,183,744-byte sanitized ELF,4,289,689-byte private fixture
  and scratch are removed; only small source preimages/receipts remain.
  [Expanded sanitizer controls](../.local/mcc-map-qualified-conversion-20261019/expansion-v1/validation-sanitizers.json).
- [x] Run all28 fresh owned contexts through the ordinary current-header
  CMake target/CTest registration. The positive fixture passes in0.39s and is
  not skipped. Source and donor pins stay exact; the4,289,689-byte generated
  fixture and private CMake build/scratch are removed. Normal Cache and native
  acceptance of the expanded table remain separate gates.
  [Expanded CMake validation](../.local/mcc-map-qualified-conversion-20261019/cmake-v2/validation.json).
- [x] Independently review every expanded profile against the pinned inventory
  and re-read79 original Gauss/CTF words. All28 exact identities/hashes/types
  match:22 null/six retained flybys, Relic-only4.0, campaign isolation and23
  multiplayer ownership/binding guards. Fourteen additional upper-bound/width/
  short-read/overlap/no-partial-write/unknown-map controls pass. Header layout,
  preflight bytes and Cache publication hook remain unchanged. The70,120-byte
  private probe and scratch are removed. This is expanded source review, not
  normal Cache/native or full parity acceptance.
  [Independent expanded review](../.local/mcc-map-qualified-conversion-20261019/independent-expansion-review-v1/validation.json).
- [x] Verify the expanded planner through all28 authenticated normal Cache
  constructor pairs plus the two unqualified MCC-only maps. Complete backings
  compare2,817,874,316bytes in RAM: only177 intended bytes differ, covering22
  null flyby references and Relic's4.0 stop-distance word. All other backing
  bytes and tag directories stay exact; five already-equivalent contexts and
  both unqualified maps stay exact. All60 host backings exercise addresses
  above4GiB. The315 V15 providers,97 shaders and both43-map inventories stay
  pinned. Scratch, the104,013,112-byte ELF and all private objects/headers are
  removed; no backing snapshots or banks were retained. This proves normal
  constructor/publication behavior, not rendered gameplay or full parity.
  [Expanded normal Cache acceptance](../.local/mcc-map-qualified-conversion-20261019/cache-all28-v1/validation.json).
- [x] Verify the expanded28-context provider in the native fixed-tick firing
  path against V15. Both runs exit0 at60frames/60ticks with exact decoded
  image, save outputs and full shot/capture state: loaded59/reserve120 and
  detonation tick4. Callback audio remains finite/nonzero; sample identity and
  full parity are not claimed. The315 parents,106 dependency origins, maps,
  four saves,97 bundled shaders, libraries and source pins stay exact. Scratch,
  private providers/runtime/map, PCM and cache are removed. Two340,383-byte
  actual PNGs are retained as required references, with identical hashes.
  [Expanded native firing acceptance](../.local/mcc-map-qualified-conversion-20261019/native-firing-v2/validation.json).
- [x] Retain a coherent expanded28-context candidate:316 link inputs comprise
  314 unchanged V15 providers, one replaced Cache and one added planner leaf.
  The retained104,754,568-byte ELF is byte-identical to the accepted ephemeral
  expanded candidate and independently passes the same60-tick shot against
  retained V15 frame/save/capture evidence. Callback PCM is finite/nonzero.
  Required delivery Cache/planner objects, ELF and owner map total130,543,356
  bytes; hashes/reasons and small frozen source pins are recorded. Temporary
  headers, PCM, saves/cache and scratch are removed. Both root/P selections,
  runtimes, launchers, sidecars, saves, maps, shaders and parent providers stay
  exact. This is a retained interim candidate, not installation/full parity.
  [Retained expanded cohort](../.local/mcc-map-qualified-conversion-20261019/cohort-v16/validation.json).
- [x] Verify interactive input against that exact retained candidate. Its
  private relocated104,708,392-byte payload survives all nine actual XTest
  enter/focus/click/release/held-fire/release/look/W/release phases for15.167s
  after frame readiness, with four live first-person particle draws and no
  runtime error. The harness intentionally sends SIGTERM after acceptance;
  this is not a normal-exit or desktop boot-speed claim. All316 providers,
  106 origins, maps, sources, four saves,97 bundled shaders, libraries, root
  material-workbench selection/runtime and P installation stay exact. Scratch,
  copied payload, private saves and caches are removed; no new large artifact
  is retained. Packaging and guarded P delivery remain separate acceptance.
  [Expanded actual input acceptance](../.local/mcc-map-qualified-conversion-20261019/native-interactive-v2/validation.json).
- [x] Package V16 with fresh build/acceptance authority and verify its actual
  relocated launcher under file tracing. Both info routes pass; the8-frame
  launch exits0 with no errors, reference-file or development-tree opens among
  108,605 attempts. All4,249 archive entries/checksums pass, only six runtime/
  release metadata files differ from V15, and97 shaders/tools/data/libraries
  stay exact. Root/P runtime metadata and saves stay exact; P cache is not
  written. Full-ready remains false. Required ZIP183,612,692bytes and118-byte
  checksum are retained; temporary hardlinks, private saves/cache/frame and
  31,098,772-byte trace are removed with scratch. Guarded P installation and
  independent archive review remain separate gates.
  [Compiled V16 package acceptance](../.local/mcc-map-qualified-conversion-20261019/delivery-v16/package-validation.json).
- [x] Independently review V16 archive metadata/delta. All4,249 entries and
  4,248 checksum rows pass: exactly six allowed files change and4,243 stay
  V15-identical, including97 shaders,2,600 tool files,190 libraries and1,235
  Python files. Current316-provider/raw/payload/sidecar/source/proof bindings
  agree; V15 evidence is explicitly historical. No game data/cache/save is
  bundled. P still matches its V15 preservation guards. Review scratch is
  removed, with no large artifacts retained or installer/native rerun.
  [Independent V16 archive review](../.local/mcc-map-qualified-conversion-20261019/archive-review-v1/validation.json).
- [x] Install only the verified six V16 files in P. All4,249 selected release
  files match the accepted archive; both P launchers select its updated wrapper
  with test available/full-ready false. All97 shaders, the entire P cache tree
  and four saves remain exact. Root's renderer workbench selection, runtimes,
  launcher/capability metadata and cache are not written. Required rollback
  copies total105,173,776bytes under P's
  `.native-rollbacks/map-vanilla-v16-5b411512d9f1/` and preserve the prior V15
  payload/metadata; installation scratch and
  staging files are removed. Actual relocated gameplay execution remains the
  prior accepted package proof; the installer performs no additional game run.
  [Guarded V16 extracted-folder installation](../.local/mcc-map-qualified-conversion-20261019/delivery-v16/install-validation.json).
- [x] Close the completed V16 delivery lifecycle. Fifty-one named completed
  scratch paths and the final verification scratch are absent. Fifteen required
  retained artifacts total420,351,091bytes: the requested ZIP/checksum, coherent
  delivery objects/runtime/map, exact V15 rollback files and three actual frame
  references. Every retained size/hash/reason is recorded. Other active agents
  and unrelated historical artifacts are excluded; final historical cleanup
  and the complete MCC/TU1.5 goal remain open.
  [Delivery artifact/lifecycle receipt](../.local/mcc-map-qualified-conversion-20261019/delivery-v16/delivery-closure.json).
- [x] Review V16 delivery metadata and evaluate all actual guards before
  executing package/install code. Fresh paired firing proof61ce and the
  retained candidate's separate ca00 shot proof have distinct roles; the316
  providers, all source pins and root/P preservation guards pass. New immutable
  delivery authority labels V15 ancestry as historical and uses current V16
  acceptance pins. A read-only private preflight passes and its scratch/temporary
  handoff are removed; no runtime/package mutation occurred in this step.
  [Reviewed delivery preflight](../.local/mcc-map-qualified-conversion-20261019/delivery-v16/preflight-validation-v2.json).
- [x] Resolve the supposedly absent normal visible-window pipeline-cache trace.
  Independent review and full-log regex extraction find it in both original
  eight-frame logs: 16,838,148 bytes loaded/exported, 241 graphics and one
  compute call, 0.037938/0.039212 seconds in pipeline creation. Buffered stdout
  and stderr interleave the message mid-line; the harness's `startswith` check
  caused the false negative. Preserve both original reports. No shutdown or
  atexit change is justified. Actual atomic file replacement remains a separate
  acceptance gate because the trace does not report IO success.
  [Corrected log audit](../.local/mcc-native-click-crash-20261018/pipeline-visible-correction-v1/validation.json).
- [x] Verify actual cache-file persistence in the current installed V13 normal
  visible route. Two separate eight-frame processes both exit0, replace the
  scratch-owned 19,233,358-byte cache with a new inode/mtime and mode0600, and
  consume exactly those saved bytes on the subsequent run. Pipeline creation
  takes 0.030262/0.028305 seconds. The first window appears at12.58seconds,
  frame5 at39.02seconds and exit at40.58seconds; the second exits at41.21seconds.
  These are isolated Xvfb observations, not physical-desktop or fixed speedup
  claims. All97 shaders and four user save files remain exact. Scratch
  `halo2-test-moun969j`, both temporary cache copies and copied saves are removed;
  no generated binary/image/audio artifact is retained.
  [Visible file persistence and reuse](../.local/mcc-native-click-crash-20261018/pipeline-visible-files-v1/validation.json).

- [x] Complete the twelfth bounded renderer reconstruction probe before changing
  texture upload hashing. Restoring the authentic particle-view API and removing
  the later `ModelView::source_w_distance` field reduces the mismatch to six of
  975 text/constant sections, with no added sections. This remains a rejected
  replacement: draw/material selection and buffer differences require proof.
  The ordinary renderer, installed runtime and shaders are unchanged, and
  scratch `halo2-test-ze5lw65d` is removed.
  [Probe evidence](../.local/mcc-native-click-crash-20261018/renderer-startup-v12/baseline.json).
- [x] Narrow that reconstruction to two differing text sections by restoring
  the retained environment-first accumulation predicate and the particle
  getter's original declaration order. All other 973 text/constant sections
  match, including texture creation; no new sections appear. This remains
  unaccepted for installation pending the draw and allocation-error comparison.
  Probe scratch `halo2-test-zu4fo4ia` is removed; product files are unchanged.
  [Thirteenth probe](../.local/mcc-native-click-crash-20261018/renderer-startup-v13/baseline.json).
- [x] Extend the reconstruction check to all 1,092 allocated ELF sections,
  normalized relocation targets and nonlocal symbol definitions. This confirms
  the two remaining functions also account for unwind/exception and relocation
  differences; raw text equality alone is insufficient for acceptance. No
  replacement is promoted. Probe scratch `halo2-test-rpo5dqqu` is removed;
  the small projected source and diagnostic contract remain as reference evidence.
  [Allocated-section comparison](../.local/mcc-native-click-crash-20261018/renderer-startup-v14/allocated-contract.json).
- [x] Independently review the remaining accumulation loop and buffer path
  against retained assembly/DWARF. The environment-first predicate, phase-one
  `emit_bsp_draw` call, original memory selection and early pStencil setting
  agree; no further semantic change is justified. The projected public renderer
  header also matches the original pinned `3fd32d4e...` bytes. Two function
  differences remain unresolved, so the optimization is not installed. Final
  checks preserve both installed runtime hashes and the ordinary renderer hash.
- [x] Review the proposed upload-bucket optimization independently. Replacing
  only the function-local pixel hash with a binary-length `string_view` hash
  preserves image-sharing decisions: identical pixels reach the same bucket;
  complete pixel/shape/mip equality rejects collisions, and candidate insertion
  order preserves the first exact owner. No bucket order or hash value is
  persisted or used for rendering order. Source lifetimes and shared GPU ownership
  remain valid. Keep the separate shadow/texture identity FNV unchanged. This is
  a design proof, not compiled/runtime acceptance or an installed optimization.
- [x] Test the source-location/code-generation hypothesis independently. Restoring
  original logical source paths and known function line numbers leaves the exact
  same two-function assembly diff as probe13; source-location metadata does not
  explain the remaining mismatch. Scratch `halo2-test-47vjg2e0` is removed and
  no product/provider replacement is made.
  [Source-location control](../.local/mcc-native-click-crash-20261018/renderer-startup-v15/baseline.json).
- [x] Exercise the reconstructed renderer in the actual coherent native game
  before optimizing it. With identical MCC Lockout data, original public-header
  bytes, fixed tick8, RNG seed, interpolation fraction and input, the selected
  V13 renderer and projection produce byte-identical 640x480 images and save
  outputs. Both exit0 with eight frames, at36.51/35.24seconds. The image was
  viewed. All315 parent inputs, all97 shaders, ordinary source and four user
  saves remain exact. This bounded test does not resolve the two compiled-body
  differences or certify full parity, and no product replacement is installed.
  Scratch `halo2-test-tgle3xk3`, projected archive/object/executable and raw frames
  are removed. Two 340,327-byte lossless PNGs remain as explicit comparison
  reference inputs.
  [Actual renderer comparison](../.local/mcc-native-click-crash-20261018/renderer-runtime-baseline-v1/validation.json).
- [x] Independently review the actual isolated optimized source against that
  runtime-tested projection. Its complete delta is two explicit standard
  includes and the local upload-bucket hash replacement; reversing them restores
  the baseline byte for byte (`9064a1a8...` to candidate `10a4a431...`). All full
  equality guards, ownership and the separate shadow FNV are unchanged. Compiling
  adds only the expected upload-lambda text difference alongside the two known
  baseline differences, with no new text/constant sections. Actual native
  optimized-frame and performance acceptance remain pending; source/install
  files are unchanged.
- [x] Run the optimized hash through the same actual fixed-tick native game.
  Its tick8 image and save outputs are byte-identical to installed V13; both
  exit0. This isolated pair takes36.86seconds for V13 and30.29seconds for the
  candidate, a6.57second observation rather than a guaranteed desktop speedup.
  Against the directly runtime-tested projection, all allocated code/data and
  semantic relocations remain exact except the intended upload lambda and its
  compiler-owned unwind/exception records; both prior differing functions and
  their exception/relocation records remain exact. All315 parent providers,
  all97 shaders, source and user saves are unchanged. Scratch
  `halo2-test-ye6atdon`, test archive/object/executable and raw frames are removed.
  Two340,327-byte PNGs remain as explicit reference inputs. Full input/firing,
  ordinary-source integration, installed delivery and wider parity remain open.
  [Actual optimized frame](../.local/mcc-native-click-crash-20261018/renderer-upload-hash-v1/validation.json)
  · [Allocated contract delta](../.local/mcc-native-click-crash-20261018/renderer-upload-hash-v1/optimized-contract-comparison.json).
- [x] Integrate exactly that reviewed local hash into ordinary renderer source,
  preserving all unrelated renderer work and all shaders. Reversing the two
  includes and hash replacement reproduces the exact prior `66bb984c...` source;
  its frozen preimage is retained. New ordinary source is `2daadf68...`. Current
  header compilation and firing/input delivery checks remain separate gates;
  installed runtimes and the V13 ZIP are unchanged.
- [x] Compile the exact ordinary updated renderer with current engine headers
  and normal configured compiler flags. The first attempt correctly reports the
  missing generated menu vertex catalog in the configured build directory;
  preserve that RED report. Supplying only the authenticated retained catalog
  include produces a warning-free26.04second compile with938 pinned dependencies
  and no input drift. The24,998,504-byte object and scratch `halo2-test-tapon9_j`
  are removed. This is current-source compilation, not a current-client link,
  installed release or full64-bit/parity acceptance.
  [Current-source compilation](../.local/mcc-native-click-crash-20261018/renderer-upload-source-v1/current-compile.json).
- [x] Compare real SMG firing through fixed tick60 with the optimized renderer
  against installed V13. Both render60frames, exit0 and produce byte-identical
  images, save outputs and complete capture-state rows: fire at3/release at4,
  loaded ammo59, reserve120 and two detonations. Both production audio callbacks
  contain finite nonzero samples; callback sample identity is not claimed.
  The independently rebuilt optimized ELF matches the earlier candidate's exact
  `23111ca3...` hash. All315 parent inputs,97 shaders and user saves are unchanged.
  Scratch `halo2-test-h4jf2jjg`, test binaries, archive/object, raw frames/audio
  and copied saves are removed. Two bounded PNG reference inputs remain.
  Live window/input, installed delivery and wider renderer/parity gates remain.
  [Firing/image/audio comparison](../.local/mcc-native-click-crash-20261018/renderer-upload-firing-v1/validation.json)
  · [Exact shot-state audit](../.local/mcc-native-click-crash-20261018/renderer-upload-firing-v1/firing-state-audit.json).
- [x] Link a reproducible coherent upload-hash candidate for guarded delivery.
  Its104,712,152-byte ELF again matches `23111ca3...`;314 other providers and all97
  shaders remain exact. The required94,403,022-byte renderer archive and
  20,568,715-byte owner map are explicitly retained with that candidate. All
  compiler/link scratch `halo2-test-o09an60q` is removed. This is an interim
  Lockout candidate, with full renderer/current-client64-bit/parity gates still
  open; installed runtimes remain unchanged pending live input qualification.
  [Candidate cohort](../.local/mcc-native-client-20261018/client-current-v14-upload-hash-v1/cohort.json).
- [x] Repeat actual pointer, focus, click, held fire, mouse look and forward
  movement with the packaged form of the upload-hash candidate. It survives all
  nine phases and15.16seconds after readiness, with four live first-person
  particle draws and no runtime errors; copied user saves remain exact. The
  isolated warm window appears at12.07seconds and frame readiness at32.00seconds.
  This is bounded Xvfb input acceptance, not a normal-exit or desktop-speed claim.
  Scratch `halo2-test-2kiwv7ag` and temporary payload/cache/saves are removed.
  [Live input qualification](../.local/mcc-native-click-crash-20261018/candidate-v8/validation.json).
- [x] Install the accepted upload-hash runtime into the source launcher and
  user's existing extracted folder, retaining exact prior binaries/capabilities
  for rollback. Both launcher variants in both locations report native test
  available and full readiness false. The actual installed user launcher
  completes eight frames successfully; all97 shaders,315 coherent inputs and
  four save files remain exact. Required rollback copies are retained; scratch
  `halo2-test-_q1rh1ry`, raw frame and temporary package payload are removed.
  The next compiled ZIP and metadata synchronization are still pending.
  [Installed upload optimization](../.local/mcc-native-click-crash-20261018/upload-installed-v1/validation.json).
- [x] Publish the user-requested compiled V14 ZIP with the accepted texture
  upload optimization. Both relocated launchers, the actual eight-frame route,
  every checksum across4,249 archive entries and all97 shaders pass. Other V13
  package files/tools/data remain exact; its43 importer controls are inherited
  evidence, not needlessly rerun. No original-reference/development-tree opens
  occur and no game data or saves are shipped. The183,596,975-byte requested ZIP
  has SHA256 `f31fc997595e3567de998e3acd6fcbaaba321689807efe8126f4e62c8b5034ed`;
  its118-byte checksum is retained. Scratch `halo2-test-x7ia7q28`, relocated
  staging, cache/save links, raw frame, trace and temporary ZIP are removed.
  Full renderer/gameplay/conversion/64-bit/loading/video/TU1.5 parity remains open.
  [V14 package validation](../.local/mcc-native-click-crash-20261018/package-v14/validation.json)
  · [Compiled V14 ZIP](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261019-v14.zip).
- [x] Synchronize the user's existing extracted folder to every V14 archive
  checksum. Five validation/capability/manifest/README/checksum files update;
  the already installed payload and all other package files match. Both actual
  launcher-info variants pass, full readiness stays false, and the user's MCC
  cache, saves and unlisted extras remain exact. Required501,563-byte rollback
  preimages remain; staging scratch `halo2-test-o08je0lx` is removed. No game-data
  copy or import is repeated.
  [Extracted-folder V14 synchronization](../.local/mcc-native-click-crash-20261018/sync-extracted-v14/validation.json).

- [x] Establish an automated real-window mouse reproduction on the user's
  exact V9 payload/cache, with copied saves on a private Xvfb display. Pointer
  entry and focus survive; live firing exits1 at the same `2839AD/48C488`
  codec read. The corrected readiness harness observes the delayed failure
  within 8.73 seconds of its first rendered readiness sample. Its cold test
  window appears at12.81seconds and frame readiness at97.63seconds; those are
  isolated Xvfb measurements, not physical-desktop startup certification.
  User save bytes are preserved and scratch is removed. The prior normal-mode
  run also reaches the same fault, but its frame-diagnostic readiness assertion
  was invalid and is retained as such.
  [Automated mouse RED](../.local/mcc-native-click-crash-20261018/v2/validation.json),
  [Actual live-input fault](../.local/mcc-native-click-crash-20261018/v2/runtime.log).
- [x] Authenticate the two live-input data extents against original TU1.5:
  the12-byte Boolean-selected codec8 decode triple at `48C488` is file-backed
  writable data; the8-byte comparison bounds at `46A7C0` are read-only.
  Twenty-four original selector prefixes and twenty-two guard continuations
  with genuine RET pass. These qualify exact bytes/access and reader selection,
  not codec children, the complete angular owner or full gameplay parity.
  Original/scratch inputs remain offline and scratch is removed. Product
  publication and the corrected live mouse sequence remain open.
  [Original20-byte authority](../.local/mcc-native-click-crash-20261018/authority-v1/primary-reviewed.json).
- [x] Publish the exact20-byte live-input compatibility in the ordinary data
  factory and review its compiled controls. All 2,442 previous tuples remain
  exact; the new factory has 2,444 regions/479,848 bytes. Original access is
  preserved: writable codec descriptor with warm writes/restore and read-only
  angular bounds. Twenty-eight rejecting controls, all twenty foreign overlap
  bytes before writes, two host pointers above4GiB, dependencies and strong
  exports pass. The primary-agent second-method review admits only these data
  owners; real mouse regression remains open. Test/link scratch is removed;
  the required 1,249,296-byte compatibility object is retained. The first
  control attempt caught a descriptive authority count error; the reviewed
  report correctly records24 selector cases and preserves the original report.
  [Ordinary20-byte compatibility](../.local/mcc-native-click-crash-20261018/product-v1/validation.json),
  [Source/access/ownership review](../.local/mcc-native-click-crash-20261018/product-v1/review.json).
- [x] Link the accepted20-byte leaf into coherent candidate v6. All 313 other
  providers, 43 MCC maps, 97 shaders and prior compatibility records remain
  exact. The required104,679,728-byte ELF and20,464,860-byte map are retained;
  link scratch is removed. Installed runtime and the user's V9 package remain
  unchanged. The actual mouse regression and startup stack sampling are the
  next gates; no fixed interactive launch is claimed yet.
  [Coherent live-input candidate](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v6/cohort.json),
  [Single-leaf link receipt](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v6/link-manifest.json).
- [x] Verify the missing codec row clears in the actual packaged-dependency
  mouse run. Candidate v6 reaches real first-person particle submissions and
  progresses to the codec child's first output store at `2962CF`, requiring
  `510EF0`; sustained firing still exits1. User saves and V9 remain unchanged,
  and scratch is removed. Sibling GDB attachment is denied by the host's
  existing ptrace policy; those attempts provide no startup stack evidence.
  [Actual v6 live-input boundary](../.local/mcc-native-click-crash-20261018/candidate-v1/validation.json).
- [x] Authenticate the complete codec8 quaternion, translation and scale
  producers and both required workspaces. Twenty original controls execute all
  three bodies through sixty real RETs with no child stubs. They cover every
  byte of the32-byte transform at `510EF0` and24-byte translation temporary
  bank at `56E6A0`; every workspace read follows the actual writer. Poisoned
  prior contents do not affect the complete output. Original writable virtual
  tails are explicitly loader-zeroed, rather than guessed initialized outputs.
  Scratch is removed; product ownership and a new live run remain open.
  [Complete codec workspace authority](../.local/mcc-native-click-crash-20261018/workspace-authority-v1/primary.json).
- [x] Publish and compile the complete56-byte codec workspace ownership.
  All2,444 previous factory tuples remain exact; the factory now has2,446
  regions/479,904bytes. Sixty-one rejecting controls include every one of the56
  foreign-overlap bytes before publication. Both host pointers exceed4GiB;
  original writable access, warm writes and memory-subowner restore pass.
  Strong exports, all314 previous inputs and six ordinary dependencies remain
  exact, with no reference opens. Scratch is removed; the required1,266,032-byte
  object is retained. Source review, coherent linking and live mouse acceptance
  remain separate gates.
  [Compiled workspace validation](../.local/mcc-native-click-crash-20261018/workspace-product-v1/validation.json).
- [x] Review the56-byte product by inverse source reconstruction and exact
  original owner/access comparisons. All prior factory rows, strong exports,
  dependencies and61 rejection controls remain verified; review scratch is
  removed. This is a second method by the primary agent, not an independent
  reviewer or proof of full interactive gameplay.
  [Workspace review](../.local/mcc-native-click-crash-20261018/workspace-product-v1/review.json).
- [x] Obtain four real startup stacks with the debugger owning only its private
  child. At4seconds the main thread is building MCC sound timing; at14 and28
  seconds it is loading/decompressing MCC textures, and at40seconds uploading
  textures. These samples identify work to measure; they do not establish its
  total cost or a startup improvement. The intentionally interrupted diagnostic
  removes scratch and leaves user saves/processes and security settings intact.
  [Actual startup samples](../.local/mcc-native-click-crash-20261018/startup-profile-v2/validation.json).
- [x] Link coherent candidate v7 with only the reviewed56-byte workspace leaf
  replacing v6's data leaf. All313 other providers,43 maps and97 shaders remain
  exact. The104,679,728-byte candidate and20,465,237-byte link map are retained;
  generated link scratch is removed. Installed/V9 bytes remain unchanged until
  actual interactive acceptance passes.
  [Coherent workspace candidate](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v7/cohort.json).
- [x] Run candidate v7 through real pointer/focus/click and held firing with
  the user's packaged libraries/cache. Both codec workspaces clear and real
  first-person particle draws reach4. Sustained firing next reaches its parent
  codec publication table at `28B1B1/48C7E0` and exits1; the full interactive fix
  remains open. Frame readiness is78.72seconds in this isolated run, without
  debugger attachment; this data-only change does not establish an optimization.
  Saves remain exact and generated scratch is removed.
  [Actual v7 boundary](../.local/mcc-native-click-crash-20261018/candidate-v2/validation.json).
- [x] Identify the final missing absolute operand in complete immediate parent
  `28B080`: `48C7E0` is a16-byte quaternion sign mask, not a dispatch pointer.
  Its original file-backed writable bytes and24 actual ANDPS instruction cases
  pass, including signed zero, infinity and NaN bit patterns. Other absolute
  parent operands fall within the existing decoder workspaces. This qualifies
  bytes/access and the static inventory, not whole-parent gameplay execution.
  The fixture was corrected to account for Unicorn reporting the16-byte SSE
  operand as two8-byte reads. Original programs remain offline; scratch removed.
  [Parent operand authority](../.local/mcc-native-click-crash-20261018/parent-authority-v1/primary.json).
- [x] Publish and compile the exact16-byte sign mask with its original
  writable access. All2,446 prior factory tuples remain exact;2,447 regions now
  own479,920bytes. Nineteen rejecting controls cover all16 foreign-overlap bytes,
  owner boundaries and warm remount; native pointers and warm writes/restore
  pass. The required1,274,584-byte leaf is retained and scratch is removed.
  [Parent-mask product](../.local/mcc-native-click-crash-20261018/parent-product-v1/validation.json).
- [x] Complete the sign-mask inverse-source/access review. Only its16-byte
  owner and preflight extent differ; all prior tuples/dependencies/exports and
  nineteen rejection controls remain exact. Review scratch is removed.
  [Parent-mask review](../.local/mcc-native-click-crash-20261018/parent-product-v1/review.json).
- [x] Link coherent candidate v8 with the reviewed parent mask. All313 other
  providers,43 maps and97 shaders remain exact; the required104,679,728-byte
  candidate and20,465,138-byte map remain retained. Link scratch is removed;
  live mouse acceptance and delivery are next.
  [Parent-mask candidate](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v8/cohort.json).
- [x] Fix the reproduced V9 click/firing exit in coherent candidate v8.
  Real SDL pointer/focus/click, one-second held firing, relative mouse look and
  forward-key press/release all survive, with4 actual first-person particle
  draws and no runtime error. The earlier user faults and decoder workspaces
  now clear. User saves,97 shaders,313 unaffected providers and43 maps remain
  exact; scratch is removed. This qualifies the reproduced input sequence,
  not all weapons, long sessions or full TU1.5 parity. Delivery remains open.
  [Actual mouse GREEN](../.local/mcc-native-click-crash-20261018/candidate-v3/validation.json).
- [x] Install the accepted click fix in both the ordinary source launcher and
  the user's extracted V9 package. All four launcher-info checks admit the
  native test and keep full readiness false. The actual user launcher completes
  eight frames with exit0 in55.61seconds. All existing save files and97 shaders
  remain exact; four pre-install runtime/capability files are retained for
  rollback and test scratch is removed. Startup experiments are excluded.
  The first smoke command used an unsupported save-directory argument; its
  private dialog was stopped, preimages restored, and the supported launcher
  command rerun successfully. That dialog wait is not a game boot measurement.
  [Installed click hotfix](../.local/mcc-native-click-crash-20261018/installed-hotfix-v2/validation.json).
- [x] Publish compiled version10 with the accepted real-click fix. Both
  relocated launcher-info checks and the real eight-frame launcher route pass.
  ZIP integrity and every bundled checksum pass; all97 shaders and unmodified
  V9 files remain exact, and no game data/save files are distributed. The
  historical V9 one-shot audio receipt is labeled as baseline evidence. Startup
  experiments are excluded. Generated package, hard-linked data, frame and trace
  scratch is removed; only the183,428,804-byte ZIP and checksum are retained.
  The initial cross-filesystem hard-link harness error is corrected by using
  the already qualified same-filesystem owned development cache.
  [Version10 compiled ZIP](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261018-v10.zip),
  [SHA256](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261018-v10.zip.sha256),
  [Portable hotfix acceptance](../.local/mcc-native-click-crash-20261018/package-v10/validation.json).
- [x] Locate the sampled renderer bottleneck at the actual full-image FNV
  byte loop (`19A13A0..19A13B4`) in the user's packaged ELF. An attempted
  ordinary renderer reconstruction does not match the frozen renderer's text
  sections, so it is excluded from this hotfix. Its disposable object and
  generated scratch were removed; shader files and selected renderer remain
  exact. Startup optimization requires a compatible seam or exact source
  reconstruction, not substitution of the current renderer body.
  [Renderer compatibility comparison](../.local/mcc-native-click-crash-20261018/renderer-startup-v1/baseline.json).
- [x] Minimize repeated MCC texture decompression into a red-capable native
  regression. Twelve reads of identical stored partition bytes reinflate in the
  selected runtime; the fixture detects that directly through the real zlib
  call. The107 selected MCC-cache dependencies are available at their exact
  recorded hashes, allowing an ABI-compatible leaf build. Reconstructed current
  and historical renderer bodies are excluded; no renderer is substituted.
  [Native partition RED](../.local/mcc-native-click-crash-20261018/partition-startup-v1/red.json).
- [x] Add ABI-compatible content-keyed MCC partition residency. The rebuilt
  baseline's text/constants match the selected MCC cache exactly, using all106
  recorded header dependencies. Native controls pass repeated-decode suppression,
  independent returned writes, same-path changed bytes, malformed rejections,
  concurrent reads and pressure beyond128MiB. Original partition/file validation
  and all public engine exports remain unchanged; one additional standard-library
  unique symbol is recorded. The cache owns at most128MiB and512 entries. The
  required compiled leaf is retained, scratch removed, and actual startup timing
  remains open. The first export assertion was corrected to distinguish that
  STL unique symbol from engine API changes; its receipts remain preserved.
  [Compatible partition validation](../.local/mcc-native-click-crash-20261018/partition-startup-v1/validation.json).
- [x] Link the startup candidate with only the accepted MCC-cache leaf changed
  from click-fixed v8. All313 other providers,43 maps and97 shaders remain
  exact. The104,684,912-byte ELF and20,477,032-byte map are retained and link
  scratch is removed. Actual timing and repeated mouse input are next.
  [Startup candidate](../.local/mcc-native-client-20261018/client-current-v13-startup-v1/cohort.json).
- [x] Strengthen partition regression with simultaneous first publication
  and a zlib-call assertion proving eviction actually occurs under pressure.
  The strengthened test passes along with the existing native MCC Cache suite
  (chunks, exact shared owners, bounds, spawns, pixel/residency behavior).
  All generated executables, maps and pressure-test files are removed.
  [Additional native regressions](../.local/mcc-native-click-crash-20261018/partition-startup-v1/supplemental.json).
- [x] Verify the combined partition-residency candidate through the same real
  pointer/focus/click/held-fire/look/movement sequence. It retains4 first-person
  particle draws, no errors and exact user saves; scratch is removed. Startup
  readiness is89.72seconds with startup load average13.60, versus75.15seconds
  at3.61 for the data-only candidate. These differing-load observations do not
  prove a startup benefit; the optimization is not accepted for delivery on
  timing evidence alone. Keep the proven click-only candidate available.
  [Combined real-input result and timing limits](../.local/mcc-native-click-crash-20261018/candidate-v4/validation.json).
- [x] Measure actual graphics-pipeline creation through transparent same-argument
  Vulkan wrappers around the unchanged coherent runtime. Before first presentation
  at147.83seconds,240 calls pass null application caches and spend87.30seconds
  inside pipeline creation. This probe uses a private empty XDG cache; it is not
  the installed desktop timing or an improvement result. All call results are
  passed through unchanged and scratch is removed. Next qualify persistent
  Vulkan pipeline residency without reconstructing the renderer body or changing
  shader programs.
  [Actual pipeline cost](../.local/mcc-native-click-crash-20261018/pipeline-profile-v1/validation.json).
- [x] Qualify the optional Vulkan cache boundary in17 control groups. Graphics
  and compute descriptions, counts, allocators, outputs and error results pass
  through exactly; explicit caller caches and extension chains are preserved.
  Warm persistence, corrupt UUID/oversize rejection, driver rejection fallback,
  allocation/I/O failures, concurrent access, opt-out and cache-before-device
  teardown pass, with opaque handles above4GiB. These are fake-driver contract
  controls, not actual GPU startup/image acceptance. Scratch is removed.
  [Pipeline-cache contract controls](../.local/mcc-native-click-crash-20261018/pipeline-cache-v1/controls.json).
- [x] Link the optional pipeline-cache owner into an isolated coherent candidate:
  all 314 prior providers, 43 map pins and 97 shader pins remain exact, with
  one new owner and four Linux Vulkan API wrappers. The renderer object and
  shader programs are unchanged. Compile/link scratch is removed. This is
  link qualification; actual GPU cold/warm and input acceptance remain open.
  [Pipeline-cache candidate](../.local/mcc-native-client-20261018/client-current-v13-pipeline-cache-v1/cohort.json).
- [x] Pass the matched actual-GPU disabled and cold-cache launches. The disabled
  control takes 78.65 seconds; cold-cache creation takes 74.08 seconds and
  persists 8,067,431 driver bytes with the correct device/UUID header. All 97
  shaders and user saves remain exact. This small cold-launch difference is
  not an accepted speedup; warm consumption and real input are still pending,
  and the enclosing benchmark owns live scratch until its final stage exits.
  [Actual GPU stages](../.local/mcc-native-click-crash-20261018/pipeline-gpu-v1/validation.json).
- [x] Complete actual-GPU warm disk-cache qualification. The second process
  consumes exactly the 8,067,431 saved bytes, completes in 44.39 seconds versus
  74.08 cold / 78.65 disabled, and spends 0.205 seconds in the same 241 graphics
  plus one compute pipeline calls versus 30.299 cold. Returned cache bytes are
  unchanged; all launches exit zero, produce native frames and preserve saves
  and all 97 shader files. Scratch is removed. These are bounded private-display
  measurements; first-launch compilation, the remaining startup work and actual
  desktop timing remain distinct. Real-input and ordinary-build wiring follow.
  [Completed GPU comparison](../.local/mcc-native-click-crash-20261018/pipeline-gpu-v1/validation.json).
- [x] Wire the optional owner and all four API wrappers into ordinary Linux
  `halo2_native` builds, with a registered standalone contract CTest. Full source
  configuration, the focused target build and the managed CTest pass; generated
  native link commands contain the owner and all wrappers. No engine-wide rebuild
  is claimed. Scratch is removed; interactive candidate acceptance remains open.
  [Ordinary build integration](../.local/mcc-native-click-crash-20261018/pipeline-cmake-v1/validation.json).
- [x] Repeat real-window input against the pipeline-cache candidate and portable
  shader relocation. Focus, click/release, held fire/release, mouse look and
  forward movement survive; four actual first-person particle draws confirm
  firing admission. No runtime errors occur, all user saves remain exact and
  scratch is removed. Cold rendered-frame readiness is 74.43 seconds; this
  input run does not claim warm-start or full gameplay/parity acceptance.
  [Pipeline-cache real input](../.local/mcc-native-click-crash-20261018/candidate-v5/validation.json).
- [x] Install the verified pipeline-cache candidate into the source launcher
  runtime and the user's extracted V9 payload, retaining exact previous runtime
  and capability preimages for rollback. All four launcher-info checks and the
  actual user launcher eight-frame route pass. Full gameplay-ready stays false;
  the native test remains available, with exact saves and all 97 shader files.
  Scratch is removed. Retain the small optional device-specific product cache
  produced by normal launcher execution to avoid recompilation.
  [Installed cache update](../.local/mcc-native-click-crash-20261018/pipeline-installed-v1/validation.json).
- [x] Publish the compiled V11 ZIP with the verified click fix and persistent
  Vulkan pipeline cache. Both launchers pass from a relocated folder with spaces,
  the actual eight-frame route passes under a file-open trace, every archive
  checksum verifies, and no Xbox/Vista/firmware reference opens occur. All 97
  shaders and unmodified V10 files remain exact; no game data or saves are shipped.
  Scratch and the large file trace are removed. First-launch and remaining startup
  costs, full gameplay/conversion and full Xbox parity remain open.
  [V11 portable validation](../.local/mcc-native-click-crash-20261018/package-v11/validation.json)
  · [Compiled V11](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261018-v11.zip).
- [x] Prepare a coherent combined startup measurement candidate containing the
  verified Vulkan cache owner and the previously byte-qualified bounded MCC
  partition residency object. The other 314 providers and all shader pins remain
  exact; link scratch is removed. This candidate is isolated and is not installed
  or packaged. Qualify it against identical warm GPU cache bytes before deciding
  whether partition residency belongs in delivery.
  [Combined startup link](../.local/mcc-native-client-20261018/client-current-v13-pipeline-partition-v1/cohort.json).
- [x] Complete two alternating actual-GPU launches per startup variant with
  identical copied warm GPU cache bytes. V11 measures 53.66 / 53.96 seconds;
  partition reuse measures 50.07 / 39.00 seconds. Every run exits zero and renders
  a native frame, with unchanged saves, providers and all 97 shaders. Pipeline
  calls/counts and warm bytes match. System load falls during the series, so
  these are bounded observations rather than a fixed speedup guarantee. Scratch
  is removed. Combined real-input and delivery acceptance remain open.
  [Alternating texture-partition comparison](../.local/mcc-native-click-crash-20261018/partition-pipeline-v1/benchmark.json).
- [x] Pass the combined candidate's real focus, click, held fire, mouse look
  and movement regression, with four actual first-person particle draws and no
  runtime errors. Copied warm GPU data is used; observed frame readiness is
  38.43 seconds at startup load average 6.98. User saves remain exact and scratch
  is removed. This qualifies the combined resource owners for guarded launcher
  installation; it does not establish a universal startup time or full parity.
  [Combined real input](../.local/mcc-native-click-crash-20261018/candidate-v6/validation.json).
- [x] Install the combined resource owners into both active runtimes after exact
  rollback preflight. Both launcher variants in both locations and the actual user
  eight-frame launch pass; saves and all shaders remain exact. A rejected hard-link
  preflight changed no product files; checked copies cover that filesystem and
  same-filesystem links preserve the other preimages without duplicating data.
  Scratch is removed. V11 stays available as the preceding compiled release.
  [Combined installed runtime](../.local/mcc-native-click-crash-20261018/combined-installed-v1/validation.json).
- [x] Remove this investigation's seven completed benchmark PPMs after checksum,
  single-link and process-open checks: 6,451,305 logical bytes removed. Keep two
  pixel-equivalent PNGs (340,909 and 341,049 bytes), small reports, source pins,
  required coherent providers, rollback files and releases. Cleanup scratch is
  removed; unrelated/reference assets are untouched. This does not complete the
  separate final goal-wide obsolete-test inventory.
  [Owned capture cleanup](../.local/mcc-native-click-crash-20261018/cleanup-startup-captures-v1.json).
- [x] Verify and select the already imported V3 graphics cache through ordinary
  cache validation, without another 31 GB source import. All 883 copied source
  files, five corrections and 40 mip chains qualify; both launcher variants,
  actual eight-frame gameplay and a fresh-save unmuted shot through tick 60 pass.
  Ammo reaches 59/120 with two detonations; finite nonzero callback audio and a
  current PNG are retained (576,044 and 340,817 bytes). Actual traced launches
  take 228.72 / 171.69 seconds under the current conditions; these are functional
  results and do not close the slow-startup gate. Rendered frames are observed
  dynamically (26 here), rather than incorrectly requiring 74. No reference-file
  opens occur; scratch, traces, raw PCM and disposable frames are removed. Prior
  cache, saves, shaders, V11 and all 315 providers remain exact. Full readiness
  and full graphics/audio parity remain false.
  [Fresh-cache native validation](../.local/mcc-graphics-corrections-v3-20261018/fresh-cache-native-v2/validation.json).
- [x] Check the refreshed V3 portable importer under the bundled Python: all
  21 bitmap and 22 onboarding controls pass. The V12 actual portable launch then
  times out at 240 seconds, so no V12 archive is published. The owned process
  group/scratch are removed, the failure receipt is kept and V11 is preserved.
  Retry with a longer diagnostic window; keep portable delivery and slow-startup
  gates open.
  [V12 timeout receipt](../.local/mcc-native-click-crash-20261018/package-v12/attempt-1/failure.json).
- [x] Complete the longer V12 portable check and publish its compiled ZIP. Both
  relocated launchers and actual eight-frame gameplay pass; all archive checksums
  verify. The five refreshed importer module/data files include the exact V3
  corrections, and all 43 bundled-Python controls pass. Existing shaders and other
  V11 files remain exact; no game data/saves are shipped and no reference or
  development-checkout file opens occur. All temporary copies, traces and frames
  are removed. Startup timing remains under investigation; the timeout is retained
  as diagnostic history rather than hidden by the later passing run.
  [V12 portable validation](../.local/mcc-native-click-crash-20261018/package-v12/validation.json)
  · [Compiled V12](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261018-v12.zip).
- [x] Separate actual untraced launcher window/frame readiness from complete
  exit. The visible private-display neutral run creates its window at 18.00
  seconds, reaches frame 5 at 52.91 and exits zero at 55.90 under high system
  load. Saves remain exact and scratch is removed. The earlier 315.83-second
  traced portable run is not ordinary startup timing. No final optional cache
  trace appears in the visible run; inspect late device teardown before claiming
  that this route persists or benefits from the application cache.
  [Actual untraced readiness](../.local/mcc-native-click-crash-20261018/actual-launcher-readiness-v1/validation.json).
- [x] Reproduce a pipeline-cache registry lifetime defect with a static device
  owner that retires after the registry's former destructor. The isolated
  late-teardown control crashes with SIGSEGV before the fix; scratch is removed.
  This is a demonstrated cache-owner bug, distinct from the previously fixed
  firing exit. Correct registry lifetime, then requalify ordinary controls and
  the visible route before promoting another runtime.
  [Late-teardown RED](../.local/mcc-native-click-crash-20261018/pipeline-lifetime-v1/red.json).
- [x] Keep the registry available for process-lifetime device lookup while each
  wrapped device teardown still exports and retires its own driver cache. The
  original 17 control groups and the new late-static-device CTest pass through
  the managed focused CMake runner. The exact prior source is preserved as a
  small reference snapshot, and the private build/scratch are removed. Native
  visible-route acceptance and promotion of this fix remain pending.
  [Lifetime controls](../.local/mcc-native-click-crash-20261018/pipeline-lifetime-v1/controls.json).
- [x] Link the lifetime correction as one replacement cache owner, keeping the
  other 314 providers and all 97 shaders exact. The prior provider remains intact
  for older coherent releases. Link scratch is removed; normal visible teardown
  and subsequent promotion are still pending.
  [Lifetime candidate](../.local/mcc-native-client-20261018/client-current-v13-pipeline-lifetime-v1/cohort.json).
- [x] Observe the corrected candidate rendering and returning zero through normal
  visible eight-frame operation, preserving saves and removing scratch. Its final
  original checker missed a mid-line cache trace and left the stronger assertion
  unaccepted at that time. Retain that diagnostic failure; the corrected log audit
  and subsequent actual-file persistence/reuse acceptance are recorded above.
  [Visible functional observation and limits](../.local/mcc-native-click-crash-20261018/pipeline-visible-v1/functional-observation.json).
- [x] Repeat real input with the lifetime-corrected coherent candidate. Focus,
  clicking, held fire, look and movement survive with four first-person particle
  draws, no runtime errors and exact user saves. Observed private warm-frame
  readiness is 42.52 seconds; scratch is removed. This accepts the independent
  lifetime correction for guarded installation. Visible-route file persistence
  was separately accepted by the later two-process check recorded above.
  [Lifetime candidate real input](../.local/mcc-native-click-crash-20261018/candidate-v7/validation.json).
- [x] Install the fixture-proven registry lifetime correction into both active
  runtimes after real input qualification. Both launcher variants in both locations
  and the actual user eight-frame route pass; saves and all shaders remain exact.
  Rollback preimages are retained and scratch is removed. This installation check
  did not certify visible/no-screenshot cache export; that separate acceptance
  now has the actual-file and subsequent-consumption evidence recorded above.
  [Installed lifetime correction](../.local/mcc-native-click-crash-20261018/lifetime-installed-v1/validation.json).
- [x] Publish compiled V13 with the lifetime correction and V3 MCC-only importer.
  Both relocated launchers, actual eight-frame gameplay, all 43 bundled-Python
  importer controls and every archive checksum pass. No reference or development
  checkout opens occur; all 97 shaders remain exact. Temporary build/cache links,
  traces and frames are removed. This supersedes V11/V12's registry lifetime
  implementation; their artifacts remain historical rollback evidence. Slow
  startup and full gameplay/parity stay open. The later installed visible-route
  file persistence and reuse check passes separately, as recorded above.
  [V13 portable validation](../.local/mcc-native-click-crash-20261018/package-v13/validation.json)
  · [Compiled V13](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261019-v13.zip).
- [x] Synchronize the user's existing extracted V9 folder to the verified V13
  package files without importing another game-data copy. Eleven tool/data/
  capability/metadata files update; all V13 archive checksums then match and
  both launchers retain native-test availability/full-ready false. Existing
  cache, saves and extra historical evidence remain intact. Checked rollback
  copies total 526,218 logical bytes, and update scratch is removed.
  [Extracted-folder V13 synchronization](../.local/mcc-native-click-crash-20261018/sync-extracted-v13/validation.json).
- [x] Narrow retained-renderer reconstruction from 73 differing sections to 13
  by restoring only the exact 24-byte private texture-mask vector documented in
  its own debug layout. No new sections appear; two old sections remain absent.
  This is a scratch-only baseline probe, not a replacement renderer or startup
  improvement. Scratch is removed and ordinary renderer/shaders remain unchanged.
  Inspect the remaining mask-related logic and accept no replacement unless the
  complete text/constant baseline matches the retained object.
  [Renderer layout reconstruction control](../.local/mcc-native-click-crash-20261018/renderer-startup-v4/baseline.json).
- [x] Restore the temporary baseline's existing texture-mask selection uses and
  the corresponding local graphics-mode definition scope. All 975 retained
  text/constant sections now exist with no new sections, and remaining mismatches
  fall to eight. The earlier compile-scope rejection is preserved; both probe
  scratch directories are removed. This still does not qualify a renderer
  replacement or hash optimization; compare the remaining code/relocations first.
  [Mask-logic reconstruction control](../.local/mcc-native-click-crash-20261018/renderer-startup-v6/baseline.json).
- [x] Reduce the temporary reconstruction to seven differing sections by
  restoring the independently selected mask material in geometry residency.
  A bounded diagnostic compares code/relocations and identifies two retained-only
  particle-subrenderer lifetime messages; the memory-preference substitution is
  rejected because it increases mismatches. All probes remove their scratch and
  retain only small diagnostics. No native/shader replacement is promoted.
  [Seven-section control](../.local/mcc-native-click-crash-20261018/renderer-startup-v9/baseline.json).
- [ ] Measure and reduce slow native startup through the real launcher/package
  path while preserving all existing shaders and Xbox TU1.5 behavior. Separate
  window creation from the first playable frame; do not call a bounded hidden
  capture a qualified interactive launch.

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
- [x] Complete a visually valid bounded native gameplay frame after the
  compatibility updates, including first-person geometry and HUD presentation.
  Root inspected V18's complete Lockout frame; its decoded pixels and save
  outputs match the accepted V16 fixed-tick run. Full material/scene and Xbox
  framebuffer parity remain separate open gates.
  [V18 fixed-tick acceptance](../.local/mcc-material-index-audit-20261019/successor-v18-executor-v1/validation.json).
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

- [x] Recover the actual tag1009 dispatch from both authenticated MCC outer
  frontend dispatchers. Each bounds-checks index9 and uses table`1432DB020`,
  cell`1432DB068`, to reach`140613CBC`. Its complete181-instruction body
  identifies `DefineExternalImage` and calls the complete184-instruction child
  at`140613A18`. Only small static receipts/disassembly are retained; temporary
  disassembler dependency and scratch are removed. Descriptor fields, active
  image binding, the missing mask and music resource remain open; this does
  not justify product wiring or fabricated loading assets.
  [Checked dispatch table](../.local/mcc-loading-production-20261018/tag1009-static-20261019-v1/case9.json)
  · [External-image child](../.local/mcc-loading-production-20261018/tag1009-static-20261019-v1/image-child.json).
- [x] Seal the two actual loading-GFX external-image records against the direct
  scalar/string readers: resource`90001`, format13,92x92, name
  `loading_screen_i7.tga`; resource`90002`, format13,1280x720, name
  `loading_screen_i1.tga`. Both exports are empty. The56-byte owner stores name
  at`0x20` and export at`0x30`; the later creator's`0x48` field belongs to a
  different structure. Resource registration`1405F0EFC` and that adapter remain
  unproved, so no conclusion about the creator's input string or image binding
  is admitted. All probe/dependency scratch is removed; no asset, executable
  or shader changes are made.
  [Raw record and owner evidence](../.local/mcc-loading-production-20261018/tag1009-static-20261019-v1/validation.json).
- [x] Decode the proved external-image registration owner`1405F0EFC` completely:
  264bytes/69instructions. Its constructor-consumed factory slots`+8/+10`
  resolve to`1405A6854`/`1405ABCAC`; later slots and the image-creator adapter
  are not inferred. Small source/disassembly receipts and the corrected local
  probe-setup failure are retained; temporary dependency/scratch is removed.
  [Registration evidence](../.local/mcc-loading-production-20261018/tag1009-registration-20261019-v1/registration.json).
- [x] Qualify the registration queue without assuming a creator slot. The
  `+130` search yields only its known writer; proved pending head`+398` instead
  flows through`1405F36F8` to a new node`+28` while count`+37C` enters`+10`.
  Frame-like nodes use load-data`+100/+108` and next`+30`. The next missing
  bridge is that new node's`+28` consumer. No factory create call, creator
  descriptor, mask or ambience is inferred; extraction stops at this verified
  boundary. Small receipts remain, and all dependency/probe scratch is removed.
  [Qualified registration boundary](../.local/mcc-loading-production-20261018/tag1009-registration-20261019-v1/validation.json)
  · [Pending-head flow](../.local/mcc-loading-production-20261018/tag1009-registration-20261019-v1/pending-xrefs.json).
- [x] Find the actual queued-frame consumer in the same bounded GFx cohort.
  Owner`1405F5148` is3,115bytes with a pinned source hash; its reads at
  `5F51F1/5F5266` take load-data`+100` into r14 and advances at
  `5F51E3/5F5258` follow node`+30`, matching the proved queue. Nine qualified
  instruction-aligned read sites span eight functions inside the fixed655,360
  byte region. Scratch is removed. The node`+28`/factory dispatch and final
  image descriptor remain unproved; no image role or loading mask is inferred.
  [Qualified queue consumer](../.local/mcc-loading-production-20261018/tag1009-queued-consumer-20261019-v1/queue-consumers.json).
- [x] Prove the actual stored-factory dispatch inside that queue consumer.
  At`5F57E7` queued node`+28` supplies the registration chain; count`+10` and
  registration next`+18` bound traversal. `5F5829/5831` load its factory/vtable,
  `5F5840` supplies raw owner`[rbx+8]` in rdx, and`5F5844` calls virtual`+18`.
  This authenticates that precise factory slot; it is no longer inferred from
  adjacent entries. Slot target, raw-owner translation and creator descriptor
  remain the next proof, with no image-role/mask/audio conclusion yet.
  [Actual factory call](../.local/mcc-loading-production-20261018/tag1009-queued-consumer-20261019-v1/consumer.json).
- [x] Resolve that precise factory slot to`1405F14B0`, a complete1,722-byte/
  460-instruction owner with pinned source bytes. Its conditional branch for
  nonzero raw-owner`+18` creates a descriptor at rsp`+40`, assigns raw`+30` to
  descriptor`+48`, then calls image-creator virtual`+18`. The zero branch uses
  another descriptor and virtual`+10`. This proves a conditional structure
  bridge, not the real loading records' selected branch, assignment semantics
  or bound image-creator implementation. All three probe scratch trees are
  removed. Those remaining bindings still precede any image/mask inference.
  [Actual factory and conditional bridge](../.local/mcc-loading-production-20261018/tag1009-queued-consumer-20261019-v1/factory-dispatch.json).
- [x] Decode the descriptor assignment helper`140634748` and tag1000 header
  parser`1405ECE94` completely. Assignment retains destination tag bits and
  either shares/refcounts matching-allocator source backing or copies its exact
  length before releasing old backing. The parser reads the actual preceding
  GFX tag1000 version`0401`, flags, format and strings, then calls metadata
  setter`1405ECD64` into stream`+2B0`. Header-state flow into data`+80` remains
  unproved, so the actual loading branch is still open. Scratch is removed;
  small primary disassembly/receipts remain, with no product or role inference.
  [Exact header/assignment owners](../.local/mcc-loading-production-20261018/tag1009-field-state-20261019-v1/fields.json).
- [ ] Resolve the bound image-creator implementation before pixel/resource binding.
- [x] Seal the canonical real-record branch: actual preceding tag1000 format13
  flows through parser/setter and copy caller into data`+80`, then constructor
  raw`+18` and factory nonzero→creator`+18`. Assignment semantics and actual
  nonnull zero-length backing are proved; empty exports do not imply a null
  pointer or zero branch. The47-byte parser log leaf preserves the argument on
  its normal cookie path. All four private environments are removed. This is
  successful canonical-parse proof for the exact records, not live MCC invocation
  or final creator/pixel/resource/mask/audio acceptance. Creator binding remains
  open in the preceding gate.
  [Sealed real-record branch](../.local/mcc-loading-production-20261018/tag1009-field-state-20261019-v1/validation.json).
- [x] Prove the exact original load-state identity through context and bind
  process storage at`+18`, distinct creator-owner construction/publication at
  state`+10`, and the default owner's seven requested IDs
  `[12,13,14,18,19,30,31]`. The third returned pointer goes to owner`+20`;
  explicit copying preserves it with refcounting. Known registered creator
  vtable slots separately resolve`+18` to`8EC954` and`+10` to`5E9814`.
  All six private extraction environments are removed. The bulk-state query
  implementation and source interface remain required before identifying that
  actual third result with the registered creator; pixels/mask/audio stay open.
  [Exact load-state/creator ownership](../.local/mcc-loading-production-20261018/tag1009-loadstate-binding-20261019-v1/validation.json).
- [x] Prove the exact bulk-state lookup and registration chain. Public loader,
  implementation interface and backing owner forward the same IDs/pointers;
  lookup matches embedded type, advances IDs4bytes/results8bytes and places
  ID14's result in the creator field. This conditionally binds the registered
  type14 creator and its`+18→8EC954` slot for the same successful loader with
  no override/replacement. The actual format13/empty-export branch then reaches
  fallback`5E9B98`. All eight extraction environments are removed. Active UI
  instance ownership and actual external resource bytes remain unproved; no
  pixels, mask, audio or product wiring are inferred from this static chain.
  [Bulk query and conditional creator binding](../.local/mcc-loading-production-20261018/tag1009-loadstate-binding-20261019-v1/bulk-validation.json).
- [x] Qualify canonical UI file-opener setup and the empty-export fallback.
  Initializer`8AB59C` installs UI-client`+390` as state12; bulk result0 reaches
  descriptor`+20` and file-opener`+8→8EBBF8`. Fallback`5E9B98` first tries the
  supplied/rebased name, then a `.dds` alternate only on failure. The exact
  opener strips directory/final extension before the hash; eight actual-source
  CPU basename controls pass. Conditional record basenames i7/i1 produce keys
  `56DBA897/4C5DE525` in namespace`442A39D9`, passed to provider`+60`.
  Name rebasing, actual provider row/resource bytes and active loading requests
  still require proof; no pack/image-role/pixel/audio conclusion is inferred.
  All five private environments are removed, with no large artifact or product
  edits. [Canonical opener and fallback](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/validation.json).
- [x] Qualify the actual namespace registry lookup with a branch-complete
  source CFG. Getter`889394` lazily returns global`14400C200`;`889444` caches
  the provider/type at registry`+58/+60` and returns found node minus8.
  `5A5534` is a tree lookup, with key`+18`, children`+8/+10` and registry
  sentinel`+20`; its74 reachable bytes are authenticated despite lacking a
  PE unwind entry. Initial first-return/body assumptions were corrected before
  certification. All five private environments are removed. Registry provider
  registration, actual asset rows and active loading invocation stay open.
  [Exact registry owners](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/registry-execution.json).
- [x] Scan exact registry-global references in the authenticated42,370,020-byte
  text cohort, accepting all nine candidates only at complete primary-fragment
  instruction boundaries. The two newly reached owners remove providers/drain
  auxiliary lists and reset state; neither binds an asset provider. Known getter
  `889394` remains the next direct-call source frontier. This is a bounded
  initialization/teardown result, not proof that no indirect registration exists.
  Both scanner/owner private environments are removed, with only small source
  reports retained. [Exact-global cohort](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/registry-global-xrefs.json)
  · [Teardown owners](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/global-owner-bodies.json).
- [x] Qualify221 exact direct getter callers at primary instruction boundaries
  and their bounded successors. Actual insertion owner`8888D0` passes the
  returned registry/provider to`5A5170`; removal uses`5A52B4`. Of36 qualified
  constructor callers, only`0E14FC` supplies exact namespace`442A39D9`: it
  zeroes the same global288-byte owner, registers it and installs a concrete
  vtable. Constructor owner`+20` matches tree key/node`+18`; lookup returns
  that same provider. All completed environments are removed. Exact selected
  provider`+60`, resource rows/bytes and active loading requests remain open.
  [Getter caller cohort](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/registry-getter-callers.json)
  · [Exact namespace constructor](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/provider-selection.json).
- [x] Seal the exact provider registration/query bridge: source-labeled
  `UIScreenInventory`, namespace`442A39D9`, global`143FA4350`, table`143470980`
  and consumed slot`+60→8891D0`. Resource lookup queries provider`+50`;
  successful opener reads length`+58` and relative bytes`+60`. The11 private
  environments are removed. A subsequent exact-global scan finds only three
  qualified constructor references; three no-pdata/data candidates remain
  excluded, so no resource-population absence is claimed. Its scanner is
  removed. Actual rows/bytes and loading invocation remain open; the direct
  file fallback is another required source path before image/pixel inference.
  [Sealed provider bridge](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/registry-binding-validation.json)
  · [Qualified provider-global references](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/provider-global-xrefs.json).
- [x] Qualify the opener's separate direct-file fallback and VFS calls.
  `8B2954` resolves the supplied/rebased path through VFS`143FA46C0`, creates
  a file owner and opens through the selected device's virtual`+88`. Actual
  `8B1C98` expands variables and invokes registered rewrite callbacks;
  `8B1E84` uses an optional device selector or its source-addressed default.
  The selected device and resolved path are stored on the file owner. Both
  private environments are removed. These active rewrite/selector choices,
  canonical device setup and actual byte reads still require qualification;
  no filesystem path, inventory contents or pixel role is guessed.
  [Exact file fallback](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/file-fallback-owners.json)
  · [VFS owners](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/vfs-owners.json).
- [x] Qualify the canonical default file-device constructor, open method and
  startup initialization. The exact virtual`+88` target uses the resolved path,
  UTF-8 conversion and read-existing `CreateFileW`; startup links this device
  into the VFS list and calls its installed virtual`+8` initializer. Constructor
  zeroes establish initial callback/selector state only. The bounded52-reference
  successor audit establishes no active path-hook or image-resource producer;
  indirect/later hooks remain open. Recorded scratch is removed, with no large
  artifacts or product changes. Actual loading invocation and image bytes are
  still required before a reconstruction claim.
  [Default device proof](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/default-vfs-validation.json)
  · [Actual startup dispatch](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/default-init-owner.json).
- [x] Check the source-proved basename keys against every qualified stored key
  in the four known texture packs: neither matches any of2,329 revalidated
  `NameHash` fields. The shared PCK has7,247 string-tag rows and the cache
  directory13,661; neither schema has a numeric name-hash field. The ambiguous
  previous47,247-row label is corrected. This finite negative supplies no image
  bytes or active loading owner. All three private environments are removed;
  the distinct `UIScreenInventory` producer remains open.
  [Qualified key candidates](../.local/mcc-loading-production-20261018/tag1009-key-candidates-20261019-v1/validation.json).
- [x] Qualify the primary MCC loading-audio service's initializer-linked
  default provider. Independent review authenticates67 instruction-boundary
  references, its exact initializer/global publication, vtable`+30` target and
  actual loading-event caller. Eighteen bounded x64 CPU cases/255 instructions
  cover success/null allocation, ignored arguments and start/stop dispatch.
  This exact provider reads no event/parameter payload and supplies no proved
  resource/PCM binding. Its`RET 0` leaf preserves RAX; it does not return zero.
  Live service selection, actual MCC music resources and audible loading remain
  open. Explicit fixture boundaries and extractor failures are preserved; all
  three review scratch paths are removed, with no large artifacts or engine
  edits. This negative result justifies no substitute ambience.
  [Default loading-audio provider review](../.local/mcc-loading-production-20261018/sound-service-binding-20261019-v1/independent-review-v1/validation.json).
- [x] Inventory exposed Halo2/frontend audio metadata without extracting banks.
  Twenty-four Halo2 candidates and112 exposed text files supply no exact loading
  music event. `Data/audio/soundbanks.xml` names the frontend banks; no matching
  loose banks exist. The packed owner is
  `MCC/Content/Paks/MCC-WindowsNoEditor.pak`; its4,096-byte footer and4,742,298
  byte metadata index authenticate by the footer's SHA1. Index strings name13
  banks and379 media files, including the named Windows banks. These are index
  strings, not parsed entry/event/resource bindings. All scratch/index copies
  are removed with no large artifact, cache/import or engine edits. Another
  game's loading FSB remains outside Halo2 authority.
  [Packed frontend audio inventory](../.local/mcc-loading-production-20261018/frontend-audio-inventory-20261019-v1/validation.json).
- [x] Parse all30,667 frontend pak entries from source-qualified version7
  serialization, consuming the complete4,742,298-byte index with its exact
  SHA1. The three Windows bank entries match their duplicated53-byte headers:
  Init1,483bytes, Main_Bank4,008,466bytes and Update1 32bytes; method0/flags0.
  Localized banks remain separate. Actual index records also identify
  `SoundbanksInfo.xml`, `Main_Bank.txt` and `Init.txt`, whose payloads remain
  unread at this gate. No large bank/media file is extracted and larger payload
  hashes are unverified. All private downloads/scratch and six generated source
  excerpt copies are removed; only independent format facts/provenance remain.
  Event/media binding and original loading-audio identity remain open.
  [Complete index and bank-header proof](../.local/mcc-loading-production-20261018/packed-frontend-banks-20261019-v1/validation.json).
- [x] Authenticate and parse the three packed Windows audio metadata payloads
  (285,025bytes) after matching entry headers and SHA1. Authored loading media
  `236657329` is named `MUS_Menu_Loading_Screen_01` and maps to
  `236657329.wem`. The separate `Play_Music` event is `2932040671`; XML supplies
  bank-level membership and no explicit event-to-media selection edge. Names do
  not prove the active Halo2 request or original loading-audio identity. No bank
  or WEM payload is read at this gate; scratch is removed without large
  retention or product changes. The actual bank graph is the next boundary.
  [Authenticated metadata](../.local/mcc-loading-production-20261018/frontend-event-metadata-20261019-v1/validation.json).
- [x] Authenticate and parse the actual Windows Main_Bank version132 graph.
  All286 HIRC record identities/extents and301 relevant numeric fields are
  independently checked against raw offsets. The directed path is
  `Play_Music`→Play action→music switch→playlist→segment→track→media236657329,
  conditional on Switch group2949940656/key1792793154. This establishes an
  authored conditional path; actual frontend switch publication, audible
  behavior and OG/Vista identity remain open. Three heuristic display-union
  meanings are excluded from qualified facts. Bank/source-body scratch is
  removed with no WEM reads, large retention or product changes.
  [Actual authored graph](../.local/mcc-loading-production-20261018/frontend-bank-graph-20261019-v1/validation.json).
- [x] Authenticate the unique packed Windows media`236657329.wem` by exact
  entry/header and payload SHA1. Its3,356,709bytes contain Wwise custom Vorbis,
  stereo44,100Hz, with2,744,320 declared samples (62.22948seconds). The header
  does not uniquely establish the external codebook variant; installed ffprobe
  cannot decode it. No playback, PCM or audible equivalence is claimed. Media
  and downloaded source bodies are removed with scratch; no other media payload
  or product data is read/changed. A qualified decoder is the next boundary.
  [Exact packed media/header](../.local/mcc-loading-production-20261018/loading-wem-20261019-v1/validation.json).
- [x] Decode the exact loading WEM using pinned, unpatched upstream vgmstream
  and existing system Vorbis/Ogg libraries. Its normal initializer admits
  WWVORBIS_V62/aoTuV603 without a forced variant. Float output has exactly
  2,744,320 stereo frames at44,100Hz, all5,488,640 samples finite and nonzero
  output; standalone WEM loop metadata is disabled. Source/decoder/build/input
  and21.95MB float output are removed with all five scratch paths. No playback,
  importer/runtime integration, live switch or original-audio identity is
  claimed. Retained evidence is small provenance, hashes and diagnostics only.
  [Actual admitted decode](../.local/mcc-loading-production-20261018/loading-wem-decode-20261019-v1/validation.json).
- [x] Qualify the actual frontend's music name-hash and conditional request
  publication path. Genuine hash instructions produce the authored group/key
  and Play_Music/Menu_Music IDs; the numeric receiver publishes command8's
  group/key and64-bit object ID. Explicit conversion/clock/allocator fixture
  boundaries are recorded. The producer of selector5, real queue consumption
  and live service selection remain unproved; seven nearby leaf candidates
  are excluded rather than called loading callbacks. All14 scratch paths are
  removed, with only small source/proof diagnostics retained and no playback
  or product change.
  [Frontend conditional request owner](../.local/mcc-loading-production-20261018/frontend-audio-owner-20261019-v1/validation.json).
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
- [x] Authenticate the exact TU1.5 damage-table consumer before interpreting
  row-order differences. Complete`D8110` consumer,`1408B0` binary search and
  `D8100` comparator total112 instructions per version, matched to retail
  through explicit call/callback/global/literal operands rather than a uniform
  address delta. Actual calls/callback pushes, identity float`3F800000` and
  genuine16-instruction publication of matg global`4ECC1C` are pinned. Scratch
  is removed; only small instruction/proof reports remain. This is source
  authority, not yet actual MCC-table execution or material-index equivalence.
  [TU1.5 damage lookup authority](../.local/mcc-damage-table-consumer-20261019/authority-v1/validation.json).
- [x] Execute the authentic TU1.5 damage-table consumer across all38 paired
  original/MCC maps. The1,299,092 deduplicated raw-key CPU cases match the exact
  selected native kernel bit-for-bit;618,010 common name-qualified cases agree
  across platforms. Authored row order, StringIDs and scalars stay unchanged,
  with existing Cache owner-pointer rebasing explicitly checked. The all/emp
  and Mombasa cutting/burning reorders require no mutation for this consumer.
  Platform-only rows/materials and numeric material-index consumers remain
  separate open boundaries. The temporary104MB probe, banks and maps are
  removed; no product/shader/runtime change was needed.
  [Actual38-map consumer matrix](../.local/mcc-damage-table-consumer-20261019/matrix-v1/validation.json).
- [x] Re-read and authenticate all38 paired numeric material tables. Every
  shared material keeps its index/order, parent identity/index and armor name.
  Twenty-six MCC contexts append only `tough_inorganic_plastic_container` at
  index182, parent index39; twelve contexts have identical ordered identities.
  This rules out an insertion/reorder fix for these tables. Reachability of
  the appended row and actual numeric-index consumers remain open. Both
  private environments are removed, with no raw banks or product edits.
  [Paired material inventory](../.local/mcc-material-index-audit-20261019/inventory-v1/validation.json).
- [x] Demonstrate one actual collision-content mismatch with the selected V16
  native provider. Floodzone's `door_key_tunnel_rusted` retains identical local
  material names/order and surface mappings, but two rows bind MCC global
  material0/default instead of original69/`hard_metal_thick`. Native
  `ModelCollision` output preserves that difference;592 and360 surfaces select
  the affected rows. This establishes a required authored mapping correction,
  separate from the harmless appended global material. Active scene contacts
  and an original TU1.5 machine-consumer comparison remain open. Probe/link/map
  scratch is removed and no product patch is applied at this gate.
  [Actual collision provider comparison](../.local/mcc-material-index-audit-20261019/consumer-v1/door-validation.json).
- [x] Qualify the minimal collision-material correction and its insertion seam.
  Fresh43-MCC/38-original inventory finds the owner only in Floodzone. Full
  scenario/globals/model fingerprints and material69 identity guard exactly
  two signed16 stores,0→69, at owner offsets`120`/`134`. All other owner bytes
  stay exact. Floodzone's existing Gauss correction must remain independently
  composable: preflight both components before publishing either. Unknown or
  modded component tuples stay unchanged. All seven probe scratch paths are
  removed; implementation/native post-conversion acceptance remain next.
  [Authenticated guards](../.local/mcc-material-index-audit-20261019/correction-guards-v1/source-guards.json)
  · [Insertion seam](../.local/mcc-material-index-audit-20261019/correction-guards-v1/seam-proposal.json).
- [x] Implement the independent guarded Floodzone material planner and compose
  it with Gauss before any ordinary internal-data publication. Exact two16-bit
  stores, full owner/material guards, all-component preflight and overlap checks
  pass actual-bank fixtures; altered material owners preserve Gauss independently.
  Strict focused CMake, explicitly instrumented ASan/UBSan and current-header
  Cache compilation pass. The option-only sanitizer run is excluded from
  sanitizer evidence. Scratch/build trees are removed; V16 frozen providers,
  shaders, runtimes and ZIPs stay unchanged. A compatible successor provider
  pair and actual native post-conversion acceptance remain next.
  [Implemented/tested source handoff](../.local/mcc-material-index-audit-20261019/implementation-v1/handoff.json).
- [x] Independently review the Floodzone planner and reopen both authenticated
  source maps. Original material indices99,69,69 versus MCC99,0,0 require only
  the two signed16 stores; they reproduce the exact target model-owner hash
  while preserving every other byte. Full salted owner binding, unknown/modded
  tuples, idempotence, Gauss independence and atomic composition have no
  concrete review findings. Both review scratch paths are removed. This adds
  no native ABI/gameplay acceptance claim; the frozen-provider probe is next.
  [Independent source/data review](../.local/mcc-material-index-audit-20261019/independent-review-v1/validation.json).
- [x] Validate the correction through normal Cache and the exact selected
  collision provider across all43 V16 map inputs. The other42 complete backing
  stores/directories are identical; Floodzone changes only the two intended
  halfwords and yields material indices99,69,69 with unchanged592/360 affected
  surface counts. Its Gauss result stays exact. Three actual modified-data
  controls preserve their backing and independent Gauss behavior. The two new
  components and probe reuse106 exact selected header dependencies; all313
  other linked inputs and original316 files, runtime/maps/97 shaders remain
  exact. All objects/probe ELFs/maps/banks/snapshots are removed. This closes
  native content acceptance; a retained successor client and live regression
  remain next, with V17 still using the unchanged V16 runtime.
  [Normal native Cache acceptance](../.local/mcc-material-index-audit-20261019/postconversion-v1/handoff.json).
- [x] Prepare the compatible two-object full-client successor after the 43-map
  content proof. The exact reviewed V18 plan preserves 314 V16 providers,
  including Main, and 106 selected dependencies while replacing only Cache and
  the map planner. Paired tick60 image/save/shot-state and finite/nonzero audio
  acceptance must precede retention. The planned owned candidate is about 125 MiB
  of necessary build inputs/runtime/link ownership and source snapshots; no
  candidate or compiled bytes exist at preparation. Root checkpoints this
  plan for executor preparation; inspect the executor before running it. The
  separate nine-phase input test, installation and ZIP remain later gates.
  [Prepared V18 successor](../.local/mcc-material-index-audit-20261019/successor-v18-preparation-v1/handoff.json).
- [x] Review the concrete V18 executor and its failure/cancellation ownership.
  It uses only the two prepared compiler commands and unchanged full-client
  inputs; all project dependencies must match the selected projection. Exact
  paired acceptance precedes atomic publication of a complete owned candidate.
  Mesa/NVIDIA/application caches are private, loader/cache-bypass overrides are
  cleared, and the wrapper/finish record actual exits and verify scratch
  removal after either result. Root checkpoints the sealed executor; actual
  compilation, native acceptance and retention remain the next gate.
  [Reviewed executor plan](../.local/mcc-material-index-audit-20261019/successor-v18-executor-v1/executor-plan.json).
- [x] Build and retain the compatible V18 full client after exact paired
  acceptance. Both clients complete tick 60 with identical decoded frame,
  complete save hashes and shot/capture state; each emits finite, nonzero
  stereo callback data. Only Cache/planner change among 316 inputs. The accepted
  ELF is `09426c7b…7553f`; all original providers, 43 maps, 97 shaders, selected
  runtimes, saves and warm cache seed remain exact. Required coherent candidate
  inputs/runtime/link ownership/frozen sources and two small reference PNGs
  total 132,437,304 bytes. Private objects, raw frames/audio, GPU caches and both
  runner scratch levels are removed; finish independently verifies the actual
  exit and candidate inventory. A pre-main Python helper-name collision is
  preserved and resolved with qualified inherited safe-path configuration;
  sealed helper bytes stay unchanged. Nine-phase real input and delivery remain
  separate gates; V17 still ships the accepted V16 runtime.
  [Accepted retained V18](../.local/mcc-material-index-audit-20261019/successor-v18-executor-v1/finish.json).
- [x] Prepare and independently review the exact nine-stage V18 input check.
  Its gate binds the accepted cohort/runtime and fixedtick receipt before any
  display or subprocess. Pointer/focus/click/held fire/look/movement events
  keep the existing recipe; held input releases and child cleanup cover failure.
  Two stale ancestor assumptions are corrected: P uses its actual V16 payload,
  and maps use the exact selected cohort while P's selector is preserved
  separately. Private saves/GPU/application caches and all original inputs are
  checked. Preparation scratch is removed. Root checkpoints actual accepted
  pins for execution; live survival remains unclaimed until it passes.
  [Prepared input helper](../.local/mcc-material-index-audit-20261019/successor-v18-interactive-preparation-v1/handoff.json).
- [x] Execute all nine actual V18 input stages and verify live cleanup. The
  process survives pointer entry, focus, click/release, held fire/release, look
  and forward movement/release, with four first-person particle draws after
  actual fire events and no native error/unmapped/crash logs. Rendered readiness
  is measured at 28.0 seconds in this run; no speed improvement is inferred.
  The client exits cleanly after intentional termination. Original/candidate
  inputs, maps, shaders, saves, selections and warm seed stay exact; children
  stop and live/closure scratch is removed. No new large artifacts are retained.
  A mistakenly invoked preparation finalizer is recorded and its original seal
  restored exactly; the separate live receipt provides this acceptance. This
  closes bounded Lockout input survival, not full scene/gameplay/audio parity.
  [Actual input and cleanup](../.local/mcc-material-index-audit-20261019/successor-v18-interactive-closure-v1/validation.json).
- [x] Prepare and review compiled V18 delivery over the immutable V17 archive.
  Only six existing paths change: payload, sidecar, validation, manifest,
  README and checksums. The other 4,246 entries, including all importer/Python
  tools, 190 libraries, wrapper and 97 shaders, must match V17. Its 46 importer
  controls are explicitly inherited; new payload acceptance requires fresh
  relocated launcher-info/eight-frame gameplay and reference/development/cache
  write audit. Root reviews source/runtime/save preservation and the qualified
  external-cache inode limit, then checkpoints ZIP-only execution. Preparation
  scratch is removed; actual publication remains next. P installation is outside
  this action, and full gameplay-ready stays false.
  [Sealed V18 package preparation](../.local/mcc-material-index-audit-20261019/delivery-v18-preparation-v1/handoff.json).
- [x] Preserve the first V18 packaging failure before any ZIP publication.
  The safe-path setting used by the test runner propagated into the app's
  Python and prevented its normal sibling import. The relocated capability
  check rejects this; no eight-frame run or archive follows. Separate review
  also finds an inherited Tk demo `.ppm` that a blanket capture ban would reject.
  The minimal retry restores the app's normal child environment and applies the
  new-capture ban only to changed/new entries while preserving all inherited
  bytes. The failed receipt and original sealed helper stay intact; scratch is
  removed. Neither issue changes the accepted native build or its live tests.
  [Actual failed package attempt](../.local/mcc-material-index-audit-20261019/delivery-v18-preparation-v1/package-validation.json).
- [x] Qualify the minimal V18 packaging retry. An isolated red/green control
  reproduces the sibling-import failure and confirms that removing only the
  child environment flag preserves the packager's safe import mode. The actual
  inherited Tk image is accepted unchanged while a newly added raw capture is
  rejected. Original V1 code/metadata/failure remain exact; all preparation and
  control scratch is removed. Root reviews the two-line policy changes and
  checkpoints the separately sealed V2 ZIP-only workflow. Actual publication
  remains next; all runtime/save/source and entry preservation checks stay.
  [Qualified minimal retry](../.local/mcc-material-index-audit-20261019/delivery-v18-preparation-v2/handoff.json).
- [x] Publish the compiled V18 ZIP after the minimal retry passes. Both relocated
  launchers offer the native test with full readiness false; actual eight-frame
  Lockout gameplay succeeds through the bundled loader. The file audit sees no
  reference/development opens or private hardlink-cache writes. All 4,251 archive
  checksums pass; only the six reviewed paths change, preserving 4,246 V17 entries,
  190 libraries and 97 shaders. Runtime/save/source and qualified cache guards
  pass. Test scratch is removed; the requested ZIP/checksum are retained.
  This publishes the accepted V18 runtime, leaving the installed P V17 folder
  and caches intact. Independent archive review follows.
  [Actual compiled V18](../.local/mcc-material-index-audit-20261019/delivery-v18-preparation-v2/package-validation.json).
- [x] Independently stream-verify the published V18 archive without extraction
  or application execution. All 4,252 members/4,251 checksum rows pass; exactly
  six existing entries change and the other 4,246 retain their bytes and modes.
  All 97 shaders, 190 libraries, wrapper, Python and importer remain exact.
  Runtime authority identifies V18 while the 46 importer controls are explicitly
  inherited from V17; full readiness remains false. Review scratch is removed
  and no large review artifacts are retained. P remains installed at V17.
  [Independent V18 archive review](../.local/mcc-material-index-audit-20261019/delivery-v18-archive-review-v1/validation.json).
- [x] Finish V18's scoped retention/scratch audit. All 26 recorded completed
  V18 scratch owners and the audit caller's own scratch are absent. The required
  coherent 119-file client/input/reference tree retains 132,437,304 bytes;
  requested ZIP/checksum retain 183,623,403 bytes, totaling 316,060,707 bytes.
  V17 delivery, P rollback preimages, frozen providers, runtime selections and
  saves remain exact. Active foot work is outside this audit. No historical
  deletion or full-cache rehash is performed; prior cache guard qualifications
  remain explicit. The initial alias-check diagnostic is preserved separately.
  [V18 retained paths and closure](../.local/mcc-material-index-audit-20261019/closure-v1/validation.json).
- [x] Authenticate the original material-effect foot lookup before treating
  ordinal row differences as defects. Five complete retail/TU1.1/TU1.5 owners,
  359 instructions per version and explicit cross-version operands establish signed 16-bit
  keys at row offset `0x14`, forward first-match lookup and globals parent traversal.
  This is source authority only; keyed paired-map execution is next. Probe
  scratch is removed and no product changes are made.
  [Foot lookup authority](../.local/mcc-foot-table-key-audit-20261019/authority-v1/validation.json).
- [x] Complete the bounded keyed foot-table inventory across 38 paired maps.
  All 76 native Cache views are complete and bind the same primary/first globals
  owner. Of 2,309 earlier ordinal candidates, 1,162 preserve the original first
  effective keyed row at a different ordinal, 1,070 need keyed assessment and
  77 contain an invalid key at one ordinal. A resolved Chief walk case keeps
  the classic primary sound while MCC adds a remastered secondary `snd!`
  reference where Xbox has null in 27 maps. That is a data difference; original
  CPU output and actual native caller group admission must precede any audible
  defect claim or conversion decision. The modeled variant counters include
  explicit unresolved/cyclic boundaries and are not CPU/live parity evidence.
  All recorded scratch is removed, protected inputs stay exact and no product
  changes or large retained artifacts result.
  [Keyed inventory and boundaries](../.local/mcc-foot-table-key-audit-20261019/keyed-inventory-v1/summary.json)
  · [Cleanup](../.local/mcc-foot-table-key-audit-20261019/keyed-inventory-v1/cleanup.json).
- [x] Confirm the resolved Chief walk case with unchanged original TU1.5 CPU
  code and exact selected native providers. Both platforms execute 218 original
  instruction addresses and match all six native selection outputs. Xbox has
  the eight-byte null reference `FFFFFFFFFFFFFFFF`; MCC's secondary resolves to the remastered `snd!`, with the
  same named classic primary. The selected gait/audio path admits that secondary
  without slot/path suppression; catalog decoding and audibility remain open.
  Backing controls execute code-write rejection before mutation and prove added
  memory regions disjoint from all original image/default/other regions. Only
  test backing admission changes, with original instructions/owners unchanged.
  All recorded scratch is removed; only small proofs/runner inputs remain.
  This is one Lockout case, not a 27-map CPU matrix or an implemented conversion.
  [Original/native confirmation](../.local/mcc-foot-table-key-audit-20261019/cpu-v1/summary.json)
  · [Cleanup](../.local/mcc-foot-table-key-audit-20261019/cpu-v1/cleanup.json).
- [x] Prepare exact pre-conversion guards for the proven Chief secondary sound
  reference in 27 stock contexts. Raw scenario/globals/foot owners, first signed
  material key and full salted support identities qualify a single eight-byte
  null restoration per context. Already-null targets require active primary
  metadata, with no dependency on the now-inactive secondary. The proposed
  independent Cache component shares all-component preflight and leaves Gauss
  and Floodzone qualification independent. All proposed write intervals are
  disjoint. A separate census identifies 2,345 physical map-context fields in
  90 named classes for possible later batching; those fields are not production
  profiles or additional CPU coverage. Product sources remain unchanged and
  all recorded scratch is removed. Independent review and implementation follow.
  [Prepared profiles and seam](../.local/mcc-foot-table-key-audit-20261019/profiles-v2/handoff.json).
- [x] Root review pins the final profile table, conditional identity policy,
  raw 27-context qualification and transaction seam. Authorize the narrow
  one-field-per-context planner and focused validation; broader census fields
  remain review-only. No existing layouts/signatures, shaders, frozen providers
  or delivered runtimes change at this checkpoint.
  [Implementation scope checkpoint](../.local/mcc-foot-table-key-audit-20261019/profiles-v2/root-checkpoint.json).
- [x] Independently cross-check all 27 prepared compact identities against the
  retained raw qualification, reconstruct donor/target row digests and verify
  the serialized eight-byte bounds/full salted references. Source review finds
  no actionable proposal defect in native selection, conditional dependencies
  or the transaction seam. This is a bounded proposal review, without fresh
  raw-map/CPU/runtime execution; implemented behavior still needs validation.
  Review scratch is removed, with no large artifacts retained.
  [Independent profile review](../.local/mcc-foot-table-key-audit-20261019/profiles-v2-independent-review-v1/validation.json).
- [x] Implement and validate the narrow 27-context Chief reference planner and
  wire its normal Cache component. Complete owner deltas, target idempotence,
  custom/dependency preservation and shared transaction controls pass below.
  Actual native Cache readback is deferred to the guarded union continuation,
  which includes the same seed fields; no seed-only Cache acceptance is claimed.
- [x] Implement the narrow Chief planner, separate 27-profile include and exact
  group-first Cache hook without changing existing layouts/signatures or Gauss
  profiles. Focused disposable CMake/CTest passes all 27 owned foot contexts
  plus the existing 28 Gauss/one material fixtures, including owner deltas,
  active/inactive dependencies, custom identities and three-component atomic
  composition. Root source review finds no actionable defect. Sanitizer and
  actual normal-Cache readback remain separate gates; delivered V18 is unchanged.
  [Focused implementation validation](../.local/mcc-foot-table-key-audit-20261019/implementation-v1/validation-regular.json).
- [x] Freeze the narrow seed implementation after actual instrumented
  AddressSanitizer/UndefinedBehaviorSanitizer compilation, linkage and execution
  pass. All 27 foot contexts, existing Gauss/material fixtures and eight
  three-component donor/target combinations pass with zero errors. Existing
  conversion/preflight bodies, Gauss include, public layouts and signatures stay
  exact; the Cache diff adds only group-first selection and its third component.
  Every recorded build/fixture/scratch path is removed. This is pure-planner
  acceptance; normal Cache execution and runtime delivery are not claimed.
  [Frozen source/test handoff](../.local/mcc-foot-table-key-audit-20261019/implementation-v1/handoff.json)
  · [Cleanup](../.local/mcc-foot-table-key-audit-20261019/implementation-v1/cleanup.json).
- [x] Prepare coherent guards for the broader authenticated Chief class before
  expanding conversion scope: full active support metadata, deduplicated owner
  fields, complete union targets and explicit untouched/seed/union transitions.
  Reuse generic selector authority while keeping actual CPU coverage bounded.
  The exact prior census is reconstructed: 27 contexts/2,345 fields, deduplicated
  into 16 owners/1,355 physical fields and 90 named classes. Exactly three full
  owner states qualify each profile. Root independently derives every active
  support set and write-offset set: seed-shared secondary owners remain required
  until their last active candidate is removed. Direct donor-to-union needs
  2,345 context writes; seed-to-union needs 2,318; union is idempotent. Unknown
  partial/custom owners stay unrecognized. Scalar/hash metadata totals 4.22 MB;
  no banks/binaries/media or new CPU/runtime execution are retained. All recorded
  preparation and root review scratch is removed. Implementation remains next.
  [Batch handoff](../.local/mcc-foot-table-key-audit-20261019/batch-profiles-v1/handoff.json)
  · [Root finite-state/dependency review](../.local/mcc-foot-table-key-audit-20261019/batch-profiles-v1/root-review.json).
- [x] After the narrow seed's instrumented acceptance, checkpoint the exact
  finite batch implementation. Replace its private planner/table while keeping
  the public declaration, Cache hook and shared transaction; do not add a fourth
  component. Require all 27 untouched/seed/union transitions and state-aware
  support preservation tests before separately gated normal Cache execution.
  Full-client rebuild and delivery remain deferred until conversion acceptance.
  [Batch implementation scope](../.local/mcc-foot-table-key-audit-20261019/batch-profiles-v1/root-implementation-checkpoint.json).
- [x] Implement and validate the guarded Chief union conversion, then verify
  all selected native Cache and direct selector outputs before runtime delivery.
- [x] Replace the narrow private foot planner/table with the exact guarded
  union conversion. The existing public hook and Cache component remain
  unchanged. Focused CMake/CTest passes all 27 complete donor/seed/union owner
  transitions, exact 2,345/2,318 field sets, all 16 owners' active-support
  mutation/missing/full-salt controls, inactive preservation, custom partials,
  alias/key/bounds controls and 12 three-component transaction combinations.
  The generated 716,757-byte include contains scalar/hash metadata. Actual
  sanitizer execution, independent final review and native Cache acceptance
  remain next; no runtime or delivery artifact has been replaced.
  [Batch focused validation](../.local/mcc-foot-table-key-audit-20261019/implementation-batch-v1/validation-regular.json).
- [x] Freeze the batch after actual instrumented ASan/UBSan compilation,
  linkage and execution pass. All 81 context/state cases pass: donor-to-union
  covers 2,345 fields, seed-to-union 2,318, and union has zero writes. Every
  required support owner/state is exercised for mutation, absence and foreign
  salt; inactive support and custom/partial controls also pass. Existing Cache
  hook, public layouts/signatures, Gauss/material/preflight bodies and Gauss
  include stay exact. All recorded build/fixture/scratch paths are removed;
  six small seed preimages remain required transition references. Native Cache
  execution, independent final review and runtime delivery remain separate.
  [Frozen batch/source-test handoff](../.local/mcc-foot-table-key-audit-20261019/implementation-batch-v1/handoff.json)
  · [Cleanup](../.local/mcc-foot-table-key-audit-20261019/implementation-batch-v1/cleanup.json).
- [x] Independently review the final source and parse every generated C++
  initializer against sealed profiles: 27 contexts, 16 owners, 1,355 physical
  rows/2,345 context fields and 1,030 unique support records match. Final regular,
  sanitizer and handoff pins agree. Finite-state admission, shared seed support
  dependencies, signed-first-key traversal, custom preservation and atomic
  publication have no actionable findings. Cache/public/Gauss/material/preflight
  contracts remain exact. Only a bounded metadata/source review runs; its
  scratch is removed, without raw-map/CPU/native/suite reruns or large outputs.
  [Final independent product review](../.local/mcc-foot-table-key-audit-20261019/final-product-review-v1/validation.json).
- [x] Record the independently caught native-Cache harness control defect
  before execution. Lockout's Chief/support owners live in `shared.map`; the
  initial control recipe incorrectly patched local `lockout.map` before linking
  an unchanged shared bank. Preserve the sealed V1 preparation as unexecuted
  evidence. A minimal V2 must construct private mutation banks by each exact
  authenticated source owner and verify their beforeimages/physical offsets.
  Stock comparison design and product sources are unaffected; native execution
  remains gated until the corrected preparation and final product checks pass.
- [x] Prepare and root-review the corrected V2 native-Cache comparison. The old
  Lockout recipe reproduces an out-of-bounds physical offset; all eight corrected
  recipes authenticate full donor/support owners in their actual selected bank
  and verify physical beforeimages/readback. Six use private `shared.map` and two
  use private Floodzone data, with unchanged companions linked for loading.
  Product regular/ASan/source review and all helper/header/profile pins agree.
  Root checkpoints only the 43-map/2,345-selector tiny-probe execution; no full
  client, installation or packaging is authorized by this gate. V1 stays exact
  and unexecuted; all preparation/checkpoint scratch is removed.
  [Corrected preparation](../.local/mcc-foot-table-key-audit-20261019/native-cache-preparation-v2/handoff.json)
  · [Native execution checkpoint](../.local/mcc-foot-table-key-audit-20261019/native-cache-preparation-v2/root-execution-checkpoint.json).
- [x] Execute the exact selected 43-map normal Cache comparison. All 27
  qualified contexts admit the full union: 2,345 eight-byte fields occupy 18,760
  bytes and change 18,759 literal bytes. Actual selected native MaterialEffects
  reads verify every direct action/key: classic primary identities are retained,
  secondary results become null, and other audio/visual selector outputs stay
  exact. All other backing bytes, complete directories and prior Gauss/Floodzone
  consumers match baseline; the other 16 contexts remain wholly unchanged.
  All eight source-owner-correct modified/seed/union controls pass. All 316
  parent inputs/runtime, 43 selected maps and product pins remain exact. Actual
  exit is zero and its private banks/objects/probes/scratch are removed. This
  verifies vanilla Cache publication/selection, without all-row original CPU,
  sample/audible, full-client or delivery claims.
  [Actual native Cache/selector acceptance](../.local/mcc-foot-table-key-audit-20261019/native-cache-preparation-v2/validation.json)
  · [Root acceptance checkpoint](../.local/mcc-foot-table-key-audit-20261019/native-cache-preparation-v2/root-acceptance-checkpoint.json)
  · [Actual-run scratch closure](../.local/mcc-foot-table-key-audit-20261019/native-cache-closure-v1/validation.json).
- [x] Prepare and root-checkpoint the compatible V19 successor after actual
  Cache acceptance. Retain 314 accepted V18 inputs and original Main, replacing
  only Cache/planner with the tested union. Project the exact 106 parent header
  inputs plus the declaration-only gameplay header/new private include; adopt
  no unrelated current-feature ABI changes. Root binds concrete successful
  43-map/2,345-selector/control receipt fields, source tests and final review
  before build/display creation. Authorize paired fixed-tick acceptance and
  atomic required candidate retention only; input, installation and ZIP remain
  separate gates. Preparation/checkpoint scratch is removed.
  [Successor preparation](../.local/mcc-foot-table-key-audit-20261019/successor-v19-executor-v1/preparation.json)
  · [Build/acceptance checkpoint](../.local/mcc-foot-table-key-audit-20261019/successor-v19-executor-v1/root-execution-checkpoint.json).
- [x] Build and accept the coherent V19 client with exactly the two intended
  provider replacements and 314 unchanged V18 inputs. Actual paired tick60
  Lockout firing runs pass: decoded 640×480 frame, complete save outputs and
  capture state are identical; each stream has finite nonzero stereo callback
  PCM. All 43 maps, 97 shaders, selected headers, previous providers, launcher
  selections and saves remain pinned. No speed or audio-identity claim follows
  from the uncontrolled durations/PCM. The required coherent candidate and two
  small reference PNGs retain 134,141,785 bytes. Actual exit is zero; private
  compile/link/frame/audio/GPU-cache scratch, its owner parent and the receipt
  check's scratch are removed. Real input and portable delivery remain next.
  [Actual V19 build/frame acceptance](../.local/mcc-foot-table-key-audit-20261019/successor-v19-executor-v1/validation.json)
  · [Actual execution/cleanup closure](../.local/mcc-foot-table-key-audit-20261019/successor-v19-executor-v1/finish.json).
- [ ] Verify converted definitions load as vanilla internal data, including
  default campaign and multiplayer paths and mod interoperability.
- [ ] Validate original CTF behavior through its native consumer before calling
  Relic's converted4.0 word a live gameplay fix. The current handwritten match
  constructor admits only source engines2/4 (Slayer/King); Cache and original
  radius-instruction proofs establish data/operand behavior, not a CTF match.
  Preserve the other original game-mode acceptance gates as well.
- [ ] Finish native 64-bit host-pointer/ownership integration across gameplay.
- [ ] Verify required serialized widths, arithmetic, precision and timing remain unchanged.
- [x] Refresh the selected V16 host-pointer boundary audit before widening or
  rebuilding anything else. Inspected World native cache/object/player/body
  APIs already use64-bit host addresses; no new host truncation is demonstrated.
  Logical tag/CPU addresses and handles stay32-bit. The1.5GiB logical cache
  budget and128MiB importer metadata cap are capacity policies, not native
  allocator-address caps. Concurrent voice/interpolation/sound/Memory page-index
  drift does not by itself require adoption for MCC-only/64-bit support. All
  inspected selected provider/source/ELF contracts remain pinned; both scratch
  paths are removed, with no build or product/runtime mutation. This bounded
  audit does not complete ownership or mod-capacity acceptance.
  [Selected host-pointer audit](../.local/mcc-host-pointer-frontier-20261019/v1/validation.json).
- [x] Test selected Memory native-address ownership and rebinding across
  restore using the exact V16 archives and compatible frozen104-byte Memory
  layout. All59 controls pass: four actual host addresses exceed4GiB, logical
  32-bit pointer/datum identities and complete four-region checkpoint bytes
  stay exact through restore/swap/rebind/remount. Read-only, uncommitted,
  foreign-owner and overflow checks pass. Stale borrowed pointers are never
  dereferenced; legitimate allocator reuse is accepted. Only a tiny probe is
  compiled, with original archive-member witnesses. Its2,469,368bytes of
  ELF/object/map/deps and scratch are removed. No bug/product change or current
  page-index/voice-feature adoption is required by this bounded result.
  [Selected Memory ownership acceptance](../.local/mcc-host-pointer-frontier-20261019/selected-memory-v1/validation.json).
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
- [ ] Check compatibility before adopting any required subsequent receiver or
  sound-I/O API/body work. The pinned earlier receiver/World bodies establish
  the internally consistent validation cohort. Adopt a current group only for
  a demonstrated required path; preserve unrelated active feature work.
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
- [x] Implement the separate V3 Metropolis delta bundle and compose it with
  immutable V2 Zanzibar corrections in ordinary MCC import. All five images
  and 40 mip chains pass exact native cache readback; the three Metropolis
  outputs also match freshly decoded original pixels byte-for-byte. The real
  MCC-only correction child and five native readbacks open no original
  references. Both maps preserve all decoded metadata except their five local
  resource pointer/length pairs; the original texture-bank prefix and every
  other map/resource remain unchanged. Twenty bitmap ownership/rejection,
  eighteen onboarding and sixteen preparation tests pass, including last-entry
  failures before any map/bank write. Only 150,118 bytes of required delta and
  manifest are retained; inspector, raw pixels, copied maps/bank and traces
  are removed. Installed Play, saves, version9 and all 97 shaders stay exact.
  The selected older cache and version9 ZIP still use the prior correction set;
  onboarding reuse/upgrade and a full fresh import are the next integration gate.
  [Actual V3 conversion and native readback](../.local/mcc-graphics-corrections-v3-20261018/v1/validation.json),
  [Verified product handoff and cleanup](../.local/mcc-graphics-corrections-v3-20261018/v1/handoff.json).
- [x] Route setup from older correction sets to a fresh private import, while
  preserving the prior selection/cache if that import fails. A complete current
  five-profile cache still reuses its verified copy. Normal graphics import now
  ignores the old development reference registry entirely and consumes only
  MCC bytes and the qualified deltas. Twenty-one bitmap and twenty-two
  onboarding tests pass; scratch is removed. The original forty-mip native
  readback remains valid for the unchanged delta bytes. Full fresh setup and
  launcher verification are still the next integration gate.
  [Graphics upgrade and legacy-reference boundary](../.local/mcc-graphics-corrections-v3-20261018/onboarding-v1/validation.json).
- [x] Execute a fresh real launcher import with the V3 correction set.
  Ordinary `--setup-only --mcc-source` copies and verifies 883 MCC files
  (31,251,040,809 source bytes), prepares all 43 maps and applies all five
  corrections/40 mips. Both launcher-info checks and the actual eight-frame
  launcher route pass on the new cache. The subsequent actual shot exits0
  at tick60 with ammo59 and 64 rendered frames; the integration harness then
  rejects its obsolete fixed expectation of 74 frames. That is a harness
  assumption, not a gameplay failure. Raw audio/capture scratch is removed;
  the prepared cache is explicitly retained for resume without reimporting.
  The previous asset selection is restored. Finish verification with a
  timing-independent frame assertion and audio capture after the user's
  interactive crash/startup priority; this does not complete full integration.
  [Actual setup, launcher and bounded shot evidence](../.local/mcc-graphics-corrections-v3-20261018/full-setup-v1/validation.json),
  [Actual new-cache firing log](../.local/mcc-graphics-corrections-v3-20261018/full-setup-v1/actual-new-cache-firing.log).
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
- [ ] Identify weapon-specific callback audio and verify audible shot output
  beyond the now-passing bounded ordinary Main SMG shot. The successful tick60
  run and later deliveries below supersede the earlier termination frontier;
  finite nonzero callback data alone does not identify the shot sound.
  Historical progression: actual step3 fire originally failed at `F0D38`; the data-only rerun clears
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
  captured shot. Those early failures are retained as history; the actual
  bounded firing gate passes in the later candidate-v5/V16/V18 records.
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

- [x] Audit the11 previously named obsolete factory/lens test executables after
  V14 delivery, preserving the first three broader native-gameplay probes. The11
  total1,056,763,768 logical bytes; ten retain every recorded reproduction input,
  while one superseded probe has a drifted object and remains deferred. No current
  V14 input or visible active-process reference points to these executables.
  Six same-UID system processes and foreign processes deny complete `/proc`
  inspection, and a read-only noninteractive elevated check is unavailable
  because existing sudo authorization is absent. Therefore no historical
  executable is deleted and final cleanup gates remain open. Both audit scratch
  directories are removed; source, objects, references and deliverables remain.
  [Bounded cleanup audit](../.local/mcc-native-click-crash-20261018/cleanup-v14/audit.json).

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

## Latest graphical conversion milestones, 2026-10-19

- [x] Prove Sacred Icon's gradient descriptor/resource correction with the exact
  selected V16 native loader. Stock MCC is64×256/nine mips; the original is
  32×128/eight mips. A private MCC map retains its format/packing flags while
  changing only width, height, mip count and resource pointer/extent. Every
  decoded mip descriptor and all21,852 output bytes equal the original. The
  316 selected providers and original/MCC source files remain exact. Probe ELF,
  object, map, raw pixels and private map/bank are removed with scratch. This
  establishes the conversion shape; bundled MCC-only reconstruction and rendered
  sky acceptance remain open.
  [Native descriptor/resource proof](../.local/mcc-sacred-icon-gradient-20261019/descriptor-v1/validation.json).
- [x] Add a fingerprint-bound MCC-only Sacred Icon gradient delta and importer
  integration. All six corrected images/48 mips decode through exact V16 native
  providers to their original target hashes after real three-map application.
  A malformed final profile fails before any map/bank write; the entire original
  bank prefix remains exact. Onboarding distinguishes complete V4 copies from
  older correction sets. V1/V2/V3 bundles stay immutable. The new product delta
  and manifest total5,521bytes; all private maps/banks/pixels/probe outputs are
  removed. Existing active caches and releases are untouched. Focused tests,
  independent review, installed delivery and rendered sky acceptance remain
  separate gates. Both focused suites also pass with the already shipped
  portable Python:46 controls, no failures/errors/skips, with scratch removed.
  [Actual MCC-only V4 proof](../.local/mcc-sacred-icon-gradient-20261019/bundle-v1/validation.json).
- [x] Independently review V4 reconstruction and cache reuse. Exact MCC-only
  reconstruction opens only its product delta/manifest. A reproduced downgrade
  edge is fixed: a complete V4 cache remains reusable if its local delta is
  missing; all four freshness controls and46 portable controls pass. The same
  review authenticates the gradient's signed registration point: MCC32,128
  versus original16,64. Pixel equality alone did not cover those words; include
  their exact guarded restoration before V4 delivery. No particular active
  gradient registration consumer or additional visible defect is claimed.
  [Independent post-fix review](../.local/mcc-sacred-icon-gradient-20261019/independent-review-v2/validation.json).
- [x] Finish V4's exact registration-point restoration and repeat real MCC
  conversion with the final product inputs. Source32,128 is guarded and target
  16,64 restored. A wrong registration tuple rejects all writes. The complete
  decoded metadata of all three maps is compared: only the five existing
  resource bindings and the gradient's resource/dimensions/mips/registration
  fields change outside cache-publication headers. All six native images/48
  mips and46 shipped-Python controls still pass. The final delta/manifest total
  5,624bytes; all probe/map/bank/pixel scratch is removed. V17 packaging and
  rendered sky acceptance remain open.
  [Final conversion/metadata proof](../.local/mcc-sacred-icon-gradient-20261019/bundle-v2/validation.json)
  · [Portable controls](../.local/mcc-sacred-icon-gradient-20261019/bundle-v2/portable-python-tests.json).
- [x] Prepare and review the importer-only V17 compiled ZIP workflow. Fresh
  guards verify final V4 inputs/proofs, immutable V16 payload and all97 shaders.
  Ten named archive paths change/add; all other release bytes must match V16.
  Historical importer pins are explicitly separated from current V4 authority.
  Preflight scratch is removed. Root authorizes package-only execution against
  this concrete sealed handoff; installed folders/caches remain untouched.
  [Sealed preparation](../.local/mcc-sacred-icon-gradient-20261019/delivery-v17/preparation-v2.json).
- [x] Record V17's first execution's final preservation-guard failure.
  Relocated46 tests and the eight-frame launcher
  path reached the final guard; no ZIP was published and private scratch is
  removed. The exact beginning snapshot was not retained, so the cause remains
  unassigned pending a measured diff. Preserve the failed attempt's small
  diagnostics and add explicit before/after state receipts before any retry.
- [x] Preserve attempt1 and qualify an instrumented retry. Fresh source/base
  hashes match; three complete repeated metadata sweeps reproduce no change.
  The earlier preflight-to-current comparison differs only in P's aggregate
  cache metadata, and the lost actual initial snapshot prevents a causal claim.
  Attempt2 keeps every strict check and records actual before/after states,
  per-field differences and each final operand. Its separate sealed preflight
  passes; root authorizes this concrete package-only retry without weakening
  the preservation gate.
  [Instrumented preparation](../.local/mcc-sacred-icon-gradient-20261019/delivery-v17/preparation-v3.json).
- [x] Publish compiled V17 after the instrumented attempt passes every original
  strict preservation check. Actual before/after state, all cached file metadata,
  runtime/selection/save hashes and every source/base operand agree. Relocated
  46 controls, both launcher-info routes, eight-frame Lockout launch and all4,251
  archive checksums pass. Exactly ten archive paths change/add; the V16 payload,
  wrapper/loader and97 shaders remain exact. No game data or saves are bundled.
  Scratch is removed; only the requested ZIP/checksum is retained. The initial
  attempt's cause remains unestablished. Independent archive review and P-only
  importer installation follow; existing caches are not upgraded by packaging.
  [Actual compiled V17](../.local/mcc-sacred-icon-gradient-20261019/delivery-v17/package-validation.json).
- [x] Independently verify V17's4,252 entries/all4,251 checksums, seven changed
  existing/three added paths and all4,242 remaining V16 entries. Runtime,
  wrapper/loader,97 shaders, current importer pins and historical authority
  roles agree; attempt2's strict preservation proof is intact. Review scratch
  is removed. P-only importer installation remains next.
  [Independent V17 archive review](../.local/mcc-sacred-icon-gradient-20261019/delivery-v17/archive-review-v1/validation.json).
- [x] Review and prepare P-only V17 deployment with cancellation-safe rollback.
  A real queued-SIGTERM control covers rename-before-record cancellation; the
  mutation is now recorded before signals resume, and rollback restores files
  before unblocking. Fresh read-only preflight verifies all old release files,
  three absent new paths and preserved user state. Seven small preimages total
  571,265bytes; no runtime copy/import/cache conversion is planned. Root
  authorizes the sealed V2 handoff against the accepted ZIP. Actual staging
  uses owned scratch on P's filesystem for atomic replacements.
  [Cancellation-safe installer preparation](../.local/mcc-sacred-icon-gradient-20261019/delivery-v17/install-v2/preparation.json).
- [x] Install the accepted V17 importer/release files into the existing P
  folder. All4,252 release paths match the verified ZIP; both P launchers keep
  the native test available/full readiness false. The runtime,97 shaders,
  saves, cache metadata, selections, previous rollback trees and root's separate
  workbench are preserved. Seven required rollback files total571,265bytes;
  staging scratch on P's filesystem is removed. No new game run was needed
  after installing the same accepted payload, and existing caches remain at
  their previous correction levels until a new import.
  [Actual installed V17](../.local/mcc-sacred-icon-gradient-20261019/delivery-v17/install-v2/install-validation.json).
- [x] Measure the later read-only closure audit's metadata mismatch without
  altering the successful package/install receipts. Two immediate scans change
  only 3,072 P inode IDs; directories/names/device/size/mtime/mode remain equal,
  including their saved package field hashes. Root's complete metadata and
  launcher/runtime/save hashes remain exact. The accepted install's original
  before/after guard passed. Preserve the failed audit and qualify the later
  storage audit against stable fields with inode identity explicitly excluded;
  this does not prove full-bank content hashes or the cause of the earlier
  packaging failure. The diagnostic scratch is removed.
  [Measured inode-only difference](../.local/mcc-sacred-icon-gradient-20261019/closure-diagnostic-v2/measurement.json).
- [x] Finish the scoped V4/V17 storage audit and reverify all 4,252 installed
  release files, 97 shaders, the ZIP/checksum and seven rollback preimages.
  All 31 recorded completed product/delivery scratch paths and the audit's own
  scratch are absent; the separate preparation scratch is also removed. The
  audit preserves the successful original package/install guards and records
  the measured later inode exclusion explicitly. Newly retained delivery data
  totals 184,194,863 bytes: the requested ZIP/checksum 183,623,598 bytes and required
  rollback 571,265 bytes. Active material/foot work, inherited references/cohorts,
  installed data and historical cleanup stay outside this bounded audit.
  [Scoped delivery closure](../.local/mcc-sacred-icon-gradient-20261019/closure-v2/validation.json).

## Next steps, in order

1. Complete the MCC-only title screen and menu vertical slice described in the
   current-priority section, then make it default Play. Keep native Lockout
   testing separately available. Wire platform binary downloads to Update;
   the official repository currently has no published release assets, which
   must remain an explicit availability limit rather than a source rebuild.
2. Continue gameplay-tag semantic coverage beyond the now-delivered V16
   map-specific Gauss/Relic conversions. All28 authenticated contexts/two
   unqualified MCC maps pass complete Cache comparisons, and the retained
   executable passes firing, real input and relocated package launch. P now
   matches all4,252 V17 files with the same V16 runtime; root's separate renderer
   workbench stays selected. The Floodzone material correction passes actual
   normal Cache checks across all43 maps and is delivered in the independently
   reviewed V18 ZIP. Its compatible client passes fixed-tick frame/save checks,
   nine real input stages and relocated launcher gameplay. P remains V17.
   The guarded Chief union conversion now passes all 43 selected Cache views,
   2,345 direct native selector cases and eight private controls. Its compatible
   V19 successor passes the paired native frame/save/audio acceptance; preserve
   that private candidate and resume its input/delivery work after the frontend,
   then continue remaining
   material-effect semantic coverage.
   Preserve unknown mods, saves,97 bundled shaders, TU1.5 semantics and interim
   Play. Data conversion does not establish native CTF or all other modes.
   Distinguish actual host-pointer/ownership limitations from unrelated current
   feature-body drift before any broader rebuild. Remaining first-launch,
   complete renderer and live parity acceptance stay open.
3. Continue rendered acceptance of the now-packaged/installed six-image/48-mip
   V4 importer. The Sacred Icon gradient's exact descriptor, registration,
   resource and pixel conversion pass; its live sky still needs acceptance.
   The fresh MCC-only preparation, resumed cache, actual native shot/audio,
   selected root V3 cache, compiled V13 importer and extracted-folder tool
   synchronization are verified above. Do not repeat the 31GB import. The user's
   extracted folder retains its working older cache; the updated setup can
   prepare the newer correction set when requested. The ocean texture needs
   no pixel correction. Preserve the current working caches during delivery.
4. Wire the honest native-test launcher capability and MCC-only loading/menu
   asset handoffs beyond the now-published interim Lockout route. Verify a fresh
   MCC-only import, remaining loading/menu roles and mod-folder interoperability.
   The loading-name kernel and actual tag1009 branch/descriptor ingress are
   sealed; actual GFx loading invocation/image bytes and the genuine
   `loading_screen_music` live-service selection remain open. The separate
   frontend music graph, exact WEM and stock decoder are authenticated. The
   selector's live caller, active loading image bytes and presentation remain
   open; decoded callback data alone does not establish audible equivalence.
   The V3 product/delta and onboarding delivery are
   verified; wider rendered scene acceptance remains open.
5. Validate the ordinary client and complete safe product/lifecycle integration.
6. Continue the remaining onboarding, conversion, parity and delivery gates above.
7. Perform the final cleanup and record the recovered storage.


## v530 MCC shortcut admission and runtime-source prerequisites, 2026-10-19

These prerequisite milestones are recorded before the dependent root-shortcut
installation. Runtime inputs remain imported MCC only. Xbox ISO may be used
only for offline validation of converted MCC tags; decompiled TU1.5 code remains
the behavior authority. Xbox/XBE/Vista banks are never runtime dependencies.

- [x] Review all 13 passing synthetic MCC shortcut admission/routing tests.
  Module `tools/mcc_sandbox_launcher.py` SHA-256
  `18e71ab5d5e3713db0e1761ee440a4334b4daf2c9694ed70bde2bf7719f4dcda`;
  test `tests/test_mcc_sandbox_launcher.py` SHA-256
  `70f2e51f6f3dfd644362d0c0876c3ac12911884b25873d6d9580d25d19ec4b8c`.
  The legacy regular ELF lacks explicit native-test support. Its `--launch-game`
  compatibility route requires native capability, no frontend and Play selecting
  exact Lockout; newer supported runtimes use explicit native selection. These
  are synthetic admission/routing proofs, not installed-wrapper or actual
  launcher-through-runtime acceptance. Evidence:
  [Shortcut admission](../work/sandbox-primary-v15/mcc-shortcut-admission-v530.json).
- [x] Verify the selected runtime's bounded MCC asset-source check without
  `--gameplay-xbe`: exit 0, six hidden muted Lockout frames, eight ticks, one
  player/one connected. Successful-open tracing records 1,696 paths, zero
  forbidden Xbox/XBE/ISO/Vista paths, zero unknown assets and zero unmatched
  traces. Owners remain unchanged; scratch `halo2-test-2093lm9c` is removed.
  Report SHA-256
  `edc1cc7ec4533950070fbef5036aba26b65dee1ffaf51cb862a21cd0fc0243cb`.
  This proves the bounded direct runtime asset source only; actual installed
  shortcut-through-runtime, visible/unmuted/audio, all-map, selected frontend,
  original presentation and full parity acceptance remain separate and open.
  [MCC runtime file opens](../work/sandbox-primary-v15/selected-mcc-runtime-file-opens-v530.json).
- [x] Install and verify the bounded root shortcut delivery. `Halo2-Sandbox-Test`
  now selects the MCC helper; the actual GUI label is `MCC sandbox test  ›`.
  Installed `--check` passes with cache v13, native capability true and frontend
  capability false. All seven installed CLI guard/info cases pass: Xbox/XBE /
  Vista/map/runtime/installation overrides are rejected and `--menu` refuses the
  unqualified frontend. Existing real Tk launcher regression tests pass 18/18.
  Eight protected files, including the regular launcher, original r6, selected
  MCC runtime and cache provenance, remain exact; test scratch root is empty and
  no new large artifact is retained. Delivery receipt SHA-256
  `2725075b6d56d4930b7c7f7ca40f15c95ff7126fed1d3fb54ec84c148f9c9736`
  (5,270 bytes). The earlier direct six-frame asset-source check retains its
  limits: this delivery does not prove installed-shortcut-through-runtime,
  visible/unmuted/audio, all-map, selected-menu or full parity acceptance.
  All 14 whole-game gates remain open, 0/102 sandbox elements accepted.
  [MCC shortcut delivery](../work/sandbox-primary-v15/mcc-sandbox-shortcut-delivery-v530.json).


### v531 MCC menu catalogue component (2026-10-19)

- [x] Replace the twelve-name menu filter with authored catalogue identity, without changing MenuState layout. The standalone production CMake test passes 12 frozen original metadata rows and 25 literal MCC rows, preserving sort/unlock, preview/launch identity and exact scenario leaves. Two blank-title DLC placeholders remain explicitly unqualified; no alias or full map acceptance is claimed. Private CMake scratch is removed. [Component receipt](../work/sandbox-primary-v15/mcc-menu-catalog-component-v531.json).
- [x] Verify the changed MenuState through the actual imported MCC Cache and compatible native link; completed by v532 below. Renderer/launcher delivery remains its separate unchecked gate.


### v532 actual MCC catalogue/MenuState integration (2026-10-19)

- [x] Verify actual MCC Cache → native profile/sign-in → Split Screen → map list → focused preview → accepted lobby identity → selected-cache launch routing. The native CPU test passes 48 previews and 40 launch/header routes across original and reversed 25-record catalogues; four blank-title and four existing energy-weapon rejections remain explicit. All316 parent link inputs and230 frozen dependency files match before/after. No Xbox/XBE/ISO/Vista assets are opened; owned scratch is removed and no executable or raw trace is retained. [Integration receipt](../work/sandbox-primary-v15/mcc-map-picker-integration-v532.json), SHA-256 `4f983c985d3fba34f833cd04821340e82bd54cfb216dcc0dd340750e8887178c`.
- [x] Deliver the basic frontend and expanded raw catalogue through the selected renderer/launcher. V24's actual818-frame menu/map-picker result and F24's actual installed Play-button/title handoff qualify this bounded delivery; see v574/v585. Gameplay, complete menus, pause, source session return and full map parity remain open.
- [ ] Publish the two qualified DLC catalogue records/previews and instant-charge profiles coherently to a separate prepared MCC cache, qualify affected native paths and adopt that cache without changing unrelated inputs. The two authored identities/native CPU routes pass v544, and complete private preparation passes v552; reuse both. No highplains/highplains2 alias or full-map equivalence is inferred.


### v533 MCC source-world weapon composition (2026-10-19)

- [x] Migrate the source weapon-effects fixture and its three active registrations to selected MCC/native construction, rejecting executable arguments and non-MCC headers. Seven Python admission/routing controls, three actual native rejection controls and the focused production CTest pass. Ordinary native MCC SMG input produces60→53 rounds, seven accepted positive effect requests and12 contact effects with no parallel adapter shots. All316 parent providers remain exact, successful-open tracing finds no Xbox/XBE/ISO/Vista inputs, and compiler/native scratch is removed. [Frozen migration receipt](../work/sandbox-primary-v15/mcc-source-weapon-migration-v533.json), SHA-256 `5bd70c61198ad372462c91f23a915d82b1e4d36c8394a2e4ddff3dd2eaffe93b`.
- [ ] Complete the original plasma-pistol charge/release lifecycle. The first-trigger `104698` failure is resolved by v542. Actual MCC execution reaches release/141 effect frames before missing vtable slot `45F8B4`; its complete method-table owner is now authenticated. Correct the fixture to the original instant damage/player-response effect: positive allocation, event execution and natural retirement before returning NONE. The whole-qualified preparation profile now projects the authenticated flag word2→6 (v550/v552); actual native allocation/event/player-response/retirement acceptance remains pending. Persistent particle-graph acceptance belongs to an actor whose authored charging effect actually has such a graph. Preserve the failed strict receipts; do not supply targets/results or invented effects. [Original effect contract](WEAPONS_TU15_PARITY_GOAL.md), [latest native frontier](../work/sandbox-primary-v15/mcc-weapon-program-tables-progress-v542.json).


### v535 complete CTF/Assault initializer closure (2026-10-19)

- [x] Port the complete original initializer and marker/query/region owners (`249460`, `24CE00`, `24CEF0`, `1A8850`). Independent execution of all three original versions passes 879 calls, normalized to 293 cases; optimized native and ASan/UBSan comparisons match the full 520-byte bank and conditional outputs. Five cases use authored marker data from imported MCC maps. The production CMake registration passes through `tools/run_tests.py`; its private build and oracle/compiler scratch are removed. [Milestone receipt](../work/sandbox-primary-v15/ctf-initializer-milestone-v535.json).
- [ ] Compose the original activation/object-birth, registry and factory owners with the real selected native world, then complete CTF/Assault rules and lifecycle acceptance. The initializer does not admit either mode or establish live gameplay, presentation, LAN or launcher delivery. Three unreachable zero-distance replacement instructions remain outside executed coverage. All 14 whole-game gates and all 102 complete-element acceptances remain open.


### v541 physics branch-owner compile fix (2026-10-19)

- [x] Fix three invalid memory-API calls in the new physics branch-data owner, preserving the authenticated table payload. Focused production CMake validation passes installation/idempotence, partial-owner retention and four atomic rejection cases, including an uncommitted reserved owner. Private build scratch is removed. [Adapter receipt](../work/sandbox-primary-v15/physics-branch-owner-adapter-v541.json).
The dependent actual MCC charge/release lifecycle remains tracked at v533 above. This compile/ownership fix supplies no runtime result, gameplay rule change, selected-runtime installation or full parity acceptance.


### v542 complete weapon program tables and MCC charge frontier (2026-10-19)

- [x] Restore all five bounded dispatch tables of the complete original `1030F0` owner: 26 words/104 bytes, including eight newly missing words. All 51 independent original guarded-selector cases and the coherent native cold-factory comparison pass. The original guards, instructions and branch targets are unchanged. The separate current-header cold executable also passes; its aggregate receipt correctly records concurrent source drift and does not qualify the subsequently changed workspace.
- [x] Verify the coherent source-world build against actual imported MCC inputs. Ordinary SMG firing passes again (60→53 rounds, seven positive effects, 11 contact effects), with all 316 parent providers unchanged and no Xbox/XBE/ISO/Vista runtime reads. The strict plasma-pistol case now passes the former `104698` failure, reaches charge/release and 141 effect frames, then fails at original `2F8B27` reading vtable slot `45F8B4`. No positive charge graph was observed. All owned build/runtime scratch is removed. [Progress receipt](../work/sandbox-primary-v15/mcc-weapon-program-tables-progress-v542.json).

The remaining charge lifecycle is the unchecked task at v533. Its persistent-graph expectation was subsequently disproved by the authenticated original instant-effect contract; that raw flag difference is now covered by the whole-qualified preparation profile in v550/v552, with actual native acceptance still pending. Failed receipts are preserved. No charged-weapon acceptance or runtime installation is claimed.


### v544 authored Tombstone/Desolation catalogue and native menu routing (2026-10-19)

- [x] Resolve both inherited blank MCC menu records from their complete authored scenario Level Data: Tombstone1402/order1500 and Desolation1400/order1400. Preserve destination bitmap identity and registration; redirect only their exact owned MCC preview partitions. The six planned stores total6,360 bytes. Eleven focused conversion controls and actual v13 reopen/idempotence/unrelated-byte checks pass. Offline converted-tag validation also finds both authored MCC preview pixel banks identical to the current Xbox LVL reference previews. [Conversion receipt](../work/sandbox-primary-v15/mcc-dlc-catalog-conversion-v533.json).
- [x] Verify the converted catalogue through actual native MCC Cache/MenuState construction: 48 previews, 44 selected-cache launch/header routes, zero blank-record rejections and four preserved energy-combat gates, including reversed authored ordering. Both new identities, localized metadata, team counts, exact MCC scenario paths and destination preview owners are checked. All316 parent link inputs remain exact, no Xbox/XBE/ISO/Vista runtime inputs are opened, and the scratch mainmenu/object/executable/profile tree is removed. The selected cache remains unchanged. [Native integration](../work/sandbox-primary-v15/mcc-map-picker-converted-integration-v544.json), SHA-256 `5ff9750cbaef09c81669a679cebf54ee20594beb8d45251d9c54099dc71d5745`.
- [x] Qualify the fresh-preparation hook, strict final DDS inventory and complete menu-asset assembly; completed by v550/v552 below. Coherent selected frontend/launcher delivery remains the separate v532 gate. CPU routing and offline preview-tag validation do not prove displayed native pixels, gameplay, transitions or complete map parity.


### v550 strict preparation inventory and original instant-charge profile (2026-10-19)

- [x] Qualify both converted preview pages through the actual full MCC inventory: all648 page owners pass,646 remain exactly unchanged, and the two authored replacements produce563 unique DDS pages. The default stock verifier rejects the changed pages; only the complete authenticated projection admits them. Original stock digests/comparisons remain recorded. [Inventory qualification](../work/sandbox-primary-v15/mcc-catalog-inventory-v534.json).
- [x] Integrate catalogue projection before private bitmap extraction, preserve all six stores/counters, record the post-serialization mainmenu hash, and pass the projection into strict final menu inventory. Production CMake preparation/conversion/inventory tests pass; a further preparation/instant-charge registration run passes after the new scalar hook. Both private builds are removed. [Catalogue CMake](../work/sandbox-primary-v15/mcc-catalog-preparation-cmake-v547/report.json), [charge-profile CMake](../work/sandbox-primary-v15/mcc-instant-charge-preparation-cmake-v550/report.json).
- [x] Validate the named instant-charge effect's complete160-byte source tuple, root marker and salted damage reference before projecting only its stored flag word2→6 to the original raw tag value. Eight profile controls and a real selected-MCC memory-only plan/idempotence check pass. Unknown custom definitions stay unchanged; this is no generic effect-bit heuristic. The current-header cold factory also passes the complete reached contact method tables. [Frozen source checkpoint](../work/sandbox-primary-v15/mcc-instant-charge-source-checkpoint-v549.json).

The copied-MCC preparation/bitmap/menu-asset pipeline subsequently passed v552. Actual converted-MCC instant charge remains the v533 gate; the selected cache/runtime remains unchanged. Retained idle-loop graph acceptance stays a separate task for an actor authored to emit that graph.


### v551 native CTF/Assault activation and registry owners (2026-10-19)

- [x] Port25 complete TU1.5 initializer/first-activation/registry owners and wire their exact entry predicates into the real World dispatcher source. Independent original/native optimized and ASan/UBSan comparisons pass278 cases:209 complete returns and69 exact unsupplied-owner stops, with1,180/1,459 production instruction sites observed. The isolated production CMake test passes with strict FP. Actual object factories, existing registry services and dirty ownership remain genuine external native owners. [Milestone receipt](../work/sandbox-primary-v15/ctf-activation-milestone-v551.json).

Real same-bank MCC factory birth, remaining CTF/Assault rules and complete lifecycle acceptance remain the unchecked v535 gate. The full World object was not rebuilt or live-qualified by the isolated test; mode admission is unchanged. FP oracle status exclusions are documented. Required text regression fixture `tests/fixtures/ctf_activation_original_tu15.json` is1,097,045 bytes; compiler/test scratch is removed, with no retained executable, raw runtime bank or capture.


### v552 actual43-map private preparation and final menu assembly (2026-10-19)

- [x] Run the real preparation path on private copies of all43 selected MCC maps, texture bank and fonts. All25 multiplayer maps are processed;1,045 ordered stores include only20 changed blocks: six complete catalogue/resource stores and14 qualified instant-charge flag words. The distinct Floodlab effect tuple remains unchanged and explicitly unqualified. Strict inventory and final asset assembly pass648 frame owners,563 unique DDS pages and40 font files. All selected assets, registry/provenance, source files and launcher owners remain exact. The4,332,372,016 generated bytes are removed with their owned scratch. [Preparation receipt](../work/sandbox-primary-v15/mcc-private-preparation-milestone-v552.json).

The combined native step did not run: its factory copy caught a concurrent extra-brace syntax error at line361, subsequently corrected in the live source. The aggregate failure remains preserved. Repeat only the native stage against fresh owned MCC copies; do not repeat this passing43-map/bitmap/menu preparation. Actual instant-charge acceptance remains the v533 gate, and selected frontend/renderer delivery remains v532.


### v555 actual prepared-MCC instant-charge caller boundary (2026-10-19)

- [x] Compile/link the corrected genuine factory with all316 retained providers unchanged, then test private MCC core copies with the qualified instant-effect conversion. The actual effective flags6/one-event/one-jpt/no-particle contract passes; SMG firing and both native input-open audits pass. No43-map/bitmap/menu preparation is repeated. All selected/frozen input guards pass and owned scratch is removed. [Native caller receipt](../work/sandbox-primary-v15/mcc-instant-charge-caller-frontier-v555.json).

The charge fixture stops at its first input tick80, original108E26 storing NONE, before any required positive damage allocation/player-response observation. Its whole108DB0 caller/control context and observer coverage are now the unresolved boundary; this is not yet a confirmed gameplay defect. Preserve that actual result and compare the complete original caller before changing expectations. The v533 charge gate remains open.


### v558 Floodlab conversion and actual CTF constructor frontier (2026-10-19)

- [x] Qualify the separate164-byte Floodlab instant-charge tuple with its authored inline root marker and complete damage graph. Fourteen focused controls, actual MCC scratch projection, production v13 serialization, independent reopen, idempotence and the registered production CMake test pass. Only the authenticated four-byte flags word changes2→6. All selected/source owners remain exact; both private scratch trees are removed. This resolves the distinct tuple preserved by v552 without repeating its passing43-map preparation. [Milestone receipt](../work/sandbox-primary-v15/mcc-floodlab-and-ctf-frontier-v558.json).

The first actual current-World MCC CTF probe compiles/links but stops during the original contrail constructor11E550/11E55D at missing backing505E90, before CTF activation. Its698-path open audit passes with no forbidden, unknown or unmatched inputs. The complete destination already has a genuine current scalar-state owner; the stale archive provider must be refreshed. Preserve the failed run and retry that dependency. No CTF birth, charge lifecycle, selected frontend or full parity gate is accepted; v533/v535/v532 remain open. No large artifact is retained.


### v559 reuse the passed corrected-menu native build (2026-10-19)

- [x] Independently review the existing V23 native menu result and preserved compiled owners. The native run exits0 after412 title/profile/keyboard/main/Settings/back frames, with finite nonzero title/menu audio; its real-input Lockout regression also exits0. Root visually inspected the actual640x480 profile and Settings frames: complete SELECT/BACK artwork and captions fit. All retained file and reference hashes match the sealed receipt; prior scratch is removed. No native test or capture is repeated. [Existing native evidence review](../work/sandbox-primary-v15/mcc-preserved-menu-native-review-v559.json).

Prepare the actual ordinary Play handoff using this coherent successor while preserving the selected cameraV3 gameplay action, runtime and shaders. Check whether its menu catalogue needs the already qualified current MenuState/content and converted data. Selected frontend delivery remains v532; native widescreen frames, complete menus/maps, original framebuffer/audio parity and full-game acceptance remain open. The118,332,732-byte compiled/reference input cohort and180,872-byte two-PNG proof are existing required retained inputs, not newly generated test outputs.


### v560 actual installed MCC sandbox route (2026-10-19)

- [x] Verify the installed shortcut through the real regular launcher and exact selected cameraV3 native runtime. It exits0 after six hidden muted MCC Lockout frames, seven ticks and one player. The1,974-open/28-trace audit has zero forbidden, unmatched or remaining unknown inputs. Only the two exact save paths are reclassified using the recorded namespace bind and independently verified scratch backing. The original mechanical-red receipt is preserved; no native rerun or broad audit exception is used. [Installed route milestone](../work/sandbox-primary-v15/mcc-installed-shortcut-milestone-v560.json).

Protected file hashes/device/inode/size/mtime, full selected-cache stable metadata, actual selection and installed saves remain unchanged. Concurrent inode ctime changes are explicitly recorded; no unchanged-ctime claim is made. Both owned scratch trees are removed and no large artifact is retained. This closes the earlier shortcut-through-runtime uncertainty only. Visible/unmuted/audio, complete maps, ordinary Play frontend delivery and full parity remain separate open gates.


### v566 actual MCC current-World initialization (2026-10-19)

- [x] Compose the genuine current World and complete updated cold-data providers with the compatible native cohort. All26 translation units compile/link; actual MCC Lockout world construction and map initialization return. The698-path input-open audit passes with no forbidden, unknown or unmatched inputs. Frozen source, all316 retained providers, selected cache/runtime and current-workspace guards pass; scratch is removed with no large artifact retained. [Initialization checkpoint](../work/sandbox-primary-v15/ctf-activation-port-20261019/current-world-initialization-checkpoint.json).

CTF session configuration now reaches original159FF0/15A0EA and requires its real static engine object48BFA0/interface466190. Configuration, activation and object birth have not completed. Audit and restore that complete authored object/interface/registration owner before continuing; no ready-state seeding or mode admission. The v535 lifecycle and all whole-game gates remain open.


### v573 compiled independent launcher routes and complete CTF static data (2026-10-19)

- [x] Implement and production-test independent frontend selection while keeping the native gameplay runtime unchanged. Six focused CTests pass, including five actual compiled-launcher routing cases, eight selection-admission cases,14 shortcut cases and18 real launcher UI cases. Missing selection preserves prior behavior; stale explicit selection blocks ordinary Play while the native test remains available. Newly compiled normal/title launchers both retain the exact cameraV3 native owner; no selection or installed launcher is changed. [Combined milestone](../work/sandbox-primary-v15/mcc-separated-launcher-and-ctf-milestone-v573.json).
- [x] Restore the authenticated complete CTF/Assault static object/interface/publication destination:212 bytes, all51 original callback identities, correct original sections/access, duplicate/partial/reserved-owner rejection and adjacent-owner preservation. Optimized and sanitized component comparisons pass69 cases; the registered production CMake test passes. The real factory now mounts this data before imported tags, with existing source activation/callback owners unchanged. Actual configured activation and object birth remain the v535 gate.

The initial launcher target's unnecessary native dependency build was canceled before tests; its229,970,142 generated bytes are removed. The launcher-only configuration builds the actual routing binaries without a native rebuild, preserving normal source-build defaults. Its179,704,999-byte private tree is also removed after retaining552,190 bytes of requested working launcher inputs/frozen source. Source-only reproduction cohorts for the current World and charge test are retained separately; no raw bank or disposable engine binary is retained. Actual ordinary Play publication, expanded native map-picker adoption and full parity remain open.


### v574 actual expanded MCC map picker and bounded cold-table correction (2026-10-19)

- [x] Adopt the qualified catalogue MenuState over the preserved V23 client, reusing its identical qualified content producer. The coherent325-provider V24 run exits0 after818 native frontend frames: real title/profile/main/Split Screen/lobby/setup/map-picker input,15 Down inputs and return. Root visually reviewed the actual Backwash selection, authored preview/description and surrounding DLC rows. Selected MCC cache, parent inputs and frozen dependencies stay exact; no forbidden asset input is opened. [Actual native picker](../work/sandbox-primary-v15/mcc-expanded-menu-native-v572/report.json).
- [x] Correct the cold weapon-table unit's owner-label assumption for the two already-authenticated185111 interface words. The native unit passes26 original words/104 bytes,51 guarded selectors and10 contact methods, retaining exact bytes/read-only/unique-owner/write-rejection checks. All316 providers and frozen inputs remain exact. [Cold unit result](../work/sandbox-primary-v15/test-cold-weapon-ownership-v571.json).

V24 retains111,479,920 bytes of requested executable/object inputs plus its small frozen source/cohort and102,933-byte required picker reference; both native-test scratch and cold-test scratch are removed. The charge diagnostic was stopped after automatic review flagged possible cybersecurity risk; the completed follow-up ran ordinary native unit tests only. Charge remains unaccepted. Two unconverted blank catalogue identities remain gated, as do energy-combat paths; native widescreen, map gameplay, ordinary Play installation and full parity are not accepted by the map-picker run. Reuse this V24 result for staging; do not repeat the passing menu flow.


### v576 final launcher consistency and CTF configuration owners (2026-10-19)

- [x] Match launcher status and controls to actual stale-selector behavior. Three focused production CTests pass: five compiled routing cases,22 real Tk UI cases and576 original/native CTF configuration cases. A changed explicit menu build disables only ordinary Play and reports that condition accurately; native test, sandbox and preview remain independent. Full-ready valid frontend selection and missing-selection fallback remain intact. Both newly compiled launchers preserve the exact cameraV3 native owner. [Milestone](../work/sandbox-primary-v15/mcc-frontend-and-ctf-configuration-milestone-v576.json).
- [x] Port and wire complete original24B390/249440 configuration owners: all18 instruction sites and576 cases compare full defined ABI, FP state and banks with optimized and sanitized native execution. The real MCC session now passes its class dispatch and249460 initializer, then reaches further callback24A6C0. Configuration, objective birth and lifecycle acceptance remain open. The old25-owner activation module stays unchanged.

The successful private CMake tree is removed after retaining the requested small normal/title launcher successors and frozen source. No selector, root launcher, selected cache or runtime is installed by this checkpoint. The2,745,863-byte declared synthetic CTF configuration fixture is a required regression input, not a captured runtime bank. V24's passing native menu proof is reused for the pending frontend staging/delivery.


### v577 qualified isolated frontend staging (2026-10-19)

- [x] Stage and independently recheck the passed V24 frontend with its own97-file shader bank and complete52-resource MCC capability. The99-file F24 owner totals107,082,951 bytes. Only22 bytes inside the authenticated64-byte read-only shader-path field change; game instructions remain exact. The existing capability checker and every retained-file hash/size pass. All protected325 inputs, old cameraV3 runtime/106 shaders, installed launchers, selected cache/registry and absent-selector state remain unchanged. [Stage receipt](../work/sandbox-primary-v15/mcc-frontend-stage-v573.json).

Owned staging scratch is removed. F24 is an explicitly retained requested working runtime; this filesystem milestone starts no game and publishes no selector. Actual normal/title launcher adoption and installed Play→native menu acceptance are the next separate delivery step. Basic menus/raw expanded catalogue only: two DLC placeholders, unfinished gameplay and full parity remain open.


### v579 installed independent MCC title/menu route (2026-10-19)

- [x] Install the qualified normal/title launcher pair and independent F24 frontend selection. Both actual installed `--launcher-info` calls identify cameraV3 for gameplay and F24 for Play/title/menus. The installed MCC sandbox `--check` passes both capabilities and still selects the exact original cameraV3 native-test owner. Selected cache/registry/provenance, sandbox wrapper, old runtime and all106 old shaders remain unchanged. Both prior launchers are retained under the explicit rollback owner. [Installation receipt](../work/sandbox-primary-v15/mcc-frontend-install-v578.json).

This is local route installation/admission only; no game is executed by the installer. The actual installed six-frame CLI route and actual GUI Play-button→title handoff are being tested separately. F24's earlier818-frame menu/map-picker proof is reused. Full menus, two converted DLC catalogue records, all-map gameplay and full parity remain open. Installer scratch is removed; working installed launchers and436,224-byte rollback launcher pair are explicitly retained.


### v581 complete CTF configuration and genuine tail composition (2026-10-19)

- [x] Extend the qualified configuration module with complete original24A6C0 and its genuine existing164930 tail owner. Optimized and ASan/UBSan execution each pass864 original/native full-ABI, defined-flags, FP-state, bank, store and trace comparisons; all26 production sites are observed. The focused production CMake test passes with the real activation module linked. Its old25-owner source remains exact;23/24 tail sites are reachable under the original fixedECX=-1 argument, with the independent whole-owner proof preserved. [Milestone](../work/sandbox-primary-v15/ctf-three-configuration-milestone-v581.json).

All compiler/oracle/CMake scratch is removed. The retained fixture is declared synthetic regression evidence, never a runtime bank. Actual MCC session configuration/activation, objective birth and lifecycles remain the existing v535 gate, with no mode admission or whole-element acceptance. Continue the frozen current-World MCC probe; do not repeat passing initializer/static-data/configuration component tests.


### v582 actual MCC CTF session configuration returns (2026-10-19)

- [x] Run the genuine current World and complete configuration owners against imported MCC Lockout. All29 translation units compile/link; construction, map initialization and configure_session return, including actual CTF bank publication and source engine kind1. The698-open audit has zero forbidden/unknown assets or unmatched lines. All316 parent providers, frozen/current source and MCC guards pass. [Actual configured-session checkpoint](../work/sandbox-primary-v15/ctf-activation-port-20261019/ctf-configured-session-checkpoint.json).

The next precondition remains unaccepted: the original occupied-team mask is0 because options-roster publication does not allocate the real player rows from which15ABC0 derives that mask. Use the genuine native player allocation/constructor lifecycle before first activation15AFB0; never seed the mask, fabricate handles or admit CTF/Assault early. Objective birth and complete rules remain v535. Scratch is removed; the119-file/2,346,364-byte source-only reproduction cohort is retained without binaries or raw runtime banks.


### Current per-map acceptance ledger (2026-10-19)

The [map ledger](../work/sandbox-primary-v15/mcc-map-acceptance-current.json) lists every literal imported MCC owner:15 campaign levels,25 multiplayer maps, mainmenu and two shared dependency banks. Each row keeps its provenance-derived cache hash, reusable preparation evidence, bounded native progress and unchecked lifecycle/presentation work. Dependency banks are not counted as playable levels. Import, content loading and private preparation do not accept whole maps. No test is repeated to create this ledger; G01-G14 and all102 whole-element acceptances remain open. Update the affected row when a new map lifecycle is actually qualified.


### v583 actual installed MCC CLI title route (2026-10-19)

- [x] Execute the installed regular launcher into the exact independently selected F24 runtime: title screen9 draws six hidden muted frontend frames and exits0. The1,868-open/29-trace audit has zero forbidden or unmatched inputs. Seven initially unclassified platform/SDK/native-variant paths are authenticated individually against their exact source, linked module or installed package owner, leaving zero unknowns. The original mechanical-red receipt is preserved; no CLI execution is repeated. [Exact-owner requalification](../work/sandbox-primary-v15/mcc-installed-play-owner-requalification-v582.json).

All277 originally protected owners are rechecked unchanged and both scratch trees are removed, with no retained capture/binary. The original early audit stop skipped complete-cache post-metadata comparison and did not pre-pin the five extra files; neither stronger claim is made. The fresh no-argument GUI Play proof separately guards these exact additional owners and complete cache metadata. Actual button acceptance, full menu/game lifecycles and full parity remain open.


### v585 actual installed Play-button delivery accepted (2026-10-19)

- [x] Verify ordinary no-argument root launcher → real Tk Play click → independently selected F24 → cached intro → title screen9 → clean native window close. The actual run exits0 with18 frontend frames,38 traces and2,169 successful opens; there are zero forbidden, unknown or unmatched inputs. An independent reviewer checks the actual exec/argv and platform categories. All45,123 MCC file metadata records, protected runtime/shader banks, launchers/selection, extra configuration, installed saves and profiles stay exact. [Installed Play milestone](../work/sandbox-primary-v15/mcc-installed-play-milestone-v585.json).

This completes bounded basic frontend delivery; reuse V24's earlier818-frame menu/map-picker qualification without repeating it. Complete menus/game return, converted DLC cache publication, gameplay/maps and OG/TU1.5 framebuffer/audio parity remain open. Normal startup includes cold initialization and the intro; this140.81-second proof is not a startup-performance claim. All three route-test scratch directories are removed; no new raw capture/binary is retained by the route probes. The milestone lists paths/sizes of the explicitly retained working F24/V24 inputs, compiled launchers, rollback and required picker reference.
