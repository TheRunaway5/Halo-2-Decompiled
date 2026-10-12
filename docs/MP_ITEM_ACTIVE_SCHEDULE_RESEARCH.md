# Original first active authored-item schedule and terrain contact

Research date: 2026-10-08. The source-created player, allocated SMG, genuine player cluster/PVS, original object heap and world now admit the unchanged first normal item update for Lockout rows **0, 1 and 12**. All four selected object phases return. The two emitted item terrain casts execute against original collision/cache data; row 12 produces an actual terrain hit and subsequent root/contact/node/bounds/location/registry stores. No activation mask, near root, node pose, contact, allocation success or game/Havok return is supplied by this extension. Production and every preceding freeze remain unchanged. [Oracle](../tools/oracle_mp_item_active_schedule_1_1.py), [actual input/output report](../analysis/multiplayer/integration/inventory-source/item-active-schedule/report.json), [audit](../analysis/multiplayer/integration/inventory-source/item-active-schedule/audit.json).

This is **one bounded initial item schedule**, with the inherited tick/rate inputs. It establishes the executed current-state producers, rather than a complete title/session/frame, continuous settling, positive Havok/object/instanced contact, local command input or Xbox numerical parity. The selected item row-table backing and preceding platform/resource/optional attachment/audio boundaries remain explicit. [Source player/PVS environment](MP_PLAYER_VISIBILITY_RESEARCH.md), [prior zero-mask item constructor](MP_ITEM_CURRENT_PRODUCERS_RESEARCH.md).

## Authority, pins and identity

Primary authority is unchanged TU1.1 `default.xbe.03`, SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`, and original Lockout cache, SHA-256 `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`. Exact original/shared graph-resource bytes remain their pinned inputs. All **167 preceding pins** are checked before and after execution, including the consumed-options/transfer and player/PVS freezes. All 46,509 executed original addresses and all listed/store instructions match the executable and guest bytes. [Report pins and instruction index](../analysis/multiplayer/integration/inventory-source/item-active-schedule/report.json), [complete executed bytes](../analysis/multiplayer/integration/inventory-source/item-active-schedule/instructions/executed-original-instructions.asm).

The frozen player helper `prepare(data, trace_type=ActiveTrace)` owns the selected original initialization sequence; `spawn_and_activate(t,0,activate=False)` runs the source join/spawn and `14EAE0..14ED0C` cluster producer, stopping before PVS. Its four complete stored prefix records are byte-identical to case `bound_player_first_ordinary_motion` in the immutable player report. The source chooser retains authored start **37**, player **EC700000**, unit **E26F0000**, carried SMG **E2700001**, and actual player cluster **4**. No player cluster or local-user binding is supplied. [Owning prefix correlation](../analysis/multiplayer/integration/inventory-source/item-active-schedule/report.json), [source-owned helper](../tools/oracle_mp_spawn_player_visibility.py).

Actual `15C320` / `15C370` initialize/reset the inherited row-table backing, then `15C110(entries,row)` executes unchanged for rows 0, 1 and 12. Original construction stores exact authored roots and allocates distinct full datums. These rows remain linked to their same datums through the entire schedule; no definition replacement or object deletion occurs. [Row constructors and literal metadata](../analysis/multiplayer/integration/inventory-source/item-active-schedule/report.json), [15C110 bytes](../analysis/multiplayer/integration/inventory-source/item-active-schedule/instructions/0015c110.asm).

| Row | Definition | Full datum / body pointer | Original allocation root bits, X/Y/Z | Loaded / reserve |
|---|---|---|---|---|
| 0 | BR | `E2710002` / `82D89B50` | `C05E30D3 40CDE93A C0FE6373` | `36 / 72` |
| 1 | BR | `E2720003` / `82D89EA0` | `40D893DB C025DEDE C0BACCCE` | `36 / 72` |
| 12 | SMG | `E2730004` / `82D8A1F0` | `BFC0B87C 412A5C92 C1432F1B` | `60 / 120` |

Both ammo fields and zero heat survive unchanged. Source shared RNG stays **12345678** through these allocations and selected phases; tick remains the declared input **0**. These are actual stored outputs, not generated native fixture expectations. [Full states, row references and ordered stores](../analysis/multiplayer/integration/inventory-source/item-active-schedule/report.json).

## Exact phase and producer order

Original `139750` source order is player `14EAE0` at **13978F**, netgame `15D7C0` at **1397AC**, masks `13A380` at **1397BB**, ordinary `B7E00` at **1397C0**, physics `1C7660` at **1397C5**, dirty `B7F50` at **1397CA**, and new/cleanup `B8040` at **1397CF**. The new bounded composition runs the recovered player prefix, selected direct original row producers in the netgame position, full masks, then those four object phases. It does not replace omitted `139750` stages or assert full-frame execution. [Original scheduling instructions](../analysis/multiplayer/integration/inventory-source/item-active-schedule/instructions/00139750.asm).

Full original `13A380` produces **FFDF0000 followed by sixty zero bytes** from the real player/PVS producers and executes the object activation callbacks. All three world-item headers change **4A→4B**; unit changes **4A→4B**; the carried SMG remains **AA**. Full `B7CE0→B7C50` uses each header's source location and actual mask. The allocated items retain their original roots and unsorted registry membership. [Actual mask/activation calls and stores](../analysis/multiplayer/integration/inventory-source/item-active-schedule/report.json), [B7CE0](../analysis/multiplayer/integration/inventory-source/item-active-schedule/instructions/000b7ce0.asm), [B7C50](../analysis/multiplayer/integration/inventory-source/item-active-schedule/instructions/000b7c50.asm).

| Selected phase | Executed item behavior and exit |
|---|---|
| `B7E00` | Queue count is actually zero. General occupied visits skip new8 items. All three world-item headers remain `4B`; no item BD360 occurs. |
| `1C7660` | Original created world steps without a game/Havok supplied return. The unit becomes dirty (`4B→4F`); all three world-item roots/headers remain unchanged. |
| `B7F50` | Queue count remains zero. General occupied visits skip new8 even when dirty4 is present. No item BD710 occurs. |
| `B8040` | Occupied bitmap order admits the first new item ticks; all original callbacks return. Final world-item headers are row0 `43`, row1 `47`, row12 `47`. |

`B8040` clears already-updated20 at **B8104**, checks new8, active1, update2 and delete10, then clears new8 at **B8128 before BD360**. Dirty4 after that call causes BD710 at **B813B**. The first BD360 invocation order is exactly **E26F0000 → E2700001 → E2710002 → E2720003 → E2730004**. The carried-SMG call is reached through the original parent's child recursion; its actual stored header governs its early return. The world items each execute the item callback **F9B10→F9EA0**, then weapon callback **1021D0**, through original indirect type dispatch at **10CE3B**. [B8040](../analysis/multiplayer/integration/inventory-source/item-active-schedule/instructions/000b8040.asm), [BD360](../analysis/multiplayer/integration/inventory-source/item-active-schedule/instructions/000bd360.asm), [actual indirect targets/arguments/order](../analysis/multiplayer/integration/inventory-source/item-active-schedule/report.json).

Row 0 enters F9EA0 with literal runtime bytes `+C0=40,+C1=01`; that stored branch leaves its root unchanged and emits no outer cast. Row 1 enters with `40,00` and emits one cast; row 12 enters with `40,00` and emits one cast. Do not replace these stored states with a common first-item physics default. [Per-call original entry/return buffers](../analysis/multiplayer/integration/inventory-source/item-active-schedule/report.json), [F9EA0 branches](../analysis/multiplayer/integration/inventory-source/item-active-schedule/instructions/000f9ea0.asm).

## Actual terrain casts and contact stores

Both original item casts are `16B240(mask=24909C0D, start, delta, NONE, NONE, output)`. Each start is the owning F9EA0 item's actual current root; source-produced delta bits are `00000000 00000000 BB97E000`. The report retains raw entry registers, all six stack words, exact vectors, output pointer and literal `5C` output bytes. Only AL is the defined hit Boolean; raw full EAX remains provenance. [Measured outer inputs/outputs](../analysis/multiplayer/integration/inventory-source/item-active-schedule/report.json), [16B240](../analysis/multiplayer/integration/inventory-source/item-active-schedule/instructions/0016b240.asm).

| Item | Actual outer result | Original lower visits | Final root bits |
|---|---|---|---|
| Row 1 BR | Type `0`, AL `0` | One `1E20C0`, leaf `[1352]` | `40D893DB C025DEDE C0BAF2C6` |
| Row 12 SMG | Type `1`, AL `1`, fraction bits `3E5D061C` | Two `1E20C0`, leaves `[2243]`, `[2243]` | `BFBC0231 412B1C12 C14300F3` |

Each lower call retains the original descriptor's literal `28` bytes before/after, result `420` bytes and actual visited leaves. The SMG's actual terrain contact identifies surface **2975 / B9Fh**, plane **000004FF**, material **17 / 11h**. The corresponding original eight-byte surface, sixteen-byte plane and twenty-byte material records are copied and verified against the untouched cache file. The second lower visit and original wrapper point changes remain actual source outputs; no correction geometry is reconstructed for expectations. [Literal contact cache records](../analysis/multiplayer/integration/inventory-source/item-active-schedule/report.json), [original lower traversal](../analysis/multiplayer/integration/inventory-source/item-active-schedule/instructions/001e20c0.asm).

The original **246A50** cluster-query bodies also execute for clusters **9** and **15**, then return AL `0`. Their literal cluster `B0` records retain the consumed **+A8 counts 624 and 96**. `246AAE→2E5CE0` and `246C26→2E5CA0` are executed source instructions, not zero-count shortcuts or supplied returns. The +A8 values are retained as an opaque source field here; this report does not invent a shape-count interpretation. These two actual misses do not prove positive Havok/object/instanced contacts. [246A50 original body](../analysis/multiplayer/integration/inventory-source/item-active-schedule/instructions/00246a50.asm), [executed bytes and actual cluster records](../analysis/multiplayer/integration/inventory-source/item-active-schedule/report.json).

F9EA0 owns the measured BR gravity/motion result and SMG contact response. SMG item flags `+12C` become **0020**, contact bytes `+130..+13B` become **0000FFFF9F0B00FF00000000**, and angular Y is actual signed-zero bits **80000000**. Its runtime bytes become `40,01`; object flags change `00001B00→20001B00`. These are stored instruction outputs. No approximate gravity, friction, plane or orientation algorithm is added. [Ordered source stores](../analysis/multiplayer/integration/inventory-source/item-active-schedule/report.json), [observed opcode sites](../analysis/multiplayer/integration/inventory-source/item-active-schedule/instructions/observed-store-sites.asm).

After moving item returns, BD710 executes **BDF80 current nodes → type dirty dispatch → BEDE0 bounds → B96A0/B94F0 registration work**, including actual `1CE650` insertion. All node banks, bounds and locations are retained literally at producer entry/exit. Final unsorted collidable chains are cluster4 `[E26F0000]`, cluster9 **[E2720003,E2710002,E26F0000]**, cluster15 `[E2730004]`; each next link and raw record remains in source order. Item owners/parents/physical components remain NONE. Source item motion here is not an invented rigid-body component. [BD710](../analysis/multiplayer/integration/inventory-source/item-active-schedule/instructions/000bd710.asm), [exact chain and current-node snapshots](../analysis/multiplayer/integration/inventory-source/item-active-schedule/report.json).

## Literal dispatcher comparison contract

`stages[].before_state/after_state` identifies actual phase buffers; `calls[]` retains original caller, registers, six raw stack words, entry/return order and raw returned EAX. For the measured dispatcher/leaf producers, before/after state IDs also pin their exact stored entry/exit buffers. Tail-jump frames sharing original return address and entry stack depth are observed together, including **B8040→C0270**; recursive frames at different depths remain distinct. No return is supplied by the observer. [Compact schema](../analysis/multiplayer/integration/inventory-source/item-active-schedule/schema.json), [per-call records](../analysis/multiplayer/integration/inventory-source/item-active-schedule/report.json).

Every state retains:

- The original **2048×12 complete header buffer**, `4C` pool header, complete 256-byte occupancy bitmap, full occupied object bodies and their actual current node banks. Pool is `82D81E24`, data `82D81E70`, bitmap `82D87E70` in this source-owned fixture.
- The complete source **1D88-byte queue at 82EB5FB4**, through `player.unit_queue1d88_hex`. Actual queue count `+1D80=0` and `+1D84=1` remain unchanged in this schedule.
- `object_context`: **global4E58C4**, pointer **82E87FC0**, raw **84 bytes** and byte `+3`. This is the same source object-world/context record already retained under `current.object_world`; it is distinct from global4E58A0 scratch storage.
- Full current unit/item/bounds/location/registry data, raw world/body/motion records, row-table datum bytes, actual source mask banks, shared RNG and tick.

The ordered stream labels actual context-target stores and retains exact before/written bytes. Its six measured `+3` stores are:

| Phase | Entry store | Actual exit store |
|---|---|---|
| B7E00 | **B7E24**, `00→01` | **B7F43**, `01→00` |
| B7F50 | **B7F6E**, `00→01` | **B7FEF**, `01→00` |
| B8040 | **B804F**, `00→01` | **B80D5**, `01→00`, before tail C0270 |

Actual **1019F0** and **C0270** entry/exit snapshots are included. Queue-priority branches, alternative B7E00 exits **B7EC4/B7F33**, and delete10 **B92A0/B9350** branches remain unexecuted. Their unchanged complete listings are retained separately; no expected native result or callback argument is fabricated for them. The raw stack words beyond source-owned ABI reads remain provenance, not new arguments. [Phase listings and callbacks](../analysis/multiplayer/integration/inventory-source/item-active-schedule/report.json), [B7E00](../analysis/multiplayer/integration/inventory-source/item-active-schedule/instructions/000b7e00.asm), [B7F50](../analysis/multiplayer/integration/inventory-source/item-active-schedule/instructions/000b7f50.asm).

This data can drive a bounded native dispatcher differential over literal headers, bitmap, queue and context, retaining explicit BD360/BD710/1019F0/B92A0/B9350/C0270 producer boundaries until their native implementations exist. It is not a claim that those live producers or the complete world already have native parity.

## Freeze and remaining scope

Coverage is **10 selected stages, 145 observed calls, 70 source states, 4,506 ordered stores, 12 indirect callbacks, 3 active world-item ticks, 2 actual outer casts, 3 lower traversals and 1 positive terrain contact**. There are **279 instruction files / 72,293 rows**, including complete executed and observed-store listings. No supplied game/Havok return occurs in any of the four selected scheduled phases. A separate process reproduces the report, all instruction files and audit byte for byte; all 167 prior pins remain intact. [Reproducibility](../analysis/multiplayer/integration/inventory-source/item-active-schedule/reproducibility.json), [manifest](../analysis/multiplayer/integration/inventory-source/item-active-schedule/manifest.json).

Oracle SHA-256: `fd63e85b9b799bcf5654b11873ae08bb782d7d0f4e22b64a1c5075287c54dc23`. Auditor: `83046c94612efee322419884acabd85382e1f900c31d23087727f77d333e0ace`. Report: `e347e495f8c37fb3f56b69835692754cb65b77947a8459fea34352ce61ce4be2`. Earlier pre-final captures remain under `drafts`, including their identical preliminary repeat; the missing explicit context alias and incorrect preliminary +A8 label were corrected before this freeze. No rejected/pre-final capture is promoted to final fixture authority.

```sh
.tools/python/bin/python tools/oracle_mp_item_active_schedule_1_1.py
.tools/python/bin/python tools/oracle_mp_item_active_schedule_1_1.py --output analysis/multiplayer/integration/inventory-source/item-active-schedule/reexecution/report.json
.tools/python/bin/python tools/oracle_mp_item_active_schedule_audit_1_1.py
.tools/python/bin/python tools/oracle_mp_item_active_schedule_audit_1_1.py --output-dir analysis/multiplayer/integration/inventory-source/item-active-schedule/reexecution
```

The remaining ordinary pickup composition needs source local device/profile/command ownership and its held-input phase, the measured current player/animation schedule and this actual current item state. Full session/title/all-world startup, additional frames/continuous item settling, positive Havok/object/instanced contact and Xbox reciprocal/contact behavior remain separate unproved gates. The earlier independently measured admission/transfer/attachment kernels retain their explicit stored-state scope; this new source producer does not silently replace it with a live completion claim. [Held-input transfer scope](MP_PICKUP_TRANSFER_MEASURED_RESEARCH.md), [consumed options input](MP_PICKUP_TRANSFER_OPTIONS_RESEARCH.md), [attachment numerical boundary](MP_WEAPON_ATTACHMENT_RESEARCH.md).
