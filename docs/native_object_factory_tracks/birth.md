# Track `birth`: native B9B70 `object_new`

Native: `src/gameplay/objects/native_object_factory_birth_tu15.cpp`
(`object_new`).

Owners from other tracks are injected through `ObjectFactoryServices`. An
empty service is a named refusal. The original's stack locals live in a
private overlay:

- the failure byte passed to the type constructors;
- the 1D3180/1D3380/1D3350 probe record;
- the C19D0 outputs.

## Proof so far

`object_new_early_failure` covers births that end before the type
constructors:

- the datum/body is refused;
- the 0x114 node block or 0x118 region block cannot fit;
- 1CC4D0 refuses a physics type without capacity.

Result: 600 physical cases with 84,945 identical ordered stores. 353 of the
614 B9B70 instructions are covered, plus 10DAF0 except its biped validate
callback, and 1496F0's pressure gate.

Not yet covered:

| Range | Why |
|---|---|
| B9E1A–B9E49 | Dead: the zero-filled body never has body +4 bit 0 set at that point. |
| B9F53–B9FC4, BA07B–BA0A0 | The animation probe and the Havok blocks need native 1D3380 (`modelstate`). |
| BA0E4–BA3B3 | The success path needs every track's owners. |

## Never executed in the six map censuses

These are named refusals: 179610 (creation effect), the BA570 deferred
delete, and the C0F60/BABA0 branch (dead, see above).
