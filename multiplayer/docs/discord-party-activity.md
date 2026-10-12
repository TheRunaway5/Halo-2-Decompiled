# Discord activity party handoff

The service and native account client support the authenticated
party handoff needed by Discord game invites. The staged SDK adapter now
sequences explicit sending, rich-presence publication, invitation reception,
user-driven acceptance and launch registration. Backend release
`20261012T154813Z` is deployed: 154 local and production-container tests passed,
51 running files were independently verified, and its private SQLite backup
was restored and checked. Native integration and activation are recorded in
[the integration report](native-online-integration.md). Real Discord invitation
delivery and cold-launch behavior still require a two-account live check.

The separate Friends consent now requests `identify openid sdk.social_layer`.
Discord's current game-invite guide requires communication scopes; the earlier
presence-only grant is sufficient for friends/presence but not this invite
flow. Identity-only ONLINE sign-in remains separate. A narrower returned grant
is rejected without changing that identity.

Discord's [game invite guide](https://docs.discord.com/developers/discord-social-sdk/development-guides/managing-game-invites)
defines its join secret as application-owned data and requires rich presence
with party information and supported platforms. Accepting an SDK invite or a
Discord-client invitation returns that secret to the application. Discord does
not authorize membership in this game's master-server party. The supplied
Social SDK1.10.19337 headers expose the corresponding send, accept, join-callback
and launch-registration APIs. This handoff retains the existing native
known-account invitation policy.

`POST /v1/party/activity` accepts `{"version": <current version>}` under
the current account-session authorization. Only a party leader with room for
another member and no party member in an active match can issue it. Its private
response contains `party_id`, `version`, `current_size`, `max_size`,
`join_secret` and `expires_in`. The party ID is a fresh presentation alias,
independent of private account and service-party IDs. The join secret is a
versioned prefix plus32 cryptographically random bytes encoded as URL-safe
text. The lease lasts600 seconds. A retry within the same live party version
returns the same alias/secret instead of invalidating an invitation in transit.
An optional `join_secret` conditionally refreshes an existing capability after
an admitted member joins. It cannot issue a missing capability, rotate the
alias/secret, renew the original expiry or bypass recipient authorization.
The ordinary `GET /v1/party` schema remains unchanged and contains neither.

`POST /v1/party/commands` accepts action `join_activity`, `join_secret` and the
usual `request_id`. No client-supplied target, version, roster or Discord
identity is accepted. Redemption requires all of the following:

- An authenticated ONLINE account with a live pending native invitation to
  exactly the linked party. Possession of the shared activity secret alone
  cannot admit an uninvited account.
- A live link for the party's current version, a nonexpired target invitation,
  no existing accepting-player party/match, a party below16 members and no
  party member in a match.
- The native inviter block policy and the last successfully synchronized
  bilateral Discord block preferences permit the invitation. These stored
  preferences do not independently prove a Discord friendship or current
  provider-side relationship state.

Native and activity acceptance use the same transactional membership owner.
It cancels the accepting player's queued search, adds the verified identity
with a new opaque member handle, consumes that player's pending invitations,
clears old party search consent and carries the same bounded activity link to
the new version while room remains. Other separately invited recipients can
still accept it. There is no await between authorization and mutation.
Transfer, departure, removal, unready, full capacity and matchmaking session
loss invalidate links. Expired links are cleaned up. Links persist across a service
restart for their remaining lease; account-session authorization must be
reestablished normally.

The existing request-ID ledger handles uncertain network results. An identical
successful replay returns the current party view without another admission;
changing the payload under the same request ID fails. Replaying an old success
after leaving does not rejoin the player. Link expiration, cancellation,
decline, blocks and capacity failures leave membership untouched.

`OnlineAccount::party_command({"join_activity", secret})` provides the native
worker handoff. It rejects malformed secrets and asserted versions before
transmission, sends the secret only in the authenticated request body to its
pinned origin, and retains the same request ID on an uncertain retry. The
normal response parser supplies chosen Halo names and opaque roster handles.
The secret is absent from the public account snapshot, status text and
ordinary party response. Neither credentials nor join secrets belong in
preferences, command-line arguments, UI or logs.

## Validation and remaining work

### Staged SDK wiring

The follow-up capsule `.local/discord-activity-sdk-20261012` connects the
existing Vista Friends-menu `Invite To Party` action to a private coordinator.
It waits for the authenticated native target invitation and the matching
activity-link response, publishes party size, public alias, join secret and
Desktop support with the official SDK, and calls `SendActivityInvite` only
after publication succeeds. The content is empty. Only the explicit menu
action can initiate this send sequence; pumps and retry callbacks cannot
create another Discord invitation. No real invite was sent by the checks.

The account worker exposes a private operation receipt to that coordinator,
not to menu snapshots. Server rejection, malformed resources, version/size
mismatch, expiry, sign-out, queued departure, match entry and owner destruction
prevent a pending publication from sending an invite. An uncertain native
invitation retries its original request ID. SDK send failure is reported
without an automatic resend or a membership-success claim. SDK callbacks
cannot introduce arbitrary provider errors or private identifiers into status
text. Friendship is checked again in the SDK immediately before sending.
The current party lease is independent of a recipient's operation receipt.
Preparing another explicit invitation, rejecting that recipient, or failing
its SDK publication/send leaves earlier valid invitations intact. Native
target invitation/cancellation also retains the lease. Membership refresh
publishes the updated roster and private lease atomically, bounded by the
original local expiry. Structural mutations, sign-out, identity changes,
expiry and match entry still invalidate presentation immediately. No refresh,
poll or callback sends another invitation automatically.
The credential owner reads the Discord bearer and verified subject atomically,
replaces the ONLINE session when that subject changes even if the chosen Halo
name is identical, and invalidates old invite/voice handoffs immediately. A
same-name account-switch fixture verifies the replacement session and rejection
of the previous account's pending publication.

`SetActivityJoinCallback` redeems a user-accepted Discord invitation through
the same account worker and authoritative server roster. It can wait for
ONLINE sign-in; it does not leave an existing party or interrupt a match.
The loopback HTTP probe now exercises this coordinator path against the real
local backend with external Discord replaced by an identity fixture.

The launcher supplies its own executable path to the runtime privately through
`H2_DISCORD_LAUNCHER`. The SDK registers that launcher with `--skip-intro`, so
asset and selected gameplay-executable resolution remain owned by the launcher.
Neither a join secret nor a credential is an argument. The staged launcher
checks replace a stale inherited path and preserve Vista assets, explicit
overrides and TU selection in disposable projects with spaces. Registration
itself and actual Discord cold launch still require live verification.

The focused Release harness compiles the real account/coordinator, both SDK
and non-SDK social implementations, both launchers and the three changed menu
adapter objects and the changed frontend fixture. Each configuration runs account,
coordinator, real local HTTP, consent isolation, PKCE and launcher-routing checks.
This is component and transport
evidence, not a full game link/render or authenticated Discord messaging test.
The follow-up `.local/discord-inbox-20261012` adds private SDK invite-created
and invite-updated storage and explicit `AcceptActivityInvite` from the native
Friends screen. Its public rows contain opaque local handles, display names
and fixed status labels. Only Join invitations for this application, from
another unblocked user, are admitted. Receiving an invitation does not accept
it. The existing Vista selected-player dialog supplies Accept Game Invite,
Dismiss Invitation and Back; it does not add a new popup layout.

The inbox rejects stale handles, repeated clicks, expired updates and late
callbacks after reset, dismissal, timeout, eviction or owner destruction.
It checks party/game state before and after the SDK result, then passes the
private secret to the existing account coordinator. That handoff is not a
membership success claim. The actual party lobby opens only when the account
worker receives authenticated server membership. Existing native invitations
retain their acceptance flow. Both SDK and non-SDK focused Release builds pass
all seven component tests with no skips. The SDK adapter follows the
[official game-invite guide](https://docs.discord.com/developers/discord-social-sdk/development-guides/managing-game-invites)
and the locally supplied1.10.19337 headers.

The inbox follow-up also builds the frozen full runtime and frontend fixture.
Six focused native tests pass with no skips, including the real local HTTP
redemption and native identity, matchmaking, clan and party menu flows.
Thirty-six GPU captures cover six invitation states at six dimensions using
the imported Vista font and bitmap banks. Bounds and mouse projections pass,
and the populated list plus long-name dialog were visually inspected.
The host has no Vulkan validation layer, so captures explicitly disable it.
External identity and SDK completion remain fixtures; no real Discord message
was sent and no authenticated game was launched. See the retained report at
`.local/discord-inbox-20261012/verification.json`.

The isolated capsule is `.local/discord-activity-20261012`; it freezes the
backend and the relevant native account/identity sources. Its minimal native
CMake harness builds the account component and tests, not the full game.
All150 backend tests pass, including seven new HTTP cases covering target
binding, expiry, changes, retries, cancellation/decline, native and Discord
blocks, full parties, competing acceptances, schema rejection, privacy,
revoked sessions and restart durability. Active-match rejection cases supply
the existing `playing` predicate explicitly; they are component policy tests,
not new live gameplay evidence. Existing native-party tests also pass after
sharing the membership gate.

`online_account_client` passes private request encoding, uncertain retry,
chosen names and sign-out checks. `online_party_activity_http` runs the real
native account HTTP transport in a separate process against an actual local
master-server application. External Discord identity is replaced with an
explicit fixture. The parent creates a target invitation and sends the join
secret over stdin; the inbox's explicit accept action hands a fixture SDK
result to the native worker, which redeems it. Independent server inspection
confirms the two chosen names, consumed outgoing invitation and invalidated
old link. The fixture checks its output for private-value leaks. It does not
send a Discord message or launch an authenticated game.

The local Docker Desktop and default Docker endpoints were unavailable during
this increment. Production Python/container validation has not been run for
these new changes. Installed runtime activation, safe deployment and a real two-account Discord
invite/launch check remain required. First-time unregistered recipients and
generic Discord Join without a preexisting native invitation remain outside
the retained known-account policy; no generic party admission has been added.
