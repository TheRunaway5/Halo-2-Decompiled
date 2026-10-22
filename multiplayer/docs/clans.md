# Native clans and Discord accounts

Asset-policy update, 2026-10-21: all delivered clan/ONLINE fonts, bitmaps and
prompts must come from MCC imports. Vista/TU1.5 supply reference evidence only.
The older donor-specific captures below preserve source geometry/service
evidence, not current MCC-only asset or launcher acceptance. U01/U02/D01,
complete shader/movie ownership and physical/fresh-delivery checks remain open
in [the maintained goal](online-completion-goal.md).

The native ONLINE gate verifies the existing Discord PKCE authorization against
`https://master.halotoforever.com` before admitting the player. No client secret,
bot token, password, email permission or Discord guild membership is needed.
A private, short-lived account bearer is independent of the matchmaking bearer.
Account sign-out clears both native credentials and revokes the account session.
Credentials are never written to profiles, screenshots or logs.

## Using clans

Sign into ONLINE, press Y and select the Clans tab. Create a clan or select an
invitation. X opens Clan Options: recruit, read/send clan messages, or leave.
Select a roster member to change their role or remove them when authorized.
Invitations use the player's **Discord username**, not their editable game screen
name. That player must have signed into this master at least once. Roster rows
show their chosen screen names and roles; account IDs and bearer tokens are
hidden. Voice status appears only for the opt-in clan voice connection.
The source implementation also supports a separate presence label in the
authored status row: ONLINE, Offline, or Playing Halo 2. A response from an older
server leaves that label unknown rather than inventing an offline status.
The historical Vista-bank controls sampled its HD artwork/fonts and shell
framing. Those captures established the authored480-pixel clan layout with
eight41-pixel rows and separate heading fields; sampling did not stretch those
bounds. Full100-member and scrolled rosters were captured at4:3,16:9,16:10 and
ultrawide sizes. The current delivered route must preserve those reference
dimensions using authenticated MCC UI/font owners, with its own screen checks.

Clan Options also offers **Join Clan Voice**. Hold **V** to talk while the game
window has focus; typing in the keyboard pauses transmission. Mute Microphone
stops capture, and member options offer Mute/Unmute Voice. Leave Clan Voice,
leaving the clan, signing out or launching gameplay closes the connection.
See [clan voice and its validation boundaries](voice.md).

Clans have immutable names of at most 15 ASCII letters, numbers and spaces, and
at most 100 members. A player belongs to at most one clan. Joining assigns
Member. The creator is Overlord; additional Overlords can be promoted, with an
irreversible-action confirmation. Staff can change Peons/Members, including
promoting them to Staff. An actor cannot change/remove a peer or higher role.
At least one Overlord must remain while other members exist. The last member's
confirmed departure deletes the clan. These rules are grounded in the
[original manual, routines and authored screens](halo2-clan-evidence.md).

## Native service choices

These are replacement policies, not claims about the retired Xbox Live server:

- Names are trimmed and case-insensitively unique on this master. No rename.
- Verified Discord usernames are indexed at sign-in; lookup is a directory of
  registered players, not a live global Discord search. Screen names may collide.
  Username changes update the private alias on the next verified sign-in. A
  reused username remains associated with its last verified owner until the new
  owner signs in, so recipients should verify invitations before accepting.
- Invites expire after seven days. There are at most 100 pending invites per
  clan/recipient. Blocking rejects future invitations from that clan. Staff may
  cancel pending invites, and recruiters may cancel their own invites.
- Broadcasts are clan-private **text** messages, a native replacement for the
  original messaging transport. Member and above may send. Retain at most 100
  per clan for 30 days; return the newest 20. No voice recording, attachment,
  delivery receipt or push notification is claimed.
- One account session per player, 15-minute lifetime, automatic renewal using
  the still-valid in-memory Discord authorization. Process restart requires a
  new session but preserves players, memberships, invitations, blocks and text
  messages in SQLite. Native retry/polling repairs that session automatically.
- Presence is observed native service activity: an authenticated account or
  matchmaking request within the last 30 seconds, or a live match connection
  with the existing challenge/expiry checks. Running matches publish Playing
  Halo 2. A stopped menu client becomes Offline after that activity window even
  if its admission bearer has not expired. This freshness window is a native
  policy, not an original Xbox Live timing claim. Retail routine `0x2b2ed4`
  supplies the evidence for a separate status row and offline/online distinction;
  the replacement transport defines how its own presence is observed. Only
  clan members see this data, and client-supplied presence is rejected.
- Mutations have UUID replay keys retained for 24 hours (256 commands per player
  per day), with authorization and transaction commit in one event-loop turn.
  At most 1,000 registered players/clans/account sessions on this initial host.
- Clan state belongs to this master. Directory federation exchanges listings;
  it does not replicate private social data. Eligible clan members can be
  invited into accepted [native parties](parties.md), with explicit invitation
  acceptance and per-member playlist consent. Minor/Major Clan Lab queues
  now use recovered launch lineup rules and separate clan-owned XP;
  see [clan matchmaking and its evidence limits](halo2-clan-matchmaking-evidence.md). Native
  [match launch](match-launch.md) is a separate authenticated service.

## Account and clan HTTP routes

`POST /v1/account/session` takes existing Discord Bearer authorization and
`{"screen_name":"Chosen Name"}`. The backend verifies the Discord application's
ID and `identify` scope, resolves the stable private player, and returns
`{"token":"private-session-bearer","expires_in":900}`. It ignores no extra
fields: client-supplied identity, Discord username or role is rejected.
`DELETE /v1/account/session` revokes that bearer.

`GET /v1/clans` requires the account bearer and returns the caller's clan roster,
role, visible outgoing invites, newest messages and incoming invitations. Private
Discord IDs/usernames never appear. Opaque UUIDs are machine action handles;
the native UI never displays them. Nonmembers cannot fetch another clan roster.
New clients request `GET /v1/clans?presence=1`, adding the bounded `presence`
member field (`offline`, `online`, or `playing`). The default response retains
the original four member fields for older clients. The same optional query is
supported on clan command responses. No bearer or private Discord identity is
added to the roster. Presence is deployed in release `20261011T224537Z`; both
installed native launchers select its SDK-enabled delivery. Existing open game
processes need a relaunch to use the new status labels. The 30-second freshness,
logout, queue activity, and real match-connection lifecycle checks passed in the
74-test backend suite, including inside the deployed Docker image. Native
parser compatibility and duplicate-name selection passed; 17 clan states were
captured at six sizes (102 images), with Vista bitmap/font byte checks and
foreground text/hit bounds checked. Installed title/sign-in and 60-frame Slayer
smoke checks passed at an actual 1280x720 with Vulkan validation enabled.

`POST /v1/clans/commands` requires the account bearer and a fresh `request_id` UUID
plus `action`. Exact per-action fields:

| Action | Additional fields |
|---|---|
| create | name |
| invite | username (verified Discord username of registered recipient) |
| accept / decline / block / cancel | target (invitation UUID) |
| remove | target (member UUID) |
| role | target, role (0–3), confirmed (true required for Overlord) |
| leave | none |
| broadcast | message (maximum 256 UTF-8 bytes) |

Success returns the updated caller view. A replay with the same key/payload
returns current state without applying another mutation; a changed payload with
that key returns 409. Missing/expired sessions return 401. All mutations enforce
permissions server-side; no UI selection is trusted.

## Configuration and verification

`H2_ACCOUNT_URL` overrides the default HTTPS origin; an invalid/empty endpoint
keeps ONLINE gated. Plaintext is only allowed for literal loopback when
`H2_ACCOUNT_ALLOW_LOOPBACK_HTTP=1` is explicitly set. Redirects and ambient proxy
settings are disabled for native credential requests; TLS verification stays on.

```sh
python3 -m unittest discover -s multiplayer/master_server/tests -t . -v
cmake --build build-performance-20261010 --target halo2_native online_account_tests frontend_online_tests online_login_probe
ctest --test-dir build-performance-20261010 -R 'online_account_client|frontend_clans_http|discord_identity' --output-on-failure
# Interactive live check: open the generated URL in the signed-in browser.
build-performance-20261010/online_login_probe /tmp/halo2-discord-authorization-url "Your saved screen name"
```

The live probe uses the actual production PKCE/account clients and performs no
clan mutation. Pass the user's saved game screen name as its second argument. The URL contains
only the public OAuth request and PKCE challenge, never the verifier/access token.
Delete that temporary URL file after the check.
