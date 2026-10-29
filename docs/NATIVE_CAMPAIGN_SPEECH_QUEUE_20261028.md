# Native TU1.5 campaign speech queue

This closes the actual `217970` queue admission capability and its full native
callee set: `217870` collision postponement, `218070` dependent cancellation,
`2180F0` cancelled-row reclamation, and the existing shared `16E8A0` pool delete.
The 322 new instruction sites implement structured queue algorithms over the
actual `505E70` speech pool and the caller's policy queue. They contain no
virtual CPU, instruction dispatch, generated routine call, host sound result,
or supplied original return value.

The public API is `include/halo2/native_campaign_ai_speech_queue_tu15.hpp`.
`cmake/CampaignNativeAiSpeechQueue.cmake` adds the semantic source to the physics
runtime and links the existing single-owner shared pool helper. The independent
test executable uses that same helper target. No whole-campaign registry or
runtime route is enabled by this module.

## Actual behavior preserved

* Admission respects the queue's optional unit restriction, source-linked
  ordering, and signed wrapped timestamps. It postpones a candidate against
  preceding rows before choosing the insertion point. Delay and dependency
  checks occur before publishing links and the 16-bit count.
* Later rows observe the real separation constant and a captured clock owner,
  with lazy tick-rate reloads and the original repeated conversion where a row
  is too early. A cancelled row does not become the preceding live row.
* Collision postponement resolves the actual dialogue definition even when
  the time-window check subsequently rejects the collision. It keeps strict
  interval ends, signed priority thresholds, close-time tie rules, active-row
  exceptions, and dependency checks.
* Cancellation follows the actual linked suffix and validates the full signed
  salt, extent and configured stride before consulting a dependency's cancelled
  flag. Rows and pool metadata remain lazy reads.
* Reclamation captures the pool once after checking the real head, unlinks only
  cancelled entries, clears a unit's active speech reference only when it
  identifies the retired row, calls the real shared pool deletion algorithm,
  then publishes the next link and decrements the 16-bit count. The next link
  is captured before optional debug poisoning.

Timing uses native scalar SSE conversion/multiplication and hardware extended
load/integer conversion with the original control word, status and MXCSR. The
helper borrows one available extended temporary and restores the host's FP
environment. It returns the real last scalar result for a future thin ABI
adapter: `XMM0` low32 changes only when conversion occurs, while its upper96
remain the caller's; the suffix's actual `MOVSS` load into `XMM1` clears its
upper96. The other SIMD registers and occupied extended operands remain intact.

## Independent reference and scope

The offline reference executes the owned, SHA-256 pinned TU1.5 instructions
unchanged on a physical i386 process. No instruction is replaced and no callee
result is supplied. Every original engine-memory read records its value and
the number of preceding stores; every store records its before/after bytes.
The native test compares that complete interleaving, all retained banks, result
widths, FP control/status, actual SIMD outcomes, and preserved caller registers.

The corpus includes interval and priority equalities, active speech exceptions,
negative and wrapped ticks, all supported extended precision/rounding modes,
SSE rounding and DAZ/FTZ states, up to seven occupied extended operands,
transitive/invalid-salt cancellation, debug poisoning, extent contraction,
matching/nonmatching unit references, negative unit-state offsets, a bitmap
aliasing the queue head, queue-count/timestamp overlap, and a clock rate that
aliases the repeatedly postponed candidate timestamp.

Two caller receipts start at the actual publication block `217812`, execute
its genuine `217310` policy call, reach `217837 -> 217970`, and return naturally
to `21783C`. The captured policy queue and datum are passed to the native API.
These authenticate this block's argument/result contract; they do not claim
execution of the preceding `2175A0` allocation, sound selection or request
construction. The queue's internal native owners are compared through genuine
original `217970` calls as well as their independent inputs.

`analysis/campaign-native-speech-queue-20261028/` retains the small receipts.
The current final matrix contains 176 original calls, all 384 sites in the
closed queue/delete graph, 4,844 ordered reads, 506 stores and 17,160 callee
instructions. Six controls reject changed interval-end equality, priority
threshold equality, maximum-delay equality, omitted salt validation, clearing
another unit's reference, and moving the pool read ahead of the head-empty test.

Required admission is mapped input owners, masked FP traps, forward string
direction (`DF=0`) for deletion, and one free extended temporary on timing
paths. Unmapped-input fault prefixes and unmasked traps are outside this proof.
Full `213D30` request dispatch, `214EA0/214710` collection/scoring,
`215390/2175A0` publication and the request-`0x34` cancellation composition
remain open. No audio provider, dialogue backend or whole campaign parity is
claimed by this queue proof.
