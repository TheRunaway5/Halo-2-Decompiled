# Discord Friends in ONLINE

The native Friends tab uses Discord's official Social SDK. It requests
`identify openid sdk.social_layer` through a separate PKCE consent
flow after the player signs into ONLINE. Declining, expiring or disconnecting
Friends leaves the existing identify-only ONLINE account intact. The SDK's
authenticated account must match the game's verified Discord subject.
The communication scope is required by Discord's game-invite guide. A previous
presence-only Friends grant must be replaced by completing the separate
Friends consent again; it cannot silently gain invitation permissions.

The application owns one SDK connection across menus and gameplay. It pumps
callbacks on the native thread, refreshes relationships on SDK updates, and
publishes the chosen Halo screen name with this game's menu/game presence.
The regular ONLINE token and the separate Friends token remain in memory.
Sign-out or an account change clears the SDK owner, relationship copies,
opaque row handles and rich presence. SDK logging is disabled.

FRIENDS remains original screen26/pane0; CLAN is pane1; accepted party members
use the PLAYERS tab, pane2. Friend actions reuse screen31 and Friends Options
reuse screen216. All use the imported Vista bitmap/font banks and the same
uniform foreground/hit-test transform as the other ONLINE screens. Friends
rows show Discord display names, presence, and incoming/outgoing requests.
Blocked relationships are hidden. Duplicate names retain selection through
an opaque local handle; Discord account IDs are never rendered.

An accepted party's leader can select a friend and choose Invite To Party.
That action privately hands the selected SDK account's stable subject to the
authenticated native party invitation service. A renamed or recycled Discord
username cannot redirect that invitation. This route retains the native
known-account invitation policy; the service does not claim to verify the
client's Discord friendship assertion. The recipient must have registered an
ONLINE account. The native invitation and Discord game invitation share the
same service admission gate; the received Discord invitation has its own
explicit Accept Game Invite action in FRIENDS.
The party roster displays each member's chosen Halo screen name,
independent of their Discord display name. The Friends screen also allows
accepting/declining incoming Discord friend requests and canceling outgoing
requests, only after an explicit native menu action.

The [authenticated activity handoff](discord-party-activity.md) now
connects the Vista Friends-menu invite action to native target invitation,
SDK party presence and explicit activity sending. The user-accepted Discord
join callback redeems through the account worker, and launch registration
uses the actual launcher's asset/configuration setup. Component checks use
SDK callbacks as fixtures and real local account HTTP; no Discord messages
are sent. The activity inbox receives SDK invitation updates and
offers explicit Accept Game Invite and Dismiss Invitation actions in the
existing Vista selected-player dialog. Private SDK records and join secrets
stay outside the public menu snapshot. Expiry, duplicate names, repeated
clicks, disconnects, account changes and late callbacks are covered by the
component checks. Backend release `20261012T154813Z` preserves the bounded
link across separately authorized member admissions. The native private lease
also survives another recipient's invitation preparation/failure without
clearing earlier invitations. See [native integration](native-online-integration.md)
for current activation and validation. Real Discord delivery/cold launch remain
unverified until the two-account live check.
Native party/clan invitation blocks remain separate
from Discord relationship blocks. These pieces are tracked in the completion
audit.

When Discord Friends is connected, its private block snapshot also feeds the
native voice service. A block silences both directions in party, clan, game
and postgame voice. The service resolves verified identities privately; no
Discord ID, block list or arbitrary identity lookup is exposed to other
players. Manual voice unmute cannot clear a Discord block. Match hosts still
relay microphones to unblocked listeners while suppressing blocked local
playback. The existing Opus and original-style audience owners remain in use.

`PUT /v1/account/voice-blocks` uses the existing Discord authorization and is a
full, idempotent preference replacement. The SDK owner survives world/menu
teardown, coalesces unchanged snapshots and retries failed updates. The last
successful list persists on the service across SDK disconnects, sign-out and
server restarts; reconnect Discord Friends to refresh changes made while it
was disconnected. An explicit empty SDK snapshot clears the list. This is a
client-provided preference bridge, not independent server verification of
Discord's relationship graph. A failed network update retains the previous
successful list until its retry succeeds. The native queue compatibility is
v7 because older native hosts cannot apply the private playback mask.

## Building and checking

Set `HALO2_DISCORD_SOCIAL_SDK_ROOT` to the extracted official SDK directory
containing `include`, `lib/release`, and `License-Notices.txt`. Without it,
ONLINE identity and native parties still build; Friends reports unavailable
and does not request unusable authorization. The Linux delivery copies the
release shared library and notices beside the executable and searches its
own directory first. Native Windows/macOS packaging remains unvalidated.

`discord_social_consent` exercises denial isolation, different-account
rejection before SDK connection, stale actions, and sign-out cleanup.
`frontend_online_identity` uses synthetic records to check duplicate focus,
100-friend scrolling, long display names, pending actions, private identity,
Vista asset bytes, text bounds, and mouse projections across nine sizes.
It also checks game-invitation focus, expiry, acceptance and dismissal.
`discord_activity_inbox` exercises explicit SDK acceptance through callback
fixtures, timeout, bounded retention and stale-owner isolation. The real local
`online_party_activity_http` check enters through that same inbox handoff;
its SDK result is a fixture, not an authenticated Discord invitation.

The current frozen Release runtime builds with the official SDK. Fifteen
focused native checks pass without skips: identity, account, SDK consent and
invitation state, real local account/menu HTTP, ranked and casual accepted
parties, a full sixteen-player clan match, bilateral voice blocks and settings
mismatch rejection. The matched clients complete gameplay, Opus, final result
agreement, postgame voice, actual Vista PCR navigation and requeue.

The nine-size capture matrix independently compares ONLINE bitmap/font bytes
with the Vista bank while MCC Classic is selected for the surrounding shell.
It covers full clan/party rosters, long names, invitation actions and the
matchmaking panes; final counts, hashes and visual review are in
`.local/discord-party-continuity-20261012/verification.json`. The host lacks the
Vulkan validation layer, so captures explicitly disable it. They establish
native asset/layout behavior, with external Discord fixtures; they do not
establish original Windows framebuffer parity or real Discord delivery.

`frontend_parties_http` also drives a synthetic Friends selection through the
real native account client and authenticated test party service. These tests
do not read or mutate real Discord relationships.

The manually invoked `discord_friends_probe` target performs a separate live,
read-only OAuth/SDK connection. On11 October2026 the user authorized it and
the official SDK authenticated the same account, returning100 friends and
2 incoming requests. It sent no invitations/messages, logged only counts,
and verified sign-out cleared state. Redacted evidence lives locally at
`.local/discord-friends-probe/verification.json`; no contacts or credentials
are saved there. This establishes a real SDK connection, not an end-to-end
Discord activity invite or physical multiplayer/headset test.

See [primary research and source seams](discord-friends-evidence.md),
[native parties](parties.md), [two-account manual check](two-account-live-check.md),
and [completion audit](online-completion-audit.md).
