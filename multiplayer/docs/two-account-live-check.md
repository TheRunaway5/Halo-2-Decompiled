# Two real Discord accounts

The installed Linux game can run two manually authenticated clients on one PC.
`tools/online_two_account_check.py` gives each client a separate SDL profile
directory under `.local/two-account-check`; the ordinary saved profile is not
copied or edited. Each starts at the native ONLINE sign-in screen in a 1280x720
window, using the Vista assets and bundled Social SDK.
Preparation and the signed-out smoke check perform no login or social mutation.
The smoke check uses the normal renderer configuration by default. Add
`smoke 1 --validation` to require the optional Vulkan validation layer; a host
without that layer will fail explicitly. The smoke report records its use.

```sh
python3 tools/online_two_account_check.py prepare
python3 tools/online_two_account_check.py launch 1
# In a second terminal:
python3 tools/online_two_account_check.py launch 2
```

The preparation folder also contains `Player 1.desktop` and `Player 2.desktop`.
The prepared installation has matching shortcuts on the Desktop named
**Halo 2 ONLINE Test - Player 1** and **Halo 2 ONLINE Test - Player 2**.
Closing a game releases that player's launch lock. `status` checks the live lock;
the existence of an old PID or state file is not treated as a running client.
Preparation copies the native runtime, title launcher, SDK library and notices
into a private directory, so another build replacing the normal launcher cannot
change these test clients. The renderer's compiled shader directory and asset
registry are checked before launch. If those change, repeat `prepare` to verify
the new installation. Existing isolated profile settings are preserved.
To select a tested build explicitly, pass its title launcher with
`prepare --launcher /absolute/path/to/Halo2-Title-Launcher`.

## User-operated login and invitation

1. Sign Player 1 into the main Discord account. Finish its browser authorization
   before starting Player 2's sign-in. Both use the already registered loopback
   callback on port 2003, so only one authorization can wait at a time.
2. Sign Player 2 into the second Discord account. Both use the existing Halo 2
   application; no second developer application is needed. Keep a separate
   browser profile or switch Discord browser accounts before authorizing.
3. In each client, choose a Halo screen name. Each isolated profile initially
   uses Test Pilot 1 or Test Pilot 2. The names are independent of Discord names.
4. Enable Discord Friends separately in each client and authorize the same
   Discord account that client used for ONLINE. The two Discord accounts must
   be friends for this Friends-list test. Reconnect Friends if it was enabled
   before the communication-scope update; the new consent includes game
   invitations, while the basic ONLINE identity grant stays independent.
5. If both accounts still share the party from an earlier check, have Player 2
   choose Leave Party first. Player 1 can retain that party or create one if
   needed. Select Player 2 in FRIENDS and choose Invite To Party. Wait for the
   explicit Discord send result. On Player 2, open FRIENDS,
   select the received game invitation and choose Accept Game Invite. Verify
   both chosen Halo names and leader/member actions. Accepting through PLAYERS
   checks the separate native invitation path and does not prove SDK delivery.
   This changes the real production party state; it is never an automatic
   preparation action.
6. Join Party Voice explicitly on both clients, then hold V in the focused game
   to talk. Use headphones to avoid a feedback loop. Mute one member and verify
   suppression, then leave the party/voice when finished.

For a cold-launch check, have Player 2 leave the party and close its game.
Explicitly send a fresh invitation, accept it in Discord, finish normal ONLINE sign-in in the launched
game, and verify party admission. With a third willing friend, send two
separate invitations and accept the first before the second; both should keep
their original expiry and remain usable while capacity permits. No helper
automatically sends these invitations or accepts them on the user's behalf.

The production two-player Slayer playlists accept solos. The four-player Team
Slayer playlists accept parties of two and require four actual participants;
two accounts in one party cannot fill a four-player queue by themselves.
Do not change the production catalog or manufacture extra players to pass this
check. Two accounts may instead queue as solos for two-player Slayer. Existing
ranked results are real persistent state; use Casual Lab Slayer for a functional
check that should not change visible ranked progression.

The helper checks the real bundled SDK path and pins the installation used for
the test. Logs are local and owner-readable only. OAuth grants and account
bearers remain owned by the game's existing memory-only login/client workers;
the helper never reads or exports them. This same-PC check can establish real
identity, invitations and service transport. It does not establish two physical
players' microphones/headsets, different internet links or impaired-WAN quality.
Real SDK delivery, cold launch and native block-to-voice validation remain
tracked separately in the [completion audit](online-completion-audit.md).
