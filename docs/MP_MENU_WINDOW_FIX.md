# Menu-to-match window and player-count regression

The application now keeps a single SDL window across the frontend and match
renderer lifetimes. Its size, position, hidden/visible state and fullscreen
state are retained. SDL shutdown occurs after the application session releases
the window. Dedicated and inspection paths construct no video window.

The existing frontend selects one local profile and has no second-user join
producer. Starting its Split Screen lobby now launches one local participant.
The previous unconditional two-player request created a participant that the
user had not joined. Direct one-to-four-player multiplayer launches remain
available; a complete original controller/profile join flow remains unfinished.
The original valid-user-count caller is described in
[the original lobby source evidence](MP_LOBBY_SOURCE_RESEARCH.md).

The real application regression test injects declared keyboard selections into
the authored menu: Split Screen, the selected profile, then Start Game. An SDL
observer records every window creation/destruction and injects a test resize.
The previous immutable build creates/destroys two windows, resets geometry and
launches two players. The fixed application creates one window, retains its
identity and resized dimensions until shutdown, and renders one local player.
The source test also verifies the reported one-profile behavior against real
menu content. Muted Vulkan-validation captures have no validation errors.

- [Previous build reproduction](../analysis/multiplayer/integration/user-gameplay-repro/menu-red-final/report.json)
- [Fixed rendered transition](../analysis/multiplayer/integration/user-gameplay-repro/menu-green-validation/report.json)
- [One-profile failing test before the fix](../analysis/multiplayer/integration/user-gameplay-repro/one-profile-red.log)
- [Regression driver](../tools/test_menu_window_flow.py)

The ordinary original lobby caller supplies countdown/minimum10/3; its separate
automatic caller supplies3/3. Their profile/session/controller/time ownership
is not yet executed. This change supplies no replacement timer and does not
claim the full original lobby, countdown or loading-screen flow is complete.

The reported player-damage failure remains unresolved: a direct Lockout test
with map collision retained damaged opponents through six clear authored spawn
pairs, but this does not reproduce the user's rendered menu-launched symptom.
Third-person actions/locomotion, explosive collision/destruction, lift contacts
and full physics integration remain unfinished. The available Map Pack game
executable is update1.1, with no verified later local update. This is not a claim
of final-patch gameplay parity.
