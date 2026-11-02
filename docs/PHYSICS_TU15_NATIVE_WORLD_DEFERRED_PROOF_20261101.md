# Native TU1.5 deferred world queues

The typed native owners319750,316E90,319CD0 and316E10 now execute world
operation, added-action, deletion and update queues. They retain cached
arguments, fresh queue/count reads after callbacks,16-bit reference transitions,
protected-reference handling and source destructor order. Native282BC0/282BE0
allocation monitors bracket genuine merge work and use the already native
149B90 heap accounting. They do not supply merge, removal or destruction results.

Authority: English TU1.5 SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The queue bodies contain42/44/45/46 sites; merge monitors5/9, allocated-byte
summation36 and the reserved-reference predicate37. The genuine2E4D60 RET4
no-op destructor is executed in admitted component cases; other owners stop
at their actual entry without supplied outcomes.

`build/test-results/native-world-deferred2-20261101/report.json` passes452
physical original cases under GCC and Clang:268 complete calls,11 genuine
required-owner stops and173 read/store/observer prefixes. All retained bytes,
5,020 ordered reads,689 stores and FP state match through18,797 unchanged
original instructions. Three private decrement/count/monitor mutations fail.
Nine RMW prewrite observer prefixes are explicitly excluded because the current
read/write memory interface cannot reproduce the oracle's pre-retirement read
observation boundary. Complete atomic fault-contract qualification remains open.

`build/test-results/native-world-deferred-binding3-20261101/report.json` passes
110 original world cases,96 native force compositions, the world model and its
negative control. The model performs genuine callback transport for merge,
body/update removal and destruction. A removal callback changes the current
queue entry; native code rereads it and releases the replacement reference.
Callee private-stack poisoning verifies that retained result bytes survive.
All four queue counters clear; none of the four old queue owners is invoked
through the residual service. Named nested merge/removal/destruction owners
remain explicit and need their own native conversion.

The first model fixture overlapped independent records and correctly failed.
The invalidated second build was cancelled and removed; only the corrected
third report qualifies this binding. A validator setup syntax error occurred
before physics execution and is retained separately as a setup diagnostic.

A staged imported-MCC multiplayer build requires all four native queue counters,
positive native reference releases and absence of the old owner keys. Visible
actual-camera and strict original whole-frame results are pending; these
component/model passes do not complete native-only physics parity.

Both component compiler trees and outer scratch are absent;143,441,920 allocated
bytes were measured before outer cleanup. All completed/cancelled world-model
build owners are also absent. Their cleanup receipts distinguish measured bytes
from unmeasured cleanup. No large new artifacts were retained. The frozen
binding is recorded in
`analysis/physics/native-world-deferred-live-binding-20261101/source-pins.json`.
