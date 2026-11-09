# Halfword bitfield query (`FA5D0`)

The primary unit and its common generator now route the complete seven-site
TU1.5 query to `native_tu15_halfword_contains_bit`. The typed kernel reads the
live pointer argument and its two-byte bitfield. The index uses its low five
bits, so indices 16..31 do not wrap to a low halfword bit. A separate adapter
publishes the retained GP/flags state, preserves the upper EAX mask bits when
setting AL, and reads the real return slot before the four-byte argument cleanup.

The original 22 bytes have SHA-256
`5abfe12c262852ef3accf49511290db90c5c30ead56d3e145d88fe030272b395`.
Whole source-body authentication retains
`caa1c7398c487d028f00ed11ed4a20a15937a234e7b424a4ea15406150796346`.
Only the exact native prefix and ordered shift expression normalize back to
that body; six mutations of bound/unbound bodies are rejected. The specializer
records `native_halfword_bit_query_entries` separately and preserves its prior
entry groups. Fresh primary dispatcher authentication still requires the
original full dispatcher and exact extra `FA5D0` case/body identity.

Hardware comparison exposed a pre-existing retained `Flags::shift` AF-context
mismatch before the first read. Both the ABI publisher and explicit retained
continuation now seed the real shift's incoming arithmetic flags using
SAHF/LAHF. This preserves count-zero behavior and undefined flag context without
writing into a compiler-owned red zone. No floating-point instructions are added.

GCC sanitizer, Release and Clang each pass 882 physical cases: 786 complete
returns and 96 stopped architectural reads, all seven original sites. Direct,
bound and fully specialized variants each match complete GP/full flags, all
XMM lanes, FP controls/status, zero/one/four/eight live 80-bit inputs, ordered
read addresses/widths and unchanged banks. Each native/reference mode records
2,646 comparisons and 7,650 full pre-I/O packets; the native mode records 2,646
admissions with zero retained continuations. Six unsafe protocol/image cases
refuse before accessing memory. Independent source review is clean.

The official five-test integration gate passes; after making the header
self-contained, both hardware modes and Clang pass again. Complete fresh
primary raw/specialized generation and Clang syntax pass with all 22 existing
native entry groups preserved. Bounded reports, hashes and cleanup receipts are
under `build/test-results/native-halfword-bit-query-20261109/` and
`build/test-results/native-halfword-bit-query-primary-generation-final-20261109/`.
Completed owned trees were removed; no large artifacts were retained.

Whole-runtime coverage, physical page-fault atomicity and the conditional legacy
image routes remain separate gates. Non-x86/MSVC ABI admission is refused.
This port is newer than the r7 frozen engine checkpoint and the selected launcher
runtime; it does not establish native-only whole-game execution or frame parity.
