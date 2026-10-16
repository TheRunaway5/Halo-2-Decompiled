# Steam identity and master hostname migration, 2026-10-16

The latest user directive makes Steam the primary ONLINE login and changes the
game service origin to `https://master.halo2forever.net`. The website apex stays
on GitHub Pages. This supersedes the earlier required-Discord-login decision;
the remaining full ONLINE goal is unchanged.

## Implemented

- Steam OpenID is verified by a fixed HTTPS request to Valve. The server checks
  the signed identity, exact destination, nonce lifetime/replay, and callback
  state. Native clients receive a one-use S256-bound handoff and an opaque
  eight-hour credential. Only its hash is stored; game preferences never store
  credentials. No Steamworks app ID is assumed or borrowed.
- The existing website repository `STEAM_KEY` was encrypted inside GitHub
  Actions for the master's ephemeral public key. Only ciphertext was downloaded.
  The private decryption key stayed on the existing server and was removed after
  import. The temporary branch and encrypted artifact were removed. The game and
  website receive no API key. Production mounts `/run/secrets/steam-key` read-only.
- Verified Steam personas are separate from chosen Halo screen names. Account
  IDs stay out of UI text. The native account/couch screens select Steam labels;
  guests still produce no persistent personal statistics. Existing Discord
  login remains available internally for social consent and legacy clients.
- Private identity aliases preserve stable player IDs. Linking requires both
  a Steam credential and an independently verified Discord credential. A new
  Steam profile can adopt an existing Discord clan/history owner. Two accounts
  with separate persisted data are rejected instead of silently merged. Linking
  invalidates replaced social sessions and rejects duplicate couch identities.
- Discord Social SDK consent remains optional. The native flow links the proved
  Discord identity before enabling its friends/invites. Steam users without a
  Discord link are not inadvertently muted by Discord-only voice lookup rules.
- GoDaddy's `master` A record points to the existing Vultr IP. Caddy serves valid
  HTTPS on both hostnames. Directory URLs and STUN discovery use the new hostname;
  the old hostname remains available for existing clients. Gameplay and match
  voice transport were not redirected through the master.
- The account callback redirects to a fixed confirmation URL with no OpenID
  assertion or private ID in the visible URL. That page grants no authorization.

## Verified evidence

Evidence lives in `.local/steam-migration-20261016/`:

- `backend-final-regression.log`: all 220 master tests passed, including seven
  Steam cases, linked clan retention, identity collisions, persistent-data
  preservation, guest/couch handling, and voice permissions.
- `live-login.log`: the real user completed Steam authorization. The native
  client verified the proof-bound handoff, connected to the real ONLINE account
  service over verified HTTPS, read the clan snapshot, and signed out. This
  proves real Steam authorization, not merely a mocked profile response.
- `native-wire.log`: the native production HTTP clients also passed a complete
  loopback account/sign-out flow with only the upstream Steam provider replaced
  by an independent signed fixture.
- Native identity tests passed security-value freshness, S256 proof, fixed
  endpoints, cancellation, rejected responses, sign-out, and dual-proof linking.
  The selected identity implementation/tests also passed address and undefined-
  behavior sanitizers with leak detection. This is not whole-program or race
  sanitizer evidence. The existing Discord PKCE regression passed too.
- `native/frontend-vista-regression.log`: account rendering passed at nine aspect
  ratios with the selected Vista font/bitmap bank. Existing checks covered 25
  ONLINE panel/context pairs. The Steam screen retained the chosen Halo name,
  hid IDs, and removed the title logo. Captures use an isolated Xvfb surface on
  NVIDIA Vulkan; the optional validation layer was unavailable. These are not
  physical-monitor or headset acceptance.
- `restored-state-verification.json`: the pre-migration production database was
  backed up online and restored into an isolated candidate. Integrity and
  foreign-key checks passed; both existing players were retained. The candidate
  admitted Steam setup before live activation. Live health and website bridge
  checks passed after activation. Backups remain under
  `/opt/halo2/backups/steam-20261016` on the server.
- The website source/default workflow now uses the new public API origin, and
  its static production build passed. Unrelated unpublished site work was not
  committed or published.

## Outstanding acceptance

Chrome returns `ERR_BLOCKED_BY_CLIENT` for every path on the new hostname,
including health and the clean confirmation URL. The initial signed callback
nevertheless reached the server and the real native login passed. The blocking
Chrome component is not identified. The browser tool's security policy rejects
`chrome://extensions/`, so no protection was disabled or bypassed. A local user
check is required to finish the visible browser-return verification. The user
reports AdBlock disabled; a controlled-tab reload still blocks. A comparison
with a normal Chrome tab opened by the user is pending.

The native candidate is linked against a complete matched, frozen source graph
with fresh identity/UI/service objects and the official Discord Social SDK.
It is not a full build of the concurrently changing campaign/rendering checkout.
The installed launchers still select `build-multiplayer/halo2_native`; they were
not redirected to an older engine snapshot. Delivery of a current complete
native runtime and live Steam-linked Discord friend/invite checks remain open.
Steam-only public profile search on the future website also needs its own design:
Steam persona names are mutable and nonunique, and must never become account keys.
The full host-migration, gameplay, voice, and WAN acceptance goal is not complete.

Valve reference: <https://partner.steamgames.com/doc/features/auth>.
