# Live Zanzibar computer and gate checkpoint

The native computer/gate path passed a visible SDL/Vulkan acceptance sequence
on 2026-10-12, in one-player and four-player local sessions. This verifies the
current native input adapter, shared device owner, ordinary machine controller
and native collision backend. Complete original player-action and Havok parity
remain separate requirements.

## Actual gameplay sequence

`tests/source_control_runtime_tests.cpp` loads the supplied Zanzibar cache and
derives its diagnostic placement points from the computer's authored model
bounds, the gate's posed collision surfaces and actual floor ray hits. It then
queues normal SDL mouse and keyboard events. It never calls the explicitly
admitted-control activation API or writes a device group/channel directly.

Both sessions passed these checks:

1. W moves the player toward the closed gate, then collision stops the player
   at signed distance `-0.293967` from the gate plane.
2. The idle action context selects the actual computer and its authored
   `zanzibar_gate_action` message (`14001E2F`). An E hold changes the shared
   `gate` group from value zero/flags one to value one/flags three.
3. The gate advances through 147 intermediate simulation ticks before
   settling at position one with zero channel velocity.
4. A second E hold cannot select or reactivate the consumed one-use control.
   The shared target changes exactly once.
5. W on the same route carries the player through the open gate to signed
   distance `+0.422876`. The final capture uses actual SDL look input to turn
   back toward the opened gate.

The one-player hold was 235 ms; the four-player hold was 275 ms. These are
observed native SDL durations, not invented source timing constants. The
existing source admission tests independently retain the authored hold boundary.
Both sessions exited normally, retained input focus and reported zero Vulkan
validation errors on the host RTX 5080 at 1280x720.

## Authored portal behavior

Zanzibar has **no authored door-portal associations** in the supplied BSP.
The live read-only portal-mask observer verifies that both mask and acoustic
banks stay unchanged before and after gate activation. It would be incorrect
to invent a portal transition for this gate. The all-21-map portal asset test
confirms that Gemini contains the six authored owners/eight portal indices in
the supplied Xbox set.

## Reproduction and evidence

The mutable shared build initially mixed HUD layouts: debugger observations
showed constructor/destructor class sizes 12904 and 13440. Those builds are
retained as failed diagnostics, not accepted results. A complete private source
snapshot and fresh build avoid changing headers during compilation.

The accepted source, build, screenshots, commands, asset hashes and logs are in
`analysis/map-world-features/control-runtime-snapshot-20261012/`. Its
`source-manifest.json` records the frozen source/header/shader/build inputs.
`live/verification.json` and `live/report.json` record both successful runs,
including zero source drift. The frozen acceptance executable's SHA-256 is
`89fd00bde0184886c6fb860afdb19f7396e5e35a93f129b18a397299b7783fd4`.

```sh
cmake --build analysis/map-world-features/control-runtime-snapshot-20261012/build \
  --target source_control_runtime_tests -j 6
python3 analysis/map-world-features/control-runtime-live/check.py \
  --build analysis/map-world-features/control-runtime-snapshot-20261012/build \
  --source-root analysis/map-world-features/control-runtime-snapshot-20261012/source \
  --output /tmp/halo2-zanzibar-live-check
```

Use a new output directory; the runner preserves frozen checkpoints. A visible
SDL display, the supplied map/shared assets and owned TU1.5 executable are
required. Nine focused checks also passed from this build: unit-center fixtures
and guards, control-target fixtures and guards, device world, actual UDP device
bank transport, rendered machine scene, and portal fixtures/assets.

The original evidence and bounded scalar kernels remain documented in
[MAP_WORLD_FEATURES.md](MAP_WORLD_FEATURES.md),
[SOURCE_UNIT_CENTER.md](SOURCE_UNIT_CENTER.md) and
[HUD_ACTION_PROMPTS.md](HUD_ACTION_PROMPTS.md). The native adapter still uses
ordinary native unit-eye inputs and native owner enumeration. Complete live
`CAFC0`/`BB050` registration and animation/contact services, complete original
effect/spatial sound services, physical-controller checks, rendered LAN control requests, late
joins/resets through that input path, and the remaining maps stay open. The
all-map world-effects goal remains active.

The subsequent [device-event checkpoint](SOURCE_DEVICE_EVENTS.md) adds and
verifies the three authored Zanzibar button/gate PCM requests in one/four local
views, real UDP playback and a four-view moving late join. Its source snapshot
and binaries are separate from the preserved interaction-only checkpoint above.

## Current normal-launcher integration

The [combined normal-launcher delivery](SOURCE_WORLD_LAUNCHER_DELIVERY.md)
supersedes the earlier isolated runtime. Its native input fixture also covers
both Containment switches and Relic's switch, with one and four native players.
All eight cases exercise prompts, held commands, shared groups, gradual motion
and repeat rejection. Containment/Relic use diagnostic collision-free placement,
including airborne starts; their walking routes and SDL control inputs are not
established by that fixture.

The Relic correction retains original `B8D30`'s missing named-marker count of
zero. Its node-zero fallback matrix does not imply a successful named lookup.
Unchanged original routines confirm this for all four authored controls, so
`1078F0` skips its side restriction when the named use marker is absent.

The latest SDL replay explicitly selects X11 on its Xvfb display and records
the actual SDL driver. An Xvfb wrapper alone was insufficient: the inherited
Wayland environment still selected the user's desktop and allowed focus
contention. Earlier accepted runs remain evidence of native SDL/Vulkan behavior;
their wrapper alone does not establish display isolation.
