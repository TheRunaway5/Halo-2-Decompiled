# Retail title screen and frontend

The native executable and launcher now start in `mainmenu.map`. The UI reads
the original disc resources at runtime; the repository does not contain copied
menu art, fonts, music, or a screenshot substituted for the scene.

The cache loader reads all **84 screen definitions / 150 panes**, their bitmap
and text widgets, button layouts, list skins, intro/ambient animation tracks,
localized strings, model scenes, and shared UI globals. It also reads all
**92 variant parameter definitions**, **31 option categories**, and **39 built-in
multiplayer variants**. The screen set includes internal, progress, error, and
older unused definitions; loading them does not make every backend functional.

## Available paths

| Menu | Native behavior |
| --- | --- |
| Title → profile → main menu | Original logo, prompt, authored flythrough cameras with roll, original scenery and looping human/Covenant flak with particle death explosions, retail font glyphs, blue UI overlay, complete retail WMA intro and four loop permutations, and ADPCM menu sounds. See [title flak evidence](TITLE_FLAK_EFFECTS.md). |
| Startup / attract movies | Decodes the disc's `intro_60.bik` at boot and `attract_60.bik` after 75 seconds without input while the title/main menu remains on channel five, including the verified profile/keyboard overlays. Original movie audio plays through SDL; a skip press is consumed by playback. Music restarts through the original transition debounce. Credits can be previewed explicitly. |
| Campaign | New game, level select, difficulty, and cooperative sign-in presentation. A single-player selection passes the chosen map and difficulty to the existing simulation. |
| Player profiles | Local create, select, edit, copy, rename, delete; controller, appearance, voice, online-status and subtitle menus. New profiles enter customization before sign-in. Delete and sign-out use the original confirmation dialogs; a signed-in profile remains protected after rename. Values persist in the host preferences file and are selected when reopening their menus. |
| Vista keyboard/video settings | Reference PC menus, custom primary/secondary keyboard and mouse bindings, sensitivity/inversion, display mode/resolution with confirmation and rollback. Renderer effects and input actions without native consumers report unavailable. See [source evidence and support boundaries](FRONTEND_VISTA_SETTINGS.md). |
| Appearance | All four color selectors, foreground/background emblems, Spartan/Elite selection, original model assets, source menu animation selection, weapon attachments and authored model-camera projection. Runtime pose, appearance bindings and complete rendered parity are tracked separately. |
| Game variants | All seven engine categories, built-in variants, complete match/player/team/game/equipment/vehicle option groups, quick options, every parameter's choices and descriptions, named local copies, rename and deletion. |
| Split screen / System Link | Profile sign-in, original local-host lobby bindings, map and variant selectors, setup and privacy menus. Built-in FFA Slayer starts with the signed-in local player or as a native UDP host. Additional local views and LAN joining use launcher arguments; cooperative and unsupported modes remain unavailable. See [lobby layout evidence](FRONTEND_LOBBY_RESEARCH.md) and [MULTIPLAYER_PROGRESS.md](MULTIPLAYER_PROGRESS.md). |
| ONLINE | Discord OAuth sign-in is required before entering the former Xbox Live screens. The verified Discord user ID stays internal; players use their chosen local profile name and can change it from Discord Account. ONLINE panels hide the retained title/main-menu logo. Browser sign-in uses PKCE and only the `identify` scope, with cancellation, expiry and sign-out. Friends data, matchmaking and service writes remain unavailable. See [Discord identity](DISCORD_IDENTITY.md). |
| Virtual keyboard | Retail keys and glyphs, shift/capitals, cursor movement, UTF-8 insertion/backspace, Done, generated profile names, name limits and duplicate checks. The source name-entry contexts disable Symbols/Accents. Physical keyboard typing is also supported. |

Preferences live under SDL's `Halo2Native/Halo2` application preference path as
`menu.cfg`. They are host-native profile/variant data, not compatible Xbox saves.
Profiles and saved variants are bounded to 32 each; writes replace the previous
file through a temporary file. Built-in variants cannot be deleted.

## Controls and inspection

Keyboard: arrows select, Enter accepts (commits text in the keyboard), Escape
backs out, X opens the current screen's secondary options, Y opens the online
menu. Mouse hover/click and right-click Back are supported. Visible controller
prompts are clickable on both their icons and labels, with a native hand cursor.
The title caption, keyboard keys and Vista binding captions also accept clicks.
See [mouse prompt coverage](FRONTEND_MOUSE_PROMPTS.md). Controller: D-pad
or left-stick vertical movement, A accepts, B backs out, X secondary options,
Y online menu; on the virtual keyboard X deletes, Y inserts a space, Start
commits the name.

```bash
./Halo2-Launcher
./build/halo2_native --menu
./build/halo2_native --menu-screen 47
./build/halo2_native --menu --skip-intro
./build/halo2_native --menu-movie intro --movie-frame 200 --frames 3
./build/halo2_native --menu --skip-intro --menu-tick 1400 --frames 3 --screenshot menu.ppm
./build/halo2_native --menu-audit
./build/halo2_native --menu-sweep analysis/frontend/sweep --hidden --mute --validation
```

`--menu-maps DIRECTORY` supplies an alternative extracted maps directory.
UI layout uses the authored 640×480 canvas and preserves its aspect ratio when
resizing. `--menu-screen` previews any available numeric retail screen ID;
parameter/category context is exercised by the sweep and normal navigation.
`--menu-tick` advances the actual 30 Hz flythrough script before rendering;
it selects reproducible scene views without changing the attract clock.
`Halo2-Title-Launcher` is a separate title-first shortcut. The standard
`Halo2-Launcher` keeps startup movie playback.

Startup decoding, movie playback and renderer loading do not consume the
title's 75-second idle interval. Input, title entry and movie exit reset it.
The host arms that interval after its first successful menu presentation,
including after movie decoding and exit. The disc's 5,500 ms pre-attract
setting requests a music stop at 69,500 ms; this initiates the authored
five-second fade rather than immediate silence. Source `1293F0` retires a
fading voice when its fade plus caller gain reaches −64 dB. Initial playback
and post-movie restart use the same `235B9C` transition rule: a subsequent
change waits until strictly more than 6,000 ms after the previous change.
An early trailer skip therefore cannot bypass that debounce.

Original `147F4F` reads channel five even when Choose Profile 10 occupies
channel three or keyboard 167 occupies channel two. Those known title/main
menu overlay contexts retain music on entry and the same 69,500/75,000 ms
thresholds. Other channel arrangements remain explicitly unresolved. The
live run in `analysis/frontend/profile-regression/native-idle-measured.json`
recorded the fade request at 69,511 ms, the attract decision at 75,014 ms,
and trailer playback at 75.186 seconds after presentation. That request
timestamp is not a measurement of the fade's eventual silence.

Menu music prepares the complete intro and all four ASF/WMA loop files,
then uses original `219110` selection state at every song boundary. The used
mask and previous choice prevent repeats before the bank is exhausted and
prevent an immediate repeat at a bank reset. Original-instruction checks
cover selection, seed initialization, fade gains/times and transition
scheduling; native mixer checks cover every sample through the complete intro
and five song boundaries. These establish the supported scheduling and decoded
file coverage. Xbox WMA waveform, hardware mixing, audio-driver cadence and
the global order of unrelated RNG consumers remain unverified. See
[the music research](FRONTEND_MENU_MUSIC_RESEARCH.md).

Start → Choose Profile no longer submits the large sharp title logo/prompt
as foreground dialog-parent geometry. The lower title widgets now receive the
original packed blue tint followed by four full-resolution blur passes. The
same single backdrop applies to Profile Name: live original channel snapshots
and the executed channel predicate prove that opening the keyboard disposes
the profile popup. All 1,228,800 RGBA pixels from the four captured original
blur inputs match the native GPU fixture. This verifies those filter passes,
not the complete native scene or background frame lifetime.

The profile picker reads its nested retail skin, original 21-pixel row union,
three icon frames, and Create New Profile stripe. Its focused row uses the
original independent focus opacity rather than multiplying another list fade;
mouse hit bounds cover the authored row union. Keyboard button art now applies
the original signed vertical offset, keeping glyph placement intact, and uses
the original focused/unfocused text colors. Profile-name Symbols and Accents
use their disabled artwork and cannot receive input. These fixes are covered
by normal title → profile → keyboard regressions using the retail assets.
The input field now follows the authored bitmap layer order, and profile creation
uses the original first-unused `Halo0001` naming rule. Text uses the original
packed colors, black shadows and native-color controller glyphs. Selection/caret
rendering is checked separately from steady field geometry. Saved rows on
screens 10/193 now use their own primary-color bar and the original nested
name/font/bounds, including the original name-color conversion. Screen 193's
heading follows the focused saved profile and disappears for Create New; focus
does not commit a different active profile. Independent original caller/skin
fixtures, semantic before/after tests and six native captures are retained in
[the saved-profile checkpoint](../analysis/frontend/profile-row-appearance/delivery.json).
Screen 193's thumbstick/button details now follow the focused profile using
the original executable's selectors. Create New shows Default for both.
The button menu admits the original four layouts and preserves their saved
values; the unused Jumpy asset is no longer exposed as an extra layout.
The unchanged constructor, writer, initializer and focused-selection fixtures
and focused regressions are retained in
[the control details proof](../analysis/frontend/profile-row-appearance/CONTROLS_SOURCE_PROOF.md).
Row emblems, per-profile progression details, keyboard
animated-depth projection, full transition timing and complete popup pixels
still need original/native comparison.
See [the popup evidence](FRONTEND_PROFILE_POPUP_RESEARCH.md),
[profile skin proof](FRONTEND_PROFILE_SKIN_SOURCE_PROOF.md), and
[keyboard geometry fixture](../tests/fixtures/frontend_keyboard_geometry.md).

## Source and validation

The behavior reference is the recovered source in
`tools/reference/halo2-decompiled`, revision
`5f08630655e3ddc9ac45909090f11ae799fa0c27`, matching the local retail XBE
SHA-256 `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
Relevant recovered routines include:

- `font_loading.cpp`: glyph headers, ARGB4444 pixel/run decoding and kerning.
- `unknown_23760b.cpp`: keyboard key indices, focus grid, label tables and actions.
- `unknown_253c8b.cpp`, `0x254173`: focused button bitmap frame selection.
- `unknown_136490.cpp` / `unknown_12c0d0.cpp`, `0x136600` / `0x12D6D0`:
  linear row alignment and the Xbox texture header's 64-byte pitch encoding.
- `unknown_21e330.cpp`, `0x21E410`: encoding 2 selects the stream codec;
  the menu music's resident bytes are ASF/WMA containers, despite their
  compression byte being 1.
- `unknown_223976.cpp`, `unknown_235b46.cpp`: 75,000 ms attract threshold,
  channel-five screen IDs, pre-attract fade request and music-state transitions.
- `0x219110`, `0x1462B0`, `0x12A810`, `0x1293F0`, `0x235B9C`: loop
  permutation history, secondary-seed initialization, authored fade values,
  retirement threshold and strict six-second transition debounce. Unchanged
  executable instructions supply the selector, initializer, fade and scheduler
  fixtures; start/stop audio backends are declared hooks in the scheduler oracle.
- `0x16FE90` / `0x1701F0` / `0x1703F0` / `0x170630`: camera observer
  interpolation, including preserved transition derivatives. The host samples
  between 30 Hz ticks without advancing the original simulation.
- `0x19530`: BSP light volumes' external texture slot 1 is the authored normalization
  cubemap. Its original `transparent_generic_reflection` vertex code,
  texture-coordinate transforms and pixel combiners execute on Vulkan.
- `0xBA8C0` / `0xBAB40` / `0x10AB80`: the carrier's `grav_lift_control`
  script input and its authored periodic `grav_lift_color` function.
- `0x17550` / `0x176A0` / `0x17960` / `0x13BC00`: animated bitmap properties,
  fixed-point color interpolation and pixel/vertex constant writes. The main
  carrier beam executes `plasma_alpha` and its secondary cloud layer; its
  emissive material also runs the authored noise/color overlay. Per-frame
  constant uploads wait for the previous Vulkan frame fence. Direct execution
  of the original instructions corrected a blue-channel error in the recovered
  `1B580` C++ source.
- `0x1BBF0` / `0x3F53D0`: material-reset winding and face-culling behavior.
  Authored forward passes retain RGB/alpha write masks, depth settings, alpha
  rejection and additive/premultiplied/alpha blending.
- `0x14B360` / `0x3EBD0`: visible cluster sky, authored clear color, sky scale,
  yaw and camera parallax. The menu's real city sky is drawn before geometry.
- `0x1301C0` / `0x1305D0` / `0x130DE0` / `0x25A10`: atmospheric and planar
  fog normalization, camera classification and vertex constants c12..17.
  The original title-beam vertex programs receive those constants each frame.
- `0x25E50` / `0x3A5C0` / `0x26710`: separate atmospheric-fog composite,
  including all 60 original program words, packed constants, depth quantization,
  submerged fallback and the original rasterizer gradient. The title stores
  integer W depth with the original near/far clipping distances. Authored layer
  7 illumination precedes this composite; layer 15 beam passes follow it.
  Per-surface planar-fog scheduling remains incomplete.
- `0x27AA0` / `0x28580` / `0x26880` / `0x26E50`: the title's four patchy
  haze slices, original noise bitmap, camera-motion state, random reseeding,
  depth-gradient lookup and premultiplied composition after layer 15.
- `0x22590` / `0x137A0` / `0x13870` / `0x214F0` / `0x211A0` / `0x20F30`:
  non-HDR cinematic bloom, the script's threshold override, original four-tap
  threshold/blur programs, five filter passes and screen-blend composite.
  `0x1D770` supplies fixed 320×240 haze and 160×120 bloom targets. Both effects
  use the scene viewport and exclude presentation borders.

Cache-layout facts were cross-checked against Assembly's Xbox `wgit`, `wigl`,
`skin`, `bitm`, `sily`, `goof` schemas at revision
`fd14ef3c46135e9422966c6a64cd926fb0bd23fb`. Their implementation was not copied.
Localized IDs stay scoped to their individual `unic` lists: the same ID can
mean different things in different screens.

`frontend_tests` checks malformed font runs and padded bitmap rows, actual retail
font/controller glyphs, scoped localization, profile creation/rename/reload,
option retention, saved variant copies/deletion, active-profile rename guards,
attract timer boundaries/reset paths, original BSP light-volume bindings, city sky selection,
all engine option branches,
all 150 panes and all 92 parameter dialogs. The systems test checks selected
campaign difficulty reaching a compiled script. Normal and ASan/UBSan runs,
GPU fixtures and resize/audio logs are retained under `analysis/frontend/`.
An earlier 2026-10-07 frontend revision passed all 18 CTest groups, six focused ASan/UBSan groups,
279 Vulkan menu fixtures and movie/title resize checks. Live playback logged
the attract transition at **75,006 ms**. These are implementation checks,
not a claim of matched Xbox pixels. The complete flythrough fixture has 2,991
original observer samples, with zero anchor-position error and no carrier
intersections after applying its original idle scale.

`retail_shader_animation` covers both carrier plasma layers and its emissive
material. `tools/differential_shader_animation.py` compares 693 sampled object
function and shader constant updates with the original executable: all 693
match byte for byte. It deliberately does not certify the GPU framebuffer.
Reproduce it with:

```bash
cmake --build build-frontend
.tools/python/bin/python tools/differential_shader_animation.py
```

`tools/differential_menu_fog.py` executes the four original fog routines and
compares all 24 native constant words over 3,044 camera cases: all match byte
for byte. `retail_menu_fog_gpu` checks the real shared Vulkan vertex interpreter,
including the complete material-buffer layout, against the arithmetic reference
for the BSP light volume and both carrier beam layers. The 840 fixtures cover
camera positions above/on/below the plane, depth saturation, and fog enablement.
These checks verify inputs and GPU varyings; they do not verify the final beam
color. See [the atmosphere research](MENU_ATMOSPHERE_RESEARCH.md).

`tools/differential_fog_composite.py` compares the standalone 25E50 composite with original
executable instructions over 3,598 camera heights: all 60 words and the general
path's lookup coordinates match byte for byte. `retail_fog_composite_gpu` checks
2,565 depth/gradient/combiner fixtures through the shared fog GLSL. An actual
frame probe with the pass enabled/disabled confirms it changes the rendered
title, but is not an original-frame pixel comparison.

The original Vulkan capture exposed a scheduling error beyond that standalone
check. Retail `25D50` selects `26520` for classification 3, and `25E50` for
classification 5 in this menu. `26520` draws at maximum W with LEQUAL and no
depth writes: it affects only clear-depth background pixels. The native title
now makes that selection, skips the zero-opacity eye pass, and preserves its
depth coverage. Run the executable comparison with:

```bash
.tools/python/bin/python tools/differential_fog_composite.py --selected-draw \
  --output analysis/frontend/xemu-capture/selected-fog-after.json
```

The previous unconditional path failed 2,787 of 3,598 cases; the selected path
matches every draw/skip decision and all 60 selected program words. In the
captured frame, the eye-fog pass changes exactly 118,320 background pixels and
leaves every geometry pixel unchanged. Its independent full-surface arithmetic
check is byte-exact over all 307,200 RGB pixels. `retail_menu_eye_fog_gpu` checks
4,096 captured pixels through the production fog GLSL and coverage rule.

`tools/differential_patchy_fog.py` checks 3,072 persistent camera/RNG frames
against the original calculation instructions. All 180 noise/cached/direct
program words and random states match; maximum coordinate/weight error is
5.96e-8. The GPU check covers 3,084 fixtures. The actual sky inputs (density
0.1–0.7, distance 0.1–50) and `bridge_fog` bitmap are used without screenshot
color tuning. Only the direct composition path is currently scheduled;
cached-target reuse around transparent groups remains incomplete. Matching
Xemu's noise placement also requires its initial RNG state and camera history.

The sampled haze path is checked against original GPU surfaces, in addition to
those calculation fixtures. The runtime now follows the original CPU-completed
float4 corner attributes and two-triangle quad, rather than evaluating the
affine UV rows per fragment. Unchanged `26CA0` and `27520` execute independently
for 512 cases each across six viewport sizes: all 327,680 attribute bytes match
the native noise and direct-composition constructors. Frozen fixtures are in
`tests/fixtures/title_haze_quad*` and `title_haze_direct_quad*`.

`retail_menu_haze_capture_gpu` runs the production quad shaders with exact
captured attributes, constants, depth, noise, gradient, and destination. All
76,800 original 8005 noise pixels match byte for byte, using both original
decoded texture bytes and the native decoded upload. Exact equality is required
for noise. Original 8034 direct composition now also matches all 307,200 RGBA
pixels exactly, and the test requires zero composition differences. Its final
one-byte mismatch came from GPU expansion of packed combiner constants: actual
hardware probes found one-ULP differences from Xemu's CPU-expanded uploads.
The title path now uploads all 18 float4 constants using the reference CPU
conversion. Captured inline D0/D1 alpha is retained through interpolation.
This changes no packed source constants and adds no pixel-specific correction. The old
inverse-row fixture had up to 9.54e-7 corner error and 277 differing noise
channels; it no longer supplies the sampled noise geometry. Original live
random/camera history and complete native-scene pixels remain unverified.

Reports and validation are `analysis/frontend/title-haze-quad.json`,
`title-haze-direct-quad.json`, `title-haze-cpu-constants-evidence.json`,
`title-haze-cpu-constants-tests.log`, and `title-haze-cpu-constants-final-aspects/`.
Five focused checks pass, including the
existing byte-exact captured bloom chain. The rebuilt title also passes 4:3,
16:9, 16:10, 21:9 and resize checks with centered 4:3 scenery, black borders and
no Vulkan validation errors. Full-width scenery and all-menu layout/flow
comparisons remain mandatory unfinished work in `FRONTEND_PARITY_GOAL.md`.

This comparison exposed two runtime sampling differences. Native BC1 sampling
produced up to three bytes of error in the noise pass. The title noise bitmap
now follows pinned Xemu 0.8.136's integer endpoint expansion and interpolation;
all 65,536 top-level RGBA texels match the original sampled image byte for byte.
The title sampler also locks min/max LOD to zero, as all four original noise
stages do; allowing the bitmap's lower mips was incorrect for this pass. The
actual native GPU capture confirms RGBA8 and the zero LOD limit. Other material
texture decoding is outside this change. See `title-haze-baseline-pixels.log`,
`title-haze-fixed-test.log`, `native-decoded-haze-state.json`, and
`native-decoded-haze_capture.rdc` in `analysis/frontend/xemu-capture/`.

`tools/differential_menu_bloom.py` checks 1,025 threshold/strength cases: all
180 program words and five filter radii match the executable exactly. Original
cinematic queries confirm amount/brightness 0.5 and strength 0.375 after time
zero. `tools/audit_title_post_targets.py` executes the original allocation and
texture-coordinate routines; 2,688 UV cases agree within 5.96e-8. The bloom GPU
check covers 6,144 fixtures. These bounded non-HDR checks do not certify a
matched original framebuffer. `--menu --viewport 640 480` supports captures at
the original authored framebuffer size.

`retail_menu_bloom_capture_gpu` now also runs the real production fullscreen
shader as a complete six-pass chain, starting with the original pre-bloom
surface. Threshold, all four filters, the final blend, and the following
authored scene colour filter are byte-exact across all 710,400 RGBA pixels in
this captured case. The filter's actual captured texture binding matches the
bloom output, and its native program matches the original shader asset and
uploaded constants. It asserts exact equality for every channel, with no
one-byte tolerance. The input world surface remains the original capture, so
this verifies the bloom/filter chain rather than complete native-scene parity. The
fixture exporter is `tools/export_title_bloom_fixture.py`; the results are in
`xemu-capture/title-bloom-filter-gpu-tests.log` and
`title-haze-cpu-constants-evidence.json`.

The rebuilt runtime's later `title-haze-bloom-final-tests.log` records 13 passing
focused checks with synchronization validation and no Vulkan errors. The
reusable `tools/verify_frontend_aspects.py` captures 4:3, 16:9, 16:10, 21:9 and a
1280×800-to-1000×600 resize; all presentation-border pixels are black and all
five runs pass validation. These still preserve the centered original 4:3 menu
viewport; full-width scenery and all-screen layout/flow parity remain open.
Screenshots are in `analysis/frontend/decoded-haze-aspects/`. The bounded pixel,
runtime state, launcher hashes and remaining scope are recorded in
`analysis/frontend/xemu-capture/title-haze-bloom-validation.json`.

The haze/bloom revision passed eight focused CTest groups and a live Vulkan
synchronization-validation resize capture. The 100-pixel presentation borders
in its 1000×600 capture are exactly black. See `haze-bloom-viewport-tests.log`,
`haze-bloom-viewport-resize.log` and `haze-bloom-viewport-border-check.json` in
`analysis/frontend/`. The later `planar-fog-tests.log` records ten passing
focused groups, including camera, frontend navigation and movies. The systems
checks also pass in `planar-fog-systems-recheck.log`.

The title BSP now submits its authored layer-16 planar fog between the depth
composite and layer-15 transparency. Both original gradient frames, vertex
instructions, pixel externs and the `00FFFFFF` constant blend factor are used.
The original aggregation, palette queries and visibility-flag instructions
confirm implementation 0 for all 70 BSP clusters while planar fog is active;
fully submerged classification skips that layer. Across 770 cluster/camera
cases, every used scalar/color input and all 60 bound pixel-program words match
the executable. The expanded GPU test checks 1,120 beam/planar cases, including
gradient sampling and planar pixel arithmetic. See `aggregated-menu-fog-state.json`
and `planar-fog-*` logs. Scene-object admission is checked separately below.

## Original Vulkan stage capture

`analysis/frontend/xemu-capture/title-vulkan_capture.rdc` contains a retail title
frame from Xemu 0.8.136 with Vulkan, 1x resolution and a 640x480 framebuffer.
The user's backend is Vulkan; their internal resolution and aspect setting
remain unspecified. The capture is an additional original reference and does
not establish those settings for their attached screenshots.

Draw order confirms a threshold pass at radius 0.5, then four blur passes at
1, 1.25, 1.5625 and 1.937500238418579, followed by the bloom composite. This
matches the existing native bloom schedule. An independent comparison of all
403,200 pixels across these six outputs differs by at most one 8-bit step.
The input to each check is the actual preceding original GPU surface, so this
isolates filtering from the native scene's remaining lighting differences.

The capture also contains the authored `effects\screen_effects\desaturate`
filter after bloom and before the title UI. It clamps a weighted brightness
using RGB weights `(128,179,51)/255`, then multiplies by `(26,35,64)/255`.
The native frontend loads that exact egor/shad/spas program from the original
map and runs it in the original position throughout its normal mainmenu world
view. Actual Main Menu6, Lobby14, Setup23 and Map206 captures confirm the same
full-strength effect and all60 bound program words as Title9 and Appearance47.
The original world service has no UI screen-ID gate. The native caller retains
the proved mode3, one-view, canonical640×480 source-canvas context, even when
that canvas is scaled into a larger host window. Movies suppress the effect.
The old initial blue-alpha UI quad is removed; separately proved channel tint
and blur remain after lower UI, including the lobby's23/206 popups. Ordinary
rendering and diagnostic sweep share this policy. See
`analysis/frontend/lobby-world-audit/SOURCE_ADMISSION_AUDIT.md` and
`analysis/frontend/customization/appearance-world-filter-audit.md` for the exact
render gates, live captures and separate framebuffer boundary. This bounded
integration does not add arbitrary offscreen or partial-view render contexts.
The full 307,200-pixel title-filter
comparison has maximum error one byte; the GPU regression checks 4,096 captured
samples against the production combiner.

Reproduce the original-stage arithmetic and coverage comparisons with
`python3 tools/verify_title_capture.py`; its report is
`analysis/frontend/xemu-capture/original-stage-pixel-check.json`. Seven focused
CTest groups pass in `eye-fog-tests.log`. Both captured-pixel GPU tests also
pass under AddressSanitizer. The six-frame resized title run passes ASan and
Vulkan synchronization validation, with both 100-pixel presentation borders
exactly black. `build-multiplayer/halo2_native`, used by the root title launcher,
was rebuilt with these changes.

Separate native and original stage captures show the remaining bridge/building
brightness difference already exists before fog and bloom. The native frame
uses a later camera phase, so those full images are diagnostic comparisons,
not matched-frame pixel evidence. `tools/audit_menu_lighting_mode.py` executes
unchanged original `2D000`: all 70 clusters and 104 instances classify as type
3. The menu lightmap group has bitmap `FFFFFFFF`, zero checksum, and no static
cluster or instance bindings. `4CBF0` selects layer 3 implementation 3 for that
classification. The native frontend now admits this authored variant for 179
BSP materials, replacing the incorrect baked-lightmap selection and its white
unbound texture. These variants have no live lighting externs. The bridge's
`x03_metal_grey` material matches all 18 original pixel constants and four active
vertex constants byte for byte against captured draw 1335. The carrier's opaque
A/B body materials also use implementation 3: all 18 pixel constants and four
active vertex constants match captured draw 1448 byte for byte. This admission
is scoped to the source carrier; it does not assume the same selection for
other scenery or preview objects. `retail_menu_lighting` checks both captures
and the real material selection. No brightness multiplier or invented
black-lightmap substitute has been added.

The carrier's emissive-only `capital_ship_c` surface also requires its authored
layer-7 state. The old surface binding retained the programs but dropped
additive ONE/ONE, RGB-only writes, alpha NOTEQUAL and disabled depth writes.
That submitted an opaque black depth writer and blocked later carrier passes.
The frontend now sends this rejected-lighting material through the existing
default-object emissive binder. The material test checks the literal original
state range and unchanged programs; a fresh native GPU capture confirms the
actual blend, alpha, color and depth state. The black patches are absent in
the corrected capture. Evidence is retained in
`analysis/frontend/title-camera-matrix-audit/carrier-emissive-state-audit.json`
and its `corrected-carrier/` directory. This correction adds no shader bias.

`--menu-tick 181 --freeze-menu` now holds the camera and authored animation
tick for capture instead of advancing it during rendering. Haze retains its
camera-motion and random history, so this does not freeze every pixel. Its live
time argument currently comes from SDL; the admitted title's cleared wind
history and zero animation period bypass time-dependent output, but an original
clock handoff has not been proved. The live haze now consumes the prepared
original inverse-view bank directly; the bounded raw BSP/fog connection and
fresh capture limits are recorded in
`analysis/frontend/title-bsp-admission-audit/LIVE_RAW_DELIVERY.md`.
The subsequent [live haze checkpoint](../analysis/frontend/title-live-haze-audit/DELIVERY.md)
records256 actual submitted canonical calls compared against unchanged original
instructions:298,542 checked words with zero differences after preserving the
source's binary32 centering operation. Original startup history, presentation
cadence and final framebuffer parity remain separate acceptance boundaries.
The initial frozen native capture has the exact original camera position but
its projection scale differed by 1.1188898. The missing path is now recovered:
`171D90` creates the observer's vertical angle, `2F800` multiplies it by the
single-view factor 0.785, and the standard-video safe rectangle produced by
`12560` / `16A30` / `2F6B0` is (36,48,444,592) inside (0,0,480,640).
`2F970` / `2FB70` consequently scale the projection axes by 544/640 and
408/480. The frontend uses this render angle and clip scale, while haze uses
the angle before clipping. `tools/audit_menu_projection.py` executes those
unchanged original routines and reproduces the captured X/Y projection scales
within 1.32e-7; `retail_menu_projection` checks three original angle fixtures.
The new frozen native GPU capture has the exact original camera position and
agrees with its captured projection scales within 3.7e-7. This is camera
evidence, not a whole-frame pixel match; the noise history and UI timing differ.
This implements the verified standard-video single-view path. The projection
oracle also checks 48 SD/HD, widescreen and cinematic input branches, including
the main-menu suppression condition. The lens helper matches every fixture
within 2.4e-7. A separate live original capture with `485AC2=1` confirms that
the mainmenu options (`flag=1`, `index=0`, `state=3`) suppress the widescreen
branch in `2FB70`; the captured scale stays at 1.5979406 / 2.1305876. See
`analysis/frontend/xemu-aspect/mainmenu-widescreen-suppression.json`. The research
copy's display input was changed through GDB; the executable code and options
condition were retained. This is not physical 16:9 video-output verification.
HD presentation, wider-than-retail aspect extensions, and frontend UI layout
behavior still need separate validation.

The title now uploads the recovered `2F970 → 2FB70 → 2FD90 → 12FA0` camera
bank directly through `menu_render_view_bank`, before embedded model views
replace their separate banks. The helper matches 4,608 exact original words
and all 48 words in the independently retained title GPU bank. The observer's
positive-trace quaternion conversion also preserves the original x87
`trace + 1` precision before its square root. Independent full-flyby replay
now matches all 63,084 state words across 3,004 steps both from original
per-step inputs and from continuously propagated native state. See
`analysis/frontend/title-camera-matrix-audit/FULL_FLYBY.md`. This proves the
captured authored observer sequence, not arbitrary observer branches, runtime
sampling phase, haze history or complete framebuffer parity. Generic world
W-depth reconstruction remains a separate comparison boundary.

Four fresh native captures at 640x480, 1280x720, 1280x800 and 1680x720 preserve
the centered reference 4:3 viewport. Every pixel outside the actual scissor is
black, and live Vulkan synchronization validation reports no errors. Images,
logs and border measurements are under `analysis/frontend/aspect-validation/`;
`title-aspect-grid.png` shows the current presentation. These checks do not
verify full-width scenery extensions or every submenu's layout and flow.

The carrier body also submits its original planar-fog layer. Original draws
1880/1885 use the same `fog_planar` shader as the BSP with 4,379/4,219 indices,
depth EQUAL, no depth writes, and the `00FFFFFF` blend factor. Both source body
materials select implementation 0. `retail_menu_carrier_fog` checks this
selection, all 18 packed pixel constants byte for byte, and the original six
active fog vertex constants within 8.8e-7 relative error. The two launchers were
rebuilt; seven focused camera/material/frontend/systems checks and a live
Vulkan synchronization-validation capture pass. The new image and capture are
`title-carrier-planar-fog.png` and `native-carrier-fog_capture.rdc`.

The reference frame has one four-slice noise draw (8005), followed by direct
haze (8034); it has no cached haze render target or reuse draw. Draws
3911/5617/6370/7807 are other transparent material draws, not the cached-haze
program at `276D0`. Cached composition remains relevant to other original
sorting branches, but is not an established cause of this frame's difference.

The entire frontend layout and navigation scope, aspect requirements, and
screenshot reporting are recorded in [the active goal](FRONTEND_PARITY_GOAL.md).

The supplied Vista archive now has a separate, optional 2D texture pack.
`--menu-ui-assets DIRECTORY` replaces compatible sampled art while keeping
Xbox layout/fonts and the current 3D backdrop. It is disabled by default;
the PC artwork is not treated as an Xbox pixel reference. See
[the export, runtime boundary and native checks](FRONTEND_OPTIONAL_UI_ASSETS.md).
The launchers select the prepared Vista GUI assets for ONLINE. See
[HD artwork, widescreen and mouse verification](FRONTEND_ONLINE_HD.md).

The three authored carrier engine glows now render. A fresh Vulkan capture
matches all 96 original quads' uploaded screen/depth coordinates, colors, UVs,
384 post-VS vertices, and 60 pixel-program words byte for byte. Captured depth,
active blend, filter, address, and LOD states agree as well. All seven
uploaded glow mip levels also match the captured texture. The original
world-node-before-marker composition order is preserved. These checks cover
the fixed captured title pose and camera; complete glow pixel equivalence and
complete transparent scheduling remain unverified. The original interleaves
54 additional billboard draws between these groups. Their three texture
identities now match every captured mip level byte for byte. An original
scenery/flak/death-effect asset chain is recorded, while the captured live
emitter identity and simulation remain open. See
[the engine-volume evidence](MENU_ATMOSPHERE_RESEARCH.md#carrier-engine-volume-source-trace).

The title-first launcher was rebuilt and installed against `build-frontend`.
Fresh 4:3, 16:9, 16:10, ultrawide, and resize captures with glow pass Vulkan
synchronization validation and border checks. Their screenshots and report
are under `analysis/frontend/title-particle-fragment-aspects/`; they retain the
centered source 4:3 viewport, with full-width extension still pending.

The original sprite calculation for the flak death effect's smoke, electrical
discharge, and sparks is now implemented up to GPU submission parameters.
3,072 controlled fixtures match the unchanged Xbox executable byte for byte,
with 192 independent retail cases retained for regression checks. This covers
atlas frames, interpolation, mirroring, tint/fade, size, and rotation. It does
not yet render the particles: simulation, emitter scheduling, transparent
sorting, and authored particle shaders still need implementation and checks.
The calculation consumes caller-supplied original-style values; no particle
state was guessed to fill the scene.

The original 17-value cache producer is now implemented. Executing unchanged
`173BA0`, `248DF0`, and `4EFB0` with 1,024 owner-state inputs yields 73,728
bytes identical to the native cache output. A separate read-only Xemu trace
provides 512 live calls across nine original particle definitions, including
the primary flak, soft/volumetric/fiery smoke, a flare, discharge and sparks.
Feeding their actual initial caches, owners, clock, camera, and sprite arguments
through the native cache producer and sprite calculation matches all 81,920
original output bytes. The loader also handles the original numeric parameter
alongside the fiery-smoke bitmap. See `tests/fixtures/title_particle_values.md`
and `analysis/frontend/title-particle-live-values-sprite.json`. This confirms
live definition identity for these observed calls, while their originating
scenery placement, emission/physics history, ordering and full-frame match
remain unverified. The frontend still does not supply live particle draws.

`for_each_particle_render_input` now implements original `175610` upstream of
sprite construction. It consumes completed simulation state in authored
emitter and linked-particle order, invalidates cached values when owner
identities change, evaluates emitter tint/alpha/scale, and transforms local
coordinates and first-person frames. Forty-eight read-only live original
groups reproduce 312 ordered handoffs/submissions and final caches: all
69,120 bytes match. Ninety-six separate controlled original-executable cases
cover local/parent coordinates, first-person frames, warm owner keys and empty
lists: 264 calls and all 63,360 expected bytes match. The executable's distinct
matrix addition order is preserved rather than inferred from recovered C.
See `tests/fixtures/title_particle_render_inputs.md`. The upstream emitter
placement/rate/request producer and successful creation prefix now have
separate exact comparisons below; live simulation history and integration
remain unverified. The frontend does not yet generate these completed records.

Original `249170` particle initialization now reads the authored lifetime,
speed, rotation, shape, radius and angle. The native sphere, inward-sphere and
ring branches preserve the original RNG/table selection and observed Xemu
normalization arithmetic. All 128 live initializers match 8,704 particle/RNG
bytes. A separate 3,078-case unchanged-executable comparison spans all 1,026
original direction indices and matches all 209,304 bytes. Other shapes are
explicitly rejected. See `tests/fixtures/title_particle_birth.md` for evidence
and the physical-Xbox normalization limit. This initializes particles from
explicit values. Separate emitter/creation work now covers measured placement
and scheduling boundaries below. Record allocation, child-system linking and
motion/physics remain unfinished. It does not yet produce visible particles
in the frontend.

Original `247470` emitter updates now match 256 live updates and 472 ordered
birth requests, all 63,904 bytes. A separate 256-case executable oracle covers
burst/spread/scaling flags, null frames, offsets, orientation and threshold
boundaries: 1,109 requests and all 122,508 bytes match. Creation callback
particle heads/counts are explicit inputs, not a claim of complete simulation.
See `tests/fixtures/title_particle_emitters.md`.

The successful `247950` creation prefix through `247E2A` now matches 256 live
calls, all 58,368 particle/cache/emitter/RNG bytes. This covers linking a
supplied cleared slot, random fields, cache completion, shape initialization,
placement, inherited velocity, color and count. Native slot allocation and
the subsequent child-system/birth-effect dispatch remain unfinished. See
`tests/fixtures/title_particle_creation.md`. None of these CPU boundaries
establish complete title pixels or any menu layout/navigation route.

The complete original `23ED30` vertex submission is now implemented separately
from sprite calculation. All 46 packet words match the unchanged original
instructions in 192 independent fixtures, including explicit attribute-binding
permutations and quad corner data. Direct inspection and execution resolved an
incorrect pointer order in the recovered C prototype: UV A goes to default
attribute 3, geometry in EBX goes to attribute 6, and UV B goes to attribute 7.
See `analysis/frontend/title-particle-submission-differential.json`.

The original particle vertex programs can now be loaded by explicit
classification. Seven captured original draws provide 28 vertices for each
of the five resident candidate programs. The interpreted classification-7
probe retained three different output words; direct compilation of its actual
51 disc instructions now matches all **1,008 compared float words** byte for
byte, including depth, W and fog alpha. `original_particle_vertex_gpu` checks
independent original capture records with no tolerance. The probe embeds no
fixture values or expected output vertices. See
`analysis/frontend/title-particle-vertex-specialized.json`.

The source-backed particle declaration selector is also implemented. Executing
original `2243C0`, its original virtual methods and `23EA80` confirms that
the three actual definitions select first-LOD layer 15, pass 0 and vertex
classification `7 + authored style`, which is 7 for these definitions. Native
dependency selection and complete program bytes agree with those original
calls. Six render-setup services are intercepted for this bounded check and
listed in `analysis/frontend/title-particle-shader-selection.json`; live
emitter identity, pixel implementation selection and full rendering remain
unverified. The particle graphics path now uses the separately translated
class-7 program; the production world shader retains its existing arithmetic.

Particle material loading now reads the authored template/pass implementation,
constants, bitmap mappings, states and samplers. Implementation choice remains
an explicit argument until the original live dispatch is reproduced. All six
implementations of the three bounded title particles match **780,078 reference
bytes**, including every bound captured RGBA8 mip texel. BC1 expansion shares
the integer decoder already verified for haze, avoiding GPU endpoint rounding
differences. `original_particle_material` retains independent original records.
See `analysis/frontend/title-particle-material.json`.

The native combiner and new W-depth calculations also match color and depth
buffers at 80 original captured particle draws, including 61 visibly contributing
draws. This replacement replay preserves the original geometry, interpolation,
textures and pipeline state. It does not prove a complete native graphics
pipeline or dynamic material-buffer arithmetic by itself. The native particle
fragment shader compiles, while frontend particle scheduling remains pending.
See `analysis/frontend/xemu-capture/particle-fragment-replay-comparison.json`.

The production vertex and geometry shaders now translate the original class-7
tokens, screen quantization, output registers, quad split and triangle W-depth
inputs. A combined graphics replacement replay preserves color/depth bytes
across all 80 sampled draws and all 15,288 captured post-stage bytes in seven
vertex/geometry samples. See
`analysis/frontend/xemu-capture/particle-graphics-replay-comparison.json`.

`ParticleRenderer` provides native Vulkan uploads, source material/sampler
bindings, dynamic material uniforms, depth clamp, original quad submission,
W-depth and blending. The production component matches 12 original captured
draws with **zero color/depth differences** using D24 and D32 attachments,
including all five visibly contributing plain-program draws in the capture.
This test invokes the actual native component with completed original state;
it does not substitute a shader into Xemu. `original_particle_graphics_gpu`
checks these captured inputs. See
`analysis/frontend/title-particle-native-graphics.json`.

`particle_view_constants` constructs 23E460's 22-vector upload at physical
registers 148..169. Its fixed rotation table and camera transpose match 90,112
original bytes across 256 inputs; the original routine's upload also matches
both sampled capture buffers byte for byte. `original_particle_view_constants`
retains independent original records. The live camera producer and dirty-flag
scheduling remain outside this constructor. See
`analysis/frontend/title-particle-view.json`.

The renderer accepts ordered completed `ParticleDraw` records, but the frontend
does not yet supply them. Original live emission/simulation, pass dispatch,
camera-state production and full transparent scheduling still need to be
implemented. Captured records remain diagnostic fixtures; they are never used
to synthesize particles in the live title screen.

## Fidelity still pending

This is **not yet a 1:1 retail frontend**. Layout resources and original assets
are present, and Xbox reference captures expose remaining atmosphere and
lighting differences; there is no passing matched-camera pixel comparison.
The world renderer still lacks complete original material passes, preview
cinematic lighting, shadows, other scene-object planar fog and cached haze
composition in the sorting branches that require it. The BSP and carrier body
planar-fog passes, direct haze and non-HDR bloom programs now run. The
carrier's two plasma programs and animated inputs also run, but their final
appearance still depends on missing lighting and pass-selection paths. Fresh native captures still differ from the
user's Xemu Vulkan references in surface brightness, background atmosphere and
beam appearance. Bloom services now update the actual renderer. Menu
initialization still ignores unimplemented lighting services; camera,
object-name and `object_set_function_variable` services run. The carrier's opaque
A/B materials now select the original authored lighting variant, whose active
constants agree with the original draw. Complete appearance comparison and
shadow layers remain incomplete. There is no global brightness multiplier
standing in for authored lighting inputs.

Model previews now use the original combat/UI animation selections, retained
30 Hz animation clocks, authored object placement, weapon attachments and
model-widget cameras. The original camera arithmetic and animation-channel
transitions have retained executable comparisons. The pose path uses literal
packed animation samples, grip overlays, held-weapon constraints and the
original deferred render-node sampling order. Weapon attachment locals are
initialized from the source-selected startup pose and retained across later
animation changes. Captured final world-node comparisons are tracked separately
from local samples and authored placement; none alone proves rendered pixels.
Both retained visible poses match all 1,144 original world-matrix words, and
all ten authored roots match the original position, facing and scale.
Armor recoloring, emblem compositing, preview lighting and complete rendered
model parity remain pending. See
[the model-scene evidence](FRONTEND_MODEL_SCENE_RESEARCH.md).
Dynamic player/rank tables and some service-populated text are not drawn.
Widget layering, blend methods, list spacing, exact transition curves/outros and
prompt pulse timing still need retail visual comparison. Polygon shape groups
and obsolete table views are empty in this disc's menu cache.

Controller/voice/subtitle preferences are stored and editable in these screens;
full gameplay application of those preferences remains work for the game loop.
Difficulty is passed to scripts, while the existing simulation's difficulty
scaling remains incomplete. Campaign resume, unlocked progress, saves, cooperative
sessions, retail Xbox multiplayer protocols, Xbox Live and dashboard/account services are outside
the current native runtime. Native LAN and local Slayer are available with the
limits documented in [MULTIPLAYER_PROGRESS.md](MULTIPLAYER_PROGRESS.md).
GPU execution and asset coverage do not prove those
services or retail visual equivalence.
