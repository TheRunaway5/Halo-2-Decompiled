# Retained world effect events

`NativeWorldEffects` now consumes `source_effect_event.hpp` for retail
1782A0 selection,179190 traversal and1792B0 advancement. The original
`assets/disc/default.xbe` is pinned to SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.

The old native scheduler sampled every event at birth, applied a skip test to
event zero, and approximated attachment repeats with a separate emission
loop. The retained clock now selects event zero directly, samples each delay
at selection and each duration when that delay completes, traverses later
skip probabilities in order, and carries remaining time across transitions.
It preserves the original eight-iteration limit, runtime repeat bits, authored
loop start, previous event, stop/suppress flags, and retirement boundary.
Event flags do not replace the authored skip probability with one.

259D0/179190 retain x87 intermediate precision until their store/comparison;
constant ranges consume no random draw. Effect definition bit2 selects the
primary or auxiliary word. This does not establish the original allocator's
complete random history or ownership of the native visual primary word.

The native adapter dispatches parts at each event begin, reuses that event's
retained systems and particles across wraps, and stops emission while remaining
particles drain. Existing particle, sound, decal and light services remain
separate consumers. The static scenery importer still has its earlier marker,
function and single-event admission boundary. Completing this clock does not
make every map attachment live by itself.

## Original and native verification

`analysis/map-world-features/effect-events-20261012/verification.json` records
5,917 exact comparisons, including 248 attached effect definition banks read
from all 21 Xbox multiplayer caches. Additional synthetic inputs cover signed
indices, suppression/freeze/stop/repeat flags, endpoint and zero-duration
transitions, skipped chains, ranged delays/durations, floating-point
cancellation and pending retirement. Every clock word, selected RNG word,
unchanged alternate RNG word, callback order and callback float argument is
compared with the original instructions. Downstream services 17ADD0,17A380,
179020,179FB0 and177610 have explicit fixture outcomes; they are not executed
as a complete original particle/backend world.

Reproduce the independent executable comparison with:

```sh
.tools/python/bin/python analysis/map-world-features/effect-events-20261012/original.py
```

`original_effect_event_timeline` replays the retained original fixture bank.
`native-checks.json` records four passing integration groups against the
previously accepted frozen engine plus this scheduler change: minute-long
main-menu flak, actual Lockout SMG/grenade contacts, Midship plasma, and actual
SMG contact/effect/audio playback on all 21 Xbox multiplayer maps. The latter
checks do not cover all map interactions, every shader, or original matched
framebuffers. `native-provenance.json` retains exact compilation inputs and
library/binary hashes; later concurrent root features are excluded from that
isolated integration result.

The normal-launcher attempt is separately retained in `normal-launcher/`:
snow on/off/repeat captures pass with 2,450 changed pixels and identical
repeats. Ordinary Zanzibar startup initially failed in the newer TU1.5 scene
factory at 10F94B calling 10F960. The original complete scenery-lightmap lookup
has since been added to the source factory and passes 512 independent
original-executable comparisons; see `SOURCE_SCENERY_LIGHTMAP_LOOKUP_TU15.md`.

The subsequent isolated runtime uses copied, pinned link inputs from a later
shared build. That version adds an original local input/camera constructor
1888A0 which is still unresolved. It stops before scene construction, so that
run does not establish that Zanzibar gets past 10F960 live. The current normal
launchers select the concurrently rebuilt root `build/halo2_native`; neither
the earlier accepted frozen snapshot nor the isolated scheduler checks prove
full acceptance of that changing runtime. The full normal-launcher bank is
not accepted and all-map completion remains unproven.

The complete all-map goal remains open: general attachment/resource/function
and marker owners, correct shared scheduling/RNG history, live controls and
machinery across all maps, full networking/late-join/reset behavior, both
Vista-only maps, and full live normal-launcher acceptance still need proof.
