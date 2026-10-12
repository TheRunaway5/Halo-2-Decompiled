# Pregame lobby game panel

The screen14 game-panel bindings are recovered from the supplied Xbox executable
and `mainmenu.map`. The implementation uses original map preview descriptors,
mode pages, category order, variant values, help visibility and text tokens.
This is a bounded presentation implementation. It does not establish online
session behavior, campaign unlock progression, controller occupancy beyond the
current local host, or full lobby framebuffer parity.

The source executable SHA256 is
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The mainmenu cache SHA256 is
`1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b`.
The reproducible [original instruction oracle](../analysis/frontend/lobby/oracle-game-panel.py)
retains its [inputs and outputs](../analysis/frontend/lobby/game-panel-original.json)
and [focused fixture](../tests/fixtures/frontend-lobby-game-original.tsv).
The [detailed source proof](../analysis/frontend/lobby/GAME_PANEL_SOURCE_PROOF.md)
records routine addresses, environment seams and limits.

## Preview and map metadata

Original250332 resolves the selected multiplayer/campaign map record and sets
bitmap widget2 to descriptor0 using137550→2B0A7B. The latter recomputes geometry
from the authored position and the descriptor's dimensions. All27 valid original
preview pages are220×207. Lockout ID50 uses E6F904D7. `unknown_map` is a same-size
fallback, so changing its scale would not repair the missing original binding.
The original matchmaking path explicitly clears the map override.

`Cache::load_frontend` now retains the first `matg+178` header's valid campaign
and multiplayer records and preloads their referenced images. English names
and campaign descriptions come from original language0 fields. Scenario paths
are verified through unchanged19C970: multiplayer+B4C, campaign `matg+170`
key-pair table. `UiLobbyMap` retains bitmap, name, description, campaign/map IDs,
path, sort order and raw flags. Original catalog order is preserved. Multiplayer
description+24C is also retained for source206's2C9B7C focused-map binding; that
screen's full producer/list validation belongs to the map-selection agent.

Original19C9A0 sorts multiplayer choices by record+C4C; its144 ordered comparisons
place Ivory Tower fourth and Headlong tenth. The previous hardcoded menu order
differed. Foundation's+C50 bit0 is gated by original2C9928's actual campaign
progress comparison. The metadata preserves that flag; it does not manufacture
an unlock. State/list integration must supply the proven progress condition.

## Icon, quick options and campaign text

Original19D220 maps the seven frontend engine enums to source variant types
2,3,7,4,1,9,8. Original250A43 chooses mode-icon pages0,3,4,1,6,7,8 respectively,
and page10 for unsupported source types. The helper retains both mappings.

Original2501C0 fills only five quick rows. Source direct text ordinals include
title/subtitle first, so runtime6..10 are authored raw4..8. Original25092C follows
the actual category's `sily` references, reads current parameter values through
2374F0, finds the first matching option and joins title + literal separator
`": "` + value label. Source defaults are retained for all7×112 parameter IDs;
the current selected variant overrides them. Slayer's source rows are25 kills,
8 Minutes, Map Default starting weapon, Map Default map weapons and Motion
Sensor On. Empty/missing option labels are not substituted with guessed text.

Original campaign E426/E427 parsers copy the selected record's name/description;
all15 pairs are retained from executable execution. E428 maps signed16 difficulty
0/1/2/3 to Easy/Normal/Heroic/Legendary string IDs and unsupported values to Normal.
The actual difficulty bitmap resource0 is the same bank as authored pane1
bitmap4, and2503C0 selects descriptor0..3 for valid values.

## Visibility and local session boundary

Original2508A8 displays party help only for online protocol2 with at most one
machine. It then chooses screen14 value block1 by privacy/matchmaking state.
Split Screen and System Link do not show the Friends/Clan paragraph. The helper
returns the raw text index and literal string ID from explicit session inputs.

The original E41F protocol token distinguishes Split Screen, System Link and
online modes. Header policy owns that binding. E425 count uses max of two source
queries, clamps positive counts to16 and retains the cached previous count on
zero/negative input. The game helper reproduces that cache rule, but the current
supported native normal route has one signed-in local player and directly binds
its count. Zero-count transitions and broader session occupancy are not claimed
as a completed runtime lifecycle.

## Validation

The original oracle executes27 map bindings plus matchmaking clear,27 scenario
path queries,144 sort comparisons,12 icon cases,784 defaults,70 quick-option
rows (35 defaults and35 original-setter alterations),144 help cases,28 count
cases,9 protocol cases,15 campaign text pairs and7 difficulty cases. Its supplied
environment covers session queries, original locale data and widget/format
services; those seams are declared in the evidence script.

`frontend_lobby_game_tests` loads the real cache and checks the frozen original
fixture against the loader/helper. Missing source assets return77. Root owns
native state/painter integration, original normal-route screenshots, builds and
aspect-ratio captures. Passing helper cases alone is not a claim that the entire
main menu or every lobby pixel matches the original.

The first four focused lobby tests passed in2.42s (`green-tests.log`), including
the initial game-helper checkpoint. The extended campaign token/difficulty and
multiplayer-description fixtures are newer than that run; final combined
verification is recorded by the parent task after its next build.

The first extended run caught an expected-text fixture error: global and scoped
`unic` ranges were flattened, selecting profile-picker "on Easy" rather than
original23620D's global "Easy". The oracle now records and reads the exact
`wigl+11C` E4D802B8 global_strings range. Executed IDs and production difficulty
helper behavior were already correct and did not change.
