# Halo 2 clan service and authored UI evidence

Research date: 2026-10-10. This records original client/manual behavior for the
Discord-authorized native clan service. It does not establish the retired Xbox
Live service's database, wire protocol, rate limits, or moderation rules.

## Authorities

- Microsoft's original Xbox Halo 2 manual, printed pp. 26–27:
  [official PDF](https://download.microsoft.com/download/f/9/9/f99ab8f0-5191-4edd-b312-7a9b9e4784fa/halo2_covenant_mnl_us-en.pdf).
  The PDF downloaded successfully from Microsoft and was read with `pdftotext`;
  the browser text tool rejected its `application/octet-stream` MIME type.
- Original client routines retained in
  [online_teams.cpp](../../tools/reference/halo2-decompiled/src/online_teams.cpp),
  [online_messages.cpp](../../tools/reference/halo2-decompiled/src/online_messages.cpp),
  [unknown_238e42.cpp](../../tools/reference/halo2-decompiled/src/unknown_238e42.cpp),
  [unknown_2b116a.cpp](../../tools/reference/halo2-decompiled/src/unknown_2b116a.cpp),
  [unknown_2c4e9c.cpp](../../tools/reference/halo2-decompiled/src/unknown_2c4e9c.cpp),
  [unknown_1a2ca7.cpp](../../tools/reference/halo2-decompiled/src/unknown_1a2ca7.cpp),
  [unknown_23760b.cpp](../../tools/reference/halo2-decompiled/src/unknown_23760b.cpp),
  and [user_interface_text_parser.cpp](../../tools/reference/halo2-decompiled/src/user_interface_text_parser.cpp).
- Actual local `assets/disc/maps/mainmenu.map` English `matg`/`unic` records and
  `wgit` definitions, read with the existing bounded `tools/audit_rendering.py`
  cache reader. SHA-256:
  `1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b`.
- Existing [Xbox/Vista definition inventory](../../analysis/frontend/hd-ui-followup-20261009/vista-menu-extension-inventory.json)
  and its [reader](../../analysis/frontend/hd-ui-followup-20261009/audit-vista-menu-extension.py).
  Vista source map SHA-256:
  `eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44`.
  The Vista cache retains these clan definitions, but their presence alone does
  not prove that the original PC service enabled clans. The existing native
  frontend uses Xbox route/layout semantics and imported Vista HD visual assets.

## Membership and lifecycle

The manual establishes a persistent organization of up to 100 players, one clan
per player, at least one Overlord, and Member as the role assigned on joining.
Its documented entry is the Y menu's Clans tab; that tab supports creation,
joining and leaving. [Microsoft manual, pp. 26–27](https://download.microsoft.com/download/f/9/9/f99ab8f0-5191-4edd-b312-7a9b9e4784fa/halo2_covenant_mnl_us-en.pdf)

`online_team_create`, retail `0xabe60`, passes capacity 100 and creator privileges
`0xffffffff` to `XOnlineTeamCreate`. `online_message_block_send_team_recruit`,
retail `0x8f500`, recruits an existing selected XUID or an entered Gamertag and
sets privilege `0x10` (Member). `online_team_answer_recruit`, retail `0xabfa0`,
maps answers 0/1/2 to No/Yes/Never. The corresponding client callbacks at
`0x239300`, `0x23933a`, `0x239374` pass Yes/No/Never respectively; the comment
above those callbacks should not be mistaken for the actual value mapping.

Leaving with more than one member removes only the actor; leaving as the last
member deletes the clan. This is explicit in `function_23955e` (`0x23955e`) and
`function_2b278e` (`0x2b278e`). `function_239292` and its confirmation callback
`function_23990c` block leaving when the actor is the sole Overlord and other
members remain. The sole-Overlord check is `function_1a334a` (`0x1a334a`).
Original localized message `0x2b00170a` instructs that actor to promote another
member before leaving. Last-member departure remains allowed and deletes the
clan.

The original flow therefore supports multiple Overlords. A leadership handoff
is promotion of another member followed by the old Overlord leaving; promotion
does not implicitly demote the actor. A modern explicit "Disband" operation
that deletes a populated clan is a native service extension: although a team
delete wrapper exists, the examined authored client invokes it for last-member
departure, not an arbitrary populated-clan destructive action.

## Roles and exact authority checks

`online_team_member_set_rank` (`0xac360`) and the inverse getter (`0xac250`)
define these client roles. The parser at `0x22e06c`/`0x22e0df` confirms the
English role string mapping.

| Role integer | Original label / string ID | Xbox Live privilege bits | Authored permissions screen |
|---|---|---|---|
| 0 | Peon / `0x040002c6` | `0` | Participate in clan games |
| 1 | Member / `0x060002c7` | `0x10` | Also recruit and broadcast clan messages |
| 2 | Staff / `0x0d0002c8` | `0x1c` | Also remove members and change Peon/Member roles |
| 3 | Overlord / `0x090002c9` | `0xffffffff` | Also change Staff roles |

Peons are allowed in clan matches: pane 0 explicitly says so. Secondary pages
claiming otherwise disagree with the supplied original asset.

For an established member, `function_2393ae` (`0x2393ae`) requires the actor to
have role >=2 and a strictly higher role than the target's current role.
Requested roles cannot exceed the actor's own role. Thus Staff may promote a
Peon/Member to Staff but cannot modify that new peer afterwards. Overlords may
modify Staff and lower, but cannot demote or remove another existing Overlord.
There is a separate friend-flags bit 11 path; its precise pending-recruit
semantics are not established here and must not become a general privilege
bypass in the replacement service.

Promotion to Overlord uses an additional confirmation and invokes
`function_2399da` (`0x2399da`), which sets only the selected member's role to 3.
Localized confirmation `0x240016e7` states that the promotion cannot be undone.
The role selector constructor `0x2c6824` adds all four role choices; the accept
callback `0x2c69b3` dispatches the authority check above. The role selector is not
limited to lower destination choices for Staff.

## Name, invite and input strings

The actual keyboard description `0x1500045c` establishes a maximum of 15
characters, letters/numbers/spaces only, and an immutable clan name after
creation. Name collision has a dedicated original error (`0x1f0016ef`). Case
folding, trimming and Unicode normalization at the retired server are unknown;
any explicit replacement rule for those is a native service policy.

| Purpose | Original Xbox English string ID |
|---|---|
| Create Clan | `0x0b0002e7` |
| Clan Options | `0x110002e6` / header `0x1f00184d` |
| Send Clan Invite | `0x110002ba` |
| Accept Clan Invite | `0x130002bb` |
| Decline Clan Invite | `0x140002bc` |
| Block Future Invites | `0x1e0002bd` |
| Cancel Clan Invite | `0x130002be` |
| Remove From Clan | `0x100002bf` |
| Change Clan Role | `0x160002c0` / screen header `0x06000233` |
| Leave Clan | `0x0a0002a5` |
| Clan-name keyboard title / explanation | `0x1100045b` / `0x1500045c` |
| Recruit-name keyboard explanation | `0x1b000456` |

The original recruit keyboard takes a Gamertag; the service can instead resolve
an explicitly supplied Discord username to its private verified identity while
retaining the editable game screen name. That mapping and ambiguity handling
are native integration choices, not recovered Xbox behavior. Never display
Discord snowflake IDs or use user-selected screen names as authorization.

## Native authored screens and layout

Coordinates below are the original UI canvas values in the supplied cache, not
new pixel layouts. The Vista definitions have the same listed clan geometry.

| ID | Original path suffix | Xbox datum | Vista datum | Useful authored children |
|---|---|---|---|---|
| 26 | `online_y_menu/online_y_menu` | `e5c103a1` | `e5ea03c8` | Three panes: Friends 0, Clan 1, Players 2; each list skin 6, eight visible rows, position (-226,101) |
| 217 | `online_y_menu/clan_options_dialog` | `e61b03fb` | `e641041f` | One pane; skin 0, five visible rows, position (-106,13) |
| 31 | `online_y_menu/player_selected_dialog` | `e5c803a8` | `e5f003ce` | One pane; skin 0, six visible rows, position (-247,29); selected-player options |
| 236 | `xbox_live/edit_clan/clan_permissions` | `e6280408` | `e64e042c` | Four role-description panes; each skin 0, four rows, position (-265,-17) |
| 167 | `virtual_keyboard/virtual_keyboard` | `e5d803b8` | `e60003de` | Existing name input, done/cancel/caret behavior; Vista keyboard geometry differs and stays source qualified |
| 33 | `misc/confirmation_dialog` | `e5d303b3` | `e5fb03d9` | Existing two-choice confirmation; skin 0, position (-106,-20) |

Clan pane 26/1 has the original heading, clan-name field, column labels and
footer; the clan-name field is Xbox string `0x1d001840`, which contains parser
token U+E421. It does not contain a static clan name. Privilege screen 236 has a
target/role text at bounds (top72,left-260,bottom-19,right-35), a role heading at
(72,7,52,260), and permission body at (50,7,-83,260). Select the pane matching
the highlighted destination role to retain the authored descriptions.

Original `c_clan_options_list` (`0x2b233d`, `0x2b24b2`) offers recruit, clan
message, clan party invite and leave; the first two require Member or higher,
party invite also needs the appropriate online state, leave is always offered.
The existing placeholder native route that puts Change Clan Role in generic
clan options differs from this recovered behavior. Role change belongs to a
selected member's actions/permissions route.

The shared social row binder in
[`unknown_2b2d7d.cpp`](../../tools/reference/halo2-decompiled/src/unknown_2b2d7d.cpp),
retail `0x2b2ed4`, binds the name and status to distinct text children. It derives
online state from the source datum's flag1 after invitation restrictions, uses
offline string `0x700024e` when unavailable, and otherwise asks for title and
session presence. This establishes a real status row and online/offline
distinction; it does not establish the modern replacement's observation window.
Native authenticated activity and challenged match connections supply that
replacement state. Duplicate chosen display names cannot identify a row: native
selection is retained by the private member handle across roster polls.

## Explicit limits for this implementation

The examined primary sources do not establish invitation expiry, queue bounds,
HTTP schema, opaque IDs, Discord credential lifecycle, global-name resolution,
federated clan ownership, blocking scope/expiry, audit retention, or modern
populated-clan disband policy. Name moderation services and their exact rules
are also unavailable. Document replacement choices as native service policy.

Social membership/roster/roles do not implement clan matchmaking, result
submission, clan rank progression, party transport, presence, message delivery,
or voice. Authored labels are evidence of original features, not evidence those
services now work. Suppress unsupported status/voice indicators and explain
unavailable gameplay actions rather than inventing state.
