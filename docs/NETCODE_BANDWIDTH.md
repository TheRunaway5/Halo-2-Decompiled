# Bandwidth-bounded snapshot netcode (protocol 105)

Implementation date: 2026-10-24. Scope: the native `LanHost`/`LanClient`
snapshot transport (raw UDP and the ONLINE relay). The retail Xbox System Link
wire format is still not spoken; this replaces the port's own transport.

## Why

Before protocol 105 the host serialized the complete `MultiplayerState` for
every client on every tick and sent it raw, in up to 8 MiB of 1400-byte
fragments. The WORLD pose bank alone carries a 52-byte matrix for every node of
every body, so a 16-player snapshot was tens of kilobytes per tick, and every
datagram also carried 77 bytes of authentication. In the
`native_lan_snapshot_stream` fixture (eight moving players, no pose bank),
complete snapshots measure **2161 kbit/s per client**.

The retail game budgets far less. Its network configuration initializer
(`0x66330`, [defaults](../tools/reference/halo2-decompiled/src/unknown_0662e0.cpp))
starts a new channel at **30720 bit/s**, never drops below **4096**, and
probes to at most **71680** ([observer analysis](../tools/reference/halo2-decompiled/docs/unknown_078900.md)).
Its host-capacity table grows by roughly 8–17 kbit/s per added player
(`value40[16]` = 276480 bit/s for a 16-player host).

## Design

The snapshot schema stays with the native encoder. The wire, budget and
replication policy are Rust, in the unsafe-forbidden protocol crate
[`netcode`](../rust/online_protocol/src/netcode/mod.rs), reached through the
CXX bridge and the [`SnapshotStream*`](../include/halo2/snapshot_stream.hpp)
adapter.

| Piece | Behavior |
| --- | --- |
| [Acknowledged deltas](../rust/online_protocol/src/netcode/stream.rs) | Every input packet carries the newest decoded snapshot sequence, a 32-message mask and a reset flag. The host codes each tick against the newest acknowledged message (64-message window). Loss never stalls: the next message references a baseline the client is known to hold. |
| [Generic codec](../rust/online_protocol/src/netcode/generic.rs) | Lossless. Runs that match the aligned baseline and did not change in it cost one length. Other units use the baseline's change history as context; floats marked by `put_float` are ordered-integer residuals with constant-velocity prediction. Keyed anchors (list sections, entity handles, event serials) re-align after list insertions/removals without searching; a new entry is compared with the baseline copy of its preceding sibling (journal events, joins and spawns share a layout). |
| [Range coder](../rust/online_protocol/src/netcode/range.rs) | 16-bit adaptive probabilities, so an unchanged world costs about 5 bytes of payload. Models reset per message; no coder state crosses a datagram. |
| [Poses](../rust/online_protocol/src/netcode/pose.rs) | Presentation-only. Rigid nodes become smallest-three quaternions (1/4096) plus positions relative to the sphere center (1/1024 world unit); non-rigid nodes stay raw. Each body has a priority that grows with viewer relevance while it differs from what the client is known to hold. Every moving body sends a root update (sphere + node 0) every tick, outside the budget, like the original's per-tick object positions, so rigid objects such as a falling barrel move at full rate. The host fills the remaining budget with full refreshes, delta-coded against an acknowledged refresh of the same body or of another body with the same skeleton. The client keeps the latest refresh per body and carries held poses rigidly with newer roots. |
| [Budget](../rust/online_protocol/src/netcode/bandwidth.rs) | Token bucket per client using the retail constants: start 30720, floor 4096, ceiling 71680, +min(3072, 20%) per 2 s round only while demand exceeds the budget, ×0.8 when more than 10% of 32 messages are lost. Game state goes out **every tick** (the original sent player state every tick; skipping would delay the final postgame state and unpair migration cuts from client receipts). The budget governs pose refreshes; one overdue full refresh may overdraw by half the 0.25 s debt cap. |
| [Compact authentication](../rust/online_protocol/src/lan_auth.rs) | After a peer proves the shared key, raw-LAN frames switch from `LNA1` (public key + 32-byte HMAC, 77 bytes) to `LNA2` (pinned session, 128-bit truncated HMAC-SHA256, 29 bytes). Same sequence space and 1024-packet replay window. |

Stream datagrams use native kind 14 (`H2N2` header, nonce, cookie, then the
Rust fragment header); kinds 11–13 already carry ONLINE common-cut, snapshot
candidate and survivor-selection controls on the same channel, and the ONLINE
session admits kind 14 only from the current host.

Generic state reconstructed by the client is byte-identical to the host's.
The snapshot stamp digest (`H2SD1` SHA-256) covers that state and the pose
bank *structure*, not pose values, so host and client stamps still agree for
continuation and migration proofs. Reconstructed bodies go through the
unchanged native `decode_state` validation; a rejected or undecodable message
requests a self-contained reset (every message is self-contained until the
client acknowledges one after the reset).

Migration snapshot candidates (`broadcast_candidate`) keep complete-packet
archives, because a reserved delivery must not depend on a client baseline.
A body the stream cannot carry also falls back to complete packets.

## Non-host responsiveness

The original client owned its player
([reference](../tools/reference/halo2-decompiled/src)): simulation mode 4
(distributed client, `unknown_067e10.cpp:158`) keeps the local player's
unquantized action, simulates it at once and sends the host its absolute
facing plus its unit's position, forward, up and velocities
(`unknown_0699a0.cpp:382`, `unknown_082b70.cpp:127`). The host writes that
motion into the biped after a finite-values check (`unknown_067e10.cpp:2026`,
`unknown_0a0190.cpp:79`); damage stays host-only (`damage.cpp:645`) and
host-replicated projectiles wait for the host (`projectiles.cpp:2151`).
Corrections blend 45% (position) and 35% (orientation) per tick
(`unknown_0a58d0.cpp:383`).

The port's host runs TU1.5 in local mode and clients have no native world, so
a non-host's own look and movement came back a round trip late.

- **Look (implemented).** Every snapshot names, per local view, the raw look
  sample its processed angles consumed (`LanClient::consumed_look`). On foot,
  the client draws the host's processed yaw/pitch plus the raw look the host
  has not consumed yet, so aim answers on the same frame. Seated views keep
  host angles, because `0x18A300` applies seat limits and auto-level on the
  host. Predicted pitch is clamped to ±88°.
- **Movement (next).** Faithful parity needs a client-side TU1.5 world that
  steps the local biped each tick (the authority replay worker already builds
  one on clients), client motion in the input request, a host setter
  equivalent to `0xA9F70`/`0xAA260`, and the 45%/35% correction blend. A
  Quake-style rewind/replay is not the original model.
- **Firing (next).** Local fire presentation on press, deduplicated by the
  host `shot_tick`; then barrel `+0x34` local projectile creation once a
  client world exists.

## Online sandbox parity

Online matches run the same host simulation as offline ones: rules,
physics, AI, devices and vehicles have no online branches. What differed was
presentation, and one non-host gap that also affected LAN:

- **Non-host first person (fixed).** TU1.5 animates first person only for a
  machine's own local views, and clients have no native world, so a remote
  viewmodel never played melee, grenade throw, fire, reload or ready. The host
  now drives remote players' replicated views with the native first-person
  clock kernels (`first_person_activity`, `first_person_animation_clock`),
  raising the same activities the TU1.5 world raised: melee and grenade from
  its action state, fire from the weapon's prepared tick, and magazine actions
  from observed mode changes (`weapon_magazine_observed_action`, the native
  `weapon_magazine_transition` table). Snapshots publish every player's clock.
  `remote_first_person_tu15` checks melee and fire over actual UDP.
- **Third-person actions (fixed for the budget).** Remote bodies are the host's
  TU1.5 world nodes. Under the retail budget a body playing a melee or grenade
  throw (and its held weapons) is refreshed every tick, never held.
- **Online host presentation (fixed).** Online hosts lacked the native seat,
  third-person vehicle and death cameras, TU1.5 weapon/object audio and the
  first sensor update that offline hosts have; those guards are removed.
- **Still client-side gaps.** Clients have no native particles, TU1.5 sound
  requests for other players, deferred render nodes (grips/fingers) or vehicle
  child-weapon markers. The faithful fix is the client-side TU1.5 world from
  the movement plan above, presenting from the authority replay world that
  clients already build for migration.

## Profiles

`H2_NET_PROFILE` selects the per-client budget when a client is admitted:

| Value | Budget |
| --- | --- |
| unset, `original` | Retail channel: 30720 → 71680 bit/s |
| `broadband` | 262144 → 1048576 bit/s (retail observer cap) |
| `start[,floor[,ceiling]]` | Explicit bits per second |
| `unlimited` | Deltas every tick, poses lossless: every client body is byte-identical to the host's (TU1.5 presentation parity) |
| `complete` | Stream off: complete snapshots every tick (old behavior) |

Budgeted profiles approximate remote presentation (quantized, prioritized
poses; lower message rates when busy). Simulation and every non-pose field
stay exact in all profiles. Use `unlimited` where bandwidth allows and exact
remote visuals matter.

`H2_NET_STATS=1` prints each client's rate, budget, message size, pose
refresh/root/held counts, skipped ticks, keyframes, resets, loss and RTT
every five seconds. The CTest suite runs with `unlimited`, because fixtures
assert that every `broadcast()` replicates; budget behavior has its own tests.

## Qualification

| Check | Result |
| --- | --- |
| `cargo test` (protocol crate) | 131 passed, 2 ignored probes (cost, captured corpus) |
| `native_lan_snapshot_stream` (real UDP, proxy) | Multi-datagram keyframe joins out of order; lost delta recovered from the acknowledged baseline; tampered and stale datagrams ignored; consumed raw look named; exact host states |
| Same fixture, 8 moving players, 5 s | `original`: **76.1 kbit/s**, 150/150 exact states. `complete`: 2162 kbit/s, 150/150 (28× more) |
| Captured ONLINE Lockout bodies replayed through the codec | 193 bytes/tick payload with 2 players, 336 with 4 (sibling alignment: −13% / −6%) |
| Rust stream simulation, 8 skeletal bodies + 10 static bodies, 30 s, `original` | 900/900 ticks delivered at 59.3 kbit/s; every moving root every tick (6881 root updates), 305 budgeted full refreshes, none held; no failures |
| Unchanged world | ≤ 6 bytes of payload per message |
| 208 KB body, release build | about 1.4 ms per tick for encode, decode, materialize and scene generation together |
| ONLINE lobby → match → postgame (Vista PCR) → requeue, `original` budget | Core, team, party, casual, casual party, ranked/casual gathering, voice blocks, variant mismatch, peer host takeover, common-cut runtime: all pass |
| `native_request_protocol` | Rust/C++ reference differential includes the ack fields |

Fixtures still red were checked with the stream disabled (`complete`) and fail
the same way: protocol-98 admission challenges unhandled by older proxies
(`source_device_network`, profile/team rejection), host-side gameplay or
frontend (`retail_slayer_accounting`, `native_death_match_*`, variant launch,
HUD glyphs), replay security policy (`source_host_continuation_*`), effect
lifecycle/runtime backing (`online_source_*`, effect history), clan party
consent, master-server history assertions after a passing native couch flow,
and missing local inputs (`default.xbe`; CTest's Python lacks `aiohttp`).

## Limits and remaining traffic

- Generic state is lossless and sent every tick, so the retail budget bounds
  presentation (poses) rather than total traffic. Captured ONLINE Lockout
  matches replay at about 193 bytes of payload per tick with two players and
  336 with four (new impact/weapon-effect/rumble journal entries and player
  floats dominate); plus 88 bytes of per-datagram overhead.
- Large joins send one keyframe burst; debt is capped at 0.25 s of budget.
- Articulated pose refresh rate falls with distance under tight budgets; roots
  still arrive every tick and carry held bodies rigidly. Very large skeleton counts can
  still leave distant bodies visibly stepping.
- Outside the budget: client input upstream (about 120 bytes per tick plus
  compact authentication), ONLINE migration snapshot candidates (complete
  packets, at most every 3 s when enabled), authority-archive transfers, voice
  and session configuration.
- Mixed-version peers are rejected by the protocol version (105).
