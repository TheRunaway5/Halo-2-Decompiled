# Actual TU1.5 `D3750` vehicle effect ratio

The unchanged TU1.5 `C9070` body calls `D3750` at `C9101`. Its World owner
dispatches through Phantom, whose actual entry table maps `D3750` to
canonical `D26C0`, and remaps all 34 instruction sites. PhantomForce forwards
to VehicleEnvironment, then ConstraintCleanup, ContactState, MotionMath,
Motion and VehicleUpdate. VehicleEnvironment has no local ratio body;
VehicleUpdate is the sole complete generated definition. Its selected root,
tag table and saturation word resolve to `4ECBD0`, `4F0414` and `46A0D0`.

The exact owner audit is retained at
`build/test-results/native-vehicle-effect-ratio-owner-audit-20261110/report.json`
(SHA-256 `ae17393f18ec830817759c6a586d9d8ee127c7d00c0fcfd95a9aa74b1c097c3a`).
Both pinned original images contain 34 childless instructions. MapPack 1.1
uses its original `D26C0` and original globals; the common PC displacement
and forwarded source spelling do not authorize a default in that image.
The earlier private vehicle-force stage supplied 42 direct physical scalar
profiles. That receipt qualifies value behavior separately from this new
raw ABI proof.

The private kernel keeps the same ordered elapsed-time decisions. Its raw
Frame preserves the first elapsed comparison, delayed definition lookup,
ordered-zero baseline decision through the narrow `AH` write, the distinct
elapsed/limit rereads, and all three actual return sites. The source register
file remains in State while the provider scope isolates host floating point
changes. A failed operand read sees the ABI before that instruction retires.

The first GCC sanitizer smoke passed all 34 actual sites, 62 paths (8 full
returns and 54 declared operand prefixes), 124 quiet/hostile comparisons,
1,628 full site packets and 832 provider packets. The receipt at
`build/test-results/native-vehicle-effect-ratio-physical-smoke-20261110/report.json`
records an unchanged i386 image and exact banks. The expanded matrix repeated
from immutable manifest `206eb774a8f21bf35b9225045634dd9cfd5a1ede138302a7b043c7e7dcb7cc67`.
Both GCC and Clang under ASan/UBSan passed 330 original paths, 660 comparisons,
11,506 full site packets and 5,814 operand packets per compiler. The 162
original read prefixes became 324 quiet/hostile provider stops per compiler.
Four compiled mutants distinguish lost AH ownership, a skipped baseline
reread, unordered comparison selection and the wrong denominator field.
The receipt is
`build/test-results/native-vehicle-effect-ratio-frozen-private-20261110/physical.json`
(SHA-256 `2d1414a6b6c675e37daafdd0c9db5e99b64704791c753c59ec4f748eef974042`).
The prior live run's numeric checks also passed, but a shared generator
changed while its imported bytes were pinned; that receipt remains failed.
All scratch owners were removed. This is a private raw candidate, with no
source admission, Vehicle default, parent `C9070` replacement or launcher
change.

The separate source view uses the real current direct or prepared callback
before reading each authored operand. It does not inherit raw CPU provider
or FP isolation semantics. The genuine VehicleUpdate generator yields a
different whole unit from the checked-in source, but the complete `D26C0`
body, selected resolver, local dispatcher and exported owner are identical
within each format. Preparation changes all 34 callbacks and the three
exact original global selectors. The source fixture compiles those exact
retained bodies and uses the original Phantom remap callable with the complete
16,402-row vehicle table, not a locally supplied result.

The frozen authored source proof at
`build/test-results/native-vehicle-effect-ratio-source-frozen-private-20261110/source.json`
(SHA-256 `88f04e5e1f029d2df06c48a570ce2f66c297f2c2014578ddfed7f6e15efe7c57`)
passed both sanitizer compilers. Each format and compiler passed 2,184
comparisons and 22,010 pre-operand packets; direct/prepared callbacks yielded
36,671/32,403 complete packets, including all 34 actual remapped sites.
Each format/compiler passed 152 callback stops and 136 provider stops, and
each compiler rejected four compiled defects. The corpus also exercises live
callback clear/rebind, trace activation, register/provider changes and
distinct NaN payloads. Both owners were removed. The complete exported
Vehicle chain and parent still require separate proof.

The first linked VehicleUpdate test compiled both source routes, but both
failed on the original operand prefix at actual `D378D`. The exact
byte-identical fixture diagnosis at
`build/test-results/native-vehicle-effect-ratio-linked-prefix-diagnosis-r3-20261110/report.json`
found one mismatch: physical `SHL4` at `D378A` leaves AF set (`0x216`), while
the retained modeled shift and the initial native source view clear it
(`0x206`). All other GP, SIMD, x87 and MXCSR fields, operand reads and callbacks
matched. The original failed linked receipt and both historical source proofs
are preserved. A private revision uses the already physically qualified shift
primitive at only this TU1.5 source expression, including the retained route,
with exact reversal to the previous complete owner. MapPack and the
unsupported-backend shift keep their modeled policy.

The corrected prepared linked VehicleUpdate CTest passed both default and
forced reference against the byte-identical original fixture: each path
matched 168 returns, 162 operand prefixes, 5,753 callback sites, 2,745 ordered
reads and the complete escaped state. See
`build/test-results/native-vehicle-effect-ratio-vehicle-update-linked-ordered-private-20261110/report.json`
(SHA-256 `8f136890d590f793be6f446413ece53fa712c24ae69eac66cf797532b84afef9`).
Both sanitizer compilers also passed the first corrected binding check,
including the old image's modeled shift and an executable unsupported-backend
fallback. Both compilers passed the full corrected source corpus and rejected
the deliberate old-shift mutant. The specialization-off linked VehicleUpdate
build also passed both default and forced reference with identical coverage.

The actual PhantomWorld export was relinked using all real R31 archives. The
archived accounting alias regenerated byte for byte, and only the corrected
VehicleUpdate object member changed. Actual default and forced reference each
passed the same 330 physical paths with the real Phantom mapping and all
seven Vehicle forwarding/owner archives. The link map and dependency checks
found no foreign scratch inputs. See
`build/test-results/native-vehicle-effect-ratio-phantom-r31-private-r3-20261110/report.json`
(SHA-256 `a9117e4da9b3e1c955f19fe06eba748a2dba1080fea4aef6a4a61eadc7f4d982`).
The earlier linker-metadata failures remain separate; all owned archive copies,
the executable and the raw original fixture were removed.

The live source installation changes only VehicleUpdate, its owning
generator, the late specialization hook, the source catalog row, and the
guarded helper dependency. Exact raw/prepared removal recovers all prior
bytes, including the existing BBE70 overlay. The insertion receipt is
`build/test-results/native-vehicle-effect-ratio-installed-overlay-20261110/report.json`.
The private full linked fixture predates this write; the installed current
and fresh source checks passed from immutable manifest `07d14759…` under
both sanitizer compilers. Every direct/prepared and default/reference
combination passed 2,184 comparisons. Each compiler rejected the old-shift,
ignored-reference and wrong-image defects. The bounded installed closure is
`build/test-results/native-vehicle-effect-ratio-production-closure-20261110/report.json`.
No C9070 parent or coherent engine/runtime parity is inferred.

The two permanent CTests, `original_tu15_native_vehicle_effect_ratio_raw`
and `original_tu15_native_vehicle_effect_ratio_source`, run through
`native_vehicle_effect_ratio_tests`. Both passed without skips in the
staged plain and sanitizer configurations and in the installed sanitizer
configuration (106.13 seconds). The real cohort selector matched all 453
registered names and excluded all 906 negative controls. The
`HALO2_REFERENCE_VEHICLE_EFFECT_RATIO` control remains covered. The
registration closure at
`build/test-results/native-vehicle-effect-ratio-registration-closure-20261110/report.json`
pins the exact registration and all three runs. All 15 owned scratch
paths were removed; no large artifacts were retained.
