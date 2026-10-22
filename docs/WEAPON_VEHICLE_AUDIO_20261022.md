# Weapon and vehicle sound audit

The request covers every weapon and vehicle sound against original Xbox
TU1.5. Full audio parity is **not certified**. The reported missing shots and
reloads on Coagulation have a measured source-request delivery correction.
Repeated final Classic-mode acceptance and launcher delivery are pending.
The [100-root matrix](../analysis/weapon-vehicle-audio-20261022/completion-matrix.json)
keeps every weapon and vehicle in scope without certifying untested behavior.

## Reference inventory

The pinned 38-map Xbox scope contains 63 weapon paths and 37 vehicle paths
(1,664 map-specific definitions). Traversing declared audio references in
`weap`, `vehi`, `proj`, `hlmt`, `jmad`, `effe`, `lsnd`, `jpt!` and `foot`
finds 422 distinct sound paths and 68 looping-sound paths.

[The inventory](../analysis/weapon-vehicle-audio-20261022/authored-sound-presence-final.json)
records each owner, its sounds, loops and compared maps. It uses Assembly
commit `fd14ef3c46135e9422966c6a64cd926fb0bd23fb`. Animation event indices and
prefetch resource indices are not mistaken for full-salt tag references.
Implicit global/material/script dependencies remain outside this traversal.
The Xbox `vehi` override has the same 27 typed reference offsets/strides as
the generic schema. Its `jmad` override adds a compiled resource-directory
edge; all 14,338 such rows across the 38 maps refer to their own animation
graph, adding no sound dependency. The semantic sound-reference offsets agree.

Eleven reference sound/loop paths are absent from the corresponding MCC tag
namespaces. These include the Phantom turret flyby, Wraith mortar charge,
chain-gun fire, Hunter cannon, Sentinel Enforcer and sniper zoom paths listed
in the inventory. A missing name does not establish a missing audible asset:
its actual MCC reference and payload correspondence still need qualification.
The [shared-bank check](../analysis/weapon-vehicle-audio-20261022/shared-sound-namespace-check.json)
finds eight of these eleven paths in both MCC shared banks. That is asset
namespace evidence; each actual map reference still needs its owning binding.
Only the two Sentinel Enforcer paths and Phantom turret flyby lack exact
matches in those shared banks as well. The final Xbox `highplains2.map` has no same-named MCC file; its namespace was
left uncompared. No alias or replacement sound was invented.

The [declared-field follow-up](../analysis/weapon-vehicle-audio-20261022/qualified-sound-bindings-merged.json)
checks 419 references to those eleven names in 36 map contexts with no schema
errors. It uses the pinned `Halo2MCC` plugins and their explicitly declared
`Halo2` fallback, and the native loader's exact full-datum/group/path shared
merge policy. 253 stored fields preserve the name; 166 differ. For example,
Coagulation's shared Phantom projectile selects
`sound_remastered\vehicles\phantom\phantom_turr_flyby`; the Sentinel
projectile selects a remastered laser pass-by. Hunter charge, battle-rifle zoom
and chain-gun definitions also select differently named sounds. One Sentinel
material row has a null sound reference. These are stored-cache field findings
before runtime projection, with block rows paired by ordinal only. Material
identity, payload correspondence and actual runtime use still need qualification;
they are not proof of an audible missing sound or an authorized substitute.

The independent [resource-range audit](../analysis/weapon-vehicle-audio-20261022/original-resource-audit/report.json)
checks all 347,922 nonempty Xbox chunk descriptors across those 38 maps.
Every exact and sector-rounded file range exists, including 73,842 distinct
ranges. This establishes reference-resource availability, not decoding,
event selection, playback or waveform equality.

The [weapon/vehicle payload check](../analysis/weapon-vehicle-audio-20261022/weapon-vehicle-resource-decode/report.json)
then exports the declared dependency closure independently from the native
loader. All 38 maps, 63 weapon paths, 37 vehicle paths and 422 sound paths are
covered. The native loader matches **39,884** literal encoded cases
(2,022,771,816 compared bytes), and all **1,289** distinct payload/format
combinations decode to usable PC PCM: zero failures and zero silent resources,
106,396,121 stereo frames. Its large fixture is removed. This is offline
reference loading/PC decoding; it does not certify MCC payload correspondence,
original Xbox decoding or live sound selection. The two historical whole-map
CTest fixtures are absent and skipped; those skips supply no acceptance.

## Coagulation regression

The launcher-selected runtime was SHA-256
`7db35ababe9ba5e2b351ae5a9ccb96fb5800f66d9e2b3bc14b1dbc37470e7c79`.
The finite MCC Coagulation replay fired and reloaded through 260 ticks, ending
with 60 loaded rounds and 118 reserve rounds. Its first-person native audio
consumer received no events and neither observed impulse route received calls.
The [completed baseline](../analysis/weapon-vehicle-audio-20261022/coagulation-source-submits.json)
retains the command, binary/map hashes and actual observations. Its separate
startup-entry counter does not observe the map-support module's submit owner.
A later two-player trace with that additional breakpoint timed out and is not
positive runtime evidence.

Source-owned matches deliberately skip the legacy native animation clock and
its first-person callback. The candidate connects their actual original
`18CEFC -> 223B20` caller requests to native stereo playback. It copies the
160-byte caller backing before admission and publishes that intent only after
the unchanged original call returns. It preserves the original ABI/result,
including `NONE`; no readiness bit or source voice is supplied. Actual local
weapon ownership, centered audibility, caller control and gain select this
bounded native route. It does not infer events from ammo, shot counters,
render frames or animation names, or advance an authority clock again.

Original controller/loop and forced-selection requests retain explicit owner
boundaries. ONLINE continuation, remote delivery, voice interruption/stop
ownership and full original admission/arbitration/DSP remain separate work.

The regression uses actual SDL input and submissions, source effect-request
counts and authored reload references. It repeats three fire/reload cycles on
Coagulation with one, two and four local players and on Lockout with one player.
The raw-request component additionally compares real mixed PCM with the
existing semantic native request and checks that caller bytes remain unchanged.
Results belong in
`analysis/weapon-vehicle-audio-20261022/session-v4`.
The earlier v2 build failed and does not provide runtime acceptance. The v3
cohort has a coherent source manifest and passed syntax preflight for all six
affected implementation/test units before its official CMake build.

The first actual v3 Coagulation run delivers all **8/8** authority SMG shot
cues and **1** authored reload, with 8 ambient packets. Its combined assertion
fails on zero footsteps, which is retained as a separate result. The explicit
`--weapons` mode now repeats stationary fire/reload input; the unchanged default
continues to inject walking and require footstep delivery. No footstep fix or
acceptance is inferred from the weapon test.

The new source impulse route also rejects MCC `snd!` bit11, explicitly named
`Play Only In Remastered Mode` in the pinned schema, before decoding or RNG.
The negative test first attempts to load the absent remastered bank and fails;
with the guard, the 48 codec/rate/PCM controls, mode rejection and authored-audio
component tests pass. Extra remastered effect layers do not become Classic
weapon sounds. The source driver's original cold admission result remains
unchanged. Its loop/controller, interruption and DSP owners remain open.

The v3 owner was removed by the cleanup supervisor after a turn interruption.
Its final build-only journal entry is stale, with no positive result; the
interruption receipt records that cleanup. V4 builds the engine and audio tests
together and requires all four actual repeated weapon cases before promotion.

## Completion gates

- [x] Inventory the 63 weapon and 37 vehicle reference paths.
- [x] Verify all reference sound chunk ranges are present.
- [x] Verify native loading and PC decoding for the explicit audio closure.
- [x] Reproduce missing local Coagulation shot/reload delivery.
- [ ] Pass the candidate's component and repeated actual runtime regressions.
- [ ] Verify delivery through the selected launcher runtime.
- [ ] Qualify the map/shared bindings, three remaining exact-path differences
  and final-DLC map pairing.
- [ ] Exercise every applicable fire, reload, ready, dry-fire, charge, vent,
  engine, movement, boost, horn, seat, damage, destruction and loop transition.
- [ ] Verify interruption, release, respawn, split-screen and remote ownership.
- [ ] Compare timing, variation, gain, pitch, placement, routing and actual
  output against substantiated TU1.5 audio references.

Generated builds, source copies, PCM, frames and debugger files belong to
`tools/run_test.py` scratch. Keep bounded reports and hashes; completed scratch
must be checked for removal before accepting this checkpoint.
