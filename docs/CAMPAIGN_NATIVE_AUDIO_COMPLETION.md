# Campaign native audio completion

The October30 user request explicitly includes **cutscene audio, sound effects
and dialogue in 1:1 native TU1.5 kernel completion**. This remains part of the
active [campaign engine goal](CAMPAIGN_NATIVE_ENGINE_GOAL_CHECKLIST.md), alongside
the [whole-game sound goal](AUDIO_PARITY_GOAL.md). Original Xbox TU1.5 behavior
is authoritative; imported MCC content is runtime data.

The live source is not complete. The
[current ownership audit](../analysis/campaign-native-audio-20261030/current-scope-v3.json)
authenticates all40 required campaign-group owners against unchanged pinned
TU1.5 code. Four have semantic/original qualification (`129500`, `12DB00`,
`225D70`, `228310`),15 remain unqualified register-level candidates, and21 are
absent. Additional qualified dependencies exist outside the required40. The
complete native campaign registry is withheld, and a transitional lifted
fallback remains. Presence in a C++ file does not establish native parity.

The initial October30 unit-speech audit found an empty registry. Its source module has nine
original owners, including415-site facial-curve owner `110520`,70-site sound
metadata owner `21F860`, and playback/attachment helpers. `18F510` already has
an object-loop implementation and `D3300` has a typed stop-notice dependency;
these do not qualify the complete dialogue/facial playback chain.

The subsequent [native envelope sampler](CAMPAIGN_NATIVE_SPEECH_ENVELOPE_TU15.md)
now registers `21F860` through the actual sound manager. Its454 physical
original cases,11 semantic mutations, compiler/sanitizer checks and independent
exact repeat pass. This is one closed dialogue dependency; the old generated
body's retirement and complete speech/facial/playback composition remain open.

The subsequent [native smoothing dependency](CAMPAIGN_NATIVE_SPEECH_SMOOTHING_TU15.md)
registers `18F7A0` with complete SIMD results and the qualified generic object
lookup. Its218 physical cases,20 physical sampler/smoothing compositions,12
explicit observer admission cases, exact repeats and compiler/control cohorts
qualify this bounded dependency. The required40 campaign-group gate and whole
dialogue/facial/playback readiness remain unchanged.

Completion requires all of the following, with independent original evidence
and composed authored playback:

- [ ] Cutscene scripted impulses, music, ambience and cinematic dialogue use
  complete generic native scheduling, playback and original audio clocks.
- [ ] Sound effects retain authored variation/RNG, gain, pitch, sample choice,
  emitter placement, attenuation/occlusion, listener routing, loop transitions
  and exact stop/retirement behavior.
- [ ] Dialogue retains eligibility, priority, interruption, queue/cancellation,
  sample progress, facial curves and authored timing/synchronization.
- [ ] Decode, resampling, mixing, filters, spatial routing and DSP behavior
  have original numerical/state evidence. Host device adaptation and actual
  audible Xbox/output comparison are verified at their appropriate boundaries.
- [ ] Pause, revert, checkpoints, BSP changes and co-op listener/participant
  state use the same retained campaign-world authority.
- [ ] Accepted paths execute native semantic kernels throughout, with a
  residual-owner census proving no lifted, virtual-register-machine,
  emulated-hardware or executable-backed gameplay fallback.
- [ ] Full authored mission/co-op playback and the launcher-selected runtime
  pass; component or packet delivery checks alone do not close this scope.

The audit only reads source and unchanged offline original instructions. It
does not activate the audio registry, alter qualification flags, change assets,
select a runtime or claim audible playback. Its original SHA-256 and current
source hashes are retained in the linked report. Its owned scratch is removed.
