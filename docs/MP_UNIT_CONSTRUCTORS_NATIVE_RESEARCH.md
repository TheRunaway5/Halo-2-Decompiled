# Retained unit constructors: original writers and required owners

The isolated native module implements complete direct owners `1CE8C0`, `1CE9B0`, `1CEAC0`, `BD270` and `C52C0`. **34 complete original buffer cases, 28 explicit nested-service boundaries and 2,394 ordered owning writes pass** in normal and ASan/UBSan builds. These are real constructor outputs from the original allocated biped/SMG path, not a preallocated unit fixture. Graph selection, body-heap growth, extension reset and variant setup remain required services. The module does not create a ready live native unit or enable switch/melee/grenade gameplay by itself.

The implementation is [unit_constructors.hpp](../include/halo2/unit_constructors.hpp), [unit_constructors.cpp](../src/unit_constructors.cpp), and [unit_constructor_tests.cpp](../tests/unit_constructor_tests.cpp). Only those new files, new observation/export/correspondence tools, this document and the constructor CMake source/test lines were added. Runtime, `Simulation`, `Unit`, frontend, renderers, shaders and immutable releases were not edited.

## Authority and genuine creation

The executed image is TU1.1 `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`. Lockout is `assets/disc/maps/lockout.map`, SHA-256 `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`. The original root preparation and join/spawn/cluster fragment come unchanged from [the frozen PlayerTrace helper](../tools/oracle_mp_spawn_player_visibility.py) and its [published ownership boundary](MP_PLAYER_VISIBILITY_RESEARCH.md).

[The new original observer](../tools/oracle_mp_unit_constructors_1_1.py) supplies no new storage fields, datum identities, component offsets, graph choices, pose, readiness or allocator results. It executes the original constructor bodies and the actual `18FD70`, `1D0A60`, `110A20`, `116680`, and `110BF0` bodies. Its first four join/spawn/cluster stage records equal the prior positive fixture byte-for-byte. Whole compatibility KPCR/real host storage, the selected CRT/new-map roots, delivered original default variant/property packet, resource residency and the inherited options context retain their prior explicit scope. The helper is a bounded nonlocal join, not complete title/session/device startup.

`B76B0` requests the original object-header pool **2048 × 12 bytes** at `B76C8..B76DB`, then requests the original 1 MiB body heap at `B76E0..B76F5`. The owning [item-current producer freeze](MP_ITEM_CURRENT_PRODUCERS_RESEARCH.md) executes that full initializer. The new report captures the real 0x4C pool records, capacity, stride, high-water, full salted datum, 12-byte header and complete body before/after each call. The native constructor routines retain the original low-16 index lookup; they do not add a salt check that is absent from these routines. The caller must supply an already owned valid object. This does not prove complete authored world allocation order.

Generic creation `B8A30` first constructs/binds a temporary manager (`B8E17/B8E30`). It then calls `BD270` for the retained `+128` descriptor and calls `1CE9B0/1CEAC0` on the resulting signed `unit+12A` manager (`B8FD9/B8FF0`). Actual biped `E26F0000` receives these source-produced component descriptors during this creation:

| Size field | Offset field | Size | Measured signed offset |
|---|---|---|---|
| `+128` | `+12A` | `90` | `1426` |
| `+33C` | `+33E` | `124` | `14B6` |
| `+340` | `+342` | `58` | `15DA` |
| `+344` | `+346` | `44` | `1632` |

These offsets are evidence, **not native constants**. The module derives them through the actual header extent and append arithmetic. `C52C0` finishes this constructor before original inventory/equip creates SMG `E2700001`. The manager has three 32-byte channels; extension `+9C` is a separate overlay. The positive constructor manager initially selects packed `0104FFFF`; source equip subsequently produces clip 185, as retained by the earlier owned-pose evidence. No native clip is installed by this constructor module.

## Exact writers and resets

`1CE8C0` individually initializes the channels, including duplicate `+4/+6` word writes, clears bytes `60..67`, and tail-jumps to full `1CE9B0`. Reset writes the three channels, graph/mode/class/name/activity sentinels, options and zero clocks in source order. Both retain `84..8F`; **reset also retains bytes `62/66`**. An explicit reset of the genuine post-equip manager proves that distinction. Source one bits are copied from TU1.1 `465030`; the module never hardcodes that floating payload or performs arithmetic with it. Complete [constructor/reset listings](../analysis/multiplayer/integration/spawn-source/unit-constructors/measured-final-stop/instructions/001ce8c0.asm) retain every instruction, including [the reset tail](../analysis/multiplayer/integration/spawn-source/unit-constructors/measured-final-stop/instructions/001ce9b0.asm).

`1CEAC0` rejects graph NONE with a real model, or unequal graph signed-short node count versus model signed dword node count, without stores. Model NONE bypasses only that comparison. Success updates byte `6C` bit 1, clears identifiers/channels, calls full `1D0A60` with EAX `07000001` and stack `(07000001,07000001,07000001,317,3F)`, then clears transition bytes and writes byte `66=1`. It returns defined AL=1 regardless of the nested selector's result. Its required service is the **actual default graph-selection owner**, not a named-clip lookup or a true return. [Complete bind instructions](../analysis/multiplayer/integration/spawn-source/unit-constructors/measured-final-stop/instructions/001ceac0.asm).

`BD270` resolves the original header by low-16 datum index. Zero requested size writes descriptor offset `FFFF`, then size zero, and returns AL=1 without heap growth. Nonzero size computes `(1 << (alignment & 31))-1`, adds that padding to the sign-extended size, then calls `18FD70` with the sign-extended old header extent plus that sum. Only actual growth success permits the header-size, aligned signed-offset and descriptor-size writes. It clears the **entire newly appended padded span**, using dword stores followed by byte stores. The zeroing start itself sign-extends the old extent. It re-resolves relocated backing. There is no full initial-unit zero fill. Missing growth throws; an explicit false result preserves owning bytes. Bounds checks reject invalid host storage instead of trying an out-of-bounds original guest access. [Complete allocator instructions](../analysis/multiplayer/integration/spawn-source/unit-constructors/measured-final-stop/instructions/000bd270.asm).

`C52C0` preserves every direct sentinel/width/order, including sequence `+214` before `+210`, four inventory slots and four 16-byte history rows. A missing manager offset returns AL=0 after that prefix, without writing the failure byte. It requests `124/58/44` bytes at descriptors `33C/340/344`; a failed append writes failure byte 1 and retains prior mutations. Then it initializes action-component `+10/+14/+18/+2C`, extension `+2C`, executes required `110A20`, initializes extra-component `+0/+44`, executes `116680`, copies forward vectors, copies the signed default grenade count's low byte only through the source type/count gates, writes flags/category, and executes `110BF0`. Finally it searches the actual model `+78/+7C` **node** block (stride `5C`) for StringIDs `080001A2/090001A3`; a miss stores byte `FF`. These source fields are node indices, not model region indices. [Complete unit constructor instructions](../analysis/multiplayer/integration/spawn-source/unit-constructors/measured-final-stop/instructions/000c52c0.asm).

The initial action words at resolved component `+4/+8` are source-produced zero because `BD270` clears that newly allocated `44`-byte component. `C52C0` does not itself write those action words. Neither a native `grounded` flag nor an all-zero invented unit body establishes this ownership.

## Remaining native construction chain

The storage interface requires actual fixed-pool headers and owned body backing. Its `grow` service represents the unchanged `18FD70` allocation protocol, including relocation and updating header `+8` while leaving `+6` to the caller. A vector growth adapter alone is not a claim of original heap/world membership parity. Genuine base-body/type/component production before `C52C0` remains the owning `B8A30` prerequisite.

The next graph owner is `1D0A60`, whose actual default request calls `1CFDE0` at `1D0B04`, followed by its graph resolution/lookup/selection/transition owners. The existing native definition importer and channel clock do not replace it. The extension reset `110A20` first invokes `110800`, then node-name lookup `16F510`, manager class/name reset and `1100E0/10F380/115030`. `116680` owns actual model variant/collision reference selection; `110BF0` owns selection and transitions through `1D0A60`. All execute unchanged in the positive oracle; none is a successful default service in native code. Their complete before/after buffers and defined input ABIs are retained so each can be ported independently next. [Required-owner listings](../analysis/multiplayer/integration/spawn-source/unit-constructors/measured-final-stop/instructions/001d0a60.asm), [extension reset](../analysis/multiplayer/integration/spawn-source/unit-constructors/measured-final-stop/instructions/00110a20.asm), [variant](../analysis/multiplayer/integration/spawn-source/unit-constructors/measured-final-stop/instructions/00116680.asm), [request](../analysis/multiplayer/integration/spawn-source/unit-constructors/measured-final-stop/instructions/00110bf0.asm).

Three additional explicit bind roots vary only actual imported graph/model identities or NONE on the same source-owned manager: missing graph rejects, actual 4-node SMG graph versus 33-node Spartan model rejects, and real Spartan graph with model NONE completes. **Both graph/model NONE reaches an unresolved invalid original access**, rather than yielding a safe bind: `1CEAC0→1D0A60→1CFDE0`, at `1CFDFD`, reads `0117FFF8` (tag-table low index `FFFF`). Its complete stopped registers/ancestry are frozen. No output fixture or success is synthesized for that unfinished call.

[The next-selection contract](../analysis/multiplayer/integration/spawn-source/unit-constructors/next-selection-owner-contract.json) records all 15 direct callee identities/sites from the complete `1D0A60` listing and nine actual completed calls with complete manager entry/return bytes. Those calls produce definition pairs `(E9180793,0104FFFF)`, `(E9180793,00B9FFFF)` and `(E5860411,0001FFFF)`. Packed low-half `FFFF` must retain the original inherited/self resolution; these are measured outputs, not authorized clip defaults. `1D0590` is only the three-channel reset, so full `1CE9B0` cannot replace it. `1D1A00` owns a root-transform delta through `1CAD30/1CA890/1CF8B0`; `1D1B70` resolves current identifiers through `1CFCE0` and calls `1E16F0`. Raw graph hierarchy/fallback records, variant RNG, channel installation and those admitted resource/transform owners are the next measured producers. No speculative typed activity selector was started.

The real integration seam is retained header/body storage plus these constructors, followed by their real graph/reset/variant services, initial equip, command delivery, ordinary action/channel updates and current-pose callbacks. It does not justify wiring `F/Q/Tab/wheel` directly to a fabricated action component or an immediate slot flip. The separate positive carry/switch evidence remains required.

## Literal differential and authenticated u5 mapping

The final [original report](../analysis/multiplayer/integration/spawn-source/unit-constructors/measured-final-stop/report.json) contains 65 calls, 3,173 observed writes, 10 complete instruction listings, all buffers/ABIs and the exact stop. The 34 completed supported owner calls export to [literal fixtures](../analysis/multiplayer/integration/spawn-source/unit-constructors/native-frozen/fixtures.tsv), SHA-256 `58e05f57f12288915009d99449d0a3ab29dd77faa1034c55fd6142f4a573a8f1`. Nested outputs are replayed **only as explicit differential services**; they are not native runtime implementations. Tests compare complete caller buffers at every service entry, the original `Game+8` reader checkpoint, exact requested growth backing, every final byte, AL, order and each store's instruction/width/prior/new value. Seven additional host boundary checks cover missing services, failed allocation and malformed extents/indices; those are not promoted into original successful gameplay cases. The v1 report field `model_regions` and TSV token `REGIONS` are legacy serialization labels for the measured model `+78` **node block**; the native API correctly calls it `model_node_records`.

The report and all 10 listings independently repeat byte-identically in a separate process; the exporter also repeats exactly. [Authenticated u5 correspondence](../analysis/multiplayer/integration/spawn-source/unit-constructors/u5-correspondence/report.json) finds unique complete normalized instruction/internal-branch matches for every native and required owner, with raw bytes, direct calls and relocated data operands retained:

| TU1.1 | Authenticated u5 |
|---|---|
| `1CE8C0` | `1D3180` |
| `1CE9B0` | `1D3270` |
| `1CEAC0` | `1D3380` |
| `BD270` | `BE3B0` |
| `C52C0` | `C6440` |
| `1D0A60` | `1D5320` |
| `18FD70` | `191130` |
| `110A20` | `111940` |
| `116680` | `1175A0` |
| `110BF0` | `111B10` |

The one word maps `465030→46A0D0` and has identical source bits. This is static u5 correspondence; actual constructor execution is TU1.1 only. Its report plus 20 complete listings independently repeat exactly. The authenticated later image SHA-256 is `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`. No address-delta inference or attached semantic label establishes these matches.

## Reproduction and pins

Run from the repository root; source/cache inputs and literal outputs remain ignored external evidence. No copyrighted fixture blobs are copied into tracked files.

```sh
.tools/python/bin/python tools/oracle_mp_unit_constructors_1_1.py --output analysis/multiplayer/integration/spawn-source/unit-constructors/measured-final-stop
.tools/python/bin/python tools/oracle_mp_unit_constructors_1_1.py --output analysis/multiplayer/integration/spawn-source/unit-constructors/repeat-final-stop
.tools/python/bin/python tools/export_unit_constructor_fixtures.py analysis/multiplayer/integration/spawn-source/unit-constructors/measured-final-stop/report.json analysis/multiplayer/integration/spawn-source/unit-constructors/native-frozen/fixtures.tsv
.tools/python/bin/python tools/research_unit_constructor_u5.py --output analysis/multiplayer/integration/spawn-source/unit-constructors/u5-correspondence
.tools/python/bin/python tools/research_unit_constructor_u5.py --output analysis/multiplayer/integration/spawn-source/unit-constructors/u5-repeat
cmake -S . -B build-unit-constructors-20261008 -DCMAKE_BUILD_TYPE=Release
cmake --build build-unit-constructors-20261008 --target unit_constructor_tests -j2
ctest --test-dir build-unit-constructors-20261008 -R '^original_retained_unit_constructors$' --output-on-failure
```

The standalone validation also builds exactly `src/unit_constructors.cpp` plus `tests/unit_constructor_tests.cpp` using GCC `-std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror`, and Clang `-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer` with the same strict warnings. [Native validation](../analysis/multiplayer/integration/spawn-source/unit-constructors/native-frozen/validation.json) records exact commands, compilers, binaries and source pins; [final manifest](../analysis/multiplayer/integration/spawn-source/unit-constructors/manifest.json) records new files, source images, previous 39 immutable pins and repeat results. The initial CMake observation was followed by an unrelated six-line `game_variant_normalization_tests` registration. Both full observations and their exact diff are retained; the constructor stanza stayed identical and its target/CTest were rebuilt against the final stable observation. No whole mutable-repository pin is asserted. Earlier drafts, first failed declaration-only bind observation and initial 31-case comparison are retained separately. No GPU, device, natural pickup/switch or complete native world-constructor claim is made.
