# TU1.5 retained helper comparisons

`death_cycle_helpers_tu15.json` comes from unchanged English OG Xbox TU1.5
instructions through `tools/oracle_death_cycle_helpers_tu15.py`. Executable SHA:
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

The 480 cases retain the previous 224 helper/physical-block cases and add 32
cases for each reached owner below. Native validation compares ordered stores,
all retained fixture bytes, GP/SSE and arithmetic flags masked with `0x8C5`.
Declared input records are component stimuli, not recovered gameplay outputs.

| Original owner | Native body | Independent fixture boundary |
| --- | --- | --- |
| 178D80 | Complete 26-instruction effect-request initializer | RET, dirty 96-byte packet and guards |
| D2AD0 | Complete 15-instruction current-object identity copy | RET, actual header/body layout and full datum |
| 179700 | Complete 61-instruction attached-effect request | Stops before BAD60; no matrix/effect result supplied |
| DCFA0 | Complete 28-instruction damage-material mask loop | RET, negative/zero counts and 16/32-bit shift boundaries |
| 1706F0 | Complete 27-instruction signed-byte state lookup | RET, first-match order and signed byte/index cases |
| 1054B0 | Complete 23-instruction indexed object-state selection/flag update | RET, signed count/index and original wrapping/low-AL behavior |
| 118760 | Complete 20-instruction light-receiver instance callback | Nonzero low-word mode runs to RET without a pool; mode0 stops before47860 |
| 282780 | Complete36-instruction world-removal callback reached at311522 | RET; actual property search, first-match/absent/NONE selection, low-word datum and bit12 update |

The original instructions and source constants remain unchanged. The bounded
cases stop before an external owner; no service return or replacement instruction
is supplied. Native live execution retains the actual external owners afterward.
DBB90/D9A30 physical blocks keep their prior explicit stops. These comparisons
do not establish complete damage/physics, a visible live effect or matched pixels.
