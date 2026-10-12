# Source portal mask producer

`source_portal_mask.hpp/.cpp` implements the persistent original portal mask and
its machine ownership. The authoritative executable is the supplied Xbox
`assets/disc/default.xbe`, SHA256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The module does not simulate door movement or infer a current Xbox door state.

## State and source chronology

- `210DF0` fills all 16 mask words with `FFFFFFFF`, calls `210F00` to bind
  ownership, then calls `210E20` to clear authored portal flag8 entries.
  It does not run the machine tick. The separately initialized acoustic bank
  at `4ED288+28` is preserved by mask initialization.
- `210F00` first runs `210E80`, resetting every actual type7 pool owner's
  body+1DA/+1DC range to NONE/0. Authored association records are visited in
  order, and `BAEB0` searches nonzero-generation type7 headers in ascending
  pool-slot order. Header visibility/activity flags have no admission effect
  because this iterator uses required flags0.
- Matching requires body unique ID+ A4, type+AA and source+AB. Origin BSP+A8
  is compared only when source is0. The first matching slot receives the
  record's range. A later matching authored record replaces its earlier range.
  Missing owners retain NONE/0; no mask bits are changed by binding itself.
- The door-only slice `1157D5..11592D` of `1152E0` updates assigned portals.
  A missing position group always means closed. With a group present, a
  nonzero current body position means open. Runtime device bit8 additionally
  admits open when the actual group record+4 value is nonzero. Platform and
  gear types do not write this mask slice.
- The per-list acoustic word interpolates definition+124/+128 using current
  body position, clamps to[0,1], multiplies by 65535 with SSE float precision,
  then uses nearest-even x87 FISTP after the float spill. It is indexed by
  association-list position, not portal number.
- `115220` restores every assigned portal bit to open and clears its per-list
  acoustic word. It does not reapply flag8, rebind another owner, or maintain
  a reference count. Shared portal writes therefore preserve caller order.
  The type7 callback table at468310 contains115220 at+3C and1152E0 at+40.
  `B8460` invokes `108BF0`'s+3C callbacks before `BC300` releases the object.
  Constructor115190 initializes NONE/0 ranges. A recreated machine does not
  implicitly inherit the destroyed lifetime's range.

`SourcePortalMaskState` exposes these operations separately. The caller retains
actual full object lifetimes and supplies the original event order. Calling
`bind()` each frame or rebuilding a mask from door positions would change the
source lifetime and cold-state semantics.

## Cache bank and supplied maps

The BSP pointer at metadata+16+220 points to an indirect 16-byte header:
record count/pointer, then signed-short portal-index count/pointer. Records
have 12-byte stride: unique ID, signed origin BSP, signed byte type/source,
signed first-list index/count. The native decoder retains all these fields.
Portal flags come from the 36-byte BSP portal record's+18 word.

All 21 supplied multiplayer BSPs have no flag8 portal. Only Gemini has owner
association records: six source0/type7/originBSP0 identities and eight unique
portal indices. There are no overlapping Gemini owner ranges.

| Unique ID | List first/count | Literal portal indices |
|---|---|---|
|30F0|0/1|33|
|318C|1/1|32|
|318E|2/2|25,36|
|3190|4/2|30,34|
|4B51|6/1|35|
|4B52|7/1|26|

The cold mask is all ones for all 21 maps. Gemini's six authored initial body
positions are0. These facts must stay distinct: an explicit door step using
zero current/group positions closes its assigned bits, whereas the original
mask constructor leaves them open before that step.

## Native state seam

`SourcePortalOwnerState` retains full object handle/header type, body identity,
assigned range and source machine definition. Its original-bank fields allow
literal Xbox datum fixtures. The overload accepting `SourcePortalDoorInput`
uses typed group presence, current body position, actual group value and actual
runtime device flags. Native callers need no fabricated Xbox group address.

Authored groupNONE does not mean absent runtime group:1076E0 allocates a
private group and copies its initial value into both group+4/+8 and body+140.
Named groups are initialized by107A30. Placement device flags must not be
copied into runtime body+12C:1076E0 sets80 and maps placement bits3/4 to runtime
bits0/1. Device106790 precedes machine1152E0 in the type7 tick callback chain;
it clears runtime bit8 and conditionally sets it during effective position
movement. Its group interpolation, acceleration, clock, door proximity scan,
script group changes and activation are separate current-state producers.
This module consumes their actual values and does not substitute initial
metadata for their later state.

## Validation

`tools/differential_source_portal_mask.py` executes the untouched complete
210DF0/210F00/210E80/210E20/BAEB0/115220 routines and the untouched bounded
1152E0 mask slice. Scratch pool/group banks are explicit inputs; no called
service is stubbed. The mask slice starts after earlier machine work and
stops before later auto-close logic, so it does not prove the whole tick.
Execution uses explicit FPCW027F and MXCSR1F80.

The 4,096 random cases plus 283 source edge/literal cases match 4,379 outputs,
671,222 words bit-for-bit. Coverage includes source0/origin and source1
identity, pool-slot order, duplicate records, missing/shared owners, repeated
bind/reset/clear/step/destruction ordering, NONE groups, platform/gear no-op,
float clamp boundaries and acoustic ties. A frozen 512-case regression retains
literal original outputs. Focused boundary controls additionally reject bad
indices/duplicate pool slots and compare the native typed group seam to the
original datum seam. The cache test visits every supplied map and checks the
literal Gemini owner/list bank and all 21 cold masks.

These checks establish the producer arithmetic and metadata under the stated
inputs. They do not establish the unported live door/HS scheduling, LAN
machine lifetime replication, physical Xbox floating-point environment, or
framebuffer parity. The original mask is tested for every portal by the
visibility consumer, regardless of flag4; 20 maps are static because no machine
association can change their cold mask.

## Native machine packets and initialization proof

`SourceMachineScene::portal_owners()` returns `SourceMachinePortalState`
snapshots keyed by full native generation, global content index and creation
tick. The packets cover all 48 admitted rendered machines, including hidden
owners. Every supplied portal association targets one of these owners; the
sole admitted no-model Ascension power_boost has no portal association.
The packet's owner range is constructor NONE/0. The persistent mask caller
keeps previously assigned ranges and uses `step(definition, owner, packet.door)`;
the Xbox group-datum field stays NONE rather than receiving a fabricated value.

The typed door input uses actual represented body position and successful named
or private group presence. With `SourceMachineScene::synchronize(scene, &devices)`
the simulation owner supplies current position, group target and runtime device
flags after ordinary `106790` progression, followed by gear `1152E0` progression.
These packets clear `initialized_group_state_only`. Unbound diagnostic scenes
retain initialized door/platform state and their private gear producer. A
reference door lacking the actual group/device bank is rejected. Original
automatic door proximity/autoclose remains unfinished. Native LAN clients now
bind replicated channels/group targets and full machine lifetimes, so their
portal packets use the same retained state as the authority. This does not
establish original networking, automatic-door scheduling or rendered network
acceptance; see [current map scope](MAP_WORLD_FEATURES.md).
Snapshots do not change mask, lifetime, phase, geometry or lighting state.

`tools/differential_source_portal_machine_initial.py` executes untouched
107A30, complete1076E0 and its actual16B840/1070D0/1086E0/1087C0 callees.
Successful allocations and controllerNONE are explicit inputs; no called
service is stubbed. All 49 automatic machine placement/group metadata records
and 1,024 synthetic named/private/placement-bit cases are retained. The frozen
fixture checks native initial flags, group flags/values and body channels.
The packet test checks all 48 rendered full lifetimes/identities and all six
Gemini doors, cold mask, explicit tick1 closed values, hidden ownership and
destruction restoration. Generic device animation and door/HS scheduling
remain outside this bounded initialization proof.

Private focused CTest passes all three mask/cache/packet tests. The owned mask
producer and packet bridge also pass ASAN/UBSAN instrumentation; this does not
claim instrumentation of every linked dependency. Tick0 retains the cold
all-one mask, and only actual native simulation tick advancement requests the
mask step. Destruction restores the old assigned range; a new generation
retains NONE/0 until an explicit source BSP mask activation/rebind event.
