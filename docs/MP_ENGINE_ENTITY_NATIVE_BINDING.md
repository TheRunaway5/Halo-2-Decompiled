# Shared engine-entity identity and creation implementation

Native dispatch now checks the actual engine/entity identifiers before bank
initialization, differences and application. Creation compares its signed
record identifier, adopts its entity before optional application, and ignores
the application boolean. This is a shared engine contract with explicit bank
providers; it contains no King identifier constant or variant-name routing.
[Implementation](../src/source_engine_entity.cpp),
[public interface](../include/halo2/source_engine_entity.hpp),
[original dispatch listings](MP_KING_REPLICATION_SOURCE_RESEARCH.md).

The independent [original-only oracle](../tools/oracle_engine_entity.py)
executes actual definition slots19/20/22/23 and registered engine slot39 in
retail, TU1.1 and Xbox1.5. All ten source selector slots are covered, including
the three unregistered slots and CTF/Assault's shared object. Engine-returned
identifiers differ from source-table indices. Matching King dispatch reaches
the real40-byte bank owners, authored Lockout hill building and real bounded
shared services. Every reached instruction is checked against original bytes;
there are no supplied returns or instruction patches.
[Original report](../analysis/multiplayer/game-variants/engine-entity-20261012/complete/report.json),
[whole comparison test](../tests/engine_entity_tests.cpp).

The report SHA256 is
`ef2593d75553a5e4d18f5f79fe3c73427b1eb6deef618ba32b2d1c008d8d48a9`;
the [fixture](../tests/fixtures/engine_entity_original.json) SHA256 is
`42e16ac18ead0e7c1360e1056cc59d7a014e76d7a422963175ea729de7d42a71`.
There are8,814 completed original dispatch calls,7,593 identity rejections,
453 entity adoptions,1,158 actual bank callback entries,1,344 shared service
entries and318 changed-hill rebuilds. Full `C18`-byte match records,40-byte
banks, game-over state, masks and callback argument/pre-state ordering match.
All seven-bit masks are exercised with ignored high bits and opaque forwarded
arguments. Initializers and differences return true after calling their owner;
application returns its owner's boolean; creation returns true after ignoring
that boolean. The latter false-result distinction also has a focused native
contract check against the complete original dispatch instructions.

Nine additional original calls deliberately reach the absent-engine fault:
creation identifier `NONE` matches an unregistered selection, adopts the entity,
then attempts nonzero-mask application through a null engine. These are recorded
faults, never successful calls. Native code reports the missing provider after
the same adoption. With mask0 that same creation returns true without an engine.
Mismatches return false before validating providers and leave bank/mask/entity
unchanged. Missing matching providers fail explicitly. This preserves source
side effects without inventing a successful runtime dependency.

Original property/recording services use an empty occupied-player bitmap and
the disabled recording gate. Received phases exclude phase1; phase3 uses the
real mode4/no-reset gate. Other registered engines have identity rejection and
service-free creation coverage here, not newly implemented mode-specific bank
callbacks or complete matches. [Bank service boundaries](MP_KING_REPLICATION_NATIVE_BINDING.md).

## Frozen verification

[The verifier](../tools/verify_engine_entity.py) overlays owned files and only
their root CMake declarations on the previous compatible frozen source tree.
The [manifest](../analysis/multiplayer/game-variants/engine-entity-native-20261012/verify-20261012T020511.031485Z/verification.json)
SHA256 is `e0a137fc690247c2795b5a3436ab5608c95a8588dd6b498ad82bb1768a968fb5`.
All six commands pass: fresh original reproduction, root configuration,24 build
steps for five actual root targets, five focused CTests, sanitizer compilation
and the full new comparison under ASan/UBSan with leak detection. All four
fresh original outputs and the fixture reproduce byte for byte; six binaries
and every command log were independently hash checked.
[CTest log](../analysis/multiplayer/game-variants/engine-entity-native-20261012/verify-20261012T020511.031485Z/ctest.log),
[sanitizer log](../analysis/multiplayer/game-variants/engine-entity-native-20261012/verify-20261012T020511.031485Z/sanitizer-test.log),
[source hashes](../analysis/multiplayer/game-variants/engine-entity-native-20261012/verify-20261012T020511.031485Z/source-hashes.json).

This does not establish occupied-world services, private/live entry-bank
orchestration, dirty publication, packet acceptance, session delivery or complete
local/network flows. No additional variant is admitted; the
[all-variant goal](GAME_MODE_PROGRESS.md) stays active and incomplete.
