# Shared score-change event producer

The shared score-change producer now implements whole original retail19F470,
TU1.1 1A2DD0 and Xbox1.5 1A9080. Remaining scores10/30/60 select the literal
FFA subtypes47/9/7 and team subtypes48/10/8. Unsigned subtraction, independent
threshold tests, selected-engine/team and raw membership reads, playback-mode4
suppression, network-before-local publication and options-pointer reloads
preserve the original order. Each publication preserves all34 defined bytes.
[Implementation](../src/source_score_event.cpp),
[explicit consumers](../include/halo2/source_score_event.hpp).

All8,640 whole unchanged original calls match the native producer, including
1,800 network and1,800 local publications. Executions cover all three images,
source modes, team/engine presence, signed membership, raw player slots and
unsigned score/limit wrapping. Original dispatch executes with actual Lockout
tags and declared nonlocal users; no audio or HUD return is supplied. Native
checks also cover options changed by a consumer between thresholds and missing
consumer failures after earlier publication.
[Original oracle](../tools/oracle_score_events.py),
[unchanged report and listings](../analysis/multiplayer/game-variants/score-events-20261012/complete/report.json),
[native comparison](../tests/score_event_tests.cpp).

A fresh frozen verification reproduces all10 original outputs byte for byte.
Both score-event/result-event CTests and the complete standalone ASan/UBSan/leak
comparison pass. All six commands pass, with pinned source, CMake overlay,
logs and three binaries. Verification SHA256:
`15e11d4866b4428f88333ac860462507cab05f4de6beeece19553e8a4a49a596`.
[Verifier](../tools/verify_score_event.py),
[frozen verification](../analysis/multiplayer/game-variants/score-event-native-20261012/verification.json).

These thresholds and event construction belong to executable code. The
response text, sound references and response attributes belong to tags. Live
native network/caption/audio publication and normal game-mode admission remain
separate. No additional variant is admitted; the goal remains incomplete.
