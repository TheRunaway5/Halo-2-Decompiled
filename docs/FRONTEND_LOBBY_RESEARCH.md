# Pregame lobby layout and flow

The local-host lobby now binds the original runtime fields instead of drawing
every authored placeholder. The host's party-leader label no longer overlaps
Start Game; unused speaker slots are hidden; the occupied row has its original
emblem/bar/name order. The selected map, game icon, quick options, player count,
status message, protocol heading and footer follow source policies. Moving
focus to Game Setup retains the multiplayer pane.

Game Setup uses the original semantic row order and selected-row preview.
The map selector uses the original catalog order, descriptions, previews and
new-profile unlock condition. Its accept and cancel both reveal the retained
lobby. Setup and its children replace the original channel's current page;
the setup page is not an extra Back destination. A fresh co-op switch selects
source map105 (Cairo Station), and difficulty selection updates the lobby
without launching. One-player co-op retains Start focus, ignores activation,
and pulses the original readiness warning.

Setup23 and map206 now use the original tint-and-blur boundary over the lobby.
First entry follows the authored250ms track; replacement keeps blur at full
strength. The generic black dimmer and duplicate initial tint are omitted for
these proven routes. Text opacity uses the original cumulative-clock cosine
calculation, including authored text flag4 and the lobby warning bit.

The normal main-menu world now receives the original scene color filter under
the lobby and its setup/map pages as well. Original `2234E0 → 13870` gates this
by the main-menu render context, with no screen-ID whitelist; live screens
6,14,23,206 retain the same captured filter and full scenario weight. The old
initial generic blue-alpha quad is removed, while the separately proven
channel tint and blur remain after lower UI. This restores the original dark
navy world treatment without moving the authored widgets. Source and runtime
evidence are under `analysis/frontend/lobby-world-audit/` and
`analysis/frontend/title-fog-state-audit/`.

## Evidence

Authority is the supplied Xbox executable SHA256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`
and mainmenu cache SHA256
`1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b`.
Authored positions, fonts, colors and dimensions were not visually fitted.

- [Header, pane and visibility policy](../analysis/frontend/lobby/HEADER_SOURCE_PROOF.md)
- [Game panel and original catalog](FRONTEND_LOBBY_GAME_PANEL_RESEARCH.md)
- [Roster binding](../analysis/frontend/lobby/roster-audit.md)
- [Setup destinations and channel replacement](../analysis/frontend/lobby/SETUP_SOURCE_PROOF.md)
- [Map list admission and focus](../analysis/frontend/lobby/maps-audit.md)
- [Readiness](../analysis/frontend/lobby/READY_SOURCE_PROOF.md)
- [Warning opacity and Start activation](../analysis/frontend/lobby/WARNING_FOCUS_SOURCE_PROOF.md)
- [Popup submission and transition replay](../analysis/frontend/lobby/POPUP_BACKDROP_SOURCE_PROOF.md)

Original Vulkan screenshots and read-only live state are retained under
`analysis/frontend/lobby/reference`. The owned disposable emulator and input
device were closed and inputs released; `reference/cleanup.json` records this.
Other reference sessions were untouched. Original System Link stopped at its
no-network-cable message; that lobby's policy comes from executable evidence,
not a claim of live original System Link capture.

## Validation and runnable build

Eight focused lobby tests and six related frontend regressions pass. The old
launch regression skipped actual profile sign-in; it was corrected to enter
through the normal profile flow after the source-backed Start guard exposed
that invalid setup. `delivery-tests.log` retains the initial failure and
`regression-recheck.log` the corrected passing regression.

Thirteen native captures exercise the normal SDL route through Split Screen,
System Link, Game Setup, map focus/accept/cancel, and co-op. Captures include
640×480,1280×720,1280×800 and1680×720. All have the expected screen sequence,
zero Vulkan validation errors and black pixels outside the centered4:3
viewport. The first border-audit formula rounded width before calculating
the16:10 origin; that audit error was corrected against the renderer's actual
scissor calculation and all saved captures revalidated. No renderer adjustment
was needed. `delivered/report.json` records all13 captures and checksums.

The tested executable is `build-frontend/halo2_native`, SHA256
`177f66240d098b191b21f1b91ff11eda480bc01e622db6388b2b48124887a8ad`.
Use `build-frontend/Halo2-Title-Launcher` for this verified menu build; its
`--launcher-info` confirms that runtime. During validation, another workflow
published the shared root Title shortcut to its separate keyboard/mouse
release. That newer shared artifact was preserved, rather than replaced by
this build. `analysis/frontend/lobby/delivery.json` records both paths and
the source hash comparison.

Screenshots: [lobby16:9](../analysis/frontend/lobby/delivered/wide.png),
[setup](../analysis/frontend/lobby/delivered/setup.png),
[map selection](../analysis/frontend/lobby/delivered/map-focus.png),
[co-op](../analysis/frontend/lobby/delivered/coop.png).

## Remaining boundaries

This does not establish complete framebuffer parity. The normal main-menu
scene-filter admission is now established, but complete world timing, haze
history and scene pixels are not synchronized with the original captures.
The outgoing Setup widgets during the250ms map replacement are not retained;
only the verified backdrop continuity and settled page are implemented.
Other dialog contexts still require their own channel evidence. Current
native local-host state exposes one signed-in player; remote party occupancy,
live online services, campaign progress persistence and co-op gameplay are
not supplied by these layout changes. The aspect tests preserve the original
4:3 composition; they do not establish extended widescreen world projection.
