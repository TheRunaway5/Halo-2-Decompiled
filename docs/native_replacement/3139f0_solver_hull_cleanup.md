# Actual 3139F0 solver hull cleanup

Primary's 56-site cleanup body has SHA-256
`c206d983eb1e95187a2b77908ecdc822fed57da3341ad38bbe8462f39846a15c`.
The private typed controller publishes the original FS-backed exception
record, checks the live capacity of the three buffers with 32-, 80- and
4-byte records, and restores the saved exception chain. It preserves the
byte store between the first two release branches, the caller's evolving
registers, and the original target-read, return-word and virtual-call
sequence.

The three `+0x14` targets are release methods of the allocator reached
through `0x48C938`. The original static object `0x486080` points at
`0x45F4CC`, whose `+0x14` word is `0x149E00`. The table's separate
`+0x28` entry `0x2E1110` is not interchangeable. This identifies only the
original static table used by the fixture. The controller does not force
that table or supply a release result to unknown providers.

The GCC and Clang sanitizer comparisons each match 64 complete retained-source
cases, all 56 parent sites, 1,896 parent pre-I/O packets, 88 genuine
`149E00` null-data calls, 28 post-release capacity changes and 616 stopped
provider/observer prefixes. A modified live capacity after the first real
call changes whether the next buffer is released. The fixture compiles
the exact genuine allocator body with actual observation addresses.
It does not exercise non-null allocator release paths or physical
page-fault publication.

The separate physical adapter passed GCC and Clang sanitizers against
32 unchanged i386 calls each: all 56 parent sites, 976 full parent I/O
packets, 48 real `149E00` entries, 448 ordered parent reads and 624
stores. The owned bridge installs an actual i386 FS descriptor pointing
to the declared bank, and augments only the three original FS operands
in observation metadata. It does not modify the image instructions.
The original rejects all three compiled controls: incorrect stride,
widened byte store, and late capture of the saved exception-chain word.
Receipts are under `build/test-results/native-solver-hull-cleanup-frame-20261110/`;
all recorded owners are removed. The null-data child path and parent
publication are qualified separately from non-null release, source
default, other images and hardware fault handling. The route is unbound.

The immutable registered cohort passes 3/3: physical Frame, retained
Source and the real CTest selector. It uses manifest
`8fb6fe1ed6c2833ff85d3b942044d1ff7949ecdeabe3905c8437cba7c1f26310`.
The first wrapper refused an already-present older private Frame in its
donor; the accepted retry records its exact before and after hashes rather
than treating the file as new. Both owners were removed. The bounded
result is `build/test-results/native-solver-hull-cleanup-private-official-r2-20261110/report.json`.
