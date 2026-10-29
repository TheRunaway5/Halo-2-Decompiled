# Team switching and armor colors

Original Xbox TU1.5 owns the behavior. Team designators 0–7 select Red, Blue,
Yellow, Green, Purple, Orange, Brown and Pink from the map's `mulg` team palette.
The original team selector changes membership; it does not edit a named team's
RGB values.

The complete player color producer is `830C0`, called by `161DB0`. A team
replaces armor channel 0, while the player's personal primary becomes channel 1.
Neutral membership makes channels 0 and 1 white. Emblem channels 2 and 3 retain
the selected personal palette entries. `83050` and `1AA2B0` own palette lookup
and their original fallbacks. The fixture and its provenance are in
`tests/fixtures/player_colors_tu15_original.*`; all 328 cases execute unchanged
TU1.5 instructions against the original cache's actual palette blocks.

`MultiplayerMatch::change_team` enforces parameter 22 (Team Changing), the
selected original engine's available teams, participant presence and game-over
state. Accepted changes execute original `14E9E0`, including `15AB10`'s unit
death/detachment and respawn delay. Current accepted configuration is separate
from the immutable transport admission profile. Detached bodies retain their
actual source color bank (`BCD80`), including the previous team color.

The pause menu resolves keyboard/controller ownership before selecting a local
player. Split-screen players have independent body, first-person hands and
lower-body material banks. Remote requests contain a peer-local view, never an
arbitrary player slot. The host resolves it through authenticated couch ownership;
serials, acknowledgements and bounded retries prevent duplicate switches.

The lobby has four independent preview/commit/cancel records. Retained System
Link sessions carry accepted teams into their next world and permit new lobby
choices independently of the in-match rule. Countdown admission remains with
the session owner. Postgame carriers validate current membership and retained
team history when Team Changing is enabled; disabled games keep the original
admission equality requirement. Native wire protocol 123 requires matching
host/client builds.

Focused validation (replace the map path with an installed prepared MCC map):

```sh
python3 tools/run_tests.py --jobs 2 --test-jobs 1 \
  --cmake-arg=-DHALO2_MCC_MULTIPLAYER_ENGINE_MAP=/absolute/path/coagulation.map \
  --target player_colors_tu15_tests --target team_change_tu15_tests \
  --target frontend_team_tests --target game_pause_menu_tests \
  --target player_armor_emblem_runtime_tests --target native_request_protocol_tests \
  --target security_transport_tests --target online_pcr_family_tests \
  --filter '^(original_player_colors_tu15|original_team_change_tu15|original_frontend_team_picker|native_game_pause_menu|native_team_switch_color_runtime|player_(armor_emblem|team_color|team_color_lan)_runtime|native_request_protocol|security_transport|online_pcr_family_carriers)$'
```

The rendered four-player test creates mapped SDL virtual controllers and drives
the actual pause handlers. These are source-world, UI, UDP and Vulkan
component/runtime checks. They do not
establish original-hardware framebuffer parity or publish a launcher runtime.
