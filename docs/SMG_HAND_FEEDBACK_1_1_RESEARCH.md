# Original 1.1 SMG hand feedback and registration boundary

This independent evidence explains the timer difference between the frozen bare
SMG firing suite and the frozen fully allocated, attached, equipped suite. It
does not change either suite or implement a first-person presentation producer.

## Verified source predicate

The positive-ammunition firing path calls `10A140` at `107719` (bytes
`e8222a0000`, return `10771E`). `10A140` calls `167D32` with the weapon datum in
EBX. That function searches the original first-person weapon slots:

```
globals = [004F1198]
slot = globals + 0x10 + view * 0x20CC + hand * 0x1010
views = 0..3; hands = 0..1
```

The returned view is `FFFFFFFF` when the datum is absent. This lookup is the
relevant admission difference; weapon-own animation-manager presence and the
player's local-view field `+28` are not the lookup predicate.

`10A140` supplies the result in EDI to original `EFCC0`. It also resolves the
view's player from `[004F01F0] + 0xC + view*4`, and that player's current unit
from `[004F01F4]+0x44` plus `(player & 0xFFFF)*0x21C + 0x2C`. ESI is hand zero
when the weapon matches the unit's current primary weapon, otherwise hand one.
For action events 1 through 4, `EFCC0` writes a hand timer only if all three
values exist: view, player datum, and player's current unit datum. The unit
timer write is at `EFDAB` for `+1FA`, or `EFDEC` for `+1FB`.

The original timer operations read the signed simulation frequency at
`[00518444]+2`, convert it to binary32, multiply by original constant
`00452000` (`3E99999A`, binary32 0.30000001192092896), store binary32, then use
FLD/FISTP with the fixture's original x87 control word `027F`. At frequency 30
the written byte is 9. The following unit update decrements it to 8 at `C6CE4`.
The ordinary observed action is EAX=1, ESI=0. The notification to `16A356` still
occurs when timer admission fails.

## Direct original controls

`tools/oracle_smg_hand_feedback_1_1.py` imports both frozen source suites and
executes two original firing ticks per case. It records exact original entry
registers, stack arguments, lookup memory, returns and hand-byte writes.

| Case | Explicit boundary variation | Result |
| --- | --- | --- |
| bare | Existing bare fixture, slot contains `00010000` | View 0; timer 9 then 8 |
| bare_registration_cleared | Only slot `+10` changed to `FFFFFFFF` | View absent; timer remains 0 |
| bare_empty | Loaded and reserve ammunition zero | No `10A140` / `EFCC0` fire callback |
| attached | Actual allocated weapon `00010002`; inherited slot still `00010000` | View absent; timer remains 0 |
| attached_registration_supplied | Only slot `+10` changed to actual `00010002` | View 0; timer 9 then 8 |
| attached_registration_empty | Supplied matching slot, ammunition zero | No fire callback; timer remains 0 |

The attached fixture already has a valid view-zero user-player mapping and
player current-unit `00010001`. Its missing weapon registration alone prevents
the hand timer. The supplied matching-slot control demonstrates the predicate;
it is **not evidence that the original registration producer has executed**.
The callback occurs before the projectile postpass and its supplied allocation
failure, so it must not be conditioned on successful projectile allocation.

Both baseline cases are checked against the existing frozen source output for
their first two weapon phases. Separately, all 599 frozen attached ticks were
audited: all 2,995 phase snapshots have zero hand timers, and all 169 relevant
first-person notifications carry view `FFFFFFFF`.

## Freeze and reproduction

Run from the repository with its Python runtime:

```
.tools/python/bin/python tools/oracle_smg_hand_feedback_1_1.py
```

Two independent executions produced byte-identical reports. The reproduction
copy is `analysis/multiplayer/map-pack/smg-hand-feedback/report-reexecution.json`.
Every instruction-listing row was independently compared to the pinned original
XBE bytes, including the firing helper `107160` that contains callsite `107719`.

| Artifact | SHA-256 |
| --- | --- |
| Original 1.1 XBE `.03` | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| Hand-feedback oracle | `0c0961a573619f2c47daad9f5762c4f77abddac53da8575492778ff49b30a98c` |
| Report and independent reexecution | `d2761d28968fe8c2ae980bd69669c2683a9a63486d9f84bd5b54e9fb921ea46c` |
| Frozen 599-tick context audit | `9f15ca637a65f79f477d8e2b7ee8bd9fef66499554f9597ec902f432ef68dba1` |
| Instruction inventory | `c97ff94fd16d899803fba7fbdd0e45ec63140f9dd847342a93c95d6b175d7a9b` |
| Original instruction/constant audit | `6da14b2b984eb271d24bc609731e92a4a5f6bd7816a3d308732bd5ff1916f09c` |

## Remaining producer boundary

The actual first-person global allocation, view lifecycle, and carried-weapon
registration upstream of `167D32` are being recovered independently. The current
evidence does not justify registering every equipped weapon by an assumed rule,
or treating a headless unit as a local first-person view. Native consumers must
keep this caller-owned binding boundary explicit until that producer is proven.
The existing original resource, world, first-person notification and projectile
allocation service boundaries remain those of the imported suites. No production
implementation or earlier frozen artifact is changed by this research.
