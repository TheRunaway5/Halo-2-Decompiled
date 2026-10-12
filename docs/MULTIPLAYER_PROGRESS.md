# Native multiplayer build

The current build provides local split screen and native LAN free-for-all
Slayer. It is an initial playable multiplayer implementation, with substantial
sandbox, presentation and networking parity work still outstanding. Campaign
work is deferred at the user's request.

The [fresh TU1.5 physics-frame entry](PHYSICS_NATIVE_FRAME_TU15_20261012.md)
now runs the complete original outer frame over its newly constructed native
world. Release and address/undefined/leak builds each pass 120 frames on Lockout
and Coagulation. Four original host-CPU comparisons match 3,948 instructions,
172 non-stack stores and all retained non-stack banks, with exact x87 control
state. The second cohort freshly compiles all linked physics/platform/heap
modules; its reused cache decoder remains uninstrumented. File-access tracing
confirms no BIOS or EEPROM input. Player construction, populated-world stepping,
full match initialization and the red lift/vehicle acceptance cases remain
open. A separate diagnostic also passes session-before-world initialization
on both maps in Release and sanitizer builds, with two additional original CPU
comparisons. This component does not deliver an updated playable launcher.

The separate [fresh TU1.5 physics pool path](PHYSICS_POOL_STARTUP_TU15_20261012.md)
now executes the real impact and physical-component constructors over native
host allocations, without copying a captured heap. Its 15 complete source
bodies and pool lifecycle match 3,552 original calls and 122,451 ordered writes
across three fresh address layouts. The selected Release and ASan/UBSan/leak
checks pass; the independent recovery, generation and original runs reproduce
identically. This resolves pool storage prerequisites. It does not create live
authored bodies or replace the current match mover, and does not change the
failing lift/vehicle acceptance results.

The separate [fresh TU1.5 main Havok heap](PHYSICS_MAIN_HEAP_TU15_20261012.md)
now joins the missing raw storage producer to the existing complete constructor,
size-class allocator and fixed-arena facade paths. Across three fresh layouts,
1,839 source calls match 402,072 original instruction observations, 24,114 ordered
writes and complete final heap bytes, including exhaustion/free/reuse. This
closes another startup prerequisite; renderer overflow storage, the mode
transition and live world/body/controller integration remain required.

The [fresh TU1.5 physical stage and partitioner](PHYSICS_PHYSICAL_STORAGE_TU15_20261012.md)
now execute four complete original bodies against actual native physical pages.
The original arena partition address is produced from the source reservation;
48 calls, 216 ordered writes and 38 allocation/protection operations match the
unchanged TU1.5 executable.

The [fresh TU1.5 arena and async startup](PHYSICS_GAME_ARENA_TU15_20261012.md)
now run 17 complete arena/queue/worker/file/CRC bodies against actual native
resources. Thirteen original calls match 1,049,656 ordered writes, complete
main-call ABI and all 52,667,416 final retained bytes, including both real
savegame files. Release, address/undefined/leak and thread-sanitizer checks
pass without skips. An additional ungated real-thread startup completes.
The encompassing physics initializer, renderer overflow and live integration
remain open; this component does not update the game launcher.

The [TU1.5 early-initializer composition](PHYSICS_EARLY_INITIALIZER_TU15_20261012.md)
now connects fresh arena/CRC, impact and component pools, and raw main storage.
Five phases match 1,049,789 ordered writes, complete phase ABI and all 53,842,968
final bytes, including four actual pool/main allocations. Four checks pass in
each Release, address/undefined/leak and thread-sanitizer build. The encompassing
initializer stops at its exact pending renderer-overflow call; its return,
later heap/world construction, mode transition and live integration remain open.
The idle async worker is retired before the synchronous pool owner accesses
the bank; concurrent live-world startup is not established by this comparison.

The [TU1.5 renderer storage producer](PHYSICS_RENDERER_STORAGE_TU15_20261012.md)
now constructs its original parameter arrays, acquires actual native pages,
publishes its base and enters its descriptor constructor. Twelve fresh cases
match 744 original instructions, 60 ordered non-stack writes and all 20,263,200
final bytes. Release, address/undefined/leak and thread-sanitizer builds each
pass both selected checks. All three complete bodies are compiled; execution
still stops at the actual ordinary process-heap allocation. No completed
renderer or live multiplayer result is supplied.

The [native desktop startup adapter](NATIVE_TU15_STARTUP_PLATFORM_20261012.md)
now supplies application-owned capability/configuration interfaces to the
original TU1.5 startup caller without BIOS, EEPROM or captured kernel tables.
Forty-eight fresh prefixes compare 2,832 original instructions and 1,800 source
stores through the genuine pending timer call. Release and address/undefined/
leak checks pass, and an actual file-access trace confirms those reference
files are not opened.

The [native TU1.5 timer and process-heap continuation](NATIVE_TU15_STARTUP_TIMER_20261012.md)
now supplies a real native one-shot timer, its lifecycle and the complete
original power DPC. Forty-eight fresh prefixes compare31,656 original
instructions and13,008 ordered source stores. Twenty-four setup calls return;
the24 full entries create and publish their process heap using actual native
pages, then reach the original required volume validator. All three selected
checks pass in both frozen Release and address/undefined/leak builds. Real
rearm, cancel, expiry, callback and cleanup tests pass without BIOS access.
The frozen audit records four later shared-file changes separately; it does
not claim to validate those changes. Full startup return, renderer completion
and live world integration remain required. This is not a new playable-build
claim. The BIOS remains reference-only and must not become a distributed
runtime dependency.

The active physics-engine goal is unfinished. The recovered native world graph
contains 477 complete paired bodies and 35,694 statically lowered instructions.
The frozen v19 cohort passes 31 component checks in both Release
and ASAN/UBSAN/leak builds. Its retained construction run matches 282,297 source
instructions, 46,066 ordered writes and 70,346 observed state bytes, including six
native platform calls. The core constructor, agent registration and original
world setup function now return, and the encompassing initializer returns for
the retained original input. This executes the required game callback and
object iterator without supplying their results. Populated dynamic-object paths
and alternative game-unit callees remain unverified.
[Complete retained initialization and remaining ownership](PHYSICS_WORLD_GAME_CALLBACK_RUNTIME_20261011.md)
records the source evidence and limits. The new public outer game-frame entry
also returns for the actual retained input, matching 24,941 original host-CPU
instruction observations and 5,080 ordered stores. A 25-case declared incoming
FP-control matrix matches 623,879 observations and 127,024 stores, including
source control/exception state and restoration of the host FP environment.
[Outer frame evidence](PHYSICS_GAME_FRAME_RUNTIME_20261011.md) records its inputs
and limits. The final audit reads the actual captured FS base; metadata-only
v19a preserves every compiled v19 input and the failed initial audit.
The complete bodies are paired against
TU1.5; the retained initialization and full-frame scenarios execute TU1.1.
The shared generator/backend changed after this freeze to correct an unexercised
qword load. The v19 checks do not establish equivalence to that later code;
its TU1.5 width evidence and current-source reconciliation are separate.
The separate current
shared-platform comparison passes the same complete retained run. MMX index repair separately
passes 240 component comparisons in both update selections, including 48 active
alias captures and an independent host CPU tag/upper-word probe.
[MMX evidence](PHYSICS_WORLD_CONSTRUCTION_RUNTIME_20261011.md) distinguishes
native CPU semantics from original Xbox hardware proof. Native critical-section
field operations separately match 1,595 pinned compatibility transitions and
pass a supplied two-thread handoff check. Native startup/heap ownership and the
remaining game/body/controller chain are still required. The live match still
uses the host collision mover; no whole-engine or all-map parity is claimed.

Terrain bullet decals/impact particles and frag/plasma detonation particles now
use the imported effect definitions. A real-contact sweep covers all21 maps;
12 GPU captures verify stable counts across three resolutions and four views.
Explosion dynamic lights and matched original explosion frames remain open.
See [world effects evidence and limits](WORLD_IMPACT_AND_GRENADE_EFFECTS.md).

The authored frag-light Vulkan draw now passes16 controlled resolution,
resize and split-screen layouts covering textured and texture-mapping target
programs. All21 maps pass4,290 authored grenade/BSP material bindings.
An additional32 layouts verify the authored frag/plasma light colors with
explicitly zero bloom strength on controlled receivers, including four views.
Original sphere accelerator and full draw-unit
comparisons pass7,296 and2,282 cases. These verify services needed for explosion
lighting. [Light admission, candidates and visibility visits](SOURCE_EFFECT_LIGHT_ADMISSION_PARITY.md)
and complete light-list/fallback collection also match independent original
calls. Native terrain collision correction and standalone spatial registration
also match512 and128 original cases on Lockout. World-effect registration
now runs through the350-record salted light pool; its complete
lifetime replay matches932 checkpoints and33,790,592 words, including untimed
authored-color clamps. Part A/B control
matches2,048 original outputs. Per-view collection/selection now matches256
original checkpoints and6,456,064 words; small-sphere receiver preparation
matches69 queries and58,278 owned/camera words. Shared-mask culling matches
1,027 cases/586,479 words, and the joined BSP collector/cull matches48 cases/
784,966 words with native-loaded Lockout geometry. The runtime prepares receivers
per view; live global main/dynamic draw ownership and ordered geometry submission
remain unfinished. The camera-aligned Lockout comparison in
`analysis/multiplayer-shading/world-effects-goal-20261011/camera-comparison-003`
still shows unmatched grenade state, first-person pose and render history.

Saved Elite profiles now start the admitted Slayer session, including mixed
Spartan/Elite split-screen and native LAN rosters. Bodies, arms, camera/HUD,
inherited locomotion and grenade attachment select each participant's authored
representation. Native LAN peers require protocol27. See
[Elite integration and validation](MP_ELITE_MULTIPLAYER_INTEGRATION.md).

The active [all-game-mode goal](GAME_MODE_PROGRESS.md) inventories all39 original
built-in variants. Original configuration buffers now match both retail and
TU1.1, but the actual menu start audit still admits1 variant and blocks38.
The linked report distinguishes configuration recovery from runtime completion.
The shared match now passes explicit-membership Team Slayer sessions, recovered
team spawning/scoring/respawn and actual loopback LAN replication. Normal Team
Slayer launch remains gated until team selection and presentation are integrated;
see the goal report for source comparisons and runtime validation limits.

## Run

The unit-input consumer now matches 10 recorded original calls, comparing the
complete unit prefixes. Its ascending full-datum inventory-slot scan matches
9 calls. Both pass normally and with focused sanitizers. Live control producers
and the ownership/admission/world-transfer chain remain unfinished. See
[the verified unit-input scope](MP_UNIT_INPUT_NATIVE_INTEGRATION.md).

Slot and sequence orchestration match 10 original calls each and 59 ordered
writes, including full unit prefixes. Normal and focused sanitizer comparisons
pass. The 19 owner/removal/drop leaves retain explicit recorded outputs and
remain native integration work. See
[the inventory orchestration scope](MP_UNIT_INVENTORY_NATIVE_INTEGRATION.md).

The three owning object dispatchers now match 13 recorded buffer states,
ten explicitly supplied producer outputs and 15 ordered stores. Nine focused
normal checks and nine focused sanitizer checks pass. The ordinary/dirty and
cleanup producers remain native integration work; the older broader suite and
captures53/54 precede this module. Fresh captures55/56 cover the rebuilt title,
four-view Turf firing/reload and actual three-participant loopback LAN, with
stable runtime/shader hashes and no Vulkan validation diagnostics. See
[the object scheduling scope](MP_OBJECT_SCHEDULE_NATIVE_INTEGRATION.md).

The native neutral-command initializer and cached delivery now match both
original owning cases: two full command buffers, two complete delivery/record
comparisons and one actual admission. Normal and focused sanitizer checks pass;
submission codec, control/device producers and the complete frame route remain
native integration work. Captures55/56 precede this new module's runtime rebuild.
See [the command kernel scope](MP_PLAYER_COMMAND_NATIVE_INTEGRATION.md).

The first-item update and motion now match six original core calls and 58
ordered body writes across resting, terrain-miss and terrain-contact paths,
including the original angular signed zero. Normal and focused sanitizer
comparisons pass. Twelve recorded original producer outputs remain explicit;
the live transform/world/physics/frame chain and continuous settling remain
unfinished. See [the motion scope](MP_ITEM_MOTION_NATIVE_INTEGRATION.md).

The native pickup path now composes its item-point producer with 207 original
capsule/visibility checks, 187 exact normalizations and 1513 ordered terrain
casts. The separate point comparison matches 208 original calls, including
three marker/contact-plane projections. Normal and focused sanitizer comparisons
pass. This is separate from live pickups; current item/contact/pose/world
producers and admission/transfer are unfinished.
Protocol8 supplies full identities for each assigned LAN view. Fresh title,
four-player Turf and separate three-player LAN captures are in ignored
`analysis/multiplayer/integration/41-*`, `42-*` and `43-*`.
[Source scope and validation](MP_PICKUP_VISIBILITY_NATIVE_INTEGRATION.md).
[Item-point composition](MP_ITEM_POINT_NATIVE_INTEGRATION.md).

The earlier item-point checkpoint has six focused normal and six focused sanitizer
checks passing. Its asset-free source snapshot has18 passes/39 original-data
skips/57 total checks. The broader normal suite completes132 checks with130
passes and two retained failures: title haze capture uniform size and source
frustum camera mode. Current installed title and four-player Turf captures are
in ignored `48-item-pipeline-build` and `49-item-pipeline-turf`, with stable
before/after binary hashes and no Vulkan validation diagnostics. These results
do not establish full live pickup, physics or rendered parity.

The later unit-input checkpoint passes seven focused normal checks and the same
seven focused sanitizer checks. Its asset-free source snapshot builds with
18 passes and 40 original-data skips (58 checks). The rebuilt multiplayer title
launcher and four-player Containment startup are captured in
`51-unit-input-build`, with stable runtime/shader hashes and zero Vulkan
validation diagnostics. The earlier 132-check broader run is historical; it is
not a pass for the current source snapshot.

The subsequent inventory-orchestration checkpoint passes eight focused normal
and eight focused sanitizer checks. Its fresh source-only snapshot runs
61 checks with 18 passes and 43 original-data skips, pinning 741 copied files.
The broader normal build succeeds. Its refreshed suite runs 153 checks with
152 passes and one retained Ascension transparent-parts asset failure. Compiled
binaries, shaders and the configured test list remain stable; shared CMake source
changes during the run. This is not a full-suite pass. Fresh title/three-player
Containment and actual three-participant loopback LAN captures are in
`53-unit-inventory-build` and `54-unit-inventory-lan`, with stable runtime/shader
hashes and no Vulkan validation diagnostics.
The ten slot operations use 19 explicit original leaf outputs, so the live
owner/world/drop chain remains unfinished.

The verified multiplayer checkpoint uses `build-multiplayer/Halo2-Title-Launcher`
and starts at the original title prompt. Its bytes match the root launcher used
in captures48/49. The shared root launcher can be refreshed by the separate
frontend chat and then use that chat's runtime; the release manifest records
the build-specific executable for this checkpoint. The Split Screen menu launches two local players;
System Link's Start Game hosts on UDP port 2002. Select the built-in Slayer
variant and an SMG-start map such as Lockout. Other engines or unsupported
variant changes produce an explicit unavailable dialog.

```sh
# Two, three or four local players; the command bypasses the title menu.
./build-multiplayer/Halo2-Title-Launcher --multiplayer --local-players 2
./build-multiplayer/Halo2-Title-Launcher --multiplayer --local-players 4

# A playing host, including two local participants.
./build-multiplayer/Halo2-Title-Launcher --multiplayer --host 2002 --local-players 2

# Join from another copy of this build; replace the address with the host's.
./build-multiplayer/Halo2-Title-Launcher --multiplayer --join 192.168.1.20:2002
./build-multiplayer/Halo2-Title-Launcher --multiplayer --join 192.168.1.20:2002 --local-players 2

# A dedicated host has no window or local participant.
./build-multiplayer/Halo2-Title-Launcher --multiplayer --dedicated --host 2002
```

Player one uses WASD/mouse, Space jump, Ctrl crouch, left click fire and F
reload. Click to capture the mouse; Escape releases it, then exits. In split
screen, attached SDL controllers control players two through four, with
left/right sticks, A jump, left-stick click crouch, right trigger fire and X
reload. A single-player view can use the first controller. Hotplug is handled;
physical controllers have not been verified in this pass.

Use `--map assets/disc/maps/ascension.map` to select another map. Ten disc maps
have an authored SMG start and load: Lockout, Ascension, Beaver Creek, Burial
Mounds, Colossus, Ivory Tower (`cyclotron`), Headlong, Zanzibar, Coagulation and
Foundation. Midship and Waterworks are rejected explicitly because their
authored plasma-rifle start needs battery/heat combat. A different weapon is
not silently substituted.

## Additional Map Pack source assets

The original 1.1 firing-error/recovery and ordinary shot-angle calculations,
including the exact direction-table random rotation, are ported into
`src/weapon_error.cpp`. All 70 source fixtures and 13,098 complete original
rotation calls match native float bits and seed progression. The new kernels
have an offline C++ fixture test and a reproducible original-instruction
differential. The recovered single-SMG Spew path is now wired through
`Simulation::update_weapon` and a deferred shared-RNG dispatch pass.
`src/smg.cpp` matches all 52 barrel bytes, ammo/trigger/hand timers, error and
random directions across 22 original cases (2,220 ticks, 566 requests). Actual
Lockout matches additionally match 960 original postpass states locally and
over actual UDP, including
short presses, full/empty reloads and held-fire resumption. Reload durations
come from the selected original animation, with unsupported frame-event timing
rejected. See [the scheduling authority](SMG_FIRING_1_1_RESEARCH.md).
The stock owner-motion projection and projectile launch constructor also match
32 original cases and feed live SMG projectiles. Default options use recovered
shared seed `78A8`; startup consumption and full world RNG order remain open.
Dual wield, other triggers, live muzzle/node transforms, supporting platforms,
impact events and the complete successful-object lifecycle remain unresolved.

The user's Multiplayer Map Pack ISO is extracted separately in
`assets/multiplayer-map-pack`, with all 148 files verified against its SHA-256
manifest. It supplies Containment, Warlock, Sanctuary, Turf, Backwash, Relic,
Elongation, Gemini and Terminal, plus their localized catalogues/previews and
the original 1.1 title-update executables. All nine caches pass the current
native reader's header/tag/BSP inspection. The integration pass also rendered
four local views on all 12 base and nine DLC maps using the recovered SMG option
5 and the base disc's shared caches/fonts. `--shared-maps assets/disc/maps`
resolves original resources for maps in the separate extraction tree. This
checks rendered startup, not complete map gameplay or default plasma loadouts.
Evidence: `analysis/multiplayer/integration/maps/report.json` and its captures.

The original direction and fade table bytes have been removed from source.
`OriginalData` reads them from the user's local validated executable, using its
actual XBE section mapping. Supported base and 1.1 images produce identical
table bits; missing, corrupted and unsupported images fail explicitly. The
fresh data-consuming kernels still match all 70 weapon fixtures, 13,098 random
direction calls, 11,059 fade calls and the existing crouch/scalar comparisons.
Normal and sanitizer data-loader tests pass. Original game files and analysis
exports remain excluded from version control; the code builds without a disc.
The later MCC adapter remains unimplemented until actual Steam files are supplied.

`tools/audit_map_pack.py` produces `analysis/multiplayer/map-pack-audit.json`.
`tools/oracle_map_pack_levels.py` executes the original record validators and
CRC callees: all 36 valid/corrupted/version cases pass. The `.lvl` payloads are
map-menu records, not weapon-tuning tables. Recovery and application of the
title-update gameplay routines remain pending. This disc identifies update
1.1, not the final-ever Xbox update; see
[MULTIPLAYER_MAP_PACK_RESEARCH.md](MULTIPLAYER_MAP_PACK_RESEARCH.md) for the
original artifacts, exact record layout, instruction evidence and limits.

The native damage kernels now pass **20,000 exact comparisons** against the
original 1.1 executable: armor lookup, shield damage/overflow and body damage.
The same cases pass against the base executable. This verifies those bounded
arithmetic paths with supplied external services; the full title-update balance
changes and sandbox integration remain unresolved. Raw combat fields and armor
tables were also compared across all nine DLC caches against Lockout, with no
numerical changes in the checked fields. Four maps add an authored Gauss
projectile flyby sound reference. No guessed tuning was applied.

The live default-Slayer stock SMG amount now uses original `DB160`'s lower
and equal upper fields (`jpt!+1C/+20/+24`) instead of treating `+1C` as the
direct amount. That former field read produced zero damage for the real SMG.
Original direct-impact initialization and default local property/Slayer
producers establish event fractions/scales and multiplier 1; the resulting
amount is 4.75 without selection RNG. Native amount composition matches 54
recorded raw/published-handicap cases. The same vitality function used by live
combat matches 170 original complete-entry samples, including overflow and
negative lethal body values. Six supplied clear-contact production firing
cases pass locally and over actual UDP, checking shield/body/death, ammunition
and published scores. These contact fixtures do not establish original hit
finding, ready/equip startup timing, animation reactions or full attribution.
See [event/strength producers](MP_SMG_EVENT_STRENGTH_RESEARCH.md) and
[damage/history scheduling](MP_SMG_DAMAGE_HISTORY_RESEARCH.md).

The four-entry damage-history writer is now connected to live ordinary stock
SMG damage. Fatal attribution reads previous history, then the current hit is
recorded, then ownership is detached synchronously in the same damage call.
The selector matches 191 original result gates and 18 admitted credit/assist
results. Six production local/UDP cases reproduce credit, assists, death and
captured-owner behavior across leave/rejoin. The original history module also
matches three resets, 14 writes and 174 result consumers. Non-SMG/kill-volume
attribution retains its earlier native boundary. Mount/vehicle assists, other
statistics helpers and complete death writer services remain open.

The player datum pool is now live: original requested-slot allocation starts
with the name-derived salt EC70, advances the pool-wide salt and checks the
complete 32-bit datum. Respawn preserves the player generation; cleanup retains
the previous full player on the native corpse representation; leave releases
the player and rejoin gets the next salt. Local and actual-UDP lifecycle tests
match the recorded slot-zero boundaries. The reusable pool module also matches
77 allocation outcomes, 96 source snapshots and 28 checked lookups, including
wrap, holes, poison bytes and allocation failure without salt rewind. Shared
object allocation now gives native units, weapons and SMG projectiles full
salted identities. Projectile ownership captures the full player/unit/designator
at firing, and a departed generation cannot credit its replacement. Corpses
retain their full header/body/history across respawn and the garbage deadline;
the source admission/list/deadline prefix is integrated, with no automatic
ten-second deletion. Full startup allocation order, body heap registration,
animation aftermath, nearby/view retention and complete world deletion remain
open. Authority: [datum ownership](MP_DATUM_OWNERSHIP_RESEARCH.md) and
[corpse lifecycle](MP_CORPSE_LIFECYCLE_RESEARCH.md).

The synchronous incoming-owner counter now runs after the damage-history write
and before owner detach. It follows current-unit preference, full checked
fallback, class-dependent forwarding, dead-body rejection, the ordered four-second
reset and wrapping signed-short increment. All 81 frozen original counter states
match; six real local/UDP lethal cases verify the retained incoming generation
receives the counter rather than selected kill credit or a replacement player.
The source multiplayer AI initializer explicitly disables its optional cap.
Vehicle/forward-link producers and medal/UI consumers remain open. Authority:
[incoming owner counter](MP_OWNER_COUNTER_RESEARCH.md).

First-person animation frame events are now imported in authored order. The
stock SMG's timed ready duration uses the original type4/frame15 event, rather
than the 19-frame clip length. Seven source clock queries, seven admission
states and 44 supplied timed-equip local plus 44 UDP ticks match. This is the
explicit E9D50(0,0) boundary: the actual starting-loadout caller instead requests
instant E9D50(1,1), so no universal startup delay was added. The allocated
weapon-manager/action admission is now integrated for the stock one-frame own
animation, with 12 original reload gates, 599 ticks, 2,396 phase snapshots and
165 requests matching. The same 599 ticks pass locally and over actual UDP.
This includes initial state9/zero blocking, admission based on the 32-bit weapon
state plus both triggers, own idle/fire/reload transitions and all three magazine
timers. The frozen allocated fixture's missing registration suppresses hand
feedback. Local single-SMG views now use the actual recovered binding consumer,
including authored node maps, unchanged-unit selector semantics and map/channel
retention on removal. Four actual local views keep independent full unit/weapon
identities; 14 frozen producer steps, 41 mapping pairs and eight hand-timer phases
match. Protocol8 now replicates full identities for each assigned LAN client
view and runs the same binding consumer. Initial observer lifecycle, dual hands
and remote presentation clocks remain open. The older 960-tick suite
remains explicitly manager-absent. Authority:
[ready timing](SMG_READY_ADMISSION_1_1_RESEARCH.md),
[equipped scheduling](SMG_CONSTRUCTOR_EQUIPPED_1_1_RESEARCH.md) and
[hand feedback boundary](SMG_HAND_FEEDBACK_1_1_RESEARCH.md) and
[first-person registration](SMG_FP_REGISTRATION_1_1_RESEARCH.md).

## Original HUD content

Multiplayer loads the shipped `nhdt` interfaces through the selected Spartan/Elite biped,
current weapon and `mulg` scoreboard references. The 41 Spartan, SMG and
scoreboard widget rows provide their original bitmap sequences, registration,
offsets, visibility masks, functions, fonts and `shad`/`stem`/`spas` pixel
programs. There is no generated score/ammo label, rectangle shield bar or plus
crosshair. Widget and font texture externs 16/15 and the original HUD blend
states are bound in a dedicated HUD pixel path.

The shield pass also retains its authored pre-combiner alpha-kill state.
Zero-alpha padding is rejected before the original ONE/SRC_ALPHA blend, so
the meter's shaped silhouette no longer darkens its entire rectangular quad.
The bitmap, shader constants, damage/pulse behavior and HUD positions are unchanged.

Full/half/quarter layout selection and safe rectangles use the original
640x480 grid and four-pixel internal gutter. Standard aspect uses horizontal
two-player splits; widescreen uses vertical splits. Three players use one
spanning view and two smaller views, with orientation selected by the same
display mode. Bottom-right anchored widgets use the original widescreen
half-to-quarter layout promotion. Outer screen margins intentionally differ
from the internal gutter, as established by the untouched Xbox instructions.
Each view has independent shader constants and a viewport scissor. Widescreen expands anchor positions while
retaining the sprites' original proportions. Font selectors resolve through
the disc's font table (retail fonts 6 and 8), with the original ink measurement,
bearing and top/baseline behavior. Text rectangles are justified and rounded
in viewport-local coordinates before restoring the viewport origin. Score/time/name
text resolves the HUD locale
tokens. The score ordering animation uses the original half-second transition.

The shield meter binds separate current/previous fractions, the original
Spartan and overshield layer colors, the half-second held damage history and
pulse arithmetic. CPU tests compare those cases with untouched Xbox
instructions. Ammo starts with the retail creation initializer's **60 loaded
plus 120 reserve**, rather than adding the inventory maximum to the clip.

The Elite shield-enabled flag now remains independent of the species sprite
selector. Actual shot/reload, body death/respawn and per-view HUD checks pass
with original and Vista artwork. The normal Elite shield now binds its original
purple palette, and bitmap edges use the original independent rounding. Radar
sampling, sweep and ten-sample draw inputs match original instructions; live
ordinary-player contacts, fire/grenade reveal, trail fading and split-view
composition are connected. The host rasterizer is not Xbox GPU pixel parity.
Map pickup and dual-wield integration remain open. See
[HUD, pickup and dual-wield verification](MP_HUD_FUNCTIONALITY.md).

The HUD still needs projected first-person reticle correction, target/zoom
state, other motion-sensor object classes, profile/emblem rendering, kill-feed and
death/respawn messages, voice and HUD sound services. Grenade widgets now receive
each player's authored starting counts and current selection through the native
state, including LAN replication. Gemini starts with two plasma grenades;
Backwash starts with none. The initializer matches 48 source cases, and first
selection follows the recovered control/unit phases for the supported SMG
idle/fire/reload scope. Throwing, pickup, other blocking animation actions and
the runtime cap's complete lobby/lifecycle producer remain open. Full pixel
parity with a live Xbox is not
claimed. Authority and instruction evidence are in
[MULTIPLAYER_HUD_RESEARCH.md](MULTIPLAYER_HUD_RESEARCH.md).

## Match and network behavior

Sixteen participant slots share the existing 30 Hz movement, magazine combat,
damage and recharge kernels. Original authored-order weighted spawn scoring and
private RNG now replace the distance approximation for original maps. Static
initial/respawn zones and living-player influences use imported original settings,
with a separate source FFA membership identity. The core matches 72 scoring cases,
101 choosers and 9,799 candidate states; actual empty-world initial spawn calls
and imported fields match all 21 maps. The original
world-shape query, death-marker/history producer, vehicles/explosive-object
influences and complete lifecycle scheduling remain explicit boundaries. Deaths are
counted once and respawns refill the authored initial loadout. Ordinary Slayer
score requests now preserve source ordering, signed-short narrowing/clamps and
the active-state gate. The writer completes immediately at the score limit;
later same-pass score writes are rejected. Time expiry runs on the pre-world
clock; a tied top score returns no winner. Differential checks cover 20 source
callbacks, 16 native callbacks, 22 writers, nine winners, 17 clock samples and
ordered completion over actual UDP. Ordinary stock SMG uses the integrated
source full-identity attribution/history path; non-SMG/kill-volume attribution
remains incomplete. Authored multiplayer kill volumes are enforced.

The stored source display clock is updated before the world phase and replicated
to clients. Unlimited matches count elapsed seconds up to 32767. HUD text
matches 16 untouched original caller/Unicode formatter cases, including blank
zero minutes and two-digit seconds. Timed and unlimited native/UDP timelines
check that the HUD clock does not use the later incremented simulation tick.
See [Slayer accounting](MP_SLAYER_ACCOUNTING_RESEARCH.md) and
[clock text](MP_CLOCK_TEXT_RESEARCH.md).

Ordinary simulation/camera eyes now use the original operation order and match
98 original outputs. A separate history ticker matches 132 original executions;
it is not yet live animated head/history capture. Authored netgame rows
are imported in original order, and the stock weapon timer/allocation-result
module matches 615 visits and 30 allocation requests across 17 source cases.
Actual Lockout imports and timer traces also pass normal and sanitizer checks.
Live weapon allocation, pickup actions and settling remain separate work.

Ordinary FFA respawns now run before movement/combat and retain the source signed
countdown and player-slot phase. All 90 original death initializations and 11,280
eligibility calls match; actual native Lockout slots 0/3/15 match the recorded
death-at-clock-100 timing boundary in normal and sanitizer tests. Stock SMG now
releases full ownership synchronously after history recording and uses selected
credit for the writer's suicide penalty. Complete death services, initial-join
scheduling, local preview and respawn messages
remain open. The new raw posed-collision query also matches 292 original queries
and 2,350 original surface-edge cases. It is not yet used for live player hits:
animated node/region-state, broadphase and world-location producers remain open.
See [timing](MP_SPAWN_RESEARCH.md) and
[posed collision](SPARTAN_COLLISION_1_1_RESEARCH.md).

The same raw traversal now imports ordinary sbsp collision records without
model transforms. All 1,506 source pickup segments match, including 1,112 hit
fractions/contact identities, descriptor state and visited-leaf order. Runtime
breakable bits are explicit inputs. This lower query remains separate from
outer world locations, endpoint correction and the live pickup action.
Authority: [pickup spatial contract](MP_PICKUP_SPATIAL_RESEARCH.md).

Immediate spawn placement now preserves authored XYZ without the former
downward ray and root offset. All 66 Lockout starts match paired original setup
and complete constructor output under suspended physics; one explicit authored
BSP-short override also matches. The source chooser does not prefilter that
short. Failed selection retains an unassigned player and retries on subsequent
eligible early phases; 100 empty-start native phases preserve the initial bank,
countdown and count without inventing a unit. Live Havok admission, creation
failure after selection, initial-join scheduling and later contact solving remain
open. These checks establish immediate placement, not subsequent physical motion.

Native UDP protocol version 7 assigns slots, accepts bounded input commands,
simulates on the host and replicates positions, root physics velocity, look, vitality, ammo, scores,
weapon action ticks, grenade counts/selection, assists/suicides, the stored display clock and authoritative variant rules. Score and
four ordinary counters use bounded signed shorts. One client may own four
local players. Imported-data mismatches, invalid datagrams and old sequence numbers are
rejected; inactive input is neutralized after 500 ms and peers expire after
five seconds. Leave releases the slots. The maximum 16-player snapshot fits
the 1400-byte datagram budget, including a 32-byte variant name. Root velocity
supplies the original moving-object lighting interpolation input on clients.

This protocol connects native builds. It does not implement retail Xbox
System Link, Xbox Live, lobby discovery, host migration, Internet matchmaking,
prediction/reconciliation or lag compensation. The compatibility fingerprint
covers the active map, three shared caches, original executable, font files,
selected BSP and starting weapon; paths are excluded so each computer may keep
its data elsewhere. This is a native compatibility contract, not a recovered
Xbox networking algorithm. Join is currently a launcher
argument; the authored join-browser menu still needs native session discovery.

The current integration smoke test waits for three actual participants before
counting capture frames, avoiding a fast client exiting while another still
loads assets. A separate dedicated host and one-player/two-player rendered
clients joined three distinct slots and released them on exit. This is a
same-computer loopback test, not physical two-computer LAN validation.

## Verification and remaining sandbox work

The local-primary-first-person-clock checkpoint passes all 105 normal checks
(`integration/fp-clock-all-tests.log`) and 27 focused multiplayer/first-person
address/undefined checks with leak detection
(`integration/fp-clock-focused-asan-tests.log`). The source clock differential
matches 541 original ticks, 2,705 phase states and four exact event records;
516 actual local match ticks match before/after presentation. The installed
launcher samples each registered view's authored frame directly. Separate
dedicated-host/one-player/two-player rendered clients pass again
(`integration/39-fp-clock-lan-run.log`). Capture38 shows four independently
controlled Turf views at tick62, with a full reload at frame16 in view0 and
independent idle clocks in the other views. Secondary presentation channels,
camera/audio, remote-client source clocks, live physics, pickup/vehicle/objective
flows and complete-match validation remain open.

A fresh source-only copy now excludes local raw binary/shader fixtures as well
as game maps/executables/assets: the clock checkpoint builds, passes18 checks
and skips30 original-data checks (48 total), recorded in
`integration/code-only-fp-clock/report.json`. The earlier missing-map
and missing-reference failures are retained in `code-only-fp-binding` and
`code-only-fp-binding-guard`. The original-particle-material reference input is
now present and its check passes in the 105-check clock snapshot. Original-data
checks skip explicitly in the disposable source-only copy.

After the clock checkpoint, the ordinary terrain outer query matches 1,785
ordinary casts / 1,392 hits, 107 normal offsets and all 2,122 retreat iterations,
including the fraction-zero/location-NONE exits. Separate classifier/locator
calls and all leaf/material imports match. Four focused normal checks and
three focused collision sanitizer checks pass; see
`integration/terrain-outer-differential.json`, `terrain-outer-focused-tests.log`
and `terrain-outer-asan-tests.log`. This is a bounded terrain kernel; source
pickup candidates, poses and active physics remain open. The frozen native
composition contract describes the earlier inspection and is kept unchanged.

The equipped-SMG/ownership/corpse checkpoint passes all 92 normal checks;
see `integration/corpse-all-tests.log`. Twenty focused multiplayer
address/undefined checks pass (`integration/corpse-focused-asan-tests.log`).
A fresh source copy without game files builds and passes 21 tests, with 22
original-data tests correctly skipped
(`integration/code-only-corpse-guard/report.json`). The initially failing
asset-free run is preserved in `integration/code-only-corpse/report.json`;
two title-particle tests now use the existing absent-original skip convention
and still pass against present local assets. Shader work remains with the
separate chat. These checks establish the stated bounded slices, not full parity.
The original amount/vitality/production-hit differential also passes normally
and with sanitizers (`integration/smg-damage-differential.json` and
`integration/smg-damage-asan-differential.json`).
Rendered sanitizer shutdown still reports leaks and is not marked passed.

The earlier player-identity/ready/startup checkpoint passes 16 focused multiplayer
checks (`integration/identity-startup-focused-tests.log`). A full rebuild at
that point failed in the separately edited shader test's undeclared vertexBank
input (`integration/identity-all-build.log`); this checkpoint is not labeled a
complete-suite pass. Its actual rendered LAN smoke test passes with one-player
and two-player clients and three participants
(`integration/29-lan-startup-smoke.log`). The client now starts its unanswered
request deadline at the first transmitted hello, preserving asset preparation
time; a 10.1-second delayed-start actual-UDP regression also passes. The finite
smoke host starts when the first renderer is ready instead of consuming its
entire gameplay window during graphics loading. The failed pre-fix run remains
in `integration/28-lan-player-identity-smoke.log`.

Earlier `tests-asan.log` records seven focused checks covering
multiplayer, HUD, frontend and campaign regressions. Tests cover original
variant defaults, spawn eligibility, both controlled-player combat directions,
kill accounting, original ordinary FFA respawn countdown/slot scheduling, score/time end, map loadouts, actual UDP
membership/state/rules, malformed packets, leave and a full 16-player snapshot.

The HUD tests compare 160 named bitmap placement cases against original Xbox
instruction output across all 1/2/3/4-player cells in both display modes, and
60 authored numeric counter placements against the original text routines and
font metrics. Checked-in fixtures in `tests/fixtures/retail_hud*_positions.tsv`
come from `tools/oracle_multiplayer_hud_positions.py` and
`tools/oracle_multiplayer_hud_text.py`; neither imports native HUD code.
`hud-position-tests.log` records the final fixture checks, and
`hud-position-asan.log` records their final sanitizer run. The stock 48x36 outer margins and four-pixel split gutter are
preserved. Progressive-video 32x24 margins are recorded by the oracle but
currently have no native display setting. Widescreen adaptation preserves
sprite proportions; final Xbox framebuffer and active-player reticle projection
remain unverified.

`hud-alpha-kill-tests.log` records three passing current checks: native layout,
the 220 original-instruction placement fixtures, and a real SDL/Vulkan
framebuffer regression. That regression preserved 146,304 transparent padding
samples at 640x480 and 1280x720 across four views, shield damage, recharge and
overshields, while checking that visible fill still renders. It failed before
the fix at pixel `(47,160)`; before/after captures and logs are in
`analysis/multiplayer/hud-framebuffer-*`. `hud-alpha-kill-split-4.png` is an
unmodified capture of the refreshed launcher running four local views.

The native Linux SDL/Vulkan runtime was captured with two, three and four local views
on an NVIDIA RTX 5080 using the Khronos validation layer. A separate dedicated
host plus one-player and two-player client processes connected and replicated
three participants. Captures and logs are under `analysis/multiplayer`. These
are loopback and rendering smoke checks; a two-computer LAN session, physical
controllers, audible output and complete multiplayer matches have not been
verified. Windows/macOS builds were not tested.

Repeat the finite loopback check with `python3 tools/smoke_multiplayer.py`.

Grenades, pickups and weapon switching/dual wield, energy weapons, melee,
vehicles, equipment/overshield drain, teams and the remaining game engines are
unfinished. Physics and combat still inherit the workbench's documented gaps;
this is not full Havok or retail sandbox parity. Third-person players currently
use authored idle poses. Per-viewport atmosphere, dynamic lighting and world
material fidelity remain incomplete; world brightness is visible in the smoke
captures. The shader/lighting work is being handled separately at the user's
request. No complete campaign or multiplayer parity claim follows from this
build.

The refreshed title launcher and runtime are recorded in
`analysis/multiplayer/release.json`, with hashes in `SHA256SUMS`. They require
this project's extracted assets and `build-multiplayer` folder.

The bounded native sound variation importer/kernels now match17 sound
definitions and23 permutation records,32 original selector calls, three
gain/pitch/selection creation slices and83 ordered LCG writes. Normal and
focused address/undefined/leak checks pass. A fresh source-only snapshot builds
with18 passes/37 original-data skips/55 total checks. The full sound scheduler,
global seed/draw order, source admission, voices and audible playback remain
open. See [native sound variation](MP_SOUND_VARIATION_NATIVE_INTEGRATION.md).

Rebuild with:

```sh
cmake -S . -B build-multiplayer -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-multiplayer -j
ctest --test-dir build-multiplayer --output-on-failure
python3 tools/install_launcher.py --build build-multiplayer --title-first --desktop
```

The current build-specific launcher checkpoint is captured in
`integration/57-item-motion-build` (title and four-player Zanzibar firing/reload)
and `integration/58-item-motion-lan` (dedicated host, one/two-view clients,
three participants and finite join/leave). Both runs retain stable runtime,
launcher and shader hashes and report zero Vulkan validation diagnostics.
The bounded command and item-motion modules are compiled into the static
native library and focused test executables. They have no live runtime callers
and are absent from the captured executable; these captures do not verify new
physics integration. The two new command/motion checks
pass normally and with focused sanitizers. The836-file source-only snapshot
runs71 checks:18 pass/53 original-data skips. The latest broader153-check run
remains152 pass/1 transparent-parts failure; this is not a full-suite pass.
