# MCC-only native gameplay goal checklist

Updated: 2026-10-28. This checklist owns the remaining scope; detailed evidence
stays in [MCC_GAMEPLAY_CONVERSION.md](MCC_GAMEPLAY_CONVERSION.md).
Check a box only after its stated acceptance test passes. Component proofs do
not complete an integration, gameplay, visual or audio acceptance gate.

Goal status: **active**. Keep this checklist current at each verified milestone
and before a handoff. Preserve unchecked work across sessions; the goal is
complete only when all required acceptance and delivery gates below pass.

**Required goal rule, part of the active objective:** Whenever a task or
acceptance gate is completed and verified, immediately update its checkbox and
supporting evidence in this checklist before starting the next task or handing
off. Record changed blockers and next steps at the same time. Leave unverified
items unchecked.
This requirement applies to work completed by the primary agent and subagents.
Saving that checklist update is a required part of completing each task; do
not defer it until the end of a session. For subagent work, the primary agent
records the verified result here before starting dependent work.

## October28 particle resource crash correction

Runtime content imports in this slice are MCC/H2EK. Rendering uses the custom
faithful Vulkan kernels; imported PC pass metadata qualifies their interfaces.

- [x] Reproduce partial publication through an actual MCC multi-system effect,
  then verify whole-effect rollback, strict retry, idempotence, cached content
  rejection and admission of unrelated effects. Device/system exceptions and
  missing native pools still propagate. The catalog prepares immutable visual
  resources without admitting source systems or effects.
  [Focused regression](../build/test-results/particle-admission-20261028/transaction-green/report.json).
- [x] Complete the resident sprite/model and authored effect dependency inventory
  across all40 prepared MCC maps, including cooked template-default bitmaps and
  model child systems:7,677 resident resources,30,138 owners and65,538 systems;
  zero resource, dependency, metadata or map-initialization failures. The107
  scene-feedback material occurrences still require actual frame/GPU owners.
  [Current-source CPU inventory](../build/test-results/particle-admission-20261028/all-map-resource-inventory-current/inventory.json).
- [x] Verify all60,276 WORLD/first-person preload plans across those40 maps:
  41,063 admitted,19,213 authored empty passes and zero content rejections,
  including156 breakable-surface and16 model-child passes. The40 actual running
  executable identities matched the pinned binary throughout validation.
  [Whole-plan result](../build/test-results/particle-admission-20261028/all-map-source-plans-final.json).
- [x] Check the menu catalog independently:49 resources,81 owners and99 systems,
  zero resource/dependency/metadata failures; one additional material requires
  actual scene feedback.
  [Menu CPU inventory](../build/test-results/particle-admission-20261028/mainmenu-resource-inventory/inventory.json).
- [x] Verify transactional GPU admission for7,677 gameplay-map materials plus49
  menu materials, all five vertex programs and11 gameplay fragment kernels:
  zero content rejections and Vulkan validation errors. Failure repair, inactive
  draw ordering, capability checks, bounded diagnostics and OOM retry controls
  pass. All108 scene-feedback material occurrences admit their resources;
  absent/invalid frame owners remain inactive. This does not establish their
  rendered pixels or full gameplay parity.
  [GPU catalog and controls](../build/test-results/particle-admission-20261028/all-map-gpu-admission-early/report.json).
- [x] Correct the model-depth join to accept same-winding cyclic triangle
  identities while rejecting reversal and incorrect owners. Load actual fixture
  materials only for submitted geometry. The full integrated build passes25
  checks, including258 actual model vertices and11 strict rejection controls.
  [Integrated validation](../build/test-results/particle-admission-20261028/integrated-final-v3/report.json).
- [x] Preserve complete native animation-resource owners when restoring a
  temporary presented pose. Reproduce the old split-journal cache corruption,
  then pass1,000 actual Dune resource loads and488 native evictions with512
  coherent live rows,64 restored spatial bytes and five strict owner controls.
  The corrected production main also completes four Dune frames and ten ticks
  in a private probe. Launcher qualification remains a separate gate.
  [Native regression and presentation controls](../build/test-results/particle-admission-20261028/resource-rollback-final-v2/report.json),
  [Dune runtime probe](../build/test-results/particle-admission-20261028/dune-private-rollback-green/report.json).
- [x] Guard all five compiled native model-palette entry copies against cached
  authority poses during a temporary presentation frame. The actual native
  control previously bypassed the guarded C++ route; the unchanged rejection
  control now passes alongside root/sphere restoration and palette equality.
  [Guard coverage](../build/test-results/particle-admission-20261028/presentation-native-guard-refresh.json),
  [Two passing native tests](../build/test-results/particle-admission-20261028/resource-rollback-final-v2/tests.log).
- [x] Qualify and deliver a launcher runtime containing the corrected loading,
  material admission, pose-resource ownership and runtime failure handling.
  Both ROOT launchers select `4bf0c86b3c41485193b431cf4f7da756`:15 launcher/loading
  checks,13 Original-mode map/player cases and four frontend checks pass. The
  tested frozen cohort preserves the prior selected Path Tracing additions;
  unrelated concurrent working-tree edits remain outside this qualification.
  No exact grenade/Warthog gameplay replay or complete visual parity is claimed.
  [Published runtime](../build/test-results/particle-admission-20261028/launcher-final-v3/report.json),
  [Launcher verification](../build/test-results/particle-admission-20261028/launcher-info-verification.json),
  [Acceptance and cleanup](../build/test-results/particle-admission-20261028/acceptance.json).

## Working rules

- Reuse the working development systems and ABI-compatible compiled objects.
  Compile actual compatibility changes and their affected consumers only; avoid
  broad engine/generated-module rebuilds. The selected old Main allocates a
  17,032-byte Simulation. The refreshed Main and current consumers agree on
  17,072 bytes, including current scene and presentation fields. Verify the
  actual allocator and complete support-object layouts before linking.
- The selected delivery's Memory layout predates the current page-index
  optimization. Do not mix its factory/consumers with the larger current Memory
  implementation. Native VM publication must use a layout-preserving method
  extension or refresh every affected consumer coherently; the first mixed
  diagnostic candidate failed in scratch and was never installed.
- Preserve Xbox TU1.5 gameplay, visuals, timing, precision and existing shaders.
- Keep serialized handles/tokens at their required widths; use real 64-bit
  host pointers and ownership for native resources.
- Treat converted gameplay definitions as normal vanilla internal data.
- MCC imports are the sole runtime asset source, including gameplay, audio,
  ONLINE/clan UI, fonts, prompts and movies. Xbox ISO/XBE/TU1.5, Vista banks and
  MCPX material are offline references, never onboarding/runtime dependencies.
- The user's October21 reaffirmation supersedes the earlier supplied intro/
  credits, authored prompt-atlas and original loading-tone exceptions. Preserve
  those files and dated evidence as references; normal MCC setup, reuse,
  packaging and runtime selection must not require, copy or select those banks.
- Use authenticated native MCC prompt glyphs with their actual metrics and
  geometry, and the MCC V3 loading bank. Unassigned MCC movie roles and other
  missing asset owners remain open delivery gates; do not substitute unrelated
  artwork/media or infer full provenance from a cache path or component pass.
- Reconstruct Xbox hardware or startup initializers only for demonstrated
  dependencies of the reused gameplay path. Preserve unrelated dirty work.
- Deliver each qualified new Linux x86_64 build to both
  `/home/eric/Developer/Halo-2-Decompiled/` and
  `/mnt/SSD4TB/Halo-2-Decompiled/`, including each designated
  `dist/builds/linux-x86_64` output and requested portable ZIP/checksum. Verify
  the SSD launcher's selected runtime after installation. Preserve the active
  development checkout's separately owned runtime selection and keep incomplete
  experimental candidates out of the validated delivery.

## Current execution slice: native gameplay and 64-bit ownership (October21)

Latest bounded progress: the selected Cache diagnostic attributes the material
LOD error to Scarab beam fire's constant distortion particle; named SMG's camera
guard is independent. The unchanged material provider passes complete emitted
equivalence, and the existing guarded constant sampler projection now passes
actual selected-Cache component comparison. Genuine MCC biped/player construction and same/fresh-World
handoff now pass the completed bounded ownership witness with five published
host addresses above4GiB. Offline continuation accounts for all66473 open rows
and matches691 original post-open input hashes, without another native execution.
The relative-map owner helper, paired producer/resolver, isolated publisher and
snapshot-library tests pass. Current production Cache translation units also
compile with matching constructor/private-call symbols and unchanged current
Cache size/alignment. Prepared-owner, onboarding, closure and combined-repair
checks pass. Preparation passes24/24 after a fixture-only finite-wait fix;
publisher passes12/12 after correcting its stale expected error wording.
The original combined RED results remain recorded. All twelve combined-repair
cases also pass on the actual SSD exFAT filesystem. The selected Cache now
passes ten actual MCC ownership cases, including legacy/indexed stock, distinct
nested owners, the resource-basename control and both native HUD exceptions.
Six complete public-data comparisons match. Direct stock resource construction
also passes six calls and four complete comparisons for both shared maps.
Broader gameplay coverage and installed-runtime adoption remain open. The selected-layout
Cache ownership projection now compiles all six units with unchanged checked
sizes/alignment and matching constructor, private-call and snapshot symbols.
Its scenario, nested-owner, HUD and direct-resource asset comparisons pass.
The coherent strict-MCC successor also builds and passes an actual owned-cache
title/menu/loading/Lockout route with sound and input. Installed launcher and
delivery adoption remain open. The strict movie-path component, coherent
fourteen-unit successor and actual normal boot without skip-intro now pass.
The focused package assembly also passes after its stale README assertion was
corrected. The first temporary launcher attempt passes admission but exposes
an incompatible newer Bomb Defusal definition in the reused Slayer-only
variant loader. An exact runtime-adjacent two-file bank now passes actual
startup/title/profile/main-menu acceptance. The successor stops at the old
Settings mouse coordinate; diagnose that input against the selected layout
before repeating. Installed adoption and both-directory delivery remain open.
The selected-compatible material
successor passes its three-unit build, actual menu/loading/Lockout frame/audio
route and both installed SSD Play launchers. SSD now selects F50, and both
designated binary outputs are updated. The runtime-only v25 ZIP passes both
extracted Play routes and is delivered with matching checksums to both roots.
Its frozen historical presentation profile does not qualify the
concurrent strict-MCC import policy.
No full native64, rendering or gameplay parity grant follows.
The earlier frozen presentation-dispatch change passed10 focused groups and
actual F49 readback under its recorded policy. The October21 MCC-only direction
now supersedes its supplied-asset exceptions. Later V3/native-font admission
changes require their own coherent runtime, fresh-import and delivered-launcher
qualification; old receipts do not grant that acceptance. Another chat has
published an independent root loading-performance selection; preserve it without
adopting it into the private C57 diagnostic. F49/v24 on the SSD and both delivered
v24 copies remain preserved historical deliveries, not proof of the new policy.

Continue from the accepted SSD F48/v23 menu/loading/Lockout route and existing
development systems. Keep shaders and required serialized widths unchanged;
the broader conversion, campaign, modes and full TU1.5 parity gates remain open.

The next CTF replay remains stopped after automatic review. Independent
import-path, native owner-capacity and graphical-source work has continued:
the SSD map-list fix is installed, the slot helper passes against both current
and selected original headers, and the unchanged shader loader's emitted
contract matches its retained provider exactly. Historical private-file hashes
remain unavailable. The changed importer and helper now pass actual selected
Cache consumption: stock output is byte-exact, and the32768-slot fixture fixes
the old unreachable-owner failure while preserving original data and fixups.
C57/F49 now passes the staged route, sound video and both installed SSD Play
launchers. Both latest binary outputs and matching v24 ZIP/checksums are
delivered to both directories; the ZIP passes archive readback and both
extracted Play routes. Preserve F48/v23 and the independent development
selector. Relative mod-bank ownership and actual particle-template
attribution remain explicit next gates. The
Uprising water material/bitmap comparisons find no justified asset replacement.

The next native CTF gate is ordinary player activation through the public World
step with real local players and machine identity. First preserve the actual
baseline result without manual unit creation, then qualify the existing source
machine-identity publisher with genuine native session inputs. Keep normal CTF
menu admission closed until activation, original spawn and subsequent match
behavior pass their separate acceptance gates.

- [x] Prepare the optional ordinary-player activation witness from the exact
  accepted scheduled-step fixture. Its three declared changes reverse to the
  original bytes. The new path uses configuration before map initialization,
  real OS machine identity, constructed team players, objective birth and local
  views, then the registered CTF update and one public World step. Snapshots
  read actual machine identities, callback table, player flags and source clock;
  they supply no manual unit, active flag, timer or callback result. Corrected
  option-field labels distinguish game kind, session mode and CTF variant mode.
  Compilation and the actual baseline result remain separate gates.
  [Activation witness source](../.local/mcc-gameplay-continuation-20261020/ctf-source-player-activation-fixture-source-checkpoint-v3.json).
- [x] Independently review and strictly compile the activation witness with the
  selected frozen ABI. All268 actual project dependencies agree: the new Main
  and267 unchanged frozen headers/private inputs. Only the required8,992,464-byte
  Main object is retained; compilation exits0 and owned scratch/cache are absent.
  No native link, activation transition or original spawn is accepted here.
  [Independent source review](../.local/mcc-gameplay-continuation-20261020/ctf-source-player-activation-fixture-independent-source-review-v3.json),
  [Activation Main compile](../.local/mcc-gameplay-continuation-20261020/ctf-source-player-activation-fixture-build-v1.json).
- [x] Execute its single new branch against C56 with all355 other link inputs
  unchanged. The registered CTF update and public World step return, and the
  same source clock advances0→1. The witness correctly fails: both real player
  rows retain flags0/current-unitNONE despite local views0/1. Their actual PC
  machine identity differs from the world's six zero bytes/invalid identity.
  CTF variant mode is0, so no24C540 branch is inferred. This is a genuine RED
  baseline, not successful activation. Protected cache/header/selection and all
  selected input hashes agree; native child, private executable and owned
  scratch/cache are removed. Next test only the source-backed input handoff.
  [Actual C56 activation baseline](../.local/mcc-gameplay-continuation-20261020/ctf-source-player-activation-c56-v1.json).
- [ ] Qualify the existing whole machine-identity publisher with actual native
  session input, then repeat the same activation witness without supplied
  outputs. Keep original spawning, unit adoption and full CTF acceptance open.
- [x] Prepare and independently review a layout-preserving native machine-input
  prototype. It supplies only17 original options-input bytes from the actual
  platform MAC, with validity last, before configuration or player allocation.
  Whole692D0 owns watcher initialization and machine publication; the native
  adapter writes no derived world identity, player flags or callback results.
  Default69FF0 behavior reverses exactly and no class/Memory layout grows.
  Selected-module lineage is known, but its full body/version and actual return
  remain runtime gates. Source preparation scratch is absent; nothing is
  compiled, installed or admitted to normal CTF by this milestone.
  [Machine-input source checkpoint](../.local/mcc-gameplay-continuation-20261020/ctf-native-local-machine-input-source-checkpoint-v1.json),
  [Independent input-prototype review](../.local/mcc-gameplay-continuation-20261020/ctf-native-local-machine-input-independent-source-review-v1.json).
- [x] Prepare the matching positive-capable activation Main with the same
  assertions and readbacks as the preserved RED baseline. It moves the unchanged
  single OS-identity inventory before configuration and calls only the new
  native input method once. Exact reversal restores the baseline Main; other
  initialization, objective/player/view ordering and the one-step assertion
  remain unchanged. The new public declaration is the only replaced header;
  compilation and runtime behavior remain separate gates.
  [Native-input witness source](../.local/mcc-gameplay-continuation-20261020/ctf-native-input-activation-fixture-source-checkpoint-v1.json).
- [x] Strictly compile the layout-preserving machine-input World against the
  selected private ABI. All374 actual project dependencies agree, with only
  source and the reviewed public declaration differing from the prior closure.
  Separate selected Memory/private-step authorities remain unchanged; they are
  not claimed as newly included dependencies. The10,520,328-byte ELF64 object
  supplies the new method; compilation
  exits0 with only inherited warnings. Required frozen inputs/object total
  12,063,544 bytes; both owned scratch/cache roots are absent. This is compile
  evidence only; no client, launcher selection or production source changes.
  [Compatible machine-input World compile](../.local/mcc-gameplay-continuation-20261020/ctf-native-local-machine-input-client-build-v1.json),
  [Independent component review](../.local/mcc-gameplay-continuation-20261020/ctf-native-local-machine-input-client-independent-review-v1.json).
- [x] Independently review and strictly compile the positive-capable Main.
  Its268 actual project dependencies consist of266 unchanged frozen headers,
  the reviewed method-only public header and the new source. The8,999,984-byte
  object is retained as the required comparison input; compilation exits0 and
  owned scratch/cache are absent. This does not accept activation. Next link
  only this Main and the compatible World against C56's354 other exact inputs,
  and run the same single ordinary activation witness.
  [Native-input Main source review](../.local/mcc-gameplay-continuation-20261020/ctf-native-input-activation-fixture-independent-source-review-v1.json),
  [Native-input Main compile](../.local/mcc-gameplay-continuation-20261020/ctf-native-input-activation-fixture-build-v1.json).
- [x] Run that private two-component composition once, preserving all354 other
  selected inputs. Actual native configuration reaches the existing whole692D0
  publisher and stops at its first real store:692E1 requires writable byte
  4DC041, which the current static-registration backing does not mount. Preserve
  that genuine failure; no player activation or publisher return is qualified.
  All protected inputs agree and the child, private executable and owned scratch
  are removed. The next demonstrated dependency is the publisher's actual
  registration-state storage, rather than a new machine query or callback body.
  [Actual native-input publisher frontier](../.local/mcc-gameplay-continuation-20261020/ctf-native-local-machine-input-v1.json).
- [ ] Supply only the demonstrated original registration-state backing through
  its existing data owner, then repeat the unchanged input-publication witness.
  Leave activation, spawning and complete CTF mode unchecked until they pass.
- [x] Independently identify and prepare precisely three cold writable output
  bytes in the existing registration-data owner:041/042 and048. Two additive
  mounts preserve all prior277 bytes and leave gaps untouched. Whole692D0
  performs the real stores; the adapter supplies no completion value. Neutral
  zero storage is justified by producer-before-consume on this route, without
  claiming an independently reauthenticated BSS image. Source scratch is absent.
  Compile the small data component with the selected declaration ABI, then
  verify actual archive-member suppression and repeat the unchanged witness.
  [Registration-data source](../.local/mcc-gameplay-continuation-20261020/ctf-registration-transition-data-source-checkpoint-v1.json),
  [Independent exact storage contract](../.local/mcc-gameplay-continuation-20261020/ctf-native-local-machine-input-registration-state-independent-review-v1.json).
- [x] Independently review the two-line data change and strictly compile it
  using22 actual project dependencies, including the selected Memory declaration
  and unchanged registration API. Compilation exits0 without warnings; the
  108,384-byte ELF64 object defines only the intended strong mounting API.
  Required object/source/header inputs total231,474 bytes. Both owned scratch
  roots and caches are absent. Actual API binding, old archive-member suppression
  and publisher/activation return still require the next private composition.
  [Exact data-source review](../.local/mcc-gameplay-continuation-20261020/ctf-registration-transition-data-independent-source-review-v1.json),
  [Registration-data compile](../.local/mcc-gameplay-continuation-20261020/ctf-registration-transition-data-build-v1.json),
  [Independent data-component review](../.local/mcc-gameplay-continuation-20261020/ctf-registration-transition-data-independent-component-review-v1.json).
- [x] Verify the actual357-input link with the new registration-data owner.
  The GNU map binds the sole mounting API to the new object, the factory refers
  to it, and the old archive member is absent from the complete map. The same
  native witness passes the previous registration-byte stores and real69FF0,
  then stops at69326: the existing watcher pointer554BD4 has no writable+4
  destination. No publisher return or player activation is accepted. All
  protected inputs agree and the private child/executable/map/member copy and
  scratch/cache are removed. Next qualify the actual watcher construction
  destination and preserve its original initialization through the whole owner.
  [Actual registration-data binding and next frontier](../.local/mcc-gameplay-continuation-20261020/ctf-registration-transition-data-v1.json).
- [x] Trace the watcher construction footprint and prepare only its proven
  writable fields. Two neutral mounts cover offsets+4..+84 and+88..+C30,
  totaling3,114 bytes. All280 existing data-owner bytes remain exact. The
  unknown first word and padding stay unmounted; distance to the next static
  owner is only a bound, not a fabricated type size. Whole source initialization
  still supplies every pointer, sentinel, MAC and status value. Source scratch
  is absent; compile and actual publisher return remain open.
  [Watcher field source](../.local/mcc-gameplay-continuation-20261020/ctf-registration-watcher-data-source-checkpoint-v2.json),
  [Independent watcher contract](../.local/mcc-gameplay-continuation-20261020/ctf-registration-watcher-independent-source-audit-v1.json).
- [x] Independently review the exact watcher mount reversal and strictly compile
  the unchanged selected22-dependency data API. The130,040-byte object exits0
  without diagnostics; required inputs total253,343 bytes. Prior data-source
  and object hashes remain exact and both owned scratch/cache roots are absent.
  No Main or World rebuild is needed. Actual watcher initialization and native
  activation still require the unchanged single-branch replay and binding gates.
  [Exact watcher data-source review](../.local/mcc-gameplay-continuation-20261020/ctf-registration-watcher-data-independent-source-review-v2.json),
  [Watcher backing compile](../.local/mcc-gameplay-continuation-20261020/ctf-registration-watcher-data-build-v2.json),
  [Independent watcher component review](../.local/mcc-gameplay-continuation-20261020/ctf-registration-watcher-data-independent-component-review-v2.json).
- [x] Pass the watcher construction stores in the unchanged357-input witness,
  with the new data owner proven by the real map and old-member exclusion.
  Whole692D0 now binds the watcher and completes86BD0, then whole86C30 reaches
  its first actual session-presence read at86C4E. Byte4DC3E8 has no backing.
  Preserve that failure; no machine publication, player activation or normal
  CTF admission is accepted. Protected inputs agree and private native/map/member
  outputs and scratch/cache are absent. Identify the byte's genuine producer
  and session-input meaning before supplying storage or a value.
  [Actual watcher initialization and next presence frontier](../.local/mcc-gameplay-continuation-20261020/ctf-registration-watcher-data-v2.json).
- [x] Identify the missing flag's existing canonical owner and lifecycle.
  Current and completed frozen network-data sources both define exactly one
  cold writable byte at4DC3E8. Real network initialization publishes1 only after
  its constructors; teardown publishes0. The selected factory never calls the
  broad network-data mount, so archive availability does not provide this byte.
  Reuse a narrow shared storage API, preserve a genuine live1, reject conflicting
  ownership and retain the original86C30 read. No full network construction,
  supplied query result or fresh original BSS checksum is claimed.
  [Independent cold-flag authority](../.local/mcc-gameplay-continuation-20261020/ctf-network-initialized-flag-independent-source-audit-v1.json).
- [ ] Implement and qualify the shared cold-presence storage owner in both
  registration and canonical network-data paths, then replay actual publication
  and activation. Keep complete network/mode/gameplay acceptance open.
  The subagent preparing the next private gameplay replay was automatically
  blocked for "possible cybersecurity risk". That replay preparation/execution
  is stopped; do not reroute or retry it through another agent. The separately
  approved small-bank ownership validation is a distinct completed task below.
- [x] Prepare and review the concrete shared presence-storage source. It mounts
  only the canonical one-byte cold owner when absent. Existing storage must
  match the exact address/extent/access/owner/page metadata and actual byte;
  a separate mutable-view check enforces guards without rewriting live state.
  The selected Memory comparison API already exists, so no layout growth or
  whole-bank snapshot is needed. Source copies route registration and the
  canonical full-data owner through this leaf and remove only the duplicated
  row; production files remain untouched. Source scratch is absent. Compilation,
  live-value/conflict/guard tests and actual native publication remain open.
  [Shared storage prototype](../.local/mcc-gameplay-continuation-20261020/ctf-network-presence-storage-source-checkpoint-v1.json),
  [Selected ownership admission API](../.local/mcc-gameplay-continuation-20261020/ctf-network-flag-selected-memory-admission-review-v1.json).
- [x] Strictly compile the shared presence leaf and registration caller with the
  selected Memory/SnapshotBytes ABI and declaration-only network header overlay.
  The139,856-byte presence object exports the exact API required by the128,104-byte
  registration object; neither references the full network mount. Both compile
  without diagnostics. Their24-project-dependency union and required objects
  total394,449 bytes; compiler and metadata scratch/cache are absent. Functional
  ownership checks and native publisher/activation acceptance remain open.
  [Shared presence component compile](../.local/mcc-gameplay-continuation-20261020/ctf-network-presence-storage-build-v1.json),
  [Independent code and component review](../.local/mcc-gameplay-continuation-20261020/ctf-network-presence-storage-independent-component-review-v1.json).
- [x] Prepare the focused ownership witness against the exact selected104-byte
  Memory and48-byte SnapshotBytes layouts. Eight cases cover adjacent cold
  storage, repeat/live1/other-byte preservation, conflicting owners and extents,
  readonly/paged/reserved storage, and guard denial with no write/topology
  notification. The witness requires and records an actual host backing pointer
  above4GiB. Whole checkpoints apply only to these tiny test banks. Source
  preparation scratch is absent; no compilation or functional pass is claimed.
  [Ownership witness source](../.local/mcc-gameplay-continuation-20261020/ctf-network-presence-ownership-probe-source-checkpoint-v1.json),
  [Independent ownership witness source review](../.local/mcc-gameplay-continuation-20261020/ctf-network-presence-ownership-probe-independent-source-review-v1.json).
- [x] Preserve the ownership validation's genuine preparation failure. Its
  extra protection preflight required an absent development-cache metadata
  file even though this small-bank test uses no game assets. No compiler,
  link or probe ran; all356 selected inputs and97 shaders remain exact and
  owned scratch is absent. Correct only that test-input assumption for one
  separately approved ownership rerun. The blocked gameplay replay remains
  stopped and is not part of this validation.
  [Ownership preflight failure](../.local/mcc-gameplay-continuation-20261020/ctf-network-presence-ownership-probe-v1.work.json).
- [x] Pass the focused selected-ABI ownership witness: all eight cases and37
  checks pass, with compile/link/run exits0 and an actual host pointer above
  4GiB. Cold0, live1 and another actual live byte remain intact; foreign,
  wider/interior, readonly, paged/reserved and guard-denied owners reject
  atomically. The real link map binds the reviewed leaf and selected Memory
  comparison API; all356 parent inputs and97 shaders remain exact. Private
  ELF/object/map/dependency files and owned scratch/cache are removed, with no
  large probe artifacts retained. No World, game or MCC assets were used;
  actual machine publication, player activation and mode acceptance remain open.
  [Focused ownership pass](../.local/mcc-gameplay-continuation-20261020/ctf-network-presence-ownership-probe-v2.json).
- [x] Integrate the storage API into the ordinary source owners. Independent
  review verifies exact prior-source reversal, preserved registration fields
  and complete CMake consumer coverage. The focused current-source CMake
  configure/build/test all exit0: both data-owner mount orders preserve the
  canonical live byte, metadata and native backing; source/native addresses
  roundtrip above4GiB. Conflicting owners, page policy and guards reject, and
  either provider rejects a foreign flag before adding unrelated owners.
  Current Memory/SnapshotBytes remain coherent and unchanged; experimental
  publisher/watcher fields are excluded. Owned build/scratch/cache are removed
  and no large test outputs are retained. This executes no World, game,
  constructor or blocked gameplay replay. The validated F48/v23 binaries and
  ZIPs remain in both directories with the SSD selection unchanged; no new
  gameplay acceptance or runtime delivery is implied by this storage change.
  [Normal source review](../.local/mcc-gameplay-continuation-20261020/normal-network-presence-production-independent-source-review-v1.json),
  [Current-source integration and cleanup](../.local/mcc-gameplay-continuation-20261020/network-presence-normal-source-integration-v1.json).

- [x] Close the reproduced native virtual-memory release/guard ownership bug.
  Retire the source bank only after its guards admit release, then unmap the
  host allocation. Rejected release now preserves both owners and permits
  genuine subsequent page commitment above4GiB; original code reproduced
  ENOMEM from the prematurely unmapped reservation. Both host virtual-memory
  and full host-resource continuation CTests pass. APIs, layouts, serialized
  widths and shaders are unchanged; private build/scratch paths are removed.
  [Release ownership proof](../.local/mcc-native64-ownership-20261020/release-v1/validation.json).
- [x] Bind virtual prepared-restore ownership to the exact staged native
  addresses and require publication before commit. Premature/foreign-bank
  commits reject without consuming the token; complete bytes, inaccessible
  holes and owner/access/commit metadata are checked. Two focused CTests pass,
  including three VM/physical/pool handoffs and genuine source-heap continuation
  above4GiB; current World syntax passes with387 pinned dependencies. Outer
  layouts, serialized widths and shaders are unchanged; owned scratch is gone.
  Coherent full-client restore delivery remains open.
  [Publication ownership proof](../.local/mcc-native64-ownership-20261020/publication-v1/validation.json).
- [x] Compose those VM publication changes with the selected World while
  preserving its complete Memory layout and original Memory provider. Add only
  the proven region-comparison method as a compatible leaf, plus the two World
  restore hunks. Actual imported MCC Lockout passes three same-World handoffs,
  one fresh-World handoff, cancelled preparation and denied/retried publication.
  Complete restored source banks/metadata match, native addresses remain above
  4GiB and genuine original CRT continuation works. Input audit has no reference
  dependency or cache writes; private scratch is removed. Full-client delivery
  and broader64-bit gameplay coverage remain separate.
  [Actual World restore](../.local/mcc-gameplay-continuation-20261020/actual-world-restore-v2.json).
- [x] Verify the combined340-provider candidate through actual MCC title,
  profile, lobby, countdown, retained loading/fade and native Lockout. Movement
  and firing input return cleanly with six rendered frames/seventeen source
  ticks, no reference inputs or cache writes, and removed scratch. All97 shader
  files remain exact. This preserves the accepted route after the compatible
  restore/loader changes; performance, broad effects and full parity remain open.
  This intermediate candidate is superseded by the compatible C48/F46 delivery
  below; full campaign-frame acceptance remains separate.
  [Combined native route](../.local/mcc-gameplay-continuation-20261020/campaign-lockout-native64.json).
- [x] Require complete authenticated stock-map conversion records before
  fresh publication and during reuse. Missing/duplicate rows, nested aliases,
  stale donor hashes and malformed records reject without publishing a cache.
  Both selected caches pass all86 bindings and eight real negative controls;
  six focused CTest groups pass, including32 onboarding/22 preparation cases.
  Additional mod maps and loose tags/data remain preserved and unqualified.
  No payload copy, selection or shader change; owned scratch is removed.
  [Coverage closure](../.local/mcc-conversion-coverage-20261020/closure.json).
- [x] Install those exact three importer owners into the SSD checkout. Its
  selected43 stock-map owners pass the installed metadata validator and a
  missing-owner control rejects. Actual launcher information still reports
  F45 available; selected cache manifests and runtime selection are unchanged.
  [Installed importer](../.local/mcc-gameplay-continuation-20261020/import-installation.json).
- [ ] Reuse the loading/renderer handoff for the existing native campaign path.
  The patched Main starts the real loading owner. Existing device/geometry/
  metadata/locale/material/model fixes reach an Armory frame with clean exit;
  tick0 is blank and is not visible gameplay acceptance. Advance the real
  startup scripts, closing only demonstrated readonly/dispatch dependencies.
  Compose the tag-root SIMD alignment correction with these owners and the
  layout-preserving VM group before final client/frame/launcher qualification.
  The unused intermediate C45 raw link is retired; its cohort/hash is retained.
- [x] Repair the reproduced campaign device-definition ABI mismatch by
  compiling the unchanged loader against the selected frozen headers. Exact
  DWARF shows old/new object extents2288/2336 and collision offsets1072/1096;
  the old producer supplied0x37 where the consumer required a model pointer.
  The one-leaf successor loads197 actual MCC objects without that SIGSEGV and
  reaches the next explicit geometry boundary. No gameplay logic or shaders
  changed; owned diagnostic scratch is removed. Native campaign frames remain
  a separate acceptance gate.
  [Device loader proof](../.local/mcc-gameplay-continuation-20261020/device-crash-v1/probe.json).
- [x] Compose the existing transparent-only class2 model fix with that loader.
  The failing real call is class2/opaque0 with three model nodes and274 vertices;
  the selected old adapter omitted the existing max(opaque,transparent) dispatch.
  Only that previously completed source hunk changes. Actual Armory advances
  to explicit tag-payload ownership validation; full campaign frame acceptance
  remains open. Both required leaf objects/dependencies are pinned and private
  scratch is removed.
  [Geometry dispatch proof](../.local/mcc-gameplay-continuation-20261020/device-crash-v1/geometry.json).
- [x] Qualify Armory's three missing native HUD material instances from the
  already preserved native libraries: widget_gradient_flash_pulse, plasma_meter
  and camo_meter. Their original same-name parameters/curves match across nine
  source comparisons. The scoped closure retains all29 global identities and
  23,998 existing records across39 libraries; only Armory gains three instances.
  Fresh MCC-only onboarding passes; no pass, vertex program or shader changes.
  [Material closure](../.local/mcc-gameplay-continuation-20261020/device-crash-v1/closure-green.json),
  [Fresh onboarding](../.local/mcc-gameplay-continuation-20261020/device-crash-v1/armory-onboarding.json).
- [x] Prove the resulting Armory sound-class publication context without
  weakening existing guards. Globals move exactly2272 bytes from the three
  aligned material allocations; the complete6480-byte class bank, contained
  sound route and all48 typed deltas remain unchanged. Original39 contexts are
  retained and six unproved class rows stay untouched. Actual runtime acceptance
  of the combined new data owners remains open.
  [Contained class context](../.local/mcc-gameplay-continuation-20261020/device-crash-v1/class-context-proof.json).
- [x] Reuse the completed MCC model node-map projection at its actual resource
  and delivery owners, preserving the selected resource pimpl and public ABI.
  Actual Cairo data passes906 model maps, five complete112-byte original hinge
  headers and23,676,834 byte/ownership/control checks. Original MCC allocations
  and rendering instructions stay unchanged. Armory no longer reads count2 as
  pointer0x2; it reaches the next finite model-render readonly input. This is
  data-owner acceptance only; the campaign frame remains open.
  [Node-map component](../.local/mcc-gameplay-continuation-20261020/device-crash-v1/node-map-frame.json).
- [x] Compose the existing finite model-render readonly profile with the
  selected compatible factory. All2459 prior cold owners remain byte/access/
  ownership exact;11 authenticated readonly owners add184 bytes. The pinned
  original image independently matches every range, while41F064 and the prior
  effect fade table stay exact. Actual Armory reaches1464A9's next missing
  original operand46A3F4; campaign frames remain unaccepted. Shaders and source
  instructions are unchanged and runtime input/cache-write audit is clean.
  [Finite model inputs](../.local/mcc-gameplay-continuation-20261020/device-crash-v1/model-literals-frame.json).
- [x] Remove demonstrated whole-bank snapshot IO from bitmap free-range lookup
  through a layout-preserving Memory method and the existing selected bitmap
  backing. All5600 independent first-fit occupancy cases pass, including reserved
  owners, policy denials, alignment and32-bit-end boundaries. Querying a real
  PROT_NONE2MiB host lease above4GiB reads no payload; ownership/heap/checkpoint
  controls pass and strace records zero fsync/fdatasync. Actual Armory advances
  through60 frames/118 ticks; its image stays blank and mission_01a reports
  the next explicit AI dispatch dependency209D04. Source game behavior
  and all shaders remain unchanged; final visible/client acceptance stays open.
  [Topology query and actual route](../.local/mcc-gameplay-continuation-20261020/device-crash-v1/gap-query-frame.json).
- [x] Reuse the existing typed AI behavior-descriptor owner at the demonstrated
  campaign dependency. All115 records/4632 immutable interface bytes and920
  authored mutable cache bytes match the pinned source. The71 actual source
  mask constructors execute213 instructions; mutable masks are never prefilled.
  Every2498 prior cold owner remains exact. Actual Armory now creates actor1 at
  tick10 with zero script faults, then reaches the absent original47C240 actor
  context. Owned scratch is removed; visible campaign/client acceptance stays
  open.
  [AI descriptor component](../.local/mcc-gameplay-continuation-20261020/device-crash-v1/ai-descriptor-frame.json).
- [x] Restore the demonstrated original active-actor context47C240 as its
  authenticated four-byte writableFFFFFFFF initial value. Actual source stores
  publish and clear the actor token; no live producer result is prefilled. All
  2772 prior cold owners remain exact. Armory clears1EB3DE with actor1/tick10/
  zero script faults and reaches the next generation operand47B90C. Compiler/
  runtime scratch is removed. Pin factoryv25 for the combined client while later
  campaign data dependencies continue separately; visible gameplay stays open.
  [Actor context proof](../.local/mcc-gameplay-continuation-20261020/device-crash-v1/actor-context-frame.json).
- [x] Restore the two demonstrated original behavior-generation words at
  47B908/47B90C, initiallyFFFFFFFF, using authenticated source load/increment/
  store instructions. All2773 prior cold owners remain exact; only eight
  writable bytes are added. Actual Armory reaches the next bounded lookup
  2700A1→4568C4 with actor1/tick10/zero script faults. Owned scratch is removed;
  this later factory is separate from C46's frozen v25 input and does not
  qualify a visible campaign frame.
  [Generation-owner proof](../.local/mcc-gameplay-continuation-20261020/device-crash-v1/behavior-generations-frame.json).
- [x] Restore only the source-proved thirteen-entry contact-priority lookup
  at4568C4. The ordinary actor producer constrains its index to0..12; independent
  source/opcode evidence authenticates the exact26 readonly bytes. All2774 prior
  cold owners remain exact. The real reader passes all13 values and two invalid
  index controls in the existing regression test. Armory advances to47D6C4 with
  actor1/tick10/zero script faults; this is a later component, separate from
  C46/C47's pinned factory. Private scratch is removed; visible campaign stays
  open.
  [Bounded lookup](../.local/mcc-gameplay-continuation-20261020/device-crash-v1/contact-priority-frame.json),
  [Existing regression](../.local/mcc-gameplay-continuation-20261020/device-crash-v1/contact-priority-regression.json).
- [x] Add the missing complete28-byte272200 actor-contact selector as a
  distinct native owner, reusing existing267410 through its real ABI. All24
  rounding/path cases and three missing-bank guards preserve registers, FP/SIMD,
  original CALL/PUSH footprint and partial failure state. The private C43 World
  with only its two dispatch hunks advances actual Armory to56AE84; this private
  route does not certify the C48 restore/charge receiver. A separate compatible
  final-World leaf preserves the full CTF/charge/VM reverse chain and old Memory
  layout, with11,974,728 required bytes retained and scratch removed. Production
  World/CMake integration is wired; the focused production CMake test passes
  with the actual existing script-builtin dependency and its private build is
  removed. No visible campaign frame or final selector delivery is claimed.
  [Selector component](../.local/mcc-gameplay-continuation-20261020/device-crash-v1/selector-owner.json),
  [Actual private campaign route](../.local/mcc-gameplay-continuation-20261020/device-crash-v1/selector-armory-frame.json),
  [Compatible World build](../.local/mcc-gameplay-continuation-20261020/selector-client-build-v1.json),
  [Focused CMake acceptance](../build/test-results/mcc-ai-actor-selector-v2-20261020/report.json).
- [x] Independently qualify the selector receiver against the actual accepted
  C48 World before composing C49. Reversing only its include and three-line
  dispatch restores the exact C48 source. Selected Memory/Contact/World/
  Simulation layouts, VM publication, compiled-symbol lease, Damage observation,
  store-listener and CTF paths remain exact. The qualified leaf/header and two
  caller/provider APIs match. This is static integration evidence; actual
  combined native acceptance and visible campaign gameplay remain open.
  [C49 selector audit](../.local/mcc-gameplay-continuation-20261020/c49-selector-static-audit-v1.json).
- [x] Compose the four previously verified v26–v29 campaign data owners with
  the accepted charge/animation factory. All2792 prior owners remain byte/
  access/metadata exact; four regions add only46 bytes,16 writable and30 readonly.
  Removing the four includes restores accepted v2 source exactly. The complete
  selected Memory layout and strict FP compilation remain unchanged; independent
  source/receipt review passes. The required factory and156-file frozen closure
  total5,182,811 bytes; supervised compiler/constructor scratch is removed.
  No new AI workspace, original executable reads or game runs are part of this
  component. Combined client/campaign acceptance remains separate.
  [V3 factory composition](../.local/mcc-gameplay-continuation-20261020/charge-factory-build-v3.json).
- [x] Compile the existing real CTF birth/carrier/drop/one-tick return fixture
  against the selected complete old Memory headers. Only its previously used
  return-path insertion and capture-removal transforms are applied; gameplay
  state, timers and results are never supplied. Required Main/source/dependency
  inputs total8,254,669 bytes,276 files; scratch is removed. The old cleanup
  capture branch is unreachable after the fixture's existing return. This is
  source/ABI preparation only; no native tick, return or scoring is accepted.
  [Compatible CTF fixture](../.local/mcc-gameplay-continuation-20261020/ctf-return-fixture-build-v1.json).
- [x] Link the qualified selector World/leaf and v3 factory into C49 using
  C48's exact remaining346 physical inputs. The resulting349-input cohort
  contains both selector APIs exactly once and extracts none of the named stale
  archive providers. Publication follows all input-map assertions; failed
  staging is owned and disposable. Native SHA256
  `dec741d26f43913003f848c48e11992f2bae8a00e56808346989a9118bf0710e`.
  Link scratch is removed. This retains a candidate only; installed F46/v21,
  caches and shaders are preserved pending actual combined acceptance.
  [C49 composition](../.local/mcc-gameplay-continuation-20261020/selector-successor-build-v1.json).
- [x] Verify C49's actual MCC64-bit restore and strict charge lifecycle after
  selector/factory composition. Complete2831-region source-bank bytes and
  metadata agree through three same-World and one fresh-World handoffs, cancelled
  preparation and denied/retried publication. Native addresses remain above
  4GiB and genuine continuation works. The unchanged240-tick charge fixture
  again passes real positive-effect allocation, damage/player response,
  retirement, charged projectile,0.15000000596 battery and both contact effects.
  Cache/reference audits are clean. Both tests are repeated with disk-backed
  supervised scratch and isolated snapshot storage; those private trees are
  removed. Earlier passing component receipts are preserved.
  [C49 contained World restore](../.local/mcc-gameplay-continuation-20261020/actual-world-restore-c49-contained-v2.json),
  [C49 contained charge lifecycle](../.local/mcc-gameplay-continuation-20261020/combined-charge-c49-contained-v2.json).
- [x] Verify C49 through actual MCC title/profile/lobby/countdown/loading,
  native Lockout rendering, movement, firing and clean exit. The first GPU
  invocation used tmpfs scratch, which snapshot backing correctly rejected;
  the unchanged binary passes when rerun in the repository's disk-backed owned
  scratch. No production change or storage fallback was introduced. The
  reference-input/cache-write audit is clean and scratch is removed. This
  muted successor route preserves prior audio evidence without broadening
  campaign or full-gameplay acceptance; F46/v21 remain selected deliverables.
  [Actual C49 native route](../.local/mcc-gameplay-continuation-20261020/campaign-lockout-c49-disk-v1.json),
  [Preserved test-environment failure](../.local/mcc-gameplay-continuation-20261020/campaign-lockout-c49-native64.json).
- [ ] Complete the existing CTF state fixture with C49-compatible actual static
  providers. Native map initialization passes, but configure_session reaches
  the missing source-owned48BFA0 backing at159FF0→15A0EA before objective birth.
  This is the first executed combined-world CTF dependency after the earlier
  header-only preflight failure. No gameplay results or timers are supplied;
  return/scoring/mode admission remain open. Scratch is removed and F46 stays
  selected. Reuse the previously qualified CTF static owner rather than changing
  the source rules or fallback values.
  [Actual C49 CTF boundary](../.local/mcc-gameplay-continuation-20261020/ctf-return-c49-v1.json).
- [x] Recover the exact completed CTF/player-result static group from the
  prior real development fixture. Its51-callback CTF interface and45-row result
  metadata retain prior source/header hashes. Earlier native player-join
  evidence independently demonstrates the result-table dependency at47A38A;
  the current CTF failure demonstrates48BFA0. The proposed two include/mount
  hunks reverse exactly to selected v3. Exclude generic branch/contrail imports:
  their necessity is unproved, and an old generic41CD08 table conflicts with
  the selected qualified model table. Preserve both authorities without
  replacing either. Constructor/client qualification follows separately.
  [Existing-provider provenance](../.local/mcc-gameplay-continuation-20261020/ctf-static-existing-reuse-audit-v1.json),
  [Selected overlap audit](../.local/mcc-gameplay-continuation-20261020/ctf-static-selected-overlap-audit-v1.json).
- [x] Compile the unchanged CTF/player-result static providers and compose
  their two existing factory include/mount hunks against the selected old
  Memory ABI. Actual cold comparison preserves all2796 prior complete owner
  fields and adds only41 regions/1537 bytes,728 writable and809 readonly. Existing
  ownership guards pass69 CTF and274 result cases. Both independent reviews
  pass; the required factory/two leaves and162-file frozen closure total
  6,116,679 bytes. Compiler, constructor and isolated disk snapshot scratch are
  removed. No game run or mode admission occurs at this source component gate.
  [Qualified static factory group](../.local/mcc-gameplay-continuation-20261020/ctf-static-factory-build-v1.json).
- [x] Compose the verified static group into C50, preserving all348 other
  C49 physical inputs and adding its two qualified leaves. The351-input native
  candidate has SHA256
  `202f504a094b5175b0b569000c64bfd185c346409eacf73fdc0face99dd01d15`.
  Exact input-map and link-map checks precede atomic publication; link scratch
  is removed. Actual CTF/restore/charge/native-render acceptance follows;
  installed F46 and the requested v21 archives stay preserved.
  [C50 static composition](../.local/mcc-gameplay-continuation-20261020/ctf-static-successor-build-v1.json).
- [x] Verify C50's complete2872-region MCC World restore and unchanged
  240-tick charge lifecycle after adding CTF/player-result metadata. The same/
  fresh-World and denied/retried/cancelled publication controls, native pointers
  above4GiB, genuine continuation and charge state/damage/contact assertions
  pass. Disk-backed owned scratch and input/cache-write audits are clean.
  [C50 restore](../.local/mcc-gameplay-continuation-20261020/actual-world-restore-c50-v1.json),
  [C50 charge](../.local/mcc-gameplay-continuation-20261020/combined-charge-c50-v1.json).
- [x] Verify C50's actual MCC title/profile/lobby/countdown/loading-to-native
  Lockout route with movement, firing, rendered frames and clean exit. Input/
  cache-write audits pass and the disk-backed owned scratch is removed. This
  proves the working Slayer path survives the CTF static-provider composition;
  objective modes and campaign remain separate gates. All97 existing shaders
  are unchanged and selected F46/v21 deliverables remain protected.
  [C50 rendered native route](../.local/mcc-gameplay-continuation-20261020/campaign-lockout-c50-disk-v1.json).
- [ ] Compose the existing completed CTF configuration dispatch with the
  selected World. C50 clears the absent48BFA0 data owner, then reaches the
  already-implemented24B390 predicate at159FF0→15A43B. Its current production
  World include/branch and three-entry generated owner were not in the frozen
  selected receiver. Reuse them without changing the predicate, caller ABI,
  timers or results. The genuine CTF fixture remains red before objective birth;
  scratch is removed and installed F46 is protected.
  [C50 executed CTF boundary](../.local/mcc-gameplay-continuation-20261020/ctf-return-c50-v1.json).
- [x] Independently authenticate the existing three-entry configuration
  provider and its two selected-World dispatch hunks. Exact source/header bytes
  match the prior real CTF fixture and864 optimized/864 sanitizer cases. The
  older576-case evidence covers only its preceding two-entry version. Preserve
  full upper EAX, partial AL writes, RET4 and existing164930 activation helper;
  no Boolean shortcut is introduced. Reversing only the include and branch
  restores the exact selector World; VM/charge/CTF/selector paths remain intact.
  Six other prior-only dispatch presences are recorded without speculative
  additions. This is source/ABI evidence, pending combined native execution.
  [Configuration reuse audit](../.local/mcc-gameplay-continuation-20261020/ctf-configuration-existing-reuse-audit-v1.json).
- [x] Compile the selected-compatible configuration World/leaf with exact
  frozen layouts and two-hunk source reversal. The unchanged three-entry
  provider passes all864 existing native unit cases with the already-selected
  activation helper; public caller/provider signatures match. Four missing
  private headers and one declaration are reused exactly from accepted frozen
  inputs. Both independent reviews pass. Required objects and380-file closure
  total12,404,644 bytes; successful and three failed header-attempt scratch/
  cache trees are removed. No game run, frontend admission or full-client
  acceptance is claimed by this compilation checkpoint.
  [Compatible configuration pair](../.local/mcc-gameplay-continuation-20261020/ctf-configuration-client-build-v1.json).
- [x] Compose that pair into C51, preserving C50's other350 physical inputs.
  The352-input candidate has SHA256
  `5620f7c842c13d1e1ba3804067e7a46d7e6158b069b0dfab82089250bbb049f7`.
  Exact input/link-map checks and atomic publication pass; factory proof stays
  attached to the unchanged v4 factory while the separate provider proof records
  the new World/configuration pair. Link scratch is removed. Actual CTF/native
  acceptance follows; selected F46/v21 remain unchanged.
  [C51 configuration composition](../.local/mcc-gameplay-continuation-20261020/ctf-configuration-successor-build-v1.json).
- [x] Verify C51 through actual MCC loading/native Lockout rendering, movement,
  firing and clean exit, plus complete2872-region native64 World restore and
  the unchanged240-tick charge lifecycle. All three acceptance paths pass;
  owned disk/snapshot scratch is removed and cache/reference audits are clean.
  This preserves the working gameplay route after configuration dispatch;
  objective modes and campaign remain separate gates.
  [C51 native route](../.local/mcc-gameplay-continuation-20261020/campaign-lockout-c51-disk-v1.json),
  [C51 restore](../.local/mcc-gameplay-continuation-20261020/actual-world-restore-c51-v1.json),
  [C51 charge](../.local/mcc-gameplay-continuation-20261020/combined-charge-c51-v1.json).
- [ ] Complete the genuine CTF fixture beyond objective construction. C51 now
  completes configure_session and constructs/joins two actual players; original
  roster code derives both teams. Whole15AFB0 reaches the previously diagnosed
  cloth literal46A8B8 at11A215 before objective birth. Reuse its existing exact
  four-byte readonly authority without importing the conflicting whole generic
  branch table. Source results, timers and carrier handles remain unsupplied;
  return/scoring/full mode stay open. Scratch is removed; F46 remains selected.
  [C51 actual CTF frontier](../.local/mcc-gameplay-continuation-20261020/ctf-return-c51-v1.json).
- [x] Retire the two superseded, unpublished root-owned C49/C50 raw binaries
  and completed selector-link helper after C51's native route/restore/charge
  acceptance:214,590,300 bytes removed. All associated game/capture handles are
  terminal; no visible same-user process or selected runtime references them.
  Restricted unrelated service visibility is recorded explicitly; this is
  bounded task-ownership retirement, not an unconditional machine-wide claim.
  Keep both parent cohorts, every frozen object/source, C51, selected/rollback
  runtimes and requested archives. Protected hashes agree and cleanup scratch
  is removed. Earlier uncertain C44/C46/C47 candidates remain untouched.
  [Owned intermediate cleanup](../.local/mcc-gameplay-continuation-20261020/c49-c50-retirement-v1.json).
- [x] Authenticate the already-completed cloth threshold as exactly4 readonly
  bytes,0B D7 23 3E at46A8B8. The prior missing-backing checkpoint, byte export,
  preserved literal_131 and passing real CTF birth/drop sources agree. Actual
  11A215 reads4 bytes; no SIMD widening or guessed floating-point value is used.
  The neighboring46A8C0 owner remains separate. A narrow selected-factory
  composition and actual CTF rerun follow; the conflicting generic provider
  remains excluded.
  [Exact existing cloth literal](../.local/mcc-gameplay-continuation-20261020/ctf-cloth-existing-literal-audit-v1.json).
- [x] Compose that exact existing literal as one narrow factory-v5 include,
  retaining the selected old Memory ABI and all2837 prior complete cold owner
  fields. Only one4-byte readonly region is added; neighboring46A8C0 remains
  exact. Source reversal, retained authority and both independent reviews pass.
  Required factory/frozen159-file closure total5,195,904 bytes; constructor/
  compiler/cache scratch and the initial source-path diagnostic attempt are
  removed. No broad conflicting provider or new gameplay behavior is imported;
  final CTF/client execution follows separately.
  [Narrow cloth factory](../.local/mcc-gameplay-continuation-20261020/ctf-cloth-factory-build-v1.json).
- [x] Compose the narrow factory into C52, retaining C51's other351 physical
  inputs and its complete World/configuration/selector/CTF/charge/VM group.
  The352-input candidate is published only after exact input-map/link-map and
  output-hash checks; link scratch is removed. Actual CTF/client acceptance
  follows. Selected F46 and both requested v21 archives remain preserved.
  [C52 cloth composition](../.local/mcc-gameplay-continuation-20261020/ctf-cloth-successor-build-v1.json).
- [x] Verify C52 through the actual MCC menu/countdown/loading/native Lockout
  route, movement, firing and clean exit. The same candidate also passes complete
  2873-region bank restoration across three same-World and one fresh-World
  handoffs, cancellation/retry and genuine continuation with native addresses
  above4GiB, plus the unchanged240-tick charge lifecycle and exact battery use.
  No reference-file access or imported-cache write is observed; owned disk
  scratch and snapshot caches are removed. This is bounded candidate acceptance;
  F46/v21 remain the delivered build and full campaign/CTF/parity remain open.
  [C52 native route](../.local/mcc-gameplay-continuation-20261020/campaign-lockout-c52-disk-v1.json),
  [C52 restore](../.local/mcc-gameplay-continuation-20261020/actual-world-restore-c52-v1.json),
  [C52 charge](../.local/mcc-gameplay-continuation-20261020/combined-charge-c52-v1.json).
- [x] Verify genuine C52 CTF configuration, two players/teams, two existing
  flags, both real biped/equipment constructors, whole existing-flag pickup
  and complete inventory-clear/drop with converted MCC data. All native identity,
  carrier/registry/inventory and authored reset-timer/tick-base assertions pass.
  No cloned objective, supplied carrier/result or contact/button shortcut is
  used. Owned scratch is removed. The subsequent full24A070 tick is still red
  at24DAEC→46A9C0; natural return, capture/scoring, rendering/networking and full
  mode admission remain open. Preserve this positive lifecycle evidence without
  relabeling the whole fixture as passed.
  [Integrated CTF birth/carrier/drop](../.local/mcc-gameplay-continuation-20261020/ctf-c52-birth-carrier-drop-milestone-v1.json),
  [Complete failing-tick receipt](../.local/mcc-gameplay-continuation-20261020/ctf-return-c52-v1.json).
- [x] Resolve the reached46A9C0 data authority without guessing a fixture value.
  The bounded retained-evidence search had no data export for this cell. An
  offline read through the existing XBE parser authenticates exactly4 readonly,
  fully file-backed bytes0B D7 23 3D in the pinned TU1.5 image; its whole-file hash
  agrees before/after and the exact24DAEC instruction confirms the4-byte read.
  Only the small data/section/offset/hash receipt is retained; no original code
  is executed and no runtime reference dependency is added. Scratch is removed.
  Narrow factory composition and genuine return-tick execution follow separately.
  [Bounded search](../.local/mcc-gameplay-continuation-20261020/ctf-return-literal-existing-search-v1.json),
  [Exact offline operand](../.local/mcc-gameplay-continuation-20261020/ctf-return-literal-authentication-v1.json).
- [x] Compose the authenticated return operand through one narrow factory-v6
  include, preserving all2838 prior complete cold-owner fields and the old
  selected Memory layout. The only added region is4 readonly bytes at46A9C0;
  source reversal, exact byte/metadata comparisons and independent authority/
  factory reviews pass. Required160-file closure totals5,210,048 bytes. Owned
  compiler/constructor/snapshot scratch is removed. No algorithm or shader
  changes are made; combined client and CTF tick acceptance follow separately.
  [Narrow return factory](../.local/mcc-gameplay-continuation-20261020/ctf-return-literal-factory-build-v1.json),
  [Independent data review](../.local/mcc-gameplay-continuation-20261020/ctf-return-literal-review-v1.json).
- [x] Carry the authenticated scalar into the normal production source through
  the existing strict immutable-owner helper. Only its provenance comment and
  one46A9C0/3D23D70B call are added; reversing those lines restores every byte
  of the prior dirty factory. Existing conflicting generic-provider work is
  preserved separately. A syntax check uses actual production flags/current
  headers and154 pinned project dependencies; independent review passes and
  owned scratch/cache are removed. No full rebuild or broader runtime parity
  is claimed by this source-only check.
  [Production scalar and syntax check](../.local/mcc-gameplay-continuation-20261020/ctf-return-literal-production-v1.json).
- [x] Compose C53 with only the qualified v5→v6 factory replacement. All351
  other inputs, complete World/configuration/CTF/charge/VM providers and shader
  metadata remain exact. The352-input candidate passes complete input/link-map
  verification before atomic publication; link scratch is removed. Genuine
  CTF and final client acceptance follow; no launcher selection changes yet.
  [C53 return composition](../.local/mcc-gameplay-continuation-20261020/ctf-return-literal-successor-build-v1.json).
- [x] Verify C53's complete2874-region source-bank restoration, native
  addresses above4GiB, three same-World/one fresh-World handoffs, cancellation,
  denied/retried publication and genuine continuation. Its unchanged240-tick
  charge lifecycle also passes all strict allocation/damage/response/retirement/
  projectile/battery assertions. Both input audits show no reference dependency
  or cache writes; owned scratch/snapshot caches are removed. Final client
  rendering and delivery remain separate.
  [C53 restore](../.local/mcc-gameplay-continuation-20261020/actual-world-restore-c53-v1.json),
  [C53 charge](../.local/mcc-gameplay-continuation-20261020/combined-charge-c53-v1.json).
- [x] Resolve C53's reached24D689→1A8F10 dependency through the qualified
  iterator and compatible C54 integration below. Preserve C53's failing receipt
  as historical evidence; C54 advances to the separate FCE50 dependency while
  genuine birth/real units/existing-flag pickup/drop still pass. The complete
  tick remains unchecked below; do not admit CTF or claim natural return,
  capture/scoring, contact/button pickup, rendering/networking or full parity.
  [C53 actual CTF frontier](../.local/mcc-gameplay-continuation-20261020/ctf-return-c53-v1.json).
- [x] Classify1A8F10 as a missing complete native helper rather than a missing
  dispatch to an existing implementation. The current/frozen CTF bodies,
  retained definition census and generation maps contain only external-call/
  address identities. Typed round-reset/player-span helpers do not establish
  its full caller ABI, and the old1A2C60 correspondence is ambiguous. Keep
  those implementations intact; no alias or successful substitute is supplied.
  Narrow offline body authentication/lowering is a separate next task.
  [Missing-owner audit](../.local/mcc-gameplay-continuation-20261020/ctf-1a8f10-existing-owner-audit-v1.json).
- [x] Authenticate the complete reached1A8F10 helper through the pinned offline
  TU1.5 reference:192 contiguous bytes/77 instructions, one return, no indirect
  calls, jump tables or fixed globals. Record the complete iterator-field
  accesses, widths and caller ABI. Its sole direct callee16EA60 remains an
  explicit required external dependency; the selected provider's identity and
  complete body are checked separately from mutable-source membership. No original
  code is executed, no reference is added to runtime, and owned authentication
  scratch is removed. Direct C++ lowering/build/integration are separate gates.
  [Complete helper authority](../.local/mcc-gameplay-continuation-20261020/ctf-player-iterator-body-authority-v1.json).
- [x] Lower that complete77-site helper through the unchanged existing native
  C++ generator into a separate26,677-byte leaf and639-byte ABI header. The
  sole16EA60 call remains an explicit real native dependency; no result or
  iterator state is supplied to bypass it. All authenticated instruction sites
  are represented, generator scratch is removed, and the original41-owner CTF
  source remains byte-identical. Required source/authority inputs total89,339
  bytes. Component compilation, independent review and World integration remain
  separate next gates.
  [One-body source checkpoint](../.local/mcc-gameplay-continuation-20261020/ctf-player-iterator-source-checkpoint-v1.json).
- [x] Compile the isolated helper as an x86_64 leaf using the selected frozen
  contact/services ABI and strict floating-point flags. The438,304-byte object
  exports only its two intended public entry/ownership APIs;26 project and300
  system compiler dependencies are pinned. Required object/source/header closure
  totals629,094 bytes; compiler scratch is removed. No substituted16EA60 test
  result, original execution or gameplay success is claimed. Independent
  component review and compatible World integration remain separate gates.
  [Isolated leaf build](../.local/mcc-gameplay-continuation-20261020/ctf-player-iterator-leaf-build-v1.json).
- [x] Independently review the complete77-site helper and selected ABI before
  World integration. Exact instruction/site order, full-EAX Boolean, saved
  registers, ten4-byte iterator store sites, real16EA60 call/return forwarding
  and exceptional-state reload agree with retained authority. All29 retained
  files and26 compiler project dependencies match frozen origins; old Memory
  layout remains intact and generation/compiler scratch is absent. V2 corrects
  V1's prose store-site count without changing source or acceptance. This is
  component review only; actual CTF execution follows compatible World glue.
  [Independent iterator review V2](../.local/mcc-gameplay-continuation-20261020/ctf-player-iterator-independent-review-v2.json).
- [x] Build the selected-compatible World with only the iterator include and
  three-line dispatch branch, reusing the already-qualified leaf object. Exact
  reversal preserves the complete previous World/VM/charge/configuration/selector
  chain and old Memory/private layouts. Strict compilation, public symbol
  coupling and independent377-file closure review pass; owned scratch/cache
  are removed. Required World/frozen inputs total11,739,259 bytes. The selected
  Phantom→vehicle_entries16EA60 fallback binding is proven, while actual callee execution
  remains a fail-closed runtime gate. No normal CTF admission is changed.
  [Compatible iterator World](../.local/mcc-gameplay-continuation-20261020/ctf-player-iterator-client-build-v1.json),
  [Independent World review](../.local/mcc-gameplay-continuation-20261020/ctf-player-iterator-client-independent-review-v1.json).
- [x] Compose C54 by replacing only C53's World object and adding the qualified
  iterator leaf. All351 other physical inputs, factory-v6 data, shader metadata
  and existing providers remain exact. The353-input candidate passes complete
  input/link-map checks before publication; scratch is removed. F47/v22 and
  both frontend selections remain preserved. Actual CTF update acceptance follows.
  [C54 iterator composition](../.local/mcc-gameplay-continuation-20261020/ctf-player-iterator-successor-build-v1.json).
- [x] Re-run the unchanged actual MCC fixture against C54. Real objective
  construction, two real player/unit/equipment paths, existing-flag pickup and
  complete drop still pass. The ordinary update advances beyond the previous
  1A8F10/16EA60 dependency to165648→FCE50. This is bounded native integration
  progress; the whole tick remains red. Protected cache/selection/cohort hashes
  agree and scratch is removed. No successful callback result or timer is supplied.
  [C54 actual CTF frontier](../.local/mcc-gameplay-continuation-20261020/ctf-return-c54-v1.json).
- [x] Carry the qualified iterator into the ordinary source build: exact
  generated source/header, the same World include/three-line dispatch and the
  existing CTF strict-floating-point CMake source list. Independent reversal
  checks preserve both prior dirty files and coherent current headers. The
  focused Ninja object compile and current-World syntax check exit0; the runner
  exits8 solely because the explicit'^$' filter selects no CTests and the runner
  requires tests. This is compile-only evidence, not a test-suite/runtime pass.
  An unrelated concurrent frontend edit is preserved and disclosed. All private
  CMake/syntax/cache outputs are removed; delivered F47/v22 remain untouched.
  [Normal source integration](../.local/mcc-gameplay-continuation-20261020/ctf-player-iterator-production-v1.json),
  [Independent source review](../.local/mcc-gameplay-continuation-20261020/ctf-player-iterator-production-independent-review-v1.json).
- [ ] Qualify natural return/capture/scoring and normal team activation/spawn
  integration after C55's completed ordinary CTF update. CTF admission,
  contacts/buttons, rendering,
  networking and full parity remain open. The earlier Phantom binding is only
  a fallback-table proof; pre-table ownership predicates take precedence.
  [Dispatch precedence clarification](../.local/mcc-gameplay-continuation-20261020/ctf-player-iterator-selected-callee-fallback-clarification-v1.json).
- [x] Classify the reached165648→FCE50 call through current production, selected
  frozen dispatch, known completed generation maps and preserved caller authority.
  No reusable complete full-ABI owner is present in that bounded scope; the
  generation maps contain only an external address identity. Actual C54 routing
  also supplies no owner for this call. Authenticate the complete callee before
  lowering it; no typed alias, successful substitute or original execution is
  supplied. This audit changes neither mode admission nor runtime selection.
  [FCE50 existing-owner audit](../.local/mcc-gameplay-continuation-20261020/ctf-fce50-existing-owner-audit-v1.json).
- [x] Authenticate the complete reached FCE50 helper from the pinned offline
  TU1.5 reference:101 contiguous bytes/33 instruction sites, two returns and
  no indirect transfer, table, tail or unresolved extent. Record its real
  BAD60/BBE00 external calls, conditional saved registers, low-byte XOR and
  non-normalized output registers, raw4-byte output copies and full callee
  floating-point forwarding. Existing selected source bodies establish presence
  only; actual routing remains a runtime gate. No original execution, lowering
  or production edit occurs. Required small reference inputs total24,870 bytes;
  owned authentication scratch/cache are removed.
  [Complete FCE50 body authority](../.local/mcc-gameplay-continuation-20261020/ctf-fce50-body-authority-v1.json).
- [x] Generate only the complete33-site FCE50 helper through the unchanged
  native C++ generator into a21,725-byte leaf and644-byte public ABI header.
  All source instruction identities/order match; the standard two-symbol
  rename reversal recovers the original generated hash. Preserve CL-only XOR,
  conditional saves, three raw DWORD copies, full EAX/flags and both real
  external calls. Required source/reference inputs total52,272 bytes, including
  the previously retained24,870-byte authority. Generation scratch/cache are
  removed; review, compilation and World/runtime integration remain separate.
  [FCE50 source checkpoint](../.local/mcc-gameplay-continuation-20261020/ctf-fce50-source-checkpoint-v1.json).
- [x] Independently review all33 FCE50 sites/101 code bytes, ordered hashes,
  the exact generated branch and two-symbol rename reversal. CL-only masking,
  conditional EBX/unconditional ESI saves, three raw DWORD copies, full output
  registers, actual callee stack cleanup, final flags/returns and complete
  floating-point ABI agree with authority. Both external callees remain real
  required owners; this review supplies no selected-availability or runtime
  proof. Selected ABI inputs/generator/41-owner source remain exact and source
  generation scratch/cache are absent. Compilation/integration remain separate.
  [Independent FCE50 source review](../.local/mcc-gameplay-continuation-20261020/ctf-fce50-independent-source-review-v1.json).
- [x] Compile the exact reviewed FCE50 leaf using the frozen selected services
  ABI and strict floating-point recipe. Its377,376-byte x86_64 object exposes
  exactly the two intended APIs;26 actual project dependencies remain exact,
  300 system dependency hashes are recorded, and old Memory remains a separate
  unchanged layout authority. The29-file required object/source closure totals
  563,597 bytes. Compiler scratch/cache are removed; no executable, World, mode
  admission or runtime acceptance is supplied by this component build.
  [FCE50 leaf compile](../.local/mcc-gameplay-continuation-20261020/ctf-fce50-leaf-build-v1.json).
- [x] Locate normal CTF's remaining integration seam: the existing match runs
  engine update before World::step_objects, which owns one source-clock advance.
  The current CTF fixture performs one direct update without advancing time;
  normal match admission/update and the FFA player-activation adapter still
  lack the required team-mode path. Original divider/outer return timers and
  proximity gates are documented without forcing expiry. No new match behavior
  or completed return/capture fixture is claimed.
  [Normal tick seam and plan](../.local/mcc-gameplay-continuation-20261020/ctf-normal-match-tick-seam-audit-v1.json),
  [Existing workflow boundary](../.local/mcc-gameplay-continuation-20261020/ctf-existing-natural-return-workflow-audit-v1.json).
- [x] Trace normal team activation through the existing join, roster publish,
  allocation and original constructor; no new team constructor is needed.
  Preserve the observed constructor flags0 and distinguish the Vista flags8
  mirror policy. Record real activation's vtable offset+0x18→entry6→249600,
  and spawn offset+0x84→entry33→24A6F0. Current-source absence is bounded;
  selected-provider availability and the actual imported CTF variant tuple
  remain unqualified. No guards, callbacks, source flags or layouts are changed.
  [Team/activation seam audit V2](../.local/mcc-gameplay-continuation-20261020/ctf-normal-team-activation-seam-audit-v2.json).
- [x] Check selected providers before authenticating the required normal CTF
  activation249600 and spawn24A6F0 callbacks. No reusable complete owner is
  found in the selected353-input closure. The pinned offline reference supplies
  both complete contiguous bodies:48/167 and80/284 instructions/bytes. Record
  all128 instruction sites, exact returns/register/stack/write widths and the
  four real external callees. Preserve spawn's altered saved-ECX high byte,
  caller argument overlap and path-derived upper EAX; do not replace AL with a
  full-register Boolean. No original execution or production change occurs;
  all census/authentication scratch is removed. Lowering, compilation, actual
  activation/spawn and complete normal CTF admission remain separate gates.
  [Activation/spawn body authority](../.local/mcc-gameplay-continuation-20261020/ctf-activation-spawn-body-authority-v1.json).
- [x] Generate only those two complete activation/spawn bodies through the
  unchanged existing native C++ generator:32,925-byte source and714-byte ABI
  header, all128 authenticated instruction sites. Reversing the standard two
  public-symbol renames recovers the exact original generated hash. Preserve
  the saved-ECX/caller-stack/partial-EAX behavior and all five real call sites
  to four external owners. The existing41-owner CTF source remains unchanged;
  generation scratch is removed. Required source/metadata inputs total56,743
  bytes. Independent review, compilation, World wiring and normal mode acceptance
  remain separate gates.
  [Activation/spawn source checkpoint](../.local/mcc-gameplay-continuation-20261020/ctf-activation-spawn-source-checkpoint-v1.json).
- [x] Independently review all128 activation/spawn instruction sites and451
  code bytes against the retained body authority and exact paired input union.
  Ordered hashes, branches, generated fragments, two-symbol rename reversal,
  selected ABI and partial-register/stack overlap agree. All five calls still
  forward the real complete ABI to their four external targets; source review
  does not establish selected callee availability. Generator/41-owner source
  are unchanged and generation scratch is absent. No new compile/runtime or
  normal mode acceptance is claimed by this review.
  [Independent activation/spawn source review](../.local/mcc-gameplay-continuation-20261020/ctf-activation-spawn-independent-source-review-v1.json).
- [x] Compile the exact two-body activation/spawn leaf using the selected
  frozen services/contact ABI and strict floating-point recipe. The698,120-byte
  x86_64 object exports exactly its two intended public APIs;26 actual project
  dependencies and explicit old Memory authority are preserved, while300 system
  dependency hashes are recorded without copying them. Required object/frozen
  input closure totals895,795 bytes. Compiler scratch/cache are removed; no
  archive, executable, raw bank, mode admission or runtime test is supplied.
  The four real external callees remain required and unqualified.
  [Activation/spawn leaf compile](../.local/mcc-gameplay-continuation-20261020/ctf-activation-spawn-leaf-build-v1.json).
- [x] Prepare a separate optional scheduled-step witness from the immutable
  actual MCC CTF fixture. Exactly two hunks reverse to its original19,404-byte
  source; default five schemas, real-input protections and one CTF update stay
  unchanged. Only explicit`--scheduled-step` adds one existing public World step
  after that returned update, with read-only clock/player/unit snapshots. No
  timer, flag, bank, callback result or extra input command is supplied. Required
  source/patch/reversal inputs total31,018 bytes and preparation scratch is gone.
  This is source preparation; compilation and actual scheduled-step acceptance,
  normal player activation/spawning and full mode qualification remain separate.
  [Scheduled-step source checkpoint](../.local/mcc-gameplay-continuation-20261020/ctf-scheduled-step-fixture-source-checkpoint-v1.json).
- [x] Independently review the optional scheduled-step fixture's exact reversal,
  original schema order and unchanged ordinary CTF call. Its only new mutation
  is the real public scheduler call; clock/player/unit queries remain read-only
  and all broader qualification flags stay false. Selected World/Memory/pool
  API headers match C54. Review scratch is absent; no compilation, game run or
  original material is used. Actual scheduled-step acceptance remains open.
  [Independent scheduled-step source review](../.local/mcc-gameplay-continuation-20261020/ctf-scheduled-step-fixture-independent-source-review-v1.json).
- [x] Compile that exact optional witness as a replacement test Main using the
  immutable fixture's strict recipe and frozen ABI. One strong public Main is
  supplied, with no new game-owner provider. All268 actual project dependencies
  remain byte-identical;607 system dependency hashes are recorded. Required
  object/source closure totals8,921,830 bytes. Compiler scratch is removed and
  no executable/archive/raw bank is retained. No link or runtime pass is claimed;
  default-path regression and actual scheduled-step execution follow separately.
  [Scheduled-step Main compile](../.local/mcc-gameplay-continuation-20261020/ctf-scheduled-step-fixture-build-v1.json).
- [x] Execute the new Main against C55 without changing other354 selected
  providers. Its default invocation passes all five original schemas and the
  complete CTF update. The optional scheduled invocation also passes those
  phases, then reaches the genuine B9180→D6FF4→1190D0/returnD6FFA missing-owner
  boundary in the public World step. Preserve the actual failing receipt;
  no scheduled schema, clock advance or activation policy is qualified. All
  protected inputs agree; both native children, link executable, private phase
  data/caches and scratch are removed. No fix, retry or supplied result occurs.
  [Actual C55 scheduled-step frontier](../.local/mcc-gameplay-continuation-20261020/ctf-scheduled-step-c55-v1.json).
- [ ] After the qualified1190D0 provider refresh and single scheduled step,
  qualify normal activation/spawn, sustained clock progression, natural return
  and full CTF mode through the genuine existing lifecycle.
- [x] Classify1190D0 as selected cloth-provider version lag. C55 already has
  the correct public cloth include and dispatch branch. Its sole strong API
  provider is the older six-owner archive member; retained qualified seven-owner
  source includes the complete destructor and matches current source exactly.
  Prior body/retirement/strict-compile authority is preserved, but its temporary
  object is absent. Refresh the unchanged module with C55's exact frozen headers
  and verify API binding/member extraction; add no World branch or regenerated
  algorithm. Audit scratch/extracted member are removed. This is source/provider
  diagnosis, not a successful scheduled tick.
  [Cloth provider reuse audit](../.local/mcc-gameplay-continuation-20261020/ctf-cloth-1190d0-existing-owner-audit-v1.json).
- [x] Independently qualify the completed seven-owner cloth source and API
  for reuse. The four-site destructor matches preserved authority and forwards
  real16E8A0/RET4. All six prior closed functions match after identity-address
  normalization; the public wrapper differs only by the added entry. The
  prior compiled object was correctly temporary and is absent. Preserve C55's
  exact private step header and existing observer declaration when compiling;
  use no prior removed object or changed private helper. No new source
  generation, original execution, World branch or gameplay pass is supplied.
  [Completed cloth source/API review](../.local/mcc-gameplay-continuation-20261020/ctf-cloth-1190d0-completed-source-review-v1.json).
- [x] Compile that unchanged qualified seven-owner cloth module with C55's
  exact selected headers/private step and existing observer declaration. All26
  actual project dependencies match those authorities;300 system hashes are
  recorded and Memory remains a separate unchanged layout authority. The
  1,657,088-byte x86_64 object exports exactly the prior two public API names.
  Required object/frozen inputs total1,923,843 bytes. Strict compilation exits0
  and scratch is removed. No World/CMake/production change, regeneration or
  runtime pass is supplied; selected API-provider composition follows separately.
  [Qualified cloth provider compile](../.local/mcc-gameplay-continuation-20261020/ctf-cloth-provider-build-v1.json).
- [x] Independently review the refreshed cloth object and all29 retained files.
  Source,26 actual project dependencies,300 system hashes, C55's exact step/API
  inputs and the existing observer declaration match their authorities. Exactly
  the same two strong public API signatures satisfy the current World references;
  no private layout growth or current-header substitution occurs. Strict compile
  and scratch/cache cleanup are verified. Selected API binding and actual
  scheduled-step success still require C56 composition and execution.
  [Independent cloth component review](../.local/mcc-gameplay-continuation-20261020/ctf-cloth-provider-independent-review-v1.json).
- [x] Prepare only the C56 checker adapter while preserving the original C55
  checker and sealer. Its twelve identity/count/pin substitutions reverse to
  the exact original source; the same Main, two invocation order, genuine
  scheduler call, six schemas, protected inputs and failure-stop remain intact.
  Final C56 cohort/executable hashes are mandatory inputs rather than guessed
  values. AST syntax/preparation cleanup pass. No executable, native run or
  new scheduled-step acceptance is supplied by this source preparation.
  [C56 checker source adapter](../.local/mcc-gameplay-continuation-20261020/ctf-scheduled-step-c56-adapter-v1.json).
- [x] Prepare and independently review the C56 cloth-provider composition.
  The corrected helper preserves all355 parent inputs, World/factory lineage
  and the exact recursive97-file shader inventory, adding only the qualified
  seven-owner cloth object before the unchanged archive. Both public API
  bindings and absence of the old archive member must pass the real link map.
  A malformed hash literal and incomplete shader enumeration were corrected
  before execution; V1 review is superseded by the sealed V2 review. All nine
  literal hashes match actual inputs. Preparation scratch is removed; no C56
  link or runtime acceptance is claimed by this source milestone.
  [C56 composition plan](../.local/mcc-gameplay-continuation-20261020/ctf-cloth-provider-link-plan-v1.json),
  [Independent corrected-helper review](../.local/mcc-gameplay-continuation-20261020/ctf-cloth-provider-link-helper-independent-review-v2.json).
- [x] Compose C56 using the completed cloth module. All355 parent inputs and
  World/factory lineage remain exact; only the qualified cloth object is added.
  Both public APIs resolve to that object in the real GNU link map; the old
  archive member is absent from the entire map. The356-input link exits0 and
  owned scratch, map and extracted member are removed. The required raw runtime
  is107,343,936 bytes with646,230-byte cohort; no gameplay acceptance or launcher
  selection change is supplied by composition alone.
  [Sealed C56 composition](../.local/mcc-gameplay-continuation-20261020/ctf-cloth-provider-client-build-v1.json).
- [x] Pass C56's unchanged default five-schema CTF witness, followed by exactly
  one public scheduled World step. The genuine step returns and the same source
  clock advances0→1; both real player rows retain flags0 and their constructed
  units/bodies. The previously demonstrated1190D0 dependency is resolved with
  the existing completed module. All protected inputs agree; both invocations,
  link helper and owned scratch/cache are gone, with no large test artifact
  retained. No additional command, timer/flag/result patch or supplied callback
  is used. Normal player activation/spawn, natural return, capture/scoring and
  full CTF admission remain open.
  [Actual C56 scheduled step](../.local/mcc-gameplay-continuation-20261020/ctf-scheduled-step-c56-v1.final.json).
- [x] Identify the genuine next activation/return precondition from existing
  source and actual C56 observations. The real team/player constructors already
  run; normal CTF still needs source-owned249600 activation/readback and the
  real24A6F0 spawn predicate, preserving constructor flags0. The dropped flag's
  observed away+nearby state correctly suspends its countdown;900 ticks is only
  conditional arithmetic, not a proved return deadline. No speculative repeat
  loop, flag/position patch or duplicate spawn path is added. Actual legitimate
  activation/input/physics movement and natural return remain unchecked.
  [Normal activation seam](../.local/mcc-gameplay-continuation-20261020/ctf-normal-activation-bridge-current-source-audit-v1.json),
  [Natural-return precondition](../.local/mcc-gameplay-continuation-20261020/ctf-natural-return-missing-precondition-v1.json).
- [x] Pass the actual unmuted staged F48 title/profile/lobby/countdown/loading
  and native Lockout route with movement/fire and clean exit. Captured PCM is
  finite and nonzero and the original four-voice loading tone starts/stops at
  the renderer handoff. Only the authenticated readonly shader-directory field
  is relocated; all97 shader files remain exact. Both selections are preserved
  and owned scratch/cache are removed. F48 is retained as the requested working
  candidate; this does not accept whole-game graphics, audio or complete modes.
  [Actual staged F48 route](../.local/mcc-gameplay-continuation-20261020/f48-native-proof-v1.json).
- [x] Install F48 on the SSD and click both actual normal/title Tk Play buttons.
  Both routes reach title/profile/lobby/countdown/loading/native Lockout with
  real input, finite nonzero PCM and clean exit. Both execute the exact staged
  F48 binary; its97 shaders and prepared MCC owners pass admission. Previous
  F47/runtime/selection rollback is retained, the development selector is
  preserved, and owned scratch/cache/children are removed. Historical C53
  restore/charge evidence is explicitly inherited, not a fresh C56 test.
  [Both installed F48 Play routes](../.local/mcc-gameplay-continuation-20261020/f48-installation-v1.json).
- [x] Publish the qualified C56 raw binary and relocated F48 binary into the
  development and SSD `dist/builds/linux-x86_64` folders using the existing
  atomic build-output publisher. Both output files match their source hashes,
  manifests/capability are updated, both selectors are preserved and owned
  scratch is removed. Other platforms remain skipped as requested.
  [Both current Linux build outputs](../.local/mcc-gameplay-continuation-20261020/f48-build-output-v1.json).
- [x] Assemble the compiled v23 Linux ZIP from qualified C56 and the existing
  portable launcher pair. It includes both bundled movies, all91 authored
  prompt source files, approved loading tone,97 unchanged shaders and bundled
  Python/libraries, without the private MCC cache. The2,724,381,317-byte archive
  is retained as requested; packaging stage and owned scratch are removed.
  Actual extracted-route validation and SSD delivery are separate next gates.
  [v23 package receipt](../.local/mcc-gameplay-continuation-20261020/portable-v23-package.json).
- [x] Validate every checksummed payload in the actual extracted v23 ZIP and
  both normal/title Tk Play routes into loading/native Lockout with input,
  finite nonzero PCM, bundled Python execution and clean exit. Full admission
  uses a private COW clone reconciled to the selected SSD MCC cache; neither
  donor is modified. Both portable launchers execute the qualified packaged
  binary with all97 shaders. The extracted bundle, private cache, phase homes,
  audio and all owned scratch/cache are removed after success.
  [Actual extracted v23 routes](../.local/mcc-gameplay-continuation-20261020/portable-v23-validation.json).
- [x] Deliver the validated v23 ZIP and checksum to both development and SSD
  `dist` folders with atomic no-replacement publication. Both archives are
  exactly2,724,381,317 bytes and SHA256
  `8b6b5317e7ddc045221b6c77979d70f5fa83d842e9da8d2daa15ac8e62ff32ea`.
  Both sidecars agree, prior requested archives and both selectors are preserved,
  and SSD copy scratch is removed. This is local delivery; no public release
  is published and full-game parity remains open.
  [v23 dual-directory delivery](../.local/mcc-gameplay-continuation-20261020/v23-ssd-delivery-v1.json).
- [x] Retire only the two obsolete private C54/C55 raw test binaries after
  final delivery qualification and fresh process/selection/link/preservation
  checks. Removed214,678,296 logical bytes; allocated space observed beforehand
  was214,683,648 bytes, without a net free-space claim. All1,821 protected file
  hashes agree before/after; cohorts, frozen inputs, current C56/F48, working
  rollbacks, both outputs and requested archives remain. Owned scratch/cache
  are removed. Historical helper replay requires reconstructing the retired
  raw binaries from their retained cohorts; procfs visibility limits are kept.
  [Exact C54/C55 retirement](../.local/mcc-gameplay-continuation-20261020/c54-c55-raw-retirement-v1.json).
- [x] Independently recheck the final F48/v23 dual-directory delivery. Actual
  SSD selection/capability, C56/cohort/356 inputs, both complete output
  manifests, all97 shader files, both2,724,381,317-byte ZIPs and matching checksum
  sidecars agree with sealed qualification. The development p53 selector is
  preserved. Completed payload/extracted/installed route evidence is reused
  without another game run. All seven named delivery/composition scratch roots
  and audit scratch/cache are absent; no new large artifact is retained by the
  audit. Requested binaries/shaders and both v23 ZIPs remain retained working
  deliverables; full CTF/campaign/64bit-capacity and whole-game parity gates
  remain open and the main goal stays active.
  [Independent final F48/v23 delivery audit](../.local/mcc-gameplay-continuation-20261020/f48-v23-final-delivery-review-v1.json).
- [x] Reconfirm both delivery locations after the user's reminder. Fresh
  native/output-manifest hashes match qualified C56 in the development output
  and installed-path F48 on the SSD. Both complete v23 archives are rehashed
  byte-identically, their sidecars agree, and all97 shaders in each bank match.
  The SSD launcher's read-only information route reports the selected F48
  frontend available with the prepared MCC cache. Both distinct launcher
  selections are preserved; no game is rerun, no new build is substituted and
  audit scratch is removed. The later source-only tag-slot fix is not included.
  [Current dual-directory confirmation](../.local/mcc-gameplay-continuation-20261020/native-build-both-directories-confirmation-v1.json).
- [x] Prepare the existing staging/install/output flows for C56/F48 with
  explicit sealed-composition/runtime/cohort hashes. Preserve the current
  development frontend selection and SSD F47 rollback, retain all97 shaders,
  and label earlier C53 restore/charge evidence as inherited lineage. The
  same actual unmuted staged route and both installed Tk Play routes remain
  required; source preparation installs nothing.
  [F48 delivery preparation](../.local/mcc-gameplay-continuation-20261020/f48-delivery-preparation-v1.json).
- [x] Compile one selected-compatible World that adds only the two new leaf
  includes and two three-line dispatch branches. Exact reversal preserves C54's
  complete prior source/order and old Memory/private layouts. All374 actual
  dependencies and379 retained files match; the four public symbol references
  match both already-qualified leaf objects, reused at their original paths.
  Independent review passes, strict compilation exits0 and scratch/cache are
  removed. Required World/frozen inputs total11,743,062 bytes. Final-client
  composition and actual CTF/scheduled gameplay acceptance remain separate;
  normal source, delivered F47/v22 and both selections are untouched.
  [Combined compatible World](../.local/mcc-gameplay-continuation-20261020/ctf-activation-spawn-position-client-build-v1.json),
  [Independent combined World review](../.local/mcc-gameplay-continuation-20261020/ctf-activation-spawn-position-client-independent-review-v1.json).
- [x] Carry both exact qualified leaves into the normal source build using
  only four new source/header files, the same two World includes/branches and
  existing CTF CMake source/strict-floating-point lists. Independent reversal
  preserves prior dirty World/CMake bytes. Both focused Ninja objects and
  current-World syntax compile successfully; runner8 is solely the empty CTest
  filter, with no tests executed. Current headers remain coherent and distinct
  from selected old-layout objects. Supplemental dependency-only checks and
  their corrected metadata assertion are disclosed. All known scratch/cache
  paths are removed; three concurrent unrelated edits are preserved. No full
  source-client/runtime pass or installed-build change is claimed here.
  [Normal six-file carryforward](../.local/mcc-gameplay-continuation-20261020/ctf-activation-spawn-position-production-v1.json),
  [Independent normal-source review](../.local/mcc-gameplay-continuation-20261020/ctf-activation-spawn-position-production-independent-review-v1.json).
- [x] Compose C55 from C54 by replacing only its compatible World and adding
  the qualified activation/spawn and flag-position leaves. All352 other inputs
  remain exact; the355-input composition and complete link-map/input checks
  pass. Owned link/publication scratch is removed. Factory-v6 data and shader
  metadata are preserved; F47/v22 and both launcher selections remain unchanged.
  This is composition evidence; the unchanged actual CTF fixture and scheduled
  World step must still pass before corresponding runtime claims or delivery.
  [C55 composition](../.local/mcc-gameplay-continuation-20261020/ctf-activation-spawn-position-successor-build-v1.json).
- [x] Pass the unchanged actual MCC CTF fixture with C55: real flags, two real
  player/unit/equipment paths, existing-flag pickup, complete drop and the
  complete ordinary24A070 update all return successfully. The demonstrated
  FCE50 dependency now resolves through its native owner; all five expected
  schemas pass with no supplied callback result, timer expiry or flag state.
  Protected cache/selection/cohort hashes agree and owned scratch is removed.
  This qualifies one ordinary update, not natural return, capture/scoring,
  source-player activation/spawn, full scheduled match, rendering or networking;
  normal CTF admission remains unchanged.
  [C55 actual ordinary CTF update](../.local/mcc-gameplay-continuation-20261020/ctf-return-c55-v1.json).
- [x] Verify C55's actual MCC title/profile/lobby/countdown/loading/native
  Lockout route with movement/fire input and clean exit. Eight rendered frames
  and24 source ticks are reported; cache/input audits pass and owned disk
  scratch/cache are removed. This muted GPU/input witness preserves the working
  route with the new compatible providers. Complete effects, audio, CTF match
  scheduling/capture and full campaign/parity remain open; staged unmuted
  installation and both-directory delivery are separate gates.
  [C55 native GPU/input route](../.local/mcc-gameplay-continuation-20261020/campaign-lockout-c55-disk-v1.json).
- [x] Qualify the actual imported MCC Multi Flag CTF record through the existing
  headless variant loader. Its empty authored setter tuple produces the exact
  304-byte TU1.5 variant used by the real fixture: engine1, teams/parameter91
  enabled, CTF flags2 and return time30. This compares the loader's converted
  source representation; it does not equate MCC's raw tag layout with TU1.5.
  Classic and both One Flag records retain their distinct authored overrides;
  Slayer's sily91 default0 is not imposed on CTF. Selected old-ABI/source/link
  evidence and prepared map/provenance hashes agree. Cache/selections stay
  unchanged; query object/executable/link-map/snapshot scratch are removed.
  Actual normal match activation/spawn and the other variant paths remain open.
  [Imported CTF variant qualification](../.local/mcc-gameplay-continuation-20261020/mcc-ctf-variant-data-v1.json).
- [x] Verify C53's actual MCC title/profile/lobby/countdown/loading/native
  Lockout route with movement, firing and clean exit. Rendering and input pass
  with the retained shaders; audits show no reference input or cache writes,
  and disk scratch is removed. This qualifies the previously playable route
  with the added compatible data/providers, while the explicit CTF, campaign
  and whole-game parity gates remain open. Stage/install acceptance follows.
  [C53 native route](../.local/mcc-gameplay-continuation-20261020/campaign-lockout-c53-disk-v1.json).
- [x] Stage the qualified C53 runtime as SSD F47 using only the verified
  compiled shader-directory relocation. All97 shader files remain identical.
  Actual unmuted title/profile/lobby/countdown/loading/native Lockout, movement,
  firing and clean exit pass with finite nonzero captured audio and the original
  four-voice loading ambience. Selected F46 remains unchanged during this test;
  owned pipeline/audio/snapshot scratch is removed. Required F47 runtime and
  unchanged shader bank are retained for launcher installation acceptance.
  [F47 unmuted staged route](../.local/mcc-gameplay-continuation-20261020/f47-native-proof-v1.json).
- [x] Select SSD F47 only after staging qualification, then verify both actual
  installed Tk Play buttons through imported MCC menus, original loading audio,
  native Lockout, movement/firing and clean exit. Both routes pass with finite
  nonzero audio; the title launcher also reaches gameplay. The independent
  development selection is preserved, F46 rollback remains available, and
  owned launcher/audio/pipeline/snapshot scratch is removed. All97 shaders stay
  exact; full CTF/campaign/weapon/effects/visual/audio parity remain open.
  [Both installed F47 Play routes](../.local/mcc-gameplay-continuation-20261020/f47-installation-v1.json).
- [x] Publish the latest qualified Linux x86_64 binaries into both repositories'
  designated dist/builds/linux-x86_64 folders: raw C53 in the development tree,
  shader-directory-relocated F47 on SSD. Complete source/runtime hashes and
  manifests agree; both frontend selections are preserved and the new bounded
  raw frontend sidecar passes full existing resource admission. Owned scratch
  is removed. Other platforms and public release publication remain skipped;
  portable v22 packaging/extracted-route acceptance follow separately.
  [Both latest build outputs](../.local/mcc-gameplay-continuation-20261020/f47-build-output-v1.json).
- [x] Retire only the superseded task-owned unpublished C51/C52 raw binaries
  after F47 passes both installed Play routes:214,609,536 bytes unlinked. Exact
  file/hash/ownership/link checks, terminal owned test handles, selectors,
  launcher references, current C53 dependencies and readable process metadata
  are checked immediately before retirement; restricted visibility is explicit.
  All1435 protected hashes agree afterward. Parent cohorts, every source/object,
  C53, F46/F47, shaders, requested archives and earlier uncertain candidates
  remain intact. Owned cleanup scratch/cache are removed; no unconditional
  process-visibility or physical free-space claim is made.
  [Bounded C51/C52 retirement](../.local/mcc-gameplay-continuation-20261020/c51-c52-raw-retirement-v1.json).
- [x] Package compiled C53 as requested portable Linux x86_64 v22 ZIP,
  retaining both bundled movies,91 authored prompt sources, original loading
  tone,97 unchanged shaders and bundled Python/launchers. No MCC game cache is
  included. Requested archive retained at dist/Halo2-MCC-Native-Test-Linux-
  x86_64-20261020-v22.zip:2,724,331,701 bytes, SHA25651c5e3059cb73d86e95dfc31b51733af2e7dfabd31b287481c25e37293e69409.
  Package scratch is removed; actual extracted Play/title routes and SSD ZIP
  delivery remain separate next gates. Earlier requested archives are preserved.
  [v22 package](../.local/mcc-gameplay-continuation-20261020/portable-v22-package.json).
- [x] Verify the actual extracted v22 ZIP through both bundled Tk launcher Play
  routes, imported MCC cache admission, retained menu/loading animation and
  ambience, native Lockout rendering, movement/firing and clean exit. Both use
  the bundled Python and produce finite nonzero audio; no reference dependency
  or imported-cache writes are observed. Independent inventory review also
  verifies all4422 checksummed payloads,97 unchanged shaders,91 prompt sources,
  both movies and the approved18,396-byte tone with no MCC private cache in the
  archive. Extracted binaries, private COW cache, captures and pipeline/snapshot
  scratch are removed. Full campaign/CTF/parity remain open; SSD ZIP copy follows.
  [Actual extracted v22 routes](../.local/mcc-gameplay-continuation-20261020/portable-v22-validation.json),
  [Independent delivery/inventory review](../.local/mcc-gameplay-continuation-20261020/f47-v22-final-delivery-review-v1.json).
- [x] Deliver validated v22 ZIP and checksum sidecar to both repository dist
  folders, including /mnt/SSD4TB/Halo-2-Decompiled/dist. Both archives are exactly
  2,724,331,701 bytes with identical51c5e305…93e69409 SHA256. The SSD filesystem
  rejects hard links; the first copy attempt is recorded and cleaned, then a
  small owned probe verifies atomic rename-without-replacement and existing-file
  rejection before successful delivery. Both frontend selections, older requested
  archives and validated source are preserved. All copy/probe scratch is removed;
  requested ZIPs and required working runtimes/shaders are explicitly retained.
  [Both v22 ZIP copies](../.local/mcc-gameplay-continuation-20261020/v22-ssd-delivery-v1.json),
  [SSD publication compatibility](../.local/mcc-gameplay-continuation-20261020/v22-ssd-atomic-publication-probe-v1.json).
- [x] Recheck both delivery directories after the user's renewed request.
  Every current Linux output matches its manifest; both2,724,331,701-byte v22
  archives and checksum sidecars match the qualified51c5e305…93e69409 digest.
  The SSD selection still points to the exact F47 binary. Record delivery to
  both directories as a standing rule for each subsequently qualified build;
  the incomplete C54 candidate is not installed. This read-only recheck changes
  no cache, runtime or selection, and its empty owned scratch is removed.
  [Current dual-directory delivery check](../.local/mcc-gameplay-continuation-20261020/dual-directory-build-recheck-v1.json).
- [x] Reproduce the converted MCC charge caller against unchanged original
  execution. Its3195 instructions,268 stores/1025 bytes, full CPU exit and
  88,003,393 retained bank bytes agree. Both naturally retire the positive effect
  before damage in this entry context. The actual location producer misses the
  compiled `root` symbol because MCC uses0400055B and original code0400054F.
  This diagnoses a namespace seam; it does not accept charge damage or relax the
  production fixture. Independent comparison passes; the separate inherited
  wrapper failure remains recorded and owned scratch is gone.
  [Charge caller context](../work/sandbox-primary-v15/charge-current-world-20261020/caller-context-milestone.json).
- [x] Bind complete17B1F0's authenticated compiled `any`/`replace`/`root`
  comparisons through a bank-owned verified namespace lease. Preserve raw MCC
  IDs; identity-namespace regeneration retains the exact original body. Focused
  Release and sanitizer controls pass, covering collisions, missing/foreign
  namespaces, unknown constants, separate banks, concurrency and lease lifetime.
  Both private builds are removed. Genuine charge damage remains unaccepted;
  unrelated pre-existing generator differences were preserved.
  [Effect symbol component](../work/sandbox-primary-v15/charge-current-world-20261020/symbols-component-v4.json).
- [x] Give the existing damage provider an explicit selected-image observation
  entry while preserving its legacy ABI, arithmetic, stores and owner calls.
  All90 original routines/8976 instruction sites reconstruct the exact canonical
  source;10881 observation operands bind to the authenticated image mapping.
  Direct/nested/legacy cases pass the focused CTest with identical CPU and store
  payloads, including absent listeners. Owned scratch is removed. Actual charge
  comparison now reaches a one-ULP x87 result difference at157B52; independent
  original host-CPU execution is required before any numeric change.
  [Observation generation](../work/sandbox-primary-v15/charge-current-world-20261020/damage-observation-generation-v1.json),
  [Focused test](../work/sandbox-primary-v15/charge-current-world-20261020/damage-observation-cmake/report.json).
- [x] Preserve both the caller and registered damage-store listeners through
  the existing observer fanout. Direct/nested original source writes notify
  each active listener exactly once, with unchanged ABI/payloads; absent,
  caller-only, registered-only and combined cases pass the focused CTest.
  The private build is removed. This closes an observation ownership defect;
  it does not accept the complete charge/release gameplay route.
  [Observer fanout](../work/sandbox-primary-v15/charge-current-world-20261020/damage-observation-tee-cmake/report.json).
- [x] Verify genuine converted MCC charge/release through the unchanged strict
  production lifecycle fixture and matched historical namespace/provider group.
  It produces one positive effect allocation and real damage return, observes
  the player response, naturally retires toNONE, and completes actual held →
  ready → release with a charged projectile and0.15 battery debit. All240 real
  effect frames/two projectile contact effects pass;701 MCC opens are clean and
  owned scratch is removed. The selected client/current mutable cohort is not
  yet qualified by this component test; full original parity remains separate.
  [Strict native charge](../work/sandbox-primary-v15/charge-current-world-20261020/full-charge-v1.json).
- [x] Qualify the bounded instant-charge caller with corrected alignment and
  unchanged original i386 execution on the host CPU. All91,653,656 retained
  accessible final bytes and GP/SSE/x87 controls/status match. Authenticate its
  actual108E87 tail jump to complete CABC0 and CAD35 return; the former60300000
  stop was a reference-observer boundary. Only the named root055B→054F input
  projection is supplied. Preserve LinuxIF/executor-stack boundaries explicitly;
  intermediate host store equality and physical Xbox execution are unmeasured.
  The joined gate also verifies the same compiled World and strict240-tick
  native lifecycle/CMake evidence; no floating-point implementation changes.
  Full weapon/render parity and compatible-client integration remain open.
  [Joined bounded charge gate](../work/sandbox-primary-v15/charge-current-world-20261020/bounded-instant-charge-closure-v1.json).
- [x] Preserve authored/normalized tag-root low-four address bits during
  runtime relocation. The real masterchief_mp root0128694C embeds aligned
  shape01287210 at+8C4; rounding the root to residue0 produced misaligned
  target91320834. Only allocation padding changes. Actual Lockout/Cairo delivery
  independently passes64,290/112,779 typed pointer fields and33,525,954/
  69,887,237 authored bytes. Focused production cache tests pass and scratch is
  removed. The old316-provider mainmenu still rejects its prior class-namespace
  guard, identically reproduced before this allocator change. All38 gameplay
  libraries/295,132 relocation roots retain source low-four bits with unchanged
  class guards. The combined C48 Lockout/charge route now passes; actual gameplay
  across the other maps and full conversion/parity remain open.
  [Alignment contract](../work/sandbox-primary-v15/charge-current-world-20261020/cache-alignment-contract-v1.json),
  [Focused cache test](../work/sandbox-primary-v15/charge-current-world-20261020/alignment-cmake-v3/report.json).
- [x] Check the aligned delivery against all38 gameplay-bearing native
  libraries:295,132 relocated roots preserve source low-four bits with zero
  schema issues and unchanged source-cache bytes/class plans. The mainmenu
  gameplay-provider rejection is the same pre-existing namespace guard under
  the original control; its accepted frontend uses the separate existing owner.
  This is complete library relocation coverage, not all-map gameplay acceptance.
  [Alignment census](../.local/mcc-gameplay-continuation-20261020/device-crash-v1/aligned-delivery-census.json).
- [x] Revalidate the unchanged strict240-tick native charge fixture after
  that allocator correction with39 coherent inputs. Actual allocation, damage,
  response, natural retirement, hold/ready/release, charged projectile and0.15
  debit remain correct;701 MCC opens are clean and private scratch is removed.
  This preserves the native component result while original-host tail-return
  observation and compatible-client integration remain open.
  [Aligned native charge](../work/sandbox-primary-v15/charge-current-world-20261020/full-charge-aligned-v1.json).
- [x] Compile the exact compatible charge World composition and existing
  symbol/damage components for the final client. The five qualified hunks preserve
  both prior VM-restore changes and the selected complete Memory layout. All
  three x86_64 objects compile with strict FP flags; exact dependency/source
  inputs and34,856,636 required bytes are retained for the coherent link. Private
  compiler scratch is removed. Selected private support/death-cycle callers,
  combined runtime acceptance and installation remain open.
  [Compatible charge component build](../.local/mcc-gameplay-continuation-20261020/charge-client-build-v1.json).
- [x] Compile the two matched selected-source private charge callers. Each
  monolith changes only its helper include and three verified symbol comparisons;
  original selected-image wrappers and every other source byte remain exact.
  Both strong API pairs and lease references verify; strict-FP x86_64 compilation
  passes. Required objects/frozen inputs total374,006,168 bytes and both owned
  scratch trees are removed. Source provenance is the contemporaneous manifest
  plus matching DWARF, with no per-object source checksum available. Combined
  gameplay qualification and installation remain open.
  [Selected caller build](../.local/mcc-gameplay-continuation-20261020/charge-selected-callers-build-v1.json),
  [Source audit](../.local/mcc-gameplay-continuation-20261020/charge-selected-callers-audit-v1.json).
- [x] Link the348-input compatible C46 successor from C44 and pinned existing
  components. Compose one World with charge/VM/CTF hunks, selected private
  support/death callers, authored-root alignment, topology lookup and existing
  campaign providers. The link map excludes the old conflicting monoliths.
  Required runtime/cohort are retained for actual qualification; this link
  alone does not accept gameplay, campaign, mode or installation gates.
  [Combined client link](../.local/mcc-gameplay-continuation-20261020/combined-successor-build-v1.json).
- [x] Revalidate actual native64 restore with the complete348-input successor.
  The real MCC World passes three same-World handoffs, one fresh-World handoff,
  cancelled preparation and denied/retried publication. All2808 region bytes/
  metadata match; native addresses remain above4GiB and original continuation
  succeeds. MCC-only/read-write audits pass and owned scratch is removed.
  The strict charge integration separately stops at the already-known original
  weapon-program table104698; reuse its completed owner next. This restore proof
  does not qualify the charge lifecycle or installed successor.
  [Combined restore proof](../.local/mcc-gameplay-continuation-20261020/actual-world-restore-combined-v1.json),
  [Charge integration frontier](../.local/mcc-gameplay-continuation-20261020/combined-charge-v1.json).
- [x] Verify C46's composed native client through actual MCC title/profile/
  lobby/countdown/loading and native Lockout. Real movement and firing survive
  71 rendered frames/98 source ticks and clean exit. The loading GPU/audio join
  completes; reference-input/cache-write audits are clean and scratch is removed.
  This muted route proves the combined client survives actual input; bounded
  charge-table qualification, unmuted delivery, campaign and full parity stay
  open. Installed F45 remains selected until successor delivery acceptance.
  [Combined actual input route](../.local/mcc-gameplay-continuation-20261020/campaign-lockout-c46-native64.json).
- [x] Reuse the existing bounded original weapon dispatch/contact tables in
  the selected compatible factory. All36 readonly words/144 bytes authenticate;
  five source count guards establish table domains. The matched-cohort cold
  comparison preserves all2773 prior owners and adds only18 absent readonly
  words/72 bytes. Earlier reduced-link overlap diagnostics are preserved; the
  complete selected link passes. C47 reuses all348 providers with only that
  factory replacement. Owned scratch is removed; strict actual charge/client
  qualification is the next gate.
  [Finite table composition](../.local/mcc-gameplay-continuation-20261020/charge-factory-build-v1.json),
  [C47 link](../.local/mcc-gameplay-continuation-20261020/charge-table-successor-build-v1.json).
- [x] Preserve native64 restore with C47's added finite tables. All2826
  regions match after three same-World/fresh-World handoffs, cancelled staging
  and denied/retried publication; genuine continuation and addresses above4GiB
  pass. MCC-only/cache-write audit and scratch cleanup pass. The unchanged
  strict charge fixture now genuinely allocates/damages/responds/retires and
  holds/readies/releases a charged projectile with0.15 debit and two contact
  effects, but stops at161 ticks on missing50EF30 animation storage. Full240-tick
  charge integration stays open until its existing owner is carried over.
  [C47 restore](../.local/mcc-gameplay-continuation-20261020/actual-world-restore-c47-v1.json),
  [Actual charge frontier](../.local/mcc-gameplay-continuation-20261020/combined-charge-c47-v1.json).
- [x] Reuse the original loader-zero animation-transform storage at the
  demonstrated50EF30 write. Independent section/source evidence proves the
  absent8160-byte prefix; original283E10 bounds by its supplied count, while
  the selected MCC admission limits nodes to256. All2791 prior cold owners and
  node255's existing owner stay exact; original codecs still produce every live
  transform. Required compatible factory/frozen closure5,145,528 bytes is retained
  and compiler/constructor scratch removed. Final240-tick integration remains
  open until C48's strict actual fixture passes.
  [Animation storage composition](../.local/mcc-gameplay-continuation-20261020/charge-factory-build-v2.json),
  [Independent domain/storage audit](../.local/mcc-gameplay-continuation-20261020/charge-factory-v2-node-prefix-audit-v1.json).
- [x] Accept the strict240-tick charge lifecycle with the complete selected-
  compatible C48 client providers and unchanged production fixture. One positive
  allocation/damage return/player response/natural retirement, held→ready→release,
  actual charged projectile,0.15 battery debit and two contact effects pass.
  All2827 regions also pass repeated same/fresh-World native64 restore, cancellation,
  denied/retried publication and original continuation above4GiB. MCC-only/
  cache-write audits pass and both private scratch trees are removed. Full
  weapon/effects/render/audio, campaign and mode parity remain open; actual
  unmuted launcher adoption is next.
  [Integrated strict charge](../.local/mcc-gameplay-continuation-20261020/combined-charge-c48-v1.json),
  [Integrated native restore](../.local/mcc-gameplay-continuation-20261020/actual-world-restore-c48-v1.json).
- [x] Stage and inspect the composed C48 game as the unselected SSD F46
  candidate. The existing bounded readonly shader-directory relocation changes
  only that path field; all97 shaders stay exact. Actual unmuted title/profile/
  lobby/countdown/loading/native Lockout accepts movement/fire and exits cleanly,
  producing3,704,320 finite audio frames. The617,860-byte required PNG shows
  textured world/sky/weapon/hands/HUD and is visually inspected. Runtime/shaders
  are retained for delivery; raw PCM/captures/scratch are removed and F45 remains
  selected. Both installed Tk Play buttons and portable adoption are next;
  campaign/full visual parity remains open.
  [Actual candidate proof](../.local/mcc-gameplay-continuation-20261020/f46-native-proof-v1.json),
  [Image review](../.local/mcc-gameplay-continuation-20261020/f46-native-image-review.json).
- [x] Adopt F46 behind both installed launcher Play buttons after actual
  end-to-end checks. Normal Play includes the bundled intro; title Play skips
  it. Both reach MCC menus/countdown/loading/native Lockout, accept movement/
  fire, mix finite nonzero audio and exit cleanly with clean input/cache-write
  audits. Exact runtime/shader/capability owners are selected atomically; F45,
  its selection preimage and all prior saves/assets remain available for rollback.
  Private test homes/PCM/traces/scratch are removed. This adopts bounded native64/
  charge integration, while campaign, full modes and TU1.5 parity remain open.
  [Both installed Play routes](../.local/mcc-gameplay-continuation-20261020/f46-installation-v1.json).
- [x] Refresh the designated Linux x86_64 build outputs with the exact
  accepted C48 raw/F46 local game binaries. Both manifests and output hashes
  verify; all other platform builds stay skipped. These are requested local
  compiled outputs; GitHub release publication/real updater acceptance remains
  separate.
  [Latest build outputs](../.local/mcc-gameplay-continuation-20261020/f46-build-output-v1.json).
- [x] Resume genuine MCC CTF player allocation before original15AFB0. Both
  complete player constructors/join continuations return with real salted
  datums3966763008/3966828545 and teams0/1; original owners derive occupied-team
  masks1 then3. Thirty translation units link with all316 base providers and
  current-source/MCC guards exact. The698-open audit has no forbidden, unknown
  or unmatched inputs; private scratch is removed. Activation now reaches
  15A625→A6780. Objective birth and complete rules remain unaccepted.
  [Real-player checkpoint](../work/sandbox-primary-v15/ctf-native-lifecycle-20261020/real-players-checkpoint.json).
- [x] Compose complete original A6780 and execute its real first-activation
  callback through native MCC World. Activation reaches the authored flag's
  cloth constructor11A160; its next missing input is COMISS operand46A8B8.
  Authenticate exactly four readonly bytes`0BD7233E` from the pinned TU1.5
  image before adding them through the existing literal-data owner. This is
  callback/constructor progress, not completed objective birth or CTF rules.
  [Cloth-constructor checkpoint](../work/sandbox-primary-v15/ctf-native-lifecycle-20261020/ctf-cloth-constructor-checkpoint.json).
- [x] Return from complete original15AFB0 and create both CTF flags through
  the genuine native object factory. Salted datums3798925312/3798990849 retain
  authored MCC definition4052505215, teams0/1 and carrierNONE; object markings
  and registry agree. All prior1380 readonly operand bytes are preserved with
  only the authenticated four-byte cloth cell added. Focused tests and actual
  MCC native input audit pass; scratch is removed. This accepts first objective
  birth only; subsequent lifecycle/rules/frontend admission remain open.
  [Real factory birth](../work/sandbox-primary-v15/ctf-native-lifecycle-20261020/ctf-real-factory-birth-milestone.json).
- [x] Execute actual Assault first activation, both genuine bomb births,
  complete160F10 registry cleanup and native deletion/salted retirement.
  Two real players and objects retain original ownership throughout; every
  case has a clean MCC input audit and removed scratch. CTF cleanup reaches
  the next missing cloth-destructor owner1190D0; it remains unaccepted.
  [Assault birth and retirement](../work/sandbox-primary-v15/ctf-native-lifecycle-20261020/assault-real-birth-retirement-milestone.json).
- [x] Complete genuine CTF flag deletion/salted retirement through original
  cloth destructor1190D0→16E8A0 in the retained coherent MCC cohort. Independent
  untouched original execution matches45 instructions,8 stores, full ABI and
  every accessible-bank changed/touched byte with zero differences. The141.7MB
  temporary capture and owned scratch are removed. Four concurrently added
  factory alias cells are recorded as unqualified workspace drift; this result
  accepts the pinned cohort only and does not admit CTF as a complete mode.
  [CTF birth and retirement](../work/sandbox-primary-v15/ctf-native-lifecycle-20261020/ctf-real-birth-retirement-milestone.json).
- [ ] Verify CTF/Assault carrier, scoring, rendering and network lifecycles
  against the current coherent factory before mode admission.
- [x] Authenticate the four intervening vector-default pointer cells and
  preserve the prior factory's complete cold bank. Both factories compiled into
  one diagnostic compare every byte/access/owner of3113 existing regions; only
  four original writable pointer cells/16 bytes are added, targeting unchanged
  immutable vectors. Write controls pass and private scratch is removed.
  Gameplay requalification awaits the matched effect-namespace provider group.
  [Factory alias preservation](../work/sandbox-primary-v15/ctf-native-lifecycle-20261020/current-factory-alias-milestone.json).
- [x] Requalify actual CTF and Assault objective birth, registry cleanup and
  full salted retirement with all36 matched effect namespace/World/death/support
  providers, excluding stale monoliths. Both MCC input audits pass; complete
  cloth teardown again matches untouched original execution with zero bank/ABI
  differences. Sources/base316 are stable through capture and scratch is gone.
  The latest factory is pinned; earlier four-cell proof is not broadened to
  unrelated later hunks. Restore-only receivers use their separately matched
  ABI; units/carriers/scoring/frontend admission remain open.
  [Matched objective cohort](../work/sandbox-primary-v15/ctf-native-lifecycle-20261020/current-symbol-objectives-milestone.json).
- [x] Reach genuine MCC biped birth and bind the actual CTF player. Complete
  constructor returns salted unit3799056386 and14F620 binds player3966763008;
  existing equipment1613B0 reaches missing original engine callback24A810 before
  completion. MCC-only audit passes and scratch is removed. Equipment, carrier
  attachment, contact/button pickup and scoring remain unaccepted.
  [Real unit checkpoint](../work/sandbox-primary-v15/ctf-native-lifecycle-20261020/current-symbol-unit-birth-v1.json).
- [x] Complete actual two-player CTF biped birth, binding and1613B0 starting
  equipment through complete24A810/106350 owners. Real units3799056386/
  3799187460 and starting weapons3799121923/3799252997 are produced without
  supplied results; all26 prior CTF bodies remain exact. Native exit and MCC
  input audit pass, with removed scratch. The wrapper's separate missing
  observation capture remains red; independent full-bank callback comparison
  and carrier attachment still require acceptance.
  [Native equipment producer](../work/sandbox-primary-v15/ctf-native-lifecycle-20261020/current-symbol-unit-birth-v3.json).
- [x] Independently qualify that real equipment callback against untouched
  TU1.5 execution:59 instructions,6 stores, complete GP/XMM/x87/flags and
  accessible-bank comparisons match with zero differences. The actual two-unit
  equipment producer, all36 matched namespace providers and278 prior ABI cases
  pass; all26 pre-existing bodies remain exact. Input/source guards pass and
  raw/compiler scratch is removed. Existing-enemy-flag attachment is next;
  player contact/button pickup remains a separate gate.
  [Original equipment callback](../work/sandbox-primary-v15/ctf-native-lifecycle-20261020/real-unit-equipment-milestone.json).
- [x] Attach the existing enemy flag through complete native CF5A0 and
  actual FC040/160C90/160D40 carrier owners. The real flag stays the same datum
  in inventory, and registry derives its real unit/player without cloning.
  Enemy-flag callback replay73 instructions/6 stores/full ABI/bank matches
  untouched original; equipment replay remains green. Input/source guards pass
  and scratch is removed. This accepts a direct existing-item request only;
  contact/distance/button pickup, drop/return/scoring remain open. Adding local
  1A87C0 changes one old callback callsite to the same direct complete callee;
  the remaining25 old bodies stay exact.
  [Existing flag carrier](../work/sandbox-primary-v15/ctf-native-lifecycle-20261020/real-existing-flag-carrier-milestone.json).
- [x] Qualify the complete direct CF5A0 pickup request with unchanged original
  i386 execution on the host CPU. All92,730,904 accessible final source bytes
  and GP/SSE/x87 controls/status match, with no supplied CPU inputs or results.
  The four matrix words and precision-status discrepancy was in Unicorn replay;
  production math remains unchanged. Preserve the existing Linux user-mode IF
  entry boundary explicitly. Exact historical factory/matched providers are
  pinned; concurrent later factory work stays untouched. MCC asset audit passes
  and raw scratch is removed. Physical Xbox transcendental behavior and actual
  contact/button pickup remain separate gates; bounded drop work can continue.
  [Whole original-host comparison](../work/sandbox-primary-v15/ctf-native-lifecycle-20261020/whole-existing-flag-original-host-milestone.json).
- [x] Complete genuine inventory-clear flag drop through wholeCF4D0,
  FC040(NONE),160EA0 and the complete original86-site24A4B0 owner. The same flag
  stays alive; actual item/registry carrier/player and real inventory become
  NONE. Authored reset timer30 and actual-clock-derived tick base30 stay exact.
  All30 prior bodies remain unchanged;278 isolated ABI cases pass with all
  inputs preserved and only the independently refreshed missing-owner frontier
  changing. MCC-only audit passes; probe/build/shared-provider scratch and
  38,531,632 temporary provider bytes are removed. This accepts the pinned
  historical native drop owner only; button actions, return/scoring, full modes
  and current-client delivery remain open.
  [Native flag drop](../work/sandbox-primary-v15/ctf-native-lifecycle-20261020/flag-drop-native-milestone.json).
- [x] Prepare the authenticated ordinary CTF update/timer/return source slice
  through the existing generator:41 complete owners/2825 instruction sites.
  All281 isolated original ABI cases pass with the278 prior inputs/outputs
  unchanged; only three original callsites move from external dispatch to the
  same newly owned complete native callees. Owned scratch is removed. Actual
  dropped-flag timer/return, proximity, scoring and mode admission are untested;
  this is source/ABI preparation only.
  [Return-source checkpoint](../work/sandbox-primary-v15/ctf-native-lifecycle-20261020/flag-return-source-checkpoint.json).

- [x] Build the existing41-owner CTF provider and its narrow selected-World
  dispatcher binding as a compatible pair. The CTF source is the exact prior
  ABI-checkpoint source; World adds only its public include and existing
  owns-entry branch. All charge/VM changes and the selected complete Memory
  layout remain exact. Strict-FP compilation passes, required frozen inputs/
  objects total17,497,458 bytes and private scratch is removed. This prepares
  the actual provider connection; native return/scoring, combined runtime and
  complete-mode admission remain unchecked.
  [Compatible CTF build](../.local/mcc-gameplay-continuation-20261020/ctf-client-build-v1.json).

## MCC frontend and binary updates (October19)

The October19 delivery opened the validated title/profile/main/Settings route.
Preserve its movies, custom prompt bank and qualification receipts as historical
evidence. Their earlier import, package and capability tasks below are superseded
where they require separate supplied assets; they are not current delivery
instructions. The next portable package must satisfy the sole-MCC rule above,
with unassigned movie roles and fresh runtime/launcher qualification still open.
Keep the accepted native Lockout test as a separate action. V19's accepted
gameplay conversion/build remains preserved within its recorded scope.

- [x] Preserve an existing qualified frontend capability in the portable
  packager through its fixed wrapper/payload/loader and compiled shader binding.
  Explicit raw-runtime qualification rejects changed owners; the portable
  capability keeps the same cache resource hashes and gameplay/parity limits.
  Mutation controls reject changed runtime/loader/wrapper/map bytes. Three
  focused CTest groups pass, including legacy movie bundling; both disposable
  build trees are removed. This is packaging admission coverage; final archive
  extraction, actual launch and delivery still require the accepted successor.
  [Packaging tests](../build/test-results/mcc-portable-frontend-20261020/report.json),
  [Movie compatibility](../build/test-results/mcc-portable-frontend-movies-20261020/report.json).
- [x] Prepare distinct v21 packaging/extracted-route helpers for the C48
  successor, preserving v20. Syntax checks and independent source review pass;
  portable launchers, movies, prompts, tone, shader and full-cache admission
  owners are reused. Both actual Tk Play route bodies stay exact. The helpers
  require the separately qualified raw sidecar; no archive is created by this
  preparation milestone.
  [V21 preparation](../.local/mcc-gameplay-continuation-20261020/portable-v21-preparation.json).
- [x] Deliver the already-compiled C48 successor in the v21 portable Linux
  x86_64 ZIP:2,724,312,689 bytes, SHA256
  `33c787d3d508a0d0816fb2851c4411df409def68c74537b626a745163b851fea`.
  Both actual extracted Tk Play routes pass title/profile/lobby/countdown/
  loading/native Lockout, movement, firing, finite nonzero audio and clean exit.
  Archive checksums, bundled Python and full883-file MCC cache admission pass;
  reference-input/cache-write audits are clean. Exact movies, custom prompts,
  approved tone and all97 shaders are included; MCC game data is imported by
  the recipient. Both packaging and extracted-validation scratch trees are
  removed. F46 stays selected on the SSD; v20 remains preserved. Campaign,
  complete gameplay modes, effects and full TU1.5 parity remain open.
  [Compiled v21 archive](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261020-v21.zip),
  [Package receipt](../.local/mcc-gameplay-continuation-20261020/portable-v21-package.json),
  [Actual extracted acceptance](../.local/mcc-gameplay-continuation-20261020/portable-v21-validation.json).
- [x] Copy the accepted v21 ZIP to the other requested checkout at
  `/mnt/SSD4TB/Halo-2-Decompiled/dist/`, with a matching SHA256 sidecar in both
  directories. Source and destination hashes match exactly; no partial copy
  remains. The SSD's selected F46 runtime, capability and designated Linux
  build-output binary are verified, and its selection remains unchanged.
  Both2,724,312,689-byte archives are retained as requested deliverables.
  [SSD delivery receipt](../.local/mcc-gameplay-continuation-20261020/v21-ssd-delivery-v1.json).
- [x] Produce the already-compiled v20 portable Linux x86_64 archive from
  qualified C44 and the existing compiled portable launcher pair:2,724,225,120
  bytes, SHA256
  `7e42a0b7ca8492c5b6e8f0862ec5b3fa4d99d6c41baa44d441f45c5edfbaae4d`.
  It includes both exact bundled movies,91 authored prompt source files, the
  approved original tone, all97 unchanged shader files and the bounded frontend
  capability. No MCC data is included; compilation is unnecessary. Both portable
  launchers use package Python through PATH. The v19 diagnostic used local
  launcher inputs and remains unqualified; its normal native route passed but
  the title fixture omitted its Tk Play click. Actual extracted v20 acceptance
  follows below; this archive preserves C44's bounded gameplay scope.
  [Compiled archive](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261020-v20.zip),
  [Package receipt](../.local/mcc-gameplay-continuation-20261020/portable-v20-package.json).
- [x] Verify the extracted v20 archive through both actual Tk Play buttons.
  Normal and title launchers use bundled Python, load the qualified MCC cache,
  reach title/profile/lobby/countdown/loading and native Lockout, accept movement
  and firing, produce finite nonzero audio and exit cleanly. All archive checksums
  pass; reference-input/cache-write audits are clean. The reconciled test cache,
  extracted binaries, audio and capture scratch are removed. This delivers the
  requested compiled test ZIP; campaign, effects, modes and full TU1.5 parity
  remain open, and installed F45 is preserved pending the combined successor.
  [Extracted portable acceptance](../.local/mcc-gameplay-continuation-20261020/portable-v20-validation.json).
- [x] Retire the superseded unqualified v19 diagnostic archive and two obsolete
  package one-offs after v20 actual acceptance. Active-process, tracked-file,
  link and runtime-selection checks pass;2,724,227,996 bytes are removed. Keep
  v20 and earlier requested archives, frozen inputs and the small v19 failure/
  code-preservation receipts. Selected runtimes are exact and scratch is gone.
  [Diagnostic cleanup](../.local/mcc-gameplay-continuation-20261020/portable-diagnostic-retirement.json).
- [x] Retire107,330,453 bytes of owned superseded C43 raw binary, release-only
  object and obsolete one-off helpers after active-process, selection and tracked
  file checks. Keep C43's frozen source/Main/cohort, the current C44 candidate,
  original assets, user-requested archives and all small failure receipts.
  F45 selection is exact and the cleanup runner's scratch is removed.
  [Retirement receipt](../.local/mcc-gameplay-continuation-20261020/superseded-retirement.json).

The user's current sequence is to carry the existing complete submenu work into
MCC-only Play, including video settings, split screen and Online. Reuse the
existing controls/layout/input implementation with MCC-owned definitions,
fonts, artwork and identifiers. Keep the MCC cache and existing shaders selected.

- [x] Adapt the existing PC menu metadata/resource loader and compiled PC
  string identifiers to the imported MCC mainmenu cache. Preserve raw authored
  MCC identifiers and source ownership; require no Vista runtime bank.
  The actual imported MCC loader parses 39 existing PC screen definitions,
  185 bitmap banks/545 pages and 3,174 locale IDs. All 7,669 compiled PC symbols
  map uniquely by their exact names into the selected SSD MCC namespace.
  Public layouts and existing shaders are unchanged. Unused all-zero MCC
  material slots follow the existing MCC reader policy; nonempty references
  remain checked. Component acceptance does not establish live menu parity.
  [Loader evidence](../.local/mcc-submenu-adapter-20261019/handoff.json),
  [Identifier evidence](../.local/mcc-frontend-string-inventory-20261019/pc-unique-v1/consumer-adaptation.json).
- [x] Validate Settings, Video/Audio/Keyboard choices and confirmation/cancel,
  split-screen sign-in/lobby, and Online sign-in/browser entry through the
  existing state/painter controllers with actual imported MCC assets. Record
  unavailable service/gameplay routes separately from working local menus.
  The actual selected SSD MCC cache passes 224,630 checks across 30 drawn
  screens in 4:3 and 16:9 using the exact V25c runtime providers. Tests cover
  primary/secondary bindings and saved preferences, resolution confirmation,
  cancel and the 16-second rollback, two local profiles/controller slots,
  lobby/Setup and offline Online admission. MCC-only PC host extras no longer
  become phantom Setup actions. The nested PC canvas receives its authored
  projection once, including three independent authored bitmap extent checks.
  No account login, network service or host display change
  occurs in this CPU/state/painter gate; actual GPU/launcher checks follow.
  [Selected-cohort regression](../.local/mcc-ui-submenu-reuse-20261019/cohort-regression.json).
- [x] Build only the affected ABI-compatible frontend providers, validate
  actual native input, rendering and sound, and install the qualified successor
  in the SSD checkout. Recheck both launchers' actual Play flows with that
  same selected cache; preserve the working V24 installation for rollback.
  V25d reuses the selected development cohort and existing 97 shader files.
  Actual GPU title/profile/keyboard editor/Settings/Video/Audio/Network/About/
  Online sign-in/split-screen lobby/Setup navigation and finite nonzero mixer
  audio pass. Normal and title-first launchers both pass actual no-argument
  Tk Play clicks, including the cached intro/skip policy. The SSD now selects
  `.local/f26/halo2_native`; `.local/f24` and `.local/f25` remain intact for rollback. The same
  selected MCC cache is read without modification; no ISO or Vista cache opens
  occur. The broader current-source CMake dependency build was cancelled after
  configure; it is not reported as passed. The complete focused test instead
  passes against the exact ABI-compatible providers used by this native build.
  Full gameplay, live Online service operation and full-frame visual parity
  remain separate gates. The source-qualified PC-only download caption is hidden
  under the native Xbox lobby policy, which has no download owner; its recovered
  PC producer also hides the quiescent 100% state. Final native and both actual
  launcher flows pass again, with a same-session screen/audio recording.
  [Actual native and both Play flows](../.local/mcc-ui-submenu-reuse-20261019/native-final-acceptance.json),
  [Test lifecycle and source pins](../.local/mcc-submenu-regression-20261019/validation.json).
- [x] Correct the specific visual regressions in the user's October19 wide
  screenshots: undersized split-screen character preview, overlapping panel
  geometry, and main-menu selection size/style. Use original authenticated
  layouts and the existing working development presentation as authority.
  Build deterministic, source-qualified failing probes before changing sizes;
  functional menu navigation and current screenshots do not prove visual parity.
  The original failing highlight/panel assertions and the exact Elite viewport
  regression pass through the actual coherent painter. Root inspected native
  640x480 and2553x1380 images and the final installed1280x720 Elite frame.
  Full-frame parity outside these specific corrections remains open.
- [x] Restore authenticated original logical definitions for stock title5,
  main6 and split-signin30 while sampling only MCC rasters, localized strings
  and the existing MCC-to-original logical font projection. The actual V26
  painter turns all four original failing extent assertions green across
  twelve 4:3/wide cases; the final coherent submenu regression passes 225,190 checks.
  Source-qualified model viewports use stable private scene copies without
  changing cameras, poses, FOV or shaders. GPU visual acceptance remains open.
  [Geometry regression](../.local/mcc-ui-visual-regression-20261019/validation.json),
  [Functional regression](../.local/mcc-menu-visual-20261019/cohort-final-regression.json).
- [x] Validate those visual fixes at the user's wide aspect ratio and 4:3,
  using the imported MCC cache and unchanged shaders, then deliver a refreshed
  compiled selection and video with sound. Keep the accepted functional build
  available while this visual work is in progress.
  SSD Play now selects`.local/f27/halo2_native`, reusing all other coherent
  providers and97 byte-exact shaders. Actual native1280x720 input/render/audio,
  plus both normal/title-first no-argument Tk Play flows pass. The selected MCC
  cache stays read-only; forbidden ISO/Vista inputs are absent. The same-session
  recorded video has finite nonzero decoded audio. Keep f24/f25/f26 for rollback.
  Ten modified-bank rejection controls, two localized-string/pixel ownership
  controls and all16 model signatures pass131 qualification checks.
  [Actual installed native and both Play flows](../.local/mcc-menu-visual-20261019/native-final-acceptance.json),
  [Profile qualification](../.local/mcc-menu-visual-20261019/profile-qualification.json),
  [Video with sound](../.local/mcc-menu-visual-20261019/mcc-menu-layout-with-sound.mp4).
- [ ] Audit remaining lobby, profile/appearance and other native menu styling
  against authenticated original metadata and full rendered references.
  Screen47's two MCC-authored FOV differences are preserved by this viewport-only
  fix and remain a separate source-qualified projection task. These targeted
  visual corrections do not certify every submenu or full Xbox framebuffer parity.
- [x] Correct the newly reported title/footer placement relative to their
  blue frame bars. The original text bounds currently sample a 2048x1303 MCC
  frame inside the original 1024x512 rectangle; this is a different aspect ratio,
  so panel/model acceptance does not validate frame-art anchoring. Compare exact
  original reference bar coordinates with the actual MCC artwork before changing
  text placement. Preserve custom prompt pixels, MCC-only runtime inputs and
  the selected working build until a corrected native successor passes.
  V27c passes actual native1280x720 title/profile/keyboard/Settings/Video/Audio/
  Network/About/Online-sign-in/split-screen navigation, finite nonzero audio and
  both no-argument normal/title-first Tk Play flows. Root visually reviewed the
  settled sign-in and Video Options frames: titles/prompts sit within the bars.
  SSD now selects`.local/f29/halo2_native`; all97 shader files stay exact, the
  selected MCC cache remains read-only and no ISO/Vista runtime input opens.
  The same-session video with sound is retained. Settings' immediate post-click
  reference is a transition frame; the original320ms shell intro is preserved.
  [Actual installed acceptance](../.local/mcc-frame-anchor-20261019/final-native/native-final-acceptance.json),
  [Video with sound](../.local/mcc-frame-anchor-20261019/final-native/mcc-frame-anchors-with-sound.mp4).
- [x] Isolate frame-art anchoring with a deterministic failing actual-painter
  probe, using the original frame only as an offline contour reference.
  The old header band projects to y27.11..48.72 instead of31..59; the footer
  projects to415.73..437.34 instead of421..462. A SHA-qualified MCC-pixel-only
  piecewise geometry projection makes both contour assertions green without
  moving the title/footer text or changing shaders. Unknown artwork retains
  its prior mapping. The final V27c integration/native acceptance is recorded above.
  [Red reproduction](../.local/mcc-frame-anchor-20261019/anchor-loop-red.json),
  [Single-variable green result](../.local/mcc-frame-anchor-20261019/anchor-loop-green.json).
- [x] Reuse the original shared fullscreen title/footer metrics and bounds for
  PC submenu bodies, while retaining their existing single PC canvas projection.
  A second red probe exposed PC/Online selecting another shell background;
  both now use the same corrected MCC frame as their shared title/footer.
  The final coherent V27c suite passes255,192 checks, including independent
  Settings title and frame anchors. Dialog and native full-surface social
  layouts retain their existing owners. Final native acceptance is recorded above.
  [Title-route red result](../.local/mcc-frame-anchor-20261019/title-shell-red.json),
  [Background-route red result](../.local/mcc-frame-anchor-20261019/shell-background-red.json),
  [Final CPU regression](../.local/mcc-frame-anchor-20261019/cohort-regression.json).
- [x] Fix the newly reported player-model flash before its slide-in. Reproduce
  the authored pre-delay frames through the actual MCC painter, recover the
  original common widget admission rule, preserve model clocks/poses/cameras,
  and validate the transition before continuing the queued Online menu pass.
  V28 passes actual native input/render/sound and both normal/title-first Tk
  Play flows with the same selected MCC cache. SSD now selects`.local/f30/halo2_native`.
  The fresh same-session recording has finite nonzero decoded audio. All97
  shader files, poses/cameras, source UI constructor requests and model clocks
  remain unchanged; no ISO/Vista input opens or cache writes occur.
  [Installed native and Play acceptance](../.local/mcc-model-delay-20261019/native-final-acceptance.json),
  [Recording with sound](../.local/mcc-model-delay-20261019/mcc-menu-transition-with-sound.mp4).
- [x] Reproduce the pre-delay flash in the actual selected MCC painter and
  execute the unchanged original22E3B4 predicate for132 boundary/visibility
  cases. The painter now honors each scene's authored delay before submission;
  constructor UI requests and all model clocks remain independent. Both species
  and three aspect ratios pass delay/slide-boundary checks in the255,258-check
  suite. Root reviewed actual MCC-only1280x720 captures at0ms(no model) and330ms
  (Elite arrived), with exact shaders and no ISO/Vista runtime opens. The
  completed launcher integration/recording is recorded immediately above.
  [Original predicate](../.local/mcc-model-delay-20261019/original-admission.json),
  [Red regression](../.local/mcc-model-delay-20261019/red.json),
  [Green regression](../.local/mcc-model-delay-20261019/regression.json),
  [Actual GPU timing frames](../.local/mcc-model-delay-20261019/native-transition-frames.json).
- [x] Reproduce the reported settings illustration mismatch and misplaced
  Start Game/Game Setup buttons in the actual MCC painter. The red tests
  identify Controller drawing Appearance and lobby hits at y48-63 instead
  of the original y67-87. Stock source-qualified logical metadata now reuses
  the existing native controls with MCC-owned pixels and strings; all seven
  profile rows select their semantic artwork, and both lobby actions use the
  original bounds. Voice Output uses MCC's surviving Communicator image;
  MCC replaced the Xbox output page with keyboard artwork, so that absent
  page is not synthesized or imported from another bank. A separate red test
  caught the dynamic lobby thumbnail being omitted; its MCC-owned image now
  retains the original220x207 logical extent. V29c passes254,665 actual
  state/painter checks. Native installation and final Online validation follow.
  [Initial red](../.local/mcc-menu-row-layout-20261019/regression-red.json),
  [Lobby bounds red/green](../.local/mcc-menu-row-layout-20261019/row-probe-green.json),
  [Map art red](../.local/mcc-menu-row-layout-20261019/map-art-red.json),
  [MCC regression](../.local/mcc-menu-row-layout-20261019/regression.json).
- [x] Install and visually verify the settings and lobby corrections through
  both real launcher Play actions. F31 passes native input/render/audio and
  unchanged shader/MCC-source audits. Reviewed captures show the headset and
  correct voice help, controller art, and Start/Setup below the title with the
  Lockout thumbnail intact. Final Online-panel successor validation is separate.
  [Native F31 acceptance](../.local/mcc-menu-row-layout-20261019/native-final-acceptance.json).
- [x] Qualify the final V29d local Online layout and resource namespace in
  the selected MCC cache. The strict nine-aspect audit checks actual font/
  bitmap byte ownership, foreground bounds, private identity handling and
  pointer projections. Matchmaking (including relay admission), clans and
  parties pass their existing local HTTP fixtures. The fixture now resolves
  its name placeholder by the actual MCC symbol and checks party bounds in
  the same qualified logical view drawn by the painter. No real accounts or
  production services are exercised. The source-qualified profile rejects
  altered metadata/ambiguous resource graphs while retaining localized text
  and custom MCC pixels (918 checks). NativeGPU/launcher acceptance is F32;
  the Online2D capture review follows before closing the overall menu task.
  [Strict Online](../.local/mcc-online-reuse-20261019/online-strict-panels.json),
  [Local HTTP fixtures](../.local/mcc-menu-row-layout-20261019/online-services.json),
  [Qualification controls](../.local/mcc-menu-row-layout-20261019/profile-qualification.json),
  [Final native Play](../.local/mcc-menu-row-layout-20261019/final-native/native-final-acceptance.json).
- [x] Convert the responsive UI asset during fresh MCC import and repair it
  for existing caches. The new bundled profile is copied into the vanilla
  cache as`presentation/ui/native-layout.json`, pinned to its MCC mainmenu and
  profile SHA256. It normalizes original logical widget/image dimensions and
  carries responsive Friends/Clans center-slice policy; MCC pixels, mod folders,
  raw directory paths and shaders remain intact. The painter reads this actual
  imported asset and checks it against the matching compiled profile. Fresh
  preparation, managed upgrades, idempotence, customization preservation and
  symlink/duplicate-key controls pass. All25 onboarding and21 preparation tests
  pass, and CMake/CTest runs the initial six importer tests (the final coherence check raises the unit total to seven) in owned scratch. The
  selected SSD cache has been upgraded without changing its mainmenu bytes.
  [Actual cache upgrade](../.local/mcc-menu-row-layout-20261019/import-upgrade.json),
  [CMake/CTest](../.local/mcc-menu-row-layout-20261019/cmake-ui-layout/report.json).
- [x] Extend the existing full-surface Clans layout to Friends for widescreen.
  The real MCC painter red test rejected the compact Friends owner. Both tabs
  now share edge-anchored body/header/footer/scrollbar geometry, growing flat
  centers while retaining corner pixels, fonts and icon sizes. Allnine aspect
  ratios pass resource, bounds and pointer tests, including long/duplicate
  friend names and scrolling. NativeGPU Friends captures pass4:3,16:9 and21:9;
  the21:9 result has been visually reviewed. Clans stress captures and final
  launcher installation follow before closing delivery.
  [Widescreen red](../.local/mcc-menu-row-layout-20261019/widescreen-red.json),
  [Nine-aspect/service regression](../.local/mcc-menu-row-layout-20261019/responsive-services.json),
  [Friends GPU](../.local/mcc-menu-row-layout-20261019/responsive-gpu.json).
- [x] After title/footer acceptance, fix the Online menus using the existing
  original menu/state/input work with the MCC asset namespace. Audit entry,
  sign-in presentation, browser/matchmaking navigation, social/submenu return,
  title/footer placement and authored resource ownership. Exercise locally
  reachable routes and retain service/account prerequisites as explicit gates;
  do not infer live Online acceptance from offline menu tests.
  Completed for locally reachable MCC menu paths. F33 is installed and selected
  in the SSD checkout. Its direct GPU/input/audio witness and both actual
  no-argument launcher Play runs pass while reading the imported native UI asset;
  that file now has an exact, independently checked capability SHA256. A red
  admission check exposed the old presentation allowlist, so only this specific
  converted path was added; altered bytes and unknown sibling files still fail.
  Friends and100-member Clans render at640x480,1280x720 and2560x1080; the two
  ultrawide results were visually reviewed. Nine-aspect CPU/bounds/pointer and
  local matchmaking/clan/party HTTP fixtures pass; real production accounts,
  networking/gameplay and whole-frame Xbox pixel parity remain separate gates.
  Optional Vulkan validation-layer support is unavailable on this host; actual
  GPU captures use the normal renderer and existing shaders without that layer.
  The full menu regression passes254,665 checks and profile controls pass918.
  [F33 native/Play](../.local/mcc-menu-row-layout-20261019/responsive-native/native-final-acceptance.json),
  [Friends GPU](../.local/mcc-menu-row-layout-20261019/responsive-gpu.json),
  [Clans GPU](../.local/mcc-menu-row-layout-20261019/responsive-clans-gpu.json),
  [Selected cohort regression](../.local/mcc-menu-row-layout-20261019/responsive-regression.json),
  [Profile controls](../.local/mcc-menu-row-layout-20261019/responsive-qualification.json).
- [x] Correct the subsequently reported selection-pane jumps, matchmaking ring
  anchor and Online browser column/title overlap. Restore the surviving MCC
  Bungie.net panel, carry supported geometry through import conversion, and
  verify all selector rows plus searching/browser states in the actual MCC
  painter, GPU and launcher-selected successor. F34 is installed and selected
  in the SSD checkout. Both actual no-argument Tk Play buttons pass with the
  converted UI asset, all97 existing shaders and no Xbox/Vista runtime opens.
  The real input route checks allseven category rows, map selection/cancel and
  the existing original channel replacement back to the lobby. The extended
  menu regression passes255,045 checks; source qualification passes972, including
  custom browser screen/skin rejection. Fifty-eight focused GPU frames cover
  4:3,16:9 and21:9; six tick0/tick30 pixel comparisons confine the animated
  MCC ring to the correct active-stage viewport. Full local matchmaking,
  clans/voice and party fixtures pass independently of capture timing. Fresh
  import/repair tests pass (8 UI,25 onboarding,21 preparation), with the UI
  suite also passing CMake/CTest. Real production services and whole-game
  TU1.5 parity remain separate gates. An initial test-route expectation and
  an interrupted capture are retained as diagnostics; F33 was restored after
  both, and the final installation retains a durable F33 rollback journal.
  [Reproduced selection/model errors](../.local/mcc-selector-search-layout-20261019/red.json),
  [Bungie artwork import audit](../.local/mcc-selector-search-layout-20261019/news-import-audit.json),
  [Native/Play acceptance](../.local/mcc-selector-search-layout-20261019/native/native-final-acceptance.json),
  [GPU evidence](../.local/mcc-selector-search-layout-20261019/gpu-final.json),
  [Ring pixel checks](../.local/mcc-selector-search-layout-20261019/ring-pixels.json).
- [x] After those menu fixes, reuse the completed menu → loading screen →
  match transition and connect it to the current game/runtime. Locate the
  existing implementation and its acceptance evidence first; preserve the MCC
  cache as the runtime asset owner and the existing shaders. Verify an actual
  menu launch through loading into the native match, then update this item.
- [x] Reuse the existing native offline session/countdown/world owner and
  reproduce the actual installed MCC Start Game failure. The original
  candidate crashes during world preparation; coherent flow and prop consumers
  remove that heap failure. Preserve all existing frontend/shader providers.
- [x] Bind the recovered announcer to the selected MCC scenario directory and
  its existing per-sound gestalt/resource decoder. Actual Lockout response
  banks contain80 general,2 Slayer and8 King rows; no Vista cache is opened.
- [x] Exercise the real observer before the first player activation, then the
  actual first native tick and first-person weapon binding. Mount three missing
  immutable camera operands only after checking their words/instructions against
  the offline TU1.5 reference. All20 static readonly reads of the two reached
  camera modules are inventoried; the missing three now have native backing.
  The MCC regression and28 camera checks/9 negative controls pass.
  [MCC native handoff regression](../.local/mcc-menu-match-handoff-20261019/cpu-camera-final.json),
  [Offline numeric authority](../.local/mcc-menu-match-handoff-20261019/camera-authority.json),
  [Camera ownership controls](../.local/mcc-menu-match-handoff-20261019/camera-contract-final.json).
- [x] Reuse the existing material library's lookup samples for native gameplay.
  Offline comparison proves the12 periodic and8 transition rows match TU1.5
  exactly. The retained container's final8 rows differ and are not gameplay
  lookups; the native binding preserves adjacent writable placement state.
  Complete admission precedes publication, preserves existing backing and is
  idempotent. Corrupt/overlapping controls and eight firing/effect ticks pass.
  [Function bank authority](../.local/mcc-menu-match-handoff-20261019/function-bank-authority.json),
  [Native firing regression](../.local/mcc-menu-match-handoff-20261019/cpu-function-bank-final.json).
- [x] Validate an actual MCC menu-to-match window, movement, a weapon shot,
  mixed gameplay audio and native frame. InstalledF40 in the requested SSD
  checkout; both actual no-argument launcher Play buttons pass through the
  title/profile/lobby/countdown into native Lockout and exit cleanly. Normal
  Play also verifies the bundled intro. Existing97 shaders are unchanged;
  no Xbox/Vista runtime input or MCC cache write is observed. The requested
  video retains actual game audio through a temporary dedicated capture sink,
  which is removed without changing the default output. This validates the
  offline Lockout route, not all maps/variants or full TU1.5 pixel parity.
  [Actual native frame/input/audio](../.local/mcc-menu-match-handoff-20261019/native-proof.json),
  [Both launcher Play paths](../.local/mcc-menu-match-handoff-20261019/installation.json).
- [x] Wire the MCC-only loading presentation into the existing renderer.
  On October20 the user explicitly chose an MCC-art reconstruction while
  retaining the completed animation. The imported MCC glow/logo produce an
  authored mask; all required draws remain. The separate MCC loading track is
  decoded into the private cache, with every stereo float sample matching the
  independently decoded stock-vgmstream reference, including the final overlap
  block. No Vista bank or borrowed menu music is used. The reconstructed mask
  does not claim exact Vista pixels or whole-game TU1.5 parity.
  [MCC loading integration](MCC_LOADING_PRESENTATION.md),
  [Complete PCM comparison](../.local/mcc-loading-delivery-20261020/audio-final.json).
- [x] Search the MCC installation more deeply for the original loading tone.
  October20 read-only checks cover63.3GB of stored data,43 Halo2 map
  directories and38,647 decoded candidates, including embedded/frontend,
  Halo2A/shared FMOD, CELT and quad-ADPCM follow-ups. The original reference
  recording is a positive control. Individual-channel/pitch/alignment checks
  do not verify the closer `monks1b` candidate as the same tone. No runtime or
  imported asset is replaced by this audit; disposable builds/media are removed.
  This finite coverage does not establish absence from every MCC version.
  [Ambience source audit](../.local/mcc-loading-ambience-20261020/report.json).
- [x] Identify the original loading tone in the installed MCC Halo2 Editing Kit.
  October21 read-only checks find the full18,396-byte reference payload,
  SHA2564f61e827be9b54b71e3d37a145c9b88fb32eaf517e23746af85236ca41ba8fc9,
  once in each of guerilla.exe, halo2_tag_test.exe, sapien.exe and tool.exe.
  Each entire match lies in file-backed read-only PE data; exact executable
  hashes and offsets are recorded. H2EK can therefore supply these same tone
  bytes for a future setup-only import. This answers the user's source question;
  dependency policy/importer adoption is a separate change. No PE runs, source
  changes, archive extraction or audio copies are required for identification.
  All checked inputs remain exact; scratch and the finished probe are removed.
  [Exact H2EK tone identity](../.local/mcc-gameplay-continuation-20261020/h2ek-loading-ambience-byte-search-v1.json)
  · [Probe retirement](../.local/mcc-gameplay-continuation-20261020/h2ek-loading-ambience-helper-retirement-v1.json).
- [x] Resolve the original four-voice loading ambience through the user's
  explicit October20 fallback authorization: bundle the original tone when
  a1-to-1 MCC reconstruction cannot be established. The18,396-byte resource
  is now authenticated in the repository, copied to`presentation/loading-v2`
  and played through the existing recovered four-voice owner. Original decoded
  PCM, four frequencies, envelopes, gain semantics, loops and teardown pass
  the source controls; no original installation is required. V1 remains for
  rollback. The earlier MCC loading music is superseded. Physical mixer parity
  and exact reconstructed-mask pixels are outside this bounded acceptance.
  [Original controls in the linked runtime](../.local/mcc-loading-original-tone-20261020/linked-audio-controls.json),
  [Fresh-clone CMake check](../.local/mcc-loading-original-tone-20261020/cmake-final/report.json).
- [x] Carry over existing verified CPU bitmap support exposed by loading into
  the match: raw10/11 and DXT1/DXT5 owners, constructor operands and checkpoint
  checks. The original sampling controls pass on actual MCC resources. Reuse
  the existing authenticated`41F064` reduction operand omitted by the old
  installed factory; all2,457 previous cold owners/480,190 bytes remain exact,
  with only that four-byte readonly owner added. No shader changes.
  [Bitmap constructor controls](../.local/mcc-loading-original-tone-20261020/texture-constructor-controls.json),
  [Factory preservation](../.local/mcc-loading-original-tone-20261020/factory-preservation.json).
- [x] Record the original ambience through the actual MCC menu/loading/native
  match window, including movement, firing, mixed sound and clean exit. Source
  tone bytes and recovered controls are exact; this is the offline Lockout
  route, not whole-game TU1.5 or physical-device parity.
  [Actual frame/video/audio](../.local/mcc-loading-original-tone-20261020/native-proof.json).
- [x] InstallF45 in the requested SSD checkout and verify both actual
  no-argument launcher Play buttons through title/profile/lobby/countdown,
  original ambience/loading, native Lockout, movement/firing and clean exit.
  All97 shaders are unchanged, and runtime uses the MCC cache without original
  Xbox/Vista inputs or cache writes. RetainF41 and its V1 bank for rollback.
  Latest Linux x86_64 binaries are staged in both designated output folders;
  normal setup/builds no longer depend on the superseded Wwise converter.
  [Both actual Play buttons](../.local/mcc-loading-original-tone-20261020/installation.json),
  [Latest Linux output](../.local/mcc-loading-original-tone-20261020/build-output.json).
- [x] Finish original-ambience validation cleanup: remove655,845,257 bytes
  of own superseded native ELF/shader copies, unused helpers and failed/control
  captures. All335 current providers, frozen source authority, selectedF45,
  rollbackF41/V1 bank, current frame evidence and the6.18MB requested video
  remain. The25 recorded scratch checks and final cleanup scratch are gone.
  Retained paths/sizes and the source policy exception are recorded in the
  [Delivery/cleanup receipt](../.local/mcc-loading-original-tone-20261020/closure.json).
- [x] Reconnect the existing animation,33ms presentation gate, fade and GPU
  renderer handoff through `DesktopSession`; loading follows actual resource
  preparation and its independent audio closes after the graphics join.
  Native atomic-admission controls, six importer/archive/mask controls and
  current-header syntax checks pass. Existing97 shaders are unchanged.
  [Native assets](../.local/mcc-loading-delivery-20261020/cpu-admission.json),
  [CMake validation](../.local/mcc-loading-delivery-20261020/cmake-final/report.json).
- [x] InstallF41 in the requested SSD checkout and verify both actual
  no-argument launcher Play buttons reach native Lockout through the loading
  presentation. The visible rings/sweep animate; loading and gameplay PCM are
  finite and nonzero; movement/firing and clean exit pass. No Xbox/Vista input
  or MCC cache write occurs during runtime. RetainF40 as rollback, record the
  requested video with actual sound, and keep the native match logic wired to
  regular Play. Native-test capability remains separate from full parity.
  [Actual window/video/audio](../.local/mcc-loading-delivery-20261020/native-proof.json),
  [Both launcher Play buttons](../.local/mcc-loading-delivery-20261020/installation.json).
- [x] Include the converter, codebooks and BSD source notices in source and
  compiled-package workflows; stage the latest Linux x86_64 native/converter
  binaries in the designated output directories. Fresh imports preflight
  loading dependencies before the large copy, and existing caches receive a
  transactional loading-bank repair. Customized banks are retained. Other
  platforms remain skipped.
- [x] Clean up the native handoff validation after installation. Removed
  1,090,018,061 bytes of own superseded binaries, replaced object providers,
  duplicate shader copies and obsolete one-off helpers. All333 current
  providers, frozen sources, original/reference assets, F34 rollback and the
  selectedF40 remain. Owned scratch and temporary recording sinks are removed;
  retained output is the installed build, current build authority,8.44MB of
  required frame evidence and the requested2.57MB video.
  [Native handoff cleanup](../.local/mcc-menu-match-handoff-20261019/closure.json).
- [x] Remove obsolete disposable frontend test/build artifacts after auditing
  both checkouts' launcher selections, active processes, tracked/link owners and
  the latest329 physical providers. Removed26 superseded raw binaries/Painter
  objects totaling1,548,816,432 bytes. Frozen sources, metadata, reference
  evidence, selected/rollback runtimes, all assets and current providers remain.
  Owned CMake/probe/capture trees are removed; final retained artifacts and
  remaining scope are recorded in the closure receipt.
  [Cleanup audit](../.local/mcc-menu-row-layout-20261019/cleanup.json),
  [Final closure and retained artifacts](../.local/mcc-menu-row-layout-20261019/closure.json).
  Closure verifies all recorded scratch trees absent and retires21 superseded
  probe helpers. Retained: the current124,280,195-byte coherent build authority,
  the108,008,740-byte installed F33 directory, and40,544,353 bytes of requested
  recordings/required reference images, with individual paths and hashes in
  the receipt. Unrelated runtime selections and all current link inputs remain
  intact. Both launchers still select and admit F33 after source installation.


- [x] Diagnose the disabled Play button in the newly cloned SSD checkout.
  Git tracked a local `.local/f24` frontend selection while ignoring the runtime
  it selected. Both checkouts now ignore local runtime selection records and
  stage only removal of that tracked record; the actual local files remain.
  No readiness or capability guard is relaxed.
- [x] Install the existing compiled V24 menu runtime and unchanged 97-file
  shader bank in `/mnt/SSD4TB/Halo-2-Decompiled/.local/f24/`. Only its authenticated
  read-only shader path field is rebound; instructions and shader bytes stay
  exact. Actual native title/profile/create/confirm/main/Settings/back navigation
  and finite nonzero mixer audio pass against the user's currently selected
  `mcc-38f68db024074a628df2f2f1c1beca68` cache. That cache's changed menu/shared/
  texture hashes are admitted by this new actual validation, never by copying
  an old sidecar. The cache selection is preserved and no MCC copy is repeated.
  [SSD native frontend acceptance](../.local/mcc-ssd-play-repair-20261019/direct-acceptance.json).
- [x] Verify actual no-argument normal and title-first Tk Play in the SSD
  installation, including cached intro/skip, menus and pointer Back; retain
  the requested installed runtime and bounded references, then remove scratch.
  Both actual installed Tk Play clicks pass and exit zero. Normal Play decodes
  the exact cached intro with sound; title-first skips it. Title/profile/create/
  confirm/main/Settings/back and a real mouse BACK click pass with finite
  nonzero audio. Launcher info reports the selected frontend available and
  Play opening MCC title/menus. Retain 107,082,886 bytes of installed runtime/
  shaders/sidecar, the 243,856-byte title-first launcher and two required PNGs
  totaling 319,363 bytes. Scratch and the one-off installer helper are removed.
  The cache selection and existing ROOT installation remain intact. The
  separate gameplay test/runtime and full parity remain unadmitted here.
  [Completed SSD installation](../.local/mcc-ssd-play-repair-20261019/closure.json).

- [x] Reproduce and correct the late fresh-import graphics failure on
  `03b_newmombasa.map`. The installed MCC source matches the pinned original;
  the earlier verified instant-charge tag conversion changes its fingerprint.
  Graphics now accepts only the original or the exact in-memory original/output
  hash pair produced by that earlier pass. All bitmap descriptor, donor/target
  mip and delta guards remain intact. The actual MCC reproduction now completes
  all six corrections/48 mips and preserves the gameplay flag. All 72 focused
  Python checks pass, including wrong/stale revisions and damaged donor mips;
  independent review passes. The two changed importer tools are installed in
  the user's SSD checkout. Actual correction scratch is removed.
  [Actual MCC correction](../.local/mcc-graphics-import-order-20261019/actual-correction.json),
  [Independent review](../.local/mcc-graphics-tag-revision-review-20261019/v1/review.json).
- [x] Complete the actual SSD fresh setup/import with that fix and verify the
  selected cache/provenance. The actual setup CLI exits zero after 883 copied
  MCC files, 43 map conversions, both movies/custom prompts and all six graphics
  corrections/48 mips. The graphics donor hash matches the preceding gameplay
  conversion output. Setup selects
  `assets/imported/mcc-8bfe3222bfc94b5ea5577f803f3ea9c5` in the SSD checkout.
  Its unfinished stage and owned runner scratch are removed. MCC originals,
  existing native runtime/shaders and unrelated checkouts are preserved.
  Full gameplay/parity remains a separate goal gate.
  [Actual fresh SSD import](../.local/mcc-graphics-import-order-20261019/full-import.json).
- [x] Confirm the actual SSD setup reuses that completed cache after verifying
  copied source/output hashes, without another copy or cache mutation.
  The first reuse check found a genuine mismatch in two small ranges of the
  private `preload/paks/shared.pck`; the installed original still matches its
  provenance hash. Automatic recopy was cancelled and its owned 11,356,433,551
  bytes removed. Only the demonstrated 524,288-byte ranges were restored from
  the original. The complete 13,149,688,019-byte file now passes a full checksum
  after flush and disk readback. The original and provenance remain unchanged;
  the second actual setup CLI exits zero and verifies all 883 copied source/
  output records, movies and custom prompts without copying again. The same
  selection and all 45,122 cache files retain their sizes/mtimes. No new import
  stage remains; runner scratch and diagnostic helpers are removed. Retain the
  user-requested completed cache (36,494,063,454 bytes). The SSD launcher reports
  the cache prepared but no built game runtime; native installation remains a
  separate gate. This does not complete mod conversion or full gameplay parity.
  [Copy diagnostics](../.local/mcc-graphics-import-order-20261019/shared-pack-durability.json),
  [Exact copy repair](../.local/mcc-graphics-import-order-20261019/shared-pack-repair.json),
  [Cancelled recopy cleanup](../.local/mcc-graphics-import-order-20261019/canceled-recopy-cleanup.json),
  [Actual cache reuse](../.local/mcc-graphics-import-order-20261019/reuse.json).

- [x] Measure the menu prompt mismatch against the authenticated custom-art
  source. Actual profile/main/Settings quads use selector 1 controls at 44x50
  logical pixels instead of 32x36. At the existing 0.4 canvas scale they render
  17.6 pixels wide and clip vertically; the authored ink should be 12.8x14.4.
  Ordinary ASCII metrics and font ascent/descent match. Source tag bounds and
  text alignment determine placement; the canvas scale needs no adjustment.
  Actual quads and pointer targets are recorded at 640x480 and 1280x720.
  [Independent menu expectations](../.local/mcc-frontend-custom-prompts-20261019/menu-expected-geometry-v1/expected.json),
  [Emitted menu quads](../.local/mcc-frontend-custom-prompts-20261019/native-geometry-v1/menu-quad-validation.json).
- [x] Correct only the qualified menu control metrics, preserving ordinary
  text, source anchors, baseline, clipping rules and existing shader samples.
  Verify complete custom glyphs, expected sizes/positions and matching click
  targets in profile, main and Settings across standard and widescreen views.
  The corrected V23 runtime passes actual title/profile/main/Settings/back
  keyboard and mouse navigation with finite, nonzero menu audio. Both required
  native reference images were viewed; complete SELECT/BACK artwork and text
  fit their panes/footer. The bounded Lockout firing/look/movement regression
  also exits cleanly. All 325 parent inputs and 97 shaders are preserved; the
  successor substitutes only the two admitted helper/painter leaves. Scratch is removed;
  retain the requested coherent candidate (118,332,732 bytes) and two required
  comparison PNGs (180,872 bytes). Full menu-tree and framebuffer parity remain
  separate. Actual installed Play is the next gate.
  [V23 native handoff](../.local/mcc-frontend-custom-prompts-20261019/native-menu-metric-v1/handoff.json),
  [Root review](../.local/mcc-frontend-custom-prompts-20261019/native-menu-metric-v1/root-closure.json).
- [x] Implement and component-test the menu metric correction for exact
  selector 1/7 owners. All 80 menu/HUD profile rows and atomic/idempotence guards
  pass the focused CMake test. Repeated selector 7 independently matches the
  same source font and samples. The painter applies this only to the default
  imported authored bank; ordinary text and the 0.4 canvas remain unchanged.
  The private build is removed. Actual emitted positions and native Play
  acceptance follow separately.
  [Menu component checks](../.local/mcc-frontend-custom-prompts-20261019/menu-metric-implementation-v1/regular/report.json).
- [x] Verify corrected emitted menu geometry against independent source-derived
  expectations. All 14 foreground controls and 14 shadows at 640x480 and
  1280x720 retain complete custom samples, baselines, alignment and four-edge
  Accept/Back click bounds. Maximum comparison error is 0.00011 pixels. The
  profile caption now fits its 245-unit pane (227 rather than 251 units),
  restoring the previously clipped BACK text without changing source anchors.
  CPU and review scratch are removed. Native rendering acceptance follows.
  [Corrected CPU witness](../.local/mcc-frontend-custom-prompts-20261019/native-menu-metric-v1/cpu-handoff.json),
  [Independent geometry review](../.local/mcc-frontend-custom-prompts-20261019/menu-expected-geometry-v1/corrected-review.json).
- [x] After prompt acceptance, bind the passed native title/menu runtime to
  the installed Play action and test the actual button-to-title-to-menu route.
  Preserve the separate native gameplay test and prior runtime for rollback.
  V2 actual direct-host menu/Lockout and both no-argument Tk Play routes all
  pass, including cached intro/skip, finite nonzero audio and a real Settings
  B/BACK click. Actual default-window reference images were viewed. Its final source
  guard rejected concurrent diagnostic-only C++ and stale-selection UI edits;
  the original RED receipt is preserved and the owned stage/scratch removed.
  Source review qualifies the absent-registry context. Adoption will reconstruct
  the exact tested native bytes, reuse those native results and verify fresh
  current-UI Play routes; unrelated live source edits must not trigger repeated
  engine builds. The subsequent actual installed normal and title-first Tk Play
  clicks both pass title/profile/create/confirm/main/Settings/back with finite
  nonzero audio and clean exits. Normal Play uses the cached intro and a real
  Settings B/BACK click. The independently selected V24 frontend preserves the
  corrected prompt leaves and the existing gameplay runtime/106 shader files;
  its own 97 shaders, cache, saves and selections remain unchanged. Both current
  default-window images were reviewed. Scratch is removed; retain two required
  reference PNGs totaling 319,740 bytes. This accepts the stated route, not the
  full menu tree or complete gameplay/framebuffer parity.
  [Installed Play acceptance](../.local/mcc-frontend-custom-prompts-20261019/installed-play-validation-v1/closure.json),
  [Actual V2 routes and cleanup](../.local/mcc-frontend-custom-prompts-20261019/root-play-installation-v2/actual-attempt-closure.json),
  [Reviewed source deltas](../.local/mcc-frontend-custom-prompts-20261019/root-play-installation-v2/root-drift-review.json).
- [x] Restore the missing bundled movie bank in the user's SSD checkout.
  The exact reported metadata error was reproduced: the entire
  `resources/movies` directory was absent. Both original movies and the bank
  manifest are now copied with full payload hashes verified and an atomic
  no-replace directory publication. The tracked outer manifest is unchanged;
  the actual target source-manifest check passes. Retain 2,538,779,177 bytes of
  required assets; owned copy scratch is removed. Full fresh MCC import remains
  a separate gate.
  [Movie-bank repair](../.local/mcc-movie-clone-repair-20261019/repair.json).
- [x] Verify the complete SSD presentation source preflight using that
  checkout's actual importer helpers. Both movie manifests and extents pass;
  the previous full movie hashes remain authoritative. All 91 custom prompt
  files (19,865,846 bytes), including 82 SVG masters, authenticate successfully.
  No import/cache or index changes occur; scratch and the one-off helper are
  removed. This accepts presentation inputs, not a complete fresh MCC import.
  [SSD presentation preflight](../.local/mcc-movie-clone-repair-20261019/presentation-preflight.json).
- [x] Track both bundled movies with Git LFS and both manifests in normal Git;
  verify small index pointers and authentic local payloads in both source
  checkouts so the required movie bank can be pushed and fetched by clones.
  Both ROOT and SSD checkouts stage exactly the two MP4s, bank manifest,
  `.gitattributes`, `.gitignore` and release-movie instructions. Each MP4 index
  blob is a 135-byte LFS pointer with the manifest's exact SHA/size; all four
  local LFS payload objects pass full hashes. The already tracked outer
  manifest stays unchanged. Local hooks/filters are installed without altering
  unrelated index entries or non-LFS config. No commit or remote push occurs.
  Retain 2,538,778,741 bytes of required LFS payloads per checkout, in addition
  to the working assets; runner scratch is removed.
  [LFS staging verification](../.local/mcc-movie-lfs-20261019/validation.json).
- [ ] Publish the compiled portable V23 ZIP with both real movies, paired
  manifests and the authenticated custom prompts. The first V3 attempt stopped
  before native execution when replacing an inherited read-only helper file;
  no ZIP was published and scratch is removed. Its RED receipt is retained.
  V4 replaces only owned extracted helper files, preserving frozen inputs.
  The actual read-only failure/replacement control and four publication controls
  pass, with independent source review. Actual relocated native/Tk acceptance
  and verified ZIP publication remain open.
  The later actual V4 attempt reached native/launcher checks but stopped at its
  development-path dependency guard and published no ZIP. The next package
  must also include the current tag-revision graphics/importer fix; the frozen
  V23 tool bank predates it. Preserve the earlier failed receipts.
  [V3 failure closure](../.local/mcc-frontend-custom-prompts-20261019/delivery-v23-preparation-v3/failure-closure.json),
  [V4 controls](../.local/mcc-frontend-custom-prompts-20261019/delivery-v23-preparation-v4/publication-validation.json).
- [x] Adopt the exact tested runtime after that reviewed source drift. V3
  reconstructs the same native SHA and unchanged 97-shader bank, retains the
  actual V2 native acceptance, and passes fresh current-UI normal/title-first
  Tk Play routes with sound. Normal Play uses the cached intro and a real
  Settings B/BACK click; title-first skips the intro. Original runtime, saves,
  cache, selectors and renderer workbench remain intact. Root verified the
  inventory, viewed the fresh Settings image and confirmed scratch removal.
  Retain the requested staged runtime/bank/sidecars (107,043,373 bytes) and
  required reference PNGs (316,781 bytes). Final root launcher replacement is
  authorized separately after this completed acceptance milestone.
  [Accepted V3 adoption](../.local/mcc-frontend-custom-prompts-20261019/root-play-installation-v3/staged-validation.json).
- [x] Qualify the final launch-test lifecycle before running that integration.
  The shared flow inherits the test runner's process group and isolates HOME,
  XDG, saves, GPU caches and raw audio. Exact descendant cleanup and forced
  owner SIGKILL controls pass: traced children and Xvfb stop, and all scratch
  is removed. The source routing review also confirms default title screen 9,
  normal intro playback, title-first intro skip and separate Lockout admission.
  These checks prepare actual Play acceptance; they do not claim installation.
  [Flow lifecycle controls](../.local/mcc-frontend-custom-prompts-20261019/root-play-installation-v1/lifecycle-validation.json),
  [Independent launcher routing review](../.local/mcc-frontend-custom-prompts-20261019/menu-expected-geometry-v1/root-play-routing-review.json).
- [x] Recheck concurrent launcher selector changes before installing Play.
  The current optional frontend selector preserves single-runtime admission
  when no independent selection exists and rejects invalid explicit owners.
  Four focused CMake groups pass: frontend capability, selector, compiled
  launcher routing and Tk home UI. Source hashes remain stable and scratch is
  removed. An initial validation command included the default engine dependency;
  it was cancelled and cleaned, then corrected to disable that dependency.
  Refresh the small compiled launchers to these matching helper semantics;
  no native engine rebuild or new frontend registry is required.
  [Current launcher checks](../.local/mcc-frontend-custom-prompts-20261019/concurrent-route-check-v3/root-closure.json).
- [x] Refresh the small root and portable launcher inputs to the current
  selector-aware source. All four compile and configuration checks pass with
  pinned source/compiler/headers; prior inputs and concurrent source edits are
  preserved. The two root binaries total 7,342,200 bytes. Private compilation
  scratch is removed. Actual title/menu adoption uses these V2 inputs and the
  accepted V23 native runtime; it remains the next acceptance gate.
  [Current root launcher inputs](../.local/mcc-frontend-custom-prompts-20261019/root-play-launchers-v2/handoff.json).
  The designated Linux output folder now also contains these two current
  launcher binaries; the V23 native and onboarding output hashes are preserved.
  All four root/portable launcher inputs total 14,675,568 bytes; compile and
  restaging scratch are removed.
  [Current input/output closure](../.local/mcc-frontend-custom-prompts-20261019/root-play-launchers-v2/closure.json).
- [x] Prepare an isolated local shader-bank binding for that launcher runtime.
  The existing ELF relocation helper can now bind a canonical absolute owner
  within the same read-only, fixed-width path field, preserving instructions
  and shader files. This avoids replacing the active workbench's shader bank.
  All three focused relocation/gameplay/frontend admission groups pass; both
  private builds are removed. This is preparation only: no native runtime has
  been relocated or installed by this helper check.
  [Local binding checks](../.local/mcc-local-frontend-owner-20261019/checks-v2/report.json).
  The host's 156 linked libraries also match the accepted native test library
  bytes exactly; no extra loader bundle is needed for this machine. That
  read-only check leaves no scratch and does not substitute for actual Play.
  [Host dependency identities](../.local/mcc-local-frontend-owner-20261019/host-dependencies.json).
- [x] Compile the two root launcher inputs for that isolated runtime owner.
  Normal and title-first binaries use the current verified routing source,
  host Python and `.local/f22`; their configuration checks pass. Retain
  7,224,640 bytes of requested compiled launcher inputs plus frozen source.
  Scratch is removed and installed launchers/runtime/selection remain intact.
  Actual Play adoption still waits for corrected native menu prompt acceptance.
  [Compiled root launcher inputs](../.local/mcc-frontend-custom-prompts-20261019/root-play-launchers-v1/handoff.json).
- [x] Refresh the designated local Linux x86_64 outputs with the accepted V23
  runtime and both root launcher inputs. The existing staging helper publishes
  exact source hashes in `dist/builds/linux-x86_64/build-output.json`; the three
  binaries total 112,860,152 bytes. The onboarding entry and other platform
  outputs are preserved. Scratch is removed. These are local compiled outputs;
  installed Play adoption and the relocatable ZIP remain separate gates.
  [V23 local output receipt](../.local/mcc-frontend-custom-prompts-20261019/designated-output-v23/handoff.json).

- [x] Load the title screen, menu graphics, text/fonts and audio from the
  port-owned imported MCC cache. Reuse existing frontend code and shaders;
  do not require a separate Vista asset bank or read an Xbox ISO at runtime.
- [x] Locate and authenticate the existing authored SVG button-prompt masters
  and exported bitmaps, including their selector/codepoint provenance. The
  earlier82-master/331-binding/six-atlas pack is described in
  [VISTA_PROMPTS_4K.md](VISTA_PROMPTS_4K.md). Its preserved relocated resources
  match the original174 artwork files and175 sealed bundled nonreference
  files. The reconstruction metadata differs only by the documented bundler
  path rewrite and matches that sealed bundle. All12 MCC font selector names
  match the authored selectors exactly. The runtime manifest/six atlases total
  19,354,278bytes; no original fonts or regeneration are needed. All discovery
  scratch is removed, with no asset copies made by that audit.
  [Authenticated custom prompt inventory](../.local/mcc-frontend-custom-prompts-20261019/inventory-v1.json).
- [x] Restore the exact authored source bundle to portable
  `resources/presentation/prompts/`. Its91 files retain19,865,846bytes: the
  seven runtime files,82 SVG masters, reconstruction provenance and the new
  sealed source manifest4255eceb. All90 payload files authenticate against the
  delivered bank; no reference fonts are copied or artwork regenerated. These
  are required retained custom assets. The selected cache is untouched by this
  source restoration; import and actual display still need separate checks.
  [Custom artwork restoration](../.local/mcc-frontend-custom-prompts-20261019/source-restoration-v1.json).
- [ ] Qualify fresh/reused imports and the compiled distribution with the
  authenticated native MCC control-font owners. Preserve existing custom
  prompt files as references; do not copy or require their separate bank.
- [x] Repair the actual selected root cache's authored prompt bank. All nine
  files/19,372,296bytes match their complete source hashes, source manifest
  4255eceb and self-relative binding. Only `port_authored_prompts` is added to
  provenance; cached/live MCC tree metadata, both movie identities, selected
  runtimes/launchers, shaders, registries and saves remain unchanged. Actual
  traced writes stay in the prompt bank, provenance update and shared lock.
  No31GB rehash/reimport or P-cache copy occurs. Scratch is removed; these nine
  required cache assets are explicitly retained. Native rendering follows.
  [Actual prompt import](../.local/mcc-frontend-custom-prompts-20261019/actual-root-import-v1/validation.json).
- [x] Implement and verify the fresh/reused prompt import contract. The
  source bank has82 exact SVG masters plus authored atlases and provenance;
  only nine runtime files enter `presentation/prompts/`, using a portable
  self-relative binding. All53 focused prompt/onboarding/preparation controls
  pass. An actual authenticated bank copy into owned scratch verifies all
  19,372,296bytes against source/destination hashes. The importer shares the
  presentation lock, resumes matching partial copies and preserves custom or
  conflicting data. All four owned scratch trees are removed. Actual selected
  cache repair, distribution and visible rendering remain separate gates.
  [Prompt importer handoff](../.local/mcc-frontend-custom-prompts-20261019/handoff-v1.json),
  [Actual private bank copy](../.local/mcc-frontend-custom-prompts-20261019/validation-real-bank-v1.json).
- [ ] Verify the native MCC prompt glyphs in actual title/menus and gameplay
  HUD screens, preserving their authenticated pixels, metrics, geometry and
  active keyboard/controller bindings. Qualify the delivered widescreen route;
  historical custom-atlas captures do not establish the changed glyphs' appearance.
- [x] Qualify actual MCC glyph ownership before applying the earlier redraws.
  All331 selector/name/codepoint/source-record bindings match MCC. Of those,
  178 have different raw dimensions across eight selectors; for example,
  Conduit12 E100 is44x50 in MCC versus the historical32x36 source and128x144
  custom raster. The pack's fourfold density is relative to its authenticated
  historical source, rather than every raw MCC glyph. A too-strict test caught
  that assumption; no artwork or logical metrics were changed. Revised component
  checks must verify historical density, exact bitmap samples and unchanged MCC
  metrics/lookup/clipping. Native display remains an open gate.
  [Measured prompt dimensions](../.local/mcc-frontend-custom-prompts-20261019/cpp-integration-v1/actual-dimensions.json).
- [x] Independently review the frozen prompt consumer changes. The default
  cache bank is selected independently of optional UI/font replacement;
  relative bindings resolve from their bank, and the sampler retains logical
  metrics and input bindings. Repeated Conduit selectors share identical
  atlas hashes, rectangles and vectors, so existing material deduplication
  remains valid. The new native button branch reads MCC fonts, with no new
  Vista font dependency. This is source/artifact review, not a rendered proof;
  its scratch is removed and no additional decoding or compile is performed.
  [Independent prompt consumer review](../.local/mcc-frontend-custom-prompts-20261019/independent-cpp-review-v1/review.json).
- [x] Validate the compatible prompt sampler components against actual MCC
  fonts and the authenticated custom rasters. All331 controls across12 raw
  selectors and three projected HUD owners preserve metrics, lookup, kerning
  and34,152,448 ordinary atlas bytes while selecting exact custom bitmap bytes.
  Relative/absolute bindings, changed CWD, aliases and malformed inputs are
  covered. Regular and ASan/UBSan probes pass. All five focused CMake groups
  pass, including the three fresh/reused import registrations. Scratch is
  removed and no large test outputs are retained. Actual native consumer
  constructors, button rectangles/X-height and GPU display remain separate.
  The selected-compatible HUD adoption must exclude only the unrelated
  pre-existing `reset_lifetimes` method privately, preserving product source.
  [Prompt component checkpoint](../.local/mcc-frontend-custom-prompts-20261019/cpp-integration-v1/handoff.json).
- [x] Construct actual selected-compatible MCC prompt consumers on CPU. The
  default Painter, full41-widget/624-material HUD and Scoreboard select exact
  custom textures/UVs from a private full056 cache view. Parent/patched menu
  vertex positions and title/main/Settings pointer/back behavior match; both
  processes exit0. Four controlled action-renderer inputs use the real
  `training_reload` row, rather than an actual SMG pickup. The real stock SMG
  row is exercised separately and has the same pre-existing token gap in both
  versions. All324 parent inputs and cache metadata remain exact; private
  compilers/binaries/cache views are removed. GPU display is still unverified.
  [Native CPU consumer checkpoint](../.local/mcc-frontend-custom-prompts-20261019/selected-consumers-v1/handoff.json).
- [x] Run the native title/menu path with imported custom prompt atlases and
  the cached bundled intro. The frozen 325-input candidate exits cleanly after
  actual X11 focus, intro skip, title/profile/keyboard/main/Settings/back input
  and 259 rendered frames. Title-only music and overall native mixer samples
  are finite and nonzero. The 97 shaders and parent inputs stay unchanged;
  owned runtime scratch is removed. This validates the basic frontend only:
  dedicated gameplay control metrics, stock pickup tokens, complete menus and
  Xbox framebuffer/audio parity remain separate. Cold window readiness took
  80.9 seconds, so this is not a startup-performance fix. Final video review
  and retention are recorded below; launcher/package adoption remains separate.
  [Native custom-prompt frontend run](../.local/mcc-frontend-custom-prompts-20261019/native-frontend-v1/validation.json).
- [x] Seal the basic frontend candidate and requested video with sound. The
  final cohort pins the immutable passed receipt; independent root checks
  verify all handoff hashes and that acceptance link. The compatible 20-second
  H.264/AAC video has 240 frames and finite nonzero stereo audio. Visual review
  confirms the bundled intro, title, profile chooser and main menu; Settings is
  proven by the input trace. Retain 125,604,679 bytes of coherent compiled inputs
  and 3,053,915 bytes of requested video/source visual evidence. Three owned
  capture/QA scratch trees are removed. No capability, installation or ZIP is
  published by this checkpoint.
  [Sealed frontend/video handoff](../.local/mcc-frontend-custom-prompts-20261019/native-frontend-v1/handoff.json),
  [Independent closure](../.local/mcc-frontend-custom-prompts-20261019/native-frontend-v1/root-closure.json).
- [x] Build and test the dedicated-control metric helper against actual MCC
  font inputs. The focused CMake test passes the 40-row profile, selector 5
  no-op, selector 6 corrections, idempotence and malformed/partial-owner guards,
  while preserving ordinary metrics, samples, atlas pixels, lookup and kerning.
  The helper is called only for private dedicated action-button fonts after
  authored sampling. Its private build is removed. Selected-compatible HUD
  geometry and native adoption remain unchecked.
  [Metric component validation](../.local/mcc-frontend-custom-prompts-20261019/geometry-implementation-v1/regular/report.json).
- [x] Preserve the prior authored dedicated-button geometry with an exact
  source-qualified logical-metric projection where MCC differs. The helper
  qualifies all 40 control records and authored samples before changing the
  16 differing selector 6 tuples; selector 5 and repeated projection are no-ops.
  Six selected-compatible MCC Cache/HUD cases confirm controller X, F and Space
  geometry against the independent source profile. Ordinary fonts, uploaded
  texture bytes, active input policy and shaders remain unchanged; 27 malformed
  or partial-state controls reject before mutation. No reference fonts ship.
  All owned scratch is removed, with no probe binaries retained. This is CPU
  geometry acceptance; native cohort adoption, visible gameplay prompts and
  Xbox framebuffer parity remain separate gates.
  [Dedicated HUD metric handoff](../.local/mcc-frontend-custom-prompts-20261019/geometry-implementation-v1/handoff.json).
- [x] Adopt the exact helper and HUD provider together in a coherent native
  runtime. V22 passes actual title/profile/main/Settings/back with unmuted audio,
  and real Lockout focus/click/held-fire/look/movement through 319 frames and
  366 ticks with a clean exit. All 325 parent inputs, 97 shaders and selected
  cache are preserved. Retain 114,415,463 bytes of coherent compiled inputs;
  test scratch is removed. The independently measured menu-control sizing
  issue still blocks prompt acceptance and Play/capability/package publication.
  [Native HUD adoption](../.local/mcc-frontend-custom-prompts-20261019/native-geometry-v1/handoff.json).
- [x] Extract and qualify the dedicated-control metric profile before changing
  native sizing. All 40 control records match the authenticated artwork
  owners. Selector 5 needs no metric changes; selector 6 has 16 differing
  tuples. The 95 printable ASCII glyphs and complete kerning tables match
  both historical owners, so a control-only projection can preserve the
  existing keyboard-label sizing policy. This is offline metadata evidence,
  with no product edits or reference fonts shipped. Implementation and native
  rendering remain unchecked; owned scratch is removed.
  [Qualified metric profile](../.local/mcc-frontend-custom-prompts-20261019/geometry-profile-v1/handoff.json).
- [x] Normalize the qualified English MCC gameplay action templates into the
  original native token namespace. The 40-map census pairs 4,705 actual fields
  against 36 original maps and qualifies 112 complete templates. Nine MCC-only
  templates preserve their wording/icons while using witnessed action prefixes.
  The formatter matches the 121 complete templates before local key/button
  substitution. Original E45A thumbstick tokens and raw locale banks remain
  intact. General callbacks, other languages, live interaction admission and
  framebuffer acceptance remain separate gates.
  [Source-qualified action-template reference](../tests/fixtures/mcc_action_prompt_templates.json),
  [implementation and validation boundary](HUD_ACTION_PROMPTS.md#binding-defaults-and-input-selection-2026-10-20).
- [x] Historical October19 policy, now superseded: required the custom prompt
  bank in frontend capability admission. All nine consumed manifest/binding/
  atlas inputs and any extra bank files had to match the passed inventory under
  `presentation/prompts/`; omitted, changed, deleted and unqualified extra atlas
  controls rejected. Seven frontend controls,
  all34 compiled launcher routing controls and22 updater controls pass through
  three focused CMake groups. Scratch is removed. These metadata checks do not
  prove actual prompt import or visible rendering.
  [Custom prompt admission checks](../.local/mcc-frontend-launcher-20261019/custom-prompt-resource-v7/closure.json).
  An independent current-cache size control also passes unchanged:52 consumed
  resources plus97 shaders need about15–16KiB, within the64KiB sidecar bound.
  The copied561 DDS pages remain inactive without `ui/pack.tsv`; the launcher
  already guards that optional overlay correctly. A hypothetical oversized
  future pack does not justify changing the current bound. Both audit scratch
  trees are removed.
  [Current receipt-size check](../.local/mcc-frontend-custom-prompts-20261019/sidecar-size-review-v2/review.json).
- [ ] Validate complete fresh and reused-cache imports before changing the
  final data-path layout. Cover copied MCC files, required converted vanilla
  gameplay tags, mod `tags`/`data` inputs and authenticated MCC presentation assets,
  including interruption/resume and actual native consumption. A helper fixture
  alone does not pass this import gate.
- [x] Audit current path preservation before proposing that later change.
  Raw copying already preserves selected Halo2-relative names, case, nested
  `tags`/`data`/mod subtrees and empty directories under `cache/halo2`.
  Current offline map conversion only enumerates top-level `.map` files;
  copied nested mod maps/loose tags and mod-owned shared-resource contexts need
  separate conversion/lookup coverage. External Workshop roots are not yet
  enumerated. Derived hash-named bitmap/UI catalogues are separate port caches,
  rather than a flattened replacement for the donor tree. This read-only audit
  changes no data paths and leaves the full import gate open; scratch is removed.
  [Canonical path audit](../.local/mcc-canonical-path-audit-20261019/read-only-v1/validation.json).
- [x] Identify the remaining complete-import acceptance boundaries. Current
  cache metadata covers 883 copied files and 43 maps, with no nested mod maps
  or loose tag inputs. Small controlled copies preserve nested/case-sensitive
  paths, but conversion still uses basenames and one standard resource context.
  Reuse verification authenticates copied outputs without proving a complete
  required-conversion set. This read-only audit does not rehash the real cache
  or run gameplay; owned scratch is removed and no data paths change.
  [Import acceptance audit](../.local/mcc-import-gate-audit-20261019/read-only-v1/review.json).
- [x] Require exact stock-map conversion ownership before publishing or reusing
  an import. Previously an unchanged map could lose its conversion result and
  still pass reuse through the copied-source hash fallback. Every stock map now
  needs one result bound to its original copied donor; missing, duplicate,
  foreign, nested-basename and stale-source owners reject. Fresh imports record
  explicit coverage, including preserved additional maps and loose tags/data
  whose conversion remains unqualified. Both selected caches pass all43 stock
  owners and eight actual-manifest rejection controls; six focused CMake groups
  pass, including32 onboarding and22 preparation cases. No cache payload,
  shader, selection or runtime changes are made by this acceptance. Full mod
  conversion, native consumption and complete semantic parity remain open.
  Owned scratch is removed; only small reports/logs are retained.
  [Conversion-owner validation](../.local/mcc-conversion-coverage-20261020/closure.json).
- [x] Make the CLI/main-thread fresh-import transaction cancellation-safe,
  including stage creation, copying, map conversion and nested presentation
  preparation. Actual SIGTERM controls now remove incomplete stages, preserve
  source/registries/prior caches and permit a clean retry. A completed published
  cache survives termination after its rename. All 21 preparation controls and
  four focused CMake groups pass; source pins remain stable across validation.
  Seven owned scratch trees are removed and no large artifacts are retained.
  Concurrent multiplayer-catalog work is preserved but is outside this fix's
  acceptance scope. The GUI worker lifetime is a separate unchecked gate.
  [CLI cancellation handoff](../.local/mcc-import-cancellation-20261019/handoff.json).
- [x] Give Linux GUI onboarding an equivalent cancellation/cleanup owner.
  Default imports now run in an owned CLI child with bounded streamed progress;
  its main-thread cancellation covers setup/select, including reuse repairs.
  Nineteen focused controls pass: real GUI termination and owner-pipe EOF,
  interruption during process creation, callback/window teardown, clean retry,
  portable Python paths and exact handler restoration. Existing controls and
  injected-function tests remain valid. Both owned scratch trees are removed;
  no real cache, registry or runtime changes occur in these tiny fixtures.
  Complete fresh/reused import and packaged GUI acceptance remain open.
  [GUI import lifetime handoff](../.local/mcc-setup-gui-lifetime-20261019/validation-v1/handoff.json).
- [ ] Qualify relative map identities and each map's shared, texture and audio
  dependencies before widening conversion enumeration. Cover duplicate mod
  basenames and nested paths without conflating resource owners. Define required
  conversion coverage and loose-tag/data producer admission; use that coverage
  in fresh/reused verification and actual native-consumption acceptance.
- [x] Preserve distinct relative mod owners in the existing asset-viewer map
  list. Stock resource-name exclusion now applies only in the actual stock
  map directory; nested `mainmenu`, `shared` and `single_player_shared` map
  names retain their relative paths and case. Seven duplicate/nested-name
  controls, external-link rejection, unchanged metadata and incomplete
  gameplay-readiness controls pass the focused onboarding CTest. Exact hunk
  reversal preserves all unrelated dirty changes. Owned CMake/test/cache
  scratch is removed; only small reports, preimages and the patch are retained.
  This corrects preview choices only; conversion/resource-context/gameplay
  admission and installed tool delivery remain separate acceptance gates.
  [Relative mod-preview owner fix](../.local/mcc-mod-preview-paths-20261020/proof-v1.json).
- [x] Install only that tested map-list function change into the SSD checkout,
  preserving its different surrounding setup/repair implementation. Exact
  whole-file reversal and function equality pass. The actual installed module
  passes an owned filesystem fixture; real launcher-info still selects F48 and
  reports MCC-only menus available. All180 protected files, including57 cache
  metadata files,97 shaders, both build outputs and v23 ZIPs, retain their
  bytes and identity. Both owned scratch/cache trees are removed. This updates
  installed Python preview behavior only; no native build, ZIP or gameplay
  qualification is claimed. Include the function change in the next qualified
  portable package rather than overwriting unrelated newer source changes.
  [SSD preview-function delivery](../.local/mcc-mod-preview-paths-20261020/ssd-delivery-v1.json).
- [ ] After that import gate passes, update importer output and runtime lookup
  paths to follow Halo2 MCC's relative directory structure. Use MCC's pattern
  as the canonical layout for this MCC-only port, preserving original internal
  tag names, relative `tags`/`data` paths, case and mod-folder subtrees. Convert
  required definitions before publishing them at those normal vanilla paths;
  keep provenance separate from logical tag ownership. Preserve Xbox-compatible
  logical tag paths where shared, without adding an ISO import dependency.
  Compiled maps retain their donor-relative `.map` paths and internal tag
  names; loose assets retain their donor-relative `tags/` and `data/` paths.
  Existing working caches need a verified migration or compatibility lookup.
- [ ] Verify path-equivalent native loading and mod-path resolution after the
  layout update. Record representative internal tags and loose/mod `tags` and
  `data` paths, confirming converted definitions are the vanilla baseline.
  This prepares the data layout for future Steam Workshop overlays; Workshop
  discovery, downloads and subscription support remain a later implementation.
- [x] Historical October19 exception, now superseded: copied the bundled repo
  intro and credits movies during cache import and bound their verified playback
  roles. Recorded them as port-supplied movie inputs, preserved MCC's copied BK2
  tree and the bundled originals, and avoided assigning intro/credits roles to
  unverified MCC filenames. Fresh/reused importer controls, selected-cache copy
  and that C++ cache-role/first-frame decoding passed as recorded below.
- [x] Implement the fresh/reused-import and no-reimport movie handoff. The
  exact two source MP4 hashes verify2,538,778,741bytes. Four focused CMake
  groups and12 Python controls pass, including interrupted-copy cleanup.
  Runtime resolution selects the pinned cache manifest, exact roles and file
  extents; it streams video without a2.5GB startup payload hash. Full payload
  verification remains in import/repair. The bundled sources are unchanged;
  test scratch is removed. Actual selected-cache copy/playback follows.
  At that time the package contract required both bundled movies and metadata.
  That requirement is superseded: a new MCC-only package must not carry them as
  runtime donors. Authentic MCC intro/credits/attract ownership remains unassigned.
  [Bundled movie handoff](../.local/mcc-bundled-movies-20261019/handoff.json).
- [x] Preserve both required movie metadata paths in CMake build and install
  outputs. The movie target previously emitted only the bank manifest; it now
  also emits and installs `resources/movie-manifest.json`. Bounded single- and
  multi-configuration CMake builds/installations pass, and each installed
  four-file bundle supports fresh and reused movie preparation. These use tiny
  fixtures to verify packaging and importer compatibility, rather than claiming
  movie playback or a complete engine build. All five test/fixture scratch
  trees are removed; no movie copies or binaries are retained by this check.
  [Movie install validation](../.local/mcc-movie-cmake-install-20261019/v2/validation.json),
  [Cleanup](../.local/mcc-movie-cmake-install-20261019/v2/cleanup.json).
- [x] Prepare the exact successor portable-package input inventory without
  publishing it. The V18 delta needs the updated six existing Python tools,
  new movie helper, both MP4s and both metadata files, plus separately accepted
  runtime/capability evidence. Portable launchers must be compiled with
  `python3`/`./bin`, rather than copying absolute-path development launchers.
  ZIP64 is required; movie assets belong in the full ZIP rather than the
  binaries-only updater feed. This is a pinned preparation, not a new package
  or native frontend acceptance. No archive or large copy is made.
  [Portable input preparation](../.local/mcc-bundled-movies-20261019/portable-preparation-v1/preparation.json).
- [x] Make the public compiled-preview packager carry both bundled movies
  and both exact metadata files, verify complete source/copied hashes, record
  provenance and explicitly enable ZIP64. Sixteen focused controls pass,
  including real tiny archive writing, invalid sources, interruption cleanup
  and no-replace publication. Only the final requested ZIP persists during
  real packaging; private stage copies are removed. These were synthetic
  fixtures, with no new release ZIP or large movie copy. Source pins and all
  private scratch removal are recorded. Custom prompt packaging follows
  separately; this does not publish the accepted native frontend.
  [Public movie packager closure](../.local/mcc-bundled-movies-20261019/public-packager-v1/validation.json),
  [Packager cleanup](../.local/mcc-bundled-movies-20261019/public-packager-v1/cleanup.json).
- [x] Extend the public packager to include all91 authored prompt source
  files alongside both movies and metadata. Full prompt hashes are verified
  before staging and after copying, with source provenance in package metadata;
  no reference fonts are bundled for this bank. All21 focused controls pass,
  including combined synthetic archive contents and existing ZIP64/cancellation
  behavior. Scratch and fixture ZIPs are removed. This updates the packager,
  rather than producing the next real compiled distribution.
  [Prompt package closure](../.local/mcc-bundled-movies-20261019/public-prompt-packager-v1/validation.json),
  [Prompt packager cleanup](../.local/mcc-bundled-movies-20261019/public-prompt-packager-v1/cleanup.json).
- [x] Copy both bundled movies into the actual selected
  `mcc-056309e12f444a3a8992c94f6fe32f0b/presentation/movies/` bank and verify
  complete source/destination SHA-256 values. The only provenance change is
  `port_bundled_movies`; MCC tree metadata, selection, current workbench/95
  shaders and saves remain exact. No MCC tree is recopied. Retained requested
  data is2,538,778,741bytes plus the721-byte manifest and empty lock. The
  6.73-second repair's actual writes stay within that bank, lock and provenance;
  its private scratch is removed. Cached playback decode follows separately.
  [Actual selected-cache movie import](../.local/mcc-bundled-movies-20261019/cache-copy-v1/validation.json).
- [x] Resolve both actual cached movie roles through current C++ and decode
  their video/audio. Each produces a3840x2160 frame; the standalone decoder
  also checks three frames per role and finite nonzero48kHz stereo audio.
  Intro/credits report866,624/18,212,224 decoded audio frames. Attract remains
  unassigned. Private compilers, frames/audio buffers and all four recorded
  scratch trees are removed. This is bounded decode, not complete playback,
  speaker output or a displayed full video.
  [Movie copy/decode closure](../.local/mcc-bundled-movies-20261019/cache-copy-v1/closure.json).
  The prospective frontend's exact compatible Movie provider also resolves the
  selected cache bank first and decodes each role through frame3. Its existing
  audio decoder produces866,624/18,212,224 finite/nonzero frames with peaks
  0.91483/0.737033. Attract stays empty. This additional provider check runs in
  the owning frontend test's private hardlink view. That full test now passes
  and its scratch is removed; custom prompt/launcher admission remain separate.
  [Prospective provider movie check](../.local/mcc-frontend-runtime-20261019/unmuted-xinput-v2/movie-role-validation.json).
- [x] Keep the launcher's normal frontend route eligible for the bundled
  startup intro, while the title-first variant explicitly skips it. The
  relocated onboarding launcher passes all34 routing tests with this split.
  Movie import and actual decode/playback acceptance remain separate. Its
  private build is removed.
  [Intro/title route check](../.local/mcc-frontend-launcher-20261019/bundled-intro-routing-v5/report.json).
- [x] Audit the actual imported frontend inputs: both prepared caches retain
  all40 MCC fonts and their12-selector table; the selected menu contains131
  screens and275 UI bitmap banks/929 frames. The newer cache also has561
  decoded DDS assets. All57 copied BK2 files remain available, but classic
  intro/attract role bindings are not established by their presence. No
  reimport or separate Vista bank is needed for the title/font inputs.
  [Imported asset audit](../.local/mcc-frontend-asset-contract-20261019/read-only-v1/validation.json).
- [x] Get the compatible native frontend through MCC metadata/image decoding
  and into a real title frame using the full imported-cache hierarchy.
  Preserve three authored empty hidden MCC lists, use the existing MCC bitmap
  reader, load the authenticated MCC font path and read animation bytes from
  their checked owned graph. The original-file raw-data guard remains intact.
  The640x480 title frame reveals cropped PC-canvas artwork; layout/navigation
  and unmuted acceptance remain open. Both audited runs open no Xbox/Vista
  reference files, preserve97 shaders and remove scratch. The153,319-byte PNG
  is retained as required comparison evidence, not an accepted title image.
  [Metadata acceptance](../.local/mcc-frontend-runtime-20261019/content-fix-v3/validation.json),
  [Initial title frame](../.local/mcc-frontend-runtime-20261019/title-frame-v3/validation.json).
- [ ] Validate title-to-main-menu, submenu navigation and return/back through
  the selected runtime and launcher using that MCC cache. Record the exact
  imported cache and opened files, rejecting Xbox/Vista cache dependencies.
  The first bounded V19 MCC menu audit stopped with
  `invalid UI list definition`. Metadata, a first title frame and the later
  CPU navigation stage now pass as recorded below. Corrected muted rendered
  navigation also passes; the launcher and unmuted/input gates remain open.
  [Initial MCC-only audit](../.local/mcc-frontend-runtime-20261019/initial-v1/validation.json).
- [x] Verify the corrected native MCC title/profile/main/Settings/back flow
  with four actual640x480 rendered cases. Creating the private profile through
  keyboard167 reaches main6; selecting Settings at319,357 reaches19; Back
  returns through confirmation7 to title9. All four processes exit0 and open
  no Xbox/Vista reference files. The existing97 shaders are unchanged. Four
  required comparison PNGs total550,453bytes; raw frames, linked test binaries
  and scratch are removed. These muted cases do not prove unmuted startup,
  physical desktop input, every submenu or Xbox framebuffer parity.
  The compatible link uses322 physical inputs: the316-input parent, one
  replaced frontend input and seven explicit overrides; the original archive
  remains intact with member-ownership witnesses.
  [Rendered navigation acceptance](../.local/mcc-frontend-runtime-20261019/navigation-v3/acceptance-summary.json).
- [x] Identify the profile-picker navigation failure by exact identifier names.
  A bounded offline original name-table reference and the imported MCC table
  give6,873 unique common names, three explicitly absent names and no ambiguous
  matches. All requested navigation/keyboard IDs are corroborated by actual
  MCC locale ID words. Independent reconstruction verifies every generated
  row; for example, `create_new_profile` maps12000195 to1200019d. Runtime
  inputs remain MCC-only. Localized text and authored MCC IDs are preserved;
  only compiled original identifier lookups may use this qualified mapping.
  Product integration/navigation acceptance remains open; scratch is removed.
  [Named identifier inventory](../.local/mcc-frontend-string-inventory-20261019/common-unique-v1/validation.json),
  [Independent mapping review](../.local/mcc-frontend-string-inventory-20261019/common-unique-v1/root-review.json).
- [x] Make default Play open the validated MCC frontend, while preserving a
  separate native gameplay test and honest incomplete full-game readiness.
  Both installed F45 Play buttons and both extracted v20 launchers reach the
  imported title/profile/menu route, retained loading and native Lockout with
  audio and clean exit. Broader gameplay remains explicitly incomplete.
  [Installed Play](../.local/mcc-loading-original-tone-20261020/installation.json),
  [Extracted Play](../.local/mcc-gameplay-continuation-20261020/portable-v20-validation.json).
- [x] Resolve the actual unmuted frontend failure before publishing that
  capability. The first eight-leaf/323-input candidate compiles and links but
  aborts before `Menu ready`, after a wmav2 warning and stack-smashing report.
  No real OS input, accepted candidate, video or sidecar is produced. Its
  scratch is removed; muted four-screen acceptance remains valid. Diagnose the
  exact software decoder/provider contract; no hardware initializer work is
  required without demonstrated dependency.
  [Unmuted failure](../.local/mcc-frontend-runtime-20261019/unmuted-xinput-v1/validation.json).
  The matching-header caller refresh now passes the full actual unmuted native
  X11 test: title9/profile10/keyboard167/profile38/main6, real pointer selection
  to Settings19 and Back6,482 rendered frames and exit0 in70.57 seconds.
  Actual SDL output contains848,384 finite/nonzero stereo frames at48kHz,
  peak0.362493. The sole opened music/UI bank is imported MCC
  `sounds_neutral.dat`; no Xbox/Vista runtime inputs are opened. All97 shaders,
  original316 inputs, user saves and runtime selections remain unchanged.
  The accepted324-input candidate and required coherent inputs retain
  150,790,930bytes. Its requested12-second video with native audio is1,647,535
  bytes. Raw captures/PCM/maps and private scratch are removed. Custom prompt
  integration, launcher admission and full parity remain separate open gates.
  [Unmuted/input acceptance](../.local/mcc-frontend-runtime-20261019/unmuted-xinput-v2/validation.json),
  [Coherent candidate retention](../.local/mcc-frontend-runtime-20261019/unmuted-xinput-v2/retention.json).
  The video independently decodes as144 H264640x480 frames and576,000 finite,
  nonzero stereo audio frames. It starts at profile entry, then shows the main
  menu and Settings transition; it does not include the accepted title frame.
  A212,855-byte contact image is retained for video QA, whose scratch is removed.
  The final custom-prompt capture must wait for recording readiness before input.
  [Video QA](../.local/mcc-frontend-runtime-20261019/unmuted-xinput-v2/video-qa.json).
- [x] Minimize the unmuted failure to the actual MCC MenuMusic constructor
  and identify a selected caller/producer layout mismatch. The old music
  caller allocates280 bytes for `SoundVariationDefinition`; the selected
  variation producer returns288 bytes, including `sample_rate_index` at280.
  Actual DWARF and the failing lambda's stack-canary epilogue corroborate the
  boundary. Both callers agree on88-byte `PcmSound`; the packaged channel-layout
  functions touch only the expected24 bytes. This diagnoses a concrete ABI
  mismatch, not a passed fix. Refresh only the unchanged music caller against
  the accepted header, then repeat the tight CPU and unmuted frontend tests.
  All three independent ABI probe scratch trees are removed.
  [Selected sound variation layouts](../.local/mcc-frontend-audio-abi-20261019/sound-variation-types-v1.json),
  [Actual minimized failure](../.local/mcc-frontend-runtime-20261019/audio-cpu-v3/validation.json).
- [x] Verify the narrow MenuMusic caller refresh against the actual MCC audio.
  Compiling the unchanged authenticated source against the selected288-byte
  header changes the minimized constructor from abort to exit0 in1.285 seconds.
  All12 UI sounds and four authored WMA loops decode finite/nonzero at48kHz;
  MenuMusic plays one voice through the existing mixer for48,000 frames, with
  peak0.047278. The companion debugger also exits normally. No music algorithm,
  decoder, gameplay, cache, shader or save bytes change; original316 link inputs
  remain exact. The private compiler, objects and PCM buffers are removed.
  Actual unmuted native/window acceptance follows as a separate gate.
  [Caller refresh acceptance](../.local/mcc-frontend-runtime-20261019/audio-cpu-v4/acceptance-summary.json).
  A later narrow DWARF closure verifies the other selected by-value consumers:
  multiplayer audio, the native provider, script-world access and Cache all
  agree on288 bytes, appended field offset280 and the same ten field offsets.
  Their exact four cohort object hashes match. No additional mismatch or
  rebuild is needed; audit scratch is removed.
  [Remaining sound caller layouts](../.local/mcc-frontend-audio-abi-20261019/v20-caller-closure-v1/review.json).
- [x] Bind frontend admission to the actual classic audio bank as well as
  mainmenu/shared/textures/fonts. An independent metadata control reproduced
  admission after an unpinned `sounds_neutral.dat` was changed/deleted. The
  reader now requires its exact hash; omission, mutation and missing-bank
  controls reject, while a complete receipt still admits. All34 compiled
  launcher routing controls, six frontend controls and22 updater controls pass
  through three focused CMake groups. Its private build is removed; no game
  binaries or raw data are retained by these checks.
  [Independent audio-bank finding](../.local/mcc-frontend-launcher-20261019/independent-resource-review-v1/review.json),
  [Audio-resource admission checks](../.local/mcc-frontend-launcher-20261019/audio-resource-routing-v6/closure.json).
  The independent exact control also passes: correct bank admits; omitted hash,
  changed bank and deleted bank reject; full restoration readmits. Its own
  scratch is removed.
  [Independent post-fix control](../.local/mcc-frontend-launcher-20261019/independent-resource-review-v2/review.json).
- [x] Exercise the named-identifier and PC-canvas integration through actual
  MCC menu state and pointer hit testing: title9 to profile10, keyboard167,
  profile38 and main6 succeeds; each of the five main entries owns3,840 pixels
  in its distinct hit region. The actual Settings hit at319,357 opens19.
  This CPU stage uses the imported MCC cache. Its owning run still has GPU
  cases in progress; no frontend sidecar or accepted binary is published from
  this stage alone. Final scratch cleanup follows the owning run's exit.
  [Actual MCC pointer/navigation stage](../.local/mcc-frontend-runtime-20261019/navigation-v3/hit-result.json).
- [ ] Make Update fetch the latest published prebuilt binaries for the host
  platform and replace only files requiring updates. Preserve imported content,
  shaders, saves, local source changes and the previous working runtime on
  failure. Do not implement Update as Git pull plus a source rebuild.
- [x] Compile the relocated onboarding launcher and verify binary-selection
  precedence, stale/escaping selection fallback and packaged `--update`
  routing without importing game data or compiling the native engine. The
  focused launcher routing suite passes; its private build is removed.
  This does not prove a published release or a frontend frame.
  [Launcher routing check](../.local/mcc-frontend-launcher-20261019/launcher-routing-v1/report.json).
- [ ] Verify the real update route against its actual release source and
  exercise changed/current, wrong-platform, corrupt and interrupted downloads.
  A local updater fixture alone does not prove a published build is available.
- [x] Implement official GitHub prebuilt selection, file-hash verification,
  changed-file downloads, staged activation/rollback and launcher forwarding.
  The integrated eight focused CTest groups pass, including the real Tk
  controls, separate frontend/native admission, binary updater and build entry
  point. The official latest-release endpoint returns404 with no installation
  writes; no published binary is available to accept end-to-end yet. Existing
  builds remain playable after a failed download. Both validation build trees
  are removed; no test binaries are retained.
  [Integration validation](../.local/mcc-frontend-launcher-20261019/integration-v2/report.json),
  [Updater receipt](../.local/prebuilt-update-audit-20261019/validation.json).
- [x] Pin the frontend texture bank as well as menu/shared/font inputs, and
  reject foreign or duplicate menu-map/UI overrides. The new duplicate control
  exposed the older first-value-only check before the correction. The stable
  corrected launcher passes all34 routing tests and frontend admission
  controls. Safe-path direct release-slot CLI admission also passes; changing
  its synthetic texture bank expires frontend admission. These are input and
  routing proofs, not rendered-menu acceptance. Private builds are removed.
  [Corrected routing](../.local/mcc-frontend-launcher-20261019/frontend-routing-v4/report.json).
  The independent review and22 updater/release-admission controls also pass;
  the release-slot CLI is exercised in a fresh safe-path Python process.
  [Current integration review](../.local/prebuilt-update-audit-20261019/integration-review-v1.json).
- [x] Stage successful builds into the designated platform output directory.
  Linux x86 means the current Linux x86_64 target. Reserve Linux arm64,
  Windows x86_64, Windows arm64 and macOS arm64 platform identifiers, but skip
  their builds and outputs until their support is implemented. Keep local
  build staging distinct from publishing downloadable release assets.
  A real isolated CMake integration builds four genuine ELF targets and
  verifies their distinct outputs and hashes; a second private build verifies
  staging defaults off under test scratch. Both trees and all eight binaries
  are removed. No unsupported-platform output is produced.
  [CMake staging acceptance](../.local/build-output-staging-20261019/cmake-integration-v1.json).
- [x] Compile and stage the three actual current launcher binaries into those
  designated outputs. The regular and title variants preserve the currently
  selected camera-performance workbench; onboarding keeps its normal relative
  first-run configuration. The second build includes the bundled-intro/title
  route split and current source6b55a7ad. Retained outputs total10,830,184bytes plus
  the1,184-byte manifest. The private compiler/build files are removed. Root
  launchers, the selected workbench and its95 shaders, caches and selectors
  remain unchanged. The gameplay binary will be staged after its separate
  frontend candidate acceptance; this does not install or publish a release.
  [Actual latest launcher outputs](../.local/launcher-designated-output-20261019/execution-v2/validation.json),
  [Build cleanup](../.local/launcher-designated-output-20261019/execution-v2/cleanup.json).
- [x] Compile the two separate portable release launchers from frozen
  source6b55a7ad using relative `python3`/`./bin` configuration. Both real
  ELF64 binaries pass bounded relocated configuration checks through the
  existing packaged loader, libraries and Python. Regular startup retains the
  intro; the title variant skips it. Required package inputs retain7,253,922
  bytes, including both ELFs, source and provenance. Root/P/designated builds,
  caches, saves, selectors and recursive shader inventories remain exact.
  Private build/fixture scratch is removed. No game, install, package or
  capability sidecar is produced; full extracted-package acceptance follows.
  [Portable launcher inputs](../.local/mcc-portable-launchers-20261019/inputs-v1/inputs.json),
  [Portable build cleanup](../.local/mcc-portable-launchers-20261019/build-v3/cleanup.json).
- [x] Add executable root `build-linux.sh` with clone-relative paths, quoted
  directories and configurable Python/build directory/job count. The focused
  entry-point checks cover another working directory, spaces and overrides.
  This is script/routing validation, not a fresh-clone full engine build.
  The bounded clone-input audit confirms the inspected generated renderer
  inputs have tracked sources and CMake producers, and adds the required
  `spirv-val` dependency to README. An untracked AI descriptor include belongs
  to unrelated dirty work and is preserved with its owner; the new source
  files must accompany their corresponding source changes. No clean-clone
  full engine compilation is claimed.
  [Clone input audit](../.local/build-output-staging-20261019/clone-input-audit-v1.json).


## Weapon runtime cache requirement (October19)

The weapon goal's remaining work and per-weapon completion entries live in
[WEAPONS_TU15_GOAL_CHECKLIST.md](WEAPONS_TU15_GOAL_CHECKLIST.md). Maintain that
checklist and its linked goal state when completing weapon milestones.

- [x] Require the prepared MCC cache in weapon snapshot validation. Explicit
  `--mcc-root` admission checks version13 headers, records file hashes and rejects
  ISO/Vista controls. Runtime cases use native TU1.5 startup without an Xbox
  image. The historical ISO-camera case remains reference-only. Probe scratch
  is removed; no large artifacts are retained.
  [Input admission](../analysis/weapons-tu15-goal-20261013/mcc-cache-input-admission-v1.json).
- [x] Inventory the actual prepared MCC directories against the retained Xbox
  weapon scope. All43 cache files decode, with1,479 authored weapon rows and66
  distinct paths. All63 Xbox scope paths are present. The three additional MCC
  paths are absent from all 41 owned original map/shared/menu directories;
  their resident definitions belong to both MCC shared banks. They remain
  preserved additions outside the original 63-weapon catalog. This is identity
  and scope coverage, with no
  conversion, gameplay or visual certification. Scratch is removed.
  [MCC weapon inventory](../analysis/weapons-tu15-goal-20261013/mcc-authored-weapon-scope-v1.json).
- [ ] Verify the campaign weapon/HUD and application cases against these MCC
  inputs, then complete feature and visual coverage through the real import and
  runtime path. Tag conversion belongs to import; no reference-tag substitution
  may be used to pass validation.
- [x] Verify the bounded native MCC HUD/map startup and704 shared-copy cases
  without an Xbox image constructor. The actual selected HUD address belongs to
  the resident MCC `hudg` directory. One/four-player MCC CLI simulations pass.
  The original47D6C0 operand restoration clears1E7F18 in the MCC rerun. Actual
  campaign placement next fails at1B0D0F on descriptor slot48AD78; actor/weapon
  submission and application acceptance remain open.
  Both private source/build trees and probe scratch are removed.
  [MCC startup checkpoint](../analysis/weapons-tu15-goal-20261013/checkpoint-weapon-mcc-native-startup-20261019.json).

The [weapon checklist](WEAPONS_TU15_GOAL_CHECKLIST.md) now records all13
corrected MCC checks passing in v13. Both namespaces pass131,276 object-random
cases and102 actual MCC bomb `scrawl` aliases, followed by four real application
frames and expected owner/definition/constant/vertex bindings. The native CPU
probe at the actual MCC camera independently checks12 weapon units/24,624
vertices, shaders/LOD, WORLD poses and lighting across turret-item, BR and SMG;
its particle path is excluded. Both full-run and probe source/build scratch are
removed. Visible pixels, Xbox reference frames, audio and full weapon sequences
remain open. The matrix has90 partial/540 unverified gates and zero complete.
The two new partial inventory entries reuse independently reviewed MCC Lockout
bomb/flag first-activation and full salted-retirement evidence. All130 frozen
sources, cache/definition identities and five invalid-evidence controls pass;
carrier/pickup/drop, scoring, all-map behavior and visuals remain open.

V14 builds and passes two arithmetic references; its three actual MCC
firing/gauge checks fail at the shared original pickup notification read
`B2C9E`/`4DB6B0`. The private source/build trees are removed and the failure
is retained. The original680C0 configuration owner is now wired before object
birth, sharing the existing cold bank with full network startup. Focused native
comparison passes100 instruction positions,84 ordered stores,6,104 bytes and12
immutable inputs, with live-state and invalid-owner controls. No constructed
outputs are installed by the data owner. Actual MCC acceptance remains open.

V15 is terminal14/17: configuration ownership and the prior material,
HUD/campaign/four-frame cases pass. Three runtime cases clear4DB6B0 and now stop
at the original prompt getter's cached-language read `1A9D98`/`48C750`.
Its source/build scratch is removed. The existing language owner is shared with
the native world, preserving the authored unset marker and complete36-byte
conversion table. All259 original/native conversion cases, two live-owner and
two invalid-owner controls pass; the real getter still resolves the locale.
V16 is terminal15/18 and clears the missing-language read. Its three live
cases then fail because the fixture requests Xbox material IDs from an MCC
callback. All16 scratch is removed. The corrected fixture verifies cache-local
one/turned_on/heat/age/ammunition names and reads Needler's authored ammo alias
from its own definition. Input/value/pose/audio requirements stay intact.
V17 rejected a concurrent world-source edit before building and removed scratch.
V18 is terminal5/7. The actual held-gauge case passes31 observations; pinned
and natural-motion firing each reach four rounds, seven attack poses and111
gauge observations, then fail with zero owned firing sounds. The unit-definition
fields were unobserved defaults because their separate option was disabled;
they do not diagnose a live-unit defect or establish species acceptance.
All18 scratch is removed; the15 passing v16 regressions remain retained.
V19 completes four genuine108A70 lower/upper-fire requests and valid effect
handles per case. V20 is terminal4/6:30/31 original sound submissions all return
NONE through the ordinary uninitialized device fallback; the FF custom-playback
field is valid. Both builds are removed. Full genuine audio startup remains open.
The alive producer now matches65,536 original flag cases and requires actual
object-owned flags. All473 supported identities qualify across43 MCC caches.
V23 passes all20 corrected MCC checks. Both namespaces match27,040 original
firing material cases and one actual MCC authored input. The real Spartan sword
pickup retains its source owner and follows21 actual source ticks,with62 alive
observations and42 hidden/20 lit blade draws. Its prior elapsed20 fixture
assumption was corrected using actual clock/countdown/flags; production ready
code did not change. Campaign/four submitted frames,gauge and prior regressions
pass. All23 build/source scratch is removed. Sword pickup/material/carried-pose
evidence advances three gates to partial;93 partial/537 unverified remain,zero
complete and zero certified weapons. V24 passed all21 checks for the
body/dual namespace correction:82 actual Cache/body contexts,49 body melee
actions and the original selector fixture pass.
all45 selectors qualify across43 MCC caches. Natural Sentinel work concerns
the AI beam audio_firing function and its actor/weapon/looping-audio owner;
full genuine audio startup remains required. V25 stopped at a shared cold-start overlap before Sword execution:11 pass/
6 fail/5 not run. Current shared factory focused checks pass. V26 failed before tests at a
shared renderer compile error;current renderer syntax passes with its normal
target/catalog. V27 passes21/22:actual Sword fire reaches originalEBF20 and
fails its absent immutable0x44A8DC candidate table. The complete48 bytes and
50-instruction selector are authenticated; the actual cold factory now passes
exact bytes,read-only and rejected-write checks. V28 passes21/22 and clears the
table read,but its160 completed frames contain only Sword ready/idle poses;
all32 source samples publish no attack. V29 passes21/22 and records14 completed
calls:all six availability/graph queries reject,including foreign Xbox strike
keys against the actual MCC graph. V30 passes all5 focused source/Cache/guard/
reference checks for the bounded compiled-query aliases,with12 actual MCC
default/fallback matches and unchanged canonical actions/graph metadata.
V31 rejects an unrelated network-service test edit during copy before building;
its scratch is removed. V32 is terminal22/23:14 completed selection calls all
return zero and no authored attack is selected. The live dispatcher selects a
second16B31B copy in the death-cycle owner before physics support. Both compiled
copies now match fresh generation with the qualified query key. V33 completes
22/23:three original MCC graph queries succeed and actual source action agrees
with the selected canonical strike1,but rendering remains52 ready/59 idle poses.
V34 completes9/10:the real169A56/1D5320 request publishes canonical strike1
and advances FP frames1–23,but its packed clip remains idle0000FFFF. A scoped
MCC record-search correction now covers all three compiled copies; six complete
generated bodies and4 guard/AI component regressions pass. V35/session88809 runs
23 full corrected MCC checks. The source-owned FP path is preserved; advancing
attack poses,contact and accepted owned audio remain required.
The same v32 Cache binding executable also passes all25 multiplayer maps,
including300 qualified source-query-key matches;
natural pickup/melee/audio and matched Xbox visuals remain open.
Renderer caches remain in test scratch and completed builds are removed. Natural bomb lifetime/world/
FP rendering, full particle/scenery instance lighting and all63 weapon map/
species/hand/visual/audio sequences remain required.

## Account usage instruction during this work

The user authorized one existing Codex usage reset at1% or less remaining.
On2026-10-19, a fresh check moved from2% remaining/two available reset credits
to100%/one credit, with the account's reset time advancing. This task did not
call `consume_usage_reset`; all three children also confirmed no reset attempt.
The account reset therefore occurred elsewhere. Treat the single-reset request
as fulfilled and do not spend another credit under it. The next fresh check at
16:26:26UTC shows99% remaining/one available credit. Null/missing usage windows
remain unavailable, never zero. No credit purchase or additional reset is
authorized. [Observed account reset](../.local/account-usage-reset-20261019/observed-reset.json).

## Immediate user-reported priority: interactive launch

- [x] Sample the installed V14 startup after the upload-hash correction. Four
  parent-owned debugger samples find MCC sound-timing relocation at4seconds,
  original combat startup memory validation at12seconds, repeated gameplay-font
  source validation at20seconds and texture bucket hashing at28seconds. These
  are bounded stack observations, not measured phase costs or normal boot timing.
  The diagnostic intentionally stops its private child; scratch is removed and
  saves, shaders, installed runtimes and host security settings remain unchanged.
  [V14 startup samples](../.local/mcc-native-click-crash-20261018/startup-profile-v3/validation.json).
- [x] Measure ordinary gameplay-font validation before adding a cache. A source
  load takes6.03ms;32 repeated complete eight-file authentications take52.69ms.
  The current API sequence for three HUD hands and scoreboard takes0.151943s,
  with175 digest calls/84,702,617bytes. This CPU-only measurement does not explain
  a multi-second boot delay or prove installed-provider cost. No persistent
  font/authentication cache is added; source mutation rejection stays unchanged.
  Disposable probe scratch is removed, with no large artifacts retained.
  [Font cost measurement](../.local/mcc-native-click-crash-20261018/font-startup-v1/validation.json).
- [x] Measure actual MCC Lockout relocation lookup with the coherent V14
  providers. All41,793 boundary/extent controls match an indexed candidate over
  6,965 admitted ranges.100,000 lookup queries take0.226813s with the existing
  scan and0.00258037s with the candidate. Whole sound-timing construction takes
  9.02226s, which is not yet attributed wholly to lookup. Index and metadata
  hashes are retained; no source/provider/map bytes change and scratch is
  removed. Product semantics review and phase comparison remain open.
  [Actual relocation profile](../.local/mcc-native-click-crash-20261018/runtime-relocation-profile-v1/validation.json).
- [x] Attribute the sound-timing startup cost with a complete paired build of
  the same plan.4,184,679 relocation calls take the existing construction from
  9.23038s to0.503806s with indexed lookup. All exposed fields of4,503 bindings,
  3,429 gaps,two owners,1,359 stored references and4,506 permutation references
  match; index/metadata hashes remain exact. This CPU phase result is not a
  native-launch guarantee. Scratch is removed; no product bytes change yet.
  [Paired timing construction](../.local/mcc-native-click-crash-20261018/runtime-relocation-profile-v2/validation.json).
- [x] Independently review sorted lookup against unchanged range admission.
  Across112,956 bounded tables,8,016 valid/104,940 rejected,960,872 comparisons
  cover empty aliases, duplicate starts, adjacent allocations, reverse mapping,
  wrap and maximum-size queries. The disjoint-owner proof passes. Product must
  sort every finalized plan and preserve validation/error behavior; generic
  relocation and reverse translation remain unchanged. Scratch is removed.
  [Independent lookup review](../.local/mcc-native-click-crash-20261018/runtime-relocation-review-v1/validation.json).
- [x] Implement and independently review the exact product lookup. Only the
  new sorted-source helper, finalization-time sort and `runtime_address`
  dispatch change. Last append225, unchanged admission228 and final sort423
  precede the sole returned plan.960,872 extracted-product controls preserve
  values, acceptance, exact errors and reverse mapping; no product consumer
  depends on relocation iteration order. Source/header layout and serialized
  widths stay unchanged. Scratch is removed; compiled/native acceptance remains
  separate and the installed V14 runtime is still unchanged.
  [Exact product review](../.local/mcc-native-click-crash-20261018/runtime-relocation-review-v2/validation.json).
- [x] Compile the exact new relocation source against ordinary current headers
  and normal cache-module flags. Compilation is warning-free in2.038s with322
  dependency pins unchanged. Inverting precisely the helper/sort/dispatch edits
  reproduces the original source hash. The2,451,704-byte object and scratch are
  removed. This verifies ordinary compilation, not a current-client ABI link,
  installed runtime or full64-bit migration.
  [Current-header compile](../.local/mcc-native-click-crash-20261018/runtime-relocation-source-v1/current-compile.json).
- [x] Run the new coherent provider through the existing129 address controls
  and complete owned Lockout/Cairo/Metropolis metadata checks. They verify
  64,290/112,779/104,548 pointer fields and33,525,954/69,887,237/60,340,131 owned
  bytes, preserving every unpatched authored byte and required sound admission.
  All four runs exit0. A subsequent extra profile compile lacks a header in the
  older frozen include tree; this probe setup issue is recorded separately and
  being repaired without changing product semantics. Its scratch is removed.
  [Actual metadata controls](../.local/mcc-native-click-crash-20261018/runtime-relocation-product-v1/metadata-controls.json).
- [x] Complete coherent-provider compilation and the repaired timing probe.
  The repeated contract/three-map controls pass; the new public lookup builds
  the complete timing plan in0.441249s, with the exact prior index/metadata hashes,
  4,184,679 calls and all exposed timing fields preserved. All315 parent inputs,
  three maps and provider dependencies remain unchanged. Temporary binaries/maps
  and scratch are removed. Native acceptance is underway; an initial native
  comparison checker used the wrong capture-row prefix after a successful V14
  tick60 baseline, and its retained failure is corrected for retry. No installed
  runtime is replaced by either probe.
  [Compiled provider validation](../.local/mcc-native-click-crash-20261018/runtime-relocation-product-v1/validation.json).
- [x] Compare real V14 and indexed-lookup native firing through fixed tick60.
  Fire at3/release at4 yields identical decoded image, save outputs and full
  capture state, including loaded59/reserve120/two detonations. Both exit0;
  callback audio is finite/nonzero. All315 prior providers,97 shaders, four
  user saves and installed reference runtime remain exact. Private runtime,
  link map, raw audio/frame/cache/save scratch is removed; the two small PNGs
  are retained as actual comparison references. These checks do not certify
  full audio identity, gameplay coverage or physical-desktop boot speed.
  [Native firing comparison](../.local/mcc-native-click-crash-20261018/runtime-relocation-firing-v1/validation.json).
- [x] Retain the accepted coherent V15 candidate for interactive validation and
  delivery, replacing exactly one leaf while preserving314 providers/97 shaders.
  Extended actual-map regression checks now verify every declared full forward
  range and its final payload byte in addition to reverse ownership; all three
  maps and generic controls pass. Required retained inputs are the2,328,600-byte
  provider,104,712,264-byte runtime and20,570,809-byte owner map. Generated test
  binaries and scratch are removed. Installation/full readiness remain separate.
  [V15 coherent candidate](../.local/mcc-native-client-20261018/client-current-v15-runtime-address-index-v1/cohort.json).
- [x] Validate V15 with real private-display pointer/focus/click, held fire,
  mouse look and movement. All nine phases survive15.17s after rendered-frame
  readiness; live first-person particles reach four draws and no errors occur.
  Frame5 is observed at20.94s, window at3.27s with copied warm GPU cache and
  saves. The separate fixed-tick firing pair takes32.55s(V14)/23.78s(V15).
  These are bounded observations, not guaranteed desktop timing or full parity.
  The test deliberately terminates only its private child after the sequence;
  all user saves remain exact and scratch is removed. Installation is next.
  [V15 real input](../.local/mcc-native-click-crash-20261018/candidate-v9/validation.json).
- [x] Install the accepted V15 lookup into both active native-test runtimes.
  All four actual launcher-info routes admit the interim test and retain full
  readiness false; the user's actual launcher completes eight frames with exit0.
  Four user save files,97 shaders and315 inputs stay exact. Prior runtimes and
  capability files are preserved for rollback; generated capture/cache/test
  scratch is removed. This delivers the measured startup improvement through
  Play while full map conversion/current-client64-bit/parity remain unfinished.
  [Installed V15 validation](../.local/mcc-native-click-crash-20261018/indexed-installed-v1/validation.json).
- [x] Publish the requested compiled V15 ZIP with the installed lookup change.
  All4,249 archive entries/checksums and both relocated launcher-info routes
  pass. The real relocated launcher renders eight frames with exit0 in27.35s
  under tracing;108,603 opens contain no reference/development dependency.
  All97 bundled shaders and unchanged V14 tools/data remain exact. The43
  unchanged importer controls are inherited, not repeated. No game cache or
  saves are distributed. Requested ZIP183,597,745bytes and118-byte checksum
  are retained; extraction,31,098,642-byte trace, raw frame and scratch are
  removed. New map-qualified conversions remain outside V15 and full readiness
  stays false.
  [V15 package validation](../.local/mcc-native-click-crash-20261018/package-v15/validation.json)
  · [Compiled V15 ZIP](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261019-v15.zip).
- [x] Synchronize the user's existing extracted folder with all4,249 V15
  archive files. Only five small metadata files change; both launcher-info
  routes pass and retain native-test admission/full readiness false. Runtime
  hashes,97 shaders,four saves and45,231 cache-tree stat entries stay exact,
  with no asset reimport. Rollback metadata totals505,646bytes and is explicitly
  retained; extraction/staging scratch is removed.
  [Extracted V15 synchronization](../.local/mcc-native-click-crash-20261018/sync-extracted-v15/validation.json).
- [x] Identify concrete authored conversion gaps against the retained original
  comparison and current owned MCC directories. Relic's flag-reset stop distance
  and Gauss flyby differ by original map generation; tag-name-only changes would
  damage other maps or mods. Two classic flyby dependencies are absent from all43
  imported directories. This is an inventory milestone; conversion, original
  consumer verification and sound reconstruction remain open.
  [Current tag-gap observations](../.local/mcc-native-click-crash-20261018/cleanup-v14/gameplay-tag-gaps-v14.json).
- [x] Authenticate the first three map-qualified conversion contexts before
  implementation: original Lockout uses CTF distance6/Gauss null, Relic uses
  distance4/Gauss Scorpion flyby, Backwash uses distance6/Gauss Scorpion flyby.
  Current scenario, mulg and Gauss donor/target banks are hash-pinned against
  fresh offline original reads. The original `24D5F0` consumer follows
  globals→mulg→runtime constants, squares field`D8` and compares squared3D
  distance. Sixteen genuine instruction-block controls preserve the strict
  near-distance boundary; full caller/team/iterator execution is not claimed.
  Scratch is removed. Pure planning, atomic integration and broader map
  contexts remain open; references remain offline only.
  [Context evidence](../.local/mcc-map-qualified-conversion-20261019/profile-evidence-v1.json)
  · [Original distance consumer](../.local/mcc-map-qualified-conversion-20261019/original-ctf-radius-v1.json).
- [x] Implement the separate pure map-qualified planner with guarded before/
  after writes for those three contexts. Exact scenario/header identity and
  original primary-globals→salted mulg binding qualify complete owner tuples.
  Strict compilation and real-context fixtures pass, including idempotence,
  changed/modded donors, identity/salt/group, absent/ambiguous dependencies,
  short/stale views, overlapping writes and invalid-width rejection. Scratch
  is removed and no binaries are retained. This source is not wired into Cache
  or V15; independent review, sanitizer/actual integration and broader coverage
  remain open. The current Cache source and active runtimes stay unchanged.
  [Pure planner controls](../.local/mcc-map-qualified-conversion-20261019/planner-validation-attempt2.json).
- [x] Run the pure three-context planner under AddressSanitizer and
  UndefinedBehaviorSanitizer. The same authenticated context and rejection
  controls pass with strict warnings and no sanitizer findings. Source pins
  remain unchanged; all generated fixtures/executables/scratch are removed.
  Independent review and actual Cache integration remain separate gates.
  [Planner sanitizer controls](../.local/mcc-map-qualified-conversion-20261019/planner-sanitizer-v1.json).
- [x] Independently review the exact pure planner and extend sanitizer controls.
  All qualification map-ID reads lie inside the captured scenario bank; target
  tuples reproduce independently. Forty-six added controls cover custom owner
  boundaries, individual stale salts, changed guards and overlapping/malformed
  multi-write publication. No blocking flaw is found within the three-context
  scope. Integration must retain immutable directory/storage ownership, resolve
  all destinations before preflight/copies, precede current donor conversions
  and preserve unknown contexts. All scratch is removed, with no large artifacts.
  [Independent planner review](../.local/mcc-map-qualified-conversion-20261019/review-v2/validation.json).
- [x] Integrate the reviewed map planner into normal Cache loading and verify
  exact intended vanilla writes, custom preservation and current/frozen builds.
- [x] Deliver the authenticated integrated map conversions through a coherent
  native runtime after bounded input/render/callback-audio acceptance. V16
  extends Gauss/Relic coverage to 28 contexts; V18 additionally ships Floodzone's
  two material indices with fixed-tick, interactive and relocated launcher
  validation. Further gameplay-tag coverage remains a separate open gate.
- [x] Wire the pure planner into ordinary build configuration and run its real
  authenticated three-context fixture through disposable CMake/CTest. The test
  passes in0.09s; it is not skipped. The public lifetime comment now explicitly
  requires unchanged directory/storage ownership through publication. Both the
  generated fixture and private CMake build are removed. The Cache hook is now
  in source before generic converters; compilation/actual normal Cache readback
  and coherent native delivery are still open.
  [CMake planner validation](../.local/mcc-map-qualified-conversion-20261019/cmake-v1/validation.json).
- [x] Authenticate the active Cache provider before coherent integration. The
  unchanged44,650-byte preimage plus106 pinned dependencies reproduces all68
  allocated sections,2,255 semantic relocations and190 nonlocal symbol contracts
  of the active V15 provider, warning-free. Generated4,472,304-byte object,
  projected headers and depfile scratch are removed; the small source preimage
  and reproduction receipts remain. This baseline proof does not admit the
  new hook or certify the whole current-header client.
  [Cache baseline compatibility](../.local/mcc-native-click-crash-20261018/map-cache-baseline-v1/validation.json).
- [x] Independently review and compile the integrated Cache hook with ordinary
  current headers. Exact source compiles warning-free in5.47s with451 dependency
  pins unchanged. Full-salt/unique lookup, immutable ownership, destination
  resolution before all guard checks, custom preservation and bounded copies
  precede generic conversion. Invalid metadata throws before any map write.
  The4,781,568-byte generated object and scratch are removed. This proves the
  source hook/ordinary compilation; actual-map execution and full client ABI
  acceptance remain separate.
  [Integrated hook review](../.local/mcc-map-qualified-conversion-20261019/review-v3/validation.json).
- [x] Execute normal Cache loading with the coherent integrated provider and
  compare each complete owned backing against V15. Lockout changes exactly its
  eight flyby-reference bytes; Relic changes exactly the required6.0→4.0 binary32
  word (one differing byte); Backwash remains byte-identical. The complete
  70,788,456/75,886,884/75,749,476-byte backings otherwise match, and every tag
  directory identity stays exact. All host backings exceed4GiB; source maps and
  315 parent inputs remain unchanged. Snapshot/binary/projected-header scratch
  is removed. Normal mod/fault controls, coherent native acceptance and broader
  map conversion remain open.
  [Normal Cache vanilla readback](../.local/mcc-map-qualified-conversion-20261019/cache-integration-v1/validation.json).
- [x] Verify normal Cache admission using a renamed stock map and two modified
  contexts. The renamed stock file receives exactly the intended eight-byte
  null flyby restoration, proving filename-independent qualification. Modified
  map-ID and header-path cases remain completely byte-identical to V15 with
  zero new writes. All backings exceed4GiB and full directory identities match.
  Source maps/315 parent inputs stay unchanged. The initial probe used a missing
  reader method; its setup failure is retained, then corrected to the checked
  pointer API. All generated map copies/snapshots/binaries/scratch are removed.
  This is focused mod preservation, not full mod-folder/render compatibility.
  [Normal Cache mod controls](../.local/mcc-map-qualified-conversion-20261019/cache-mod-controls-v1/validation.json).
- [x] Run the coherent map-conversion candidate through actual native SMG
  firing beside installed V15. Both complete60frames/ticks with identical
  complete capture state, decoded image, PNG and save outputs; loaded59,
  reserve120 and two detonations at tick4 match. Both PCM callbacks are finite/
  nonzero; sample identity is not claimed. All315 parent providers,106 projection
  origins,97 shaders,four saves, P libraries/maps and source pins remain exact.
  Private providers/ELF/link map/header/cache/save/raw-frame/audio scratch is
  removed. Two340,383-byte actual PNG references are explicitly retained.
  This accepts the bounded native shot path; installed V15/ZIP are unchanged
  and expanded map context/current-client/full parity gates remain open.
  [Native map-conversion firing](../.local/mcc-map-qualified-conversion-20261019/native-firing-v1/validation.json).
- [x] Authenticate broader Gauss context coverage before expansion. Fresh43
  MCC directory parses and38 pinned offline Xbox reads find28 paired contexts:
  five campaign and23 multiplayer, with22 null targets/six retained. All23
  multiplayer maps share exact mulg/Gauss donor owners, but distinct scenario
  banks/paths/IDs; only Relic's original CTF distance is4.0, the other22 use6.0.
  Campaign Gauss definitions have distinct salted handles/root addresses and
  bank digests, type0, and no resident mulg; do not require multiplayer owners
  or IDs for those profiles. MCC-only/resource contexts remain unqualified.
  A superseded audit Type offset is corrected to original schema`+10`. All
  source ownership pins stay exact; no banks/maps/binaries remain and both
  audit scratch trees are removed. Product expansion is a separate gate.
  [All-context authoring inventory](../.local/mcc-map-qualified-conversion-20261019/all-contexts-v2/inventory.json).
- [x] Expand the pure planner to all28 authenticated paired Gauss contexts.
  Warning-free controls pass all28 fresh owned fixtures and the prior three
  retained fixtures, covering exact original targets, all other owner bytes,
  idempotence, custom preservation, full identity rejection and guard atomicity.
  Five campaign profiles use exact resident scenario/Gauss identities without
  reading their zero-extent mulg owners. All23 multiplayer profiles keep full
  mulg and primary-globals-binding guards. Public source layout/serialized widths
  stay unchanged; the Cache hook remains exact. The4,289,689-byte private fixture,
  215,664-byte test ELF and generator/test scratch are removed. Only the13,215
  byte generated identity/hash/scalar table enters source. Independent expanded
  review, sanitizer/normal Cache/native acceptance remain open.
  [Expanded planner controls](../.local/mcc-map-qualified-conversion-20261019/expansion-v1/validation-regular.json).
- [x] Run the expanded28-context planner and retained three fixtures under
  AddressSanitizer/UndefinedBehaviorSanitizer, warning-free with no findings.
  The preflight function remains byte-identical to the reviewed version and
  the public header's noncomment lines remain exact. Source/table/test pins
  stay unchanged. The9,183,744-byte sanitized ELF,4,289,689-byte private fixture
  and scratch are removed; only small source preimages/receipts remain.
  [Expanded sanitizer controls](../.local/mcc-map-qualified-conversion-20261019/expansion-v1/validation-sanitizers.json).
- [x] Run all28 fresh owned contexts through the ordinary current-header
  CMake target/CTest registration. The positive fixture passes in0.39s and is
  not skipped. Source and donor pins stay exact; the4,289,689-byte generated
  fixture and private CMake build/scratch are removed. Normal Cache and native
  acceptance of the expanded table remain separate gates.
  [Expanded CMake validation](../.local/mcc-map-qualified-conversion-20261019/cmake-v2/validation.json).
- [x] Independently review every expanded profile against the pinned inventory
  and re-read79 original Gauss/CTF words. All28 exact identities/hashes/types
  match:22 null/six retained flybys, Relic-only4.0, campaign isolation and23
  multiplayer ownership/binding guards. Fourteen additional upper-bound/width/
  short-read/overlap/no-partial-write/unknown-map controls pass. Header layout,
  preflight bytes and Cache publication hook remain unchanged. The70,120-byte
  private probe and scratch are removed. This is expanded source review, not
  normal Cache/native or full parity acceptance.
  [Independent expanded review](../.local/mcc-map-qualified-conversion-20261019/independent-expansion-review-v1/validation.json).
- [x] Verify the expanded planner through all28 authenticated normal Cache
  constructor pairs plus the two unqualified MCC-only maps. Complete backings
  compare2,817,874,316bytes in RAM: only177 intended bytes differ, covering22
  null flyby references and Relic's4.0 stop-distance word. All other backing
  bytes and tag directories stay exact; five already-equivalent contexts and
  both unqualified maps stay exact. All60 host backings exercise addresses
  above4GiB. The315 V15 providers,97 shaders and both43-map inventories stay
  pinned. Scratch, the104,013,112-byte ELF and all private objects/headers are
  removed; no backing snapshots or banks were retained. This proves normal
  constructor/publication behavior, not rendered gameplay or full parity.
  [Expanded normal Cache acceptance](../.local/mcc-map-qualified-conversion-20261019/cache-all28-v1/validation.json).
- [x] Verify the expanded28-context provider in the native fixed-tick firing
  path against V15. Both runs exit0 at60frames/60ticks with exact decoded
  image, save outputs and full shot/capture state: loaded59/reserve120 and
  detonation tick4. Callback audio remains finite/nonzero; sample identity and
  full parity are not claimed. The315 parents,106 dependency origins, maps,
  four saves,97 bundled shaders, libraries and source pins stay exact. Scratch,
  private providers/runtime/map, PCM and cache are removed. Two340,383-byte
  actual PNGs are retained as required references, with identical hashes.
  [Expanded native firing acceptance](../.local/mcc-map-qualified-conversion-20261019/native-firing-v2/validation.json).
- [x] Retain a coherent expanded28-context candidate:316 link inputs comprise
  314 unchanged V15 providers, one replaced Cache and one added planner leaf.
  The retained104,754,568-byte ELF is byte-identical to the accepted ephemeral
  expanded candidate and independently passes the same60-tick shot against
  retained V15 frame/save/capture evidence. Callback PCM is finite/nonzero.
  Required delivery Cache/planner objects, ELF and owner map total130,543,356
  bytes; hashes/reasons and small frozen source pins are recorded. Temporary
  headers, PCM, saves/cache and scratch are removed. Both root/P selections,
  runtimes, launchers, sidecars, saves, maps, shaders and parent providers stay
  exact. This is a retained interim candidate, not installation/full parity.
  [Retained expanded cohort](../.local/mcc-map-qualified-conversion-20261019/cohort-v16/validation.json).
- [x] Verify interactive input against that exact retained candidate. Its
  private relocated104,708,392-byte payload survives all nine actual XTest
  enter/focus/click/release/held-fire/release/look/W/release phases for15.167s
  after frame readiness, with four live first-person particle draws and no
  runtime error. The harness intentionally sends SIGTERM after acceptance;
  this is not a normal-exit or desktop boot-speed claim. All316 providers,
  106 origins, maps, sources, four saves,97 bundled shaders, libraries, root
  material-workbench selection/runtime and P installation stay exact. Scratch,
  copied payload, private saves and caches are removed; no new large artifact
  is retained. Packaging and guarded P delivery remain separate acceptance.
  [Expanded actual input acceptance](../.local/mcc-map-qualified-conversion-20261019/native-interactive-v2/validation.json).
- [x] Package V16 with fresh build/acceptance authority and verify its actual
  relocated launcher under file tracing. Both info routes pass; the8-frame
  launch exits0 with no errors, reference-file or development-tree opens among
  108,605 attempts. All4,249 archive entries/checksums pass, only six runtime/
  release metadata files differ from V15, and97 shaders/tools/data/libraries
  stay exact. Root/P runtime metadata and saves stay exact; P cache is not
  written. Full-ready remains false. Required ZIP183,612,692bytes and118-byte
  checksum are retained; temporary hardlinks, private saves/cache/frame and
  31,098,772-byte trace are removed with scratch. Guarded P installation and
  independent archive review remain separate gates.
  [Compiled V16 package acceptance](../.local/mcc-map-qualified-conversion-20261019/delivery-v16/package-validation.json).
- [x] Independently review V16 archive metadata/delta. All4,249 entries and
  4,248 checksum rows pass: exactly six allowed files change and4,243 stay
  V15-identical, including97 shaders,2,600 tool files,190 libraries and1,235
  Python files. Current316-provider/raw/payload/sidecar/source/proof bindings
  agree; V15 evidence is explicitly historical. No game data/cache/save is
  bundled. P still matches its V15 preservation guards. Review scratch is
  removed, with no large artifacts retained or installer/native rerun.
  [Independent V16 archive review](../.local/mcc-map-qualified-conversion-20261019/archive-review-v1/validation.json).
- [x] Install only the verified six V16 files in P. All4,249 selected release
  files match the accepted archive; both P launchers select its updated wrapper
  with test available/full-ready false. All97 shaders, the entire P cache tree
  and four saves remain exact. Root's renderer workbench selection, runtimes,
  launcher/capability metadata and cache are not written. Required rollback
  copies total105,173,776bytes under P's
  `.native-rollbacks/map-vanilla-v16-5b411512d9f1/` and preserve the prior V15
  payload/metadata; installation scratch and
  staging files are removed. Actual relocated gameplay execution remains the
  prior accepted package proof; the installer performs no additional game run.
  [Guarded V16 extracted-folder installation](../.local/mcc-map-qualified-conversion-20261019/delivery-v16/install-validation.json).
- [x] Close the completed V16 delivery lifecycle. Fifty-one named completed
  scratch paths and the final verification scratch are absent. Fifteen required
  retained artifacts total420,351,091bytes: the requested ZIP/checksum, coherent
  delivery objects/runtime/map, exact V15 rollback files and three actual frame
  references. Every retained size/hash/reason is recorded. Other active agents
  and unrelated historical artifacts are excluded; final historical cleanup
  and the complete MCC/TU1.5 goal remain open.
  [Delivery artifact/lifecycle receipt](../.local/mcc-map-qualified-conversion-20261019/delivery-v16/delivery-closure.json).
- [x] Review V16 delivery metadata and evaluate all actual guards before
  executing package/install code. Fresh paired firing proof61ce and the
  retained candidate's separate ca00 shot proof have distinct roles; the316
  providers, all source pins and root/P preservation guards pass. New immutable
  delivery authority labels V15 ancestry as historical and uses current V16
  acceptance pins. A read-only private preflight passes and its scratch/temporary
  handoff are removed; no runtime/package mutation occurred in this step.
  [Reviewed delivery preflight](../.local/mcc-map-qualified-conversion-20261019/delivery-v16/preflight-validation-v2.json).
- [x] Resolve the supposedly absent normal visible-window pipeline-cache trace.
  Independent review and full-log regex extraction find it in both original
  eight-frame logs: 16,838,148 bytes loaded/exported, 241 graphics and one
  compute call, 0.037938/0.039212 seconds in pipeline creation. Buffered stdout
  and stderr interleave the message mid-line; the harness's `startswith` check
  caused the false negative. Preserve both original reports. No shutdown or
  atexit change is justified. Actual atomic file replacement remains a separate
  acceptance gate because the trace does not report IO success.
  [Corrected log audit](../.local/mcc-native-click-crash-20261018/pipeline-visible-correction-v1/validation.json).
- [x] Verify actual cache-file persistence in the current installed V13 normal
  visible route. Two separate eight-frame processes both exit0, replace the
  scratch-owned 19,233,358-byte cache with a new inode/mtime and mode0600, and
  consume exactly those saved bytes on the subsequent run. Pipeline creation
  takes 0.030262/0.028305 seconds. The first window appears at12.58seconds,
  frame5 at39.02seconds and exit at40.58seconds; the second exits at41.21seconds.
  These are isolated Xvfb observations, not physical-desktop or fixed speedup
  claims. All97 shaders and four user save files remain exact. Scratch
  `halo2-test-moun969j`, both temporary cache copies and copied saves are removed;
  no generated binary/image/audio artifact is retained.
  [Visible file persistence and reuse](../.local/mcc-native-click-crash-20261018/pipeline-visible-files-v1/validation.json).

- [x] Complete the twelfth bounded renderer reconstruction probe before changing
  texture upload hashing. Restoring the authentic particle-view API and removing
  the later `ModelView::source_w_distance` field reduces the mismatch to six of
  975 text/constant sections, with no added sections. This remains a rejected
  replacement: draw/material selection and buffer differences require proof.
  The ordinary renderer, installed runtime and shaders are unchanged, and
  scratch `halo2-test-ze5lw65d` is removed.
  [Probe evidence](../.local/mcc-native-click-crash-20261018/renderer-startup-v12/baseline.json).
- [x] Narrow that reconstruction to two differing text sections by restoring
  the retained environment-first accumulation predicate and the particle
  getter's original declaration order. All other 973 text/constant sections
  match, including texture creation; no new sections appear. This remains
  unaccepted for installation pending the draw and allocation-error comparison.
  Probe scratch `halo2-test-zu4fo4ia` is removed; product files are unchanged.
  [Thirteenth probe](../.local/mcc-native-click-crash-20261018/renderer-startup-v13/baseline.json).
- [x] Extend the reconstruction check to all 1,092 allocated ELF sections,
  normalized relocation targets and nonlocal symbol definitions. This confirms
  the two remaining functions also account for unwind/exception and relocation
  differences; raw text equality alone is insufficient for acceptance. No
  replacement is promoted. Probe scratch `halo2-test-rpo5dqqu` is removed;
  the small projected source and diagnostic contract remain as reference evidence.
  [Allocated-section comparison](../.local/mcc-native-click-crash-20261018/renderer-startup-v14/allocated-contract.json).
- [x] Independently review the remaining accumulation loop and buffer path
  against retained assembly/DWARF. The environment-first predicate, phase-one
  `emit_bsp_draw` call, original memory selection and early pStencil setting
  agree; no further semantic change is justified. The projected public renderer
  header also matches the original pinned `3fd32d4e...` bytes. Two function
  differences remain unresolved, so the optimization is not installed. Final
  checks preserve both installed runtime hashes and the ordinary renderer hash.
- [x] Review the proposed upload-bucket optimization independently. Replacing
  only the function-local pixel hash with a binary-length `string_view` hash
  preserves image-sharing decisions: identical pixels reach the same bucket;
  complete pixel/shape/mip equality rejects collisions, and candidate insertion
  order preserves the first exact owner. No bucket order or hash value is
  persisted or used for rendering order. Source lifetimes and shared GPU ownership
  remain valid. Keep the separate shadow/texture identity FNV unchanged. This is
  a design proof, not compiled/runtime acceptance or an installed optimization.
- [x] Test the source-location/code-generation hypothesis independently. Restoring
  original logical source paths and known function line numbers leaves the exact
  same two-function assembly diff as probe13; source-location metadata does not
  explain the remaining mismatch. Scratch `halo2-test-47vjg2e0` is removed and
  no product/provider replacement is made.
  [Source-location control](../.local/mcc-native-click-crash-20261018/renderer-startup-v15/baseline.json).
- [x] Exercise the reconstructed renderer in the actual coherent native game
  before optimizing it. With identical MCC Lockout data, original public-header
  bytes, fixed tick8, RNG seed, interpolation fraction and input, the selected
  V13 renderer and projection produce byte-identical 640x480 images and save
  outputs. Both exit0 with eight frames, at36.51/35.24seconds. The image was
  viewed. All315 parent inputs, all97 shaders, ordinary source and four user
  saves remain exact. This bounded test does not resolve the two compiled-body
  differences or certify full parity, and no product replacement is installed.
  Scratch `halo2-test-tgle3xk3`, projected archive/object/executable and raw frames
  are removed. Two 340,327-byte lossless PNGs remain as explicit comparison
  reference inputs.
  [Actual renderer comparison](../.local/mcc-native-click-crash-20261018/renderer-runtime-baseline-v1/validation.json).
- [x] Independently review the actual isolated optimized source against that
  runtime-tested projection. Its complete delta is two explicit standard
  includes and the local upload-bucket hash replacement; reversing them restores
  the baseline byte for byte (`9064a1a8...` to candidate `10a4a431...`). All full
  equality guards, ownership and the separate shadow FNV are unchanged. Compiling
  adds only the expected upload-lambda text difference alongside the two known
  baseline differences, with no new text/constant sections. Actual native
  optimized-frame and performance acceptance remain pending; source/install
  files are unchanged.
- [x] Run the optimized hash through the same actual fixed-tick native game.
  Its tick8 image and save outputs are byte-identical to installed V13; both
  exit0. This isolated pair takes36.86seconds for V13 and30.29seconds for the
  candidate, a6.57second observation rather than a guaranteed desktop speedup.
  Against the directly runtime-tested projection, all allocated code/data and
  semantic relocations remain exact except the intended upload lambda and its
  compiler-owned unwind/exception records; both prior differing functions and
  their exception/relocation records remain exact. All315 parent providers,
  all97 shaders, source and user saves are unchanged. Scratch
  `halo2-test-ye6atdon`, test archive/object/executable and raw frames are removed.
  Two340,327-byte PNGs remain as explicit reference inputs. Full input/firing,
  ordinary-source integration, installed delivery and wider parity remain open.
  [Actual optimized frame](../.local/mcc-native-click-crash-20261018/renderer-upload-hash-v1/validation.json)
  · [Allocated contract delta](../.local/mcc-native-click-crash-20261018/renderer-upload-hash-v1/optimized-contract-comparison.json).
- [x] Integrate exactly that reviewed local hash into ordinary renderer source,
  preserving all unrelated renderer work and all shaders. Reversing the two
  includes and hash replacement reproduces the exact prior `66bb984c...` source;
  its frozen preimage is retained. New ordinary source is `2daadf68...`. Current
  header compilation and firing/input delivery checks remain separate gates;
  installed runtimes and the V13 ZIP are unchanged.
- [x] Compile the exact ordinary updated renderer with current engine headers
  and normal configured compiler flags. The first attempt correctly reports the
  missing generated menu vertex catalog in the configured build directory;
  preserve that RED report. Supplying only the authenticated retained catalog
  include produces a warning-free26.04second compile with938 pinned dependencies
  and no input drift. The24,998,504-byte object and scratch `halo2-test-tapon9_j`
  are removed. This is current-source compilation, not a current-client link,
  installed release or full64-bit/parity acceptance.
  [Current-source compilation](../.local/mcc-native-click-crash-20261018/renderer-upload-source-v1/current-compile.json).
- [x] Compare real SMG firing through fixed tick60 with the optimized renderer
  against installed V13. Both render60frames, exit0 and produce byte-identical
  images, save outputs and complete capture-state rows: fire at3/release at4,
  loaded ammo59, reserve120 and two detonations. Both production audio callbacks
  contain finite nonzero samples; callback sample identity is not claimed.
  The independently rebuilt optimized ELF matches the earlier candidate's exact
  `23111ca3...` hash. All315 parent inputs,97 shaders and user saves are unchanged.
  Scratch `halo2-test-h4jf2jjg`, test binaries, archive/object, raw frames/audio
  and copied saves are removed. Two bounded PNG reference inputs remain.
  Live window/input, installed delivery and wider renderer/parity gates remain.
  [Firing/image/audio comparison](../.local/mcc-native-click-crash-20261018/renderer-upload-firing-v1/validation.json)
  · [Exact shot-state audit](../.local/mcc-native-click-crash-20261018/renderer-upload-firing-v1/firing-state-audit.json).
- [x] Link a reproducible coherent upload-hash candidate for guarded delivery.
  Its104,712,152-byte ELF again matches `23111ca3...`;314 other providers and all97
  shaders remain exact. The required94,403,022-byte renderer archive and
  20,568,715-byte owner map are explicitly retained with that candidate. All
  compiler/link scratch `halo2-test-o09an60q` is removed. This is an interim
  Lockout candidate, with full renderer/current-client64-bit/parity gates still
  open; installed runtimes remain unchanged pending live input qualification.
  [Candidate cohort](../.local/mcc-native-client-20261018/client-current-v14-upload-hash-v1/cohort.json).
- [x] Repeat actual pointer, focus, click, held fire, mouse look and forward
  movement with the packaged form of the upload-hash candidate. It survives all
  nine phases and15.16seconds after readiness, with four live first-person
  particle draws and no runtime errors; copied user saves remain exact. The
  isolated warm window appears at12.07seconds and frame readiness at32.00seconds.
  This is bounded Xvfb input acceptance, not a normal-exit or desktop-speed claim.
  Scratch `halo2-test-2kiwv7ag` and temporary payload/cache/saves are removed.
  [Live input qualification](../.local/mcc-native-click-crash-20261018/candidate-v8/validation.json).
- [x] Install the accepted upload-hash runtime into the source launcher and
  user's existing extracted folder, retaining exact prior binaries/capabilities
  for rollback. Both launcher variants in both locations report native test
  available and full readiness false. The actual installed user launcher
  completes eight frames successfully; all97 shaders,315 coherent inputs and
  four save files remain exact. Required rollback copies are retained; scratch
  `halo2-test-_q1rh1ry`, raw frame and temporary package payload are removed.
  The next compiled ZIP and metadata synchronization are still pending.
  [Installed upload optimization](../.local/mcc-native-click-crash-20261018/upload-installed-v1/validation.json).
- [x] Publish the user-requested compiled V14 ZIP with the accepted texture
  upload optimization. Both relocated launchers, the actual eight-frame route,
  every checksum across4,249 archive entries and all97 shaders pass. Other V13
  package files/tools/data remain exact; its43 importer controls are inherited
  evidence, not needlessly rerun. No original-reference/development-tree opens
  occur and no game data or saves are shipped. The183,596,975-byte requested ZIP
  has SHA256 `f31fc997595e3567de998e3acd6fcbaaba321689807efe8126f4e62c8b5034ed`;
  its118-byte checksum is retained. Scratch `halo2-test-x7ia7q28`, relocated
  staging, cache/save links, raw frame, trace and temporary ZIP are removed.
  Full renderer/gameplay/conversion/64-bit/loading/video/TU1.5 parity remains open.
  [V14 package validation](../.local/mcc-native-click-crash-20261018/package-v14/validation.json)
  · [Compiled V14 ZIP](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261019-v14.zip).
- [x] Synchronize the user's existing extracted folder to every V14 archive
  checksum. Five validation/capability/manifest/README/checksum files update;
  the already installed payload and all other package files match. Both actual
  launcher-info variants pass, full readiness stays false, and the user's MCC
  cache, saves and unlisted extras remain exact. Required501,563-byte rollback
  preimages remain; staging scratch `halo2-test-o08je0lx` is removed. No game-data
  copy or import is repeated.
  [Extracted-folder V14 synchronization](../.local/mcc-native-click-crash-20261018/sync-extracted-v14/validation.json).

- [x] Establish an automated real-window mouse reproduction on the user's
  exact V9 payload/cache, with copied saves on a private Xvfb display. Pointer
  entry and focus survive; live firing exits1 at the same `2839AD/48C488`
  codec read. The corrected readiness harness observes the delayed failure
  within 8.73 seconds of its first rendered readiness sample. Its cold test
  window appears at12.81seconds and frame readiness at97.63seconds; those are
  isolated Xvfb measurements, not physical-desktop startup certification.
  User save bytes are preserved and scratch is removed. The prior normal-mode
  run also reaches the same fault, but its frame-diagnostic readiness assertion
  was invalid and is retained as such.
  [Automated mouse RED](../.local/mcc-native-click-crash-20261018/v2/validation.json),
  [Actual live-input fault](../.local/mcc-native-click-crash-20261018/v2/runtime.log).
- [x] Authenticate the two live-input data extents against original TU1.5:
  the12-byte Boolean-selected codec8 decode triple at `48C488` is file-backed
  writable data; the8-byte comparison bounds at `46A7C0` are read-only.
  Twenty-four original selector prefixes and twenty-two guard continuations
  with genuine RET pass. These qualify exact bytes/access and reader selection,
  not codec children, the complete angular owner or full gameplay parity.
  Original/scratch inputs remain offline and scratch is removed. Product
  publication and the corrected live mouse sequence remain open.
  [Original20-byte authority](../.local/mcc-native-click-crash-20261018/authority-v1/primary-reviewed.json).
- [x] Publish the exact20-byte live-input compatibility in the ordinary data
  factory and review its compiled controls. All 2,442 previous tuples remain
  exact; the new factory has 2,444 regions/479,848 bytes. Original access is
  preserved: writable codec descriptor with warm writes/restore and read-only
  angular bounds. Twenty-eight rejecting controls, all twenty foreign overlap
  bytes before writes, two host pointers above4GiB, dependencies and strong
  exports pass. The primary-agent second-method review admits only these data
  owners; real mouse regression remains open. Test/link scratch is removed;
  the required 1,249,296-byte compatibility object is retained. The first
  control attempt caught a descriptive authority count error; the reviewed
  report correctly records24 selector cases and preserves the original report.
  [Ordinary20-byte compatibility](../.local/mcc-native-click-crash-20261018/product-v1/validation.json),
  [Source/access/ownership review](../.local/mcc-native-click-crash-20261018/product-v1/review.json).
- [x] Link the accepted20-byte leaf into coherent candidate v6. All 313 other
  providers, 43 MCC maps, 97 shaders and prior compatibility records remain
  exact. The required104,679,728-byte ELF and20,464,860-byte map are retained;
  link scratch is removed. Installed runtime and the user's V9 package remain
  unchanged. The actual mouse regression and startup stack sampling are the
  next gates; no fixed interactive launch is claimed yet.
  [Coherent live-input candidate](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v6/cohort.json),
  [Single-leaf link receipt](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v6/link-manifest.json).
- [x] Verify the missing codec row clears in the actual packaged-dependency
  mouse run. Candidate v6 reaches real first-person particle submissions and
  progresses to the codec child's first output store at `2962CF`, requiring
  `510EF0`; sustained firing still exits1. User saves and V9 remain unchanged,
  and scratch is removed. Sibling GDB attachment is denied by the host's
  existing ptrace policy; those attempts provide no startup stack evidence.
  [Actual v6 live-input boundary](../.local/mcc-native-click-crash-20261018/candidate-v1/validation.json).
- [x] Authenticate the complete codec8 quaternion, translation and scale
  producers and both required workspaces. Twenty original controls execute all
  three bodies through sixty real RETs with no child stubs. They cover every
  byte of the32-byte transform at `510EF0` and24-byte translation temporary
  bank at `56E6A0`; every workspace read follows the actual writer. Poisoned
  prior contents do not affect the complete output. Original writable virtual
  tails are explicitly loader-zeroed, rather than guessed initialized outputs.
  Scratch is removed; product ownership and a new live run remain open.
  [Complete codec workspace authority](../.local/mcc-native-click-crash-20261018/workspace-authority-v1/primary.json).
- [x] Publish and compile the complete56-byte codec workspace ownership.
  All2,444 previous factory tuples remain exact; the factory now has2,446
  regions/479,904bytes. Sixty-one rejecting controls include every one of the56
  foreign-overlap bytes before publication. Both host pointers exceed4GiB;
  original writable access, warm writes and memory-subowner restore pass.
  Strong exports, all314 previous inputs and six ordinary dependencies remain
  exact, with no reference opens. Scratch is removed; the required1,266,032-byte
  object is retained. Source review, coherent linking and live mouse acceptance
  remain separate gates.
  [Compiled workspace validation](../.local/mcc-native-click-crash-20261018/workspace-product-v1/validation.json).
- [x] Review the56-byte product by inverse source reconstruction and exact
  original owner/access comparisons. All prior factory rows, strong exports,
  dependencies and61 rejection controls remain verified; review scratch is
  removed. This is a second method by the primary agent, not an independent
  reviewer or proof of full interactive gameplay.
  [Workspace review](../.local/mcc-native-click-crash-20261018/workspace-product-v1/review.json).
- [x] Obtain four real startup stacks with the debugger owning only its private
  child. At4seconds the main thread is building MCC sound timing; at14 and28
  seconds it is loading/decompressing MCC textures, and at40seconds uploading
  textures. These samples identify work to measure; they do not establish its
  total cost or a startup improvement. The intentionally interrupted diagnostic
  removes scratch and leaves user saves/processes and security settings intact.
  [Actual startup samples](../.local/mcc-native-click-crash-20261018/startup-profile-v2/validation.json).
- [x] Link coherent candidate v7 with only the reviewed56-byte workspace leaf
  replacing v6's data leaf. All313 other providers,43 maps and97 shaders remain
  exact. The104,679,728-byte candidate and20,465,237-byte link map are retained;
  generated link scratch is removed. Installed/V9 bytes remain unchanged until
  actual interactive acceptance passes.
  [Coherent workspace candidate](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v7/cohort.json).
- [x] Run candidate v7 through real pointer/focus/click and held firing with
  the user's packaged libraries/cache. Both codec workspaces clear and real
  first-person particle draws reach4. Sustained firing next reaches its parent
  codec publication table at `28B1B1/48C7E0` and exits1; the full interactive fix
  remains open. Frame readiness is78.72seconds in this isolated run, without
  debugger attachment; this data-only change does not establish an optimization.
  Saves remain exact and generated scratch is removed.
  [Actual v7 boundary](../.local/mcc-native-click-crash-20261018/candidate-v2/validation.json).
- [x] Identify the final missing absolute operand in complete immediate parent
  `28B080`: `48C7E0` is a16-byte quaternion sign mask, not a dispatch pointer.
  Its original file-backed writable bytes and24 actual ANDPS instruction cases
  pass, including signed zero, infinity and NaN bit patterns. Other absolute
  parent operands fall within the existing decoder workspaces. This qualifies
  bytes/access and the static inventory, not whole-parent gameplay execution.
  The fixture was corrected to account for Unicorn reporting the16-byte SSE
  operand as two8-byte reads. Original programs remain offline; scratch removed.
  [Parent operand authority](../.local/mcc-native-click-crash-20261018/parent-authority-v1/primary.json).
- [x] Publish and compile the exact16-byte sign mask with its original
  writable access. All2,446 prior factory tuples remain exact;2,447 regions now
  own479,920bytes. Nineteen rejecting controls cover all16 foreign-overlap bytes,
  owner boundaries and warm remount; native pointers and warm writes/restore
  pass. The required1,274,584-byte leaf is retained and scratch is removed.
  [Parent-mask product](../.local/mcc-native-click-crash-20261018/parent-product-v1/validation.json).
- [x] Complete the sign-mask inverse-source/access review. Only its16-byte
  owner and preflight extent differ; all prior tuples/dependencies/exports and
  nineteen rejection controls remain exact. Review scratch is removed.
  [Parent-mask review](../.local/mcc-native-click-crash-20261018/parent-product-v1/review.json).
- [x] Link coherent candidate v8 with the reviewed parent mask. All313 other
  providers,43 maps and97 shaders remain exact; the required104,679,728-byte
  candidate and20,465,138-byte map remain retained. Link scratch is removed;
  live mouse acceptance and delivery are next.
  [Parent-mask candidate](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v8/cohort.json).
- [x] Fix the reproduced V9 click/firing exit in coherent candidate v8.
  Real SDL pointer/focus/click, one-second held firing, relative mouse look and
  forward-key press/release all survive, with4 actual first-person particle
  draws and no runtime error. The earlier user faults and decoder workspaces
  now clear. User saves,97 shaders,313 unaffected providers and43 maps remain
  exact; scratch is removed. This qualifies the reproduced input sequence,
  not all weapons, long sessions or full TU1.5 parity. Delivery remains open.
  [Actual mouse GREEN](../.local/mcc-native-click-crash-20261018/candidate-v3/validation.json).
- [x] Install the accepted click fix in both the ordinary source launcher and
  the user's extracted V9 package. All four launcher-info checks admit the
  native test and keep full readiness false. The actual user launcher completes
  eight frames with exit0 in55.61seconds. All existing save files and97 shaders
  remain exact; four pre-install runtime/capability files are retained for
  rollback and test scratch is removed. Startup experiments are excluded.
  The first smoke command used an unsupported save-directory argument; its
  private dialog was stopped, preimages restored, and the supported launcher
  command rerun successfully. That dialog wait is not a game boot measurement.
  [Installed click hotfix](../.local/mcc-native-click-crash-20261018/installed-hotfix-v2/validation.json).
- [x] Publish compiled version10 with the accepted real-click fix. Both
  relocated launcher-info checks and the real eight-frame launcher route pass.
  ZIP integrity and every bundled checksum pass; all97 shaders and unmodified
  V9 files remain exact, and no game data/save files are distributed. The
  historical V9 one-shot audio receipt is labeled as baseline evidence. Startup
  experiments are excluded. Generated package, hard-linked data, frame and trace
  scratch is removed; only the183,428,804-byte ZIP and checksum are retained.
  The initial cross-filesystem hard-link harness error is corrected by using
  the already qualified same-filesystem owned development cache.
  [Version10 compiled ZIP](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261018-v10.zip),
  [SHA256](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261018-v10.zip.sha256),
  [Portable hotfix acceptance](../.local/mcc-native-click-crash-20261018/package-v10/validation.json).
- [x] Locate the sampled renderer bottleneck at the actual full-image FNV
  byte loop (`19A13A0..19A13B4`) in the user's packaged ELF. An attempted
  ordinary renderer reconstruction does not match the frozen renderer's text
  sections, so it is excluded from this hotfix. Its disposable object and
  generated scratch were removed; shader files and selected renderer remain
  exact. Startup optimization requires a compatible seam or exact source
  reconstruction, not substitution of the current renderer body.
  [Renderer compatibility comparison](../.local/mcc-native-click-crash-20261018/renderer-startup-v1/baseline.json).
- [x] Minimize repeated MCC texture decompression into a red-capable native
  regression. Twelve reads of identical stored partition bytes reinflate in the
  selected runtime; the fixture detects that directly through the real zlib
  call. The107 selected MCC-cache dependencies are available at their exact
  recorded hashes, allowing an ABI-compatible leaf build. Reconstructed current
  and historical renderer bodies are excluded; no renderer is substituted.
  [Native partition RED](../.local/mcc-native-click-crash-20261018/partition-startup-v1/red.json).
- [x] Add ABI-compatible content-keyed MCC partition residency. The rebuilt
  baseline's text/constants match the selected MCC cache exactly, using all106
  recorded header dependencies. Native controls pass repeated-decode suppression,
  independent returned writes, same-path changed bytes, malformed rejections,
  concurrent reads and pressure beyond128MiB. Original partition/file validation
  and all public engine exports remain unchanged; one additional standard-library
  unique symbol is recorded. The cache owns at most128MiB and512 entries. The
  required compiled leaf is retained, scratch removed, and actual startup timing
  remains open. The first export assertion was corrected to distinguish that
  STL unique symbol from engine API changes; its receipts remain preserved.
  [Compatible partition validation](../.local/mcc-native-click-crash-20261018/partition-startup-v1/validation.json).
- [x] Link the startup candidate with only the accepted MCC-cache leaf changed
  from click-fixed v8. All313 other providers,43 maps and97 shaders remain
  exact. The104,684,912-byte ELF and20,477,032-byte map are retained and link
  scratch is removed. Actual timing and repeated mouse input are next.
  [Startup candidate](../.local/mcc-native-client-20261018/client-current-v13-startup-v1/cohort.json).
- [x] Strengthen partition regression with simultaneous first publication
  and a zlib-call assertion proving eviction actually occurs under pressure.
  The strengthened test passes along with the existing native MCC Cache suite
  (chunks, exact shared owners, bounds, spawns, pixel/residency behavior).
  All generated executables, maps and pressure-test files are removed.
  [Additional native regressions](../.local/mcc-native-click-crash-20261018/partition-startup-v1/supplemental.json).
- [x] Verify the combined partition-residency candidate through the same real
  pointer/focus/click/held-fire/look/movement sequence. It retains4 first-person
  particle draws, no errors and exact user saves; scratch is removed. Startup
  readiness is89.72seconds with startup load average13.60, versus75.15seconds
  at3.61 for the data-only candidate. These differing-load observations do not
  prove a startup benefit; the optimization is not accepted for delivery on
  timing evidence alone. Keep the proven click-only candidate available.
  [Combined real-input result and timing limits](../.local/mcc-native-click-crash-20261018/candidate-v4/validation.json).
- [x] Measure actual graphics-pipeline creation through transparent same-argument
  Vulkan wrappers around the unchanged coherent runtime. Before first presentation
  at147.83seconds,240 calls pass null application caches and spend87.30seconds
  inside pipeline creation. This probe uses a private empty XDG cache; it is not
  the installed desktop timing or an improvement result. All call results are
  passed through unchanged and scratch is removed. Next qualify persistent
  Vulkan pipeline residency without reconstructing the renderer body or changing
  shader programs.
  [Actual pipeline cost](../.local/mcc-native-click-crash-20261018/pipeline-profile-v1/validation.json).
- [x] Qualify the optional Vulkan cache boundary in17 control groups. Graphics
  and compute descriptions, counts, allocators, outputs and error results pass
  through exactly; explicit caller caches and extension chains are preserved.
  Warm persistence, corrupt UUID/oversize rejection, driver rejection fallback,
  allocation/I/O failures, concurrent access, opt-out and cache-before-device
  teardown pass, with opaque handles above4GiB. These are fake-driver contract
  controls, not actual GPU startup/image acceptance. Scratch is removed.
  [Pipeline-cache contract controls](../.local/mcc-native-click-crash-20261018/pipeline-cache-v1/controls.json).
- [x] Link the optional pipeline-cache owner into an isolated coherent candidate:
  all 314 prior providers, 43 map pins and 97 shader pins remain exact, with
  one new owner and four Linux Vulkan API wrappers. The renderer object and
  shader programs are unchanged. Compile/link scratch is removed. This is
  link qualification; actual GPU cold/warm and input acceptance remain open.
  [Pipeline-cache candidate](../.local/mcc-native-client-20261018/client-current-v13-pipeline-cache-v1/cohort.json).
- [x] Pass the matched actual-GPU disabled and cold-cache launches. The disabled
  control takes 78.65 seconds; cold-cache creation takes 74.08 seconds and
  persists 8,067,431 driver bytes with the correct device/UUID header. All 97
  shaders and user saves remain exact. This small cold-launch difference is
  not an accepted speedup; warm consumption and real input are still pending,
  and the enclosing benchmark owns live scratch until its final stage exits.
  [Actual GPU stages](../.local/mcc-native-click-crash-20261018/pipeline-gpu-v1/validation.json).
- [x] Complete actual-GPU warm disk-cache qualification. The second process
  consumes exactly the 8,067,431 saved bytes, completes in 44.39 seconds versus
  74.08 cold / 78.65 disabled, and spends 0.205 seconds in the same 241 graphics
  plus one compute pipeline calls versus 30.299 cold. Returned cache bytes are
  unchanged; all launches exit zero, produce native frames and preserve saves
  and all 97 shader files. Scratch is removed. These are bounded private-display
  measurements; first-launch compilation, the remaining startup work and actual
  desktop timing remain distinct. Real-input and ordinary-build wiring follow.
  [Completed GPU comparison](../.local/mcc-native-click-crash-20261018/pipeline-gpu-v1/validation.json).
- [x] Wire the optional owner and all four API wrappers into ordinary Linux
  `halo2_native` builds, with a registered standalone contract CTest. Full source
  configuration, the focused target build and the managed CTest pass; generated
  native link commands contain the owner and all wrappers. No engine-wide rebuild
  is claimed. Scratch is removed; interactive candidate acceptance remains open.
  [Ordinary build integration](../.local/mcc-native-click-crash-20261018/pipeline-cmake-v1/validation.json).
- [x] Repeat real-window input against the pipeline-cache candidate and portable
  shader relocation. Focus, click/release, held fire/release, mouse look and
  forward movement survive; four actual first-person particle draws confirm
  firing admission. No runtime errors occur, all user saves remain exact and
  scratch is removed. Cold rendered-frame readiness is 74.43 seconds; this
  input run does not claim warm-start or full gameplay/parity acceptance.
  [Pipeline-cache real input](../.local/mcc-native-click-crash-20261018/candidate-v5/validation.json).
- [x] Install the verified pipeline-cache candidate into the source launcher
  runtime and the user's extracted V9 payload, retaining exact previous runtime
  and capability preimages for rollback. All four launcher-info checks and the
  actual user launcher eight-frame route pass. Full gameplay-ready stays false;
  the native test remains available, with exact saves and all 97 shader files.
  Scratch is removed. Retain the small optional device-specific product cache
  produced by normal launcher execution to avoid recompilation.
  [Installed cache update](../.local/mcc-native-click-crash-20261018/pipeline-installed-v1/validation.json).
- [x] Publish the compiled V11 ZIP with the verified click fix and persistent
  Vulkan pipeline cache. Both launchers pass from a relocated folder with spaces,
  the actual eight-frame route passes under a file-open trace, every archive
  checksum verifies, and no Xbox/Vista/firmware reference opens occur. All 97
  shaders and unmodified V10 files remain exact; no game data or saves are shipped.
  Scratch and the large file trace are removed. First-launch and remaining startup
  costs, full gameplay/conversion and full Xbox parity remain open.
  [V11 portable validation](../.local/mcc-native-click-crash-20261018/package-v11/validation.json)
  · [Compiled V11](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261018-v11.zip).
- [x] Prepare a coherent combined startup measurement candidate containing the
  verified Vulkan cache owner and the previously byte-qualified bounded MCC
  partition residency object. The other 314 providers and all shader pins remain
  exact; link scratch is removed. This candidate is isolated and is not installed
  or packaged. Qualify it against identical warm GPU cache bytes before deciding
  whether partition residency belongs in delivery.
  [Combined startup link](../.local/mcc-native-client-20261018/client-current-v13-pipeline-partition-v1/cohort.json).
- [x] Complete two alternating actual-GPU launches per startup variant with
  identical copied warm GPU cache bytes. V11 measures 53.66 / 53.96 seconds;
  partition reuse measures 50.07 / 39.00 seconds. Every run exits zero and renders
  a native frame, with unchanged saves, providers and all 97 shaders. Pipeline
  calls/counts and warm bytes match. System load falls during the series, so
  these are bounded observations rather than a fixed speedup guarantee. Scratch
  is removed. Combined real-input and delivery acceptance remain open.
  [Alternating texture-partition comparison](../.local/mcc-native-click-crash-20261018/partition-pipeline-v1/benchmark.json).
- [x] Pass the combined candidate's real focus, click, held fire, mouse look
  and movement regression, with four actual first-person particle draws and no
  runtime errors. Copied warm GPU data is used; observed frame readiness is
  38.43 seconds at startup load average 6.98. User saves remain exact and scratch
  is removed. This qualifies the combined resource owners for guarded launcher
  installation; it does not establish a universal startup time or full parity.
  [Combined real input](../.local/mcc-native-click-crash-20261018/candidate-v6/validation.json).
- [x] Install the combined resource owners into both active runtimes after exact
  rollback preflight. Both launcher variants in both locations and the actual user
  eight-frame launch pass; saves and all shaders remain exact. A rejected hard-link
  preflight changed no product files; checked copies cover that filesystem and
  same-filesystem links preserve the other preimages without duplicating data.
  Scratch is removed. V11 stays available as the preceding compiled release.
  [Combined installed runtime](../.local/mcc-native-click-crash-20261018/combined-installed-v1/validation.json).
- [x] Remove this investigation's seven completed benchmark PPMs after checksum,
  single-link and process-open checks: 6,451,305 logical bytes removed. Keep two
  pixel-equivalent PNGs (340,909 and 341,049 bytes), small reports, source pins,
  required coherent providers, rollback files and releases. Cleanup scratch is
  removed; unrelated/reference assets are untouched. This does not complete the
  separate final goal-wide obsolete-test inventory.
  [Owned capture cleanup](../.local/mcc-native-click-crash-20261018/cleanup-startup-captures-v1.json).
- [x] Verify and select the already imported V3 graphics cache through ordinary
  cache validation, without another 31 GB source import. All 883 copied source
  files, five corrections and 40 mip chains qualify; both launcher variants,
  actual eight-frame gameplay and a fresh-save unmuted shot through tick 60 pass.
  Ammo reaches 59/120 with two detonations; finite nonzero callback audio and a
  current PNG are retained (576,044 and 340,817 bytes). Actual traced launches
  take 228.72 / 171.69 seconds under the current conditions; these are functional
  results and do not close the slow-startup gate. Rendered frames are observed
  dynamically (26 here), rather than incorrectly requiring 74. No reference-file
  opens occur; scratch, traces, raw PCM and disposable frames are removed. Prior
  cache, saves, shaders, V11 and all 315 providers remain exact. Full readiness
  and full graphics/audio parity remain false.
  [Fresh-cache native validation](../.local/mcc-graphics-corrections-v3-20261018/fresh-cache-native-v2/validation.json).
- [x] Check the refreshed V3 portable importer under the bundled Python: all
  21 bitmap and 22 onboarding controls pass. The V12 actual portable launch then
  times out at 240 seconds, so no V12 archive is published. The owned process
  group/scratch are removed, the failure receipt is kept and V11 is preserved.
  Retry with a longer diagnostic window; keep portable delivery and slow-startup
  gates open.
  [V12 timeout receipt](../.local/mcc-native-click-crash-20261018/package-v12/attempt-1/failure.json).
- [x] Complete the longer V12 portable check and publish its compiled ZIP. Both
  relocated launchers and actual eight-frame gameplay pass; all archive checksums
  verify. The five refreshed importer module/data files include the exact V3
  corrections, and all 43 bundled-Python controls pass. Existing shaders and other
  V11 files remain exact; no game data/saves are shipped and no reference or
  development-checkout file opens occur. All temporary copies, traces and frames
  are removed. Startup timing remains under investigation; the timeout is retained
  as diagnostic history rather than hidden by the later passing run.
  [V12 portable validation](../.local/mcc-native-click-crash-20261018/package-v12/validation.json)
  · [Compiled V12](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261018-v12.zip).
- [x] Separate actual untraced launcher window/frame readiness from complete
  exit. The visible private-display neutral run creates its window at 18.00
  seconds, reaches frame 5 at 52.91 and exits zero at 55.90 under high system
  load. Saves remain exact and scratch is removed. The earlier 315.83-second
  traced portable run is not ordinary startup timing. No final optional cache
  trace appears in the visible run; inspect late device teardown before claiming
  that this route persists or benefits from the application cache.
  [Actual untraced readiness](../.local/mcc-native-click-crash-20261018/actual-launcher-readiness-v1/validation.json).
- [x] Reproduce a pipeline-cache registry lifetime defect with a static device
  owner that retires after the registry's former destructor. The isolated
  late-teardown control crashes with SIGSEGV before the fix; scratch is removed.
  This is a demonstrated cache-owner bug, distinct from the previously fixed
  firing exit. Correct registry lifetime, then requalify ordinary controls and
  the visible route before promoting another runtime.
  [Late-teardown RED](../.local/mcc-native-click-crash-20261018/pipeline-lifetime-v1/red.json).
- [x] Keep the registry available for process-lifetime device lookup while each
  wrapped device teardown still exports and retires its own driver cache. The
  original 17 control groups and the new late-static-device CTest pass through
  the managed focused CMake runner. The exact prior source is preserved as a
  small reference snapshot, and the private build/scratch are removed. Native
  visible-route acceptance and promotion of this fix remain pending.
  [Lifetime controls](../.local/mcc-native-click-crash-20261018/pipeline-lifetime-v1/controls.json).
- [x] Link the lifetime correction as one replacement cache owner, keeping the
  other 314 providers and all 97 shaders exact. The prior provider remains intact
  for older coherent releases. Link scratch is removed; normal visible teardown
  and subsequent promotion are still pending.
  [Lifetime candidate](../.local/mcc-native-client-20261018/client-current-v13-pipeline-lifetime-v1/cohort.json).
- [x] Observe the corrected candidate rendering and returning zero through normal
  visible eight-frame operation, preserving saves and removing scratch. Its final
  original checker missed a mid-line cache trace and left the stronger assertion
  unaccepted at that time. Retain that diagnostic failure; the corrected log audit
  and subsequent actual-file persistence/reuse acceptance are recorded above.
  [Visible functional observation and limits](../.local/mcc-native-click-crash-20261018/pipeline-visible-v1/functional-observation.json).
- [x] Repeat real input with the lifetime-corrected coherent candidate. Focus,
  clicking, held fire, look and movement survive with four first-person particle
  draws, no runtime errors and exact user saves. Observed private warm-frame
  readiness is 42.52 seconds; scratch is removed. This accepts the independent
  lifetime correction for guarded installation. Visible-route file persistence
  was separately accepted by the later two-process check recorded above.
  [Lifetime candidate real input](../.local/mcc-native-click-crash-20261018/candidate-v7/validation.json).
- [x] Install the fixture-proven registry lifetime correction into both active
  runtimes after real input qualification. Both launcher variants in both locations
  and the actual user eight-frame route pass; saves and all shaders remain exact.
  Rollback preimages are retained and scratch is removed. This installation check
  did not certify visible/no-screenshot cache export; that separate acceptance
  now has the actual-file and subsequent-consumption evidence recorded above.
  [Installed lifetime correction](../.local/mcc-native-click-crash-20261018/lifetime-installed-v1/validation.json).
- [x] Publish compiled V13 with the lifetime correction and V3 MCC-only importer.
  Both relocated launchers, actual eight-frame gameplay, all 43 bundled-Python
  importer controls and every archive checksum pass. No reference or development
  checkout opens occur; all 97 shaders remain exact. Temporary build/cache links,
  traces and frames are removed. This supersedes V11/V12's registry lifetime
  implementation; their artifacts remain historical rollback evidence. Slow
  startup and full gameplay/parity stay open. The later installed visible-route
  file persistence and reuse check passes separately, as recorded above.
  [V13 portable validation](../.local/mcc-native-click-crash-20261018/package-v13/validation.json)
  · [Compiled V13](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261019-v13.zip).
- [x] Synchronize the user's existing extracted V9 folder to the verified V13
  package files without importing another game-data copy. Eleven tool/data/
  capability/metadata files update; all V13 archive checksums then match and
  both launchers retain native-test availability/full-ready false. Existing
  cache, saves and extra historical evidence remain intact. Checked rollback
  copies total 526,218 logical bytes, and update scratch is removed.
  [Extracted-folder V13 synchronization](../.local/mcc-native-click-crash-20261018/sync-extracted-v13/validation.json).
- [x] Narrow retained-renderer reconstruction from 73 differing sections to 13
  by restoring only the exact 24-byte private texture-mask vector documented in
  its own debug layout. No new sections appear; two old sections remain absent.
  This is a scratch-only baseline probe, not a replacement renderer or startup
  improvement. Scratch is removed and ordinary renderer/shaders remain unchanged.
  Inspect the remaining mask-related logic and accept no replacement unless the
  complete text/constant baseline matches the retained object.
  [Renderer layout reconstruction control](../.local/mcc-native-click-crash-20261018/renderer-startup-v4/baseline.json).
- [x] Restore the temporary baseline's existing texture-mask selection uses and
  the corresponding local graphics-mode definition scope. All 975 retained
  text/constant sections now exist with no new sections, and remaining mismatches
  fall to eight. The earlier compile-scope rejection is preserved; both probe
  scratch directories are removed. This still does not qualify a renderer
  replacement or hash optimization; compare the remaining code/relocations first.
  [Mask-logic reconstruction control](../.local/mcc-native-click-crash-20261018/renderer-startup-v6/baseline.json).
- [x] Reduce the temporary reconstruction to seven differing sections by
  restoring the independently selected mask material in geometry residency.
  A bounded diagnostic compares code/relocations and identifies two retained-only
  particle-subrenderer lifetime messages; the memory-preference substitution is
  rejected because it increases mismatches. All probes remove their scratch and
  retain only small diagnostics. No native/shader replacement is promoted.
  [Seven-section control](../.local/mcc-native-click-crash-20261018/renderer-startup-v9/baseline.json).
- [ ] Measure and reduce slow native startup through the real launcher/package
  path while preserving all existing shaders and Xbox TU1.5 behavior. Separate
  window creation from the first playable frame; do not call a bounded hidden
  capture a qualified interactive launch.

## Native gameplay frame compatibility

Compatibility checks below distinguish the private native frame cohort from
promoted product modules. Launcher delivery and full gameplay acceptance have
their own gates; passing a module test does not complete those gates.

- [x] Reproduce the MCC/Xbox bitmap layout failure in the actual native model path.
- [x] Supply owned CPU texture rows without rewriting canonical MCC source rows.
- [x] Handle both the ordinary bitmap accessor and the inline D5610 reader.
- [x] Compare the retained native DXT5 constructor/sampler against original
  execution: 40 cases, plus negative controls and sanitizer checks.
- [x] Complete the public native player/model test: 484-byte collector,
  33 rendered nodes, no reference-file opens, unchanged MCC map.
- [x] Supply the verified lighting constants, draw banks, collision lookup
  tables, cold query count, shadow history and fixed shadow bases reached so far.
- [x] Supply the complete authored shadow light/projection template and its
  proven publication storage: exact authored bytes, original execution controls
  and preservation checks pass.
- [x] Supply the one-byte scoped visibility-query flag required by `134C00`;
  original write/clear ordering and complete immediate static-owner coverage verified.
- [x] Promote the reviewed startup-data compatibility through V24 into product
  source; compile with current headers and preserve all prior factory state.
  The V24 promotion preserves the full 2,420-region/478,041-byte factory;
  current-header compilation, fog policy, overlap and warm-state controls pass.
  [Startup product evidence](../.local/mcc-angle-crt-20261018/product-promotion-v1/validation.json).
- [x] Verify and supply all six missing numeric constants in the retained
  `2CBA0` static draw closure (39 source bodies; 24 bytes, no mutable-state substitutions).
- [x] Supply the complete bounded `2E6ED0` selector and target tables:
  256 original dispatch controls pass; no fallback state or code backing supplied.
- [x] Verify and supply the full nested `4CBE0` sort tables and four repeat-model
  constants in V18 (109 bytes). The conditional child table supports only its
  four authenticated entries; broader shader-property admission remains unproved.
- [x] Supply the authenticated `object_light_interpolate` script boolean with
  original true default and warm script-write preservation (V19).
- [x] Validate the private sparse lens CPU-cache candidate with 54 setup and
  45 callback execution fixtures plus rejection of an unsupported coordinate
  lookup; retain the real renderer lease, images and original producer kernels.
  [Lens candidate evidence](../.local/mcc-lens-sdk-ownership-20261018/v2/validation.json).
- [x] Supply and validate the three authored texture-header lookup tables in
  the private lens adapter: prior sparse state is identical, 99 execution
  fixtures pass, including 21 format10 headers; supported formats are unchanged.
  [Table adapter evidence](../.local/mcc-lens-sdk-ownership-20261018/v3/validation.json).
- [x] Promote the validated sparse lens/table adapter into ordinary product
  sources while preserving current camera/clip-plane support. All 99 fixtures,
  shape and rejection checks pass; CMake source-specific floating-point flags
  match the accepted compile, with no original-runtime file opens.
  [Lens product evidence](../.local/mcc-lens-sdk-ownership-20261018/product-promotion-v1/work-status.json).
- [x] Supply the proved 20-byte shared renderer-mask cold field in private
  startup helper V20; original/native five-OR stores, observers, overlap
  rejection and warm-state preservation pass. Constructor completion and the
  full renderer-state lifecycle are separate from this cold backing proof.
  [Mask field evidence](../.local/mcc-model-bitmap-selector-20261018/v20/validation.json).
- [x] Supply the single missing authored `135970` view scalar in private V21:
  the original immediate closure is verified, prior startup owners are exact,
  and read-only/overlap/warm-mask controls pass.
  [View scalar evidence](../.local/mcc-model-bitmap-selector-20261018/v21/validation.json).
- [x] Supply and validate the missing fog suppression byte in private V22:
  81 original CPU gate cases and 81 actual C++ policy cases pass, with overlap
  and warm-state controls. The existing atmospheric-fog field and its whole
  frame-record owner remain unchanged. This does not prove a complete render pass.
  [Fog byte evidence](../.local/mcc-model-bitmap-selector-20261018/v22/validation-provenance-v2.json).
  The initial capture generator and its later complete-owner qualification are
  now explicitly separated and reproducible; the helper and raw behavior
  evidence are unchanged.
  [Provenance qualification](../.local/mcc-fog-control-storage-20261018/provenance-v2/validation.json).
- [x] Authenticate the next first-person finish dependency: original `16A2B0`
  reads exactly four bytes at `44A498`, authored FLOAT9 (`00001041`). It matches
  an existing development data definition. All 19 immediate owned bodies and
  their 82 fixed reads are reviewed; external, indexed and pointer-derived
  accesses remain separate.
  [Independent constant review](../.local/mcc-model-cpu-bitmap-20261018/first-person-finish-review-v1/root-primary-review.json).
- [x] Supply the exact read-only four-byte constant in private V23, preserving
  all 2,418 prior owner tuples. Factory, late-conflict, read-only, native-pointer
  and warm-state controls pass; Frame42 passes this scalar and reaches the
  separately recorded angle-wrap dependency.
  [V23 evidence](../.local/mcc-model-bitmap-selector-20261018/v23/validation.json).
- [x] Authenticate the next angle-wrap dependency: both complete startup
  producers, the 88-byte original dispatcher and its 418-row forward table
  prove callbacks `380D50` then `380D30`. Their two inputs already have exact
  owners; the two writable destinations need genuine initialization.
  [CRT admission review](../.local/mcc-model-cpu-bitmap-20261018/angle-crt-review-v1/qualified-review.json).
- [x] Validate both real callbacks in the private current-header World:
  24 original/native ABI, FP, memory and ordered-write cases pass; six actual
  World groups cover both constructor writes, eight angle queries, warm
  cancellation, same/fresh restore and store-denial fault recovery. The cold
  factory adds only eight bytes and preserves all prior 2,419 owners; no
  reference files are opened during native execution.
  [Private callback/lifecycle evidence](../.local/mcc-angle-crt-20261018/current-v1/validation.json).
- [x] Promote the reviewed angle callback integration into three ordinary
  product modules. All allocated sections, symbolic relocations, strong exports
  and actual dependencies match the accepted private candidate, so its complete
  CPU/World evidence applies. No Main, shaders or generated engine rebuild.
  [Product callback evidence](../.local/mcc-angle-crt-20261018/product-promotion-v1/validation.json).
- [x] Validate the promoted callbacks through ordinary native Main's tick3
  route: V7 submits two frames and exits successfully. This short route clears
  the demonstrated callback dependency; broader gameplay acceptance stays open.
  [Ordinary callback runtime](../.local/mcc-native-client-20261018/client-current-v7/run-v1-real-mcc/validation.json).
- [x] Compile and link the refreshed ordinary Main client with its required
  current-layout providers: 65 handwritten files plus the schema3 codec;
  successful private link and all 308 inputs verified.
  [Client link evidence](../.local/mcc-native-client-20261018/client-current-v4/cohort.json).
- [x] Verify the actual ordinary-client provider and COMDAT selection: all 65
  refreshed owners are loaded, no stale same-source by-value owner is selected,
  and 262 critical text sections in the audited type set have compatible owners.
  This bounded audit does not cover the newly identified nested clock-clip
  types below. Both linked Main
  allocation sites use 17,072 bytes. Actual runtime validation remains open.
  [Client selection evidence](../.local/mcc-native-client-20261018/client-current-v4/validation.json).
  The subsequent V22 helper-only client link preserves all 307 other inputs,
  all selected archive members and their load order.
  [Helper-only client evidence](../.local/mcc-native-client-20261018/client-current-v5/provider-preservation.json).
  Independent review rehashes all inputs and confirms that the sensitive
  by-value COMDATs select current owners; the stale model-pose return owner
  is absent.
  [Independent selection review](../.local/mcc-native-client-20261018/prefix-reader-review-v1/client-v4-selected-provider-review.json).
- [x] Refresh the demonstrated nested first-person clock-clip consumers.
  The retained content loader expects 40-byte clips and 752-byte bindings;
  current producers return 48-byte clips and 776-byte bindings. The actual
  crash was a 40-byte traversal of the current 48-byte vector. Expanded selected
  consumer checks and the actual V7 tick3 runtime now pass. This does not
  certify first-person geometry or every weapon's behavior.
  [Crash layout evidence](../.local/mcc-native-client-20261018/client-current-v5/diagnosis-v1/clock-clip-layouts.json).
- [x] Compile the four demonstrated stale clock-clip consumers with current
  headers and qualify all nine direct source users at 48-byte clips/776-byte
  bindings. Containing-type selection and the later V7 runtime pass.
  [Consumer compile evidence](../.local/mcc-native-client-20261018/client-current-v5/diagnosis-v1/clock-consumer-qualification.json).
- [x] Verify the exact cache-loader crash changes from red to green through
  ordinary Main: V5 reproduces SIGSEGV in 0.884 seconds; V6 returns from the
  same content-loading boundary in 1.595 seconds. Full native-world/render
  runtime remains open.
  [Loader regression evidence](../.local/mcc-native-client-20261018/client-current-v5/diagnosis-v1/minimized-green-v6.json).
- [x] Link the 69-source coherent V6 client and verify its expanded selected
  owners: the clock-clip traversal now steps 48 bytes; all 17 affected FP and
  19 containing-content COMDATs select fresh compatible owners. The narrower
  earlier audit is superseded for these nested types. Full runtime remains open.
  [Expanded client evidence](../.local/mcc-native-client-20261018/client-current-v6/validation.json).
- [x] Compile the two demonstrated nested ScriptVm consumers with current
  headers and pass the small VM fixture at 1/5/8/16/31 thread rows, including
  authority capture/restore and ticking. The retained-owner negative fixture
  aborts with SIGABRT; the full client's original failure is SIGSEGV in tick,
  so exact full-runtime validation remains open.
  [VM fixture evidence](../.local/mcc-native-client-20261018/client-current-v6/diagnosis-v2/vm-fixture-green.json).
- [x] Link and independently review the coherent 71-source V7 client with
  the promoted V24 World/CRT/data bundle: all 312 inputs and 307 unchanged V6
  identities are pinned. All 39 VM-family sections, 19 thread/container helpers
  and five SavedState helpers select current compatible owners. The exact
  full-client tick regression is closed by the successful run below.
  [V7 client evidence](../.local/mcc-native-client-20261018/client-current-v7/validation.json).
- [x] Close both exact ordinary-Main crash regressions: V7 returns from content
  loading, ticks the repaired VM and captures at simulation tick3; exit0 in
  94.999 seconds, two submitted frames, one local player. All 312 linked inputs,
  97 shaders and MCC maps remain unchanged; zero original-reference opens.
  Scratch is removed. This muted run does not prove audible output or parity.
  [Actual crash closure](../.local/mcc-native-client-20261018/client-current-v7/runtime-crash-regression-closure-v1.json).
- [x] Capture and inspect ordinary Main after native simulation updates.
  The actual tick3 image shows textured Lockout, HUD and the SMG. It also shows
  severe stretched first-person arm geometry and oversized HUD elements, which
  require source-led diagnosis before visual acceptance.
  [Native tick3 image](../.local/mcc-native-client-20261018/client-current-v7/run-v1-real-mcc/lockout-view.png).
- [x] Capture and inspect one actual native initial frame: Frame44 submits one
  view/frame at simulation tick0 and exits successfully. Textured Lockout
  geometry and HUD are visible. All 97 shaders/206 inputs/maps are unchanged;
  no original-reference opens. The original PPM capture is preserved and its
  PNG view has identical decoded pixels. No weapon is visible at tick0 and
  there are zero audio packets; this does not complete the gates below.
  [Initial frame](../.local/mcc-lens-bridge-20261017/frame-v44/run-v1-real-mcc/lockout-view.png),
  [Runtime evidence](../.local/mcc-lens-bridge-20261017/frame-v44/run-v1-real-mcc/validation.json).
- [x] Correct the demonstrated MCC first-person CPU vertex compression and
  local-to-global skin-map decoding. The actual 4,980-vertex Xbox/MCC arm pair
  now has zero wrong active indices (previously all 6,648 were wrong), with
  position and UV errors inside the original quantization bounds. This proves
  source geometry only; the ordinary Main capture and animation-pose parity
  remain separate gates.
  [CPU geometry regression](../.local/mcc-native-fp-geometry-20261018/product-v1/regression.json).
- [x] Verify the arm-geometry correction in ordinary Main at native tick3.
  V8-FP replaces only the proved model object and captures successfully after
  two frames/three ticks. Inspection confirms the giant stretched arm triangles
  are gone. All 312 input providers, 97 shader files and maps remain unchanged;
  there are no original-reference opens. HUD scale/placement and full Xbox
  animation/pixel parity remain open.
  [V8-FP image](../.local/mcc-native-client-20261018/client-current-v8-fp/run-v1-real-mcc/lockout-view.png),
  [Runtime validation](../.local/mcc-native-client-20261018/client-current-v8-fp/run-v1-real-mcc/validation.json).
- [x] Restore original logical bitmap sizes for the canonical MCC HUD banks
  without changing physical texture dimensions, pixels or the geometry kernel.
  The actual shared and Lockout metadata each pass all 70 banks/328 original
  frames and 708 altered-tuple controls. The extra MCC backpack page has no
  invented original owner; unknown/edited metadata is left unchanged. This is
  logical-size evidence; widget placement and the combined GPU capture remain open.
  [Logical-extent validation](../.local/mcc-hud-bitmap-geometry-20261018/v1/validation.json).
  The independent source/data review and actual CTest generator check also pass.
  [Independent review](../.local/mcc-hud-bitmap-geometry-20261018/independent-review-v1/review.json),
  [CTest manifest check](../.local/mcc-hud-bitmap-geometry-20261018/v1/ctest/report.json).
- [x] Convert the demonstrated canonical HUD placement differences through
  normal owned Cache metadata: 17 owners/259 stored-field patches, preserving
  the existing scoreboard path. Complete bitmap-geometry guards reject edited
  layouts while permitting texture-only replacements. Portable, actual
  shared/Lockout/Cairo, sanitizer and generator checks pass; original fields,
  screen references and shared curves are preserved. Whole-frame acceptance
  remains separate.
  [HUD metadata publication](../.local/mcc-hud-logical-extent-audit-20261018/v2/validation.json).
- [x] Complete a visually valid bounded native gameplay frame after the
  compatibility updates, including first-person geometry and HUD presentation.
  Root inspected V18's complete Lockout frame; its decoded pixels and save
  outputs match the accepted V16 fixed-tick run. Full material/scene and Xbox
  framebuffer parity remain separate open gates.
  [V18 fixed-tick acceptance](../.local/mcc-material-index-audit-20261019/successor-v18-executor-v1/validation.json).
- [x] Resolve the combined V9 namespace admission failure through the normal
  Cache publication order. Stage the new HUD owners until after the existing
  shader/globals bootstrap, preserving the authenticated sound-class namespace.
  The exact V9 failure remains recorded; minimized regression, portable and
  actual-cache checks, and sanitizer checks pass. Ordinary Main V10 now exits
  successfully after two frames/three ticks instead of failing before rendering.
  [Namespace regression validation](../.local/mcc-hud-namespace-integration-20261018/v3/validation.json),
  [Combined V10 run](../.local/mcc-native-client-20261018/client-current-v10-hud-mcc/run-v1-real-mcc/validation.json).
- [x] Capture and inspect the combined arm/HUD corrections through ordinary
  Main. V10 shows normal arm geometry and the restored logical sizes and
  placements of the reticle, radar, ammunition and grenade widgets. All 314
  providers, 97 shaders and maps are unchanged during execution. Its PNG view
  preserves every decoded source pixel. This bounded tick3 image does not
  establish complete materials, animation, gameplay or TU1.5 framebuffer parity.
  [Combined native image](../.local/mcc-native-client-20261018/client-current-v10-hud-mcc/run-v1-real-mcc/lockout-view.png),
  [Lossless image conversion](../.local/mcc-native-client-20261018/client-current-v10-hud-mcc/run-v1-real-mcc/image-conversion.json).
- [x] Independently audit the complete V10 file trace against known Xbox,
  Vista, MCPX and original-reference roots, beyond the initial filename filter.
  All 107,928 open attempts are parsed; the only four map opens are validated
  MCC maps. No reference-bank path is accessed. Two Vista-named compiled shader
  files belong to the preserved shader bank and are explicitly distinguished
  from asset banks. This covers observed pathname operations, not unidentified
  aliases or inherited descriptors.
  [Independent file audit](../.local/mcc-native-client-20261018/client-current-v10-hud-mcc/run-v1-real-mcc/file-audit.json).
- [ ] Inspect the captured image and resolve material rendering defects.
- [x] Repeat the bounded capture through ordinary compiled Main/client, reusing
  ABI-compatible objects; V7 reaches tick3 and returns successfully.
  Checkpoint schema2→3 grows from 824 to 872 bytes; match every by-value consumer
  before combining cohorts. See the [ABI audit](../.local/mcc-native-client-20261018/authority-abi-audit-v1/validation.json).

Latest GPU capture: coherent V13 completes ordinary unmuted tick30 with the
class1 correction and reviewed counter/response data. Root inspected its image
and the version8 packaged image: the oversized left edge is gone, with arms,
SMG and HUD retained. All 97 shaders remain unchanged. V13 is now selected by
both installed launchers and is shipped in the validated version8 ZIP; the
actual installed eight-frame route also passes. Further firing-table work,
complete materials, audio timing and full TU1.5 acceptance remain open.
[Current native frame](../.local/mcc-native-client-20261018/client-current-v13-class1-fire-mcc/run-v1-real-mcc/lockout-view.png),
[Actual combined run](../.local/mcc-native-client-20261018/client-current-v13-class1-fire-mcc/run-v1-real-mcc/validation.json).

Frame44 records the bounded initial frame above.
Its one-frame harness uses the renderer's existing default shader directory;
all 97 files match the pinned copies exactly and product capture guards are
unchanged. It advances zero simulation ticks and is separate from ordinary Main.

Latest multi-tick attempt: Frame42 passes the sparse lens, texture-header
tables and shared mask dependencies. The actual lens query route completes
with zero issued queries/flares and stamp=1; this does not test visible flares.
It passes the `135970` view scalar, first-person particle setup and the V22 fog
control read and the exact `44A498` finish constant. The next actual root
`168CA1` stops at `C1F96` while accessing `5564C8`; its constructor/lifetime
must be verified before supplying backing. Table presence does not
admit unsupported format/flag combinations.
The selected-object shadow route still completes with queue=1, roots=1 and
history=1. No image is produced; all 97 shaders and 206 linked inputs are
unchanged, with no original-reference opens. Evidence:
[Frame42 result](../.local/mcc-lens-bridge-20261017/frame-v42/run-v2-real-mcc/validation.json).
The diagnostic requests simulation tick3. A later update failure before that
capture does not prove that earlier draws were never submitted; Frame44 records
the separate initial-frame boundary directly.
The current-header Main client links privately and passes the bounded final
provider/COMDAT audit and independent helper-only transfer review. Its first
actual V5 launch exits with SIGSEGV after 0.665 seconds, before producing a
frame or stdout. GDB reproduces the content-loader crash and exact selected
object layouts establish the nested clock-clip/binding mismatch described above.
All linked inputs, shaders and MCC maps remain unchanged;
no original-reference files are opened. V6 fixes this specific loader crash;
the later native tick failure remains a separate acceptance gate.
[Ordinary-client run](../.local/mcc-native-client-20261018/client-current-v5/run-v1-real-mcc/validation.json).
After the proved loader fix, V6 enters native-world/effect setup, then exits
with a later SIGSEGV after 71.664 seconds, before its requested capture.
The reproduced backtrace is `ScriptVm::tick`: retained nested thread records
are 64 bytes versus current 88 bytes, despite the same 408-byte outer VM.
V7 passes the recursive nested-owner and continuation/deque selection review
and the exact full-runtime rerun. All 311 V6 inputs, 97 shaders and
MCC maps remain unchanged, with no original-reference opens.
[V6 native run](../.local/mcc-native-client-20261018/client-current-v6/run-v1-real-mcc/validation.json).

Component evidence:
[V13 shadow template](../.local/mcc-model-bitmap-selector-20261018/v13/validation.json),
[V24 product startup-data promotion](../.local/mcc-angle-crt-20261018/product-promotion-v1/validation.json).

## MCC-only onboarding and asset ownership

- [x] Recover the actual tag1009 dispatch from both authenticated MCC outer
  frontend dispatchers. Each bounds-checks index9 and uses table`1432DB020`,
  cell`1432DB068`, to reach`140613CBC`. Its complete181-instruction body
  identifies `DefineExternalImage` and calls the complete184-instruction child
  at`140613A18`. Only small static receipts/disassembly are retained; temporary
  disassembler dependency and scratch are removed. Descriptor fields, active
  image binding, the missing mask and music resource remain open; this does
  not justify product wiring or fabricated loading assets.
  [Checked dispatch table](../.local/mcc-loading-production-20261018/tag1009-static-20261019-v1/case9.json)
  · [External-image child](../.local/mcc-loading-production-20261018/tag1009-static-20261019-v1/image-child.json).
- [x] Seal the two actual loading-GFX external-image records against the direct
  scalar/string readers: resource`90001`, format13,92x92, name
  `loading_screen_i7.tga`; resource`90002`, format13,1280x720, name
  `loading_screen_i1.tga`. Both exports are empty. The56-byte owner stores name
  at`0x20` and export at`0x30`; the later creator's`0x48` field belongs to a
  different structure. Resource registration`1405F0EFC` and that adapter remain
  unproved, so no conclusion about the creator's input string or image binding
  is admitted. All probe/dependency scratch is removed; no asset, executable
  or shader changes are made.
  [Raw record and owner evidence](../.local/mcc-loading-production-20261018/tag1009-static-20261019-v1/validation.json).
- [x] Decode the proved external-image registration owner`1405F0EFC` completely:
  264bytes/69instructions. Its constructor-consumed factory slots`+8/+10`
  resolve to`1405A6854`/`1405ABCAC`; later slots and the image-creator adapter
  are not inferred. Small source/disassembly receipts and the corrected local
  probe-setup failure are retained; temporary dependency/scratch is removed.
  [Registration evidence](../.local/mcc-loading-production-20261018/tag1009-registration-20261019-v1/registration.json).
- [x] Qualify the registration queue without assuming a creator slot. The
  `+130` search yields only its known writer; proved pending head`+398` instead
  flows through`1405F36F8` to a new node`+28` while count`+37C` enters`+10`.
  Frame-like nodes use load-data`+100/+108` and next`+30`. The next missing
  bridge is that new node's`+28` consumer. No factory create call, creator
  descriptor, mask or ambience is inferred; extraction stops at this verified
  boundary. Small receipts remain, and all dependency/probe scratch is removed.
  [Qualified registration boundary](../.local/mcc-loading-production-20261018/tag1009-registration-20261019-v1/validation.json)
  · [Pending-head flow](../.local/mcc-loading-production-20261018/tag1009-registration-20261019-v1/pending-xrefs.json).
- [x] Find the actual queued-frame consumer in the same bounded GFx cohort.
  Owner`1405F5148` is3,115bytes with a pinned source hash; its reads at
  `5F51F1/5F5266` take load-data`+100` into r14 and advances at
  `5F51E3/5F5258` follow node`+30`, matching the proved queue. Nine qualified
  instruction-aligned read sites span eight functions inside the fixed655,360
  byte region. Scratch is removed. The node`+28`/factory dispatch and final
  image descriptor remain unproved; no image role or loading mask is inferred.
  [Qualified queue consumer](../.local/mcc-loading-production-20261018/tag1009-queued-consumer-20261019-v1/queue-consumers.json).
- [x] Prove the actual stored-factory dispatch inside that queue consumer.
  At`5F57E7` queued node`+28` supplies the registration chain; count`+10` and
  registration next`+18` bound traversal. `5F5829/5831` load its factory/vtable,
  `5F5840` supplies raw owner`[rbx+8]` in rdx, and`5F5844` calls virtual`+18`.
  This authenticates that precise factory slot; it is no longer inferred from
  adjacent entries. Slot target, raw-owner translation and creator descriptor
  remain the next proof, with no image-role/mask/audio conclusion yet.
  [Actual factory call](../.local/mcc-loading-production-20261018/tag1009-queued-consumer-20261019-v1/consumer.json).
- [x] Resolve that precise factory slot to`1405F14B0`, a complete1,722-byte/
  460-instruction owner with pinned source bytes. Its conditional branch for
  nonzero raw-owner`+18` creates a descriptor at rsp`+40`, assigns raw`+30` to
  descriptor`+48`, then calls image-creator virtual`+18`. The zero branch uses
  another descriptor and virtual`+10`. This proves a conditional structure
  bridge, not the real loading records' selected branch, assignment semantics
  or bound image-creator implementation. All three probe scratch trees are
  removed. Those remaining bindings still precede any image/mask inference.
  [Actual factory and conditional bridge](../.local/mcc-loading-production-20261018/tag1009-queued-consumer-20261019-v1/factory-dispatch.json).
- [x] Decode the descriptor assignment helper`140634748` and tag1000 header
  parser`1405ECE94` completely. Assignment retains destination tag bits and
  either shares/refcounts matching-allocator source backing or copies its exact
  length before releasing old backing. The parser reads the actual preceding
  GFX tag1000 version`0401`, flags, format and strings, then calls metadata
  setter`1405ECD64` into stream`+2B0`. Header-state flow into data`+80` remains
  unproved, so the actual loading branch is still open. Scratch is removed;
  small primary disassembly/receipts remain, with no product or role inference.
  [Exact header/assignment owners](../.local/mcc-loading-production-20261018/tag1009-field-state-20261019-v1/fields.json).
- [ ] Resolve the bound image-creator implementation before pixel/resource binding.
- [x] Seal the canonical real-record branch: actual preceding tag1000 format13
  flows through parser/setter and copy caller into data`+80`, then constructor
  raw`+18` and factory nonzero→creator`+18`. Assignment semantics and actual
  nonnull zero-length backing are proved; empty exports do not imply a null
  pointer or zero branch. The47-byte parser log leaf preserves the argument on
  its normal cookie path. All four private environments are removed. This is
  successful canonical-parse proof for the exact records, not live MCC invocation
  or final creator/pixel/resource/mask/audio acceptance. Creator binding remains
  open in the preceding gate.
  [Sealed real-record branch](../.local/mcc-loading-production-20261018/tag1009-field-state-20261019-v1/validation.json).
- [x] Prove the exact original load-state identity through context and bind
  process storage at`+18`, distinct creator-owner construction/publication at
  state`+10`, and the default owner's seven requested IDs
  `[12,13,14,18,19,30,31]`. The third returned pointer goes to owner`+20`;
  explicit copying preserves it with refcounting. Known registered creator
  vtable slots separately resolve`+18` to`8EC954` and`+10` to`5E9814`.
  All six private extraction environments are removed. The bulk-state query
  implementation and source interface remain required before identifying that
  actual third result with the registered creator; pixels/mask/audio stay open.
  [Exact load-state/creator ownership](../.local/mcc-loading-production-20261018/tag1009-loadstate-binding-20261019-v1/validation.json).
- [x] Prove the exact bulk-state lookup and registration chain. Public loader,
  implementation interface and backing owner forward the same IDs/pointers;
  lookup matches embedded type, advances IDs4bytes/results8bytes and places
  ID14's result in the creator field. This conditionally binds the registered
  type14 creator and its`+18→8EC954` slot for the same successful loader with
  no override/replacement. The actual format13/empty-export branch then reaches
  fallback`5E9B98`. All eight extraction environments are removed. Active UI
  instance ownership and actual external resource bytes remain unproved; no
  pixels, mask, audio or product wiring are inferred from this static chain.
  [Bulk query and conditional creator binding](../.local/mcc-loading-production-20261018/tag1009-loadstate-binding-20261019-v1/bulk-validation.json).
- [x] Qualify canonical UI file-opener setup and the empty-export fallback.
  Initializer`8AB59C` installs UI-client`+390` as state12; bulk result0 reaches
  descriptor`+20` and file-opener`+8→8EBBF8`. Fallback`5E9B98` first tries the
  supplied/rebased name, then a `.dds` alternate only on failure. The exact
  opener strips directory/final extension before the hash; eight actual-source
  CPU basename controls pass. Conditional record basenames i7/i1 produce keys
  `56DBA897/4C5DE525` in namespace`442A39D9`, passed to provider`+60`.
  Name rebasing, actual provider row/resource bytes and active loading requests
  still require proof; no pack/image-role/pixel/audio conclusion is inferred.
  All five private environments are removed, with no large artifact or product
  edits. [Canonical opener and fallback](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/validation.json).
- [x] Qualify the actual namespace registry lookup with a branch-complete
  source CFG. Getter`889394` lazily returns global`14400C200`;`889444` caches
  the provider/type at registry`+58/+60` and returns found node minus8.
  `5A5534` is a tree lookup, with key`+18`, children`+8/+10` and registry
  sentinel`+20`; its74 reachable bytes are authenticated despite lacking a
  PE unwind entry. Initial first-return/body assumptions were corrected before
  certification. All five private environments are removed. Registry provider
  registration, actual asset rows and active loading invocation stay open.
  [Exact registry owners](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/registry-execution.json).
- [x] Scan exact registry-global references in the authenticated42,370,020-byte
  text cohort, accepting all nine candidates only at complete primary-fragment
  instruction boundaries. The two newly reached owners remove providers/drain
  auxiliary lists and reset state; neither binds an asset provider. Known getter
  `889394` remains the next direct-call source frontier. This is a bounded
  initialization/teardown result, not proof that no indirect registration exists.
  Both scanner/owner private environments are removed, with only small source
  reports retained. [Exact-global cohort](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/registry-global-xrefs.json)
  · [Teardown owners](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/global-owner-bodies.json).
- [x] Qualify221 exact direct getter callers at primary instruction boundaries
  and their bounded successors. Actual insertion owner`8888D0` passes the
  returned registry/provider to`5A5170`; removal uses`5A52B4`. Of36 qualified
  constructor callers, only`0E14FC` supplies exact namespace`442A39D9`: it
  zeroes the same global288-byte owner, registers it and installs a concrete
  vtable. Constructor owner`+20` matches tree key/node`+18`; lookup returns
  that same provider. All completed environments are removed. Exact selected
  provider`+60`, resource rows/bytes and active loading requests remain open.
  [Getter caller cohort](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/registry-getter-callers.json)
  · [Exact namespace constructor](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/provider-selection.json).
- [x] Seal the exact provider registration/query bridge: source-labeled
  `UIScreenInventory`, namespace`442A39D9`, global`143FA4350`, table`143470980`
  and consumed slot`+60→8891D0`. Resource lookup queries provider`+50`;
  successful opener reads length`+58` and relative bytes`+60`. The11 private
  environments are removed. A subsequent exact-global scan finds only three
  qualified constructor references; three no-pdata/data candidates remain
  excluded, so no resource-population absence is claimed. Its scanner is
  removed. Actual rows/bytes and loading invocation remain open; the direct
  file fallback is another required source path before image/pixel inference.
  [Sealed provider bridge](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/registry-binding-validation.json)
  · [Qualified provider-global references](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/provider-global-xrefs.json).
- [x] Qualify the opener's separate direct-file fallback and VFS calls.
  `8B2954` resolves the supplied/rebased path through VFS`143FA46C0`, creates
  a file owner and opens through the selected device's virtual`+88`. Actual
  `8B1C98` expands variables and invokes registered rewrite callbacks;
  `8B1E84` uses an optional device selector or its source-addressed default.
  The selected device and resolved path are stored on the file owner. Both
  private environments are removed. These active rewrite/selector choices,
  canonical device setup and actual byte reads still require qualification;
  no filesystem path, inventory contents or pixel role is guessed.
  [Exact file fallback](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/file-fallback-owners.json)
  · [VFS owners](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/vfs-owners.json).
- [x] Qualify the canonical default file-device constructor, open method and
  startup initialization. The exact virtual`+88` target uses the resolved path,
  UTF-8 conversion and read-existing `CreateFileW`; startup links this device
  into the VFS list and calls its installed virtual`+8` initializer. Constructor
  zeroes establish initial callback/selector state only. The bounded52-reference
  successor audit establishes no active path-hook or image-resource producer;
  indirect/later hooks remain open. Recorded scratch is removed, with no large
  artifacts or product changes. Actual loading invocation and image bytes are
  still required before a reconstruction claim.
  [Default device proof](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/default-vfs-validation.json)
  · [Actual startup dispatch](../.local/mcc-loading-production-20261018/tag1009-fallback-20261019-v1/default-init-owner.json).
- [x] Check the source-proved basename keys against every qualified stored key
  in the four known texture packs: neither matches any of2,329 revalidated
  `NameHash` fields. The shared PCK has7,247 string-tag rows and the cache
  directory13,661; neither schema has a numeric name-hash field. The ambiguous
  previous47,247-row label is corrected. This finite negative supplies no image
  bytes or active loading owner. All three private environments are removed;
  the distinct `UIScreenInventory` producer remains open.
  [Qualified key candidates](../.local/mcc-loading-production-20261018/tag1009-key-candidates-20261019-v1/validation.json).
- [x] Qualify the primary MCC loading-audio service's initializer-linked
  default provider. Independent review authenticates67 instruction-boundary
  references, its exact initializer/global publication, vtable`+30` target and
  actual loading-event caller. Eighteen bounded x64 CPU cases/255 instructions
  cover success/null allocation, ignored arguments and start/stop dispatch.
  This exact provider reads no event/parameter payload and supplies no proved
  resource/PCM binding. Its`RET 0` leaf preserves RAX; it does not return zero.
  Live service selection, actual MCC music resources and audible loading remain
  open. Explicit fixture boundaries and extractor failures are preserved; all
  three review scratch paths are removed, with no large artifacts or engine
  edits. This negative result justifies no substitute ambience.
  [Default loading-audio provider review](../.local/mcc-loading-production-20261018/sound-service-binding-20261019-v1/independent-review-v1/validation.json).
- [x] Inventory exposed Halo2/frontend audio metadata without extracting banks.
  Twenty-four Halo2 candidates and112 exposed text files supply no exact loading
  music event. `Data/audio/soundbanks.xml` names the frontend banks; no matching
  loose banks exist. The packed owner is
  `MCC/Content/Paks/MCC-WindowsNoEditor.pak`; its4,096-byte footer and4,742,298
  byte metadata index authenticate by the footer's SHA1. Index strings name13
  banks and379 media files, including the named Windows banks. These are index
  strings, not parsed entry/event/resource bindings. All scratch/index copies
  are removed with no large artifact, cache/import or engine edits. Another
  game's loading FSB remains outside Halo2 authority.
  [Packed frontend audio inventory](../.local/mcc-loading-production-20261018/frontend-audio-inventory-20261019-v1/validation.json).
- [x] Parse all30,667 frontend pak entries from source-qualified version7
  serialization, consuming the complete4,742,298-byte index with its exact
  SHA1. The three Windows bank entries match their duplicated53-byte headers:
  Init1,483bytes, Main_Bank4,008,466bytes and Update1 32bytes; method0/flags0.
  Localized banks remain separate. Actual index records also identify
  `SoundbanksInfo.xml`, `Main_Bank.txt` and `Init.txt`, whose payloads remain
  unread at this gate. No large bank/media file is extracted and larger payload
  hashes are unverified. All private downloads/scratch and six generated source
  excerpt copies are removed; only independent format facts/provenance remain.
  Event/media binding and original loading-audio identity remain open.
  [Complete index and bank-header proof](../.local/mcc-loading-production-20261018/packed-frontend-banks-20261019-v1/validation.json).
- [x] Authenticate and parse the three packed Windows audio metadata payloads
  (285,025bytes) after matching entry headers and SHA1. Authored loading media
  `236657329` is named `MUS_Menu_Loading_Screen_01` and maps to
  `236657329.wem`. The separate `Play_Music` event is `2932040671`; XML supplies
  bank-level membership and no explicit event-to-media selection edge. Names do
  not prove the active Halo2 request or original loading-audio identity. No bank
  or WEM payload is read at this gate; scratch is removed without large
  retention or product changes. The actual bank graph is the next boundary.
  [Authenticated metadata](../.local/mcc-loading-production-20261018/frontend-event-metadata-20261019-v1/validation.json).
- [x] Authenticate and parse the actual Windows Main_Bank version132 graph.
  All286 HIRC record identities/extents and301 relevant numeric fields are
  independently checked against raw offsets. The directed path is
  `Play_Music`→Play action→music switch→playlist→segment→track→media236657329,
  conditional on Switch group2949940656/key1792793154. This establishes an
  authored conditional path; actual frontend switch publication, audible
  behavior and OG/Vista identity remain open. Three heuristic display-union
  meanings are excluded from qualified facts. Bank/source-body scratch is
  removed with no WEM reads, large retention or product changes.
  [Actual authored graph](../.local/mcc-loading-production-20261018/frontend-bank-graph-20261019-v1/validation.json).
- [x] Authenticate the unique packed Windows media`236657329.wem` by exact
  entry/header and payload SHA1. Its3,356,709bytes contain Wwise custom Vorbis,
  stereo44,100Hz, with2,744,320 declared samples (62.22948seconds). The header
  does not uniquely establish the external codebook variant; installed ffprobe
  cannot decode it. No playback, PCM or audible equivalence is claimed. Media
  and downloaded source bodies are removed with scratch; no other media payload
  or product data is read/changed. A qualified decoder is the next boundary.
  [Exact packed media/header](../.local/mcc-loading-production-20261018/loading-wem-20261019-v1/validation.json).
- [x] Decode the exact loading WEM using pinned, unpatched upstream vgmstream
  and existing system Vorbis/Ogg libraries. Its normal initializer admits
  WWVORBIS_V62/aoTuV603 without a forced variant. Float output has exactly
  2,744,320 stereo frames at44,100Hz, all5,488,640 samples finite and nonzero
  output; standalone WEM loop metadata is disabled. Source/decoder/build/input
  and21.95MB float output are removed with all five scratch paths. No playback,
  importer/runtime integration, live switch or original-audio identity is
  claimed. Retained evidence is small provenance, hashes and diagnostics only.
  [Actual admitted decode](../.local/mcc-loading-production-20261018/loading-wem-decode-20261019-v1/validation.json).
- [x] Qualify the actual frontend's music name-hash and conditional request
  publication path. Genuine hash instructions produce the authored group/key
  and Play_Music/Menu_Music IDs; the numeric receiver publishes command8's
  group/key and64-bit object ID. Explicit conversion/clock/allocator fixture
  boundaries are recorded. The producer of selector5, real queue consumption
  and live service selection remain unproved; seven nearby leaf candidates
  are excluded rather than called loading callbacks. All14 scratch paths are
  removed, with only small source/proof diagnostics retained and no playback
  or product change.
  [Frontend conditional request owner](../.local/mcc-loading-production-20261018/frontend-audio-owner-20261019-v1/validation.json).
- [x] Inventory the existing launcher/cache/asset handoffs and their concrete
  remaining hooks. This read-only source/manifest audit does not verify a fresh
  install, decoded UI delivery or cache-only gameplay. The next work is honest
  native-test routing and MCC-owned loading/menu inputs.
  [Onboarding inventory](../.local/mcc-onboarding-delivery-audit-20261018/read-only-v1/report.json).
- [x] Detect/import MCC content into the port-owned cache in the existing test setup.
- [x] Assemble the copied MCC menu image/font resources transactionally without
  changing their source cache. The actual-cache scratch check covered 648 image
  owners, 561 unique DDS pages and 40 font files; all scratch output was removed.
  This does not admit menu widgets, localization or the complete loading presentation.
  [Actual asset assembly](../.local/mcc-loading-production-20261018/v1/actual-assets.json).
- [x] Replace the production Main loading hooks with the owned five-input MCC
  factory and remove automatic Vista loading/settings discovery for MCC.
  Two current-header syntax checks, 22 preparation controls and the actual MCC
  factory check pass. The modified Main/frontend are now linked in V10; its
  multiplayer capture does not execute the menu/loading presentation. The
  missing mask, ambience and menu metadata remain open.
  [Production source handoff](../.local/mcc-loading-production-20261018/v1/validation.json).
- [x] Compile the three necessary MCC loading consumers against the coherent
  client headers: Main, frontend and the newly referenced owned loading factory.
  Their source/header identities and retained object sizes are recorded; all
  compiler/test scratch is removed. Actual menu/loading rendering remains open.
  [Loading consumer build](../.local/mcc-native-client-20261018/mcc-loading-consumers-v1/build.json).
- [x] Reproduce the remaining loading-presentation failure with the actual
  five owned MCC inputs and unchanged presentation constructor. Input preparation
  succeeds; construction rejects the unresolved required slot11 mask. All 314
  cohort inputs/dependencies and source maps remain unchanged, with no reference
  opens or retained test binaries. This CPU-only expected failure does not
  complete a loading frame or establish an ambience producer.
  [Actual missing-role reproduction](../.local/mcc-loading-production-20261018/investigation-v2/repro/actual-v1/validation.json).
- [x] Trace the narrow MCC loading-asset candidates and menu-music ownership.
  All 1,557 mainmenu owners, 16 candidate bitmap frames and 577 installed
  prebuild/preload filenames are accounted for. The verified `wigl`/`lsnd`
  references identify menu music; they do not establish loading ambience.
  None of the tested images authenticates slot11. All three scratch trees are
  removed. The actual loading producer/role mapping remains the next dependency.
  [Asset-role investigation](../.local/mcc-loading-production-20261018/investigation-v2/assets/status.json).
- [x] Reauthenticate the installed MCC DLL's complete legacy loading-bank
  consumer: all four chained-unwind fragments cover 1,045 bytes and 224
  instructions, with a real six-offset image loop for `loading_dx11.bin`.
  Its loading-audio call targets a literal `RET 0` leaf. The named bank is
  absent from the bounded 577-file inventory. These static facts do not prove
  the active outer frontend path, image roles, or a replacement audio policy;
  those bindings remain open. No product files or shaders change, and scratch
  is removed.
  [Current MCC loader authority](../.local/mcc-loading-production-20261018/investigation-v2/source/mcc-legacy-loader-primary.json).
- [x] Inspect the actual outer loading GFX and installed pack indexes without
  importing replacement content. The 6,352-byte GFX has 302 framed nested
  records and two external image names; all 17 pack indexes identify the
  small loading UIF/WLIB/SSL entries. These metadata references guide the
  next producer investigation; they do not yet supply the mask or ambience.
  [Outer loading metadata](../.local/mcc-loading-production-20261018/investigation-v2/source/adjacent-metadata.json).
- [x] Identify the sixth legacy MCC image's actual masking role. The complete
  loading thread/renderer and four child owners bind image5 to the destination
  color/zero blend draw. Its genuine source is the sixth DDS extent in
  `loading_dx11.bin`; no equivalent tag or available payload is established.
  This identifies the required role without admitting a substitute. The mask,
  independent ambience and production draw binding remain open.
  [Producer and role seal](../.local/mcc-loading-production-20261018/investigation-v2/source/producer-role-seal.json).
- [x] Qualify the installed MCC PCK block reader from its actual DLL consumers.
  Its header, 32 KiB logical blocks, physical offsets and mode0 raw-DEFLATE
  framing decode all 929 `cache.pck` blocks with matching checksums. The
  30,439,696-byte logical stream contains the exact loading UIF/WLIB/SSL
  resource names; no decoded stream is retained. Named resource extents and
  roles still need their source-derived lookup schema. A real
  `loading_screen_music` request is identified separately from the retired
  legacy audio hook; its sound/event binding is not yet admitted.
  [PCK reader authority](../.local/mcc-loading-production-20261018/pck-lookup-v1/first-block.json),
  [Actual logical resource metadata](../.local/mcc-loading-production-20261018/pck-lookup-v1/cache-logical-metadata.json).
- [x] Qualify the outer PCK resource header and its two actual resource extents
  from the installed reader and registered classes. The header ends at logical
  byte217; its cache block and SSL block extents are exact. Later loading-name
  metadata lies outside those rows, so its neighboring integers are not
  admitted as payload offsets. The inner-cache lookup remains the next step.
  [Source-derived PCK header lookup](../.local/mcc-loading-production-20261018/pck-lookup-v1/pak-header-lookup.json).
- [x] Extract the three named loading resources from the authenticated nested
  cache lookup. All 13,661 entries resolve; the exact CSV lengths and hashes
  identify 163 bytes across UIF/WLIB/SSL, using 27 physical blocks. Their
  contents bind `hcex_loading_screen` and native `UiLoadingScreen`; they do
  not contain a mask or music payload. The native implementation is the next
  owner to trace. Scratch is removed and no decoded pack stream is retained.
  [Actual named loading resources](../.local/mcc-loading-production-20261018/pck-lookup-v1/named-loading-resources.json).
- [x] Qualify the actual shared-pack codec and metadata lookup. The installed
  DLL selects a complete copy decoder for codec4; the source-derived lookup
  resolves all 7,247 shared-pack rows without a loading-name or nested cache
  match. Only metadata is read; no loading image or audio role is inferred.
  [Codec4 authority](../.local/mcc-loading-production-20261018/pck-lookup-v1/codec4-qualified.json),
  [Actual shared-pack lookup](../.local/mcc-loading-production-20261018/pck-lookup-v1/shared-loading-lookup-v2.json).
- [x] Inventory the four specifically named outer UI texture packs by their
  actual chunk metadata. The loading pack has 268 definitions; the other
  packs have 306, 758 and 997. Neither external GFX image name is found.
  Map pictures and named UI textures remain candidates until their real
  rendering consumers establish a role; no pixel payload is copied.
  [Outer UI texture metadata](../.local/mcc-loading-production-20261018/pck-lookup-v1/loading-texture-metadata.json).
- [x] Qualify the actual outer MCC image-name lookup kernel and its source
  registration. The complete conditional callback and name-hash consumer pass
  101 genuine x64 CPU controls against all 256 CRC words. The two exact
  external GFX name keys have no match in the four inventoried packs. The
  specific tag-to-descriptor ingress, active GFx binding, mask and independent
  ambience remain unproved; no image or audio substitute is admitted. Source,
  metadata and CPU receipts are sealed with scratch removed and no retained
  binaries, pixel banks or captures.
  [Loading lookup source and CPU handoff](../.local/mcc-loading-production-20261018/pck-lookup-v1/handoff.json).
- [ ] Verify fresh-machine detection and onboarding with only an installed MCC copy.
- [ ] Verify complete required assets: textures, bump maps, models, audio,
  gameplay definitions, UI and loading presentation.
- [ ] Verify MCC internal paths and mod-folder tags/data paths are preserved.
- [x] Reconstruct the requested loading presentation from MCC and retained
  custom assets; remove dependence on the Vista loading bank. Reuse the existing
  animation and renderer handoff, MCC artwork reconstruction, and the explicitly
  approved18,396-byte bundled original tone/four-voice playback. Both installed
  Play routes pass; no original/Vista installation is needed at runtime.
  [Loading delivery](../.local/mcc-loading-original-tone-20261020/closure.json).
- [ ] Verify cache-only startup after import without access to the MCC source files.
- [x] Establish a process-local read/execute confinement fixture for the cache
  test. Six actual MCC/Original/XBE files open before restriction and return
  `EACCES` after exec; an owned cache map and shader stay readable. Landlock
  ABI10, no-new-privileges and closed inherited descriptors are verified;
  allowlisted roots include only system paths and owned scratch. No global
  permissions or directories change. This is the confinement primitive only,
  not a game run or a filesystem-write/IPC security claim.
  [Confinement controls](../.local/mcc-cache-only-containment-20261018/v1/primitive-validation.json).
- [x] Run the actual portable version8 native Lockout test with original MCC
  and Xbox inputs unreadable. It exits0 at 17 frames/30 neutral ticks after
  133.73 seconds; the real frame is inspected. Six negative opens return
  `EACCES` after inherited exec. All 107,868 runtime open attempts are audited:
  no successful reads outside allowed roots and no original-source matches.
  All map/resource containers come from the owned cache. Package checksums,
  97 shaders, 25,004 cache source identities and original identities stay
  unchanged. All processes and three scratch roots are gone; temporary cache
  links, saves, binaries, trace and frame are removed. This proves only the
  neutral test route; the full cache-only/campaign/UI/onboarding gate stays open.
  [Contained actual gameplay](../.local/mcc-cache-only-containment-20261018/v1/validation.json),
  [Cleanup and terminal process audit](../.local/mcc-cache-only-containment-20261018/v1/cleanup.json).
- [x] Verify the complete accepted launcher-to-native-Lockout path opens no
  Xbox, Vista or MCPX references. Both actual extracted v20 Tk Play routes audit
  clean through menus/countdown/loading/input/audio/clean exit. Other campaign/
  mode paths retain their own open acceptance gates.
  [Full accepted route audit](../.local/mcc-gameplay-continuation-20261020/portable-v20-validation.json).

## Converted vanilla gameplay and 64-bit integration

- [ ] Finish the gameplay-tag conversion audit; convert every actual TU1.5 difference.
- [x] Authenticate the exact TU1.5 damage-table consumer before interpreting
  row-order differences. Complete`D8110` consumer,`1408B0` binary search and
  `D8100` comparator total112 instructions per version, matched to retail
  through explicit call/callback/global/literal operands rather than a uniform
  address delta. Actual calls/callback pushes, identity float`3F800000` and
  genuine16-instruction publication of matg global`4ECC1C` are pinned. Scratch
  is removed; only small instruction/proof reports remain. This is source
  authority, not yet actual MCC-table execution or material-index equivalence.
  [TU1.5 damage lookup authority](../.local/mcc-damage-table-consumer-20261019/authority-v1/validation.json).
- [x] Execute the authentic TU1.5 damage-table consumer across all38 paired
  original/MCC maps. The1,299,092 deduplicated raw-key CPU cases match the exact
  selected native kernel bit-for-bit;618,010 common name-qualified cases agree
  across platforms. Authored row order, StringIDs and scalars stay unchanged,
  with existing Cache owner-pointer rebasing explicitly checked. The all/emp
  and Mombasa cutting/burning reorders require no mutation for this consumer.
  Platform-only rows/materials and numeric material-index consumers remain
  separate open boundaries. The temporary104MB probe, banks and maps are
  removed; no product/shader/runtime change was needed.
  [Actual38-map consumer matrix](../.local/mcc-damage-table-consumer-20261019/matrix-v1/validation.json).
- [x] Re-read and authenticate all38 paired numeric material tables. Every
  shared material keeps its index/order, parent identity/index and armor name.
  Twenty-six MCC contexts append only `tough_inorganic_plastic_container` at
  index182, parent index39; twelve contexts have identical ordered identities.
  This rules out an insertion/reorder fix for these tables. Reachability of
  the appended row and actual numeric-index consumers remain open. Both
  private environments are removed, with no raw banks or product edits.
  [Paired material inventory](../.local/mcc-material-index-audit-20261019/inventory-v1/validation.json).
- [x] Demonstrate one actual collision-content mismatch with the selected V16
  native provider. Floodzone's `door_key_tunnel_rusted` retains identical local
  material names/order and surface mappings, but two rows bind MCC global
  material0/default instead of original69/`hard_metal_thick`. Native
  `ModelCollision` output preserves that difference;592 and360 surfaces select
  the affected rows. This establishes a required authored mapping correction,
  separate from the harmless appended global material. Active scene contacts
  and an original TU1.5 machine-consumer comparison remain open. Probe/link/map
  scratch is removed and no product patch is applied at this gate.
  [Actual collision provider comparison](../.local/mcc-material-index-audit-20261019/consumer-v1/door-validation.json).
- [x] Qualify the minimal collision-material correction and its insertion seam.
  Fresh43-MCC/38-original inventory finds the owner only in Floodzone. Full
  scenario/globals/model fingerprints and material69 identity guard exactly
  two signed16 stores,0→69, at owner offsets`120`/`134`. All other owner bytes
  stay exact. Floodzone's existing Gauss correction must remain independently
  composable: preflight both components before publishing either. Unknown or
  modded component tuples stay unchanged. All seven probe scratch paths are
  removed; implementation/native post-conversion acceptance remain next.
  [Authenticated guards](../.local/mcc-material-index-audit-20261019/correction-guards-v1/source-guards.json)
  · [Insertion seam](../.local/mcc-material-index-audit-20261019/correction-guards-v1/seam-proposal.json).
- [x] Implement the independent guarded Floodzone material planner and compose
  it with Gauss before any ordinary internal-data publication. Exact two16-bit
  stores, full owner/material guards, all-component preflight and overlap checks
  pass actual-bank fixtures; altered material owners preserve Gauss independently.
  Strict focused CMake, explicitly instrumented ASan/UBSan and current-header
  Cache compilation pass. The option-only sanitizer run is excluded from
  sanitizer evidence. Scratch/build trees are removed; V16 frozen providers,
  shaders, runtimes and ZIPs stay unchanged. A compatible successor provider
  pair and actual native post-conversion acceptance remain next.
  [Implemented/tested source handoff](../.local/mcc-material-index-audit-20261019/implementation-v1/handoff.json).
- [x] Independently review the Floodzone planner and reopen both authenticated
  source maps. Original material indices99,69,69 versus MCC99,0,0 require only
  the two signed16 stores; they reproduce the exact target model-owner hash
  while preserving every other byte. Full salted owner binding, unknown/modded
  tuples, idempotence, Gauss independence and atomic composition have no
  concrete review findings. Both review scratch paths are removed. This adds
  no native ABI/gameplay acceptance claim; the frozen-provider probe is next.
  [Independent source/data review](../.local/mcc-material-index-audit-20261019/independent-review-v1/validation.json).
- [x] Validate the correction through normal Cache and the exact selected
  collision provider across all43 V16 map inputs. The other42 complete backing
  stores/directories are identical; Floodzone changes only the two intended
  halfwords and yields material indices99,69,69 with unchanged592/360 affected
  surface counts. Its Gauss result stays exact. Three actual modified-data
  controls preserve their backing and independent Gauss behavior. The two new
  components and probe reuse106 exact selected header dependencies; all313
  other linked inputs and original316 files, runtime/maps/97 shaders remain
  exact. All objects/probe ELFs/maps/banks/snapshots are removed. This closes
  native content acceptance; a retained successor client and live regression
  remain next, with V17 still using the unchanged V16 runtime.
  [Normal native Cache acceptance](../.local/mcc-material-index-audit-20261019/postconversion-v1/handoff.json).
- [x] Prepare the compatible two-object full-client successor after the 43-map
  content proof. The exact reviewed V18 plan preserves 314 V16 providers,
  including Main, and 106 selected dependencies while replacing only Cache and
  the map planner. Paired tick60 image/save/shot-state and finite/nonzero audio
  acceptance must precede retention. The planned owned candidate is about 125 MiB
  of necessary build inputs/runtime/link ownership and source snapshots; no
  candidate or compiled bytes exist at preparation. Root checkpoints this
  plan for executor preparation; inspect the executor before running it. The
  separate nine-phase input test, installation and ZIP remain later gates.
  [Prepared V18 successor](../.local/mcc-material-index-audit-20261019/successor-v18-preparation-v1/handoff.json).
- [x] Review the concrete V18 executor and its failure/cancellation ownership.
  It uses only the two prepared compiler commands and unchanged full-client
  inputs; all project dependencies must match the selected projection. Exact
  paired acceptance precedes atomic publication of a complete owned candidate.
  Mesa/NVIDIA/application caches are private, loader/cache-bypass overrides are
  cleared, and the wrapper/finish record actual exits and verify scratch
  removal after either result. Root checkpoints the sealed executor; actual
  compilation, native acceptance and retention remain the next gate.
  [Reviewed executor plan](../.local/mcc-material-index-audit-20261019/successor-v18-executor-v1/executor-plan.json).
- [x] Build and retain the compatible V18 full client after exact paired
  acceptance. Both clients complete tick 60 with identical decoded frame,
  complete save hashes and shot/capture state; each emits finite, nonzero
  stereo callback data. Only Cache/planner change among 316 inputs. The accepted
  ELF is `09426c7b…7553f`; all original providers, 43 maps, 97 shaders, selected
  runtimes, saves and warm cache seed remain exact. Required coherent candidate
  inputs/runtime/link ownership/frozen sources and two small reference PNGs
  total 132,437,304 bytes. Private objects, raw frames/audio, GPU caches and both
  runner scratch levels are removed; finish independently verifies the actual
  exit and candidate inventory. A pre-main Python helper-name collision is
  preserved and resolved with qualified inherited safe-path configuration;
  sealed helper bytes stay unchanged. Nine-phase real input and delivery remain
  separate gates; V17 still ships the accepted V16 runtime.
  [Accepted retained V18](../.local/mcc-material-index-audit-20261019/successor-v18-executor-v1/finish.json).
- [x] Prepare and independently review the exact nine-stage V18 input check.
  Its gate binds the accepted cohort/runtime and fixedtick receipt before any
  display or subprocess. Pointer/focus/click/held fire/look/movement events
  keep the existing recipe; held input releases and child cleanup cover failure.
  Two stale ancestor assumptions are corrected: P uses its actual V16 payload,
  and maps use the exact selected cohort while P's selector is preserved
  separately. Private saves/GPU/application caches and all original inputs are
  checked. Preparation scratch is removed. Root checkpoints actual accepted
  pins for execution; live survival remains unclaimed until it passes.
  [Prepared input helper](../.local/mcc-material-index-audit-20261019/successor-v18-interactive-preparation-v1/handoff.json).
- [x] Execute all nine actual V18 input stages and verify live cleanup. The
  process survives pointer entry, focus, click/release, held fire/release, look
  and forward movement/release, with four first-person particle draws after
  actual fire events and no native error/unmapped/crash logs. Rendered readiness
  is measured at 28.0 seconds in this run; no speed improvement is inferred.
  The client exits cleanly after intentional termination. Original/candidate
  inputs, maps, shaders, saves, selections and warm seed stay exact; children
  stop and live/closure scratch is removed. No new large artifacts are retained.
  A mistakenly invoked preparation finalizer is recorded and its original seal
  restored exactly; the separate live receipt provides this acceptance. This
  closes bounded Lockout input survival, not full scene/gameplay/audio parity.
  [Actual input and cleanup](../.local/mcc-material-index-audit-20261019/successor-v18-interactive-closure-v1/validation.json).
- [x] Prepare and review compiled V18 delivery over the immutable V17 archive.
  Only six existing paths change: payload, sidecar, validation, manifest,
  README and checksums. The other 4,246 entries, including all importer/Python
  tools, 190 libraries, wrapper and 97 shaders, must match V17. Its 46 importer
  controls are explicitly inherited; new payload acceptance requires fresh
  relocated launcher-info/eight-frame gameplay and reference/development/cache
  write audit. Root reviews source/runtime/save preservation and the qualified
  external-cache inode limit, then checkpoints ZIP-only execution. Preparation
  scratch is removed; actual publication remains next. P installation is outside
  this action, and full gameplay-ready stays false.
  [Sealed V18 package preparation](../.local/mcc-material-index-audit-20261019/delivery-v18-preparation-v1/handoff.json).
- [x] Preserve the first V18 packaging failure before any ZIP publication.
  The safe-path setting used by the test runner propagated into the app's
  Python and prevented its normal sibling import. The relocated capability
  check rejects this; no eight-frame run or archive follows. Separate review
  also finds an inherited Tk demo `.ppm` that a blanket capture ban would reject.
  The minimal retry restores the app's normal child environment and applies the
  new-capture ban only to changed/new entries while preserving all inherited
  bytes. The failed receipt and original sealed helper stay intact; scratch is
  removed. Neither issue changes the accepted native build or its live tests.
  [Actual failed package attempt](../.local/mcc-material-index-audit-20261019/delivery-v18-preparation-v1/package-validation.json).
- [x] Qualify the minimal V18 packaging retry. An isolated red/green control
  reproduces the sibling-import failure and confirms that removing only the
  child environment flag preserves the packager's safe import mode. The actual
  inherited Tk image is accepted unchanged while a newly added raw capture is
  rejected. Original V1 code/metadata/failure remain exact; all preparation and
  control scratch is removed. Root reviews the two-line policy changes and
  checkpoints the separately sealed V2 ZIP-only workflow. Actual publication
  remains next; all runtime/save/source and entry preservation checks stay.
  [Qualified minimal retry](../.local/mcc-material-index-audit-20261019/delivery-v18-preparation-v2/handoff.json).
- [x] Publish the compiled V18 ZIP after the minimal retry passes. Both relocated
  launchers offer the native test with full readiness false; actual eight-frame
  Lockout gameplay succeeds through the bundled loader. The file audit sees no
  reference/development opens or private hardlink-cache writes. All 4,251 archive
  checksums pass; only the six reviewed paths change, preserving 4,246 V17 entries,
  190 libraries and 97 shaders. Runtime/save/source and qualified cache guards
  pass. Test scratch is removed; the requested ZIP/checksum are retained.
  This publishes the accepted V18 runtime, leaving the installed P V17 folder
  and caches intact. Independent archive review follows.
  [Actual compiled V18](../.local/mcc-material-index-audit-20261019/delivery-v18-preparation-v2/package-validation.json).
- [x] Independently stream-verify the published V18 archive without extraction
  or application execution. All 4,252 members/4,251 checksum rows pass; exactly
  six existing entries change and the other 4,246 retain their bytes and modes.
  All 97 shaders, 190 libraries, wrapper, Python and importer remain exact.
  Runtime authority identifies V18 while the 46 importer controls are explicitly
  inherited from V17; full readiness remains false. Review scratch is removed
  and no large review artifacts are retained. P remains installed at V17.
  [Independent V18 archive review](../.local/mcc-material-index-audit-20261019/delivery-v18-archive-review-v1/validation.json).
- [x] Finish V18's scoped retention/scratch audit. All 26 recorded completed
  V18 scratch owners and the audit caller's own scratch are absent. The required
  coherent 119-file client/input/reference tree retains 132,437,304 bytes;
  requested ZIP/checksum retain 183,623,403 bytes, totaling 316,060,707 bytes.
  V17 delivery, P rollback preimages, frozen providers, runtime selections and
  saves remain exact. Active foot work is outside this audit. No historical
  deletion or full-cache rehash is performed; prior cache guard qualifications
  remain explicit. The initial alias-check diagnostic is preserved separately.
  [V18 retained paths and closure](../.local/mcc-material-index-audit-20261019/closure-v1/validation.json).
- [x] Authenticate the original material-effect foot lookup before treating
  ordinal row differences as defects. Five complete retail/TU1.1/TU1.5 owners,
  359 instructions per version and explicit cross-version operands establish signed 16-bit
  keys at row offset `0x14`, forward first-match lookup and globals parent traversal.
  This is source authority only; keyed paired-map execution is next. Probe
  scratch is removed and no product changes are made.
  [Foot lookup authority](../.local/mcc-foot-table-key-audit-20261019/authority-v1/validation.json).
- [x] Complete the bounded keyed foot-table inventory across 38 paired maps.
  All 76 native Cache views are complete and bind the same primary/first globals
  owner. Of 2,309 earlier ordinal candidates, 1,162 preserve the original first
  effective keyed row at a different ordinal, 1,070 need keyed assessment and
  77 contain an invalid key at one ordinal. A resolved Chief walk case keeps
  the classic primary sound while MCC adds a remastered secondary `snd!`
  reference where Xbox has null in 27 maps. That is a data difference; original
  CPU output and actual native caller group admission must precede any audible
  defect claim or conversion decision. The modeled variant counters include
  explicit unresolved/cyclic boundaries and are not CPU/live parity evidence.
  All recorded scratch is removed, protected inputs stay exact and no product
  changes or large retained artifacts result.
  [Keyed inventory and boundaries](../.local/mcc-foot-table-key-audit-20261019/keyed-inventory-v1/summary.json)
  · [Cleanup](../.local/mcc-foot-table-key-audit-20261019/keyed-inventory-v1/cleanup.json).
- [x] Confirm the resolved Chief walk case with unchanged original TU1.5 CPU
  code and exact selected native providers. Both platforms execute 218 original
  instruction addresses and match all six native selection outputs. Xbox has
  the eight-byte null reference `FFFFFFFFFFFFFFFF`; MCC's secondary resolves to the remastered `snd!`, with the
  same named classic primary. The selected gait/audio path admits that secondary
  without slot/path suppression; catalog decoding and audibility remain open.
  Backing controls execute code-write rejection before mutation and prove added
  memory regions disjoint from all original image/default/other regions. Only
  test backing admission changes, with original instructions/owners unchanged.
  All recorded scratch is removed; only small proofs/runner inputs remain.
  This is one Lockout case, not a 27-map CPU matrix or an implemented conversion.
  [Original/native confirmation](../.local/mcc-foot-table-key-audit-20261019/cpu-v1/summary.json)
  · [Cleanup](../.local/mcc-foot-table-key-audit-20261019/cpu-v1/cleanup.json).
- [x] Prepare exact pre-conversion guards for the proven Chief secondary sound
  reference in 27 stock contexts. Raw scenario/globals/foot owners, first signed
  material key and full salted support identities qualify a single eight-byte
  null restoration per context. Already-null targets require active primary
  metadata, with no dependency on the now-inactive secondary. The proposed
  independent Cache component shares all-component preflight and leaves Gauss
  and Floodzone qualification independent. All proposed write intervals are
  disjoint. A separate census identifies 2,345 physical map-context fields in
  90 named classes for possible later batching; those fields are not production
  profiles or additional CPU coverage. Product sources remain unchanged and
  all recorded scratch is removed. Independent review and implementation follow.
  [Prepared profiles and seam](../.local/mcc-foot-table-key-audit-20261019/profiles-v2/handoff.json).
- [x] Root review pins the final profile table, conditional identity policy,
  raw 27-context qualification and transaction seam. Authorize the narrow
  one-field-per-context planner and focused validation; broader census fields
  remain review-only. No existing layouts/signatures, shaders, frozen providers
  or delivered runtimes change at this checkpoint.
  [Implementation scope checkpoint](../.local/mcc-foot-table-key-audit-20261019/profiles-v2/root-checkpoint.json).
- [x] Independently cross-check all 27 prepared compact identities against the
  retained raw qualification, reconstruct donor/target row digests and verify
  the serialized eight-byte bounds/full salted references. Source review finds
  no actionable proposal defect in native selection, conditional dependencies
  or the transaction seam. This is a bounded proposal review, without fresh
  raw-map/CPU/runtime execution; implemented behavior still needs validation.
  Review scratch is removed, with no large artifacts retained.
  [Independent profile review](../.local/mcc-foot-table-key-audit-20261019/profiles-v2-independent-review-v1/validation.json).
- [x] Implement and validate the narrow 27-context Chief reference planner and
  wire its normal Cache component. Complete owner deltas, target idempotence,
  custom/dependency preservation and shared transaction controls pass below.
  Actual native Cache readback is deferred to the guarded union continuation,
  which includes the same seed fields; no seed-only Cache acceptance is claimed.
- [x] Implement the narrow Chief planner, separate 27-profile include and exact
  group-first Cache hook without changing existing layouts/signatures or Gauss
  profiles. Focused disposable CMake/CTest passes all 27 owned foot contexts
  plus the existing 28 Gauss/one material fixtures, including owner deltas,
  active/inactive dependencies, custom identities and three-component atomic
  composition. Root source review finds no actionable defect. Sanitizer and
  actual normal-Cache readback remain separate gates; delivered V18 is unchanged.
  [Focused implementation validation](../.local/mcc-foot-table-key-audit-20261019/implementation-v1/validation-regular.json).
- [x] Freeze the narrow seed implementation after actual instrumented
  AddressSanitizer/UndefinedBehaviorSanitizer compilation, linkage and execution
  pass. All 27 foot contexts, existing Gauss/material fixtures and eight
  three-component donor/target combinations pass with zero errors. Existing
  conversion/preflight bodies, Gauss include, public layouts and signatures stay
  exact; the Cache diff adds only group-first selection and its third component.
  Every recorded build/fixture/scratch path is removed. This is pure-planner
  acceptance; normal Cache execution and runtime delivery are not claimed.
  [Frozen source/test handoff](../.local/mcc-foot-table-key-audit-20261019/implementation-v1/handoff.json)
  · [Cleanup](../.local/mcc-foot-table-key-audit-20261019/implementation-v1/cleanup.json).
- [x] Prepare coherent guards for the broader authenticated Chief class before
  expanding conversion scope: full active support metadata, deduplicated owner
  fields, complete union targets and explicit untouched/seed/union transitions.
  Reuse generic selector authority while keeping actual CPU coverage bounded.
  The exact prior census is reconstructed: 27 contexts/2,345 fields, deduplicated
  into 16 owners/1,355 physical fields and 90 named classes. Exactly three full
  owner states qualify each profile. Root independently derives every active
  support set and write-offset set: seed-shared secondary owners remain required
  until their last active candidate is removed. Direct donor-to-union needs
  2,345 context writes; seed-to-union needs 2,318; union is idempotent. Unknown
  partial/custom owners stay unrecognized. Scalar/hash metadata totals 4.22 MB;
  no banks/binaries/media or new CPU/runtime execution are retained. All recorded
  preparation and root review scratch is removed. Implementation remains next.
  [Batch handoff](../.local/mcc-foot-table-key-audit-20261019/batch-profiles-v1/handoff.json)
  · [Root finite-state/dependency review](../.local/mcc-foot-table-key-audit-20261019/batch-profiles-v1/root-review.json).
- [x] After the narrow seed's instrumented acceptance, checkpoint the exact
  finite batch implementation. Replace its private planner/table while keeping
  the public declaration, Cache hook and shared transaction; do not add a fourth
  component. Require all 27 untouched/seed/union transitions and state-aware
  support preservation tests before separately gated normal Cache execution.
  Full-client rebuild and delivery remain deferred until conversion acceptance.
  [Batch implementation scope](../.local/mcc-foot-table-key-audit-20261019/batch-profiles-v1/root-implementation-checkpoint.json).
- [x] Implement and validate the guarded Chief union conversion, then verify
  all selected native Cache and direct selector outputs before runtime delivery.
- [x] Replace the narrow private foot planner/table with the exact guarded
  union conversion. The existing public hook and Cache component remain
  unchanged. Focused CMake/CTest passes all 27 complete donor/seed/union owner
  transitions, exact 2,345/2,318 field sets, all 16 owners' active-support
  mutation/missing/full-salt controls, inactive preservation, custom partials,
  alias/key/bounds controls and 12 three-component transaction combinations.
  The generated 716,757-byte include contains scalar/hash metadata. Actual
  sanitizer execution, independent final review and native Cache acceptance
  remain next; no runtime or delivery artifact has been replaced.
  [Batch focused validation](../.local/mcc-foot-table-key-audit-20261019/implementation-batch-v1/validation-regular.json).
- [x] Freeze the batch after actual instrumented ASan/UBSan compilation,
  linkage and execution pass. All 81 context/state cases pass: donor-to-union
  covers 2,345 fields, seed-to-union 2,318, and union has zero writes. Every
  required support owner/state is exercised for mutation, absence and foreign
  salt; inactive support and custom/partial controls also pass. Existing Cache
  hook, public layouts/signatures, Gauss/material/preflight bodies and Gauss
  include stay exact. All recorded build/fixture/scratch paths are removed;
  six small seed preimages remain required transition references. Native Cache
  execution, independent final review and runtime delivery remain separate.
  [Frozen batch/source-test handoff](../.local/mcc-foot-table-key-audit-20261019/implementation-batch-v1/handoff.json)
  · [Cleanup](../.local/mcc-foot-table-key-audit-20261019/implementation-batch-v1/cleanup.json).
- [x] Independently review the final source and parse every generated C++
  initializer against sealed profiles: 27 contexts, 16 owners, 1,355 physical
  rows/2,345 context fields and 1,030 unique support records match. Final regular,
  sanitizer and handoff pins agree. Finite-state admission, shared seed support
  dependencies, signed-first-key traversal, custom preservation and atomic
  publication have no actionable findings. Cache/public/Gauss/material/preflight
  contracts remain exact. Only a bounded metadata/source review runs; its
  scratch is removed, without raw-map/CPU/native/suite reruns or large outputs.
  [Final independent product review](../.local/mcc-foot-table-key-audit-20261019/final-product-review-v1/validation.json).
- [x] Record the independently caught native-Cache harness control defect
  before execution. Lockout's Chief/support owners live in `shared.map`; the
  initial control recipe incorrectly patched local `lockout.map` before linking
  an unchanged shared bank. Preserve the sealed V1 preparation as unexecuted
  evidence. A minimal V2 must construct private mutation banks by each exact
  authenticated source owner and verify their beforeimages/physical offsets.
  Stock comparison design and product sources are unaffected; native execution
  remains gated until the corrected preparation and final product checks pass.
- [x] Prepare and root-review the corrected V2 native-Cache comparison. The old
  Lockout recipe reproduces an out-of-bounds physical offset; all eight corrected
  recipes authenticate full donor/support owners in their actual selected bank
  and verify physical beforeimages/readback. Six use private `shared.map` and two
  use private Floodzone data, with unchanged companions linked for loading.
  Product regular/ASan/source review and all helper/header/profile pins agree.
  Root checkpoints only the 43-map/2,345-selector tiny-probe execution; no full
  client, installation or packaging is authorized by this gate. V1 stays exact
  and unexecuted; all preparation/checkpoint scratch is removed.
  [Corrected preparation](../.local/mcc-foot-table-key-audit-20261019/native-cache-preparation-v2/handoff.json)
  · [Native execution checkpoint](../.local/mcc-foot-table-key-audit-20261019/native-cache-preparation-v2/root-execution-checkpoint.json).
- [x] Execute the exact selected 43-map normal Cache comparison. All 27
  qualified contexts admit the full union: 2,345 eight-byte fields occupy 18,760
  bytes and change 18,759 literal bytes. Actual selected native MaterialEffects
  reads verify every direct action/key: classic primary identities are retained,
  secondary results become null, and other audio/visual selector outputs stay
  exact. All other backing bytes, complete directories and prior Gauss/Floodzone
  consumers match baseline; the other 16 contexts remain wholly unchanged.
  All eight source-owner-correct modified/seed/union controls pass. All 316
  parent inputs/runtime, 43 selected maps and product pins remain exact. Actual
  exit is zero and its private banks/objects/probes/scratch are removed. This
  verifies vanilla Cache publication/selection, without all-row original CPU,
  sample/audible, full-client or delivery claims.
  [Actual native Cache/selector acceptance](../.local/mcc-foot-table-key-audit-20261019/native-cache-preparation-v2/validation.json)
  · [Root acceptance checkpoint](../.local/mcc-foot-table-key-audit-20261019/native-cache-preparation-v2/root-acceptance-checkpoint.json)
  · [Actual-run scratch closure](../.local/mcc-foot-table-key-audit-20261019/native-cache-closure-v1/validation.json).
- [x] Prepare and root-checkpoint the compatible V19 successor after actual
  Cache acceptance. Retain 314 accepted V18 inputs and original Main, replacing
  only Cache/planner with the tested union. Project the exact 106 parent header
  inputs plus the declaration-only gameplay header/new private include; adopt
  no unrelated current-feature ABI changes. Root binds concrete successful
  43-map/2,345-selector/control receipt fields, source tests and final review
  before build/display creation. Authorize paired fixed-tick acceptance and
  atomic required candidate retention only; input, installation and ZIP remain
  separate gates. Preparation/checkpoint scratch is removed.
  [Successor preparation](../.local/mcc-foot-table-key-audit-20261019/successor-v19-executor-v1/preparation.json)
  · [Build/acceptance checkpoint](../.local/mcc-foot-table-key-audit-20261019/successor-v19-executor-v1/root-execution-checkpoint.json).
- [x] Build and accept the coherent V19 client with exactly the two intended
  provider replacements and 314 unchanged V18 inputs. Actual paired tick60
  Lockout firing runs pass: decoded 640×480 frame, complete save outputs and
  capture state are identical; each stream has finite nonzero stereo callback
  PCM. All 43 maps, 97 shaders, selected headers, previous providers, launcher
  selections and saves remain pinned. No speed or audio-identity claim follows
  from the uncontrolled durations/PCM. The required coherent candidate and two
  small reference PNGs retain 134,141,785 bytes. Actual exit is zero; private
  compile/link/frame/audio/GPU-cache scratch, its owner parent and the receipt
  check's scratch are removed. Real input and portable delivery remain next.
  [Actual V19 build/frame acceptance](../.local/mcc-foot-table-key-audit-20261019/successor-v19-executor-v1/validation.json)
  · [Actual execution/cleanup closure](../.local/mcc-foot-table-key-audit-20261019/successor-v19-executor-v1/finish.json).
- [ ] Verify converted definitions load as vanilla internal data, including
  default campaign and multiplayer paths and mod interoperability.
- [ ] Validate original CTF behavior through its native consumer before calling
  Relic's converted4.0 word a live gameplay fix. The current handwritten match
  constructor admits only source engines2/4 (Slayer/King); Cache and original
  radius-instruction proofs establish data/operand behavior, not a CTF match.
  Preserve the other original game-mode acceptance gates as well.
- [ ] Finish native 64-bit host-pointer/ownership integration across gameplay.
- [x] Identify the next scoped native64 integration witness from actual
  selected C57 evidence. No new host-pointer truncation is demonstrated. The
  accepted initialized-MCC restore and strict charge fixtures are separate;
  the existing live object/player view-continuation test uses an original-input
  startup path. Prepare a genuine ordinary MCC same/fresh-World publication
  witness using existing lifetime/pool binding APIs and real source births,
  preserving all logical widths and current clocks. C57's two removed generated
  objects are explicitly rebuild-required. This is source/evidence analysis,
  not compile, World execution, product change or native64 completion.
  [Live-view integration frontier](../.local/mcc-gameplay-continuation-20261020/native64-c57-live-view-handoff-plan-v1.json).
- [x] Prepare and review that ordinary MCC live-view fixture source. It uses
  real biped/player constructors, existing lifetime/pool checkpoint APIs,
  standard clocks and same/fresh World publication. Five actual host address
  roles must exceed4GiB; complete banks, full logical handles and live view
  bytes must survive. Premature outer-view publication is rejected and expired
  spans are never read. This first witness invokes only read-only restored
  view consumers; later physics/model/gameplay continuation stays unaccepted.
  No compile, source-state override, cache/World/game/replay or product change
  occurs in source preparation. Exact compatible link preparation remains next.
  [Live-view fixture source checkpoint](../.local/mcc-gameplay-continuation-20261020/native64-c57-live-view-handoff-source-checkpoint-v1.json).
- [x] Review its exact selected-C57 coordinator before a single bounded run.
  Reconstruct only the two removed C57 importer objects and replace Main;
  all354 other persistent link inputs stay exact. Importer and World public
  header families have separate dependency guards; their Cache declaration
  difference changes no fields. Link reversal, actual native view API providers,
  complete asset-open classification and all-outcome preservation are checked.
  The actual MCC fixture uses ordinary constructors and current clocks, with
  no source-state seeding or later gameplay ticks. Retain its first actual
  build/constructor/ownership result; full migration remains open.
  [Reviewed live-view recipe](../.local/mcc-gameplay-continuation-20261020/native64-c57-live-view-handoff-recipe-v1.json).
- [x] Preserve the first live-view attempt's actual preflight frontier. Four
  SSD delivery-file identities differ from the prior terminal record; no
  compiler, linker, fixture, World or MCC asset read runs. All1921 recorded
  protected hashes remain exact and owned scratch/cache are removed. Metadata
  alone cannot prove those four payloads unchanged; renew only their content
  proofs before any successor attempt, preserving the original RED.
  [Actual preflight result and exact metadata deltas](../.local/mcc-gameplay-continuation-20261020/native64-c57-live-view-handoff-v1.json).
- [x] Verify the four changed SSD payloads against their exact prior content
  hashes. All5,663,525,512 bytes match, with stable descriptor/path identity;
  five unchanged payload identities are reused and all nine remain stable.
  No duplicate payload or generated binary is retained; scratch is removed.
  A separate root selector publication occurs after the worker's final check,
  so the terminal record preserves that concurrent change and grants no new
  execution authority. Preserve the new independent root selection; separate
  unrelated publication observations from the actual C57/MCC test inputs.
  [Exact payload checks and concurrent publication](../.local/mcc-gameplay-continuation-20261020/f49-v24-delivery-identity-renewal-v1.json).
- [x] Review the native64 V2 coordinator's same input/publication separation.
  Its exact fixture, three compile commands,357-input link projection, frozen
  ABI families, native provider bindings and real MCC guards are unchanged.
  Unrelated selector, ZIP/output and renderer-bank publications are observed
  independently and never adopted or written. Their concurrent changes cannot
  decide the private ownership witness; actual inputs still fail closed.
  Original preflight/renewal RED and immutable evidence remain pinned. Execute
  the bounded ordinary MCC handoff once; no whole-game or delivery grant follows.
  [Reviewed native64 V2 source checkpoint](../.local/mcc-gameplay-continuation-20261020/native64-c57-live-view-handoff-v2-source-checkpoint.json).
- [x] Reach genuine MCC object/player construction with five actual host
  addresses above4GiB. All three compilations,357-input link and selected
  native-provider gates pass. The native process then returns1 from authority
  export with `Arena checkpoint file namespace differs`, before either handoff.
  Separately, the open classifier stops on the selected project's Discord SDK
  library; full open classification is incomplete and no runtime acceptance is
  assigned. All1918 hard pins and522 prehashed MCC inputs remain exact. Owned
  scratch/cache are removed, with no executable or raw bank retained. Preserve
  this first result; diagnose the real arena contract and exact SDK ownership
  separately, without manual activation, clock changes or relaxed acceptance.
  [Actual high-address construction and first frontier](../.local/mcc-gameplay-continuation-20261020/native64-c57-live-view-handoff-v2.json).
- [x] Correct and review the fixture's owned-directory precondition. It
  created the parent but omitted the empty source/fresh save directories that
  the normal production caller creates before World construction. Only those
  two mkdir calls change; arena ownership validation, game state, clocks and
  all selected providers stay exact. Four exact read-only platform paths now
  have finite source/package/kernel proofs, including OpenCL's actual selected
  FFmpeg dependency. Nine known cache paths receive new prehashes; V2's missing
  historical post-open evidence remains explicit. The9,215,060-byte raw report
  is retired only after exact lossless archive verification;86,964 compressed
  bytes remain. Review the V3 diff and execute one corrected bounded handoff.
  [Reviewed V3 checkpoint](../.local/mcc-gameplay-continuation-20261020/native64-c57-live-view-handoff-v3-source-checkpoint.json)
  · [Finite platform ownership](../.local/mcc-gameplay-continuation-20261020/native64-c57-platform-retained-open-qualification-v2.json)
  · [Lossless report retirement](../.local/mcc-gameplay-continuation-20261020/native64-c57-live-view-handoff-v2-raw-retirement-v2.json).
- [x] Preserve V3's actual completed native output before audit recovery.
  Both same-World and fresh-World handoffs report complete source-bank and
  full logical-handle/live-view byte equality; five published native addresses
  exceed4GiB. Premature publication rejects and cancellation preserves the
  existing owner. No gameplay ticks, manual bank writes or custom clock are
  used. This is native output only: actual exit, all-open classification and
  final preservation still need sealing. The classifier is rehashing the same
  926MB audio file for29,712 opens; preserve its exact trace losslessly, stop
  only the completed run's audit worker and prepare an offline continuation
  with all per-row checks and unique content hashes. No native replay is
  authorized by this diagnostic correction.
  [Native result pending audit](../.local/mcc-gameplay-continuation-20261020/native64-c57-live-view-handoff-v3-native-output-pending-audit-v1.json).
- [x] Seal the audit-only interruption separately from native acceptance.
  Native exit0, three compile exits0 and link0 are retained; only the outer
  classifier is interrupted with130 after2003/66473 rows. Both finalizers
  preserve1933 hard input pins and all531 prehashed/observed unique MCC files.
  The old work report's compile-only PASS string is explicitly not runtime
  acceptance. Full stdout and the original22,376,086-byte trace survive
  losslessly in small retained files; the18,533,404-byte report is verified
  against its107,880-byte archive before retiring that raw duplicate. All
  owned scratch/cache trees are removed. Offline completion remains required,
  with no native/compiler replay or engine/provider/state/clock change.
  [Actual interrupted audit and cleanup](../.local/mcc-gameplay-continuation-20261020/native64-c57-live-view-handoff-v3.json).
- [x] Review the offline audit continuation without replaying the native test.
  It parses the exact retained original trace and accounts for every66473
  successful-open row. All691 external paths must match their original
  post-open hashes;1997 previously observed external rows are checked again,
  with consistent duplicate pins and all531 preexecution MCC pins retained.
  Per-row flags/deleted/category checks and stable identity remain mandatory;
  unique content hashes are rechecked finally. Only the two removed owned
  save outputs and exact anonymous snapshots lack a persistent-path hash;
  their limits stay explicit. Execute this finite offline continuation once,
  preserving original native0/outer130 and all earlier RED receipts. No
  native/compiler/Cache/game execution or engine/provider change is authorized.
  [Reviewed offline source checkpoint](../.local/mcc-gameplay-continuation-20261020/native64-c57-live-view-handoff-audit-continuation-source-checkpoint-v2.json).
- [x] Complete the bounded ordinary MCC live-view handoff's input audit.
  The single offline continuation exits0, classifies all66473 original opens
  and matches all691 external unique files to their original post-open hashes.
  All1997 prior external observations and531 MCC preexecution pins match;
  final unique/immutable checks find no changes. The actual native0 result
  preserves2874 source-bank regions, full logical handles and live-view bytes
  across same/fresh World publication, with five actual host roles above4GiB
  and cancellation/premature-publication controls. Original outer130 and all
  prior RED evidence remain immutable. The30,205,226-byte full-row audit is
  retained losslessly in98,244 compressed bytes; owned scratch/cache are gone.
  This proves construction/publication/read-only restored consumers only;
  continued gameplay, all owner families and complete TU1.5 parity stay open.
  [Sealed live-view ownership milestone](../.local/mcc-gameplay-continuation-20261020/native64-c57-live-view-handoff-audit-continuation-v2.json).
- [ ] Verify required serialized widths, arithmetic, precision and timing remain unchanged.
- [x] Correct native shader-owner insertion at the original signed tag-slot
  boundary. The prior importer could append an unreachable low16 index32768
  while null lower slots remained. The shared internal helper preserves append
  order below32768, then reuses only complete null datums in the reachable
  half; unloaded/empty live owners remain occupied and a full directory rejects
  before mutation. Existing identities, paths, D000 salt,32-bit serialized
  fields, Cache layout and shader programs remain unchanged. Asset-free
  boundary/occupancy/whole-directory preservation tests pass, and the changed
  MCC producer object compiles without rebuilding the game/archive. Both owned
  configure/test scratch trees are removed and no large artifacts retained.
  This is logical owner-admission evidence; actual mod/native consumption,
  selected-runtime delivery and complete64-bit gameplay remain open.
  [Native directory capacity correction](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-capacity-v1/validation.json).
- [x] Audit the narrow carry-forward into the selected runtime before rebuilding
  it. A two-hunk copy reverses to the exact retained shader-loader source and
  all11 static public includes/Tag ABI agree. Its old private include bytes
  lack original build pins, and the selected lookup-provider source body has
  not been authenticated. Preserve those concrete boundaries; current-source
  component tests do not qualify substitution into the older working client.
  No compile, runtime adoption, cache change or delivery is performed here.
  [Selected loader source/ABI readiness](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-selected-source-v1/readiness.json).
- [x] Qualify the working C57/F49 client successor with the compiled and
  Cache-qualified selected-compatible slot fix. The unchanged-source comparison
  below supplies effective private-input evidence without historical source-byte
  claims. Staged, installed and extracted normal/title routes pass; latest
  binaries and the exact qualified v24 ZIP/checksums are delivered to both
  directories. F48/v23 and the independently owned development selector remain
  preserved. Full migration, map/mode acceptance and TU1.5 parity stay open.
  [Native route](../.local/mcc-gameplay-continuation-20261020/f49-native-proof-v1.json)
  · [Both-directory delivery](../.local/mcc-gameplay-continuation-20261020/v24-ssd-delivery-terminal-v1.json).
- [x] Authenticate the selected ordinary lookup/validation contract directly
  from its exact retained archive member. The complete named method branches
  reject signed-low16 slots at8000 and above; directory validation rejects
  live ordinals above7FFF while permitting complete null padding rows. This
  independently confirms the selected-client capacity seam without assuming
  current source equals the older provider. Small named-method disassembly and
  hash reports are retained; the extracted object and owned scratch are removed.
  No original Xbox executable, game/World execution or replacement compilation
  is involved. The separate private-input authority gate remains open.
  [Selected lookup/validation authority](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-selected-methods-v1/validation.json).
- [x] Finish the original component's retained metadata audit. Its DWARF5
  file table names all three omitted private includes but records no content
  checksums; the inspected compile receipts retain no dependency hashes or
  dependency file. Preserve that absence instead of assigning current bytes
  historical authority. The original compiler producer and exact flags are
  recorded for a separate unchanged-source emitted-code comparison. No
  component compilation or runtime mutation occurred, and all four owned
  inspection scratch paths are removed. Private source bytes remain unpinned.
  [Original debug metadata audit](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-original-debug-metadata-v1.json).
- [x] Prepare a separate unchanged-source compiler comparison using the old
  frozen public headers and three explicitly labeled private candidates. The
  installed GCC producer matches the original recorded version/date. Compare
  all allocated bytes, symbolic relocations, linkage symbols, COMDAT groups,
  unwind data and linker controls; any difference or unsupported metadata
  prevents acceptance. Root reviewed this scope before component compilation.
  A pass can establish emitted behavior for this unchanged translation unit,
  not recover historical private-file hashes or accept a modified client.
  The source-only plan's inspection scratch is removed.
  [Baseline comparison recipe](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-baseline-equivalence-plan-v1.json).
- [x] Compile and run the exact standalone slot helper and existing complete
  boundary suite against the selected shader loader's original frozen public
  headers. All262,157 logical capacity/occupancy/directory-preservation checks
  pass with original optimization, floating-point and warning flags. Actual
  dependencies pin11 project inputs and292 system headers, with no current
  Cache-header fallback. Serialized widths remain exact. This is a tag/vector
  component test, not Cache consumption or above4GiB gameplay acceptance; the
  modified importer/client remains uncompiled. Source copies, objects, test
  ELF, dependency files and owned scratch/cache are removed.
  [Selected-header helper qualification](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-selected-helper-v1.json).
- [x] Complete the unchanged-old-source baseline comparison once. Compilation
  exits0 without diagnostics; all243 allocated sections/198,099 bytes,
  5,202 relocations,845 linkage symbols,133 groups and unwind/exception data
  match the selected retained object's emitted contract exactly. Identical
  input passes and real code-byte/addend changes reject; independent review
  passes. Raw ELF debug/container bytes differ, and historical private-source
  hashes remain unrecovered. The412 actual dependencies are recorded;397
  system-header hashes were first collected after compilation and rechecked
  afterward. The review closes the completed invocation's proof-pin guard gap;
  strengthen that guard before any future helper revision. All compiler,
  object, negative-control and metadata scratch is removed. This qualifies
  effective private inputs only for the unchanged translation unit. The
  two-hunk slot patch adds no new private-definition use, but changed-provider
  compilation, real imported-directory consumption and runtime delivery remain
  open. The validated binaries/ZIPs in both directories stay unchanged.
  [Baseline emitted equivalence](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-baseline-equivalence-v1.json)
  · [Independent comparator review](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-baseline-equivalence-independent-review-v1.json).
- [x] Prepare and review the concrete changed-importer cache-only experiment.
  Its public inspector checks real full-handle lookup, preserved-native flags,
  reference/StringID fixups and current/retained allocation windows. The owned
  sparse-map generator preserves all original live rows, names and payloads
  while adding complete null rows to32768 using documented v13 fields. Exact
  old-header compiler commands and selected C56 cache-provider inputs are
  pinned. Root and independent source review found no concrete inspector
  blocker. The first ordinary link must expose any genuine missing dependency;
  link-map checks exclude World/Simulation/client entry providers before any
  asset inspection. No execution has occurred at this source checkpoint.
  [Component and asset recipe](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-asset-consumption-v1/component-asset-recipe-v1.json)
  · [Owned sparse-map source](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-asset-consumption-v1/sparse-fixture-source-checkpoint-v1.json).
- [x] Preserve the first component attempt's real preflight failure. SSD
  inode identifiers changed, so it stopped before any compiler, linker or
  Cache execution. Fresh full hashes of the three affected SSD files confirm
  both native binaries and the v23 ZIP still match their qualified bytes;
  current identities remain stable across that check. Development identities,
  selectors, capabilities, manifests and sidecars remain exact. This renews
  preservation authority without assuming inode drift proves unchanged data.
  Hash-check scratch is removed. A separately versioned coordinator also
  tightens complete successful-open classification before any asset inspection;
  the original failed helper/diagnostic remain unchanged.
  [First preflight failure](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-asset-consumption-v1/preflight-failure-v1.json)
  · [Fresh SSD byte confirmation](../.local/mcc-gameplay-continuation-20261020/native-build-ssd-identity-reconfirmation-v1.json).
- [x] Compile the narrow changed importer against the original selected public
  headers. Its actual compiler exit is0 with no diagnostics and the same three
  strong public APIs as the retained importer; the new helper reference is
  expected. The coordinator then misclassified its own intended staged private
  header because scratch is nested inside the repository. Preserve that real
  validation failure: helper/inspector compilation, linking, Cache consumption
  and client adoption have not passed. The next coordinator may classify only
  the exactly pinned staged inputs before rejecting current project headers.
  Source/dependencies and both deliveries remain preserved; the observed
  5,146,040-byte object and all owned scratch/cache are removed.
  [Changed compile and coordinator failure](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-asset-consumption-v1/component-asset-experiment-v2-preparation-red.json).
- [x] Correct the owned-header dependency classification and compile both
  changed importer and slot helper successfully against the selected headers.
  The actual inspector compile then rejects a comparison between a returned
  string and a JSON value. This is a test-source type error; no link or asset
  execution occurred. Preserve that diagnostic and use an explicit JSON string
  extraction in the next separately versioned inspector. All recorded inputs,
  delivery identities and dependencies remain exact, and owned scratch/cache
  are removed. Actual Cache consumption remains unchecked.
  [Selected component compiler frontier](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-asset-consumption-v1/component-asset-experiment-v3.work.json).
- [x] Compile the corrected inspector, changed importer and helper, and attempt
  both actual thin links. All three compiler exits are0; exact importer API
  coupling and454 actual dependency paths are recorded. Both links fail on the
  same seven existing cache APIs: logical bitmap extent, map/material/foot
  conversion planning, map preflight, script builtin signatures and script
  layout projection. Preserve these genuine missing dependencies before
  selecting their exact working-build providers. No executable or asset check
  passed and no runtime was adopted. Inputs and both deliveries remain exact;
  objects, partial maps and all owned scratch/cache are removed. Independent
  source review confirms the explicit JSON conversion and unchanged guards.
  [Actual thin-link frontier](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-asset-consumption-v1/component-asset-experiment-v4.json).
- [x] Resolve those seven observed missing signatures to three existing C56
  objects: the selected planner, script-layout and HUD-bitmap-geometry providers.
  Named-object symbol inspection confirms their definitions; the planner/HUD
  objects add no project undefined symbols and the script provider needs only
  existing Bytes/Cache APIs. Preserve C56 order and use those exact objects in
  the next thin-link attempt. No extra archive, stub, engine rebuild or asset
  execution is accepted by this metadata result; inspection scratch is removed.
  [Planner provider](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-asset-consumption-v1/selected-planner-provider-v1.json)
  · [Script provider](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-c56-script-provider-audit-v1.json).
- [x] Link both real asset inspectors using those exact three added providers.
  All three compiles and both links exit0; link-map/API checks authenticate old
  and changed loader ownership and exclude World/Simulation/client entry and
  stale archive shader providers. One old-stock subprocess then actually runs,
  but the open-audit parser rejects the native snapshot owner's anonymous
  O_TMPFILE descriptor suffix. Its counter was recorded after parsing and
  therefore incorrectly says zero asset invocations; preserve that caveat.
  No stock or padded Cache acceptance is claimed. The next coordinator must
  record execution first and recognize only that exact owned anonymous-snapshot
  case, without treating an absent pathname as a hashed persistent input.
  Both deliveries and recorded inputs remain unchanged; owned scratch is removed.
  [Successful links and real audit frontier](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-asset-consumption-v1/component-asset-experiment-v5.work.json).
- [x] Correct the exact anonymous-snapshot audit case and run the old-stock
  inspector to actual exit0. Both links and map/API guards pass again. The
  remaining open-audit frontier is the exact read-only installed OpenSSL
  configuration used for hashing. All557 successful opens are retained; only
  the loader cache and that configuration lie outside owned scratch, prepared
  MCC data and installed shared libraries. Admit only that identified file
  after prehashing it, and preserve the earlier guard failure. The finally
  check confirms all522 prehashed assets and three already classified opened
  assets remain exact, with incomplete classification explicitly recorded.
  No changed-stock comparison or sparse case ran, so full consumption remains
  open. All generated objects, inspectors, raw JSON and owned scratch are removed.
  [Old-stock execution and configuration frontier](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-asset-consumption-v1/component-asset-experiment-v6.work.json).
- [x] Qualify actual selected Cache consumption of stock and sparse directories.
  Both inspectors compile/link with authenticated providers and all four asset
  subprocesses run: old/new stock exit0 with byte-exact canonical output;
  old padded Cache constructs but public lookup rejects actual D0008000;
  changed padded Cache passes. The owned32768-slot fixture preserves all5,816
  original live owners, names and payloads. All78 genuinely new native owners,
  4,398 references,253 StringIDs and7,426 pointer words pass full-handle and
  allocation checks; wrong salts still reject. All2,228 successful opens are
  classified and527 actual MCC dependencies rechecked, without Xbox/Vista or
  external asset fallback. Installed OS hashing configuration is separately
  identified. All522 prehashed assets, source/dependency pins, shaders and both
  deliveries remain exact. Raw outputs, maps, two inspectors, objects, padded
  map and owned scratch/cache are removed; only small reports remain. This
  passes the actual owner-consumption seam, not full mod/gameplay/64-bit parity
  or client delivery.
  [Stock and sparse Cache acceptance](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-asset-consumption-v1/component-asset-experiment-v7.json)
  · [Actual execution and dependency inventory](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-asset-consumption-v1/component-asset-experiment-v7.work.json).
- [x] Prepare the concrete ordinary C57 successor build from the accepted C56
  command. Only the old importer is replaced by the Cache-qualified importer
  and slot helper; all355 other physical inputs, their order and97 shader
  files remain exact. The planned357-input link checks exact API ownership and
  excludes stale importer extraction. Retain only the required raw candidate,
  provenance and six frozen source inputs; generated objects/maps/dependencies
  remain disposable and their future absence is explicit. Ordinary native
  acceptance and dual-directory delivery remain separate, unchecked gates.
  [Minimal native successor plan](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-client-plan-v1.json).
- [x] Preserve C57's actual preflight failure before compiling. The same three
  SSD runtime/output/ZIP inode identifiers changed again while their device,
  size and timestamps remain exact; development identities still match. Do
  not infer unchanged content from that metadata. The ordinary build stopped
  before either compiler or linker and published no candidate. Its owned
  scratch/cache are removed. Renew only those three SSD hashes with stable
  identities before retrying the unchanged build recipe; keep this failure
  and the prior identity authorities intact.
  [C57 preservation preflight failure](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-client-preflight-red-v1.json).
- [x] Renew that preservation authority with full byte checks of exactly the
  three affected SSD files. Both native binaries still match the qualified
  F48 bytes and the ZIP matches v23; identities remain stable during hashing.
  Development identities and all small selectors/capabilities/manifests and
  sidecars stay exact. The new proof chains both previous authorities and the
  real failed preflight. Scratch/cache are removed; no copy, build or game
  execution occurs in this renewal. Retry only the unchanged ordinary C57
  compilation/link with this separately versioned authority.
  [Second SSD byte confirmation](../.local/mcc-gameplay-continuation-20261020/native-build-ssd-identity-reconfirmation-v2.json).
- [x] Build the ordinary C57 native successor. Both component compiles and the
  full native link exit0; all355 inherited inputs and97 shaders remain exact,
  and the map/final-ELF checks bind precisely the three importer APIs and new
  helper. The raw candidate is107,348,416 bytes, SHA35f3f519ca26; cohort/source
  provenance and six frozen inputs retain108,245,643 bytes in total as required
  working-build inputs. Its two generated objects, dependency files, full map
  and owned scratch/cache are removed. No native execution or installation is
  accepted by this build-only result; stage/Play/video/portable delivery remain
  next, with F48/v23 still selected and preserved in both directories.
  [Sealed ordinary native successor build](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-client-build-v2.json)
  · [Actual build and dependency inventory](../.local/mcc-gameplay-continuation-20261020/native-tag-directory-client-build-v2.work.json).
- [x] Prepare and independently review the concrete F49/v24 delivery flow.
  It qualifies the ordinary staged route and requested sound video before
  selecting F49 on the SSD, then checks both installed Play paths and both
  designated outputs. The portable package reuses accepted v23 contents with
  only five declared entry changes, including the qualified preview-map path
  hunk; all4,429 other hashed entries preserve movies, prompts, loading artwork,
  tone, libraries and shaders. Preserve unrelated current packager changes and
  the development selector. C57's two removed objects remain honestly recorded,
  rather than requiring nonexistent inputs. This is a source plan only; native
  stage/installation/portable tests remain unchecked.
  [Dual-directory delivery plan](../.local/mcc-gameplay-continuation-20261020/native-tag-delivery-f49-v24-plan-v3.json)
  · [Independent delivery source review](../.local/mcc-gameplay-continuation-20261020/native-tag-delivery-f49-v24-independent-source-review-v1.json).
- [x] Stage F49 beside F48 and pass the actual unmuted MCC route: title/profile/
  menus, loading, Lockout GPU, keyboard/mouse input and clean exit0. The original
  four-voice ambience starts and stops at graphics join; PCM is finite with
  6,966,680 nonzero samples. The relocated runtime SHA531399ba4ee8 and97 exact
  shaders retain108,775,508 bytes as required candidate inputs. The open audit
  reports no forbidden asset fallback. Both launcher selectors remain exact
  and F48 stays selected. Scratch/cache are removed. This bounded route retains
  explicit source-effect boundaries and does not certify full gameplay/visual
  parity. Requested video, installed Play paths and both outputs/ZIPs remain next.
  [Staged native route](../.local/mcc-gameplay-continuation-20261020/f49-native-proof-v1.json)
  · [Stage terminal and cleanup](../.local/mcc-gameplay-continuation-20261020/f49-stage-terminal-v1.json).
- [x] Record and inspect the requested F49 menu/loading/Lockout video with sound.
  The1280x720 H.264/AAC clip lasts78.68 seconds and retains4,078,513 bytes.
  Loading/gameplay PCM both contain finite nonzero audio; the native process
  exits0, input changes904,245 frame pixels and the captured frame has35,335
  colors. Root visually inspected sampled menu, loading-logo and textured
  gameplay/HUD frames. Both selectors remain exact; the private Pulse sink,
  raw frames/PCM, capture and review scratch are removed. This is bounded
  visible/audible flow evidence, not full Xbox framebuffer or gameplay parity.
  [Requested video](../.local/mcc-gameplay-continuation-20261020/f49-menu-loading-lockout-with-sound.mp4)
  · [Capture acceptance](../.local/mcc-gameplay-continuation-20261020/f49-video-validation-v1.json)
  · [Root media review](../.local/mcc-gameplay-continuation-20261020/f49-video-root-review-v1.json).
- [x] Preserve the first real F49 installation failure and rollback. The
  selector check rejects before either launcher/game session runs; exact SSD
  F48 selector bytes and the independent development selector are restored.
  The newly written F49 capability remains unselected and pinned. Diagnosis
  finds development-tool drift: the imported development validator rejects
  the explicitly approved bundled original ambience, while the target SSD
  retains the accepted validator. Use the target's pinned tools for its own
  installation instead of changing either asset policy or capability fields.
  Owned scratch/cache are removed. Installed F49 Play acceptance remains open.
  [Failed install and rollback](../.local/mcc-gameplay-continuation-20261020/f49-installation-v1.json)
  · [Failure terminal/cleanup](../.local/mcc-gameplay-continuation-20261020/f49-installation-terminal-v1.json).
- [x] Prepare and independently review the narrow target-validator repair.
  It pins the target module bytes and import origins, preserves both actual
  Play-route tests and rollback, and reuses only the exact existing22,743-byte
  capability after checking complete computed-byte equality. No validator,
  asset-policy or capability fields are changed. Later publication uses the
  same accepted target validator; packaging preserves v23's accepted tool
  closure and binds the separate installation-v2 result. Source review passes;
  the actual retry and subsequent delivery remain unaccepted until execution.
  [Target-context repair review](../.local/mcc-gameplay-continuation-20261020/f49-install-admission-repair-independent-review-v1.json).
- [x] Install and qualify F49 through both actual SSD Play launchers. Normal
  and title Tk Play both reach MCC menus/lobby/loading/Lockout GPU, accept real
  input, start/stop the original ambience and exit0 without session errors.
  Both PCM streams are finite/nonzero. Target module origins/pins agree; the
  existing capability stays byte-exact. SSD selection SHA5089b828639e now names
  F49, while the independent development selector remains unchanged. All owned
  children, scratch and cache are removed; earlier F48 and failed-install
  rollback evidence remain preserved. Latest binary outputs and portable ZIP
  qualification/copy to both directories remain next.
  [Both installed Play routes](../.local/mcc-gameplay-continuation-20261020/f49-installation-v2.json)
  · [Installation terminal and cleanup](../.local/mcc-gameplay-continuation-20261020/f49-installation-terminal-v2.json).
- [x] Publish the qualified latest Linux binary and build manifest to both
  designated directories. Development output contains raw C57 SHA35f3f519ca26;
  SSD output contains the shader-path-relocated F49 SHA531399ba4ee8. Both are
  107,348,416 bytes; existing manifest rows and both selectors are preserved.
  Exact target admission/staging modules validate the outputs. Source capability
  and publication provenance are recorded; owned scratch/cache are removed.
  The portable v24 ZIP still requires creation, extracted-route qualification
  and the matching SSD copy/checksum.
  [Both latest build outputs](../.local/mcc-gameplay-continuation-20261020/f49-build-output-v1.json)
  · [Publication terminal and cleanup](../.local/mcc-gameplay-continuation-20261020/f49-build-output-terminal-v1.json).
- [x] Create and read back the requested compiled v24 ZIP. It retains
  2,724,447,363 bytes, SHA6f29e3d691ed, with4,434 entries and4,433 hashed files.
  Only runtime/capability/manifest/preview-map-function hashes and the checksum
  inventory change; all4,429 other hashes preserve bundled movies,91 custom
  prompt entries, ambience, custom loading artwork,97 shaders and190 libraries.
  No private imported MCC cache is bundled. Both selectors remain unchanged;
  intermediate payload/ZIP and owned scratch/cache are removed. This qualifies
  archive construction only: extracted normal/title Play and the final matching
  SSD ZIP/checksum copy remain next.
  [Portable v24 construction](../.local/mcc-gameplay-continuation-20261020/portable-v24-package.json)
  · [Archive readback and cleanup](../.local/mcc-gameplay-continuation-20261020/portable-v24-package-terminal-v1.json).
- [x] Independently reconcile the completed C57/F49 delivery receipts before
  final archive delivery. Build, staged route, video, installed routes, output
  publication and package receipts share the same exact native/cohort lineage.
  Both97-shader maps agree; current small selectors/capabilities/manifests bind
  development p53 and SSD F49 as intended. Six completed owned scratch trees
  are absent, and required runtime/video/ZIP retention is explicit. This
  source/receipt review does not accept the still-running extracted routes or
  pending SSD ZIP/checksum copy and repeats no gameplay or large payload hash.
  [Independent delivery consistency](../.local/mcc-gameplay-continuation-20261020/f49-v24-precopy-consistency-review-v1.json).
- [x] Qualify the extracted compiled v24 ZIP through both actual Tk Play
  routes using its bundled Python and an independent owned MCC cache clone.
  Normal/title reach menus, loading and Lockout GPU/native frames, accept33/30
  actual input events and exit0 without session errors. Both original loading
  ambience transitions occur and PCM is finite/nonzero. The complete archive
  checksum inventory, native admission and97 shaders pass. All extracted
  binaries/libraries/movies, raw audio, GPU cache and private MCC clone are
  removed with owned scratch; both installed selectors remain unchanged.
  This qualifies the exact2,724,447,363-byte ZIP SHA6f29e3d691ed for the pending
  matching SSD copy; full gameplay/TU1.5 parity remains open.
  [Extracted Play validation](../.local/mcc-gameplay-continuation-20261020/portable-v24-validation.json)
  · [Both route results and cleanup](../.local/mcc-gameplay-continuation-20261020/portable-v24-validation-terminal-v1.json).
- [x] Deliver the exact qualified compiled v24 ZIP and matching SHA-256
  sidecars to both requested repository directories. Both full archive reads
  match2,724,447,363 bytes/SHA6f29e3d691ed; both118-byte sidecars are exact.
  Latest native outputs/manifests still match raw C57 and relocated F49.
  SSD publication uses atomic no-replacement, preserves previous v23 archives
  and both selectors, and removes its owned SSD copy scratch/cache. Explicitly
  retain the requested ZIPs/builds and prior working runtimes/video; no public
  release or broader gameplay/parity completion is claimed.
  [Matching SSD delivery](../.local/mcc-gameplay-continuation-20261020/v24-ssd-delivery-v1.json)
  · [Terminal readback and cleanup](../.local/mcc-gameplay-continuation-20261020/v24-ssd-delivery-terminal-v1.json).
- [x] Close the bounded C57/F49/v24 delivery metadata and storage audit.
  Exact small selectors/capabilities/manifests and both checksum sidecars agree;
  all97 recorded shader owners are unchanged. Twenty-four named completed
  scratch/cache paths are absent, and the final audit wrapper exits0 with its
  own scratch/cache removed afterward. No native/gameplay/large payload check
  is repeated. Required working clients, both107,348,416-byte latest outputs,
  both2,724,447,363-byte v24 ZIPs and the4,078,513-byte requested sound video
  remain explicitly retained, with original/reference/unrelated work preserved.
  Scoped diff checks pass; the broader goal remains active.
  [Final delivery/storage seal](../.local/mcc-gameplay-continuation-20261020/f49-v24-final-metadata-review-v1.json).
- [x] Trace the remaining nested-map ownership boundary without executing or
  changing a mod. Exact relative conversion coverage keeps nested owners
  unqualified and native-test admission rejects them. Asset preview preserves
  the relative path but supplies the stock shared directory; the native shader
  bank still selects by basename and can attempt the stock bank for a nested
  map named lockout.map. Existing payload/closure checks remain, so actual mod
  loading success is unproved. Thirteen source pins and concrete call sites
  are recorded; no stock conversion borrowing or full mod support is inferred.
  [Mod preview/vanilla admission boundary](../.local/mcc-gameplay-continuation-20261020/mcc-mod-map-owner-admission-audit-v1.json).
- [ ] Define and qualify explicit relative-map ownership for native bank and
  resource-role selection before accepting nested/mod maps as converted vanilla
  data. Preserve mod paths/case and existing stock/native shader ownership;
  basename alone must not establish conversion or material-bank equivalence.
- [x] Prepare the concrete owner-binding contract from existing copied-source,
  conversion/output/graphics and native-bank metadata. A separate relative-map
  index can bind the final encoded map snapshot to its exact owned bank without
  changing preserved bank JSON/programs or canonical common-template hashes.
  Explicit nested A/B positive cases, resource roles, read-only stock migration
  and publication rollback are specified; copied MCC and original Xbox shader
  provenance remain distinct. This is an implementation/test plan only, with
  no new map decode, product edit, mod conversion/readiness or runtime claim.
  [Relative-map/native-bank contract](../.local/mcc-gameplay-continuation-20261020/mcc-native-bank-owner-contract-plan-v1.json).
- [x] Implement and review the pure metadata owner helper and regression
  fixtures. Caller-verified final encoded identities remain distinct from
  copied MCC and original Xbox shader provenance. Exact source-bound stock
  conversion rows and ordered graphics changes select final hashes; explicit
  custom receipts bind nested same-leaf maps to separate banks. Duplicate
  keys, case/path aliases, stale donors and stock-bank borrowing reject.
  This source-only layer performs no filesystem/symlink admission, producer
  publication, legacy Cache wiring or semantic-readiness promotion. Register
  its focused CTest before running the ten metadata/snapshot cases.
  [Owner helper](../tools/mcc_native_map_owners.py)
  · [Focused fixtures](../tests/test_mcc_native_map_owners.py).
- [x] Pass the focused production CTest for the pure map-owner helper.
  All ten metadata/snapshot cases pass through the registered CMake test,
  including three same-leaf owners with distinct banks, case/spaces, strict
  JSON/path rejection, final conversion/graphics identities, explicit resource
  roles,43 stock rows with39 banks/two HUD/two resource modes and separate
  Cairo dependency ownership. Before/after helper/test hashes remain exact;
  the private configure/build tree is removed and scoped diff checks pass.
  No actual map decode, filesystem ownership admission, import publication,
  C++ Cache consumer, mod gameplay or build installation is claimed.
  [Focused CMake result](../.local/mcc-gameplay-continuation-20261020/map-owner-metadata-focused-v1/report.json).
- [x] Implement and review the separate C++ map-owner resolver and focused
  filesystem/snapshot fixtures. Independently authenticated index hashes bind
  exact map/shared paths to already admitted encoded map/library snapshots.
  Local strings/paths own their storage; no Cache field, serialized width,
  shader program or runtime readiness changes. Case, symlink/traversal,
  same-leaf custom banks, explicit roles, post-admission filename replacement
  and separate HUD/common dependencies receive actual owned-file fixtures.
  Run only the small resolver target and its registered focused contract test;
  producer publication and production Cache integration remain open.
  [Reviewed C++ owner checkpoint](../.local/mcc-gameplay-continuation-20261020/mcc-native-map-owner-cpp-source-checkpoint-v1.json).
- [x] Pass the focused standalone C++ ownership contract on its first build.
  Configure, the small resolver/test target and registered CTest all exit0;
  exact snapshot/path/bank/role/HUD cases pass with the reviewed source hashes.
  The entire private build and synthetic filesystem fixtures are removed;
  no large artifacts are retained. Python producer metadata and C++ resolver
  are separately tested; paired production import/Cache integration and
  existing-stock compatibility still require their own acceptance.
  [Focused C++ result and cleanup](../.local/mcc-gameplay-continuation-20261020/mcc-native-map-owner-cpp-focused-terminal-v1.json).
- [x] Qualify the actual Python producer's encoded index and snapshots through
  the standalone C++ resolver. The prepared bridge covers eight exact owners,
  four stale/path/hash rejection controls and one unbound same-leaf map,
  including Unicode/u32 canonical metadata and ordered final-output identity.
  Independent source review found a test-helper replacement-write containment
  gap. The reviewed V2 now binds both writes to declared fixture owners and
  validates exact relative names, symlink-free existing regular targets and
  private-root containment before either write. Implementations stay exact;
  The typed V3 comparison correction below then passes configure, the small
  target and all three serial Python/C++/cross-language CTest groups. Eight
  positive, four negative and one unbound case complete, preserving three
  distinct same-leaf banks. Before/after implementation/test/CMake pins match;
  owned build, synthetic fixtures and cache are removed with no large retention.
  Production import/Cache integration remains a separate open gate.
  [Reviewed bridge correction](../.local/mcc-gameplay-continuation-20261020/mcc-map-owner-cross-language-source-checkpoint-v2.json)
  · [Actual paired producer/consumer acceptance](../.local/mcc-gameplay-continuation-20261020/mcc-map-owner-cross-language-focused-terminal-v2.json).
- [x] Preserve the first cross-language build failure and review its narrow
  correction. Configure succeeds, build exits2 and no tests execute: six new
  test comparisons require explicit JSON string extraction with nlohmann3.12.
  The reviewed V3 adds only those six get<std::string>() calls; all producer,
  resolver and expected-case definitions remain unchanged. The first private
  build is removed and its actual diagnostics remain immutable. Run the three
  focused groups once against V3 in a separate result directory.
  [First compile failure](../.local/mcc-gameplay-continuation-20261020/mcc-map-owner-cross-language-focused-terminal-v1.json)
  · [Reviewed typed-comparison fix](../.local/mcc-gameplay-continuation-20261020/mcc-map-owner-cross-language-source-checkpoint-v3.json).
- [x] Audit the production Cache integration seam and selected ABI boundary.
  Admit the actual encoded snapshot before decode; keep a local owning context
  through both private MCC initializers without adding Cache fields. Existing
  HUD and Cairo helpers reopen identity filenames, so prepared consumers need
  snapshot/fingerprint overloads retaining all original closure checks. The
  selected F50 library has a separate two-argument API and its Cache layout
  predates current bitmap-cache growth; normal source changes do not qualify
  selected adoption. Preserve the working delivery throughout integration.
  [Pinned constructor/importer/library audit](../.local/mcc-gameplay-continuation-20261020/mcc-cache-owner-context-integration-audit-v1.json).
- [x] Integrate and qualify source-level staged/repair owner publication with
  verified cache admission, preserving map/tag/data/program bytes, custom receipts and
  rollback. Qualify pointer/index consistency, real scalar constants and the
  SSD filesystem's absent-only atomic publication before source integration.
- [x] Review the combined native-repair transaction source before execution.
  Missing Cairo, catalog, common scalar tables, owner index and provenance now
  share one SIGTERM-aware rollback boundary. Journal entries precede renames;
  failed restoration retains its recovery copy. Candidate ownership planning
  keeps old pointer/provenance authenticated and source/graphics/readiness
  fields unchanged, including explicit custom bindings. Existing programs and
  gameplay bytes are not written. Twelve dedicated synthetic cases exercise
  the real planner with shipped scalar constants; legacy shallow fixtures
  explicitly stub only their unrelated ownership seam. No cross-file crash
  atomicity or concurrent-writer guarantee is claimed. Register the focused
  CTest and qualify actual SSD exFAT renames before wider source acceptance.
  [Reviewed source and regression checkpoint](../.local/mcc-gameplay-continuation-20261020/mcc-owner-shader-transaction-source-checkpoint-v1.json).
- [x] Qualify all twelve combined-repair cases on the actual SSD exFAT volume.
  The first run passes12/12 with wrapper exit0. The entire synthetic fixture
  tree lives under the SSD's owned scratch and exercises directory/Cairo/LUT/
  catalog/index/provenance rollback, including real SIGTERM after the final
  rename and deliberate recovery-copy retention inside its temporary fixture.
  All eight source/runner pins remain exact; the SSD scratch and all generated
  fixtures are removed. No large artifacts, installed caches, native binaries,
  shaders, selections or readiness flags change. This qualifies exception/
  cancellation rollback on this filesystem, not crash or multiwriter atomicity.
  [Actual SSD transaction result and cleanup](../.local/mcc-gameplay-continuation-20261020/mcc-owner-shader-transaction-ssd-v1.json)
  · [Independent source review](../.local/mcc-gameplay-continuation-20261020/mcc-owner-shader-transaction-independent-source-review-v1.json).
- [x] Qualify the isolated publisher and admitted-library snapshot overloads.
  Both focused CTests pass with configure/build/test exits0. Twelve synthetic
  publication cases use the actual shipped scalar constants and cover exact
  final output, custom owners, idempotence, corrupt beforeimages, rollback and
  absent-only rename without hard links. Genuine shipped Lockout/Cairo metadata
  passes two snapshot supplementation cases and three atomic rejections; twelve
  HUD negatives preserve its existing fingerprint/directory/body guards.
  Full positive HUD and actual Cache/game acceptance remain separate. Only the
  small library/test target builds; all owned build/fixture storage is removed.
  Scoped sources stay exact except separately recorded concurrent CMake edits.
  [Focused publisher/library proof](../.local/mcc-gameplay-continuation-20261020/map-owner-publication-library-focused-v1-terminal.json).
- [x] Qualify the prepared-cache C++ adapter, legacy stock projection and the
  normal Cache source wiring. Check stock/resource/HUD and nested same-leaf
  owners, exact library namespaces, admitted-snapshot lifetime and unchanged
  constructor/layout contracts. Source compilation and standalone tests alone
  do not prove actual Cache/game execution or qualify a new installed runtime.
  Actual Cache acceptance now passes the ten-case scenario/nested/HUD witness
  and six direct-resource calls below. Native gameplay consumption, strict-MCC
  presentation integration and installed-runtime qualification remain separate.
- [x] Audit the real selected-cache metadata before planning actual Cache use.
  Both installations retain883 source records,43 immediate map/conversion rows
  and39 native-bank proofs with no owner index. The exact HUD input extents and
  recorded hashes match the compiled policies. Lockout/Cairo metadata has the
  same canonical identities despite different formatting; raw hash proofs must
  stay with their own preparation. SSD shared/mainmenu/DAT extents include
  recorded conversions absent from the development copy, so do not mix their
  files or chains. This reads metadata and file stats only; actual payload
  hashes/Cache construction remain unverified. Preserve full43-map metadata
  authority for the owned witness rather than silently omitting its inputs.
  [Selected preparation availability and closure](../.local/mcc-gameplay-continuation-20261020/mcc-prepared-cache-metadata-availability-v2.json).
- [x] Prepare and review a selected-layout Cache ownership projection.
  Three exact selected source bases receive only constructor/context/role/
  snapshot changes; metadata resolver/admission and two snapshot-library APIs
  are additive. The old Texture/Material/Cache fields and legacy library provider
  remain intact. The latter's retained source differs from its historical
  compile receipt, so preserve its binary and qualify new APIs independently.
  Existing HUD/closure guards are carried over without the unrelated current
  bitmap-cache, texture-owner or intro-template changes. All reversals/copies
  are pinned; source preservation does not prove compiled layout, linking or
  actual loading. Compile the six changed/additive units in disposable storage
  before the genuine-stock/indexed/custom/HUD Cache witness.
  [Exact selected source projection and provider plan](../.local/mcc-gameplay-continuation-20261020/mcc-selected-cache-owner-source-plan-v1/plan.json)
  · [Independent field/type/API review](../.local/mcc-gameplay-continuation-20261020/mcc-selected-cache-owner-independent-source-review-v1.json).
- [x] Measure the selected Cache projection's layout before component loading.
  Original, selected and projected declarations agree: Cache592, Texture72,
  Material2760 and Tag80 bytes, all aligned to8. The first component compile
  then stopped at the missing frozen `mcc_resource_reader.hpp` dependency;
  no component object, symbol pairing, link or actual Cache load qualified.
  Preserve that first failure while reviewing the exact original-manifest
  dependency for the successor. Scratch and generated assembly were removed.
  [First selected compile and layout result](../.local/mcc-gameplay-continuation-20261020/mcc-selected-cache-owner-source-plan-v1/compile-only-v1.json).
- [x] Compile the selected-layout ownership projection with its original
  resource-reader declaration restored from the exact original manifest.
  All six actual compiler invocations and three layout probes pass. Old/new
  constructor symbols, both private call pairs and the two new snapshot-library
  overloads match; the old library provider remains unchanged. Actual dependency
  records cover3469 rows/666 unique paths. Preserve the earlier failed attempt.
  All28380216 temporary object bytes and scratch were removed. This qualifies
  compilation and symbol pairing only; final linking and actual MCC Cache
  consumption, including nested and HUD owners, remain unchecked.
  [Selected six-unit compile qualification](../.local/mcc-gameplay-continuation-20261020/mcc-selected-cache-owner-source-plan-v1/compile-only-v2.json)
  · [Independent dependency correction review](../.local/mcc-gameplay-continuation-20261020/mcc-selected-cache-owner-source-plan-v1/compile-only-v2-independent-source-review-v1.json).
- [x] Complete the ten-case actual selected Cache ownership comparison.
  V4 passes all ten intended cases (nine successful constructions and one exact
  missing-own-library rejection). All six full public-object comparisons match;
  parent/C++ canonical hashes agree. Each Lockout comparison covers23209 tag
  slots,6400 resident definitions,565 retained allocations and559 native tags.
  Separate nested banks stay isolated, B still loads while A is unavailable,
  and nested `shared.map` keeps its declared scenario role. Both HUD maps retain
  their three required native definitions, complete typed references and pointer
  maps. Own/common library metadata opens exactly once per candidate call.
  All188 protected inputs/666 compiler dependencies and1467 source/3028 copied
  fixture pins remain intact. Complete compact diagnostics retain all traces
  losslessly in237260 bytes; scratch, temporary banks and binaries are removed.
  This is actual Cache/data acceptance only: arbitrary-mod conversion,
  native gameplay/visual/audio and delivery adoption
  remain separate gates. Earlier failed attempts below remain preserved.
  [Ten-case actual Cache qualification and cleanup](../.local/mcc-gameplay-continuation-20261020/mcc-selected-cache-owner-witness-v1/terminal-v4.json)
  · [Independent compact per-call evidence](../.local/mcc-gameplay-continuation-20261020/mcc-selected-cache-owner-witness-v1/progress-v4.json).
- [x] Qualify both direct stock resource constructors against the selected
  baseline. All eleven compiles, two links and six actual Cache calls pass;
  four full-object comparisons match across legacy and indexed admission.
  `shared.map` retains23131 tag rows/6098 resident definitions/87 retained ranges;
  `single_player_shared.map` retains10262/10220/111 respectively. Every call
  preserves the authentic28672-byte scalar tables, with zero native tag flags,
  pointer maps, BSPs, spawns or gameplay writes. All772 successful opens are
  classified; no native program payload, own resource bank or recursive map
  contents are consumed. The indexed common library metadata opens once.
  All191 protected inputs/666 compiler dependencies/86 source/87 fixture pins
  remain intact. Complete diagnostics are retained losslessly in68896 bytes;
  the1665776730-byte copied asset set,44459496 object bytes and scratch are
  removed. This closes the Cache adapter's resource gate, without granting
  World/game/render/audio, full import, runtime adoption or delivery acceptance.
  [Direct-resource Cache qualification and cleanup](../.local/mcc-gameplay-continuation-20261020/mcc-selected-cache-resource-witness-v1/terminal-v1.json).
- [x] Recheck the whole installed H2EK directory for the exact loading tone.
  All18396 reference ADPCM bytes match inside `guerilla.exe`,
  `halo2_tag_test.exe`, `sapien.exe` and `tool.exe`; the current executables'
  complete hashes and matching ranges were reverified read-only. The temporary
  probe directory is removed. This confirms an extractable H2EK source only;
  no H2EK onboarding/runtime dependency or presentation-policy change follows.
  [Exact H2EK loading-tone evidence](../.local/mcc-gameplay-continuation-20261020/h2ek-loading-ambience-byte-search-v1.json).
- [x] Preserve the earlier failed Cache ownership witnesses. The first run
  compiled all eleven units and linked the baseline inspector, then stopped at
  the exact constructor-provider map check. Candidate linking, asset copying
  and all ten Cache calls were unattempted. Preserve this failure; constructor
  demangling ambiguity is a hypothesis until the actual map rows qualify it.
  All184 protected inputs/666 actual compiler dependencies remain intact;
  scratch and44542472 generated object/ELF bytes were removed. Retained bounded
  diagnostics are lossless, with no new large artifacts.
  [First thin-link gate result](../.local/mcc-gameplay-continuation-20261020/mcc-selected-cache-owner-witness-v1/terminal-v1.json).
  A controlled baseline reproduction now qualifies the checker's ambiguity:
  C1/C2 each have one strong definition from the selected Cache object, but the
  default map emits two identical demangled constructor labels. An output-only
  no-demangle control gives one distinct raw stanza for each constructor with
  its exact provider. Use raw symbol stanzas and retain diagnostics before the
  assertion; do not relax definition counts/provider checks. Both links and all
  four diagnostic compiles passed. This is new reproduction evidence; the
  original failed map remains unavailable. Diagnostic scratch and helper were
  removed, with no Cache/asset execution.
  [Controlled constructor-map diagnosis](../.local/mcc-gameplay-continuation-20261020/mcc-selected-cache-owner-witness-v1/baseline-cref-diagnostic-v1.json).
  The second full attempt passes all eleven compiles, both links and exact raw
  provider checks (30 baseline/36 candidate). It copies the genuine full43-map
  metadata/input set and1465 required files, then baseline construction stops
  at `Invalid preserved native shader file`. No candidate/index/HUD case ran.
  The test copy omitted each bank's separately declared464-byte rasterizer
  payload: both genuine records exist outside `shader_tags`. Add that explicit
  declaration to the temporary copy; engine/providers stay unchanged. Original
  failed-open trace text was not retained, so its exact failed filename remains
  unproven; retain complete finite traces before guards in the successor.
  All protected/copied inputs are preserved and the2234442123-byte temporary
  asset set, objects and scratch are removed.
  [First actual baseline Cache result](../.local/mcc-gameplay-continuation-20261020/mcc-selected-cache-owner-witness-v1/terminal-v2.json).
  The third attempt supplies the separate rasterizer records and reaches all
  ten calls. Recorded exits are0 for1–5/7–9, expected2 for6, and1 for10. Its
  duplicated full tag dumps inflate the report to384862318 bytes and compressed
  details exceed the8MiB retention cap. Final HUD error text/fingerprints/traces
  were lost during cleanup; exit codes alone do not qualify those cases. Keep
  this RED and retain compact actual errors, section hashes/counts and explicit
  deep-comparison outcomes before every assertion in the successor, while full
  tag objects stay temporary. Sources/copies remained intact and scratch was
  removed. No runtime/adoption gate is completed from this attempt.
  [Retention and final HUD frontier](../.local/mcc-gameplay-continuation-20261020/mcc-selected-cache-owner-witness-v1/terminal-v3.json).
  Source review identifies a separate test-command typo: the filename contains
  `street_sweeper`, while the inspector's declared mode uses `street-sweeper`.
  Correct that one coordinator argument before the next run, keeping the map
  and inspector unchanged. This source diagnosis does not recover the missing
  historical stderr. Independently reviewed compact reporting preserves exact
  errors and full-object comparison outcomes even if detailed traces exceed
  their storage bound.
- [x] Retire the completed Main probes and four superseded Cache coordinators.
  Only the six exact audited, untracked scripts are removed (108120 logical
  bytes). Full source is preserved in bounded receipts before deletion; live
  V3/V4, frozen projections, inspector, policies, recipes, evidence and selected
  runtimes remain protected. All63 scoped protected files and scratch cleanup
  checks pass; no free-space increase or broad unused-test cleanup is inferred.
  [Main helper retirement](../.local/mcc-gameplay-continuation-20261020/mcc-preview-prepared-root-bridge-v1/current-main-helper-retirement-terminal-v1.json)
  · [Superseded Cache helper retirement](../.local/mcc-gameplay-continuation-20261020/mcc-selected-cache-owner-helper-retirement-v1.json).
- [x] Trace exact prepared-root ownership through real production callers.
  The launcher already knows the selected private cache, but nested previews
  lose that context before the three-argument Cache constructor. Conventional
  stock menus still satisfy the adapter's exact inference rule. A future
  context option must carry the selected root/map/shared tuple through a
  coherently qualified runtime; frozen F50 cannot receive an unknown option.
  Custom shared directories and prefetched-world reuse need their own exact
  owner matching. No runtime/mod readiness or caller implementation is granted.
  [Caller and capability boundary](../.local/mcc-gameplay-continuation-20261020/mcc-prepared-root-production-caller-audit-v1.json).
- [x] Implement and compile the launcher half of explicit preview-root
  transport. A bounded query checks the chosen executable before the launcher
  authors the selected cache root; unsupported stock builds keep exact old argv
  and unsupported nested previews reject. All three launcher variants build;
  their three first-run suites and existing frontend/gameplay routing suites
  pass. Synthetic owners prove argv routing only. The native source query and
  local four-argument Cache call are implemented without public option/layout
  growth. Current Main compilation now passes; actual native feature/admission execution and
  coherent delivered-runtime adoption remain open. Keep the broader gate below
  unchecked until those steps pass.
  [Exact three-file source change](../.local/mcc-gameplay-continuation-20261020/mcc-preview-prepared-root-bridge-v1/source-checkpoint-v1.json)
  · [Five compiled-launcher suites](../.local/mcc-gameplay-continuation-20261020/mcc-preview-prepared-root-bridge-v1/validation-v1/report.json).
  The first Main compile preparation stopped before invoking the compiler:
  selecting by source file alone was ambiguous because CMake also reuses Main
  in a campaign test entry. Qualify the native target's generated output path
  and retain candidate rows before that guard. The original generated rows were
  not retained, so no exact historical row-count claim follows. Configure and
  target listing passed; the empty CTest selection is not test acceptance.
  Scratch was removed and inputs remain intact.
  [First Main preparation result](../.local/mcc-gameplay-continuation-20261020/mcc-preview-prepared-root-bridge-v1/current-main-compile-terminal-v1.json).
  The target-qualified successor now passes the sole actual native Main compile
  using a freshly generated Release command and coherent current headers.
  Both current Main rows were retained; exactly one names the native target.
  All443 consumed project dependencies match precompile/final hashes, with785
  installed headers hashed after compilation and rechecked. Four existing
  indentation warnings remain recorded. The1237056-byte object and scratch
  were removed. No native link/run or current/selected ABI interchange follows.
  [Current Main compile qualification](../.local/mcc-gameplay-continuation-20261020/mcc-preview-prepared-root-bridge-v1/current-main-compile-terminal-v2.json).
- [x] Identify the current strict-presentation runtime boundary before adoption.
  The examined independent loading-performance runtime already declares MCC
  loading/native-font profiles, but its newer Cache/Texture/Material/Memory
  layout and removed object/build tree do not provide a reusable old-layout
  link cohort. Preserve that selection. F50 has an exact persistent parent but
  needs separately qualified MCC loading/native-font source families before a
  new build can meet the current policy. Its mechanical Cache/Main cut alone
  cannot grant strict presentation or delivery readiness. Frontend-font archive
  ownership and emitted equivalence are now qualified separately below.
  [Recorded cohort/policy and provider boundary](../.local/mcc-gameplay-continuation-20261020/mcc-selected-cache-owner-strict-runtime-readiness-v1.json).
- [x] Qualify the strict MCC loading/font source families with the selected
  layouts. All six component compiles pass. The unchanged frontend-font source
  matches the selected archive member across26 allocated sections/20816 bytes,
  520 relocations,120 linkage symbols and11 groups; real byte and relocation
  changes are rejected by the controls. The unchanged gameplay-font helper
  stays reusable. One sampler-route hunk selects native MCC glyphs; the V3-only
  loader and its two new independent providers compile without growing the old
  Cache/Texture/Material/Memory/Desktop types. All1441 protected pins and922
  actual compiler dependencies remain intact;11461352 object bytes and scratch
  are removed. Tool paths were resolved/pinned but the helper used bare tool
  names; enforce their exact lookup explicitly in any successor. This proves
  source/component compatibility, without actual bank/font loading, final link,
  visible prompts, native audio or installed-runtime acceptance.
  [Strict presentation components and cleanup](../.local/mcc-gameplay-continuation-20261020/mcc-strict-presentation-source-v1/components-v1.json).
  Initial font-probe metadata shows the inspected archive fallback's monolithic
  Bytes provider pulls additional Cache dependencies. No font read or compiler
  ran. The explicit selected Cache provider is still unmeasured; inspect that
  authority or a genuinely identical narrow leaf before constructing the probe.
  [Preserved font-closure frontier](../.local/mcc-gameplay-continuation-20261020/mcc-strict-presentation-source-v1/native-font-probe-v1/closure-metadata-v1.json).
  The actual selected provider also proves monolithic. A bounded test-support
  unit copies only its four exact byte-reader bodies/declarations, retaining
  the actual font, gameplay-font, sampler and UI/texture providers. Three
  compiles, the thin link and all786686 native-font assertions pass:12 selectors,
  331 bindings,34152448 unchanged atlas bytes and three negative cases. The
  enclosing audit remains RED at nine unclassified read-only cleanup-directory
  opens; its later installed-input final loop did not run. Complete original
  traces/results are retained, all declared/compiler/font pins are preserved,
  and scratch is removed. Finish those exact rows and preservation offline;
  do not rerun fonts or promote the component to rendered UI acceptance.
  [Actual font result and audit frontier](../.local/mcc-gameplay-continuation-20261020/mcc-strict-presentation-source-v1/native-font-probe-v1/actual-v1.json).
- [x] Complete the native MCC font component's original open audit offline.
  The retained trace reparses identically to all100 successful rows, including
  the nine exact named read-only cleanup-directory opens. All twelve originally
  recorded installed-input hashes/identities and1792 unique source/dependency
  hashes agree; final path metadata is stable. Original font/source preservation
  remains the actual first run's evidence. No font, compiler or native replay
  occurs. This completes the12-selector/331-binding/786686-assertion component
  gate with34152448 unchanged atlas bytes, three negatives and no authored
  prompt dependency; the original wrapper1 audit result remains immutable.
  Both scratch owners and temporary objects/fonts are removed. The four exact
  byte-reader bodies are test support only; full Cache, rendered glyph geometry,
  actual menus/audio and installation still need their own acceptance.
  [Sealed offline font audit completion](../.local/mcc-gameplay-continuation-20261020/mcc-strict-presentation-source-v1/native-font-probe-v1/offline-audit-v1.json).
- [x] Project the selected Main's actual compatibility changes in isolation.
  Seven substitutions add the early preview capability query, local prepared
  root parsing/admission and conditional Cache constructor, then replace the
  loading error catch-and-continue with the explicit V3 call. Every other byte
  remains selected v43 Main; reversing all seven restores its exact hash.
  Independent source review confirms the query precedes Desktop initialization
  and V3 failure reaches the existing outer error return. The first generation
  stopped before writes because another chat changed current Main; the successor
  reauthenticates only the required current hunks and preserves unrelated work.
  No Main compile, native link or runtime behavior is qualified here.
  [Selected Main source and reversal](../.local/mcc-gameplay-continuation-20261020/mcc-strict-presentation-source-v1/main/source-checkpoint-v1.json)
  · [Preserved prewrite guard](../.local/mcc-gameplay-continuation-20261020/mcc-strict-presentation-source-v1/main/source-preparation-red-v1.json).
- [x] Create and verify a genuine MCC V3 loading bank in disposable storage.
  The existing producer extracts the authenticated WEM from installed MCC and
  reconstructs the mask from the selected cache's exact MCC bitmap owners.
  Preparation and verification return the same complete manifest. The stereo
  float decode has2744320 frames and the expected full PCM hash; the512-square
  mask and21954604-byte WAV match their recorded identities. All eight distinct
  native font payloads and the selector table also match installed MCC and the
  current native profile. This is byte/provenance acceptance, without C++ glyph,
  loading renderer, audible playback or original Xbox timing/pixel acceptance.
  Sources and nine resolved converter libraries remain intact;23096121 generated
  bytes and scratch are removed. No selected cache, provenance or launcher
  selection changes. An existing development-cache V3 bank is also available
  for later coherent native validation; the SSD cache still needs migration.
  [Actual isolated V3 extraction and input identities](../.local/mcc-gameplay-continuation-20261020/strict-loading-bank-isolated-v1.json).
- [x] Build a coherent selected-layout Cache/loading/native-font successor.
  All thirteen compiles and the native link pass. The actual command retains
  363 unique inputs/364 ordered occurrences, including the original doubled
  curl input;350 other persistent inputs and all97 shader files remain exact.
  All47 checked APIs have one final definition and their expected raw-map
  provider. The replaced old objects/archive members are absent. Selected font,
  gameplay-font, Desktop and legacy-library providers remain unchanged. Main's
  isolated prepared-root and strict V3 calls now compile with the same old
  layouts. All2216 protected pins/1476 actual compiler dependencies are intact.
  The107581128-byte candidate, cohort/recipe and138 frozen inputs are explicitly
  retained (111400579 bytes total);59006224 generated object bytes, map/dependency
  outputs and scratch are removed. This closes compilation/provider coupling,
  without a native frame, loading/audio/prompt, capability, selection or delivery
  acceptance grant. Next run the reviewed ordinary route on an owned authentic
  MCC-cache projection, preserving both installations until that result passes.
  [Coherent native build and cleanup](../.local/mcc-gameplay-continuation-20261020/mcc-strict-presentation-source-v1/native-client-build-v1.json).
- [x] Execute the coherent candidate's genuine MCC ordinary native route.
  The capability query returns the exact supported schema. Removing only the
  owned V3 manifest produces the exact error/exit1; after byte-exact restoration,
  title/profile/lobby/countdown/loading/Lockout, movement/input and clean exit
  pass. The frame has85558 colors and902653 changed pixels after input. Both
  captured loading and gameplay audio are finite/nonzero. The owned projection
  preserves all43 maps/39 library metadata files and883 source records while copying904
  actual route/input files; it is not full onboarding acceptance. Native traces
  account for90503 positive regular opens/868 unique payloads and299 negative
  opens/292 unique payloads, with no external-cache/reference/legacy-bank borrow
  or writes. Runtime/cohort, source inputs, selections and all97 shaders remain
  exact. Root inspected title/loading/match frames and verified all fourteen
  retained artifact hashes/lossless roundtrips. The requested69.16-second video
  has1280x720 H.264 plus stereo48kHz AAC; video/four PNGs and bounded reports total
  6961508 bytes. All3397059232 owned input bytes, scratch, child processes and
  audio sink are removed; the default sink is unchanged. This is direct native
  acceptance with genuine MCC audio's finite one-shot policy, not original Xbox
  ambience/timing/pixel parity, installed Tk Play or complete native64/gameplay.
  Existing firing-effect `f4105503` still requires its coordinate/lifecycle owner.
  [Sealed actual native route and cleanup](../.local/mcc-gameplay-continuation-20261020/native-strict-mcc-ordinary-frame-terminal-v1.json)
  · [Requested video with sound](../.local/mcc-gameplay-continuation-20261020/native-strict-mcc-ordinary-frame-v1/actual.mp4).
- [x] Identify the remaining normal-Play and delivery ownership gap.
  The retained movie provider still chooses the old bundled intro/credits;
  only title-first launcher routes add `--skip-intro`. The new direct capture
  deliberately uses that flag, so it cannot qualify normal Play's intro path.
  Current strict movie-path source already leaves unassigned MCC roles empty;
  project that verified decision onto the selected source without adopting new
  decoder/layout/performance code or assigning unrelated .bk2 media. SSD/package
  tools also still use legacy V2 admission and movie preparation; a runtime-only
  repack cannot update that closure. F51/v26 names are available, but selections,
  outputs and ZIP remain unchanged pending this qualification. The full movie
  role/fidelity and broader gameplay/native64 gates stay open.
  [Concrete two-directory adoption/delivery plan](../.local/mcc-gameplay-continuation-20261020/native-strict-mcc-f51-v26-delivery-readonly-plan-v1.json).
- [x] Qualify the selected movie source before changing its MCC path decision.
  The unchanged source compiles and matches its actual selected provider across
  213 allocated sections/123690 bytes,3937 relocations,718 linkage symbols and
  97 groups. Byte/relocation controls reject modifications. The one-function
  strict projection also compiles and retains all thirteen public APIs, with
  every decoder/lifetime/non-MCC source byte preserved. Its first routine then
  stops at an incorrectly transcribed expected symbol: both actual providers
  define the same `frontend_movie_path` symbol with `S4_`, while the recipe
  expected `S5_`. No routing test compiled, linked or ran. Keep this RED and
  correct only the recipe literal from the retained actual symbol row before
  the successor; no engine/source or expected routing behavior changes.
  All scoped inputs are intact and scratch/objects are removed. Movie routing,
  coherent successor and normal Play remain unqualified at this point.
  [Actual movie baseline and first guard frontier](../.local/mcc-gameplay-continuation-20261020/mcc-strict-movie-source-v1/qualification-v1.json).
- [x] Qualify the strict MCC movie-path decision after the exact recipe fix.
  The successor preserves the original RED and changes only its expected API
  literal from the authenticated original symbol inventory. Baseline equivalence
  and controls pass again; the changed module retains all thirteen public APIs.
  All three compiles, the focused link and22 actual-function checks pass. MCC
  intro/credits/attract remain empty with absent or poisoned retained banks;
  invalid roles reject, and non-MCC reference routing stays unchanged. No real
  movie/media/cache asset is opened or decoded. All120 protected inputs and728
  actual compiler dependencies remain intact;4631856 object bytes and scratch
  are removed. This qualifies the one-function compatibility change, without
  assigning MCC movie roles or qualifying a normal client/launcher route.
  Link the coherent movie-fixed successor before the no-skip-intro capture.
  [Selected movie-path component and cleanup](../.local/mcc-gameplay-continuation-20261020/mcc-strict-movie-source-v1/qualification-v2.json).
- [x] Link the coherent movie-fixed strict-MCC successor. All fourteen compiles
  and the native link pass, preserving363 unique inputs/364 ordered occurrences,
  the349 unaffected persistent owners and all97 shaders. All60 checked APIs
  have their sole expected provider; old replaced providers are absent. Public
  and private layouts remain unchanged. All2226 protected inputs and1513 unique
  actual compiler dependencies remain exact. The107538208-byte candidate plus
  cohort, recipe and139 frozen inputs are explicitly retained (111583314 bytes
  total). All59742504 temporary object bytes, dependency/map outputs and scratch
  are removed. No native execution, selection, cache, output or ZIP change is
  implied. Normal boot and installed launcher acceptance remain separate.
  [Movie-fixed coherent build and cleanup](../.local/mcc-gameplay-continuation-20261020/mcc-strict-presentation-source-v1/native-client-build-v2.json).
- [x] Execute normal startup with the movie-fixed coherent successor, without
  the skip-intro option. Capability query0 and missing-V3 refusal1 pass; actual
  title/menu/lobby/countdown/loading/Lockout, input, sound and clean native exit0
  pass with no movie or movie-audio cues. MCC movie roles remain unassigned.
  The positive trace accounts for90515 regular opens/868 unique owners; the
  negative for299/292, without external asset borrowing or cache writes. Loading
  has71993 finite stereo frames; gameplay has687740. Root inspected title,
  reconstructed loading and textured match frames, authenticated all fourteen
  retained artifacts and their lossless roundtrips, and verified the70.68-second
  1280x720 H.264/stereo48kHz AAC video. Requested video/frames and bounded reports
  total7180472 retained bytes. The3374013282-byte owned input copy, scratch,
  XDG, native processes and audio sink are removed; default sink, all97 shaders,
  source inputs and both selections remain unchanged. This closes direct normal
  native startup, without installed Tk Play, movie fidelity, original Xbox
  ambience/pixels or complete gameplay/native64 acceptance.
  [Normal native route and cleanup](../.local/mcc-gameplay-continuation-20261020/native-strict-mcc-normal-frame-terminal-v1.json)
  · [Normal menu/loading/match video with sound](../.local/mcc-gameplay-continuation-20261020/native-strict-mcc-normal-frame-v1/actual.mp4).
- [x] Preserve the first focused strict packaging result and its fixture gap.
  Configure and the small target pass. Frontend capability, presentation
  dispatch and loading import groups pass; `release_movie_bundling` fails two
  assembly subcases because its fixture lacks the separately supported active
  controller-artwork manifest. Current V3 package/import routing already avoids
  the prior supplied movie, original tone and single authored prompt bank; no
  product change is justified by this missing fixture. A concurrent two-line
  fixture correction now supplies the existing synthetic controller helper;
  its exact reversal restores the pre-run file. Preserve that owner's edit and
  rerun only the failed group. Later source/CMake drift is recorded separately
  from the completed run; scratch is removed and no actual package/runtime/
  cache/selection/ZIP is written. Full source-tool qualification remains open.
  [Original packaging result and cleanup](../.local/mcc-gameplay-continuation-20261020/strict-mcc-packaging-source-qualification-v1/focused-v1-terminal.json).
- [x] Preserve the failed-group packaging successor and its actual next frontier.
  The controller fixture correction succeeds; configure and the small target
  pass. Both assembly subcases now stop at the old README expectation that all
  button prompts use MCC font files. The actual package describes per-player
  Xbox/PlayStation/keyboard selection, separate controller artwork and ordinary
  MCC fonts. No product failure is demonstrated. The assertions after this
  point remain unqualified. All14 scoped source pins remain exact; scratch and
  synthetic package outputs are removed. Correct only this stale text assertion
  before another failed-group run, preserving both earlier failures.
  [Actual README assertion frontier and cleanup](../.local/mcc-gameplay-continuation-20261020/strict-mcc-packaging-source-qualification-v1/focused-v2-terminal.json).
- [x] Complete the focused packaging assembly successor. Preserve the concurrent
  ordinary-font assertion correction and add the current per-player prompt
  wording assertion, leaving all routing and forbidden prior-bank checks intact.
  Configure, the small target and the single release_movie_bundling CTest pass.
  All14 scoped source pins remain unchanged during execution. Both earlier
  failures are retained; private build, synthetic package outputs and scratch
  are removed. No production, installed package, runtime, cache or selection
  change follows from this synthetic assembly qualification.
  [Focused assembly pass and cleanup](../.local/mcc-gameplay-continuation-20261020/strict-mcc-packaging-source-qualification-v1/focused-v3-terminal.json).
- [x] Prepare and review the owned normal/title launcher qualification source.
  It reuses the accepted authentic native fixture, adds all40 genuine imported
  font files and seven loading bitmap banks/thirteen frames, and produces the
  required loading manifest through the existing producer. A fresh strict
  capability is bound to the actual normal native proof; existing admission
  and selection readers must pass before either real Tk Play click. Original
  trace parsing rejects external imported-cache/reference consumption. The
  original Session remains intact; its owned projection changes only the frame
  retention limit and removes its HOME assignment, retaining XDG isolation.
  Attempted and completed route counts remain distinct. Root reviewed the final
  preparer/coordinator/checks and authenticated their recipe and V2 bindings.
  This is source readiness only; actual Tk execution and delivery remain open.
  [Frozen launcher qualification source](../.local/mcc-gameplay-continuation-20261020/native-strict-mcc-tk-play-source-checkpoint-v2.json).
- [x] Preserve the first actual launcher qualification frontier. Both real
  launcher-info calls identify the owned root and exact V2 runtime. Fresh
  strict admission passes with90 resources/all40 fonts and genuine seven-bank,
  thirteen-frame loading metadata. Normal Tk Play starts that runtime against
  the owned maps, reads the newer Bomb Defusal JSON and exits1 with
  `Invalid bundled Slayer variant schema` before title readiness. One route is
  attempted, none completed; title is unattempted. The source audit accounts
  for2319 successful imported opens/387 paths without external cache/reference
  consumption. All five retained diagnostics/roundtrips are authenticated
  (123372 bytes); scratch, XDG and processes are removed. Source inputs and real
  selections remain unchanged. Current producer/data includes the separate
  Assault definition, while the selected provider scans all JSON as Slayer.
  Resolve that specific data/producer compatibility boundary; do not skip
  malformed definitions or remove the unrelated newer mode's source work.
  [Original Tk failure and cleanup](../.local/mcc-gameplay-continuation-20261020/native-strict-mcc-tk-play-terminal-v1.json).
- [x] Resolve the variant compatibility source owner. The authenticated retained
  loader checks Linux executable-adjacent `data/tags/variants` before cwd and
  compile-time fallback. Its selected archive member exposes the Slayer-only
  API/schema family; current newer source/producer also supports Assault.
  The two qualified Fiesta/Infection definitions are byte-identical in both
  roots (1183 bytes total). A new runtime-adjacent bank can therefore reuse the
  existing strict loader without skipping invalid JSON, rebuilding unrelated
  gameplay owners or changing the root three-variant/Bomb Defusal work. No old
  T bank receipt exists; do not invent one. Actual adjacent-bank startup remains
  unverified until the successor launcher run. Packaging must explicitly carry
  the bank beside its relocated executable; the current assembler omits it.
  [Exact provider/source/path-order and data diagnosis](../.local/mcc-gameplay-continuation-20261020/strict-native-variant-compatibility-readonly-v1.json).
- [x] Review the launcher successor's minimal compatibility change. Its only
  fixture delta adds the exact1183-byte two-file bank beside the copied native
  runtime, validates the original Slayer schemas/IDs/rulesets and preserves the
  root Bomb Defusal definition. Coordinator changes select new recipe/output
  names only; engine, admission, input, source-audit and cleanup checks remain
  unchanged. Root authenticated the final recipe/source rows and read both
  complete patches. Original failure/source evidence stays immutable. This
  authorizes one scoped successor run, without granting launcher acceptance.
  [Two-file successor source](../.local/mcc-gameplay-continuation-20261020/native-strict-mcc-tk-play-successor-source-checkpoint-v1.json).
- [x] Verify the adjacent-bank startup fix and preserve the next launcher frontier.
  The actual native trace opens the two qualified adjacent Slayer files and no
  root Bomb Defusal definition. Normal Tk Play reaches title and profile screens
  10/167/38, then main menu6 with retail music. The old helper's Settings mouse
  target(319,357) opens screen30 rather than expected19 and times out. Wrapper1,
  one attempted route, zero complete routes; title is unattempted. Native exits0
  during owned timeout cleanup, which does not grant route acceptance. The
  source audit covers13080 imported opens/387 paths without external borrowed
  cache/reference reads. All five retained diagnostics/roundtrips are verified
  (166824 bytes); scratch/XDG/processes are removed and source/shader/selection
  inputs remain intact. No PCM or UI-engine correctness grant follows. Preserve
  this second failure and establish the exact selected hit region before fixing
  the test input; do not guess coordinates or change the UI to fit the helper.
  [Adjacent-bank acceptance and Settings frontier](../.local/mcc-gameplay-continuation-20261020/native-strict-mcc-tk-play-terminal-v2.json).
- [x] Diagnose the Settings test input against the exact selected UI owners.
  The helper transform and native inverse match. The selected list has28-pixel
  rows: the old point hits entry2/screen30 exactly, while Settings entry4 is
  x80..560/y389..417 with center(320,403). This matches the actual wrong-screen
  result and establishes a stale test coordinate, without a UI-engine change.
  The old left-footer Back point is not certified by the outer container bounds;
  actual hit regions use glyph ink. The existing right-button handler supplies
  a genuine mouse Back action independently of pointer position. A successor
  may test that explicitly named action, retaining left-footer ink/hit-target
  validation as a separate open gate rather than claiming it passed.
  [Selected layout/input diagnosis](../.local/mcc-gameplay-continuation-20261020/strict-mcc-tk-settings-target-readonly-v1.json).
- [ ] Validate actual left-click Back prompt ink/hit targets with imported MCC
  font metrics and rendered geometry across the required viewports. A valid
  right-click or keyboard Back route does not complete this acceptance.
- [x] Carry explicit runtime-adjacent variant ownership through the production
  packager. A qualified declaration binds the exact two relative payloads;
  direct-directory/file, hash, missing/extra owner and destination checks precede
  copying beside the packaged executable. Package inventory and SHA256SUMS
  include the bank, and the source is rechecked before publication. The newer
  root three-definition support data stays intact. README wording now states
  imported fonts and included artwork without granting per-player artwork
  switching. Seven new metadata ownership regressions plus the existing
  release_movie_bundling group pass through the official disposable runner.
  All15 scoped source pins remain exact; scratch/build/generated outputs are
  removed. No native or actual portable-package acceptance follows.
  [Applied package change and focused qualification](../.local/mcc-gameplay-continuation-20261020/native-strict-f51-v26-delivery-v1/package-focused-terminal-v1.json).
- [x] Review the source-derived launcher-input successor. Its owned Session
  projection uses Settings center(320,403), records real XTest button3 down/up
  for normal mouse Back, and keeps Escape for the title route. Expected new
  screen6 and exact input records are required. Original helper, engine, fixture,
  shaders and existing audits remain unchanged. The only existing production
  source refresh is the explicitly reviewed/tested package module. Root read
  the complete delta and authenticated source/recipe/proof bindings. This
  permits one scoped successor run; left-footer glyph-hit and actual full
  normal/title route acceptance remain open until their own results.
  [Reviewed input successor](../.local/mcc-gameplay-continuation-20261020/native-strict-mcc-tk-play-successor-source-checkpoint-v2.json).
- [x] Preserve the encoded Settings-ID frontier from the next real launcher run.
  The corrected point reaches actual screen65555 after title/profile/main-menu
  navigation; the legacy helper still expects19 and times out. One normal route
  is attempted, none completed; mouse3 Back, title and PCM completion are not
  reached. Native exits0 during owned timeout cleanup; wrapper exits1. The
  trace proves the exact adjacent bank and13080 imported opens/387 owners
  without external borrowing. All five retained diagnostics/roundtrips are
  authenticated (166872 bytes); scratch, XDG and processes are removed, with
  runtime/cohort/build, shaders, real selections and source inputs preserved.
  Source inspection now identifies the missing distinction: Settings logical19
  is dispatched through the existing PC menu builder and logged with its encoded
  frame ID. Correct that specific expected ID from the complete source route;
  do not accept arbitrary screens or change the engine to match the old helper.
  [Actual encoded-screen frontier and cleanup](../.local/mcc-gameplay-continuation-20261020/native-strict-mcc-tk-play-terminal-v3.json).
- [x] Complete the Settings identity source diagnosis. The earlier hit-region
  report correctly identified the requested logical destination but omitted the
  selected PC builder's final translation. The exact selected header defines
  `vista_menu(id)=0x10000|id`; MenuState dispatches logical19 through build_pc,
  which retains encoded65555 in the actual frame and diagnostic log. This
  matches the retained run. The fresh Settings frame's generic Back pops to
  main6. Existing MCC-owned definitions supply this PC widget family; no
  Windows game executable or reference asset is involved. Keep the verified
  point(320,403), correct only its expected logged ID to65555, and retain the
  existing real-input and separate left-footer gates.
  [Complete encoded Settings/Back source chain](../.local/mcc-gameplay-continuation-20261020/strict-mcc-tk-settings-screen-identity-readonly-v2.json).
- [x] Prepare and review the additive SSD cache upgrade source. It stages genuine
  V3 artwork/audio, requires exact source/PCM/mask lineage and derives the actual
  43-map/39-library owner index through production APIs. Locked beforeimage
  comparisons, provenance-last publication and guarded rollback preserve
  foreign successors and historical banks. Original provenance/registry bytes
  remain rollback evidence; all readiness fields stay unchanged. Owned XDG,
  existing lock identity and temporary-storage cleanup are explicit. Root read
  the complete helper and authenticated its final source/recipe/checkpoint.
  No installed write or execution has occurred; actual launcher compatibility
  remains a prerequisite for running this upgrade.
  [Reviewed installed-cache upgrade source](../.local/mcc-gameplay-continuation-20261020/strict-mcc-installed-cache-upgrade-v1/source-checkpoint-v1.json).
- [ ] Carry explicit prepared root and declared shared ownership into nested
  preview Cache calls using a qualified launcher/runtime feature. Preserve
  stock/F50 argv and existing option-structure layouts; reject conflicting,
  external or stale owners. Actual nested-map consumption and later gameplay
  caller integration remain separate from metadata fixture acceptance.
- [x] Preserve the first adapter integration build failure without hiding it.
  Configure exits0, the small adapter target exits2 and no CTests execute.
  Its local absolute(path) helper collides through argument-dependent lookup
  with std::filesystem::absolute; later errors cascade from that declaration.
  All28 scoped source pins remain equal and the entire private build is removed.
  Rename only the local helper and its three calls before the successor run;
  do not change path validation or expected ownership results.
  [First adapter build frontier](../.local/mcc-gameplay-continuation-20261020/map-owner-cache-integration-focused-v1-terminal.json).
- [x] Review the exact local-helper correction before a successor build.
  Only its definition and three calls become exact_absolute; the qualified
  std::filesystem implementation and all ownership/path/scalar expectations
  stay unchanged. Preserve the first failure and execute the four focused
  groups in a separate result directory.
  [Reviewed ADL correction](../.local/mcc-gameplay-continuation-20261020/mcc-prepared-map-owner-source-checkpoint-v2.json).
- [x] Preserve and revalidate the independent configure-time interruption.
  The successor reaches no compiler/tests because another current CMake target
  references bomb_defusal_tests.cpp before that concurrent file exists. All28
  scoped sources stay equal and its scratch is removed. Root then confirms the
  file now exists in the working tree; no unrelated target/file is edited or
  skipped. That observed state change permits one fresh focused attempt with
  the same reviewed owner sources, preserving both earlier failures.
  [Actual configure failure](../.local/mcc-gameplay-continuation-20261020/map-owner-cache-integration-focused-v2-terminal.json).
- [x] Preserve the actual third integration result and its remaining RED.
  Configure/build pass. Prepared-owner admission, onboarding and four actual
  production Cache/library translation units pass, including both constructor
  symbols, both private initializer pairs and unchanged current Cache layout.
  Gameplay preparation runs24 cases with23 passes and one existing SIGTERM
  timeout after presentation helpers. All29 scoped source pins stay equal;
  owned build, objects and child fixtures are removed. Do not increase the
  timeout, hide the failure or grant Cache/game/runtime acceptance from these
  component results. Diagnose the cancellation boundary before a successor run.
  [Three passing groups and cancellation failure](../.local/mcc-gameplay-continuation-20261020/map-owner-cache-integration-focused-v3-terminal.json).
- [x] Isolate a deterministic lost-wakeup model at the cancellation fixture.
  The original isolated case passes once, so that alone is not resolution.
  A child-only libc-entry gate then delivers the parent's real SIGTERM after
  Python releases its GIL and before pause starts waiting: the installed
  cancellation handler and empty signal mask remain correct, but the child
  stays in the fixture barrier until its original10-second timeout. A watchdog
  stack shows pause, not importer cleanup. The original failure's precise
  timing is unavailable; no CPython or production-handler defect is claimed.
  All four scoped source hashes stay exact and the compiled shim/scratch are
  removed. Replace only the fixture's indefinite wait with a finite wait,
  retaining real signals, stage cleanup and the original timeout/assertions.
  [Controlled stuck-barrier evidence](../.local/mcc-gameplay-continuation-20261020/mcc-sigterm-pause-entry-controlled-v2.json).
- [x] Qualify the narrow finite-wait fixture correction against original tests.
  Only import time and the barrier's10ms sleep/comment change. The original
  cancellation case passes1/1, then gameplay preparation passes24/24 through
  owned run_test invocations. Real SIGTERM, exit143, handler restoration,
  cleanup/retry assertions and the10-second timeout remain unchanged; three
  production helper hashes stay exact. Both private scratch roots are removed.
  This resolves the fixture's indefinite-wait vulnerability; the original
  uninstrumented failure's precise timing remains unknown. Combined source
  integration still requires its separately coordinated focused CMake run.
  [Reviewed fixture-only patch](../.local/mcc-gameplay-continuation-20261020/mcc-sigterm-finite-wait-source-checkpoint-v1.json)
  · [Original isolated case](../.local/mcc-gameplay-continuation-20261020/mcc-sigterm-finite-wait-isolated-v1.log)
  · [Full preparation group](../.local/mcc-gameplay-continuation-20261020/mcc-sigterm-finite-wait-full-v1.log).
- [x] Preserve the combined seven-group result before correcting a stale test.
  Configure/build pass and six CTests pass: onboarding, preparation, all four
  production translation units, native closure onboarding, twelve combined
  repair cases and prepared-owner admission. The sole failing group is the
  publisher's concurrent-destination negative: real EEXIST becomes FormatError
  with "existing destination retained", while its old assertion still expects
  "existing index retained". Its later preservation assertions are not reached.
  Keep the RED; update only that expected wording and rerun the affected
  publisher group. All34 scoped implementation/test pins stay exact; separate
  concurrent CMake drift is recorded. Private build/objects are removed.
  No actual Cache/game/runtime or SSD filesystem acceptance is inferred.
  [Actual six passes and stale-message failure](../.local/mcc-gameplay-continuation-20261020/map-owner-cache-integration-focused-v4-terminal.json).
- [x] Correct only the stale expected error wording and pass the affected group.
  Exact inverse comparison proves a single assertion substring changed from
  "existing index retained" to "existing destination retained". All twelve
  publisher cases then pass through run_test; the real EEXIST/FormatError,
  unchanged target/provenance and cleanup assertions execute successfully.
  Six scoped source pins remain exact and its private scratch is removed.
  The earlier combined CTest exit8 stays preserved; its other six groups
  already passed against the same production implementations. No repeat
  native/C++ build or actual private MCC cache mutation is required here.
  [One-word assertion correction](../.local/mcc-gameplay-continuation-20261020/mcc-owner-publication-message-fix-source-v1.json)
  · [Twelve passing publisher cases and cleanup](../.local/mcc-gameplay-continuation-20261020/mcc-owner-publication-message-fix-validation-v1.json).
- [x] Audit the actual F49 particle-boundary diagnostics against existing source
  systems. Selected source already has separate FP/WORLD camera residency;
  its caller's single catch can skip WORLD preloading after FP failure, while
  current source has independent catches. Current source also contains a
  tightly gated constant-distortion sampler projection absent from the selected
  provider's diagnostic anchors. Neither observation identifies the actual
  failed SMG template or which environment/camera/flag operand triggered its
  legacy guard. Preserve that attribution boundary; no shader replacement,
  original-flag guess or guard bypass is justified. Inspection scratch and
  extracted provider copies are removed; no code/game behavior changed.
  [Selected particle-owner frontier](../.local/mcc-gameplay-continuation-20261020/f49-particle-selected-owner-audit-v1.json).
- [ ] Bind the failed material's actual converted metadata to the existing
  constant-distortion contract, then qualify only demonstrated compatible
  producer/caller changes and their real native consumers.
- [x] Prepare and review a bounded selected-Cache material witness for the
  actual F49 SMG effect. It reads the full salted effect/system/particle owners,
  actual guard operands, primary-trigger location and template/pass/animation
  metadata before the same layer15/pass0/implementation0 residency call. The
  planned three compiles rebuild only the two removed qualified C57 components
  and the inspector; the proven Cache-only thin link keeps the old public ABI
  and selected particle provider. Genuine matching LOD failure remains RED;
  divergence/partial evidence stays explicit. No source fix, compile, asset
  read, World/game/replay or particle lifecycle acceptance occurs here.
  [Material witness source and exact plan](../.local/mcc-gameplay-continuation-20261020/particle-owner-material-inspection-v1/source-and-execution-plan-v2.json).
- [x] Review the concrete Cache-only witness coordinator before execution.
  Its pinned recipe keeps the three declared compiles and one proven thin
  link, exact old-public/staged-source dependencies and the selected particle
  archive member. API/ELF/map gates precede the single actual asset call;
  native exit/output are saved before parsing and material exit3 stays RED.
  Exact read-only file classifications and all prehashed/actually opened
  dependency pins are checked again in finally, with owned compiler/cache/raw
  outputs removed by the outer wrapper. Independent source review finds no
  material blocker. Actual compilation and material attribution remain next.
  [Reviewed coordinator recipe](../.local/mcc-gameplay-continuation-20261020/particle-owner-material-inspection-v1/coordinator-recipe-v1.json).
- [x] Preserve the actual first material-witness preparation failure. All
  three compiles and the thin link exit0, with the exact selected particle
  member authenticated. The following map-line binding check cannot prove
  the first importer API, so no Cache/material/asset call runs. This does not
  establish a bad provider: the prior proven reader accepted mangled or
  demangled map labels, while this coordinator searches only the mangled form.
  All1,422 protected hashes and12 delivery identities remain exact; generated
  ELF/objects/map/dependencies and owned scratch/cache are removed. Preserve
  this RED, repair only the evidence parser/diagnostics and requalify the same
  immutable source/provider plan before retrying the single actual witness.
  [Preparation failure and cleanup](../.local/mcc-gameplay-continuation-20261020/particle-owner-material-inspection-v1/material-owner-experiment-v1.json).
- [x] Review the exact immutable coordinator V2 repair. It restores the
  prior proven mangled-or-demangled label handling with the pinned c++filt,
  records candidate ELF/map hashes before gates and preserves bounded observed
  labels/providers for all required APIs before assertions. C++ sources,
  providers, compile/link/asset arrays and preservation gates stay unchanged;
  the original RED is pinned. No Cache/material result is inferred. Execute
  this same witness once after the parser repair, retaining its genuine result.
  [Parser-only repair diff](../.local/mcc-gameplay-continuation-20261020/particle-owner-material-inspection-v1/coordinator-v1-to-v2.patch)
  · [Exact V2 recipe](../.local/mcc-gameplay-continuation-20261020/particle-owner-material-inspection-v1/coordinator-recipe-v2.json).
- [x] Preserve the V2 actual Cache frontier and correct an unsupported
  association. Compile/link and all five exact API bindings pass; the sole
  actual Cache call finds named SMG fire_bullet at full handleF0235116, not
  F4105503. F4105503 belongs to the separate firing-particle log context; its
  earlier association with the named warning was an inference and is now
  rejected. The identity assertion stops before any system/material read.
  All552 opens are classified,527 actual asset pins and522 prehashed inputs
  remain exact, and owned scratch/cache/generated outputs are removed. Next
  bind the two actual log contexts independently through exact Cache lookups;
  no substituted expected handle, shader change or particle fix is justified.
  [Actual identity correction and preserved RED](../.local/mcc-gameplay-continuation-20261020/particle-owner-material-inspection-v1/material-owner-experiment-v2.json).
- [x] Review the exact V3 diagnostic correction before execution. Named SMG
  is resolved by unique name and complete salted/group identity; the separate
  observed firing handle is looked up independently. Both actual identities
  are recorded before fallible metadata reads. Per-effect/event/system/particle/
  template rows alone attribute the existing material error, with shared calls
  cached by complete particle datum. Providers, flags, shaders and the bounded
  compile/link/input closure stay unchanged. Execute this witness once and
  retain its actual RED or first divergence; no product fix is inferred yet.
  [Exact V3 recipe](../.local/mcc-gameplay-continuation-20261020/particle-owner-material-inspection-v1/coordinator-recipe-v3.json)
  · [Reviewed source delta](../.local/mcc-gameplay-continuation-20261020/particle-owner-material-inspection-v1/inspector-v2-to-v3.patch).
- [x] Preserve V3's genuine delivery-identity preflight stop. Its unchanged
  guard independently catches the SSD output's changed inode/ctime before
  any compile/link/Cache/material call;1236 reached protected hashes remain
  exact. Owned scratch/cache are removed. This supplies no new material
  result and is not a particle failure. Reuse the separate four-owner content
  renewal before a reviewed successor; do not relax the preservation guard.
  [Actual V3 preflight and cleanup](../.local/mcc-gameplay-continuation-20261020/particle-owner-material-inspection-v1/material-owner-experiment-v3.json).
- [x] Review V4's test ownership boundary. Delivered ZIPs, independently
  selected Play runtimes and renderer shader inventories are not inputs to
  this CPU Cache/material diagnostic; observe their metadata and concurrent
  publication without making them test prerequisites. Every actual frozen
  source/provider/tool/dependency and prehashed/opened MCC guard stays strict.
  Inspector-V3 and all compile/link/asset commands remain exact. Earlier RED
  results and four-payload content proof stay immutable; no runtime admission
  or shader-family substitution is inferred. Execute the unchanged witness once.
  [Reviewed V4 delta](../.local/mcc-gameplay-continuation-20261020/particle-owner-material-inspection-v1/coordinator-v3-to-v4.patch)
  · [Exact V4 recipe](../.local/mcc-gameplay-continuation-20261020/particle-owner-material-inspection-v1/coordinator-recipe-v4.json).
- [x] Attribute the actual selected material RED through native MCC Cache.
  Three compilations/link and exact API gates pass; native returns3 with
  complete independent metadata for eight systems/seven unique materials.
  Named SMG F0235116 has camera1 as its sole static-guard operand and all
  four system materials return. Separate F4105503 is Scarab beam fire;
  event0/system2 distortion_ripple alone reproduces the LOD error with the
  retained native distortion template/pass, empty D3D animation shape and
  authored finite distortion_scale5. All527 opened asset pins,522 prehashes
  and1225 actual input pins remain exact; scratch/cache are removed. This
  establishes the owner, not a rendering or lifecycle pass. Reuse only the
  existing guarded constant projection with the selected ABI after review.
  [Actual material attribution and cleanup](../.local/mcc-gameplay-continuation-20261020/particle-owner-material-inspection-v1/material-owner-experiment-v4.json).
- [x] Review reuse of the existing constant-distortion implementation.
  The contemporaneously recorded old material source and retained/Git copies
  agree; current source differs only by two includes and the completed guarded
  projection. Its original sampler producer already exists in the selected
  archive. Reuse the old public ABI and require unchanged-source emitted
  equivalence before one material-provider override. Original complete18540
  evidence establishes ordered writes22=5,23=0,25=0,25=5 and untouched SDK24;
  actual selected positive/unsupported-call controls and later pixels remain
  separate. No compile/provider adoption occurs in this reviewed plan.
  [Selected-compatible reuse plan](../.local/mcc-gameplay-continuation-20261020/particle-constant-distortion-reuse-v1/source-and-substitution-plan-v1.json).
- [x] Establish unchanged material-provider equivalence before substitution.
  One baseline compile exactly matches the selected b0c9 provider's complete
  emitted contract:23 allocated sections,605 relocations,111 linkage symbols,
  COMDAT groups and exception/unwind bytes. Identical-object acceptance and
  real allocated-byte/relocation mutations validate the comparator. All1231
  protected inputs and301 actual dependencies remain exact; owned scratch/
  cache are removed and no object is retained. No changed material, Cache or
  asset execution occurs. Prepare only the existing guarded branch and actual
  sampler-write readbacks for the next bounded component check.
  [Material baseline equivalence](../.local/mcc-gameplay-continuation-20261020/particle-constant-distortion-reuse-v1/material-baseline-equivalence-v1.json).
- [x] Review the phase-two material component's source-only preparation.
  Its inspector records ordered sampler writes, deferred external23 textures,
  full public snapshots, three excluded call tuples and existing producer
  rejection/atomic-bank controls. The baseline and projected thin closures
  retain the same20 objects/nine libraries; only the exact existing material
  source provider is inserted for the projected case. Old material extraction
  and World/Simulation definitions must be absent. No compile, link or Cache
  execution occurs yet. Check repeated-open audit cost before sealing the
  execution recipe; actual material success and pixels remain open.
  [Phase-two source checkpoint](../.local/mcc-gameplay-continuation-20261020/particle-constant-distortion-reuse-v1/phase2-source-checkpoint-v1.json).
- [x] Review the component coordinator's finite audit correction before its
  first execution. Existing V4 evidence contains22 redundant whole-file hashes
  totaling7.37GB. Every successful open still passes exact path/category/flags
  and deleted-descriptor checks; content reuse requires unchanged device, inode,
  size, mtime and ctime on each row. Every unique prehashed/observed regular
  input is rehashed finally. Complete native output and original exits are
  preserved before parsing/auditing. The inspector, material source, four
  compiles, two thin links and two Cache invocations remain exact. Execute this
  bounded component recipe once; retain its actual first frontier and remove
  owned scratch. No native client, World, shader or delivery change is granted.
  [Reviewed phase-two V2 checkpoint](../.local/mcc-gameplay-continuation-20261020/particle-constant-distortion-reuse-v1/phase2-source-checkpoint-v2.json).
- [x] Qualify the existing guarded projection in the selected Cache component.
  Four compilations and both thin links exit0; unchanged baseline preserves
  its genuine material RED3 and the projected provider exits0. All seven
  authored materials return: only distortion_ripple changes outcome, with
  actual ordered stage1 writes22=5,23=0,25=0,25=5 and deferred external23.
  All six previous material public snapshots and authored/context readbacks
  remain equal; three excluded call tuples reject identically. Nine producer
  shape controls and three prior-bank/atomic-write controls pass. All1583
  hard pins,527 opened MCC pins and522 prehashes remain exact;1132 successful
  opens are fully classified. The3,073,924-byte work report is retained
  losslessly in96,240 compressed bytes; owned scratch/cache and generated
  objects/ELFs are removed. This closes the material component only: named
  SMG's camera guard, gameplay rendering/lifecycle, client integration and
  installation remain unqualified. Prepare a selected-compatible native
  consumer/build plan before any runtime adoption.
  [Actual component comparison and cleanup](../.local/mcc-gameplay-continuation-20261020/particle-constant-distortion-reuse-v1/material-reuse-experiment-v2.json).
- [x] Review the selected-compatible native consumer adoption plan.
  Reconstruct only the two removed C57 importer/helper objects and compile
  the qualified guarded material source against the original public ABI.
  Reversing those substitutions and one material insertion recovers the
  exact C57 command; all355 persistent inputs and existing shaders stay exact.
  Require the old material member absent from the entire map, actual sole
  provider bindings and genuine native consumer/frame/audio acceptance before
  delivery. Both repository output/ZIP destinations are required after
  qualification; independent current root selection and SSD F49/v24 stay
  preserved. No new runtime/build or installation occurs in this plan.
  [Native material adoption plan](../.local/mcc-gameplay-continuation-20261020/particle-constant-distortion-reuse-v1/native-C57-material-adoption-plan-v1.json).
- [x] Review the concrete native successor build coordinator and recipe.
  Only three qualified source units compile, retaining the exact old public
  ABI,355 persistent inputs and97 existing shader files. Command reversal,
  dependency hashes, sole function-provider bindings, generated LOADs and
  whole-map absence of the replaced archive members are mandatory. Publish
  only an isolated raw ELF/cohort/small source/rebuild recipe after all gates,
  using atomic no-replace publication; generated objects and raw maps remain
  disposable. Runtime acceptance is explicitly pending and neither working
  selector/install/output/ZIP changes. Execute this build recipe once; the
  later ordinary MCC frame/audio check and both-directory delivery remain open.
  [Reviewed native build checkpoint](../.local/mcc-gameplay-continuation-20261020/particle-constant-distortion-reuse-v1/native-material-client-source-checkpoint-v1.json).
- [x] Preserve the first native build's actual inventory-gate failure.
  Three compiles and the full link exit0, but the builder's assertion assumes
  every distinct physical input occurs once in argv. Actual command readback
  shows358 distinct owners and exactly one inherited duplicate:
  `/usr/lib/libcurl.so.4.8.0` occurs twice, as in C57. No runtime/cohort is
  published; all1994 protected pins,355 persistent inputs and97 shaders stay
  exact, and generated scratch/ELF/map are removed. Correct only this metadata
  assumption by preserving the parent's exact per-owner argv multiplicity;
  do not deduplicate, reorder or replace libraries or change game behavior.
  Review the corrected source before another build; original RED stays intact.
  [First native build frontier](../.local/mcc-gameplay-continuation-20261020/particle-constant-distortion-reuse-v1/native-material-client-build-v1.json).
- [x] Review the exact input-multiplicity correction before another build.
  After material insertion, the C57-derived command has359 occurrences for358
  distinct physical owners; libcurl's existing duplicate is preserved.
  V2 compares each argv occurrence count with the parent plus the one new
  material and requires those same counts in actual expected-path LOAD rows.
  Other implicit linker LOADs are reported without invented historical-map
  equality. Source units, compile/link flags, command reversal, providers,
  persistent inputs, shader files and publication gates stay exact. Execute
  this corrected build once, preserving V1's failure and all working installs.
  [Reviewed multiplicity checkpoint](../.local/mcc-gameplay-continuation-20261020/particle-constant-distortion-reuse-v1/native-material-client-source-checkpoint-v2.json).
- [x] Build the isolated selected-compatible material successor.
  Three compiles/link and all eight sole API/provider checks pass. Its358
  explicit physical owners/359 argv and expected LOAD occurrences preserve
  the parent's libcurl duplicate. All355 persistent inputs,97 shader files
  and1996 protected pins remain exact. Atomic publication retains only the
  107,353,016-byte raw runtime,908,318-byte cohort and small frozen source/
  recipe:108,292,854 bytes total under the isolated native-material-client-v1
  owner. All generated objects/map/dependencies and owned scratch/cache are
  removed. V1 RED remains recorded. No game/frame/audio runs, capability,
  selector, installed build output or ZIP changes occur yet; ordinary native
  acceptance and subsequent both-directory delivery remain required.
  [Native build-only receipt](../.local/mcc-gameplay-continuation-20261020/particle-constant-distortion-reuse-v1/native-material-client-build-v2.json).
- [x] Review the concrete F50/v25 delivery plan for both repositories.
  The new358-input build retains355 persistent owners and three removed
  generated objects. Reuse existing staged/installed/portable route mechanisms,
  exact shader-literal relocation and the immutable working v24 portable tool
  closure. Keep approved movie/prompt/loading-tone exceptions and all existing
  programs; do not replace concurrent strict-V3 importer work or reimport data.
  A new actual native frame/audio receipt is required before capability and
  installed Play qualification. Preserve independent root selection and SSD
  F49/v24 rollback; deliver designated outputs and v25 ZIP/checksums to both
  roots after qualification. Source preparation performs no copy or install.
  [Reviewed both-directory delivery plan](../.local/mcc-gameplay-continuation-20261020/native-material-f50-v25-delivery-source-plan-v1.json).
- [x] Review the ordinary native candidate capture and owned retention wrapper.
  Bind the exact raw/cohort/build hashes; reuse the existing actual title,
  profile, lobby, countdown, loading and Lockout input flow with unchanged
  shaders. Require an advancing frame, movement response, finite nonzero
  loading/gameplay audio and clean native exit. Imported-open rows retain
  read-only/identity checks with one content pin per unique file; no repeated
  large-file hashing. The wrapper preserves actual first output/exit, bounded
  complete compressed logs and the requested video/four PNGs before deleting
  scratch. It writes no selection, capability or cache data. Execute once and
  inspect the actual result; neither a rendered Scarab effect nor full parity
  follows from this ordinary Lockout route.
  [Reviewed native frame checkpoint](../.local/mcc-gameplay-continuation-20261020/native-material-ordinary-frame-source-checkpoint-v2.json).
- [x] Complete and inspect the actual material-successor native route.
  One real title/profile/lobby/loading/Lockout run exits0 after193 frames and
  281 ticks. The captured frame contains46288 colors and893348 changed pixels
  after movement; loading/gameplay PCM are finite and nonzero. The old sampler
  LOD/layer error is absent from the complete native log. The independent
  coordinate/lifecycle and camera/flag effect limits remain recorded and are
  not bypassed. All91049 successful imported regular opens are checked using
  873 unique pins, including342 first observed after the run with that timing
  limit explicit. Inputs, shader bank and selections remain intact. Native
  children stop, the owned Pulse sink is removed, and scratch/cache are gone.
  The requested video/four PNGs plus complete compressed diagnostics retain
  6801578 bytes; no raw PCM or banks remain. Root visually inspects actual
  loading and match frames. Full visual/effect/TU1.5 acceptance remains open;
  proceed with the qualified runtime-only installed route and both outputs.
  [Actual frame/audio evidence](../.local/mcc-gameplay-continuation-20261020/native-material-ordinary-frame-v1/result.json).
- [x] Seal native acceptance after scratch cleanup and retained-output checks.
  The final receipt binds exact runtime/cohort/build, actual exit0 and full
  retained result with removed scratch/Pulse sink. Root reviews the complete
  evidence and actual loading/match images before delivery. The F50 delivery
  helpers require this new sealed proof separately from build/component PASS;
  they reuse only the exact working target presentation/admission closure.
  The SSD selector changes only inside a compare-and-restore transaction;
  independent root selection is observed without writes or historical gates.
  Stage the relocated candidate and exercise its ordinary route once before
  installing it. Existing F49/v24 files stay preserved.
  [Sealed native acceptance](../.local/mcc-gameplay-continuation-20261020/native-material-ordinary-frame-v1.json)
  · [Reviewed concrete delivery helpers](../.local/mcc-gameplay-continuation-20261020/native-material-f50-v25-delivery-source-checkpoint-v1.json).
- [x] Stage and verify the relocated F50 runtime on the SSD.
  Only the authenticated64-byte read-only shader-directory literal changes;
  all97 program files remain identical. The actual staged menu/loading/Lockout
  run exits0 after91 frames/114 ticks with real movement/fire and finite
  nonzero audio. Its shader binding points at the separate F50 bank. The
  selector remains F49 and the staging scratch/cache are removed. Retain
  F50 as the required installed candidate; now verify both normal/title
  launchers inside the explicit rollback transaction before adopting it.
  [Actual relocated route](../.local/mcc-gameplay-continuation-20261020/f50-native-proof-v1.json).
- [x] Verify and adopt F50 through both real SSD Play launchers.
  The normal and title launchers receive actual Tk Play clicks, enter the
  imported menu/loading/Lockout route, accept movement/fire and exit0 with
  finite nonzero audio. The bounded capability binds this runtime, exact
  resources and new native/build proofs without promoting old test claims.
  T now selects F50; F49 runtime/capability and v24 remain rollback owners.
  The independent development selection is preserved. Installation scratch
  and cache are removed. Publish the qualified raw/relocated outputs to both
  designated folders, then validate the new portable bundle before copying it.
  [Actual installed normal/title routes](../.local/mcc-gameplay-continuation-20261020/f50-installation-v1.json).
- [x] Publish the qualified material-successor binaries to both designated
  Linux output folders. R carries the107353016-byte raw17169143 runtime;
  T carries its same-sized relocated4a2df913 image. Each build-output manifest
  records its exact source/hash. The new raw capability binds actual installed
  and native-frame proofs; no broader gameplay flags are promoted. Both
  selections are preserved by the publisher and its scratch/cache are removed.
  Package v25 from the immutable v24 closure, replacing only runtime, coupled
  capability and build manifest/checksums, then verify both extracted routes.
  [Both output folders and cleanup](../.local/mcc-gameplay-continuation-20261020/f50-build-output-terminal-v1.json).
- [x] Package and verify the runtime-only v25 archive inventory.
  The2724450168-byte ZIP is SHA256 d38cc9ba972f950b9aa046e3103ee1cc6bb77250f83ff7c9c78fd54eaf517f8f.
  Exactly three hashed entries change from v24: native image, coupled capability
  and build manifest; SHA256SUMS is regenerated. All4430 other entries, frozen
  tools and97 shaders remain exact. No private MCC cache is bundled. Packaging
  exits0 and removes its scratch/cache/intermediates. This retains the already
  qualified historical presentation family; it does not qualify the concurrent
  strict-MCC import policy. Execute both extracted portable Play routes next;
  SSD ZIP/checksum delivery remains pending that separate acceptance.
  [Sealed package inventory](../.local/mcc-gameplay-continuation-20261020/portable-v25-package-terminal-v1.json).
- [x] Verify both actual extracted v25 Play routes with the owned MCC clone.
  Both real Tk Play clicks execute bundled Python, traverse menu/loading/Lockout,
  accept movement/fire and exit0. Normal advances24 frames/34 ticks; title
  advances36 frames/45 ticks. Both finite PCM captures contain nonzero samples;
  four-voice loading ambience starts and stops at graphics join. All archive
  checksums/97 shaders and full private clone admission pass. Open audits find
  no forbidden assets or import writes. The883-file donor reconciliation writes
  only the disposable clone. Scratch, extraction, clone, raw PCM/logs and owned
  processes are removed. No full gameplay/visual parity grant follows.
  [Both extracted routes and cleanup](../.local/mcc-gameplay-continuation-20261020/portable-v25-validation-terminal-v1.json).
- [x] Deliver the qualified v25 ZIP/checksum to both repositories and verify
  the SSD launcher selection. Both2724450168-byte archives have actual complete
  SHA256 readback d38cc9ba972f950b9aa046e3103ee1cc6bb77250f83ff7c9c78fd54eaf517f8f;
  both118-byte checksum sidecars match. Both107353016-byte designated native
  outputs and their manifests are rechecked. Actual normal/title launcher-info
  commands exit0 and select SSD F50 with the prepared MCC frontend; full native
  gameplay readiness remains explicitly ungranted. Independent root performance
  selection and old F49/v24 owners remain exact. Both copy/info scratch/cache
  roots are removed. Explicit retained ZIPs total5448900336 bytes, designated
  binaries214706032 bytes, plus2924 bytes of sidecars/manifests.
  [Both-directory delivery and launcher seal](../.local/mcc-gameplay-continuation-20261020/v25-ssd-delivery-terminal-v1.json).
- [x] Audit the remaining effect warnings against the selected source owners.
  Separate FP/WORLD admission and draw owners already exist. SMG camera1 is
  correctly excluded from the CPU WORLD fallback; F4105503 is the independent
  Scarab beam effect. Neither warning proves a missing FP implementation. The
  selected ordinary logger omits FP draw counts unless capture options request
  them, so absent output does not establish zero draws. Existing observer APIs
  permit the next genuine firing/draw witness without ABI growth or guessed
  flags. No execution, shader change or whole-current-provider adoption occurs.
  [Read-only effect lifecycle audit](../.local/mcc-gameplay-continuation-20261020/native-material-effect-lifecycle-readonly-audit-v1.json).
- [ ] Correlate genuine SMG source instances and full salted system/location
  owners with nonempty FP submissions and an actual GPU frame during ordinary
  firing, using existing observers. Keep separate Scarab lifecycle guards and
  remaining effects/visual parity acceptance open.
- [x] Reproduce current-source presentation admission drift against the exact
  qualified installed F49 runtime/cache. The installed reader accepts it;
  the current development reader rejects the same owner with its mandatory
  MCC V3 profile error. Current setup also omits the approved movie/prompt
  producers and selects V3 loading. This is a compatibility boundary between
  separately owned presentation families, not evidence that F49 or its cache
  changed. Both selectors, readers and capability remain exact; the read-only
  seven-second probe runs no game and its owned scratch is removed. Preserve
  the newer V3/native-font work while restoring the selected working family's
  complete presentation import/admission contract; do not relabel old proofs.
  [Actual presentation admission RED](../.local/mcc-gameplay-continuation-20261020/frontend-presentation-admission-red-v1.json).
- [ ] Qualify selected-runtime presentation dispatch through fresh/reused
  imports and admission, including the approved movies, custom prompt artwork
  and four-voice tone for the working family. Preserve distinct strict proof
  requirements for newer media/font families and all copied MCC data.
- [x] Restore exact admission of the already-qualified authored-prompt family
  while preserving strict newer media/font proof requirements. A regression
  first reproduces the V3-only error; the correction then passes four focused
  CTest groups and the original actual F49/cache read-only loop. Every prompt,
  tone, font, map, native binary and shader remains hash-bound. Presence of
  either newer profile field still requires both complete newer profiles and
  rejects mixed/partial claims; no capability is relabelled or gameplay grant
  added. Source, cache and both selectors remain otherwise preserved. All
  three named probe/CMake scratch roots are removed; no game is repeated.
  This closes admission only; fresh/reused import and packaging dispatch remain
  the preceding open gate.
  [Actual corrected F49 admission](../.local/mcc-gameplay-continuation-20261020/frontend-presentation-admission-green-v1.json)
  · [Focused CMake groups](../.local/mcc-gameplay-continuation-20261020/presentation-families-cmake-v1/report.json)
  · [Independent profile-boundary review](../.local/mcc-gameplay-continuation-20261020/frontend-presentation-family-independent-review-v1.json).
- [x] Prepare and review selected-family import/package dispatch without
  changing newer presentation work. Share the existing cache-independent
  runtime/capability/shader owner checks; explicit invalid/partial owners fail
  before publication, and packaging follows its exact candidate runtime.
  Authored p53/F49 families use intact approved movie/prompt/V2 APIs while
  retaining each original proof scope. Explicit V3 keeps its own strict APIs;
  the no-capability V3 default remains unqualified and cannot close the full
  fresh-import/custom-artwork goal. Separate profile caches and resource-root,
  cancellation, tamper and source-preservation controls are specified. No
  product edits or actual import/package execution occur in this plan.
  [Presentation dispatch plan](../.local/mcc-gameplay-continuation-20261020/mcc-presentation-family-dispatch-plan-v1.json).
- [x] Implement selected-runtime presentation dispatch through the existing
  preparation, repair and package APIs. Malformed explicit selection rejects
  before source import; exact authored owners retain movies, custom prompts
  and V2 tone without new proof flags. Different active profiles preserve the
  old cache and require a separate transaction. Packaging follows its explicit
  runtime and requires the existing frontend-cache qualification for authored
  packages, preventing first setup from selecting an incompatible default.
  Ten focused CTest groups pass serially, including21 new owner/real tiny-file
  copy/repair/package-source controls. One preceding cancellation-test timeout
  is retained; the isolated case and final serial group pass, with no proven
  cause claimed. Concurrent renderer and macOS SDL source changes are preserved.
  Actual installed-owner readback remains next; these fixtures do not qualify
  a new real MCC import, runtime, ZIP or complete presentation family.
  [Implementation source pins](../.local/mcc-gameplay-continuation-20261020/presentation-dispatch-source-pins-v2.json)
  · [Focused serial validation](../.local/mcc-gameplay-continuation-20261020/presentation-dispatch-focused-v4/report.json).
- [x] Recheck the final source against the actual installed F49 owner and
  existing MCC cache. Dispatch selects authored-V2 and full current admission
  accepts the exact native executable,97 shaders, map, fonts, prompt/tone
  resources and unchanged capability. Both selectors and cache registry/
  provenance stay exact. No import, game or decoder runs; probe/CMake scratch
  are removed. This accepts the dispatch/admission compatibility change,
  while a fresh real converted cache and packaged successor remain separate.
  [Actual final-source F49 readback](../.local/mcc-gameplay-continuation-20261020/presentation-dispatch-actual-f49-v2.json).
- [x] Independently review the qualified frozen presentation dispatch source.
  No concrete defect is found in selected-owner, mixed-profile, resource-root,
  fresh/repair or package-boundary handling. A second package-only review
  agrees. These findings apply to exact pins-V2 only: subsequent concurrent
  edits replace authored-family dispatch and its tests with strict V3-only
  rejection. Those new bytes are preserved and are not covered by this proof.
  [Independent frozen-source review](../.local/mcc-gameplay-continuation-20261020/presentation-dispatch-independent-source-review-v1.json).
- [ ] Resolve the presentation-policy conflict with the user before publishing
  another importer/package. This chat explicitly approved bundled intro/credits,
  authored prompts and the original tone; the active ONLINE chat now removes
  those routes under its stricter MCC-source interpretation. The question is
  pending. Preserve both frozen evidence and current concurrent source; do not
  relabel old acceptance or overwrite the new root loading-performance runtime.
- [x] Refresh the selected V16 host-pointer boundary audit before widening or
  rebuilding anything else. Inspected World native cache/object/player/body
  APIs already use64-bit host addresses; no new host truncation is demonstrated.
  Logical tag/CPU addresses and handles stay32-bit. The1.5GiB logical cache
  budget and128MiB importer metadata cap are capacity policies, not native
  allocator-address caps. Concurrent voice/interpolation/sound/Memory page-index
  drift does not by itself require adoption for MCC-only/64-bit support. All
  inspected selected provider/source/ELF contracts remain pinned; both scratch
  paths are removed, with no build or product/runtime mutation. This bounded
  audit does not complete ownership or mod-capacity acceptance.
  [Selected host-pointer audit](../.local/mcc-host-pointer-frontier-20261019/v1/validation.json).
- [x] Test selected Memory native-address ownership and rebinding across
  restore using the exact V16 archives and compatible frozen104-byte Memory
  layout. All59 controls pass: four actual host addresses exceed4GiB, logical
  32-bit pointer/datum identities and complete four-region checkpoint bytes
  stay exact through restore/swap/rebind/remount. Read-only, uncommitted,
  foreign-owner and overflow checks pass. Stale borrowed pointers are never
  dereferenced; legitimate allocator reuse is accepted. Only a tiny probe is
  compiled, with original archive-member witnesses. Its2,469,368bytes of
  ELF/object/map/deps and scratch are removed. No bug/product change or current
  page-index/voice-feature adoption is required by this bounded result.
  [Selected Memory ownership acceptance](../.local/mcc-host-pointer-frontier-20261019/selected-memory-v1/validation.json).
- [x] Refresh the demonstrated by-value consumers of current checkpoint and scene
  layouts, then validate the coherent private client. Preserve current public fields;
  do not mix old and current object layouts. Main, campaign runtime, Simulation
  and multiplayer agree on 17,072 bytes. The current campaign-view, 288-byte
  AnimationClip, optional/containing presentation, model-pose and particle
  consumers are refreshed; reviewed sky/model/audio support remains compatible.
  Missing API providers are resolved in the linked 69-source V6 client plus
  the schema3 codec. The expanded FP/Content/Weapon selection checks and the
  88-byte nested ScriptVm thread/deque review pass. V7 combines all 71 owners
  with the V24 World/CRT/data bundle and passes ordinary Main at tick3;
  V8-FP also passes with the corrected model owner. Detailed receipts are above.
- [ ] Check compatibility before adopting any required subsequent receiver or
  sound-I/O API/body work. The pinned earlier receiver/World bodies establish
  the internally consistent validation cohort. Adopt a current group only for
  a demonstrated required path; preserve unrelated active feature work.
- [x] Inventory current source/header drift beyond the narrow V13 boundaries.
  All 314 coherent inputs and 44 narrow header entries still match. Of the
  1,191 frozen include files, 13 differ, including one comment-only change;
  nine changed sources from the 71-source ledger still supply selected objects.
  Static review identifies voice-frame/packet, interpolation, sound-DMA and
  voice-device virtual-interface risks. This inventory does not certify the
  current bodies; paired layout checks and the minimal integration set remain open.
  [Current compatibility inventory](../.local/mcc-current-compatibility-audit-20261018/v1/inventory.json).
- [x] Measure the demonstrated current/frozen public layouts with paired tiny
  probes. `GameVoiceFrame` grows 48→56 bytes, `MatchVoicePacket` 56→64,
  `ModelNodeInterpolation` 176→184 and `SourceRuntimeSoundDmaResources` 240→264.
  Pimpl owner sizes stay unchanged; additive methods alone do not establish
  a layout break. These measurements identify actual mixing risks and do not
  invalidate the internally consistent frozen V13 baseline. Minimal affected
  consumer/provider integration remains open.
  [Paired layout measurements](../.local/mcc-current-compatibility-audit-20261018/v1/layouts.json).
- [x] Check actual selected providers against current-only APIs and layouts.
  The coherent ELF lacks all 17 inspected new APIs while older positive
  controls are present. The selected multiplayer object confirms the frozen
  48/56/176-byte voice/packet/interpolation layouts. This establishes a current
  feature/body delivery gap; compatible rebuild groups remain required before
  adopting the new providers. The frozen test cohort remains internally consistent.
  [Selected provider and layout evidence](../.local/mcc-current-compatibility-audit-20261018/v1/selected-providers.json).
- [x] Seal the minimal current/frozen compatibility matrix. Six owners must
  refresh together for the demonstrated public-layout changes: Main,
  multiplayer runtime, game voice, online voice, online session and render
  interpolation. A separate 16-file handwritten-body set includes current
  World, whose eleven required sound providers are absent from the selected
  ELF. Five already-current provider snapshots can be reused. All 314 frozen
  inputs and 44 narrow pins remain exact; later additive header changes are
  separately pinned. This identifies the integration work and does not close
  the current-product compatibility gate. Tiny probes and audit scratch are removed.
  [Compatibility matrix](../.local/mcc-current-compatibility-audit-20261018/v1/matrix.json),
  [Audit cleanup](../.local/mcc-current-compatibility-audit-20261018/v1/cleanup.json).
- [x] Validate schema3 authority encoding/decoding and rejection of unsupported
  schemas and malformed archives: 25 focused checks pass; archive framing is unchanged.
  [Codec evidence](../.local/mcc-authority-codec-schema3-20261018/v1/validation.json).
- [ ] Verify real attachment admission, restoration and continued gameplay in
  the coherent client; the codec fixture alone does not establish those behaviors.
- [x] Promote the proved texture compatibility modules into current product
  source and verify current-header compilation and CMake registration.
- [x] Integrate authenticated texture-cache export/restore lifecycle handling in
  current product World; the promoted code passes the same real lifecycle suite.
- [x] Verify current-World cold same/fresh restore, cancellation, rejection, denied
  and successful swaps, fault recovery and stale-owner teardown. The corrected
  suite passes 12 groups/15 rejection controls and nine public model calls with
  484-byte collectors/33 nodes. It proves actual resource republication after
  genuine cold restores and poisoned/partial/rollback-denied resource failures;
  no original-reference files are opened. Broader gameplay/parity gates remain open.
  [Product promotion evidence](../.local/mcc-model-cpu-bitmap-20261018/product-promotion-v1/native/validation.json).
- [ ] Verify live controls, player lifecycle, weapons, vehicles, AI, scripts,
  objectives and relevant map transitions against TU1.5.
- [ ] Complete representative campaign and multiplayer playthrough acceptance.

## Graphics and audio

- [x] Preserve the existing 97 shader files in the current native rendering tests.
- [ ] Finish graphical corrections using Reformation as a reference and Xbox
  TU1.5 as authority, including relevant bitmap/bump-map inconsistencies.
- [x] Correct the Uprising investigation's malformed reference-note path.
  Both actual original and MCC maps contain the resident
  `scenarios\bitmaps\solo\deltacontrolroom\ocean_specmask`, plus the matching
  ocean floor/froth bitmap identities. Preserve exact descriptor/resource
  metadata and map hashes; both source maps remain unchanged and owned scratch
  is removed. The earlier missing separator cannot establish asset absence.
  This is offline identity evidence only; complete pixel/mip comparison and
  any required correction remain open.
  The corrected descriptor report treats MCC depth as byte+8, separately from
  byte+9 sampling/creation metadata; an initial raw-word label is preserved as
  a diagnostic rather than a claimed depth difference.
  [Actual Uprising identities](../.local/mcc-graphics-uprising-20261020/identity-v2/report.json).
- [x] Compare those three actual Uprising bitmaps through the coherent native
  Cache/bitmap loader, compiling only its inspector entry and reusing313 exact
  providers. All three returned compressed texture buffers and20 resident mips
  through4x4 match the original byte-for-byte. No ocean pixel correction is
  justified by these results. Raw2x2/1x1 tails, material sampling and rendered
  scene parity remain unverified. All eight compile/link/inspector commands
  exit0; preserve the helper's genuine later shader-path postflight error.
  Independent review resolves the97 relative shader names against their actual
  directory and verifies their hashes, all nine frozen headers and scratch
  removal. Original/MCC maps/resources and314 link inputs remain exact. No raw
  pixels, executable, object or map is retained; no gameplay/World test runs.
  Next inspect the actual water material/tag bindings and sampling parameters.
  [Actual native comparisons and preserved error](../.local/mcc-graphics-uprising-20261020/native-v1/report.json),
  [Independent comparison and postflight review](../.local/mcc-graphics-uprising-20261020/native-v1/independent-review.json).
- [x] Trace the actual Uprising ocean bitmap consumers in the installed retained
  material library: `sea`, `ocean_froth` and `dcr_water_ocean`. Fresh offline
  canonicalization from the authenticated original map matches all31 resident
  material/template/pass/vertex records, complete payload bytes, relocations,
  reference identities and schema hashes. No unresolved original shader owner
  is hidden and no existing program or material is changed. This identifies
  seven additional bitmap dependencies for comparison; their pixels and actual
  runtime sampling/scene parity remain open. Source map/library are unchanged;
  temporary canonical payloads and owned scratch are removed.
  [Original water material closure](../.local/mcc-graphics-uprising-20261020/material-v1/report.json).
- [x] Compare all seven additional bitmap dependencies of that water material
  graph through the same qualified native decoder. All26 actual frames,
  including17 water animation frames, and101 returned mips match byte-for-byte
  across1,655,960 bytes per source. No identity/decode boundary or differing
  byte is hidden. All314 inputs, nine frozen headers, original/MCC sources,
  installed material manifest and97 shaders remain exact; owned pixel/tool/
  object/map/cache scratch is removed. Together with the earlier three ocean
  bitmaps, these data support preserving the current textures and original
  material graph. Actual runtime relocation/sampling, other render state and
  a rendered Uprising comparison remain open; no artwork/program correction
  or gameplay acceptance is inferred.
  [Complete additional water bitmap matrix](../.local/mcc-graphics-uprising-20261020/native-v2/report.json).
- [x] Compare five additional locally resident bitmap pairs through the
  coherent native decoder, compiling only the inspector entry point and
  reusing 313 providers. Metropolis's `matte_grey_edge_dif`, `metal_slats_bump`
  and `metal_slats_dif` have matching descriptors but different complete
  seven/nine/seven-mip bytes. Its `ec_ocean_alpha` is already byte-identical;
  no guessed pixel replacement is admitted. Sacred Icon's gradient has changed
  dimensions/mip counts and needs a separate descriptor proof. All 314 inputs,
  source maps, resource banks and shaders remain unchanged; temporary tool,
  link map and pixels are removed. This is original retail-map source evidence,
  not captured TU1.5 visual parity or a complete shared-resource inventory.
  [Native bitmap comparison](../.local/mcc-graphics-restoration-audit-20261018/v1/validation.json),
  [Bounded audit handoff and cleanup](../.local/mcc-graphics-restoration-audit-20261018/v1/handoff.json).
- [x] Implement the separate V3 Metropolis delta bundle and compose it with
  immutable V2 Zanzibar corrections in ordinary MCC import. All five images
  and 40 mip chains pass exact native cache readback; the three Metropolis
  outputs also match freshly decoded original pixels byte-for-byte. The real
  MCC-only correction child and five native readbacks open no original
  references. Both maps preserve all decoded metadata except their five local
  resource pointer/length pairs; the original texture-bank prefix and every
  other map/resource remain unchanged. Twenty bitmap ownership/rejection,
  eighteen onboarding and sixteen preparation tests pass, including last-entry
  failures before any map/bank write. Only 150,118 bytes of required delta and
  manifest are retained; inspector, raw pixels, copied maps/bank and traces
  are removed. Installed Play, saves, version9 and all 97 shaders stay exact.
  The selected older cache and version9 ZIP still use the prior correction set;
  onboarding reuse/upgrade and a full fresh import are the next integration gate.
  [Actual V3 conversion and native readback](../.local/mcc-graphics-corrections-v3-20261018/v1/validation.json),
  [Verified product handoff and cleanup](../.local/mcc-graphics-corrections-v3-20261018/v1/handoff.json).
- [x] Route setup from older correction sets to a fresh private import, while
  preserving the prior selection/cache if that import fails. A complete current
  five-profile cache still reuses its verified copy. Normal graphics import now
  ignores the old development reference registry entirely and consumes only
  MCC bytes and the qualified deltas. Twenty-one bitmap and twenty-two
  onboarding tests pass; scratch is removed. The original forty-mip native
  readback remains valid for the unchanged delta bytes. Full fresh setup and
  launcher verification are still the next integration gate.
  [Graphics upgrade and legacy-reference boundary](../.local/mcc-graphics-corrections-v3-20261018/onboarding-v1/validation.json).
- [x] Execute a fresh real launcher import with the V3 correction set.
  Ordinary `--setup-only --mcc-source` copies and verifies 883 MCC files
  (31,251,040,809 source bytes), prepares all 43 maps and applies all five
  corrections/40 mips. Both launcher-info checks and the actual eight-frame
  launcher route pass on the new cache. The subsequent actual shot exits0
  at tick60 with ammo59 and 64 rendered frames; the integration harness then
  rejects its obsolete fixed expectation of 74 frames. That is a harness
  assumption, not a gameplay failure. Raw audio/capture scratch is removed;
  the prepared cache is explicitly retained for resume without reimporting.
  The previous asset selection is restored. Finish verification with a
  timing-independent frame assertion and audio capture after the user's
  interactive crash/startup priority; this does not complete full integration.
  [Actual setup, launcher and bounded shot evidence](../.local/mcc-graphics-corrections-v3-20261018/full-setup-v1/validation.json),
  [Actual new-cache firing log](../.local/mcc-graphics-corrections-v3-20261018/full-setup-v1/actual-new-cache-firing.log).
- [ ] Validate complete rendered scenes against authenticated TU1.5 captures.
- [x] Verify the reused MCC sound decoder and native SDL mixer produce genuine
  PCM on the pinned client providers. The actual SMG sound yields one 46,080-frame
  packet, nonzero output, completion and silence before/after; no original-reference
  opens and no provider changes. This manual-delivery fixture does not prove the
  live gameplay sound scheduler, physical audibility or video synchronization.
  [MCC PCM validation](../.local/mcc-native-audio-capture-audit-20261018/manual-v1/validation.json).
- [x] Verify genuine SDL device-callback capture through the existing disk
  backend. The real SMG output matches the manual mixer sample-for-sample
  after the production float32 WAV quantization; raw callback scratch was removed.
  [Device capture validation](../.local/mcc-native-audio-capture-audit-20261018/device-v1/validation.json).
- [x] Capture genuine nonzero PCM during ordinary unmuted MCC gameplay,
  without manually submitting a sound. V10 produces 74,752 stereo callback
  frames (1.557 seconds), peak 0.02663 and RMS 0.002109. The corrected looping
  loader no longer reports its previous allocation error. The requested WAV
  is preserved; source identification, timing, a successful longer run and
  synchronized video remain separate gates. The run fails later in rendering.
  [Live neutral audio result](../.local/mcc-native-client-20261018/client-current-v10-hud-mcc/audio-neutral-v1-real-mcc/validation.json),
  [Actual gameplay audio](../.local/mcc-native-client-20261018/client-current-v10-hud-mcc/audio-neutral-v1-real-mcc/native-gameplay-audio.wav).
- [x] Authenticate the later neutral-render failure's complete `2C390`
  constructor: 214 original bytes/56 instructions, no calls or branches, all
  generated instruction identities exact. All five immutable input tuples
  are reviewed; only `46A4BC` (four bytes) and `46A4C8` (eight bytes) lack
  backing. Nine complete original executions cover three precision controls
  and three destination sentinels. No constructor output is supplied as data.
  [Complete constructor authority](../.local/mcc-native-render-neutral-20261018/constructor2c390-v1/primary.json).
- [x] Supply the authenticated twelve constructor-input bytes through the
  ordinary data module. All nine complete native executions match Original
  GP/defined flags, ordered reads/stores, stack, instruction trace and all 128
  destination bytes; 25 negative controls pass. All 2,423 prior factory owners
  are exact, and the new native backings are above 4 GiB. Independent source
  and reconstructed-factory review passes.
  [Constructor product validation](../.local/mcc-native-render-neutral-20261018/product2c390-v1/validation.json),
  [Independent review](../.local/mcc-native-render-neutral-20261018/root-review-v1/review.json).
- [x] Qualify the constructor fixture's emulator status mismatch without
  changing engine arithmetic. The exact original floating-point instruction
  sequence on the physical host produces `FPSW=0220`, matching native, while
  Unicorn reports zero. All original stored float words still match. Preserve
  both initial failed checks and the frozen emulator report; compare status to
  the separate host instruction witness. This is not physical Xbox certification.
  [Host instruction qualification](../.local/mcc-native-render-neutral-20261018/host-x87-v1/validation.json),
  [Preserved failures and cleanup](../.local/mcc-native-render-neutral-20261018/product2c390-v1/handoff.json).
- [x] Pass the actual later-frame neutral rendering regression with the
  reviewed constructor inputs. Combined V12 exits successfully after 26 frames
  and 30 simulation ticks, with a real gameplay capture and 107,520 nonzero
  stereo callback frames (2.24 seconds). All 314 providers, 97 shaders and maps
  stay unchanged; the initial reference filename filter finds no opens.
  The prior missing-input failure is closed. This is a bounded runtime test,
  not complete visual, audio or gameplay acceptance.
  [Longer unmuted runtime](../.local/mcc-native-client-20261018/client-current-v12-neutral-mcc/audio-neutral-v1-real-mcc/validation.json).
- [x] Preserve a reproducible image check for the left-edge geometry symptom.
  The pinned V10 image has zero matching pixels in the declared region;
  V12 tick30 has 4,639. This checks the visible symptom only; different capture
  ticks do not identify its cause. Diagnostic scratch is removed. Next, join
  actual accepted draw spans to the same submitted camera and model identity.
  [Bounded symptom replay](../.local/mcc-left-edge-geometry-20261018/v1/capture-symptom.json).
- [x] Diagnose and correct the oversized left-edge world-weapon geometry.
  Actual accepted Needler geometry used normalized class1 MCC positions/UVs
  whose compression flags the CPU loader skipped. The bounded source fix
  below restores authored expansion. The actual model-only tick30 image
  removes the symptom; root inspected the lossless result. Whole-scene
  materials and full TU1.5 framebuffer parity remain separate gates.
  [Original symptom](../.local/mcc-native-client-20261018/client-current-v12-neutral-mcc/audio-neutral-v1-real-mcc/lockout-view.png),
  [Corrected image](../.local/mcc-left-edge-geometry-20261018/class1-fix-v1/capture/lockout-class1-fixed.png).
- [x] Reproduce the geometry symptom in one observer-only ordinary tick30 run.
  It retains the same 4,639 matching pixels and accepts a Needler draw whose
  verified model/vertex join projects into the affected region. Its accepted
  world bounds span about two units. All 314 V12 providers, maps and 97 shaders
  stay pinned, no reference files open, and generated binaries/captures/traces
  are removed. Projected overlap alone does not prove pixel ownership or the
  decoding cause; product geometry, clipping and visibility are unchanged.
  [Actual diagnostic run](../.local/mcc-left-edge-geometry-20261018/v1/actual-v1/validation.json),
  [Accepted draw identities](../.local/mcc-left-edge-geometry-20261018/v1/actual-v1/edge-geometry.json).
- [x] Inspect the actual paired MCC/Original world-weapon resource data.
  Needler's 1,992 and Energy Blade's 2,148 selected vertices preserve counterpart
  counts, section/part order, bounds and node maps. Their class1 MCC streams
  retain normalized FLOAT positions/UVs with compression flags3; the current
  CPU path skips that class's expansion. Applying authored compression only
  in a diagnostic copy reduces XYZ differences to 3.94e-6/9.24e-6 and UV
  differences to about 2.3e-5, within the observed Xbox quantization.
  Existing selected human-weapon controls remain unchanged. No product fix
  or new gameplay image is admitted by this census. The full-section gate
  below extends this evidence; the actual decoding correction is still pending.
  [Actual resource counterparts](../.local/mcc-left-edge-geometry-20261018/v1/intrinsics-v1/report.json),
  [Probe provenance and cleanup](../.local/mcc-left-edge-geometry-20261018/v1/intrinsics-v1/validation.json).
- [x] Complete all-section counterpart and Original class1 consumption review.
  All 22 sections and 18,219 expanded vertices retain counterpart counts,
  section/part order, bounds and node maps. Diagnostic flagged expansion
  reduces maximum differences to 9.24e-6 XYZ and 2.27e-5 UV. The authenticated
  original class1 blade program consumes `c170..172` for XYZ/UV before palette
  transforms. Independent review supports adding class1 to the existing MCC
  flag gates; class0, Xbox parsing, formats, normals, origins, matrices and
  shaders stay outside that change. Offline Original inspection is reference
  work, not a runtime independence test. Actual decoding and gameplay image
  acceptance remain next; no bit-identical or full-parity claim is made.
  [Full-section counterpart census](../.local/mcc-left-edge-geometry-20261018/v1/intrinsics-v2-summary.json),
  [Independent Original draw and pair review](../.local/mcc-left-edge-geometry-20261018/v1/intrinsics-v1/independent-review.json).
- [x] Apply the bounded class1 MCC decoding correction and verify all paired
  sections and unchanged controls. All 22 sections/18,219 expanded vertices
  pass the real Cache regression; Original reports and every unrelated vertex,
  skin, origin and bone field remain exact. Only the existing class gate and
  its comment change. The 3,312,952-byte model object uses the exact frozen V12
  dependencies, and compiler/link scratch is removed. Coordinate residuals
  remain qualified as quantization differences. Source promotion matches the
  tested candidate; installed runtimes and releases remain unchanged.
  [Class1 regression](../.local/mcc-left-edge-geometry-20261018/class1-fix-v1/controls/validation.json),
  [Guarded source promotion](../.local/mcc-left-edge-geometry-20261018/class1-fix-v1/promotion.json).
- [x] Inspect the model-object-only V12 tick30 capture after the class1 fix.
  Actual ordinary Main exits successfully after 31 frames/30 ticks; the
  left-edge discriminator drops from 4,639 pixels to zero. Root visually
  confirms the corrected edge and retained arms/SMG/HUD. All prior 314 inputs,
  maps and 97 shaders are pinned; generated ELF/audio/trace scratch is removed.
  Animation fractions vary with the ordinary clock, so this is a geometry
  regression rather than a whole-image or phase-parity comparison.
  [Causal native run](../.local/mcc-left-edge-geometry-20261018/class1-fix-v1/capture/validation.json),
  [Verified image and retention handoff](../.local/mcc-left-edge-geometry-20261018/class1-fix-v1/handoff.json).
- [x] Adopt the verified model fix and reviewed counter/response data into the
  coherent V13 client, validate its combined runtime, and update installed
  native testing plus compiled version8. Actual local and relocated launchers
  pass. Subsequent classifier/firing work and full readiness remain separate.
- [x] Link the coherent V13 candidate with only the reviewed class1 model and
  latest ordinary data object replacing V12 inputs. All 312 other inputs,
  43 MCC maps and 97 shaders stay pinned; source/header compatibility checks
  pass and link scratch is removed. The 104,675,632-byte runtime and
  20,465,553-byte provider map are retained for required current integration.
  The separate ordinary unmuted tick30 validation below passes; launcher and
  ZIP adoption remain separate from this link-only milestone.
  [Coherent V13 cohort](../.local/mcc-native-client-20261018/client-current-v13-class1-fire-mcc/cohort.json),
  [Provider and compatibility manifest](../.local/mcc-native-client-20261018/client-current-v13-class1-fire-mcc/link-manifest.json).
- [x] Validate ordinary unmuted V13 gameplay through tick30 after combining
  the class1 correction and reviewed firing data. It exits successfully after
  17 frames; root inspected the clean left edge and retained arms/SMG/HUD.
  Real callbacks produce 117,760 finite stereo frames, peak 0.05824 and RMS
  0.007582. The broader 108,120-open audit finds four observed MCC map paths
  and zero legacy asset references. All 314 providers, 43 maps and 97 shaders
  stay pinned; raw audio/trace/frame scratch is removed. This closes the
  combined neutral-runtime gate; installed/package adoption and full firing,
  gameplay and audiovisual parity remain open.
  [Combined V13 runtime](../.local/mcc-native-client-20261018/client-current-v13-class1-fire-mcc/run-v1-real-mcc/validation.json),
  [Full file audit](../.local/mcc-native-client-20261018/client-current-v13-class1-fire-mcc/run-v1-real-mcc/file-audit.json),
  [Retention handoff](../.local/mcc-native-client-20261018/client-current-v13-class1-fire-mcc/handoff.json).
- [ ] Identify weapon-specific callback audio and verify audible shot output
  beyond the now-passing bounded ordinary Main SMG shot. The successful tick60
  run and later deliveries below supersede the earlier termination frontier;
  finite nonzero callback data alone does not identify the shot sound.
  Historical progression: actual step3 fire originally failed at `F0D38`; the data-only rerun clears
  that exact boundary and reaches the separate `1694A2` read at `16957F`.
  The next table-only run clears that boundary too, then fails at `CC26A`
  reading `CC30C`. The reviewed hand tables clear that dependency; the fourth
  firing run now reaches `E999F` reading `E9BB8`. The fifth run clears those
  tables and reaches evaluator `17F8A0`, reading missing response row3 at
  `478EA8`. The sixth run clears row3 and reaches the separate `15F77A`
  read at `48C7A4`, under root `B9090`. The seventh clears the counter and
  reaches `19B8FE` reading classifier row8 at `19BA18`. The eighth clears
  that leaf and reaches `FFF44` reading scalar `41C4CC`, under root `B9180`.
  All eight isolated
  firing runs have silent device output and no
  captured shot. Those early failures are retained as history; the actual
  bounded firing gate passes in the later candidate-v5/V16/V18 records.
  [Actual firing run](../.local/mcc-native-client-20261018/client-current-v8-fp/audio-fire-v1-real-mcc/validation.json).
- [x] Authenticate and supply the demonstrated `F0BE0` dispatch data: one
  31-byte read-only owner containing four targets and fifteen selectors.
  All 2,420 prior factory owners are unchanged. Twenty-six Original/native
  prefix cases and 40 negative controls pass; independent direct-XBE review
  confirms the bytes, unsigned guard, exact read operands and ownership.
  Descendant calls and the actual firing/audio rerun remain separate.
  [Dispatch data validation](../.local/mcc-native-audio-capture-audit-20261018/fire-dispatch-product-v1/validation.json),
  [Independent primary review](../.local/mcc-native-audio-capture-audit-20261018/fire-dispatch-root-review-v1/review.json).
- [x] Clear the exact `F0D38` firing dependency in ordinary Main using only
  the reviewed 31-byte data owner. The next failure is the separate `16957F`
  table read; other providers and shaders are unchanged. No audible shot is
  claimed from this failed run.
  [Data-only runtime regression](../.local/mcc-native-client-20261018/client-current-v8-fire-data-v1/audio-fire-v2-real-mcc/validation.json).
- [x] Authenticate and supply the complete 164-byte `169493` read-only table.
  All 65,536 low-word selectors plus eight upper-word aliases match unchanged
  original execution; 173 negative controls pass and all 2,421 prior factory
  owners are preserved. Independent direct-XBE/source/factory review passes.
  A separate corrected illustrative report qualifies a mutable-list reporting
  defect; the complete Original/native row digest and frozen product proof are
  unchanged. Actual descendant firing/audio remains unchecked.
  [Table product validation](../.local/mcc-native-audio-capture-audit-20261018/fire-symbol-product-v1/validation.json),
  [Independent review](../.local/mcc-native-audio-capture-audit-20261018/fire-symbol-independent-review-v1/review.json),
  [Corrected illustrations](../.local/mcc-native-audio-capture-audit-20261018/fire-symbol-proof-v2/primary.json).
- [x] Clear the exact `16957F` dependency through ordinary Main using only the
  reviewed 164-byte table. The run advances to the separate `CC30C` dependency
  before failing after 98.02 seconds. All 312 providers, maps and 97 shaders
  stay pinned; zero reference opens. Its 82,432 callback frames are silent,
  so this closes only the prior dependency, not shot or audio acceptance.
  [Table-only runtime regression](../.local/mcc-native-client-20261018/client-current-v8-fire-data-v2/audio-fire-v3-real-mcc/validation.json).
- [x] Authenticate and supply both complete `CC1E0` hand-selection tables in
  one 64-byte read-only owner. All 51 Original/native prefix cases and 73
  negative controls pass; all 2,422 prior factory owners remain exact. Root's
  independent direct-XBE review verifies the entire 297-byte/88-instruction
  owner, both unsigned selector guards, all generated instruction identities,
  fresh Original witnesses and the reconstructed full factory digest. This
  admits the data only; actual descendant firing/audio remains a runtime gate.
  [Hand-table product validation](../.local/mcc-native-audio-capture-audit-20261018/fire-cc2-product-v1/validation.json),
  [Independent root review](../.local/mcc-native-audio-capture-audit-20261018/fire-cc2-root-review-v1/review.json).
- [x] Clear the exact `CC30C` dependency in ordinary Main with only the
  reviewed hand-table data. The fourth run advances to `E999F`/`E9BB8`, then
  fails after 74.54 seconds. All 312 providers, maps and 97 shaders stay
  pinned; zero reference opens and removed scratch. Its 63,488 callback
  frames are silent; no completed shot or firing audio is claimed.
  [Hand-table runtime regression](../.local/mcc-native-client-20261018/client-current-v8-fire-data-v3/audio-fire-v4-real-mcc/validation.json).
- [x] Authenticate and supply the `E9990` dispatch data as four exact read-only
  spans totaling 204 bytes, excluding padding and overlapping default words.
  The real 60-entry authored catalog selects this owner for 18 types; it is
  not a new guard for arbitrary requests. All 104 Original/native prefixes,
  3,030 instructions and 227 negative controls pass, including 60 genuine
  `B9390` calls. Independent whole-body/domain/source and full-factory review
  preserves all 2,425 prior owners exactly. The actual firing rerun and
  `116570` descendant completion remain separate gates.
  [E99 product validation](../.local/mcc-native-audio-capture-audit-20261018/fire-e99-product-v1/validation.json),
  [Independent review](../.local/mcc-native-audio-capture-audit-20261018/fire-e99-independent-review-v1/review.json).
- [x] Clear the exact E99 table dependency through ordinary Main. The fifth
  firing run reaches `17F946`/`478EA8`, then fails after 72.97 seconds. All
  312 providers, maps and 97 shaders remain unchanged; zero reference opens.
  Its 57,344 callback frames are silent. The reached function-table provider
  and real type domain need verification; no curve or shot is invented.
  [E99 runtime regression](../.local/mcc-native-client-20261018/client-current-v8-fire-data-v4/audio-fire-v5-real-mcc/validation.json).
- [x] Authenticate the reached response row3 against the existing canonical
  material-table provider. All 1,024 bytes equal Original `478EA8`; the full
  unchanged `17F8A0` evaluator and its genuine remainder helper execute in
  2,059 dense cases plus 36 complete state witnesses at three x87 controls.
  Every row byte is read, and observed writes stay in the declared stack.
  Original stores this authored table in a writable `.data` section; this
  proof does not establish arbitrary types, NaN behavior or a whole-bank
  lifetime contract. Native projection/pairing and actual firing remain next.
  [Existing-provider identity](../.local/mcc-native-audio-capture-audit-20261018/fire-curve3-proof-v1/inspection.json),
  [Complete Original evaluator proof](../.local/mcc-native-audio-capture-audit-20261018/fire-curve3-proof-v1/primary.json),
  [Independent raw-XBE and fresh replay review](../.local/mcc-native-audio-capture-audit-20261018/fire-curve3-root-review-v1/primary-review.json).
- [x] Project the reached response row through the ordinary data module,
  and verify complete native evaluator behavior and prior-owner preservation.
  Only the exact 1,024-byte row is added; all 2,429 prior owners remain exact.
  The 36 complete and 2,059 dense cases pass 269,990 instructions and 1,032
  negative controls with no supplied child return. Strict non-FP behavior
  matches Original; CW, FPSW, MXCSR and empty x87 stack/tags match the
  physical-host witness below.
  The required 1,125,904-byte ordinary data object is retained; compiler/link
  scratch is removed. Independent review confirms the exact source change,
  reconstructed complete factory digest and unchanged dependencies/providers.
  The actual firing rerun is next; shot/audio output remain a runtime gate.
  [Response-row product validation](../.local/mcc-native-audio-capture-audit-20261018/fire-curve3-product-v1/validation.json),
  [Independent product review](../.local/mcc-native-audio-capture-audit-20261018/fire-curve3-root-review-v1/product-review.json).
- [x] Clear the exact `17F946` response-row dependency in ordinary Main.
  The sixth firing run advances to `15F77A` reading `48C7A4`, then fails after
  47.99 seconds. All 312 providers, maps and 97 shaders stay pinned, zero
  reference opens are reported, and scratch is removed. Its 45,056 callback
  frames are silent; no completed shot, firing audio or capture is claimed.
  The next owner and its existing development provider need verification.
  [Response-row runtime regression](../.local/mcc-native-client-20261018/client-current-v8-fire-data-v5/audio-fire-v6-real-mcc/validation.json).
- [x] Authenticate the next counter/slot-publication prefix in Original
  `15F6E0`: 67 cases and 6,359 instructions cover all eight slot choices,
  expired/oldest selection, stable ties, early returns and low-word wrap.
  The read at `48C7A4` spans a low16 counter, a separately mutable adjacent
  policy byte and one high byte; raw cold bytes are `2A000100`. Only the
  low16 counter is incremented, while the separate original policy owner can
  clear its byte. No child execution/return or normal Xbox post-boot state is
  supplied or claimed. Independent review supports process-cold fields with
  warm/snapshot preservation; full boot and map-lifetime parity stay open.
  [Counter publication Original proof](../.local/mcc-native-audio-capture-audit-20261018/fire-counter-proof-v1/primary.json),
  [Independent owner/domain review](../.local/mcc-native-audio-capture-audit-20261018/counter-independent-review-v1/review.json).
- [x] Verify a private native three-owner overlay against all 67 Original
  counter prefixes and 6,359 instructions. GP, defined flags, XMM/FPU state,
  ordered reads/stores/PCs and complete stack/player/counter bytes match.
  Seven controls verify cross-owner DWORD reads, independent policy mutation,
  atomic write rejection at the RO byte and checkpoint retention. No child
  return or complete policy execution is supplied. Both private compile/test
  scratch trees are removed; no binaries are retained. Ordinary product
  mounting and actual firing are still separate gates.
  [Native counter proof](../.local/mcc-native-audio-capture-audit-20261018/fire-counter-proof-v1/native-validation.json).
- [x] Admit the three process-cold fields through the ordinary data module.
  Only four bytes in three exact owners are added; all 2,430 prior owners
  remain exact. All 67 native prefixes/6,359 instructions and 15 ownership,
  atomic preflight, policy, checkpoint and warm-preservation controls pass.
  The required 1,152,640-byte ordinary object is retained; compile/link scratch
  is removed. Independent source, complete reconstructed factory, object,
  dependency and control review passes. Actual firing is next; full process/map
  lifetime parity remains separate. No per-shot reset is added. The 15 controls
  comprise ten rejection and five positive ownership/state checks.
  [Counter product validation](../.local/mcc-native-audio-capture-audit-20261018/fire-counter-product-v1/validation.json),
  [Independent product review](../.local/mcc-native-audio-capture-audit-20261018/counter-product-independent-review-v1/review.json).
- [x] Clear the exact counter dependency through ordinary Main. The seventh
  firing run reaches `19B8FE`/`19BA18`, then fails after 51.75 seconds. The
  actual consumed DWORD is `0001002A`, preserving the adjacent policy bit.
  All 312 providers, maps and 97 shaders stay pinned; no reference opens.
  Its 58,368 callback frames are silent. No completed shot or firing image
  is claimed. The classifier's demonstrated row8 is the next dependency.
  [Counter runtime regression](../.local/mcc-native-client-20261018/client-current-v8-fire-data-v6/audio-fire-v7-real-mcc/validation.json).
- [x] Prove the complete unchanged `19B8F0` classifier with a private native
  table overlay. All 136 complete Original/native RET cases pass 856
  instructions and 88 table reads, matching GP, defined flags, XMM/FPU,
  ordered reads/PCs and complete memory. Its low6 mask and unsigned guard
  bound the reachable table to 41 targets; no writes or children are supplied.
  The overlay leaves product source unchanged and retains no large artifacts.
  Independent domain, raw-XBE, whole generated body and actual-reach review
  pass. Ordinary product publication and firing are next.
  [Original classifier proof](../.local/mcc-native-audio-capture-audit-20261018/fire-classifier-proof-v1/primary.json),
  [Private native proof](../.local/mcc-native-audio-capture-audit-20261018/fire-classifier-proof-v1/native-validation.json),
  [Independent classifier review](../.local/mcc-native-audio-capture-audit-20261018/classifier-independent-review-v1/review.json).
- [x] Publish the demonstrated classifier table through ordinary native data.
  Its only new owner is 164 bytes RO; all 2,433 prior owners remain exact.
  All 136 complete Original/native RET cases, 856 instructions, 88 table reads
  and 171 ownership controls pass (170 rejections and one positive native
  pointer check). Independent source, reconstructed full-factory, dependency,
  object and actual control-source review passes. The required 1,161,088-byte
  ordinary object is retained and scratch is removed. The actual eighth firing result follows below;
  successful firing/audio remains open.
  [Classifier product](../.local/mcc-native-audio-capture-audit-20261018/fire-classifier-product-v1/validation.json),
  [Independent product review](../.local/mcc-native-audio-capture-audit-20261018/classifier-product-independent-review-v1/review.json).
- [x] Clear the exact classifier dependency through ordinary Main. The eighth
  firing run advances to root `B9180`/`FFF44` reading scalar `41C4CC`, then
  fails after 79.53 seconds. All 312 providers, maps and 97 shaders stay
  pinned; zero reference opens and removed scratch. Its 54,784 callback frames
  are silent; no completed shot, firing image or sound is claimed. Authenticate
  the new immediate owner and existing scalar provider before any publication.
  [Classifier runtime regression](../.local/mcc-native-client-20261018/client-current-v8-fire-data-v7/audio-fire-v8-real-mcc/validation.json).
- [x] Authenticate the newly reached angle-offset scalar and complete immediate
  owner independently. A separate XBE parser and recursive decoder reproduce
  all 1,194 `FFA60` instruction sites, including both guarded seven-entry
  switches and all 26 direct numeric reads. `41C4CC` is exactly the read-only
  binary32 word `3FC90FDB`, already used by the development projectile path;
  its three-instruction consumer follows the genuine `11F7E0` call. This is
  static authority only. The bounded native execution pairing and ordinary
  publication follow below; an actual successful firing run remains open.
  `46A7D4` is inventory-only.
  [Immediate-owner inventory](../.local/mcc-native-audio-capture-audit-20261018/fire-angle-offset-proof-v1/inspection.json),
  [Independent raw-owner review](../.local/mcc-native-audio-capture-audit-20261018/angle-offset-independent-review-v1/authority-review.json).
- [x] Pair the unchanged three-instruction angle-offset consumer with Original
  execution using a private four-byte read-only overlay. All 104 finite-domain
  cases pass 312 instructions, both input flag seeds and four SSE rounding
  modes, with exact GP, all XMM lanes, ordered reads/store, full stack, MXCSR
  and measured x87 control/status. Flags are unchanged by the three operations;
  empty x87 tags follow initialization with no x87 operation in this slice.
  All 312 cohort inputs remain unchanged, no reference opens occur and scratch
  is removed. This does not execute the complete owner, arbitrary float inputs
  or a shot; ordinary publication follows below and actual firing integration
  remains open.
  [Original scalar consumer](../.local/mcc-native-audio-capture-audit-20261018/fire-angle-offset-proof-v1/primary.json),
  [Private native pairing](../.local/mcc-native-audio-capture-audit-20261018/fire-angle-offset-proof-v1/native-validation.json).
- [x] Publish the exact four-byte `41C4CC` operand through the ordinary data
  factory. All 2,434 prior owners remain exact; the final factory has 2,435
  regions and 479,712 bytes. All 104 finite-domain consumer cases and ten
  ownership rejections plus a native pointer above 4 GiB pass. Independent
  source/factory/object/dependency review passes, with no `46A7D4`, new math,
  ABI, generated code or shader changes. The required 1,170,328-byte leaf is
  retained; compiler/test scratch is removed. Actual coherent firing remains
  the next gate.
  [Ordinary scalar publication](../.local/mcc-native-audio-capture-audit-20261018/fire-angle-offset-product-v1/validation.json),
  [Independent publication review](../.local/mcc-native-audio-capture-audit-20261018/angle-offset-independent-review-v1/product-review.json),
  [Existing-provider qualification and cleanup](../.local/mcc-native-audio-capture-audit-20261018/fire-angle-offset-proof-v1/handoff.json).
- [x] Link the reviewed data leaf into the current coherent V13 client. Only
  the startup-data object changes; its other 313 providers, 43 MCC map files,
  97 shaders and ordinary header dependencies stay pinned. The private ELF
  and map are retained for actual firing validation; link scratch is removed.
  The installed V13 and published version8 stay unchanged. Actual firing and
  sound acceptance remain open.
  [Coherent firing cohort](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v1/cohort.json),
  [One-leaf link evidence](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v1/link-manifest.json).
- [x] Verify the angle-offset read clears in the actual coherent firing run.
  The ninth run advances through it to `100105`, reading `100D08` in
  `FFA60`'s first guarded seven-entry dispatch table, then exits1 after
  72.16 seconds. All 313 other providers, 43 map files and 97 shaders stay
  pinned, with zero reference opens and removed scratch. The genuine callback
  supplies 67,072 nonzero ambience frames; no completed shot, firing sound or
  screenshot is claimed. Authenticate both full-owner dispatch tables and
  the remaining effect-scale operand before the next runtime attempt.
  [Coherent ninth firing result](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v1/audio-fire-v9-real-mcc/validation.json).
- [x] Execute the complete Original local effect-scale selector and clamp
  (`10070C..100756`): 504 finite-domain cases cover all signed16 mode classes,
  threshold angles, preceding damage factors and both flag seeds in 3,648
  instructions. No child or supplied result is used. The exact `46A7D4` word
  already exists in the development effect-scale routine. Native pairing,
  ordinary publication and full firing remain open.
  [Original effect-scale proof](../.local/mcc-native-audio-capture-audit-20261018/fire-effect-scale-proof-v1/primary.json).
- [x] Pair the complete bounded effect-scale segment with unchanged native
  code and the selected development routine: all 504 cases/3,648 instructions,
  84 scalar reads and the existing 924-case contact fixture pass their result,
  GP/XMM/defined-flag/read/store/memory checks. Thirty-six strict emulator
  MXCSR discrepancies remain preserved as RED evidence; exact execution of
  the Original eight-byte `MULSS` on the host qualifies the missing sticky
  precision flags for all 504 native cases. The test return lies outside those
  eight bytes; no complete host segment or Xbox hardware claim is made. All
  test scratch is removed. Independent raw-code and integer nearest-even FP
  review passes. Original mode0 is unclamped; agreement with the development
  helper is limited to the declared damage factors `[0,.25,1]`. Ordinary
  operand publication remains open.
  [Qualified native effect scale](../.local/mcc-native-audio-capture-audit-20261018/fire-effect-scale-proof-v1/qualified-validation.json),
  [Independent effect-scale review](../.local/mcc-native-audio-capture-audit-20261018/effect-scale-independent-review-v1/review.json),
  [Physical multiplication authority](../.local/mcc-native-audio-capture-audit-20261018/fire-effect-scale-proof-v1/host-multiply-validation.json).
- [x] Prove both complete bounded `FFA60` dispatch prefixes against unchanged
  native generated instructions and production WorldState/flags/memory helpers.
  All 68 cases/244 instructions match GP, all XMM lanes, flags, target PC,
  ordered reads and complete memory. Signed16 extension and unsigned `<=6`
  guards bound both tables to seven 32-bit targets/28 bytes each; valid modes,
  defaults and upper aliases are covered. Stops precede the genuine target
  instructions; no target body, child or guest return is supplied. Only private
  read-only overlays are used. All compiler/link scratch is removed, with no
  retained binaries. Independent raw-XBE/guard/helper review reconstructs all
  68 witnesses algebraically and passes; ordinary publication is next.
  [Original dispatch authority](../.local/mcc-native-audio-capture-audit-20261018/fire-dispatch-owner-proof-v1/primary.json),
  [Private native dispatch pairing](../.local/mcc-native-audio-capture-audit-20261018/fire-dispatch-owner-proof-v1/validation.json),
  [Independent dispatch review](../.local/mcc-native-audio-capture-audit-20261018/fire-dispatch-owner-independent-review-v1/review.json),
  [Dispatch proof cleanup](../.local/mcc-native-audio-capture-audit-20261018/fire-dispatch-owner-proof-v1/handoff.json).
- [x] Publish and independently review the two dispatch tables and effect-scale
  operand through the ordinary factory. Only three read-only owners/60 bytes
  are added; all 2,435 previous tuples stay exact, yielding 2,438 owners and
  479,772 bytes. Ordinary-provider checks pass 68 dispatch cases, 504 scale
  cases, the prior 924-case fixture and 81 ownership controls: 77 rejections,
  three native pointers above 4 GiB and one exact cross-owner read. All source
  words match Original; dependencies, 314 prior inputs and strong exports stay
  pinned. No arithmetic, ABI, generated code or shader changes. The required
  1,194,984-byte data object is retained and all product/review scratch is gone.
  Actual coherent firing and sound acceptance remain open.
  [Combined ordinary publication](../.local/mcc-native-audio-capture-audit-20261018/fire-impact-closure-product-v1/validation.json),
  [Independent combined review](../.local/mcc-native-audio-capture-audit-20261018/impact-closure-product-independent-review-v1/review.json),
  [Publication handoff and cleanup](../.local/mcc-native-audio-capture-audit-20261018/fire-impact-closure-product-v1/handoff.json).
- [x] Link the combined data closure into the next coherent V13 client.
  Only the data leaf changes from the ninth-run cohort; 313 other providers,
  all 43 map files and 97 shaders stay pinned. The required private ELF/map
  are retained and link scratch is removed. Installed V13 and packaged
  version8 remain unchanged; the actual tenth firing run is the next gate.
  [Combined firing cohort](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v2/cohort.json),
  [Combined one-leaf link](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v2/link-manifest.json).
- [x] Verify the first dispatch table clears its actual firing boundary.
  The tenth run consumes it and advances through genuine `1027D0`/`18B1D0`
  effects to `18AEC9`, reading `18AF08` in the next bounded table, then exits1
  after 52.20 seconds. The later second-table and effect-scale branches have
  component proofs and remain unobserved in this runtime. All other providers,
  maps, shaders and dependencies stay pinned, with zero reference opens and
  removed scratch. Its 57,344 nonzero callback frames are unidentified ambience;
  no completed shot, firing sound or screenshot is claimed.
  [Actual tenth firing result](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v2/audio-fire-v10-real-mcc/validation.json).
- [x] Execute the complete Original `18AE80` category/fallback leaf and exact
  eight-entry table. All 336 whole-RET cases pass 10,343 instructions, covering
  both category fields, empty/populated metadata, presence patterns, every
  fallback selector, early failure and upper handle aliases. The 123-byte body
  excludes its padding byte and has no child calls. Surviving development
  oracle contracts supply input layouts; missing historical fixture files are
  not claimed as rerun evidence. Publication and actual firing remain open.
  [Original complete fallback leaf](../.local/mcc-native-audio-capture-audit-20261018/fire-effect-fallback-proof-v1/primary.json).
- [x] Pair all 336 cases with the existing public native `18AE80` module and
  the exact private 32-byte table. GP/XMM, defined flags, ordered reads/stores,
  stack, registry, row data and floating-point state match. Only explicitly
  proven observer preimage reads are normalized; the raw failures and diagnostic
  are preserved. No child result is supplied, all 314 providers stay pinned,
  no reference file opens and no large probe artifact remains. Independent
  review and ordinary publication are the next gates.
  [Public native fallback pairing](../.local/mcc-native-audio-capture-audit-20261018/fire-effect-fallback-proof-v1/native-validation.json),
  [Qualified proof and cleanup](../.local/mcc-native-audio-capture-audit-20261018/fire-effect-fallback-proof-v1/handoff.json).
- [x] Inventory the complete direct/tail effect-helper closure rooted at
  `1027D0` and `18B1D0` in its two selected generated modules. All eight whole
  functions and 526 instruction PCs are covered with no external or indirect
  call boundary. Full byte-extent checks against the 2,438-owner factory find
  only the missing `18AEFC` table's 32 immutable bytes. This is a static input
  inventory, not execution or full firing acceptance; both inventory scratch
  directories are verified absent.
  [Bounded effect-helper inventory](../.local/mcc-native-audio-capture-audit-20261018/effect-helper-inventory-v1/inventory.json).
- [x] Independently accept the whole fallback-leaf proof and helper inventory.
  Raw Original bytes and an independent reconstruction match all 336 cases,
  10,343 instruction PCs, 173 table reads and 945 register PUSH stores. Every
  owned site and table index is covered; observer normalization is limited
  to callback-proven preimage reads with the raw failures retained. The
  eight-function closure and every prior factory tuple also match. The
  exact 32-byte read-only table is eligible for ordinary publication;
  product-provider tests and actual firing remain separate open gates.
  [Independent fallback and closure review](../.local/mcc-native-audio-capture-audit-20261018/effect-fallback-independent-review-v1/review.json).
- [x] Publish the exact 32-byte fallback table through the ordinary factory
  and pass its provider fixtures. All 2,438 previous tuples remain exact,
  yielding 2,439 regions/479,804 bytes. The same 336 whole-RET cases and 40
  rejecting controls plus one native pointer above 4 GiB pass, including every
  overlap byte, read-only/bounds guards and warm-state preservation. Only the
  data leaf and bounded preflight change; no generated body, ABI or shader
  changes. The required 1,206,112-byte object is retained and both scratch
  directories are removed. Independent product acceptance and the next actual
  firing run remain open; installed V13 and packaged version8 stay unchanged.
  [Ordinary fallback provider](../.local/mcc-native-audio-capture-audit-20261018/fire-effect-fallback-product-v1/validation.json),
  [Product handoff and cleanup](../.local/mcc-native-audio-capture-audit-20261018/fire-effect-fallback-product-v1/handoff.json).
- [x] Independently accept the ordinary fallback-table publication. Source
  normalization, all eight Original words, all 2,438 previous factory tuples,
  the 2,439-region snapshot, object, dependencies, 314 prior inputs, exports
  and exact control links match. The complete accepted execution/observer
  loop is unchanged except ordinary-provider checks and labels. All 40
  rejecting controls plus the native pointer case are accounted for; actual
  firing remains unverified beyond the tenth run.
  [Independent ordinary fallback review](../.local/mcc-native-audio-capture-audit-20261018/effect-fallback-product-independent-review-v1/review.json).
- [x] Link the accepted fallback data leaf into coherent firing candidate v3.
  The link succeeds with all 313 other providers, 43 maps and 97 shaders exact.
  The required private ELF is 104,675,632 bytes and its map is 20,465,770 bytes;
  installed V13 and published version8 remain unchanged. Actual eleventh firing
  validation is the next gate.
  [Coherent fallback-table candidate](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v3/cohort.json),
  [Single-leaf link receipt](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v3/link-manifest.json).
- [x] Verify the fallback table clears its actual firing boundary. The eleventh
  coherent run advances beyond `18AEFC` to native root `179DF0`, then stops at
  `2C2AD8` reading the next table at `2C2B30`, exiting1 after 53.60 seconds.
  All 313 other providers, 43 maps, 97 shaders and dependencies remain exact,
  with zero reference opens. Its 59,904 nonzero callback frames are unidentified
  ambience; no completed shot, firing sound, screenshot or full run is claimed.
  The complete new table owner and surrounding particle-frame dependencies
  are the next compatibility work.
  Link/runtime/sealer scratch, raw PCM, full syscall trace and temporary save
  are removed; the requested 239,660-byte WAV and small diagnostics are retained.
  [Actual eleventh firing result](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v3/audio-fire-v11-real-mcc/validation.json),
  [Actual run handoff and cleanup](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v3/audio-fire-v11-real-mcc/handoff.json).
- [x] Inventory the new particle-module direct/tail dependency closure from
  `179DF0` and `2C2AD0`. All 50 reached whole functions and 3,686 instruction
  sites match the unchanged Original CFGs. Its 62 external/indirect boundaries
  are explicit and unexpanded. The only unowned fixed inputs are the 16-byte
  read-only table at `2C2B30`, a separate four-byte read-only scalar at `46A784`,
  and a mutable virtual-tail global at `56E0E8`. The latter is not admitted
  as zero or any other runtime default. The 2,439-owner factory stays exact;
  this scan admits no data and proves no new execution. Scratch is removed.
  [Whole particle-module static inventory](../.local/mcc-native-audio-capture-audit-20261018/particle-frame-inventory-v1/inventory.json),
  [Qualified inventory handoff](../.local/mcc-native-audio-capture-audit-20261018/particle-frame-inventory-v1/handoff.json).
- [x] Execute untouched Original `2C2AD0` dispatch and declared no-active-instance
  paths. All four table indices and unsigned rejection controls pass 32 prefix
  cases; 52 whole-RET cases execute type0 with the real `176A40` child or reject
  an out-of-range type. Together they execute 3,314 instructions with complete
  input/output, reads, stores, stack and FP receipts. The full owned CFGs match
  current generated sites. Historical particle-motion reports are reused as
  context only: their absent memory banks prevent claiming a rerun. Native
  pairing, active-instance/collision branches and ordinary publication remain open.
  [Original particle dispatch and declared whole paths](../.local/mcc-native-audio-capture-audit-20261018/particle-dispatch-proof-v1/primary.json).
- [x] Pair those 32 dispatch controls and 52 whole-RET paths with native
  execution, reusing the existing public particle module for every whole path.
  All 3,314 executed instructions, GP/XMM, defined flags, ordered reads/stores,
  complete 1,024-byte stacks, input banks and FP state match Original. Every
  dispatch index is read; only exact observer-proven preimage reads are
  normalized. The private 16-byte table has a native pointer above 4 GiB.
  All 314 prior providers stay exact, zero reference files open, and scratch
  is removed. This proves the declared paths, not active collision or firing.
  [Native particle dispatch and declared whole paths](../.local/mcc-native-audio-capture-audit-20261018/particle-dispatch-proof-v1/native-validation.json).
- [x] Review the table with a separate raw-file and unsigned-algebra method.
  All 32 prefix states/targets/flags/reads and 16 complete rejected paths are
  independently reconstructed, and every target belongs to the owned CFG.
  The 16 bytes are fully file-backed and read-only. The other 36 whole paths
  retain their Original/native pairing and 1,008 exactly qualified observer
  preimages. All providers/dependencies remain pinned. This is a second method
  by the primary agent, not a second-agent review; active-particle/collision
  and firing claims remain open. Exact ordinary table publication is eligible.
  [Raw-file and algebraic table review](../.local/mcc-native-audio-capture-audit-20261018/particle-dispatch-proof-v1/review.json).
- [x] Publish the exact 16-byte particle dispatch table through the ordinary
  factory. A real old/new factory pair confirms every previous tuple is exact:
  2,439 regions/479,804 bytes become 2,440/479,820 with one read-only owner.
  The 32 dispatch controls, 52 whole paths and 24 rejection controls plus one
  native pointer check pass, including all 16 overlap bytes, endpoints, exact
  bounds, excluded padding/code and warm-state preservation. Only data and its
  bounded preflight change. The required 1,214,872-byte data object is retained;
  scratch executables, maps, raw snapshots and traces are removed.
  [Ordinary particle dispatch provider](../.local/mcc-native-audio-capture-audit-20261018/particle-dispatch-product-v1/validation.json).
- [x] Review the ordinary 16-byte projection with a separate inverse-source
  and raw-file method. Removing only the new mount/preflight reproduces the
  exact prior source; its four words match file-backed Original bytes. The
  accepted execution/observer loops are otherwise byte-identical, all prior
  providers/dependencies and exports match, and both native addresses and
  factory bounds stay exact. Proof/product/review scratch is removed. This
  primary-agent second-method review admits the bounded table; the next actual
  firing run and full active-instance/collision behavior remain open.
  [Ordinary particle product review and cleanup](../.local/mcc-native-audio-capture-audit-20261018/particle-dispatch-product-v1/review.json).
- [x] Link the accepted 16-byte table into coherent firing candidate v4.
  Only the data leaf changes; all 313 other providers, 43 maps, 97 shaders
  and prior compatibility records stay pinned. The required private ELF is
  104,679,728 bytes and its link map is 20,465,765 bytes. Link scratch is
  removed; the installed V13 runtime and version8 ZIP remain unchanged.
  Actual twelfth firing validation is the next gate.
  [Coherent particle-table candidate](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v4/cohort.json),
  [Single-leaf link receipt](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v4/link-manifest.json).
- [x] Verify the particle table clears its actual firing boundary. The twelfth
  run advances through tick markers 0/3/6/8/10 and records up to five world
  and two first-person particle draw submissions. It then exits1 at `179DF0`
  /`2C3098`, reading the unowned mutable `56E0E8` field, after 46.48 seconds.
  This is submission evidence, not a captured firing frame or complete run.
  The 51,200 finite nonzero callback frames remain unidentified audio. All
  other providers, maps, shaders and dependencies stay exact; zero reference
  files open. Raw PCM, trace, temporary save and link/runtime scratch are
  removed, with the requested 204,844-byte WAV and small receipts retained.
  [Actual twelfth firing result](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v4/audio-fire-v12-real-mcc/validation.json),
  [Run progress and cleanup handoff](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v4/audio-fire-v12-real-mcc/handoff.json).
- [x] Identify and execute the real collision-threshold initializer. Complete
  `3829C0` is 17 bytes/three instructions and copies the existing read-only
  `46A1B8` value `3E2AAAAB` into writable `56E0E8`; its genuine constructor-table
  entry is `46DD64`. Sixteen Original whole-RET controls preserve the expected
  GP/XMM/flags/stack/FP state and confirm the unconditional store. Four more
  controls qualify the actual `46A784` scalar load. Both the exact `1/6`
  threshold and `.005` collision offset already occur in the working development
  collision implementation. The missing field is a cold-construction memory
  bridge; its virtual loader zero is not its runtime default. Native ownership,
  warm preservation and actual collision acceptance remain open.
  [Original constructor and existing development constants](../.local/mcc-native-audio-capture-audit-20261018/collision-constructor-proof-v1/primary.json).
- [x] Supply the authenticated collision defaults in the ordinary factory.
  The four-byte mutable field reproduces the complete initializer's persistent
  `1/6` memory effect; the four-byte `.005` word stays read-only. Existing
  `46A1B8` is mounted later in the full factory, so its exact immutable bits
  are materialized without moving, aliasing or changing that previous owner.
  All 2,440 prior tuples stay exact, yielding 2,442 regions/479,828 bytes.
  Eight foreign overlap bytes reject before writes; native pointers exceed
  4 GiB, mutable writes preserve the shared source, and memory-subowner restore
  plus warm counter/policy controls pass. Physics calculations and shaders are
  unchanged. Required data object: 1,232,000 bytes; temporary probes are removed.
  Product review and actual firing remain open; no original CRT caller CPU
  context or whole-world restore claim is made.
  [Ordinary collision-default compatibility](../.local/mcc-native-audio-capture-audit-20261018/collision-defaults-product-v1/validation.json).
- [x] Review the eight-byte collision bridge using direct Original bytes,
  inverse source normalization and the actual compiled control receipts.
  Only the two mounts/preflight pairs change; all 2,440 prior tuples, existing
  development collision source, 314 providers, dependencies and strong exports
  stay exact. Twenty rejecting controls, two high native pointers, four mutable
  write patterns and the quiescent memory-subowner restore pass. Proof/product
  scratch is removed. This primary-agent second-method review admits the
  persistent data bridge, not full CRT CPU context, whole-world restore or
  complete collision/firing parity.
  [Collision bridge review and cleanup](../.local/mcc-native-audio-capture-audit-20261018/collision-defaults-product-v1/review.json).
- [x] Link the accepted collision defaults into coherent firing candidate v5.
  All 313 other providers, 43 maps, 97 shaders and compatibility records stay
  exact. The required private ELF is 104,679,728 bytes and its map is
  20,465,798 bytes; link scratch is removed. Installed V13 and the published
  version8 package remain unchanged. Actual thirteenth firing is the next gate.
  [Coherent collision-default candidate](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v5/cohort.json),
  [Single-leaf collision link receipt](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v5/link-manifest.json).
- [x] Complete the actual bounded SMG firing run in coherent candidate v5.
  Genuine fire-at-3/release-at-4 input completes tick60 with exit0 and 74
  rendered frames. Ammo decreases from the neutral baseline's 60 to 59,
  reserve stays 120, and two detonations are recorded. World/first-person
  particle draw submissions reach seven/two. The captured image was viewed:
  normal arms, SMG, HUD and clear left edge are present. The actual callback
  stream contains 142,336 finite nonzero stereo frames; identifying the firing
  sound and physical audibility remain unverified. All 313 other providers,
  43 maps and 97 shaders stay exact, with zero original-reference opens.
  Link, runtime, conversion and sealing scratch is removed; requested image
  and audio evidence and the required coherent candidate remain. This proves
  one bounded shot, not sustained firing, all weapons or complete parity.
  Installed V13 and the version8 ZIP remain unchanged; refreshing the interim
  launcher/package is next.
  [Actual firing validation](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v5/audio-fire-v13-real-mcc/validation.json),
  [Verified handoff and cleanup](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v5/audio-fire-v13-real-mcc/handoff.json),
  [Actual firing image](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v5/audio-fire-v13-real-mcc/lockout-fire-tick60.png),
  [Actual callback audio](../.local/mcc-native-client-20261018/client-current-v13-fire-data-v5/audio-fire-v13-real-mcc/native-gameplay-audio.wav).
- [x] Execute the entire unchanged Original response evaluator and its real
  CRT helper on the physical host to qualify emulator floating-point reports.
  A disposable static 32-bit ELF retains all Original section bytes at their
  exact addresses; all 36 cases match Original emulator GP, XMM, defined flags
  and every final stack byte. The host supplies separate status/control/tag
  and MXCSR witnesses. All 36 native status/control states match these host
  witnesses exactly, including the emulator's omitted precision/condition
  flags. FIP/FDP/last-opcode fields are outside this comparison. No emulator,
  patched instruction or product dependency is used, and
  all generated ELF/image/capture scratch is removed. This is current-host
  evidence, not Xbox hardware certification.
  [Whole Original physical-host qualification](../.local/mcc-native-audio-capture-audit-20261018/fire-curve3-host-v1/validation.json),
  [Exact native comparison](../.local/mcc-native-audio-capture-audit-20261018/fire-curve3-host-v1/native-comparison.json),
  [Independent capture and authority review](../.local/mcc-native-audio-capture-audit-20261018/fire-curve3-host-v1/independent-review.json),
  [Scratch cleanup](../.local/mcc-native-audio-capture-audit-20261018/fire-curve3-host-v1/cleanup.json).
- [ ] Wire proved native sound-query/playback components into Main and multiplayer.
- [x] Correct the demonstrated MCC looping-sound root layout in the reused
  content loader: its extra eight-byte prefix no longer becomes an Xbox track
  count. All 40 actual MCC maps pass 6,825 definitions, 7,454 tracks, 12,625 sound
  references and 240 negative controls. The original Xbox parser and complete
  resident source bytes remain unchanged. The separate unmuted V12 runtime
  check above captures nonzero ambience; source timing and lifecycle remain open.
  [Looping-sound layout validation](../.local/mcc-looping-sound-ambience-20261018/v1/validation.json).
- [ ] Verify voice/loop lifecycle, timing, RNG, restoration, mute and output behavior.
- [ ] Verify audible gameplay output and synchronization during real play.

## Launcher and delivery

- [x] Keep Play enabled for the clearly labeled interim MCC content test.
- [x] Retain the existing packaged MCC content test build (v6).
- [x] Implement and test the separate native Lockout test capability. Runtime,
  selected map and all 97 shader hashes must match; missing or stale validation
  retains the preview route. The launcher-only checks passed 111 cases across
  three launcher variants, the capability reader and Tk controls. The later
  installed-route and relocatable-package gates below also pass; complete
  gameplay readiness remains false.
  [Launcher capability handoff](../.local/mcc-launcher-native-test-20261018/v1/handoff.json).
- [x] Register the native-test capability and MCC menu-asset controls in CTest.
  An actual disposable CMake/CTest run passed both tests (14 cases), without
  compiling engine targets; all generated build/test scratch was removed.
  [CTest registration validation](../.local/mcc-launcher-native-test-20261018/ctest-registration-v2/validation.json).
- [x] Verify the combined CMake registrations for both HUD manifests, native
  capability, portable shader contract and MCC menu assets. All five focused
  CTests pass; no engine targets are rebuilt and the private build is removed.
  [Combined CTest validation](../.local/mcc-hud-bitmap-geometry-20261018/cmake-final-v1/results/report.json).
- [x] Implement and verify the portable compiled-default shader contract:
  exact read-only path-field relocation, canonical wrapper/loader binding and
  full 97-file inventory. Twenty focused cases and moved-folder launcher/resource
  controls pass, with independent review. Executable instructions and shader
  bytes are unchanged. The actual packaged-execution and publication results
  are recorded separately below.
  [Portable package contract](../.local/mcc-packaged-shader-contract-20261018/v1/handoff.json).
- [x] Build the two portable release launcher binaries for the requested ZIP.
  Both retained release inputs pass 58 routing cases and actual wrapper/relocation
  info checks. These portable binaries are shipped in version7; the installed
  checkout uses the separate local launcher pair below.
  [Release launcher validation](../.local/mcc-native-launcher-delivery-20261018/v1/validation.json).
- [x] Build the two current local launcher binaries with the installed
  checkout's existing absolute project/runtime/Python settings. Both actual
  `--launcher-info` routes select `build-multiplayer/halo2_native` and report
  capability state honestly. Only these two small consumers were compiled;
  their later installation and actual native route are verified below.
  [Local launcher build](../.local/mcc-native-local-launchers-20261018/v1/validation.json).
- [x] Connect the validated interim native build to both installed launchers.
  Both current launcher-info routes now select the exact V12 runtime and report
  the native Lockout test available with full readiness false. The actual
  installed launcher route renders eight frames successfully. Existing shaders
  and source-cache provenance stay unchanged; exact pre-install runtime and
  launcher copies are retained for rollback. The 8,478,720-byte default native
  test save/cache is explicitly retained for user Play.
  [Installed route validation](../.local/mcc-native-install-20261018/v1/validation.json).
- [x] Publish the validated native-test capability in a relocatable compiled
  ZIP after actual packaged execution. The real relocated payload passes
  tick30, both bundled launcher-info routes admit it, the actual launcher
  renders eight frames, and unmuted packaged tick30 produces nonzero PCM.
  Captured file opens show no original-reference or external checkout
  dependency. Missing capability falls back honestly; full readiness stays false.
  [Actual packaged validation](../.local/mcc-native-package-delivery-20261018/v1/run-v1/validation.json).
- [ ] Verify an ordinary user can import, launch and test through the launcher.
- [x] Package a fresh compiled native-test ZIP and verify its extracted contents.
  Version7 contains the current V12 runtime, existing 97 shaders, launchers and
  bundled onboarding dependencies. Every extracted checksum passes and both
  extracted launchers run. Test-only MCC cache/selection/saves are removed;
  no game banks are distributed. README records the verified firing-termination
  limitation and unfinished parity; this is an interim test release.
  [Native test ZIP](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261018-v7.zip),
  [SHA256](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261018-v7.zip.sha256),
  [Actual capture](../dist/Halo2-MCC-Native-Test-Lockout-20261018-v7.png),
  [Actual callback audio](../dist/Halo2-MCC-Native-Test-Ambience-20261018-v7.wav).
- [x] Publish compiled version8 with the verified class1 geometry correction
  and coherent V13 data. The real relocated payload passes tick30, both
  portable launcher-info checks, the actual eight-frame launcher route and
  unmuted tick30 with 113,664 callback frames. ZIP extraction/checksums,
  fallback admission and reference/external-checkout path audits pass.
  All 97 shaders are preserved, test-only cache/selection/saves are removed,
  and original source-cache identities stay unchanged. This remains an interim
  native Lockout test; firing/full readiness, fresh-import acceptance and full
  audiovisual parity are unfinished. Requested ZIP, PNG and WAV are retained.
  [Version8 ZIP](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261018-v8.zip),
  [SHA256](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261018-v8.zip.sha256),
  [Actual package validation](../.local/mcc-native-package-delivery-20261018/v2/run-v1/validation.json),
  [Corrected image](../dist/Halo2-MCC-Native-Test-Lockout-20261018-v8.png),
  [Actual callback audio](../dist/Halo2-MCC-Native-Test-Ambience-20261018-v8.wav).
- [x] Publish compiled version9 with the accepted collision defaults and
  passing bounded SMG firing path. The actual relocated package passes neutral
  tick30, both launcher-info routes, the eight-frame launcher route, and an
  unmuted fire-at-3/release-at-4 run through tick60: 74 frames, ammo 60 to 59,
  two detonations and 156,672 finite nonzero callback frames. Every ZIP
  checksum and extracted launcher check passes; no game data is distributed.
  Captured opens contain no original-reference or external checkout dependency.
  All 97 shaders, 314 providers, 43 maps and 25,004 source-cache identities are
  preserved. Actual PNG was viewed; generated staging, trace, PCM and sealing
  scratch is removed. Previous version8 artifacts stay unchanged. The README
  records the bounded result and unfinished full readiness accurately. Local
  installation of this candidate is next.
  [Version9 compiled ZIP](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261018-v9.zip),
  [SHA256](../dist/Halo2-MCC-Native-Test-Linux-x86_64-20261018-v9.zip.sha256),
  [Actual package validation](../.local/mcc-native-package-delivery-20261018/v3/run-v1/validation.json),
  [Verified handoff and cleanup](../.local/mcc-native-package-delivery-20261018/v3/handoff.json),
  [Actual image](../dist/Halo2-MCC-Native-Test-Lockout-20261018-v9.png),
  [Actual callback audio](../dist/Halo2-MCC-Native-Test-Firing-20261018-v9.wav).
- [x] Finish the installed V13 launcher-route test and preserve rollback
  runtime, capability and existing save files. Both actual launcher-info routes
  select runtime `2151bd97...`, native test available and full readiness false;
  the real installed launcher renders eight frames successfully. Shader and
  source-cache provenance are unchanged. Prior runtime/launchers/capability and
  both save files are backed up; installed tests preserve existing save bytes.
  Generated capture/trace scratch is removed, with rollback copies retained.
  [Installed V13 validation](../.local/mcc-native-install-20261018/v2/validation.json).
- [x] Install the exact accepted firing candidate behind both existing Play
  routes. Both actual launcher-info checks select runtime `915a647c...`, admit
  the native Lockout test and keep full readiness false; the actual installed
  launcher renders eight frames with exit0. Both 4,239,360-byte existing save
  files preserve hash `1292a582...`. All 314 providers, 43 maps, 97 shaders
  and source-cache provenance remain exact, with zero reference opens.
  Runtime, launchers, capability and save rollback copies total 113,601,200
  bytes and are explicitly retained. Test/trace/capture and sealing scratch
  is removed. This completes interim installation, not full save/lifecycle,
  sustained firing, campaign or TU1.5 audiovisual acceptance.
  [Installed firing-build validation](../.local/mcc-native-install-20261018/v3/validation.json),
  [Verified selection, rollback and cleanup](../.local/mcc-native-install-20261018/v3/handoff.json).
- [x] Record and deliver the requested loading-to-native-match video with
  sound. The retained6,181,804-byte MP4 records the existing animation, recovered
  four-voice ambience and native handoff. Full gameplay/audio parity stays open.
  [Video and audio proof](../.local/mcc-loading-original-tone-20261020/native-proof.json),
  [Requested video](../.local/mcc-loading-original-tone-20261020/mcc-loading-to-native-match-with-sound.mp4).
- [ ] Report remaining limitations honestly; do not label partial tests full TU1.5 parity.

## Final cleanup, after validation/delivery

- [x] Audit the11 previously named obsolete factory/lens test executables after
  V14 delivery, preserving the first three broader native-gameplay probes. The11
  total1,056,763,768 logical bytes; ten retain every recorded reproduction input,
  while one superseded probe has a drifted object and remains deferred. No current
  V14 input or visible active-process reference points to these executables.
  Six same-UID system processes and foreign processes deny complete `/proc`
  inspection, and a read-only noninteractive elevated check is unavailable
  because existing sudo authorization is absent. Therefore no historical
  executable is deleted and final cleanup gates remain open. Both audit scratch
  directories are removed; source, objects, references and deliverables remain.
  [Bounded cleanup audit](../.local/mcc-native-click-crash-20261018/cleanup-v14/audit.json).

- [ ] Inventory confirmed unused, reproducible test executables and temporary outputs.
- [ ] Check active processes, mapped/open files, current build inputs and launcher selection.
- [ ] Delete obsolete test/build artifacts; preserve useful test source,
  compiled engine/current coherent objects, validation evidence, reference
  assets, installed runtimes and delivered releases.
- [ ] Record exact removed paths and recovered bytes; verify current builds
  and launcher selections still work.

The preliminary cleanup inventory currently identifies approximately 1.26 GiB
of superseded private diagnostic executables. Nothing in that inventory has
been deleted. See
[cleanup plan](../.local/mcc-model-cpu-bitmap-20261018/world-v2-inline/cleanup-after-delivery.json).
Separately, the newly generated V10 syscall trace was summarized by the full
independent audit, then removed with its exact hash and byte count recorded.
This recovered 22,242,541 bytes while preserving the audit, frozen validation,
logs and requested images; it does not complete the final cleanup gates.
[Bounded trace cleanup](../.local/mcc-native-client-20261018/client-current-v10-hud-mcc/run-v1-real-mcc/trace-cleanup.json).

## Latest graphical conversion milestones, 2026-10-19

- [x] Prove Sacred Icon's gradient descriptor/resource correction with the exact
  selected V16 native loader. Stock MCC is64×256/nine mips; the original is
  32×128/eight mips. A private MCC map retains its format/packing flags while
  changing only width, height, mip count and resource pointer/extent. Every
  decoded mip descriptor and all21,852 output bytes equal the original. The
  316 selected providers and original/MCC source files remain exact. Probe ELF,
  object, map, raw pixels and private map/bank are removed with scratch. This
  establishes the conversion shape; bundled MCC-only reconstruction and rendered
  sky acceptance remain open.
  [Native descriptor/resource proof](../.local/mcc-sacred-icon-gradient-20261019/descriptor-v1/validation.json).
- [x] Add a fingerprint-bound MCC-only Sacred Icon gradient delta and importer
  integration. All six corrected images/48 mips decode through exact V16 native
  providers to their original target hashes after real three-map application.
  A malformed final profile fails before any map/bank write; the entire original
  bank prefix remains exact. Onboarding distinguishes complete V4 copies from
  older correction sets. V1/V2/V3 bundles stay immutable. The new product delta
  and manifest total5,521bytes; all private maps/banks/pixels/probe outputs are
  removed. Existing active caches and releases are untouched. Focused tests,
  independent review, installed delivery and rendered sky acceptance remain
  separate gates. Both focused suites also pass with the already shipped
  portable Python:46 controls, no failures/errors/skips, with scratch removed.
  [Actual MCC-only V4 proof](../.local/mcc-sacred-icon-gradient-20261019/bundle-v1/validation.json).
- [x] Independently review V4 reconstruction and cache reuse. Exact MCC-only
  reconstruction opens only its product delta/manifest. A reproduced downgrade
  edge is fixed: a complete V4 cache remains reusable if its local delta is
  missing; all four freshness controls and46 portable controls pass. The same
  review authenticates the gradient's signed registration point: MCC32,128
  versus original16,64. Pixel equality alone did not cover those words; include
  their exact guarded restoration before V4 delivery. No particular active
  gradient registration consumer or additional visible defect is claimed.
  [Independent post-fix review](../.local/mcc-sacred-icon-gradient-20261019/independent-review-v2/validation.json).
- [x] Finish V4's exact registration-point restoration and repeat real MCC
  conversion with the final product inputs. Source32,128 is guarded and target
  16,64 restored. A wrong registration tuple rejects all writes. The complete
  decoded metadata of all three maps is compared: only the five existing
  resource bindings and the gradient's resource/dimensions/mips/registration
  fields change outside cache-publication headers. All six native images/48
  mips and46 shipped-Python controls still pass. The final delta/manifest total
  5,624bytes; all probe/map/bank/pixel scratch is removed. V17 packaging and
  rendered sky acceptance remain open.
  [Final conversion/metadata proof](../.local/mcc-sacred-icon-gradient-20261019/bundle-v2/validation.json)
  · [Portable controls](../.local/mcc-sacred-icon-gradient-20261019/bundle-v2/portable-python-tests.json).
- [x] Prepare and review the importer-only V17 compiled ZIP workflow. Fresh
  guards verify final V4 inputs/proofs, immutable V16 payload and all97 shaders.
  Ten named archive paths change/add; all other release bytes must match V16.
  Historical importer pins are explicitly separated from current V4 authority.
  Preflight scratch is removed. Root authorizes package-only execution against
  this concrete sealed handoff; installed folders/caches remain untouched.
  [Sealed preparation](../.local/mcc-sacred-icon-gradient-20261019/delivery-v17/preparation-v2.json).
- [x] Record V17's first execution's final preservation-guard failure.
  Relocated46 tests and the eight-frame launcher
  path reached the final guard; no ZIP was published and private scratch is
  removed. The exact beginning snapshot was not retained, so the cause remains
  unassigned pending a measured diff. Preserve the failed attempt's small
  diagnostics and add explicit before/after state receipts before any retry.
- [x] Preserve attempt1 and qualify an instrumented retry. Fresh source/base
  hashes match; three complete repeated metadata sweeps reproduce no change.
  The earlier preflight-to-current comparison differs only in P's aggregate
  cache metadata, and the lost actual initial snapshot prevents a causal claim.
  Attempt2 keeps every strict check and records actual before/after states,
  per-field differences and each final operand. Its separate sealed preflight
  passes; root authorizes this concrete package-only retry without weakening
  the preservation gate.
  [Instrumented preparation](../.local/mcc-sacred-icon-gradient-20261019/delivery-v17/preparation-v3.json).
- [x] Publish compiled V17 after the instrumented attempt passes every original
  strict preservation check. Actual before/after state, all cached file metadata,
  runtime/selection/save hashes and every source/base operand agree. Relocated
  46 controls, both launcher-info routes, eight-frame Lockout launch and all4,251
  archive checksums pass. Exactly ten archive paths change/add; the V16 payload,
  wrapper/loader and97 shaders remain exact. No game data or saves are bundled.
  Scratch is removed; only the requested ZIP/checksum is retained. The initial
  attempt's cause remains unestablished. Independent archive review and P-only
  importer installation follow; existing caches are not upgraded by packaging.
  [Actual compiled V17](../.local/mcc-sacred-icon-gradient-20261019/delivery-v17/package-validation.json).
- [x] Independently verify V17's4,252 entries/all4,251 checksums, seven changed
  existing/three added paths and all4,242 remaining V16 entries. Runtime,
  wrapper/loader,97 shaders, current importer pins and historical authority
  roles agree; attempt2's strict preservation proof is intact. Review scratch
  is removed. P-only importer installation remains next.
  [Independent V17 archive review](../.local/mcc-sacred-icon-gradient-20261019/delivery-v17/archive-review-v1/validation.json).
- [x] Review and prepare P-only V17 deployment with cancellation-safe rollback.
  A real queued-SIGTERM control covers rename-before-record cancellation; the
  mutation is now recorded before signals resume, and rollback restores files
  before unblocking. Fresh read-only preflight verifies all old release files,
  three absent new paths and preserved user state. Seven small preimages total
  571,265bytes; no runtime copy/import/cache conversion is planned. Root
  authorizes the sealed V2 handoff against the accepted ZIP. Actual staging
  uses owned scratch on P's filesystem for atomic replacements.
  [Cancellation-safe installer preparation](../.local/mcc-sacred-icon-gradient-20261019/delivery-v17/install-v2/preparation.json).
- [x] Install the accepted V17 importer/release files into the existing P
  folder. All4,252 release paths match the verified ZIP; both P launchers keep
  the native test available/full readiness false. The runtime,97 shaders,
  saves, cache metadata, selections, previous rollback trees and root's separate
  workbench are preserved. Seven required rollback files total571,265bytes;
  staging scratch on P's filesystem is removed. No new game run was needed
  after installing the same accepted payload, and existing caches remain at
  their previous correction levels until a new import.
  [Actual installed V17](../.local/mcc-sacred-icon-gradient-20261019/delivery-v17/install-v2/install-validation.json).
- [x] Measure the later read-only closure audit's metadata mismatch without
  altering the successful package/install receipts. Two immediate scans change
  only 3,072 P inode IDs; directories/names/device/size/mtime/mode remain equal,
  including their saved package field hashes. Root's complete metadata and
  launcher/runtime/save hashes remain exact. The accepted install's original
  before/after guard passed. Preserve the failed audit and qualify the later
  storage audit against stable fields with inode identity explicitly excluded;
  this does not prove full-bank content hashes or the cause of the earlier
  packaging failure. The diagnostic scratch is removed.
  [Measured inode-only difference](../.local/mcc-sacred-icon-gradient-20261019/closure-diagnostic-v2/measurement.json).
- [x] Finish the scoped V4/V17 storage audit and reverify all 4,252 installed
  release files, 97 shaders, the ZIP/checksum and seven rollback preimages.
  All 31 recorded completed product/delivery scratch paths and the audit's own
  scratch are absent; the separate preparation scratch is also removed. The
  audit preserves the successful original package/install guards and records
  the measured later inode exclusion explicitly. Newly retained delivery data
  totals 184,194,863 bytes: the requested ZIP/checksum 183,623,598 bytes and required
  rollback 571,265 bytes. Active material/foot work, inherited references/cohorts,
  installed data and historical cleanup stay outside this bounded audit.
  [Scoped delivery closure](../.local/mcc-sacred-icon-gradient-20261019/closure-v2/validation.json).

## Next steps, in order

1. Complete the MCC-only title screen and menu vertical slice described in the
   current-priority section, then make it default Play. Keep native Lockout
   testing separately available. Wire platform binary downloads to Update;
   the official repository currently has no published release assets, which
   must remain an explicit availability limit rather than a source rebuild.
2. Continue gameplay-tag semantic coverage beyond the now-delivered V16
   map-specific Gauss/Relic conversions. All28 authenticated contexts/two
   unqualified MCC maps pass complete Cache comparisons, and the retained
   executable passes firing, real input and relocated package launch. P now
   matches all4,252 V17 files with the same V16 runtime; root's separate renderer
   workbench stays selected. The Floodzone material correction passes actual
   normal Cache checks across all43 maps and is delivered in the independently
   reviewed V18 ZIP. Its compatible client passes fixed-tick frame/save checks,
   nine real input stages and relocated launcher gameplay. P remains V17.
   The guarded Chief union conversion now passes all 43 selected Cache views,
   2,345 direct native selector cases and eight private controls. Its compatible
   V19 successor passes the paired native frame/save/audio acceptance; preserve
   that private candidate and resume its input/delivery work after the frontend,
   then continue remaining
   material-effect semantic coverage.
   Preserve unknown mods, saves,97 bundled shaders, TU1.5 semantics and interim
   Play. Data conversion does not establish native CTF or all other modes.
   Distinguish actual host-pointer/ownership limitations from unrelated current
   feature-body drift before any broader rebuild. Remaining first-launch,
   complete renderer and live parity acceptance stay open.
3. Continue rendered acceptance of the now-packaged/installed six-image/48-mip
   V4 importer. The Sacred Icon gradient's exact descriptor, registration,
   resource and pixel conversion pass; its live sky still needs acceptance.
   The fresh MCC-only preparation, resumed cache, actual native shot/audio,
   selected root V3 cache, compiled V13 importer and extracted-folder tool
   synchronization are verified above. Do not repeat the 31GB import. The user's
   extracted folder retains its working older cache; the updated setup can
   prepare the newer correction set when requested. The ocean texture needs
   no pixel correction. Preserve the current working caches during delivery.
4. Wire the honest native-test launcher capability and MCC-only loading/menu
   asset handoffs beyond the now-published interim Lockout route. Verify a fresh
   MCC-only import, remaining loading/menu roles and mod-folder interoperability.
   The loading-name kernel and actual tag1009 branch/descriptor ingress are
   sealed; actual GFx loading invocation/image bytes and the genuine
   `loading_screen_music` live-service selection remain open. The separate
   frontend music graph, exact WEM and stock decoder are authenticated. The
   selector's live caller, active loading image bytes and presentation remain
   open; decoded callback data alone does not establish audible equivalence.
   The V3 product/delta and onboarding delivery are
   verified; wider rendered scene acceptance remains open.
5. Validate the ordinary client and complete safe product/lifecycle integration.
6. Continue the remaining onboarding, conversion, parity and delivery gates above.
7. Perform the final cleanup and record the recovered storage.


## v530 MCC shortcut admission and runtime-source prerequisites, 2026-10-19

These prerequisite milestones are recorded before the dependent root-shortcut
installation. Runtime inputs remain imported MCC only. Xbox ISO may be used
only for offline validation of converted MCC tags; decompiled TU1.5 code remains
the behavior authority. Xbox/XBE/Vista banks are never runtime dependencies.

- [x] Review all 13 passing synthetic MCC shortcut admission/routing tests.
  Module `tools/mcc_sandbox_launcher.py` SHA-256
  `18e71ab5d5e3713db0e1761ee440a4334b4daf2c9694ed70bde2bf7719f4dcda`;
  test `tests/test_mcc_sandbox_launcher.py` SHA-256
  `70f2e51f6f3dfd644362d0c0876c3ac12911884b25873d6d9580d25d19ec4b8c`.
  The legacy regular ELF lacks explicit native-test support. Its `--launch-game`
  compatibility route requires native capability, no frontend and Play selecting
  exact Lockout; newer supported runtimes use explicit native selection. These
  are synthetic admission/routing proofs, not installed-wrapper or actual
  launcher-through-runtime acceptance. Evidence:
  [Shortcut admission](../work/sandbox-primary-v15/mcc-shortcut-admission-v530.json).
- [x] Verify the selected runtime's bounded MCC asset-source check without
  `--gameplay-xbe`: exit 0, six hidden muted Lockout frames, eight ticks, one
  player/one connected. Successful-open tracing records 1,696 paths, zero
  forbidden Xbox/XBE/ISO/Vista paths, zero unknown assets and zero unmatched
  traces. Owners remain unchanged; scratch `halo2-test-2093lm9c` is removed.
  Report SHA-256
  `edc1cc7ec4533950070fbef5036aba26b65dee1ffaf51cb862a21cd0fc0243cb`.
  This proves the bounded direct runtime asset source only; actual installed
  shortcut-through-runtime, visible/unmuted/audio, all-map, selected frontend,
  original presentation and full parity acceptance remain separate and open.
  [MCC runtime file opens](../work/sandbox-primary-v15/selected-mcc-runtime-file-opens-v530.json).
- [x] Install and verify the bounded root shortcut delivery. `Halo2-Sandbox-Test`
  now selects the MCC helper; the actual GUI label is `MCC sandbox test  ›`.
  Installed `--check` passes with cache v13, native capability true and frontend
  capability false. All seven installed CLI guard/info cases pass: Xbox/XBE /
  Vista/map/runtime/installation overrides are rejected and `--menu` refuses the
  unqualified frontend. Existing real Tk launcher regression tests pass 18/18.
  Eight protected files, including the regular launcher, original r6, selected
  MCC runtime and cache provenance, remain exact; test scratch root is empty and
  no new large artifact is retained. Delivery receipt SHA-256
  `2725075b6d56d4930b7c7f7ca40f15c95ff7126fed1d3fb54ec84c148f9c9736`
  (5,270 bytes). The earlier direct six-frame asset-source check retains its
  limits: this delivery does not prove installed-shortcut-through-runtime,
  visible/unmuted/audio, all-map, selected-menu or full parity acceptance.
  All 14 whole-game gates remain open, 0/102 sandbox elements accepted.
  [MCC shortcut delivery](../work/sandbox-primary-v15/mcc-sandbox-shortcut-delivery-v530.json).


### v531 MCC menu catalogue component (2026-10-19)

- [x] Replace the twelve-name menu filter with authored catalogue identity, without changing MenuState layout. The standalone production CMake test passes 12 frozen original metadata rows and 25 literal MCC rows, preserving sort/unlock, preview/launch identity and exact scenario leaves. Two blank-title DLC placeholders remain explicitly unqualified; no alias or full map acceptance is claimed. Private CMake scratch is removed. [Component receipt](../work/sandbox-primary-v15/mcc-menu-catalog-component-v531.json).
- [x] Verify the changed MenuState through the actual imported MCC Cache and compatible native link; completed by v532 below. Renderer/launcher delivery remains its separate unchecked gate.


### v532 actual MCC catalogue/MenuState integration (2026-10-19)

- [x] Verify actual MCC Cache → native profile/sign-in → Split Screen → map list → focused preview → accepted lobby identity → selected-cache launch routing. The native CPU test passes 48 previews and 40 launch/header routes across original and reversed 25-record catalogues; four blank-title and four existing energy-weapon rejections remain explicit. All316 parent link inputs and230 frozen dependency files match before/after. No Xbox/XBE/ISO/Vista assets are opened; owned scratch is removed and no executable or raw trace is retained. [Integration receipt](../work/sandbox-primary-v15/mcc-map-picker-integration-v532.json), SHA-256 `4f983c985d3fba34f833cd04821340e82bd54cfb216dcc0dd340750e8887178c`.
- [x] Deliver the basic frontend and expanded raw catalogue through the selected renderer/launcher. V24's actual818-frame menu/map-picker result and F24's actual installed Play-button/title handoff qualify this bounded delivery; see v574/v585. Gameplay, complete menus, pause, source session return and full map parity remain open.
- [ ] Publish the two qualified DLC catalogue records/previews and instant-charge profiles coherently to a separate prepared MCC cache, qualify affected native paths and adopt that cache without changing unrelated inputs. The two authored identities/native CPU routes pass v544, and complete private preparation passes v552; reuse both. No highplains/highplains2 alias or full-map equivalence is inferred.


### v533 MCC source-world weapon composition (2026-10-19)

- [x] Migrate the source weapon-effects fixture and its three active registrations to selected MCC/native construction, rejecting executable arguments and non-MCC headers. Seven Python admission/routing controls, three actual native rejection controls and the focused production CTest pass. Ordinary native MCC SMG input produces60→53 rounds, seven accepted positive effect requests and12 contact effects with no parallel adapter shots. All316 parent providers remain exact, successful-open tracing finds no Xbox/XBE/ISO/Vista inputs, and compiler/native scratch is removed. [Frozen migration receipt](../work/sandbox-primary-v15/mcc-source-weapon-migration-v533.json), SHA-256 `5bd70c61198ad372462c91f23a915d82b1e4d36c8394a2e4ddff3dd2eaffe93b`.
- [x] Complete the bounded original plasma-pistol charge/release lifecycle.
  The historical104698/45F8B4 frontiers are resolved by their authenticated
  existing program/interface owners. C48's matched client now passes the strict
  240-tick instant damage/player-response/allocation/natural-retirement/held/
  ready/release/projectile fixture with the0.15 debit and two contact effects.
  The unchanged original caller's separate final-state comparison also passes;
  no gameplay result or persistent particle graph is supplied. Full weapon/
  render/audio parity and actual launcher delivery remain separate gates.
  [Original effect contract](WEAPONS_TU15_PARITY_GOAL.md),
  [Integrated lifecycle](../.local/mcc-gameplay-continuation-20261020/combined-charge-c48-v1.json).


### v535 complete CTF/Assault initializer closure (2026-10-19)

- [x] Port the complete original initializer and marker/query/region owners (`249460`, `24CE00`, `24CEF0`, `1A8850`). Independent execution of all three original versions passes 879 calls, normalized to 293 cases; optimized native and ASan/UBSan comparisons match the full 520-byte bank and conditional outputs. Five cases use authored marker data from imported MCC maps. The production CMake registration passes through `tools/run_tests.py`; its private build and oracle/compiler scratch are removed. [Milestone receipt](../work/sandbox-primary-v15/ctf-initializer-milestone-v535.json).
- [ ] Compose the original activation/object-birth, registry and factory owners with the real selected native world, then complete CTF/Assault rules and lifecycle acceptance. The initializer does not admit either mode or establish live gameplay, presentation, LAN or launcher delivery. Three unreachable zero-distance replacement instructions remain outside executed coverage. All 14 whole-game gates and all 102 complete-element acceptances remain open.


### v541 physics branch-owner compile fix (2026-10-19)

- [x] Fix three invalid memory-API calls in the new physics branch-data owner, preserving the authenticated table payload. Focused production CMake validation passes installation/idempotence, partial-owner retention and four atomic rejection cases, including an uncommitted reserved owner. Private build scratch is removed. [Adapter receipt](../work/sandbox-primary-v15/physics-branch-owner-adapter-v541.json).
The dependent actual MCC charge/release lifecycle remains tracked at v533 above. This compile/ownership fix supplies no runtime result, gameplay rule change, selected-runtime installation or full parity acceptance.


### v542 complete weapon program tables and MCC charge frontier (2026-10-19)

- [x] Restore all five bounded dispatch tables of the complete original `1030F0` owner: 26 words/104 bytes, including eight newly missing words. All 51 independent original guarded-selector cases and the coherent native cold-factory comparison pass. The original guards, instructions and branch targets are unchanged. The separate current-header cold executable also passes; its aggregate receipt correctly records concurrent source drift and does not qualify the subsequently changed workspace.
- [x] Verify the coherent source-world build against actual imported MCC inputs. Ordinary SMG firing passes again (60→53 rounds, seven positive effects, 11 contact effects), with all 316 parent providers unchanged and no Xbox/XBE/ISO/Vista runtime reads. The strict plasma-pistol case now passes the former `104698` failure, reaches charge/release and 141 effect frames, then fails at original `2F8B27` reading vtable slot `45F8B4`. No positive charge graph was observed. All owned build/runtime scratch is removed. [Progress receipt](../work/sandbox-primary-v15/mcc-weapon-program-tables-progress-v542.json).

The old v533 persistent-graph expectation was disproved by the authenticated
original instant-effect contract. Its whole-qualified preparation and bounded
native240-tick lifecycle now pass in the compatible C48 provider group, as
recorded in the current execution slice. Failed historical receipts remain;
full weapon/render/audio parity and installed successor acceptance stay separate.


### v544 authored Tombstone/Desolation catalogue and native menu routing (2026-10-19)

- [x] Resolve both inherited blank MCC menu records from their complete authored scenario Level Data: Tombstone1402/order1500 and Desolation1400/order1400. Preserve destination bitmap identity and registration; redirect only their exact owned MCC preview partitions. The six planned stores total6,360 bytes. Eleven focused conversion controls and actual v13 reopen/idempotence/unrelated-byte checks pass. Offline converted-tag validation also finds both authored MCC preview pixel banks identical to the current Xbox LVL reference previews. [Conversion receipt](../work/sandbox-primary-v15/mcc-dlc-catalog-conversion-v533.json).
- [x] Verify the converted catalogue through actual native MCC Cache/MenuState construction: 48 previews, 44 selected-cache launch/header routes, zero blank-record rejections and four preserved energy-combat gates, including reversed authored ordering. Both new identities, localized metadata, team counts, exact MCC scenario paths and destination preview owners are checked. All316 parent link inputs remain exact, no Xbox/XBE/ISO/Vista runtime inputs are opened, and the scratch mainmenu/object/executable/profile tree is removed. The selected cache remains unchanged. [Native integration](../work/sandbox-primary-v15/mcc-map-picker-converted-integration-v544.json), SHA-256 `5ff9750cbaef09c81669a679cebf54ee20594beb8d45251d9c54099dc71d5745`.
- [x] Qualify the fresh-preparation hook, strict final DDS inventory and complete menu-asset assembly; completed by v550/v552 below. Coherent selected frontend/launcher delivery remains the separate v532 gate. CPU routing and offline preview-tag validation do not prove displayed native pixels, gameplay, transitions or complete map parity.


### v550 strict preparation inventory and original instant-charge profile (2026-10-19)

- [x] Qualify both converted preview pages through the actual full MCC inventory: all648 page owners pass,646 remain exactly unchanged, and the two authored replacements produce563 unique DDS pages. The default stock verifier rejects the changed pages; only the complete authenticated projection admits them. Original stock digests/comparisons remain recorded. [Inventory qualification](../work/sandbox-primary-v15/mcc-catalog-inventory-v534.json).
- [x] Integrate catalogue projection before private bitmap extraction, preserve all six stores/counters, record the post-serialization mainmenu hash, and pass the projection into strict final menu inventory. Production CMake preparation/conversion/inventory tests pass; a further preparation/instant-charge registration run passes after the new scalar hook. Both private builds are removed. [Catalogue CMake](../work/sandbox-primary-v15/mcc-catalog-preparation-cmake-v547/report.json), [charge-profile CMake](../work/sandbox-primary-v15/mcc-instant-charge-preparation-cmake-v550/report.json).
- [x] Validate the named instant-charge effect's complete160-byte source tuple, root marker and salted damage reference before projecting only its stored flag word2→6 to the original raw tag value. Eight profile controls and a real selected-MCC memory-only plan/idempotence check pass. Unknown custom definitions stay unchanged; this is no generic effect-bit heuristic. The current-header cold factory also passes the complete reached contact method tables. [Frozen source checkpoint](../work/sandbox-primary-v15/mcc-instant-charge-source-checkpoint-v549.json).

The copied-MCC preparation/bitmap/menu-asset pipeline subsequently passed v552. Actual converted-MCC instant charge remains the v533 gate; the selected cache/runtime remains unchanged. Retained idle-loop graph acceptance stays a separate task for an actor authored to emit that graph.


### v551 native CTF/Assault activation and registry owners (2026-10-19)

- [x] Port25 complete TU1.5 initializer/first-activation/registry owners and wire their exact entry predicates into the real World dispatcher source. Independent original/native optimized and ASan/UBSan comparisons pass278 cases:209 complete returns and69 exact unsupplied-owner stops, with1,180/1,459 production instruction sites observed. The isolated production CMake test passes with strict FP. Actual object factories, existing registry services and dirty ownership remain genuine external native owners. [Milestone receipt](../work/sandbox-primary-v15/ctf-activation-milestone-v551.json).

Real same-bank MCC factory birth, remaining CTF/Assault rules and complete lifecycle acceptance remain the unchecked v535 gate. The full World object was not rebuilt or live-qualified by the isolated test; mode admission is unchanged. FP oracle status exclusions are documented. Required text regression fixture `tests/fixtures/ctf_activation_original_tu15.json` is1,097,045 bytes; compiler/test scratch is removed, with no retained executable, raw runtime bank or capture.


### v552 actual43-map private preparation and final menu assembly (2026-10-19)

- [x] Run the real preparation path on private copies of all43 selected MCC maps, texture bank and fonts. All25 multiplayer maps are processed;1,045 ordered stores include only20 changed blocks: six complete catalogue/resource stores and14 qualified instant-charge flag words. The distinct Floodlab effect tuple remains unchanged and explicitly unqualified. Strict inventory and final asset assembly pass648 frame owners,563 unique DDS pages and40 font files. All selected assets, registry/provenance, source files and launcher owners remain exact. The4,332,372,016 generated bytes are removed with their owned scratch. [Preparation receipt](../work/sandbox-primary-v15/mcc-private-preparation-milestone-v552.json).

The combined native step did not run: its factory copy caught a concurrent extra-brace syntax error at line361, subsequently corrected in the live source. The aggregate failure remains preserved. Repeat only the native stage against fresh owned MCC copies; do not repeat this passing43-map/bitmap/menu preparation. Actual instant-charge acceptance remains the v533 gate, and selected frontend/renderer delivery remains v532.


### v555 actual prepared-MCC instant-charge caller boundary (2026-10-19)

- [x] Compile/link the corrected genuine factory with all316 retained providers unchanged, then test private MCC core copies with the qualified instant-effect conversion. The actual effective flags6/one-event/one-jpt/no-particle contract passes; SMG firing and both native input-open audits pass. No43-map/bitmap/menu preparation is repeated. All selected/frozen input guards pass and owned scratch is removed. [Native caller receipt](../work/sandbox-primary-v15/mcc-instant-charge-caller-frontier-v555.json).

The charge fixture stops at its first input tick80, original108E26 storing NONE, before any required positive damage allocation/player-response observation. Its whole108DB0 caller/control context and observer coverage are now the unresolved boundary; this is not yet a confirmed gameplay defect. Preserve that actual result and compare the complete original caller before changing expectations. The v533 charge gate remains open.


### v558 Floodlab conversion and actual CTF constructor frontier (2026-10-19)

- [x] Qualify the separate164-byte Floodlab instant-charge tuple with its authored inline root marker and complete damage graph. Fourteen focused controls, actual MCC scratch projection, production v13 serialization, independent reopen, idempotence and the registered production CMake test pass. Only the authenticated four-byte flags word changes2→6. All selected/source owners remain exact; both private scratch trees are removed. This resolves the distinct tuple preserved by v552 without repeating its passing43-map preparation. [Milestone receipt](../work/sandbox-primary-v15/mcc-floodlab-and-ctf-frontier-v558.json).

The first actual current-World MCC CTF probe compiles/links but stops during the original contrail constructor11E550/11E55D at missing backing505E90, before CTF activation. Its698-path open audit passes with no forbidden, unknown or unmatched inputs. The complete destination already has a genuine current scalar-state owner; the stale archive provider must be refreshed. Preserve the failed run and retry that dependency. No CTF birth, charge lifecycle, selected frontend or full parity gate is accepted; v533/v535/v532 remain open. No large artifact is retained.


### v559 reuse the passed corrected-menu native build (2026-10-19)

- [x] Independently review the existing V23 native menu result and preserved compiled owners. The native run exits0 after412 title/profile/keyboard/main/Settings/back frames, with finite nonzero title/menu audio; its real-input Lockout regression also exits0. Root visually inspected the actual640x480 profile and Settings frames: complete SELECT/BACK artwork and captions fit. All retained file and reference hashes match the sealed receipt; prior scratch is removed. No native test or capture is repeated. [Existing native evidence review](../work/sandbox-primary-v15/mcc-preserved-menu-native-review-v559.json).

Prepare the actual ordinary Play handoff using this coherent successor while preserving the selected cameraV3 gameplay action, runtime and shaders. Check whether its menu catalogue needs the already qualified current MenuState/content and converted data. Selected frontend delivery remains v532; native widescreen frames, complete menus/maps, original framebuffer/audio parity and full-game acceptance remain open. The118,332,732-byte compiled/reference input cohort and180,872-byte two-PNG proof are existing required retained inputs, not newly generated test outputs.


### v560 actual installed MCC sandbox route (2026-10-19)

- [x] Verify the installed shortcut through the real regular launcher and exact selected cameraV3 native runtime. It exits0 after six hidden muted MCC Lockout frames, seven ticks and one player. The1,974-open/28-trace audit has zero forbidden, unmatched or remaining unknown inputs. Only the two exact save paths are reclassified using the recorded namespace bind and independently verified scratch backing. The original mechanical-red receipt is preserved; no native rerun or broad audit exception is used. [Installed route milestone](../work/sandbox-primary-v15/mcc-installed-shortcut-milestone-v560.json).

Protected file hashes/device/inode/size/mtime, full selected-cache stable metadata, actual selection and installed saves remain unchanged. Concurrent inode ctime changes are explicitly recorded; no unchanged-ctime claim is made. Both owned scratch trees are removed and no large artifact is retained. This closes the earlier shortcut-through-runtime uncertainty only. Visible/unmuted/audio, complete maps, ordinary Play frontend delivery and full parity remain separate open gates.


### v566 actual MCC current-World initialization (2026-10-19)

- [x] Compose the genuine current World and complete updated cold-data providers with the compatible native cohort. All26 translation units compile/link; actual MCC Lockout world construction and map initialization return. The698-path input-open audit passes with no forbidden, unknown or unmatched inputs. Frozen source, all316 retained providers, selected cache/runtime and current-workspace guards pass; scratch is removed with no large artifact retained. [Initialization checkpoint](../work/sandbox-primary-v15/ctf-activation-port-20261019/current-world-initialization-checkpoint.json).

CTF session configuration now reaches original159FF0/15A0EA and requires its real static engine object48BFA0/interface466190. Configuration, activation and object birth have not completed. Audit and restore that complete authored object/interface/registration owner before continuing; no ready-state seeding or mode admission. The v535 lifecycle and all whole-game gates remain open.


### v573 compiled independent launcher routes and complete CTF static data (2026-10-19)

- [x] Implement and production-test independent frontend selection while keeping the native gameplay runtime unchanged. Six focused CTests pass, including five actual compiled-launcher routing cases, eight selection-admission cases,14 shortcut cases and18 real launcher UI cases. Missing selection preserves prior behavior; stale explicit selection blocks ordinary Play while the native test remains available. Newly compiled normal/title launchers both retain the exact cameraV3 native owner; no selection or installed launcher is changed. [Combined milestone](../work/sandbox-primary-v15/mcc-separated-launcher-and-ctf-milestone-v573.json).
- [x] Restore the authenticated complete CTF/Assault static object/interface/publication destination:212 bytes, all51 original callback identities, correct original sections/access, duplicate/partial/reserved-owner rejection and adjacent-owner preservation. Optimized and sanitized component comparisons pass69 cases; the registered production CMake test passes. The real factory now mounts this data before imported tags, with existing source activation/callback owners unchanged. Actual configured activation and object birth remain the v535 gate.

The initial launcher target's unnecessary native dependency build was canceled before tests; its229,970,142 generated bytes are removed. The launcher-only configuration builds the actual routing binaries without a native rebuild, preserving normal source-build defaults. Its179,704,999-byte private tree is also removed after retaining552,190 bytes of requested working launcher inputs/frozen source. Source-only reproduction cohorts for the current World and charge test are retained separately; no raw bank or disposable engine binary is retained. Actual ordinary Play publication, expanded native map-picker adoption and full parity remain open.


### v574 actual expanded MCC map picker and bounded cold-table correction (2026-10-19)

- [x] Adopt the qualified catalogue MenuState over the preserved V23 client, reusing its identical qualified content producer. The coherent325-provider V24 run exits0 after818 native frontend frames: real title/profile/main/Split Screen/lobby/setup/map-picker input,15 Down inputs and return. Root visually reviewed the actual Backwash selection, authored preview/description and surrounding DLC rows. Selected MCC cache, parent inputs and frozen dependencies stay exact; no forbidden asset input is opened. [Actual native picker](../work/sandbox-primary-v15/mcc-expanded-menu-native-v572/report.json).
- [x] Correct the cold weapon-table unit's owner-label assumption for the two already-authenticated185111 interface words. The native unit passes26 original words/104 bytes,51 guarded selectors and10 contact methods, retaining exact bytes/read-only/unique-owner/write-rejection checks. All316 providers and frozen inputs remain exact. [Cold unit result](../work/sandbox-primary-v15/test-cold-weapon-ownership-v571.json).

V24 retains111,479,920 bytes of requested executable/object inputs plus its small frozen source/cohort and102,933-byte required picker reference; both native-test scratch and cold-test scratch are removed. The charge diagnostic was stopped after automatic review flagged possible cybersecurity risk; the completed follow-up ran ordinary native unit tests only. Charge remains unaccepted. Two unconverted blank catalogue identities remain gated, as do energy-combat paths; native widescreen, map gameplay, ordinary Play installation and full parity are not accepted by the map-picker run. Reuse this V24 result for staging; do not repeat the passing menu flow.


### v576 final launcher consistency and CTF configuration owners (2026-10-19)

- [x] Match launcher status and controls to actual stale-selector behavior. Three focused production CTests pass: five compiled routing cases,22 real Tk UI cases and576 original/native CTF configuration cases. A changed explicit menu build disables only ordinary Play and reports that condition accurately; native test, sandbox and preview remain independent. Full-ready valid frontend selection and missing-selection fallback remain intact. Both newly compiled launchers preserve the exact cameraV3 native owner. [Milestone](../work/sandbox-primary-v15/mcc-frontend-and-ctf-configuration-milestone-v576.json).
- [x] Port and wire complete original24B390/249440 configuration owners: all18 instruction sites and576 cases compare full defined ABI, FP state and banks with optimized and sanitized native execution. The real MCC session now passes its class dispatch and249460 initializer, then reaches further callback24A6C0. Configuration, objective birth and lifecycle acceptance remain open. The old25-owner activation module stays unchanged.

The successful private CMake tree is removed after retaining the requested small normal/title launcher successors and frozen source. No selector, root launcher, selected cache or runtime is installed by this checkpoint. The2,745,863-byte declared synthetic CTF configuration fixture is a required regression input, not a captured runtime bank. V24's passing native menu proof is reused for the pending frontend staging/delivery.


### v577 qualified isolated frontend staging (2026-10-19)

- [x] Stage and independently recheck the passed V24 frontend with its own97-file shader bank and complete52-resource MCC capability. The99-file F24 owner totals107,082,951 bytes. Only22 bytes inside the authenticated64-byte read-only shader-path field change; game instructions remain exact. The existing capability checker and every retained-file hash/size pass. All protected325 inputs, old cameraV3 runtime/106 shaders, installed launchers, selected cache/registry and absent-selector state remain unchanged. [Stage receipt](../work/sandbox-primary-v15/mcc-frontend-stage-v573.json).

Owned staging scratch is removed. F24 is an explicitly retained requested working runtime; this filesystem milestone starts no game and publishes no selector. Actual normal/title launcher adoption and installed Play→native menu acceptance are the next separate delivery step. Basic menus/raw expanded catalogue only: two DLC placeholders, unfinished gameplay and full parity remain open.


### v579 installed independent MCC title/menu route (2026-10-19)

- [x] Install the qualified normal/title launcher pair and independent F24 frontend selection. Both actual installed `--launcher-info` calls identify cameraV3 for gameplay and F24 for Play/title/menus. The installed MCC sandbox `--check` passes both capabilities and still selects the exact original cameraV3 native-test owner. Selected cache/registry/provenance, sandbox wrapper, old runtime and all106 old shaders remain unchanged. Both prior launchers are retained under the explicit rollback owner. [Installation receipt](../work/sandbox-primary-v15/mcc-frontend-install-v578.json).

This is local route installation/admission only; no game is executed by the installer. The actual installed six-frame CLI route and actual GUI Play-button→title handoff are being tested separately. F24's earlier818-frame menu/map-picker proof is reused. Full menus, two converted DLC catalogue records, all-map gameplay and full parity remain open. Installer scratch is removed; working installed launchers and436,224-byte rollback launcher pair are explicitly retained.


### v581 complete CTF configuration and genuine tail composition (2026-10-19)

- [x] Extend the qualified configuration module with complete original24A6C0 and its genuine existing164930 tail owner. Optimized and ASan/UBSan execution each pass864 original/native full-ABI, defined-flags, FP-state, bank, store and trace comparisons; all26 production sites are observed. The focused production CMake test passes with the real activation module linked. Its old25-owner source remains exact;23/24 tail sites are reachable under the original fixedECX=-1 argument, with the independent whole-owner proof preserved. [Milestone](../work/sandbox-primary-v15/ctf-three-configuration-milestone-v581.json).

All compiler/oracle/CMake scratch is removed. The retained fixture is declared synthetic regression evidence, never a runtime bank. Actual MCC session configuration/activation, objective birth and lifecycles remain the existing v535 gate, with no mode admission or whole-element acceptance. Continue the frozen current-World MCC probe; do not repeat passing initializer/static-data/configuration component tests.


### v582 actual MCC CTF session configuration returns (2026-10-19)

- [x] Run the genuine current World and complete configuration owners against imported MCC Lockout. All29 translation units compile/link; construction, map initialization and configure_session return, including actual CTF bank publication and source engine kind1. The698-open audit has zero forbidden/unknown assets or unmatched lines. All316 parent providers, frozen/current source and MCC guards pass. [Actual configured-session checkpoint](../work/sandbox-primary-v15/ctf-activation-port-20261019/ctf-configured-session-checkpoint.json).

The next precondition remains unaccepted: the original occupied-team mask is0 because options-roster publication does not allocate the real player rows from which15ABC0 derives that mask. Use the genuine native player allocation/constructor lifecycle before first activation15AFB0; never seed the mask, fabricate handles or admit CTF/Assault early. Objective birth and complete rules remain v535. Scratch is removed; the119-file/2,346,364-byte source-only reproduction cohort is retained without binaries or raw runtime banks.


### Current per-map acceptance ledger (2026-10-19)

The [map ledger](../work/sandbox-primary-v15/mcc-map-acceptance-current.json) lists every literal imported MCC owner:15 campaign levels,25 multiplayer maps, mainmenu and two shared dependency banks. Each row keeps its provenance-derived cache hash, reusable preparation evidence, bounded native progress and unchecked lifecycle/presentation work. Dependency banks are not counted as playable levels. Import, content loading and private preparation do not accept whole maps. No test is repeated to create this ledger; G01-G14 and all102 whole-element acceptances remain open. Update the affected row when a new map lifecycle is actually qualified.


### v583 actual installed MCC CLI title route (2026-10-19)

- [x] Execute the installed regular launcher into the exact independently selected F24 runtime: title screen9 draws six hidden muted frontend frames and exits0. The1,868-open/29-trace audit has zero forbidden or unmatched inputs. Seven initially unclassified platform/SDK/native-variant paths are authenticated individually against their exact source, linked module or installed package owner, leaving zero unknowns. The original mechanical-red receipt is preserved; no CLI execution is repeated. [Exact-owner requalification](../work/sandbox-primary-v15/mcc-installed-play-owner-requalification-v582.json).

All277 originally protected owners are rechecked unchanged and both scratch trees are removed, with no retained capture/binary. The original early audit stop skipped complete-cache post-metadata comparison and did not pre-pin the five extra files; neither stronger claim is made. The fresh no-argument GUI Play proof separately guards these exact additional owners and complete cache metadata. Actual button acceptance, full menu/game lifecycles and full parity remain open.


### v585 actual installed Play-button delivery accepted (2026-10-19)

- [x] Verify ordinary no-argument root launcher → real Tk Play click → independently selected F24 → cached intro → title screen9 → clean native window close. The actual run exits0 with18 frontend frames,38 traces and2,169 successful opens; there are zero forbidden, unknown or unmatched inputs. An independent reviewer checks the actual exec/argv and platform categories. All45,123 MCC file metadata records, protected runtime/shader banks, launchers/selection, extra configuration, installed saves and profiles stay exact. [Installed Play milestone](../work/sandbox-primary-v15/mcc-installed-play-milestone-v585.json).

This completes bounded basic frontend delivery; reuse V24's earlier818-frame menu/map-picker qualification without repeating it. Complete menus/game return, converted DLC cache publication, gameplay/maps and OG/TU1.5 framebuffer/audio parity remain open. Normal startup includes cold initialization and the intro; this140.81-second proof is not a startup-performance claim. All three route-test scratch directories are removed; no new raw capture/binary is retained by the route probes. The milestone lists paths/sizes of the explicitly retained working F24/V24 inputs, compiled launchers, rollback and required picker reference.

Weapon goal checkpoint: V35 actual MCC original-source Sword strike renders22
advancing poses; owned audio remains red. V36 checks shared CRT dispatch after
a private copy preempted the complete owner. See
[weapon checklist](WEAPONS_TU15_GOAL_CHECKLIST.md). All63 weapons remain open.


### LAN/split-screen custom-mode settings and launch milestone (2026-10-28)

- [x] Verify Fiesta, Infection and Bomb Defusal normal menu settings/Starts with fixed defining rules, ordered native extensions and complete source settings; copied/reopened variants, native Defusal timer constraints, real local team selection and pointer reachability. The pinned cohort passes 11/11 focused CTests, including 429 metadata checks and 564 custom edited Starts per original/MCC catalog. All 92 stock parameters still pass 4,146 edited Starts per catalog.
- [x] Verify 12 actual MCC custom native cases with two/four local players in LAN/split-screen, authenticated loopback configuration/map/admission/snapshots, three real three-tick headless launches, and three real two-frame/two-view rendered launches. Defusal stays out of stock objective presentation and retains its custom site/rules data. Private source/build/probes were removed; no large artifacts retained.

Evidence: [custom-mode acceptance](../build/test-results/custom-mode-cohort-20261028/acceptance.json), with effective pinned hashes, declared source refreshes, failed-attempt diagnostics and final native/render reports. [Settings scope](MATCH_CUSTOMIZATIONS_LAN_SPLITSCREEN.md). Unrelated CMake/network/runtime work advanced after the source freeze; this does not qualify those concurrent edits or rebuild/promote a launcher runtime. Actual graphical LAN-client interaction, cross-machine play and full gameplay/visual/audio parity remain open. Retained stock LAN lobbies require a fresh session for these custom modes.

### October28 native campaign engine implementation

- [x] Bounded semantic component work is tracked in the [native campaign engine checklist](CAMPAIGN_NATIVE_ENGINE_GOAL_CHECKLIST.md). Its checked milestones link independent original comparisons, meaningful mutants, compiler/sanitizer checks and owned-storage cleanup. The [sixth evidence checkpoint](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-6.json) records48 passed reports; later completed components retain their own linked receipts until the next aggregate.
- [ ] Full campaign native-only engine closure remains open. Actual v5 Marine/Grunt behavior278460 and Johnson navigation1C9F10 still require complete parent composition; full HS/script scheduling, orders/action begin, speech/presentation, map/save lifecycle, co-op, missions and launcher qualification remain open. Imported tags/scripts consume native engine behavior; they do not replace missing algorithms. No selected runtime is promoted by component proof.


### Split-screen tick-zero shadow startup crash (2026-10-28)

- [x] Reproduce the exact installed-runtime 640×480 reference-guard error through the real MCC Split Screen profile/lobby/countdown/native handoff at 1280×720, with zero gameplay frames; confirm the missing nonzero-reference check in the installed binary and lock down the permanent regression.
- [ ] Qualify and select a fresh immutable runtime containing the source correction, requiring that actual unbounded menu Start to submit frames and exit cleanly, plus launcher/loading/frontend and ordinary local multiplayer checks.

[Diagnosis and regression scope](SPLITSCREEN_STARTUP_SHADOW_REFERENCE.md). Installed runtime `4bf0c86b3c41485193b431cf4f7da756` remains unchanged while delivery is pending. Private profiles/caches are disposable; no capture retention is requested.


### Native player shadows across offline, System Link and ONLINE (2026-10-28)

- [ ] Accept the current coherent Original/pStencil/Path Tracing matrix, including real raw-LAN and ONLINE host/joined native ownership, completed local-player GPU receipts, independent gameplay cadence and historical WORLD/depth consistency. The transport, recipient native owner, typed native timeline, scene/effects isolation and model-depth implementation are integrated; final linked/runtime evidence is pending. Preserve native fixed-shadow disabling for two to four local views.
- [ ] Complete the all-25 prepared-MCC-map full archive/native restore census under the actual production shadow caps and verify disposable owner cleanup. The earlier offline 35-check and pixel controls remain bounded evidence for their earlier pinned source, not acceptance of these new changes.

See [native player shadow status](PLAYER_SHADOWS_NATIVE.md). No launcher promotion is part of this work; the selected runtime and unrelated installed work remain unchanged.
