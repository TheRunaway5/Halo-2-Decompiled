# Native ONLINE integration

The 12 October increment connects the existing native Friends invitation action
to the official Discord Social SDK, keeps the received invitation inbox in the
Vista selected-player flow, and isolates ONLINE fonts and bitmaps from the
surrounding shell's selected asset pack. The reproducible Release inputs and
logs are in `.local/discord-party-continuity-20261012`.

## Invitations and consent

ONLINE identity remains an independent `identify` grant. Enabling Discord
Friends separately requests `identify openid sdk.social_layer`, following
Discord's [game-invite guide](https://docs.discord.com/developers/discord-social-sdk/development-guides/managing-game-invites)
and [communication scopes](https://docs.discord.com/developers/discord-social-sdk/core-concepts/oauth2-scopes).
An existing presence-only Friends grant must be reauthorized by the player;
the client rejects a narrower grant instead of presenting it as invite access.
The SDK's default communication scopes were checked against the bundled
official version 1.10.19337.

The account worker retains the current private party capability independently
of the latest recipient's operation receipt. Explicit invitations to another
friend, rejected recipients, SDK publication failures, and SDK send failures
therefore preserve an earlier valid invitation. Membership admissions refresh
the advertised size and version without extending its original expiry. No
refresh, retry callback, or background poll sends another invitation. Structural
party changes, full capacity, expiry, account changes and sign-out invalidate
the capability. Recipient authorization remains checked by the service.

Red/green regressions reproduce the original second-invite continuity and
missing communication-scope failures. Seven Release component checks pass with
the official SDK and seven pass with SDK support disabled. The local HTTP check
uses independent native clients and the actual account/service implementation;
Discord identity and SDK endpoints in these repeatable checks are fixtures.
Calling the official SDK's local game-launch registration API succeeds for the
stable title launcher. This does not prove Discord delivery or a cold launch.

## Vista asset ownership

ONLINE and clan stacks create a separate painter from the imported Vista
`pack.tsv` and `maps/fonts`. This includes Friends, received game invitations,
parties, clans, Quickmatch, Optimatch, playlists and matchmaking progress.
ONLINE postgame owners explicitly carry their session provenance through the
fresh results stack. Missing required Vista assets fail explicitly. The
surrounding shell and HUD retain their selected registries.

The controls retain the authored 640 by 480 geometry and use the existing
physical canvas transform for widescreen. Clan roster panels use their full
authored surface; scrolled rosters and modal hit regions are checked separately.
Tests compare submitted font and bitmap bytes with the imported Vista bank even
when MCC Classic is selected for the surrounding shell. The nine viewport
matrix covers 4:3, 16:9, 16:10, ultrawide, 32:9 and nonintegral scaling:
640x480, 1280x720, 1280x800, 1920x1080, 2560x1080, 3440x1440, 3840x1080,
2560x1437 and 1001x601.

## Service and validation boundaries

Production release `20261012T154813Z` implements the shared invitation lease and
conditional version/size refresh. Its 154 backend checks pass locally and in
the VPS Python 3.13 container. Independent verification compares 51 running
file hashes, checks SQLite integrity and restores the private rollback backup.
Accepted parties, retained state, six queue definitions and Caddy voice
timeouts are preserved. See [party activity](discord-party-activity.md).

Both root launchers select the tested Release runtime and report the imported
Vista ONLINE/clan bank. The existing TU1.5 gameplay XBE and selected shell/HUD
registries are retained. Both isolated 1280x720 sign-in smoke checks pass with
the bundled SDK, without changing the normal saved profile. The helper's
optional Vulkan validation mode is now explicit; this host lacks that layer.

The final native suite passes all fifteen ONLINE integration checks, including
four-client ranked and casual accepted parties and a sixteen-client clan game,
gameplay/postgame Opus, result agreement, Vista PCR navigation and requeue.
Two separate PCR isolation checks retain the ordinary Vista and MCC shell
behavior. The successful final GPU matrix contains 810 captures:

| Flow | Captures |
|---|---:|
| Discord game invitation states | 54 |
| ONLINE, Friends, browser, playlists and matchmaking | 324 |
| Gathering search and waiting | 36 |
| Clans, including full and scrolled roster | 153 |
| Parties, including full lobby and scrolled roster | 135 |
| Original Vista PCR16/185, twelve panes | 108 |

All nine sizes pass bitmap/font provenance, text bounds and applicable mouse
projection checks. Visual review includes full clan and party rosters, modal
isolation, long names and postgame panes. Ring phase pixel differences remain
inside the original physical model viewport. Captures explicitly disable the
unavailable Vulkan validation layer. Earlier failed/incomplete attempts and
their resolved causes remain in the capsule; they are excluded from this table.

Final build, capture, match-launch and installed-runtime evidence is recorded
in the capsule's `verification.json`. The prepared two-client procedure is in
[the live check](two-account-live-check.md). Real users must complete separate
Friends consent, explicitly send and accept a Discord game invitation, and
verify cold launch. Preparation never sends a message or opens a microphone.

The animated progress ring still uses the existing Xbox model/material while
the exact original Vista shader binding remains unresolved; see
[the progress evidence](native-matchmaking-progress.md). Original Vista PCR
emblem shader rendering is also unresolved and remains an explicit rendering
gap. The Vista font/bitmap, bounds and native transport checks do not establish
those shader paths, Windows rendering parity, original downloaded playlist
policy, host migration, or physical WAN/headset quality.
