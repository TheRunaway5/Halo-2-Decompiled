# Native 1E3270 descriptor/object lookup

The sole compatible production copy, in physics live support, now defaults to
`src/physics/runtime/native_tu15_record_lookup.hpp`. Its typed lookup reads the
signed descriptor count, resolves a 32-byte slot, and decodes a non-NONE datum's
low 16 bits into the global 16-byte object table. Signed negative indices and
wrapping 32-bit addresses retain the source behavior. The ABI adapter preserves
the actual saved-SI stack word, escaped ECX, EDX ownership and return cursor.
All descriptor/global reads occur after the source PUSH at their original
boundaries. There are no child functions or supplied service outcomes.

`tools/native_record_lookup_replacements.py` authenticates the complete literal
body with SHA-256
`d4e2cbc2eb377e47991d6f16dbe2bf1a43c0d3b5d90ae24cfbd4e399dad47f34`.
The shared generator applies the same conditional binding. The reference switch
is `HALO2_REFERENCE_RECORD_LOOKUP`; incompatible image, trace, real service-site
and observer conditions retain the reference body. Native ABI flag transport
currently admits GNU-compatible x86 backends; other platforms remain separate.

The first found-case hardware comparison exposed a limitation in the old
canonical `Flags::shift`: it forces undefined AF to zero, while the unchanged
original and native CPU return AF set for this host. The adapter now executes
the original CMP ECX,-1 / AND ECX,FFFF / SHL ECX,4 flag producers in one native
asm block and captures LAHF/SETO. C++ memory/address operations cannot seed those
flags. Every EFLAGS bit is compared against unchanged original execution; none
is masked. The secondary canonical alias checks compare GP/FP/read/store state
and deliberately do not qualify canonical EFLAGS.

Both optimized Release and ASan/UBSan/LSan direct gates passed 168 physical
original cases, each comparing 6,226,752 exact bytes. All 17/17 original source
sites execute. Cases cover signed and wrapping bounds, absent/salted/overflow
datums, all sixteen caller alignments, unaligned inputs, descriptor/count/global
and saved-SI aliases, live 80-bit callers, varying FP precision/rounding/TOP and
MXCSR. Eight canonical-reference supplements compare exact ordered reads and
stores under feedback. Four complete-body mutations are rejected and six
admission/ref-mode checks retain their original route. The native path emits
zero translated instruction callbacks.

The official registered CMake sanitizer gate passed at
`build/test-results/native-record-lookup-official-20261109`, retaining
`evidence/native-tu15-record-lookup.json`. Fresh primary generation and full-unit
syntax also passed at `build/test-results/native-record-lookup-generation-20261109`.
Later default-pose bindings are outside that generation receipt; their owner
reruns its own fresh generators. The original English executable is read only.
Private ELF/observer/probe files, raw banks and build trees were removed.

This qualifies the bounded normal-return function. Arbitrary read faults and
partial ABI, native-only production closure, live frame parity, performance
and launcher delivery remain unqualified. The larger 1E5C80 parent still has
real 1E5AF0/126200 lifecycle owners; this leaf does not supply their outcomes.
