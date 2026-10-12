# Retained shield and damage region delivery

The native source-object world now routes `D8070` shield depletion and `D73E0`
shield reset through complete original `BB6E0` and `16F0E0` bodies. Explicit
damage-state requests use complete `BB5E0`. The construction generator emits
these bodies and its ownership wrapper; the generated death-world registry
routes their actual reached calls through the existing retained ABI and bank.
There is no replacement region mutation or successful lower callback.

The [source research](DAMAGE_REGION_LIVE_DELIVERY_HANDOFF.md) authenticates the
complete TU1.1/TU1.5 bodies, exact stack arguments, signed component lengths,
variant mapping, fallback selection and attachment/physics consequences. The
original stock Spartan and Elite variants contain zero authored damage-state
records. Requesting a different damage state must therefore preserve the
original no-candidate behavior. A requested flag is not blindly stored in every
region, and a looping effect is not invented for an absent authored record.

`multiplayer_object_runtime_tests` now exercises both flag set/reset and all
four Spartan regions' state0..5 requests after an actual native source biped
birth, and again for each of the actual two-player prepared Match bipeds. These
calls use the same retained object bank and completed body/physical components
as gameplay and presentation. They preserve the complete40-byte region bank,
matching the stock original fallback. The confirmed-fatal Match path also
passes the previously missing `D8070→BB6E0→16F0E0` call.

The independent [region capture tool](../tools/measure_source_damage_region_delivery.py)
runs unchanged original instructions on declared valid Spartan/Elite region,
variant, attachment and physical-component inputs with the original cache.
Its510 cases all return through the actual composed native dispatcher, matching
220,130 instructions,36,205 ordered stores, complete GP/SSE/x87 state and951,828
retained bytes. Cases cover all valid region ordinals and the all-regions
selector, requested flag bits/masks, variant0/NONE, explicit state requests and
the already-depleted/zero-maximum depletion branches. These are controlled
diagnostics, not naturally admitted hits.

The vitality capture's deeper composition now declares valid model-region
lengths/offsets, default variant selection, current base permutations, NONE
looping effects, no attachments and no physical component. Its earlier random
region lengths cannot validly enter the newly owned copy/selection routines.
The earlier raw and composed captures are retained independently. The refreshed
3,530-case composition has3,516 completed cases, including2,801 continuous
charge/drain phases; its14 remaining real callee boundaries are `DD5B0` and
`179B90`. The54 previously excluded nonzero section masks still refer to
nonexistent authored permutations and are recorded as excluded inputs.

The selected CMake checks use the versioned captures under
`analysis/damage-shields-goal-20261011/live-region-delivery/`: `regions-v2`,
`vitality-composition-v3` and `composition-v3`. Repeated original region and
vitality captures were byte-identical before the call-free fatal state
initializer was added; both sets remain available. Native execution is compared
through the same `source_run_death_object_phases_1_1` ownership routing used by
the retained live world, including full register state and strict unknown-owner
stops.

This increment does not finish authored stateful variants, looping-effect
allocation, physical shape changes, all damage sources, shared host/source
vitality authority or shield overlay pixels. Fatal cleanup is tracked separately
in [its source handoff](DAMAGE_FATAL_CLEANUP_LIVE_DELIVERY_HANDOFF.md). Those
boundaries remain part of the active goal.
