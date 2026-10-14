# Tombstone team initialization follow-up

The map addition exposed an existing Team Slayer initialization gap on both
Lockout and Tombstone in the installed native build. The live catalog therefore
adds only Tombstone Fiesta. Ordinary Slayer/Team Slayer map entries remain
staged in `master_server/examples/matchmaking-tombstone.json`.

The source fix adds these complete authenticated TU1.5 bodies to the existing
native instruction lowerer and dispatch owner:

| Actual entry | Reached path | Original responsibility |
| --- | --- | --- |
| `25BB50` | `159FF0 → 15A43B`, indirect engine callback | AL-only team admission return; original callee stack cleanup |
| `15A2C0` | `15A470 → 15A4AB` | Team mask from sixteen game-option profiles and available/maximum teams |
| `15A1A0` | `15A470 → 15A4B0` | Original team/color mapping and profile-update request |
| `1CB030` | `15A1B8`, engine vtable slot `68` | Original default first-team selector, including OR flags |
| `154EF0` | `15A253` | Actual player-pool iteration and existing `14E9E0` profile consumer |

The original image is the verified English TU1.5 executable, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
No instruction bytes or dependency returns are substituted. The small original
callbacks are complete source bodies, not native unconditional-success shims.
The generated source merge preserves other concurrent additions. A private
optimized build uses the installed spectating build's immutable objects and
headers, with only the selected physics-support object replaced.

`oracle_engine_team_admission_tu15.py` runs 256 original callback controls.
`oracle_engine_team_reset_tu15.py` runs 32 complete original pre-spawn resets,
varying team limit and declared game-option occupancy with an empty actual
player pool. Native comparisons match instruction paths, every GP register,
flags, stack cleanup and all mounted memory bytes. These controls cover the
pre-spawn reset; they do not certify every live-player profile rewrite.

The staged native patch creates four players and renders four Vista HUD views
on Tombstone at 3440×1440. This capture is **one frame with zero simulation
ticks**. A four-frame gameplay probe then reaches the separate missing live
physics owner `13B6F0 → 1816AA → 181CB0`. Full ordinary-match simulation remains
unfinished. The patched candidate is not selected by the launchers or required
by live Fiesta, and the broader Tombstone catalog is not activated.

Evidence, source/object hashes, primary fixtures, negative controls and captures
are retained under `.local/tombstone-playlists-20261013`. The registered
`original_tu15_engine_team_admission` and `original_tu15_engine_team_reset`
tests share the native `engine_team_admission_tu15_tests` runner.
