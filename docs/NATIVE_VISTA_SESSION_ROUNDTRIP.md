# Native Vista offline session round trip

Current integration boundary (2026-10-12): the actual native offline FFA Slayer
round trip described below is operational, but its live lobby is still Xbox14.
The full Vista transition remains unfinished. New isolated PC owners and retained
memory do not by themselves change the visible screen or establish audiovisual
equivalence.

The application now connects the recovered PC lifecycle to an actual native
world for one signed-in offline FFA Slayer participant. Start runs the source
countdown, initializes the selected world once, retains completed statistics
before destroying it, loads mainmenu resources, creates PC report16, and uses
Done to allocate a fresh lobby. A second Start retains the selected profile,
map and variant. This supersedes the earlier ownership-only application's
return boundary in `analysis/frontend/vista-session-flow-audit/NATIVE_SESSION_OWNERSHIP.md`.

`NativeVistaOfflineFlow` owns the PC session, controller/user/peer/XUID join,
lifecycle and source services. `MultiplayerPreparedWorld` owns real Cache,
content, rules, match and local players. The synchronous load adapter finishes
that construction before publishing its1188-byte current-options owner; the
runtime reuses this same match instead of joining a second time. The source
session seed reaches the actual native simulation random-state initializer.
Imported PC132-byte final profile and native sanitized144-byte player
configuration are distinct owners; only their actual requested payloads are
compared across that conversion.

The source countdown emits widget sound slot10 at stages3,2,1,0. Source
lifecycle3/4 consumes the real native game-over completion; a renderer frame
limit cannot manufacture a completed report. `NativeVistaPcr` consumes the
actual frozen statistics, genuine session membership and imported PC locale
and appearance assets. Its retained data outlives world destruction. The new
report's creation clock is sampled after the real mainmenu resource load.
The original Done observer requests a new lobby instance, rather than resuming
the old stack. Callback adapters are detached before invocation resources die.

The original lobby input owner now handles quick-advance and cancels before
Game Setup or Back. Cancellation clears the pending native launch only after
the actual source session clears its countdown. The full source countdown
wrapper retains the resolved local user's XUID. A process-owned UI clock
continues across world/resource gaps and is sampled before report factories,
preventing retained animation timestamps from wrapping on return.

Gameplay darkening now consumes the actual native engine/player/round fields
at the proved pre-world update boundary. Its source opacity and packed color
reach an actual viewport quad before the scoreboard. Geometry is synchronously
resident before this draw; native cubemap capture is inactive. The original
four-second hold and one-second darkening execute at the configured native
30Hz. The independent source fixtures also verify the native player-life
predicate conversion, screenshot/cubemap guard, and geometry preload counter.

The announcer's game-over event now follows the recovered pre-world queue and
post-world one-shot publication. The actual selected PC scenario supplies the
response records, and a separate PC shared-cache owner supplies their original
sound definitions. This avoids resolving shared announcer indices through the
scenario's independent custom sound bank. Native voice tokens retain the real
SDL completion identity; actual class/master gains, successful completion and
flush follow their owned playback path. Muting creates no audio device. The
initial player-activation event now consumes actual local player birth and
activation before the ordinary player update. The real offline session builds
raw mode3 and simulation kind2. The frontend protocol now comes from the actual
615EA9 query and its distinct current-session/parameter-session gates. Original
617138(2) selects multiplayer with campaign ID NONE; the former native producer
incorrectly put cache type2 in that field while hardcoding protocol3 elsewhere.
The source-derived query replaces those three literals. Mainmenu similarly
retains its constructor's campaign/map IDs NONE: world-kind3 belongs to the
separate options+0 field. The unchanged-executable protocol corpus contains
1,549 cases, and its actual mainmenu prefix is also executed. This is documented
in `pc14-layout-ownership-original/README.md`.
Original mode1/3/5 constructors, watcher branches, world loader and activation
are independently compared, including actual native PCM completion.

Leaving a local lobby completes its actual session disposal and progress task,
then returns directly to a fresh native main menu6. The completed session owner
is detached immediately. Later title, profile, Settings and Split Screen
navigation cannot run a disposed session's observer. A new Start constructs a
fresh native session owner with the currently selected profiles and controls.

Main-menu Back confirms sign-out and creates a fresh title9, including when
there is no retained startup title in the navigation stack. Screen13 is retired
from native session navigation; both source namespaces reject attempts to open it.
ONLINE's Server Browser uses PC13 as a presentation layout for the authenticated
native directory219 route. Its listings, details, refresh and Back stay owned by
that directory route; the original session browser lifecycle remains detached.
The native menu retains its startup ONLINE, Split Screen, System Link and
Settings actions. Original platform-only source records remain isolated
reference data rather than native return destinations.

## Validation

`frontend_online_tests` verifies the ONLINE-only PC13 presentation with
advertised test listings, source columns, full-row mouse targets, loading,
scrolling, maximum-length names, expiry, empty/error/retry states and Back.
The visible Vulkan run captures six browser states at nine aspect ratios;
its imported font/bitmap audit and validation layer pass. This is native UI
renderer evidence with fixture authentication/listings, not live online joining.
The local disposed-session/title/profile/Split Screen regression also passes
with the presentation mapping enabled.

`native_vista_flow_tests` runs two complete native CPU matches. Each uses the
authored eight-minute default Slayer limit, ordinary30Hz world stepping,
seven-second game-over completion and the actual wall-clock pregame countdown.
It checks exactly four countdown sound requests, one initialized match,
retained final statistics, actual resource destruction, a new report factory,
Done, a new lobby and an unchanged second Start. Both rounds pass. CPU world
steps run faster than wall time; this is not an eight-minute audiovisual test.
Post-report return coverage also exercises Escape/B, the mouse Done prompt and
source component13 across the solo and two-player runs. A stale report click is
rejected,120 subsequent frontend updates retain the same ready lobby, and Start
begins another real countdown with the same map, variant, profiles and input
bindings. Cancelling that countdown stays in the lobby.
The visible Vulkan/SDL regression also retains each mouse click sequence's
initial screen instance, preventing later presses from activating a different
screen. It sends Escape and actual SDL mouse presses, retains at least120
rendered lobby frames, and uses SDL Return to begin the next countdown.
It captures `native-lobby-after-done-{1,2}.ppm`,
`native-next-countdown-{1,2}.ppm` and `native-lobby-after-title-{1,2}.ppm`.
Set `H2_TEST_VISIBLE=1` to show the windows.

The second completed match now follows the full report, Leave, main-menu Back,
title, selected-profile and Split Screen route. It must load and run the next
actual world, beyond merely starting and cancelling its countdown. Both solo
and two-player runs verify current controller bindings. The final return checks
Settings/Back and repeats title reentry. `H2_TEST_NAV_ONLY=1` runs the minimal
disposal/title/profile/reentry check without playing the complete matches.

The initial clean delivery build passed 16 focused CTest checks for the integrated round
trip, frontend/session ownership, PC settings and transition assets, loading
geometry/music/thread/assets, retained music PCM and main-menu services. The
separate original loading-ambience test also passes. The earlier 17-check
lifecycle/birth/announcer run is historical evidence, not a replacement for
the current delivery results.
Independent PE fixtures and sanitizer scope are documented with each module in
`analysis/frontend/vista-session-flow-audit/`.

The generation 12 transition validation uses the pinned source tree
`analysis/frontend/vista-session-flow-audit/delivery-source-20261011-1720`.
Its initial 609-step clean build replaced an older incremental build whose
damaged Ninja dependency database left incompatible material objects behind.
Actual native capture exposed that failure; it was not a successful runtime
check. Snapshot amendments record the original absent-unit/absent-hand guards:
first-person definitions may be installed before deferred player birth, but a
NONE hand cannot resolve an inventory weapon. Both actual rounds exercise the
genuine first tick that activates and binds the unit.

The following example uses the recorded delivery test binary. Check its hash
against the intended source/capture pins before reproducing that run. Query the
launcher for its actual gameplay update and pass that XBE explicitly; the
base-disc `assets/disc/default.xbe` lacks required scoreboard reference data.

```sh
./Halo2-Launcher --launcher-info
./build-frontend-delivery/native_vista_flow_tests assets/disc/maps \
  'Halo 2 Vista installer/maps/mainmenu.map' \
  '/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe'
```

An optional fourth argument captures the actual completed native reports and
native main-menu/title returns through `run_frontend`, including real Cache/GPU
initialization and Vulkan validation. It waits for the real network clock to
pass the source report intro; it supplies neither fixture results nor a
substituted clock. The reproducible wrapper also captures bounded actual
world-start and postgame-darkening frames and pins its executable, sources,
maps, fonts and shaders:

```sh
TMPDIR="$PWD/build-frontend/compiler-temp" \
  python3 tools/capture_native_vista_roundtrip.py \
  build-frontend-delivery/native_vista_flow_tests \
  analysis/frontend/vista-session-flow-audit/live-native-loading-new-capture \
  --source-root analysis/frontend/vista-session-flow-audit/delivery-source-20261011-1720 \
  --gameplay-xbe '/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe' \
  --timeout 300
```

The historical generation 7 retained run at
`analysis/frontend/vista-session-flow-audit/live-native-loading-delivery-20261011-r5/report.json`
passes with eleven 1280x720 screenshots, 262 unchanged input pins,
both completed-round markers, actual Leave/Create and browser Back/Settings
markers, and zero Vulkan validation errors. Audio is muted and CPU match ticks
are accelerated; these captures do not establish full audiovisual equivalence.

The combined Release delivery installed on 2026-10-12 at 00:01 UTC is retained
at `.local/frontend-combined-delivery/20261011T235850Z/`. At installation, both
root launchers and the Desktop/application shortcuts were refreshed, and
both actual `--launcher-info` queries selected its `halo2_native`, SHA-256
`39d732f7060e6f332704b22e51f376a2e79e1f333fe39ae7e2003057a625d14e`,
with the current Xbox, Vista GUI/loading and DLC XBE banks. `ldd` resolves the
official Social SDK from the same runtime directory, with no missing libraries.
The preparation and installation receipts are retained in that directory;
the installation receipt checks all eight launcher/shortcut targets and
preserves the previous runtime and the other task's delivery receipt.

Subsequent concurrent performance work refreshed the root launchers again.
A later direct `--launcher-info` inspection on 2026-10-12 found
`build-performance-4k-current-20261011/halo2_native`, SHA-256
`eefa6372c22ea87a1df51ed55ef547dc8de5f03765d3300d1330c34d10ec5d33`,
for both launchers. That Release CMake cache uses the live root source tree and
has an empty `HALO2_DISCORD_SOCIAL_SDK_ROOT`. The combined delivery's checks
below apply to its retained executable and source snapshot; they do not
validate this later performance executable. This work does not overwrite
the concurrent launchers to restore the older runtime.

This delivery was built from the immutable
`delivery-source-20261011-successor` tree: 2,750 files, manifest SHA-256
`7b74b3990e59f9b4401ef056e821bb536da15e96d9e31953a73b370f7db41c9e`.
Its eighteen focused CTest checks pass without skips, including the online
hover/prompt, native material selection and BSP frame-plan reuse checks.
Its Vulkan loading bridge report is
`loading-bridge-native-audio-successor-20261011/report.json`; all four captures
pass with unchanged frozen source/consumed inputs and zero validation errors.
The two-round R9 report is
`live-native-loading-successor-20261011-r9/report.json`: both native rounds,
Leave/Create and browser Back/Settings pass, with eleven captures and 269
unchanged input pins. The same muted/accelerated validation limits apply.

Before installation, both actual prepared launchers ran separately at 16:9
and 32:9 with current assets and Vulkan validation. The report and four
PPM/PNG captures are in `prepared-launcher-successor-20261011/`; both exits
are zero, consumed inputs are unchanged, and no validation errors are present.
These fixed-clock screenshots verify launch and presentation, not original
framebuffer equivalence. The native executable consumes the pinned compiled
`build-frontend-successor-delivery/shaders` bank. Its packaged shader copy is
also pinned, but does not replace that compiled lookup path.

The installer preserves the original 67-entry delivery guard. An explicit,
receipt-pinned review in `historical-diagnostic-guard-review-20261011/`
classifies only the old performance build's CMake cache and standalone CPU
profiler as unconsumed historical diagnostics; all 65 remaining guards remain
strict. Classified paths are rejected if consumed by reports, tests,
executables, shaders, assets, launchers or compiler inputs. Negative checks
also retain the default strict refusal when no review is provided.

The generation 7 launcher receipt is historical:
`analysis/frontend/vista-session-flow-audit/launcher-delivery-20261011.json`.
At that delivery, both installed launchers selected the tested
`build-frontend-delivery/halo2_native`. A subsequent concurrent delivery changed
their selection. Earlier `--launcher-info` queries selected
`.local/online-presence-delivery/20261011T224537Z/halo2_native`, and reported the
Xbox game files, Vista GUI/loading asset paths and actual DLC XBE. That selection is a
separate artifact from the generation 12 test binaries described here. A new
combined build and launcher refresh are established by the separate successor
evidence and receipts above, not by the generation 7 reports.

The subsequent combined Release build uses the separate pinned tree
`delivery-source-20261011-combined`, including the current online/title/renderer
changes and official Social SDK binding. Its 16 focused checks pass without
skips, including online identity, matchmaking, clans, parties, a two-process
native match, the source lobby owners/assets, mouse prompts and both loading
audio checks. The XML/log are `build-frontend/vista-combined-tests.xml` and
`vista-combined-tests.log`. Its pinned Vulkan loading test is recorded in
`loading-bridge-native-audio-combined-20261011-r2/report.json`: four captures,
all 2,680 frozen source files checked before/after, unchanged consumed inputs
and no validation errors. The R8 report at
`live-native-loading-combined-20261011-r8/report.json` also passes both native
rounds, Leave/Create and browser Back/Settings, with eleven captures and 269
unchanged input pins. These checks retain the same muted/accelerated roundtrip
and SDL dummy output limits described below. Packaging and installed-launcher
selection require their own final receipt.

The historical nine focused generation 7 checks passed the two-round CPU flow,
real Vista transition-asset import, mouse prompts, Settings, general frontend,
PC presentation/PCR painter and the independent PC14 layout/privacy helpers.
The special-button importer retains the signed source hit-top offset and full
flag word; the native PC hit rectangle consumes that offset without moving
drawn geometry. Its actual PC14 record regression and controlled import test
both pass. The 787-case layout and 889-case privacy/help helpers are retained
for the forthcoming PC14 binding. The isolated status/caption/session-bank
helper additionally matches 4,587 original instruction cases; it does not yet
replace the live Xbox lobby status. The final-player roster publisher matches
249 original zero/solo cases, and the protected chat predicate matches76 cases;
both also pass strict compilation and ASAN/UBSAN/leak checks. Their integrated
CTest targets pass independently. Source-qualified PC14/23 assets are now
imported without changing the visible lobby route; the existing Settings and
transition-asset checks still pass. Those helper validations do not establish
live PC14 integration. The exact historical generation 7 captured manifest is
retained as `source-pins-generation7-final.json` in the source snapshot; later
snapshot amendments have their own generation and validation scope.

The current root source additionally integrates six complete isolated owners:
queries (1,701 original cases), generic details/map resources (1,532), chat
queue (343), browser gate (4,080), custom child layout (468), and button
dispatch (1,656). All six CMake/CTest targets pass in
`build-vista-lobby-owners-20261011`; the log/XML are retained under
`build-frontend/vista-lobby-owners-tests.*`. Each also has strict and sanitizer
evidence. These newer helpers are separate from the immutable installed
successor source, and still require the genuine live PC14 owner and services.

The exact source-local metadata audit in `pc14-import-inputs-original/`
executes 392 original cases against the full actual PC tag bank. It retains
all20 campaign and50 multiplayer records, including invalid slots, all70
preview dependencies, ordered goof/sily categories and owner-local locales.
Both actual Xbox and PC metadata identify Lockout as map50;17 in older
controlled fixtures was synthetic. A probe linked to the frozen successor
archives confirms that the real native profile/join/lobby route selects50
and `scenarios\\multi\\lockout\\lockout`, with unchanged inputs. No production
map-ID conversion is required or introduced.

The current root source now also retains the actual PC metadata bank through
`VistaLobbyMetadata`, without enabling a new visible route. Native strict and
sanitizer runs pass6,214 checks; the independent sidecar export passes7,406,
including all33,517,440 preview payload bytes. Original43DEFD locale execution
passes4,542 owner/SID cases and confirms that a miss returns the empty string
without searching another locale owner. The new project metadata/update tests
pass, as do the existing Vista Settings, transition-assets and lobby-assets
regressions. Evidence and immutable input pins are in
`pc14-metadata-import-validation-20261011/`.

The complete64564B update orchestration matches393 original cases, including
pending publication, exact callback order, retained map/identifier changes and
the full shared display tuple. The original642FCC comparator and
68B128/68B0AE/68B087 sort additionally match1,509 original cases and exact
configuration-read traces, including equal-team row order and signed team
bytes. The source comparator reads both configurations after both owner
lookups; it does not snapshot the first team's byte prematurely. These
helpers have strict and ASan/UBSan/leak evidence and separate project tests.
Their session and widget services remain external, and the update has not
been wired to a fabricated native session or the existing Xbox lobby view.

The complete6463AB constructor and its reached constructor helpers separately
match36 original cases,324 full-bank checkpoints before cached-clock reads,
all1,854 written bytes and all11,862 untouched bytes. Clock callbacks read the
retained cached UI clock independently; they must not advance it. Source
32-bit callback/vtable/bank identities remain data and are never converted to
callable native pointers. Strict and sanitizer evidence is frozen in
`pc14-constructor-native-original/`; constructor output alone does not establish
the initialized, activated native PC14 factory.

The PC206 standard/custom map commit callbacks and label update match804
original cases and30,303 original instructions, including service-time row
and edit-field mutations. Native strict/sanitizer and project tests pass.
Original session publication, selector factory/row lifetime and CRT error
paths remain explicit boundaries. Evidence is in
`pc206-map-commit-original/`; these callbacks do not enable a partial visible
map selector. All these later source-owner changes are separate from the
immutable installed successor delivery described above.

Later bounded lifetime helpers now retain the original initialization and
widget graph orchestration. The complete643FF9 initializer and reached
map/chat/callback/identifier owners match153 original cases plus6 manager
births (100,178 instructions), with strict and ASan/UBSan/leak checks and stable
dependency pins in `pc14-initializer-original/`. This includes separate factory
and host samples, descriptor-bit rereads after focus, root ID counter wrap,
callback-time sibling changes and font/style/animation reads in source order.
The actual first selected-map output controls map admission. Font/cache
preparation, typed child dispatch and clone consumption require actual bindings
to their source owners, not assumed successes.

The ordinary610A44/6107DF registration,60FB1B child creation orchestration,
61208E/611891 links and610227/60FA8B rebuild match89 original cases,
57,283 instructions and2,082 checks. Strict and sanitizer evidence and the
frozen manifest are in `pc14-widget-registration-original/`. Separate genuine
construction evidence proves that PC206 runtime text ordinals4/5 are generated
by610B4E after its authored text ordinals2/3; they are not missing authored tag
records. Forced/multi-list registration remains unsupported. Bitmap/grid/table
construction, generated footer measurement and residency are still real lower
owners required by the live frontend.

The grid virtual620441 is now a separate complete owner: it invokes the
recursive611E23 identifier method before clearing grid+6E. Its144 original
mixed-tree cases execute13,152 instructions and pass1,299 strict/sanitizer
checks, including nested grids, root-counter wrap and callback-time visibility
and sibling changes. The project grid test passes. Evidence is frozen in
`pc14-grid-identifiers-original/`; full grid construction remains separate.

The complete generic611D81,61BA07,62DEFA,60F65D and63F756 births match320
original cases and35,904 instructions, including full-bank checkpoints before
the cached clock and callback mutations. Strict and ASan/UBSan/leak runs each
pass1,290 checks with settled, unchanged inputs in
`generic-widget-birth-original-20261012/native-validation-r2/`. The first
validation attempt correctly rejected its changing evidence file even though
both binaries passed; it is retained as a failed input-stability result.
These lower births preserve padding/string buffers and do not allocate a
screen, prepare fonts, activate the manager or render a frame.

The complete61BC81/61BB0B clone/text descriptor path and its reached root,
locale-resolution, bounds and style orchestration match370 original cases and
73,320 instructions. Strict/sanitizer and the project clone test pass. Explicit
caller scratch retains descriptor padding and untouched text bytes; callback
mutations, aliasing, signed font/animation words and repeated style queries
preserve their original order. Evidence is in
`widget-text-clone-original-20261012/`. The embedded clone's actual string owner,
font cache and locale/context providers still need real binding; a fixture
string copy or ignored font request does not establish that binding.

The complete612604 recursive activation and6115FE animation-state refresh
match103 original cases,249 cached clock reads and297 descriptor requests.
Strict/sanitizer evidence is in `widget-activation-original-20261012/`. Kind2
dispatches virtual18 without ordinary recursion; other widgets refresh their
own retained clocks after descriptor production. Partial or absent descriptor
writes retain explicit caller scratch bytes, and the ignored AL result does
not become a fabricated successful animation. Kind2 virtual behavior remains
an explicit service. Native typed
links must project the real retained graph and change when callbacks change
the graph; updating only raw source32 link words is insufficient.

The separate complete612250 descriptor producer now reads the raw addressed
PC tag, skin, manager and widget banks through explicit registry services.
It matches1,405 completed original cases plus one preserved original memory
fault boundary (300,808 instructions), passing7,029 strict/sanitizer checks.
Actual59-record selection, forward/backward direction, focus slots, source
pointer/NONE behavior and borrowed-buffer aliases are retained. Evidence is
frozen in `pc14-animation-descriptor-original/`. The semantic UiAnimation
projection omits source addresses and raw frame/header fields, so it cannot
stand in for these banks. The later raw-cache and keyframe owners below retain
those fields; their live per-class/manager services still require composition
before visible animation parity.

PC206 factory/selector-constructor orchestration, focus/selected-handle,
direct text lookup and mouse/tab/input behavior match840 original cases under
strict and sanitizer checks in `pc206-selector-lifetime-original/`. Signed
WORD ordinals, transformed hit bounds and source DWORD tab arithmetic are
retained. Standard/custom list constructors, generic creation and manager
registration remain separate services. These additions do not enable a live
PC206 selector or establish the complete Vista transition.

The imported PC content now retains an immutable `VistaWidgetCacheBank`: the
exact2,048-byte file header and5,505,024-byte index/resource region, including
all1,504 tag records and621 complete raw animation keys. Source32 placement is
explicit and independent of host pointers. The cache survives destruction or
overwrite of the original input, and49,468 original tag/block leaf calls match
the native registry. Strict/sanitizer evidence is frozen in
`pc14-widget-cache-original/`. Its five loaded-state global projections are
declared inputs; the original loader/bootstrap is not implemented by this bank.

Complete612758 raw-key animation and generic612CD8 clock/child traversal now
match12,064 finite original calls, including4,448 actual Vista keyframe cases.
Strict/sanitizer runs compare60,450 output/write-order/callback/MXCSR-control
checks. The129 original count-one IDIV faults and one repeated-zero-step
nontermination are rejected explicitly. SSE sticky status is not modeled by
the original-instruction emulator and is not claimed equivalent. Actual child
class overrides, activation and dismissal remain required services. Evidence
is frozen in `pc14-animation-consumer-original/`.

The complete PC14 factory/manager orchestration646752/6467E9,60B8C3,637787 and
637670 matches340 original cases, including allocation failures and reentrant
pending-owner changes. It preserves constructor publication, registration,
prepare/focus/initialize/child-layout/footer/activation order and the actual
manager bank selection. Widget lookup/layout/focus leaves separately match501
original cases, including four actual PC14 registration graphs. PC14's actual
duration virtual returns zero; its prepare and notification leaves are literal
no-ops from its own vtable. Strict/sanitizer evidence is frozen in
`pc14-factory-manager-original-20261012/` and
`pc14-widget-queries-original-20261012/`. These helpers do not manufacture the
resource, platform, focus-manager or session services needed by a live screen.

The actual clone string owner643DDB/44BD27/687AB5 and getter643E66 preserve
UTF16 truncation, terminators, forward-overlap behavior, padding and counter
clears across374 original cases and4,887,021 instructions. Font preparation
remains separate. Complete61C3FB bitmap and62044F/642405 grid births match334
original cases, including155 actual authored definitions, callback-time source
reloads, UCOMISS/DAZ scale rules, first-frame bounds and four original fault
prefixes. Strict/sanitizer evidence is frozen in
`widget-clone-string-original-20261012/` and
`widget-asset-birth-original-20261012/`.

The independent PC17/206 asset inventory now matches both imported selector
definitions, two lists, one skin, nine bitmap banks and fifteen frames, plus
their exact global layout/animation records and image/mip payloads. The
original PC14/23 fixture remains unchanged. Evidence is frozen in
`pc206-pc17-ui-assets-original-20261012/`; importing these assets does not select
a visible route or alter the3D backdrop. The expanded32 project checks pass
without skips, including both native offline round trips, Settings, transition
assets and all integrated widget owners. The first broader run caught stale
shared-header callers during concurrent source changes and outdated selector
inventory assertions. The subsequent stable-header rebuild and corrected
inventory pass are retained in
`build-frontend/vista-widget-resources-expanded-tests-r2.log`.

The generation 12 GPU bridge report at
`analysis/frontend/vista-session-flow-audit/loading-bridge-native-audio-20261011-r3/report.json`
passes with four captures and zero Vulkan validation errors. It combines actual
renderer handoffs, guarded departed CPU storage, resize and source graphics
fade/join with the production loading-audio adapter and real SDL dummy
callbacks. The test waits for at least 1,024 mixed output frames during loading,
then verifies final attenuation -5000 and joined audio worker/device shutdown.
Controlled partial admission, cancellation and worker-failure cleanup also
pass. This proves the combined native GPU/dummy PCM lifetime, not physical
DirectSound or speaker output.

The subsequent generation 12 R7 run at
`analysis/frontend/vista-session-flow-audit/live-native-loading-delivery-20261011-r7/report.json`
passes both actual native rounds, Leave/Create, browser Back and Settings return.
It retains eleven 1280x720 screenshots and 269 unchanged input pins, with no
image errors or Vulkan validation errors. The report pins its test binary,
generation 12 source manifest, original loading-tone bank and actual DLC XBE.
Audio is muted, and intervening CPU match ticks are accelerated. R7 exercises
the muted audio admission/lifetime with actual resource handoffs; it is not a
full GPU playthrough or an audible transition comparison.

The production `Renderer::draw_loading` path also matches all RGB pixels in
16 complete original D3D9 frames and 22 cumulative draw prefixes: 135,091,200
compared pixels, zero differences, zero validation messages, unchanged inputs.
The reports are in `native-loading-presentation-original/native-frames-20261011-delivery`
and `native-prefixes-20261011-delivery`. They use supplied clock/progress values
over a black input framebuffer; this is a bounded rendering comparison, not a
complete transition or physical Windows comparison. A separate GPU bridge run
checks inaccessible departed CPU storage, resize, both handoffs, cancellation
and worker exceptions. Retained music owner/importer/mixer/fade units also pass
ASAN/UBSAN/leak checks; linked archive internals are not instrumented.

## Remaining parity boundaries

The following lower-layout checkpoint adds four independently frozen owner
proofs and passes their integrated project CTests. Complete620493 grid setup
matches3,220 original cases; complete642910 ordered skin-child creation/layout
matches164; complete60F790 resource layout and its small raw-cache/cursor leaves
match413; complete6425BF direct-child bounds aggregation and its actual61C344/
640F86 skip flags match960. Their evidence directories are
`widget-grid-initialize-original-20261012`,
`widget-skin-children-original-20261012`,
`widget-resource-layout-original-20261012` and
`widget-child-bounds-original-20261012` under the session-flow audit. Each
retains exact raw reads, callback-time banks and partial failure boundaries,
with strict and ASan/UBSan/leak validation. Actual child virtual20 bounds,
typed allocation/constructors, renderer residency requests, cursor application
and font measurement remain genuine lower owners, rather than guessed success
or authored-bounds substitutions.

The native rebuild now renders fresh4:3/32:9 Settings, keyboard and Network
popup captures without Vulkan validation errors. Exact commands and binary
SHA-256 are in `native-resources-capture-20261012/runs.json`; the new executable
is `build-vista-lobby-owners-20261011/halo2_native`, not an installed launcher
update. This presentation check does not prove original/native pixel equality.

This does not finish the complete Vista transition. Native world construction
remains synchronous on the resource thread. A separately owned presentation
worker now starts immediately before the actual prepared-world construction,
retains only the departing GPU resources, transfers presentation to the prepared
renderer, and runs the recovered fade/join before ordinary world/report frames.
The reverse resource boundary starts at the actual completed-world close.
The actual loading-thread initializer/caller and its33ms pacing are pinned in
`analysis/frontend/vista-session-flow-audit/native-loading-thread-original/`.
The ordinary loading thread's x87/SSE context is measured under the retained
WineGE run. The actual Microsoft D3DX9_31 decoder also produced all12 loading
textures under both source memory-pool choices; each pair agrees byte-for-byte.
Exports preserve the actual allocation dimensions separately from source image
dimensions. The dedicated loading draw path skips ordinary world, borrowed CPU
BSP, effects and ImGui work. Its swapchain transfer, resize, cancellation and
worker exception paths have a separate bounded GPU fixture. Ownership and source
scope are documented in
`analysis/frontend/vista-session-flow-audit/native-loading-presentation-original/native-bridge.md`.

The selected PC wigl now supplies its actual E53D031B music bank and5500ms
silence value. Decoded phases, voice IDs, RNG bank and transition state belong
to the retained frontend session, rather than one renderer. Actual world
presentation updates continue deferred stop requests. The complete PC intro
and all four loop payloads are separately checked after Cache destruction.
This proves native resource/PCM continuation; the actual Vista selector, global
RNG ordering, physical audio driver and movie-driven621B68 permit publisher
are not established by that test.

The final 782052 consumer is the dedicated four-buffer loading ambience owner
at E9E2DC..E9E30F, independent of menu music. Its recovered decoder
matches all 65,408 PCM bytes; 1,017 gain inputs, 1,064 final integer volume/cache
cases, 13 setup cases, 4 worker schedules and 2 shutdown traces match unchanged
original instructions. `NativeVistaLoadingAudio` now binds that source helper
to four continuous SDL streams at the original 21,738 / 16,290 / 29,008 / 10,876
Hz playback rates. The output boundary is explicitly native: 48,000 Hz float
stereo, SDL rate/channel conversion, per-voice decibel amplitudes and clipped
mixing. No menu/global master-volume fade replaces the ambience owner.

`src/main.cpp` retains one shared adapter through `DesktopLoadingAudioServices`
begin/gain/end callbacks across renderer handoffs. The source setup precedes
graphics-thread creation; every recovered gain is forwarded to the ambience
owner; graphics join precedes audio signal/join/stop/release. Muting is an
explicit native admission policy, and partial admission, cancellation and
presentation failure still close that native interval exactly once.

Strict and ASan/UBSan/leak adapter builds pass four original worker fixtures,
4,068 forwarded gain inputs and 336,137 output frames across repeated periods
and different render partitions. Real SDL dummy callbacks consume PCM before
shutdown. Gain updates retain queued source bytes, and device-open failures,
partial setup, clock exceptions, concurrent gain/shutdown and active-owner
destruction are checked. Evidence and source pins are in
`music-binding-20261011/loading-gain-consumer/native-adapter-20261011/`.
The unresolved original setting404544(2)/86D828 producer, physical DirectSound
interpolation/mixing, real audio hardware and full audiovisual timing parity
remain separate boundaries.

Actual map-progress publication, captions/network event dispatch and the PC
emblem shader remain unfinished joins. E4AFE8's fresh zero is present in the original PE data and was observed
in316 actual mode-zero draw calls; its indirect publisher and complete loading
lifetime remain unresolved. Neither native percent completion nor PC209's
distinct progress owner is substituted for it.
Native gameplay runs30Hz; the supplied Vista tick-rate leaf returns60. The
selected world-options service explicitly records this difference.

The live binding admits one offline FFA Slayer player. Team, multiple local
players, LAN/online, campaign, pause/leave-game UI and their report flows retain
their existing paths. The lobby still uses the original Xbox14 layout with
source countdown presentation; `MenuState::open_vista_lobby` still builds Xbox
screen14. Imported PC14/23 assets and independently verified layout, help,
status, roster, chat, spinner and Setup owners do not establish their live
binding or exact rendered layout/flow. The Xbox 3D backdrop remains retained.
Retained PCR consumed fields are bound, but full PC469DFD header/machine/event
publication is not claimed. No original/native framebuffer or full audiovisual
equivalence has been established by these checks.

The20261012 widget-layout extension closes60FD41/60FF73/610B4E header/footer
orchestration against228 original cases. Typed642425/6424A9/64252D/642786 child
wrappers and640D6E/64268E/6426FB births match421 original cases, including146
authored records from all26 skins; the wrappers reuse the recovered generic,
bitmap/grid birth and append/detach owners. Font/style preparation and measure,
resource residency, native allocation and live manager ownership remain explicit
lower services. No native width or residency success is supplied by these helpers.

The complete bitmap virtual20 owner61C364 now matches4,898 unchanged original
hostCPU cases, and611703/61174B/6117E0 with full61D162→61D05F→6096A0 projection
matches6,351. Strict and ASan/UBSan/leak checks compare source32 read/write order,
complete banks, output identities, fault prefixes, full MXCSR and x87 control.
Actual signed rectangles, animation offsets/scales and viewport globals remain
registered inputs. Hardware originals are authoritative;34 bitmap and11 bounds
Unicorn discrepancies are retained. The production arithmetic was not fitted
to the emulator. Both integrated project tests pass. These are geometry-owner
comparisons, not a live class manager or original/native framebuffer proof.

The separate frozen `widget-text-assignment-original-20261012` proof matches501
cases and1,199,232 unchanged original instructions through61BAA9/61BAB9/61BA98,
the actual short/long/clone assignment methods and their getters/copy/length
owners. It preserves the three32/512/1024-unit capacities, actual preparation
AL gate, first-font capture and fresh second style lookup, including callback
mutation and class switching. Complete banks, return values and callback-time
header snapshots match in strict, ASan/UBSan/leak and project tests. The actual
499D1F provider, renderer/font residency and live screen remain separate owners.

The frozen `widget-font-measure-original-20261012` addition matches762
physical-original cases for UTF16 conversion, escape parsing, iterator birth
and advancement,4996E4/499D1F preparation and611EA1 measurement orchestration.
Strict, ASan/UBSan/leak and project CTest pass. Hardware remains authoritative
for28 retained emulator FP discrepancies;630A68 and actual glyph/font providers
are required rather than replaced with Painter measurements. The separate
`widget-locale-disposal-original-20261012` proof passes1,489 original cases,
including all26 prompt selectors and an actual nested610257 call. Its strict,
sanitizer and integrated tests compare ordered accesses, callback snapshots and
partial failures. Original locale providers, style methods, free ownership and
the live retained screen graph remain required joins.
