# Original pickup input and dual first-person transforms

The live native weapon adapter now uses the authored 233ms interaction threshold,
the original player-owned held-button consumption state, and the original
secondary weapon hierarchy. These changes close specific input and offhand pose
errors. They do not establish complete weapon, animation, physics or framebuffer
parity.

## Original executable comparisons

Both differential exporters execute unchanged original instructions in Unicorn, including
all callees of the dual hierarchy producer. There are no substituted original
callee results. They use explicit finite inputs and actual Lockout graph node
records. Expected output comes only from original execution.

| Original image | SHA256 | Player edges | Dual WORLD |
| --- | --- | --- | --- |
| Retail disc | `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d` | `14F6F0` | `1DD290` |
| TU1.1 | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` | `150DD0` | `1E0C80` |
| TU1.5 English | `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd` | `1521B0` | `1E5540` |

Every case produces identical outputs across these three images. Each exported
fixture records the pinned images and original entry addresses. Function location
requires a unique original byte anchor; the complete player-edge leaf is also
byte-identical except its one relocated pool-global operand.

`check_player_action_edges.py` covers all 4,096 combinations of the low action
byte, both consumption flags and both retained down bits, with randomized
unrelated player/action bytes. The native test compares the entire 21C-byte player
and 5C-byte action records after applying the represented fields. It separately
checks 233ms conversion and 232/233ms admission boundaries.

`check_first_person_dual.py` uses all ten authored first-person graphs for the
five dual-compatible Lockout weapon definitions and both species. Sixteen
explicit local-pose/root controls per graph produce 160 cases and 87,360 exact
WORLD float words. Quaternion/translation/scale decoding, channel selection,
feedback, IK and camera production are outside that hierarchy differential.

## Live integration

The player edge leaf retains previous gameplay bits, previous interaction down
bits and consumption flags. A successful native pickup consumes its button until
release. Keyboard and controller adapters retain elapsed milliseconds separately
for each interaction; native UDP transports both durations and held states.
Completed holds retain a duration and completion serial separately from current
held input, so later neutral packets cannot erase a 233ms request before delivery.
The UDP regression rejects 232ms and admits 233ms after coalesced release; it
also checks that a completed hold is delivered only once. Protocol 32 includes
these fields. Multiple completed holds before one received input remain a native
latest-completion transport boundary, not recovered original network delivery.
Controller reconnection and debug input capture discard stale hold clocks.
This does not reproduce Xbox analog hysteresis or the original polling interval.

Original 169965 selects the first weapon model node, examines its graph parent,
and finds the opposite hand by graph flags 8/10. Secondary arms map only nodes
carrying flag 20. Original 1E0C80 replaces only the selected weapon root's parent
and mirrored local transform; descendants follow that hierarchy. The live pose
path uses this transform and the primary hand's camera-control adjustment for
both hands. It preserves mesh winding and tangent bases. Original 168483 assigns
channel options 103F and 203F, preserving hand-specific event admission.

The SDL queue test checks elapsed time at 232/233ms, consumption without restarting
the hold, repeat events, independent controls, saturation and released requests.
`weapon_actions_tests` also samples 156 actual offhand poses, validates the
secondary arm mapping and the two event-option owners. The live Vulkan test uses
real SDL hold/release, two firing/reloading SMG datums, four submitted clips,
offhand drop and grenade throw. The four-view grenade regression verifies one
request/debit, source throw poses, release/detonation and grenade geometry in
every view. Vulkan validation reports no errors in these runs.

## Native GPU residency

The full-capacity native UDP graphical probe initially failed with Vulkan
out-of-device-memory while uploading duplicate weapon materials. Immutable
authored image snapshots now share allocations when their format, dimensions,
layer/depth layout, complete pixel bytes and mip records match. Per-owner
samplers, constants, frames and descriptors remain independent. Render-target
diffuse placeholders retain separate allocations. A dynamic 2D image update
detaches its allocation; volume/cube updates through that 2D API are rejected.

`renderer_material_residency_tests` failed on the previous allocation path, then
passed with 16 ordinary material banks, all four stages and animated frames
sharing two images. An equal-byte 1x1x1 volume remains a distinct image type.
Framebuffer comparisons verify private updates, restoration and unchanged pixels
for the other owner. The final native UDP graphical fire/reload probe passed with
92 posed submissions and 91 fractional first-person samples. The separate dual
capture submitted 458 poses across four clips. The final dual/grenade sequence
submitted 248 dual poses and dropped the actual offhand on grenade request.
The final four-view grenade run submitted 14 first-person and 36 body throw poses.
HUD regressions and 9,216 authored volume/42,090 scope-copy checks passed.
These are native GPU resource and delivery checks, not an original framebuffer
comparison or a change to original weapon behavior.

## Remaining limits

Original animation manager transitions and saved-local blends, third-person
firing/reload layer composition, secondary layers,
feedback, full IK admission, inventory request/action admission, animated bounds,
registry enumeration, full source collision/Havok and non-SMG charge/heat/lunge
remain outside the complete live native path. Native compatible ammunition
transfer, simultaneous pickup requests and dropped-item poses still use adapter
logic. Full matched original rendered sequences are needed before a 1 to 1 claim.

## Reproducible delivery build

Concurrent changes to the shared simulation/scene headers during compilation
caused a mixed-layout executable and a constructor link failure. The delivery
build uses a captured source snapshot instead. Its retained source hashes are in
`.local/weapon-parity-source-20261011/source-manifest.json`; the runtime and shader
outputs are in `build-weapon-parity-20261011`. Assets and original research files
remain shared read-only inputs. The frozen source files are checked against that
manifest, and the regression/runtime tests are rerun on the resulting binary.

Final live probes use an isolated X11 display, explicit ordinary SDL window focus
and a process-local controller filter so unrelated desktop windows or controllers
cannot consume/inject keyboard test input. This is host SDL/Vulkan evidence on
the RTX 5080, not physical Xbox or physical-LAN validation. The 18 focused tests,
the 16-participant UDP fragmentation/reorder/drop test, the held dual-SMG sequence
and the four-view grenade sequence pass on the stable build. Original fixture
comparisons still retain the explicit input and emulation boundaries above.
The additional image-residency regression passes with no Vulkan validation
messages. [Delivery provenance](../build-weapon-parity-20261011/weapon-parity-validation.json)
records the source manifest, binary/log hashes and the outstanding parity limits.

## Pickup admission follow-up

The subsequent [source admission port](MP_PICKUP_ADMISSION_SOURCE_PARITY.md)
adds complete original pickup/request CPU owners and replaces the live fixed
carry limit with original player restrictions. Its component differential and
new delivery provenance are separate from the earlier input/dual build above.
Full live animation, inventory/action and Havok parity remains unfinished.
