# Native ONLINE modes and clan rank audit

Checked 2026-10-13 against current native owners, owned original Xbox/Vista
caches and retained unchanged-executable research. This audit changes no product
source, playlist activation, build or deployment. It does not make the unfinished
game-mode goal complete.

The current usable matchmaking family is **Slayer**, with ordinary FFA and team
settings. King has substantial verified native engine, replication, result and
presentation components, but still lacks the complete normal-world join. The
other objective families have decoded variants and original registration
evidence, not active native gameplay. Consequently a larger catalog alone
cannot restore original online game coverage. [Normal launch admission](../../src/frontend_matchmaking.cpp#L79),
[world construction](../../src/multiplayer_world.cpp#L12),
[engine admission](../../src/multiplayer.cpp#L159),
[operational acceptance and recorded limits](../../docs/GAME_MODE_PROGRESS.md).

## Current original inputs

Fresh read-only extraction is saved in
[assets.json](../../.local/native-online-mode-clan-audit-20261013/assets.json).
It reads the currently owned source files, all 39 original `goof` variant rows,
screen26/pane1 records, both literal rank banks and all50 `wigl` rank ranges.
It does not execute original gameplay or render a reference frame.

| Input | SHA-256 |
| --- | --- |
| Xbox `assets/disc/maps/mainmenu.map` | `1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b` |
| Vista `Halo 2 Vista installer/maps/mainmenu.map` | `eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44` |

The primary field readers are the existing bounded
[Xbox metadata reader](../../tools/inspect_multiplayer.py#L149) and
[Xbox/Vista UI inventory reader](../../analysis/frontend/hd-ui-followup-20261009/audit-vista-menu-extension.py#L92).
The retained original variant executions independently pin the retail andTU1.1
images and compare defaults, all112 parameter descriptors, setters/getters and
built-in buffers. Decoded configuration is separate from operational gameplay.
[Original variant controls](../../tools/oracle_game_variants.py),
[captured original buffers](../../analysis/multiplayer/game-variants/original.json).

## Exact mode inventory and active owners

Frontend indices are cache/UI selectors; registered source engine IDs are a
different domain. The native initializer preserves the literal mapping
`[2,3,7,4,1,9,8]` for frontend indices0–6.
[Source default initializer](../../src/source_game_variant.cpp#L54).

| Family | Frontend index | Source engine | Original built-in rows | Normal native world |
| --- | ---: | ---: | ---: | --- |
| Slayer | 0 | 2 | 9 | Ordinary FFA and Team Slayer admitted; other seven authored rows remain gated |
| Oddball | 1 | 3 | 6 | No admitted objective update/carry/drop/score lifecycle |
| Juggernaut | 2 | 7 | 5 | No admitted selection/traits/scoring lifecycle |
| King | 3 | 4 | 5 | Bounded direct native match binding; normal launch remains gated |
| CTF | 4 | 1 | 4 | No admitted flag objective lifecycle |
| Assault | 5 | 9 | 5 | No admitted bomb objective lifecycle |
| Territories | 6 | 8 | 5 | No admitted capture/contested/scoring lifecycle |

These counts come from the fresh original `goof` extraction. The39 names and
their exact override records remain in the linked JSON. The last strict
normal-menu audit records2/39 admissions and37 blocked rows; it must not be
mistaken for2 complete arbitrary-rule families. The current owners independently
still reject non-Slayer normal startup and unimplemented field changes.
[Current native rules](../../src/multiplayer.cpp#L74),
[goal and audit history](../../docs/GAME_MODE_PROGRESS.md).

The original registration tool executes the original table setup and inventories
51 callback slots for seven IDs in each of two executable images. Source1/9
share the same original engine object. This is registration evidence only; it
does not execute any objective lifecycle. Do not implement CTF/Assault by
relabeling Slayer merely because both appear in that table.
[Original registration tool and explicit scope](../../tools/inspect_game_engines.py),
[literal original engine tables](../../analysis/multiplayer/game-variants/engine-tables.json).

King's existing owners should be reused, not re-created from approximations:

- `source_king` preserves the0x1DC state, scenario group11–18 markers, spline,
  occupancy, timers, shared RNG, actual scoring/phase and relocation order.
  The retained unchanged-original corpus covers all57 hills on all12 disc maps
  across retail,TU1.1 andTU1.5. [Core and limits](../../docs/MP_KING_ENGINE_IMPLEMENTATION.md),
  [native interface](../../include/halo2/source_king.hpp).
- Direct `king_rules`/`MultiplayerMatch` has actual lifecycle, player position,
  score/end/recording owners. It requires real `score_changed` and
  `notification` services; empty callbacks are not a completed runtime join.
  [Constructor gate](../../src/multiplayer.cpp#L238),
  [world update](../../src/multiplayer.cpp#L1157).
- Actual16-player fragmented UDP tests preserve hills, occupant masks, player
  timers, scores, slot reuse and result fields, and reconstruct authored hill
  geometry from received indices. Their debug positions and observed callbacks
  isolate transport. [Test](../../tests/king_objective_network_tests.cpp),
  [verification and limits](../../docs/MP_KING_OBJECTIVE_NETWORK_NATIVE_BINDING.md).
- The live score HUD accepts the actual source engine/round selector and formats
  King time scores. Original hill color, marker records and boundary geometry
  also have comparison-tested native owners. Material preparation, projected
  waypoints and full live world submission still have separate original-owner
  gates. [HUD](../../docs/MP_KING_HUD_NATIVE_BINDING.md),
  [visual consumers](../../docs/MP_KING_VISUAL_NATIVE_BINDING.md),
  [boundary](../../docs/MP_KING_BOUNDARY_NATIVE_BINDING.md),
  [actual preparation corridor](../../docs/MP_KING_HILL_PREPARATION_SOURCE_RESEARCH.md).

The normal frontend and `MultiplayerPreparedWorld` currently both select
`slayer_rules`, and world construction supplies empty King services. The ONLINE
PCR producer additionally requires a verified PC Slayer variant. Therefore
replacing one dispatch call with `king_rules` would still fail the service/PCR
boundaries and would not prove a completed King match.
[Frontend](../../src/frontend_matchmaking.cpp#L79),
[world](../../src/multiplayer_world.cpp#L12),
[ONLINE publication](../../src/online_postgame.cpp#L49),
[Vista variant bridge](../../src/vista_pcr_publication.cpp#L17).

## Historical rotations versus native capability

The already preserved dated Bungie catalog and original executable preset
evidence remain the authoritative starting point. The30November2004 catalog
has nine Head-to-Head entries and17 Team Slayer entries, including ordinary,
rifle/sword, fiesta, rockets, shotguns and phantoms. Team Training also includes
objective games. Numeric rotation weights and complete map-specific downloaded
variant settings were not recovered. The2December2005 catalog is a distinct
revision and must not be silently merged with it.
[Dated primary-source links and extracted pools](online-rank-playlist-evidence.md#dated-actual-playlist-catalog).

Current native restrictions relevant to those rows are explicit:

- The admitted starting-weapon options are Map Default0 and SMG5. Other source
  options fail, as do Slayer bonus-score changes and unimplemented rule bits.
  [Rules adapter](../../src/multiplayer.cpp#L74).
- Global camouflage is rejected in the actual match constructor. Enabling
  Phantoms or Phantom King requires the complete existing camouflage/render
  owners rather than deleting the guard. [Constructor](../../src/multiplayer.cpp#L239).
- ONLINE currently rejects Midship/Waterworks at launch because of unfinished
  starting-weapon support; native energy-start validation is also explicit.
  Merely installing those maps is insufficient.
  [ONLINE map gate](../../src/frontend_matchmaking.cpp#L92),
  [match weapon checks](../../src/multiplayer.cpp#L272).
- Authored Primary Light Vehicle overrides are rejected on maps with affected
  equipment rows until creation/control is bound.
  [Map option validator](../../src/multiplayer.cpp#L168).
- An ONLINE catalog variant must resolve to an original `goof` name. Executed
  executable presets such as Slayer Duel/Rifle Duel/Sword Duel are different
  original input buffers; their existence does not make them current `goof`
  rows or downloaded playlist settings.
  [Name resolution](../../src/frontend_matchmaking.cpp#L79),
  [original named presets](online-rank-playlist-evidence.md#original-named-presets-and-settings-boundary).

The bundled catalog is accurately labeled a custom validation rotation.
Unsupported historical rows should be retained as unavailable evidence with
their real blockers until whole runtime/settings admission exists; publishing a
smaller pool under an original complete-playlist claim would lose source scope.
[Historical development catalog](../master_server/examples/matchmaking-development.json).

## Clan role, competitive skill and artwork are separate

The parser names `target_player_clan_level` and `live_ui_driver_clan_level`
are **membership role** producers. They select Peon,Member,Staff,Overlord using
integer0–3 and string IDs040002C6/060002C7/0D0002C8/090002C9. They cannot supply
a1–50 competitive clan badge.
[Original parser](../../tools/reference/halo2-decompiled/src/user_interface_text_parser.cpp#L1200),
[role evidence](halo2-clan-evidence.md#roles-and-exact-authority-checks).

Actual competitive clan rank comes from the selected type5 hopper. Original
`74AA0` queries the clan identity at cache+10 instead of player XUID+4;
`65530` publishes selected-hopper raw rank at configuration+7E and rank/XP at
+8A/+8C. Original lobby producer`25042D` passes that+7E byte through the rank
range adapter. It is thus the selected clan-hopper raw rank on this chain,
not the social role or personal XP.
[Original stat query](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp#L1462),
[property publication](../../tools/reference/halo2-decompiled/src/unknown_054fe0.cpp#L1412),
[lobby rank producer](../../tools/reference/halo2-decompiled/src/unknown_250155.cpp#L2178),
[executed clan provenance](halo2-clan-matchmaking-evidence.md#clan-rank-is-the-statistic-subject-generic-averaging-still-occurs).

Freshly inspected Xbox/Vista rank artwork has50 pages in each of two banks:

| Bank | Xbox datum | Vista datum | Original dimensions |
| --- | --- | --- | --- |
| `ui\\global_bitmaps\\rank_icons` | `E4FA02DA` | `E50802E6` |28×26 |
| `ui\\global_bitmaps\\rank_icons_sm` | `E4E202C2` | `E4EC02CA` |17×17 |

Both source `wigl+158` tables have ranges`[i,i]` for0–48 and`[49,127]` for
the final page. RawNONE/-1 hides the badge. Valid raw0 selects page0/display1;
raw49–127 selects page49/display50. Frames43–49 are original celestial artwork,
not replacement numbers. The earlier pixel audit found all100 Xbox/Vista pages
byte-identical; their unchanged input hashes were verified again here. Their
small authored extents should not be described as newly higher-resolution art.
[Fresh source metadata](../../.local/native-online-mode-clan-audit-20261013/assets.json),
[original range adapter](../../tools/reference/halo2-decompiled/src/unknown_2b01a2.cpp#L13),
[complete range lookup](../../tools/reference/halo2-decompiled/src/unknown_1490ec.cpp#L359),
[pixel comparison and conversion evidence](online-rank-playlist-evidence.md#stored-049-represents-displayed-150).

Screen26/pane1 retains the original Level/Msg/Voice/Status headings and direct
`rank_icons` bitmap4 at(16,152), default page1, in both caches. That static page
is not a real clan-level value. Original tab update`2314CE` binds the profile's
maximum raw rank byte+7F through a descriptor, then explicitly hides direct
bitmap4. The selected-hopper lobby+7E and social-profile maximum+7F are distinct
owners. Re-enabling the static page as a clan badge would invent behavior.
[Fresh pane records](../../.local/native-online-mode-clan-audit-20261013/assets.json),
[original tab update](../../tools/reference/halo2-decompiled/src/unknown_2312b4.cpp#L129),
[maximum versus selected publication](../../tools/reference/halo2-decompiled/src/unknown_054fe0.cpp#L1412).

The native clan page currently replaces those source rank columns with member
count/voice state and hides unowned social placeholders. Selected clan playlist
details show numeric clan level; `ClanSkills` stores/version-checks clan-owned
raw rank/XP and commits one clan result per team. It does not presently expose
the original selected-hopper badge in a native lobby/PCR owner.
[Native clan strings](../../src/frontend_clans.cpp#L177),
[native placeholder suppression](../../src/frontend_painter.cpp#L1163),
[playlist details](../../src/frontend_matchmaking.cpp#L189),
[server clan rank](../master_server/matchmaking_http.py#L239),
[clan XP transaction](../master_server/clan_skill.py#L63).

The existing PC PCR appearance consumer already maps actual display rank
field+78 to the original range/frame/visibility contract. The separately
recovered PC PCR builder explicitly writesNONE to that display field; the
fresh native selected-profile bridge also rejects unproved rank/account bytes.
Thus editing a profile field alone would not expose a badge, and overwriting
the recovered PC builder would change its verified behavior. Adding a native
rank presentation needs a separately declared authenticated selected-hopper
adapter and an original Xbox clan PCR publication trace. Rendering a derived
personal level in a clan match would bind the wrong subject. The original
type5 result producer updates clan team records, not each member's personal
skill.
[PCR appearance](../../src/vista_pcr_appearance.cpp#L81),
[PC PCR display producer](../../src/vista_pcr_results.cpp#L140),
[fresh profile boundary](../../src/native_vista_session.cpp#L141),
[original clan result owner](halo2-clan-matchmaking-evidence.md#clan-owned-postgame-xp-and-rank).

## Later ranking and quit-policy boundary

The current personal/clan arithmetic implements the recovered retail launch
tables and signed integer owners. Later Bungie pages supply changed thresholds
and exchanges but contain contradictory level10/11 loss factors, and no exact
downloaded later configuration was recovered. Do not silently interpolate,
repair or blend those rows into a policy called original.
[Later primary-source data and contradictions](online-rank-playlist-evidence.md#published-later-era-data-and-contradictions),
[current versioned implementation](../master_server/playlist_skill.py#L13).

Bungie's retained original overview supports final-standing treatment of
departures: teams with finishers rank ahead of teams with no finishers, and a
departing player inherits a finishing team's result. This is a match/result
construction rule, not evidence for an arbitrary extraXP fine, timed ban or
modern cooldown. Disconnect survival and once-only trusted result publication
must exist before verified departure results can produce progression.
[Primary source and recorded distinction](online-rank-playlist-evidence.md#published-later-era-data-and-contradictions),
[source departure investigation](halo2-nonhost-departure-evidence.md),
[existing progression limits](ranked-progression.md).

No original whole ranked-quit or later-configuration control was executed by
this audit. Exact extra penalties, later-era ambiguous coefficients, complete
downloaded rotation weights/settings and original clan PCR rank-change
publication remain unresolved evidence requirements.
