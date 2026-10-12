# Native unit input and inventory-slot consumers

The native `C8140` unit-input consumer matches all **10 recorded calls** from
the five original transfer cases. Each check compares the complete `0x800`-byte
unit prefix, including every untouched byte. The `CEA70` empty-slot consumer
matches all **9 recorded scans**. Normal and focused address/undefined/leak
sanitizer comparisons pass. Authority: [the measured original transfer chain](MP_PICKUP_TRANSFER_MEASURED_RESEARCH.md).

These are consumers of actual source storage and marshalled input. They do not
produce live player controls, initialize native unit storage, admit a pickup,
assign ownership, remove an item from the world or attach it to a hand.
The production match does not yet run this complete live transfer chain.

## Original behavior

`apply_source_unit_input` takes a separate `0x7C`-byte input buffer and a mutable
unit body. It copies the source-defined fields in original order without
normalizing vectors, interpreting opaque fields or changing untouched bytes.
The final copy contains nine words through input offset `0x7B`; the allocated
unit's separate `0x44`-byte control block cannot substitute for this input.
Source: [C8140 instructions](../analysis/multiplayer/integration/inventory-source/pickup-transfer-measured/instructions/000c8140.asm).

The inventory selection copy is conditional on the incoming 16-bit sequence
matching the unit word at `+0x214`. The measured primary BR pickup has stale
sequence 1 after the unit advances to 2, so its selection write is skipped.
The measured secondary SMG pickup refreshes its incoming sequence to 2 and
takes the copy. The original remaining gameplay flags are copied in both
paths. Neither result is replaced with a pickup-only payload.

`source_inventory_empty_slot` scans four complete 32-bit datum slots at
`+0x218..+0x224` in ascending order. The observed results are slots 0 and 1.
The source-listed full-inventory result is `-1`, but that branch has no executed
case in this measured suite. Source: [CEA70 instructions](../analysis/multiplayer/integration/inventory-source/pickup-transfer-measured/instructions/000cea70.asm).

## Reproduce the comparison

The differential verifies the immutable original manifest, measurement tool,
auditor, document, payloads, 114 dependency pins and 41 instruction listings.
It requires the original independent reexecution to remain byte-identical.
Fixture rows retain the owning case/call IDs and literal before/after buffers;
native code does not calculate expected results.

```sh
cmake --build build-multiplayer --target unit_input_tests -j4
python3 tools/differential_unit_input.py
ctest --test-dir build-multiplayer --output-on-failure -R '^retail_unit_input$'

ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
python3 tools/differential_unit_input.py \
  --binary build-multiplayer-asan/unit_input_tests \
  --output analysis/multiplayer/integration/unit-input-asan-differential.json
```

Literal source data and reports stay in ignored `analysis` folders. The C++
module builds without original data; the comparison returns skip code 77 when
its local fixture is absent. The native module contains code only.

Seven focused normal checks and the same seven sanitizer checks pass, including
the existing item-point/visibility/terrain and sound-variation comparisons.
A fresh asset-free source snapshot builds and runs 58 checks: 18 pass and
40 original-data comparisons skip. Its report pins 710 copied source files.
The rebuilt title and four-player Containment captures in `51-unit-input-build`
have stable runtime/shader hashes and no Vulkan validation diagnostics.
These rendering checks do not exercise this input consumer in a live pickup.

## Remaining integration

Source hardware/action marshalling, live current unit bodies, `CFE60` game-mode
and sequence orchestration, repeated capability/admission, `CFED0` ownership,
world removal, marker attachment and current physics remain separate gates.
This comparison covers 19 calls, not all 815 calls in the original transfer
measurement. Ordinary scheduler activation and contact producers are still
being traced against the original executable.
