# Native prepared SMG effect request

The native stock-SMG preparation path now exposes its original `107160→107B50`
effect request before projectile postpass. It imports the authored firing-entry
references, preserves the full carried-weapon datum and previous effect handle,
and stores the actual configured owner's return in the original barrel handle.
The callback does not infer an effect from `shot_tick`, render frames or animation
clip names. It is the integration seam for the separately recovered
[effect consumer](MP_SMG_EFFECT_NATIVE_INTEGRATION.md) and
[actual particle births](MP_SMG_PARTICLE_OWNER_RESEARCH.md).

`WeaponDefinition::firing_effects` retains every first-barrel authored `0x34`
entry's count bounds and three effect/damage references. This follows the
existing first-barrel content interface; it does not generalize the live weapon
state machine to other barrels or weapons. The original instruction at107672
selects the reference at entry+08/+10/+18 for normal/misfire/empty, and107676
selects its damage companion at+20/+28/+30. Stock data admission additionally
checks weapon+154 and+25C are zero before using the recovered zero normalized
scalar and excluding the unported misfire path.

The prepared callback runs after the source-equivalent ammo, shot-count,
marker-count, feedback and pending-projectile stores, with the pre-rate-update
barrel scalar. In `107B50` ABI order its scalar words are the normal barrel rate
then zero, or one then zero for the empty request. These names describe stack
order, not invented color/intensity values. The effect entry's normal datum is
the original `effe`; its empty datum is the original `snd!`. Neither is replaced
with a generic flash. Existing reload and other `107B50` callers remain outside
this prepared-shot seam.

`MultiplayerMatch::configure_weapon_effect_owner` installs the required generic
owner as an explicit service. The live executable currently has no completed
allocator/event/lifetime/render owner to install. With no callback, the existing
unavailable effect handle stays unchanged. A configured owner must resolve real
attachment/current marker, visibility, effect and particle state; returning a
successful datum without those owners would not satisfy the contract.

The [fixture exporter](../tools/export_smg_prepared_effect_fixtures.py) reads the
immutable original TU1.1 bare and genuine allocated/equipped traces. Expected
inputs and phase outputs come exclusively from those traces, with original
cache/XBE and every case pinned. Original lower effect services explicitly
returned NONE; tests preserve that failure boundary and do not claim a visual
allocation. Non-preparation reload/NONE calls are counted separately in the
fixture metadata, rather than silently included as muzzle requests.

The [comparison](../tests/smg_prepared_effect_tests.cpp) passes22 original cases,
2,220 ticks,572 prepared-effect callbacks including6 empty requests, and566
projectile requests. It checks the exact effect/previous-handle/scalar words,
all52 barrel bytes, represented ammo/trigger/feedback state and shared projectile
RNG at the callback and each original phase. A second composition runs all599
allocated/equipped ticks through the real native Match and again over actual
loopback UDP:166 source effect requests in each, exact ABI, full live
player/unit/carried-weapon identity, unchanged represented postpass fields and
effect-before-projectile timing. The Match portion does not expose or assert
its internal RNG bank; the separate kernel comparison checks the source RNG.

Normal and ASan/UBSan runs, independent fixture exports and source snapshots are
retained under ignored `analysis/multiplayer/integration/smg-prepared-effect-20261008`.
The original effect request/visibility, equipped-SMG and FP clock/presentation
regressions pass. Native full-roster UDP clock checks retain21,304 sampled frames
and16,598 fractional frames with packet loss/reorder/duplicate recovery.

Visible muzzle flashes, ordinary effect/particle expiry, per-view particle
render submission and LAN effect-instance delivery remain incomplete. This
change establishes the actual causal request and its native owner seam; it does
not claim those remaining consumers are present or TU1.5 execution is covered.
The completed201923 checkpoint predates this change and is preserved.
