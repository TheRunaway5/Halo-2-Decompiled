# Native registered SMG primary clock

This bounded port consumes the original registered single-SMG hand, original
weapon bit17 and five authored FP graph clips. It follows the frozen original
[clock/request/event authority](SMG_FP_CLOCK_1_1_RESEARCH.md). Original assets and
raw source traces remain under ignored local `assets/` and `analysis/`.

`src/fp_clock.cpp` preserves separate float32 delta/rate/30/add operations,
the authored frame-count epsilon, event traversal before frame storage, wrap
and predictive end flags, future timing masks and original post-clock activity
selection. A repeated prepared shot refreshes signed hand counterF2 without
resetting the ongoing fire clip. This counter is distinct from the unit's
nine-tick hand feedback timer. A reload transfer's original idle notification
is retained separately from the earlier visible reload animation end.

`Cache::load_first_person_binding` imports the five selected original clip
indices/frame/loop records, graph sound references and original timing/sound
event bytes. Original full StringIDs, marker and reference flags are preserved.
The runtime does not synthesize sound playback from these records. The actual
source audio consumer, successful sound/resource handles and camera admission
remain separate integration work.

Native prepared-shot, explicit/automatic reload and transfer notifications
feed the last source-bound local view for the actual weapon/full current unit.
The observer clocks run after the native object postpass. The renderer samples
that exact frame directly, avoiding a seconds-to-frame round trip. Four-player
Turf capture38 records independent clips/frames at tick62: reload31/frame16,
idle21/frame62, idle21/frame38, idle21/frame62.

`tools/differential_fp_clock.py` pins the independently frozen executable, map,
tools, audit, canonical source fixture and byte-identical reports. It exports
only source outputs. `tests/fp_clock_tests.cpp` compares541 ticks across five
source phases,2705 represented primary-channel states and four exact20-byte
callbacks. Five ordinary cases additionally execute516 actual local match
ticks. The clean ready control supplies the original admission boundary and
does not claim that native current-index/timed-equip producer is integrated.

The first comparison failure was a fixture-reader offset error: hand+84 is
manager+7C, because the manager begins at hand+8. The source trace was unchanged;
the extractor now reads the correct recorded manager field. Its original
failure log is retained. A concurrent shared weather compilation failure and
a target added after configuration were also retained before the successful
reconfigured build. No source expected output was changed to fit native output.

Secondary/dual blend channels, full observer/camera/aim response, feedback
spring decay, fidget local-seed initialization, resource lifetime, audible
playback and original Xbox reciprocal/matrix bit parity remain open. Remote
native LAN clients lack replicated full source binding/event state and retain
the earlier display bridge. This result establishes neither retail networking
nor complete multiplayer parity.
