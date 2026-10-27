# TU1.5 native island collision coordinator and allocation monitors

The new typed module implements English TU1.5 `321E50`, allocation monitor
begin/end `282B70`/`282B90`, and the closed allocated-byte summation `149B90`.

The summation reads the live secondary allocator at `486070`, then the default
allocator at `486074`. It admits only signed-positive record counts, reads size
before base, traverses the final word of each eight-byte record in reverse and
wraps arithmetic at 32 bits. Null/empty heaps do not eagerly read their data
storage descriptors.

The monitor publishes the current island before reading the allocation total.
On completion it clears that subject before recomputing the current total,
rereads the original baseline, and writes island byte `AE` followed by global
byte `56DFCC` only when the totals differ. Unchanged flags retain their incoming
bytes.

The coordinator calls the monitor, requires the actual `321700` first phase,
then rereads `island+20` and the resulting world's byte `23E`. That neutral flag
selects the actual `321A70` or `321D20` service, followed by the monitor end. The
flag's meaning is not inferred. All remaining phase services receive actual
island/dispatcher/configuration identities and numeric state; they do not
supply a guessed collision result. The implementation has no guest register
machine, instruction decoder or original-code fallback.

## Independent original proof

The physical oracle executes unchanged instructions from the English TU1.5 XBE,
SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The complete source listings contain 32/5/9/36 instruction sites for the four
owners. These are authenticated listing counts, not an all-path coverage claim.

GCC and Clang both passed:

- 190 physical-original cases: 107 complete summation/monitor calls, 35 real
  `321700` entry stops, 41 operand-read fault prefixes and seven integer-store
  fault prefixes.
- 1,668 ordered reads, 260 ordered stores, 7,495 original instructions and all
  retained banks; exact returned `149B90` totals and CW/SW/MXCSR were checked.
- Null and shared heap identities, zero/negative counts, wrapping sums and
  address arithmetic, unchanged versus changed totals, and aliases between
  monitor fields and allocator descriptors.
- A private mutation changing the allocation-record stride from eight bytes to
  four was rejected against the actual original return/read/store evidence on
  both compilers.

The physical observer received one narrowly recorded private change: it writes
its actual stopped GP register array to the report, allowing the original
summation return value to be checked. Its single-step loop and original
instructions were unchanged. The exact diff and hashes are retained.

Separate continuation models change the allocator and the island's world
pointer inside the first service, select both values of the reread neutral
flag, alter allocation totals in the selected phase, and propagate actual phase
numeric state. Missing owners retain preceding monitor publications. These are
explicit continuation models; they do not establish original execution of
`321700`, `321A70` or `321D20`.

The coordinator's remaining phase execution, a complete original coordinator
return after those phases, unmasked platform traps and live multiplayer
composition remain separate gates. No synthetic return advances the original
oracle, and no whole-frame raw-bank comparison was relaxed.

## Evidence and storage

Results, exact source/listing hashes, commands, bounded GCC/Clang build/test
logs and the negative control are in
`analysis/physics/native-island-collision-tu15-20261026-first/`.
The authenticated small listing is in
`analysis/physics/native-island-collision-tu15-20261026/original-listing.txt`.

`tools/run_tests.py` removed both private compiler builds. The enclosing
`tools/run_test.py` owner, containing the original ELF, observer, fixture and
private source, was verified removed. Immediately before that owner ended,
its remaining files occupied 30,535,680 allocated bytes (30,494,801 logical
bytes), including the 19,082,012-byte fixture. Those files were deleted; no
large artifact is retained. Bounded reports occupied 12,884 logical bytes
before the cleanup receipt was added. The receipt records removal checks.

Source pins for integration:

| Source | SHA-256 |
| --- | --- |
| `native_tu15_island_collision.cpp` | `1ac7b29fa439d1d1cc12b07f1a7100c4c115722fcae70d07396f3e619621c3c7` |
| `native_tu15_island_collision.hpp` | `54db13916e1c089a347c070b7c0d4eb526b028cbd24166ff955c80e7db1e8029` |
