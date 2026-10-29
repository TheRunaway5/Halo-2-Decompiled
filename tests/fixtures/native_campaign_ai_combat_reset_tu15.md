# Native actor combat and perception reset

This closes original `1EC7B0`, including `267220`, `267290`, `266490` and
the shared actual `16E8A0` data-array deletion owner. It is a required
dependency of authored order-reset `1EC820`; it does not complete
`1EC820`, unit actions, or `ai_set_orders`.

Modes greater than 3 return before any actor input is read. Negative modes
and mode 3 publish the actual actor mode and zero timer. Mode 2 walks the
real perception list, honors signed state/query-NONE/null-pointer gates,
and marks the existing query field. Modes 0/1 retire eligible queries,
publish query-NONE, clear only the first matching cached actor reference,
apply the original type policy, then publish the perception's state and
float-zero fields in original order. Query records are actually freed;
there is no supplied action/behavior outcome or success stub.

The policy WORDs read at `47D638 + kind*16` already belong to the current
native readonly engine metadata in `native_tu15_ai_object_tracking_data.inc`,
included by `native_tu15_final_startup_storage_data.cpp`. Their literal
intervals include the nine policy WORDs; no new runtime executable input
or synthetic policy bank is introduced. The original table resides in its
writable `.data` section; the native kernel still reads actual caller
storage rather than substituting a constant policy switch.

The reference executes unchanged English Xbox TU1.5, SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Physical i386 checks complete 73728-byte banks and SSE status; Unicorn
records source read/store order. Controls include signed mode/state
boundaries, empty/mixed lists, actual NONE gates, both source null guards,
nonzero actor indices, query poisoning/reclamation, all cached-reference
positions, duplicate-first and absent matches, and both type policies.
Every selected read/store fault prefix has an independently executed
physical partial-state hash. Source x87/FP arithmetic is absent; the native
algorithm preserves the caller's ambient FP environment.

The production binding decodes real EAX/DI inputs, reads the caller's return
word and consumes exactly that return word. It preserves engine forward
string direction admission (`DF=0`), rejects other entry ownership, and
exposes a read-only atomic entry diagnostic. It does not reconstruct dead
volatile GPR/flags/private stack state. Actual partial state survives
faults; no source instruction-machine fallback is present.

GCC, Clang and ASan/UBSan build the production CMake module and shared
delete library. Actual shared-library sanitizer flags are retained.
Private compiled mutations remove query reclamation or clear every
duplicate cache entry; both must fail the original reference. Only the
required compact JSON, source hashes and bounded reports survive; original
ELFs, raw banks, binaries and private mutants are disposable. Original
references must regenerate byte for byte. Full campaign/runtime delivery
acceptance remains separate work.
