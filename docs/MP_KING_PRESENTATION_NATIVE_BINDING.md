# Native King announcer and activation consumers

King's actual category6 response bank now reaches the existing native event
selector, source queue, PC sound cache and SDL PCM/completion owners. The
registered King start publication is selected by the match's source engine type
at the existing native activation boundary. This is bounded consumer work within
the [all 39 variant goal](GAME_MODE_PROGRESS.md); normal King admission remains
gated. It does not establish an operational King match or complete presentation.

## Original data and event contracts

The loader imports the literal A8-byte rows from the selected PC `matg`→`mulg`
runtime+C8 bank. General and Slayer banks retain their original98/A8 offsets.
King uses the same recovered subtype/audience/participant/locale/hash/queue
consumers. Authored row order and cause/effect/team/general audience precedence
are preserved. Sound datums belong to the distinct PC shared sound owner and
must match that owner's exact definition bytes before native playback.

`vista_announcer_king_notice` maps the recovered world notice into the complete
34-byte prefix: category 6, actual subtype, recipient NONE, cause-player/team,
effect fields NONE, zero word 7 and FFFF data. Start uses a separate registered
slot 6 owner: category 6/subtype 0, actual player in the recipient field and cause/
effect fields NONE. It suppresses mode4 without touching caller storage and
preserves the two opaque stack-padding bytes. Native activation selects that
owner for source engine 4 and the recovered Slayer owner for source engine 2.
No built-in variant names or weapon lists select these paths.

The original PC King bank has eight rows. Portuguese sound fields are NONE in
all eight, and subtype 3 has no row. Neither case gains an invented fallback.
Caption/display identifiers survive selection, but their formatting and HUD
rendering remain separate consumers. Xbox gameplay and PC UI/sound references
retain their own namespaces. The full original evidence and limitations are in
[the source research](MP_KING_PRESENTATION_SOURCE_RESEARCH.md).

## Verification

The [verification manifest](../analysis/multiplayer/game-variants/king-presentation-native-20261011/verification.json)
pins an immutable overlay over the previously verified complete native King
snapshot. It records source, asset, executable, fixture, binary and log hashes,
base provenance and live overlay drift. Original-only generators reproduce the
complete Vista fixture/report and Xbox start-producer report before native
compilation. The base snapshot stays unchanged.

All three scoped ASan/UBSan executions passed:

- `king_presentation_tests`: 3,584 complete original delivery/queue comparisons,
  all eight response ordinals, 614 authored absent-cue selections, 1,751 world
  event-prefix mappings and 54 original start publications. Complete queue and
  recent-event banks and playback request order are compared, including opaque
  start padding and suppression. Actual source recipient bindings are consumed
  in delivery order, including the explicitly declared unused-record stimuli.
- `native_vista_announcer_tests`: actual imported King rows match the independent
  original parser. Slayer and King start cues in admitted local modes, every
  King objective response audience, Portuguese NONE handling, real decoded PCM,
  native voice tokens, completion accounting, gains, mute and flush behavior
  pass through the explicit manual SDL provider.
- `vista_announcer_events_tests`: 19,348 preexisting original general/Slayer
  event/queue comparisons and 2,688 original end-publication comparisons pass,
  including actual PC tag import.

The updated event, asset, native announcer and King owners and shared queue are
instrumented; other linked runtime libraries retain the base's ordinary build.
The updated native activation-flow translation unit also compiles against that
consistent source/header overlay. Its King branch remains gated by normal setup
and has no claimed end-to-end flow proof. Manual PCM verification does not
establish physical speaker output or Windows/Xbox mixer parity.

Reproduce with `.tools/python/bin/python tools/verify_king_presentation.py` after
the native base verifier has succeeded. Full normal King still needs its live
world-to-presentation handoff, caption/HUD and hill renderer, trait consumers,
enabled recording, objective transport and complete local/network match flows.
The five King variants and all other outstanding variants remain in scope.
