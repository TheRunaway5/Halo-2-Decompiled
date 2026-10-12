# Live engine entry-bank application implementation

The native module preserves both original entry layouts and copies the private
bank into live storage before calling definition slot23. A declined engine
identity check leaves the copy in place. The entry owner ignores its callback
boolean and does not repeat upstream handle admission or stream validation.
This is an implemented shared entry consumer, not yet native session delivery.
[Implementation](../src/source_engine_entry.cpp),
[literal types and providers](../include/halo2/source_engine_entry.hpp),
[complete orchestration source](MP_KING_REPLICATION_ORCHESTRATION_SOURCE_RESEARCH.md).

Retail rows are32 bytes; both title updates use36 bytes. The added update mask
is preserved separately from the requested mask. Creation size precedes its
pointer, and state size precedes its pointer. Sizes are signed32-bit fields;
copying uses their original raw unsigned `REP MOVS` count. Creation success is
a byte, and typed block size/type are signed16-bit fields. The update-apply
owner consumes the row's state size, not the block's size/type metadata.
Explicit native spans bind source address metadata to actual backing.
[Original row/callback ABI](MP_KING_REPLICATION_ORCHESTRATION_SOURCE_RESEARCH.md),
[reference declarations](../tools/reference/halo2-decompiled/src/unknown_08ad30.cpp).

The independent [original-only oracle](../tools/oracle_engine_entry.py)
executes whole retail8AC40/TU1.1 8B810/TU1.5 8EA70 through their actual interface
tables, then actual definition slot23 and matching King bank owners. It starts
from declared original match records, authored Lockout marker rows and genuine
definition vtables. Actual slot39/slot0 getters establish identifiers. No native
result, instruction patch, supplied service return or allocation return is used.
[Final original report](../analysis/multiplayer/game-variants/engine-entry-20261012/typed-layout/report.json),
[comparison test](../tests/engine_entry_tests.cpp).

The report SHA256 is
`09b429d4a8aef518926d4af456115e877e3be20feb715d24740632d86f813a36`;
the [fixture](../tests/fixtures/engine_entry_original.json) SHA256 is
`d16e65f21cd35d00376e61ff0fb074876016c63142bafde3c0c87f5aca6afee1`.
There are4,062 whole entry calls,3,678 identity rejections after copying,384 real
King bank callbacks,672 shared service entries and126 changed-hill builds. All
`C18` match bytes, all192 arena bytes, complete32/36-byte entries, game-over
state and callback arguments/pre-state order match. Masks, opaque row fields,
stale full identifiers and arbitrary block metadata remain literal inputs.

The222 additional owner controls exercise lengths0..64, every dword/byte
remainder and forward overlap. They establish the owner's copy contract, not
accepted packet inputs; the original update decoder normally allocates a
distinct private bank. The native copy uses forward dword temporaries and then
bytes, retaining the original overlap behavior without host alignment/aliasing
requirements. Missing matching application providers fail after the copy.
Unavailable table/backing inputs fail explicitly rather than inventing data.
[Original controls](../analysis/multiplayer/game-variants/engine-entry-20261012/typed-layout/report.json),
[native guards](../src/source_engine_entry.cpp).

Shared property/recording callbacks execute the real empty-player/disabled
gates in the oracle. Phase1 world initialization is excluded, while phase3
uses the source mode4/no-reset gate. Native observations for these contexts
are not occupied-world providers. The [previous bank implementation](MP_KING_REPLICATION_NATIVE_BINDING.md)
and [identity dispatcher](MP_ENGINE_ENTITY_NATIVE_BINDING.md) retain their own
independent original/native comparisons.

## Frozen verification

The [verifier](../tools/verify_engine_entry.py) overlays owned files and only
their root CMake declarations on the compatible frozen source tree. The final
[manifest](../analysis/multiplayer/game-variants/engine-entry-native-20261012/verify-20261012T022137.698867Z/verification.json)
SHA256 is `39ac2d5cde1d545b7f718810a4bcaeb984104b76b55e50cfbfe11cf3e0f0cd9e`.
All six commands pass: original reproduction, root configuration,32 build steps
for six actual root targets, six focused CTests, sanitizer compilation and the
complete new comparison under ASan/UBSan with leak detection. All four fresh
original outputs and the fixture reproduce byte for byte. Seven binaries and
all logs were independently hash checked; concurrent checkout drift is separate.
[CTest log](../analysis/multiplayer/game-variants/engine-entry-native-20261012/verify-20261012T022137.698867Z/ctest.log),
[sanitizer log](../analysis/multiplayer/game-variants/engine-entry-native-20261012/verify-20261012T022137.698867Z/sanitizer-test.log),
[source hashes](../analysis/multiplayer/game-variants/engine-entry-native-20261012/verify-20261012T022137.698867Z/source-hashes.json).

An earlier passing snapshot preserves the initial layout declarations and
corpus. The final run corrects creation size/pointer naming/order and signed
size/block types from the complete source trace; no prior proof was replaced.
[Earlier snapshot](../analysis/multiplayer/game-variants/engine-entry-native-20261012/verify-20261012T021817.925966Z/verification.json).

Positive private-bank allocation/decode, creation, dirty publication, peer
scheduling, aggregate application, final packet handling and actual native
delivery remain required. The module adds no variant admission or complete
match proof. The [all-variant goal](GAME_MODE_PROGRESS.md) stays active.
