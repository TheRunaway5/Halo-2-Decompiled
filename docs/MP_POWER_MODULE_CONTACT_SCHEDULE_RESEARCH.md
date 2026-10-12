# Source-created Lockout power-module contact and current bounds

This separate CPU experiment executes the unchanged original TU1.1 world and
object schedule for **Lockout crate row1**, `power_module` BLOC **E6F80583**.
The source creates a real transient crate-to-BSP contact, fills its point and
normal from the actual Havok contact manager, then removes it during the same
world pass. The selected PVS and all four object phases return without a supplied
game/Havok result. A separately declared current-bounds query also executes the
real backend and returns a positive quantized AABB. No position, contact, PVS,
node pose, impulse, damage, body identity or allocation success is written by
this extension. Production remains untouched.

This closes a **bounded cold crate/contact/bounds producer composition**, not
continuous settling, destruction, native physics, a complete frame, or Xbox
hardware parity. The inherited tick/rate fixture and platform/raw backing inputs
remain explicit. The preceding [factory/collision freeze](MP_SANDBOX_CREATION_COLLISION_1_1_RESEARCH.md),
[player/PVS freeze](MP_PLAYER_VISIBILITY_RESEARCH.md),
[later-executable correspondence](MP_SANDBOX_LATER_UPDATE_SOURCE_RESEARCH.md),
and every item/effect/title freeze remain unchanged.

## Original object and owning initialization

Primary authority is TU1.1 `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`,
SHA256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`,
and Lockout SHA256 `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
Ghidra labels are navigation; original instruction/cache bytes and their actual
executions own the findings. [Oracle](../tools/oracle_mp_power_module_schedule_1_1.py),
[actual reports](../analysis/multiplayer/sandbox-source/power-module-schedule/frozen/measured/report.json).

The source-owned player helper initializes the real 2048-object header pool,
1MB body heap, source registries, physical components/body heap/world, impact
pools, player/engine/queue/trigger storage, and selected original reset/activation
callbacks. It does not claim all title/map callbacks. Whole measured native
compatibility KPCR/ETHREAD/stack/process records are replayed from the immutable
power-fixed constructor case; no selected context field is patched.

The original selected `phmo` tag iterator/postprocess and complete
**BFFE0→D6660→B8820/11F680→B8A30→11D1C0** factory run first. Their whole
constructor state/ABI and pre-query call/store prefix are byte-identical to the
earlier power-fixed case. Its live call/store arrays also contain later direct
collision probes; those appended probes are not confused with constructor
inputs. The selected-tag postprocess is unchanged; a complete map postprocess is
still outside scope. [Original request/factory evidence](../analysis/multiplayer/sandbox-source/owner-creation/final/power-fixed-measured/report.json).

| Actual retained field | Source output |
|---|---|
| Scenario row address / bytes | `80158FCC`, original `4C` row, palette1 |
| Original row root X/Y/Z | `40EDC4BA C104B838 C0FF5EF8` |
| Full object datum / allocated body | `E26F0000` / `82D87FC0`, 382 bytes |
| Model / collision / physical definition | `E6F90584` / `E7080593` / `E7090594` |
| Physical component / initial body | `E1680000` / `0601A480` |
| Original world / BSP body | `06005470` / `06005250` |
| Crate-specific `+12C` | `00000002` |
| Object runtime `+C0/+C1` | `41/01` |

The actual factory, not a decoded shape index, creates the physical body.
Serialized rigid-body record `+38=4` requires original `1EE380/1EE8F0` to become
the real shape pointer `8025CC40`. Passing the unprocessed index still retains
the earlier exact `1D9994` stop. Source-authored scale/rotation/request defaults
remain their original produced words; this extension assigns no pose or bounds.

After crate construction, original player join/spawn and `14EAE0..14ED0C`
produce a real player/current-unit/cluster. Then full **13A380** computes the PVS
and actual object activation callbacks. The player and carried weapon occupy
subsequent pool slots; the crate keeps `E26F0000`. This is an explicitly selected
world composition, not all14 authored power-module placements or a complete
scenario placement loop.

## Exact selected schedule

The measured source order is **13A380 → B7E00 → 1C7660 → B7F50 → B8040**.
Original `139750` contains that phase order; the full frame and its owning clock
producer are not replaced or asserted complete. Both source-player and
source-empty-player cases run all five roots. [Complete phase inputs/outputs](../analysis/multiplayer/sandbox-source/power-module-schedule/frozen/measured/report.json).

With the actual player, crate header flags change **4A→4B** under PVS,
**4B→4F** during world physics, and **4F→41** in new-object cleanup. The ordinary
and dirty passes skip this new object. The new pass calls **BD360**, then actual
type callback dispatch at **10CE3B** calls complete **11D2B0(crate)**. Its defined
AL is0: runtime `+C0` bit12 is clear, so no phantom `1D5E90` is called; `+C1`
bit0 is set, so the timestamp store at **11D31D** is skipped. The retained
authored root and current node bank remain unchanged.

BD360 then calls complete **D73E0(crate)**. **D7160** resolves actual damage-info
record **80255D6C**. Constructor stores **B90F1/B90FC/B910F/B911D** previously
copied damage-info `+28/+8C` and established the actual object words:
`+E4=41F00000`, `+E8=00000000`, `+EC=3F800000`, `+F0=00000000`.
The actual `+120` section bank is16 bytes at relative offset130; all pending
response words are zero. D73E0 returns AL0 and does not dispatch destruction.
Those outputs remain unchanged throughout both cases. No incoming damage is
supplied to cause a desired result.

The source-empty-player control retains zero source masks. Its scheduled
crate update/contact callbacks do not execute. That control separates genuine
PVS admission from a manually active mask. Full headers, bitmaps, queues,
context, bodies/nodes/physical packets, damage state, impact pools and unsorted
registry chains are retained at owning entry/exit state IDs.

## Genuine transient terrain contact

Complete **1C7660** calls the actual world step **317650**. On this created
crate, source callback **27DFE6→1D3DF0→1D2DF0** allocates/constructs a
**0x48 (72-byte) contact row at0601A720**, count0→1. The source allocator owns its
capacity; no contact row or count is supplied. The caller packet contains actual
crate body **0601A480**, original BSP body **06005250**, raw counterpart key
**2000081D**, and source material words **0056/0068**. The lower key is retained
as an opaque source key, not interpreted as a guessed BSP surface index.

The source crate body's real property pairs include
`2001→E1680000` and `2002→0`; the original BSP body has `2001→FFFFFFFF`.
Both complete property packets and body prefixes are captured. This identifies
the actual source counterpart rather than a supplied terrain-hit Boolean.

Complete **1D2F10** runs twice. It consumes the actual manager **0601AFA0**,
actual contact datum/physical group, source Boolean bytes, and the two real
virtual getter outputs. Return sites **1D2F2D/1D2F3C** retain actual manager
point/distance/normal packet **0601A980** and scalar word **0601B04C**. No expected
vector is recomputed by the exporter.

| Actual emitted contact words | Original bits |
|---|---|
| Point X/Y/Z | `40F1DD57 C1037D9F C1063D32` |
| Normal X/Y/Z | `B58C6F20 36040BC0 3F7FFFFF` |
| Initial / refreshed distance | `37606000 / 375FFFFC` |
| Getter scalar | `00000000` |

Actual initial 1D2F10 ABI is EAX=manager, ECX=contact row, stack inputs
`[1,0,E1680000,0]`. The later refresh reads the same source Boolean low bytes;
the raw second word contains unrelated upper bytes `64E39500`. Its full raw word
is provenance, not a new 32-bit mode. Each routine's stack cleanup and actual
entry/return row are retained. Original point/normal stores and their owning
math/flag branches execute unchanged.

The same callback builds the original **208FB0** impact packet, uses real
**2285A0** allocation and **209040** ownership, and stores the retained typed
impact datum. Complete32×A0 impact and32×40 impact-array banks are retained
under globals **52648C/526490**. Their handles are distinct pool identities even
when raw handle bits coincide. No effect/audio/rendered explosion is inferred
from these records.

Within this same world pass, original **27E353→1D3FC0→1D40B0** matches the real
contact, calls **2298B0** for its impact reference, then removes the contact:
count1→0. The allocated one-row capacity remains, including literal retired
bytes. These are captured transient creation/refresh/removal outputs, not proof
of a persistent support, later moving-prop contact, impulse or settling state.
The crate's phantom bank `+94` remains null. The source clock/tick and primary
RNG remain their declared unchanged inputs.

## Actual quantized bounds consumer

After the selected schedule, a **separately declared direct diagnostic root**
calls complete **BEDE0(E26F0000)**. This call is not claimed to have been
naturally dispatched by the preceding new-object tick. It uses the same actual
allocated crate body/component/node/definition state without a guest write.

BEDE0's type11/physical gates admit original **1D7EE0**. The latter calls its
actual backend vtable at **1D80EF**, target **2E7610**, with backend **060056D0**,
the genuine body handle **0601A49C**, and the original stack output buffer.
The actual authored margin read is zero. Both source-player and empty-player
cases return the same genuine quantized AABB; no `2E7610` result or geometric
shape bound is supplied.

| Actual backend/result | X/Y/Z words |
|---|---|
| Minimum | `40E915E0 C1071BC0 C1069010` |
| Maximum | `40F283E0 C10259F0 C0F1AF40` |
| Emitted sphere center | `40EDCCE0 C104BAD8 C0FF67B0` |
| Emitted sphere radius | `3EF4CCA1` |

The backend packet contains the original fourth lane on both vectors; it is
retained in the report. **1D7EE0** returns AL1; original BEDE0 stores the sphere
and executes its actual registration/update leaves. These source outputs differ
from the initial cached authored sphere. They establish the admitted current
backend consumer, not permission to update bounds early or to replace backend
quantization with a shape AABB. [Literal virtual ABI/result](../analysis/multiplayer/sandbox-source/power-module-schedule/frozen/measured/report.json).

## Native integration seam and remaining gates

Current **SceneObjects** retains authored placement, native lifetime handle,
root/rotation/scale and visibility; it does not own original physical components
or source damage state. **SourcePropScene** prepares/poses the source geometry
and consumes a caller-owned lighting/query sphere. It explicitly requires real
backend quantized bounds for the physical override. Neither creates these body,
contact, impact or damage owners. [Current scene API](../include/halo2/scene.hpp),
[prop API](../include/halo2/source_prop_scene.hpp),
[existing prop bounds arithmetic](SOURCE_PROP_PARITY.md).

The smallest faithful composition retains a distinct source object datum/header,
constructor-produced body/node/damage buffers, actual component/body identity,
authored collision/physics tags and real source registry membership. The earlier
complete posed-collision query already hits the actual allocated box's material86
on three center segments. This new data supplies genuine initial physical
activation/contact packets and backend bounds. A native consumer must preserve
the original dispatch order and typed identities, while exposing the solver,
quantization, contact-manager and future clock producers it does not implement.
Neither a uniform explosion sphere nor a generic static collision radius follows
from the authored tag.

**D9180→DC0A0**, the pending-response countdown, **DB710→DB360/DB470→D8280**,
children/debris and area-target/radial/visibility consumers remain unexecuted
positive branches here. The model's actual delayed destruction response and
`E72B05B6 fusion_coil_explosion` tag remain the earlier authored/source-listed
evidence. The next destruction gate is an actual admitted incoming projectile/
damage packet and its authority/region/threshold stores, not a write to health,
a fabricated depleted section or an injected DB710 success. Full world/frame
clock progression, additional authored objects, later contact/impulses, impact
event aging and positive phantom/lift owners remain separate.

The existing later-XBE freeze proves full instruction correspondence for the
crate constructor/update, damage resolution/timer/response, area-event and posed
query owners. It does not execute this new full contact/backend schedule in1.5.
Newly observed Havok/contact/backend routines therefore retain their exact1.1
addresses and execution boundary; no relabeled1.5 address or full latest-patch
world parity is asserted.

## Reproduction and freeze

[Oracle](../tools/oracle_mp_power_module_schedule_1_1.py),
[auditor](../tools/audit_mp_power_module_schedule_1_1.py),
[final manifest](../analysis/multiplayer/sandbox-source/power-module-schedule/frozen/manifest.json),
[audit](../analysis/multiplayer/sandbox-source/power-module-schedule/frozen/audit.json),
[compact schema](../analysis/multiplayer/sandbox-source/power-module-schedule/frozen/schema.json).
The schema retains `stages` state IDs, literal `calls`/stack/registers/packets,
ordered non-stack `stores`, full `states`, actual backend packets, original
instructions and all explicit inherited services. Raw extra stack words are
provenance. No native implementation produces expected values.

```sh
.tools/python/bin/python tools/oracle_mp_power_module_schedule_1_1.py \
  --output /tmp/halo2-power-source/measured/report.json
.tools/python/bin/python tools/oracle_mp_power_module_schedule_1_1.py \
  --without-player --output /tmp/halo2-power-source/inactive/report.json
.tools/python/bin/python tools/audit_mp_power_module_schedule_1_1.py
```

Independent reports and every instruction file must match byte for byte. The
auditor checks original image bytes, untouched preceding manifests, full source
buffers, real contact transitions, source packet/ABI outputs, exact unsorted
registry chains and original ordered-store replay. Both new title/DSP freezes
are verified intact as well. Earlier observer-development captures remain
separate; Python observer corrections changed no guest instruction or source
state. Numerical hardware and complete source startup remain explicit limits.

The final independent executions reproduce both reports and all **622 original
instruction files** byte for byte. The player case retains **74 complete calls,
4,544 stores and32 states**, covering50,297 original setup/schedule addresses;
the source-empty-player control retains **16 calls,155 stores and5 states**,
covering16,575 addresses. Ordered-store replay verifies40/10 owning call outputs
and **2,355,105/506,100** known state bytes respectively. The auditor checks43
prior pins, all189 prior factory-manifest artifacts, and both new title/DSP
manifests with their320 artifacts. [Independent file comparison](../analysis/multiplayer/sandbox-source/power-module-schedule/frozen/reproducibility.json).

Frozen report SHA256 values are
`6b3f94596397bf3eb2b90540e55089b428778e6716f4c8f04bf1933faabd7af9`
(source player) and
`8b4d272f367b46aab22c617955e9a3aa911befd74573f7649ce074fa1e38beaa`
(source-empty-player). Oracle SHA256 is
`54a498e282d041b95c02438d3549d18e2aabfa7c3574cefe6e467f2845d3750f`;
auditor SHA256 is
`249c16edc984395979d598d57428b134a070edfbcf006f2fb5ae8e4a21448439`.
The manifest pins the final doc, schema, full reports, original listings and
audit without changing any preceding freeze.
