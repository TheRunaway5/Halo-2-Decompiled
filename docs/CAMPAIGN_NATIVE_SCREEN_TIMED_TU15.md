# Native timed cinematic screen kernels

`native_campaign_screen_timed_tu15.cpp` implements the complete call-free
TU1.5 `1FC10` DOF setter and `1FD00` timed screen update. The existing lifecycle
kernels implement `1FCB0` and `1FCE0`. The production compatibility entry in
`source_campaign_screen_native_tu15.cpp` delegates all four entries to these
semantic kernels; the old generated screen source remains reference material.

The setter clamps negative focal distance, accepts each nonnegative ordered
transition duration independently, publishes its start clock before endpoint
values, and computes the finish from the original double clock. The updater
handles opacity, both color channels and default direction, DOF interpolation
and retirement, five effect interpolations, four optional RGB multipliers,
configuration publication and crossfade capture. Authored endpoints, times,
flags and records drive the algorithms; there is no mission-specific behavior.

## Native arithmetic and storage contract

The kernels hold named controller values and traverse typed storage lazily.
Mutable pointer slots `474C78`/`474C80` and their vector components are real
reads. In particular, the first pointer is read before output14 is published;
comparison stops at the first differing raw word, and replacement words are
read/copied individually, preserving tested aliases. Only authenticated
read-only numerical literals (zero, one and epsilon) are compiled constants.

SSE scalar helpers execute actual host ADDSS/SUBSS/MULSS and comparisons with
the original operand order. The elapsed helper stages actual hardware x87
arithmetic around lazy clock/endpoint reads, retaining extended precision until
the original float publication boundary. Staging is needed because an earlier
memory failure must not perform a later arithmetic operation. No software x87
register file, guest instruction cursor, virtual GPR/flags machine, opcode
interpreter, captured result, or original binary execution exists in production.
The hardware environment scope saves/restores the host's real FP environment;
its temporary policy comes from the ordinary caller's explicit FP context.

An ordered equal pair of time endpoints selects the controller-specific
fallback; unordered endpoints execute the arithmetic. COMISS-based clamps
retain unordered values and signed zero. The setter's start clock spill does
not pop its extended value. A rejected destination restores the hardware spill
state; rejection by an observer after a completed write retains that write and
its FP effects.

The native transport consumes the real return word and only the public
argument/return contract. `1FC10` is void with six stack REALs and focus in the
lower XMM0 lane, consuming28 stack bytes. `1FD00` consumes4 and returns the
active bank address inEAX; an absent/inactive bank preserves incomingEAX. All
three original direct callers explicitly zeroEAX before calling. The typed
binding retains caller live x87 values, completed TOP/depth, callee-saved
registers and actual persistent SW/CW/MXCSR. It does not reconstruct dead
volatile register/flag outputs or private arithmetic values. A fatal storage
exception is not a resumable guest instruction boundary; caller TOP is retained
while persistent FP condition/exception status is published.

The qualified ordinary transport requires explicit FP policy, masked FP
exceptions, clear direction, no active MMX and no more than five live x87
values (the algorithm needs up to three temporary hardware slots). Unsupported
callers stop explicitly. The current arithmetic backend requires GNU/Clang x86;
there is no lifted fallback. Qualification was run on the current Linux x86-64
host executing the independent original reference as unchanged i386 code.

## Evidence and limits

`analysis/campaign-native-screen-timed-20261028/proof.json` joins final source
hashes, static caller/constant evidence, original execution and compiler tests.
The required original fixture is
`tests/fixtures/native_campaign_screen_timed_tu15.json.gz` (180546 bytes,
SHA256 `5d92635874d5545a73617782ad833de863b4c02198fe23291294be7410a96fa4`).
It contains505 complete original calls and339 independently stopped prefixes;
no instruction was patched, no dependency result supplied and no stopped
prefix resumed. Each case runs through the semantic API, numeric binding and
production compatibility API. Checks compare retained bytes, ordered reads and
stores, exact public outputs, FP status, live-value preservation and host FP
restoration. Two missing-RET cases also verify that preceding publications
remain and no return is fabricated.

The cases include all controllers, ordinary time regions, equal/reversed/NaN
endpoints, signed zero, subnormals, infinities and signaling/quiet NaNs,
precision/rounding policies, DAZ/FTZ, all RGB flags, view-count/epsilon gates,
DOF retirement, capture flags, raw vector mismatch and vector aliasing. The
complete original calls alone cover488 of492 sites. The four remaining sites
are two unreachable source clamp assignments/jumps: after clamping and spilling
elapsed to[0,1], `1-elapsed` cannot be below zero or above one under the tested
rounding modes; unordered values take neither assignment. No fabricated flags
are used to force those dead branches.

The339 fault prefixes comprise167 before-read,86 rejected-write and86
completed-observer-write boundaries. Ptrace stops before an instruction; it
cannot independently stop CMPSD's second memory operand. Six duplicate
second-operand labels were removed from the reference grid. Completed
comparison read order and per-instruction persistent failure state are tested;
intra-instruction memory-bus/MMIO micro-order is not established. Static caller
inventory authenticates all four direct sites and no literal entry pointers;
arbitrary computed callers and hidden register contracts inside transitive
indirect calls remain outside the ordinary ABI qualification.

Final GCC normal and actual ASan/UBSan runs passed1/1 each. Clang passed6/6:
normal plus rejection of actual source mutations omitting the opacity end term,
treating NaNs as equal endpoints, dropping DOF retirement, publishing the wrong
capture request, and caching the setter duration instead of rereading it.
Earlier receipts are retained as diagnostics; only `final-validation-v2` and
the final `original.json` are acceptance evidence. The first minimal sanitizer
attempt did not enable instrumentation; the corrected final project explicitly
applies address/undefined sanitizer compile and link flags.

All original ELFs, native probes, mutant sources/binaries, raw banks and private
build trees were owned by the official test wrappers and removed. Only compact
reports and the required compressed original input/reference fixture remain.
These component and ordinary-ABI results do not establish rendered/Xbox-silicon
parity or the complete campaign dependency closure.
