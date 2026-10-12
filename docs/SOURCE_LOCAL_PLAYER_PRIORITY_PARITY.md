# Original local-player shadow priority

`source_local_player_priority` ports the complete ordinary `3E9C0` predicate,
including the unchanged `155760` observer classification and `BAF80` parent
traversal. It can replace the shadow candidate priority callback when the
caller supplies the actual per-view local-user, observer, and owner banks.
It does not initialize or infer those banks.

## Source fields and control flow

`3E9C0` returns **AL**, not the complete EAX register. Owner `NONE` returns zero
before reading anything else. Otherwise it reads the signed current local
slot at `4B9ED8`; slot `NONE` also returns zero. For a valid slot it runs
`155760`, and every nonzero observer class returns zero before owner lookup.

Observer records begin at `4E8C44`, with four records of stride `0x140`.
The callback identity is at `+0xC` and the scalar binary32 value is at `+4`:

| Callback | Additional condition | Class |
| --- | --- | --- |
| `23C110` | Scalar compares ordered-equal to zero | 0 |
| `23CBB0` | None | 1 |
| `16C840` | None | 2 |
| `23D090` | Byte `51EC10` is nonzero | 2 |
| Any remaining case | None | 3 |

The original `UCOMISS`, `LAHF`, `TEST AH,44`, and parity branch reject NaNs.
Both signs of zero match. The recorded MXCSR is `1FB0`, with denormals-as-zero
disabled, so subnormals do not match zero. The native helper preserves raw
scalar bits and implements this exact ordered-zero comparison without
depending on the caller's floating-point environment. No callback is invoked.

For class zero, `BAF80` follows actual full-handle parent links at object
`+0x14` until `NONE`. Object headers come from `(*4E0300)+0x44`, stride12;
the root header's byte `+3` must be zero. The root body pointer is header
`+8`. Its player datum at `+0x13C` must equal the local player datum at
`(*4E8C20)+0xC+slot*4`. Explicit `NONE` datums can compare equal. The routine
does not add a separate player-validity test.

The public input types retain optional fields. Only fields read on the
selected source branch are required. Missing data, invalid source slots,
missing full handles, duplicate selected identities, and parent cycles are
reported as unresolved input errors rather than replaced with defaults.

## Independent evidence

`tools/differential_source_local_player_priority.py` loads the pinned original
disc XBE and executes unchanged `3E9C0`, `155760`, and `BAF80` in Unicorn.
There are no observer, owner-lookup, math, or return service hooks. Instruction
observation verifies the executed bytes against the original image; a single
external stop address ends each call. Fixtures cover callback identities,
signed zeros, subnormals, infinities, quiet/signaling NaNs, all four local
slots, nonboolean special bytes, root kinds, parent chains, equal/unequal
player handles, and the source `NONE` comparisons and early returns.

The last frozen row reconstructs an actual recorded call from our isolated
original OpenGL/llvmpipe session: source tick12475, frame111186, view0,
owner/root `E2E60077`. Entry `3E9C0` and return `3355B` are retained directly.
The original return EAX is `82D96901`, so AL is one; the exit ECX is
`EC700000`, the datum used by the final comparison.

The separate read-only export at paused `3290A` retains current slot0,
observer callback `23C110`, scalar `+0`, local player `EC700000`, and the
same root/header/body fields. The consumed local-slot and root fields agree
with the earlier `33400` exports; the observer and local-player fields agree
with both earlier complete RAM snapshots using their actual page tables.
The actual AL1 independently requires observer class zero. The raw observer
bank was exported later at `3290A`; it is corroborating metadata, not a
simultaneous `3E9C0` bank export. The guest remained paused, with identical
clock and CPU registers before and after export.

The report is `analysis/multiplayer-shading/local-player-priority-original.json`.
Its manifests preserve file lengths, hashes, executable bytes, exact phase,
and earlier/later provenance. Frozen fixtures are under
`tests/fixtures/retail_source_local_player_priority.*`.

The fixture contains 8,545 cases and 42,725 output bytes. The native test also
checks 16 unresolved-input cases and eight source early/read-branch controls.
Normal and ASAN/UBSAN/leak-enabled CTest runs pass. Final read-only QMP
verification still reports paused `3290A`, with complete CPU registers
identical to the metadata export.

## Integration boundary

The helper proves the predicate from explicit input banks. It does not prove
native observer transitions, local-user assignment, active render-slot
selection, object ownership, successful registration, PVS ordering, candidate
cache lifetime, or physical Xbox behavior beyond the executed original CPU
routine. Production callers must supply their actual state producers. The
existing shadow candidate callback can call this helper with that state;
the helper does not change renderer or runtime submission order.

The version3 retained live-shadow fixture now supplies corroborated complete
priority inputs, independently asserts the actual original AL, and feeds only
the computed byte to its candidate callback. A changed local-player datum
must fail the recorded priority assertion. Its generator independently runs
unchanged3E9C0/155760/BAF80 against the actual202-owner source bank, preserving
the earlier/later provenance above. This closes the observed-return service
boundary for that fixture, while native state production remains explicit.
