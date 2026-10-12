# Discord friends in the native Vista ONLINE frontend

Research date: 2026-10-11. Research only: this agent performed no SDK download,
portal change, account operation, or implementation. The user subsequently
enabled Social SDK and supplied the official archive; its extracted public
header was inspected below. Discord behavior below
comes from current first-party documentation; Halo layout comes from the
original local Vista cache and recovered Xbox source. Recommendations are
native integration policy, not a claim that Xbox Live used Discord.

## Supported path and access prerequisite

Use the **Discord Social SDK** as the native relationship/presence owner. It
supplies data and expects the game to build its own UI, which permits the
existing Vista asset frontend. A Discord bot, the application's public key,
and the present identity-only login do not themselves implement this owner.
[Official SDK overview](https://discord.com/developers/docs/social-sdk/index.html).

The official setup is: select the application, complete **Discord Social SDK
→ Getting Started**, submit the game details, then download the standalone
C++ archive from **Downloads**. It contains `discordpp.h` and the platform
shared library. Define `DISCORDPP_IMPLEMENTATION` in exactly one translation
unit; link and distribute the matching runtime. Windows uses an import
library plus `discord_partner_sdk.dll`; Linux uses
`libdiscord_partner_sdk.so` with a working runtime search path.
[Official C++ setup](https://docs.discord.com/developers/discord-social-sdk/getting-started/using-c%2B%2B).

Current requirements are C++20+, Windows x64 10+ or ARM64 11+, and macOS x64
and ARM64. Linux is **Experimental**, with glibc 2.31+; the compatibility
matrix does not establish a Linux architecture list or universal distro
support. [Official platform matrix](https://docs.discord.com/developers/discord-social-sdk/core-concepts/platform-compatibility).

Two version-sensitive details require care. The
matrix says macOS Intel 10.5+, whereas release notes explicitly say **10.15**
Intel / 11.0 Apple Silicon. Do not advertise 10.5. The current release notes
list **1.10.19337, 2026-09-01**, and change macOS packaging to
`discord_partner_sdk.framework`; the C++ starter still illustrates a
`.dylib`. The supplied 1.10.19337 archive contains both the framework and a
release `.dylib`, so follow its actual selected integration packaging rather
than claiming that only one exists.
[Official release notes](https://discord.com/developers/docs/social-sdk/release_notes.html).

The SDK is separately licensed: its terms grant distribution **integrated
into the application**, subject to those terms; this is not evidence that
the SDK can be relicensed with this source repository. Keep the downloaded
dependency and its notices distinct from game code and package according
to the supplied terms. No agreement was accepted in this research.
[Official Social SDK terms, section 2](https://support-dev.discord.com/hc/en-us/articles/30225844245271-Discord-Social-SDK-Terms).

At the initial source inspection there was no SDK target in
[CMakeLists.txt](../../CMakeLists.txt); CMake already requires C++20. The user
then reported enabling Social SDK for application `1558522073272426519` and
provided `/home/eric/Downloads/DiscordSocialSdk-1.10.19337.zip`; root extracted
the public headers and Linux release library into `.local/discord-sdk`.
This agent directly inspected that header, zip member names, and library
file metadata. The extracted Linux library is x86-64 ELF; this establishes
the supplied binary architecture, not every supported platform. A real
SDK connection remains unverified by this research.

| Supplied artifact | SHA-256 |
| --- | --- |
| `DiscordSocialSdk-1.10.19337.zip` | `d784097504685953849cc2842561d8a8013326fe0b8df65a1de63ddefde62045` |
| `include/discordpp.h` | `822e71509bf11b7155becca5fb00d215a7db7b305f357c0f3b5f6e27ff45b6ab` |
| `include/cdiscord.h` | `86ef9280ab8432493952bb974bd18c5a25d7725011393b5dba05162bcfdb3371` |
| `lib/release/libdiscord_partner_sdk.so` | `040f4c85ce2a54563292b20c0b8f4b2058d2080b7ba97e435defd981be0f2fbd` |

All relative SDK paths in that table are inside
`.local/discord-sdk/discord_social_sdk`. The zip includes
`License-Notices.txt`; preserve it in the dependency/package workflow.

## Authentication reuse and consent

`Client::GetDefaultPresenceScopes()` returns
**`openid sdk.social_layer_presence`**. These cover relationships and
presence. Limited-access messaging, Discord lobbies/voice, and linked
channels require `openid sdk.social_layer`. Additional necessary scopes may
be appended. Native OG-style voice does not require adopting Discord voice.
[Official scope definitions](https://docs.discord.com/developers/discord-social-sdk/core-concepts/oauth2-scopes).

The current [DiscordLogin](../../src/discord_login.cpp) requests only
`identify`, validates that returned grant, calls `/api/v10/users/@me`, and
retains the access token only in memory until expiry/cancel. Its configured
redirect is `http://127.0.0.1:2003/discord/callback`. `identify` is specifically
required by `/users/@me`; `openid` alone is not a documented replacement for
the existing master-server identity verification.
[Official Get Current User](https://docs.discord.com/developers/resources/user#get-current-user).

Recommended grant for this integration: **`identify openid
sdk.social_layer_presence`**. Check the returned scope set rather than
assuming an old login gained access. An existing identify-only credential
requires a new consent flow; a current credential with the necessary grant
can be passed privately to `UpdateToken(Bearer, ...)`, then `Connect` after
success. The SDK's account-linking guide explicitly supports token reuse,
refresh, and PKCE. Keep one authentication owner; do not run two competing
loopback listeners or insert a client secret in the native binary. Preserve
the present memory-only policy unless persistence is separately designed.
Do not adopt provisional accounts: this game's stated policy requires a
real Discord login. [Official account linking](https://docs.discord.com/developers/discord-social-sdk/development-guides/account-linking-with-discord).

Pump `discordpp::RunCallbacks()` from the native loop; read relationships
only after `Client::Status::Ready`. Disconnect, clear relationship handles,
pending invites, and display copies when the identity signs out or changes.
Merely possessing a token is not a successful SDK connection.
[Official SDK startup](https://discord.com/developers/docs/social-sdk/getting_started.html).

## Original Friends screen and control seams

Original Vista cache:
`analysis/frontend/vista-asset-inventory-20261009/extracted/maps/mainmenu.map`,
SHA-256 `eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44`.
The existing [bounded inventory](../../analysis/frontend/hd-ui-followup-20261009/vista-menu-extension-inventory.json)
and [reader](../../analysis/frontend/hd-ui-followup-20261009/audit-vista-menu-extension.py)
provide raw offsets and bytes; inventory values were re-read for this note.

| Authored Vista screen | Exact retained controls |
| --- | --- |
| 26, `online_y_menu`, datum `e5ea03c8` | Pane 0 FRIENDS; pane 1 CLAN; pane 2 PLAYERS / Recent Players. Each uses skin 6, eight visible rows at `(-226,101)`. |
| 31, `player_selected_dialog`, `e5f003ce` | Six action rows, skin 0 at `(-247,29)`; right-side information texts 0–7 in 20-unit bands, x `0..253`. Text 8 is an offscreen 1×1 Status placeholder, not a message area. |
| 216, `friends_options_dialog`, `e63e041c` | Seven-row skin-0 list at `(-106,24)`. Its retained header says Xbox Live Options; the native replacement should say ONLINE. |

For Friends pane 0 the text bounds are `[top,left,bottom,right]`: text 0
FRIENDS `[193,-262,173,-145]`; text 1 footer
`[-186,-272,-210,203]`; text 2 player-name Friends heading
`[163,-219,143,0]`; text 3 Y prompt `[218,-272,198,-145]`.
Texts 4/5/6/7 are Level/Msg/Voice/Status in adjacent columns x `0..50`,
`50..100`, `100..150`, `150..205`, with y `163..143`. The list name field
must fit its authored width; do not place a service sentence over columns
or invent a ninth text. Same source: original cache/inventory above.

Recovered [Y-menu source](../../tools/reference/halo2-decompiled/src/unknown_2b2d7d.cpp)
supplies the behavior seam: **0x2B386F** opens the selected friend's options;
the empty row opens the name-entry workflow. **0x2B2ED4** binds text child
`(6,0)` to name, `(6,1)` to status, and bitmap children `(8,3)` invite,
`(8,5)` session, `(8,4)` voice. Its voice frames 3/5/4 mean
muted/disabled/available only when actual native voice state supports that
meaning. Its session frames 29/19/0 represent joinable/active/ordinary online.
Discord online presence alone does not establish native session joinability,
rank, voice availability, or a Halo party invitation.

The selected-player action switch **0x2B5D3B**, case 9, invokes
**0x2390F8**, which calls `online_friends_game_invite` for the selected
identity. Friends options are constructed at **0x2B298D**, bound at
**0x2B2AD9**, handled at **0x2B2B40**. These are recovered Xbox behavior
references, not proof that Vista's PC executable constructs all retained
screens identically.
[Selected action source](../../tools/reference/halo2-decompiled/src/unknown_2b116a.cpp),
[game invitation source](../../tools/reference/halo2-decompiled/src/unknown_238e42.cpp).

Current [frontend_party.cpp](../../src/frontend_party.cpp) claims all
screen-26/context-0 and screen-216 flows for party membership; the
[MenuState](../../src/frontend_state.cpp) Y action opens screen 26 and its
horizontal navigation cycles contexts 0/1/2. Consequently Friends is
currently a party roster. Restore Friends ownership explicitly and give the
party roster/options a distinct native context or owner flag; a context
value above 2 alone is insufficient without separating the pane selector
and three-tab cycling. Screen-31 context 42 already identifies party actions,
and context 41 identifies clan actions.

Retain actual Vista-sourced banks/fonts through the existing
[painter](../../src/frontend_painter.cpp). The retained Y-menu geometry is
480-high; shared PC skin-6 resources have incompatible raw row pitch if
blindly imported. Current centered 640×480 canvas uses uniform scale
`min(width/640,height/480)` and the same transform for drawing/hit regions.
The measured mismatch and original PC transform evidence are recorded in
[the layout evidence note](online-match-launch-evidence.md). Friends needs
the same bounded adoption used for clans, plus visible long-name/scroll and
widescreen checks; a new Discord-shaped panel has no original asset basis.

## Relationships, chosen Halo names, and privacy

Use `GetRelationshipsByGroup` with `OnlinePlayingGame`, `OnlineElsewhere`,
and `Offline`; refresh display copies from
`SetRelationshipGroupsUpdatedCallback` and user updates. SDK handles remain
linked to mutable SDK state, so frontend frames should retain immutable
display values plus an opaque local selection handle. Rich presence exposes
this application's activity, **not the friend's other game activities**.
[Official unified list guide](https://docs.discord.com/developers/discord-social-sdk/development-guides/creating-a-unified-friends-list).

Discord recommends showing the game's identity when available, otherwise
the Discord **display name**, not the unique username. Preserve the locally
chosen Halo screen name; do not overwrite it with either Discord name.
Use compatible in-game friends first, then online elsewhere, then offline.
[Official list design](https://docs.discord.com/developers/discord-social-sdk/design-guidelines/unified-friends-list).

The [identity database](../master_server/player_identity.py) already keys
players privately by `(provider,subject)` and deliberately allows duplicate
Halo screen names. The [party service](../master_server/parties_http.py)
returns chosen names and per-party UUID member handles; it resolves typed
invites by a **verified** Discord username. Preserve that authority: names
are labels, never authorization keys. The SDK owner may keep Discord
snowflakes privately to issue SDK operations, but rows, tooltips, dialogs,
logs, voice metadata, and frontend snapshots should expose no snowflakes.
Use local handles for selected friends and server-issued handles for known
Halo accounts. Do not copy the guide's sample user-ID debug output.

The supplied [1.10.19337 public header](../../.local/discord-sdk/discord_social_sdk/include/discordpp.h)
pins the usable calls: `RelationshipHandle::Id()` / `User()` and independent
relationship types at lines 2480–2493; `Client::GetRelationships()` /
`GetRelationshipsByGroup()` at 5168–5178. `UserApplicationProfileHandle`
at 2496 explicitly represents an **external identity provider**, such as
Steam/Epic. Its `Username()` is not an established writer/reader for this
game's locally chosen Halo screen name. Do not use that object to invent a
Halo profile link.

### Server-side relationship proof and Halo-name mapping

Current official OAuth documentation now describes `relationships.read`
as Social SDK access to friends, pending requests, and blocked users, and
links its access application. This is newer wording than search results
which describe only a restricted approval scope. The scope description
does **not** document a REST path, response schema, or establish that
`sdk.social_layer_presence` authorizes an arbitrary relationship HTTP call.
[Official current OAuth scope table](https://docs.discord.com/developers/topics/oauth2#shared-resources-oauth2-scopes).

The official REST integration guide recommends HTTP on the backend **when
possible**, illustrates user bearer versus bot authentication and
`/users/@me`, and points to Discord's OpenAPI. It supplies no server-side
GetRelationships route.
[Official SDK/REST integration](https://docs.discord.com/developers/discord-social-sdk/how-to/use-with-discord-apis).

Both official OpenAPI documents were fetched and inspected in memory on
2026-10-11: each had **153 paths**, zero paths/operation IDs containing
`relationship`, and zero relationship-named schemas. Standard SHA-256:
`7fd37d558c01b7a1a16287bd31cb2b65abfaa372c18e912dc67fbeead5e23653`;
preview: `19717b2f503773f856ab75952da7af935c7f5d6f5226568d6ffa6b965a1f5e80`.
This is a bounded absence finding, not proof that Discord has no private
partner API. **A supported server-side GetRelationships HTTP endpoint and
its authorization have not been established.** Do not guess
`/users/@me/relationships` or add a bot/user-account automation workaround.
[Official standard spec](https://github.com/discord/discord-api-spec/blob/main/specs/openapi.json),
[official preview spec](https://github.com/discord/discord-api-spec/blob/main/specs/openapi_preview.json).

Recommended first implementation uses the **existing verified-username
party route**. The privately selected SDK user supplies `Username()` to
`OnlinePartyCommand{"invite", ...}`. The backend resolves its own verified
`player_profiles.discord_username`, creates a target-bound invite, and
returns the target's chosen Halo name in `party.outgoing`; accepted
membership provides the same chosen name and opaque per-party handle.
Do not send a claimed Halo name. Friends rows without an authorized native
mapping display Discord DisplayName; once the mapping is established, use
the server's Halo name. An SDK username rename can make the registered
username stale until that target reconnects: fail/refresh, never match a
screen name or a different Discord identity heuristically.
[Current party route](../master_server/parties_http.py),
[current native command encoding](../../src/online_account.cpp).

For pre-invite Halo names, a **new** authenticated private resolver could
look up selected SDK IDs, but client-supplied IDs are assertions, not
friendship proof. Per-request bounds/rate limits alone do not eliminate
account enumeration. Restrict disclosure to already shared native
party/clan/room membership or explicit target consent until a supported
trusted relationship-verification route is documented and tested. Return
opaque handles/names rather than snowflakes. Existing native party policy
allows verified-account invitations without a friendship requirement;
retain that explicit policy instead of claiming server-enforced
friends-only membership. Do not upload/store the entire friend graph for
rendering. These are native design recommendations, not Discord API facts.

## Game invitations and blocks

Rich presence must contain party ID, current/max size, a join secret, and
supported Desktop platform. Register the actual installed game's launch
command each SDK startup. `SendActivityInvite` sends to the privately
selected Discord identity. Current invite-guide line 353 expressly says
presence-only invitations can be sent/accepted with **empty message
content**, despite its older prerequisites broadly referring to
communication scopes. Receive into pending UI with
`SetActivityInviteCreatedCallback`; explicit Accept calls
`AcceptActivityInvite`, and `SetActivityJoinCallback` receives the join
secret for acceptance in Discord or the game. The secret is application
defined; Discord does not automatically create our master-server party.
[Official game invite guide](https://docs.discord.com/developers/discord-social-sdk/development-guides/managing-game-invites).

`SendActivityInvite` eligibility includes both users in-game and unblocked,
friendship, or a mutual Discord server plus prior DM. Do not treat a failed
send as membership. The SDK also offers `SendActivityJoinRequest` for asking
to join an available activity; this is distinct from accepting an existing
party invitation.
[Official Client API reference](https://discord.com/developers/docs/social-sdk/classdiscordpp_1_1Client.html).

Native design: retain the master server's invite/accept/version/capacity
checks. Publish a separate expiring opaque party join capability; redemption
must authenticate the accepting player and enforce the intended invite or
party privacy policy. **Do not publish account bearers, match transport
tokens, or Discord IDs as join secrets.** Current party invitation IDs are
target-bound UUIDs, but the common rich-presence join secret is not a
different per-recipient invitation; supporting first-time Halo users or
generic Discord Join needs a new explicit redemption route. For an initial
known-account integration, a Friends-row action can create the existing
master invitation, then acceptance joins only through that owner's checks.
Sending a Discord invite is a user-directed operation, never a background
consequence of rendering the list.

Discord and per-game friendships are independent; a valid Friends row can
have either type. Blocks live in `DiscordRelationshipType::Blocked` and are
**Discord-wide**, removing friendships and cancelling pending requests.
Unblocking does not restore them. Friend requests require explicit user
action; error 80001 must produce a generic failure without disclosing that
the target blocked the caller, and 80009 requires both players to authorize
the same game for game friendship. Current native “Block Future Invites”
stores only a local service block; do not relabel it as a Discord block or
silently call `BlockUser`.
[Official relationship rules](https://docs.discord.com/developers/discord-social-sdk/development-guides/managing-relationships).

Discord blocks do **not** automatically mute voice. Drive the native party
and gameplay voice block filter explicitly; manual unmute must not defeat
an active block. Use private identity-to-room-slot mapping and existing
opaque member handles instead of publishing the documentation example's
comma-separated Discord IDs in lobby metadata. End-to-end bidirectional
voice suppression requires both endpoint/host routing to honor the block;
muting received audio locally alone is not that proof.
[Official voice/block guide](https://docs.discord.com/developers/discord-social-sdk/how-to/voice-muting-for-blocked-players).

## Concrete implementation boundary and validation still required

1. Add one separately linked Social SDK owner, private credential handoff
   from `DiscordLogin`, and display-only Friends snapshots. Expand consent
   and scope validation without changing chosen Halo names.
2. Restore pane 0 Friends, use screen 31 for selected-friend actions and 216
   for Friends options, and preserve party/clan owners through explicit
   context dispatch. Bind status/icons only from verified corresponding
   SDK or native state; do not fabricate ranks or voice state.
3. Invite a selected SDK user through the existing verified-username route,
   use authorized party results for chosen Halo names, and implement authenticated
   join-capability redemption before supporting Discord launch/Join.
4. Verify downloaded headers and runtime on the actual target, real
   consent upgrade, reconnect/sign-out/account change, two real accounts'
   friendship/presence/invite acceptance, expiry/full-party/stale-version
   rejection, blocks, native voice, and long names at actual widescreen
   dimensions. SDK docs and a build do not prove that live loop.

Research commands included bounded `rg`/`sed` over the listed current/native
and recovered files plus supplied public SDK header, Python selection of
inventory screens 26/31/216 and zip names, SHA-256/file metadata, and official
web documentation/API/spec reads. No runtime or account behavior was
tested. This note is the sole file written by this research task.

## Implementation follow-through — 11 October 2026

The delivered Linux integration and its limits are recorded in [Friends](friends.md).
Native party invites from Friends now use the privately selected stable Discord
subject, rather than the earlier username-only proposal. The authenticated
service resolves only a previously verified native account; no raw subject
enters UI snapshots or public responses. The official SDK live read-only probe
confirmed the same account with100 friends and2 incoming requests; it sent
no invitations or messages and retained only redacted counts. Activity invite
and launch-link bridging remain open.

## Native voice block bridge (12 October2026)

The original Xbox voice audience continues to apply; Discord relationship
blocks are a new identity-provider preference. The current official
[voice block guide](https://docs.discord.com/developers/discord-social-sdk/how-to/voice-muting-for-blocked-players)
was rechecked during implementation. It requires explicit bilateral silence
and describes deriving the block list from `GetRelationships()`. The native
integration applies that preference through its own existing Opus relays,
without adopting Discord lobbies or exposing Discord identity metadata.

`DiscordSocial` collects blocked subjects even when an SDK relationship has
no display-user record or the visible Friends list has reached its2000-row
limit. `DiscordVoiceBlocks` privately sends a full replacement to the pinned
ONLINE service using the current memory-only game authorization. Its worker
coalesces repeats, retries an uncertain PUT and drops old work on identity
reset. Neither credentials nor subjects enter UI snapshots or diagnostic
messages. Lists above2000 blocks are rejected as a whole; no partial list
silently removes previously stored blocks.

The authenticated endpoint accepts only its verified owner's preferences,
persists unknown target subjects for future registrations, and returns the
same acknowledgment for known and unknown targets. There is no list/read or
resolver endpoint. Party/clan writers recheck both ends before delivery.
Gameplay relays suppress blocked host-to-listener microphone routes and send
each player only its own room-slot silence mask. Client microphones still
reach the host as transit data so other unblocked players can hear them.
`GameVoice` applies that separate service-owned mask to local playback and
talking indicators; manual unmute changes only the user's manual mask.
Postgame retains the same block owner. Native protocol compatibility v7
prevents admitting older hosts which cannot consume the playback control.

The service retains the last successfully submitted list across SDK sign-out,
disconnect and restart. An SDK reconnect refreshes it, including explicit
empty lists. There is no claim that the service independently verifies a
client's Discord block relationships or learns changes while the SDK is
offline. Failed updates retain the previous list and retry. Server limits
(2000 per player,100000 total), retry cadence, slot masks and the private HTTP
bridge are native service policy, not original Xbox behavior.
