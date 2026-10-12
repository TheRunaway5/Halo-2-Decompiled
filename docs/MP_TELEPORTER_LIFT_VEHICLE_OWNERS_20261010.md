# Teleporter, lift and vehicle source owners, 2026-10-10

This change ports and verifies three source components. **Live multiplayer
teleporters, gravity lifts and vehicles remain incomplete.** The existing
revision6 game/launcher has not been replaced with a claim of working objects.

The live regression places an ordinary native player at Beaver Creek's actual
entrance through the existing diagnostic teleport input, then runs
`MultiplayerMatch::step`. The player remains near X=-16.50. Complete original
`1A0580` execution with the authored entrance input delivers X=18.3940754.
The diagnostic target remains red. Its post-world check detects missing
traversal; it does not compare a complete original world step. Exact controller
coordinates and headings are checked independently below.

## Original authority

Both untouched, locally supplied images execute independently:

| Image | SHA-256 |
| --- | --- |
| Map Pack update1.1 `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03` | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| Supplied DLC `u5English/default.xbe` | `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd` |

Executable correspondence does not by itself prove complete gameplay parity or
that this is the latest historical update. New oracle runs retain their exact
input/output fixtures and tool hashes, and independent repeats are byte-identical.

## Teleporter controller

`source_teleporter.cpp` implements complete `1A0340/1A0390` pair initialization
with the no-position/max-one branch of `1A25A0`, and complete
`1A0580/1A0680/1A0790` control. It preserves:

- Channels0..15, first matching entrance/exit, and failed-channel bank writes.
- Occupied player slot order and current-unitNONE exclusion from `1A2C60`.
  Flag2 or a host dead-body enum does not replace that iterator.
- ParentNONE admission, strict entrance-distance comparison, and the distinct
  SSE addition orders in the entrance-mark and nearest-player passes.
- The three flag passes, nearest-player tie behavior and latch rejection.
- Pair cooldown decrement, signed blocked-notice behavior and callback order.
- Original x87 heading and nearest-even cooldown conversion. Heading uses
  FPATAN/FADD/FSUB/FCOS/FSIN followed by the recovered `30BD0` normalizer.
- The selected executable's default up vector, independently read through
  `[46FC50]`/`[474D10]`, and original zero/zero pose-delivery arguments.

The x87 implementation is validated on Linux/x86-64 with GCC. Unsupported
compiler/CPU combinations reject at the math owner; host trigonometry is not
silently substituted. The caller must supply the original nearest-even SSE
environment. Masked FP status flags are not compared.

The oracle runs 96 full initialization banks and 768 controller cases per
image. Native results match all bank words, player flags, roots, forward bits,
query/action/view callback arguments and their order. The cases include 597
query requests, 200 traversal branches, 108 view deliveries and 123 blocked
unit notices. These are controller branches with explicit dependency services,
not 200 live native gameplay teleports.

`oracle_teleporter_control_20261010.py` executes the original controller,
initializer, destination reader and normalizer. The occupied-player iterator,
root reader, destination-query result and external mutations are explicitly
provided test services. The genuine original empty/occupied-exit experiments
remain separate in `teleporters-lifts-20261010`; no query or telefrag success
is manufactured for a live match.

Whole caller correspondence additionally verifies `139750 ->15D7C0`, with
`15D80E ->1A0580` before `1397C0 ->B7E00` world-object work. The u5 caller
is `13AAC0 ->15EBA0`, with `15EBEE ->1A6830`. The engine caller skips the
teleporter call when its actual raw mode byte is4. This caller gate is not part
of the direct `1A0580` function and must be preserved by future integration.

## Lift admission

`source_lift_contact.cpp` implements complete `1D56F0/1D9FB0` against
`SourcePhysicsContactMemory`. It follows the selected image's real global-slot
layout through component, object, definition, model, physics, shape and rule
backing. It preserves the signed shape-rule index and size bytes, size-zero
range bypass, player binding, unit+10A control bit and rule/type exclusion bits.

Both original images and native implementations agree on 80,642 admission
results, including counterpartNONE returning without reading any source bank.
These packets are declared gate inputs, not produced Havok contacts. The
previous 62,464 scalar delta/ratio cases remain a separate verified component.

## Vehicle constructor and reset

`source_vehicle_construction.cpp` implements complete direct `F01C0/F10E0`
constructor control and `EFFF0/F0F10` reset stores. It preserves exact write
width/order, untouched bytes, full salted identity, the phmoNONE rejection,
optional BAA80 call, both animation requests, ignored fallback return, and
signed body+12A manager delivery. Missing dependency owners fail at their
actual boundary after preceding stores; no successful allocation/body/animation
result is supplied to a live game.

There are 192 cold cases per image, 384 native comparisons, and 13,248 matching
ordered writes. Original `BADE0/BACC0` executes the unparented/componentNONE
point branch. Animation admission, BAA80 and1D1940 are declared test services;
their complete live implementations are not established by these tests.

## Validation and remaining integration

Normal and AddressSanitizer/UndefinedBehaviorSanitizer CMake builds pass all
three original-comparison targets. The verification composition copies the new
owner source and links immutable revision6 dependency libraries, excluding
concurrent frontend/renderer changes. `inputs.json` pins the exact copied
source, fixtures and libraries. The primary project also configures with the
new targets registered. A full current desktop/game composition was not built.

```sh
ctest --test-dir analysis/multiplayer/integration/transport-owners-native-20261010/build --output-on-failure
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir analysis/multiplayer/integration/transport-owners-native-20261010/asan --output-on-failure
```

Required work remains:

- Teleporters: live `1A0400` collection/deepest-contact query, occupied-player
  flag/dirty-entity publication, event22, B8490 pose/registry updates, local
  camera delivery and LAN/presentation discontinuity handling in the original
  engine phase.
- Lifts: live collision production/delivery into the contact-list owners,
  body/marker/mass/velocity producers, complete vector-force owner `1D5E90`,
  activation and world-step delivery. No upward-impulse substitute is added.
- Vehicles: actual row allocation, body/constraint construction, seats and
  entry/exit ownership, controls, per-type physics selectors, collision,
  damage/destruction and live LAN/render/audio behavior. The existing inventory
  of 57 authored vehicle rows in12 maps remains unimplemented in the live game.

## Contact membership continuation and preview executable

`source_lift_membership.cpp` now implements complete direct
`1D5830/1DA0F0` addition and `1D5A60/1DA320` removal. Both original images
execute independently in `oracle_lift_membership_20261010.py`. The paired
fixture has152 cases,588 direct operations and1692 ordered writes, with a
byte-identical independent repeat. It covers duplicate reference counts,
byte wraparound, signed body-index matching, full object identities,
swap-last removal, allocation/growth/free dispatch, and the rule-bit24
unit+3E4 binding/clearing conditions. Native comparison also rejects absent
allocation, growth and release owners. Optimized and ASan/UBSan checks pass.

These routines consume an entity delivered by collision. The fixture declares
raw contact/property inputs and explicit storage services; neither is a
collision producer or successful world callback. The actual body/phantom
construction, overlap producer and complete vector-force/world delivery are
still absent from live multiplayer.

The new `lockout_lift_match_tests` diagnostic reads the actual Lockout lower
phantom polyhedron center, transforms it by the authored placement, places a
native player through the diagnostic teleport input and runs30 ordinary
`MultiplayerMatch::step` calls. It remains **RED**, with peakZ equal to the
startingZ and no upward velocity. This detects the reported missing lift;
it does not compare an original whole-world trajectory. The Beaver Creek
live traversal diagnostic also remains **RED**.

`Halo2-Sandbox-Preview` opens the separate frozen current-source preview at
`analysis/multiplayer/integration/sandbox-preview-r7-20261010/delivery/runtime`.
It starts at the title screen. Its manifest records the exact frozen source,
compiled runtime/shaders, validation logs and both known live failures.
The prior revision6 and canonical launchers are preserved. This preview
includes current workspace HUD/audio/frontend changes as well as the new
source components; it is not a working-lift or completed-sandbox release.

Eight focused original/HUD/audio tests, the multiplayer combat/UDP suite,
four-view frag/plasma LAN delivery, prop-audio LAN delivery and four-view
SMG muzzle LAN/render checks pass in the new composition. The actual delivery
launcher renders the title screen. SDL dummy audio verifies submissions,
not physical output; these Linux/GPU checks do not prove full Xbox parity.

There is no new visual/gameplay screenshot for these components because they
are not connected to live matches yet. Source-component comparison and
sanitizer success do not establish native object behavior or Xbox parity.
