# Native perception group clone, TU1.5

The Armory's real `ai_place training_done_tram` path reaches `273290`, which
calls `274570` at `2734E6`. The new native owner snapshots the source group
before allocating another group. It reuses the existing semantic native
`274200` allocator (including real pressure selection and retirement),
`274020` initializer, and their existing native children. It then performs
the source-ordered `2744E0` payload copy. No allocated datum, child outcome,
or actor state is supplied by a fixture or runtime callback.

`274570` takes the source group record in EAX and destination clump record in
EBX, returns the allocated datum or NONE in EAX, and consumes only its return
address. `2744E0` takes source EAX and destination EDX. Both require the
engine's forward string direction; a reverse-direction ABI refuses before
memory operations. The snapshot includes all 0xC4 source bytes. The payload
copy preserves the WORD/DWORD/byte widths, 27 point DWORDs, unaligned fields,
and the distinct byte at +31 after +35.

The unchanged original English TU1.5 executable is pinned by SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The 714-case physical original and native comparison reaches all 38 original
`274570` sites and all 49 `2744E0` sites. Cases cover allocation refusal,
pressure and real retirement, all initializer body branches, source-slot
reuse, eight overlapping/disjoint copy layouts, four stack alignments, and six FP
profiles. A second physical execution produced an identical reference.
All eight deliberate mutations were rejected. ASan, UBSan and LSan passed
with the clone owner and driver instrumented; existing child libraries were
not reinstrumented in that run.

All retained banks and 83,598 ordered stores match. The new copy owns exact
PC tags for 26,496 stores. The other 57,102 stores come from existing typed
native children; their address, width, before/after bytes and order remain
exact, while PC attribution is limited to an explicit original child-site
allowlist. There are 5,094 result/saved-register/FP assertions; the direct copy
also checks its volatile GP outputs. Dead child registers, private child CPU
frames, arbitrary fault continuation and ordered reads are outside this
cohort. Existing unknown multiplayer/alliance or behavior callback boundaries
remain the responsibility of their declared owners.

The retained reference is
`tests/fixtures/native_campaign_ai_perception_group_clone_tu15.json.gz`
(9,031,241 bytes, SHA-256
`a1d11f40193c03c32ffde87394d0a349a74c2ebcc7a26c20a950066d8c0651ea`).
It is an original reference input, not a runtime dependency. Reports,
original disassembly, review, controls and cleanup receipts are under
`build/test-results/armory-native-progress-20261113/placement-helper/`.
The candidate reference was moved into its canonical path without retaining
a duplicate. Executables, raw banks, expanded JSON and sanitizer objects use
`tools/run_test.py` scratch. The engine build is owned independently by the
root integration task and is not a retained artifact of this helper.
