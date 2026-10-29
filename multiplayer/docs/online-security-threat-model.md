# S01: ONLINE threat model and source inventory

Scope update, 2026-10-22: this is the historical pre-EOS baseline. Concurrent
EOS SDK, session, callback and Rust reassembly work requires a fresh ingress
inventory; S01 is reopened in the authoritative goal. Earlier v12/DTLS source
defaults below do not establish the combined worktree's current transport or
direct/relay policy. The [archive qualification](online-security-authority-archive.md)
accepts neither SDK callback safety nor live transport behavior.

Source inspection, 2026-10-21. This is the implementation baseline for the
[security-first goal](online-security-first-goal.md), not an exploit finding,
penetration test or certification that a path is safe. The source manifest in
`.local/online-security-s01-20261021/source-pins.json` pins inspected owners;
the companion verification receipt records source drift and coverage checks.
Installed binaries, live deployment settings and remote dependency versions
were not audited here. Source defaults below are not deployment observations.

## Assets, attackers and trust

Protect the player's process, files and credentials; native world ownership;
account/seat identity; voice privacy and mute/block decisions; fair authority
transitions; availability for healthy peers; and guest-filtered result history.
Public Halo names, stable public identities and friend tags remain discoverable
across blocks. Blocking prevents communication and private presence disclosure;
denied friend/clan/party actions can make a block inferable. The fact of a block
is not promised confidential. Public friend search itself is block-independent.

An ordinary player process runs with the user's privileges. A memory-corruption
bug in that process could therefore affect more than the current match.

| Attacker | Capability to assume | Required defense and remaining limit |
| --- | --- | --- |
| Unauthenticated internet/LAN sender | Scan listening sockets, send malformed traffic, spoof connectionless source addresses and flood handshakes | Authenticate before application admission, bound pre-authentication work and keep direct UDP alternatives explicit. Rust parsing cannot absorb an unlimited link-level flood. |
| Authenticated malicious player | Modify the client, send arbitrary application payloads through a valid encrypted connection, lie about measured quality and repeat requests | Validate schema, role, seat, generation and resource usage independently of transport authentication. Login is not a gameplay permission check. |
| Malicious player host | Send invented but well-formed state, corrupt archives, inconsistent admissions/results or deliberately withhold packets | Authenticate the source, validate local native invariants, require protocol evidence where applicable and contain import/replay. Host authority does not grant access to arbitrary memory/code. A player-host model does not guarantee an honest simulation. |
| Colluding voters | Coordinate lies or send conflicting observations/votes from their own admitted identities | Enforce exact electorate/generation and distinguish crash/partition assumptions from Byzantine safety. A strict majority alone does not prove tolerance of malicious equivocation. Do not claim such tolerance without a protocol argument and tests. |
| On-path observer or signaling attacker | Observe addresses, tamper/replay traffic, or substitute connection metadata if setup is compromised | Retain DTLS/TLS verification and bind authenticated setup to the correct physical peer. Direct P2P reveals connection addresses to peers; encryption does not hide them. |
| Compromised master/account/SDK dependency | Supply malicious control, listings, identity/social metadata or dependency code | Treat parsed control as untrusted data, restrict credentials/URLs and audit dependencies. Master compromise remains a distinct trust assumption; the game must not turn a control field into executable code. |

Memory safety, message authenticity, authorization, gameplay cheating,
availability and IP privacy are different properties. The work below addresses
each explicitly; no language choice proves all of them. Preserve MCC-only
assets, Steam-primary identity, optional Discord social features and direct
player-hosted gameplay/match voice throughout.

## Actual transport selection and trust transitions

1. [Matchmaking options](../../include/halo2/online_matchmaking.hpp) default to
   `halo2-native-queue-v12`. [Transport construction](../../src/online_matchmaking.cpp)
   accepts a `direct-udp-v1` descriptor, validates local seat/physical-peer
   associations and creates `OnlineMatchSession` with direct UDP enabled.
2. [Session options](../../include/halo2/online_match_session.hpp) leave
   `peer_host_migration` and `snapshot_candidates` false. They are internal
   qualification gates, not normal matchmaking defaults. Preserve this distinction
   while securing their existing test/opt-in receive paths.
3. [The session](../../src/online_match_session.cpp) uses `/signal` for direct
   rooms and `/relay` for older compatibility choices. The backend
   [room transport](../../multiplayer/master_server/match_transport.py) derives
   direct mode from `NATIVE_DIRECT`, authenticates its websocket, rewrites the
   signaling source from that authenticated owner and rejects binary gameplay/
   voice relay for v12. Legacy relay remains a separate explicit path.
4. [OnlinePeerTransport](../../src/online_peer_transport.cpp) creates negotiated,
   unordered, zero-retransmission data channels. It accepts only direct UDP
   candidate pairs, rejects relayed candidates and retains DTLS fingerprint
   verification. Peer IDs delivered to the application come from the retained
   per-peer channel callback, not from a packet's claimed sender.
5. `OnlineMatchSession::run_peers` passes that physical sender into `datagram`.
   The latter dispatches native packets, voice and the distinct authority,
   admissions, quality, host-agreement, common-cut and snapshot-candidate formats.
   Format-specific receive methods must continue to verify the trusted sender,
   current context, recipient and permitted operation.
6. [MultiplayerDatagrams](../../include/halo2/multiplayer_datagrams.hpp) bridges
   the matched transport into the existing `LanHost`/`LanClient` implementation.
   Its synthetic sockaddr values represent retained physical endpoint slots.
   They must never be confused with raw UDP source authentication.
7. [Runtime startup](../../src/multiplayer_runtime.cpp) rejects combining an
   ONLINE matched transport with raw `--host`/`--join`. Separate CLI/System Link
   constructors still open real UDP sockets; public reachability depends on
   the chosen bind/firewall and is not restricted by the name “LAN.”

## Entry-point and consumer inventory

The receive-funnel census searches actual socket, libcurl websocket/HTTP, RTC
and codec entry points under `src`, then follows their application dispatch.
SDK callbacks and backend endpoints are listed separately. This is a source
reachability inventory; it does not establish that every feature is enabled in
an installed release or on the deployed server.

| Route and caller | Current decoder/consumer | Existing admission and limits | Migration/disposition |
| --- | --- | --- | --- |
| HTTPS matchmaking and pre-game QoS | [online_matchmaking.cpp](../../src/online_matchmaking.cpp), [online_search_qos.cpp](../../src/online_search_qos.cpp) → catalog/roster/descriptor and challenge replies | HTTPS verification, restricted origin/no redirects, bearer setup; matchmaking body cap 65,536 bytes; QoS text cap 256 bytes and bounded challenge fields | Rust control parsing and trusted peer/seat context; retain C++ UI owners and current TLS library. Never let a descriptor select an arbitrary parser or relax authentication. |
| Match signaling/control websocket | [online_match_session.cpp](../../src/online_match_session.cpp) `run_control` → RTC descriptions, phase/generation/results | TLS/bearer, assembled frame cap 119,000 bytes, at most 256 read iterations per loop; backend room websocket cap 120,000 bytes and packet budget | Rust bounded control schema/state parsing. Preserve authenticated signaling provenance. Audit depth/duplicate-field handling per format; byte limits alone do not prove structural bounds. |
| ICE/STUN, DTLS and SCTP wire handling | [online_peer_transport.cpp](../../src/online_peer_transport.cpp) → libdatachannel/libjuice/crypto/usrsctp | At most 16 physical slots; up to four STUN URLs; signal description cap 16,384, candidate cap 1,024/pending cap 64; direct UDP-only selected pair | Retained native pre-Rust attack surface. Keep established crypto and audit/update/contain dependencies; a Rust callback wrapper does not replace these parsers. |
| Decrypted direct application datagrams | RTC per-peer callback → [session datagram dispatch](../../src/online_match_session.cpp) → native/control consumers | RTC message cap 1,404, receive queue 512; ordinary native payload cap 1,400; protocol, host-role and optional epoch checks | First Rust packet dispatcher and owning typed messages. Add whole-loop/per-peer work accounting; the direct receive drain itself currently runs until empty. |
| Raw UDP host/client and session query | [multiplayer_network.cpp](../../src/multiplayer_network.cpp) `Udp`, `query_session`, `LanHost::receive`, `LanClient::receive` → Controls/MultiplayerState → Match/replica | IPv4 nonblocking socket binds `INADDR_ANY`; native magic/protocol, packet cap 1,400, finite input values, ranges, endpoint/nonce/cookie and sequence checks | Same Rust wire decoder on every ingress. Add authenticated transport/admission before any internet community join; endpoint/cookie/nonce are not equivalent to DTLS. Explicitly gate legacy raw internet exposure. |
| Native snapshot fragments and reserved candidate packet archives | `LanClient::receive`, `candidate_packets` → state decoder → retained snapshot origin/proof | Snapshot cap 8 MiB; reserved packet archive cap 9 MiB; fragment shape/sequence/connection checks; unchanged complete packet group preflight | Rust fragment/archive decoding with owning immutable results and aggregate budgets. Preserve real proof and native validation; do not simply return bytes to the old general decoder. |
| Source admissions and player/profile mapping | [peer_source_admissions.cpp](../../src/peer_source_admissions.cpp) → AcceptedSourcePlayers → receiver/native authority | Format kind 8/version 1, max packet 522; exact context/target/physical sender, host-only offers, accepted roster, digest consistency | Rust admission parsing/state. Keep identity/seat ownership distinct; the digest is consistency evidence, not a sender credential. |
| Authority transfer | [peer_authority_transfer.cpp](../../src/peer_authority_transfer.cpp) → completed archive → SourceAuthorityReceiver | Kind 7/version 1; 1,024-byte chunks; exact context/target/member; host-only offers/chunks; default archive 64 MiB, send 512 KiB/s, burst 16 KiB, window 64, idle/lifetime limits; bounded delivered cuts | Rust transfer state/reassembly; retain byte completion versus actual world validation as separate events. Receiving/hashing/staging costs need one combined budget. |
| Full archives, deltas and host-continuation bundles | [archive decoder](../../src/source_authority_codec_decode.cpp), [schema](../../src/source_authority_archive.hpp), [delta](../../src/source_authority_delta.cpp), [host continuation](../../src/source_host_continuation_codec.cpp), [bundle](../../src/source_host_continuation_bundle.cpp) → coupled checkpoint | Versioned formats, size/count/type checks, zlib consumption and hashes, explicit owning schema rejecting process pointers; delta base/digest checks | High-priority Rust deep schema/decompression migration. Preserve every required field; hashes are corruption/consistency checks, not authentication or safe-native-state proof. |
| Replay batches and nested execution phases | [source_replay_batch.cpp](../../src/source_replay_batch.cpp) and [source_execution_codec.cpp](../../src/source_execution_codec.cpp) → receiver replay/execution | Exact expected binding/base/sequence/tick, per-phase and batch limits, actual execution-input owners | Rust nested decoding/work accounting; keep source executor semantics and local-owner gates. Validate cancellation and sustained replay CPU independently of encoded size. |
| Native import/publication | [source_authority_receiver.cpp](../../src/source_authority_receiver.cpp) → [SourceMatchAuthority](../../src/source_match_authority.cpp) → [PhysicsLiveWorldTu15](../../src/physics_live_world_tu15.cpp) | Admitted host/context, worker generation, cut tick/roster/player datum, local service roles, map/BSP/tag bindings, immutable image/topology, executor/TLS/input and allowed exit callbacks | Preserve the actual C++ prepare/commit gates after Rust decoding. In-process `jthread` is not containment. S10 must implement and qualify an import/replay containment design. |
| Host votes, common-cut agreement and telemetry | [peer_host_agreement.cpp](../../src/network/peer_host_agreement.cpp), [peer_common_cut.cpp](../../src/network/peer_common_cut.cpp), [peer_mesh_quality.cpp](../../src/network/peer_mesh_quality.cpp) | Distinct formats/current contexts; host agreement kind 10/version 2/max 134, suspicion after more than 500ms, fixed nonhost promise majority and unanimous locked-value decisions; common cut kind 11/fixed 116; quality magic H2QO/version 1/max 1,200 and bounded probes | Rust state machines and adversarial transition tests. Fixed-electorate availability, stale membership, equivocation and truthful telemetry remain separate review questions. Never invent native-ready votes from timeout. |
| Reserved snapshot-candidate delivery | [peer_snapshot_candidate.cpp](../../src/peer_snapshot_candidate.cpp) → foreground delivery/capture/retirement | Kind 12, 1,024-byte chunks, default archive cap 9 MiB per recipient, max 15 recipients; explicit Arm/capture/retire; internal opt-in | Rust protocol handling with exact foreground tokens. The host's potential 135 MiB archive storage is not a whole-session memory bound. |
| Match/postgame voice | [session voice framing](../../src/online_match_session.cpp) → [GameVoice](../../src/game_voice.cpp) → VoiceCodec/SDL | Voice kind 6; allowed sender/seat and host forwarding, 22-byte frame prefix/max 278, finite bounded gains, mute/block and delivery-generation checks, bounded queues; decoder packets max 256 and exact frame sample count | Rust frame/speaker/replay/queue handling. Test sequence wrap/reconnect and forwarded-original-speaker trust. Preserve separate physical microphone and generation fences. |
| Party/clan voice websocket | [online_account.cpp](../../src/online_account.cpp) authorization → [OnlineVoice](../../src/online_voice.cpp) → [clan_voice.py](../../multiplayer/master_server/clan_voice.py) | HTTPS-derived WSS/bearer, bounded frames/talkers/queues and server membership/budget; native Opus/SDL | Existing separate service-mediated voice route, not the v12 match channel. Inventory it explicitly and secure framing/dependencies; any change to direct topology must preserve the user's intended scope and cannot be silently substituted for match P2P. |
| Directory/browser and account/social HTTP | [online_directory.cpp](../../src/online_directory.cpp), [online_account.cpp](../../src/online_account.cpp), [discord_voice_blocks.cpp](../../src/discord_voice_blocks.cpp) → UI/party/clan/block state | Origin/TLS and bounded bodies; directory 4 MiB/depth 16/max 1,000 listings with duplicate-key rejection; identity-sensitive operations authenticated | Rust control/schema parsing and URL/text contracts; no peer-supplied command or file authority. [Browser UI](../../src/frontend_directory.cpp) currently exposes discovery/details and explicitly says joining unavailable. Secure admission must precede a future join action. |
| Steam identity and optional Discord linking/login | [steam_login.cpp](../../src/steam_login.cpp), [discord_login.cpp](../../src/discord_login.cpp), [steam_login.py](../../multiplayer/master_server/steam_login.py) → private account grants | Steam native device/verifier handoff with backend OpenID verification; bounded response body. Legacy Discord callback binds loopback with state/PKCE and request deadline/size checks | Preserve primary Steam and optional linking. Harden/retire unnecessary legacy listeners through explicit feature reachability. No account bearer, API key or login subject becomes peer transport identity solely because a packet includes it. |
| Discord SDK/social invite callbacks | [discord_social.cpp](../../src/discord_social.cpp), [discord_activity_inbox.cpp](../../src/discord_activity_inbox.cpp), [discord_party_activity.cpp](../../src/discord_party_activity.cpp) → user-visible inbox/party request | Retained callback owners, bounded/eligible inbox handles and separate account/party admission | Retain vendor dependency review, message lifetime and text validation. Invites cannot directly choose arbitrary game commands, code, assets or file paths. |
| Native cold-start socket adapters | [native_tu15_network_service.cpp](../../src/native_tu15_network_service.cpp), [startup](../../src/native_tu15_network_startup.cpp), [sockets](../../src/native_tu15_network_sockets.cpp), [macOS datagram adapter](../../src/macos/native_datagram.hpp) | Actual UDP service, source callsite/ABI/creator checks and recvmsg truncation rejection; src census finds adapter definitions and test consumers, not windowed runtime construction | Reference/component path, not an alternate authenticated ONLINE implementation. Keep it out of production receive routing unless it passes the same admission/decoder gate. Recheck reachability when startup integration changes. |
| Backend results/history | [match transport](../../multiplayer/master_server/match_transport.py) → [match_results.py](../../multiplayer/master_server/match_results.py) → [carnage_history.py](../../multiplayer/master_server/carnage_history.py) | Authenticated room host submission, validation and required physical confirmations; persistent guest filtering, parameterized database operations | Backend remains Python. Preserve schema/identity/guest checks; Rust client validation cannot establish truthful competitive results from colluding clients. Audit result-retry/idempotency and names as data, not identity authority. |

The source census found twelve C++ HTTP/websocket/socket/RTC/voice-codec receive
owner files. All appear in the table. Local MCC media decoding in `mcc_audio.cpp`
is a separate local-asset consumer, not a peer voice receive path. Third-party
internal socket/codec implementations and the SDK are retained dependencies;
the census does not claim their internals were fully audited.

## Dependency ownership baseline

| Dependency | Inspected declaration and responsibility | Security migration requirement |
| --- | --- | --- |
| libdatachannel, libjuice and usrsctp | [OnlinePeers.cmake](../../cmake/OnlinePeers.cmake) pins libdatachannel commit `6b1e2e620f1e37f0eafeee702eaea0043cb305fd`, including its declared submodules; media/websocket/example/test features disabled; [local patch driver](../../tools/apply_online_peer_patches.py) runs before use | Preserve actual submodule/patch identities in release manifests. Rust application parsing leaves ICE/DTLS/SCTP processing native. |
| libcurl and TLS/crypto | [Root CMake](../../CMakeLists.txt) finds CURL at least 7.86 and OpenSSL Crypto, with separate [macOS OpenSSL configuration](../../cmake/macos/OpenSSL.cmake) | Inventory actual linked TLS backend, build versions and advisories per platform. Source minimums do not pin installed/system library versions. |
| zlib and OpenSSL SHA routines | Root CMake and authority codec/transfer owners | Decompression and checksums remain native until explicitly replaced or contained; do not confuse checksum validity with peer authority. |
| nlohmann JSON and Boost.PFR | Root CMake/control parsers and [authority schema](../../src/source_authority_archive.hpp) | Network JSON and reflective archive traversal remain C++ until their real consumers migrate. Preserve schema/version compatibility and structural limits. |
| libopus and SDL2 | `halo2_online_voice` links both; [macOS Opus configuration](../../cmake/macos/Opus.cmake) supplies its platform path | Audit the codec and device ownership independently of Rust voice framing. SDL device lifetime/fences stay local, not serialized world state. |
| Discord Social SDK | Optional root CMake include/library selection and packaging; callback ownership in `discord_social.cpp` | Pin the selected vendor binary/notices and audit callbacks. The SDK is a distinct native dependency, not made safe by a Rust network wrapper. |
| Python backend, aiohttp and SQLite | [Backend Dockerfile](../../multiplayer/master_server/Dockerfile), room/account/voice/result owners | Retain authenticated backend controls, body/rate limits, guest filtering and safe database operations. Rust client work does not certify the backend. |

This baseline records build declarations, not current vulnerability status or
the deployed software bill of materials. S02/S13 must verify resolved versions,
updates, packaging and hardening against the actual selected builds.

## Verified safeguards and unresolved hardening work

| ID | Source observation | Required disposition |
| --- | --- | --- |
| R01 | The standalone UDP socket binds all IPv4 interfaces. Its nickname and endpoint/nonce/cookie checks do not make it an authenticated internet transport. | S04/S06/S07: preserve LAN functionality while preventing community/internet join from bypassing authenticated admission. Measure public exposure rather than inferring it from a local bind. |
| R02 | ONLINE's trusted physical sender exists at the transport callback, but many lower-level interfaces accept an ordinary integer named `authenticated_peer`. | S03/S07: restrict construction of trusted sender/context values; test that packet fields, casts or alternate ingress cannot manufacture that authority. |
| R03 | Archive decoding has real bounds. The receiver sets an encoded cap of 64 MiB; default decoded and dynamic-allocation limits are each 1 GiB and explicitly separate from RSS. Deltas/bases/staging coexist. | S05/S11: select and enforce one measured lifetime-based budget across retained and in-flight owners. Do not replace these values with guessed smaller caps that reject genuine full worlds. |
| R04 | Archive hashes and admission digests are unkeyed consistency checks. Native preparation independently checks local image/topology and service/executor/callback ownership. | Preserve both transport authentication and native semantics. Successful Rust decode/checksum must never bypass prepare/commit or authorize executable bytes. |
| R05 | Host agreement wire v2 requires unanimous fixed-survivor decisions and locks honest votes across ballots. Pairwise votes still originate from potentially malicious voters. | The October 23 equivocation regression refuses split majority views. Safety depends on at least one honest survivor retaining its vote for the context; a missing or withholding survivor can prevent migration. Common-cut and telemetry assumptions remain separately scoped. |
| R06 | Direct receive queues are bounded but the session drains packets until empty; native import/replay and retained RTC/Opus/zlib execute in the player process. | S10/S11: bounded work scheduling plus implemented containment for residual unsafe consumers, with latency/CPU/memory evidence and failure isolation. |
| R07 | Legacy websocket relay and legacy login/native startup adapters still exist alongside the default direct/Steam paths. Browser joining is unfinished. | Inventory/retain reference evidence, explicitly gate reachable alternatives and prove packaged production callers cannot fall back around Rust/authentication. No silent default changes. |
| R08 | Match voice permits forwarding by the authoritative player host; transport authentication proves that forwarding host, not automatically the original speaker's honesty. | S06/S09: state which speaker authenticity property is required and bind original provenance accordingly. Preserve receiver-local mute/block rules. |
| R09 | Account display names and provider subjects are separate domains; guest rows are filtered in persistent history. | S07/S14: preserve physical-peer/seat/account separation, hidden IDs and guest filtering across the bridge and result paths. Never authorize by display name. |
| R10 | Current source versions and dependency pins are inspectable; deployed configuration, packaged linkage, fuzz coverage and platform containment are not proven by this inventory. | S02/S12/S13/S14 remain open until actual build, adversarial, independent review and release evidence is retained. |

These observations identify work and trust assumptions. They do not demonstrate
an exploitable vulnerability or prove any particular attacker can achieve RCE.

## Replacement order and retained ownership

1. S02/S03: create a reproducible Rust build and one small owned-message
   interface. Record normal compatibility vectors and source limits before
   switching any caller. Keep errors and callbacks outside publication holds.
2. S04/S07: replace the application packet readers/dispatcher used by both
   matched and raw ingress. Trusted sender/context comes from transport admission;
   C++ receives typed commands and retains legal-operation/object-lifetime checks.
3. S05/S10/S11: replace authority/candidate fragment handling and complete deep
   archive/delta/replay decoding. Carry owned data across the bridge; retain
   actual local C++ native preparation and qualify containment/budgets before
   importing an untrusted world. No decode-then-reparse legacy shortcut.
4. S06/S08/S09: finish cryptographic admission/replay policy for every alternate
   path, Rust vote/telemetry/common-cut transitions and voice framing. Preserve
   native readiness gates, DTLS and direct P2P; do not enable incomplete migration.
5. S12–S14: stateful/differential fuzzing, retained-native sanitizer coverage,
   malicious-peer integration, independent review and actual packaged callsite
   verification. Retire or explicitly disable old receive paths and unsafe
   fallback. Reapply the gate whenever downstream work adds a new path.

Cryptographic identity and required authorization checks must remain effective
through every intermediate migration step; their placement later in this list
is not permission to postpone them until after exposing a new parser.

## Source and validation boundary

The accompanying receipt records SHA-256/size for the inspected files, the
receive-funnel census and default/format selectors. Checkoff of S01 concerns
this source inventory and threat model only. S02–S14, actual Rust replacement,
security review, platform/runtime/WAN acceptance and all downstream ONLINE
features remain open. No scan of credentials, browser sessions or live users
was needed, and no runtime setting or deployment changed for this inventory.
