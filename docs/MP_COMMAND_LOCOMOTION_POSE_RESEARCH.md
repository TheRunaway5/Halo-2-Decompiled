# Original admitted command, locomotion request, selection and current pose

The original allocated Spartan now has measured non-neutral command-to-animation coverage: the unchanged source submission/delivery/player/object chain produces movement classifications, selects the authored four rifle movement clips, and updates19 local channels and all33 current matrices. The experiment supplies only explicitly declared command axes, and in separately named cases a look heading taken from the actual chosen spawn row. It never assigns physical mode, world velocity, movement classification, activity request, animation clip/frame, pose, contact, admission or success.

This is a bounded source-owned command consumer seam. Natural controller/local-member ownership, ready session scheduling and a finite successive full-frame clock remain open. Dynamic execution uses unchanged TU1.1; the admitted owners have separately frozen complete TU1.5 instruction correspondence in [MP_LIVE_LOCOMOTION_OWNERS_U5_RESEARCH.md](MP_LIVE_LOCOMOTION_OWNERS_U5_RESEARCH.md). The work does not silently turn the TU1.1 trace addresses into TU1.5 addresses or claim a full TU1.5 owning runtime.

## Immutable measurements and reproduction

```sh
.tools/python/bin/python tools/oracle_mp_command_locomotion_pose_1_1.py \
  --output analysis/multiplayer/integration/inventory-source/command-locomotion-pose/measured/reproduction/report.json
.tools/python/bin/python tools/oracle_mp_command_locomotion_pose_audit_1_1.py \
  --directory analysis/multiplayer/integration/inventory-source/command-locomotion-pose/measured/reproduction
```

| Frozen artifact | SHA256 |
|---|---|
| [Original execution/observer](../tools/oracle_mp_command_locomotion_pose_1_1.py) | `807e303f942380bc809816e00b28053cc1c78e8542a1ac92516ab26cc25645ad` |
| [Independent buffer/instruction auditor](../tools/oracle_mp_command_locomotion_pose_audit_1_1.py) | `0ef9d64f1628695b1475a6986d588f7011a6d406a0c1927670612afa7f1a12be` |
| [Measured report](../analysis/multiplayer/integration/inventory-source/command-locomotion-pose/measured/report.json) | `12b7411de3bfa03f089de889d860011fb57cf20f9128cadaa4a1e272b1b65dd0` |
| [Literal buffer audit and compact observed cases](../analysis/multiplayer/integration/inventory-source/command-locomotion-pose/measured/audit.json) | `5a3c8dc9382f021e79882edb8bf5e98bd710654bb63803b22b1cd50104c1452f` |

The report retains15 completed command/object compositions,2,642 actual calls,244 full deduplicated states,69,221 operand reads,179,204 ordered stores and69,877 arithmetic/branch CPU snapshots. The auditor verifies209 prior pins and83 original instruction files/25,424 listing rows. It replays1,470 complete owning-buffer intervals and4,199 packet intervals against121,776 overlapping read/store observations. Full x87 registers/control/status, SSE registers/MXCSR, raw stack arguments and input/output packet alias addresses are retained. No expected output is obtained from a native implementation.

The original authority is `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`, with the unchanged Lockout/shared cache bytes. The entire positive neutral runtime from [MP_COMMAND_INPUT_RESEARCH.md](MP_COMMAND_INPUT_RESEARCH.md) compares byte-identically first under the new observer. Its exact actual114,573,312-byte guest backing/CPU context is captured after `source_complete_early_player_phase`, before the first explicit command and motion composition. Each independent case restores that same source output; all22 mapping ranges and their hashes are retained. The source allocation/player/loadout/PVS and all initialization services remain the earlier declared ones.

Fresh-process report/auditor/instruction reproduction is byte-identical. The adjacent `manifest.json` pins the complete final artifacts. `draft-*` captures remain separate excluded development observations. Every older tool/report/doc is unchanged, and this extension makes no production edits.

## Exactly what enters from outside

Every case calls original `14E780`, then changes only command+0C/+10 to its declared two binary32 input words. Submission through `852A0` executes the actual original codec; assembly through `69350` and full `14EAE0` delivers the resulting actual5C command. Small input and view-angle quantization are preserved rather than replaced with ideal input values. These are explicit command roots, not a successful whole device scheduler.

The authored-heading cases additionally call original `11DDA0` with EAX=0, EDX=actual chosen start row, ECX=command+4. That routine copies the row's actual+0C heading word to the declared command-look destination. This is an **explicit external command-look delivery**, not the natural controller heading initializer. The actual spawn chooser selected Lockout row37 at file `01D26E9C`, virtual `8015369C`; the untouched row heading is `3FC4B24A`. The source codec delivers `3FC4AB07`, and its zero input pitch becomes `3A491C93`. Both the source row and submitted/delivered words are retained and checked against original cache bytes.

The other eight cases retain the source neutral initializer's yaw zero. That means zero movement axes do not imply the authored unit is looking in its initial facing direction. No yaw, facing or activity is overwritten on the unit to hide the difference.

## Measured requests and selected clips

All15 cases admit original `151A20` with AL1 on the real player `EC700000` and allocated unit `E26F0000`, actual type0/parentNONE/physical-state1 and genuine `E5680` false. The owned SMG remains `E2700001` in the source inventory. Classification and all activity/clip/frame outputs below are measured results.

| Command case | Actual classification | Original final activity | Actual clip/name | First updated frame bits |
|---|---:|---|---|---|
| Authored heading, zero axes | 0 | `0400000C` | 185 `combat:rifle:idle` | `3F800000` |
| Authored heading, +forward | 4 | `0A000014` | 189 `combat:rifle:move_front` | `00000000` |
| Authored heading, -forward | 5 | `09000015` | 188 `combat:rifle:move_back` | `00000000` |
| Authored heading, +side | 3 | `09000016` | 190 `combat:rifle:move_left` | `00000000` |
| Authored heading, -side | 2 | `0A000017` | 191 `combat:rifle:move_right` | `00000000` |
| Authored heading, declared forward1/16 | 0 | `0400000C` | 185 idle | `3F800000` |
| Authored heading, declared forward1/4 | 4 | `0A000014` | 189 move_front | `00000000` |
| Zero yaw, zero axes | 0 | `0A00000F` | 221 `combat:rifle:turn_right` | `00000000` |
| Zero yaw, each single full axis | 4/5/3/2 | `0A00000F` | 221 turn_right | `00000000` |
| Zero yaw, +forward/+side diagonal | 4 | `0A000014` | 189 move_front | `00000000` |
| Zero yaw, +forward/-side diagonal | 4 | `0A00000F` | 221 turn_right | `00000000` |
| Zero yaw, declared forward bits`3C23D70A` | 0 | `0A00000F` | 221 turn_right | `00000000` |

`E1800` writes the turn request **before** the physical classifier. `E44C0` changes the activity only when its incoming request activity is idle `0400000C` (besides its actual mode/state gates). A movement classification therefore does not automatically select a movement clip: the original turn request retains precedence in the yaw-zero cases. The positive diagonal also exercises original cardinal-facing selection, rather than choosing a clip based on guessed world direction. [Actual call/packet/store records](../analysis/multiplayer/integration/inventory-source/command-locomotion-pose/measured/report.json).

Newly selected movement/turn channels end this first bounded update at frame0. The retained idle channel advances frame0→1. The original request/selection/clock ordering produces this difference; sampling a new clip at `global_tick*30` would not reproduce it. All15 source `BDF80` updates change the same19 local node indices and all33 current matrices. `113830` receives the genuine21-node prefix and actual mask; it does not sample all33 local nodes merely because all33 current matrices are stored. [Compact literal audit](../analysis/multiplayer/integration/inventory-source/command-locomotion-pose/measured/audit.json).

## Stored-field ownership and required order

The exact root order is `14E780 → [declared inputs / optional11DDA0] → 852A0 → 69350 → 14EAE0 → B7E00 → 1C7660 → B7F50 → B8040`. All roots return unchanged, with no gameplay/physics service used during these added command/object phases. The source-created player/header/PVS, queues, shape/body/world state and terrain/negative-contact decisions are restored from actual owning output, never manufactured per case.

| Stage | Original required input/output |
|---|---|
| `151C90` | EAX=full player datum; stack `(unit_datum, command5C_pointer)`, RET8. Reads command yaw/pitch and axes, constructs actual7C control. |
| `151AA0` | EAX=player datum, EBX=command pair+0C; stack `(aim12, facing12_output, motion12_output)`, RET12. Uses actual current-unit admission and unit+70 fallback basis. |
| `151980` / `30BD0` | Flattened actual aim/unit forward, x87 normalization/fallback and side `{-Y,X,0}`; in/out buffers and full FP outputs retained. |
| `E7B10` | EAX=world movement12, EDI=actual unit forward12, ESI=aim12; source stack flags/output pointer. Chooses the original facing candidate, including real aliasing of aim/output. |
| `C8250` / `C8140` | Default7C constructor followed by whole source copier. Control+14/+18/+1C become unit+1B0/+1B4/+1B8, in the original store order. |
| `DE960` | Consumes those owned unit motion words, applies its original deadzone, then calls motion/request/physical/animation producers. |
| `E1800` | Actual body/definition/manager/control fields produce the activity request and current facing stores before locomotion classification. Complete source unit and request12 before/after are retained. |
| `DDBC0 → 1E9540` | Actual unit+1B0 pointer becomes request+DC/+E0/+E4. The full154-byte physical request and all incoming physical/state vectors remain literal source buffers. |
| `E3B20 → 1E8F60 → 1E94E0/1F52F0` | Genuine physical-state1 and actual physical result produce the70-byte classification; request flags/other states are not invented. |
| `E44C0` | EAX=unit datum; EBX/EDI=request mode/activity pointers; stack `(classification,blend,request+8)`, RET12. Retains existing turn activity; otherwise source mapping produces the movement activity. |
| `E7E60 → 112E10` | Actual final request, manager/graph/current channels and earlier explicit dt feed original channel selection/clock. |
| Dirty pose `BDF80 → 113830` | Actual masks/resources/local callback produce the local banks; original matrix/current-node/bounds consumers update the stored current bank used by both visual and collision pose. |

`E7B10` is an essential input owner. In the +/+ diagonal, the submitted pair remains1/1; source `151AA0` produces facing bits`3F3504F3/3F3504F3/0` but motion bits1/0/0 after original facing selection and projection. A direct Controls-pair-to-classifier adapter would omit that transformation. All register/stack packet inputs, output addresses and ordered reads/stores are retained for an independent native port; no recomputed expected geometry is used.

The source deadzone is also measured on a **nonzero delivered** vector. Declared1/16 is quantized by original submission to motion X=`3D888889`. `C8140` stores that word, then original `DE960` replaces the vector with source zero before request construction and classification. Declared1/4 becomes `3E888889`, survives the same owner and produces classification4. The threshold is actual source word `3C23D70B`, loaded from TU1.1 `4654C0` / TU1.5 `46A560` before comparing with `(X²+Y²)+Z²`. The smaller `3C23D70A` declared input is already quantized to zero by the codec; it is a distinct upstream effect. [Complete DE960 and input store sites](../analysis/multiplayer/integration/inventory-source/command-locomotion-pose/measured/instructions/000de960.asm), [TU1.5 correspondence](MP_LIVE_LOCOMOTION_OWNERS_U5_RESEARCH.md).

## Concrete native integration seam

The source-controlled unit needs a persistent owned control prefix, physical-state byte/fields, manager primary/secondary channels, local/current banks and actor forward/up basis. Current `Simulation::Unit` does not represent that whole state. `unit_input.cpp` faithfully copies7C but has no live command constructor; `player_command.cpp` implements neutral5C/validation/cached delivery only. The next smallest owner to port is the admitted `151AA0/151C90` command-to-control path, including `151980/30BD0/E7B10` and exact source default constructor/copy. Its output should feed the existing classifier only through the original unit/deadzone/request stages.

For `Simulation/MultiplayerMatch`, the external host input adapter must explicitly define its command-axis/look/button delivery. The present Controls.forward/right/yaw/pitch are usable declared input boundaries, but the source device profile/sign/scaling/quantization/local-member producer is not established by these cases. This report proves command+10 positive selects left under the actual admitted source basis; it does not assert unmeasured Xbox device-layout ownership. Original yaw/pitch codec output and heading/basis stores must remain part of the composition rather than being replaced with host ideal values.

The renderer currently skins `player.idle` at global tick time and uses the same idle hand sample for the carried item. It should consume the owned source primary/secondary local/current pose for the corresponding object phase, so rendered skin, hand attachment and posed collision share that state. Existing [equipped/unit-phase](SPARTAN_EQUIPPED_UNIT_PHASE_1_1_RESEARCH.md) freezes independently retain source firing/reload requests and clocks under their declared ready/action fixture. They must not overwrite this allocated owner's readiness or become guessed transitions from native fire/reload booleans.

Native `Body.grounded`, world velocity and actor yaw do not establish original physical state1, source unit+70 or channel activity. The host triangle/capsule solver remains a different producer from the original Havok state. This experiment's source-created physical1 is valid for its literal one-update fixture; it does not authorize supplying1 for every live unit or mapping grounded to1. Full local device/readiness, options rate/finite game clock, successive admitted frames, positive physical contacts, source current-state producers on every live update and Xbox reciprocal hardware parity remain open. Original matrix sampling uses the existing declared reciprocal/emulator boundary. Player-versus-player damage is not measured by this animation extension.
