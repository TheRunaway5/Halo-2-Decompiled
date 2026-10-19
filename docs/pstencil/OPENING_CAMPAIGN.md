# Opening campaign presentation

**2026-10-19 delivery: campaign-v10.** `./Halo2-pStencil-Campaign` now adds
the recovered [texture accumulation](TEXTURE_ACCUMULATION_2003.md) on compatible
current material programs. Its native regular-Elite color response matches the
accepted v8 image while preserving the existing metallic Arbiter and visor paths.
The complete native opening returns0 in228.54 seconds with1180 main frames,
zero script faults and zero Vulkan validation messages. All20 Original
roundtrips and20 full-material restores match;14 light/shadow pairs are positive,
with no shadow brightening. Four fresh F1/mouse selections on this runtime pass
in140.73 seconds: Original,pStencil,Original,pStencil. The receipt is
`analysis/pstencil/campaign-live-v10/live-selector/result.json`.

Fresh group/visor images are under
`analysis/pstencil/texture-accumulation-native-v10/native-group/` and `native-visor/`.
They use5120×3840 internal targets; point shadows darken426,394 and472,817 pixels
respectively. Three actual opening images and their hashes are retained in
`texture-accumulation-native-v10/opening-result.json`. The earlier v9 texture
candidate darkened blue Elite armor and was rejected; it was never installed.
The corrected input-context regression fails before the fix and passes afterward.

The fresh native recording is
`analysis/pstencil/campaign-live-v10/recording-window-native/opening-presentation.mp4`
(200.8045 seconds,1920×1080,H264/AAC). Its1128 native frames complete with zero
script/Vulkan errors and17,926,710 nonzero audio samples. Only12.974301 seconds
of verified silent startup were removed, with0.5-second preroll; the uncut
reference is retained. No interpolation or simulation changes are used.
Native interval median131.381ms,p95374.246ms,max686.525ms under current host
load remains visibly uneven; final dialogue balance/synchronization is unverified.
The runtime/shaders, requested native photographs, selected opening frames and
two encoded recordings are explicitly retained in the v10 storage receipt.
Its `storage-cleanup.json` verifies removal of owned build/capture workspaces;
temporary archives, object files, raw frame controls and menu photos are gone.

Full May2003 material/scheduling coverage, original framebuffer comparisons and
smooth native pacing remain unfinished. The v8 results below retain their
historical scope and do not certify every opening asset or receiver face.

**2026-10-18 delivery: campaign-v8.** `./Halo2-pStencil-Campaign` now uses the
[recovered world-normal accumulation path](NORMAL_ACCUMULATION_2003.md), including
compatible current Elite and Arbiter materials. The complete native opening
returns 0 in 238.32 seconds with 621 main frames, no script faults and no Vulkan
validation errors. All 20 Original roundtrips and 20 full-material restores match;
14 light/shadow pairs are positive, with zero shadow brightening.

The actual GPU group frame matches its ordinary native screenshot exactly.
Fifteen normal producers write target9 before fifteen normal utilities read it;
the exported target contains 1,041,781 written normal pixels and preserves bump
alpha. The same new renderer passes the 72 existing stencil/cap GPU cases.
Disabling only the normal-path adapter reproduces the previous v5 pStencil group
image exactly. These are bounded integration checks, not full May 2003 GPU parity.

The fresh recording is
`analysis/pstencil/campaign-live-v8/recording-window-native/opening-presentation.mp4`
(200.4725 seconds, 1920×1080, H264/AAC with game-only audio). The uncut capture
is retained. Only 11.814804 seconds of verified silent startup before the first
native scene submission were removed, with 0.5-second preroll. No simulation or
video interpolation changes are used. The native recording returns 0 with 2662
frames and zero script/Vulkan errors; audio has 17,929,211 nonzero samples.
The observed native interval median is 39.5191ms, p95 is 260.827ms and maximum
is 735.949ms. This remains a recording with native hitches, not a smooth 30fps
certification or a complete May 2003 presentation match.

Fresh group/visor images and controls are under
`analysis/pstencil/normal-accumulation-native-v8/native-group/` and `native-visor/`.
They retain 5120×3840 internal targets. Point shadows darken 426,390 group pixels
and 472,818 visor-view pixels without brightening. Full material/light-family
coverage and smooth native frame pacing remain unfinished.

**Previous campaign-v6 recording update:** the explicit recording presentation option
removes the measured one-second FIFO image wait on this output-less Xwayland
surface. Default game presentation remains unchanged. The full native opening
recording with game audio is retained at
`analysis/pstencil/campaign-live-v6/recording-window-native/opening-presentation.mp4`
(208.7385 seconds,1920×1080, startup before the first scene frame removed with
half-second preroll). No simulation changes or video interpolation are used.
Native frame hitches and full material parity remain unfinished; see
[capture presentation evidence](CAPTURE_PRESENTATION.md).

**2026-10-18 delivery: campaign-v5.** `./Halo2-pStencil-Campaign` selects
Original or pStencil through F1 → Graphics → Graphics Mode. The corrected
[shadow-cap transform](SURFACE_CAP_ALIGNMENT.md) removes the sampled false
stencil self-shadows on Prophet metal and skin without changing the shadow
comparison or darkening. Actual GPU replay matches the native screenshot;
10,158 exported matching surface/cap vertices now have identical clip positions.

The complete 2560×1378 diagnostic exits normally in 229.81 seconds with 865
native frames, zero script faults and zero Vulkan validation errors. It passes
20 exact Original roundtrips and 14 positive light/shadow pairs with no shadow
brightening. Four actual F1/mouse mode changes pass in 123.07 seconds.

Fresh three-character and visor captures are under
`analysis/pstencil/surface-alignment-native-v5/native-group/` and `native-visor/`.
They use 5120×3840 internal targets. Original remains byte-identical to v4 in
both views. Point shadows darken 423,784 group pixels; the visor reflection
control changes 48,239 pixels. Full May 2003 presentation remains unfinished.

**2026-10-17 baseline:** campaign-v4 retains the coherent
current-engine opening at 1920×1080 in pStencil. F1 → Graphics → Graphics Mode
selects Original or pStencil. Its manifest verifies the tested binary, all 66
runtime shaders, the TU1.5 reference and the three campaign caches. The general
`Halo2-pStencil` launcher continues to select its stable gameplay runtime.

Campaign-v4 adds the recovered 2003 utility6/10 and dependent7 lighting passes,
with a real RGBA8 utility target, full current texture RGBA and the existing
stencil test on both passes. Its static authored cinematic gel images feed
their actual projected lookup and specular masks. The complete 2560×1378
native diagnostic finishes in 229.62 seconds with 968 native frames, no script
faults or Vulkan validation messages, 20 exact Original restorations and 14
positive light/shadow pairs with no shadow brightening. The new response
submits during real campaign shots, with up to 204 companion draws in a
captured frame. Counts alone do not establish an original framebuffer match.

The new high-resolution three-character and visor captures use 5120×3840
internal stencil/color/depth targets. Their sphere rendering and Original
images remain byte-identical to the prior sphere integration. The visor's
reflection control changes 48,239 pixels with no darkening. Actual source
constants, current/original vertex comparisons and per-draw image-free binding
checks are documented in [projected response](PROJECTED_LIGHT_RESPONSE_2003.md).
Animated families, accumulated normals, complete original face/asset coverage
and recording pacing remain active work.

The fresh campaign-v4 native-desktop owned-window recording retains the
complete opening and actual game-only audio. Its H264420/AAC preview is
`analysis/pstencil/campaign-live-v4/recording-window-native/opening-preview.mp4`
(210.0915 seconds, 1920×1080, untrimmed startup). The run has 3,207 native
frames, zero script faults and zero Vulkan validation messages. Audio contains
17,922,960 nonzero samples, peak0.8974 and RMS0.05944; final dialogue mix and
synchronization remain unverified. The uncut H264444/FLAC recording is retained.

Actual interval median is28.60895ms, p95207.237ms and maximum1622.49ms. Native
window presentation median is0.088399ms. A separate retained private-Xvfb
recording of the same build had37.5678ms median presentation and101.263ms median
interval; it is a diagnostic capture environment, not the selected preview.
No frames are interpolated. Larger source-side stalls remain. The sampled
Prophet point-light stippling in this retained v4 video is addressed by the
v5 shadow-cap alignment fix; this older video retains the defect.
The four actual F1/mouse mode transitions pass in104.83seconds.

Campaign-v3 retains the same renderer, shaders and visor response while
updating only the actual scene material roots plus shared-pass owners. All 20
same-frame active/full material images match exactly, alongside the existing
20 Original restorations and 14 positive light/shadow pairs. Matched native
playback lowers median interval from 44.06 to 31.09 ms and material update cost
from 8.56 to 1.50 ms. Its actual F1 selector passes all four switches. The fresh
209.54-second untrimmed recording has game audio, 3,255 native frames and no
script/Vulkan validation errors. It is retained at
`analysis/pstencil/campaign-live-v3/recording-window/opening-preview.mp4`.
Median native interval is 34.33 ms; large stalls and full presentation work
remain. See [pacing](CAMPAIGN_PACING.md) for the measurements and boundaries.

The lighting component has [verified original programs and sphere
inputs](LIGHT_RESPONSE_2003.md), including the actual opening cache. All 560
constructor rows and 4,480 two-stage GPU fixtures pass. The retained
campaign-v3 delivery predates their renderer integration; campaign-v4 now
consumes them. A matching May 2003 GPU frame and full presentation parity remain
unverified.

The campaign-v2 delivery adds the recovered six-combiner environment
response to the [combined Chief visor](COMBINED_VISOR.md), keeping its current
bump/cube, bitmap frames, affine UVs and texture alpha. Its full opening run
completes in 222.66 seconds with 2,668 native frames, zero script faults and zero
Vulkan validation messages. Fourteen paired images have positive light and
shadow pixels with zero shadow brightening; all 20 Original roundtrips restore
exactly. The visor companion submits 367 times during actual opening playback,
all at shader LOD 0. All three LODs pass separate actual-cache CPU checks.

The revised native Cairo close-up has 48,234 reflection-dependent pixels.
Its Original image is byte-identical to the previous delivery. New group and
visor images/controls are retained under
`analysis/pstencil/visor-response-v1/candidate-v5/`. The immutable parent build
remains available; campaign-v2 pins only its own renderer/visor/shader overrides
against that coherent engine cohort. Complete original MRT/alpha scheduling
and recording pacing remain unfinished. The actual campaign-v2 F1 selector
passes all four mouse/keyboard transitions in 108.81 seconds, including resumed
native shadow owners after switching back to pStencil.

The fresh campaign-v2 owned-window recording completes in 227.85 seconds with
2,281 native frames, zero script faults and zero Vulkan validation messages.
Its actual game-only audio has 17,926,913 nonzero samples, peak 0.817 and RMS
0.05963. The untrimmed 208.59-second H264/AAC preview is retained at
`analysis/pstencil/campaign-live-v2/recording-window/opening-preview.mp4`;
the H264/FLAC source and complete receipts remain alongside it. Startup is
included and no native frames are interpolated. Final dialogue balance and
synchronization remain unverified.

Across all render intervals after the first, the median is 57.97 ms and p95
215.57 ms. The renderer median is 9.79 ms. This remains a diagnostic recording,
with CPU/source preparation and full original presentation still active work.
The campaign-v1 recordings discussed below are retained history, not the
revised visor response.

The earlier current-engine-v1 playback completes the opening in 222.18 seconds,
rendering 2,581 frames with zero script faults or Vulkan validation errors.
At an actual 2560×1378 framebuffer, 14 paired captures have positive lighting
and shadow contributions, zero shadow brightening, and 20 same-frame mode
roundtrips restore Original byte-exactly. The launcher smoke also opens the
default PipeWire audio device successfully; final audible mix and timing remain
separate acceptance work. The material response and frame pacing still need
work before calling this a finished recording presentation.

The actual campaign F1 selector also passes four consecutive X11 keyboard and
mouse transitions: pStencil → Original → pStencil → Original → pStencil. The
live frame trace observes every requested mode in the campaign loop, including
resumed distant-shadow submissions after switching back. UI screenshots and
the transition receipt are in `campaign-live-v1/live-selector/`.

The first full Xvfb recording is retained as a failed audio control: SDL chose
the normal speaker sink while the recorder listened to an empty null sink.
Its audio track is entirely zero. The recording helper now routes only its
child game's actual PulseAudio streams to the game-only recording sink.
A live three-second probe of that sink has 288,115 nonzero samples and peak 0.423.
This establishes captured output, not the final dialogue mix or Xbox timing.
Capturing the actual game window also avoids the measured 37 ms median Xvfb
presentation stall. The replacement owned-window recording completes with
2,540 native frames, zero script faults and zero Vulkan validation errors.
Its stereo track has 17,922,771 nonzero samples, peak 0.798 and 188 active
one-second windows. The uncut H264/FLAC capture and receipts are under
`campaign-live-v1/recording-window/`. Final dialogue balance and timing still
need inspection.

The browser-compatible `opening-preview.mp4` is 1920×1080 H264/AAC and lasts
195.58 seconds. It removes only the first 14 blank, silent startup seconds;
the original uncut recording remains alongside it. It is a diagnostic preview,
not a completed visual or audio parity recording.

Across that full run, the median native render interval is 55.59 ms and the
95th percentile is 223.66 ms. The renderer itself takes a median 9.00 ms;
presentation takes 0.09 ms. These measured stalls keep recording pacing open.
The video's 30 fps stream includes repeated presented frames and must not be
reported as 30 distinct native frames per second.

New native Cairo photos stage Chief, Arbiter and an Elite together under a
declared steady point light. They use 2560×1920 output and 5120×3840 internal
stencil/color/depth surfaces. The group shadow control darkens 422,259 pixels
without brightening any channel. The visor close-up's authored-cube control
changes 47,567 pixels. These are actual current-engine frames, not matched 2003
frames. See `analysis/pstencil/campaign-live-v1/armor-group/` and
`analysis/pstencil/campaign-live-v1/armor-visor-v2/` for images and controls.

**Earlier private validation:** private campaign-v21 completes all13 scenes
with zero script faults and Vulkan validation errors. The coherent v75 speech
join observes all20 facial-capable cues. All 20 same-frame Original/pStencil/
Original checks restore Original byte-exactly. Fourteen lighting/shadow image
pairs show positive light and shadow pixels, with zero shadow brightening.
These include the early character scenes and late armory scenes, captured at
an actual2560×1378 framebuffer. No finished recording or installation is
claimed.

The actual projected GPU bridge now includes
[V10/V11 material vertex attributes](PROJECTED_VERTEX_ATTRIBUTES.md) and
[projected fog alpha](PROJECTED_FOG_ALPHA.md). Constants-only completion avoids
reloading resident textures per command; cached coherent readback reduces the
median720p screenshot stall from312ms to3.8ms. These changes preserve the
original shader bank and normal upload memory selection.

The missing early pair now has its complete native
[scenario-light producer](SCENARIO_LIGHTS.md), verified against unchanged
Xbox execution. Actual early44710/40E80 snapshots match unchanged CPU execution;
the missing main-view queue handoff is now fixed, restoring their color
admission and positive light/shadow pixels. The current-source module borrows
the complete modern main view; its fresh engine build passes the full opening.
Remaining material response,
audio timing and an uninterrupted inspected recording remain acceptance gates.

The requested deliverable is pStencil throughout the complete opening campaign
cinematic, including everyone and everything in its shots, with an uninterrupted
recording-ready presentation. Current levels, geometry, animation and assets
remain authoritative inputs. Later assets need supported 2003 shading stages;
they do not receive guessed historical material values.

The existing campaign renderer accepts `--graphics-mode Original|pStencil` and
shares the live F1 graphics selector. Existing pStencil material adapters reach
prepared campaign materials. The new own `campaign` adapter submits actual
posed biped/vehicle geometry with a valid raw native incident sample. It runs
complete TU1.5 `3DFD0` independently of visible cinematic lighting, retaining
its failed-query retry interval and accepted-cache interpolation. A real Cairo
floor test first accepts on native render frame62 and preserves the cinematic
flag and default lighting bank. No initialized fallback vector is admitted.

The coherent private `campaign-v3` runtime includes the existing native camera,
scene, animation and authored audio cohort, the source-backed per-cluster sky
repair, native cinematic-lighting bridge, current neutral Elite detail and
material masks, and the new distant-caster join. Its199.74-second diagnostic
run completed all13 authored main scenes, rendered2293 frames, mixed native
audio and reported zero script faults or Vulkan validation errors. It observed
506 frames with accepted distant casters across13 distinct native owners.
Ten native frame captures and the diagnostic recording are retained under
`analysis/pstencil/opening-campaign/campaign-v3/playback-pstencil/`.
These counts establish submissions, not positive shadow pixels in every shot.

The white-background stage shot remains visible at tick972, including with
per-cluster sky resources. That existing Original defect, cast/shot coverage,
frame pacing, effects and full presentation acceptance remain unfinished.
The recording is diagnostic and is not installed as a finished opening.

The new own `native_lights` adapter copies the actual complete350-record native
light bank plus both2048-record spatial lists, retaining salts, occupancy,
registered ordering, attachment/effect state and native retirement. It never
allocates/registers/ticks source lights. `campaign_lights` runs private source
view fades/queries over that snapshot and complete native `3E9B0`/`BCFB0` for
owner priority/root identity. The real heated Plasma Rifle camera/PVS test
retains its native light generation in visible/candidate output and preserves
all native light/spatial records and the complete owner body. A timed plasma
explosion exists in the snapshot only until the normal native tick retires it.
The first private `campaign-v4` run stopped at the previously unimplemented
frustum definitions. The corrected `campaign-v5` retains those as explicit gaps
and completed the full sequence with audio:214.28 seconds,1762 rendered frames,
zero script faults and zero Vulkan validation errors. Its10 paired native
captures render the same completed frame with/without submitted casters, without
advancing native queries or simulation between the pair. Six pairs have positive
darkening, with zero brightened channels. At target3400,236242 pixels darken;
at target4700,143086 pixels darken. Four other cameras have zero shadow pixel
changes and are retained honestly as controls. No sphere point-light draw
submitted during that opening run; the authored cinematic frustum shading is
still unimplemented. See [native shapes](NATIVE_LIGHT_SHAPES.md) for recovered
shape/camera inputs, source pass evidence and remaining GPU/query owners.

The opening cache contains 79 resident object definitions and 288 material
definitions. The new pStencil resource audit visits all567 authored shader LODs.
Against the retained pStencil material owner, 522 prepare under cinematic
classification2. It finds24 supported independent environment responses,
47 recognized combined environment-strength responses, 105 texture-mask passes,
and10 exact Elite neutral-detail passes. These counts include resident unused
resources; they are not a live shot coverage claim. Layer15-only backdrops and
other unlit/effect materials need their own consumer. Their campaign producer
has separate current work and must be included coherently in a recording build.

The [object/material matrix](../../analysis/pstencil/opening-campaign/object-material-matrix.json)
retains every resident object link, shader template, prepared LOD response and
literal local2003 loose-path match. It includes the native Elite variants,
Tartarus, the Prophets, Chief, Marines, Banshees, gravity thrones, cinematic
ships, scenery and matte paintings. Exact paths identify comparison candidates;
they do not establish equivalent material values or historical absence.

The remaining acceptance sequence is:

1. Verify the coherent campaign/pStencil join across all actual cast and shots.
2. Submit actual attached/effect light state, native selected caster geometry and
   receiver faces; retain offscreen and shadow-only provenance.
3. Establish positive distant-shadow pixels and controls across the actual
   campaign receivers, using the recovered2003 construction and accepted native
   incidents. Complete original manifold/opacity/ISQ admission remains separate.
4. Verify every authored shot with real geometry, material LODs, poses, lighting,
   environment and transparency. Include later assets and scene transitions.
5. Record the uninterrupted sequence with audio, inspect it, and deliver both
   the recording and useful close-ups. Verify Original restoration separately.

The shared [opening playback status](../OPENING_CINEMATIC_STATUS.md) tracks
remaining visibility, presentation and audio/recording gates. Executing all13
scripts does not establish finished pixels or synchronized playback. The main
pStencil goal remains active.


The current isolated follow-up retains the large-light/native admission work
in its own pStencil modules. Whole49790 supplies native projected constants;
whole134C00/31650 and3817 authenticated missing portal/admission instructions
supply the actual single-camera receivers and classifications. The real
opening-cache fixture verifies all five named cinematic lights, including the
large Arbiter/Prophets portal branch, and120 actual Chief/Elite armor/LOD
material bindings.720 unchanged-original selector cases match. This is CPU and
compiled-binding evidence, not native campaign light pixels.

The freshly rebuilt live-v15 candidate passes12 focused checks, including the
native dynamic-biped projected receiver/collection, Original restoration on a
real map and the live graphics selector under Vulkan validation. It is not
installed. The gameplay launcher continues to select stable live-v14.

The private campaign-v6 diagnostic reached c01_intro_scene_02 but stopped on
its old sphere-only candidate admission, rather than completing. v7 confirms
that early flashlight records really are inactive (flags1, zero bound/fade/
RGB); those must remain dark. The candidate therefore uses original31570
admission after the actual main collector's C35D0 update, preserving current
shape/owner/cinematic gates and original candidate/history math. campaign-v8
now completes that diagnostic. No later screenshot or video from this follow-up
has been accepted as a finished opening recording.


The corrected v8 run completes all13 main scenes (215.65 seconds,3666 frames)
with zero script faults and zero Vulkan validation errors. It runs1908 real
projected receiver/geometry queries for the three active Chief-side lights,
with up to eight dynamic-object receivers and17 native light commands per
light.480 distinct tick samples produce positive commands. These are actual
source command outputs; no projected GPU lighting draw has been submitted yet.
The retained observation.json states that boundary explicitly. The recording
acceptance sequence remains open.
