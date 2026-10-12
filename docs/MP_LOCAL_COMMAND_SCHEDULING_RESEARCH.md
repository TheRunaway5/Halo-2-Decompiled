# Original local control storage and map-ready ownership stop

Original control, view and camera storage constructors and their complete new-map resets now execute from the source-owned game arena. The complete simulation scheduler preserves the closed map-readiness gate. Complete `190A30` then enters the original map-reset traversal and stops at `19F727`: callback 13 writes to NULL+29 because its owning pool at `[4F5CBC]` has not been constructed. Local membership, device commands and the simulation state-four transition remain unexecuted. No readiness, local binding, membership, profile, command packet or device result was assigned. Production and every earlier freeze are untouched.
[Independent oracle](../tools/oracle_mp_spawn_local_schedule.py),
[complete report](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/report.json),
[separate neutral-command freeze](MP_COMMAND_INPUT_RESEARCH.md).

## Authority, repeat and retained boundaries

Original TU1.1 XBE SHA-256 is `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`; original Lockout SHA-256 is `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`. The earlier complete compatibility context is generated unchanged using real owned host storage, thread and stack services; its worker stays alive throughout source execution. Compatibility-produced unknown zeros remain compatibility outputs rather than recovered Xbox hardware defaults. The same source-owned heap, fixed capacity, object/world and empty-player producers are reused.
[Complete compatibility context](MP_COMPLETE_COMPAT_KPCR_RESEARCH.md),
[preceding player/world preparation](MP_PLAYER_VISIBILITY_RESEARCH.md).

The fixture is a declared composition of selected original roots after the preceding prepared world. It executes callback storage records 20/21/35, their new-map callbacks, the network configuration handler, selected simulation callbacks and complete `190A30`. It does **not** execute complete title startup or all 68 storage constructors. Original `139390` is the whole game constructor: it calls whole `125280`, allocates the game record and traverses the storage callbacks. The inherited arena preparation executes its preserved fragments around unavailable `217090` savegame I/O; that earlier boundary remains explicit. Existing game/options backing and the exact separately produced default Slayer variant remain input delivery, not recovered lobby/sign-in/session producers. In the complete-map call EAX receives the existing options backing at `Game+8`; the original callee copies from that supplied input contract. The source player and object pools remain empty in these cases.
[Whole game constructor listing](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/00139390.asm),
[exact preparation and full stage records](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/report.json).

The report retains entry registers, stack ABI, raw complete control 264/view 6C8/camera-time 78/camera-flag 4/static-camera 50C records, complete game 1200 and simulation 1224 records, player-global 130 records, source arena owner words, actual source stores, callback records and the exact invalid memory access. No original instruction is patched. Unique executed instructions and observed store sites are checked against the pinned XBE and both unchanged emulator code images. All earlier and clock input pins are verified before and after execution. Per-run raw host context records are separate from the deterministic source report.
[Verified executed instructions](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/executed-original-instructions.asm),
[observed source store sites](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/observed-local-source-store-sites.asm),
[independent verification](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/independent-freeze-verification.json).

| Evidence | SHA-256 |
| --- | --- |
| Oracle | `0b0e90125fb792755ece1316092ea05845f49e4c50b241ae7a9110d4b263bbba` |
| Report | `7f4286e44d419ece0c8948cdb2bcd8dd46c0d9b55a361a18560f07ada111f668` |
| Instruction manifest | `8b8156556f78356027b4735798a1ad3e4298bf134b7de1a143fe19c229975a1d` |
| Executed source instruction listing | `c978916cbc4a2a4278ea2315d38735d27071f032e38fb97225e41eab2042b728` |
| Observed source store-site listing | `de631537fa63cf4f71cbc6a38206aed51eec4ca545d270e2236fd36677004f68` |
| Additional measured clock fixture | `e672f902e7cb280c152eae2039b1ac56c7fd977e01442e8fdd8f803941ec034f` |

Two independently executed cases audit 12,397 unique original instruction addresses. The 48 routine listings contain 4,033 rows, in addition to the complete executed listing and 202 original observed store sites. All 46 earlier/input pins are preserved. The deterministic report and all 50 instruction files must reproduce byte-for-byte.


Reproduce from the repository root:

```sh
.tools/python/bin/python tools/oracle_mp_spawn_local_schedule.py \
  --output analysis/multiplayer/integration/spawn-source/local-command-scheduling/reproduction/report.json
```

## Executed control, view and camera producers

| Source owner | Actual source allocation/binding | Actual complete reset |
| --- | --- | --- |
| Record 20, `1874D0` | Arena allocation 264 bytes; `187507` binds `[4F4854]`; first byte becomes zero | `187520` clears header dwords +4/+8/+C/+10, calls `188D00` four times with EAX 0/1/2/3 and ECX NONE, then sets the first byte to one |
| Record 21, `1EA3D0` | Arena allocation 6C8 bytes; `1EA407` binds `[526250]` | `1EA410` clears the whole record, writes NONE at +80/+190/+230/+340/+3E0/+4F0/+590/+6A0, and sets byte +6C5 to one |
| Record 35, `156A30` | Clears original static 50C-byte camera record at 4F0208; allocates four bytes for `[4F0204]` and 78 bytes for `[51845C]` | Whole `156AD0` executes its actual four camera-slot branches, fills its source function/table fields and writes camera-time +2=0 and +3C=NONE |

Every constructor updates the real arena cursor and checksum through original `1656A0`; no reserved buffer, camera slot, user mapping, profile, owner pointer or readiness flag is inserted. The report preserves all literal before/after records rather than reconstructing only their zero fields. These reset flags belong to their own records; they do not set game readiness or prove a local player.
[Control constructor](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/001874d0.asm),
[control reset](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/00187520.asm),
[view constructor](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/001ea3d0.asm),
[view reset](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/001ea410.asm),
[camera constructor](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/00156a30.asm),
[whole camera reset](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/00156ad0.asm).

## Original RNG owner and declared clocks

The comparison case omits `1479B0` and retains the older explicitly supplied global RNG adapter at 01018000, initially raw `7856341200000000`. It is **not** a missing-RNG dependency case. The owning case executes whole original callback 3, `1479B0`: it consumes eight bytes of the actual source arena, binds `[4EE9D8]` at `1479E6`, writes initial dword 78A8 at `1479EC`, and initializes dword +4 at `147A13` from the XOR of actual source `time(NULL)`, `rand()` and `GetTickCount` returns. Full cold CRT fallback allocation through `32C609` executes with the existing produced heap/context. The source map reset later writes the first RNG dword through `13958B` in both cases; the fixture supplies no RNG field in the owning case.
[Owning RNG constructor](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/001479b0.asm),
[original time conversion](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/0032803e.asm),
[original rand](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/00328505.asm),
[CRT state producer](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/0032c609.asm),
[source constructor/reset stores and ABI](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/report.json).

Original `32803E` is `time(NULL)`, not `srand`. `336983` uses the pinned real-host FILETIME wrapper provider; original conversion then executes. Original `337A40` executes unchanged and reads its declared ordinal-156 data backing with measured host boot milliseconds. The earlier three simulation-new-map measurements and the two additional RNG-constructor measurements are replayed verbatim. The compatibility-produced ETHREAD field +12C is retained unchanged; a printed native thread ID is not substituted. These are explicit platform/time inputs and do not establish Xbox kernel clocks or complete ordinary title RNG history.
[Additional measured clock fixture](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/measured-extra-clock-inputs.json),
[earlier simulation clock fixture](../analysis/multiplayer/integration/spawn-source/command-input/measured-host-clock-inputs.json),
[separate CRT/RNG startup scope](SMG_AUDIO_RNG_STARTUP_1_1_RESEARCH.md).

## Executed readiness gate and exact full-map stop

Complete `68300` checks simulation enable `4D6D38`, stop flag `4D6D3A`, the game pointer and then game byte +1120. With the source reset state, +1120 remains zero and the original branch at `6832F` returns before `69170`. The source simulation context remains kind +8=1, state +18=2. Neither field is assigned to admit a later path.
[Complete scheduler](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/00068300.asm),
[raw before/after records](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/report.json).

Both complete `190A30` calls have EAX=0101A008, the declared existing options input pointer. They enter full `139410`: the original control-word call executes, then `139540` copies 0x446 dwords into game+8 and performs its original stores. The subsequent callback loop uses offset ESI from zero to 0x990 in steps of 0x24, reads the new-map target at `447430+ESI` and calls every nonzero target in ascending order. The executed nonzero callback targets are indices 2 (`11E330`), 9 (`187550`) and 13 (`19F720`); the first two return before the stop. Actual callback identities and ABI are retained, including callback 13's unchanged record at 4475FC. It contains `19F160/19F200/19F720/19F760` followed by five zero words.
[Complete map entry](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/00190a30.asm),
[whole reset and callback traversal](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/00139410.asm),
[options-copy owner](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/00139540.asm),
[actual callback rows](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/report.json).

The exact stop in both cases is:

```text
19F721  mov esi,[4F5CBC]       ; actual BSS pointer is zero
19F727  mov byte [esi+29],1    ; unmapped write to 00000029, size 1
```

At this instruction ESI=0, EAX=19F720, EBX=ECX=EDX=0. The original in-progress game byte +0 remains one at the stop; readiness byte +1120 remains zero. The partial source writes are retained, not rolled back. No failure result is supplied and the original callback is not skipped. Original `19F160` is the next owning storage constructor: it requests two allocator-backed pool records and invokes original `16D070`, binding them to 4F5CBC and 4F5CB8. Its listing is retained as the next producer to execute in a separate extension; it is not an executed producer in this freeze. Therefore complete `1395B0`, `11D780` and readiness writer `190A6E` are not reached.
[Exact callback stop](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/0019f720.asm),
[next unexecuted storage producer](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/0019f160.asm),
[pool construction ABI](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/0016d070.asm),
[complete invalid-access record](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/report.json).

## Recovered caller and membership ownership, still unexecuted

Static call flow establishes the next owning chain without assigning its inputs. `190A30` calls `1395B0` after the whole reset, then `11D780(requested signed-short BSP from options+120, 1)`. Only a true AL from that complete BSP entry admits the `190A6E` store of one to game+1120. `11D780` has an original cached-BSP fast path and a full callback/loading/switch path; neither is reached by these cases. Bounded earlier direct world/PVS construction does not substitute for this full-map scheduling.
[Map-ready store order](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/00190a30.asm),
[complete BSP entry](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/0011d780.asm).

`1395B0` takes game+270's member mask and game+274's sixteen six-byte keys into `14DDA0`; it passes game+2D5's local key to `14DFF0` when game+2D4 is present, and then visits sixteen E4-byte creation records from game+2DC, calling `14D350` only for occupied records. `14DDA0` owns player-global member mask +2C, key table +30 and membership remapping. `14DFF0` owns local presence +90, key +91 and member index +98, detaches existing local mappings and invokes `1508B0`/`150950` for matching players using their original user/controller fields. Those game-options identity and profile producers remain unresolved; no record is populated here to make this path succeed.
[Identity creation owner](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/001395b0.asm),
[member-key owner](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/0014dda0.asm),
[local-key/binding owner](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/0014dff0.asm),
[user binding](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/001508b0.asm),
[controller binding](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/00150950.asm).

The whole ordinary loop `12CBC0` calls simulation scheduler `68300` and uses `68460` before its time/input path to `188AD0`. That local-command entry reads the four actual player-global current local player datums, calls `1884E0` for present owners and passes its produced mask to `69280`. `1884E0` obtains the real player's +28 user and +24 controller fields and the control record at `[4F4854]+14+94*user`, then invokes `187610` to consume processed device input. `69280` resolves actual session/member identity through `6A860` before `852A0`. Readiness permits `69170`; its kind-one/state-two path calls `69E90→6B650→6B750`, which owns state four. These are static original caller/producers only. Full `12CBC0`, those state transitions, session membership, raw-device ownership/delivery and local command scheduling are not execution claims. No earlier explicit neutral-command roots are entered by this suite.
[Ordinary loop](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/0012cbc0.asm),
[input admission gate](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/00068460.asm),
[local dispatch](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/00188ad0.asm),
[command construction](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/001884e0.asm),
[processed input consumer](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/00187610.asm),
[session submission](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/00069280.asm),
[state transition](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/00069170.asm),
[state-four writer](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/0006b750.asm).
