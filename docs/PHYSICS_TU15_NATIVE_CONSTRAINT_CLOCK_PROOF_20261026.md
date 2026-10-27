# Native TU1.5 constraint clock arithmetic

The initializer's actual `37C5C0` owner calls `32BBAE` with a null optional
output pointer. The original obtains an eight-byte FILETIME from `33A4D3`,
adds `0xFE624E212AC18000` modulo 2^64, divides the unsigned result by 10,000,000
through `32AE00`, and returns the low 32 bits. All four original bodies are
integer-only and preserve incoming x87 values and FP state.

The native clock kernel performs that exact unsigned arithmetic. It obtains
FILETIME through one required callback. The live world must supply its
existing recorded input:

```cpp
source_execution_input(bank, SourceExecutionInputKind::UtcFiletime, 0,
    [this] { return clocks->utc_filetime() + utc_adjustment; });
```

The kernel queries no independent clock and authors no timestamp. Missing or
failed input is explicit; it never supplies a fallback seconds value. This
preserves the selected world clock, adjustment and source input event order
when the caller wires the typed owner.

Both GCC and Clang pass 292 unchanged original unsigned division cases and
ten genuine original clock-return cases. The arithmetic cases cover epoch
boundaries, unsigned wrap and low-32-bit seconds rollover. The clock cases
consume FILETIME from the actual native host-clock import; the unchanged
original computes its own seconds return. Native tests use only the genuine
raw FILETIME input. Actual two live 80-bit x87 values, control/status and MXCSR
survive the native callback and conversion. Changing the authenticated epoch
delta by one fails the original comparison.

The small report, commands, source hashes, original listing hashes and
negative-control results are in `build/test-results/native-constraint-clock-tu15/`.
Its 156,706-byte fixture, private original banks/executables and both compiler
build trees were removed; `cleanup.json` confirms owner removal. The world
input receipt wiring and visible multiplayer participation require their own
current live evidence.
