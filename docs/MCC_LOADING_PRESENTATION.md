# MCC loading presentation

## Original loading screens, October 28, 2026

The user asked for the original logo/animation loading screen, not the
`LOADING NN%` caption, with its music taken from the installed Halo 2 Editing
Kit (Steam app 1613450). Multiplayer starts and returns use the original
`PLEASE WAIT` dialog, including local and System Link sessions.

**Logo/sweep screen.** H2EK `loading_dx11.bin` holds the original PC loading
art as six contiguous DDS images for normal-mode slots 0, 1, 3, 4, 5 and 11:
the blue 1280×1024 background, the HALO 2 logo, both spinners, the sweep strip
and the real starburst glow mask. The logo is slot 1's 1024×128 DXT3 DDS,
SHA256 `e11a54772d4361fa560effce9640645acca84e0a8d4629db145fdad05380854d`.
Every staged DDS is byte-identical to its extent in the installed kit's
`loading_dx11.bin`; no Xbox/Vista image provider is linked into this runtime.
The user confirmed the classic kit logo with correct widescreen proportions.
The imported logo is fitted to its original centred box using its 1024:128
pixel aspect. The legacy normalized box otherwise stretches that same bitmap
by 4/3 on 16:9 output. This production adapter leaves the original draw oracle
and the background/spinner/sweep owners unchanged.
Against the exported original PC loading
bank they match within DXT error (background 42.95 dB PSNR once D3DX's
power-of-two stretch is undone; about 1% RMSE elsewhere). The MCC V3 bank's
greyscale background and reconstructed mask are therefore not used when the
kit is present. The music is the original four-voice ambience already pinned in
the H2EK tool executables' `.rdata`.

`tools/h2ek_loading_ambience.py` stages both into
`presentation/original-loading-v2` (policy
`tools/data/h2ek-original-loading-v2.json`, schema
`H2_H2EK_ORIGINAL_LOADING_V2`). It reads data only; no PE is executed. The
earlier ambience-only V1 bank is byte-checked and kept as a reference. At
launch, `configure_h2ek_original_loading_presentation` admits the bank,
checking the exact DDS headers and hashes, the ADPCM hash and the decoded PCM.
It decodes BC through the same path as the MCC inputs and binds
`NativeVistaLoadingAudio`. Without a kit, MCC V3 remains selected. Returning to
the menu after a multiplayer match uses the separate `Leaving game...` dialog.
The `LOADING NN%` map-copy wait remains a fallback for a cache without screen231.

**Multiplayer dialog.** TU1.5 opens screen 231
(`network_transition_progress_screen`) from `0x19430A → 0x1959C7` while a
networked simulation world exists but is not yet active. Its update `0x258605`
replaces pane text0 with the status selected by `0x19481B`. The UI-only frame
comes from `0x229C50 → 0x2C880`, and `0x255AFB` swallows all input. Before the
panel, `0x23B971` draws the shared `wigl+6C` tint. MCC and the reference both
retain RGBA(.85,0,20/255,44/255), packed as `D900142C` at full intro opacity.
It blends to RGB(0,17,37) over the cleared surface. The source 750 ms channel
intro controls its alpha; it covers the complete window, including widescreen
margins. The port maps its statuses as follows:

| Port phase | TU1.5 string | Text |
|---|---|---|
| resident imported map preparation through the first gameplay draw | `network_joining` | Setting up game... |
| post-game, disconnect or escape back to the menu | `network_leaving` | Leaving game... |

TU1.5 itself shows `Loading map...` only for a pending blocking map copy,
whose frames draw the percentage wait instead. Imported MCC maps need no
utility-drive copy. Their setup keeps `network_joining` throughout admission.
`FrontendNetworkTransitionPainter` paints the dialog with the frontend's own
pane primitives from the selected MCC content, fonts and sampled pages. That
covers bitmap order, animation58, the flag-4 pulse, kerning and shadows.
`frontend_network_transition_layout_data.inc` retains the authenticated original
screen 231 geometry and the exact raw MCC definition used to admit it. The
MCC 256/512-pixel rasters occupy the original 128/256-unit quads; text uses the
original eleven font roles projected from MCC. This avoids the smaller PC
dialog produced by its separate 480/1200 transform. The exporter retains
metadata only, with zero Xbox artwork pixels. The painter keeps only its
used materials (all prefixed
`network-transition/`). The loading bridge presents it with the map-wait
lifecycle, and the renderer admits that small UI bank. The original dialog's
one-second fade of all named sound classes to −64 dB (`258565 → 228390`)
uses the retained frontend audio engine and an independent loading-thread
clock. UI/loading updates serialize their shared fade owner; an older clock
query cannot rewind it or become a wrapped multi-day time step.
It continues while the new renderer is quiescent before its first
world draw. The returning menu restores its classes over two seconds. The
map-activation owner's immediate selector is literally `scripted_dialog_player`,
which matches none of TU1.5's 54 class names; other dialogue classes retain
the two-second restore.
The campaign's independent four-voice ambience never starts for this dialog.
Campaign loading and gameplay also continue the retained menu music clock,
so the original strict six-second music-transition debounce can stop the title
voices after frontend departure. Loading ambience has its own gain/lifetime;
the menu engine is not left looping underneath it.

`mcc_onboarding.select` repairs/imports the H2EK bank before publishing a cache
selection, including a cache imported before the kit was installed. Fresh
preparation imports the same authenticated raw/decoded sound. Existing V3 and
V1 reference banks are preserved. Frontend qualification now pins the actual
selected bank and requires its native admission log; a V3 receipt cannot
describe an H2EK runtime path. Engine promotion also requires the loading GPU
handoff and onboarding regression tests when that bank is present.

The installed-runtime reproduction on October 28 admitted MCC V3 despite an
already prepared H2EK bank. Source fixes, effective build hashes, commands,
qualification and cleanup receipts are recorded under
[`analysis/loading-flows-20261028`](../analysis/loading-flows-20261028/).
Rendered handoffs and source controls do not establish physical Xbox
framebuffer/audio equivalence or every campaign checkpoint/network role.

`original_loading_gpu_tests` (`HALO2_ORIGINAL_LOADING_CACHE`) captures both
screens through real forward/reverse Vulkan handoffs.

## MCC-only requirement, October 21, 2026

The user's requirement, reaffirmed October21, 2026, makes MCC imports the sole
runtime asset source. Vista and OG Xbox TU1.5 provide reference evidence only.
This supersedes the earlier bundled-original-tone exception described below.
Preserve that bank and its evidence as references; it cannot qualify the
delivered MCC-only loading path.

`NativeMccLoadingAudio` now plays authenticated, decoded MCC stereo PCM through
the native SDL owner. Its component and genuine-track tests pass together with
the material regression in the [frozen three-test cohort](../.local/mcc-material-callgrind-20261020/03-mcc-genuine-audio-corrected/cohort.json).
The actual MCC archive media decodes to 2,744,320 stereo float32 frames at
44.1 kHz, PCM SHA256
`4170c17c50ed19f4fd1fcd292431a78564c139ff6f1324e3ea56fdefcac34d6e`.
Playback preserves the complete converted extent, one-shot EOF/restart/mute
and the caller's floating-point state. Full decoded one-shot playback is an
explicit native policy; these tests do not establish MCC-authored segment,
loop or transition timing, or physical output.

Explicit `preflight_v3`, `prepare_v3` and `verify_v3` APIs now produce and verify
`presentation/loading-v3`. The separate
`configure_mcc_loading_presentation_v3` API admits its strict PCM/mask bank and
retains the native playback callbacks in the idle DesktopLoading bridge.
The [six-check cohort](../.local/mcc-material-callgrind-20261020/08-mcc-v3-genuine-producer-and-bridge-corrected/cohort.json)
passes all registered tests without skips, including the genuine MCC producer
and V3-only native admission. Its private bank uses the real archive media and
the two authenticated imported MCC artwork resources; the selected cache is
unchanged. The separate V2 control remains reference evidence. These checks
do not render a complete loading interval or qualify physical audio.

The [final restored six-check cohort](../.local/mcc-material-callgrind-20261020/12-final-untraced-component-restoration/cohort.json)
also passes without skips, with the original material implementation and no
profiling code. `HALO2_MCC_LOADING_V3_TEST_CACHE` now registers the reusable
genuine-bank admission check. It requires an explicit absolute V3 cache and
an explicit absolute `HALO2_MCC_FRONTEND_TEST_CACHE` containing `mainmenu.map`;
missing requested inputs fail rather than skip. This remains component
validation, separate from the unsuccessful native ONLINE history gate.

Normal source setup and launch now select V3: fresh imports preflight/prepare
the genuine track, cache reuse adds only an absent V3 bank and atomically
updates its provenance, and menu launch requires V3 admission. V1/V2 banks
remain references and are never a fallback. A separate passed V3 loading
profile is required by frontend capability admission; old receipts do not
authorize the changed binary or loading path.

Normal builds include the setup decoder. Portable packages stage its owned
wrapper/payload, exact codebook, loader and shared-library closure instead of
requiring the original loading tone. Setup checks that entire runtime snapshot
before and after conversion and invokes the owned loader with loader-injection
environment variables removed. These source changes still need a genuine
fresh/reuse/package/launcher loading interval and physical output qualification.
They do not close the D01 delivery gate in the
[ONLINE checklist](../multiplayer/docs/online-completion-goal.md), establish
authored MCC timing, or resolve complete MCC material/shader provenance and
authentic intro/credits/attract source assignments. Earlier supplied-asset
exceptions are superseded, not permitted delivery substitutes. Focused
synthetic Python checks establish transaction and
admission behavior only; they cannot manufacture a passed frontend receipt.

## Historical V2 implementation and evidence

The following records the earlier implementation and validation scope. Its
asset exception is superseded by the MCC-only requirement above.

The normal launcher Play path reuses the existing native menu/session owner:
title → profile → lobby → source countdown → resource request → loading →
native match. `NativeVistaOfflineFlow` requests loading at the actual resource
boundary. `DesktopLoadingBridge` retains the departing GPU scene, transfers
presentation to the prepared renderer, then joins the existing fade before
ordinary gameplay frames begin. It also supports the existing reverse handoff.

The animation, 33ms frame gate, original 1.5-second end fade and existing
loading shaders are reused. No estimated map duration or synthetic percentage
is introduced. The renderer's existing zero sweep-progress input remains zero;
that is separate from actual resource readiness and the animated sweep clock.

`mcc_loading_presentation.py` prepares `presentation/loading-v2/` in the private
MCC cache. Five bitmap roles still come directly from imported MCC
`mainmenu.map` through `load_mcc_loading_bitmap_inputs`. The sixth image is an
authored reconstruction using the MCC `rasterizer\glow` bitmap and loading-logo
alpha. The user authorized this reconstruction on October 20, 2026. It retains
the required multiplicative masking draw; it is not the original Vista raster
and does not establish exact Vista pixel parity. The recipe and source resource
identities are recorded with the generated mask.

The October 20 read-only audio search covered 63.3 GB of stored MCC data,
43 Halo 2 map directories and 38,647 decoded candidates, including embedded
frontend banks, Halo 2A/shared FMOD, CELT and quad ADPCM. No candidate established
identity with the original four-voice tone. This is bounded search evidence,
not proof that every MCC version lacks it. The previously imported
`MUS_Menu_Loading_Screen_01` is a different track. Search coverage, hashes and
controls remain in `.local/mcc-loading-ambience-20261020/report.json`.

At that earlier stage, the user authorized bundling the original tone if an exact MCC
reconstruction could not be established. This narrowly scoped exception is
`resources/audio/loading-ambience/loading-tone.xbox-adpcm`: 18,396 exact bytes,
SHA256 `4f61e827be9b54b71e3d37a145c9b88fb32eaf517e23746af85236ca41ba8fc9`.
Its manifest records the source executable hash, original offset and approval.
No original installation is needed for onboarding, packages or runtime.
The Xbox ISO/TU patch and full Vista loading bank remain offline references.

`NativeVistaLoadingAudio` reuses the recovered decoder, four voice frequencies
(21738, 16290, 29008 and 10876 Hz), original volume envelopes and gain word
semantics. Decoding yields exactly 32,704 mono S16 frames at 22050 Hz, SHA256
`b740acf4ce447da6583b4f6c7fd6696fabbb249664a53fb780b18c4dd58cd73d`.
The existing loading owner starts it before the graphics worker, forwards the
retained fade gain, then stops it after graphics join, including cancellation.
SDL mixes the voices for the host device. Exact source bytes, decoded samples
and recovered control behavior do not certify a physical DirectSound/Xbox
mixer or whole-game framebuffer parity.

Fresh onboarding authenticates the bundled manifest/tone before copying MCC.
Existing caches receive an atomic V2 bank repair; V1 is preserved for rollback.
Changed/custom banks are rejected without replacement. The new capability pins
the V2 manifest, reconstructed mask and original tone. Packages copy and verify
the resource bundle and no longer require the superseded Wwise converter.
The Wwise archive/decoder tools remain available for offline audit work.
That statement describes the historical V2 build. Normal current builds use
`HALO2_BUILD_MCC_LOADING_AUDIO=ON` and require Vorbis/Ogg for genuine MCC setup.
The old audit switch remains a compatibility alias for enabling the decoder.

`bundled_loading_ambience_tests` runs without any original installation and
checks the original instruction-execution worker/gain controls, decoder hash,
continuous loops, restart, concurrent gain/shutdown and failure cleanup. The
small source control fixture is included alongside the approved tone.

Validation receipts and the requested video are under
`.local/mcc-loading-original-tone-20261020/`. Earlier Wwise conversion and
animation evidence remains in `.local/mcc-loading-delivery-20261020/`.
The current delivery records atomic bridge admission, original audio controls,
visible animation, mixed loading/gameplay sound, movement/firing and launcher Play.
Whole-game TU1.5 parity and complete map/variant coverage remain separate work.
