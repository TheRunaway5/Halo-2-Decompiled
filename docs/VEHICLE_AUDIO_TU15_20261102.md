# Vehicle boost, horn and loop-controller wiring — November 2, 2026

The baseline reproduces both Warthog horn failures through seated driver input.
Left-click emits the authored horn request, but native playback rejects its
entry/exit controller. The adapter also rejects a flag used by the original
Ghost engine tracks. The prepared Coagulation cache contains the horn and
boost assets.

The shared playback path now consumes ordinary, always-spatialized,
synchronized and fake-spatialization loop profiles independently of their
authored track phases. It accepts alternate-only tracks, retains centered or
spatial caller locations, and preserves streams and permutation state across
location changes. There are no vehicle-name sound selections in production.

Start-to-loop transitions retain the existing voice's initial pitch. The new
phase's gain scale is consumed by the following controller frame, matching
`222C17` and `222D37`/`222E10`. Source rate, class, pitch-control shape and
routing changes retain their explicit output requirements. Centered detail
sounds select from actual local listeners before drawing their offset; the new
240-case fixture compares the unchanged original `223670` prefix, including
its `150900` and `223870` children, before the BSP query. No original callee
result, availability flag or voice result is supplied.

## Evidence and remaining scope

Reports are under
[`build/test-results/vehicle-controls-audio-20261102`](../build/test-results/vehicle-controls-audio-20261102).

- `red-v2` retains both actual horn-input failures and the initial playback
  matrix. Its Ghost scene did not enter a seat and is not boost acceptance.
- `all-vehicle-loop-profiles.json` inventories 59 vehicle loop definitions
  across the authenticated original reference maps. All six root-flag values
  and all five track-flag values appear in the 39-loop Coagulation fixture.
- `candidate-v2` passes six original-reference controller tests, including
  all 240 centered-detail cases. Its matrix identifies the four traversal
  start-to-loop failures fixed subsequently.
- `campaign-matrix` passes six suites, including the explicit-map phase
  checkpoint/replay and the original combined-request controller test. The
  matrix exercises all 59 inventoried definitions with actual imported data
  from Coagulation, Flood Zone, Gas Giant and Old Mombasa. Of 196 requests,
  194 produced PCM and drained on stop. The two spatial gasoline-fire cases
  verify the explicit four-bin output requirement. Imported flag differences
  are confined to the MCC extension bit `0x100`.
- `integration` and `qualification` are earlier build attempts that stopped
  before the input scenes. The disposable scene build compiles one generated
  non-audio unit at `-O1`, preserving strict floating-point flags and the
  normal Release configuration for the audio code.
- `live-input` and `live-input-v2` missed Warthog seat admission after changing
  the warmup. `live-input-v3` restored the reproduced driver approach. Both
  Warthogs then submitted two Classic horn starts and four loop packets from
  their actual horn weapon children, with no horn voice after release. Its
  assertion incorrectly expected the ordinary horn to leave common RNG
  untouched because the same vehicle also has synchronized engine loops.
  `live-input-final` corrects the assertion and passes all nine tests,
  including both Warthog variants, the Ghost, and six original controller tests.
- The Ghost test confirms both authored boost sides, four start submissions
  for two presses, eight loop submissions, and no boost voices after release.
  The actual source boost charge is 1 while held and 0 after release.

These checks do not certify full TU1.5 sound-engine or audible-game parity.
Combined four-bin gasoline-fire output, full spatial/DSP behavior, all campaign
and online vehicle actions, original waveform equality and physical speaker
output remain separate requirements. The launcher's selected runtime is not
promoted by these source/test changes.

The [qualification and cleanup receipt](../build/test-results/vehicle-controls-audio-20261102/qualification-summary.json)
records the final tests, source pins, and removal of the disposable builds and
temporary source copies. It records 2.63 GB for the main scratch owner before
cleanup. Bounded reports, source pins and the original-reference fixture remain.
