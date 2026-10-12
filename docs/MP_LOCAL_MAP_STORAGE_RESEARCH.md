# Original clock and callback18 storage before map reset

Original storage callbacks17 `147CC0` and18 `1E2E50` now return and own their complete records. The same full `190A30` map-start root then completes ten actual map-reset callbacks, in their original table order, and stops at callback23's first new platform clock read. No clock observation, readiness, local membership, controller/view binding or successful world result is added. Every earlier freeze remains unchanged.
[Independent tool](../tools/oracle_mp_spawn_local_map_storage.py),
[complete report](../analysis/multiplayer/integration/spawn-source/local-map-storage/report.json),
[preceding applied-protection boundary](MP_LOCAL_MAP_PROTECTION_RESEARCH.md).

## Source-owned storage and complete records

The two constructors use the existing source-created game arena, update its cursor, call original checksum `1656A0`, clear their whole records and bind the actual owner slots. Their addresses and sizes are outputs of those unchanged instructions. No record pointer or field is supplied.
[Clock constructor](../analysis/multiplayer/integration/spawn-source/local-map-storage/instructions/00147cc0.asm),
[callback18 constructor](../analysis/multiplayer/integration/spawn-source/local-map-storage/instructions/001e2e50.asm),
[complete allocation, cursor, checksum and store ledger](../analysis/multiplayer/integration/spawn-source/local-map-storage/report.json).

| Owner | Actual slot | Produced address | Bytes | Cursor before → after | Checksum after, raw four bytes |
|---|---|---|---|---|---|
| Clock17 | `518444` | `82EBAA00` | `24` hexadecimal (36 decimal) | `173A00→173A24` | `d65f90db` |
| Callback18 | `4FCDC4` | `82EBAA24` | `E4` hexadecimal (228 decimal) | `173A24→173B08` | `e1d8825d` |

Both entire constructor outputs are zero. Original reset17 `147D20` then clears all nine clock dwords, reads the signed short at `Game+0E`, copies it to clock+02 and computes clock+04 with `1.0f / float(signed_short)`. It writes clock+0C=`1.0f` and clock+00 byte=`1`. This fixture's existing options/Game record has `Game+0E=0`; the resulting clock+04 is positive infinity, raw `0000807f`. No30Hz or other rate is inferred or assigned. Its actual nonzero producer is a separate research boundary.
[Exact reset and float order](../analysis/multiplayer/integration/spawn-source/local-map-storage/instructions/00147d20.asm),
[literal complete before/after clock records](../analysis/multiplayer/integration/spawn-source/local-map-storage/report.json),
[independent ordinary-tick ownership boundary](MP_ITEM_TICK_OWNER_RESEARCH.md).

Original reset18 clears the source-created `E4` record, fills its final mask words and returns. The full constructor/reset records, rather than selected reconstructed fields, are retained for comparison.
[Reset18](../analysis/multiplayer/integration/spawn-source/local-map-storage/instructions/001e2ea0.asm),
[complete record, calls and actual stores](../analysis/multiplayer/integration/spawn-source/local-map-storage/report.json).

## Actual owning order and exact remaining stop

The bounded prerequisite composition follows the same existing preparation and actual control/view/camera/configuration/simulation/RNG/pool13 roots, then executes storage17 and18 before the full `190A30` root. Those storage callbacks come from the unchanged `139390` table ownership. This selected composition is not a claim that all68 title-storage callbacks or the whole authored title/session startup have run.
[Owning root and table traversal](../analysis/multiplayer/integration/spawn-source/local-map-storage/instructions/00190a30.asm),
[original map reset](../analysis/multiplayer/integration/spawn-source/local-map-storage/instructions/00139410.asm),
[exact bounded stages and retained preparation](../analysis/multiplayer/integration/spawn-source/local-map-storage/report.json).

Actual reset order is `2→11E330`, `9→187550`, `13→19F720`, `16→125370`, `17→147D20`, `18→1E2EA0`, `19→14CCD0`, `20→187520`, `21→1EA410`, `22→158C10`, `23→680F0`. The first ten return. Each callback's full before/after owner state, registers, stack and observed return is captured; callback23 remains interrupted.
[All eleven callbacks and ten return observations](../analysis/multiplayer/integration/spawn-source/local-map-storage/report.json),
[observed original store sites](../analysis/multiplayer/integration/spawn-source/local-map-storage/instructions/observed-map-storage-source-store-sites.asm).

The first new clock request stops at original `337A45`, before `mov eax,[eax]`, with return address `68E87`. The untouched XBE's import417CD4 contains ordinal156 `8000009C`; the inherited earlier input fixture redirects that import to `0B000000`. Its backing is explicitly retained but is not read for this new observation. EAX=`0B000000`, ECX=`0101A000`, ESI=`0054C670`, EBP=`0054E4C8`; the full literal stack and all other registers are in the report. No new timestamp or success result is supplied.
[Original clock wrapper](../analysis/multiplayer/integration/spawn-source/local-map-storage/instructions/00337a40.asm),
[interrupted callback23](../analysis/multiplayer/integration/spawn-source/local-map-storage/instructions/000680f0.asm),
[exact stop ABI and complete live records](../analysis/multiplayer/integration/spawn-source/local-map-storage/report.json).

Earlier platform clock observations replay the prior measured fixture only in its originally recorded preparation phases. During this new full map-reset, only earlier Python clock observers at336983/337A40 are bypassed, allowing unchanged original XAPI instructions to execute. The first new kernel observation is stopped before its data read. Applied memory protection and the complete declared compatibility thread/process/raw-storage providers retain their preceding scope. Actual compatibility initializer/lock instructions and stores are recorded separately from verified original-XBE instructions.
[Observer exclusions and exact platform ledger](../tools/oracle_mp_spawn_local_map_storage.py),
[complete platform and clock boundary records](../analysis/multiplayer/integration/spawn-source/local-map-storage/report.json).

The full map start remains inside `139410`, before `1395B0` membership, `11D780` BSP startup and `190A6E` readiness. Readiness is still zero and local users remain NONE. No ordinary local command scheduling, live countdown or local participant count is established. The separate lobby note retains the original count loop and10/3 literals without promoting them to a completed timer lifecycle.
[Static lobby and count ownership](MP_LOBBY_SOURCE_RESEARCH.md),
[complete retained local, pool, options and owner state](../analysis/multiplayer/integration/spawn-source/local-map-storage/report.json).

## Reproduction and freeze

One bounded case verifies12,985 executed original instruction addresses and78 instruction files:76 routines, the complete executed listing and1,757 observed original store sites. Independent execution reproduces the report and all listings byte-for-byte and preserves265 previous source/tool/report/listing/platform pins. Native context and allocation addresses remain in separate raw files. No original instruction or production file changes.

```bash
.tools/python/bin/python tools/oracle_mp_spawn_local_map_storage.py \
  --output analysis/multiplayer/integration/spawn-source/local-map-storage/reproduction/report.json
```

[Verified original executed bytes](../analysis/multiplayer/integration/spawn-source/local-map-storage/instructions/executed-original-instructions.asm),
[independent verification and final artifact pins](../analysis/multiplayer/integration/spawn-source/local-map-storage/independent-freeze-verification.json).
