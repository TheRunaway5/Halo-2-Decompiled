# VehicleEnvironment BCDF0 public entry

The private candidate preserves the complete26-function VehicleEnvironment
unit and real public API. The original unit SHA256 is
`d68c9ad2f8e26a2e6ef70c3b40c19ae80b970ccacdb23f4199f75b852b276eae`;
the childless lookup body is
`0b00de7a5443ca306ccdc00399f92a82e90f9ce380177cf2468010b578fc4355`.
The exact owning helper inserts only its native entry/include. The25 neighboring
functions, copied Services table, FpScope, MathState load/save, catch and FP
publication remain byte-identical in the private source copies.

The proof adds one uniquely authenticated pointer-only hook immediately after
the actual MathState load in both API copies, plus a fallback entry counter.
This exposes the real internal state and copied service table to the test. The
reference API symbol alone is renamed. Removing the instrumentation and native
overlay exactly restores the full original source. The only linker stub is an
unreachable cleanup service that throws; every process verifies it is unused.
No lookup or child outcome is supplied.

The full baseline passed GCC and Clang sanitizers in native and forced-reference
processes:21,664 comparisons per mode,1,024 normal API calls,1,376 throw prefixes,
202,736 provider packets and all35 raw callback sites. It compares all returned
GP/XMM/flags/FS/logical80/status/depth/optional-FP fields, banks, ordered memory
operations/stores, callbacks, observers, exceptions, public FP publication and
caller host CW/SW/MXCSR restoration. GCC records208,440 callback packets and
Clang208,392; equality is compiler-local, with no cross-compiler undefined-flag
parity claim.

A later, separately pinned finite driver strengthens the external service-copy
witness and adds16 real pre-load refusals plus24 valid opaque MMX returns per
mode. Both sanitizer compilers passed1,608 comparisons per mode and rejected
four compiled defects in real public statements: omitted success ABI save,
omitted catch ABI save, aliased caller Services, and omitted FP publication.
The first guard trial incorrectly classified valid MMX as a load refusal.
MathState inherits WorldState's MMX-capable loader; the corrected refusal uses
incompatible nonzero x87 depth. The failed receipt is retained separately.

The full baseline and later finite controls have different test revisions and
are not merged into a full expanded-suite claim. Root's raw-owner proof is
separate authority for the original instruction stream. These public receipts
cover the retained API candidate, with the explicit private hook; they do not
claim installation, other images, whole-engine parity or physical page faults.
Strict installed-input preparation later proves the same compiled candidate and
reference bytes, supports `--require-binding`, and retains the full original
unit pin. Installed acceptance belongs to Root's subsequent qualification.

All disposable owners were removed and no large artifacts remain. The bounded
aggregate is
`build/test-results/native-vehicle-object-lookup-public-20261110/closure.json`.
