# Native score-change announcer consumer

The native announcer now exposes the recovered shared general/Slayer/King
event-audio dispatcher. Actual native King score changes can feed the verified
shared threshold producer into this consumer, preserving the original event
prefix and selecting the authored PC general response bank. The existing King
notice entry point delegates to the same consumer while retaining its category
check. Caption/display identifiers remain intact.
[Native consumer](../src/native_vista_announcer.cpp),
[shared original threshold producer](MP_SCORE_EVENT_NATIVE_BINDING.md).

Four actual Lockout/Xbox1.5 match runs cover FFA/team and stationary/moving
King with score limit61. Each run produces61 score changes, three threshold
events and three successful native PCM completions. All12 cues select actual
authored cause-player/team audience rows, sound references and text identifiers.
Playback/game-over guards, queue scheduling, finite audible PCM, real native
voice accounting and completion status/frame counts pass. The test uses actual
match player datums/membership and the genuine pre-world presentation callback.
[Score-to-audio test](../tests/score_event_audio_tests.cpp).

An immutable overlay reuses the verified complete HUD/native runtime build.
All15 commands pass: ordinary native score-audio runs, existing native announcer
regressions, all8,640 original score-producer comparisons and scoped
ASan/UBSan/leak checks. The producer, native announcer, source event selector,
queue and new test are instrumented; linked game-runtime libraries retain the
ordinary frozen build. Source, actual assets, commands/logs, base artifacts and
three new binaries are pinned. Verification SHA256:
`4ede21546de6394fff7dacecf6c50d320c7fe09b0caf0386ebf8c3a54b84169f`.
[Verifier](../tools/verify_score_event_audio.py),
[frozen verification](../analysis/multiplayer/game-variants/score-event-audio-native-20261012/verification.json).

Declared debug positions and a Manual device provider isolate these checks.
Network/hill-notice service calls are observed; native event transport, caption
formatting/delivery, normal world handoff/admission and complete playable match
flows remain open. Manual PCM does not establish physical speaker or original
Xbox/Windows mixer output. The all39-variant goal remains active and incomplete.
