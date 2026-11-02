# Campaign camera cuts and Earth framing

The reported opening defects are reproduced in the separate
`Halo2-Campaign-Test` runtime with prepared MCC V7 content. Its executable SHA256
is `5beee9be189faeda1deee85d0a9d2d8218c1a59c9d536083c0b625c6ec22e83e`.
Evidence is in `analysis/campaign-camera-20261030/`.

## Cuts within one animation

The opening's source observer changes shots between ticks1400 and1401 while
the HS camera-command counter remains8. The completed source camera jumps
directly. At a faster display rate, the presentation adapter can observe held
subframes and then interpolate between those different shots. Invalidating
history only on new HS commands cannot protect this case.

The ordinary scripted campaign caller now marks its actual completed1720E0
observer output. The presentation adapter passes that output through unchanged
and discards its own history. The original observer retains responsibility for
both cuts and authored transitions. No movement-distance threshold, guessed
cut detector, source clock change or synthetic script command is introduced.
This preserves the original animation sample cadence; it does not qualify
additional high-refresh interpolation of authored camera animations.

`cinematic_camera_interpolation_tests` replays the captured pose words through
the held-subframe/within-command cut pattern. It fails before the change and
passes afterward. Existing fallback interpolation, pause, restore, BSP, command
cut and frame-rate checks remain in that suite.

## Finite Earth matte on wide surfaces

The actual tick5000 frame exposes the Earth matte's right boundary at960x540.
An independent720x540 capture largely conceals that boundary. A read-only
resource diagnostic finds identical plane positions in the original Xbox and
prepared MCC maps. The UV differences are approximately one millionth and do
not account for the exposed geometric edge. This borrowed-archive diagnostic
is resource evidence, not a newly qualified engine build.

The cinematic lens now preserves the completed observer's horizontal
composition on a wider PC surface, reducing the vertical projection angle by
the authored/surface aspect ratio in tangent space. The same resulting lens
feeds source visibility/culling and actual rendering. The canonical HUD safe
rectangle, camera pose, FOV commands, fades, crossfades and gameplay camera
path are unchanged.

The matching overlay uniform producer adapts only physical bar height to the
completed observer's aspect. At16:9 the original4:3 view's active16:9 picture
fills the surface; retaining the old one-eighth bars would crop its top/bottom
again after the lens adjustment. Original progress/opacity remains unchanged,
including during fades, and the standard4:3 uniform words remain unchanged.
The renderer receives this aspect explicitly for campaign cinematics and clears
it for ordinary presentation. A red/green producer regression covers that
within the same lens suite, including the intermediate16:10 extent.

The lens regression fails before this change. Its current checks retain all
three original4:3 contexts, complete192-byte view/projection banks,64-byte
projection programs and raster coefficients. Additional960x540,1920x1080 and
2560x1080 cases check horizontal composition and shared culling/raster framing.
These wider physical extents are a native PC extension, not original Xbox
widescreen framebuffer qualification.

## Validation and scope

The frozen camera-v2 engine passes all three selected CMake tests: camera
interpolation, cinematic lens/letterbox and GPU capture resolution/crossfade.
Its complete3,283-module AOT shader package passes strict executable-bound
validation before and after publication. The retained test candidate is
`.local/campaign-test-20261030-camera-v2/`; its executable SHA256 is
`aa99f7ba494a25e8a5ec850d4f053e5d8026f31179b67899461f31b56d99950b`.

The actual candidate replay uses fresh isolated preferences and real menu
Return keys for New Game/Normal, with subtitles enabled. It naturally reaches
tick5300. All6,533 completed camera frames preserve the source camera words,
including the within-command cut at1400/1401. The equivalent old-runtime
diagnostic had1,211 mismatches among1,889 source-camera frames. The retained
Earth frame passes the reported right-edge regression: its right region's
mean luminance is0.0899529, compared with0.00607044 before the fix. The Earth
and subtitle frames were also visually inspected. See
[`camera-v2-runtime-check.json`](../analysis/campaign-camera-20261030/camera-v2-runtime-check.json).
The capture's owned scratch was removed; three required PNG references,
compressed camera trace and bounded receipts total1,719,007bytes.

Regret's authored HLMT override now reaches the original source-ordered
hologram group. The renderer copies target19 before the group's body and raw
layer21 overlays. Attached bodies consume the native base coverage and
geometry LOD in their existing resident bank. The exact owner, part, pass,
material and range guards stay active. Source strip ranges are retained
separately from the triangle-list counts submitted to Vulkan.

The delivered final-v7 candidate passes thirteen CTests and the actual
Original-mode opening through tick5302 with subtitles enabled and zero Vulkan
errors. All4,427 camera frames preserve the native pose; Earth framing passes.
The actual tick3400 GPU capture visibly shows Regret as a hologram beside the
ordinary Prophet. Its command ledger validates1,095 scene copies and12,045
raw overlay draws. This establishes the local rendered feature and command
handoff, with no original Xbox framebuffer parity claim.

Both `Halo2-Campaign-Test.sh` and the existing bare test launcher select
`.local/campaign-test-20261030-final-v7/`, executable SHA256
`604a99b4ab52af3eff0c87f37a5794154574775ba1ccb170f7ea02858cf5fd92`.
The actual script passes real menu/Normal input through1453, observes this
exact ELF, and passes its camera/overlay gates. Source K117 speech smoothing
is included in the full executable; complete audio parity remains open. See
[`final-v7-delivery.json`](../analysis/campaign-camera-20261030/final-v7-delivery.json).

The additional pStencil-mode replay exposed a pre-existing missing writable
bank at49219C, also reproduced by pre-hologram camera-v2. The source-backed
180-byte cold-owner correction passes fourteen CTests in V8; its actual
pStencil replay is separate and still being validated. Earlier red packages
and diagnostics do not count as accepted runtime results.

The baseline diagnostic reaches the natural Heretic-to-Armory handoff, then
hits the existing missing AI owner at1B0195→1BBB50. That is separate from these
camera defects. The full campaign, co-op and native-audio goal is active and
remains incomplete. These focused camera checks do not establish physical
Xbox framebuffer parity. The launcher-selected normal runtime is preserved;
the separate test launcher is promoted only within the qualified scope above.
