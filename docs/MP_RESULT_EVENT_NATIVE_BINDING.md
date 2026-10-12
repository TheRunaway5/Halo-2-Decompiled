# Original result-event recording implementation

The shared result recorder and ring append are implemented from whole original
retail/TU1.1/TU1.5 execution. They preserve both enable/freeze checks, the
unsigned `(last-first_now)>3` condition, two separate clock calls before pose
resolution, aligned XYZ fields, raw argument order, relative-time wrapping,
last-time mutation on pose failure and the1000-entry ring overwrite.
[Implementation](../src/source_result_event.cpp),
[explicit time/pose services](../include/halo2/source_result_event.hpp),
[original contract](MP_KING_RECORDING_EVENT_SOURCE_RESEARCH.md).

All1,944 whole original event calls and72 direct append calls match, including
360 recorded events and1,512 actual event-path clock queries. Every36000-byte
ring is compared, including untouched bytes, with exact index/last/start and
callback order. Disabled/frozen recording does not access missing services;
missing admitted time/pose owners throw at the corresponding source boundary.
The separate original report also executes startup, timestamp and time probes,
bringing its total real kernel queries to1,527.
[Native differential test](../tests/result_event_tests.cpp),
[unchanged original report](../analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/report.json).

The frozen verification freshly reproduces all65 original report/listing
files byte for byte. Four focused CMake/CTest checks pass for event recording,
statistics, combat capture and retained remapping; a complete standalone
ASan/UBSan/leak run also passes. The manifest pins the source, CMake overlay,
commands, logs, original outputs and five binaries. Its SHA256 is
`1ea1d9e4d59a284cf0b03d996a2dc2af9874e84359bcb1e883c5b6565b660c72`.
[Verifier](../tools/verify_result_event.py),
[frozen verification](../analysis/multiplayer/game-variants/result-event-native-20261012/verification.json).

This isolated verification uses declared platform time32 and object-position
services. It does not establish native world lifecycle/pose delivery, retained
identity/machine/header completion, normal King admission or complete matches.
The all39-variant goal remains active and incomplete.
