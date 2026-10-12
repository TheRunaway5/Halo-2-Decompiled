# Native gathering bridge validation

Production release20261012T022410Z enables the gathering profile in all six
validation queues. Native source defaults to v8. The installed launchers
currently select `build-armor-framebuffer-20261012/halo2_native`; disassembly
confirms its environment options construct the v8 compatibility string.
Older v7 clients retain account access but cannot enter these queues and see
these playlists as ineligible. Playlist revisions changed, requiring fresh
member consent. This restores evidenced selection/lifecycle rules without
claiming a complete historical playlist catalog or Xbox Live transport.

A playlist selects `candidate_selection: "retail-candidates-native-relay-v1"`
and requires an explicit `gathering` configuration. `{"retail_hopper":0..6}`
loads one of the seven recovered launch countdown/timeout tables and rejects
mismatching minimum/maximum dimensions. Alternatively, an explicitly sourced
native configuration supplies all sixteen countdown entries and timeout.
The two-player validation queues use the original Head to Head countdown;
Minor/Major use their respective launch clan countdowns. Four-player Team
Slayer dimensions and rotation remain explicitly custom.

A gathering has a private identity, founding party, current admitted whole
parties, fixed gameplay host, title-language index and stable birth wall-clock.
Its summary updates after each accepted join. Session age is seconds since
birth. A searching party cannot simultaneously own an advertised gathering.
Selection admits the whole party before team proposal and gameplay admission.
Cancellation currently destroys the native gathering; survivors can initialize
a fresh search. That immutable-party withdrawal rule is a native policy.

## Recovered behavior

Admission, scoring, sorting, readiness selection and thresholds execute through
`session_candidates.py`. Raw0–127 skill and integer unadjusted averages remain
private and separate from visible artwork. Advertisements use
`max(raw_minimum,F(raw_maximum))`; search uses the recovered raw-extrema envelope.
The complete launch table and all128 inverse results match original bytes.
See [rank-floor evidence](halo2-rank-floor-evidence.md).

`search_lifecycle.py` retains the original signed jitter, search/gather role
comparison, signed32-bit tick subtraction, completed-query radius progression,
150-result budget, ranked exhaustion and final unbounded casual retry. Pending
real probes prevent query completion. Elapsed five-second/30-second timers no
longer refresh a query or remove casual skill bounds. Completed failed targets
are ineligible and remembered in the original150-ID append-only rejection bank
for that initialized search. New initialization clears it; radius/same-radius
queries preserve it. Overflow does not evict existing IDs.

Gathering growth clocks reset only at a new historical peak. The recovered
countdown uses current count, launches immediately at capacity, and rejects
counts outside the configured dimensions. All seven launch hopper tables and
signed wrap controls are covered by original-byte fixtures. Timeout returns
the owner to role choice. Map/variant selection occurs at gathering creation
and is retained. The founding host stays the gameplay host, subject to measured
connectivity; another better normalized link does not replace it. See
[lifecycle owners](halo2-gathering-session-evidence.md) and
[423 follow-up controls](halo2-gathering-lifecycle-followup.md).

## Native transport and remaining boundaries

`/v1/matchmaking/qos` binds one WebSocket to the memory-only queue session. The
server sends different unpredictable challenges to two participants in
sequence, times that actual server→A→server→B→server cycle, and takes the median
of three samples in milliseconds. This is a measured native relay route.
Clients return only challenges; they cannot submit targets, latency, bandwidth,
rank or identity. Unknown original quality remains valid `(0,-1)`. v8 requires
an explicit title-language index; Discord locale and region do not supply it.

Queues, outstanding pairs, replies, deadlines, sample lifetimes and traffic are
bounded. Logout/rotation revokes sockets; disconnect invalidates measurements.
The native worker pins the origin, verifies TLS, rejects redirects/proxies and
malformed controls, and reconnects. Private identities and credentials never
enter menu snapshots, errors or logs.

The missing Live query service is explicitly replaced by synchronous current
compatible gatherings sorted by birth/private ID, capped at50 per query and
bounded by the native request budget. Original query ordering/pagination is
unknown. Playlist capacity replaces original host-bandwidth-derived capacity;
restricted-NAT policy stays disabled without an invented Xbox NAT type. Native
reservation release is atomic and does not fabricate a platform drain/join
acknowledgment. Relay probes/allocation deadlines, team balancing, game pools,
leases and final loading admission remain native policies. Full original host
migration, historical downloaded policies and other gameplay modes remain open.
The menu still reports searching while gathering/countdown work progresses.

## Reproducible validation

Artifacts in `.local/gathering-lifecycle-20261012`:

- `backend-snapshot`, `backend-source-manifest.json`, `staged-release.json`:
  frozen68-file deployment archive.142 local backend tests pass, as do all142
  tests in actual VPS Docker; its production catalog also loads in the image.
- `source-snapshot`: real copied backend files and the native HTTP fixture,
  paired with the pinned previously compiled native fixture binary. The older
  gathering-relay C++ snapshot has a live multiplayer symlink; pair historical
  reruns with its separate frozen backend rather than assuming that symlink
  preserves historical source.
- `native-ranked-party-final.log`, `native-casual-party-final.log`: independent
  four-client parties complete actual relay preflight, recovered query/countdown
  selection, retained-host admission, gameplay/Opus, saved results, PCR and requeue.
- `native-minor-clan-final.log`, `native-major-clan-final.log`,
  `native-full-clan-final.log`:6/14/16 independent clients exercise clan lineup,
  clan-owned results, retained-host admission, gameplay/voice/PCR and requeue.
- `native-blocks-final.log`, `native-mismatch-final.log`, `native-legacy-final.log`:
  Discord voice-block routing, settings-mismatch rejection and legacy v7 path.
- `deployment.log`, `remote-active-inspection.json`, `public-compatibility.json`:
  active source hashes, catalog, private SQLite backup integrity, retained
  accounts/party/master keys, public authorization gates, and active voice proxy
  configuration. `verification.json` records hashes and validation scope.

External Discord endpoints in automated checks are fixtures. These checks do
not establish real-user Discord actions, physical WAN/headset quality, full
current-launcher behavior or original Xbox transport equivalence. The user
separately confirmed sustained bidirectional party voice after the proxy fix.
