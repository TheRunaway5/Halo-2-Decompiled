# Native weather producers and shared random ownership

The normal multiplayer loop now runs the authored cold weather constructors and
per-view wind, plate and particle simulation over its retained resource banks.
It uses actual current cameras and clipped portal visibility. Ordinary matches
now render [Lockout snow](SOURCE_WEATHER_LIVE_SNOW.md),
[Warlock/Backwash rain](SOURCE_WEATHER_LIVE_RAIN.md), and
[Lockout/Warlock background layers](SOURCE_WEATHER_LIVE_PLATES.md).

`DesktopSession` retains one auxiliary random word across multiplayer BSP and
renderer handoffs. Its native CRT stream starts at 1. Actual native `time()` and
SDL timer values feed the existing original `146240`/`1462B0` seed recipe. Private
secondary initialization advances the CRT owner without replacing the auxiliary
word. No snapshot seed, saved random history or fixture ISA answer enters play.

The binding follows independently inspected original executable routines:

- `146240` occupies row 3 of `137C20`'s global initialization table, rather than a
  weather, sound, BSP or renderer constructor.
- `177C20` selects the deterministic word for effe flag 2 and the auxiliary word
  otherwise. World event skip/delay/duration selection follows that selector.
- `173FD0`'s two particle-system fields and `248620`'s location byte consume the
  auxiliary word. Native world and muzzle particle births now share this owner
  with weather rather than copying a fixed visual seed.
- `125260` and `126000` consume global auxiliary sound gain/pitch draws. `126000` then
  copies the post-pitch word into the impulse voice's own permutation stream.
  Native impulse playback retains that private copy. Loop selection keeps its
  separate native controller stream.

Original bytes and full disassembly are retained in
`analysis/map-world-features/weather-native-owner-source.json` and its `.asm`
companion. The disc executable SHA256 is
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
Existing sound arithmetic helpers use the separately selected update's address
labels; the original disc routines above establish stream ownership.

The native world-effect primary visual-event channel retains its existing
isolated compatibility state. It does not borrow gameplay's deterministic word
from render-batched playback: doing so before the original per-tick dispatcher
is connected would make gameplay random ordering depend on frame batching.
The original flag selector is implemented, but complete primary-event
scheduling and its gameplay binding remain open. This restriction does not
give weather or other auxiliary consumers a private copied stream.

The runtime accumulates its existing admitted native frame duration (the match
loop's current 100 ms clamp) into an absolute double. Only view 0 advances the
shared source weather clock/intensity. All local views consume its same elapsed
value, use their own camera banks and update their original equal particle
partitions. Wind runs only through view 0. This explicit native elapsed policy
does not implement the original Xbox vblank counter, game-time governor or
network-budget admission.

`SourceWeatherExecution` temporarily selects nearest/masked arithmetic, PC53 and
no SSE DAZ/FTZ, then restores the caller's entire floating environment. Native
x87 reads reserved bit 6 as 1 (`027F`), while the retained emulator reports `023F`;
precision, rounding and exception-mask fields match. The explicitly selected
calculated xemu 0.8.136 reciprocal-square-root profile reads actual operands.
Physical Xbox approximation precision is still unverified.

`SourceWeatherWorld::simulate_view` remains a producer-only API and reports
zero draw calls. The normal match now uses `visit_view` with actual plate and
particle consumers, preserving the source update/draw chain and stopped-state
suppression. Immediate packets read current owned rows; original programs,
authored textures and current native depth feed the GPU consumers. Finite
diagnostic counts/hashes remain distinct from the live draw itself.
Weather-modulated ambient gain now reads the existing simulation weather state.

The source comparison covers all seven retained original producer prefixes with
95,473 exact output words and 2,018 independently calculated ISA results. The
new producer-only seam is checked against those same independent original
banks. Borrowed muzzle-stream checks retain the authored first birth, leave the
primary stream untouched and accompany the existing 600 expiry/reallocation
lifetimes. Audio checks verify private post-pitch ownership and ensure packet
callbacks cannot have subsequent auxiliary draws overwritten by playback.

This does not reconstruct every Xbox startup consumer, title-to-game random
history, loop-controller lifetime, original asynchronous residency, map-script
startup, client weather-command replication or map-reset chronology. Existing
title/debug capture callers retain their explicit isolated-seed API. The full
map-world goal remains active. Existing launchers remain unchanged; a separate
test entry runs the frozen, verified visible-snow runtime.
