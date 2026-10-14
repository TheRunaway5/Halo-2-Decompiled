# Host configuration before multiplayer world preparation

This is the native port's transport integration. It does not claim retail
Xbox System Link wire compatibility or complete physics parity. Gameplay
authority remains the original English TU1.5 routines and owned Xbox assets.

## Recorded failure

`live-light-client-default-v244` declares a Spectre host (Primary Light5)
and a client with saved Map Default0. Both use the actual UDP session. The
client prepares a Warthog model bank before learning the host's settings,
then fails with `posed model world bank count changed` when the actual source
Spectre WORLD hierarchy arrives. Matching local preferences cannot prove this
integration.

## Native configuration discovery

Protocol76 adds a configuration query and fragmented description. The host
retains its validated `UiVariant`, including every ordered settings write.
It publishes that actual configuration and its existing asset/starting-weapon
fingerprint without admitting a player or allocating a body. The client adopts
the settings before `MultiplayerPreparedWorld`, removes its saved CLI starting
weapon override, and verifies the host fingerprint against its local map,
shared assets, executable and resulting configuration before joining.

The description preserves aliases/repeated writes rather than sorting them.
Its bounded fragments retain the query nonce, fingerprint, index/count and
total size. Reassembly rejects inconsistent/conflicting fragments and waits
for the complete description. Discovery retries missing replies independently
of the subsequent gameplay peer timeout.

The normal rules builder retains its validated configuration. Existing host
constructors therefore publish their actual input rather than reconstructing
settings from partial parsed fields. Handwritten rules can leave it absent;
such a host explicitly rejects discovery instead of inventing a preset. The
runtime and acceptance host also provide their effective starting-weapon
override as an ordered source setting.

## Loading and ownership

`session-loading-red-v252.log` proves that a host with declared settings cannot
answer discovery while its loading/rendering owner has not started gameplay
receive. The query times out in that version.

The LAN configuration service now answers these read-only requests during
loading. The passing loading test uses an actual UDP query with no gameplay
receive, then sends an actual hello and confirms that no player/tick is created
until the gameplay owner receives it. Other packets are queued under the receive lock and remain for
`LanHost::receive` on the gameplay owner thread. The service never calls
join, step, physics, camera or snapshot production. Its thread stops before
the socket and queue are destroyed. Regular receive can also drain the socket
under the same lock, preserving immediate owner-thread receive semantics.

ONLINE's current transport gates gameplay datagrams until loading completes.
Its existing loaded-configuration digest/admission path is preserved. The
configuration codec/query overload is tested with a routed transport, but
actual ONLINE configuration adoption before loading remains a separate open
integration; a pre-loading query through its gated gameplay channel is not
claimed to work.

## Evidence

Evidence is under `analysis/multiplayer/physics-live-tu15-20261012/`.

`session-bootstrap-ctest-v254.log` passes all four checks: native multiplayer,
original light-choice, session configuration and loading ownership. The transport fixture
preserves1200 ordered writes over actual UDP with a dropped fragment, reversed
delivery and duplicates. It verifies no peer/player admission or world-clock
advance, rejection for undeclared settings, and the routed transport overload.
A stale test expecting all specific weapon choices except SMG to be rejected
was updated to the already implemented original3..20 admission; physics and
weapon behavior were not changed to satisfy it.

| Visible UDP case | Client's saved preference | Exact states | Actual travel | Camera turn |
| --- | --- | ---: | ---: | ---: |
| Spectre, `live-session-spectre-client-default-v254` |Primary Light0, starting weapon8 |901 |22.803234 |0.600000 |
| Gauss Warthog, `live-session-gauss-client-default-v254` |Primary Light0, starting weapon8 |902 |28.591244 |0.600000 |
| Wraith, `live-session-wraith-client-default-v254` |Heavy0, starting weapon8 |902 |12.757482 |0.600000 |
| Secondary Spectre, `live-session-secondary-spectre-client-default-v254` |Secondary Light0, starting weapon8 |901 |17.943495 |0.600000 |

These runs adopt host settings before model preparation and pass actual entry,
driving, turning, exit, authority/client camera position/basis/FOV comparison,
camera response/restoration, submitted model definitions, geometry in both
views and complete UDP state comparisons. Their unaltered recordings and
executed ELF hashes are retained beside each report.

The Secondary Light live case initially exposes an omitted parameter77 in the
native settings capability mask (`live-session-secondary-spectre-client-default-v250`).
The fixed class2 selector/template path already matches the unchanged original
dispatcher; parameter77 is now admitted, while random births remain explicitly
unavailable in the template owner. Its final live run is included above and requires actual class2 placement.

`live-session-regression-v254` passes movement/lift/objects with181 states each.
The recreated prop's two actual point impulses again match all512 motion bytes
against unchanged TU1.5. The older runtime host constructor also passes the actual four-view saved-Elite
LAN runtime (`session-legacy-species-v255.log`): 121 authoritative ticks,
Vulkan bodies/arms/cameras and actual grenade attachment/release. Legacy fixture hosts now derive their complete rules from the declared source
settings, keeping advertised configuration and actual rules together.

These are composed native regressions, not proof of
all retail trajectories, every vehicle/seat, every contact/constraint mode,
campaign coverage or default-launcher delivery.

Native protocol78 additionally carries source-selected vehicle action intent
(object and seat) into the original delivered command. The selected team is
also delivered through the original roster/mask and biped creation inputs;
see [source player-team birth](MP_PLAYER_TEAM_BIRTH_TU15_20261014.md).

Pickup completion retains its release-frame vehicle target and exact saved
duration while later native UDP packets are coalesced. The regression first
failed with a retargeted hold, then passes with delivery exactly once and the
subsequent continuous target retained. Eight focused tests pass in
`release-intent-focused-v285.log`.
