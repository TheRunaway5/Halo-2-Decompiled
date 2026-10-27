# Native contact buffer ownership and exception cleanup

The native buffer owners implement actual English TU1.5 `303C70`, `303D00`
and `303EE0`, together with the owned-array base cleanup owners `303B70` and
`303F50`. Component backing has256-byte elements; contact records have128-byte
elements. These are compiled data algorithms with native C++ ownership guards,
not an Xbox register file, virtual FS chain or exception dispatcher.

The append interface takes an explicit caller-owned opaque constructor record.
It initializes only7D and7C to zero, in source order, grows through the already
qualified native2E3250 owner when necessary, increments count at the source
phase and calls native303BA0's ordered fieldwise copy. It never zeroes the
remaining constructor bytes or supplies a recorded contact result. Inline and
borrowed backing remain attached to their outer lifetimes; base cleanup only
releases owned backing and does not reset header fields.

The [current-source buffer matrix](../analysis/physics/native-contact-buffers-tu15-20261026-current-unwind/report.json)
runs45 physical cases per compiler, with GCC16 and Clang23. All45 match the
physics bytes, opaque packets and CW/SW/MXCSR. This includes actual pool-backed
stack exhaustion, owned growth and release, aliases, an actual alignment fault,
prewrite/observer/read-failure prefixes and all four rounding modes crossed with
DAZ/FTZ settings. Three private stride/flag/count mutations are rejected.

The strict whole-bank result is still RED:39 cases match every compared byte;
six interrupted constructor/destructor cases differ in two bytes of captured
FS:[0]. Actual original execution has an active Xbox SEH registration there.
The native implementation uses the host C++ exception mechanism. It does not
recreate that virtual registration. Those differences remain in the reports,
the acceptance calculation and the existing raw gates.

The [source ownership audit](../analysis/physics/native-contact-cleanup-ownership-tu15-20261026/report.json)
authenticates the actual handler thunks, FuncInfo headers, unwind-map bytes,
cleanup code and runtime dispatch against XBE SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

| Source owner | Actual ownership transitions |
| --- | --- |
| `303C70` | State0 arms at303CA9, after allocator/remaining reads; its action is303B70 |
| `303D00` | State0 arms at303D33 and disarms at303D66, after capacity read and before owned release; its action is303B70 |
| `304350` | State0 owns record backing; state1 additionally owns the component buffer. State1 unwinds through303D00 before state0 unwinds through303F50 |

Handler37ED68 selects FuncInfo46C02C and map46C024. Handler37EDF6 selects
FuncInfo46C084 and map46C074. The actual unwind runtime330D50 indexes8-byte
map entries and writes next-state at330DA8 before dispatching the cleanup
action. Actual3315C0 establishes the action frame at registration+0C. The native
guards follow that order and disarm before invoking cleanup.

The [independent cleanup qualification](../analysis/physics/native-contact-unwind-tu15-20261026-second/report.json)
passes18 cases per compiler. The reference reaches genuine original prefixes
and then executes actual original cleanup funclets37ED60/37EDE0 on real
pool-backed objects. The native side throws C++ exceptions and runs its guards.
No allocator result, expected packet, GP/SSE postimage or source exception
dispatcher is supplied. Each compiler records47 original reference calls and22
actual original pool calls. Fourteen cases propagate injected native exceptions.

Cases cover constructor success/disarm, before-arm failure, before and after
actual allocation, late prewrite and committed-observer failures, destructor
failures before and after actual stack release, marker/capacity reads, cursor
prewrite/commit, and disarmed owned release. Borrowed and genuinely grown owned
record bases are included. Early constructor arm, early destructor disarm and
late destructor disarm mutations are independently rejected. This qualification
establishes native cleanup semantics; it explicitly leaves original live FS
registration differences raw RED. Secondary exceptions, termination details and
platform dispatch remain open.

The full534-site producer has matching native guards and a native constant
sentinel for actual30BAD0 (`MOV EAX,48CDC4; RET`). Its broader qualification is
still separate. The [producer cleanup qualification](../analysis/physics/native-contact-producer-unwind-tu15-20261026-second/report.json)
passes13 actual original-prefix/funclet cases with both compilers in the real
inline Elongation context. State1 destroys the component buffer before state0
releases the record base. Cases exercise failures around initialization, the
post-constructor reads, the state1 publication store, query, append, contact and
distance publication, explicit destruction and the final capacity read. Reverse
cleanup order, early component arm and late component disarm mutations are
rejected. These are native cleanup semantics, with source FS differences kept
explicit; this does not qualify dynamic owned producer records or secondary
cleanup exceptions.

The [current-source seven-call cohort](../analysis/physics/native-contact-producer-cohort-tu15-20261026-current-unwind/report.json)
has exact FP/counts with GCC and Clang. The single inline case matches all
persistent bytes, but six dynamic cases retain84–105 differing bytes in copied
opaque constructor data. All differences fit the retained bounded metadata.
No gate was relaxed.
Before declaring those bytes irrelevant, their read/write/lifetime behavior,
validity-publication windows, heap reuse and future users still require closure.
The producer has not been admitted by these component results.

The buffer and cleanup reports include source/dependency hashes, commands and
small physical diagnostics. Their cleanup receipts verify owned scratch is
absent and no large artifacts remain. The current buffer run removed288,993,280
allocated bytes; the cleanup run removed288,989,184. These measurements exclude
already removed nested CMake products. The producer cleanup and current-source
cohort owners are also absent, removing289,087,488 and865,292,288 allocated
bytes respectively. Their small reports and cleanup receipts remain. All
protected v1458 input banks remain
direct reference inputs, and the old whole-frame and source-FS raw failures are
unchanged. Live native-only multiplayer and Xbox hardware identity remain
separate acceptance gates.
