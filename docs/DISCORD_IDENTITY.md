# ONLINE identity

The main menu's Xbox Live entry is now **ONLINE**. Its original screen 20 is
the Discord sign-in screen, and screen 186 remains the ONLINE service menu.
The original layout, font resources, screen IDs and tag files are retained.
This is a requested native service replacement, not recovered Xbox Live logic.
The source menu routes are recorded in `src/frontend_state.cpp`: main screen 6,
label 0x283, account screen 20 and service screen 186.
The guards follow the actual disc tag names under `dialog_live_*`,
`online_y_menu` and `game_shell/xbox_live`. Generic fade/error/progress and
Extras screens retain their offline uses. Legacy Xbox password screen 21 routes
to Discord's browser sign-in instead of collecting credentials in the game.

Select **ONLINE**, then **Sign in with Discord**. Complete authorization in your
browser and return to the game. The game enters ONLINE only after the token
exchange succeeds and Discord's `/users/@me` endpoint returns a valid user.
The stable Discord user ID is kept internally and is never shown in the menus.
The player's saved local profile name remains their screen name, separate from
their Discord username. **Discord Account → Change screen name** uses the existing
profile-name keyboard, validation and persistence; changing it keeps the same
verified Discord identity. The account panel shows the chosen name and Discord
username and offers **Sign out of Discord**. ONLINE panels suppress the retained
title/main-menu UI, whose original `start_screen` bitmap supplies the Halo 2 logo,
so the logo and title prompt cannot overlap the information. The world background
and authored panel geometry remain in use. Back cancels an outstanding sign-in. Failed, declined,
cancelled or expired sign-ins cannot enter the service screens, including the
Players shortcut and direct menu navigation. Offline modes use local profiles.

Application ID: `1558522073272426519` (public, compiled into the native login
options). No bot, email, friends, guild or messaging permissions are requested.
The application public key is not an OAuth credential and is not used.

The application's OAuth2 settings in the Discord Developer Portal must have:

- **Public Client** enabled, for authorization-code exchange with PKCE and no
  embedded client secret.
- The exact redirect URI `http://127.0.0.1:2003/discord/callback` registered.

The callback listener binds only IPv4 loopback while signing in. Each attempt
uses fresh cryptographic state and an S256 PKCE verifier. Invalid or duplicate
callback parameters are rejected. Discord API traffic uses HTTPS with peer and
host verification, bounded response sizes and timeouts, and no redirects.
OAuth runs on a cancellable worker while the game continues rendering.
The login lifetime comes from Discord's token response. Access/refresh tokens
are never written to menu preferences, logs or files; the current implementation
keeps the access token only in the private login worker for the configured
matchmaking service handoff. Expiration or game restart requires another login.
Sign-out clears local identity and authorization and revokes the queue session.

Discord's documented authority for the integration:
[OAuth2 and identify scope](https://docs.discord.com/developers/topics/oauth2),
[user endpoint](https://docs.discord.com/developers/resources/user), and
[public clients with PKCE](https://docs.discord.com/developers/discord-social-sdk/development-guides/account-linking-on-mobile#authentication-flow-for-public-clients).

Build dependencies add libcurl 7.85+, OpenSSL Crypto and nlohmann-json 3.10+.
Focused verification:

```bash
cmake --build build-multiplayer --target discord_login_tests frontend_online_tests halo2_native -j 3
ctest --test-dir build-multiplayer -R '^(discord_identity|frontend_online_identity|frontend_bounds|retail_frontend)$' --output-on-failure
```

The identity test exercises a real loopback callback with a simulated Discord
HTTP transport; the frontend test uses actual disc menu assets and the same
simulated transport. They prove gating and protocol/error handling, not a live
Discord login. On 2026-10-10 the approved application was configured in the
Developer Portal, and the rebuilt game completed live browser OAuth, token
exchange and `/users/@me` verification, advancing from screen 20 to screen 186.
Evidence is under `analysis/frontend/discord-identity-20261010/`: the saved
portal configuration, original screen tag names, menu captures and
`live-login.log`. Four focused CTest checks and the authentication suite under
address/undefined-behavior sanitizers passed on Linux. Windows and macOS remain
unvalidated.
The 2026-10-10 privacy/readability revision also passes all four focused checks:
the frontend test verifies that no menu heading, description or action exposes
the ID, name edits persist without altering Discord identity, cancellation keeps
the prior name, and title-logo geometry is absent from ONLINE panels and name
entry while remaining on the main menu. Updated native captures are in
`analysis/frontend/online-privacy-20261010/`.
ONLINE Quickmatch/Optimatch/Playlists now use the [authenticated queue API](../multiplayer/docs/matchmaking.md).
The backend verifies the existing Discord authorization independently and maps
it to a private player key. Gameplay admission, friends, invitations and the
Internet game transport remain unfinished. Future game servers must verify
admission independently. System Link retains its native LAN protocol.
