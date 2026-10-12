# Original weapon tick and cold header boundary

`tools/oracle_source_weapon_tick.py` reconstructs all 31 original no-graph,
non-spinning levitated weapons from the existing constructor proof. It freezes
682 CPU-only cases and 7,502 output words in
`tests/fixtures/retail_source_weapon_tick.tsv`. Every executed original
instruction is checked against the pinned disc executable. No production or
CMake changes are part of this audit.

The type-2 definition at `467CD0` inherits `4678E8` (object), `467C08`
(item), and itself. Its `+40` callbacks are zero, `10B4D0`, and `FDFB0`.
`108C60` calls both nonzero callbacks and ORs their results. The original
game tick `137FE0` reaches this dispatcher through the object passes
`B6F10` / `B7150` and `BC470`. The admitted item-only callback therefore
does not cover the complete weapon update.

All 31 cold headers have flags `0A`, type 2, and cluster NONE in the
explicit disabled-registry constructor scope. These flags come from original
stores, not captured later state:

| Store | Produced state |
| --- | --- |
| `B7C0E` | Header `+2` becomes `08`: defer the newly created object. |
| `B7C1B` | Header `+3` becomes type 2. |
| `B7398`, called from `1058B0` with return address `1058E0` | Header `+2` becomes `0A`: wake the weapon during its constructor. |

The later constructor call to `B7360` at `B82B6` sees it already awake and
makes no further flag store. Active bit `01` is absent in this scope.
The real source registry/BSP producer is still outside this proof.

The scheduling conditions are distinct:

| Routine | Admission |
| --- | --- |
| `B6F10` | Header active `01`, awake `02`, and no deferred `08`; unlisted objects also need object `C0` bit 3 clear. |
| `B7150` | Header deferred `08`, active `01`, awake `02`, and no deletion `10`; it clears `08` at `B7238` before calling `BC470`. |
| `BC470` | Header awake `02`; header `20` returns true without callbacks. Header `08` itself is accepted here. |
| `B7290` | An inactive object with header cluster other than NONE and object parent NONE becomes active at `B72C4`. |

Object `C1` bit 0 disables physics. It is not a gate on these weapon
callbacks. Explicit `C1=0/1` and header controls run complete `BC470`,
including its damage and change-colour callbacks, with unchanged routines.

In the actual cold disabled-registry scheduler cases, `B6F10` and `B7150`
leave the complete weapon body unchanged and call no weapon update.
Calling original `B7290` with the actual NONE cluster also leaves `0A`
unchanged. In the separate declared-cluster-0 controls, original `B7290`
produces header `0B`. `B6F10` first skips its deferred update; `B7150`
then produces `03` and runs it. The next `B6F10` finds no continuing
activity, clears awake at `BC5C6`, and leaves header `01`; subsequent
passes skip it. These controls prove the conditional scheduling rule,
not a recovered map-registration result.

The first complete weapon callback performs `FE175`'s signed 16-bit
decrement of body `19E`: cold zero becomes -1. Its signed comparison to
zero sets byte `16C` bit 5 at `FE186` and returns true. The next callback
keeps that bit and countdown, returns false, and can put the object to
sleep. The source stores produce `20`; the oracle never supplies it.
All callbacks retain `C1=1`, item `12C=0`, and power `184=0`. Original
`FF5F0` returns turned_on=1 throughout this scope because item `12C`
bit 0 is clear. This is independent of whether bit 5 has been produced.

The 19 BR/carbine bodies also set trigger-0 flags at body `210` from
0 to 1 through `FEB93` (trigger behavior 1 with no firing input).
The 12 sword bodies have no such change. This trigger state is a
separate output; it must not be silently treated as shader state.
Stores of already-zero body `188` and barrel flags are recorded too.

`FDFB0` does not store body `184`. Its use at `FE3CE` participates in
cooling the distinct `180` value. The literal weapon resource producers
found in the weapon range are:

| Routine / stores | Inputs and effect |
| --- | --- |
| `101740` / `10188E`, `101894` | Same-definition weapon transfer (caller `E8DE0`) can swap `184` between two weapons. |
| `101A10` / `101A72` | Explicit fraction setter (caller `1E2A90`); when `101D20` is true, stores `1-clamp(fraction,0,1)`. |
| `101DB0` / `101E44`, `101E52` | Resource consumption from callers `101C80` / `102FB0`; requires item `12C` bit 3, resource below 1, and no owning-player disable. Adds caller XMM0, caps at 1, then can play definition `2FC` and replace the definition from `304`. |

The admitted cold item flags and zero firing inputs do not enter these
resource-consuming paths. This audit does not establish held-weapon,
damage, player input, graph, effect, activation, or whole-gameplay parity.
Inherited allocator storage and the explicit `BF380` no-pressure boundary
remain declared services; update and shader services are never replaced.
