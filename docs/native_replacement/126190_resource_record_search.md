# 126190 resource-record search

The primary live-support and AI-frame TU1.5 copies now default to a shared
typed native search. A resource with an existing handle returns `FFFFFFFF`.
Otherwise the search returns the generation/index datum of the occupied pool
record whose owner pointer matches the resource. This leaf has no callees.

The kernel uses the existing bitmap-slot primitive one candidate at a time.
It preserves repeated bitmap-word reads, fresh bounds while skipping empty
slots, and descriptor recapture after occupied nonmatches. Record address
arithmetic wraps at 32 bits, and the owner word is read before the generation
halfword. The adapter retains the before-PUSH handle read, pool capture between
the DI and BP pushes, live saved-register pops, escaped ECX/EDX, complete final
flags and the two distinct RET-site cursors.

Both complete original bodies remain available through
`HALO2_REFERENCE_RESOURCE_RECORD_SEARCH`. The binder checks complete body SHA
pins and authenticates the complete selected-operand resolver for the AI copy.
It rejects altered bodies and altered resolver semantics. Service/trace
admission preserves the original route when callbacks within this leaf are
required.

The registered `original_tu15_native_resource_record_search` gate passed under
ASan/UBSan: 200 unchanged original i386 cases compared against both production
copies, 400 complete comparisons and all 47 original instruction sites. All
returned GP/EFLAGS/XMM, MXCSR, x87 control/status/live 80-bit values, four full
banks, ordered stores and exact source RET cursor agreed across 34,501,600
bytes. Cases cover signed bounds, empty/sparse bitmaps, positions across word
boundaries, zero/negative/overlapping record strides, generations, all 16 caller
alignments and ten stack/global/descriptor alias arrangements. Twelve further
canonical cases compare exact ordered reads/stores and backing mutations. Ten
body/resolver mutations and six unsafe/reference admission cases are checked.
An optimized non-sanitized hardware gate also passed before binding.

Final receipt:
`build/test-results/native-resource-record-search-official-20261109/evidence/native-tu15-resource-record-search.json`
(35,042 bytes, SHA-256
`a91e6982ce97b4072700615078efc0b1f210a869ce0b5cee6418375c52b57e16`).
Fresh complete primary and AI-frame generation plus full C++ syntax checks
passed. The AI replay authenticated two archived required reference banks in
scratch and immediately removed the temporary reconstructions. Original
references were unchanged. All probe, CTest, configure and generator scratch
trees were removed; no large artifacts were retained.

This qualifies complete normal returns in the admitted TU1.5 domain. Partial
GP/flag publication on read faults, observer exceptions, physical page faults,
other image variants, the parent resource lifecycle and whole-engine execution
closure remain separate obligations. The r4 engine accounting checkpoint was
frozen before this binding and does not exercise this change.
