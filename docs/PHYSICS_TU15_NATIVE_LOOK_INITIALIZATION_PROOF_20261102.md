# Native TU1.5 look initialization

The complete 92-instruction `18A0C0` constructor now has a native kernel and
a bounded runtime candidate for its `prepare_player_look` caller. The candidate
is private until independent review and multiplayer validation. `18A300` and
its `CD4A0` camera dependency remain separate native migration work.

The kernel preserves the early returns, all 37 `REP STOSD` stores, redundant
later field stores, original direction rereads, native x87 atan/sqrt sequence,
spill rounding, signed byte copies, current FP state and store observer phases.
It has no child execution services, virtual CPU state or supplied results.

Both compilers pass 317 original comparisons, including 140 read/write and
poststore observer prefixes. Every original instruction was executed across
the controls. The comparison covers 3,213 ordered reads, 17,371 ordered stores
with preimages, retained banks, FP and `RET 0`. All three deliberate changes
to zeroing, direction sign and signed field publication were rejected.

The first run failed a write-fault prefix at the yaw `FSTP`: native arithmetic
had popped its result before the backend DATA write rejected it. The correction
stages that publication with a snapshot of the native kernel's own live
hardware FP state. A failed write restores that preceding native phase; an
observer exception retains the completed store and pop. No captured control
frame or original result is loaded. The first failure report remains saved.

The exact production helper binds only the caller's CW, status, MXCSR and
preserved depth fields. It passes the same 317 cases and all exception prefixes
in both compilers, including activity counts and FP publication on throws.
Another eight profiles per compiler use all 89,795,481 bytes of the authentic
multiplayer look input, its real unit and independently declared constructor
indices/rounding controls. All bytes and FP agree. That archive captures
`18A300` after constructor work; these are declared direct constructor calls,
not a claimed captured `18A0C0` invocation.

The runtime candidate changes only the actual new-unit branch inside
`prepare_player_look`. It retains fresh caller-frame setup, binds actual FP
fields, preserves the source store-observer callback and current SourcePC,
and rejects unsupported mutable observation without CPU fallback. Six activity
counters reach the getter and both JSON reports. The optional positive gate
requires completed initialization, positive unit direction work, consistent
record clears and zero refusals. Other constructor call sites stay explicit.

Both compilers pass the exact Host block/counter/gate model and all four
coherent World models with the 138-CPP closure. These are component and binding
proofs, not visible multiplayer or complete camera/controller acceptance.

Frozen stage and all seven proof hashes:
`analysis/physics/native-look-initialization-world-stage-20261102/source-pins.json`.
The first failing prefix is retained in
`analysis/physics/native-look-initialization-tu15-20261102-first`.
All temporary products use owned test storage and have been removed; the
cleanup receipt is `analysis/physics/native-look-initialization-cleanup-20261102/report.json`.
