# Native ONLINE parties

ONLINE → Create Party opens the original matchmaking lobby with imported Vista
fonts and bitmap assets. X opens Party Options; Y opens accepted members and
invitations. Switch to FRIENDS to enable Discord Friends with separate consent;
a leader can select an existing friend and choose Invite To Party. This privately
resolves the stable selected Discord account. The recipient must already have
an ONLINE account. Invite Player also accepts a previously verified Discord username.
The roster displays each person's chosen screen name. Identical names are
allowed; commands use private per-party handles, which are never rendered.

Invitations must be accepted by their recipient. Leaders can cancel outgoing
invitations, transfer leadership or remove a member. Anyone can leave. If the
leader leaves, the oldest remaining accepted member becomes leader. Accepted
membership persists across service restart; playlist consent does not.
Invite Clan To Party sends invitations to eligible observed online clan members;
it never silently adds them. Clan recruiting roles gate sending, while Peons
can accept invitations and participate normally.

Each member opens Playlists and chooses the same playlist. Members wait for
the leader, who starts the one authoritative group ticket. The service checks
every accepted member's current consent, catalog revision, game compatibility,
installed maps and queue session. Native members discover the leader's ticket
through authenticated polling. Clients cannot supply rosters, ranks or teams.
Whole parties stay on one team. Cancel, leave, removal, leadership change or
queue-session loss cancels an unadmitted group search and clears stale consent.
Parties cannot change membership during an active admitted game.

The shipped validation catalog has four-player Ranked/Casual Lab Team Slayer
on Lockout, accepting parties of two and solo players. These are explicitly
custom playlists. They do not represent the complete historical Halo 2 catalog.
Ranked results use the separately pinned retail launch XP configuration;
casual has no fabricated hidden rating update.

Party Options → Join Party Voice opens the passive accepted-party channel.
Hold V to talk. Microphone mute, member mute and Leave Party Voice are local
controls. Switching clan/party channels joins the old device worker before
opening the new owner. Starting a game carries an existing party voice opt-in
into source-routed match voice. PCR keeps the admitted match audience; Done
joins that audio worker before the opted-in party channel resumes. Remote idle
device state is unavailable in the party relay, so the lobby does not invent
an enabled icon for a silent remote member.

`GET /v1/party` and `POST /v1/party/commands` use the ONLINE account session.
Mutations carry an idempotency key and membership version. Invitation acceptance
checks the version actually displayed by the recipient. Invitations expire
after ten minutes. Requests, invitations, memberships and queues are bounded.
These HTTP/consent/expiry policies are native replacements; they are not claimed
as recovered Xbox Live wire protocols.

`GET /v1/party/voice` uses the same pinned HTTPS origin and authenticated
WebSocket/Opus framing as the clan relay. Server-side accepted membership owns
both audience and per-party speaker handles. Outstanding invitations grant no
voice access. Removal, logout, expiry and entry into active gameplay revoke the
passive channel. Frames use 20ms Opus, bounded queues and sequence/rate checks.
The service stores no microphone audio. Physical headsets and real WAN quality
still require independent validation.

Evidence and reproduction:

- [Original UI, invitations and session voice](online-party-integration-evidence.md).
- `python -m unittest multiplayer.master_server.tests.test_parties_http multiplayer.master_server.tests.test_party_voice -v`.
- `ctest --test-dir build-online-social-20261011 --output-on-failure -R 'frontend_parties_http|online_match_party_launch|online_party_voice_http'`.
- `DISPLAY=:197 python tests/frontend_parties_http_test.py build-online-social-20261011/frontend_online_tests --capture .local/party-ui-final` captures actual Vulkan output at640x480,1280x720,1280x800,2560x1080,2560x1437 and1001x601, including sixteen legitimately accepted members and the last scrolled row.

Original clan matchmaking/rating services, split-screen account admission,
in-game host migration/backfill, missing historical policies and the full
historical game pool remain separate requirements.
