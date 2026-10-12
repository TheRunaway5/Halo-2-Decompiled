# Original spatial sound CPU handoff

Status,2026-10-12: the original game and driver CPU spatial-update owners are
ported and validated against actual initialized Xbox game calls. The native
multiplayer adapter now retains world positions and applies authored distance
gain to actual SDL playback. Its channel routing is still fixed stereo;
Xbox HRTF/DSP, occlusion and complete source admission remain unfinished.
This does not complete the user's all-sounds goal.

## Original authority and captured calls

The owned base executable has SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
Complete instruction bodies, internal branches, literal operands and mapped
callees were paired with the native TU1.1 authority. Eight owners total721
instructions:

| Base disc | TU1.1 | Owner |
|---|---|---|
|129790|12AEE0|Game spatial voice update|
|12ABE0|12C330|Location position selection|
|142700|143E70|Inverse listener transform|
|21DFF0|220100|Shared voice/tick update|
|2197F0|21B900|Linear gain to decibels|
|221810|223920|Authored sound class resolution|
|220E00|222F10|Driver spatial update|
|2211E0|2232F0|Driver environmental parameters|

Pair evidence is in
`analysis/audio-spatial-20261011/paired-v1/report.json`. The port in
`src/sound_spatial.cpp` implements the complete12AEE0 game CPU owner and its
five CPU dependencies.222F10 remains a mandatory provider at the game seam;
its separate CPU implementation is now in`src/sound_driver_spatial.cpp`.
The reached SDK/platform methods remain mandatory providers.

Three actual game calls were captured from the original executable running in
xemu0.8.136. Their source/readiness/listener/pool/register/FPU inputs were already
initialized by the game. Hardware breakpoints observed the calls without
patching instructions or writing guest memory/registers. Independent Unicorn
execution fetched actual guest pages on demand and stopped at the unchanged
220E00 driver boundary or at the original caller return. The game then reached
the same boundary normally. Parameters and retained writes agreed exactly.

| Capture | Original Lockout sound | Observed behavior |
|---|---|---|
|live-v1|E7770602,`sound\ambience\lockout\lockout\front_speakers\loop`|Local audible type0; no spatial driver call; gain retained by its original attenuation branch.|
|live-v2|E7790604,`sound\ambience\lockout\lockout\details\lockout_monogust`|Spatial audible type1; source location type1 selects listener height; silent driver request; shared voice and orphan gate updated.|
|live-v3|E7AD0628,`sound\weapons\frag_grenade\frag_projectile\frag_proj\loop`|A normally thrown grenade's flight loop; nonzero world position transformed into the chosen listener's basis and a driver request published.|

The flight-loop capture is distinct from the earlier grenade-bounce admission
capture. Sound names above were read from the owned Lockout cache.

## Native boundary and regression checks

`include/halo2/sound_spatial.hpp` requires retained original state, raw caller
position backing, literal source operands and the original floating-point
context. Missing required memory or a reached driver owner throws at that
phase. No successful platform return, device readiness, listener or pool is
manufactured. An unsupported location type retains the original caller's
private vector bytes. The inverse transform preserves the original reciprocal
then multiply operations and the separately rounded SSE dot-product order.

The permanent actual-game fixture,
`tests/fixtures/live_sound_spatial.original.tsv`, compares:

- Three genuine calls and two exact36-byte driver requests.
-29 ordered semantic retained stores and307,200 complete captured page bytes.
- The private12-byte position vector, compared separately from native stack
  layout. Original dead stack/consumed argument scratch is excluded from the
  semantic native ABI; the actual original boundary comparison retains it.
- Two mandatory-driver stops, after the actual CPU writes, when that provider
  is unavailable.

Separately,133 declared raw-ABI numerical and branch cases run through unchanged
original instructions. These vary explicitly listed audible/location types,
flags, shared tick state, scale, attenuation, class controls and elapsed time,
including signed zero, subnormals, infinities and NaNs. They preserve captured
readiness and activation roots. They are not133 additional genuine game events.
Their inputs and reference output are in
`analysis/audio-spatial-20261011/declared-v2`.

Normal and ASan/UBSan independent builds pass both fixture sets. An isolated
diagnostic copy with the inverse transform removed fails the genuine grenade
flight-loop request. Production source was never reverted or mutated for that
red control. Reproduce with:

```sh
python3 tools/validate_live_sound_spatial.py \
  --actual tests/fixtures/live_sound_spatial.original.tsv \
  --declared analysis/audio-spatial-20261011/declared-v2/fixtures.tsv \
  --output <new-output-directory>
```

The new frozen source snapshot also builds the full cache library and passes
the CMake-integrated`original_live_sound_spatial` test. Existing stable audio
snapshot sources were copied, then only the spatial source/header, fixture,
test and seven-line CMake additions were overlaid. Concurrent checkout changes
and the previous frozen build were preserved.

## Actual driver and SDK observations

An additional genuine grenade flight-loop call was observed at220E00. The
original function and every SDK callee executed normally, returning to its
actual caller with the original stack cleanup. Hardware observers recorded the
original SetPosition, SetI3DL2Source and SetFilter inputs. No callee return was
supplied. Evidence is in
`analysis/audio-spatial-20261011/driver-live-v4/report.json`. This selected
capture also observes the original filter controls at entry and verifies them
again at the normal return.

The source publishes game coordinates in`x,y,z` order. Its driver passes
`x,z,y,DS3D_DEFERRED=1` to the SDK. That permutation is verified both by the
owned instructions and the actual call's raw words. The environment and filter
payloads are also retained byte for byte. These are real SDK handoff
observations; no native mixer, DSP response or physical speaker parity is
claimed.

A subsequent read-only inspection verified all332 original driver/environment
instructions against the guest's actual code bytes. Its initialized SDK
function-pointer table exactly matches the owned LightHRTF4Channel selector,
including the horizontal-position, angle, front/rear-volume and HRTF-kernel
owners. This selected mode is evidence from the running game, not a proposed
host panning policy. The inspection is retained in
`analysis/audio-spatial-20261011/sdk-current-original.json`;24 nonempty original
DSOUND bodies,1,593 instructions, are under`audio-spatial-20261011/sdk-base-v1`.
Earlier empty SDK extraction attempts used the wrong executable section and
are excluded from selected evidence.

The first driver attempt ran into the original game's postgame screen. Loading
the saved reference match interrupted that debugger attempt; its failed log is
retained. The next diagnostic observer received a repeated hardware notification
at one SDK entry. The final observer disables each source call-site observation
after its first hit, so repeated notifications are not counted as additional
executed SDK calls. Only the final successful capture is selected as evidence.

## Remaining work

Finish live retained SDK calculation/application integration, original selected
HRTF/mix-bin/filter/DSP behavior and an explicitly named native backend. World
positions and original distance attenuation now reach actual SDL voices, as
recorded in the 2026-10-12 live correction below. Complete selected-listener
and original local-only routing ownership remains unfinished. Preserve event timing, shared-voice/lifetime gates,
environment smoothing, packet scheduling and class/variation RNG ownership.
The grenade flight loop also requires a live publication/lifetime audit, not
just positional impulse playback. Detail/weather ambience and other unported
event owners remain open. Both launchers now deliver the bounded tested native
corrections; full local/split/LAN audible Xbox comparison remains required
before completing the all-sounds goal. See
[the installed delivery report](MP_AUDIO_DELIVERY_20261012.md).

## Driver environment and filter CPU checkpoint

`src/sound_driver_spatial.cpp` implements the complete222F10/2232F0 owners,
the220380/2203E0 frequency/gain helpers and the required original32909C
integer conversion. The added helper pair evidence covers119 instructions,
including normalization and integer conversion. Original literal operands,
the actual x87 control word and MXCSR are explicit inputs. The implementation
preserves the original coordinate permutation, position/environment thresholds,
retained flag/store order, obstruction/occlusion smoothing, I3DL2 payloads and
filter coefficient conversion. The provider cannot fabricate SDK completion:
if a reached method is absent, the CPU owner stops there.

The permanent`tests/fixtures/sound_driver_spatial.original.tsv` contains one
genuine initialized game call,186 separately declared original CPU boundary
cases and8,992 complete original filter helper calls. The native regression
matches133 SDK payloads,413 ordered prefix stores and the complete retained
driver bank. Declared cases stop before the first reached SDK method; no
original SDK return, kernel readiness or extra game event is supplied.

For numerical reference cases, the owned executable's unchanged32-bit
instructions execute on the host CPU in a private compatibility-mode process.
Hardware breakpoints stop before SDK calls. No original opcode is patched.
This avoids a verified Unicorn x87 reference defect: seven declared invalid-log
inputs differ between Unicorn and actual original instruction execution. Native
expectations were not used to generate either reference. The selected original
CPU result for a negative direct-volume input is0; Unicorn incorrectly gives
-2000. See`unicorn-host-reference-differences.json` and the selected
`driver-host-oracle-v2/report.json`. Host CPU execution remains distinct from
physical Xbox hardware and the genuine initialized Xbox-in-xemu capture.

Independent normal and ASan/UBSan builds pass. An isolated wrong-axis control
fails the genuine game SDK payload. An isolated signed32 conversion substitute
fails the original filter gain on a NaN input. Production source was not changed
for those deliberate failures. Reproduce with:

```sh
python3 tools/validate_sound_driver_spatial.py \
  --fixture tests/fixtures/sound_driver_spatial.original.tsv \
  --output <new-output-directory>
```

This adds source and CPU/SDK handoff evidence. The module has not yet been wired
into the live multiplayer mixer; it does not prove spatial output, the selected
HRTF response, environmental DSP, audible timing or physical speaker parity.

## SDK numerical implementation and immutable-data checkpoint

Five additional genuine initialized SDK calls were observed with hardware
entry/return breakpoints. Each helper and every reached callee returned normally;
no guest opcode, memory, register or callee return was changed:

| Original helper | Captured output |
|---|---|
|37DEBF position, zero input|Zero horizontal vector and distance.|
|37DEBF position, nonzero input|Source words`3ea917ed,bcfe507a,3d67ae7c` become horizontal unit vector`3f7c5379,00000000,3e2cdc65` and distance`3eab8e36`.|
|37DEF9 angles|The captured zero-distance call publishes zero azimuth/elevation.|
|37DFE1 front/rear volume|The actual disabled-rear call publishes0 and-10000 hundredths of a decibel.|
|37E098 kernel selection|Actual azimuth word`c0f0e308`, fold0, selects original left/right identities`003868b8,00386898`; both32-byte filters match the owned executable.|

These calls are retained in`analysis/audio-spatial-20261011/sdk-live-*`.
They are independent observations at different times, not a fabricated single
composed game event. All five replay exactly through unchanged original
instructions on the host32-bit CPU.17,924 additional declared ABI cases cover
position, angles, front/rear volume and kernel-address selection, including
rotated bases, varied volume products, nonfinite/subnormal words and explicit
floating-point contexts. The prior smaller15,592-case original repeat remains
retained separately; it is not a repeat claim for this expanded set.

`src/sound_sdk_spatial.cpp` now implements all four numerical owners and their
complete normalization/volume-conversion dependencies. It preserves original
x87/SSE ordering, float spills, table arithmetic and the caller's floating-point
environment. The original SDK volume conversion uses1000log10 and signed32 SSE
truncation; the game-side2000 scale and x87 low-word conversion do not apply.
Kernel outputs are original32-bit identities, never cast to host pointers.

The permanent`tests/fixtures/sound_sdk_spatial.original.json` comes solely from
the selected original instruction runs in`sdk-host-oracle-v4`. Independent
normal and ASan/UBSan builds pass all17,929 outputs. Four isolated diagnostic
copies deliberately break projection, angle scale, volume scale or kernel
quantization; all fail. The projection and kernel failures now occur on genuine
initialized captures. Production files are never mutated for these controls.
See`sdk-validation-v3/report.json`, or reproduce with:

```sh
python3 tools/validate_sound_sdk_spatial.py \
  --fixture tests/fixtures/sound_sdk_spatial.original.json \
  --output <new-output-directory>
```

`tools/research_sound_sdk_images.py` pairs20 complete DSOUND bodies across the
base disc, TU1.1 and TU1.5. Complete normalized instruction/branch topology,
raw encoded consumers and direct callee mappings are checked. Identical emitted
SDK helpers are disambiguated by a separately paired complete caller, not by
assuming a relocation offset. All10 scalar words, two mode1 basis vectors,
61 complete stereo HRTF filters and the null filter agree byte for byte in all
three owned images. Selected proof is`sdk-image-pairing-v5/report.json`.

`OriginalData::load_xbe` imports those per-image operands and full filters.
`SoundSdkSpatialData::kernel` resolves only the retained aligned table identities
and the null filter. It rejects unavailable identities instead of manufacturing
filter bytes. The new permanent data fixture and regression check all three
supported images, every imported byte and the resolver bounds.

## Genuine original HRTF application capture

The earlier idle kernel observer did not reach its owner; its failed log remains
excluded from passing evidence. A fresh initialized reference session reached
the selector normally after ordinary X11/controller input. A separate complete
3812E0 application was then observed with hardware execution breakpoints. The
original kernel lock/release owners, FIFO polling and every device write ran
normally. No guest instruction, memory, register, kernel return or device result
was supplied. The helper returned to its original caller with normal stack
cleanup. This is one separate call, with zero azimuth; it is not combined with
the earlier nonzero selector capture or assigned an unproven sound-event name.

`hrtf-live-v1/report.json` retains the actual voice/source/parameter backing,
selected original filters, voice bank toggle and all19 pending CPU write
addresses/values: HRTF entry selection,15 coefficient-pair writes, final
coefficient/delay write, voice selection and entry commit. Each pending write
was followed by normal execution through the owner's return. The driver packs
two coefficient bytes per ear into each of15 words, then the remaining
coefficient byte per ear and a selected final parameter byte into the16th word.
That exact packing is established here; physical DSP interpretation and
parameter transitions remain outside this capture. The capture tool is
`tools/capture_live_sound_hrtf.py`.

These CPU/data/device-command observations do not establish the physical MCPX
DSP's parameter transitions or waveform. Retained SDK state owners, native
HRTF/mix-bin/filter/DSP application, event timing, packet scheduling and live
local/split/LAN spatial delivery remain required. The live multiplayer adapter
still uses its fixed stereo matrix and drops emitter positions at its call sites.

## SDK HRTF CPU application checkpoint

`source_sound_sdk_hrtf_apply` now implements the complete3812E0 CPU branch,
store and command-packing order. It retains source/parameter override fallback,
mode2/null-filter behavior, speaker-setting fold selection, original floating
loads/spills, voice bank toggle,16-bit entry arithmetic, FIFO threshold and all
19 device writes. The final parameter selection preserves the original x87
negative/unordered test and wrapped shift arithmetic.

`SoundSdkHrtfServices` requires actual retained memory, device locking, current
algorithm dispatch, original filter bytes, FIFO availability and a device/DSP
command provider. Missing reached services throw at their boundary. A device
lock provider that skips or repeats the body also fails. This CPU owner creates
no platform readiness, kernel return, DSP result or replacement selected mode.
The live native backend for these services remains required.

`tools/oracle_sound_sdk_hrtf.py` executes the complete original LightHRTF
initializer, then measures explicit original CPU fragments: after the kernel
lock through the first FIFO read, and each packing block through its pending
device-write boundary. All original helper/callee instructions inside those
fragments run unchanged. Declared ABI scratch and the mutable speaker-setting
word are supplied as numerical inputs; no original lock/device result or callee
return is supplied. The actual captured19 command words and retained voice byte
agree first, followed by1,135 separately declared cases. They cover folded
angles, source/parameter overrides and absence, source modes, nonfinite words
on the null-filter branch, full bank bytes and16-bit handle wrapping. Independent
original repeats agree for all1,136 cases in`hrtf-host-repeat.json`.

The permanent`tests/fixtures/sound_sdk_hrtf.original.json` contains those
original expectations. The native regression compares every command word and
the complete retained voice bytes; six missing-service cases stop explicitly.
Its lock/FIFO providers are declared test models. Independent normal and
ASan/UBSan builds pass. Isolated wrong bank-toggle, coefficient packing and final
parameter-shift implementations all fail; the bank error fails the genuine
command capture. Production source is not mutated for these controls. See
`hrtf-validation-v1/report.json`, or reproduce with:

```sh
python3 tools/validate_sound_sdk_hrtf.py --output <new-output-directory>
```

The existing17,929-case SDK numerical regression and all four isolated red
controls also pass again after this addition (`sdk-validation-v4`). These are
CPU/data/command checks. Retained SDK calculation owners, a real native
device/DSP provider, exact waveform/transition behavior, source scheduling and
actual local/split/LAN audible delivery remain unfinished.

The new frozen`hrtf-source-snapshot-v1` builds the complete cache library and
all seven selected CMake audio tests pass (`hrtf-native-ctest-v1.log`). This
snapshot overlays only the HRTF module/header, fixture, test and registration
onto the successful SDK/importer snapshot. It preserves concurrent checkout
changes separately. The installed launchers still select the concurrently
maintained performance runtime; this audio goal has not validated or replaced
that live executable. Selected source, original evidence and logs are pinned
in`hrtf-checkpoint-final/manifest.json`.

## Retained SDK calculation checkpoint

`source_sound_sdk_calculate` implements the complete37CA0A CPU update order:
listener basis recalculation; dirty/valid dependency gates; source modes;
position, distance, angle, cone and velocity dispatch; exact comparisons;
ordered retained stores; and final listener/source flag masks. Its mandatory
services own actual retained memory, state synchronization and the currently
initialized SDK algorithm. Reached missing services, skipped lock bodies and
repeated lock bodies stop explicitly. No selected mode, kernel success or
numerical output is supplied by this caller. The native live owners remain
unintegrated.

The additional37DB87 cross product,37DBC4/37DAE0 cone calculation and37DC5A
relative-velocity calculation preserve original x87 instruction order, float
spills, mode1 behavior, unordered comparisons and caller floating-point
context restoration. The cone's authored scale is4.0, as read from the
original45DC10 operand. This immutable word and its encoded consumers are
paired across all three owned executable images; the native importer verifies
it alongside the existing SDK data. Static pairing now covers25 complete
bodies and16 immutable ranges. It does not establish title-update execution.

`tools/capture_live_sound_sdk_retained.py` observes complete original entry
and normal return, including real kernel lock owners. `sdk-live-calculate-v1`
is a genuine listener-only no-op. `sdk-live-calculate-v2` is a separate genuine
source-position update. Both compare complete original instruction bytes and
selected algorithm table, and retain their actual raw inputs/outputs. No guest
instruction, memory, register or callee return was supplied. These calls are
not assigned an unproven sound-event identity or composed with earlier calls.

`tools/oracle_sound_sdk_calculate.py` executes the unchanged original LightHRTF
initializer, then37CA1F..37CC39 between the original kernel-lock boundaries,
including every reached numerical callee. Both genuine final states agree
first. The separate declared controls cover each dirty/valid bit; source modes;
finite, nonfinite and signed-zero state; rotated bases; absent sources; and
five floating-point contexts. There are1,072 declared states,11,519 ordered
retained stores and1,445 independently measured complete helper calls. The
original CPU replay repeats byte for byte in`calculate-host-repeat.json`.
Declared ABI backing is not extra game-event or kernel-readiness evidence.

The permanent`tests/fixtures/sound_sdk_calculate.original.json` contains only
those original measurements. The native regression compares complete retained
buffers, every ordered store and every helper output. It verifies floating-point
context restoration on success and service errors, six missing-service stops
and two invalid lock-provider stops. Native test lock and LightHRTF dispatch
services are explicitly declared models. Normal and ASan/UBSan builds pass.
Four isolated incorrect source copies fail: position dependency gate, basis
orientation, cone scale and mode1 velocity. The position and cone errors fail
the genuine changed-source capture. Production source is never mutated by
these controls. Reproduce with:

```sh
python3 tools/validate_sound_sdk_calculate.py --output <new-output-directory>
```

Selected results are in`calculate-validation-v2/report.json`. The previous
SDK numerical and HRTF command suites also pass again with their seven red
controls (`sdk-validation-v5`, `hrtf-validation-v2`). The coherent
`calculate-source-snapshot-v1` builds the full cache library and passes all
eight selected integrated audio checks (`calculate-native-ctest-v1.log`).
It overlays only audio files and two narrow SDK-import/test-registration
blocks onto the prior successful snapshot. An initial build command used an
incorrect target name and did not build; that unsuccessful log is retained.

A genuine37AE57 voice-data capture also returns normally with unchanged
retained output (`sdk-live-voice-data-v1`). A later conditional observer never
selected a dirty-source entry before the private reference session was
intentionally closed; its failed connection log is not passing evidence or a
claim that the event is unreachable. Complete static dependencies are retained
for the next voice-parameter/environment owner, which remains unported here.

Live retained SDK ownership, voice-parameter/environment updates, real native
DSP/device application, source scheduling, exact waveforms and actual
local/split/LAN audible delivery remain unfinished. The live multiplayer
adapter still discards emitter positions and uses its fixed stereo matrix.
Both launcher inspections now select the concurrently maintained
`.local/frontend-combined-delivery/20261011T235850Z/halo2_native`, whose bytes
are pinned in this checkpoint. This goal has not validated or replaced that
live executable. Selected source and evidence are retained in
`calculate-checkpoint-final/manifest.json`.

## Retained voice-data original reference

`tools/oracle_sound_sdk_voice_data.py` now measures the next37AE57 voice-data
owner. The genuine whole-call capture is a no-op and agrees first. On the
host32-bit CPU, the unchanged original LightHRTF initializer establishes
the dispatch table;37AE68..37B129 then runs between the original kernel-lock
boundaries, including every reached numerical callee. No instruction patch,
kernel/device result or callee return is supplied.

The initial reference had1,490 separately declared states covering flag bits, source modes and
feature bits, finite/nonfinite input words, previous output buffers, optional
environment absence, explicit curve backing, varied factors and five
floating-point contexts. These are declared numerical inputs, not additional
initialized gameplay calls. Its1,666 complete helper calls cover distance and
curve interpolation, cone gain, Doppler, rear widening, environmental voice
parameters and gain/volume conversions. The original unsigned curve-index
converter in`.text` executes too. Its reported2,624 ordered voice writes missed
azimuth FSTP stores because of Capstone operand-access metadata. Final state
and helper values remain valid; the expanded corrected write-order evidence
below supersedes that count. Every executed source instruction is checked
against the owned image.

An independent repeat is byte identical for every state and helper output
(`voice-data-host-repeat.json`). The original oracle/tool/runner are frozen in
`voice-data-host-oracle-v1` and`voice-data-host-repeat-v1`. The static three-image
pairing now includes37 complete bodies and28 full immutable ranges
(`sdk-image-pairing-v8`). The`.text` converter's target comes from each separately
paired complete distance caller; it is not selected by an assumed displacement
or duplicate helper signature. The first16 data ranges remain byte identical
to the validated importer fixture. Extra ranges are reference evidence for the
next import/implementation; they are not yet imported into a native voice owner.

The initial original evidence is pinned in
`voice-data-reference-v1/manifest.json`. This is an original-reference
checkpoint only. It is superseded by the expanded original measurements and
native CPU validation below. Actual backend synchronization/dispatch,
physical DSP/waveforms and audible local/split/LAN placement remain open.

## Retained voice-data CPU checkpoint

`source_sound_sdk_voice_data` implements the complete37AE57 CPU branch and
store order. It combines listener/source dirty flags; honors mode2; updates
distance, cone gain, front/rear, rear widening, Doppler, azimuth/elevation and
optional environmental parameters; and preserves original dirty-flag widths
and publication order. The original azimuth write precedes its flag update.
The environmental filter flag is a byte write. Existing output flags persist.
Actual retained memory, state synchronization and the currently initialized
algorithm dispatch remain required services. Missing reached services and
skipped/repeated lock bodies stop; the caller supplies no selected-mode default
or invented platform result.

The numerical implementation covers37DCA5 distance/curve interpolation,
37DDB6 cone gain,37DE46 Doppler,37E112 rear widening and38368B environmental
parameters, including complete322B0C,37DB07,379D54,3801DF,383167 and3834F3
conversion/logarithm/filter dependencies. It preserves unsigned integer
conversion, x87 operation order and spills, separate gain/volume scales, SSE
truncation, x87 pitch rounding, integer wrapping and filter packing. The
original LightHRTF rear-widening method explicitly writes0/-10000. Curve
identities resolve through a required retained-word provider rather than a
native pointer cast. All additional scalar/double operands import exactly from
the three owned executable images; the data regression now covers all28 ranges.

The expanded original oracle (`voice-data-host-oracle-v3`) contains one genuine
whole-call no-op,2,825 separately declared original states and3,555 complete
helper calls. Additional controls cover all interior curve intervals and their
boundaries, unsigned cone widths, both Doppler saturation boundaries, meaningful
negative environment levels/mixtures/frequencies and five floating-point
contexts. These are numerical/ABI inputs, not more initialized gameplay calls.
The original LightHRTF initializer executes unchanged; all reached numerical
callees and all873 observed instructions are checked against the owned image.
Independent original replay is byte identical for all output buffers, helpers
and8,757 ordered voice stores (`voice-data-host-repeat-v2.json`).

The initial whole-state regression exposed a reference-observer defect:
Capstone reports FSTP memory operands as reads. `OriginalAudioCpu.run_traced`
now recognizes the original x87 store opcodes before single-stepping them.
No original instruction or expected numerical value was changed. The corrected
oracle supersedes the earlier voice-data store counts. Both other users of the
observer, the driver and retained calculation oracles, were re-executed; their
permanent fixtures remain byte identical (`x87-store-observer-audit.json`).

The permanent`tests/fixtures/sound_sdk_voice_data.original.json` contains the
corrected original expectations. The native regression checks complete retained
buffers, every ordered write and all helper outputs, plus floating-point
context restoration and ten missing-service/invalid-lock controls. Its lock
and current-algorithm providers are explicitly declared native test models.
Normal and ASan/UBSan builds pass. Six isolated incorrect source copies fail:
curve indexing, unsigned cone width, Doppler rounding, filter scaling, byte-flag
width and azimuth-store order. Production source is never mutated. Reproduce:

```sh
python3 tools/validate_sound_sdk_voice_data.py --output <new-output-directory>
```

Results are pinned in`voice-data-validation-v1/report.json`. The coherent
`voice-data-source-snapshot-v1` builds the complete cache library and passes all
nine selected integrated audio checks (`voice-data-native-ctest-v1.log`). It
overlays only audio source/test files and narrow SDK-import/registration blocks
onto the previously validated calculation snapshot. Concurrent unrelated work
remains separate. Original evidence, selected source and build logs are retained
in`voice-data-checkpoint-final/manifest.json`.

The live adapter still drops emitter positions and supplies its fixed stereo
matrix. Real retained SDK/device ownership, waveform/transition behavior,
source scheduling and actual local/split/LAN audible placement remain
unfinished. The next gain/pitch/filter CPU stages are implemented below; they
do not yet install a native spatial playback route.

## Gain, pitch and filter device-command CPU checkpoint

`sound_sdk_device.cpp` implements complete381CE4 gain,381D7C pitch and381710
filter CPU owners, with complete380B97 gain packing and380DEB pitch packing.
Gain preserves signed volume clamping after unsigned wrapping, per-voice
routing, logical shift/unsigned division and twelve-bit/nibble packing.
Pitch preserves parent parameters, signed limits and intrusive child-list
traversal. Filter preserves override/retained selection, masks, the optional
environmental intermediate store and final publication order. Command loops
reread the original retained counts and values at their original boundaries.
The gain helper stops if zero would divide by zero or a count exceeds the
caller's nine-word scratch extent; no unproven extra backing is supplied.

Actual retained memory, device synchronization, FIFO readiness and device
command application remain mandatory reached providers. The synchronization
provider must execute each body once and support original nested pitch calls.
No lock result, ready flag, native pointer cast or DSP completion is invented.
All five numerical/control owners use portable fixed-width integer operations.

Three separate genuine original calls execute through their original lock
owners, FIFO reads and device writes to normal returns in xemu. Captures retain
voice/data/parameter backing, original helper outputs and ordered device words:
six gain writes, two pitch writes and six filter writes. They are separate
calls, not a fabricated single gameplay event. Whole routines and reached
helpers/lock owners also pair across all three owned images
(`sdk-image-pairing-v9`:44 complete bodies and28 unchanged immutable ranges).
These are Xbox-in-emulator command observations, not physical DSP/audio proof.

`device-host-oracle-v3` compares those actual calls first, then measures2,526
declared ABI controls,9,226 command words and1,483 ordered retained stores.
Complete helpers execute unchanged on the host's32-bit CPU. Application
fragments run after locking, before FIFO access and up to pending MMIO writes;
no device or kernel return is supplied. Six recursive pitch cases use original
caller fragments before/after each child call and separately measured child
CPU bodies. Declared changes between commands verify count/filter reloads;
these are not claimed real backend concurrency events. The permanent fixture
is `tests/fixtures/sound_sdk_device.original.json` with its provenance sibling.

An isolated wrong-routing control exposed a fixture coverage gap: the captured
ordinary calls have no retained spatial-parameter object. Earlier declared
controls allocated parameter bytes but retained that null pointer, so the
original skipped the intended branch. The selected v3 generator supplies
explicitly backed parameter identities and admits all eight routing slots.
Genuine captured inputs and original instructions remain unchanged. Earlier
v1/v2 declared spatial-coverage claims are superseded; their logs are retained.

The coherent `device-source-snapshot-v1` builds the complete cache library and
passes all ten selected integrated audio checks (`device-native-ctest-v1.log`).
It verifies the preceding frozen snapshot's2,178 pins, then adds only these
audio files and narrow CMake registrations. Concurrent unrelated source and
launcher work remains intact. Independent original replay is byte identical
(`device-host-repeat-v2.json`). Normal and ASan/UBSan checks pass. All eight
isolated wrong implementations fail: gain units, routing, nibble packing,
parent pitch, pitch limit, filter mask, intermediate filter store and count
reload. Twenty missing-service/invalid-lock/count controls also stop. Native
test providers model lock/FIFO/commands explicitly; no test claims DSP output.
Reproduce the focused validation with:

```sh
python3 tools/validate_sound_sdk_device.py --output <new-output-directory>
```

The selected result is `device-validation-v3/report.json`. Earlier failed
validation attempts are retained separately, including the initial FIFO test
model that assumed all recursive children needed equal credits. Original
expectations did not change to accommodate that model. The complete evidence
and source checkpoint is `device-checkpoint-final/manifest.json`.

The381E2A dispatcher and382032 update/commit owners are implemented in the
following checkpoint. Live SDK allocation/state ownership, actual device/DSP
processing, waveform/transition behavior, event scheduling and all-sounds
audible validation remain open.

## Combined spatial commit CPU checkpoint

`sound_sdk_commit.cpp` implements the complete381E2A dirty-parameter dispatcher
and382032 retained update. The dispatcher rereads parameter identity before
each dirty test, calls gain, pitch, HRTF and filter in original order, then
clears the entire retained dirty DWORD. The update preserves the active gate,
state-lock extent, calculation/voice-data argument reloads, source and source
environment dirty clears, then dispatch. Missing reached owners and skipped
or repeated lock bodies stop explicitly.

`source_sound_sdk_spatial_commit` composes the actual native calculation,
voice-data, gain/pitch/HRTF/filter implementations through one retained-memory
owner and shared state/device services. Numerical callees are not replaced
with expected answers. The original nested state-lock extent is preserved.
The HRTF bank-byte publication now participates in the optional ordered-store
observer; its application behavior is unchanged. Actual state/device locking,
FIFO readiness, current algorithm selection, kernel bytes and command
application remain required integration owners.

Two genuine original382032 commits execute through their original numerical
callees, kernel lock owners, FIFO reads and pending device writes to normal
returns in xemu. `sdk-live-commit-v2` is an initialized no-op. A normal grenade
input produces the changed `sdk-live-commit-dirty-v1` capture: calculate,
voice-data, dispatch, gain, HRTF and filter, with31 actual device writes
(6 gain,19 HRTF,6 filter). ItsD0 parameter flags do not request pitch; pitch
dispatch coverage is separately declared. All retained buffers, three caller
dirty clears and selected44-byte algorithm table are captured. Guest
registers, memory, instructions and service returns are never replaced.
The failed first capture is retained separately: the tracer attempted a
zero-byte memory read for the dispatcher's empty argument list. Its corrected
observer handles zero arguments without changing guest behavior.

`commit-host-oracle-v4` first matches both whole captures, then measures817
declared ABI cases. Of819 total cases,266 execute the complete CPU pipeline,
293 isolate dispatcher contracts and260 isolate update contracts. All8,871
command words and7,196 ordered retained stores agree. Complete numerical
callees execute unchanged original32-bit instructions;1,723 distinct executed
instructions are checked against the owned image and retained as a listing.
The original Light4 initializer executes and its selected table matches both
genuine captures. Independent `commit-host-repeat-v1` reproduces both fixture
and instruction listing byte for byte. The permanent fixture is
`tests/fixtures/sound_sdk_commit.original.json` with its provenance sibling.

The host replay preserves CPU/FPU state between explicitly selected fragments,
stopping at actual kernel-call, FIFO-read and pending-MMIO boundaries. It does
not execute kernel synchronization or device writes, supply their results, or
patch original instructions. Caller-only controls record actual argument lists
and apply explicitly declared retained mutations around callee boundaries;
those controls do not establish numerical callee behavior. Earlier failed
host attempts that advanced into pending MMIO are retained separately; the
selected runner handles immediate boundaries before resuming. Original code
and expected numerical results were not changed to accommodate the runner.

Native tests compare complete buffers, ordered stores and commands, and check
floating-point context restoration. They explicitly model state/device locks,
FIFO readiness and command collection.28 missing-owner/invalid-lock controls
stop. Normal and ASan/UBSan checks pass. Seven isolated incorrect source copies
fail on their intended cases: gain mask, dispatch order, dirty-store width,
active gate, source reload, parameter reload and source-environment address.
Production source is never mutated. Reproduce:

```sh
python3 tools/validate_sound_sdk_commit.py --output <new-output-directory>
```

The selected validation is `commit-validation-v2/report.json`. The first
validation attempt already caught the parameter-reload error, but reported it
before naming the failing case; the selected test improves that diagnostic.
The coherent `commit-source-snapshot-v1` verifies the preceding2,409 build pins,
overlays only audio source/test files and narrow CMake additions, then builds
the complete cache library. All11 selected integrated checks pass with the
final test binary (`commit-native-ctest-current-v1.log`). The complete selected
evidence and source checkpoint is `commit-checkpoint-final/manifest.json`.

Static extraction of the three genuinely observed SDK setter wrappers and
their immediate callees is retained in `sdk-setters-base-v1`; deeper setter
and retained-construction behavior is not inferred from those listings. The
live multiplayer adapter still discards emitter positions and supplies a
fixed stereo matrix. Retained SDK construction, setters and device/output
integration must reach real gameplay before placement, transitions, scheduling
or all-sounds audible parity can be verified. The inspected launchers select
`build-armor-emblem-20261011/halo2_native`
(SHA-256`b9732c15b2e42272838ff79618fbbd08364902da272c43f96f7b00654aa44c90`).
This goal has not replaced or audio-validated that concurrently delivered build.

## SDK setters and driver operation bridge checkpoint

`sound_sdk_setters.cpp` implements the complete public wrappers37C668,
37C6E5 and37B68B, synchronized callers37C3BF/37C4D2/37B1E6, retained
setters37C031/37C0E9 and immediate bridges37A669/37A616. Original interface
identities resolve object+1C through bounded retained memory. Position preserves
both original x87 argument spills, each destination-pointer reload and the
one-byte dirty OR. Environment preserves sequential, potentially overlapping
payload reads, each destination reload, nine words and the one-byte7F dirty OR.
Apply bit0 defers position/environment; other bits do not defer. Immediate
updates reach the required commit CPU owner. Filter reaches the required
filter CPU owner immediately.

Platform state access must supply the actual379E9E acquisition decision and
release a real acquisition once. The synchronized busy gate preserves80004005
without touching the source or device. Native exceptions release acquired
access. These providers are not replaced with chosen kernel success values.
The API requires both access providers before acquisition so an unavailable
release cannot strand a real native lock.

`source_sound_sdk_driver_operation` connects the actual driver operation
contract to these setters. Position already carries the original x,z,y words;
the bridge preserves that order. Environment/filter publication supplies a
bounded32-bit payload identity valid for exactly one callback. No host pointer
is serialized. Missing, skipped, repeated or unbacked payload ownership stops.
Initialized SDK object/state ownership and reached commit/filter services
remain mandatory. This bridge is not installed in the gameplay adapter yet.

Three genuine public calls from the original game execute through their
wrappers and state locks to normal returns. Position and environment have
apply1 and remain deferred; the genuine filter call executes six actual device
writes. Capture directories are `sdk-live-setter-position-v1`,
`sdk-live-setter-environment-v1` and `sdk-live-setter-filter-v1`. The original
acquisition flag is1 in all three. Guest instructions, memory, registers,
kernel/FIFO outcomes and callee returns are never replaced in these captures.

`setters-host-oracle-v5` first matches all three whole retained-state/command
captures, then measures154 declared CPU cases. Original public wrappers run to
their synchronized caller. That caller's CPU fragments start at the observed
post-access boundary using the captured acquisition flag. Declared access0
controls separately execute379E9E's actual nonzero-IRQL-byte branch after its
platform FS input boundary. These are explicitly composed CPU boundaries,
not whole kernel execution on the host. Original filter CPU code executes;
release, FIFO and pending MMIO remain explicit boundaries. Immediate spatial
commits are caller contracts here; their numerical execution is covered by
the preceding combined commit checkpoint.

All928 ordered stores and66 command words agree. The280 executed original
instructions remain byte unchanged. Controls include apply bits, busy words,
five floating-point contexts, signed zero/nonfinite argument spills, overlapping
environmental inputs and source-pointer changes after an exact observed store.
Two initial alias controls extended beyond captured source backing; native
bounds checking rejected those undeclared bytes. The selected v5 fixture
provides explicit adjacent input backing and supersedes the v4 alias inputs.
The three genuine inputs and original instructions remain unchanged. Independent
`setters-host-repeat-v2` repeats fixture, instruction listing and report exactly.
Permanent fixture/provenance files are `sound_sdk_setters.original.json` and
its provenance sibling under `tests/fixtures`.

Both direct native APIs and the driver operation bridge compare every retained
byte, ordered store, call and device command against those original results.
Native state/FIFO/publication services are declared test models; the actual
native filter CPU implementation executes.17 required-owner/FP/publication
failure controls stop and check state release. Normal and ASan/UBSan runs pass.
Eight isolated wrong copies fail: dirty-store width, deferred apply bit,
source-pointer caching, payload snapshotting, argument spilling, busy HRESULT,
conditional release and driver axis order. Production source is not mutated.
Reproduce with:

```sh
python3 tools/validate_sound_sdk_setters.py --output <new-output-directory>
```

The selected validation is `setters-validation-v2/report.json`; an earlier
validator attempt rejected an ambiguous mutation needle before building that
fault control. `sdk-image-pairing-v12` pairs55 complete SDK bodies and28
immutable ranges across all three images. Ambiguous wrappers require exact
already-paired complete callee identities. The synchronization object is
recorded as mutable identity evidence, not interchangeable initialized bytes.
The coherent `setters-source-snapshot-v1` verifies the preceding2,414 build
pins, overlays five audio files and narrow CMake additions, then builds the
complete cache library. All12 selected integrated checks pass. Selected source,
evidence and build pins are in `setters-checkpoint-final/manifest.json`.

Next retained-construction evidence is anchored to a complete420-instruction
startup match: map-pack2205C0 corresponds to base21E4B0, not base2205C0.
An initial address-only base lookup is retained as rejected evidence. The
verified base caller reaches SDK device creation37D797 and buffer creation
37D4BE; their complete static wrappers are retained in
`sdk-construction-base-v1`. Deeper construction is unported/unverified. Live
gameplay still discards emitter positions and supplies the fixed stereo mix.
Retained objects, real device/output processing, scheduling, transitions and
all-sounds audible validation remain open. The latest inspected launchers
select `.local/vista-crosshair-delivery/20261012T015436Z/halo2_native`
(SHA-256`70948eabb0d00c0bb2fc4d3040274a1530bebcac3676be02ff711f8ae089a389`).
This goal has not replaced or audio-validated that concurrently delivered build.

## Retained SDK construction checkpoint

`source_sound_sdk_construct_device`, `construct_device_data`, `construct_buffer`,
`construct_voice` and `bind_buffer` port complete37A132,37CC6A,37C1D7,382B22
and37BE6C retained owners. Reached37BE27,381E93,37A110,37B16F and actual
379D0C/37A14F reference methods are included. Original identities resolve
bounded caller-owned memory; no host pointer is serialized. These are
preallocated-object constructors, not complete device/buffer creation.

Device defaults import the exact15-word listener and12-word environment
blocks from the selected executable. Speaker override validation preserves
both the low-word limit and high-bit mask; invalid overrides require the
real saved-audio-setting owner. The seven repeated DWORD stores, following
word/byte writes and duplicate dirty stores remain in original order.
Reference dispatch uses the selected original vtable's reached method, with
plain increment or synchronized busy80004005 semantics. Nullable retention
returns the identity even when synchronized AddRef reports busy. Counter
wrap follows the original unsigned DWORD. Required state-access providers
must exist before acquisition, and actual acquisitions release on exceptions.

The voice constructor retains hardware/data owners, initializes four list
heads, preserves unwritten bytes and field widths, sets each of six node
indices separately and ORs the three flags. Spatial binding retains both
owners, inserts the buffer node at the actual device-list tail and preserves
each source-pointer reload around speaker/flag updates. It does not assume
an empty list. The two-word initializer preserves overlapping sequential reads.

`sdk-live-construction-v1` captures five whole original cold-boot calls in xemu,
including six actual reference calls and one actual saved-setting call returning0.
Original constructors, kernel calls and reached services execute to normal
returns. Guest instructions, registers, memory and callee results are never
changed. The synchronized reference acquisition flag was not observed in these
captures; no acquisition value is inferred from its successful count change.

`construction-host-oracle-v5` first matches all five genuine retained-state
captures, then143 declared CPU cases. Original constructor/reference
instructions execute unchanged. State access/release and saved-setting
implementation are explicitly separated platform boundaries. Both declared
acquisition0/1 CPU inputs are tested; saved-setting input is the captured
actual0 or a labeled declared value. This is not host execution of Xbox locks
or a native saved-setting provider. All1,967 ordered stores and232 executed
instructions are retained. Independent `construction-host-oracle-repeat-v2`
repeats the fixture, executed listing and report byte for byte.

An initial repeated-fill pair-alias input hid a premature snapshot error.
The selected fixture uses distinct overlapping words and catches that error;
the five genuine inputs and production code did not change. The selected
native normal and ASan/UBSan checks pass. Nine isolated wrong implementations
fail their specific original-backed cases: voice slot index, second data
reference, nonempty list tail, override mask, busy result, conditional release,
pair payload snapshot, source-pointer reload and field width. Ten missing
owner/extent/binding/provider controls stop, including acquired-state cleanup.
Production source is never mutated by these fault controls.

Reproduce the standalone native checks with:

```sh
python3 tools/validate_sound_sdk_construction.py --output <new-output-directory>
```

Selected validation is `construction-validation-v2/report.json`.
`sdk-image-pairing-v14` pairs68 complete SDK bodies and30 immutable ranges
across the supported images. Constructor table references are checked against
complete paired AddRef methods; mutable state identities are recorded separately
from immutable defaults. The exact importer verifies all eight table/method
bindings and27 new default words for each supported image. The coherent
`construction-source-snapshot-v1` overlays the new audio files and narrow
CMake/importer additions on the prior frozen setter baseline. It builds the
complete cache library; all13 selected integrated audio checks pass. Selected
source, reference and build pins are in `construction-checkpoint-final/manifest.json`.

Remaining creation owners include37D72A/37D159 allocation and failure cleanup,
37D5F1/37CF56 hardware initialization and descriptor/source allocation.
Their actual original bodies are retained under `sdk-construction-deeper-base-v1`,
`sdk-construction-owners-base-v1` and `sdk-construction-state-base-v1`.
No allocation, Release or hardware initialization result is invented. These
new retained owners and the preceding spatial CPU chain are still not installed
in the live adapter. Emitter positions, real output processing, scheduling,
transitions and all-sounds audible validation remain open. Both inspected
launchers currently select `build-armor-framebuffer-20261012/halo2_native`
(SHA-256`daf79bcb5c31588ace836dbe147b0cb3d0a01dfa08d33f7263b51a57062b1d43`).
This goal has not replaced or audio-validated that concurrently delivered build.

## Voice data initialization checkpoint

`source_sound_sdk_initialize_voice_data` implements complete37CEF1/37CDCA
descriptor and retained-data initialization. Reached37C825 format selection,
37DA87/37DA3E/37DA17/37D9EE conversion,379F7A/3801DF frequency conversion,
37BC89/37A500 bin routing and37C8AB/37C1B3 buffer/cursor owners are included.
The caller supplies an actual E0B object and bounded memory. Allocation,
release and reached state/IRQL guards require real providers. Hardware voice
initialization remains a separate owner.

Source and environment defaults retain the18- and9-DWORD original copies,
their original pointer reloads and dirty stores. The A4B source and38B
parameter allocations preserve exact tags, zero arguments and failure order.
In particular, the parameter dirty bit is set even when allocation returns0.
PCM, ADPCM and extensible formats preserve original field widths and complete
GUID comparisons. Extensible channel-mask reads remain ordered after format
conversion, including overlapping input/output. Routing selects at most eight
lowest mask bits and imports per-image relocated default headers, pointer
tables and records. Parent-required bins preserve clipping and append order;
source dirty updates reload the source pointer between stores.

Frequency conversion runs under the required state guard and preserves the
caller's floating-point context. The unsigned DWORD rate conversion, exact
reciprocal operand, intermediate float spill, x87FYL2X and FISTP follow the
original instructions.48000 takes the original integer-only special case.
Buffer replacement frees owned backing through its actual owner, preserves
borrowed backing, records actual allocation failure and resets cursors using
the reloaded retained buffer size. Unwritten bytes remain unchanged.

`sdk-live-voice-initialization-v1` captures two whole original cold-boot calls
in xemu. The nonspatial call uses actual eight-bin input; the spatial call
performs the actual A4B and38B allocations. Original format, allocator and
kernel implementations execute to normal returns. Hardware breakpoints only
observe entry, allocation returns and final retained bytes; no guest code,
memory, registers or callee results are changed.

Selected `voice-init-host-oracle-v3` executes unchanged original format,
routing, numeric, descriptor and cursor instructions. Both whole genuine
retained outputs must match before260 declared controls run. Allocation/free
and kernel implementations are explicit host boundaries; inputs are captured
actual returns/backing or labeled declared ABI success/failure controls.
The262 cases retain2,661 ordered stores and461 executed original instructions.
Independent `voice-init-host-repeat-v1` repeats the fixture, executed listing
and report byte for byte. Native outputs never supply expected results.

The expanded oracle initially placed its scratch stack over a declared
replacement-source buffer and did not reach one declared reload action. The
selected oracle separates all backing, asserts disjoint extents and requires
every declared action to execute. Explicit five-bin input reaches the intended
dirty/reload branch; original default spatial bins have fifth ID10, not2.
These were harness corrections; production code did not change in response.

Selected `voice-init-validation-v3/report.json` passes normal and ASan/UBSan
checks. Every case compares complete retained bytes, ordered stores, allocation
and release requests, reached guard counts, semantic result and caller FP state.
Both genuine cases additionally pass using actual `NativeSoundStorage`, the
complete `source_sound_sdk_allocate` owner against explicit raw allocation ABI
inputs, and actual `NativeSynchronization` recursive mutexes. This is native
CPU composition, not natural DirectSound boot, Xbox allocator execution or DSP.
Eleven isolated wrong implementations fail their intended original-backed
cases: source dirty value, format width, allocation failure, parameter dirty
failure order, GUID, mask limit, parent clipping, source reload, cursor reset,
frequency operand and borrowed release. Ten required-owner controls stop.
Production source is never mutated by the fault controls.

```sh
python3 tools/validate_sound_sdk_voice_initialization.py --output <new-output-directory>
```

`sdk-image-pairing-v15` pairs81 complete SDK bodies and36 immutable ranges
across the three supported images. Relocated mix-bin headers and their complete
records are compared semantically across images and imported byte-exact per
selected image. The importer regression checks all identities and retained
regions for each image. `voice-init-source-snapshot-v1` overlays this addition
on the preceding coherent construction snapshot, preserving its2,423 pinned
files. Its complete Release cache build and all14 selected integrated audio
checks pass. Source, evidence and build pins are retained in
`voice-init-checkpoint-final/manifest.json`.

The device/buffer creation wrappers37D72A/37D159,37D5F1/37CF56 hardware
initialization,382F58 dependencies, Release, device output and event/scheduling
integration still need complete native ownership and live validation. The live
multiplayer adapter continues to discard emitter positions and supply fixed
stereo routing. Both latest inspected launchers select
`build-unit-object-covenant-20261011/halo2_native`
(SHA-256`b8db86b2a4b71f402fed662448456d3082307ff0eda698c0214cfd96996ba0a3`).
This goal has not replaced or audio-validated that concurrently delivered build.

## Hardware voice activation CPU checkpoint

`source_sound_sdk_activate_hardware_voice` ports complete381AA9 and composes
the actual complete native gain, pitch and HRTF CPU owners. New reached owners
are37E737 slot allocation,381A92/381A74/3808E4 voice mapping,3808BE dirty
OR and380AE6 bin-command packing. Mutable mapping-cell identities import from
the selected image. Actual bounded retained backing, recursive device locking,
FIFO readiness, device application and reached HRTF are mandatory services.
This is CPU/device-command behavior, not an MCPX DSP implementation.

Slot allocation preserves the original nested guard, busy counter wrap,
ordinary/spatial count selection, occupied-slot scan, WORD handle stores and
retained count reloads. Insufficient available slots return88780032 without
inventing an assignment. Mapping starts at `(first_slot*4+2)*16` and initializes
four16B records, independently of the activated hardware voice count. Bin
packing retains selected order and duplicates, fills missing bins, preserves
the format word's upper bits and uses the original split shifts. Caller scratch
is actual native storage; no host pointer is serialized.

Activation preserves the active gate, nested slot allocation failure and dirty
WORD width. Each reached hardware voice waits for18 DWORD credits and submits
all18 words in original order. The second/third voice format receives400000;
register fields reload after preceding device callbacks. Gain/pitch scratch
uses the complete existing native owners. Spatial flags reach complete3812E0
with zero overrides and its real algorithm/device services.

`sdk-live-hardware-voice-v2` captures two whole original381AA9 cold-boot calls
in xemu. Nonspatial activation writes18 device words; spatial activation writes
37, including19 HRTF words. Original slots, locks, packing, FIFO and MMIO execute
to normal returns. No guest code, registers, memory or callee/FIFO results are
changed. The initial capture retained one mapping record using the wrong
extent; v2 instead retains the four records addressed by complete381A92/381A74.
The original execution was unchanged; the earlier capture is retained as an
incomplete attempt.

Selected `hardware-voice-host-oracle-v2` executes unchanged original activation,
slot, mapping, packing, numeric and HRTF instructions between explicit kernel,
FIFO and MMIO host boundaries. Both complete genuine retained outputs and every
device word agree before445 declared CPU controls run. The first nonspatial
capture precedes algorithm initialization, has a zero dispatch table and never
reads it. When HRTF is reached, complete37E126 initialization produces exactly
the spatial capture's selected dispatch table. No current algorithm is guessed.
The447 cases retain4,664 ordered stores,9,304 device words and632 executed
original instructions. `hardware-voice-host-repeat-v1` repeats the fixture,
executed listing and report byte for byte. Native output never supplies an
expected result.

Selected `hardware-voice-validation-v2/report.json` passes normal and ASan/UBSan
checks. Complete retained bytes, ordered stores, results, guard counts, all
device words and caller FP state agree. Both genuine cases additionally pass
with actual `NativeSoundStorage` and `NativeSynchronization` recursive locks.
Their native HRTF, gain and pitch owners execute in full. FIFO and device sinks
are explicitly declared boundaries; these are not natural device boot, PCM
output or physical DSP tests. Eight missing-owner/extent/guard controls stop.

Eleven isolated incorrect copies fail their intended original-backed cases:
slot pool, occupied slot, mapping formula, mapping count, dirty width, bin
shift, subvoice format, register reload, HRTF omission, retained count reload
and pitch word. The first validation attempt already caught the mapping error
through a bounded extent diagnostic; v2 accepts that exact diagnostic. No
production source is mutated by fault controls.

```sh
python3 tools/validate_sound_sdk_hardware_voice.py --output <new-output-directory>
```

`sdk-image-pairing-v16` pairs88 complete SDK bodies and36 immutable ranges
across all three supported images. The three mapping-cell operands come from
complete381A74 correspondence and are tested by the exact importer regression.
`hardware-voice-source-snapshot-v1` overlays the new audio files and narrow
CMake/importer additions on the preceding2,427-file pinned snapshot. Its full
Release cache build and all15 selected integrated audio checks pass. Complete
source, evidence and build pins are in `hardware-voice-checkpoint-final/manifest.json`.

At the hardware-voice checkpoint, the382F58/380900/382FE3/38094B/382CD4
initialization/dispatch chain remained incomplete. The subsequent section
records its completed CPU callers.37D72A/37D159/37D5F1/37CF56 creation,
Release and actual platform output remain incomplete. Reached listings are retained under
`sdk-hardware-initialization-base-v1` and `sdk-hardware-voice-*-base-v1`;
their unported service results are not invented. Live gameplay still discards
emitter positions and supplies fixed stereo output. Event inventory, scheduling,
transitions, local/split/LAN audible validation and runtime delivery remain open.
Both inspected launchers select `build-unit-object-covenant-20261011/halo2_native`
(SHA-256`fffc5d1d02245de2d3020edb8dcf54be625730d28b80720359c020c11950e88b`).
This goal has not replaced or audio-validated that concurrently delivered build.


## Voice format and initialization dispatch CPU checkpoint

The complete380900 dirty-marker/virtual dispatch,38094B format update,
382FE3 active reconfiguration,382CD4 activation/attachment dispatch and382F58
initializer callers are implemented in `sound_sdk_voice_dispatch.cpp`.
Original channel-count byte wrap, PCM/ADPCM format bits, mono/channel fields,
active-format rejection, WORD/BYTE store widths and HRESULT propagation are
preserved. Format commands retain the original FIFO requirement and reload
registers/counts after preceding callbacks. The active predicate is read before
the final format store, matching the original instruction ordering.

Reconfiguration retains the original pre-stop active predicate, re-reads data
through voice+80 after stop/drain, releases changed channel counts, then runs
format and preparation in original order. Stream attachment's EAX is ignored;
buffer attachment's HRESULT is propagated. Reached382DD2 stop,381E0E drain,
382F96 release,382BE3 stream and382B7E buffer attachment owners remain required
services. No success result is manufactured for these unported inner owners.

`sdk-live-voice-dispatch-v1` captures two whole original382F58 calls during
Xbox cold boot in xemu. They return normally after18 nonspatial and37 spatial
device words, with the latter including19 HRTF words. Guest registers, memory,
instructions, kernel/FIFO results and callee returns are unmodified. Complete
retained voice/data/device/count/mapping/source/parameter outputs are retained.

`voice-dispatch-host-oracle-v1` executes unchanged original caller, activation,
slot, mapping, packing, gain/pitch and HRTF instructions between explicit host
kernel/FIFO/MMIO boundaries. Both genuine outputs and all55 captured commands
must match first. The following2,649 cases declare raw CPU inputs, including
explicit lifecycle/attachment ABI responses and callback mutations where reached.
They cover PCM widths, ADPCM/unknown formats, channel count/wrap, active-format
errors, dirty marking, both original vtables, HRESULTs, stream/buffer gates,
allocation failures and retained reloads. These declared boundaries are not
additional gameplay captures or implementations of the inner owners.

All2,651 cases retain5,210 ordered stores,1,367 device words and823 executed
original instructions. Independent `voice-dispatch-host-oracle-v2` repeats the
fixture, full executed listing and report byte for byte. No expected value comes
from native output. `voice-dispatch-validation-v2` passes normal and ASan/UBSan
checks, including full retained bytes, store widths/order, service calls,
commands, results, guard counts and caller FP preservation. Both genuine cases
additionally compose actual OS storage and recursive locks with complete native
activation/gain/pitch/HRTF CPU owners. FIFO/MMIO sinks remain declared boundaries.

Ten isolated incorrect copies fail: channel count, active-format HRESULT,
PCM16 bits, mono flag, subvoice format, reconfiguration data reload, attachment
HRESULT, initialization skip, command register reload and active-predicate order.
Sixteen required-owner/extent/lock/virtual-dispatch controls stop. The first
validation attempt detected the PCM16 error in a genuine case before its declared
case; v2 corrects only the validator's expected first-failure label.

`sdk-image-pairing-v17` pairs93 complete SDK bodies and36 immutable ranges across
three owned images. The importer checks both original vtable+14 format slots
against the separately paired complete method identities in every image.
`voice-dispatch-source-snapshot-v1` overlays narrow audio/CMake/importer changes
on the preceding2,431-file pinned snapshot. The full Release cache build and16
selected integrated audio checks pass. An initial target-name typo and an importer
local-name collision are retained in unsuccessful build logs; the final build
fixes them. `voice-dispatch-checkpoint-final/manifest.json` retains complete pins.

Static next-owner evidence is retained under
`sdk-voice-attachment-dependencies-base-v1` and
`sdk-sample-pages-dependencies-base-v1`. Complete buffer attachment maps sample
pages through3806BE/380327/380419 before setting3824EE's cursor. Its actual
kernel mapping, device application and lifecycle owners remain unfinished.
Creation/Release, the live spatial adapter, complete event/scheduling/timing
inventory, local/split/LAN audible comparison and rebuilt launcher delivery
remain required. The currently installed runtime still discards emitter
positions; no audible or physical DSP parity is claimed.

At the voice-dispatch checkpoint, both launchers select
`build-unit-object-covenant-20261011/halo2_native`
(SHA-256`96a6eaf2c57c5b0465c221fe03b20bb035944a95aa7d36d8410bdef1cddf630e`). The concurrent runtime has changed
since the preceding inspection. This audio goal has neither replaced nor
audio-validated that selected build.

## Sample-page ownership CPU checkpoint

The complete38025A/38029C manager construction/initialization,3806BE acquisition,
380327 best-fit allocation,379C60/3801FD list/split helpers,380419 device mapping,
38085D release,38074D/3803F8/37E612 free coalescing and380486 page unlock are
implemented in `sound_sdk_sample_pages.cpp`. Source WORD count/index widths,
32-bit page rounding/wrap, reference wrap, available-page accounting and list
store order are preserved. Unchanged cached mappings increment references and
produce no new kernel pin/translation or device commands. New mappings unlock
the manager before pin/translation and map each page under its own device guard.
The descriptor index reload follows that outer unlock. Final release saves the
original buffer/count before merging free neighbors, then unlocks that original
page extent after leaving the device guard. Zero-page mapping still invokes the
original pin owner. Actual allocator, pin/translation, synchronization, FIFO and
device owners are mandatory services; no physical address or completion is guessed.

`sdk-live-sample-pages-v1` retains two whole original3806BE cold-boot calls in
xemu. The first maps an actual64-byte buffer through one page, records the real
page-pin and physical-address calls and submits two device words. The second
shares that mapping, incrementing its reference without device writes. Complete
manager, actual40,960-byte node pool and available-page cell outputs are retained.
Pool extent comes from walking both actual intrusive rings, not a guessed count.
Original kernel calls, pool instructions, FIFO and MMIO execute to normal returns;
no guest instructions, memory, registers or callee results are modified.

`sample-pages-host-oracle-v7` executes unchanged original CPU owners. Both genuine
retained outputs, normal returns, platform calls and device words must match
before228 separately declared ABI inputs run. Cases include initialization/OOM,
unsigned wrap and offsets, fragmented best-fit allocation, cached reference wrap,
page-index wrap, final-release coalescing and store/provider reloads. Every original
non-stack store must fit a retained input extent. Host allocator/pin/translation,
lock/FIFO/MMIO boundaries are explicitly captured or declared inputs. Original
void routines' incidental volatile EAX values are retained but not semantically
compared with native void functions. No native result supplies an expected value.

All230 cases retain7,823 ordered stores,356 device words and349 executed original
instructions. Independent `sample-pages-host-oracle-v8` repeats the fixture,
listing and report byte for byte. `sample-pages-validation-v1` passes normal and
ASan/UBSan checks, comparing complete retained bytes, store widths/order, kernel
calls, commands, semantic returns, guard counts and caller FP preservation. Both
genuine acquisitions additionally pass with actual OS storage and recursive
locks. Physical translation/pinning and device sinks remain declared boundaries;
this is not natural native device initialization, PCM or physical DSP proof.

Nine isolated incorrect copies fail: cached reference, best-fit selection, split
width, mapping index, final unlock extent, merge count, missing zero-page pin,
allocation HRESULT and available count. Eleven missing-owner/extent/guard controls
stop. No production source is mutated by these controls.

`sdk-image-pairing-v21` pairs105 complete SDK bodies and36 immutable ranges across
three owned images. The manager's exact vtable/count-cell constructor operands
are imported and checked in all three. Five adjacent executable-pointer words
starting at that identity also receive complete-body static correspondence.
Their membership and the table's length are not established; importing an
identity implements no destructor/Release behavior. The previous static attempt
incorrectly required relocated pointer bytes to be equal; v21 checks neighboring
method bodies without assuming the table's extent.
The first host replay omitted a stdcall boundary's argument cleanup; the selected
replay implements its actual ABI. Initial declared initialization inputs also
reused seeded rings pointing outside the smaller new allocation. The selected
fixture supplies the constructor's empty-ring precondition and rejects every
unretained original store. Genuine captures are unchanged throughout.

`sample-pages-source-snapshot-v1` overlays narrow audio/CMake/importer changes on
the preceding2,438-file coherent snapshot. Its full Release cache build and17
selected integrated audio checks pass. Selected source, original evidence, build
inputs and binaries are pinned in `sample-pages-checkpoint-final/manifest.json`.
Both launchers currently select `build-interaction-prompts-20261011/halo2_native`;
this audio goal has not replaced or audio-validated that concurrent runtime.

Whole382B7E/382BE3 voice attachment and3824EE cursor/382560 cleanup, stop/drain/
Release, remaining creation, real platform/device/PCM output, live positional
routing and all-sound trigger/scheduling/timing validation remain open. Page
ownership CPU parity is a necessary dependency and does not complete those paths.

## Whole buffered and streamed voice attachment checkpoint,2026-10-12

Complete382B7E buffered attachment,382BE3 stream attachment,3824EE cursor
packing,382560/3824EA detachment and37F43E sample-to-byte conversion are now
implemented in `sound_sdk_voice_attachment.cpp`. These owners compose the
complete native sample-page acquisition/release and37C8AB buffer owners.
They preserve the source's WORD flags and page indices, unsigned offset and
sample-byte wrap, stream sentinel page, ADPCM shift, original returned-entry
branch, ignored stop/stream-buffer HRESULTs and post-callback pointer reloads.
Reached stop/drain owners remain mandatory services; those bodies are not
implemented by declaring callback success.

`sdk-live-voice-attachment-v1` retains three whole original cold-boot calls:
two buffered calls, allocating then reusing the same sample page, and one
streamed call. Hardware breakpoints only observed the unmodified original
executable in xemu. Real guest page pinning, physical translation, FIFO and
MMIO executed normally. The buffered sample was64 bytes at83AC422C; its first
mapping emitted physical03AC4000. The stream selected128 bytes at83AEC100
and packed cursor007FF100. Those raw original observations are not a native
DSP, decoded waveform or physical-console comparison.

`sdk-image-pairing-v22` pairs111 complete SDK bodies and36 immutable ranges
across all three owned images. Stream backing is a mutable pointer cell;
complete382BE3 operands establish00387330,0038D970 and00391410 for base,
map-pack1.1 and title-update1.5 respectively. `SoundSdkSpatialData` imports
those identities without importing a live pointer value. Neighboring vtable
words retain the preceding checkpoint's explicit unknown-extent boundary.

The selected unchanged-original oracle is `voice-attachment-host-oracle-v6`;
v7 independently repeats its fixture, executed listing and report byte for
byte. Its281 cases comprise three genuine whole calls and278 declared ABI
cases,1,071 ordered stores,50 device command words and470 executed original
instructions. Every non-stack store must fit retained backing. Declared cases
exercise sample conversion, cursor overflow/sentinel paths, idempotence,
cached pages, cleanup failure handling, borrowed/owned stream buffers,
allocation failure and provider/observer/guard pointer changes. Two prepare
cases compose complete382CD4 and active381AA9 with the actual attachment
owners. They are declared states, not additional captured gameplay events.

`voice-attachment-validation-v2` passes normal and ASan/UBSan builds, catches
ten isolated incorrect implementations and stops at14 missing/invalid owner
boundaries. All three genuine cases additionally compose real OS storage and
recursive native locks. Physical-page/FIFO/MMIO sinks remain explicit CPU
boundaries. Genuine calls reach no modeled allocator/free/stop/drain returns.
Those declared ABI returns do not establish the remaining original lifecycle
or native device implementation.

The initial capture failed before the game started because SDL had no usable
display; the successful retry explicitly selected X11. The first compiler run
failed because `/tmp` was full; subsequent runs used workspace scratch backing.
The first oracle attempts exposed a missing Python dependency/environment and
a code-section selection error; the selected runner resolves both executable
sections. The first validation's dirty-store mutation matched two sites and
was rejected. Its replacement isolates one site, and the stream-reload case
now selects a cursor path that consumes the reread argument. Failed attempts
remain retained; they are not counted as passes.

`voice-attachment-source-snapshot-v1` preserves the preceding coherent source
and overlays only the new audio/importer/CMake/test changes. The complete
cache library builds and all18 selected integrated audio checks pass. Source,
original evidence, validation copies and binaries are pinned separately in
`voice-attachment-checkpoint-final/manifest.json`. Reproduce component checks:

```sh
TMPDIR=<workspace-scratch-directory> python3 tools/validate_sound_sdk_voice_attachment.py \
  --output <new-output-directory>
.tools/python/bin/python tools/oracle_sound_sdk_voice_attachment.py \
  --capture analysis/audio-spatial-20261011/sdk-live-voice-attachment-v1 \
  --pairing analysis/audio-spatial-20261011/sdk-image-pairing-v22 \
  --output <new-original-output-directory>
```

The live multiplayer adapter still discards emitter/listener position inputs
and routes fixed stereo. Remaining creation/stop/drain/Release/device/output
owners, source scheduling, complete sound-event/placement/timing inventory,
rendered local/split/LAN audible validation and final launcher delivery are
unfinished. This checkpoint does not complete the user's all-sounds goal.

## Notification queue and pending-voice CPU checkpoint,2026-10-12

`sound_sdk_notifications.cpp` implements complete37F245/37F11A/37EAE2
notification arming,37F068/37EB07 cancellation,382AD9/382B08 voice wrappers,
3811A6 polling,381E0E drain and37F407 mapping completion. It composes the
reached379C60 list detachment. The source retains stable equal-deadline order,
signed64-bit comparisons, relative-time conversion and wraparound, exact
ordered stores, timer rearming/cancellation/DPC order, nested guards, callback
rereads and literal mapping flag1. The drain has no invented iteration limit
or pending-flag clear. Expiration requires the actual3810BD forced-stop owner;
that device implementation remains unported and is a mandatory callback.

`sdk-image-pairing-v24` pairs121 complete SDK bodies and36 immutable ranges
across the base, map-pack1.1 and title-update1.5 executables. Complete382B22
constructor operands establish derived tables41727C/41D8BC/42132C. Their
arm slots are382AD9/389119/38CBB9 and cancel slots382B08/389148/38CBE8.
The importer verifies those exact reached slots. This admits neither unknown
tables nor the base pure-virtual owner, and proves no other table extent.

`notifications-host-oracle-v6` and independent v7 produce identical fixtures,
executed listings and reports. The274 declared ABI cases contain776 ordered
stores,264 platform calls and264 executed unchanged original instructions.
Cases cover zero/relative/absolute/expired deadlines, signed-time wraparound,
equal ordering, queue head replacement, slot arithmetic, repeated polling,
mapping flags and changes during guards, stores and callbacks. Expected
values come from original instructions, not the native implementation.

`notifications-validation-v3` passes normal and ASan/UBSan runs, catches12
isolated wrong implementations and checks12 missing/invalid owner stops.
Every case also repeats with real OS storage and recursive native locks.
Time/timer/DPC and forced-stop inputs remain declared ABI boundaries. No
native kernel timer, DSP, PCM or audible timing comparison is claimed.

Four emulator capture attempts are retained as unsuccessful evidence. Each
observed an immediate-arm return, but cancellation's shared guard return
address was repeatedly reached on other guest stacks without the requested
activation returning. Address-plus-stack matching and hardware-only retries
did not resolve that capture. The partial arm recording also contains device
changes between entry and the guard boundary, so the full retained state
does not match independent replay with only the recorded guard effects.
No genuine emulator case is selected for this checkpoint. Oracle-v2 records
that replay failure. Validation-v1/v2 stopped on overly narrow diagnostic
expectations after catching their wrong copies; selected v3 corrects those
expectations without changing native implementation or original expectations.

`notifications-source-snapshot-v1` verifies the preceding coherent source
and overlays only this audio/importer/CMake/test change. The complete cache
library builds and all19 selected integrated audio checks pass. Current
inspection source, coherent compiled source, binaries and original evidence
are separately pinned in `notifications-checkpoint-final/manifest.json`.

```sh
TMPDIR=<workspace-scratch-directory> python3 tools/validate_sound_sdk_notifications.py \
  --output <new-validation-directory>
.tools/python/bin/python tools/oracle_sound_sdk_notifications.py \
  --pairing analysis/audio-spatial-20261011/sdk-image-pairing-v24 \
  --output <new-original-output-directory>
```

Both launchers still select `build-interaction-prompts-20261011/halo2_native`.
This CPU component checkpoint has not replaced or audio-validated that build.
The live adapter continues to route fixed stereo. Creation, forced stop,
remaining lifecycle/device/output, scheduling, the complete sound-event and
placement/timing inventory, rendered local/split/LAN audible comparison and
final launcher delivery remain unfinished. The all-sounds goal is incomplete.

## Live world placement and LAN event correction,2026-10-12

The live SDL adapter now retains the emitter position on world impulses,
projectile impacts/detonations, prop effects and posed material footsteps.
Current local camera positions supply listener locations. Nearest-listener
distance uses the imported authored sound/class bounds, the original distance
owner and the21B900 gain conversion before the original driver-volume owner.
Missing world listeners produce silence. This applies to actual decoded PCM;
it does not add HRTF, directional/cone processing, occlusion or source PVS
admission. The initial failing PCM fixture reported distant sounds at full
volume. Near/far, missing, split-screen and moving-listener PCM checks now pass.

The real native UDP/SDL test also exposed a second, independent bug. The
client played local SMG fire through both the generic replicated shot route
and the authored weapon-effect route. Seven actual host firing events produced
fourteen client submissions. A slow client also consumed only the final
weapon-effect record in a rendered frame. The correction suppresses the
redundant local SMG shot route and consumes each genuinely accepted snapshot's
emitted effect with the existing serial/lifetime/owner checks. It does not
invent events for missing snapshots. The original firing owner is documented
in `MP_SMG_LOCAL_EFFECT_OWNER_RESEARCH.md`:17AD20→17BDF0→18AA90→18AD70
selects the authored firing sound.

The isolated corrected runtime passes actual local one/four-player and LAN
one/four-player plus delayed-client checks. Authority events and real SDL
submissions agree8/8,7/7 and11/11 in the selected LAN runs. Reload, ambient wind
and material footsteps also reach actual SDL packets. SDL uses dummy output;
physical speakers and original Xbox waveforms are not measured. The selected
red/green logs, diagnostic source/object/binary and build commands are pinned
in `analysis/audio-spatial-20261011/lan-sound-fix-v1/manifest.json`.

Earlier unforced-X11 runs are retained as failed evidence: inherited Wayland
selection invalidated several focus checks. The current delivery verifier
explicitly selects SDL's X11 driver on its isolated Xvfb display. No synthetic
focus event replaces actual OS focus. A full current-source build and its
launcher validation are tracked separately from the isolated proof above.

The full delivery now includes the newer installed prompt/input runtime,
`build-interaction-prompts-verified-20261012/halo2_native`. Its 100 sound/module/
observer source files agree byte for byte with the independently validated
audio snapshot. Both installed launchers pass actual gameplay and title-screen
routes. Fresh component, HUD, saved Elite, world/prop and local/split/LAN audio
validation is indexed in `audio-delivery-final/manifest.json` and summarized in
[the delivery report](MP_AUDIO_DELIVERY_20261012.md). These live corrections
remain distinct from the CPU SDK checkpoints and the unfinished all-sounds
goal.

## Retirement CPU scope,2026-10-12

Complete384A36 gate,380EE1 unlink,3829F7 mapping status and3810BD forced
retirement compose their reached native CPU owners.459 original-instruction
cases,3,194 ordered stores,1,301 device reads,649 command words and1,384
platform calls repeat byte for byte independently. Native ordinary and
ASan/UBSan comparisons pass; eight isolated incorrect copies fail and14
missing-owner controls stop. Every case also runs with actual OS storage and
recursive locks.37 immutable ranges and125 complete SDK bodies are paired
across the three supported images. Kernel/FIFO/MMIO and a reached381EF8 stop
remain explicit external inputs in this checkpoint. They are not original
kernel execution, hardware DSP, PCM or audible proof. The original validator's
inaccurate blanket phrase about forced-stop boundaries is preserved in its
selected old source and corrected in the current tool. The exact distinction
and source/evidence pins are in `voice-retirement-checkpoint-final/manifest.json`.

## Common voice stop CPU scope,2026-10-12

Complete3811FA pause,381122 envelope stop and381EF8 common stop now compose
native count/notification/pending owners. The37ED0F/37ED36 counted-slot helpers
retain the original wraparound, failure limit and result.707 unchanged-original
cases compare2,158 ordered stores,195 device reads,423 device command words
and73 external calls. Independent original repeats are byte identical. Native
ordinary/ASan/UBSan runs pass, eight isolated wrong implementations fail and15
missing-owner controls stop. All cases also compose actual OS storage and
recursive locks.130 complete bodies and37 immutable ranges pair across the
three supported images. Original parent382DD2 buffer stop, forced pending
retirement and kernel/FIFO/MMIO boundaries are declared services here; this
does not establish native DSP output or audible timing. Exact source/evidence
pins are in `voice-stop-checkpoint-final/manifest.json`. The first oracle draft
contained overlapping unused slot backing and was rejected; the selected
fixture removes that unused region and retains disjoint declared state.
