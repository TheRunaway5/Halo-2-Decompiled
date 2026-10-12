# Native inventory-slot and sequence orchestration

The native `CFED0` orchestration matches all **10 recorded slot operations**
and `CFE60` matches all **10 recorded sequence consumers**. Each comparison
checks the complete `0x800`-byte unit prefix and the exact ordered instruction
stores, including zero writes. The combined checks compare **59 native stores**
and preserve **19 explicit recorded leaf boundaries**. Normal and focused
address/undefined/leak sanitizer comparisons pass.

`FB120`, `FB3F0` and `CF8C0` remain explicit services in this slice. The comparison
supplies their literal original unit outputs and checks their order, arguments
and complete entry prefixes. It does not implement or prove native ownership,
world removal or physical drop inside those leaves. This module is separate
from the live production pickup path. Authority:
[original transfer measurement](MP_PICKUP_TRANSFER_MEASURED_RESEARCH.md),
[consumed options and store order](MP_PICKUP_TRANSFER_OPTIONS_RESEARCH.md).

## Original order

An insertion assigns the original full item datum's owner, removes world
membership, writes that same datum into the selected slot, and clears that
slot's ready word. It then checks the network entity before advancing the
inventory sequence. It does not allocate or delete the item or clear its
originating map row. Ownership assignment and world removal remain distinct
services; the nested bounds/location unregister does not substitute for final
world removal.

A removal clears the inventory slot before entering the drop service, retaining
the original item identity and removal selector. This path leaves the slot's
ready word untouched. Removal selector 4 skips the drop service in the source
listing, but has no executed case here. Native slot inputs outside 0..3 fail
explicitly; they are not represented as measured original behavior.

The sequence consumer reads the caller's actual options byte before resolving
the unit. It then increments/masks the low sequence byte and writes `+0x210`
before `+0x214`, both as 16-bit words. The measured consumed byte is `01`; it
comes from the separate read-only owning observation. No semantic mode name
or live options initializer is inferred. Value 4's early-return branch and
low-nibble wrap are source-listed but unexecuted by this suite.

All 20 original network gates read `NONE`. The native non-NONE path exposes
the instruction-derived dirty masks as explicit services in the same order,
and fails if the service is unavailable. There is no measured network-dirty
callback or invented successful network result. Sources:
[CFED0](../analysis/multiplayer/integration/inventory-source/pickup-transfer-measured/instructions/000cfed0.asm),
[CFE60](../analysis/multiplayer/integration/inventory-source/pickup-transfer-measured/instructions/000cfe60.asm).

## Reproduction and evidence boundary

```sh
cmake --build build-multiplayer --target unit_inventory_tests -j4
python3 tools/differential_unit_inventory.py
ctest --test-dir build-multiplayer --output-on-failure -R '^retail_unit_inventory$'

ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
python3 tools/differential_unit_inventory.py \
  --binary build-multiplayer-asan/unit_inventory_tests \
  --output analysis/multiplayer/integration/unit-inventory-asan-differential.json
```

The differential pins both original freezes, their tools/documents/payloads,
154 inherited options dependencies, the prior 114 transfer dependencies and
41 complete transfer listings. Original independent repeats must remain
byte-identical. Fixture values are literal original stores and buffers, never
native-calculated expectations. Original data and exported fixtures remain
under ignored local folders. The code builds without those files and the
comparison skips with code 77 when its local fixture is absent.

Eight focused normal checks and the same eight focused sanitizer checks pass.
The fresh asset-free source snapshot builds and runs 61 checks: 18 pass and
43 original-data comparisons skip. Its report pins 741 copied source files;
shared rendering additions are included in that snapshot. Records are
`unit-inventory-focused-tests.log`, `unit-inventory-focused-asan-tests.log`
and `code-only-unit-inventory/report.json` under ignored integration outputs.

The rebuilt broader suite runs 153 checks: 152 pass and the Ascension source
transparent-parts asset check fails. The runtime, test binaries, shaders and
configured test list stay stable during that run; shared CMake source changes.
The failed check and configuration drift are retained in
`unit-inventory-refreshed-all-tests.json/log`; this is not a full-suite pass.
Fresh title/three-player Containment and three-participant loopback LAN captures
are in `53-unit-inventory-build` and `54-unit-inventory-lan`. Both preserve
before/after runtime/shader hashes and produce no Vulkan validation diagnostics.
These captures exercise the existing multiplayer path, not live inventory leaves.

There are 39 stores inside the ten slot operations; independently comparing
the ten sequence consumers repeats their 20 stores, producing 59 comparisons.
The 19 recorded leaf boundaries comprise nine owner assignments, nine world
removals and one direct drop. The direct drop retains the original fixture's
unavailable Havok boundary and is not live settling evidence.

Live controls/unit storage, repeated admission, ownership/activation/bounds,
world registries, hand attachment, ordinary motion/contact, full frame order,
options initialization and actual network-dirty ownership remain integration
work. [Current item producers](MP_ITEM_CURRENT_PRODUCERS_RESEARCH.md) and
[allocated biped world research](MP_LIVE_BIPED_WORLD_RESEARCH.md) establish
source-created initial state with zero player masks; active motion remains
separate. Unavailable producers are not replaced with a successful return.
