# Original u5 classifier correspondence and bounded execution

The authenticated English u5 classifier's admitted leaf/state1/flag20 paths match all **1,176** frozen explicit-input cases: every result packet byte, all **1,536** stores, **21,600** instruction CPU snapshots and all1,176 return CPU snapshots. Normal and ASan/UBSan native comparisons also pass. This is measured packet-consumer correspondence, not source-created u5 controls, unit physics, movement, pose, clock or frame production.

The earlier [1.1 classifier](MP_UNIT_LOCOMOTION_RESEARCH.md) and every earlier freeze remain immutable. No native source, CMake, live runtime or shader/render code changes are made by this extension. The native API's instruction identities remain1.1; actual u5 sites are explicitly mapped and recorded separately.

## Original mapping and authority

The authenticated source is the imported English u5 XBE outside the repo, SHA256 **`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`**. Its original numerical `build_number` producer independently emits **`1.5`**; that proof does not use the annotation export's names/comments, certificate version or directory name. [Original display producer](MP_TITLE_UPDATE_DISPLAY_RESEARCH.md).

The [new oracle](../tools/oracle_mp_unit_locomotion_u5.py) locates the leaf using the **entire130-byte original1.1 instruction body**, with exactly one match in u5. It independently locates the wrapper's exact first22 bytes, also unique, then verifies every instruction/conditional branch, the actual tail jump to the mapped leaf and all six original table targets. No1.1 address is assumed to address u5 code; no annotation or1.1 function inventory is used to seed the new routine mapping.

| Source role | Original1.1 | Actual u5 | Evidence |
|---|---|---|---|
| Classifier leaf | `1F52F0` | `1F9BF0` | All130 contiguous original bytes equal |
| Wrapper | `1E94E0` | `1EDE00` | Complete instruction shape/internal branches equal; external targets/table separately verified |
| Six-entry dispatch table | `1E9520` | `1EDE40` | Every target matches explicit instruction mapping |
| Store class2 | `1F5349` | `1F9C49` | Executed original store |
| Store class3 | `1F5351` | `1F9C51` | Executed original store |
| Store class5 | `1F5362` | `1F9C62` | Executed original store |
| Store class4 | `1F536A` | `1F9C6A` | Executed original store |
| Flag20 store class1 | `1E94EB` | `1EDE0B` | Executed before blend store |
| Flag20 store blend1 | `1E94EE` | `1EDE0E` | Executed after class store |

The actual u5 table targets are `1EDE1D,1EDE26,1EDE11,1EDE34,1EDE34,1EDE2D`. State1 executes `1EDE1D→1F9BF0`; the flag20 branch returns before the physical byte. States2/4/5/6 retain actual source lower targets `203170/2048E0/1F46D0`, and state3 is the source-listed RET at`1EDE11`. Those other routes are unexecuted and remain unavailable in the native bounded API. [Leaf listing](../analysis/multiplayer/integration/inventory-source/unit-locomotion-u5/instructions/001f9bf0.asm), [wrapper listing](../analysis/multiplayer/integration/inventory-source/unit-locomotion-u5/instructions/001ede00.asm).

## Declared input and execution boundary

All request154/result70/physical16 packets, vector patterns, retained class/blend seeds and complete root-entry CPU states come from the frozen1.1 original report. They are **explicit input configuration**, not a preexisting u5 player, source-created u5 request or supplied original return. The new oracle loads unchanged u5 sections and actual u5 table bytes, restores each declared packet/CPU input into explicit backing, and executes original u5 code without service stubs or patches. The only guest stores are the original output fields; no unit pose, physical-state producer, contact, controller, PVS, world, clock or admission state is fabricated.

The original zero/axis/tie/subnormal/extreme/NaN results all agree. The bit-pattern semantics and preservation rules are identical to the earlier bounded classifier: exact signed-zero no-write; Y selected only when ordered absoluteY>absoluteX; ties/unordered magnitude take X; original sign/unordered rules; Z only admits the axis comparison; flag20 owns class1 then blend1. The report retains actual u5 before/after buffers, every read/store and full per-instruction CPU state. [Measured report](../analysis/multiplayer/integration/inventory-source/unit-locomotion-u5/report.json).

[Independent auditor](../tools/oracle_mp_unit_locomotion_u5_audit.py) verifies57 original listing rows, all six relocated table targets,3,528 full packet intervals,6,104 memory reads,1,536 stores and all CPU correlations. It replays original outputs from actual source reads/stores; no native-derived expected output is used. A fresh-process repeat reproduces the report, audit and both listings byte-identically.

## Native comparison and address identity

[Separate differential](../tools/differential_unit_locomotion_u5.py) exports only **actual u5** initial/expected packets and store data/order. It adapts the six store-site identifiers through the independently proven u5→1.1 bijection because the immutable native API/test still returns1.1 instruction IDs. The adaptation applies only to trace labels; the original u5 result bytes, offset/width/value/store order are preserved. [Explicit adapter](../analysis/multiplayer/integration/inventory-source/unit-locomotion-u5/native-site-adapter.json).

Both normal and sanitizer builds report: `1176 original classifier/wrapper packets and 1536 ordered stores matched; 32 no-write paths preserved`. [Normal](../analysis/multiplayer/integration/inventory-source/unit-locomotion-u5/native-normal.json), [sanitizer](../analysis/multiplayer/integration/inventory-source/unit-locomotion-u5/native-sanitizer.json). This supports the native classifier's stored-output semantics for the measured u5 inputs. It does not make its source-address metadata native u5 data. Any future production selection must preserve executable identity and explicitly choose/map u5 sites and unsupported dispatch targets; the old1.1 addresses must not be silently represented as1.5 addresses.

```sh
.tools/python/bin/python tools/oracle_mp_unit_locomotion_u5.py \
  --output analysis/multiplayer/integration/inventory-source/unit-locomotion-u5/another-repeat/report.json
.tools/python/bin/python tools/oracle_mp_unit_locomotion_u5_audit.py \
  --output-dir analysis/multiplayer/integration/inventory-source/unit-locomotion-u5/another-repeat
.tools/python/bin/python tools/differential_unit_locomotion_u5.py
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
  .tools/python/bin/python tools/differential_unit_locomotion_u5.py \
  --binary analysis/multiplayer/integration/unit-locomotion-native/unit_locomotion_tests_san \
  --output analysis/multiplayer/integration/inventory-source/unit-locomotion-u5/native-sanitizer.json
```

Original report SHA256 **`4adb6786e9554be1395594a77a216680a80124bb782d24139ee1a74e4ded2d87`**. [Freeze manifest](../analysis/multiplayer/integration/inventory-source/unit-locomotion-u5/manifest.json) pins all new source/report/audit/listing/fixture/adapter files, old inputs and both original executables, with independent repeat hashes and separate native comparison hashes. This closes only the bounded classifier correspondence. Actual u5 controller/request/physical ownership, channel/pose/current-matrix producers, other physical branches, live motion/clock/admitted frames and Xbox floating-point hardware parity remain separate gates.
