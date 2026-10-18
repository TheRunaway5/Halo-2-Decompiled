# Original TU1.5 audio frame and listener fixtures

Authority: unchanged English Xbox TU1.5 executable, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
`tools/generate_campaign_audio_tu15.py` recovers complete128650 audio-frame,
129F10 listener-update and their authenticated frame/voice/class/driver callers:
24 bodies,3388 instructions,12069 original bytes. Every added entry has its
actual original CALL authenticated; required SDK owners remain explicit.

Run the generator with fresh `--output NEW_CPP --directory NEW_RECOVERY`, then
`tools/oracle_campaign_audio_tu15.py --recovery NEW_RECOVERY
--connected-recovery NEW_RECOVERY --output
tests/fixtures/campaign_audio_tu15`. Both run serially through
`tools/tag_match_memory_budget.py` under the unchanged60MiB hard/64MiB measured
ceiling, zero swap and host reserve. Existing evidence is never overwritten.
The CTest returns77 when the local original-data fixtures are absent.

The262 offline declared caller cases execute1060 original instruction addresses.
They cover null/inactive/nonzero game gates; every nonempty subset of four
registered users; two camera bases/positions; five actual director callbacks;
environment flags/planes; retained-unit camera paths; final SDK boundaries;
three audio-frame readiness bytes; and pool counters including signed maximum
and wrap. Connected cases add all54 class fades, multiword driver voice bitmaps,
local-user search, basis interpolation before required normalization, full caller
chains to SDK boundaries, listener parameter caching and the original elapsed-time/
gain ramp. Clock cases cover pause/countdown/resume gates, zero/positive/negative
delta,32-bit wrap, and inactive/noncampaign state. Original158290 director queries,
32A660 stack probes,33B590 clock-data reads and12E9A0 gate queries execute rather
than receiving supplied results. Other external owners stop before execution.
No instruction patch, successful platform result or native readiness is supplied.

The native reader compares both observed and unobserved stores:524 comparisons,
9053 ordered stores,68 external stops,228 actual director queries,56 actual
stack probes and242 actual clock reads. All declared retained banks, GP/XMM registers and defined
arithmetic/direction flags match. `--omit-listener` holds the listener output
records and correctly fails. `--omit-class-fades` and `--omit-clock-gain` fail
on the actual changed class/gain banks. A separate private source mutation is rejected at
the exact scale-write instruction12A4DA by the strict provenance audit.

The reached2221BB FIADD32 requires a native x87 helper. A separate1092-case
comparison executes the unchanged six original instruction bytes on actual
host32 x86 and compares the actual generated helper's80-bit results, C1 and
exception flags for normal finite inputs under three precision/four rounding
modes. Omitting the add fails. This does not execute the whole221AF0 caller,
cover every exceptional input or establish physical Xbox hardware behavior.

The original listener records retain the full camera transform and projected
velocity. Original12A4DA writes scale1. Invalid users clear owner/cluster while
retaining other bytes; native recovery preserves this behavior. The original
writer requires an active game. A frozen live-world integration probe using the
actual opening cache executes the recovered entry for install arguments0/1,
retains game-active0, and changes only the original stack push. It does not
create an active listener or call the SDK.

These are CPU caller comparisons, not a saved Xbox session, complete execution
of all3388 instructions, complete x87 state for every caller, startup completion, final spatial/DSP mix, audible
volume, output sample timing or finished-cutscene proof. The latest diagnostic
Prophet gap still exceeds the original by2368 samples/49.333ms. Full active
listener/device admission and final audio remain open.
