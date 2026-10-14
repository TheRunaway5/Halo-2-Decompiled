# Two-player split screen

**Settings → Video Settings → 2-Player Split Screen** selects the camera divider.
The same row is available in pause Video Settings and changes the layout
immediately:

- **Horizontal** (default): Player 1 above Player 2, each using the full width.
- **Vertical**: Player 1 on the left and Player 2 on the right.

The choice persists as `pc_two_player_split 0|1` in `menu.cfg`. Existing settings
without the entry default to Horizontal, and Video Defaults restores Horizontal.
One-player and three/four-player layouts retain their existing behavior.

Ordinary gameplay passes the preference into the shared HUD/world viewport grid.
The world camera uses each selected physical rectangle's aspect for its FOV;
crosshairs, hands/weapon, HUD, scoreboard, scopes, fog, water and particle/effect
passes retain the same per-view boundaries. The choice does not change player
input assignment or simulation/replicated observer state.

Reference callers that omit the layout preference keep the original automatic
aspect-based `hud_viewport` behavior, including retail viewport fixtures. Native
presentation explicitly supplies the saved preference. The recovered original
camera and grid routines are unchanged.

`hud_tests` checks both player orderings, safe gutters, other player counts and
gap-free physical edges at even/odd drawable sizes. The settings/pause tests
check persistence, defaults and scrolled mouse activation. The source rectangle
GPU suite also covers selected horizontal/vertical layouts with scoped bloom
and scope-copy passes at odd and ultrawide sizes. The real multiplayer runtime
test changes Video Settings during a four-frame two-player match and checks
both submitted rectangles and each camera's actual physical aspect.

Local validation and rendered examples are recorded in
`analysis/split-screen/video-settings-20261014/`. Both root launchers select the
verified `.local/split-20261014/halo2_native` package.
