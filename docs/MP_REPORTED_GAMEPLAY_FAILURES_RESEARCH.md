# Reported canister, reload, grenade and sound failures

The current launcher has two reproduced delivery gaps: Q reaches the keyboard
binding but is discarded before gameplay, and the registered first-person
animation clock has no sound-event consumer. The stock Lockout probe starts
with two frag grenades and zero plasma grenades. Missing grenade inventory
therefore does not explain that probe's Q failure. The screenshot's exact map
and running inventory were not recovered from the image.

This checkpoint connects the real local first-person clock to an explicit
event owner and corrects the sound resource size mask. It also verifies every
resident sound permutation in all 21 imported multiplayer maps. The full
throw, live sound request/playback and canister collision/explosion owners are
still being integrated; these checks do not establish those gameplay fixes.

## Current-launcher reproduction

The isolated probe links the immutable `build-kbm-20261008-221100` Cache and
Systems archives with their matching source headers. It uses the actual owned
Lockout cache, selected u5 executable, stock Slayer rules, one naturally joined
player and its registered first-person binding. It does not relocate the unit,
remove collision, inject a target/body or assign an animation result.

After firing, a normal R tap at tick60 starts authored activity `0B000065`.
The clock emits reload sound `E5AD0438` at tick62. Its full registered clock
states match a read-only event observer. The production sound queue contains
23 shot events and zero reload events. This isolates the missing event
delivery from clip availability and reload admission. Earlier R at tick45
was rejected while the original fire clock remained active; that ordinary
rejection is not used as the reload reproduction.

The actual reload resource decodes to 73,422 stereo frames at 48kHz and the
real offline SDL mixer creates a private host voice and consumes it. The probe
uses `SDL_AUDIODRIVER=dummy`; it does not establish audible physical output,
original voice admission, listener gain or scheduler parity. Original source
datums and private host voice tokens remain distinct.

The probe and log are retained under
`analysis/multiplayer/integration/reported-failures-20261008/native-probe*`.
The matching original starting-grenade case remains in the immutable
`inventory-source/grenades.json` evidence: equipmentflags0, enabled,
maximum-one false and stock weapon set produce `[2,0]`.

## Production event delivery and retained animation metadata

`MultiplayerMatch::configure_first_person_event_owner` supplies the actual
local view/hand binding and original authored event at the registered clock's
callback phase. It rejects an absent provider. Remote presentation clocks
emit no duplicate local events. The consumer still owns original camera,
object/marker/location, audible-type, sound request, selection, admission and
playback logic; no generic play-first-permutation callback is installed.

The original FP suite retains all541 ticks and2705 phase states. The actual
Match cases retain516 ticks and deliver both expected local events with the
actual full unit/weapon/graph identities and original20-byte event packets.
The existing presentation checks retain533 continuous pairs, eight cuts and
8313 intermediate samples across30/60/120/144/240Hz. Event delivery does not
move gameplay clocks or animation events to the render frame rate.

`UnitAnimationMetadataStore` retains its owned Cache and complete read-only
graph backing through provider lifetimes. Lazy definition imports keep stable
references. It compares58 original graph payloads and3372 original definition
queries, checks graph boundaries and absent/escaped inputs, and releases the
Cache after the final callback is released. Mutable resource descriptors,
source caches/queues, shared RNG and active unit channels have separate owners.

`OriginalData` imports five literal animation operands from verified1.1 and
u5 images: one, zero, frame subtractor, RNG scale and unsigned-conversion
correction. Base-image addresses remain unrecovered, so the optional operands
are absent for that image. Both verified imports compare with independently
observed words; no fallback constants or seed are supplied.

## Sound resources on every imported multiplayer map

The native sound loader used a lower24-bit chunk size. Complete original
`21ABD0` masks only the top two bits, retaining the lower30 bits. The corrected
loader uses `0x3FFFFFFF`. A unique complete18-instruction body correspondence
maps1.1 `21ABD0` to u5 `21F4D0`, including its internal branch and relocated
chunk-bank operand. Both independent exports are identical and all36 listed
instruction rows compare directly with the original image bytes. This is
static u5 correspondence, not new u5 scheduler or driver execution.

Two independent first-person exports cover483 permutations referenced by the
authored SMG first-person graphs across21 maps. The native loader compares all
encoded bytes and metadata. Their23 distinct PC samples decode successfully
to2,979,087 stereo frames.

Two further independent exports enumerate every resident `snd!` tag, pitch
range, permutation and chunk descriptor in those21 maps. The native test reads
the actual original resource files independently of `Cache::load_sound` and
compares complete payload bytes and codec/channel/rate/class/sample metadata.
The normal run checks88,254 cases and3,334,678,947 encoded bytes from13,008
original chunk ranges. All5,197 distinct payload/format/class combinations
decode successfully to545,509,646 stereo frames; one actual payload is silent.
The normal run passes in17.12s and the GCC address/undefined/leak sanitizer
run passes in171.73s with exactly the same counters and no reported error.

The explicit PC owned-file adapter uses location0/current map,
1/mainmenu.map,2/shared.map and3/single_player_shared.map. The exporter pins all
actual resource files and map hashes. This namespace is not an assertion of
executed Xbox file-registry initialization. These checks establish PC sample
availability, not Xbox hardware decoder equality, source RNG/permutation
selection, spatial mixing, voice arbitration or sound-event occurrence.

The whole-map fixture SHA256 is
`e12bed43e5e6ac645b6194ec233775f4ab801d3ddbd0d28c97aca2360a679a7d`;
its report SHA256 is
`222167461f0e9c57588541321b0eebb60008cf0e6fd7f145c4579ea6b308231d`.

## Remaining live owners

The ordinary grenade path needs its full retained unit/action/channel state,
begin gates, real marker/attachment/projectile constructor, release/motion,
fuse/contact/detonation and damage/effect delivery. The independently measured
count debit and partial allocation cannot be promoted to a completed throw.

The power-module investigation has real source body/contact/factory/world
insertion evidence and native retained contact lifecycle kernels. The live
Scene/Simulation body/entity/shape/current-bounds/solver bridge and original
damage/explosion owners are still separate work. A visible mesh is not a
collision body, and a generic collider or timed explosion is not substituted.

First-person and world sound requests need original current object/location/
marker/listener owners followed by selection, scheduler/cache/voice lifecycle
and actual backend output. Complete resource coverage alone cannot prove all
sound effects play at the correct events, positions and times on every map.

The immutable checkpoint manifest and source/log snapshots are written to a
new `reported-failures-20261008/frozen` directory. Prior releases, original
assets and earlier immutable fixtures remain unchanged. No new launcher is
published as fixing these four symptoms before their live paths are verified.
