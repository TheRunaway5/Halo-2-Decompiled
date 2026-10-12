# Active frontend parity goal

The scope is the entire Halo 2 frontend outside a running multiplayer match or
campaign level. This includes the title screen, all main-menu screens and
submenus, profiles, campaign selection, multiplayer setup, lobbies and postgame
menus, settings, appearance, variant editors, dialogs, credits, startup movies,
and idle trailers. The entire main menu outside a match or campaign is required;
no submenu is complete until both its original layout and its flow match.
Every screen's layout, selection and focus behavior, navigation, acceptance,
back/cancel behavior, transitions, and return destination must be checked against
the original game's authored assets and executable or verified original behavior.
Spartan and Elite previews must use the original menu poses, placement, scale,
facing, camera, and per-player scene assignment, including the four-player
profile sign-in screen and appearance menus. Bind-pose geometry alone does
not satisfy model-preview parity.
Unimplemented external services remain explicit; a visually correct pane alone
is not evidence of working menu flow.

The required transition corridor also includes pregame → in-game → postgame,
using the user's supplied Halo 2 Vista installation as the authority for this
sequence. Match countdown numbers and beeps, cancellation, intermittent loading
screens and readiness gates, fade-in/fade-out, announcer cues and ordering,
audio continuity/fades, game-over handoff, retained results, postgame report,
and return to the correct lobby must follow that installation. Its supplied
Cartographer DLL and game executable must be identified separately when
establishing behavior. Xbox routines alone do not prove Vista equivalence.
Do not invent fixed delays for loading or attach a lobby directly to a completed
game-over countdown while skipping the original transition owners. Completion
requires a recorded original/native route through an actual match, including
the screen, audio and phase timeline. Existing isolated countdown or sound-class
tests do not complete this corridor.

Whole-frontend layout and navigation parity is a mandatory completion gate for
this active goal. Layout includes positions, sizes, spacing, typography,
highlight and focus presentation, backgrounds, previews, and dialogs. Flow
includes initial selection, directional navigation, accept, back/cancel,
transitions, and the destination and retained state when returning to a parent
screen. Each required screen and route needs original-game evidence and a
native comparison across the supported aspect ratios.

Completion requires a screen-by-screen record of the original and native
layouts and a route-by-route record of selection, accept, back/cancel, dialogs,
and return destinations. This is a completion requirement for the entire main
menu, not only the title screen or a subset of implemented submenus. Cover both
normal entry and return from child screens, including profile creation/editing,
campaign setup, local and System Link
pregame setup, online menu paths, settings, appearance, variant editing, and
the virtual keyboard. Loading a screen definition, passing a menu sweep, or
matching the title screen alone does not complete this goal. Any missing or
unverified path must remain listed until it is implemented and checked against
the original.

The original screen-definition inventory and unverified layout/flow coverage
are tracked in `analysis/frontend/menu-parity-coverage.json`. Screen-definition
scope still needs original dispatch/entry evidence; unused definitions must not
be mistaken for required player-visible routes.

Visual work proceeds in the user's order: fog and haze, bloom, then remaining
lighting, shaders, effects, geometry, and camera differences. Use the original
disc assets, executable/decompiled routines, and measured Xemu Vulkan captures.
Do not guess behavior or tune effects by eye. Whole-frame 1:1 claims require
matched camera, frame, display settings, and original framebuffer comparisons.

Support 4:3, 16:9, 16:10, and ultrawide presentation, including resizing, correct
movie proportions, and centered authored UI. Establish original projection and
layout behavior first and identify wider-than-retail extensions separately.

Preserve unrelated workspace changes, rebuild the launcher runtime, and keep the
goal active until required implementation and validation are complete. Post
screenshots in the chat as meaningful visual changes are made so the user can
review them on their phone, with remaining differences clearly identified.

The current title-rendering checkpoint is recorded in
`analysis/frontend/title-camera-matrix-audit/world-depth-checkpoint.json`.
Original-form shader generation now matches all 27,945 complete geometry
records and all before/after framebuffer banks for four retained original
draws. Raw GPU storage matches the captured vertex bytes and ordered triangles.
The existing resident selector now accepts the explicit title lighting mode
and source cinematic inputs, with original-instruction fixtures and the
21-map regression passing. These are bounded integration prerequisites;
the live title renderer still needs the verified raw pipeline. An independently
implemented production emitter now matches all four retained draws exactly,
including the final captured title image with its three shader replacements.
The replay retains the original renderer for every other operation; it is not
native-menu integration. The authored-program inventory identifies the remaining
unsupported programs, and original device setup now proves initial hardware-fog
parameters with reset/retention limits stated separately. See
`analysis/frontend/title-camera-matrix-audit/WORLD_VERTEX_INDEPENDENT_TITLE.md`,
`TITLE_EMITTER_COVERAGE.md`, and `WORLD_HARDWARE_FOG_STATE.md` in that directory.
The emitter extension now matches four additional class0/class1 draws exactly
(4,083 complete records and all 16 before/after image banks). A combined title
replay using seven replacement resources across 470 original draws also matches
the final frame byte for byte. A build-time catalog compiles seven supported
class1 programs from the actual cache. Its compact descriptor ABI now also
matches all 27,945 retained geometry records in an isolated native Vulkan test,
with four mutation controls and zero validation errors. Fragment/material
binding and live per-draw state remain pending. See `WORLD_VERTEX_EXTENSION_REPLAY.md`
and `WORLD_VERTEX_CATALOG.md` in the checkpoint directory.
The fragment bridge now matches five original fragment resources across eight
retained draws, including complete before/after color, D24 and stencil banks.
Combining all seven verified vertex resources with those five fragment resources
also preserves every final captured title pixel. The native world shader now
uses the original byte-rounded alpha comparison; its rebuilt title launcher
rendered three frames with zero graphics validation errors. The new explicit
fragment contract and source raster context have original-derived CPU fixtures.
Full raw-pipeline binding, exact native constant expansion and live caller state
remain separate gates. See `WORLD_FRAGMENT_ADAPTER.md`,
`WORLD_TITLE_RASTER_CALLERS.md`, and `WORLD_FRAGMENT_DELIVERY.md` in the checkpoint
directory. A long native same-frame diagnostic crashed inside the driver/replay
compiler, and a smaller retry hit the same compiler failure. Both incomplete
runs are retained and are not passed native comparisons.
The whole-menu, live framebuffer, and aspect-ratio completion gates remain open.

The next native checkpoint fixes measured packed-color rounding by uploading
an explicit completed18-vector float bank. All576 original float words now
match on the GPU, with an independently sensitive arithmetic control. A
queue-owned stencil predicate passes2,816 original mappings/copy paths and
its existing raster-context regression. The first complete native raw draw
(event1335) now matches original seeded and final color, D24 and stencil
attachments with all three production stages and exact original sampled-image
fixtures. It affects only a small number of visible pixels; larger draw
families, native cache image import and live title integration remain open.
See `analysis/frontend/title-camera-matrix-audit/WORLD_NATIVE_FRAGMENT_DELIVERY.md`.

The requested native aspect extension now keeps the authored frame center,
buttons, text and model-preview layout centered through 32:9, extends only the
frame's outside edges, and renders the title scenery across the full drawable.
Left/right DDS replacements are independently loaded from
`<maps>/ui/frame_edges/`. Original source bytes remain the default, and the
canonical 4:3 projection and post-process sampling are retained. Six focused
CPU checks, eleven native aspect/resize captures, and the rebuilt F1 Vulkan
probe passed; source/binary/asset identities and pixel comparisons are recorded
under `analysis/frontend/widescreen-debug-20261009/`. The optional debug controls
are native extensions with their scope documented in `OPTIONAL_DEBUG_PANEL.md`.
These results complete this bounded aspect/texture delivery, not the remaining
original-framebuffer or whole-menu layout/navigation completion gates.

The next bounded delivery corrects frontend BC2 storage using the pinned Xemu
decoder, with exact CPU and GPU texel checks. Actual Cache planar-fog gradient
images also pass their independent import check and the complete retained1880
native draw comparison. Setup23 now remains transiently behind Map206 with the
original cadence-dependent exit sampling and stock row-marker focus behavior,
outside Back history. The guarded mainmenu portal initializer passes original
fixtures, but remains separate from live source visibility/admission. Eight
actual launcher captures include title,32:9 appearance, profile, keyboard,+lobby and transition phases. See `WORLD_BC2_DELIVERY.md`,
`WORLD_FOG_1880_CACHE_IMPORT_AUDIT.md`, and the retained
`analysis/frontend/title-camera-matrix-audit/world-bc2-production/` results.
Full live title rendering, appearance emblem composition, complete menu exit
lifecycle and whole-frontend layout/navigation verification remain open.

The 2026-10-09 follow-up fixes the two frame boundary sample columns without
resizing the original center. Seven before/after aspect captures confine all
changed pixels to those boundary footprints, including 4:3/5:4 no-change
controls; see `analysis/frontend/frame-strip-seam-20261009/`.
The actual Settings annotations now use the source standard-text setup and
typewriter lifetime/cadence. Native comparisons pass 527 original prefixes,
92 original setups and all 440 final glyphs / 2640 triangle vertices. Seven
native typing/aspect/resize captures pass with zero validation errors. Screen1
reconstruction, arbitrary iterator measurement and original whole-frame text
parity remain separate gates; see `FRONTEND_SMALL_TEXT_SOURCE_PROOF.md`.

The canonical title-frame producer now passes all 3008 unchanged-original
instruction records. This exposed and corrected the original sky-opacity
complement-product/eye-fraction branch in the live aggregate fog input. Raw
base/fog per-draw pipeline integration remains pending; see
`analysis/frontend/title-bsp-admission-audit/LIVE_INPUT_DELIVERY.md`.

The supplied Vista archive is now exported as a separate 930-frame 2D DDS
library with original mips and independent payload readback. Its opt-in native
pack selects 191 compatible existing frontend banks / 719 frames and leaves
Xbox logical dimensions, fonts, framing, flow and 3D backdrop unchanged.
Twelve focused tests pass, including all150 panes /173 paired geometry/hit
draws, and eight stock/optional native captures have zero validation errors.
Default32:9 Settings is byte-identical to the pre-adapter native capture.
Vista fonts and incompatible frame banks remain unactivated; PC artwork is
not the Xbox visual authority. See `FRONTEND_OPTIONAL_UI_ASSETS.md`.
The complete frontend and original live-frame parity goal remains active.

The 2026-10-11 Vista delivery now passes two actual native offline FFA Slayer
rounds through source countdown, real resource handoff/loading, authored match
expiry, world darkening, retained PC report16, Done and a newly allocated lobby.
Leave/Create and browser Back/PC Settings also pass. The retained generation7
GPU run has eleven screenshots, 262 unchanged input pins and no Vulkan
validation errors. Both desktop launchers selected that generation7 runtime at
the time of its delivery; later deliveries are recorded separately.
This capture is muted and accelerates intermediate CPU match ticks; it does
not complete the required original/native audiovisual comparison.

The actual multiplayer producer now preserves campaign ID NONE and queries
the original frontend protocol rather than mixing cache type2 with a literal
protocol3. The protocol helper matches 1,549 unchanged executable cases.
PC14 layout and privacy/help helpers separately match 787 and 889 original
cases and are compiled/tested in the delivery source. They have not switched
the live lobby away from Xbox14: PC roster, status, map details and external
service ownership must be joined before that switch. The dedicated original
four-voice loading ambience now has a native SDL adapter independent of menu
music, retaining continuous voice cursors and the source gain and shutdown
sequence. Strict/sanitizer and dummy PCM checks pass; the generation12 Vulkan
loading bridge and R7 two-round native capture also pass. This does not prove
DirectSound device/resampling equivalence or the full audiovisual corridor.
Current integration and validation boundaries are recorded in
`NATIVE_VISTA_SESSION_ROUNDTRIP.md`.

The combined successor Release delivery installed at 2026-10-12 00:01 UTC
refreshed both root executables and all six desktop/application shortcuts.
It retains the current online/SDK and renderer changes together with the
independent loading ambience. Eighteen focused checks, the Vulkan loading
bridge and the two-round R9 capture pass; actual prepared launchers additionally
rendered at16:9 and32:9 with unchanged inputs and no validation errors.
Receipts are in `.local/frontend-combined-delivery/20261011T235850Z/`.
Concurrent performance work subsequently retargeted both root launchers to
`build-performance-4k-current-20261011/halo2_native`, SHA-256
`eefa6372c22ea87a1df51ed55ef547dc8de5f03765d3300d1330c34d10ec5d33`.
The retained combined-delivery checks do not validate that later executable;
the precise installation and current-selection boundary is recorded in
`NATIVE_VISTA_SESSION_ROUNDTRIP.md`.
The separate six PC14 query/detail/chat/browser/child-layout/button owner
targets now pass integrated CTest, but their source-backed helper behavior
does not establish a live PC14 screen or the complete Vista transition.
Later root work adds the complete update orchestration, original team sorting,
map commit callbacks, constructor-only state and retained PC metadata.
The expanded eighteen lobby/asset project checks pass without skips. Later
focused grid and clone project checks also pass. Strict and sanitizer evidence
now covers initialization, ordinary registration/rebuild, recursive activation,
generic widget/text/button births, clone descriptors and the raw-bank animation
descriptor producer. These source methods still need their genuine resource,
font, manager, dynamic-child and session bindings before replacing the visible
lobby. The first viable PC14 factory binding is the existing postgame Done and
browser Create route, where the actual source session is already reconciled;
initial entry requires earlier genuine local-session admission.
The goal remains active, including the whole out-of-match frontend and its
navigation, sizing, animation and original/native audiovisual comparisons.

The next 2026-10-12 checkpoint passes 32 integrated checks, including both
actual offline round trips, Settings, transition assets, raw-cache lifetime,
factory/manager orchestration, child queries and animation consumers. The
standalone lower-layout additions also pass project CTest: the grid initializer
matches 3,220 original cases, ordered skin-child layout matches 164, resource
layout matches 413 and direct-child bounds aggregation matches 960. The raw
cursor resource is correctly retained as `mcsr`; it is not a music owner.
These helpers preserve source32 registry identity, callback order and partial
writes. Genuine child constructors, font measurement, class dispatch and session
bindings are still required before activating PC14/23/206 as visible screens.

The rebuilt native executable SHA-256
`89160bf2679cce0847b60e9a1ef976327067ead60d712698feefb742d810b625`
captured Settings at4:3 and32:9, the keyboard and Network popup without Vulkan
validation errors. Captures and exact commands are retained in
`analysis/frontend/vista-session-flow-audit/native-resources-capture-20261012/`.
This executable has not replaced the desktop launcher runtime. These native
presentation checks are not a matched original Windows framebuffer comparison
and do not finish the Vista transition or the full frontend parity goal.

The next lower-layout checkpoint adds the complete header/footer owners
(228 original cases), typed child construction (421 cases, including146 actual
records from all26 skins), bitmap bounds (4,898 hardware-original cases) and
short/float/projected bounds (6,351 hardware-original cases). These additions
pass integrated project CTest. Bounds consume the actual live animated offsets,
scales and viewport globals, preserving source access order and floating-point
state. Hardware executes unchanged original bytes at their original addresses;
retained Unicorn discrepancies are not used to change native arithmetic.
The genuine projection chain through61D162/61D05F/6096A0 is closed for the
admitted resident banks. Class construction and header/footer orchestration
still require their genuine lower text, resource, allocator and manager owners.
Evidence is in the separate20261012 `widget-header-footer-original`,
`widget-child-creation-original`, `widget-bitmap-bounds-original` and
`widget-bounds-original` directories under the session-flow audit. These
isolated owner proofs do not activate the live Vista lobby or establish pixels.

Text assignment now separately matches501 unchanged original cases through
61BAA9/61BAB9/61BA98 and actual short/long/clone styles, including dynamic
class changes, low-AL preparation gating, fresh style lookup, forward overlap,
signed counters and the original32/512/1024-unit capacities. Strict,
ASan/UBSan/leak and integrated CTest pass with unchanged consumed inputs.
The frozen `widget-text-assignment-original-20261012` audit retains complete
banks and callback-time snapshots. Actual font readiness remains the preparation
provider's responsibility; this addition supplies no width or readiness default.

Font conversion, iterator/preparation and the611EA1 measurement caller now
match762 physical-original cases through226,901 instructions. Strict,
ASan/UBSan/leak and integrated CTest pass. The separate frozen
`widget-font-measure-original-20261012` audit preserves28 emulator FP
disagreements; hardware-original arithmetic remains authoritative. The actual
630A68 layout provider, glyph residency and live screen binding remain separate.
Header/footer locale lookup and child disposal separately match1,489 original
cases through93,961 instructions, including all26 prompt selectors, callback
mutation and nested disposal. Strict, sanitizer and integrated CTest pass in
`widget-locale-disposal-original-20261012`. These owners require actual string
lists, live interfaces, style dispatch and deallocation; they supply no defaults
for missing providers and do not establish visible Vista lobby parity.

The subsequent retained UI checkpoint closes text measurement (154 original
cases), private glyph/copy support (1,243), bitmap update/residency orchestration
(2,130, including310 actual authored records), manager lifetime (443), screen
disposal (260), and UI pool allocation/release/retirement (914). Each has separate
unchanged-executable evidence, strict and sanitizer validation. Project CTest
also passes for these owners. The writable native cache retains the actual
5,505,024-byte tag region; its original bitmap corpus passes through that same
storage. It is not a readiness or GPU-residency substitute.

`FrontendSession` now retains the actual manager and cache storage across world
handoffs. Once the manager bank exists, its `D71900` cell is the UI clock's sole
authority; native updates preserve source producer writes and DWORD wrap. The
original clock's complete floating-point/status and scheduling behavior remains
separate from this native steady-clock adapter. Screen disposal includes nested
original disposal and fresh class/manager lookups; the UI pool retains genuine
backing allocation, deallocation, disposal and time services as required bindings.

The visible lobby remains Xbox14. These additions do not activate PC14/23/206,
finish the Vista transition, prove original/native pixels, or refresh a desktop
runtime. Two attempted current-checkout native builds detected concurrent source
changes and are retained as failed stability checks in
`native-retained-ui-build-20261012/`. A successful compile alone does not qualify
either executable as a current validated delivery. The next integration boundary
is the source32 registry, genuine aligned backing, font/glyph ownership and class
dispatch into the actual reconciled session; full audiovisual comparison remains
required.
