# King update wire codec native implementation

The native codec preserves the original seven optional King fields, shared
prefix, capacity reservations, rollback clearing and partial decoder writes.
It uses actual engine banks rather than named variant cases. Native entity
delivery, buffer preparation and outer packet handling remain required; King
stays gated and the all-variant goal remains incomplete. [Original contracts](MP_KING_WIRE_SOURCE_RESEARCH.md),
[implementation](../src/source_king_wire.cpp), [public types](../include/halo2/source_king_wire.hpp),
[bank implementation](MP_KING_REPLICATION_NATIVE_BINDING.md).

The independent [original-only oracle](../tools/oracle_king_wire.py) executes
the registered writer, reader and size owners in unchanged retail, TU1.1 and
Xbox1.5 images, plus the real stream-overflow query. Every reached instruction
is checked against original bytes. It supplies no service returns and imports
no native results. The [complete original report](../analysis/multiplayer/game-variants/king-wire-20261012/complete/report.json)
SHA256 is `c0740fb1327dcb918271085b4cef2071bd4c68a429a092249a2217d3309c376e`.
The [fixture](../tests/fixtures/king_wire_original.json) SHA256 is
`6f2520b3864189d06cb3b4f80f92431ad406eaa974aab425b24911301936e891`.

The corpus has15,852 complete codec/size calls and15,468 actual overflow queries
across the three images. Native comparison covers6,198 writes,9,270 reads and384
size hints, including3,960 rollbacks. Each call compares all52 stream-state
bytes, all80 declared backing bytes including padding, and all40 bank bytes.
Capacities0..64, unaligned cursors, dirty backing, existing error bytes, every
seven-bit request and out-of-range diagnostic paths are represented.
There are93 successful zero-mask writes and4,086 successful reads with an
overflowed stream. The return value alone therefore cannot decide application.
[Complete comparison test](../tests/king_wire_tests.cpp),
[original observations](../analysis/multiplayer/game-variants/king-wire-20261012/complete/report.json).

Integer writes OR their first word and assign their entire second word when
crossing a word boundary. Rollback clears the remainder of the declared buffer,
not merely the attempted payload. A presence read at exactly the declared bit
limit reads the supplied guard byte. Native access requires explicit padded
backing; it does not invent unavailable bytes. Invalid shared metadata can
leave a partially changed bank and still consume later fields. The King reader
replaces its caller mask even on failure. Size hints are original scheduling
minima, not encoded lengths or sufficient allocation sizes. These contracts
are directly compared with whole original calls. [Implementation](../src/source_king_wire.cpp),
[source research](MP_KING_WIRE_SOURCE_RESEARCH.md).

Original out-of-range values execute the real formatting diagnostic before
continuing with the original low-bit write. Native code preserves the stream,
bank and return behavior; the discarded stack diagnostic string is not a
runtime consumer. No range rejection or error flag was added.

## Frozen verification

The [verifier](../tools/verify_king_wire.py) overlays owned sources and only their
root CMake declarations on the previous compatible frozen tree. The final
[manifest](../analysis/multiplayer/game-variants/king-wire-native-20261012/verify-20261012T015126.443485Z/verification.json)
SHA256 is `27c9c334ceb2b66b23509728e971662a4fd2536fa1cd98730cd109201da2ae7a`.
All six commands pass: original reproduction, root configuration,17 build
steps for four actual root King targets, four focused CTests, sanitizer
compilation and the complete wire comparison under ASan/UBSan with leak
detection. All four fresh report/listing outputs and the fixture reproduce
byte for byte. Logs and all five binaries were independently hash checked.
[CTest log](../analysis/multiplayer/game-variants/king-wire-native-20261012/verify-20261012T015126.443485Z/ctest.log),
[sanitizer log](../analysis/multiplayer/game-variants/king-wire-native-20261012/verify-20261012T015126.443485Z/sanitizer-test.log),
[source hashes](../analysis/multiplayer/game-variants/king-wire-native-20261012/verify-20261012T015126.443485Z/source-hashes.json).

This establishes the CPU codec and its existing King regressions. Original
entity identity, dirty publication, channel scheduling, packet setup and
application order still need their actual consumers before normal local/network
matches establish the [operational goal](GAME_MODE_PROGRESS.md).
