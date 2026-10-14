# Tombstone playlist coverage

`master_server/examples/matchmaking-tombstone.json` extends the existing native
catalog with the user's requested Tombstone coverage. These are newly authored
rotations alongside the final-2010 restoration target; they are not recovered
Bungie hopper weights or a claim that the full 2010 catalog is finished.

**Activation boundary:** `matchmaking-tombstone-fiesta.json` is the deployable
Fiesta extension to the existing nine queues. The broader
`matchmaking-tombstone.json` is staged. After the
[staged team-initialization repair](tombstone-team-owner-followup.md), ordinary
match simulation stops in the TU1.5 live world at
`13B6F0 → 1816AA → 181CB0`; those additional Tombstone entries must remain
inactive until the normal physics/runtime path passes gameplay checks. CTF is
also staged, pending its objective engine. A catalog/admission test alone does
not establish that an entire match can run.

| Playlist | Tombstone selection | Players |
| --- | --- | --- |
| Casual Slayer | Slayer | 6–8, free for all |
| Casual Team Slayer | Team Slayer | 4 versus 4 |
| Ranked Team Slayer | Team Slayer | 4 versus 4 |
| Action Sack | Fiesta Slayer | 6–8, free for all |

Slayer and Team Slayer give Lockout and Tombstone equal weight. Action Sack
retains its four existing Lockout/Foundation Fiesta/Infection selections and
adds Tombstone Fiesta at the same per-selection weight. This makes Fiesta
three of five selections; Infection remains two of five. The two-player and
four-player validation queues, clan queues, skill/rank policies, couch identity
and guest-stat exclusions retain their existing behavior.

The map key is **`highplains2`**, the revised original Xbox Tombstone cache,
SHA-256 `b14a0debdb2e279c7bd7c7cda2b3e2a5cf69ca85f457923c1cfb9de6ca0b1bb1`.
The supplied older `highplains.map` is a different revision and cannot satisfy
this capability. Both revisions remain preserved as original private inputs.

The owned map has 91 player-start records and authored red/blue flag spawn and
return markers, including multiple/single-flag flags. Those records establish
authored CTF placement coverage, not native objective behavior or a recovered
playlist balance policy. **Include Tombstone when the CTF objective engine and
its matchmaking coverage are completed.** CTF is deliberately absent from the
active native catalog while that engine remains unavailable.

## Install the owned map

The native ONLINE capability scan reads the prepared Xbox maps directory.
Install the owned revised cache there, retaining its original name:

```sh
python3 tools/install_tombstone.py '/path/to/owned/highplains2.map'
```

The tool reads the launcher's selected Xbox asset root, checks the exact cache
hash, makes a private copy and refuses to replace a different existing cache.
`--maps DIRECTORY` selects an explicit prepared Xbox maps directory. No map
bytes are included in source or release bundles. Restart the game after adding
the map so the client advertises its new capability. The existing admission
policy requires every map in a playlist; players without revised Tombstone
retain the smaller validation queues but cannot enter these expanded pools.
Future catalog/map-package changes must preserve that loading guarantee.

## Validation

The existing custom-mode test passes the actual revised cache through all 14
Fiesta weapon loadouts and Infection conversion, sword respawn, survival and
completion/PCR checks. A finite four-view Vulkan launch uses the installed
native runtime, original TU1.5 data and Vista HUD bank. Backend tests exercise
actual gathering and authenticated relay admission for two four-seat couches
on Tombstone Slayer, Team Slayer and Fiesta; their map chooser is forced to
Tombstone to exercise that branch. Tests also reject the missing-map and old
revision cases. These are bounded native/host checks, not a WAN playtest or
proof of complete original map/gameplay parity.

Two independent native processes on one Linux host also complete Tombstone
Fiesta with four local players each, all four movement inputs per couch,
authenticated relay traffic, original Vista PCR, Done/requeue and carnage-history
lookup. Four verified personal records are stored; four guests remain only in
the local eight-player report. External Discord authorization is a fixture in
this automated test. Set `HALO2_COUCH_TEST_MAP=highplains2` when running the
existing couch launch harness to reproduce it.

The live master uses `/opt/halo2/catalogs/tombstone-20261013T225156Z.json`.
Its existing backend image is retained. The stopped-state backup was restored
in isolation, SQLite integrity and foreign keys passed, and retained account,
party, clan, rank, result and key hashes matched after activation. The local
delivery evidence is `.local/tombstone-playlists-20261013/verification.json`.
