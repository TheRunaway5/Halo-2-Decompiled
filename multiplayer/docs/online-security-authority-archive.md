# Rust authority-archive decoding

S03/S05/S11/S12 implementation milestone, 2026-10-22. These full security items
remain open. This extends the [security-first goal](online-security-first-goal.md)
from native requests to the complete named authority-archive grammar.

## Actual boundary

[decode_source_authority](../../src/source_authority_codec_decode.cpp) calls
the mandatory [Rust archive parser](../../rust/online_protocol/src/authority_archive.rs)
through the [owned CXX adapter](../../src/authority_archive_protocol.cpp).
Rust checks the envelope, declared extents, exact compressed-stream consumption
and zlib Adler checksum, decompresses with pinned `miniz_oxide` 0.9.1, and parses
the complete named tree. The original C++ byte reader and this decoder's native
zlib decompression call are removed. There is no rejected-packet fallback.
The existing C++ encoder and its wire format remain unchanged.

Rust handles all fifteen tags: booleans, signed/unsigned numbers, float/double
bit patterns, binary strings/blobs, dynamic/fixed sequences, maps/sets,
optionals, null pointers, records and enumerations. Counts, names, extents and
complete consumption are checked before the tree is returned. Non-null process
pointer records fail. Float bits and binary strings are preserved rather than
normalized. A flat node arena and explicit work stack avoid recursive parsing
and recursive destruction of hostile nesting.

The [C++ materializer](../../src/source_authority_tree_reader.hpp) consumes typed
nodes, never a second unchecked wire reader. It retains expected native field
names/order, types, fixed extents, narrowing/enum admission, duplicate key
checks and the original private-owner shape validators. Moving this remaining
expected-schema admission into Rust is still S05 work. The existing vetted
SHA-256 consumer compares the raw backing with the owned expected digest before
constructing a checkpoint. A checksum is consistency evidence, not peer authority.
`source_only`, source-version policy and real native prepare/commit remain in
their existing owners; parsing success supplies no native-ready or callback
permission.

The opaque Rust owner retains the immutable decompressed backing and flat nodes.
The adapter's byte views are borrowed only while that owner lives, for immediate
hashing or copying a typed string/blob into its native value. No packet buffer
or borrowed Rust payload is published as gameplay state. Rust unwinding stops
at the parse bridge; OOM/process failure is a separate containment concern.

## Reservation limits and remaining limits

The per-decode reservation ceiling derives from the caller's encoded, decoded
and native-allocation limits plus its declared fixed native/Rust owners. The
parser accounts for the actual input extent, raw vector capacity, node/work-stack
capacities and measured `sizeof` decompressor state. Vector growth reserves both
old and new backing during reallocation. Native dynamic claims retain their
original allocation/element limits and share the working reservation counter
through a Rust mutex; failed claims do not publish partially accepted state.

This adds a temporary-storage admission policy: a frame near the previous
individual maxima can exhaust combined headroom. Genuine complete-world
compatibility must be measured, without dropping fields to obtain a pass.
This is an accounted logical reservation bound, not an exact RSS ceiling.
Allocator internals, implicit default-owner allocations, other queued jobs,
retained bases/current worlds, staging and later native import/replay still
require the whole-session lifetime budget and containment work in S05/S10/S11.
Early expected-schema rejection, cancellation, scheduler/work budgets and
coverage-guided fuzzing remain open.

## Evidence and scope

The frozen [normal bridge qualification](../../.local/online-security-archive-20261022/frozen-bridge-retry2/qualification.json)
and [C++ ASan/UBSan bridge qualification](../../.local/online-security-archive-20261022/frozen-sanitized-retry1/qualification.json)
start/pass all three selected CTests without skips. The archive control covers
44,719 independent grammar/value comparisons, packet-buffer retirement, exact
scalar bits, 100,000-level nesting with flat teardown and synchronized native
reservation exhaustion. Existing voice/native-request comparisons also pass.
The frozen parser crate passes all eighteen Rust tests and strict Clippy; the
count includes concurrent EOS wire tests, not live EOS SDK acceptance. Five
archive-specific Rust groups cover ownership, containers, deep trees,
corruption/expansion and concurrent reservations. Deterministic comparisons are
not a coverage-guided fuzz campaign or proof about unsafe native consumers.

The [native qualification](../../.local/online-security-archive-20261022/native-normal/qualification.json)
is still running at this document's initial write. It builds the real codec and
selects the complete current schema regression plus an available separate-world
TU1.5 source reference. Those original assets are offline reference inputs;
their use does not accept an MCC-only runtime, visible gameplay, physical
devices, WAN play or seamless migration.

[The validator](../../tools/validate_online_protocol_security.py) freezes real
code under the caller's owned scratch tree, runs the official CMake test runner,
checks that the compiled copies remain unchanged and retains changed copied
inputs as explicit reference preimages. Large original assets are borrowed.
Temporary source copies, compiled objects, libraries and test binaries are
removed by the enclosing `tools/run_test.py` owner. Reports retain actual input
hashes and distinguish later worktree edits from their compiled cohort.

Initial bridge attempts failed on a header introduced by concurrent EOS work;
the generated-header/private include dependency was corrected. An early
partial-source freeze missed the newly included SDK CMake module; validation
now freezes the complete code cohort. Strict Clippy also required replacing
manual saturating arithmetic with its equivalent standard operation. Failure
logs remain beside the successful results. Shared SDK callbacks, relay policy,
credentials and live transport behavior are outside this archive qualification
and still require security inventory/review before combined release acceptance.
No installed runtime, backend deployment or migration activation follows.
