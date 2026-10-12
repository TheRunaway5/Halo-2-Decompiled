# Native ONLINE server discovery

After Discord sign-in, choose **ONLINE → Server Browser**. The browser fetches
`GET /v1/servers` in a background worker. Focus a server for map, game type,
player count and region; select it for name, address, version and other details.
**X** refreshes. The browser also refreshes every 30 seconds while open.
Selecting a server does not start a game: admission and Internet transport are
still unimplemented. Listed endpoints have unverified reachability, and no
version compatibility or available-slot reservation is implied.

The separate **Quickmatch / Optimatch / Playlists** routes now connect to the
[matchmaking coordinator](matchmaking.md) through its authenticated queue API.
Discovery does not enqueue players, choose playlists, match opponents, balance
teams or reserve server slots. Gameplay admission remains unfinished for both
routes. The matchmaking document records the original Halo2 fidelity evidence
and the explicit limits of the reconstruction.

Discord verification remains required to enter the native ONLINE screens.
Discovery requests themselves are anonymous: no Discord token, subject, screen
name or account data is sent to masters. Owner/master UUIDs are discarded after
response validation; server UUIDs are used internally to preserve selection.
Names remain the player's local choice. The retained Halo 2 logo is suppressed
on the browser and details panels, like the other ONLINE panels.

## Configure masters

The default public master is `https://master.halotoforever.com`. Override it by
setting `H2_MASTER_URLS` before starting the game or either desktop launcher.
An explicitly empty value disables discovery. It accepts up to eight comma/whitespace
separated HTTPS origins, in failover order. Origins may have an optional final
slash; paths, credentials, queries and fragments are rejected. For example,
replace these documentation-only hostnames with masters you operate/trust:

```sh
H2_MASTER_URLS='https://master-a.example,https://master-b.example' ./Halo2-Launcher
```

For the loopback Docker setup in [deployment.md](deployment.md):

```sh
H2_MASTER_URLS='http://127.0.0.1:18081,http://127.0.0.1:18082' \
H2_DIRECTORY_ALLOW_LOOPBACK_HTTP=1 ./Halo2-Launcher
```

The HTTP opt-in applies only to literal `127.0.0.1` and `::1`. HTTPS certificate
and hostname validation remain enabled. No master addresses are learned from
responses or federation. Redirects and ambient HTTP proxies are disabled.
Each endpoint has a two-second connection and three-second overall timeout;
failures try the next configured origin. A successful empty response stops
failover. JSON is capped at 4 MiB, 1000 listings and nesting depth 16, with
duplicate-key/ID, field, integer, text, player-count and lease checks.

Loading retains previously verified, unexpired directory metadata. A failed
refresh clears it, and expired leases disappear locally even during an outage.
Leaving the browser or signing out cancels requests and clears the cache. Late
replies cannot restore cancelled results. Missing configuration, no servers,
and failed lookup have separate messages and an explicit Refresh action.

## Original asset evidence and extension boundary

The supplied `assets/disc/maps/mainmenu.map` has SHA-256
`1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b`.
The imported original tags are:

| Screen | Tag suffix under `ui\screens\game_shell\xbox_live\matchmaking` |
|---|---|
| 219 | `optimatch_listing` |
| 237 | `optimatch_listing_lobby` |

Both have one pane, one 16-visible-row list with a single text cell, and three
description text fields. Their authored heading is **CHOOSE MATCHMAKING
PLAYLIST**. The directory deliberately repurposes those existing assets as a
native extension: it changes text and row data, retaining their bounds, skin,
font metrics and scroll behavior. The normal launcher supplies the imported
Vista texture pack and larger font glyphs selected by `assets/asset-sources.tsv`.
ONLINE uses that same pack; no separate low-resolution asset path is introduced.
As described in [font sampling](../../docs/FRONTEND_FONT_SAMPLING.md), the supplied
PC distribution's pristine Vista provenance is not established; this does not
change the fact that the loaded glyphs and bitmap pages have higher resolution.
It does not claim that original Halo 2 used this
master API or that these listings implement original playlist matchmaking.
The supplied service-menu strings are **Quickmatch**, **Optimatch** and
**Create Party**. These remain the native matchmaking routes. The independent
**Server Browser** entry uses a separate screen context with the same authored
listing assets. Quickmatch connects to the queue API described above.
Untrusted directory text bypasses the menu's player/profile token expansion.

## Verification

```sh
cmake --build build-multiplayer --target online_directory_tests frontend_online_tests halo2_native -j2
ctest --test-dir build-multiplayer \
  -R '^(online_directory_client|online_directory_http|discord_identity|frontend_online_identity|frontend_bounds|retail_frontend)$' \
  --output-on-failure
build-multiplayer/frontend_online_tests assets/disc/maps \
  --asset-sources assets/asset-sources.tsv \
  --capture .local/online-discovery/captures-vista
```

`online_directory_http` starts two actual Python masters with independent
keys/databases, signs a simulator registration, updates metadata, kills the
origin, discovers through the federated backup with the compiled C++ client,
checks expiry, restarts the origin, registers again and removes the listing.
It requires `multiplayer/master_server/requirements.txt`; otherwise it skips.
The menu test uses a controlled Discord verifier and directory fixture, the
actual loopback OAuth callback, and the launcher's registered Xbox layout plus
Vista HD assets. It asserts that ONLINE actually draws larger imported font
atlases and UI bitmap pages. It covers 20 rows,
details, selection-preserving refresh, expiry, failure/recovery, sign-out,
screen-name privacy and logo suppression. Its optional capture path renders
browser, scrolling, details, empty and failed states at 960×720 and 1280×720.
This validates discovery and presentation, not live Discord service operation,
public TLS deployment, Internet joining, matchmaking, clans or rankings.
