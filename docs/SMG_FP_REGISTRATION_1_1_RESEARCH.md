# Original 1.1 first-person SMG registration

This independent slice executes the original first-person allocation,
initialization, player-profile conversion and carried-weapon registration with
the fully allocated and attached Lockout SMG. It supplies no first-person weapon
datum or final node mapping. It is sufficient evidence for a bounded native
binding consumer; it does not prove first-person channel clocks or posed output.

## Executed producer and lifecycle

`1677C3` requests `8330` bytes through original `125490`, then `8000` bytes with
alignment exponent 4 through `1254D0`. The outputs populate globals `4F1198`
and `4F1194`. The supplied backing arena is `07600000`, with initial used=0
and allocator checksum=`FFFFFFFF`; the original bump/alignment/checksum code
executes. The resulting globals/scratch addresses are `07600000`/`07608330`,
with used=`10340`. This is an explicit backing-storage boundary, rather than
execution of the complete global arena allocator lifecycle.

`1677E5` clears four views of stride `20CC`; each contains two hands of stride
`1010`. It initializes view unit/representation and hand weapon handles to NONE,
then executes original `1CE9B0` and `1C9D30` controller initialization. A hand's
base is `globals + view*20CC + hand*1010 + C`.

The input profile is recorded byte-for-byte as a supplied `90`-byte block from
the existing player `+D4` storage. Original `14D600` calls `14D3D0` and copies
the sanitized block to player `+44`. With options state 2, the sanitizer converts
profile `+44` values 1 or 3 to 3, and other values to 2. Thus the original copy
at `14D685` writes player `+88=2`, selecting the multiplayer Spartan
representation. The independent frozen ordinary join report corroborates this:
its original `14D350→14D800→14D600` path produces player `+88=2` after original
controller/profile production. This registration oracle does not replay that
separate full join fixture.

`1678C1` loops over four local views. It reads the view-to-player mapping through
`14F5B0`, reads that player's current unit `+2C` and representation byte `+88`,
then invokes `169D8A` when stored unit or selector differs. **`169D8A` changes
the stored unit/selector and rebuilds hands only when the unit differs.** The
control with selector changed while the same unit remains bound reproduces
this: player selector becomes 2 but the stored view selector remains 0. The
fixture explicitly invokes original view release `169D7A` between this control
and the valid rebind. This control must not be generalized into an assumed
automatic representation refresh.

For each bound view, `1678C1` queries current hand indices at unit `+212/+213`
and calls `CD1A0`. Signed index NONE returns NONE; otherwise the original lookup
uses the datum at unit `+218 + index*4`. It compares the result with the stored
first-person weapon and runs `169965` when they differ. The requested indices
at `+216/+217` are not the registration input. The recorded requested-index
NONE control retains the current SMG binding; current-index NONE unregisters it.

`169965` first calls `169E15` with NONE, then validates and reconstructs the
hand. Original `169E15` writes its weapon datum at `169E4B` and calls original
`179150` before/after a changed registration. The supplied first-person effect
datum pool is empty; those original traversals execute, but populated effect
records are outside this evidence. Original `169D7A` supplies unit/selector NONE
to `169D8A` and removes both hand bindings. `16800D` is the source path that
searches existing weapon bindings and rebuilds matching hands; its instructions
are listed, but this oracle does not execute a complete object-deletion caller.

## Minimum binding inputs and gates

| Input | Source location or operation |
| --- | --- |
| Local view's player | `[4F01F0]+C+view*4`; NONE skips that observer view |
| Player's current unit | `[4F01F4]+44+(player & FFFF)*21C+2C` |
| Player representation | Same player record `+88`, signed byte |
| Current weapon per hand | Unit `+212/+213` signed indices, then `CD1A0` |
| Weapon definition | Exact original object datum/header/tag lookup |
| Weapon first-person record | Definition block `+2A8/+2AC`, stride `10`, through `11CF00` |
| Representation arms model | Globals tag block `+138/+13C`, stride `BC`, model datum at record `+4` |
| Weapon model / graph | Weapon first-person record `+4` / `+C` |
| Node-name maps | Original `1CF7A0→1E0EB0`, exact full StringID equality |

The view must have a unit, the representation must fall within the globals
representation block, the current hand must have a weapon, and the weapon's
first-person record/model/graph must exist. `11CF00` selects record zero for
representations 0/2, record one for 1/3, with its original count fallback.
`169965` invokes `1CEAC0` for graph assignment; it does not add a new Boolean
return-value test after that call.

Each model render node is matched against the graph's node table. Render-node
records have stride `60`, name at `+0`, count/pointer at model `+48/+4C`.
Graph-node records have stride `20`, name at `+0`, count/pointer at graph
`+C/+10`. Original `1E0EB0` walks render nodes in authored order, searches graph
nodes in authored order, writes the first equal full StringID index or -1, and
returns low-byte success only when every render node found a match. No geometric
pose or floating-point reciprocal is needed for this mapping.

The source sets hand flag 4 for a successful arms mapping, flag 2 for a successful
weapon mapping, and registers the weapon only when both are present. The frozen
normal single-SMG result is:

| Authored object | Datum | Render nodes | Graph mapping |
| --- | --- | --- | --- |
| Multiplayer Spartan arms | `E9E90864` | 37 | 0 through 36 |
| First-person SMG model | `E5A80433` | 4 | 37, 38, 39, 40 |
| First-person SMG animation graph | `E5AA0435` | 42 graph nodes | Both maps above |

All 41 mapping pairs were independently checked against the original cached
name fields. The fixture contains the raw authored model, graph-node,
representation and weapon first-person records. Representation records 0/1 have
arms datum NONE in Lockout; this is why the unconverted-selector control assigns
the graph and weapon mapping but cannot register the hand.

For hand base H, the source stores the weapon at `H+4`, the animation manager at
`H+8`, weapon-node count/map at `H+F4/H+FC`, and arms-node count/map at
`H+F8/H+1FC`. The recorded map buffers include 64 entries, preserving unused
state as source output. The supported hand-zero result has flags 6 and weapon
`00010002`, view unit `00010001`, selector 2. The other hand remains NONE.
Secondary/dual mapping branches are in the instruction listing but are not
validated by a successful dual-weapon case in this suite.

## Selected channel and firing feedback

After successful node maps, `169965` calls `168483` at `169C85` with activity
`0400000C` (idle). The original graph lookup selects `first_person:idle`, index
21, 89 authored frames. The channel identifier is `0015FFFF`, rate bits
`3F800000`, frame bits `00000000`, flags `+11=00`, `+12=003F`. This is initial
selection only; this slice supplies `168866` and does not advance its clock.

Original `167D32` takes the weapon as a stack argument and compares it against
all eight registered weapon datums. It returns the last matching view found, or
NONE. It does not require the hand-active flag. Original `10A140` forwards its
EBX weapon with PUSH, resolves the returned view's player/current unit, and
selects hand zero when the weapon matches current primary, otherwise hand one.
`EFCC0` requires non-NONE view, player, and player current unit for hand-timer
events 1 through 4. These predicates are detailed in the separately frozen
[hand-feedback research](SMG_HAND_FEEDBACK_1_1_RESEARCH.md).

After actual source registration, two original unit/weapon/postpass ticks produce
hand-zero timers 9 on tick-zero weapon update, then 8 after tick-one unit update.
These calls reach `167D32` from original `10A140`, with stack datum `00010002`,
and return view zero. No supplied slot datum is used. Projectile allocation is
still supplied failure and the muzzle segment is supplied clear. Original
`16A356` first-person animation notification remains a supplied service, so no
fire/reload/ready presentation-channel behavior is claimed here.

The original active-frame routine `139750` calls `1678C1` after the object
update/postpass roots (`B7E00`, `1C7660`, `B7F50`, `B8040`). Its instructions
are included as ordering evidence; the complete frame/world path is not executed
by this bounded registration oracle.

## Frozen artifacts and reproduction

```
.tools/python/bin/python tools/oracle_smg_fp_registration_1_1.py
.tools/python/bin/python tools/oracle_smg_fp_registration_1_1.py \
  --output analysis/multiplayer/map-pack/smg-fp-registration/report-reexecution.json
.tools/python/bin/python tools/audit_smg_fp_registration_1_1.py
```

Independent executions are byte-identical. The audit checks 2,553 listed original
instructions against the pinned XBE, 41 node-name mapping pairs, initial channel,
source profile copy, registration/removal writes and hand feedback controls.
The report contains 150 original entry/return observations and 10,702 source
writes. The compact source fixture extracts authored metadata and the 15 source
steps without constructing native expected values.

| Artifact | SHA-256 |
| --- | --- |
| Original XBE `.03` | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| Lockout cache | `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9` |
| Oracle | `06c0ddfece364a4361b60219b4f2ab26e98acc693aacccbe16eb89f8d0535e92` |
| Report / independent reexecution | `fe7c7ea7ef620b526ab6a9debda83068055a4e02ee700bdd1ed3dea9cb375ca2` |
| Instruction inventory | `e4fb2c631cbc68ee2799ad5226bba8a7f72abcb77044711c1bf04d1f618ddf82` |
| Audit tool | `e215f575f5eb23e15aa71f39cf04d2fba7432d74ae938b6b9871253f515a57c9` |
| Audit report | `04d8237948359fad338fd67319bc61e77e6e6f0505a088d7030dc3ac9e033f30` |
| Compact source fixture | `bf4a515f17d1f66d2bfe243893c3060c0c4e175733d8ed14d68fee0e30a0d0eb` |
| Existing join corroboration | `9d93d609507bd352f13d6bd9c202071f26b05ee2ea4bb942a87ce3fcdf945629` |

Files are under `analysis/multiplayer/map-pack/smg-fp-registration/`, with the
two tools under `tools/`. All earlier frozen suites remain unchanged. Native
gameplay application is false for this research. Remaining boundaries include
full local-view/player creation in this composed fixture, global heap backing,
populated effects, resource residency, first-person notification/channel clocks,
world queries and first-person posed output. Xbox reciprocal uncertainty remains
separate from the proven integer registration and node-name mapping behavior.
