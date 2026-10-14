# ONLINE couch players

The native queue supports one to four local players on a PC. Sign the primary
profile into Discord, enter ONLINE, then press A or Start on another controller.
Choose an existing screen-name profile, create one, or choose Guest. In that
controller's local-player dialog, select Play as Guest or Sign in with Discord.
X reopens the local player's options. B at the ONLINE root or party lobby removes
that controller's player. Roster changes are unavailable while a search or game
handoff is active. Keyboard and mouse can remain assigned to player one.

Each additional Discord login must authorize a distinct account. A chosen local
screen name is independent of its Discord username. Credentials and Discord
subjects stay private; neither the local-player dialog nor match rosters display
them. Local authorizations survive the match and PCR Done in the retained menu
session. They are cleared on primary sign-out and are not written to profiles.

Guests use casual playlists. Ranked and clan playlists require independently
verified Discord identity for every seat. The server enforces this rule and the
playlist's maximum party/team size. A four-player couch enters as one indivisible
party, using the existing skill/candidate/gathering policy. Guest seats are
excluded from the authenticated party's skill average and from XP comparisons.
The new eight-player Casual Team Slayer and Ranked Team Slayer pools are
explicitly custom Lockout validation rotations, not recovered historic hopper
rotations. Ranked requires four verified accounts for a four-player couch.

Queue protocol v9 describes logical player slots and their owning physical
connection. One authenticated QoS/relay connection per PC carries all local
inputs. The host admits the complete group at server-selected slots with its
server-selected team and screen names. Loading counts actual admitted players,
not PCs; four cameras/HUDs retain the existing local split-screen renderer and
the original Vista UI sampling. Controller instance bindings are retained in
local-player order across launch and PCR return.

The master decides whether a seat is a guest from verified authorization, never
from its name or a client-selected `guest`/`publish_stats` flag. Guests receive
random, transient seat identities. They create no persistent master-server account/profile,
personal result, match membership, playlist rank/XP, or skill-history rows. Team
totals and the match digest are still retained. A verified player named “Guest 2”
receives their own stats normally.

The complete original PCR remains visible locally, including guests. Its raw
bytes transit the relay in memory so the participating PCs can agree on their
native final state; raw reports are not persisted as master-server statistics.
Guest personal rows are filtered during server commit. This does not claim that
guest report details never transit the server.

Voice uses one shared microphone and playback device per PC. Proximity admission
can use any local listener, and each Opus frame is delivered once per PC. Shared
playback respects every authenticated local user's block preferences; guests
inherit the primary's policy. Separate controller headsets are not implemented.

Validation is recorded under `.local/online-couch-20261013/`: real HTTP, measured
QoS, relay and native gameplay between two independent four-player processes;
complete eight-player source PCR agreement/Done/requeue; mixed authenticated and
guest result publication; finite four-view Vulkan captures; and nine-size Vista
bitmap/font, glyph-bound and pointer audits. External Discord authorization is
synthetic in repeatable tests. Physical four-controller hardware, real secondary
Discord accounts and WAN couch sessions still require human runtime validation.
The optional Vulkan validation layer is unavailable on this host; UI captures
record that limitation. Existing gameplay/effect parity gaps remain outside this
queue and local-player integration.
