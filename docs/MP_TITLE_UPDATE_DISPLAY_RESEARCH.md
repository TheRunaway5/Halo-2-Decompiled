# Original numerical update display: owned 1.1 and authenticated u5

The authenticated English u5 executable **produces `1.5` through its original `build_number` text callback and complete private-marker replacement path**. The corresponding owned 1.1 executable produces `1.1`. Both full original formatter/CRT paths execute without supplied service or formatting results. This numerical display evidence comes from original executable bytes and actual emitted text; it is independent of certificate803, the `u5` directory, readme claims or annotation names/comments.

This is bounded text production with declared ABI/text storage, not a complete title-screen render or a claim that every live gameplay producer has migrated to this executable.

| Source | SHA256 | Callback → formatter | Marker replacement |
|---|---|---|---|
| Owned TU1.1 `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03` | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` | `22F94F→164BE0` | `22F40E` |
| Authenticated English u5, imported outside the repo | `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd` | `234232→165FC0` | `233CF1` |

The second path is `/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe`. Root's RSA/header/section audit separately owns authenticity and certificate interpretation. This report identifies the executable by its full hash and does not claim a latest-ever update.

## Exact original producer

The u5 callback at`234232` preserves ESI, loads its second stack argument as the output buffer, pushes ASCII literal pointer`463994` (**`31 2E 35 00`**, `1.5`), then wide format pointer`45FA60` (**`25 00 68 00 73 00 00 00`**, `%hs`). It calls original`165FC0`, releases those two formatter arguments, restores ESI and returns with `RET8`. The earlier callback at`22F94F` has the same ABI, with ASCII pointer`45E858` (**`31 2E 31 00`**, `1.1`) and format pointer`45AE98`. [Original callback listings](../analysis/multiplayer/integration/inventory-source/title-update-display/instructions/authenticated_u5-00234232.asm), [earlier callback](../analysis/multiplayer/integration/inventory-source/title-update-display/instructions/owned_1_1-0022f94f.asm).

The formula is **wide-format `%hs` of the executable's ASCII constant**. There is no numeric calculation from a certificate or version field. The adjacent `%.1f` constant belongs to a different callback; it is not the update label's producer.

`165FC0` builds a varargs pointer from stack+8, passes output ESI/count`FF`/original format/varargs to original CRT`32BFF9`, then forces a terminating UTF-16 zero at output+`1FE` and returns EAX=output. The complete CRT executes; it is not replaced with a host format function or a successful return. [Formatter](../analysis/multiplayer/integration/inventory-source/title-update-display/instructions/authenticated_u5-00165fc0.asm), [all actually executed original instructions](../analysis/multiplayer/integration/inventory-source/title-update-display/instructions/authenticated_u5-executed.asm).

The u5 source table's record38 at`47CAE0` is exactly:

| Offset | Original field |
|---|---|
| +0 | `4645CC`, original UTF-16 key `build_number` |
| +4 | `0000E415`, private UTF-16 replacement marker |
| +8 | `00234232`, callback |

The table base is`47C918`,131 records of12 bytes (bound`624`). The earlier image's record38 is at`4778B0`: key pointer`45F494`, marker`E415`, callback`22F94F`; its table base is`4776E8`. The key text is read from each image's original bytes, not supplied by the annotation export. [Literal table/key/marker records](../analysis/multiplayer/integration/inventory-source/title-update-display/report.json).

Original`233CF1` admits private characters strictly above`DFFF` and below`F900`, scans marker fields at table+4 in original12-byte order, resolves callback at table+8 and invokes it with `(marker,local512_text)`. For marker`E415`, this invokes the callback above. It then replaces the one-character marker in the caller's text with the source-produced three-character string, retains the original capacity test and updates termination. The complete path, including original string length and memory move/copy helpers, executes on declared input `E415,NUL`, capacity256. The result is UTF-16 **`31 00 2E 00 35 00 00 00`**. The earlier full resolver analogously emits UTF-16 `1.1`. [Original resolver](../analysis/multiplayer/integration/inventory-source/title-update-display/instructions/authenticated_u5-00233cf1.asm).

## Frozen measurements and limits

[Oracle](../tools/oracle_mp_title_update_display.py) runs four independent fresh address spaces: direct callback and complete marker resolver in each image. It loads original raw sections with zero virtual tails, then supplies separate stack/output backing and declared text/ABI. It executes neither included attachment scripts nor full XAPI/CRT/game initialization, and does not manufacture readiness or a title-screen/widget state. Every emitted character is an original instruction output.

[Auditor](../tools/oracle_mp_title_update_display_audit.py) verifies original bytes for1,218 listing rows and replays **all2,426 memory reads and1,336 ordered stores**, including stack/local/string/table accesses and every byte of all four512-byte output buffers. No service/format results or instruction patches are present. A second fresh-process execution reproduces report, audit and all eight instruction files byte-identically. Earlier frozen oracles/reports and production remain untouched.

```sh
.tools/python/bin/python tools/oracle_mp_title_update_display.py \
  --output analysis/multiplayer/integration/inventory-source/title-update-display/another-repeat/report.json
.tools/python/bin/python tools/oracle_mp_title_update_display_audit.py \
  --output-dir analysis/multiplayer/integration/inventory-source/title-update-display/another-repeat
```

Original report SHA256: `6b824f7e5544637b51953d087fd686ffe77b76df88febba13e784c2fb768b8d7`. [Freeze manifest](../analysis/multiplayer/integration/inventory-source/title-update-display/manifest.json) pins tools, doc, input images, report, audit, listings and fresh-process reproduction. This closes numerical `build_number` label production for the pinned u5 image. Full authored title-screen delivery/rendering, live initialization, update-wide gameplay equivalence and retained1.1 native instruction-address identity remain separate gates.
