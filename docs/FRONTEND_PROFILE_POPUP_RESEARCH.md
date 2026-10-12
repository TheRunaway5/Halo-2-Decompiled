# Original profile popup backdrop

The profile selector is composited over the completed title image. The
original first applies the shared UI tint, then executes four full-resolution
blur draws, and then submits the profile dialog. A sharp title logo drawn in
the foreground and a generic black dimmer do not reproduce this ordering.

## Authority and capture

The original executable is `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The original mainmenu cache SHA-256 is
`1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b`.
The Xemu Vulkan transition capture is
`analysis/frontend/title-effects-runtime/profile-original_capture_2.rdc`,
SHA-256 `fc9f3883c18398e15e5462167c2401dbceae99b3a8bef4cf7be774eff43412fc`.
All comparisons below are tied to these bytes.

`analysis/frontend/profile-regression/popup-blur-fixture/manifest.json`
records the extracted original inputs, outputs, post-vertex coordinates,
shader constants, executable and GPU proof reports, implementation inputs,
and their hashes. Its scope fields explicitly distinguish the proved blur
passes from the unverified complete native popup. Extraction is offline; it does not
operate the reference emulator or any native application window.

## Tint, fade and submission order

`tools/reference/halo2-decompiled/src/unknown_234c64.cpp`, entry `234F8C`,
sums and clamps the active channel's source fade. It obtains the authored
tint through `148350` and calls `147CDB` before `235756`. The relevant
`wigl` tag is `ui\ui_shared_globals`; its four floats at offset `0x6C` are
ARGB `(0.8500000238, 0, 0.0784313977, 0.1725489944)`.

At fade 1, the original byte-packed color is `0xD900142C`. Capture event
3963 has vertex RGBA `(0, 20/255, 44/255, 217/255)`, source-alpha blending,
and RGB write mask 7. It preserves the lower image's alpha, which is zero
throughout this captured input. The preceding title UI draws also preserve
that alpha. The source alpha is used by the original blur's register
combiner, so the prefix UI and tint need their original channel-write state.

`230374` derives fade from the original animation flags, elapsed UI-clock
milliseconds and duration. `22E65E` selects transition track `b` in reverse
for modal animation type 1. Screen 10's pane refers to global animation 23;
its `a` and `b` durations are both 250 milliseconds. The native painter uses
this authored duration for the current profile-entry transition. Matching
the entire original animation state machine, integer clock and outgoing
transition remains a broader frontend requirement.

`analysis/frontend/profile-regression/oracle-popup-tint.py` executes the
original `234F8C` color arithmetic while intercepting its source queries and
draw submissions. Its comparison against `frontend_popup_tint` checks 1,032
fade/color cases with zero packed-color differences. The report is
`analysis/frontend/profile-regression/popup-tint-oracle.json`.

## Original blur program and coordinates

The executable `235756` multiplies fade by the literal 3.75 at `45DE14`
and calls `211A0` with initial radius 1, growth parameter 1, setup weight
0.25 at `45DC20`, and adaptive parameter 0. `211A0` calls `20F30` to build
the original eight-stage register-combiner program, whose three packed RGB
weights are `0x00404040`.

At fade 1 the submitted radii are exactly `[1, 2, 4, 6]`. Their four texture
lookups use the corner offsets `(-r/2,-r/2)`, `(r/2,-r/2)`, `(-r/2,r/2)` and
`(r/2,r/2)` in the original 640×480 rectangle. The capture confirms these
post-vertex coordinates and the original combiner constants independently:

| Draw event | Radius | Absolute tap displacement per axis |
| --- | ---: | ---: |
| 3977 | 1 | 0.5 pixel |
| 3991 | 2 | 1 pixel |
| 4005 | 4 | 2 pixels |
| 4019 | 6 | 3 pixels |

Fractional fades reduce the last active pass's radius. For example, fade
0.5 submits `[1, 1.75]`, while fade 0.75 submits `[1, 2, 3.25]`.
`analysis/frontend/profile-regression/oracle-popup-blur.py` executes the
original routines and compares all 60 program words, draw count and radii
against `frontend_popup_blur`. Its 1,030 source-input cases have zero
differences; `popup-blur-oracle.json` retains the report and canonical
original submissions.

`tests/frontend_popup_blur_capture_gpu_tests.cpp` executes the production
`bloom.vert` and `bloom.frag` shaders with this original program, using the
captured input at each pass. All four 640×480 outputs match the original:
1,228,800 RGBA pixels, zero changed channels, maximum error zero bytes.
`analysis/frontend/profile-regression/popup-blur-gpu-check.log` contains the
standalone result. This proves these GPU passes on the captured inputs.

## Live channel policy and the virtual keyboard

The live profile state is retained in `original-profile-channels.json`:
title screen 9 occupies channel 5, profile screen 10 occupies channel 3,
and channel 2 is empty. Both screens use the original base `v27` function
at `1473C9`, with `value5F4` zero. The active UI mode is 2.

The actual Create New Profile transition was then performed with the
reference controller. `original-keyboard-channels.json` records keyboard
screen 167 on channel 2 and title screen 9 still on channel 5. Profile
channel 3 is empty, including current, next and previous pointers. The
keyboard also uses `1473C9` with `value5F4` zero. It is incorrect to infer
that a native navigation stack retaining profile 10 means the original
still renders that profile channel behind the keyboard.

`oracle-popup-channel-policy.py` loads these captured channel and screen
bytes and executes the unchanged original `23029A`, `1490EC`, virtual
`v20`/`v21`/`v27`, `230374`, and `234F8C`. It intercepts only the authored
globals query and final draw services. `popup-channel-policy-oracle.json`
records exactly one `0xD900142C` tint and one fade-1 blur submission in
each settled state: channel 3 before profile 10, or channel 2 before
keyboard 167. Title channel 5 submits its widgets without tint or blur.
Thus the measured keyboard path uses the same single backdrop over the
title, without shell framing, a retained profile layer, or a second tint.

The dispatcher `147AF1` renders the shared channels in order default,
5, 4, 3, 2, 1, 0. `238C21` and `238C69` open keyboard 167 through
`149F49` with channel 2 and shared index 4; `23784F` and `147F6D` place it
in the separate channel-2 window. Channel membership is therefore a
source property, rather than the native screen stack's numerical order.
The live reads used GDB without guest memory writes or function calls;
the reference controller performed the transition and GDB detached after
each read.

An attract movie subsequently started while the retained title and keyboard
channels remained occupied (`original-movie-idle-channels.json`). The first
B-button pulse skipped the movie, leaving keyboard 167 in place
(`original-after-movie-B-channels.json`). The correct visible keyboard
was saved in `original-keyboard-after-movie-full-window.png`; this is the
owned 2560×1378 Xemu client image including its black side bars, rather than
an inferred 640×480 crop. A second B pulse canceled the keyboard. The live
`original-keyboard-cancel-channels.json` then has only title 9 and the default
screen 5 occupied, with profile channel 3 and keyboard channel 2 empty.
`original-keyboard-cancel-full-window.png` retains that visible destination.
For this measured Create New Profile path, cancellation returns to title 9;
it does not restore a profile dialog retained in the native navigation stack.
Other keyboard entry paths and completion destinations remain separate.

## Native integration and remaining requirements

`UiDraw::backdrops` marks ordered boundaries between lower UI channels and
the foreground dialog or keyboard. The renderer splits UI batches at each boundary,
restores the authored UI viewport and scissor, applies the original tint
through an RGB-only pipeline, and runs the source blur program before the
remaining dialog geometry. Prefix title UI also uses its captured RGB-only
write state. The lower title world retains its authored post-bloom color
filter. The painter supplies the title prefix instead of shell framing when
profile selection overlays screen 9.

The renderer reserves four dedicated constant/descriptor slots per boundary
and uses its existing full-resolution scene-copy image. The current two-slot
capacity is an implementation bound; the measured profile and keyboard paths
each request one boundary. It does not introduce another shader or
approximate a blur kernel. The lookup coordinates scale with the same
centered 640×480 authored rectangle as the lower UI. The 640×480 pass-level
pixel comparison does not prove every larger drawable size or full-menu
presentation; live border, resize, and aspect-ratio checks remain required.

The production navigation capture bank is
`analysis/frontend/profile-regression/verified/report.json`. It records the
runtime hash and twelve runs: profile and keyboard at 4:3, 16:9, 16:10,
21:9 and after a 1000×600 resize, plus keyboard cancel and profile mouse
hover. All runs use the actual SDL event/navigation and Vulkan drawing path.
The completed bank has no Vulkan validation errors or nonblack pixels outside
the centered authored canvas. The keyboard cancel trace is `10 → 167 → 9`.
These are integration and aspect-layout checks; they are not synchronized
original/native whole-frame comparisons. In particular, a clean validation log
does not establish keyboard selection pixels, wrapping, model appearance or
all transition frames. Each bank's hash identifies the tested build.

Persistent background freezing is **unverified**. The channel render source
`147AF1` invokes lower channels, and `234F8C` computes its tint and blur each
render. A settled reference capture containing only copies and UI draws
does not establish the lifetime or invalidation rule for a persistent title
snapshot. No guessed snapshot lifecycle has been implemented.

Complete native popup parity is **unverified**. The captured-input blur and
packed tint comparisons do not establish the native world's geometry,
lighting, haze history, title bitmap/text placement, animation clock,
entire channel hierarchy, outgoing transition, or a complete synchronized
reference/native screenshot match. The overall frontend parity goal remains
active.
