# LAN and split-screen match customizations

The original match catalog contains 92 parameter definitions across seven
engines: Slayer, Oddball, Juggernaut, King, CTF, Assault, and Territories.
Both local modes use Setup's Change Rules and Quick Options pages. Match,
player, team, game, equipment, and vehicle settings are applied as ordered
writes to the complete 304-byte source variant. Stock matches use the native
source engine rather than the separately bounded custom-mode adapter.

The October 28 changes remove the retained LAN lobby's CTF/Assault-only Start
restriction and carry the same stock-engine routing through interactive join,
runtime, and postgame return. Changing between free-for-all and team games
retains actual profiles and admissions; the authenticated profile supplies the
committed team when a later game needs one. No membership is synthesized.
New LAN lobbies also accept additional local controllers through the normal
profile picker before transport admission, allowing the selected local roster
to launch together. Retained sessions keep their actual admitted seats.
Fiesta, Infection, and Defusal retain their separate custom-mode lifetime and
explicitly require a new LAN session when selected from a retained stock lobby.

Menu state retains a received source buffer alongside its ordered overrides.
Pickers, lobby team policy, Start, named copies, and preferences all read the
same effective variant. A raw base may require an override before it passes
validation; admission validates the effective buffer. Browsing another engine
and backing out preserves the active game; opening its focused preset editor
commits that preset coherently. Midship and Waterworks no longer have the
obsolete menu refusal based on their default energy weapons.

## Newly authored custom modes

Fiesta Slayer, Infection Slayer and Bomb Defusal use their native custom match
authority when launched from MCC menus, including after authenticated LAN
configuration discovery. They are excluded from the stock Vista lifecycle
whose original variant buffer cannot identify these native extensions.

Their Change Rules and Quick Options pages expose settings consumed by the
custom runtime. Empty original categories and unsupported or fixed controls
are omitted. The authored category buttons use the resulting labels and hit
indices, so filtering does not leave invisible active rows.

| Mode | Editable settings | Fixed defining rules |
| --- | --- | --- |
| Fiesta Slayer | Score, time, player limit, lives, respawn and suicide penalties, grenades, supported damage controls | Random primary on spawn, no secondary; free-for-all |
| Infection Slayer | Survival time, player limit, respawn and suicide penalties, supported damage controls | Factions, human/zombie loadouts and zombie traits, conversion, unlimited lives, survival/all-infected endings |
| Bomb Defusal | Plant window, player limit, friendly fire, supported damage controls; Mode Settings adds fuse, plant and defuse times | Two sides, one life per round, first to 13, side switch, loadouts and map-equipment policy |

Timer choices pass the real Defusal rule validator; the fuse exceeds the
defuse time. An existing valid custom tick value remains selectable and keeps
its exact integer value. The original 150-second plant window is also shown
correctly despite not being an authored stock time choice. Named copies and
preferences retain the native writes and complete original source base.
Custom Player Limit displays the effective capacity, and its picker cannot
reduce capacity below the selected local roster. Split-screen Infection needs
two local players. Split-screen Defusal requires actual Red and Blue choices;
the frontend does not assign invented teams. LAN may wait for remote players.

Fresh custom LAN joins publish and adopt the authenticated map as well as the
variant. The interactive join chooser retains the selected profiles, teams
and controller instances, then starts a client without a competing host port.
Host-controlled map/settings edits are restored before joining. Leaving the
chooser clears its pending join request. The retained stock lobby still
requires a fresh LAN session to switch into a custom mode.

## Validation

The focused CMake run uses `tools/run_tests.py`, with bounded results under
`build/test-results/match-customization-20261028`. Frozen source validation and
the source hashes are recorded in its `final-cohort` directory. Its private build, generated
native worlds, temporary profiles, and captures are disposable.

The final frozen cohort passed **10/10 CTests**. Both original-reference and
imported-MCC menu catalogs passed the 4,146 edited Starts. The retained LAN
case completed seven customized native worlds, and all three local-view
cases preserved the complete settings buffer across three native frames.
The native runtime handoff check also passed in the preceding frozen cohort.
Commands, source hashes, bounded failure diagnostics, reports, and cleanup
receipts remain in
[`acceptance.json`](../build/test-results/match-customization-20261028/acceptance.json)
and its linked cohort reports. All owned source/build/probe directories were
removed; no large binary, raw memory, or capture artifacts were retained.

Other work changed `CMakeLists.txt`, `multiplayer_runtime.cpp`, and
`physics_live_world_tu15.cpp` after the final source freeze. The report records
that drift; this acceptance applies to the pinned cohort rather than those
subsequent concurrent edits.

| Check | Coverage |
| --- | --- |
| `frontend_match_customization_tests` | 92 parameters, 84 category routes and 4,146 edited Starts (1,100 through Quick Options), LAN and two-controller split-screen; complete buffer equality at ordinary Start; Midship/Waterworks; copied and reloaded full-buffer variants; cross-engine browsing and editor entry. |
| `multiplayer_tests --session-config` | Authenticated loopback UDP transfer of all seven engines with valid descriptor edits and full source buffers. |
| `native_mcc_multiplayer_prepared_views[234]_tu15` | Edited match settings in actual native source worlds with two, three, and four local views; full buffer equality at construction and after ordinary source frames; four-player case also changes team rules. Exact phase ordering includes the genuine native physics-frame observer and requires one completed native physics frame per step. |
| `native_mcc_ctf_assault_system_link_return_tu15` (Single Bomb extended case) | Actual retained encrypted UDP peers, all-seven-engine edited Start/cancel, successive customized native worlds including free-for-all/team transitions, natural results, retained socket/slots/profiles, and next-world admission. |
| Existing objective Start/runtime checks | Original preset option bytes, teams, and launch/session handoff remain covered. |

This is settings availability, preservation, and bounded native execution
coverage. It does not establish every combination's full gameplay, rendering,
audio, physical-Xbox, or cross-machine parity. Richer local Vista postgame
presentation remains separately bounded; other stock modes use the ordinary
runtime launch path. Launcher-selected immutable runtimes need a later build
and qualification to incorporate these source changes.

### Custom-mode follow-up validation

The custom-mode follow-up passed **11/11 CTests** through `tools/run_tests.py`.
Its results are under `build/test-results/custom-mode-cohort-20261028`:

- 429 metadata checks, preserving the original catalog and admitting every
  offered custom value through the real rules validator.
- Each original-reference/MCC menu catalog passed 22 category routes and 564
  edited custom Starts, including 176 Quick Options edits and 84 native timer
  edits. Pointer reachability, fixed controls, actual local teams, named copies
  and reopened settings passed. The original persistent category list remains
  present when focus changes.
- Twelve actual MCC custom native cases cover all three modes with two/four
  local players in LAN/split-screen, authenticated loopback discovery including
  the selected map, admission and snapshots. Three actual headless runtime
  launches each advanced three ticks.
- Each custom mode submitted two real rendered frames with both local views.
  Defusal uses its custom unit HUD rather than stock source waypoints/goals;
  its report records the valid initial waiting state before the first tick,
  rules and two sites. This is bounded launch coverage, not visual parity.
- The original 92-option and objective Start regressions also passed.

[`acceptance.json`](../build/test-results/custom-mode-cohort-20261028/acceptance.json)
links commands, effective pinned hashes, declared refreshes, bounded failure
diagnostics and final native/render reports. Concurrent CMake/network/runtime
work advanced after the freeze. Only the reviewed custom objective guards were
refreshed in that frozen runtime; those other concurrent changes are outside
this acceptance. The graphical LAN-client chooser was reviewed and compiled;
its live interaction and cross-machine play were not exercised here.
All 11 recorded private source/build/probe directories were removed. No large
binary, raw memory or capture artifacts were retained. Launcher-selected
runtimes were not rebuilt or promoted.
