# King engine-bank native implementation

The native module preserves the original40-byte King update bank, shared36-byte
prefix, signed comparisons, occupant mask, service order and authored hill
rebuilding. Actual native session/entity delivery, occupied-world services and
received-update orchestration remain required. King stays gated and the
all-variant goal remains incomplete. [Source contracts](MP_KING_REPLICATION_SOURCE_RESEARCH.md),
[implementation](../src/source_king_replication.cpp),
[public types](../include/halo2/source_king_replication.hpp).

The independent [original-only generator](../tools/oracle_king_replication.py)
executes registered slots41..46 in unchanged retail, TU1.1 and Xbox1.5 images.
Every reached instruction is checked against original bytes after execution;
there are no patches, supplied service returns or imported native results.
The [extended original report](../analysis/multiplayer/game-variants/king-replication-20261012/extended/report.json)
SHA256 is `5ba10ce44574d926bf723c855fa03ab434deba89b650e7c740625553a6bf4cbe`.
The [fixture](../tests/fixtures/king_replication_original.json) SHA256 is
`aae2221fef93f455f18869d1245716882daa445ffe3a0dcaec315acf12497612`.

The three-image corpus contains9 initializations,6,528 differences including
repeated updates, and7,086 applications:13,623 whole bank calls. It covers all57
hills on12 disc maps and14 declared empty/partial/irregular scenarios. Complete
`C18`-byte match records, complete `1DC`-byte King states, unchanged input banks
and guard bytes match. There are6,153 real shared callback entries and765
changed-hill builds, with their pre-state/order compared separately from final
bytes. Every low-byte difference request mask and all32 shared application masks
are represented. [Original results and listings](../analysis/multiplayer/game-variants/king-replication-20261012/extended/report.json),
[native comparison test](../tests/king_replication_tests.cpp).

The service contexts are bounded: original property refresh uses an empty
occupied-player bitmap; original recording refresh executes its disabled gate.
Received phases0/2/4/255 execute their actual return paths. The78 changed phase3
calls use the real mode4/no-map-reset gate. Phase1 initialization and populated
player/recording effects remain additional required bindings. Both active
game-over no-op and inactive conversion/store paths execute using each image's
actual `7.0f` operand and declared signed tick rate. These are distinct from
authority-side phase transitions. [Original source boundaries](MP_KING_REPLICATION_SOURCE_RESEARCH.md).

The native module copies shared words before property/recording refresh, calls
received-phase application before storing the new phase, and calls game-over
when bit4 is present even if the bank boolean is false. It stores a changed hill
before the existing [source geometry owner](../src/source_king.cpp). Missing
services throw at their actual call position. An unavailable geometry span
retains the preceding hill store and prevents later occupant writes. No empty
callback is authorized for a populated native world. [Implementation and guards](../src/source_king_replication.cpp),
[complete original comparison](../tests/king_replication_tests.cpp).

## Frozen verification

[The verifier](../tools/verify_king_replication.py) copies a compatible complete
source snapshot and overlays owned files plus only the new root CMake source
and test declarations. It builds the actual root targets. The final
[frozen manifest](../analysis/multiplayer/game-variants/king-replication-native-20261012/verify-20261012T014047.910516Z/verification.json)
SHA256 is `e26168337409827cf18408575a6b595ff55f769fb4c62e74a26e62ec5a213434`.
All six commands succeed: fresh original reproduction, configuration,14 build
steps, three focused CTests, sanitizer compilation and the complete new test
under ASan/UBSan with leak detection.

All31 fresh original report/listing files and the fixture reproduce byte for
byte. Existing King geometry/occupancy/scoring/world and callback tests also
pass. The manifest pins source, binaries, commands and logs; concurrent checkout
drift is recorded separately. [CTest log](../analysis/multiplayer/game-variants/king-replication-native-20261012/verify-20261012T014047.910516Z/ctest.log),
[sanitizer log](../analysis/multiplayer/game-variants/king-replication-native-20261012/verify-20261012T014047.910516Z/sanitizer-test.log),
[source hashes](../analysis/multiplayer/game-variants/king-replication-native-20261012/verify-20261012T014047.910516Z/source-hashes.json).

The earlier11,127-call baseline and its passing verification remain preserved.
The extended corpus adds all32 shared mask combinations per scenario and actual
changed phase3 calls. [Baseline report](../analysis/multiplayer/game-variants/king-replication-20261012/report.json),
[baseline frozen manifest](../analysis/multiplayer/game-variants/king-replication-native-20261012/verify-20261012T013418.699888Z/verification.json).

This milestone does not establish packet serialization, native delivery, live
recording, hill/HUD rendering, camouflage submission or complete local/network
matches. Those remain part of the [operational goal](GAME_MODE_PROGRESS.md).
