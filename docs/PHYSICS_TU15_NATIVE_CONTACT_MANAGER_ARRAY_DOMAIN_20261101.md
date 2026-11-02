# Native contact-manager owned array domain

The native manager and collector code is unchanged. Actual1467 manager10082380
has A4=0, two owned arrays of capacity8 (224B and256B), a borrowed byte array
80000008, and no base lock. Direct original3232B0 versus native comparison
passes GCC and Clang in
`analysis/physics/native-contact-manager-owned8-actual1467-tu15-20261101-qualified`.
All89,795,481 typed BEFORE bytes, actual FP,196 reads,83 stores and16 genuine
allocator entries are compared over actual flag1 and declared flags0/3.

The capacity-family aggregate is
`analysis/physics/native-contact-manager-array-family-tu15-20261101-qualified/aggregate.json`.
Nine pipelines per compiler pass993 reads,395 stores and110 genuine allocator
entries, with three semantic mutants rejected. Every57 normal31F660 site,
42 reserve sites and14 byte-copy sites executes. Both original and native start
from the SAME actual typed BEFORE, independently call their own2E31E0 reserve
owners, then destroy through3232B0. Neither pointer nor capacity outcomes from
one side enter the other. Original leaf transitions marshal ordinary formal
ABIs; they are test-driver control, not a physics interpreter or native CPU frame.

The controls independently reserve the28B,32B and1B arrays; reserve all three
with requested capacities9,33 and64; and genuinely allocate/free capacity0 for
the existing empty28B and32B arrays. The byte array starts with actual count5
and capacity8, so its larger owned buffers preserve all five current bytes.
Zero-size ownership comes from real reserve/allocation calls, not a fabricated
pointer or capacity. The full initial header/pipeline inputs are retained in
`family-input-provenance.json`. A first test-driver capacity/stride ABI error
is retained separately with its exact compressed validator; no kernel change or
byte waiver was used to correct it.

The generic release branch has no capacity-dependent control after the sign
ownership test: it masks7FFFFFFF, multiplies by28/32/1, reads current data and
current allocator dispatch, and invokes149E00. Its lower233420 pool service
already has independent source qualification for the small size-class domain.
A bounded production guard can therefore admit any owned logical capacity for
which `uint64(capacity & 7FFFFFFF) * stride <= 2000hex`. This excludes arithmetic
overflow and the unqualified large-free path. Source-valid headers have count
no greater than logical capacity; the destructor itself does not consume count.
Borrowed arrays remain skipped. The same size-zero lower path covers byte
capacity0; no byte-count5 input was illegally shrunk to zero.

Ownership provenance remains the current source149E00 route and strict native
233420 provider. Null data is an authenticated149E00 early return. Unknown,
default-heap and large-release owners must still refuse. If admission must
refuse before native stores, current primary-pool `[base+4340,base+F0340]` and
current virtual14=233420 are the authenticated source routing checks; use the
actual current pool, not a recorded address or a literal-capacity list. An
extent check can additionally require data+bytes to remain in that pool data
range. No new pointer, result, heap descriptor or allocation metadata is invented.

A4-positive world updates, collector base locks, zero-reference body destruction
and exception unwind remain separate open domains. This normal array closure
and its component evidence do not claim a full visible multiplayer pass.
All generated fixtures/builds belong to the test owners and were removed.
