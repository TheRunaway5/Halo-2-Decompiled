# Actual 314ED0 solver hull edge insertion

The complete 48-site Primary body has SHA-256
`d231619017a5ca841fc1db48b2b04b4caecb30f28bca868111fe751a6701abe3`.
The typed owner writes the seed edge, calls actual `3149B0` to obtain
facet slots and actual `314CB0` to link the ring, and rereads the live
temporary storage descriptor before considering the virtual release.
The private continuation reads the virtual target before its return
store and adopts the child's returned frame and registers.

GCC and Clang sanitizers each pass 40 unchanged original calls using
inline temporary storage, covering 39 of 48 sites. Each comparison
includes 5,057 full parent/child packets, 2,794 ordered architectural
reads and 2,223 original stores. Each compiler sees 40 actual slot
calls and 40 actual ring calls, including 28 nested plane calls and 40
nested clears. Hostile provider equivalents match the ordinary-backed
comparison. Both compilers reject an incorrect seed edge stride and
omission of the seed vector write. No child result is supplied.

Reports: `build/test-results/native-solver-hull-edge-insert-frame-20261110/{clang,gcc}-r3.json`.
Both owned trees were removed. The original recorder's first run did
not sample the ring's pure entry instruction, so its count disagreed;
the correction adds the entry packet on both sides and leaves the
kernel unchanged.

The separate dynamic-release proof executes all nine sites from
`314F4F` through the real indirect call at `314F67`. The actual Clear
child aliases the private capacity's high byte or whole word, so the
same original code reaches the archived allocator instance and its
authenticated vtable slot. It arrives at `149E00` with the actual
return word, inline-buffer alias, zero or 400 bytes, and memory class
`0x12`. Per sanitizer compiler, six original prefixes match 447 full
packets, 240 reads and 207 stores; hostile provider equivalents match.
See `build/test-results/native-solver-hull-edge-insert-release-20261110/{clang,gcc}-r1.json`.
The two owned trees were removed.

The dynamic proof stops at the genuine allocator entry before its body.
It does not establish a backed dynamic allocation, a completed release,
the returned child state, retained-source fault semantics, or a
production default.
