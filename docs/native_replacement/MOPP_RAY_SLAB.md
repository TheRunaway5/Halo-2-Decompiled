# TU1.5 MOPP ray slab stage

The private native slab stage implements opcodes `26..2B` from actual
`30B17D..30B323` inside `30AA70`. The shared typed DATA kernel owns encoded
plane construction, direction tests and both segment clips. Its raw view owns
caller storage, register/flags/SIMD publication and memory transaction boundaries.
The full ray owner has no production binding from this component.

`native_mopp_ray_caller_detail::slab(state,before,scope,opcode,program)` returns
true for the source's rejection path, after discarding both plane values. On a
continuing path it consumes both plane values and advances `program`. Its caller
owns the real return epilogue or next-opcode acquisition. The composed fixture
uses the real private `traverse` controller; unprovided leaf/diagnostic owners
throw and never supply outcomes.

The wide decode reads upper-middle byte 5 before publishing the lower integer
scratch, then reads upper bytes 4 and 6 after the authored lower-plane work.
Cache scale, variable scale, translation and caller scratch remain live reads.
Both full endpoint vectors are cached once, while scalar scratch reads retain
their authored positions. The two interpolation phases keep their distinct
constant-load and shift/arithmetic order, including SIMD operand ownership.

Every FSTP uses the existing RawScope staged spill: publish pre-conversion FP
state before I/O, commit the converted bits and post-spill FP image only after
provider write success, then invoke a completed-store observer. The unchanged
hardware oracle independently records real FTW/TOP and logical 80-bit values.

The expanded proof in
`build/test-results/native-mopp-ray-slab-final-20261109/report.json` passes
GCC and Clang ASan/UBSan: 462 complete slab paths (158 real rejection returns),
90 actual MOVAPS alignment faults, and 1,374 physical retained prefixes.
Each compiler performs 9,552 direct/composed, ordinary/hostile-provider
comparisons. All 124 slab source instructions are exercised; 53,739 complete
original pre-I/O GP/flags/XMM/FP/live80 packets are compared. Ordered reads,
stores, observer preimages and every declared bank are checked.

Four independently compiled mutations are rejected: premature spill commit,
unprotected provider FP state, a hoisted upper byte read, and reversed SIMD
ADD operands with distinct NaN payloads. The original instruction image and
expected outcomes remain unchanged in these controls.

The registered paired CMake gate also passes sanitizer and release modes
(`native-mopp-ray-slab-official-20261109/cmake/report.json`, 2/2 tests, no
skips). It archives committed dependency pins into disposable copies; both
retained proofs preserve exact before/after source hashes. Its owner was removed.

The qualified input domain is ordinary backed source memory, masked FP controls,
dense incoming x87 stacks of 0, 1, 4 or 5 values, supported precision/rounding
modes, and callbacks that preserve DATA/policy/State while freely clobbering host
FP state or throwing at the declared transaction phase. Five incoming values
leave room for the source's three live temporaries. Source active MMX, sparse
x87 tags, unmasked exceptions, stack overflow, asynchronous delivery and arbitrary
callback mutation are excluded. MOVAPS uses an independently measured fault
image with RF set, distinct from unchanged execution flags. The existing outer
runtime host condition-bit restoration limitation is separate; RawScope carries
the complete source status through State.save.

This proof does not establish physical page-split store atomicity, returned leaf
service behavior, complete ray owner admission, live gameplay, or performance.
The expanded owned scratch was removed; no large artifacts were retained.
