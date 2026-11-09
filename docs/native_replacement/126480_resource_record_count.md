# 126480 inactive-resource count

The sole primary TU1.5 copy defaults to a typed native traversal of the pool
at `4F0418`. It counts occupied records whose activity byte at `record+4` is
zero. This leaf has no callees.

The kernel shares the bitmap-slot primitive. Empty slots retain the current
bitmap pointer and bound; an occupied candidate starts a new descriptor scan.
Stride and records-base reads retain their source order. Record addresses wrap
at 32 bits, and a null address returns before reading the activity byte. The
adapter preserves pool capture after the saved BX push, four real saved-word
stores/live pops, escaped ECX/EDX, null-address ADD flags, bound CMP flags and
the original RET cursor. The complete translated body remains available through
`HALO2_REFERENCE_RESOURCE_RECORD_COUNT`, guarded by a whole-body SHA binder and
the existing service/trace admission protocol.

The registered `original_tu15_native_resource_record_count` CTest passed with
ASan/UBSan: 190 unchanged original i386 cases, all 42 source sites and 16,393,200
exact comparison bytes. Complete GP/EFLAGS/XMM, MXCSR, x87 control/status/live
80-bit values, four full banks, ordered stores and the exact RET cursor agreed.
Cases include signed bounds, empty/dense/sparse bitmaps, word boundaries,
zero/negative/overlapping strides, null-address carry/overflow, all 16 caller
alignments and ten stack/global/descriptor/activity aliases. Twelve canonical
cases additionally compare ordered reads and backing mutations; four body
mutations and six admission/reference cases are checked. An optimized hardware
gate and fresh complete primary generation/full C++ syntax also passed.

Receipt:
`build/test-results/native-resource-record-count-official-20261109/evidence/native-tu15-resource-record-count.json`.
All probe, CTest, configure and generator scratch trees were removed. No large
artifacts were retained, and original evidence was unchanged.

This qualifies normal returns in the admitted TU1.5 domain. Read-fault partial
GP publication, observer exceptions, physical page faults, other image
variants, parent lifecycle closure and whole-engine execution remain separate.
The r4 engine checkpoint predates this binding.
