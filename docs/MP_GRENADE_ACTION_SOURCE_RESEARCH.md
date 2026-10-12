# Actual allocated-player grenade request and retained dependency

This separate experiment delivers a declared native Q intent through the actual source-created multiplayer player, unit and command submission owners. It establishes genuine request encoding, source grenade selection and an actual starting-SMG ready-state rejection. It does **not** establish an admitted grenade throw, animation duration, count debit, allocation, release, trajectory, impact, fuse or explosion. No production files or prior source freezes are changed.

The later world update reaches an original texture-cache dependency. A new genuine cache constructor completes using actual source-requested raw host storage; its reset then stops at the original Xbox write-combined memory-protection request. The platform result and cache readiness are not supplied.

## Owned original context

The executable is the disc1.1 image `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`. Lockout SHA256 is `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.

`tools/oracle_mp_grenade_action_1_1.py` composes the unchanged `tools/oracle_mp_command_locomotion_pose_1_1.py` preparation. It reproduces that frozen neutral runtime byte-identically, then restores the exact source-produced pre-command guest memory and CPU fixture for each new case. Original heap/world/header allocation, join, spawn, loadout and physics create player `EC700000`, Spartan `E26F0000` and carried SMG `E2700001`; no identity, object header, high-water, valid bit, owner, count or action component is assigned. The actual weapon definition is `E57B0406`.

The frozen Wine/Cxbx complete-KPCR context remains compatibility-produced host context, not proof of ordinary Xbox thread/hardware setup. Existing cache/resource/pose/raw storage and session/controller boundaries remain explicitly inherited. No real local-player/session construction is newly established here. The original command clocks remain the prior fixed-clock fixture; repeated phase composition is not a recovered advancing match clock or original animation duration.

For each input, the tool executes original `14E780` to initialize the command, modifies only incoming command DWORD+0 flags and signed-short+22 grenade selector, then executes original `852A0` submission, `69350` assembly, `14EAE0` player phase, `B7E00` object prephysics, `1C7660` world, `B7F50` postphysics and `B8040` new-object flags. These roots are composed as declared; no new source return or instruction patch is supplied.

## Actual request and selection results

The declared Q pulse is command flag `04000000`. Actual `150F00` builds an otherwise zeroed20h action request with i32 type22 (`16000000`); actual `E7F30` uses the type22 descriptor at `46EA64` from dispatcher table `46EC68`. Its descriptor bytes are `e0950e00b0990e0000000000509a0e00`: begin `E95E0`, update `E99B0`, third callbackNONE, interruption `E9A50`.

The source-created unit has current/requested grenade bytes+23C/+23D=`FF/FF` and counts+23E `[2,0]`. The selected case first calls unchanged `CD310` with the actual unit datum, currentNONE and direction0. It returns selector0 from the real count scan. The tool delivers that returned selector on a neutral command; the original unit owner writes current/requested0. A later Q pulse reaches the same genuine type22 request path. This is source selection/request execution, not a substitute local-device producer.

Both requests return without throwing:

| Case | Actual entry to `E95E0` | Original result |
| --- | --- | --- |
| `constructor_unselected_Q` | first command Q, selectorNONE | `1055F0` returns AL1; action/counts remain unchanged |
| `source_selected_frag_Q` | source-selected frag0, later Q | `1055F0` returns AL1; current/requested0 and counts2/0 remain unchanged |

At each actual `1055F0` call, the source snapshot contains weapon own-state signed DWORD+178=`9`. Its full unchanged predicate returns false for weaponNONE; otherwise it reads actual weapon definition flags+12C bit6, then forces true when own state is in signed range5..10 inclusive. The latter branch rejects these actual ready-state9 requests. `E95E0` tests AL at `E9685` and jumps to `E993E`; later capability, animation, allocation and release gates are **not executed**. The result is not evidence that they would admit the request after ready state ends.

The action component is the source-produced unit+i16(unit+346), here `82D895F2`. Its complete44h bytes and unit bodies are retained. Active words+4/+8 remain zero; phase+0C, counter+0E and release timer+0F remain0, held-object+10 isNONE, held-type+14 isNONE. No count debit or projectile is claimed.

## Actual authored grenade metadata and static later mapping

The source matg grenade block is count2/pointer `80146F00`, stride2Ch. Actual definition rows and projectile bytes are retained without substituting another grenade or tuning:

| Type | Projectile datum | Actual tag |
| --- | --- | --- |
|0 | `E7A80623` | `objects\weapons\grenade\frag_grenade\frag_grenade` |
|1 | `E7B2062D` | `objects\weapons\grenade\plasma_grenade\plasma_grenade` |

This experiment executes no grenade damage or balance postprocessor and makes no damage-radius/fuse tuning claim from those raw bytes. The independently frozen balance-postprocessor evidence remains separate.

The previous actual dispatcher-table comparison at `analysis/multiplayer/integration/requested-controls/owned-update-functions/report.json` gives complete original body/branch correspondence to the ownedu5 executable, SHA256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`:

| Disc1.1 owner | Ownedu5 entry |
| --- | --- |
| `E7F30` dispatcher | `E8E50` |
| `E95E0` begin | `EA500` |
| `E99B0` retained update | `EA8D0` |
| `E9A50` interruption | `EA970` |
| `1055F0` weapon gate | `106510` |
| `EFEE0` animation capability | `F0E00` |
| `E8D60` held creation | `E9C80` |
| `E8F30` release | `E9E50` |

Matching complete static instructions and operands are distinct from executing an admitted later-update throw. Unknown runtime producers/callbacks remain unknown. In the source listing, update `E99B0` advances component+0E and, in phase1 when counter>=2, calls `E8D60`; actual allocation/count/attachment ownership lives there. That later branch is not reached in these negative cases and must not be replaced with guessed counts or launch behavior.

## Retained repeated-world dependency

During the second `B7E00`, original `BD360` updates the carried SMG through `10CDF0` and its `F9B10` attachment-resource callback. The next generic page allocator `13EAE0` reads allocator+34 at `13EAFA` with allocatorNULL, stopping on read address `00000034`. The requested resource size isA800. This is a missing texture-cache owner during resource loading, not a grenade admission return and not the sound cache `5098E4`.

The actual texture owner `4EDA34` is produced by `12D840`. New `tools/oracle_mp_grenade_texture_context_1_1.py` restores the same source-created pre-command world and executes that genuine constructor/reset order, retaining the original allocator pointed to by static `46FBF8`. Its only new supplied service is the actual `33785C` raw VirtualAlloc request: real host mmap storage, a declared guest alias, exact requested size and access flags, no engine fields or constructor/admission result.

Original `12D840` requests508C/6A8/30FC bytes through `11E150`; the native raw allocations are at aliases `09303000`, `09309000`, `0930A000` with capacities6000/1000/4000. Actual `16D070` and `13E8E0` write the texture, predicted-texture and cache/page fields, and original `12D8ED` publishes owner `4EDA34=0930A000`. The owner8Ch prefix and before/after source arena/cache fields are retained, together with ordered original stores. This does not claim a complete GPU cache or every unused raw-buffer byte as source initialized state.

Then original `12D950` reaches the platform data thunk at `417BA8`, kernel ordinal182 `MmSetAddressProtect`, with exact args:

`(80061000, 02CE6000, 00000404)`; original return site `12D9C3`.

The requested policy is PAGE_READWRITE|PAGE_WRITECOMBINE. The inherited physical raw backing remains its earlier explicit provider boundary; the older measured protection request covered a different RW-only range. No unsupported Xbox cache-policy return, GPU/device readiness or texture admission is supplied. The source owner remains published, but `4EDA30` texture storage is stillNULL and `13F020`/map-reset pool validity has not run. The new experiment stops here and executes no further grenade cases.

## Reproduction, audit and open integration gate

Directory: `analysis/multiplayer/integration/grenade-action`.

* `exploratory/report.json` / independent `repeat/report.json`: negative actual-player request and first texture-owner dependency, original report SHA256 `e8d29e0f3f5f3b6eb2d837a496072a100fdbdf61edcdd09e40f8ad8fb163db8c`.
* `texture-context/report.json` / independent `texture-repeat/report.json`: genuine texture constructor and exact write-combined protection stop, original report SHA256 `71017d3a24311c4c918a985ce1d83b58b49f93fb66ad11eaceb0c5e6fdfdb5b1`.

Use fresh output paths; prior tools/reports stay immutable. The dedicated audit verifies unchanged instruction bytes, actual allocated bodies/ready-state9, real type22 request buffers, count/selector states, raw storage ABI and the first unavailable platform request. The manifest verifies all earlier scope/source pins remain unchanged.

The next required positive gate is genuine weapon-ready clock completion and original action/animation admission, with the texture/protection/resource context supplied by an honest supported provider. Actual held creation, count debit, marker attachment, release, physical motion, detonation and replicated ownership remain unported by this checkpoint. The native KBM Q producer alone does not make grenade gameplay work.
