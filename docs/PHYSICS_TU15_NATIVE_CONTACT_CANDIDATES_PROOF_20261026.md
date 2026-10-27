# Native TU1.5 contact candidate update

`native_tu15_update_contact_candidates` owns the full 575-site English TU1.5
`2F36F0` algorithm. Its typed inputs are the producer, two payloads, context,
factory identity, retained aligned data packets and native FP context. The
kernel contains no instruction decoder, GP/XMM execution file, virtual Xbox
stack or supplied contact result.

The algorithm composes the independently qualified native inverse-affine
owner `2E4550`, computes the source bounds margin with native x87 operations,
checks cached bounds with native SSE and updates the producer's bounds. It
collects child keys through the real shape service. The unsorted path removes
and shifts obsolete entries, then inserts missing keys; the sorted path merges
the old entries and sorted keys into temporary backing. Both retain the
opaque word at entry offset 4 rather than inventing an initialization value.
The source ownership stages arm only after their actual preceding reads and
disarm before the final explicit release.

The [source review](../analysis/physics/native-contact-candidate-source-review-tu15-20261026/report.json)
reauthenticates all 575 original instruction bytes against XBE SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Its fourteen findings cover cached post-allocation counts, the later insertion
base read, cached merge keys, removal scans, ownership stages, filter operand
order and physical callback context. Semantic branch and release-phase enums
distinguish those service phases without exposing an emulated register file.

The historical 42-case growth cohort remains a bounded completed-call result
in `build/test-results/native-contact-candidates-growth-tu15`. Its original
source hashes and results are preserved. The corrected implementation is being
qualified separately with a stronger observer:

- An independently executing original parent supplies the observed service
  arguments, consumed ECX/result values, actual return boundaries and FP state.
  Those observations are compared after execution; they never feed the native
  implementation.
- The original parent and composed native inverse owner have their ordered
  retained-data reads and writes compared. The expanded observer also records
  the bytes at those boundaries, catching transient differences that a final
  bank comparison would miss.
- A separate uninterrupted original parent must match the instrumented
  original's persistent captured bytes and FP state.
- Native candidate, inverse, dispatcher `2F3140`, reserve `2E31E0` and forward
  byte-copy `2E2E30` owners compose in the same host process. Remaining class,
  selected factory and allocator callbacks execute genuine unchanged original
  bodies at their authentic nesting depths for the offline reference proof.

The explicit operand contract includes external storage and retained packet
ranges `2C..37`, `40..73`, `80..38B` and `390..3CF`. Source control locals,
argument/return cells, saved registers and FS registration are outside this
typed data contract. The original strict persistent-bank comparison retains
its existing returned-control-stack contract; no additional heap range or
physics field is excluded.

The [final composed cohort](../analysis/physics/native-contact-candidates-composed-tu15-20261026-qualified/aggregate.json)
passes 42 physical cases with both GCC16 and Clang23. Each compiler checks
3,795,642,138 persistent bytes, 33,227 actual parent/composed instructions and
8,883 retained read/write events, including each event's bytes. The independently
captured 1,044 service calls cover 160 creations, 14 releases and 43 native
reserves. FP state matches throughout; traced and uninterrupted original
persistent state have zero differences. Wrong relative-transform order, filter
key and opaque-entry initialization controls are rejected.

Cache-enabled paths, broader alias/fault/unwind qualification, actual production
caller binding and visible native multiplayer remain separate gates. This
document does not admit the producer's opaque constructor bytes, relax any
strict raw gate or establish Xbox hardware numerical identity.

The [later helper composition](../analysis/physics/native-contact-candidates-helpers-composed-tu15-20261026-qualified/aggregate.json)
also closes key sort and entry-buffer construction/destruction in the native
host. It passes the same 42 cases and three controls with both compilers,
checking 38,630 actual instructions and 9,915 ordered events per compiler. Its
temporary owner contract includes the full `2C..3F` range, adding the original
backing/count fields to the earlier 12-byte array-header checks. Remaining
original service calls fall from 1,044 to 984; all persistent bytes, FP and plain
original controls remain exact. Native cleanup actions have their separate
[physical qualification](../analysis/physics/native-contact-candidate-unwind-tu15-20261026-first/report.json).

All compiler and original-reference products use owned `run_test.py` and
`run_tests.py` scratch. Reports, commands, hashes, bounded diagnostics and
cleanup receipts remain; no large artifact from these focused tests is retained.
The final cohort owner and both compiler build trees are absent, removing
857,620,480 measured allocated bytes from the outer owner. That measurement
excludes the nested CMake products already removed by their own runners.
