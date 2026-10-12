# Shared camouflage timer and fade owners

The recovered camouflage refresh, two signed16 powerup timers, activation,
expiry, unit fade and disruption now have native source kernels. They preserve the original
selected-engine trait query, gradual primary/secondary amounts and shared unit
action bits. Their [native world binding](MP_CAMOUFLAGE_NATIVE_BINDING.md)
records the separate integration verification and remaining renderer/replication
requirements. Normal camouflage variants remain gated.

## Original contract

[source_camouflage.cpp](../src/source_camouflage.cpp) implements the original
TU15C8B0,1537F0,153990,153920,D22B0 and the multiplayer actor-NONE branch of
C7B70, plus D2310's intrinsic-zero disruption branch.
[Reference owner research](MP_CAMOUFLAGE_STATE_SOURCE_RESEARCH.md)
records their original image addresses and complete instruction comparisons.
The typed service views keep the action word and fade values in their caller's
authoritative storage; these arithmetic owners do not invent allocation defaults
or replace the ordinary unit action word with a camouflage boolean.

Refresh queries selector1 before checking the current unit. With a true trait
and a current unit, it converts signed tick-rate times0.5 using the original
x87 rounding-control bits and grants the signed16 maximum of the old timer and
converted value. Only an exactly-zero old timer activates bit3 and installs
speed0.25. Negative nonzero timers do not activate. Expiry clears bits3/4 and
sets speed0.25 without zeroing either fade amount. The primary and secondary
amounts approach their directional endpoints independently. The actual dirty
entity request is required only when the supplied original entity is not NONE.

Disruption subtracts its authored amount from primary and caps speed to the
supplied limit, selecting0.25 for a zero limit. Active bit3 supplies the0.05
floor; inactive primary may become negative before a later fade clamps it.
The actual weapon and damage callers supply `weap`+27C/+280 and selected
`jpt!`+30 respectively. All6,912 finite intrinsic-zero whole-call comparisons
pass in the native standalone test. Nonzero intrinsic mode requires its actual
controller service; no successful substitute is supplied.

## Corrected floating-point reproduction

The earlier trait fixture's requested rounding labels did not reach the original
conversion: inherited `Source.call` reset FPCW to027F and MXCSR to1F80. Its
original files remain immutable, and its damage comparisons remain valid.
The [corrected oracle](../tools/oracle_camouflage_fp.py) preserves declared FP
controls and observes their actual values immediately at the original FISTP,
along with the input bits and produced integer. At31Hz it now observes grants
16/15/16/15 under027F/067F/0A7F/0E7F respectively. This fixes the harness rather
than making native code reproduce a mislabeled environment.

The new fixture contains1,656 declared consumer inputs, run in all three
unchanged images:4,968 whole refresh/timer/fade calls. It includes signed-rate
boundaries, negative/positive/zero timers, trait/inside overrides, missing units,
independent unit bits, out-of-range fade inputs and original binary32 update
ordering. All state bits and ordered queries match. The fixture also records
864 actual FISTP observations with matching requested/observed controls.

## Verification and operational boundary

The [first frozen kernel verification](../analysis/multiplayer/game-variants/camouflage-native-20261011/kernel-verification.json)
configures the real repository CMake target, builds `camouflage_tests`, passes
`original_camouflage`, freshly reproduces the unchanged original fixture and
passes a fully instrumented standalone ASan/UBSan build. It pins the complete
source snapshot, the three executable images, actual Lockout tag backing,
logs and binaries; concurrent checkout drift is recorded separately.
That historical run predates the added disruption corpus and live binding.
The [complete native verification](MP_CAMOUFLAGE_NATIVE_BINDING.md) also passes
the expanded original target and both fully instrumented standalone fixtures.
`tools/verify_camouflage.py` now also supplies the disruption fixture to the
standalone target and freshly reproduces it. Run with the repository Python
environment: `.tools/python/bin/python tools/verify_camouflage.py`.

These are individual-owner comparisons, with actor and replication entity NONE
in the executable corpus. They do not establish complete normal world frames,
physical Xbox frames, enabled replication, intrinsic Elite camouflage or final
pixels. The native binding separately checks its actual gameplay consumers.
King startup remains gated, and the
[39-variant goal](GAME_MODE_PROGRESS.md) remains active and incomplete.
