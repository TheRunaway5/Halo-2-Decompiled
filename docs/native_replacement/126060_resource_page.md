# TU1.5 resource-page allocation —126060

The sole compatible generated owner in `src/physics/physics_live_support_tu15_generated.cpp`
now defaults to a structured native controller. The complete retained original body remains
available through `HALO2_REFERENCE_RESOURCE_PAGE`. The shared generator applies the same
whole-body and full-dispatcher authentication; it fails closed on unknown source shapes.

The typed kernel captures requested bytes and the page-cache owner before saving registers,
then acquires the actual mode policy after those saves. Genuine13FE50 computes page success
or failure. Success calls genuine16E770 to allocate the matching inactive datum, then writes
its pending byte, empty job, resource pointer and resource page handle in original order.
The adapter preserves actual child continuation GP state and restores saved words after
publication, including descriptor/output overlap with the saved SI/BX slots.

Implementation:

- `include/halo2/native_tu15_resource_page_kernel.hpp`: allocation/publication algorithm.
- `src/physics/runtime/native_tu15_resource_page.hpp`: retained caller ABI and phase transport.
- `tools/native_resource_page_replacements.py`: exact prefix, full-body SHA and dispatcher pins.
- `tools/validate_native_tu15_resource_page.py`: unchanged physical-i386 oracle and genuine inputs.
- `cmake/NativeTu15ResourcePage.cmake`: registered CMake gate.

The official GCC sanitizer and Release gates both passed156 native transfers and156
forced-reference transfers:96 complete returns (64 with saved-word/output aliases) and60
before-parent-read prefixes. All31 original parent sites execute. Each route compares236
real13FE50/16E770 entry packets, full GP/EFLAGS/all XMM, MXCSR/x87 control/status/live80-bit
values, complete banks, ordered reads/stores, and the final source cursor. Seven malformed
body/dispatcher/prefix controls and seven admission refusals are checked. No proof-source
drift was observed. No child outcomes are supplied.

The page allocator uses the authenticated original captured owner and banks:7889 total
pages, unchanged7FFFFFFF policy budgets and empty original span list. A4096-byte request
succeeds; a0x10000000-byte request fails the real oversized span search. Complete page calls
in this cohort use DF0; reverse-direction page fault behavior is outside normal-return
qualification. Parent read-prefix comparison does not qualify physical page-fault atomicity,
implicit stack-write faults or arbitrary observer exceptions.

Evidence (all bounded):

- `build/test-results/native-resource-page-official-20261109/report.json`: paired CMake result.
- Sanitizer `evidence/native-tu15-resource-page.json`:10,900 bytes, SHA256
  `b1c1edbca69aa3695ebc9786dd5cd4088766ce8bd2a1c5a35d2c49d0dba4c638`.
- Release `evidence/native-tu15-resource-page.json`:10,824 bytes, SHA256
  `1700c8042be5e721f85ef62c56b00719fbd8eccab32effbc4524e8d7d1367bee`.
- `build/test-results/native-resource-page-primary-generation-20261109/report.json`:
  fresh full primary generation, exact native prefix/idempotency and raw/specialized
  whole-translation-unit syntax checks passed.

The native parent does not establish native dependency closure for13FE50,16E770 or16E860.
Their real owners remain explicit migration work. This change is later than the frozen r7
engine/accounting cohort. Private build trees, generated units, ELF probes and raw banks
were removed; no large artifacts are retained.
