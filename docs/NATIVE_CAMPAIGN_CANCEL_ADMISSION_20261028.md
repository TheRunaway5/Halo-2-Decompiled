# Campaign property cancellation admission, TU1.5

`native_campaign_ai_cancel_property_admission_campaign_tu15` implements the
actual `275770` campaign/nonmode2 owner using the proved native track-conflict,
bit, pool-iterator, timer and group-head dependencies. Its closure has 460
original sites: 450 qualified sites and ten explicitly unqualified mode2
instructions (four in `275770`, six in `217310`). The public header and source
are `native_campaign_ai_cancel_admission_tu15.hpp/.cpp`; production integration
is `cmake/CampaignNativeAiCancelAdmission.cmake` after the cancel-support and
cancel-tracks modules. It uses the existing single shared pool helper library.

The algorithm reads actual property disabled state and object suppression
bit, then the actual group/team and campaign `5020C4+C4` relation. It either
uses the real `275610` conflict result or scans the real frame pool at
`4F54F4`, captures clock/tick ownership, checks both actual timer records,
and traverses actual group actor tracks. The private iterator is a typed C++
local. No source stack, register file, instruction dispatcher, binary runtime,
policy success, timer result, absent actor or speech result is synthesized.

Clock ticks are captured once; timer values and signed WORD tick rate retain
their read order. Elapsed ticks use wrapped DWORD subtraction followed by
signed comparison. The scan continues through all players after finding a
fresh timer. Group-head lookup precedes unconditional capture of track and
actor pool owners, including an empty group. Each member reloads rows from
its captured owner and reads the next link before the current track datum.
Statuses1/2 reject admission; other signed status values remain exact. There
is no invented self exclusion in this group-track traversal.

Mode2 requires its actual `160A60` policy owner and fails explicitly. The
inherited inactive-to-active undefined-iterator boundary from `275610` also
remains unavailable. Their negative admission checks are not parity coverage.
The entire generic mode2/cancellation/speech/campaign gate remains open.

The offline oracle executes unchanged owned TU1.5 instructions on the physical
i386 CPU, image SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
GCC and Clang ASan/UBSan pass 108 complete calls, 37,393 callee instructions,
6,696 ordered engine-memory reads, zero callee engine-memory stores and all
450 qualified sites. Checks include complete retained banks, functional AL
results, original return-stack/nonvolatile contracts, SIMD/occupied80-bit x87
state and FP controls/status. Native integer calls preserve host FP controls.
Original private-local traffic is transported through typed native locals;
shared iterator field ordering is independently proved by its native-local
component profiles. No original instruction/state edit or supplied return
appears in this oracle.

One case enters the real `1BD8F0` cancellation owner, executes 41 original
prefix instructions including its actual secondary-property flag publication
at `1BD957` (`00 -> 01`), captures the real `1BD973 -> 275770` arguments/state,
and returns naturally at `1BD978` before speech dispatch. The prefix store is
retained as caller evidence and as captured callee input; this does not claim
native implementation of the complete `1BD8F0` callback.

Eight semantic mutants are rejected: ignoring property disabled state, using
the wrong object flag, substituting A4 for the real C4 relation, accepting the
timer equality boundary, unsigned elapsed-time comparison, stopping the player
scan at its first fresh timer, admitting status3, and moving the member link
read after the current-track read. Cases include timer equality/future/wrap,
signed age/rate boundaries, multiple timers, missing timer owners, multiple
group members, an active self member, and actual caller argument transport.
Mapped finite lists and the stated policy/enable profile are covered; dead
scratch state, unmapped faults and unmasked FP traps are not claimed.

Reports are under `analysis/campaign-native-cancel-admission-20261028/gcc-v1`,
`clang-sanitizers-v1`, and `module-integration-v1`. Focused module validation
builds the production sources/shared owner, checks the provided executable,
returns skip77 with explicit false/not-run evidence for a missing reference,
and rejects a wrong image pin. Official wrappers remove original ELF, banks,
mutation sources and builds; the cleanup receipt records hashes and retained
small reports. The actual constant34 speech path is the next open frontier.
