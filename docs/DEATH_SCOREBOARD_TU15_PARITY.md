# Death screen and scoreboard TU1.5 acceptance

The requested scope is every mode, including campaign, all 39 original
multiplayer presets, and local/network/split-screen presentation. This checklist
does not close from a layout fixture or a submitted GPU frame. Full 1:1
acceptance remains open until the runtime and reference rows below are closed.

Authority is the original Xbox TU1.5 executable with SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Prepared MCC assets supply native runtime data. Vista artwork is not the
behavioral or visual authority for these changes.

| Mode family | Source kind | Original presets | Required display checks |
| --- | ---: | ---: | --- |
| Slayer | 2 | 9 | Decimal score; lives and elimination gates |
| Oddball | 3 | 6 | Time score; possession status |
| Juggernaut | 7 | 5 | Decimal score; role status and restricted respawn |
| King of the Hill | 4 | 5 | Time score; hill status |
| Capture the Flag | 1 | 4 | Decimal score; team/flag status |
| Assault | 9 | 5 | Decimal score; bomb/round status |
| Territories | 8 | 5 | Time score; team/region status |
| Campaign solo | Campaign | Separate | Death/revert/checkpoint presentation |
| Campaign co-op | Campaign | Separate | Failure reason, teammate view, safe respawn, shared checkpoint |
| Custom Fiesta/Infection/Defusal | Extension | Separate | Preserve extension rules; original shared display regressions |

## Source owners and repairs

- `15BB50` selects time text only for serialized kinds 3, 4 and 8. The native
  adapter now delivers the actual engine kind and round settings instead of
  hard-coding Slayer. Extended final comparisons use the authored variant flag.
- `127710` publishes Back's held counter at processed input `+1D`, consumed by
  `15E70D`. Every bound local controller now reaches its own scoreboard.
- `127965..1279B9` owns right-stick Y's dead zone and signed scaling. The native
  scoreboard receives that physical axis independently of gameplay remapping.
  `163360`'s clamped scroll also writes back, preventing accumulated overscroll
  from delaying reversal and invalidating stale restore preparations.
- Player `+164` is raw respawn ticks; `+170` is displayed seconds. They are
  exported separately. The automatic scoreboard consumes the seconds field.
- The additive `15E6BC` observation captures actual counter-gate inputs before
  object death, late respawn and the clock increment. The original instructions
  still execute. Captured source values travel with network snapshots.
- `161610` caches `1618B0` status before object updates. An additive `15ED4F`
  observation retains those local results; remote players use the same complete
  selector at that phase with a separate bounded caller frame. Arbitrary
  `Match::state()` reads only copy the retained result.
- `161BA0` resolves the authored status table and locale tokens. The normal HUD
  suppresses ordinary winning/tied/losing text; the full scoreboard retains it.
  Current draw-time lifecycle and match-over fields remain distinct from the
  earlier status cache and counter phase.
- `25B53A..25B5BE` draws centered status text at safe-bottom minus 22, font 6
  for one view and font 5 for split-screen, with original species color,
  shadow, metrics and clipping. Dead players can receive this separate text
  without drawing their living weapon/shield HUD.

## Verification matrix

| Check | Evidence / test | Acceptance boundary |
| --- | --- | --- |
| Source score/wins/layout | `multiplayer_scoreboard_modes_tests` | 333 unchanged original cases through native draw options |
| All original presets | `multiplayer_scoreboard_modes_native_tests` | Actual constructor/font/layout; declared API roster states |
| Scroll transport | `oracle_scoreboard_scroll_tu15.py` | All 65,536 signed SDL axis inputs; 269 retained probes |
| Status selection/import | `source_player_status_tests` | 896 unchanged selector cases; 23 authored locale rows |
| Status text geometry | `source_player_status_tests` | Original draw rectangles and real-font glyph/shadow submission |
| Controller ownership | `multiplayer_scoreboard_controller_runtime_tests` | Actual SDL Back holds/releases and GPU submission in four views |
| Death/countdown/network | `multiplayer_death_cycle_tests` | Actual SMG fatal hits, raw/seconds publication, UDP, respawn/HUD restoration |
| MCC locale | `mcc_scoreboard_locale_tests` | Original and prepared MCC labels/status bank comparison |
| Launcher delivery | Qualification report for the selected runtime | Must be rebuilt and exercised after these source changes |

For every family retain held, automatic death and final display; team/FFA as
authored; one through four local views; both two-player split orientations;
4:3, widescreen and ultrawide; local, host, remote and late join; countdown,
finite-life exhaustion, observer, sitting-out, round transition and respawn.
Full played-match/camera/corpse coverage is separate from declared API states.

## Open acceptance

- Original `69E80` network-quality bars require original session admission and
  four channel metrics. Native RTT/loss/upload values are not a proven mapping.
- Native ordinary status text currently receives opacity one. Full original
  per-view HUD opacity/fade behavior needs its actual local presentation owner.
- Campaign status/failure text uses different owners from multiplayer
  `161BA0`. Campaign co-op still lacks the complete shared player, safe-respawn,
  checkpoint/BSP and network session integration described in
  [the campaign checklist](CAMPAIGN_COOP_DEATH_IMPLEMENTATION.md).
- No complete matched Xbox TU1.5 framebuffer/death-to-respawn capture matrix
  exists for every mode, split configuration and network role. Native component
  comparisons and GPU submission do not replace that evidence.

Validation uses `tools/run_tests.py` inside an owned `tools/run_test.py`
session. Small commands, source hashes and diagnostics are retained under
`analysis/death-scoreboard-all-modes-20261025/`; disposable builds and captures
must be removed when validation finishes. Any retained installed runtime must
be recorded separately with its path and size.
