# Original particle sprite context and source-pool rendering inputs

`source_particle_sprite_context` restores the read-only setup in original
TU1.5 `22AE16..22AEC4`. It follows the postprocessed bitmap selector, selects
the authored atlas sequence, and supplies the first bitmap's signed width/height
ratio. Both world and muzzle draws now supply that context to the existing sprite
kernel. They also perform the shared-value prelude at `22AEC4..22AEFF` before the
generic emitter/particle producer. These operations do not advance simulation
or consume RNG.

Untouched original instruction prefixes agree with **3,655** native contexts
across all **21 Xbox multiplayer maps**, including **282** non-square bitmap
ratios and **3,138** atlas selections. Three existing Wraith spill definitions
remain outside the sprite loader's supported shader-parameter shape. There are
no new context-loader boundaries. The selector-NONE and bitmap-no-data branches
are recovered from the original code but are not exercised by those 3,655 cases.
This comparison covers the context prefix, not the preceding GPU binder or
the full original sprite callback.

The original view-admission helper `136E10` is also registered: **40 instructions**
reached by `1372C5`. **1,024** original calls compare complete GP/SIMD/flags,
instruction paths, ordered writes and retained banks, including row exhaustion
and every bit in the 128-bit row. The map module now contains **188 bodies /
14,808 instructions**; all 187 previous bodies remain unchanged.

`source_particle_render_state_tu15` is a separate, read-only bridge from actual
source system/location/emitter/particle pools into the existing generic render
producer. It preserves full salted identities and chain order, rejects foreign
locations and stale systems, and uses retained effect controls, source clock,
particle seeds, bases and velocities. It does not create particles or mark them
as drawn. This API is implemented and tested separately; the normal render loop
does not yet consume it.

For Backwash, a declared native camera and the complete original admission
owners retain 14 particles through **240 explicit frames**. The original
`1784B0` runs over the source-created bank and agrees with all **14** native
callback inputs from its two systems: position, velocity, scale, color, value
slots/mask and particle identities. Original A/R/G/B callback storage is
explicitly mapped to the native R/G/B/A interface. The graphics callback is a
declared void input recorder; no original instruction or game-service result
is replaced. The fixture test also rejects stale/foreign identities and verifies
unchanged read-only pools.

This probe accumulates **478** unconsumed transparent commands below the original
1,024-entry bound. It is not continuous transparent-queue consumption, ordinary
glowbug rendering, original complete frame scheduling, split-screen particle
history, network history or Xbox framebuffer acceptance. The source-pool bridge
still needs the normal frame/admission/graphics consumer.

Evidence: `analysis/map-world-features/swarm-lifecycle-20261014/`, including
`sprite-context-original/report.json`, `visibility-row-original/report.json`,
`source-render-inputs-original-v2/report.json`, the retained first comparison's
explicit color-layout mismatch, the 240-frame probe and exact build receipts.
Only generated numeric pool state and callback outputs are in the fixture;
owned Backwash assets are required for the native test.

[Normal launcher delivery](SOURCE_WORLD_LAUNCHER_DELIVERY.md)
