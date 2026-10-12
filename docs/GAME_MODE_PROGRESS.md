# Game mode operational goal

The goal's objective includes **vanilla Halo 2's data-driven runtime**: make
all seven game mode families, their 39 built-in variants and associated editable
rules operational through functional original tag layouts, data types,
references and recovered game-engine/weapon behavior.

Acceptance requires:

- Shared tag handling preserves field sizes and signedness, enums, bitflags,
  tag blocks and references, plus the original variant settings contracts.
  Authored data drives the corresponding runtime behavior. Acceptance covers
  the editable settings supported by the original game, not just the subset
  currently admitted by the native adapter. Changed settings and referenced
  weapon/equipment tags must reach their runtime consumers; decoding them,
  ignoring them or substituting fixed values does not satisfy this requirement.
- The weapon/equipment paths needed by the variants consume their original
  tags and recovered behavior. Named variant, map or weapon special cases
  cannot substitute for missing general functionality; constants and dispatch
  rules that the original engine itself uses remain appropriate.
- Each variant and its supported editable rules work through normal setup,
  spawning, combat/objectives, scoring, HUD/scoreboards and match completion,
  including local and network flows where applicable. Isolated fixtures,
  configuration decoding or startup alone do not establish operation.
- Every behavioral claim has original asset, executable/decompiled-source or
  verified Xbox evidence. Unresolved data contracts and runtime gaps stay
  explicit, and the goal stays incomplete while required paths remain missing.

The goal therefore includes making the underlying tag, variant, weapon and
equipment systems functional as vanilla Halo 2 defines them. A growing list of
special cases for the 39 built-in names is not the completion target.

The goal remains **active and incomplete**. All seven original game mode
families, their 39 built-in variants, and the associated editable rules need
working runtime behavior. The current start-action audit admits ordinary
free-for-all Slayer and Team Slayer. The other37 built-in variants remain
explicitly blocked.
Admission and startup alone do not establish a complete playable match.

King's shared initialization, authored spline/occupancy, scoring and full
world-update control owners are now implemented and compared against unchanged
retail, TU1.1 and TU1.5 executions. All57 hills on the12 disc maps are covered;
The bounded native binding now connects its lifecycle/update, per-player timers,
death statistics, shared RNG, scoring/results and authored equipment/spawn
selectors. A frozen build passed seven focused CTests and scoped ASan/UBSan.
Normal King startup remains gated pending live camouflage submission, authored
hill/waypoint rendering, live event handoff, complete retained publication and
normal match-flow validation. See
[King engine implementation](MP_KING_ENGINE_IMPLEMENTATION.md) and
[native binding](MP_KING_NATIVE_BINDING.md).
The [King engine-bank implementation](MP_KING_REPLICATION_NATIVE_BINDING.md)
now matches13,623 complete original initialization/difference/application calls
across all three Xbox images, including6,153 shared callbacks and765 authored
hill rebuilds. Fresh byte-identical reproduction, three focused root CTests and
the complete scoped ASan/UBSan comparison pass. Native entity delivery,
occupied-world services, packet orchestration and full match flows remain open;
this adds no admission.
The [King wire codec](MP_KING_WIRE_NATIVE_BINDING.md) now matches15,852 complete
original codec/size calls and15,468 actual overflow queries across the same
three Xbox images. Full stream/buffer/bank comparisons, four focused root
CTests and scoped ASan/UBSan pass. Original entity identity, packet preparation,
outer packet handling and session delivery remain required. A successful
decoder can still overflow or leave partial writes, so serialization alone
does not admit King or establish a complete match.
The [shared engine-entity dispatcher](MP_ENGINE_ENTITY_NATIVE_BINDING.md)
now matches8,814 complete original calls across all ten selector slots, plus
nine explicitly recorded absent-engine faults after entity adoption. Full
records, bank callbacks, masks and service order match across the three Xbox
images; five focused root CTests and full scoped ASan/UBSan pass. Private/live
entry banks, dirty scheduling, actual packet handling and native session
delivery remain required. This adds no variant admission.
The [live entry-bank application module](MP_ENGINE_ENTRY_NATIVE_BINDING.md)
now matches4,062 whole original calls with literal retail32-byte and update36-byte
entry layouts. It preserves copies across3,678 subsequent identity rejections,
and matches384 real King bank callbacks plus shared services/hill builds.
Six focused root CTests, fresh original reproduction and full scoped ASan/UBSan
pass. Actual aggregate/packet handling and session delivery remain open.
This adds no variant admission.
The [entry update decoder](MP_ENGINE_ENTRY_DECODE_NATIVE_BINDING.md) now also
matches13,350 whole original calls using the real constructed allocator in all
three images. Identity admission, private-bank initialization/copy, all real
reader results, typed output and allocator cleanup/order match. Seven focused
root CTests, fresh byte-identical original reproduction and full scoped
ASan/UBSan pass. The4,275 successful overflow decodes retain the original later
packet-validation boundary. Native allocator backing is an explicit provider;
this does not port physical boot or establish accepted/live objective packets.
Normal King admission and the full39-variant goal remain incomplete.
The [native objective snapshot binding](MP_KING_OBJECTIVE_NETWORK_NATIVE_BINDING.md)
now passes real fragmented UDP delivery for stationary/moving FFA/team King
with16 participants, original authored hill reconstruction, signed player
timers, score/results delivery, partial loss/recovery, stale packets and slot
reuse. Five baseline CTests, FP/scope network regressions and scoped ASan/UBSan
also pass. Explicit debug positions and observed service callbacks isolate
transport; normal gameplay consumers and complete match flows remain open.
The [shared result-event recorder](MP_RESULT_EVENT_NATIVE_BINDING.md) now
matches1,944 whole original event calls and72 direct appends, including complete
ring bytes, both real clock calls, source throttle, pose-failure updates and
shared enable/freeze gates. Fresh65-file original reproduction, four focused
CTests and full standalone ASan/UBSan pass.
The [native recording binding](MP_KING_RECORDING_NATIVE_BINDING.md) now joins
actual King FFA/team and stationary/moving ticks to that recorder, genuine
current/retained native unit positions, default platform time32 and deep runtime
outcome retention. Twelve unchanged baseline CTests, a corrected actual-runtime
handoff check, FP/scope network regressions and scoped ASan/UBSan pass. Full
retained participant/machine publication and normal presentation remain open;
this adds no admission.
The [shared score-change producer](MP_SCORE_EVENT_NATIVE_BINDING.md) also
matches8,640 whole original calls and3,600 network/local publications, preserving
all defined event bytes, threshold wrapping, source gates and options reload
order. Fresh10-file original reproduction, both focused CTests and complete
standalone ASan/UBSan pass. Live native event consumers remain required.
The [native threshold-audio consumer](MP_SCORE_EVENT_AUDIO_NATIVE_BINDING.md)
now connects actual King scoring to that producer and authored general-bank
responses. All four FFA/team and stationary/moving runs produce the three
original events, audible native PCM and successful registered completions.
Existing announcer/producer regressions and scoped ASan/UBSan also pass. Native
event transport, captions and normal world handoff remain required.
The [live score HUD binding](MP_KING_HUD_NATIVE_BINDING.md) now feeds actual
engine and round-selector fields to the existing original score owner. King
time text and all seven limit-one denominators pass; actual King scores reach
the same HUD locally and through fragmented UDP. A complete frozen build
passes ten focused CTests, FP/scope network regressions and scoped ASan/UBSan.
Hill/waypoint rendering and normal King presentation/admission remain open.
The [native hill-color/marker producers](MP_KING_VISUAL_NATIVE_BINDING.md) now
match1,488 complete original RGB calls,105 invalid marker returns and84 defined
marker prefixes across all three executables, using actual mulg team colors.
Root CMake and full standalone sanitizer checks pass. The original renderer
[snapshot probe](MP_KING_DRAW_SNAPSHOT_SOURCE_RESEARCH.md) also preserves the
real hill-bitmap request boundary; neither slice claims projected markers or
rendered hill geometry.
The [native boundary generator](MP_KING_BOUNDARY_NATIVE_BINDING.md) now matches
all14,520 emitted position/UV/RGBA words from22 original post-preparation
corridors, including the observed x87 controls and UV repeat conversions.
Two focused root CTests and the full standalone sanitizer comparison pass.
Material preparation, live world submission, projected waypoints and normal
King admission remain open; this adds no admitted variant.
King's authored announcer responses and separate activation publication now
also have [verified native consumers](MP_KING_PRESENTATION_NATIVE_BINDING.md):
3,584 original event/queue operations,54 original start callbacks and actual
native PCM/completion checks pass. Normal world presentation, hill/HUD rendering,
live camouflage, recording and replication remain incomplete.
The [shared player-damage consumer](MP_PLAYER_DAMAGE_NATIVE_BINDING.md) now binds
actual native shots and area requests to authored global/hill traits, sanitized
strength, spawn countdown and friendly fire. All5,004 unchanged original
arithmetic cases, seven focused native/source checks and scoped native plus full
arithmetic ASan/UBSan runs pass. Stale captured attackers retain the original raw
slot lookup across leave/reuse, while kill attribution still checks the complete
datum. This milestone adds no normal King startup admission.
The [camouflage native binding](MP_CAMOUFLAGE_NATIVE_BINDING.md) now connects
the recovered timer/fade owners and actual weapon/damage disruption attributes.
Its frozen build passes nine focused CTests, scoped native and full standalone
ASan/UBSan, with4,968 corrected refresh/fade calls,6,912 disruption calls and546
declared original timeline calls freshly reproduced. Native completed frames,
accepted firing/positive damage, leave/rejoin/respawn and actual Lockout tag
attributes pass. Native snapshot/LAN publication now also passes nine checks
and scoped ASan/UBSan after rebuilding all 41 affected header consumers. A
separate first-biped render-context kernel matches156 original results/read
slots and passes frozen CMake/CTest plus full standalone ASan/UBSan. The
[equipped-weapon material binding](MP_CAMOUFLAGE_MATERIAL_NATIVE_BINDING.md)
now resolves current world/FP owners before authored animation, passing1,280
original material comparisons,12 focused CTests, a separately frozen corrected
GPU isolation check and scoped ASan/UBSan. Live biped/shadow rendering,
authored pickups and intrinsic suit control
remain open; these milestones add no admission.
The [shared shader texture resolver](MP_SHADER_TEXTURE_REFERENCE_NATIVE_BINDING.md)
now follows actual normalization/global-bump tag references, including forwarded
resident arrays and the original signed bump fallback. All24 original texture
prefixes, resources on all21 owned maps, material-order/GPU/ordinary-rendered
checks and scoped ASan/UBSan pass. The separate
[depth-alias research](MP_CAMOUFLAGE_DEPTH_ALIAS_SOURCE_RESEARCH.md) resolves
the stencil-byte layout but retains the unverified concurrent alias draw.
The [native vertex extern consumer](MP_VERTEX_EXTERN_NATIVE_BINDING.md) now
replaces the ordinary material RGB adapter and supports the scalar/RGB forms
needed by camouflage. It matches9,124 whole original19BE0 cases, including420
actual camouflage banks, with fresh byte-identical reproduction, ordinary
rendered/GPU checks and scoped ASan/UBSan. High destination bits select a
texture input; the original vertex caller passes scalar mode0. Live camouflage
pass selection/submission and complete matches remain required.

The [native shader-state consumer](MP_SHADER_STATE_NATIVE_BINDING.md) now
imports the original render-state extern ranges and evaluates their seven
executable descriptors. Camouflage binding supplies vertex scalar37 and
state color40, and native draws retain each material's dynamic blend color.
A full frozen rebuild passes18 focused CTests,8,156 original RGB cases,
7,392 repeated state cases, eight ordered state cases,5,040 original material
compositions, all21-map imports, rendered state/material/ordinary checks and
scoped ASan/UBSan. Fresh original outputs reproduce byte for byte. The frozen
manifest records concurrent checkout drift. Full camouflage pass submission,
framebuffer resources, alias writes and complete matches remain open; no
additional variant is admitted.

The [native fog-provider binding](MP_SHADER_FOG_NATIVE_BINDING.md) now supplies
the exact scalar39/color41 arithmetic and retained default RGB to selected
camouflage vertex banks. A fresh frozen build passes nine focused CTests,
all4,608 original provider calls across retail/TU1.1/TU1.5,64 complete original
pass-bank comparisons, fresh byte-identical reproduction and scoped ASan/UBSan.
The separate [title-update comparison](MP_RENDER_PRIMITIVES_TITLE_UPDATE_SOURCE_RESEARCH.md)
also establishes matching complete RGB/vertex/state primitive owners and
literal tables through Xbox1.5. Live view/default production, normal camouflage
submission and complete match flows remain required; this adds no admission.

The [native scene-copy geometry](MP_SCENE_COPY_NATIVE_BINDING.md) now matches
all840 emitted attributes and60 program words across35 complete original calls,
including separately declared half/quarter/offset/odd-size camera rectangles.
A fresh257-object build passes four focused CTests, all33 fresh byte-identical
original outputs and the complete scoped ASan/UBSan comparison. The shared CPU
owner preserves the quarter-destination-pixel tap contract and original rounded
reciprocal/product order. Native GPU copy/sampling, the existing water tap
mismatch, normal camouflage scheduling and alias rendering remain open; no
additional variant is admitted.

The later [destination and water-copy binding](MP_SCENE_COPY_DESTINATIONS_NATIVE_BINDING.md)
fixes that water tap mismatch using37 complete original calls over genuine
target19/21/22 resources. All888 attributes,60 program words per call and96
normalized water tap vectors match. A fresh656-object build passes four focused
CTest checks, the21-map water-content regression, fresh byte-identical original
outputs, and the complete scoped ASan/UBSan comparison. The production water-copy
shader passes12,582,912 channel comparisons on declared host images with the
validation layer enabled. A separate42-comparison static audit establishes all21
copy owners through Xbox TU1.5 and reproduces64 outputs byte-for-byte. Original
GPU pixels/centers, normal camouflage/water scheduling, alias rendering and full
matches remain required; this adds no admission.

## Data-driven operational scope

The completion target is the original tag/data contracts and recovered game
engine behavior working together through the normal runtime. Adding named
variant exceptions, supplying fixed weapon behavior or passing isolated
fixtures does not satisfy that target. Original compiled constants, parameter
IDs and engine callback registration are retained where source evidence
requires them; authored tag values and references must still drive their
runtime consumers.

The current implementation is a mixture. It reads all39 authored rows from
`mainmenu.map`, preserves the304-byte settings contract and112 parameter
descriptors, and follows actual cache references for map-default/SMG equipment.
However, `slayer_rules` still gates other engines and many settings, and the
combat/content adapters explicitly restrict the recovered single-SMG
trigger/barrel/magazine path. General weapon paths, objective callbacks and
complete edited-settings runtime flows remain incomplete. These narrow
adapters establish source-backed progress, not vanilla-equivalent general tag
support. Shared loaders, field contracts and engine dispatch must underpin the
remaining work; normal complete matches remain the operational acceptance.

Ordinary Slayer now also owns final player/team totals and wins, sends them
over native UDP, binds them to the original HUD's limit-one query, and opens
the full result scoreboard automatically. Admitted postgame world updates now
continue with scoring/statistics and respawns gated separately. The original
seven-second countdown and class fades are bound locally and over native UDP;
session teardown and lobby return remain incomplete. These owners do not make
the admitted rows fully operational.
The recovered session phase/request policy is now independently checked;
the original reason6 return constructs the authored Postgame Carnage Report.
That report, its exit/navigation and the actual session callback chain remain unwired.
The admitted native match now publishes verified combat counters and win
awards into the original retained stat bank using imported per-stat descriptors,
then freezes recording after the ending world pass. Retained participant
properties, remaining stat producers and report construction remain incomplete.
Recovered weapon counters, best spree, multikills and lifetime now also consume
the selected executable's descriptors/dispatch and original clock gates through
the admitted SMG path. This does not establish the other weapon/medal/PvP/event
producers, complete retained records or an operational Carnage Report.
The shared original identity/machine remapper now preserves complete retained
records, both PvP axes and event participant IDs, including the actual
retail/TU1.1 versus TU1.5 machine policy. Live native machine/participant identity
production and the complete participant callback binding remain unfinished.
The original local-profile identity/configuration constructors, complete local
converter and sanitizer now share exact types with retained results and pass
whole original comparisons. The native selected-profile handoff now delivers
the signed-in user's full144-byte configuration through Start, match setup and
native UDP. HUD/scoreboard consumers use its actual name and palette fields.
The native machine-address provider and complete original player-copy callback
chain remain unfinished; the retail/later-update representation difference is
preserved rather than assumed equivalent. This is configuration delivery, not
complete retained participant identity or Carnage Report publication.

The shared signed16 remaining-life field now consumes the original configured
limit, active-death decrement and respawn-admission gates. Native authoritative
snapshots and the original scoreboard consume those fields. Whole original
comparisons and native finite-life cases pass, including the winning SMG death
and postgame gates. The original participant/life ending check runs before
native world updates. Lives Per Round now reaches the existing one-round
adapter through normal Split Screen/System Link edits. One/two-life FFA and
team cases consume those launch settings and finish through real SMG impacts,
respawns, exhaustion, win/stat awards, UDP and automatic results. Exhausted
participants retain manual scoreboard input and remain excluded from the
ordinary dead-timer automatic branch. Complete spectator behavior, later-round
transitions/reset and related HUD messages remain unbound; authored Elimination
stays gated. No additional built-in variant gains admission. See
[the current bounded integration](MP_FINITE_LIFE_MENU_INTEGRATION.md).

Original round-lifecycle research now closes the earlier missing-scenario
initializer stop using real Lockout metadata. Whole original phase/post-world
calls agree across retail, TU1.1 and TU1.5, and edited empty-world first-to-three
chains complete with Reset Map Every Round disabled. The unchanged authored
Elimination buffer enables that setting and reaches unresolved full-map-reset
services. Live-object destruction has separate retained dependency stops.
These source-owner checks do not establish native multi-round operation or
admit another variant. See [the exact evidence and next owners](MP_ROUND_LIFECYCLE_RESEARCH.md).

The shared phase owner is now bound to the admitted native match's first-active
and final phase2 boundaries. Before the first tick it initializes the configured
signed16 clock and original stat validity byte. After the ending world pass it
clears validity, freezes enabled recording and loads the original150-tick round
countdown. Final game-over retains that round countdown while its separate
seven-second owner advances. Complete original-record comparisons also cover
the non-game-over countdown controller and cap-before-phase ordering.
Required live object/map reset, preparation-event and original replication
services remain unavailable in this binding. Later-round settings remain gated,
and the unchanged authored Elimination reset bit remains required. The subsequent
one-round life admission above does not enter those later-round services.
See [the native binding and validation boundary](MP_ROUND_LIFECYCLE_NATIVE.md).

Original storage research now supplies the actual player, view/control, object,
mask, decal, contrail, effect and multiplayer AI contexts to a controlled
unchanged Elimination reset probe. It passes the earlier decal stop and reaches
the missing sound context in all three executables. The physical arena and
vertex-buffer platform producers, live object destruction and the subsequent
scenario/placement tail remain unverified. This adds no runtime admission and
does not establish a complete reset or normal match. See
[the storage owners and retained stops](MP_ROUND_RESET_STORAGE_RESEARCH.md).

The shared original sound-source stop and round cleanup now exist in the native
library. They preserve actual effect/controller/cache/voice references and
datum-pool deletion, with required reached callback/platform owners. Whole
original comparisons cover225 complete/partial records across the three
executables, and scoped ASan/UBSan passes. Normal sound initialization and the
complete world-reset binding remain missing; later rounds and additional
variants stay gated. See [the shared cleanup and its validation scope](MP_SOUND_SOURCE_LIFECYCLE_NATIVE.md).

Original deactivation/deletion now returns for actual constructed inactive,
unowned Lockout bipeds after their real view/control, effect and AI storage
owners run. Ten complete original deletions include reconstruction and linked
multi-body heap retirement. The shared native final storage-release owner
preserves body-heap links and raw datum-pool deletion. Complete native
object/world/physics callback binding, current-unit ownership and the authored
world/scenario reset remain unfinished, so this adds no variant admission.
See [the constructed-object evidence and native boundary](MP_CONSTRUCTED_OBJECT_DELETION.md).

Actual original joined-player flows now construct, deactivate and delete the
owned biped and attached weapon, then spawn again from the surviving player
record at slots0,3 and15. The shared native assignment/reference/status owner
preserves signed fields, full handles, sparse roster iteration and the real
retail/update local-control difference.339 original API comparisons and scoped
ASan/UBSan pass. Full native view/control/physics callback binding and the
authored round/scenario reset remain unfinished; direct source phase/spawn
composition does not establish a normal round or complete match. No additional
variant or edited setting gains admission. See
[the ownership, constructed-player evidence and remaining binding](MP_PLAYER_UNIT_LIFECYCLE.md).

The shared native full round-reset owner now preserves the original variant
flags, object ancestry/types, weapon-tag category and sparse player/object
iteration.228 original comparisons across all three executables and scoped
ASan/UBSan pass. Separate unchanged Elimination probes really delete the joined
player's biped and attached SMG, then retain the sound-context stop or reach
authored scenario reconstruction on a separately tested cold sound prefix.
That reconstruction stops at the missing original object-looping-sound pool;
the sound initializer itself retains its unprovided physical-protection stop.
Complete world/scenario binding and normal sound startup remain unfinished.
No additional variant or setting gains admission. See
[the general reset owner and exact retained boundaries](MP_ROUND_RESET_NATIVE.md).

The unchanged original full reset now returns twice with the real joined player
and rebuilt Lockout scenario. Original class/looping-sound storage and whole
physics-tag postprocessing close the prior pool/serialized-shape stops. 23 actual
object deletions include the second rebuilt-world teardown; each reset restores
the 19 authored objects, 15 physical components/bodies and six looping records.
Literal cache-index/header API delivery and independent cold audio-constructor
prefixes remain explicit boundaries. This does not establish normal sound,
ordinary round dispatch, native world/scenario binding or a complete match.
No additional variant or edited setting gains admission. See
[the repeated source reset and data contracts](MP_ROUND_RESET_SCENARIO_RESEARCH.md).

The shared preparation phase now preserves the original replication gate:
flag 8 reaches the dirty-entity owner only for an existing match entity. NONE
entities require no notification provider; a missing reached owner still stops
before the countdown store. Complete original source phase transitions also
invoke the full constructed-world reset twice, with source player activation
and respawn-admission checks. Their explicit round requests, delivered clocks,
non-network membership context and direct spawning remain bounded API inputs.
Live native world reset and complete playable variant matches remain unfinished.
See [the original preparation contract and phase evidence](MP_ROUND_PREPARATION.md).

## Reproduce the current result

```sh
.tools/python/bin/python tools/oracle_game_variants.py
.tools/python/bin/python tools/oracle_game_variant_normalization.py
.tools/python/bin/python tools/inspect_game_engines.py
.tools/python/bin/python tools/oracle_local_profile_team.py
cmake --build build --target source_game_variant_tests game_variant_normalization_tests audit_game_variants multiplayer_tests frontend_tests -j2
ctest --test-dir build -R '^(original_game_variant_buffers|original_game_variant_normalization|native_multiplayer|native_lan_startup|retail_multiplayer|retail_frontend)$' --output-on-failure
./build/audit_game_variants assets/disc/maps analysis/multiplayer/game-variants/runtime-audit.json --require-all-admitted
```

The final command currently returns1: it runs78 actual Split Screen/System Link
menu Start Game actions and observes37 blocked variants. Its JSON records each
dialog's exact text. Each route first creates a local profile, completes its
customization/sign-in flow, and checks the actual lobby's host/readiness status.
An empty response from an unsigned-in lobby is an audit failure, rather than
a variant rejection. For each admitted Slayer variant it additionally joins four participants
on actual Lockout data and advances90 production match ticks, then joins a
three-participant loopback UDP session and verifies90 replicated ticks and the
host's variant name, score limit, time limit and team mode. Team participants
have explicit audit selections, and the UDP check compares their membership.
This is startup/transport
coverage; it does not render four viewports, exercise an objective, finish a
stock Slayer match, or prove a two-computer LAN session.

The separately rebuilt `build/halo2_native` also passes its production90-tick,
four-participant Lockout command. A separate four-view initial frame renders
at1280×720 on the RTX5080 using the repository's bundled Khronos validation
layer, with no validation diagnostics. The retained capture is
`analysis/multiplayer/game-variants/slayer-four.png`. That rendered check
contains zero gameplay ticks and establishes initial presentation only; full
visual parity and complete match behavior remain open.

## Original configuration recovery

[source_game_variant.cpp](../src/source_game_variant.cpp) now implements the
original304-byte settings buffer, all seven initializers and the original
parameter descriptor/setter/getter contract. The native Slayer adapter consumes
these fields. Score-limit IDs1..9 share source offset50; a supplied alias now
sets the actual score limit instead of being rejected or silently ignored.
The adapter still rejects unfinished sandbox/game mode rules.

The [original instruction oracle](../tools/oracle_game_variants.py) independently
executes both images without replacing instructions or callback results:

| Authority | SHA256 |
| --- | --- |
| Original retail XBE | `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d` |
| Original TU1.1 XBE | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| Original mainmenu cache | `1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b` |
| Original-output test fixture | `18acf6e0c926943057f2d087630d205b05109ae6e9cfa83ac244af26ead11f2c` |

Retail initializer19D220, setter2373BE, getter2374F0 and normalizer19D650
correspond to TU1A0A90,2394EA,23961C and1A0EC0. Descriptor tables470828 and477D10
agree. Original instructions produce identical complete buffers for all39
authored variants,4368 built-in parameter reads, all seven defaults and1347
boundary setter/getter cases. Authored override order and native map-key order
produce the same buffers for these39 rows. Original normalization changes none
of these authored buffers.

The getter's32-bit numeric path still converts through single-precision float:
INT_MAX becomes the original indefinite INT_MIN result. The native code retains
that behavior, as well as byte/short narrowing and shared-field aliases. The
tests compare complete original buffers rather than reconstructed expectations.
The focused normal comparison and isolated ASan/UBSan/leak comparison pass.
That first fixture exercises the initializer's no-locale branch. The additional
loaded-language and edited/corrupt-buffer recovery below covers the shared
normalization contract; it does not implement the original saved-variant codec
or every native settings consumer.

## Shared variant validation and data contracts

[oracle_game_variant_normalization.py](../tools/oracle_game_variant_normalization.py)
executes the unchanged retail/TU1.1 copy validator `19D620`/`1A0E90` and
normalizer `19D650`/`1A0EC0`. Both images agree on4249 complete input/output
buffers: descriptor boundary edits across all seven families, individual-byte
mutations, authored variants, and localized defaults/edits. The original copy
validator leaves its caller's304 bytes unchanged. The normalizer returns true
only for an already-valid complete buffer. Any correction replaces the entire
buffer with default Slayer and returns false; it does not retain individually
clamped user settings. Unexamined bytes remain intact for valid inputs.

The native implementation preserves the source flag masks, signed byte/short
validation, scalar bounds and engine-specific union rules, including the shared
CTF/Assault payload and forced team flags. UTF16 names occupy32 words at+4;
the bounded writer retains31 code units and terminates at+42. Native callers
supply the localized tag result rather than an invented English title.

The oracle also executes original English string-table allocation/loading,
UNIC range lookup, UTF8 conversion and bounded copies. Its seven literal
localized defaults agree across both images. It uses actual mainmenu bytes and
an explicitly initialized English language/physical pool; raw cache reads and
memory protection are the platform seams. Allocation, localization, validation
and reset results are never supplied. The native frontend now follows
matg+16C -> mulg universal+1C -> unic for its separate variant string list.
This establishes the tested English data path, not all-language parity.

The native tag loader applies the original `149F88` type/validity admission:
only frontend types0..6 and source-valid constructed buffers enter the list.
Disposable copies of actual mainmenu metadata test invalid engine/scalar rows
and a missing-descriptor no-op; original assets remain untouched.

The Slayer adapter now validates the serialized source buffer and gates changes
by supported fields/bits. Original aliases, narrowed byte writes, nonzero bit
setters and absent-descriptor no-ops have the admission of their resulting data.
Source scalar ranges0..32767 replace guessed score/time/respawn upper limits.
The unsupported-engine, equipment and rule gates remain: shared validation
does not establish a runtime implementation for every source-valid setting.
Zero native participant capacity and zero-interval team cycling also remain
explicitly unresolved rather than receiving invented runtime semantics.
The initial native map-key representation agreed with all39 original authored
buffers but lost arbitrary reordered/duplicate writes. The ordered-setting
recovery below replaces it. Original saved-variant codecs remain unrecovered.

The fixture and source reports are retained under
`tests/fixtures/game_variant_normalization_original.tsv` and
`analysis/multiplayer/game-variants/normalization-original.json`. Earlier team
and lobby verification manifests record their own historical source/binary
snapshots; later shared-contract changes are recorded separately.

The4249 source-buffer comparisons, actual cache mutations and13 focused CTests
pass, as does an ASan/UBSan/leak run instrumenting the comparison plus
`source_game_variant.cpp`, `frontend_content.cpp` and `multiplayer.cpp`. Other
linked libraries retain their ordinary production build. A freshly built
four-participant Lockout session advances90 ticks, and the16-participant native
UDP first-person regression passes. The actual78 signed-in menu Start actions
still admit one variant and block38; strict all-admitted validation returns1
as expected for the incomplete goal. These checks establish shared contract
and regression coverage, not rendered complete matches, Xbox wire compatibility
or complete game mode operation. Their hashes and logs are recorded in
`analysis/multiplayer/game-variants/normalization-verification.json`.

## Ordered settings through native loading and editing

The original `1489B5`/`149F88` tag builders apply each setting row in authored
order. Numeric aliases1..9 write the same score field, other descriptors overlap
mode-union fields/bits, and a later duplicate ID can replace an earlier write.
Sorting or deduplicating by ID before source construction can therefore change
the settings or their validation result. For example, parameter2=32768 followed by
alias1=2 is valid with final score2; reversing those rows is invalid.

`GameVariantSettings` now retains the authored row sequence, including duplicate
IDs. The source constructor replays that sequence through the original setter
contracts. An editor change is a later write; it removes only earlier writes
to that same ID, which its fixed descriptor fully overwrites. The catalogue,
selected/saved variant, native preferences and launch payload carry the sequence.
The native preference reader also retains full32-bit parameters and signed
values instead of imposing its previous guessed10000-value bound. This is the
port's existing `halo2-native-menu-v1` text format, not the original Xbox codec.

Editor selections, team flags and lobby quick options read canonical source
getter results, including aliasing, nonzero bits and narrowed values. The
scoreboard adapter also reads the source buffer; command-line weapon changes
become later writes. The lobby's separate784-value default table is removed;
default queries now use the shared source initializer and getter.

[oracle_variant_write_order.py](../tools/oracle_variant_write_order.py) executes
unchanged retail/TU1.1 initializers, setters, getters and copy validators. Both
images agree on3195 complete buffers and their queries/validity, covering all
150 overlapping descriptor pairs in each of seven families, reversed/duplicate
writes, all39 literal authored blocks and specific datatype/no-op cases. The
no-locale environment is explicit. This oracle does not execute the complete
tag builder, native menu callbacks or original preference codec.

The native comparison also uses disposable copies of actual mainmenu metadata.
Reversing two aliases changes catalogue admission as the original validator
requires. A five-row block containing a duplicate score write, narrowed weapon
value261 and nonzero grenade bit survives both normal signed-in Start routes.
Its canonical settings drive a real three-participant Lockout UDP session for90
ticks and score-limit completion through the established non-SMG death boundary.
The latter supplies death/last-damager inputs and verifies actual scoring,
completion and replication; it does not establish a weapon-combat playthrough.
The ordinary Game Setup -> Quick Options -> parameter editor route and the
native preference save/reload preserve the resulting settings, including the
source-valid32767 time value and out-of-range descriptor no-ops.

All16 focused regression checks pass, including the original lobby default and
quick-option fixtures. The final ordered-flow comparison also passes ASan,
UBSan and leak detection with its test, shared source settings, frontend loader
and frontend state instrumented; remaining libraries use the ordinary build.
Production four-participant startup and native first-person/scope UDP
regressions pass. The fresh78-action stock audit remains one admitted and38
blocked variants. This shared data-path work leaves the full operational goal
active. Current pins and validation logs are in
`analysis/multiplayer/game-variants/ordered-settings-verification.json`.

## Shared original HUD score queries

[source_hud_score.cpp](../src/source_hud_score.cpp) recovers the score portion
of retail `15F3A0` / TU1.1 `160A40`, the owning `13992A` / `13B09A` HUD conversion,
and the engine condition bank in `13A050` / `13B7C0`. Its API keeps source
membership/color bytes, flag words, salted player identities and signed-short
score/win statistics separate. Team games select the highest **playing** other
team; FFA selects the highest unsuppressed other player in pool order, including
membership-NONE rivals. Strict comparisons retain the first equal candidate,
exclude candidates at the initial-32768 sentinel, and treat tied scores as not
leading. A retained departed team is not a playing HUD rival.

The native HUD now uses this shared query for score/rival glyphs, meters,
team/FFA condition bits and its winning transition. An unlimited score limit
uses `max(own score, rival score, 1)` as the meter denominator. The HUD follows
the original `matg` appearance color block and the first `mulg` universal team's
RGB block; shader specials3/4 bind the resulting owner/rival colors. Appearance
currently comes from the recovered default constructor. Copying a selected or
saved profile's appearance into the match remains unfinished. Variant names
follow the query's31-UTF16-unit bound, and timer glyphs use its actual timer text.

[oracle_hud_score_display.py](../tools/oracle_hud_score_display.py) executes the
unchanged engine registration, complete score query and owning conversion with
actual Lockout tag/color data and explicit pool/stat/local-view/clock records.
Both pinned images agree on1330 cases covering all seven source families,
round-mode limits, signed scores, wins, suppression, membership, rival ties,
missing local/engine data, timer bounds, tag colors, masks and exact float
division bits. The complete709/519/281-instruction pairs and their relocated
operands/switches are recorded. No original instruction or return is replaced.
Current units are NONE and Territories has zero zones: this does not recover
charge/zone producers, objective gameplay, original joining/scoring or graphics.

The original query treats a limit of1 as a request for round-win statistics,
rather than individual scores. Its source branch is recovered with explicit
win inputs. At this stage the native producer was missing and normal admission
rejected the setting, including aliases. The match-end work below now carries
the actual awarded player/team wins into the HUD and admits the ordinary
Slayer limit-one setting. The shared HUD core still rejects absent win inputs;
it cannot silently replace wins with player/team scores.

The native comparison matches all1330 original displays, colors, masks and
meter divisions. A production four-player Team Slayer session advances90 ticks
and reaches the query with its actual roster; explicit presentation stimuli
then verify shared team scores through real HUD glyphs, FFA suppression/ties,
and departed-team presence. These presentation stimuli are not combat-produced
scores. The team scoreboard recovery below supplies the active team table and
player layout; normal launch handoff and complete results ownership remain
unfinished at this stage, so its menu Start gate remained in place. The selected
profile handoff below subsequently opens ordinary Team Slayer startup. Complete
results ownership and the overall operational goal remain incomplete.

All16 focused HUD/team/match/settings/menu regressions pass, along with native
first-person and scope UDP checks and production four-participant90-tick
startup. ASan, UBSan and leak detection pass with the new score comparison,
score module, HUD loader/binding and match unit instrumented; the remaining
libraries use the ordinary build. The fresh stock audit runs78 normal menu
Start actions and still blocks38 variants, returning1 for the required
all-admitted check. These checks do not establish rendered full-match parity
or physical LAN. Current evidence, file hashes and logs are recorded in
`analysis/multiplayer/game-variants/hud-score-verification.json`.

## Shared original team scoreboard

[full_scoreboard.cpp](../src/full_scoreboard.cpp) now supports the original
team table and player table together. The eight team stat records retain their
signed-short fields and 18-byte source stride. Retained membership selects the
team rows; playing membership controls ordering and dimming. Player comparison
uses team statistics before individual statistics and the original team-index
secondary sort. Its rank scan preserves the original early exit, while displayed
player places come from each retained team's rank. Suppression affects row
presentation but is deliberately absent from the team-mode sort's first selector.

The original layout includes a21-pixel gap after team rows, separate team/player
round-win columns, viewport-relative scroll bounds and the same staggered fade
over both tables. Time scores preserve the original leading space for zero
minutes, signed remainder formatting and widget truncation. Team names follow
the actual first `mulg` universal record's `team_names` `unic` reference and
original ordinal SIDs; all nine names, including Observers, and all nine RGB
records are imported from the owned map. Membership NONE uses the executable's
imported fallback color. No team label or palette is supplied by a named-mode
exception.

[oracle_team_full_scoreboard.py](../tools/oracle_team_full_scoreboard.py)
executes the unchanged TU1.5 ranking and complete roster builder with declared
pool/stat/options/view inputs. All333 layouts return normally:280 team and53
FFA cases cover signed statistics, ties, all eight teams, retained/playing masks,
observer/suppression/death/life branches, active and game-over comparisons,
round columns, sparse bitmaps and split-view scrolling. Fourteen complete
TU1.1/TU1.5 function pairs agree in instruction shape, internal branches,
switch topology and mapped direct-call targets. Serialized locale readiness
stays false. Original ordinal and disk-index lookup separately resolve the
actual team labels, then eight original row API calls consume those deliveries.
Undefined discarded emblem bytes are excluded; defined icon kinds are compared.

[oracle_team_scoreboard_painter.py](../tools/oracle_team_scoreboard_painter.py)
executes four original post-begin renderer fragments, recording180 ordered
header/team/player/pulse rectangle requests. The declared void submission service
records arguments and returns with EAX unchanged. Native vertices match the
source rectangles and all ARGB words. This is request comparison; graphics begin,
emblems, GPU execution and original font startup remain outside that boundary.

The active native scoreboard adapter now consumes actual team masks, memberships
and shared scores. The authored Team Slayer variant joins four explicitly selected
participants on actual Lockout data, advances90 ticks, and supplies the table,
literal labels, colors and team places through the production font/painter adapter.
An explicit existing death boundary followed by the real score writer verifies
published team score delivery; leaving both opposing players preserves and dims
their retained row. Manual release restores the HUD, with independent local views.
These checks do not recover weapon contact, auto team allocation, game-over stat
production or the normal lobby's selected-team handoff. The normal Team Start
gate remained at this stage; the later selected-profile handoff opens bounded
startup. The all-variant goal remains active and incomplete.

All18 focused scoreboard/team/HUD/match/menu regressions pass. ASan, UBSan and
leak detection also pass with the new test, layout, asset importer, painter and
native scoreboard adapter instrumented; remaining libraries use the production
build. The live four-view1280x720 FFA F1 probe on the RTX5080 verifies actual SDL
focus, hold/release/resume and independent local views. Its resume assertion is
bounded before deliberate SDL_QUIT, which releases keyboard ownership. The fresh
production executable advances90 four-player ticks, and the signed-in78-action
menu audit still admits1 variant and blocks38. Current hashes and logs are in
`analysis/multiplayer/game-variants/team-scoreboard-verification.json`. These
checks do not establish a rendered Team match, full-match completion, physical
LAN or Xbox visual parity.

```sh
.tools/python/bin/python tools/oracle_team_full_scoreboard.py
.tools/python/bin/python tools/oracle_team_scoreboard_painter.py
cmake --build build --target team_full_scoreboard_tests full_scoreboard_tests full_scoreboard_painter_tests full_scoreboard_assets_tests full_scoreboard_data_tests full_scoreboard_providers_tests halo2_native -j2
ctest --test-dir build -R '^(original_team_full_scoreboard|original_full_scoreboard|original_full_scoreboard_painter|original_full_scoreboard_assets|original_full_scoreboard_import|original_full_scoreboard_providers)$' --output-on-failure
```

## Team score and attribution recovery

[slayer.cpp](../src/slayer.cpp) now contains the recovered team score writer and
team winner core. [oracle_team_slayer.py](../tools/oracle_team_slayer.py) executes
the unchanged TU1.1 player writer `15CE00`, team writer `15CC90`, stat writer
`1ED7B0`, ranking `241500` and winner `15C970`. It also runs original lethal
attribution `1ED960` with explicit session membership bytes, followed by the
original Slayer death callback `2C2350`. Actual instructions establish the
18-byte team stat stride, rather than the misleading stride inferred by Ghidra.

The native comparison covers 104 score cases, 20 valid-mask team winner cases,
and 16 lethal-attribution cases. It verifies every supplied player/team score,
score-writer admission, score-limit completion and explicit winning team,
enemy/betrayal/suicide credit, assists and nonenemy history. All eight team
indices, signed-short narrowing, signed-32-bit wrap, score clamps, retained
team masks, ties, suppressed players and membership NONE are represented.
Sum scoring applies the full requested delta to the team; Minimum and Maximum
scan unsuppressed players and retain a previous team high score when the new
aggregate falls. Completion checks the prospective 32-bit team total even when
the stored short wraps or clamps. Team Play and Team Scoring are original
parameters 91 and 23; the authored Team Slayer overrides set Team Play to On
and its score limit to 50.

Five additional original ranking observations use an empty team mask. In that
case `15C970` consumes a short that `241500` did not write. The native core
returns an unresolved result instead of treating residual emulated stack data
as a source winner contract. The fixture supplies salted session/player records,
disables persistent profile writes, and uses NONE replicated engine entities.
The earlier fixture supplies13A130 and1A24F0, plus the separate attribution
history/death writer. Its13A130 label incorrectly called that routine a session
notification: it is the game-over flag/countdown owner. The score writer's
C04 completion status executes, but this earlier fixture does not establish
game-over records or finalization. The unchanged frozen report remains a stage
record; the new game-end replay below executes13A130's complete u5 peer and
the finalization owners without supplying those returns. This earlier scoring
recovery is TU1.1 only.

Reproduce the source comparison:

```sh
.tools/python/bin/python tools/oracle_team_slayer.py --fixture tests/fixtures/team_slayer_original.tsv
cmake --build build --target team_slayer_tests -j2
ctest --test-dir build -R '^original_team_slayer$' --output-on-failure
```

The final comparison and its isolated address/undefined-behavior/leak sanitizer
run pass. Seven focused CTests pass, including configuration, existing FFA
scoring, frontend, native startup and native LAN startup. The freshly built
production executable passes four-participant, 90-tick Lockout startup. The
signed-in menu audit again records one admitted and 38 blocked variants, with
four-participant local and three-participant UDP startup for admitted Slayer.
That recovery stage's source/binary hashes and logs are retained in
`analysis/multiplayer/game-variants/team-slayer-verification.json`.

## Native Team Slayer session integration

The shared match now accepts explicitly selected source team memberships through
`slayer_rules(variant, true)` and `MultiplayerMatch::join(name, team)`. This is a
tested match API; the ordinary menu and production launcher still reject Team
Slayer while their team-selection and presentation flows are unfinished.
Neither an alternating-team allocator nor an automatic balancing rule is supplied.
Custom maximum-team counts, Force Even Teams and Friendly Fire Off remain gated.

[oracle_team_session.py](../tools/oracle_team_session.py) independently executes
the original retail and TU1.1 team bookkeeping and spawn functions. The original
Slayer slot4 and permitted-designator routine `157970`/`159010` admit all eight
map designators. With default maximum-teams0, a real selected-team roster pads
the logical team mask to eight; slot26 returns NONE, giving an identity mapping.
An empty roster stays empty. The 168 native mapping comparisons cover sparse
and empty permitted masks, logical teams, padding and all nine mapping entries.
The subsequent original player-property refresh has an empty player bitmap and
NONE replicated entities; its live property and session codec producers remain
separate from these explicit valid membership inputs.

Forty presence comparisons execute complete original `158140`/`1597E0`, including
its original stat-recording leaf with recording disabled. Suppressed players
contribute to sticky team presence, but not current playing teams. The retained
mask never loses departed teams; time-limit ranking therefore retains their
scores. Forty additional original full collector/chooser runs compare every
float bit in actual Lockout team/FFA initial and respawn influence banks, and
the selected authored spawn. The cache loader now keeps all raw zone team/mode
masks in source order. Native Slayer filters those banks through the recovered
mapping and uses source membership equality for teammate/enemy influences.
Vehicle/explosive influences, death-marker production and live Havok spawn-shape
admission remain the previously declared missing world inputs.

The TU1.1 death writer runs unchanged to `15E7C3`, after its timer/penalty writes
and before its marker, announcement, life and medal services. Forty-eight
original cases cover Inheritance, Cycling and None, selected enemy/self/teammate/
NONE credit, and existing teammate/victim countdowns. All 48 also pass through
the native death boundary at source clock200. Betrayals add the default300-tick
penalty to the attacker when the victim's countdown is zero. A later attacker
death consumes it once, producing450 ticks with the default150-tick interval.
Actual SMG damage invokes the same timer service synchronously before the clock
increment; the fallback death boundary explicitly passes that earlier clock.
Zero-interval cycling and an unrecovered overflowing cycle are rejected.

Team score writes, Minimum/Maximum high-water behavior, immediate score-limit
completion, time-limit ties and departed-team winners now run inside the match.
Source membership remains distinct from each participant's FFA identity. Native
LAN protocol12 carries the selected membership in each hello and replicates
team scores, retained masks and winning team with the complete authoritative
roster. Older native protocol11 packets are incompatible. This protocol is the
port's transport, not the original Xbox System Link format. A two-view request
with only one free slot is rejected before any participant is created, so a
never-joined team cannot enter the sticky ranking mask.
Newer complete snapshots now publish roster changes at an unchanged simulation
tick; the transport sequence still rejects stale fragments and the original
simulation clock is not advanced by publication.

The actual authored Team Slayer row from `mainmenu.map` passes four local
participants and90 production match ticks with explicit team choices. Two
actual loopback UDP peers join two local views each, giving six participants
and90 compared authoritative snapshots. The same test extends the roster to16,
compares the fragmented team state, rejects missing/over-capacity selections,
and completes when teammates on different peers contribute to one score limit.
Four production lethal SMG timelines cover enemy and teammate hits, locally and
through UDP; betrayal consumption and source identity-gated respawn also pass.
Controlled positions, contact radius and low victim vitality are explicit
fixtures for those projectile cases, rather than a claim about authored combat
encounter geometry. Aggregate/time-expiry cases use the declared non-SMG death
boundary; they do not fabricate projectile hits.

Reproduce this integration:

```sh
.tools/python/bin/python tools/oracle_team_session.py --fixture tests/fixtures/team_session_original.tsv
cmake --build build --target team_session_tests team_match_tests -j2
ctest --test-dir build -R '^(original_team_session|native_team_match)$' --output-on-failure
```

The296 new native source comparisons, native team session tests and13 focused
configuration, FFA, spawn, identity, lethal, frontend and LAN regression checks
pass. Focused ASan/UBSan/leak runs instrument the changed spawn/match/transport
units and the team score/attribution units; the linked cache and remaining
simulation libraries are the normal production build. A separate existing
first-person UDP regression passes16 participants, four source SMG clocks and
loss/reorder/duplicate/late fragment recovery. The fresh production executable
also passes its four-participant90-tick FFA command. Evidence and current hashes
are in `analysis/multiplayer/game-variants/team-session-verification.json`.
Those hashes describe that integration before the subsequent lobby picker
changes below; they are historical after newer source/binary changes.

The fresh signed-in78-action menu audit still admits1 variant and blocks38,
recorded in `analysis/multiplayer/game-variants/runtime-audit-team-session.json`.
The earlier initial FFA rendered capture predates this integration. Team menus,
HUD/scoreboard/results, rendered team matches, physical LAN, objective modes,
complete sandbox rules and full Xbox parity remain unverified. This progress
does not establish operational Team Slayer through the normal user flow and
does not complete the all-variant goal.

## Native lobby team picker

[oracle_frontend_team_selection.py](../tools/oracle_frontend_team_selection.py)
now executes unchanged retail `250B1E`, `250CDA`, `250D5C`, `251364`,
`199967/19989D/199951`, `251139/2510E1` and `19C5F0`. Real `mainmenu.map`
multiplayer records are relocated without changing their16 count bytes at+C54.
The source count uses game type1..9, independently of the frontend enum.
Missing records and counts0/1 produce1. The loader previously discarded those
bytes; it now retains all16 for every map.

The fixture contains958 comparisons:143 map/type count cases,80 original
availability queries,128 begin/commit cases across controller slots0..3,
400 cycle/wrap/narrowing cases,204 full idle-handler event steps and3 admission
guards. Named seams supply session/map/controller records, idle channel and
countdown queries, base-widget delegation and `18FE9E` profile-update delivery.
Team availability, selection, wrapping, map lookup and event decisions execute
their original instructions. Original profile request encoding and delivery,
countdown changes and unsigned-controller sign-in remain separate producers.

The normal signed-in native Split Screen and System Link lobbies now bind
those decisions. X begins a preview; left/right cycles the map's source team
count; A/Start or X commits; B/Back cancels while retaining membership and the
lobby. Preview validity suppresses Start's hit admission. The original roster
consumer receives the committed member and the local preview, using its
recovered team labels and literal team colors. The old X-to-Game-Setup shortcut
was inconsistent with the original handler and is removed; the authored
Game Setup button remains the route to that screen. FFA X is consumed without
opening an editor or Game Setup.

Source flag5, serialized by parameter13, permits NONE in the cycle. The retail
cache contains no authored `sily` selector for that parameter, so no selector is
invented. Its boundary is tested with an explicitly supplied settings fixture.
The original19A2CE scan excludes NONE from distinct playing teams; a one-player
NONE membership therefore produces status7, returning to team0 restores9.
This does not implement spectator gameplay.

The actual one-profile native route passes all eight team colors through
`FrontendPainter::draw`, preview versus membership separation, commit/cancel,
and retained Team Slayer launch rejection, for both lobby protocols. Ten focused
configuration, team match and frontend regression tests pass, along with a
focused ASan/UBSan/leak run instrumenting the test and changed frontend
state/painter/content/policy units. Other linked cache/simulation units use
the ordinary build. No new SDL/GPU screenshot, live Xbox UI, multiplayer
controller attach, automatic team allocation or original session codec is
claimed here. The initial native controller still supplies team0; explicit
edits replace that lobby input. Gameplay handoff remained behind the Team
launch gate at this stage; the recovered profile conversion below connects it
to the active match/HUD/scoreboard adapter. Complete results remain unfinished.

```sh
.tools/python/bin/python tools/oracle_frontend_team_selection.py
cmake --build build --target frontend_team_tests -j2
ctest --test-dir build -R '^original_frontend_team_picker$' --output-on-failure
```

The fresh78-action normal menu audit still admits1 variant and blocks38:
`analysis/multiplayer/game-variants/runtime-audit-team-picker.json`. Current
picker evidence/hashes are in
`analysis/multiplayer/game-variants/frontend-team-verification.json`. The all
variant goal remains active; a working lobby editor does not count as an
operational Team Slayer match or a general tag-driven game engine.

## Selected profile team through normal startup

[oracle_local_profile_team.py](../tools/oracle_local_profile_team.py) independently
executes the complete retail18FD94, TU1.1 1917C6 and u5 192BAA local-profile
converters, followed by their original18FE9E/1918D0/192CB7 input-member update
owners. The local record stride isC70 in retail andC74 in the updates. Each
controller's committed team is a signed32-bit property at+1FC; the90-byte
multiplayer profile carries its signed byte at+7C. NONE(-1) remainsFF; other
negative values become0, and positive values above16 become16. This preserves
the recovered field contract instead of truncating an integer to a byte.

All4044 executions return through the unchanged original routines:337 signed32
inputs for all four controller slots in each of three executables. Cases cover
every signed-byte input,32-bit extremes, byte aliases and deterministic random
32-bit values. The original updater copies each complete produced90-byte
profile into its input-member record; identity, literal caller name and
appearance delivery also match. The report records the exact two retail/TU1.1
converter differences (stride and relocated record base), complete instruction
listings, observed team stores and full profile bytes. u5 changes its calling
convention and another field's helper, so it is independently executed. There
are no instruction patches or supplied service returns. These are declared
caller records, not proof of profile allocation, sign-in, automatic team
assignment, saved-profile encoding or the Xbox network codec.

The native one-profile frontend now converts the committed lobby selection and
puts it in `FrontendLaunch.selected_teams`. Main carries that vector into
`MultiplayerOptions`; production local joins and `LanClient` consume it. Team
play requires exactly one explicit playing team0..7 per actual local user. The
native Slayer adapter still rejects NONE and source profile codes8..16 as
playing teams; it never truncates or automatically redistributes them. Source
readiness continues to exclude NONE. The initial native controller's team0
remains a declared input; recovering the original automatic assignment owner
is still separate work. FFA retains its independently allocated identities.

The ordinary Team Slayer row now reaches normal Start through its source
Team Play setting, with active shared scoring, HUD and full scoreboard support.
There is no variant-name branch in admission or gameplay. Unsupported settings,
weapons and other engines remain explicit failures. General command-line
`--mp-rule PARAM VALUE` appends signed32 writes to the shared112-descriptor
settings path, preserving write order and aliases; repeated `--mp-team 0..7`
supplies the separate per-user profile selection. For example, Team Play uses
parameter91; setting it does not manufacture participant teams.

All eight selections pass both actual signed-in Split Screen/System Link menu
routes, commit/Start handoff, real Lockout join/spawn and90 production ticks;
System Link cases also join a distinct explicitly selected team over native UDP
and compare both replicated memberships. The fresh78-action catalogue audit
admits2 variants and blocks37, with four-player local and three-player UDP
startup coverage for each admission. Its strict all-admitted check returns1,
correctly preserving the incomplete goal.

The actual SDL/GPU F1 probe imports the authored Team Slayer menu launch and
then supplies three additional explicit test participants for four-view
coverage. On the RTX5080 at1280x720 it observes teams2 and7 through real
runtime joins, original Yellow Team/Pink Team labels and colors, team/player
roster rows, manual hold/release/resume and independent local UI ownership.
Default SDL display initialization fails in this environment; explicitly
selecting its available X11 backend succeeds. The retained renderer PPM and
PNG preview contain identical RGB bytes. This is a native render, not Xbox
raster, geometry, team body tint, emblem or full results parity.

The actual production executable also passes a one-player Team host and
three-local-view rendered client over loopback UDP, with explicit selections
2/7/2/7. The client joins slots1/2/3, observes all four participants and finishes
its12-frame run; the host advances900 ticks and observes only its remaining
local player after client cleanup. The first shorter360-tick host attempt ended
before the client finished rendering and reported a disconnect. The finite
host lifetime was extended; no networking/gameplay return was supplied.

All22 focused settings/team/profile/HUD/scoreboard/match/menu regressions pass.
ASan, UBSan and leak detection pass with the changed frontend state, match
implementation and expanded frontend team integration test instrumented;
remaining linked libraries use the ordinary build. A fresh four-player
production CLI session advances90 ticks; a missing per-user selection is
rejected explicitly. Source fixtures, logs and current hashes are recorded in
`analysis/multiplayer/game-variants/team-handoff-verification.json`.

The all-variant goal stays active and incomplete. Admission, selected-team
delivery and rendered active play do not establish complete match/results
ownership, general weapons and equipment, objective engines, spectator play,
automatic team allocation, physical LAN or Xbox protocol/platform fidelity.

## Engine dispatch evidence

[inspect_game_engines.py](../tools/inspect_game_engines.py) executes original
retail378500 and TU37EB40 registration. Each copies original static object
pointers into the actual source engine table; no object pointer or vtable is
supplied by the fixture. The report inventories51 literal callback pointers per
mode for both images under
`analysis/multiplayer/game-variants/engine-tables.json`. These are recovery
entry points, not implemented callbacks. Supporting Ghidra exports are in
`analysis/retail/functions` and `analysis/map-pack-update/functions`; readable
reference declarations are in `tools/reference/halo2-decompiled/include/engine_peer.h`.
That header's slot numbering is0..50. Its neighboring `unknown_1523c0.h`
explicitly documents a30-slot numbering error, so its subclass positions must
not be used as dispatch authority.

| Family | Frontend enum | Source type | TU object / vtable | TU death callback slot30 | Built-ins |
| --- | --- | --- | --- | --- | --- |
| Slayer | 0 | 2 | `487188 / 463B48` | `2C2350` | 9 |
| Oddball | 1 | 3 | `48718C / 463C18` | `2C2F90` | 6 |
| Juggernaut | 2 | 7 | `484DF0 / 45AEB0` | `193D20` | 5 |
| King | 3 | 4 | `487190 / 463CE8` | `2C3DA0` | 5 |
| CTF | 4 | 1 | `486C90 / 460E78` | `242FF0` | 4 |
| Assault | 5 | 9 | `486C90 / 460E78` | `242FF0` | 5 |
| Territories | 6 | 8 | `487194 / 463DB8` | `2F3D70` | 5 |

Original CTF and Assault share the same engine object. Their source type and
variant payload distinguish their behavior; they cannot be implemented as
separate guessed capture/arming rules. Callback semantics beyond independently
recovered original callers remain unresolved.

## Built-in runtime inventory

| Family | Variants | Current first blocker |
| --- | --- | --- |
| Slayer | Slayer, Team Slayer | Admitted; broader sandbox/match fidelity remains incomplete |
| Slayer | Team Phantoms | Remaining unsupported source settings |
| Slayer | Rockets, Swords, Snipers | Starting weapon/sandbox |
| Slayer | Phantoms | Motion sensor/camouflage and equipment rules |
| Slayer | Elimination, Phantom Elim | Later rounds/full reset and remaining variant rules |
| Oddball | Oddball, Rocketball, Swordball, Team Ball, Low Ball, Fiesta | Engine/objective lifecycle |
| King | King, Team King, Phantom King, Crazy King, Team Crazy King | Live camouflage, authored hill/waypoint rendering, normal-world event handoff, complete retained publication and full match flows; bounded native scoring, HUD time text, announcer audio, recording and fragmented objective UDP verified |
| Juggernaut | 2 on1,3 on1, Ninjanaut, Phantom Fodder, Dreadnaut | Engine/role lifecycle |
| CTF | Multi Flag CTF, CTF Classic,1 Flag CTF,1 Flag CTF Fast | Engine/objective lifecycle |
| Assault | Multi Bomb, Single Bomb, Single Bomb Fast, Neutral Bomb, Blast Resort | Engine/objective lifecycle |
| Territories | 3 Plots, Land Grab, Gold Rush, Control Issues, Contention | Engine/objective lifecycle |

These are the first observed blockers, not an assertion that fixing each one
alone completes its variants. The full authored overrides are retained in
`analysis/multiplayer/game-variants/original.json`, including every original
parameter's title/choices. Shared prerequisites include teams and betrayal,
round/life management, objective item interaction, all selected weapons and
secondary loadouts, equipment/camouflage/overshields, vehicle selection,
respawn modifiers, and mode-specific scoring/termination.

## Completion evidence still required

For each built-in and its editable rule groups, preserve source-backed
initialization and demonstrate actual menu selection, match startup, valid
spawns/loadouts and team/role assignment. Exercise each mode's authored objective
and adversarial cases: pickup/drop/death/return/reset, contested/moving regions,
role transfer, betrayal/suicide, round/life transitions and score/time outcomes,
as applicable to recovered source. Then verify the actual HUD/scoreboard/results,
local split-screen views and authoritative host/client replication together.
Use the original executable or verified Xbox behavior for every rule that is
still unknown. No complete-goal claim follows from passing configuration tests
or removing launch rejection alone.

## Original match end and native final results

[oracle_game_end.py](../tools/oracle_game_end.py) executes the unchanged u5
15DDC0 round-end,13B4A0 game-over,15F450 finalizer,15EFA0 lifecycle and
160750/1F1A60 score-carry owners. It registers the actual original engine
table first. There are70 full endings,128 full-bank carry cases,896 round
selector cases plus six raw mode observations, and400 automatic scoreboard
tail calls. Thirteen complete TU1.1/u5 routine pairs contain1037 instructions;
the only additional literal difference is the declared persistent-player
record base5193F0→51EF00 in the end owner. The report preserves both complete
instruction bodies, relocated operands, branches and switch-table constraints.

Pools, occupied bitmap, salted player records, stats, options, local input
bindings and clocks are declared API stimuli. Persistent profile recording is
disabled and replicated entities are NONE. Original event functions execute
against cold source globals; no game, allocator, session, platform or graphics
return is supplied. One raw mode5 finalization stops atB8061 on its missing
simulation owner. That stop remains explicit, rather than supplying a successful
notification. The older13A130 notification classification is corrected above;
these new replays execute its complete13B4A0 peer and observe the actual
game-over flag and210-tick countdown at30Hz. They do not establish an original
world tick, initialized Xbox session, audio fade or lobby transition.

[source_game_end.hpp](../include/halo2/source_game_end.hpp) preserves the
596-byte original bank: four header bytes, sixteen28-byte player records and
eight18-byte team records. Unidentified shorts keep their literal positions.
Field0 is current score, field1 is accumulated score and field7 is wins.
Player fields9/10 and the last-kill clock clear during carry; other fields
remain unchanged. Accumulated scores wrap as signed16-bit values, while wins
increment with the original±30000 clamp. Team wins use1A8E50's iterator,
excluding player flag2; FFA validates the complete winning datum directly.
Sparse slots, different salts, NONE, stale winners, negative/clamped wins,
overflowing totals, disconnected records and unretained teams are covered.

The native admitted Slayer path now awards wins at the end request, so later
score requests in that world pass observe inactive status. It accumulates final
scores once after that pass. Native UDP publishes finalization,
player/team totals and wins, plus the existing authoritative respawn countdown
needed by the finished-scoreboard input. Totals retain their complete signed
short range. The LAN score-limit decoder now accepts the recovered nonnegative
signed-short range through32767, matching the shared settings contract.

The HUD consumes actual player/team wins, including its limit-one branch;
ordinary Slayer's score-limit1 capability gates are removed. The shared query
still reports unavailable statistics for an explicitly absent win input.
Rounds, lives and unrelated unsupported settings remain gated. The full result
scoreboard consumes accumulated scores and wins through its original game-over
comparisons. Its automatic display uses the recovered shared inactive byte and
per-view fades: one local view reaches the full overlay after37 deliveries,
four views after16 fixed ticks in original view order. No independent per-view
delay or variant-name special case is added. Active death-observer scheduling
and the contextual scoreboard title remain separate owners.

The focused comparison checks4104 assertions against the original full banks,
round selectors and automatic display sequence. Six native local/UDP endings
cover FFA/Team score limits1/2 and actual Lockout time expiry with score limit
32767, four participants, repeated finalization, the original HUD providers and
automatic host/peer result layouts. Score cases use the declared existing death
API; they do not establish stock weapon-contact playthroughs. The renderer
checks actual FFA and Team endings at tick31 using editable source parameter10
set to one second, with all four local views showing results without an F1
press. The retained Team capture is1280×720 on the RTX5080, using imported
Yellow/Pink team labels, palettes and the original scoreboard font.

This initial result slice left the postgame world frozen. The continuation
below binds the countdown and class fades while keeping simulation/session
events, final lobby transition, contextual title, emblems, connection quality,
complete source profile persistence, general weapons and other engines open.
The catalogue still admits2 of39 rows and blocks37; the goal remains active
and incomplete.

```sh
.tools/python/bin/python tools/oracle_game_end.py
cmake --build build --target game_end_tests game_end_native_tests halo2_native multiplayer_scoreboard_runtime_tests -j2
ctest --test-dir build -R '^(original_game_end|native_game_end)$' --output-on-failure
SDL_VIDEODRIVER=x11 build/multiplayer_scoreboard_runtime_tests assets/disc/maps/lockout.map assets/disc/maps assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03 --split4 --wide --team --end
```

## Original local player profile contracts

The shared profile producer now recovers the12-byte original offline identity,
480-byte full-profile constructor,144-byte configuration constructor, complete
local-profile converter and source sanitizer. It preserves signed field widths,
full local input-member configurations, UTF16 names, raw appearance/account
bytes, nullable outputs and version policy. Retail's sanitizer omits the later
multiplayer representation remap; TU1.5's actual supplement override is also
preserved. Existing native lobby team conversion and retained result records
now use these shared field/type contracts.

Native tests compare all bytes of9,606 complete calls through three unchanged
original executables, with no instruction patches or supplied service returns.
This original-contract slice compiled the profile producers into the systems
library. The later selected-profile configuration handoff is described below;
native machine identities, original input allocation and the complete
player-creation callback chain remain required. No authored variant, weapon or
edited rule gained admission. See[the profile evidence](MP_PLAYER_PROFILE_RESEARCH.md) and
`analysis/multiplayer/game-variants/player-profile-verification.json` for the
current verification boundary.

## Selected native player configurations

The actual signed-in frontend profile now supplies the complete144-byte
configuration through normal Start, local match setup and native UDP. The
unaltered request and source-sanitized live copy remain separate; actual FFA or
team membership updates the live+7C field. HUD primary color and full-scoreboard
UTF16 name/appearance now consume that selected profile and the original tag
palette providers. The fresh native input configuration matches every byte of
the complete original owning local-initialization bridge. Original standalone
constructor sentinels are not substituted for that owning state.

The saved Unicode profile test joins a16-player roster through three actual
four-view UDP peers, verifies every configuration byte after90 replicated ticks
and checks the font-backed scoreboard and HUD. Unsupported fourth-view bodies
are rejected before player allocation or sticky team-mask mutation. All30
rebuilt focused regressions pass. ASan, UBSan and leak detection pass for the
eight instrumented profile/menu/match/network/HUD/scoreboard comparison units;
other linked production libraries remain uninstrumented.

The fresh catalogue audit still admits2/39 variants and blocks37 through78
normal Start actions. This does not establish rendered frontend-to-match
delivery, general body/appearance support, original machine/participant
identities, complete player callbacks, retained Carnage Report publication or
physical LAN. Complete matches and the remaining mode/weapon/settings owners
are still required. See[the native handoff evidence](MP_NATIVE_PLAYER_PROFILE_RESEARCH.md)
and `analysis/multiplayer/game-variants/native-player-profile-verification.json`.

## Original player lives and native respawn admission

This section records the earlier gated verification snapshot. The subsequent
[one-round menu integration](MP_FINITE_LIFE_MENU_INTEGRATION.md) admits the
life field without admitting Elimination or later rounds.

[Original player-life recovery](MP_PLAYER_LIVES_RESEARCH.md) preserves the
signed32 configured limit, signed16 player+1AC count and original NONE sentinel.
Initialization, the active-match decrement and player/team exhaustion queries
come from whole retail/TU1.1/TU1.5 executions. All9,252 native comparisons pass;
an independent execution reproduces every fixture/report byte and all39
reached-owner instruction listings.

Native joins and deaths now own the count, the respawn timer receives the
recovered admission query, and authoritative native UDP protocol16 snapshots
deliver count/limit to the original scoreboard. Diagnostic finite-life cases
exercise actual respawn, exhaustion, two-view localhost UDP, font-backed
ranking/icon/dimming, and winning production SMG deaths in FFA/Team Slayer.
Postgame deaths do not consume lives. The later
[participant/life ending binding](MP_ROUND_END_RESEARCH.md) now closes the
automatic single-round decision. Normal finite-life variant settings remain
gated because complete round transitions/reset and related HUD messages still
require their actual owners.

All33 rebuilt focused regressions and scoped ASan/UBSan/leak checks pass.
The fresh78-action menu audit still admits2/39 variants and blocks37. The fresh
production four-local Team Slayer run reaches300 ticks with `finished=1`;
it is not a finite-life frontend session or300 active combat ticks. Current
boundaries, commands and exact pins are recorded in the research document and
`analysis/multiplayer/game-variants/player-lives-verification.json`. These field
owners do not establish complete finite-life matches or Xbox transport parity.

## Original participant/life ending check

The counts and commands below describe that owner's earlier verification
snapshot. Current normal menu life-setting coverage is recorded in
[the later integration](MP_FINITE_LIFE_MENU_INTEGRATION.md).

[Whole original end-check recovery](MP_ROUND_END_RESEARCH.md) preserves occupied
player versus retained-team counts, distinct present/eligible memberships,
pending and excluded flags, the original force flag and full winning datum.
Life exhaustion uses the recovered respawn-admission query. All 9,801 complete
query/wrapper comparisons pass across retail, TU1.1 and TU1.5; independent
execution reproduces all fixture/report bytes and 66 reached-owner listings.
Nonfinal notification owners execute against literal Lockout matg/mulg backing.
That supplied cache binding does not establish original tag loading or live HUD.

The native check runs after the clock owner and before world updates. Real
FFA/team SMG cases end on the next engine pass after opposing lives exhaust,
award/finalize wins and totals, replicate results over actual UDP and open the
font-backed scoreboard automatically. All-exhausted cases retain NONE winners.
Ordinary Team Slayer also ends for the remaining team before a later deadline;
departure at the actual time deadline still follows the earlier score/clock
owner. Complete round transitions/reset and related HUD messages remain gated.

All 35 rebuilt focused regressions and scoped ASan/UBSan/leak checks pass.
The fresh 78-action audit still admits 2/39 variants and blocks 37. The fresh
production four-local-player Team Slayer command reaches 300 ticks with
`finished=1`; those are not 300 active combat ticks or a finite-life frontend
session. Scope, exact commands and pins are retained in
`analysis/multiplayer/game-variants/round-end-verification.json`.
No complete finite-life match, physical LAN or Xbox parity claim follows.

## Retained identities, PvP and event remapping

The shared systems library now implements the complete original retained
identity remapper and machine binder against the full`DE78`-byte context.
It matches players by all12 identity bytes, preserves complete`A4` property
and437-word stat records, remaps both PvP axes and the two participant bytes
in every1,000 event records, and retains all unclassified bytes. Earlier
retail/TU1.1 owners prune unreferenced machine records; U5 retains them.
Recording-frozen and result-complete are separate source flags.

The independent oracle executes1,224 whole original calls across three
unchanged images, including duplicate identities, holes, full machine tables,
signed high-water inputs and repeated transitions. SHA256 comparisons cover
every byte of each complete input/output context. Nine complete source pairs
and20 listings are independently reproduced. The fully instrumented core
passes ASan/UBSan/leak checks, and19 rebuilt focused regressions pass.
See [the source/data contract and boundaries](MP_RESULT_RETENTION_RESEARCH.md);
current pins are in
`analysis/multiplayer/game-variants/result-retention-verification.json`.

The raw live-slot native bank is still awaiting real source identity,
144-byte configuration and6-byte machine-roster inputs before this remapper
can be bound to actual participant callbacks. Capture/ranking, valid/complete
publication, further stat/event producers, PCR/navigation and session return
remain unfinished. The fresh78-action menu audit remains2/39 startup-admitted
and37 blocked; no variant, weapon or edited rule gained admission.

## Weapon, spree and lifetime result capture

The admitted native FFA/Team Slayer match now binds the original credited-kill,
victim-death and lifetime producers to the retained stat bank. Seven complete
weapon descriptors, the compiled cause dispatch and the4.0-second multikill
operand are imported from the selected retail/TU1.1/U5 executable. Shared
writers retain signed widths/wrap, original bounds, bit15 and recording gates.
The missing reporting cause in the fatal-input adapter remains explicit and
does not publish a guessed weapon index.

Source ordering places time expiry before lifetime sampling, then objects and
the later clock increment. Native capture follows that order: death clears
running streaks, original stock respawn binding resumes lifetime, best spree
and weapon totals persist, and end-of-world finalization freezes the bank.
Two ongoing real SMG/loopback-UDP kill/death/respawn timelines and the two
existing winning-hit timelines verify those producers in FFA and Team Slayer.
Controlled projectile geometry/vitality are declared inputs.

The independent oracle checks6,063 original cases across three unchanged
images, including108 cutoff-owner prefixes that stop before unrecovered
dependencies. Complete retained and match banks agree with native outputs.
Fourteen complete source pairs and27 instruction listings are independently
reproduced. Three-image imports,18 rebuilt focused regressions, complete-core
and bounded-native ASan/UBSan/leak checks pass; native linked archive objects
outside the changed multiplayer/capture/stat/end sources remain uninstrumented.
The rebuilt production four-local-player Team Slayer command advances300 ticks
on actual Lockout content. Evidence and reproduction boundaries are in
[MP_RESULT_CAPTURE_RESEARCH.md](MP_RESULT_CAPTURE_RESEARCH.md); current pins
are in `analysis/multiplayer/game-variants/result-capture-verification.json`.

The fresh78-action strict menu audit still admits2/39 rows and blocks37.
Retained identities/properties and valid/complete publication, additional
weapon/damage/headshot/PvP/medal/event producers, peer retained-bank transport,
PCR construction/navigation and session/frontend return remain required. No
mode, weapon or edited rule gained admission; the goal stays active.

## Retained result-stat publication

The admitted match now uses the complete original player/team stat writers
for verified kills, assists, deaths, suicides and win awards. These populate
the signed match bank and the separate original`4040`-byte retained-stat bank,
using all45 actual descriptors imported from the selected retail/TU1.1/U5
executable. Unclassified descriptor tails and bank fields are preserved.
Original lower/upper bounds, bit15 preservation,32-bit additive wrap and
recording/frozen gates replace the former independent counter increments.

The final winning hit publishes its victim-death stat before the end-of-world
finalizer freezes recording. Original win publication also reaches the team
and matching member records. Two real production-SMG/loopback-UDP winning
timelines check that ordering; the six existing local/UDP ending cases and
four postgame SMG fatal timelines verify that frozen retained records remain
unchanged throughout continuing world updates.

The independent oracle executes10,996 whole original writer/award cases
without instruction patches or supplied game/platform returns. Native goldens
compare every retained-bank byte and complete match-bank records. Eight full
cross-image writer pairs and the actual source caller listings preserve the
data and call contracts. Three-image descriptor import,16 rebuilt focused
regressions and ASan/UBSan/leak checks pass. The native sanitizer instruments
the changed multiplayer/stat/end sources; other archive objects remain
uninstrumented. Reproduction and file pins are in
`analysis/multiplayer/game-variants/result-statistics-verification.json`.

The strict catalogue remains2/39 admitted and37 blocked after78 normal menu
starts. This raw stat view still uses the native live-slot adapter: original
retained identity remapping, participant names/ranks/scores, additional
weapon/PvP/medal/event producers, source completion publication, peer-side
retained-bank transport, PCR construction/navigation and session/frontend
return remain open. No mode, weapon or editable setting gained admission.
See [the complete evidence and boundaries](MP_RESULT_STATISTICS_RESEARCH.md).
The later capture work above binds the admitted SMG weapon/spree/lifetime
producers; this section's16-check manifest records its earlier source snapshot.

## Session phase handoff and the authored Postgame Carnage Report

[Source research](MP_SESSION_GAME_OVER_RESEARCH.md) now recovers the complete
quiet session game-over owners in all three original images. Native policy
checking makes150254 comparisons over288 phase writes,3012 whole-owner
updates,48 repeated handoff sequences and3072 signed-mode/completion inputs.
Twenty-two complete source pairs and51 original instruction listings are
pinned and independently repeated. No source instructions or dependency
returns are supplied. Source session records, role bytes, distinct identities
and the frozen millisecond clock are explicit API fixtures.

The unchanged owner writes phase5 only after active game-over/countdown0.
The following update queues request4 through the actual source request owner.
Its request consumer and state-object exit/entry callbacks remain necessary.
The later reason6 path constructs screen16, which the original disc's wgit
tag identifies as the six-pane Postgame Carnage Report. The pregame lobby is
the separate screen14. A direct frontend lobby reopen would skip that authored
report and its result/navigation producers.

The reusable native phase/request policy preserves original widths, wrapping
counters, record changed bytes, once-only phase transitions and the separate
millisecond clock. Unrecovered identity-matched map, campaign-end and closing
state3/4 services remain explicit failures. The module builds into the systems
library but is not bound to a live native session or frontend return yet.
Six focused ending/countdown/import/frontend checks and the fully instrumented
policy/countdown/class slice pass. Current source, fixture, original image,
instruction/report and validation hashes are recorded in
`analysis/multiplayer/game-variants/session-game-over-verification.json`.
A fresh strict catalogue audit runs78 normal menu starts and records2/39
startup admissions and37 blocked rows in
`analysis/multiplayer/game-variants/runtime-audit-session-game-over.json`.
The complete ordinary Slayer/Team Slayer lifecycle and all other variant families,
general weapons, original tag contracts and editable rules remain required.

## Continuing postgame world, countdown and sound classes

[oracle_game_over.py](../tools/oracle_game_over.py) executes complete unchanged
retail/TU1.1/TU1.5 game-over start, countdown, completion query and audio-class
request/advance owners. All three images register their actual engine tables.
There are48 starts,1320 countdown boundary ticks,675 sequential countdown/audio
ticks and72 interrupted-envelope cases. Fourteen complete cross-version owner
pairs compare the complete211-instruction owner set in each image; the setter's class-table end pointer
is recorded explicitly as a relocated literal. Static world admission,
pause, player-action, respawn and engine-clock evidence includes nine complete
TU1.1/u5 pairs. This does **not** execute a complete original world tick: its
external world/object/session dependencies are not supplied as successful calls.

Game, clock, selected engine and all54 saved class records are declared API
inputs. No original instructions or game/platform returns are replaced. Each
original class record's fourth word remains unchanged. Native comparisons cover
the target/current/remaining binary32 words and the actual54-name table, without
assigning semantics to that fourth word. Native checking makes120150
comparisons against all three original fixtures. The selected executable's7.0-second start,
2.0-second threshold and0.2 linear ambient gain are imported through
`OriginalData`; the fade's1.0-second duration is an original immediate operand.

The complete13AAC0/139750 world owner has no game-over early return. Its admitted
phase body runs player input, engine/object updates, finalization, countdown,
audio and the later world-clock increment. Original160780/15F3A0 separately
rejects inactive-engine respawns without decrementing their timers. Original
15E870 stops writing the displayed engine clock after status1, while the world
clock continues. Finalization clears the statistic-bank validity byte; the
already recovered original death-stat reader then omits attribution, stat and
respawn callbacks, while history, incoming unit counters and current ownership
cleanup continue. The native admitted ordinary Slayer path now retains these
separate gates instead of returning from the whole match step.

Game-over starts with210 ticks at the native30-Hz clock. The ending world pass
immediately decrements it to209. The fade begins on the150th total delivery,
when countdown reaches60; completion is exactly game-over with countdown0.
Zero and negative countdown inputs, absent engines, repeated start requests,
already-started fades and binary32 threshold-adjacent inputs are replayed. An
absent engine suppresses the audio requests but still sets the fade-start byte.

Complete18BAC0/188CD0 first fades every nonempty named class to-64dB over one
second. It then overwrites the `ambient_nature`, `ambient_machinery` and
`ambient_computers` substring targets with the original0.2 linear gain converted
to dB. The54-slot source table has no `ambient_computers` match. Empty slots are
not selected. Existing envelopes preserve their current level when interrupted;
their advancement uses audio elapsed wall time separately from simulation
ticks. The native SDL mixer consumes these class gains for both existing and
new voices. Native UDP protocol14 publishes countdown/fade state; peers issue
the same recovered request once when the authority's flag arrives, advancing
their local audio envelopes with wall time. This is native transport behavior,
not a claim about the Xbox System Link wire codec or synchronized hardware audio.
The multiplayer ambient-loop producer remains a separate owner; this change
does not establish audible map ambience from those class targets alone.

The six local/loopback-LAN ending cases now continue225 postgame ticks each.
They compare actual host/peer countdown and final banks, verify movement/world
clock continuation, once-only fades, frozen displayed clocks and respawn timers,
and ensure postgame deaths detach current ownership without changing results.
Four cases fire an actual production SMG projectile after finalization using
explicitly positioned/vitality-adjusted bodies and disabled encounter collision;
the other two retain Lockout geometry and use a declared fatal API stimulus.
The actual decoded SMG weapon-fire sound also passes through the SDL mixer with
the live match envelope. Deterministic mixing does not prove physical audio.

All27 focused checks pass, including selected-image operand imports. ASAN,
UBSAN and leak detection pass for the complete countdown/class core and the
native match/network/end/scoreboard/HUD slice; remaining cache/systems archive
objects are uninstrumented. The oracle's43 instruction listings contain3305
rows and have verified hashes. Actual FFA and Team four-view GPU probes each
submit120 frames at1280×720 on the RTX5080, reach world tick356, and retain
automatic results on all views without F1. They observe time expiry at tick31,
fade start at180 and countdown completion at240. Their80ms submission pacing
ensures elapsed time for the whole countdown. Both exact-RGB PNGs and source
captures are retained in `analysis/multiplayer/game-variants/game-over-*-four`;
the current code/binary/log/capture hashes and validation limits are in
`analysis/multiplayer/game-variants/game-over-verification.json`.

The strict catalogue audit still runs78 normal Split Screen/System Link starts,
admits2 of39 authored rows and reports37 blocked rows. Complete native session
teardown, source simulation-event delivery and return to the lobby remain open;
a zero countdown is exposed as a completion condition, not a fabricated lobby
transition. General weapons, objective engines and full editable-rule flows
remain required by the active goal.

```sh
.tools/python/bin/python tools/oracle_game_over.py
cmake --build build --target game_over_tests game_over_data_tests game_end_native_tests multiplayer_scoreboard_runtime_tests halo2_native -j2
ctest --test-dir build -R '^(original_game_over|original_game_over_import|native_game_end)$' --output-on-failure
SDL_VIDEODRIVER=x11 build/multiplayer_scoreboard_runtime_tests assets/disc/maps/lockout.map assets/disc/maps assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03 --split4 --wide --team --end
```
