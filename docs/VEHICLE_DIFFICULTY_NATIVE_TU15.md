# Native TU1.5 damage difficulty selection

Status: original-code and installed-source component comparisons pass. Rebuilt
live destruction and launcher delivery of this change remain open. This does
not close complete vehicle, damage, audio or visual parity.

Complete original `1F1950` selects an authored scalar parameter. It reads the
default word before checking the global pointer, count and data pointer. The
count gate accepts any nonzero word, including negative values. Difficulty uses
the signed low16 bits, clamped to0..3. The parameter is also signed16; its index
arithmetic wraps as the original32-bit owner does. Selection copies the raw
SSE word, preserving NaNs, signed zero and denormals without arithmetic.

Complete original `1F19B0` reads the game mode. Campaign mode uses the authored
signed difficulty; every other mode selects literal1 and tail-calls the same
parameter owner. The replacements use structured C++ control flow and a native
parameter-index calculation. Their adapters preserve original ABI, ordered
reads, faults, flags, raw SSE state and observation sites.

The pinned English TU1.5 executable is an offline oracle only, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The [qualification](../analysis/vehicles-tu15-goal-20261023/native-difficulty-v5/report.json)
executes528 declared original inputs, including40 actual missing-page faults,
and covers all33 instructions in the two complete owners. The native kernel
and four retained source compositions make2,372 comparisons against those
original executions. Comparisons include ordered reads, full GP/flags/SSE
state, floating-point controls and unchanged complete declared memory.

The [negative control](../analysis/vehicles-tu15-goal-20261023/native-difficulty-negative-v2/report.json)
changes the lower difficulty clamp from0 to1 only in private native source.
The unchanged oracle rejects its ordered table read. No original instruction,
dependency return or expected native result is supplied.

The [scoped installation](../analysis/vehicles-tu15-goal-20261023/native-difficulty-routes-v1/admission.json)
adds native routes to AI frame, direct damage, vitality and damage delivery.
Every retained literal body remains byte-identical. The separate [installed-route comparison](../analysis/vehicles-tu15-goal-20261023/native-difficulty-installed-v3/report.json)
passes the same528 inputs and requires native activity counters in every route.
The delivery adapter preserves both its canonical and selected-image observation
APIs. Older-image paths retain their existing literal owners. Native activity
counters are process diagnostics and never enter gameplay or serialized state.

Generators retain the qualified routes. Production C++ executes these kernels
without loading or executing the original executable. Full vehicle acceptance
continues in [the active checklist](VEHICLES_TU15_GOAL_CHECKLIST.md).

Direct-damage and vitality regeneration reproduce the current source exactly.
Delivery regeneration first rejects a pre-existing canonical hash difference.
The [baseline attestation](../analysis/vehicles-tu15-goal-20261023/native-difficulty-delivery-regeneration-base-v1.json)
binds the newer canonical preimage to the coherent374-case frozen source.
The recovery tool retains its old exact guard and additionally accepts only
this exact qualified preimage. [Regeneration](../analysis/vehicles-tu15-goal-20261023/native-difficulty-delivery-regeneration-v2.json)
then authenticates90 whole original routines/8,976 static sites and reproduces
the current delivery source byte-for-byte. No original arithmetic or body is
changed to satisfy the guard.
