# Process-heap SEH and complete copy correspondence

This supplements the [reserve/commit research](SHIELD_PROCESS_HEAP_RESERVE_COMMIT_RESEARCH.md) for complete original constructor lowering. The earlier688-row constructor/segment report remains unchanged. No implementation, tests, original execution, platform state or resource outcomes were supplied.

| TU1.1 | Owned u5 | Complete instructions/image |
| --- | --- | --- |
|326B74 SEH prologue|32A6E4|19|
|326BAF SEH epilogue|32A71F|9|
|326E20 overlap-safe copy|32A990|231, exactly672 instruction bytes|
|2D9803 descriptor return|2DD2B3|10|

The [supplemental complete report](../analysis/damage-shields-goal-20261011/overlay-pass/registration/process-heap-seh-copy/complete-report.json) contains the three small owners in `supplemental_pairs[]` and the fully closed copy owner in `copy_entry`. All pairs originate from fresh unique matching of both original images; instruction/internal branch/table target mappings are verified without an assumed address delta. Complete report SHA-256`fb79138067717c85ae7013fe48b69f84f2a36efaae717be2feea5537a71ad136`.

## Copy closure required by the generator

The generic tracer initially retained136 copy instructions. That subset omitted95 instructions reached through alignment and negative-index dispatch. Its [initial report](../analysis/damage-shields-goal-20261011/overlay-pass/registration/process-heap-seh-copy/paired/report.json) is retained as navigation evidence; it must **not** be used as the complete326E20 body. The new [closure script](../analysis/damage-shields-goal-20261011/overlay-pass/registration/process-heap-seh-copy/close-copy.py) pairs operand bases through the uniquely matched original instruction rows, reads actual entries in both images and walks every resulting branch. The closed graph is231 instructions/672 bytes, matching the independently recovered function's672-byte instruction extent. Complete listings are [TU1.1](../analysis/damage-shields-goal-20261011/overlay-pass/registration/process-heap-seh-copy/complete-copy/TU11.asm) and [u5](../analysis/damage-shields-goal-20261011/overlay-pass/registration/process-heap-seh-copy/complete-copy/u5.asm).

| Actual indexed-jump operand baseTU1.1/u5 | Effective indices | Source route |
| --- | --- | --- |
|326F6C /32AADC|0..3|Forward remaining bytes.|
|326E80 /32A9F0|1..3|Forward destination alignment.|
|326F7C /32AAEC|−4..−1|Unsigned size−4 underflow; effective addresses alias326F6C..326F78.|
|326F00 /32AA70|0..7|Forward short DWORD copy.|
|327108 /32AC78|0..3|Backward remaining bytes.|
|3270B8 /32AC28|−7..0|NEG ECX before indexed jump; table begins at32709C/u532AC0C.|
|32700C /32AB7C|1..3|Backward destination alignment.|

The alignment dispatches occur after nonzero destination-lowbits branches, then AND EAX,3; index0 is excluded. Its raw slot bytes are `6f320090`/`71320090` inTU and`aa320090`/`ac320090` inu5, not admitted code pointers. The report retains these excluded bytes explicitly instead of treating them as targets. `copy_entry.table_dispatches[]` includes every operand base, signed effective index, raw field bytes and target address; `branches[]` includes all30 direct/indirect branch instructions, and `complete_instruction_address_map[]` maps all231 code addresses. [Exact table/closure evidence](../analysis/damage-shields-goal-20261011/overlay-pass/registration/process-heap-seh-copy/complete-report.json).

The copy routine is a three-stack-argument `(destination,source,count)` owner, plainRET, EAX destination. It selects backward copying only for the original overlapping-address comparison, uses STD/REP MOVSD/CLD on the long backward path, and preserves its exact arithmetic/flags/ESI/EDI/frame operations. Lowering must retain these instructions and original dispatch rather than replacing them with an unqualified copy callback. Its forward REP route relies on the caller's direction-flag domain; this static correspondence supplies no current flag state. [Complete original body](../analysis/damage-shields-goal-20261011/overlay-pass/registration/process-heap-seh-copy/complete-copy/TU11.asm).

## SEH and descriptor boundary

`326B74` is a frame-building helper, not a normal stack-cleaning constructor. It consumes caller-pushed allocation/scope operands and return routing, pushes handlerTU326BC8/u532A738, saves actual FS:[0], stores the source frame/scope, saves EBX/ESI/EDI, sets scope levelEBP−4 toFFFFFFFF, installs FS:[0]=EBP−10, and RETs through its source-preserved caller return address. Handler-address correspondence is an actual pushed operand; the handler body is **not** separately proven/executed here. Actual current FS chain/storage remains external owned state. [TU prologue](../analysis/damage-shields-goal-20261011/overlay-pass/registration/process-heap-seh-copy/paired/instructions/00326b74-old.asm), [u5 prologue](../analysis/damage-shields-goal-20261011/overlay-pass/registration/process-heap-seh-copy/paired/instructions/00326b74-u5.asm).

`326BAF` reads previous FS chain fromEBP−10, restores FS:[0], POPs its own return intoECX, restores EDI/ESI/EBX and theEBP frame, PUSHes that return and RETs. It preserves EAX, allowing the constructor's actual pointer/zero result to survive. This is the complete nine-instruction owner; it requires the actual source-created frame, not a replacement normal function return. [Original epilogue](../analysis/damage-shields-goal-20261011/overlay-pass/registration/process-heap-seh-copy/paired/instructions/00326baf-old.asm).

`2D9803(segment,descriptor)` is a complete ten-instruction prepend/clear leaf, RET8: heap=`*(segment+18)`; descriptor.next=`*(heap+4C)`; updates that heap's+4C free-head todescriptor; clears descriptor+4 and+8. It has no kernel call, release of virtual reservation, callback or independently allocated storage. Its exact EAX/ECX/EDX/flags effects remain in the original listing; it must not be modeled as OS free. [Original leaf](../analysis/damage-shields-goal-20261011/overlay-pass/registration/process-heap-seh-copy/paired/instructions/002d9803-old.asm).

Evidence consists of269 supplementary complete instructions/image:38 small-owner rows and231 copy rows, with7 dispatch expressions and all30 copy branch records. Original image pins are TU1.1`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`, ownedu5`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`. The source SEH chain, handler execution, exceptions, TLS, kernel/lock results and whole process heap remain explicit external boundaries.
