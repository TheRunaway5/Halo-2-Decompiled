# Original Lockout kill-volume death admission

The actual original kill-volume branch admits a genuine allocated moving Spartan, creates the authored `globals\distance` damage request, sets its dead bit and selects the authored `fall_death` dialogue sound. It preserves object `+88` momentum through the completed `D7E00` call. Execution then stops in the missing sound runtime before `CBDC0` corpse cleanup. It does **not** establish `landing_dead` animation selection, a naturally falling multi-frame simulation, natural ragdoll admission, continuous solver motion or corpse rendering.

This is a new CPU-only experiment. The earlier moving-death, ten-body constructor, current-body-matrix and all other freezes remain immutable. No Simulation, Match, renderer, shared header, CMake or release file changes belong to this scope.

## Authority and independent reproduction

- Original TU1.1 `.03`: SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
- Actual Lockout cache: SHA-256 `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
- Authenticated owned u5English executable: SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
- [New original execution observer](../tools/oracle_mp_spartan_kill_trigger_1_1.py).
- [Independent raw-state/read/store auditor](../tools/audit_mp_spartan_kill_trigger_1_1.py).
- [Complete instruction correspondence tool](../tools/research_mp_kill_trigger_update_functions.py).
- Ignored evidence: `analysis/multiplayer/integration/spartan-kill-trigger-20261010/`.

The new `probe/report.json` and separate `repeat/report.json` both have SHA-256 `e43e73718e3a327842ad1b3e30dfb3101bafd59492e71997fc1a76861b0dcb5d`. All **307** report/instruction files repeat exactly. The observer executes **16,617** distinct instruction addresses, records **1,335** calls, **33,142** reads and **17,222** raw stores. The independent audit verifies every one of **45,634** listed instruction rows against original bytes and replays **91** complete captured backing intervals, including 3,109 overlapping reads, 1,786 stores and 1,198 changed bytes. New guest owning fields, successful returns and instruction patches are not supplied. [Report](../analysis/multiplayer/integration/spartan-kill-trigger-20261010/probe/report.json), [audit](../analysis/multiplayer/integration/spartan-kill-trigger-20261010/audit.json).

Before the new roots, original command submission/control/locomotion/pose/world scheduling reproduces the entire frozen `authored_heading_forward` case's before/after states. Its explicit command axes `1/0`, source delivery of the actual chosen start37 heading, selected startup/platform/resource context and inherited tick0/`dt=3D088889` remain declared inputs. The diagnostic does not supply live controller admission or increment game time. Original selected Spartan phmo postprocessing and complete `1775D0→175860` effect-pool construction/`177690` reset execute next. [Prior moving-state contract](MP_SPARTAN_MOVING_DEATH_RESEARCH.md), [command contract](MP_COMMAND_LOCOMOTION_POSE_RESEARCH.md).

## Actual authored volume and diagnostic placement

Lockout's scenario `E1780003` lives at `80152498`. Scenario `+230/+234` is a count/pointer block of signed 16-bit kill-volume indices, stride2. The unchanged cache contains one index, **0**. Its trigger volume is scenario `+108/+10C`, stride`44`, row address **80153C80**. The row retains object-name index `FFFF` (unparented), node word at`+08`, forward`+0C`, up`+18`, position`+24` and extents`+30`.

Actual authored position is `(-50.62419509887695, -35.332130432128906, -103.72113800048828)` and extents are `(82.43272399902344, 101.97207641601562, 75.63876342773438)`. Forward/up are the actual axis basis. The sole new external diagnostic point is explicitly derived as `origin + .5*(forward*extent.x + cross(up,forward)*extent.y + up*extent.z)`, rounded to three caller-owned float words **C116867C 417A7668 C283CDB3**. This is declared test placement inside authored geometry, not a source-produced falling trajectory or an expected result reconstructed from native code. The report retains the full44-byte source row, caller-scratch previous/input bytes and formula.

Original `11DA40` accepts that caller point without changing the player. Complete original `B8490(unit,0,0)` then consumes the same point with EAX=point, ECX=EDX=0. All transform, physical/bounds/location/registry work executes. This stores the requested placement at object`+64` and produces its current **bounds center** at object`+30`. The latter differs: **C11679F2 417A70AF C2831561**. No object root, center, velocity, pose, bounds or physical state is written by the observer. The actual object and physical body remain allocated. [B8490 listing](../analysis/multiplayer/integration/spartan-kill-trigger-20261010/probe/instructions/000b8490.asm), [captured full unit/physical backing](../analysis/multiplayer/integration/spartan-kill-trigger-20261010/probe/report.json).

**Field naming:** `unit_literal.root_hex` and the probe stage names containing “root” retain the observer's initial label for literal bytes `unit+30`. That is the **bounds center**, not placement root`+64`. Both locations are retained in `unit_literal.raw800_hex` and complete world object records. Original `11DB9A..11DBA5` resolves the object then adds`30` before calling the point predicate. Future integration must use the produced bounds center; using native `Body.position` without its bounds-center producer is not established by this evidence.

`11D950` constructs the unparented volume's scale/basis/translation through original `143860`. Parent-attached alternatives resolve actual object-name/marker/current-node storage and remain unexecuted here. `11DA40` subtracts translation, divides by nonunit scale if needed, and uses the exact scalar-SSE basis products/order before **strict** tests `0<x<extent.x`, `0<y<extent.y`, `0<z<extent.z`. The original initial moving bounds-center test returns AL0; the declared point and both subsequently retained center tests return AL1. No collision/contact return is supplied. [Point predicate](../analysis/multiplayer/integration/spartan-kill-trigger-20261010/correspondence/instructions/0011da40-disc-0011da40.asm), [transform](../analysis/multiplayer/integration/spartan-kill-trigger-20261010/correspondence/instructions/0011d950-disc-0011d950.asm), [datum wrapper](../analysis/multiplayer/integration/spartan-kill-trigger-20261010/probe/instructions/0011db80.asm).

## Owning admission and fatal request

`DEFC0` is the complete biped motion owner in untouched original bytes; it was missing from the old Ghidra function inventory. Its entered path resolves the actual component, calls `BBE70`, `DEE00` and `E46F0`, then at`DF037` skips kill-volume admission only if the resolved object`+C1` bit0 is set. Actual source state admits the call `DF042→1EFEF0`.

`1EFEF0` reads actual tick`clock+8` and low16 of the full unit datum, sums with wrapping32-bit arithmetic and uses signed IDIV by the scenario kill-count. That remainder selects **one** row per call. It tests the corresponding disable bit in original global`526258`, created/reset by the inherited real `1EFE10/1EFE70` owners. A disabled bit or signed row`-1` returns AL0. Otherwise `11DB80(unit,index)` checks actual object`+30` through `11DA40`. This is not an unconditional scan of all volumes. The measured single-row bank has zero disable bits; other indices/disabled/parented cases are source-listed only.

At positive admission `DF04B` sets unit`+349` bit7. Options byte at`Game+0C` equal4 skips damage; that alternative is listed, not tested by a changed mode. The actual mode continues:

1. Resolve original globals pointer`4E791C`, block pointer`+144`, then damage datum at block`+28`: actual **EAE90964**, `jpt! globals\distance`.
2. `D7C60` initializes the complete88-byte damage request. Caller ORs request flags`+04` with4, sets request`+84=1` and preserves the constructor's remaining fields.
3. `D7CD0` writes the genuine current owner at request`+08`: player **EC700000**, unit **E26F0000**, team0. The experiment supplies no owner, damage amount or depleted health.
4. `D9180(request, E26F0000, -1, -1, -1, 0)` executes its actual model/damage path. It zeros source body/shield vitality as that path requires, calls complete `D7E00(unit, request+08,1,0)` and produces the normalized fatal result.
5. `D7E00` sets unit`+10A` bit2. It returns while the real player still owns the same unit. The object`+88` words remain **3C33EFD9 3EA3BE55 BE6F9DB2**, the same nonzero source-produced movement captured before placement/death.
6. `D9180→DAC40→CB410` receives that fatal result. The request/result/owner buffers, exact stack words, complete raw unit/player/world/body before/after states and all ordered stores are retained.

[Complete biped owner](../analysis/multiplayer/integration/spartan-kill-trigger-20261010/correspondence/instructions/000defc0-disc-000defc0.asm), [request initializer](../analysis/multiplayer/integration/spartan-kill-trigger-20261010/probe/instructions/000d7c60.asm), [fatal dispatcher](../analysis/multiplayer/integration/spartan-kill-trigger-20261010/probe/instructions/000d9180.asm), [complete dead-bit owner](../analysis/multiplayer/integration/spartan-kill-trigger-20261010/probe/instructions/000d7e00.asm).

## Genuine first unavailable dependency and animation limit

The source chooses dialogue sound **E96C07E7**, actual `snd! sound\dialog\combat\masterchief\fall_death`, through `CB410→115FA0→115CE0→115C60→20EF90`. The actual `20EF90` input is the original sound datum. At **20EFC2**, source attempts to read global`526464`'s`+24`; the pointer is NULL, so the exact invalid read is **00000024**. No successful audio availability, ready bitmap or sound allocation is supplied. This is a missing sound-runtime owner, not evidence that the authored sound is unavailable in a complete running title.

The earlier inherited firing-feedback service `D2310` is explicitly supplied unavailable and recorded during this branch. All other inherited services remain in the report; no successful sound/animation/physics return is added. The source stop happens **before** final `CB410→CBDC0`. Dead bit and depleted vitality have happened; player detach, death tick, equipment drop and corpse cleanup have not happened in this experiment.

`C5190`/`EFF50` are never entered. The complete static `CB410` fatal tail at`CBA26` pushes **0** as `CBDC0`'s fourth argument. `CBDC0→E6050` forwards that argument. Original `E6050`'s `landing_dead` selection at its graph request is guarded by a **nonzero** argument. Thus this ordinary kill-volume chain does not request that forced clip through this tail. Separately, `C5190` packet`+4` bit0 calls `CBDC0(...,1,0)`; its natural packet admission remains a different unresolved owner. A graph containing `landing_dead` is not proof that falling out of Lockout selects it. [Fatal cleanup call site](../analysis/multiplayer/integration/spartan-kill-trigger-20261010/correspondence/instructions/000cb410-disc-000cb410.asm), [E6050 guard](../analysis/multiplayer/integration/spartan-kill-trigger-20261010/correspondence/instructions/000e6050-disc-000e6050.asm), [C5190](../analysis/multiplayer/integration/spartan-kill-trigger-20261010/correspondence/instructions/000c5190-disc-000c5190.asm).

The separately frozen ordinary `CBDC0` direct-root experiment already proves momentum retention through complete controller rebuild on the genuine moving unit. It does not complete this new kill-volume caller across its sound stop. The separately frozen ten-body direct constructor and body-matrix leaf do not supply natural `+34B`/contact admission, live world insertion, initial corpse body momentum delivery or a solver. No standing idle corpse, death clip, fall timer or ragdoll default is justified here. [Moving death](MP_SPARTAN_MOVING_DEATH_RESEARCH.md), [ten-body construction](MP_SPARTAN_RAGDOLL_CONSTRUCTOR_RESEARCH.md), [body-matrix leaf](MP_SPARTAN_BODY_MATRIX_RESEARCH.md).

## Authenticated1.5 correspondence and native seam

All18 investigated complete routines have equal normalized instruction shape/internal branch topology; actual image-memory operand bytes and direct call targets are retained independently. Pointer contents that relocate are not asserted byte-identical. The independent static export repeats in `correspondence-repeat/`. This is correspondence evidence, not a whole1.5 dynamic execution claim.

| TU1.1 | Authenticated1.5 | Owner |
| --- | --- | --- |
| `DEFC0` | `DFEE0` | Biped motion/kill request |
| `1EFEF0` | `1F47E0` | Rotating enabled kill-volume row |
| `11DB80` | `11ECE0` | Full datum to current bounds center |
| `11DA40` | `11EBA0` | Strict transformed point predicate |
| `11D950` | `11EAB0` | Actual volume transform |
| `D7C60` | `D8BB0` | Damage request constructor |
| `D7CD0` | `D8C20` | Current owner |
| `D9180` | `DA0D0` | Ordinary damage dispatch |
| `D7E00` | `D8D50` | Dead bit/physical damage owner |
| `DAC40` | `DBB90` | Normalized result delivery |
| `CB410` | `CC590` | Unit feedback/death tail |
| `CBDC0` | `CCF40` | Ordinary cleanup/corpse ownership |
| `C5190` | `C6310` | Separate forced-death packet |
| `EFF50` | `F0E70` | Separate packet consumer |
| `B8490` | `B95D0` | Diagnostic actual position delivery |
| `B8320` | `B9460` | Position/transform owner |
| `E6050` | `E6F70` | Death state and guarded forced clip |
| `E6DC0` | `E7CE0` | State3 producer |

[Full paired bodies/operands/calls](../analysis/multiplayer/integration/spartan-kill-trigger-20261010/correspondence/report.json).

The next live seam is source-current **bounds center**, retained full unit identity, current tick, kill-index block and actual disabled bank→one enabled volume→the authored distance-damage request→ordinary fatal ownership. Native `SceneObjects::contains` already imports unparented authored basis/position/extents, but the current Match loop tests native body position against every volume after player stepping and directly marks vitality/death/clears velocity. This freeze provides the original differences; it makes no runtime change and does not promote the partial caller to complete fall/corpse proof. Exact sound runtime initialization, the natural complete caller, retained action/animation/physical state and contact-driven ragdoll admission remain required before live fall animation/physics can be claimed.

Reproduction:

```sh
.tools/python/bin/python tools/oracle_mp_spartan_kill_trigger_1_1.py \
  --output analysis/multiplayer/integration/spartan-kill-trigger-20261010/repro/report.json
.tools/python/bin/python tools/audit_mp_spartan_kill_trigger_1_1.py \
  --directory analysis/multiplayer/integration/spartan-kill-trigger-20261010/probe \
  --repeat analysis/multiplayer/integration/spartan-kill-trigger-20261010/repeat \
  --output analysis/multiplayer/integration/spartan-kill-trigger-20261010/audit-repro.json
```
