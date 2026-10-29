# Native behavior82 initialization

The current-source Johnson encounter stopped at tick571/BSP5 on
`1B0D1C ->1C9CE0`. The actual behavior registry is `47B478[82] ->48B770`,
whose initializer slot at+18 contains `1C9CE0`. The original initializer is a
21-site, call-free owner. Its only literal address reference is that registry
slot; there are no direct relative calls or tails to the leaf.

`native_campaign_ai_behavior_reset_tu15.cpp` now implements its complete
semantic operation. It obtains the actor datum, reads the actor pool and row
base, obtains the action record, masks the datum to its low16-bit row index,
clears seven actor behavior fields and four action fields using the original
widths and publication order, then reports successful initialization. Pointer
and argument accesses remain lazy; no record or successful lookup is supplied.
Null/unbacked destinations fail after any preceding completed writes.

The numeric binding only consumes the actual return word and eight argument
bytes and publishes the owed bool inAL. The original caller copiesAL intoDL at
`1B0D1E` and testsDL at `1B0D20`; its later paths define fullEAX/ECX/EDX before
using those volatile values. The binding preserves the dead upperEAX bits
instead of rebuilding the original actor-address scratch result. It preserves
callee-saved state, flags, SIMD, x87/live values and FP policy. No guest register
machine, per-instruction execution, fallback or production binary execution is
introduced.

`analysis/campaign-native-ai-behavior-reset-20261028/proof.json` joins the
reference, caller evidence and compiler results. The required compressed
original fixture is `tests/fixtures/native_campaign_ai_behavior_reset_tu15.json.gz`
(98212bytes, SHA256
`ad15928e15fc6dd6c6007ab992f5349023307278263518d0c9ac8504ebb474dc`).
Its26 completed calls and52 stopped prefixes cover all21 original sites,
including salted datums, unaligned records, actor/header/binding aliases and a
stack-resident action. The prefix grid includes every read and store boundary
in two seed paths. Typed and ABI entry tests compare292 original ordered reads,
528 stores, retained memory and FP, caller state and a genuine missingRET.

GCC normal and actual ASan/UBSan runs pass1/1 each. Clang passes5/5: normal
plus rejection of actual wrong-actor, wrong-sentinel, wrong-width and false-bool
source mutations. Declared sources remained unchanged during validation and
all private original ELFs, native probes, mutant binaries and build trees were
removed. The fixture and compact receipts remain. This is component/ordinary
caller qualification; a subsequent current-source encounter must demonstrate
passage beyond the former Johnson boundary. It does not establish complete
native AI, campaign or rendered parity.
