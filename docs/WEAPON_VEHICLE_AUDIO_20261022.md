# Weapon and vehicle sound checkpoint — 2026-10-22

The missing local source-owned shot/reload route on Coagulation is corrected
and delivered through both launchers. Full weapon/vehicle TU1.5 audio parity
is **not certified**. The [100-root matrix](../analysis/weapon-vehicle-audio-20261022/completion-matrix.json)
keeps every original weapon and vehicle in scope; untested behavior remains open.
The [accepted checkpoint](../analysis/weapon-vehicle-audio-20261022/accepted-checkpoint.json)
links the bounded evidence.

## Correction and ownership

Source-owned matches use the original first-person clock and skip the legacy
native clock callback. Their actual original sound requests had no native
consumer. The new observer copies the original 160-byte `18CEFC -> 223B20`
caller request, executes the unchanged original admission call, and publishes
the caller intent afterward. Actual local weapon ownership selects the view;
the native stereo adapter consumes the caller's sound, control and gain.
No sound is inferred from ammunition, shot counters, animation names or render
frames, and no authority clock is advanced again.

The original ABI/result, including `NONE` at the current cold-driver gate,
remains unchanged. No source readiness, voice identity, capability or DSP result
is supplied. Controller/loop requests and forced selection keep their explicit
owner boundaries. Full original admission/arbitration and spatial/DSP output
remain separate work.

The native source-impulse adapter also honors MCC `snd!` bit11, explicitly
named `Play Only In Remastered Mode` in the pinned Assembly schema. It rejects
these additional layers before decoding, consuming variation or creating an
SDL voice. The mode regression fails before the correction and passes after;
caller bytes and RNG remain unchanged on rejection.

## Actual delivery evidence

Ten release/launcher/audio CTests pass with no skips. This includes 48 MCC
codec/rate/PCM controls, remastered-only rejection and the authored-audio suite.
The promotion policy also passes 13 synthetic policy tests; those establish
selection/cleanup behavior, not gameplay parity.

Actual SDL input and device submissions pass three stationary fire/reload
cycles in each of four cases:

| Map | Local players | Source shot cues / SDL submissions | Reload submissions |
|---|---:|---:|---:|
| Coagulation | 1 | 24 / 24 | 3 |
| Coagulation | 2 | 24 / 24 | 3 |
| Coagulation | 4 | 24 / 24 | 3 |
| Lockout | 1 | 24 / 24 | 3 |

The [four-case report](../analysis/weapon-vehicle-audio-20261022/session-v4/delivery/weapon-audio/report.json)
compares firing against actual authority effect requests and reloads against
authored clock references. It requires real voice handles/resources and no
duplicate firing voice. Walking acceptance is explicitly separate. The first
combined replay's zero-footstep failure remains retained; the unchanged default
runtime test still injects walking and requires footsteps.

The [direct installed-executable replay](../analysis/weapon-vehicle-audio-20261022/coagulation-installed-final.json)
repeats the original 260-tick Coagulation script through the shipped executable.
Its two Classic fire requests and two reload requests all reach actual SDL
submission; two remastered-only requests reach none. Classification uses actual
resident class4/class6 metadata. A diagnostic signed-register decoding error was
corrected by normalizing the captured unsigned 32-bit ABI values, with the raw
report retained and the postprocessing recorded; no runtime rerun or altered
request is claimed. Physical speakers remain unmeasured.

## Resource and namespace coverage

The pinned 38-map Xbox inventory contains 63 weapon paths and 37 vehicle paths
(1,664 map-specific definitions), with 422 sound paths and 68 looping-sound paths
in the explicit `weap`, `vehi`, `proj`, `hlmt`, `jmad`, `effe`, `lsnd`, `jpt!`
and `foot` reference closure. Implicit global/material/script dependencies remain
outside this traversal. Assembly commit is
`fd14ef3c46135e9422966c6a64cd926fb0bd23fb`.

The Xbox `vehi` override agrees on all 27 typed reference offsets/strides.
Its `jmad` override adds a compiled resource-directory edge: all 14,338 such
rows across the scope refer to their own graph, adding no sound dependency.

| Check | Coverage | Result |
|---|---|---|
| Xbox resource ranges | 347,922 nonempty descriptors / 73,842 distinct ranges | All raw and sector-rounded ranges present |
| Xbox literal loading and PC decoding | 38 maps / 39,884 encoded cases / 1,289 distinct payload-format combinations | Exact bytes; zero decode failures or silent resources |
| MCC Classic literal loading and PC decoding | 37 paired maps / 41,251 encoded cases / 1,351 distinct payload-format combinations | Exact bytes; zero decode failures or silent resources |

The [Xbox check](../analysis/weapon-vehicle-audio-20261022/weapon-vehicle-resource-decode-final/report.json)
compares 2,022,771,816 encoded bytes and decodes 106,396,121 stereo frames.
The [MCC check](../analysis/weapon-vehicle-audio-20261022/mcc-resource-decode-admitted/report.json)
compares 1,035,951,082 encoded bytes and decodes 112,365,022 stereo frames.
Expectations come independently from declared metadata and actual resource
files. Both large fixtures are removed. PC decoding does not establish Xbox
decoder output or equality between Xbox and MCC waveforms.

MCC's actual directory globals datum selects the sound-class owner. Shared
metadata is admitted only when the level has nonresident directory entries,
matching the native loader. The initial wider audit incorrectly merged unused
shared entries into self-contained campaign maps; its diagnostic remains
retained. The admitted-namespace rerun passes all 37 paired maps. The two older
whole-map CTest fixture files are absent and skipped; those skips are not used
as acceptance.

Eleven original exact sound/loop names differ in per-map MCC namespaces. Eight
exist in shared resource banks; the two Sentinel Enforcer names and Phantom
flyby lack exact matches there too. The [declared-field follow-up](../analysis/weapon-vehicle-audio-20261022/qualified-sound-bindings-admitted.json)
checks 419 references with no schema errors: 250 preserve the name and 169
differ or lack a paired owner/field. For example, Coagulation's shared Phantom
projectile selects `sound_remastered\vehicles\phantom\phantom_turr_flyby`.
Material rows are paired by ordinal only; material identity and final runtime
projection still need qualification. No substitute or audible equivalence is
inferred. Xbox `highplains2.map` has no same-named MCC file and remains unpaired.

## Delivered build and storage

Both launchers select
`bin/engine-builds/2bba11f861bf41328cde22faaf8a6ff1/halo2_native`.
Runtime SHA-256:
`4173a97cf66badbc27f0e464eb251e01cac9e74196f78041a520a0a92ec6a356`.
All 12 selected-map startup cases pass four actual submitted frames, including
Coagulation with two/four players. The promotion requires all four repeated
audio cases before changing the selection. The prior runtime is preserved.

The [source check](../analysis/weapon-vehicle-audio-20261022/delivered-audio-source-check.json)
binds the delivered audio files/fragments to the tested cohort. Later unrelated
shield/model/rendering edits remain in the working tree. Historical failed
builds and the interrupted v3 journal supply no additional acceptance.
Temporary builds, source copies, fixtures, PCM and frames belong to the owned
scratch lifecycle. Final cleanup and retained runtime size are recorded in
`analysis/weapon-vehicle-audio-20261022/final-cleanup.json`. The 2.5 GiB private
build/source tree is removed; the installed runtime/resources retain
300,771,138 bytes (about 287 MiB). No large test captures or fixtures remain.

## Native joined SMG follow-up — 2026-10-25

Prepared MCC Lockout now passes actual native local and authenticated localhost
joined SMG firing/reload delivery. The joined authority emits eight authored
firing cues and the client submits exactly eight SDL voices, with one reload
submission and no pre-join or unseen-tail cues counted. The local scenario also
matches eight requests to eight submissions. The recorded six-test cohort
passes the native effect-lifetime regression, local/joined announcer and damage
voice checks, and the rendered remote death scenario. See the
[bounded source and delivery receipt](../analysis/online-presentation-20261025/native-audio-delivery-endproof.json).

The missing publication was a real birth that retired before its factory
returned. Original `108A70` requested the authored SMG effect; `17AFB0`
allocated its full salted generation and colors, then the original initial
one-second update retired it and returned `NONE`. The old observer discarded
that birth. The native regression went from nine actual births/nine original
`NONE` returns/zero publications to nine/nine/nine.

The correction scopes capture to the original `1796E5` factory request and
retains the actual generation, colors and WORLD locations at `179063`, before
the first update. Original allocation, retirement, return value, stack and
native stores remain unchanged. The existing allocation/color publication
guards remain in force; no replacement effect, host audio voice or admission
result is supplied. A further private color snapshot protects the captured
parent generation from nested child stores. Its final four-test formal rerun
passes native source-execution replay, actual birth retention and both
local/joined announcer/audio scenarios, as recorded separately in the receipt.

Local and joined native Slayer startup each also reach one original journal,
one authored announcer cue and one SDL voice. The rendered joined fatal-hit
case submits one hurt packet and one lethal packet from the actual native
request journal, retaining authored chance and `NONE` suppression.

This closes the measured stationary single-SMG route on Lockout. Other remote
weapons, actions, maps, vehicle audio, spatial/DSP behavior, original waveform
equality and physical speaker output remain unverified. These results apply to
the recorded test cohort; they do not add an installed-runtime claim to the
earlier launcher checkpoint.

## Remaining acceptance

- [x] Inventory all 63 weapon and 37 vehicle reference paths.
- [x] Verify reference ranges and explicit-closure native loading/PC decoding.
- [x] Verify admitted MCC Classic resource loading/PC decoding.
- [x] Reproduce and correct local Coagulation shot/reload delivery.
- [x] Pass repeated one/two/four-player source-owned weapon replays.
- [x] Verify the selected installed executable and preserve prior runtimes.
- [ ] Exercise every applicable weapon action and vehicle engine/movement/boost/horn/seat/damage/destruction transition.
- [ ] Verify loop start/update/stop, interruption, respawn and remote/ONLINE ownership.
- [ ] Qualify changed sound bindings, original/MCC payload correspondence and final-DLC map pairing.
- [ ] Match TU1.5 timing, variation, gain, pitch, placement, arbitration and actual output, including spatial/DSP behavior.
