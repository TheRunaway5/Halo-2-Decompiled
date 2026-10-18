# Authored scenario lights in the opening

The opening has two lights in the scenario's `+E8/+EC` placement block and
`+F0/+F4` palette: `x01_light_prophets` and `x01_light_arbiter`. They are
independent placed lights. The late Chief, Armorer and fill lights are attached
to the Cairo armory scenery. Loading attachments alone misses the early pair.

The complete original TU1.5 `C29F0` loop and `C2550` constructor now have a
separate native module under `src/pstencil/`. Its generator authenticates all
248 instructions against the user's unchanged TU1.5 executable. The actual
caller at `191E03/191E08/191E0B` reads game-options `+8` into EAX. The loop reads
the authored placement/palette; original allocation, Euler direction, shape,
spatial registration and light-state producers retain ownership. No light
position, color, radius, ready flag or allocation result is supplied.

`scenario_lights_tu15_tests.cpp` creates a cold actual opening world, uses that
caller input, and observes both correct definitions and placement ordinals.
The saved native before/after bank is then executed by
`tools/pstencil/oracle_scenario_lights_tu15.py` with unchanged Xbox instructions.
The current comparison executes 23,820 instructions: all 662 ordered data
stores outside the source call stack, final memory, GP registers and EFLAGS
match exactly. No service is replaced and no instruction is patched. A
deliberately changed expected store is rejected. These are CPU constructor
checks, separate from in-game image evidence.

This also corrects an earlier field interpretation: light record `+8` is the
scenario placement ordinal, with NONE for attached/effect lights. It is now
named `scenario_placement`. The native projected renderer consumes the whole
original shape owner and accepts placed lights. The older sphere-only adapter
continues to reject a placement whose separate shape owner it cannot consume.

The private `campaign-v18` replay creates both placed lights and observes
positive native fades/PVS visibility in the early scenes. Its obsolete
placement rejection still prevents their GPU draws. `campaign-v19` removes
that rejection and completes all13 scenes with zero script/Vulkan errors.
Both early lights are selected and collect caster geometry, but that version's
native command bank remains empty. `campaign-v20` captures actual44710/40E80
inputs and outputs. All four unchanged Xbox comparisons match the native
stores, memory and registers: the shader and geometry gates accept the parts,
then the category owner rejects their cleared color bits.

The missing producer was the actual main view's four-column queue handoff.
`campaign-v21` publishes those completed queues before the auxiliary light
queries. Both early lights then produce real packets, up to101 for the Arbiter
light. Its full2560×1378 replay reaches all13 scenes with20 facial-capable cues,
zero script/Vulkan errors,14 positive same-frame light/shadow image pairs and
zero shadow brightening. All20 Original restorations are byte-exact. The first
early positive target1150 has2,398 shadow pixels; target1400 has14,402.

The production `pstencil::CampaignRenderer` now accepts the current complete
campaign main frame. The sphere bridge reuses that completed main state,
preserving the full environment, particles, cull/commands and owned base draws.
A fresh coherent current-engine build passes the full opening and14 positive
light/shadow image pairs at2560×1378, with20 exact Original restorations and
zero script/Vulkan errors. `Halo2-pStencil-Campaign` launches that tested build.
Remaining material response, pacing and recording gates are tracked in
[opening campaign](OPENING_CAMPAIGN.md).

Reproduce the comparison with the repository's Unicorn-enabled Python:

```sh
.tools/python/bin/python tools/pstencil/oracle_scenario_lights_tu15.py \
  --case analysis/pstencil/opening-campaign/campaign-v18/scenario-test-output \
  --output analysis/pstencil/opening-campaign/campaign-v18/scenario-original-comparison.json
```

Primary references are the local unchanged
[scenario loop](../../analysis/multiplayer/title-update-1.5/decompiled/functions/000c29f0.c),
[constructor](../../analysis/multiplayer/title-update-1.5/decompiled/functions/000c2550.c),
and [placement shape selector](../../analysis/multiplayer/title-update-1.5/decompiled/functions/000c3a20.c).
