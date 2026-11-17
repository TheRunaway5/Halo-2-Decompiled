# TU1.5 296620 scale interpolation

ModelRender owns the complete twenty-one-site scalar interpolation. The typed
kernel preserves the first and second stack spills, reloads the first sample
before the late output pointer, and reads the factor only after publishing the
subtraction in XMM1. The factor read and multiply retire together. The result
goes to output+1C before the stack adjustment and actual RET read.

The current phased hardware proof passed 562 unchanged-original paths under
both sanitizer compilers: 192 dense inputs, all 256 opaque MMX masks and 114
read, write and completed-observer prefixes. Each compiler passed 2,248 raw
comparisons, 32,776 complete pre-I/O packets and four compiled defects. It also
compared 448 retained-view complete returns directly with the original ABI,
ordered reads and writes, and final banks. Source pre-I/O flag publication is
checked separately against the unchanged retained body.

That direct retained-body comparison passed 856 cases per compiler, including
576 late factor-read SIMD feedback cases and 88 read/preimage/write/observer
prefixes. The first fixture had an ambiguous test-only State alias; the failed
compile is retained separately. The repaired comparison uses the complete
unchanged ModelRender body and resolver.

The private owner/binder authenticates four current/fresh raw/prepared
body/resolver/dispatcher/LiveState tuples, fifteen adjacent retirement labels,
an exact global preamble include, and explicit raw/prepared tags. Whole-unit
candidate syntax and strict mutations passed. The frozen tagged source proof
passed 11,040 comparisons, 116 refusals and seven compiled defects per
sanitizer compiler. An additional control rejected early factor handoff in
the actual late policy change under both compilers. The separate full Clang
matrix passed 140,832 comparisons; its own-reference route executed 948,480
retained statements and zero native calls. All owned source, builds and banks
were removed. The exact ModelRender default, owning generation hooks and
required-binding CMake checks are installed. The public facade is unchanged.

Final installed acceptance passed all three registered tests, plus the
permanent selector, catalog and copied-helper checks. The original raw and
retained-view comparisons passed under GCC and Clang, with four compiled raw
defects rejected per compiler. The installed source gate passed 11,040 GCC
and 140,832 full-input Clang comparisons across four genuine contexts. Its
Clang own-reference mode executed 948,480 retained statements and zero native
calls. The combined installed manifest is
`b43ccc823220bdfa717b0a7a5f34a8be702922d1c026b87b46fc6890ca46b22b`.
The source copy, inner CMake tree and all binaries and banks were removed.

Raw callbacks use the declared readonly GP/XMM/policy domain with isolated
incidental host FP effects. The retained view checks actual late SIMD feedback
with readonly source FP metadata and host controls/status. These proofs do not
qualify signals, MMIO, physical partial stores, arbitrary FP mutation, other
images or engine runtime.
