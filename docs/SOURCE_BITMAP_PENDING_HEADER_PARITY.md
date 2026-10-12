The new `source_bitmap_pending_header` produces all five words of the original
`12D6D0` pending header: Common, Data, Lock, Format and Size. It admits signed
format indices 0 through 23 and partitions 0 through 2 before any caller can
mutate reservation banks. Width, height and mipmap count retain their complete
signed 16-bit ranges; depth retains its signed 8-bit range; type and flags
retain their unsigned 16-bit ranges. The return is a value, and inputs remain
unchanged. Format-table entries equal to `FFFFFFFF` remain literal source
metadata; admitting those entries does not establish renderer support.

[The complete unchanged-source execution report](../analysis/multiplayer-shading/source-bitmap-pending-header-original.json)
records 19,472 exact native/source cases, 97,360 header words and 2,258,752
original input-record bytes preserved. The first 42 cases recover actual
metadata, partition, level address and previously observed header words from
the immutable 168-call allocator report. Each group of four wait/force controls
has the same header observation; the full original producer independently
recreates every observation. Those inputs establish no successful resource
read or completion. The remaining 19,430 cases are explicit CPU metadata
controls:

- 1,728 format/type/flags/partition combinations.
- 576 additional raw type combinations.
- 13,464 signed and zero dimension, pitch-clamp and pitch-rounding cases.
- 1,584 signed and wrapped maximum-level cases.
- 30 level-address masking cases.
- 2,048 full narrow-field controls.

The proof executes complete original `12D6D0`, including its actual `359E0`,
`1365A0` and `13E3A0` calls. It checks each of 272 reached instructions against
the pinned XBE once; input patches are isolated from source code, and original
writes are confined to the disposable output and stack. All original format
and bit-depth tables are used unchanged. The separate
[independent disassembly review](../analysis/multiplayer-shading/source-bitmap-pending-header-independent-review.json)
corroborates code and table inspection; it supplies no execution results.

The nonlinear branch retains zero after signed shifts, accepts zero in the
original power test, wraps level subtraction to signed 16-bit, returns a signed
minimum level count, and uses unsigned floor-log2 for the widened dimension
words. The linear branch calls `1365A0` with shift zero, clamps signed width to
one before optional alignment to four, then sign-extends the rounded 16-bit
width. Thus widths 32765 through 32767 with flag 2 can round to -32768 and yield
a negative pitch. Signed pitch division truncates toward zero. Both branches
preserve flag `20` overrides for formats 10 and 11; the nonlinear branch
preserves type 1 and type 2 encoding. All final packing uses wrapping 32-bit
words, and Data retains only the level address's low 28 bits.

The native frozen-corpus test validates all 97,360 words and six metadata
admission controls. Missing, empty and last-row-truncated corpus files fail.
The same corpus passes ASAN, UBSAN and leak checks. This proof supplies no
readiness, callback, async job identifier, I/O outcome, kernel result, GPU state
or runtime activation. It is a pure preflight for a separately proved pending
caller.

[Durable native validation](../analysis/multiplayer-shading/source-bitmap-pending-header-validation.json)
records the preserved native binary identities and corpus rejection checks.
It retains the completed original report as execution authority and does not
rerun or relabel the original corpus.
