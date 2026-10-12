# Multiplayer integration goal

The active goal is a complete native multiplayer runtime with recovered logic
connected to local split-screen and native LAN play. Original Xbox instructions,
authored local data and verified original behavior remain the authority.
Campaign is deferred. Shader/world-lighting work remains with the separate chat.

Completion requires all of these gates, rather than only standalone kernel tests:

- Original player/weapon tick order and RNG ownership; integrated firing,
  recovery, reload, heat, charge, zoom, switching and dual wielding.
- Authored inventory starts, pickups, drop/respawn, ammunition, grenades, melee,
  equipment, overshields and camouflage.
- Projectile collision, hit-region/material selection, damage event creation,
  explosions, death attribution and complete player/vehicle interactions.
- Vehicle controls, seats, boarding, mounted weapons and physical interactions.
- Source-backed spawn scoring, teams, rounds, variants and all original match
  engines, with their objective objects and scoring/state transitions.
- Original HUD consumers for inventory, sensor contacts, target/zoom state,
  objectives, messages and sounds in each local viewport.
- Native lobby/discovery/join/leave, authoritative replication of every game
  action/object, reconnect/lifecycle behavior and complete-match LAN validation.
- All base and Map Pack multiplayer maps resolve their local shared resources;
  each supported configuration completes an end-to-end match/restart flow.
- Code builds independently of game assets. Original data stays local and is
  imported from user-supplied Halo 2 discs. The [MCC Classic HUD/font importer](MCC_CLASSIC_HUD_IMPORT.md)
  now reads the installed Steam caches, textures and fonts directly. A full MCC
  gameplay-cache adapter remains separate work.
- Fresh packaged title launcher, focused/source differential checks, complete
  normal/sanitizer tests and actual multiplayer runtime validation. Platform and
  physical-hardware validation limits remain explicit.

The existing native UDP protocol is the LAN transport for this goal. Retail Xbox
network interoperability and Xbox Live service replacement are separate scopes.

Completed single-SMG scheduling slice: the actual multiplayer update now uses
the original Spew firing marker, barrel counter/modes, animation-selected reload
timers and a deferred pending-shot pass with shared RNG. The bounded module
matches 22 original cases, 2,220 ticks and 566 requests; actual Lockout matches
match 960 instruction-derived postpass states locally and over actual UDP.
Stock SMG owner-motion projection and launch construction match 32 original
cases and are integrated, with marker/platform/world boundaries still open.
The broader weapon gate remains
open: successful projectile construction/muzzle transforms, global RNG startup
and other consumers, dual wield, alternate triggers and animation interruption
are not covered by that result. Authority: [SMG scheduling](SMG_FIRING_1_1_RESEARCH.md).

The independently recovered timed-equip path imports the actual first-person
ready event (type4/frame15), with seven original clock queries, seven admission
states and 44 local plus 44 actual-UDP ticks matching under the supplied timed
boundary. Starting loadout uses instant equip. The stock allocated own-manager
path now matches 12 reload gates and 599 source ticks/165 requests, with all
barrel bytes and represented scalar state checked at four phases. The same
599 ticks pass through actual local and UDP matches. Its one-frame own animation
and action transitions are distinct from the earlier manager-absent fixtures.
Local single-SMG first-person registration now imports authored arms/weapon/graph
node names and preserves the original binding/removal semantics. Four actual
local views own separate full bindings; 14 frozen producer steps, 41 mapping
pairs and eight hand-timer phases match. Initial native observer creation, dual
hands remain open. Protocol8 now supplies full identities for each assigned LAN
client view and runs the same registration/removal consumer through reconnect
and respawn. No unconditional initial
ready delay is applied. The local registered primary SMG clock now preserves
repeated-shot animation continuity, the original epsilon wrap and predictive
end flags, and authored timing/sound callback records. All541 original ticks,
2705 primary-channel phase states and four callback records match;516 ticks
also pass through actual local matches. The supplied current-index-NONE ready
control is checked separately and does not establish a native ready producer.
The renderer samples the exact source clock frame, rather than a shot/reload
elapsed-time approximation. Native LAN clients still use the earlier presentation
bridge for clock sampling; their bindings use authoritative full identities.
Secondary blends, full aim/camera response, audio playback, fidgets and
Xbox matrix bit parity remain open. Authority:
[equipped scheduling](SMG_CONSTRUCTOR_EQUIPPED_1_1_RESEARCH.md) and
[first-person registration](SMG_FP_REGISTRATION_1_1_RESEARCH.md),
[registered FP clock](SMG_FP_CLOCK_1_1_RESEARCH.md).

The original player pool is integrated into live native matches. Full salts
survive respawn, leave invalidates checked lookup, and rejoining the same slot
advances the generation. Native cleanup retains the previous full player field.
The storage/identity module
matches 77 original allocation outcomes, 96 pool snapshots and 28 full lookups.
Actual local/UDP tests verify the slot-zero death/respawn/leave/rejoin boundary.
The shared object pool now allocates full native unit/weapon/projectile identities;
SMG projectiles capture their full owner triple. Ordinary fatal SMG damage reads
previous history for attribution, records the new hit and synchronously releases
player ownership, preserving the full corpse. Respawn allocates a new unit while
keeping the old header/body/history/previous binding. Original garbage admission
and signed deadlines are integrated, and 67 separate scheduling/pressure/writer
results match. The incoming-owner counter tail now follows the original current
unit/fallback/forwarding lookup, four-second reset and signed-short wrap. It
matches 81 frozen original cases and six actual local/UDP ownership scenarios.
The original multiplayer AI initializer proves its optional cap is disabled.
Full world startup, vehicle/forward-link producers, death animation,
nearby/view retention and complete deletion callbacks remain open. A deadline
does not delete a corpse. Authority: [corpse lifecycle](MP_CORPSE_LIFECYCLE_RESEARCH.md).

Rendered startup exposed a native LAN timeout during graphics preparation.
The unanswered-host clock now begins with the first sent request. The finite
three-participant smoke test waits for renderer readiness before starting the
host's gameplay window; its actual SDL/Vulkan local/UDP capture passes. A
10.1-second delayed-first-request regression verifies the same network API.

Starting grenade counts use the original initializer and selected authored
starting-equipment flags, with all 48 source option/cap combinations matching.
Counts are viewport-local HUD inputs and one packed byte per native LAN player.
The supported SMG idle/fire/reload states also use the recovered first-selection
control/unit phases. Throwing, pickup, other blocking animation actions, runtime
cap lifecycle ownership and the complete inventory lifecycle remain open.
Authority: [inventory](MP_INVENTORY_RESEARCH.md).

The ordinary terrain outer-query kernel now joins the recovered raw traversal
with source location, oriented plane/material, normal offset and retreat.
It matches 1,785 original ordinary casts, 2,122 retreat iterations, 2,645
classifiers, 2,364 locators, three plane calls and every leaf/material import.
Lower contact/traversal remains unchanged when the outer endpoint retreats.
Two active-physics casts are excluded. This closes the bounded terrain query
gate, while actual pickup roots/bounds/current poses, original unsorted registry
and active Havok motion remain separate live integration gates. Authority:
[frozen fixtures](MP_PICKUP_OUTER_FIXTURES.md),
[native terrain integration](MP_TERRAIN_OUTER_NATIVE_INTEGRATION.md).

The native capsule and pickup visibility consumer now matches207 original
capsules/consumer calls,187 exact normalizations and1513 ordered casts through
the native terrain kernel, with21 visible/186 blocked results. Current crouch,
full identities, source vectors and outer results come from the separately
measured original replay. This closes that bounded arithmetic/query consumer;
live item state/current poses, pickup admission/transfer and world/Havok
producers remain open. Both normal and focused sanitizer comparisons pass.
Authority: [measured source](MP_PICKUP_CONSUMER_MEASURED_RESEARCH.md),
[native consumer integration](MP_PICKUP_VISIBILITY_NATIVE_INTEGRATION.md).

The native item-point producer separately matches 208 original calls, including
three marker/contact-plane projections and 21 defined arithmetic stages. It now
feeds the native visibility/terrain path for all 207 consumer cases directly;
the combined comparison includes two marker projections and 205 root fallbacks.
Both normal and focused sanitizer checks pass. Current nodes, contacts, root
state and capsule inputs remain literal original observations; this does not
yet close their live world/physics producers or pickup admission/transfer.
See [native item-point integration](MP_ITEM_POINT_NATIVE_INTEGRATION.md).

Authored netgame equipment is now imported without compacting source row indices.
The bounded stock weapon timer and allocation-result module matches 17 original
cases, 615 visits and 30 allocation requests. An actual Lockout-cache integration
test checks the BR/SMG row imports and the complete recorded timer sequence.
This module is not yet the live world-weapon allocator: stable carried-object
ownership, physical allocation/settling, targeting, action input and animation
admission remain integration work.

The original weighted spawn scorer/chooser, static zones and current living
players replace the former distance heuristic on original maps. All 72 scoring
cases, 101 selectors and 9,799 candidate states match; imported fields and actual
empty-world initial native spawns match all 21 maps. The original world-shape
service, death-history producer and moving-world influences remain open inputs.
Authority: [spawn selection](MP_SPAWN_RESEARCH.md).

The follow-up original lifecycle execution establishes the history ticker,
spawn-before-expiration order and head-marker capture contract. The native
history ticker matches 132 original cases; it remains separate from the live
match until current-unit ownership and animated head capture are integrated.
Ordinary eye arithmetic is integrated into simulation and multiplayer cameras,
matching 98 original outputs. These results do not establish attachment modes,
full pose production or Havok contact generation.

The ordinary FFA respawn path now retains a signed countdown and runs its
eligibility stage before world updates, using the original decrement, one-second
minimum and 32-tick player-slot gate. The differential matches 90 original death
initializations and 11,280 eligibility calls. Native Lockout timelines for slots
0, 3 and 15 match the original death-at-clock-100 scheduling boundary. The queue
uses the inspected and independently executed original shared-RNG removal order.
The earlier timing checks supply immediate ownership release; ordinary live SMG
now follows the original synchronous post-history detach. Complete death services,
local wall-clock preview, notification delivery, membership and other lives/team
modes remain separate. Initial native joins still use their existing direct
startup path. Authority: [respawn timing](MP_SPAWN_RESEARCH.md#follow-up-active-tick-live-countdown-and-early-respawn-scheduling).

Immediate native player creation preserves the selected authored XYZ. It no
longer applies an unsupported downward ray/root offset or prefilters the
player-start BSP short. All 66 Lockout starts and one explicit metadata override
match original setup and complete constructor output under suspended physics.
Empty-start native fixtures retain unassigned players and retry 100 eligible
phases without incrementing the spawn count or replacing the initial-zone bank.
Live Havok shape admission, later contact solving, post-selection creation
failure and full initial-join queue timing remain open. Authority:
[placement](MP_SPAWN_RESEARCH.md#follow-up-authored-player-placement-and-actual-lockout-bsp-construction).

The raw posed-collision module retains authored 3D/2D trees, planes, leaf
references, surface edges and model-material indices. Its actual Lockout import
and query match all 292 untouched original queries (234 hits), plus 2,350 surface
containment cases (1,696 accepts), including endpoint and equal-fraction behavior.
The coordinate-order table comes from the user's local executable. This module
is separate from the live capsule hit test until the actual animation poses,
region state, broadphase and world-location producers are established.
Authority: [posed collision](SPARTAN_COLLISION_1_1_RESEARCH.md).

Ordinary sbsp collision import now shares the original raw traversal without
applying model transforms. The differential matches all 1,506 recorded source
pickup segments, including 1,112 hits, fraction/contact identities, final
descriptor state and visited-leaf order. Intact breakable state is an explicit
query input. The ordinary outer branch/location/endpoint correction is now
ported and checked separately; active world dispatcher branches and the actual
pickup action remain integration work. Authority:
[pickup spatial contract](MP_PICKUP_SPATIAL_RESEARCH.md).

Ordinary Slayer score callbacks and writers retain ordered requests, signed
short narrowing/clamps and active-state rejection. Score limits end at the
writer, and time limits check the pre-world clock; tied leaders produce no
winner. The stored display clock counts down or, for unlimited time, counts
up with the original cap. Native LAN protocol8 carries that value. Original
clock text uses blank zero minutes and two-digit seconds. Differential checks
cover source callbacks/writers/ranking/clocks, native completion ordering and
actual UDP completion/clock state. Full damage attribution, team/bonus engines,
round transitions and notification delivery remain open. Authority:
[Slayer accounting](MP_SLAYER_ACCOUNTING_RESEARCH.md) and
[clock text](MP_CLOCK_TEXT_RESEARCH.md).

The live default-property stock SMG damage path imports all three original
amount fields and executes the equal-upper `DB160` composition. It fixes the
former zero amount from reading only tag+1C. Source direct-event/default-Slayer
producer evidence establishes the supported multiplier and event scales,
yielding 4.75 without damage-selection RNG. The amount differential matches 54
original raw/published-handicap cases; the live-used no-region vitality
composition matches 170 complete-entry samples. Six supplied clear-contact
cases execute the production SMG firing/projectile/damage path locally and
over actual UDP, including shield/body hits and death/ammo/score replication.
Edited/retained remote handicap properties and traits, full impact/result
construction, original collision and startup ready timing remain open.
Authority: [event/strength producers](MP_SMG_EVENT_STRENGTH_RESEARCH.md).

The damage-history writer/consumer matches three source resets, 14 writes and
174 result consumers with full identities. It is connected to live stock SMG
damage in original order: lethal attribution reads previous history before the
current hit is recorded, then aftermath synchronously detaches ownership.
The attribution selector matches 191 result gates and 18 admitted selections;
six production local/UDP cases check credit, assists, deferred projectiles across
leave/rejoin and persistent corpse identity after respawn/deadline. Native UDP
protocol8 replicates assists and suicides alongside signed-short score/counters.
Other damage sources, mount/vehicle assists and full writer/notification services
remain open. Authority:
[damage/history](MP_SMG_DAMAGE_HISTORY_RESEARCH.md).

Completed asset slice: original direction/fade tables now come from a validated
local executable provider, with no original table blob embedded in source.
Explicit shared-resource roots resolve all 21 original multiplayer maps, including
HUD fonts in the separate DLC extraction tree. The native LAN compatibility
identity includes shared game data and selected BSP/weapon, independent of paths.
Original maps, textures, audio, fonts, movies and executables stay outside version
control. Packaging and the future MCC adapter remain separate completion gates.

The goal remains active until the above integration and validation work is done.
The native unit-input and empty-inventory-slot consumers now match 10 and 9
original calls, respectively, including complete before/after unit prefixes.
Normal and focused sanitizer comparisons pass. Source marshalling, live unit
storage, admission, ownership/world removal and transfer orchestration remain
open. This is a bounded consumer slice of the measured transfer chain, not its
complete integration. See [native unit input](MP_UNIT_INPUT_NATIVE_INTEGRATION.md).
The native slot and sequence orchestrators separately match 10 calls each and
59 exact ordered writes. Nine ownership, nine world-removal and one drop leaf
retain literal original outputs as explicit service boundaries. Normal and
focused sanitizer comparisons pass; these results do not establish native
leaf producers or complete live transfer. See
[inventory orchestration](MP_UNIT_INVENTORY_NATIVE_INTEGRATION.md).

Native neutral initialization and the measured cached-command branch now match
both source-owned command cases normally and with focused sanitizers. Complete
command/delivery/record buffers and the original rejection/admission AL agree.
Codec, control/device ownership, readiness/state4 and live frame integration
remain open. See [command kernels](MP_PLAYER_COMMAND_NATIVE_INTEGRATION.md).

Native first-item update/motion matches six original resting/miss/contact calls,
complete captured raw buffers and58 ordered stores, normally and with focused
sanitizers. Twelve original transform/effect/world/moving-state output leaves
remain explicit; continuous settling and the live frame route remain open.
See [first-item motion](MP_ITEM_MOTION_NATIVE_INTEGRATION.md).
The three native ordinary/dirty/new-object dispatchers match 13 literal
header/bitmap/queue/context/body states, ten explicit original-output producer
boundaries and 15 ordered owning writes. Normal and focused sanitizer checks
pass. This supplies source scheduling order, but does not integrate native
motion/contact/cleanup producers or the complete frame. See
[object scheduling](MP_OBJECT_SCHEDULE_NATIVE_INTEGRATION.md).
The native resident sound variation importer and gain/pitch/permutation kernels
now match17 sound/23 permutation metadata records,32 original selector calls,
three creation scalar slices and83 exact ordered LCG stores. Normal and focused
sanitizer checks pass. The local seed is a supplied boundary; this does not
establish accepted ordinary scheduler playback or global audio RNG ordering.
See [native sound variation](MP_SOUND_VARIATION_NATIVE_INTEGRATION.md).
The user requested actual gameplay screenshots in this chat at visible milestones
to review on their phone; captures are kept under `analysis/multiplayer/integration`.
`MULTIPLAYER_PROGRESS.md` records current implemented behavior and limitations;
source discoveries are recorded in the existing research documents.

Native item matrix/current-node/bounds producers now match46 math calls and
eight full-storage producer calls, including original parent topology, aliases
and ordered writes. RCPSS, positive callbacks, location/registry owners and
live frame integration remain explicit boundaries. The FBBE0 contact ABI is
corrected to point input/normal input/adjusted-point output; its original
over-wide observation label remains immutable. See
[item producer integration](MP_ITEM_TRANSFORM_NATIVE_INTEGRATION.md).
