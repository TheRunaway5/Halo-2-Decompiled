# Optional F1 debug panel

The user requested Phase-Distorter's optional F1 panel with all applicable Halo
controls. The reference is `/home/eric/Documents/GitHub/Phase-Distorter/cpp/`:
`include/eb/debug_panel.hpp`, `src/debug_panel.cpp`, `include/eb/game_debug.hpp`,
and `docs/debug-tools.md`. The port follows its session-only settings, captured
input, read-only observations, queued commands, teleport search, snapshot list,
and deletion confirmation. These controls are native workbench extensions;
they do not claim to reproduce retail Xbox cheat flags.

F1 opens/closes the window. Escape closes it. The game and music keep updating,
but menu, camera, gameplay keyboard, mouse and controller actions are captured
while it is open. Frontend idle activity is refreshed so an attract movie does
not interrupt the panel. Closing it restores the prior text-input and relative
mouse modes. The multiplayer scoreboard now uses F3. `--debug-panel` opens the
optional window initially, including deterministic screenshot runs.

| Phase-Distorter control | Halo implementation |
| --- | --- |
| Infinite HP | Incoming damage is gated for the selected controlled player's health and shields. It does not resurrect dead units. |
| Infinite PP/resources | Active-weapon ammunition is restored to the actual weapon's authored capacity/reserve before and after weapon updates; firing/reload clocks remain live. The stock SMG also updates its authoritative magazine fields. |
| Noclip | Selected-player movement bypasses collision, using the authored movement speeds. Jump ascends; crouch descends. |
| Enemies ignore | Campaign target selection excludes the selected player. Existing projectiles remain in flight. |
| Maximum outgoing damage | The selected player's damage request has a native minimum of1,000,000 before the existing armor/vitality handler. |
| Teleport | Searchable scenario player starts, restricted to the loaded BSP. The actual player position, velocity, facing and pitch are updated between ticks. |
| Party editing | The panel selects among existing local player slots. Player count/profile selection remains in the original frontend; live roster editing is unavailable. |
| Named snapshots | Up to16 named, in-memory campaign snapshots use the existing checkpoint state, including VM, world/BSP ownership, actors, projectiles, RNG, presentation and sound classes. Loading preserves the campaign checkpoint that existed when the snapshot was saved. Save, load and confirmed delete are supported. |

Gameplay controls default off, are filtered to local participants, and are
currently disabled in LAN sessions. Campaign AI controls do not apply to local
multiplayer. Multiplayer snapshots remain unavailable because a Simulation
checkpoint alone cannot restore the match's player/object allocators, scores,
round state, first-person owners and network state. Disk serialization and
cross-process snapshots are not implemented. The panel states these limits
instead of presenting inactive buttons as working features.

The renderer creates Vulkan graphics resources only when the panel opens,
submits the overlay after game/UI composition, retires its backend before a
swapchain rebuild, and recreates it against the new pass/image bank. Closed
panels submit no overlay graphics. Hidden input does not accumulate in ImGui;
visibility changes clear both SDL backend and ImGui key/mouse state. The pinned
Dear ImGui1.91.9b core and SDL2 backend come from Phase-Distorter's verified
vendor files; the Vulkan backend is from the same upstream commit. License,
provenance and SHA256SUMS are in `external/imgui/`.

Verification is recorded in
`analysis/frontend/widescreen-debug-20261009/` and the associated
`analysis/frontend/title-camera-matrix-audit/widescreen-debug-*.log` files.
CPU regressions cover default-off behavior, firing cadence and ammo reset,
incoming/outgoing damage, AI exclusion, noclip/wall crossing, authored teleport
validation, snapshot restore/checkpoint retention/deletion/limits and selected
multiplayer slots. Actual SDL queue tests cover input release and F3. The
Vulkan panel probe verifies F1/Escape, text ownership, stale-input clearing,
visible overlay, byte-identical pixels after closing, resize and resource
cleanup with zero validation output. An isolated native multiplayer run also
passed156 observed simulation ticks with an attached idle controller, F1 input
capture, held-input clearing on close, and fresh-input resume. Its actual
overlay screenshot and input trace are retained under `panel/` in the evidence
directory; the shared-desktop attempt ended before rendering and is not a pass.
Native launcher screenshots are separate
presentation evidence; these checks do not prove whole-game or whole-frame
Xbox parity.

The final edge-extension build repeated the Vulkan probe successfully. Evidence
is retained in `analysis/frontend/widescreen-debug-20261009/panel-edge-texel-center/`
and `analysis/frontend/title-camera-matrix-audit/debug-panel-edge-texel-center-gpu.log`.
The disabled overlay again restores exact pixels, including after resize and
renderer replacement. The Display/CRT checks exercise separately contributed
display controls and do not change the debug gameplay control scope above.
