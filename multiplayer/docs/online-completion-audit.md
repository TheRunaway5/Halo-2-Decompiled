# ONLINE completion audit

Goal requested11 October2026: finish the full ONLINE feature set in this thread.
This audit preserves that scope. A passing two-player Slayer lab is not proof
that the original multiplayer ecosystem has been restored.

| Requirement | Current evidence | Remaining requirement |
|---|---|---|
| ONLINE label, private IDs, chosen screen names | Native frontend/account/queue code; native HTTP privacy checks | Preserve across added party/results features |
| Discord identity | Shared PKCE client; independent backend application/scope/subject verification; prior production live probe recorded in `.local/vultr/deployment.json` | Repeatable checks use external Discord fixtures; no fabricated live user login |
| Discord Friends | Official Social SDK1.10.19337; prior live same-account Friends connection and native separate-consent/account-binding/sign-out checks. The current Friends grant requests communication access separately from ONLINE identity, as required by the official game-invite API. | Existing presence-only users must reconnect Friends. Two real friends must verify SDK invitation delivery/acceptance and live block changes. |
| Discord activity handoff | Production release20261012T154813Z passes154 local/container backend checks,51 running file hashes, retained-state and backup-restore verification. Native issuance/publication/explicit-send, private received-invitation inbox and account redemption are connected. Independent party leases preserve multiple authorized recipients across admission and later send failures without extending expiry. Official SDK local launch registration succeeds. SDK/non-SDK component suites each pass seven checks; final full native identity/account/menu/service suite passes ten. See [native integration](native-online-integration.md). | Real Discord send/receive/acceptance and cold-launch validation remain open; fixture callbacks and local registration do not establish delivery. |
| Quickmatch, Optimatch, Playlists | `frontend_matchmaking_http`; original menu geometry and Vista assets | Historical catalog restoration |
| Casual and ranked selection | Private raw ranks0–127 and integer unadjusted averages; exact original candidate core with264 banks,27 admission cases,124 selections and100 thresholds; v8 public gathering profile now connects actual session age, title language and measured native relay routes to original role/query/countdown progression and150-ID rejection lifetime | Historical downloaded configuration, Live query ordering/pagination, direct-path/NAT/host-bandwidth semantics and full historical catalog remain unresolved |
| Active native gathering bridge | Release20261012T022410Z enables all six validation queues. Original signed role jitter/ticks, response-driven widening/result budget, final casual unbounded retry, peak-count clocks and all seven launch countdown tables are integrated; founding gameplay host is retained.142 frozen backend tests pass locally and in VPS Docker; independent four-client ranked/casual and6/14/16-client clan gameplay/voice/results/PCR/requeue pass; voice blocks and settings mismatch guards pass | Native service ordering, relay capacity/probes, team balancing, game pools, immutable-party withdrawal and atomic reservation release remain explicit replacements. Release20261012T032408Z adds opt-in private gathering count/roster progress while retaining the legacy response. The installed native painter selects actual Vista searching/waiting/joining panes;48 original viewport controls,768 ring clock controls and48 GPU captures pass. Actual PC bitmap decode matches71 face/mip hashes, and all1,622 class2 input vertices match original position/UV constant expansion. Exact Vista3D material binding remains open; launcher activation and the nine-size matrix are recorded in [native integration](native-online-integration.md). See [progress validation](native-matchmaking-progress.md). See [bridge validation](native-gathering-bridge.md) |
| Actual online games | Independent native processes and accepted parties complete gameplay/voice/results/PCR/requeue. This release repeats four-client ranked/casual, Minor3v3, Major6v8 and Major8v8 cases against the real copied backend and verifies retained founding host at admission | Public two-player Slayer, four-player Team Slayer and Minor/Major clan queues are validation pools. Complete historical rotations and gameplay modes remain unfinished; real-user matchmaking/WAN validation remains open |
| Identical game settings | Full selected source variant added to loading admission; two-native-process mismatch test | Preserve for new modes/settings |
| Completed results | Full original Vista consumed bank; every client validates final snapshot then confirms the same digest; transactional immutable private ledger | This verifies agreement, not independent server simulation or anti-cheat |
| Ranked and hidden casual progression | Versioned retail launch XP tables, pairwise averaging, demotion hysteresis and private per-playlist persistence; release20261011T230633Z enables recovered hidden casual updates with outward level null; 30 rank controls plus12 independently rerun original casual controls and97 backend tests passed; real HTTP/WS casual win changes next-search skill exactly once and preserves it through visibility changes | Later downloaded policies, original PCR rank-update owner, quit penalties and independent arbitration remain unresolved |
| Party membership and invites | Deployed authenticated service persists accepted parties, invitation lifecycle, leader transfer, clan batch invites and explicit member playlist consent; four independent native clients completed actual team gameplay/voice/results/PCR and group requeue; two real Discord accounts accepted a username-addressed party invitation on 11 October and the user confirmed both chosen names in the roster | Full16-member lobby, scrolled roster, leader/follower and invitation screens passed Vulkan capture and bounds/hit checks at nine sizes; live SDK-selected friend invitations and physical WAN friends still require validation |
| Clans | Authenticated lifecycle/roles/messages/private handles and presence retained; release20261012T001914Z adds recovered Minor/Major lineup and clan-owned XP rules.74 unchanged retail controls,97 Docker backend tests, native6/14/16-client gameplay/voice/results/PCR/requeue checks passed. No personal XP is awarded by clan matches; immutable clan audit survives dissolution. The full16-client regression passed again with v7 release20261012T003658Z | Real-user clan and physical WAN validation; original clan rank artwork/update owner and later downloaded policies remain unresolved |
| ONLINE couch players | Up to four native local profiles/controllers; two four-player PCs complete an eight-player match, local Vista PCR and requeue. Server-owned guest admission excludes guest personal result/XP rows, while independently verified local accounts receive stats. Four-view GPU and nine-size Vista UI checks are recorded in [couch integration](online-couch-players.md). | Physical four-controller hardware, real secondary Discord accounts and WAN couch validation; separate per-controller headsets. Guest PCR transits the relay in memory for native agreement. |
| In-game voice | Source audience/radio/death-grace/mute rules; real Opus across independent native clients up to16. Validated per-speaker/destination voice budgets include preferences; fragmented game snapshots have bounded per-destination budgets. Authenticated game/audio/preferences floods still close their room. v7 adds a separate private Discord block mask; server routing and native local playback enforce bilateral silence while preserving host transit to other listeners | Physical WAN players/headsets and impaired-link quality; controller/audio platform owners |
| Production party/clan voice proxy | Exact disconnect reproduced twice through actual Caddy; response-header timeout replacement passed both native paths through 32-second idle, Opus, mute and revocation checks, plus a 65-second party regression; reloaded at 22:28 UTC without backend restart; user confirmed sustained bidirectional voice after rejoining and waiting at least 30 seconds | Physical WAN/headset quality |
| Postgame voice | Original passive session audience retained after gameplay; persistent codec/session owner and F6/F7/M controls; native6/14/16-client Opus, mute, talking-state and capture-shutdown checks passed | Physical WAN/headset quality remains open |
| Vista UI and widescreen clans | ONLINE/clan stacks independently select imported Vista font/bitmap bytes even when the surrounding shell selects MCC Classic. ONLINE postgame owners retain this selection through fresh results stacks. Nine viewport sizes cover4:3,16:9,16:10,ultrawide,32:9 and fractional scaling; full100-member clan and16-member party checks are in [native integration](native-online-integration.md). | Exact Vista progress-ring material and PCR emblem shader rendering remain unresolved. Real-user visual confirmation remains useful. |
| Postgame and clean requeue | Source game-over, original Vista PCR16, Done, retained memory-only Discord login; two real native games and clean disconnect checks; four-client accepted parties completed native group requeue | Physical WAN players remain untested; preserve continuation across later queue/mode extensions |
| Original host departure | Active departures currently return remaining clients to ONLINE; recovered watcher/world-promotion and election owners establish the missing original simulation authority handoff | Implement full authoritative state restoration, host migration, surviving-gameplay policy for nonhost quits and result arbitration. Automatic matchmaking rejoin/backfill policy remains unproven |
| Original nonhost player retention | Recovered `84F30/152DF0/157DE0` source transition keeps allocated identity/profile/stats, sets left flags/tick and decrements active census once.18 unchanged original controls compare complete player, census and retained-result bytes; source FFA/team standings integration, Release and sanitizers pass. Adjacent1224 retained-result and9606 profile controls pass | Native binding remains unfinished: recovered default-damage event84/report800 suppression requires its complete unit/control/event/world owners; ordinary native damage/death helpers cannot substitute it.15 further original controls include four complete calls and eleven explicit partial observations. Actual platform identity, surviving transport and quit result confirmation remain open. See [unit-damage follow-up](halo2-departure-unit-damage-evidence.md). The source kernel does not activate match continuation. See [departure evidence](halo2-nonhost-departure-evidence.md) |
| Deployment and launchers | Couch release20261013T171821Z passes160 local/VPS container tests and70 running source hashes, with retained state, SQLite integrity and a private restore-tested backup. Both installed launchers select `.local/online-couch-20261013/delivery/halo2_native`, original Vista UI/HUD/font/loading banks and the retained TU1.5 gameplay executable. Two four-player native clients validate Quickmatch, mixed-identity results/PCR/requeue and four-view rendering; the Vista UI audit covers nine window sizes. See [couch integration](online-couch-players.md). Earlier activity and invite verification remains documented in [native integration](native-online-integration.md). | Real Discord invitation delivery/acceptance and cold launch, exact Vista ring/PCR emblem shaders, physical four-controller/WAN/headset checks and the broader original multiplayer requirements above remain open. |

See [match launch](match-launch.md), [matchmaking evidence](halo2-matchmaking-evidence.md),
[postgame evidence](online-postgame-evidence.md), [clans](clans.md), [voice](voice.md),
[candidate scoring](halo2-candidate-scoring-evidence.md), and
[host migration](halo2-host-migration-evidence.md).
Latest couch activation and verification are recorded in
`.local/online-couch-20261013/verification.json` and `remote-verified.json`.
Earlier activity activation and verification are recorded in
`.local/discord-party-continuity-20261012/verification.json`, `active-remote.json`
and `public-verification-final.json`. The previous progress checkpoint remains
under `.local/matchmaking-progress-20261012`; earlier bridge verification is in
`.local/gathering-lifecycle-20261012/verification.json`. Source-owner details
are in [party averages](halo2-party-average-evidence.md) and
[bandwidth inputs](halo2-bandwidth-input-evidence.md),
[lifecycle owners](halo2-gathering-session-evidence.md) and
[lifecycle follow-up](halo2-gathering-lifecycle-followup.md).
The broader goal remains incomplete while required behavior or validation above is missing.
Prepared isolated manual clients and the live check procedure are documented in
[the two-account check](two-account-live-check.md). Both prepared 1280x720 native
sign-in smoke checks passed with Vista assets and no normal-profile change;
the user operates login, invitations, queues and microphone controls.

## 13 October follow-up: selected2010 target and carnage history

The user selected final Xbox Live2010 as the default target and requested an
additional unranked Action Sack with party games including Fiesta. Preserve
this choice in [online-default-target.json](online-default-target.json).
[Historical research](online-historical-config-followup-20261013.md) recovers
exact TU1.5 rank defaults and earlier dated catalogs; an actual final 2010 TMS
download remains unavailable. [Fiesta research](action-sack-fiesta-source-followup-20261013.md)
recovers exact weapon selection and Random Set semantics, with explicit native
weapon/objective gates. Do not present the current Slayer validation pools as
a complete 2010 restoration or activate a Fiesta queue before those owners work.

[Persistent carnage history](carnage-history.md) adds normalized full source
details, guest exclusion, stable public aliases, exact verified Discord username
lookup, pagination and read-only report endpoints. Summary-only old records stay
explicitly summary-only. Player and clan progression now support the independently
executed TU1.5 defaults with their pinned table digest while retaining the launch
policy and refusing implicit skill migration. Host promotion, active departure,
secure community-server admission, objective coverage and clan badge publication
remain unfinished; no host-field or unsupported-mode substitution establishes
those requirements.

The carnage/history and policy-support backend is deployed as `20261013T185856Z`.
All 75 running source hashes, 168 local/VPS tests, isolated backup restoration
and two native four-player clients completing PCR/Done/requeue pass. Native
history persists four authenticated players and excludes four guests. Existing
production validation queues retain their prior configuration; this release
does not claim that Action Sack or a complete 2010 rotation is active. See
[release details](carnage-history.md) and
[verification](../../.local/online-completion-20261013/verification.json).

## Custom Slayer release: Fiesta and Infection

Release `20261013T200402Z` activates a ninth, unranked Action Sack queue for native
v10. It selects newly authored Fiesta Slayer and Infection Slayer on
Lockout/Foundation. Old clients retain all existing queues and cannot enter the
new rulesets. The complete original final 2010 TMS rotation remains unrecovered;
this explicitly custom party rotation does not replace that requirement.

[Custom Slayer rules and delivery](../../docs/CUSTOM_SLAYER_VARIANTS.md) covers
the repo definitions in `data/tags/variants`, release ZIP, per-spawn original
random-weapon sampling, automated Infection conversion/green appearance and
source-qualified PC secondary-weapon publication. Two native four-player
clients complete each mode through relay gameplay, Vista PCR, Done/requeue and
normalized website history with four authenticated players persisted and four
guests excluded. 172 backend tests pass locally and in the VPS image; all 77
running source hashes and isolated backup restoration pass. Host migration,
community admission, historical objective coverage, competitive gaps and full
weapon/effect parity retain their separate unfinished scope.
