# Original update data in the live multiplayer importer

An explicit `--gameplay-xbe PATH` selects the user's verified Map Pack1.1 or
owned1.5 executable as a data source. The application does not execute or
compile that original binary. `OriginalData` validates the supported image and
imports its original tables/constants. Native C++ uses them while importing a
private copy of each map. The on-disk maps and executable remain unchanged.
Default runs retain the previous disc-data path; the update must currently be
selected explicitly, including when passed through the title launcher.

`Cache` now executes native equivalents of the original map-load postprocessors
in their original order, before gameplay/BSP/spawn readers consume metadata:

1. Netgame correction, original1.1 `11DF00` /1.5 `11F060`.
2. Combat tuning, original1.1 `164750` /1.5 `165B30`.

The complete combat routine has262 original instructions and no call/service
dependencies. Both updates produce the same41 ordered stores,38 actual value
changes and13 changed tags on the raw Lockout cache. The native implementation
preserves source reference gates, signed32 BR-barrel count, and write order;
the19 tuning words come from the selected original executable. The SMG upper
damage bounds therefore become4.625 from raw4.75 while the lower bound remains
unchanged. This existed in1.1 and is not a newly discovered1.5 balance change.
See [complete original source](MP_COMBAT_TAG_POSTPROCESS_UPDATES_RESEARCH.md)
and [original branch controls](MP_COMBAT_POSTPROCESS_BRANCH_CONTROLS_RESEARCH.md).

The netgame routine preserves each image's source arithmetic order and map-ID,
type and squared-distance gates. Both versions perform the measured Beaver
Creek correction;1.5 adds the Relic correction. Exact constants are imported
from the image. The native leaf records the original access requests but
operates on already writable private host storage; it does not implement Xbox
page-table protection. See [original netgame source](MP_NETGAME_POSTPROCESS_1_1_TO_1_5_RESEARCH.md).
Native teleporter traversal still lacks its live consumer, so a processed
Relic record is not a claim of working teleport gameplay.

The runtime reports both store counts when an image is selected. Multiplayer
compatibility identity includes the selected executable's bytes, as well as
the disc resources still required by rendering. Actual UDP joins reject raw
versus updated data identities, accept matching identities, and give the same
identity to an identical update file at a different path. Choosing different
updates or modified files does not silently join one session.

## Verification and limits

`verify_mp_balance_native.py` reconstructs the complete expected cache from
the frozen original processed tag buffers and compares every native store and
the complete cache. Its live-consumer mode additionally compares native
`Cache::load_content` damage/error values with a separate original-processed
cache and exercises actual UDP compatibility joins. Both selected updates
pass; the current integrated report is
`analysis/multiplayer/title-update-1.5/native-balance-integrated/report.json`.

`verify_mp_balance_controls_native.py` compares43 declared variants/86 original
runs, including2798 ordered stores,82 normal returns and4 original invalid
access stops. Native host bounds rejection preserves the same writes preceding
those invalid accesses. Normal and ASan/UBSan builds pass. The current
integrated sanitizer report is
`analysis/multiplayer/title-update-1.5/native-balance-controls-integrated-sanitized/report.json`.
The separate netgame leaf differential matches all six original complete-cache
outputs and the original store/access request order.

A dedicated1.5-selected runtime ran three gameplay ticks; a one-player rendered
Lockout1.5 capture ran on NVIDIA GeForce RTX5080 with Vulkan validation and no
VUID messages. These are bounded import/runtime checks. They do not prove all
1.5 executable gameplay, every update/region, damage from human input,
animation/action managers, explosions, gravity lifts, teleports, or whole-game
parity. The separate keyboard/mouse rendered probe establishes only its
[documented live controls](MP_KBM_CONTROLS.md).

No original asset, XBE payload, DLC cache or frozen generated fixture is added
as repository source. The local imported originals remain outside the code
repository; generated comparisons stay ignored under `analysis`.
