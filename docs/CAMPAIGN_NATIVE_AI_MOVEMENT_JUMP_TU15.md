# Native TU1.5 movement jump calculation

`native_campaign_ai_movement_jump_tu15.cpp` admits the actual FF1A0 argument
profile used by 200420 at 200578 and 2005DB. EAX and ECX identify the target
and origin, ESI receives the direction, and the eleven stack arguments carry
maximum speed, gravity scale 1, minimum-speed output, two absent speed-input
pointers, high-arc flag 1, three absent outputs, vertical-velocity output and
horizontal-speed output. It returns AL and performs RET 2C. The original
caller consumes the saved registers, stack, retained output words and FP state.

The adapter reuses the existing qualified jump-arc mathematics. The extension
publishes the computed minimum speed at FF2E1 before solving the trajectory,
then the normalized direction at FF4F1/FF4F3/FF4F6, vertical velocity at FF525
and horizontal speed at FF537. Zero scalar pointers remain optional. The
straight FF580 fallback publishes direction while leaving these three scalar
outputs untouched. No trajectory, arrival, unit action or completed result is
supplied. The alternate 200420 action-mode-5 branch selects E8E50 action 2E,
whose EF300 start callback remains an explicit unsupported owner.

The actual original reference contains 327 complete calls with 1,617 ordered
stores, including all FP rounding modes, precision modes, denormal controls,
finite and exceptional inputs, optional-output combinations and overlapping
output publications. All 220 FF1A0 sites applicable to this fixed option
profile are covered. The 39 omitted FF1A0 sites belong to absent alternate
speed inputs and distance/chosen-speed/duration outputs; three optional FF580
stores are likewise absent. All 38 normalization sites execute.

Native replay compares AL, saved GP/ESP, full declared retained DATA, every
ordered store with its exact source PC, x87 control/status and MXCSR, and host
FP restoration. It does not claim EFLAGS, unconsumed volatile GP/XMM, ordered
reads or private child stack bytes for this new ABI profile. The required
physical-original fixture is
`tests/fixtures/native_campaign_ai_movement_jump_tu15.json.gz` (73,859 bytes,
SHA-256 `40fa071df615010dcfbb004a514a3a8ae92cfc1545f78fdb3a7e475a3a37c953`).

The shared arc regression exposed an existing pending-read defect: the current
buffered value normalization API fetched epsilon before entering its native FP
scope. If that read failed, the arc's precise exception adapter observed the
host's live FP value. The new explicit ordered value API uses the same native
normalization arithmetic, but reads epsilon at its original location after
the extended norm has been computed. Only arc composition opts into this API;
other value-normalization callers retain their current interface. Both the
pre-extension arc and its extension reproduced the old failure, and the
ordered API restores all 410 original arc/link cases, 284 private-point
variants and 48 pending-quantity boundaries.

Original captures and their independent repeat execute unchanged physical
i386 instructions offline. Production receives neither the original image
nor fixtures. Commands, source pins, mutation/sanitizer results and verified
scratch cleanup are retained under
`build/test-results/armory-native-progress-20261113/movement-link-transition/`.
The parent 200420 owner, actual mission progression and runtime integration
have their own qualification receipts.
