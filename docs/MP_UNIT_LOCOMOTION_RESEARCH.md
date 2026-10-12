# Bounded original unit locomotion classifier

The native `1F52F0` packet consumer and the measured normal/flag branch of `1E94E0` match **1,176 original executions**, all112 result bytes and **1,536 ordered stores**. Both an optimized GCC build and Clang AddressSanitizer/UndefinedBehaviorSanitizer build pass. Non-neutral request vectors are explicitly supplied fixtures on the unchanged source-created Spartan's actual physical state1. This does **not** establish live device admission, a naturally moving player, clip selection, current pose production or a complete frame.

Every earlier [owned-pose](MP_SPARTAN_OWNED_POSE_RESEARCH.md), [tick-owner](MP_ITEM_TICK_OWNER_RESEARCH.md), player/world, item, attachment and inventory freeze remains immutable. The new original oracle reproduces all observed owned-pose sections byte-identically before varying any input. No CMake or live multiplayer runtime file is edited.

## Authority, scope and reproduction

The authority is the exact unchanged TU1.1 executable `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`, with actual Lockout/shared cache data and the prior source-owned initialization/schedule. This identifies one pinned executable, not an unverified newer patch or Xbox hardware behavior.

| Frozen source artifact | SHA256 |
|---|---|
| [Original oracle](../tools/oracle_mp_unit_locomotion_1_1.py) | `ec102f6366669797c040c804978d1d35fa343afb811abfeee906a6e5cfb7a614` |
| [Independent auditor](../tools/oracle_mp_unit_locomotion_audit_1_1.py) | `6c1d3ef774314d06df770863c93a2ea9946facda910c20dee3d15d8e92b08580` |
| [Original report](../analysis/multiplayer/integration/inventory-source/unit-locomotion-measured/report.json) | `70b1fc431af6636358fadc942677f2351ed8382f893f58f15729ecfabfe8e6f0` |
| [Buffer/instruction audit](../analysis/multiplayer/integration/inventory-source/unit-locomotion-measured/audit.json) | `31f268fdabaf95e7262030b201544a7ab9bcb42b9e3e8c446ee5c45bdb381b0c` |
| [Original fixture export](../analysis/multiplayer/integration/unit-locomotion-fixtures/manifest.json) | `642cb4afa40b6725394d90642822f81943dc8c490ea22f4995b8fdef8344204c` |

The fresh-process `reproduction/report.json`, `reproduction/audit.json` and both complete unchanged instruction listings are byte-identical to the first run. The audit verifies206 prior pins,57 source instruction rows,6,104 literal reads,21,600 complete instruction/CPU snapshots and every ordered output store. The prior owned-pose runtime is correlated before the extension; its full object bodies, manager/current/local matrices, callbacks, stores, packets, resources and services must match the earlier report. [Reproduction verification](../analysis/multiplayer/integration/inventory-source/unit-locomotion-measured/independent-freeze-verification.json).

```sh
.tools/python/bin/python tools/oracle_mp_unit_locomotion_1_1.py \
  --output analysis/multiplayer/integration/inventory-source/unit-locomotion-measured/another-repeat/report.json
.tools/python/bin/python tools/oracle_mp_unit_locomotion_audit_1_1.py \
  --output-dir analysis/multiplayer/integration/inventory-source/unit-locomotion-measured/another-repeat
.tools/python/bin/python tools/export_unit_locomotion_fixtures.py
.tools/python/bin/python tools/differential_unit_locomotion.py
```

## Source input ownership and declared variations

The real source-created owner is player`EC700000`, unit`E26F0000`, body`82D87FC0` of`0x1676` bytes, actual authored start37, cluster4 and source PVS`FFDF0000` followed by60 zeros. Equipped SMG`E2700001`, unit physical component`E1680000`, full unit body/pose banks and original unsorted world membership remain unchanged. The physical-state record consumed by the wrapper is unit+3DC at`82D8839C`, raw16 bytes `0100000000006fe2ffffffffffffffff` (state byte1). [Actual owning initialization/schedule](MP_SPARTAN_OWNED_POSE_RESEARCH.md), [literal source-owner export](../analysis/multiplayer/integration/unit-locomotion-fixtures/source-owner.json).

The owning normal path is `DE960→DDBC0→1E9540→E3B20→1E8F60→1E94E0→1F52F0`. `DDBC0` at`DDF2E` passes the actual unit+1B0 vector. `1E9540` copies its words to request+DC/E0/E4 at`1E9638/1E963F/1E9645`. **These are control-request axes, not world velocity.** The source-created unit+1B0 remains zero in this fixture; no native/world velocity is used to choose a class. [Original owning request writer](../analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/instructions/001e9540.asm), [caller](../analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/instructions/000ddbc0.asm).

Before any variation, the entire frozen owning composition executes and compares unchanged. The oracle takes the original request154/result70 packets captured at source call387, then restores their exact bytes into their recorded original stack backing (`021FCE68` / `021FCD7C`). It retains the actual current source-owned unit/body/physical state after the composition; it does not reconstruct a unit pose or reset its manager. The classifier only consumes the recorded request words and the actual physical-state byte. [Source packet provenance and complete body](../analysis/multiplayer/integration/inventory-source/unit-locomotion-measured/report.json).

Only the following supplied fixture fields vary:

- Request+DC/E0/E4:196 explicit binary32 bit-pattern vectors, including the zero/±1 cube, every signed-zero combination, minimum/maximum subnormal, minimum normal, adjacent-to-one, maximum finite, infinities, signed quiet/signaling NaNs and signed axis/tie comparisons.
- Result+4/+8: original zero class/blend, or explicitly supplied retained class`12345678`/blend`A5`, to measure preservation rather than assume a new idle store.
- Request+14 bit20: clear for direct/normal wrapper cases; set for the independent flag branch. The other request bits/bytes remain exactly source-produced.

Each vector/seed combination executes the direct leaf, the actual state1 wrapper, and the flag20 wrapper:196×2×3=1,176. Root-call ABI and inherited CPU reset FPCW`027F`, MXCSR`1F80` remain explicit. The untouched source instructions execute on each case; no lower service, contact, pose, frame, velocity, queue, membership, physical-state or success output is assigned. The report retains full request/result before/after bytes, unchanged complete owner bytes/hash, physical bytes, every source read/store and full registers/EFLAGS/XMM/x87/FPCW/MXCSR at each instruction.

## Exact classifier output contract

`1F52F0` takes ECX=request154 and EDX=result70. It has no inferred semantic EAX/AL return. The native API reproduces stored result bytes and store order; source register/FP flags are retained in evidence but are not returned by the native API.

| Source condition, in original order | Owning output store |
|---|---|
| All three axes compare equal to zero, including either signed zero | No store; retain result+4 and result+8 |
| Any nonzero/unordered axis; ordered `abs(y)>abs(x)`; `y<0` | `1F5349`: result+4 dword2 |
| Same Y-dominant branch; `y>=0` | `1F5351`: result+4 dword3 |
| X-dominant, equal-magnitude or unordered magnitude comparison; ordered `x<0` | `1F5362`: result+4 dword5 |
| Same X branch; `x>=0`, either signed zero or unordered X | `1F536A`: result+4 dword4 |

The admission test is SSE `UCOMISS→LAHF→TEST AH,44` parity, with exact zero comparisons; **no epsilon or magnitude threshold exists here**. The x87 `FLD/FABS/FCOMPI` compares absolute Y against absolute X; equality and unordered take JBE's X branch. The final SSE `COMISS 0,axis/JBE` treats equality/unordered as the positive branch. Z only affects admission, so a pure nonzero Z with zero X/Y stores class4. A NaN in Y takes the X branch; negative finite X with NaN Y stores class5. NaN X stores class4 regardless of its sign. All these edge cases were measured, not supplied as expected native outputs. [Unchanged classifier](../analysis/multiplayer/integration/inventory-source/unit-locomotion-measured/instructions/001f52f0.asm).

Only result+4 changes on the directional leaf; result+8 and every other byte remain intact. The wrapper's state1 output equals its paired direct leaf output for all392 cases, including retained nonzero classifications and blends. There are32 measured no-write direct/wrapper calls, and every one retains the complete original result packet. [Literal packet/store audit](../analysis/multiplayer/integration/inventory-source/unit-locomotion-measured/audit.json).

## Exact bounded wrapper and unsupported routes

`1E94E0` takes EAX=request154, ECX=physical state, ESI=result70. It first tests request+14 bit20. If set, it writes result+4 dword1 at`1E94EB`, then result+8 byte1 at`1E94EE`, and returns before reading the physical-state byte. All392 flag cases execute both stores in that exact order. If clear, it reads the actual state byte1 and dispatches through the real table at`1E9520` to`1E94FD→1F52F0`. [Wrapper and original stores](../analysis/multiplayer/integration/inventory-source/unit-locomotion-measured/instructions/001e94e0.asm).

The original six-entry table is retained raw (`FD941E00 06951E00 F1941E00 14951E00 14951E00 0D951E00`):

| Physical byte | Original table target / lower path | New source execution coverage |
|---|---|---|
| 1 | `1E94FD→1F52F0` | Measured actual source-owned state |
| 2 | `1E9506→1FE870` | Listed only; lower producer unavailable in this port |
| 3 | `1E94F1` RET | Listed only; not executed by this fixture |
| 4/5 | `1E9514→1FFFE0`, EDX=physical+10 | Listed only; lower producer unavailable |
| 6 | `1E950D→1EFDE0` | Listed only; lower producer unavailable |

The native bounded wrapper returns `unavailable_physical_state` for every non1 state with bit20 clear and leaves its output untouched. That is an **explicit native stop**, not a claimed original result or successful admission. The returned target is the literal source table target for bytes2..6; out-of-range table indexes report target0. Even the source-listed state3 RET remains outside measured native execution. Missing physical storage fails when the physical byte is actually needed. [Literal table and unchanged physical record](../analysis/multiplayer/integration/unit-locomotion-fixtures/source-owner.json).

## Native files, export and validation

The added API is [unit_locomotion.hpp](../include/halo2/unit_locomotion.hpp), implementation [unit_locomotion.cpp](../src/unit_locomotion.cpp), with [source differential harness](../tests/unit_locomotion_tests.cpp). `classify_source_locomotion_request` consumes fixed0x154/0x70-byte packets; `dispatch_source_locomotion` additionally consumes literal physical storage. Both return up to two ordered source store records (instruction, output offset, width, bits) plus a measured path or explicit unavailable route.

The output implementation uses binary32 magnitude bits to preserve exact comparisons, subnormal ordering, signed zero and unordered semantics independently of host floating arithmetic. It does not reproduce FP exception/status-register side effects. It changes only the original owned stores, and emits no idle default, clip, movement threshold or world-velocity substitution. Source request/output nonaliasing matches the admitted original fixture; the suite does not claim other ABI alias layouts or altered DAZ/FTZ/unmasked-FP environments.

[Exporter](../tools/export_unit_locomotion_fixtures.py) verifies the independent original report/audit/listings and206 old pins, then writes original buffers/store traces and complete owner/provenance into ignored analysis. [Compact schema](../analysis/multiplayer/integration/unit-locomotion-fixtures/schema.json). Its expected values never depend on native implementation output. [Differential](../tools/differential_unit_locomotion.py) compares every output byte and each source store's site/order/width/value; it also verifies request/physical buffers stay intact. Interface checks separately ensure unmeasured routes remain unavailable.

```sh
mkdir -p analysis/multiplayer/integration/unit-locomotion-native
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 -Iinclude \
  src/unit_locomotion.cpp tests/unit_locomotion_tests.cpp \
  -o analysis/multiplayer/integration/unit-locomotion-native/unit_locomotion_tests
.tools/python/bin/python tools/differential_unit_locomotion.py
clang++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O1 -g \
  -fno-omit-frame-pointer -fsanitize=address,undefined -Iinclude \
  src/unit_locomotion.cpp tests/unit_locomotion_tests.cpp \
  -o analysis/multiplayer/integration/unit-locomotion-native/unit_locomotion_tests_san
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
  .tools/python/bin/python tools/differential_unit_locomotion.py \
  --binary analysis/multiplayer/integration/unit-locomotion-native/unit_locomotion_tests_san \
  --output analysis/multiplayer/integration/unit-locomotion-differential-san.json
```

Both output: `1176 original classifier/wrapper packets and 1536 ordered stores matched; 32 no-write paths preserved`. [Normal comparison](../analysis/multiplayer/integration/unit-locomotion-differential.json), [sanitizer comparison](../analysis/multiplayer/integration/unit-locomotion-differential-san.json).

Exact additions for the parent to apply to CMake, alongside existing bounded source kernels/tests (not applied here):

```cmake
target_sources(halo2_systems PRIVATE src/unit_locomotion.cpp)
add_executable(unit_locomotion_tests tests/unit_locomotion_tests.cpp)
target_link_libraries(unit_locomotion_tests PRIVATE halo2_systems)
add_test(NAME retail_unit_locomotion COMMAND unit_locomotion_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/unit-locomotion-fixtures/fixtures.tsv")
set_tests_properties(retail_unit_locomotion PROPERTIES SKIP_RETURN_CODE 77)
```

The prior [selector suite](SPARTAN_EQUIPPED_ANIMATION_1_1_RESEARCH.md) independently associates classes2/3/4/5 with source selector clips191/190/189/188 for its declared inputs. This classifier does not rerun that selector or produce those channel states. Source input submission/delivery/C8140 and the full non-neutral physics request producer still must own the inputs, followed by the actual `E44C0` request selection, channel clock, local/current pose and common rendered/collision matrices. Live local-device identity/readiness, finite source game-clock production/successive admitted frames and other physical/Havok routes remain open. No live-animation or player-versus-player damage fix is claimed by this bounded port.
