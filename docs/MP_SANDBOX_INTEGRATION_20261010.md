# Native multiplayer sandbox integration, 2026-10-10

This checkpoint integrates recovered source kernels with actual imported map,
weapon, animation, collision and sound data. The port remains incomplete.
The native triangle mover, native UDP protocol and SDL audio delivery are
explicit platform adapters; these tests do not establish complete Xbox Havok,
hardware, network wire, rendering or sound-system parity.

## Implemented live paths

- Actual posed prop collision participates in native movement and SMG traces.
  A Lockout power module takes ordinary shot damage, ages its authored pending
  response, applies its authored attenuated and occluded area-damage request,
  and removes its model and collider. Full identities and the actual emitted
  effect request survive native LAN replication. Dynamic rigid-body motion and
  blast acceleration still require their original owners.
- Stock primary SMG muzzle callbacks create retained source particle pools.
  Births use the current authored first-person model marker, actual emitter and
  sprite/material data, and the separate native visual random stream. Each local
  view receives only its own foreground particles. Retirement releases original
  pool-sized allocations. Original sprite code also proves valid zero-alpha
  fade endpoints; the runtime regression permits those while requiring visible
  flashes across multiple ticks. World/remote muzzle paths and other weapons
  remain separate work.
- Q reaches the configured grenade sandbox through actual SDL consumed input.
  The authored inventory, selected body and first-person throw clips, current
  body marker, salted projectile allocation, attachment/release, ordinary frag
  terrain contacts, fuse, area damage and actual detonation callback are live.
  The native UDP client retains the authority's action/clip and projectile state.
  Released frag/plasma geometry uses the two actual matg projectile models,
  their original singleton region/default-node poses, source geometry/shader
  LOD selection and independently retained lighting. Each salted owner shares
  resident images but receives separate draw constants. Held world visibility
  remains unproved and is explicitly outside this rendering subset; parent
  attachment is not treated as an inferred Xbox hide rule.
  Released angular physics, live unit impacts, plasma sticking and full explosion
  particle playback are incomplete.
- Authored first-person reload events, normal BSP ambient tracks and actual gait
  foot events reach SDL sound voices. Actual prop/grenade detonation callbacks
  schedule authored fixed effect sound parts. Native effect sound scheduling uses
  host elapsed time; complete original effect-clock admission, spatial mixing,
  PVS/DSP, weather alternatives, detail tracks and all remaining sound consumers
  are not established by this checkpoint.
- Ordinary fatal damage retains actual current movement velocity. A real
  moving-player SMG kill and respawn preserve the old full body datum, previous
  player, original death tick and velocity words. The removed unconditional
  reset made this test fail. The separately completed original kill-trigger
  fatal return also preserves momentum, so its native postpass reset is removed.
  A real authenticated-u5 falling Match and a native movement twin verify the
  current falling velocity and salted corpse ownership through respawn; the
  old reset fails. Normal and actual ASan/UBSan runs pass. This does not add
  dead-body motion, ragdoll solving or a fall animation, and the native kill
  predicate/bounds/scheduling remain incomplete.

## Render-rate-dependent LAN sound fix

Before this change, `LanClient::receive()` published only its newest completed
snapshot. A render frame that drained several genuinely consecutive host ticks
discarded their first-person sound intervals and reset the native gait adapter
as if those intermediate ticks had been lost on the network.

`received_states()` now exposes each fully validated, accepted snapshot from
that receive call. Runtime presentation processes their actual clocks in order
before rendering the newest state. The bank clears on every receive; malformed,
stale and duplicated packets never supply a presentation tick. Genuine missing
intervals retain their existing reset boundary. No missing gait or sound events
are synthesized, and native protocol23 is unchanged.

The real four-view UDP/SDL test delays each rendered frame by85ms while the
authority continues ticking independently. Its failing run recorded0 reload
packets and0 footstep packets. The corrected run recorded1 reload and8 footstep
packets, plus2 ambient packets and6 shot impulses. This deliberately slow
presentation check is not a performance claim or physical-speaker verification.
A second regression compares the actual authority's starting-SMG `SoundEvent`
impulses with unique client SDL voice submissions, within an explicit joined
roster/final-observed-snapshot interval. The old latest-only firing loop delivered
5 of10 and6 of14 authored impulses in two immutable failing runs. Firing sounds
now enter the frame's pending sound bank for every fully accepted snapshot.
The corrected isolated X11 run delivered all10 of10, one reload, two ambient
packets and nine footsteps. An earlier corrected run delivered14 of14 shots
but failed its separate reload-input requirement; that failed log remains
preserved. The isolated run uses Xvfb with the actual NVIDIA Vulkan device and
SDL dummy audio callback, avoiding another desktop window stealing OS focus.
It proves actual voice submission and snapshot draining, not physical speakers,
recovery of genuinely missing network snapshots or original Xbox networking.

The separate real late-join scenario keeps an existing host player neutral after
an actual prejoin SMG shot, joins the SDL client, then allows ordinary postjoin
fire only after the observed baseline. Its immutable failing run replayed one
stale shot and delivered all10 new shots. A newly observed connected full player
identity now establishes its retained-shot baseline; a respawn's changed unit
does not restart player identity. The fixed delivery run submits zero stale sounds and all10 actual postjoin
impulses, with no unobserved host tail. The same build's slow four-view run
submits all10 host impulses, one reload, four ambient packets and nine footsteps.
These are separate checks of lifetime baseline and multiple-snapshot draining.
They do not claim reconnect coverage or an original Xbox wire policy.

The separate real UDP test retains three completed consecutive snapshots in
one receive and verifies every local view's original animation interval. Empty
receives do not replay an interval. Its16-player/4-view source-clock, fragmentation,
reorder, duplicate, drop, late recovery and30/60/120/144/240Hz checks also pass.

## Launcher update selection

The ordinary launchers previously selected the base disc executable, while the
source-backed runtime tests explicitly selected the supplied Title Update1.5.
The ignored `assets/gameplay-source.tsv` now records a separately selected
external executable. `tools/select_gameplay_source.py` validates the supplied
update's complete SHA-256 before atomic selection; unknown images leave the
prior selection untouched. Both launchers forward it through the title/menu
route into multiplayer. Explicit `--gameplay-xbe` overrides remain available;
a missing or malformed saved selection fails instead of silently reverting.

The actual launcher-recorder regression failed eight selection/fallback cases
before the fix and passes all66 cases afterward. The refreshed title launcher
reports the verified supplied1.5 path and renders the real title screen. These
checks prove routing and bounded startup, not complete1.5 gameplay parity.

## Observed validation

- Normal and ASan/UBSan source-backed prop tests: actual Lockout module collision,
  shot/destruction/removal, original section snapshots, LAN idempotence and
  collision removal. All21 Xbox/Map Pack maps import and step390 actual props.
- Normal and ASan/UBSan audio tests: ordinary reload/ambient data on all23 available
  maps,154 original foot selectors and4,838 original ambient selectors, with
  independently repeated source results.
- Normal and ASan/UBSan grenade delivery:454 actual SDL/UDP checks. The independent
  timer-prefix comparison checks540 original cases and281,880 complete output
  bytes; it stops before the original world/contact owner.
- Real X11/Vulkan Q probes pass single/four views in local and native LAN matches,
  including actual rendered authored first-person/body throws and inventory.
- Rebuilt real X11/Vulkan stock SMG local-four and LAN-four probes pass actual
  particle submission, view ownership, ammunition use and expiry.600 independent
  muzzle births/expiry/reallocation lifetimes pass the production owner test.
- A real LAN SMG destroys an actual module; the client receives its real removal
  and submits its authored explosion sound exactly once.
- Eight independently repeated original grenade pose/region/bounds controls
  match all node/sphere words. Shared model templates match twenty independent
  object/view geometry selections. The scene rejects attached, duplicate,
  invalid, conflicting and out-of-capacity owners without a smaller pool cap.
- Real Vulkan draws of two same-material grenades match independently bound
  source shader constants within one pixel level in single/four views through
  resize, descriptor storage growth, retirement and recreation. Swapping the
  owners' lights changes over20,000 components, ruling out equal blank captures.
- The isolated physics0 controller matches1,080 original cases,138,240 complete
  output bytes and1,200 ordered stores in normal and ASan/UBSan runs. It is not
  yet a complete world-effect runtime or a substitute for collider1.

The named captures/logs and precise source/binary hashes are retained under
`analysis/`; frozen manifests from the independently owned source checkpoints
remain immutable. Some earlier rendered audio evidence predates protocol23;
the rebuilt LAN-four and slow-client logs explicitly identify the later runs.
Vulkan validation layers are unavailable on this host. Four-view full-refresh
performance, physical LAN conditions and complete map/gameplay coverage remain
unverified.

## Next source owners

Complete multi-event world explosion systems require their actual controller
dispatch, including collision-dependent particle movement and classification8.
The original canister now passes visibility using its genuine player/PVS
producer and source-created breakable-mask storage. Its effect/location remain
allocated and its clock reaches event1. Execution then stops at the first
required decal part. The separately measured vertex-cache and texture-cache
constructors return using real shared platform allocations. The selected actual
async queue prefix and real native synchronization let the required texture
request publish, with its done byte still zero: the decal remains pending.
The effect reaches an absent source sound-system record; the separate original
worker reaches a zero file token. No IO, ready flag, decal or particle birth is
supplied. The texture destination's inherited whole-file mapping overlap remains
an explicit ownership boundary. Neither this composition nor physics0 provides
complete world explosion particle playback or bypasses the remaining owners.
The real Spartan corpse requires the ten rigid bodies and nine authored
constraints, its original death/body rebuild, physical-first node delivery,
and an independently retained corpse pose/lifetime in native LAN/presentation.
The measured original constructor creates all ten bodies and nine constraints
as an explicit callee root, without proving natural ragdoll admission or world
insertion. The new production body-matrix leaf matches all ten complete original
calls and ordered bytes, with its unmeasured component correction branch still
an explicit required service. Neither checkpoint provides a live solved corpse.
The original kill-trigger experiment uses an explicitly declared diagnostic
placement; the original position/bounds producers must admit its actual current
bounds center before the fatal call. A separate selected source sound-storage
and current-ugh prefix now lets the held original fatal call return completely,
with unavailable playback and unchanged velocity. The natural sound-system/map
startup is still blocked; this is not a naturally falling original frame loop.
Its complete velocity rule supports the narrow native reset removal, while the
native bounds-center producer, rotating kill-volume selection, distance-damage
request and attribution remain separate work.
The original `landing_dead` clip is conditional; the measured fatal-tail call
passes its forced-landing flag as zero. It is not a universal fatal or
fall-out-of-world animation. Gravity-lift phantom admission and body-force
delivery are also incomplete. None of these are replaced with guessed impulses,
generic particles, a standing idle corpse or a selected-by-name death clip.

Source checkpoints:

- [Prop collision and destruction](MP_LIVE_PROP_SANDBOX_RESEARCH.md)
- [Grenade bridge and limits](MP_NATIVE_GRENADE_SANDBOX_RESEARCH.md)
- [Authored audio and gait](MP_LIVE_AUTHORED_AUDIO_INTEGRATION.md)
- [Explosion particle admission](MP_PROP_EXPLOSION_PARTICLE_ADMISSION.md)
- [Death and ragdoll contracts](MP_DEATH_RAGDOLL_INTEGRATION_RESEARCH.md)
- [Moving death and retained momentum](MP_SPARTAN_MOVING_DEATH_RESEARCH.md)
- [Original ragdoll constructor](MP_SPARTAN_RAGDOLL_CONSTRUCTOR_RESEARCH.md)
- [Current-body matrix consumer](MP_SPARTAN_BODY_MATRIX_RESEARCH.md)
- [Canister effect owners](MP_CANISTER_PARTICLE_OWNER_RESEARCH.md)
- [Canister vertex-cache construction](MP_CANISTER_VERTEX_CACHE_RESEARCH.md)
- [Canister texture-cache and IO boundary](MP_CANISTER_TEXTURE_CACHE_RESEARCH.md)
- [Particle pool label correction](MP_PARTICLE_POOL_LABEL_CORRECTION.md)
- [Kill-trigger source path](MP_SPARTAN_KILL_TRIGGER_RESEARCH.md)
- [Complete held kill-trigger return](MP_SPARTAN_KILL_TRIGGER_SOUND_RESEARCH.md)
- [Canister async queue and genuine stops](MP_CANISTER_ASYNC_QUEUE_RESEARCH.md)
- [Released grenade scene](MP_GRENADE_SCENE_RESEARCH.md)
- [Particle physics controller](MP_PARTICLE_PHYSICS_MOVEMENT_RESEARCH.md)
