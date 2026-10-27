# TU1.5 v1457 contact scratch difference diagnosis

The retained Cyclotron lower0 capture still fails the unchanged full-bank
original comparison: 95 bytes in 22 ranges. Physical original hardware tracing
identifies every range inside one temporary contact-record allocation that has
been freed before the frame returns. This diagnosis does not change the raw
comparison, expected banks, exclusions, or physics implementation. It does not
close live parity or native-only runtime qualification.

Input: `analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-original-v1457/iteration-00-lower-body1-phase0/original/required-failure-inputs`.
Authority: the pinned English TU1.5 XBE used by
`research_mp_sandbox_update_functions.NEW_PIN`; each retained report records the
exact image and capture metadata hashes. Diagnostics execute original i386
instructions unchanged. GDB hardware execution breakpoints and data watchpoints
observe the original; they do not replace original text or provide callee
results.

## Source and physical lifetime

The actual collision owner `304350` initializes a private array at
`3043E0..3043F7`: local aligned storage, count zero, capacity `80000004`.
Records have stride `80` bytes. Actual `303EE0` appends a record, growing through
`2E3250` when count equals capacity. The fifth append grows the capacity from
four to eight. At `2E327E`, physical return value is `10082380`, the allocation
size is `400`, and `2E2E43 REP MOVSL` copies the four existing records (`200`
bytes) from stack `600EFD20` to this allocation.

Physical allocator object `486080`, vtable `45F4CC`, uses allocate owner
`149DA0` and free owner `149E00`. This buffer lies in pool `10076040`, vtable
`4638B8`. Pool class lookup `[pool+2B0]` is 13. The corresponding metadata is:

| Field | Address | Allocated / before free | After free / frame return |
| --- | --- | --- | --- |
| Free-list head | `10076098` | `10083B80` | `10082380` |
| In-use count | `1007635C` | 2 | 1 |
| Free-block count | `101663FC` | 6 | 7 |

At actual `304AA0`, observed free arguments are exactly
`[10082380, 400, 12]`, and the private array state is `[10082380, 6, 8]`.
Actual `149E00` dispatches this address to pool free owner `233420`.
Hardware watchpoints observe `23346E` writing the previous free-list head
`10083B80` into buffer word zero; `233470` installs `10082380` as the head.
At `304AA3`, the free has returned. The complete `400`-byte buffer and all
listed metadata remain unchanged through frame return `1CCACA`. Hardware
breakpoints on both root allocate `149DA0` and pool allocate `233250` observe
zero further allocations after this free.

All 22 ranges/95 bytes are within `[10082380,10082780)`, the freed buffer.
Every differing byte is in the first five records. Their private contact-valid
byte `+7D` is false in the physical original. The sixth record's valid byte is
true, and that record has no differences. The differences occur at record
offsets `0C..19`, `28..2F`, `4C..4D`, and `7E..7F`. Actual body/component/key
fields `+70/+74/+78`, flags `+7C/+7D`, and the sorting scalar `+20` match in
this capture. This is a capture-specific observation, not a general waiver for
these offsets in active records or live allocations.

## Stale pointer provenance

`303EE0` reserves `8C` stack bytes and initializes only the record's two private
flag bytes to false (`303EF4` and `303EFB`). It then passes local record
`600EFB70` to `303BA0`, which copies most fields to the new array slot.
`303BC8/303BCC` copy the `+40` vector, including its `+4C` lane. There is no
record initialization of that lane on this false-record path. The constructor
also leaves `28..2F` and `7E..7F` unwritten; the array growth copies complete
`80`-byte records, including these gaps.

An independent hardware watchpoint on stack word `600EFBBC` finds its precise
writer: actual `1F600E PUSH EBP` in the preceding `1F5FE0` helper writes the
then-caller frame pointer `600EFF38`. The later local record reuses that stack
address as `600EFB70+4C`. The physical constructor snapshots show the same stale
word passed to `303BA0`. The sort in `304350` subsequently copies it through
`304792` and writes it at `30482B` into the first heap record; corresponding
records contain the same stale lane. Thus `600EFF38` is a copied saved frame
pointer, not an initialized physics object backpointer. The native residual
collision call uses another stack location and leaks the corresponding stale
`600EEF3C` into the same inactive payload. Exact native writer corroboration is
being captured separately by the live builder.

When `+7D` is true, `30456F..3045F9` fills contact vectors/scalars before use.
The false records retain source private scratch content. In particular, their
first vector's fourth lane and later payload can differ even though the
contact body/key/flags and active-contact state agree. Source sorting still
reads selected fields of every record, so this diagnosis does not authorize
ignoring arbitrary inactive payload during execution.

## Retained diagnostics and cleanup

All helpers ran through `tools/run_test.py` with generated ELF, raw banks,
debugger scripts, and debuggee output under its private scratch. These three
small diagnostic directories remain beneath the v1457 evidence directory:

- `lower0-original-write-watchpoints`: 11,909 bytes; six observed writes.
- `lower0-original-constructor-stack-writers`: 22,816 bytes; thirteen observed
  writes, including the saved-EBP origin and pool free-list write.
- `lower0-original-contact-lifetime`: 51,304 bytes; constructor, allocation,
  free, pool metadata, exact range coverage, and final-frame snapshots.

The report-recorded private scratch paths were checked absent after each
completed run. No ELF, raw bank, frame, or runtime binary was retained by these
diagnostics. Required existing failure inputs were preserved. Total retained
diagnostic size is 86,029 bytes.

The appropriate next proof is a separate source-owned allocation-lifetime
audit plus the native writer trace, while preserving the raw-bank failure.
Recreating old stale stack bytes or fabricating original register outputs
would not constitute a native semantic port.
