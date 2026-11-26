# Campaign audio parity with OG Xbox (2026-11-22/23)

The requests: make all voice, ambience and music cues trigger correctly (the
Armory music and some voice timing were wrong), and make the sound engine
play imported MCC sound assets exactly as the original Xbox game does.

Original TU1.5 code and the owned sound tags are the authority. The imported
MCC maps are runtime data. Campaign plays through the native `MultiplayerAudio`
mixer (`scripted_audio` in `src/app/main.cpp`).

## Fixes

### 2026-11-22 (Codex session, completed and verified here)

| Cause | Fix |
|---|---|
| Classic mode played the four Remastered-only campaign cues (MCC `snd!` flag bit 11). | `plays_in_classic_audio` gates script impulses, scripted loops and native script preparation. |
| Stopping the tram score cut it off instead of fading it over its authored ten seconds. | Retail `18D360` music selection and `18D6E0` fade: the Sound Mix's Game Music Fade Out Time, with the Classic Sound Classes' per-class fade curves (`load_looping_music_fade`, `AudioEngine::fade_out` class curves). |
| Dialogue waits used decoded clip lengths. The Armorer's two-variant shield-training line always waited 57 ticks, but its longer variant needs 94. | The native script sound clock answers the waits for the variant that actually plays (`SourceScriptSoundClockQueryBindingTu15`). The legacy PCM-duration clock is skipped while it is bound. |

### 2026-11-23: BSP background sound and object loops

Campaign played BSP ambience through a simplified lane (`MultiplayerAudio::ambient`).
That lane starts only each palette sound's main tracks. It never scheduled the
authored random **detail sounds**, never played weather alternates or
transitions, and campaign played **no object loops** at all (machines, the
tram, scenery hums).

On Xbox, the 18EE50 object-audio frame owns all of these. `18E970` reads each
local player's listener record (written by `127400`), chooses the palette and
portal targets (`18DC70`), and requests the loops through `220C60`, with
details, alternates and fades. The native kernels for this group already
existed, and multiplayer already plays object loops through them. Campaign
never ran the group, and `18E970`'s listener loads were never bound to the
native camera, so they always read an inactive record.

| Change | Where |
|---|---|
| `18E970`'s five listener loads (`18EA19` active, `18EA24` cluster, `18EA34`/`39`/`3C` position) read the native camera view when the view opts in (`SourceObjectAudioViewTu15::background_sound`). Multiplayer leaves it false, so its behaviour is unchanged. | `source_object_audio_views_tu15.hpp`, native `r_18e970`, the generated lifted reference (via `tools/object_audio_views_tu15.py` `SITES`) |
| Native `18DC40`, the portal-distance comparator that `140BC0`'s sort calls through a function pointer. It was checked against the retail bytes (`COMISS`/`JBE`, ret 12). | `native_tu15_sound_kernels_object_loop_update.cpp` and the group's entry list |
| The native weather state is published to its original globals: current `47365C`, target `473660`, stopped/transition/rate `515F00..08`. These are mounted at their authored `.data` values (1.0, 1.0, 0, 0, 0). | `PhysicsLiveWorldTu15::publish_native_weather`, `native_tu15_world_memory.cpp` |
| `cinematic_enable_ambience_details` is published where `2AC400` writes it (`[4F9B58]+0x240`), so cinematics suppress details as authored. | `PhysicsLiveWorldTu15::publish_native_ambience_details` |
| Authored `.rdata` operands are mounted: ln(10) at `455B14`, −2π at `46A580`, and `220C60`'s seven-phase tables at `455BA8`/`455BC4`/`455BE0`. These values come from the retail XBE. A probe against a real campaign World found them missing, the only gaps among the 57 addresses the group's code reads. The sixth unbacked address, `51DF90`, is the DSP globals pointer; it is read only when `22C560` builds an environment DSP block, and no level run reached that. | `native_tu15_world_memory.cpp` |
| Once per simulation tick, campaign runs the 18EE50 frame with the render camera and the real time since the previous step. It then feeds the frame's loop requests to the native loop lane (`object_loop_request`/`object_loop_frame`), as multiplayer does. Xbox ran this update once per 30 Hz game frame; running it every render frame cost about 11% of Cairo's frame rate. | `src/app/main.cpp` |
| The native lane rejects loops whose controller it does not support yet (synchronized/profile loops, four-bin combined loops). Those loops are skipped. BSP background sounds among them keep the earlier main-track lane, so no room goes silent. | `src/app/main.cpp` |
| Only Impulse-kind submissions with an owner count as unit speech. Loop tracks and details keep their emitter object as owner. | `src/app/main.cpp` |

`HALO2_REFERENCE_CAMPAIGN_AMBIENT_LANE=1` restores the earlier lane for A/B
comparison on the same binary.

Two test diagnostics changed to fit the new voice traffic:

- `HALO2_SCRIPT_AUDIO_RETIRE_TRACE` still caps ambient voices at IDs 1..256,
  but now always logs impulses (dialogue). Loop voices push dialogue IDs
  above 256.
- The Armory room-tone check accepts the native lane's packet stream as well
  as the earlier lane's single looping buffer. Both play for the whole level.

Any exception inside `step_native_object_audio` permanently faults the World
(`continuation_fault`), which then breaks the script sound clocks. A missing
backing word must therefore never be reached. The all-levels test below
checks this on every campaign start.

## Evidence

All runs use one binary in a hidden Xvfb window with SDL dummy audio.

- `tools/test_campaign_armory_audio.py --continue-cairo` runs the Armory to
  Cairo route through Johnson's first scene.
  - With the reference lane, every earlier check passes. That covers the
    Codex fixes: dialogue order and spacing, the tram score's start, stop and
    ten-second fade, and the Johnson cue timing.
  - The new check, `cairo_ambience_details_reach_callback`, fails with the
    reference lane (0 detail packets). With the new lane, all five of the
    Cairo room's detail sounds reach the mixer.
- `tools/test_campaign_loop_audio_levels.py` starts every campaign level and
  runs 900 ticks. No level shows an object-audio fault. The levels that fail
  do so with the same script or engine faults in both lanes: unsupported
  builtins `cinematic_clone_players_weapon`, `objects_detach` and
  `device_set_position_track`, a missing backing word at `480844` in
  Sentinel Walls, and a non-native contact-agent destruction in Flood Zone.

| Level (900 ticks) | Earlier lane: packets / sounds | New lane: packets / sounds |
|---|---|---|
| 01a Armory | 5 / 1 | 182 / 9 |
| 01b Cairo | 8 / 3 | 28 / 12 |
| 03a Old Mombasa | 2 / 2 | 104 / 4 |
| 07a High Charity | 27 / 7 | 40 / 13 |
| 08a Delta Cliffs | 12 / 1 | 174 / 4 |

Frame rate on Cairo over 1800 ticks, interleaved runs in a hidden 640x360
window (higher is better):

| Lane | Frames rendered |
|---|---|
| Earlier lane | 5139, 4945, 6089, 5026 (mean 5300) |
| Object audio every render frame | 4607, 4576 |
| Object audio once per tick (shipped) | 5569, 4303, 5814, 5870 (mean 5389) |

## World sound requests and multiplayer ambience (2026-11-23, second pass)

### Campaign: the World's own sounds now play

Campaign never observed the source World's completed sound requests. That is
`18CEFC`'s 160-byte submit, where the original caller has already chosen the
sound, owner, scale, gain and position. Footsteps, armour foley, doors,
switches, the tram, weapon fire, impacts and AI combat dialogue were all
silent.

A trace of the Armory route found 2,050 such requests in 9,600 ticks:
1,533 marine foley, 490 footsteps, 22 device sounds and 5 weapon-handling
sounds. Delta Cliffs adds plasma rifle and brute shot fire and Elite combat
dialogue.

Campaign now delivers each request through `MultiplayerAudio::source_impulse`,
the path multiplayer uses for local weapon sounds:

| Request | Handling |
|---|---|
| Classic sounds | Play. The original requests both the Classic and the MCC Remastered sound for each weapon shot; `classic_sound` keeps only the Classic one in Classic mode. |
| Forced selection (flag `0x400`, AI dialogue) | Now supported. As `223B20` does, it plays the request's own pitch range (`+9C`) and permutation (`+9E`) instead of a random choice. |
| Forced-selection requests with a speaker | Feed the speaker's facial animation (`18F510`), as scripted dialogue does. Foley and footsteps don't. |
| Listener-relative location (type 2) | Supported since the ninth pass. |

`HALO2_REFERENCE_CAMPAIGN_NO_SOURCE_SOUNDS=1` drops these requests, as
before. The tests trace them as `Source audio packet:`.

### Multiplayer: BSP ambience through the original owner

The host's source World now runs `18E970` from its local views, with the
weather published, so multiplayer ambience plays its authored details.
Ambience the native lane rejects keeps the main-track lane. Replicated clients
keep the main-track lane, because they have no source World.
`HALO2_REFERENCE_MULTIPLAYER_AMBIENT_LANE=1` restores the earlier behaviour.

The host's object-audio step now runs once per simulation tick, as on Xbox,
instead of every render frame. `HALO2_OBJECT_AUDIO_TIMING=1` reports its mean
cost every 600 steps. On Zanzibar, at about 53 fps, the step costs about
1.3 ms in both lanes, so the details cost nothing measurable.

### Results

| Run | Result |
|---|---|
| Full Armory to Cairo route | Passes every check, including the new `armory_world_sounds_reach_callback`. It delivers 3,251 world-sound packets; previously none. |
| All campaign levels, 1,200 ticks | No audio faults. World sounds include 700 on Delta Cliffs, 190 on Cairo and 149 on Gas Giant. Failures are existing gameplay or script gaps; Old Mombasa's `1C4E80` owner crash after about 1,000 ticks also happens with every audio change switched off. |
| Lockout, Zanzibar, Coagulation | Start and run cleanly. |

## Synchronized, detailed and scripted loops (2026-11-23, third pass)

### Synchronized loops, centered phased loops and their details

The phased loop owner (`source_loop_phased_output_tu15.cpp`, the native
`220C60` scheduler) used to refuse four kinds of loop:

- lsnd flag `0x20` ("synchronize tracks");
- `0x10` loops without a controller;
- loops with detail sounds;
- loops with no emitter.

Those include the BSP ambience of Lockout, Coagulation and several campaign
rooms. They now follow the original engine (reference decomp `21A250`,
`21AEB0`, `21B940`, `21CB90`):

| Original behaviour | Native port |
|---|---|
| Either sync bit (`0x30`) gives each track its own seed. | Same. |
| With a controller (`args[3]`), tracks also keep per-track permutation masks and share the controller's seed. | Unchanged existing path. |
| Without a controller, each track advances the default controller's seed and takes the result; masks stay shared and no previous permutation is supplied (`21CC30`). | New `common_permutation_without_previous`. |
| `0x20` with a real controller counts a controller's new voices (`21B940`/`21F8A0`) and starts them together once every channel is primed (`219D90` → `21F430`). | Allowed. The native lane submits a frame's new voices in one mixer pass, so they already start together. The original's brief position hold while waiting (`21BF00`) is the only difference. Most rejected ambience (Lockout, Coagulation, Old Mombasa, Gas Giant, Sentinel Walls, High Charity) uses this profile. |
| Definition bits `0x01` (deafening to AI) and `0x04` (stops music, read only by `18D220`) don't affect scheduling. | Allowed. |
| Centered, unowned loops are ordinary owners. | Allowed. |
| Detail deadlines are drawn at creation before track seeds (`21AEB0`), then scheduled each frame while the loop isn't stopping and details are enabled (`21B0E0`). | The object-loop lane's detail code (`223670`/`2229C0`/`222166`) now serves phased owners too, with deadlines, voices and alternate/disable state kept on the owner (so they checkpoint). |
| A centered `0x80` ("combine") loop is ordinary; only located sources combine (`220CCD`). | No longer refused. |

### Four-bin combined loops

Combined loops (`0x80` with a location, e.g. Old Mombasa's gasoline fire)
carry the original's four mix-bin amplitudes, in front-left, front-right,
back-left, back-right order (the direction rows at `44EDA0`). The native mixer
now plays them: on the stereo output each side takes its front and back bin,
capped at unity like an Xbox mix-bin volume. The amplitudes already include
the original distance gain (`192D70`), so scalar distance attenuation is
skipped for these voices.

### Scripted loops through the World's loop pool

`sound_looping_start` (`2B3E50`) calls `18D220`, `sound_looping_stop`
(`2B3EA0`) calls `18D2C0`, and `sound_looping_set_alternate` (`2B3FA0`) sets
pool flag `0x10`. They work on the same pool (`[4F9B5C]`) that the `18EE50`
frame requests every frame. These are now native, ported instruction by
instruction from the retail image and registered at their original addresses:

- `18D220` start, `18D2C0` stop;
- `1901C0` stop all music, `1900D0` stop a row with a kind;
- `190150` is-music test, `18D3F0` scripted row, `190080` current marking.

Campaign Simulation calls them with the original arguments, so scripted
loops get their authored details, alternates with transition tracks, object
positions and music-stop behaviour from the original code.

One-shot loops (flag `0x02`) were still on the host lane after this pass; the
fourth pass below moves them to the pool too.

`HALO2_REFERENCE_CAMPAIGN_SCRIPT_LOOP_LANE=1` keeps every scripted loop on the
host lane, and so does `HALO2_REFERENCE_CAMPAIGN_AMBIENT_LANE=1`.

### Third-pass results

| Run | Result |
|---|---|
| All 15 campaign levels, 1,200 ticks | Every loop the World requests is owned by a native lane: the earlier rejections ("synchronization/profile owner", "four-bin playback owner") are gone. Ambient variety rose, e.g. Flood Lab 38 packets vs 18, Sentinel Walls 12 vs 6. No audio faults; level failures are the same existing script and gameplay gaps. |
| Lockout, Zanzibar, Coagulation | Clean, with no loop rejections. Lockout's and Coagulation's synchronized background loops now play with their details. |
| Full Armory to Cairo route | Passes every check. |
| Pool music probe, Cairo `01b_01` | Start phase at frame 0 (15.9 s), loop body queued at frame 53 to follow it seamlessly, stop fade and retirement after `18D2C0`. |
| Pool music probe, Old Mombasa `03a_02` | Start and body, a body repeat at frame 48, and after `set_alternate` the authored transition sound queued at the body boundary (frame 96). |
| Census of six campaign maps | 88 of 99 Classic and Remastered music lsnds are true loops, so they take the pool path. |

`multiplayer_authored_audio` (Lockout, Xbox disc map) failed after the second
pass, and the cause was this work, not another session's. The test's
`0xA5`-filled source request reached forced selection (`0x400`) with variant
`0xA5`, and `State::play` indexed past the sound's pitch ranges. Forced
selection now validates the authored range and permutation first and rejects
an escaping one, as the original's selected index must exist. The test also
gained a positive forced case. It passes.

## One-shot loops and 3D placement (2026-11-23, fourth pass)

### One-shot loops through the pool

The phased loop lane now plays one-shots (lsnd `0x02`) the way base `21C400`
does. Once the loop phase of a track has queued its last packet, with no
transition pending, the channel stops (state 4, sound flags `|= 8`) instead of
choosing another permutation. The owner is then finished: a later request
changes nothing, and only a stop still applies.

The original's caller acts on that. `21A250` deletes a stopped sound once
none of its voices is playing and returns true. 18D5A0 then marks the pool
row stopped and frees it (`16E8A0`). The World's own `220C60` runs without
sound hardware, so its `21A250` returns early with `state == 2`, and a finished
one-shot was never freed. `PhysicsLiveWorldTu15::answer_finished_loop_sounds`
now lets the native lane answer at 18D5A0's two `220C60` call sites
(`18D7FA`, `18D93B`). `MultiplayerAudio::object_loop_finished` answers true
for a finished one-shot with no voice left. Campaign and the multiplayer host
both bind it.

So every scripted loop, one-shots included, now goes to the World's pool.
`sound_looping_start` no longer keeps one-shots on the host lane, and no longer
runs the host-lane `1901C0` workaround. The route test's score check follows
the pool. It expects the score's start, its body packets from the native lane,
and `18D2C0`'s stop request carrying the authored 10-second fade, in that order.

### 3D placement: the original HRTF kernels

Until now every voice played on fixed stereo routes with only the distance
gain, so world sounds were dead centre. On Xbox, `129790` hands each 3D voice
to the driver as a position in its own listener's head frame. That listener
is the one `127D00` selected, so split screen places each sound for its own
player. The SDK then:

- projects the position onto the horizontal plane (`380168`);
- measures the azimuth with its linear arctangent (`37DEF9`);
- picks that azimuth's kernel pair in 3-degree steps (`37E098`).

`3812E0` then loads both ears' coefficients and the interaural delay into the
APU's voice processor. Only surround and Dolby Digital setups fold rear angles
onto front kernels; stereo keeps the rear kernels.

`SourceSoundHrtfTu15` (`source_sound_hrtf_tu15.hpp`) runs those steps with
the existing native ports of the original routines, on the kernel table
imported from the executable (61 azimuths). Only the voice processor's filter
is replaced, by a plain 31-tap FIR per ear in the mixer (`SdlSoundHrtf`). A
changed filter takes over across 32 frames (64 before the seventh pass).

- **Coefficient format.** Each coefficient byte is a sign bit, a 3-bit exponent and a 4-bit
  mantissa. Of the readings tried, it is the only one under which the table
  behaves like a head. Straight ahead, both ears are flat (about 57.5 dB in
  table units). At 90 degrees the near ear is flat about 4 dB higher, and the far ear rolls
  off from 57 dB to 42 dB at 12 kHz. Plain two's-complement and sign-magnitude
  readings put the near ear below the far ear.
- **Delay.** Byte 31 is the ear's delay in 48 kHz frames: 12 at 30 degrees, 22 at 60,
  30 at 90. That is the spherical-head (Woodworth) delay.
- **Scale.** The voice processor divides each coefficient by 2048 (seventh
  pass). Until then taps were scaled so a source straight ahead had unity
  power gain; `HALO2_REFERENCE_HRTF_UNITY=1` restores that.
- **Which voices.** A voice is placed when it has a world position and is mono (every
  permutation decodes to identical channels; DirectSound 3D takes mono only).
  It must not use the direct channel (`0x800`) and must not be a four-bin combined loop, which
  keeps its original bins.
- **Listener frames.** Listeners now carry their camera's forward and up: campaign's render camera,
  each multiplayer player camera, and the object-audio views.
- **Comparison switch.** `HALO2_REFERENCE_CENTERED_3D=1` restores centred voices.

Probe, a mono authored sound (`macgun_fire`, Armory) 3 units from a listener
facing +x:

| Place | Delays (L/R frames) | Level |
|---|---|---|
| Right | 30 / 0 | right ear +4.0 dB |
| Left | 0 / 30 | left ear +4.0 dB |
| Ahead | 0 / 0 | equal |
| Behind | 0 / 0 | equal, 0.9 dB quieter (rear kernel) |

Mixer cost: 64 HRTF voices take 6.2% of one core in real time, against 1.9% dry.
The mirrored history buffer and eight-lane partial sums vectorize without
fast-math reassociation.

Not reproduced: elevation (the SDK ignores it too), the APU's exact
fixed-point filter arithmetic, and surround output.

### Fourth-pass results

| Run | Result |
|---|---|
| One-shot probe, Armory tram score `01a_01` | The score starts from the pool and plays its 46.6 s body once. The frame after its last voice ends, `220C60` answers finished and 18D5A0 frees the row. Starting it again 5 frames later creates a new row, and it plays again. |
| Full Armory to Cairo route | Passes all 17 checks. The score's start and its 10 s stop fade are both owned by the pool, and no host-lane music starts. |
| All 15 campaign levels, 1,200 ticks | No loop rejections and no audio faults. The same 7 levels pass as in the third pass; the other 8 stop on the known gameplay gaps. |
| Lockout, Zanzibar, Coagulation | Clean. |
| Audio CTest subset (332 tests) | 237 pass, including the new `original_sound_hrtf_tu15` and the extended `native_sdl_sound_platform`. The same 95 fail or are not run as before this pass: missing fixtures, no `unicorn` module, targets this configuration does not build, and the existing `multiplayer_audio_continuation_tests` link error. |
| Frame rate, HRTF vs `HALO2_REFERENCE_CENTERED_3D`, 900 ticks x2 | Spacestation 1,918 vs 1,784 mean frames; Delta Cliffs 3,285 vs 3,169. No loss beyond run-to-run noise. |

## Doppler, environment reverb and obstruction (2026-11-23, fifth pass)

### Doppler

1290A0 (impulses) and 21C400 (loops) add the sound class's Doppler factor
(+0x48) times 12A9D0's shift to a 3D voice's pitch, before the driver's
pitch-to-rate conversion. The device's own Doppler is switched off at
startup.

- **12A9D0.** It takes both velocities along the line from listener to
  source. Sound travels 111.548553 world units per second, the frequency
  ratio is held to 1/8..4, and the shift is `log(ratio) × 1731.234` cents.
- **Which voices.** World-located sounds are always hardware 3D
  (`189010`: priority `audible ? 3 : 1`), so every located voice takes the
  shift. The exceptions are direct-channel and combined voices, and loops
  without request source flag 2.
- **Velocities.** A source's velocity is the request's location +0x1C
  (impulse request +0x28). A listener's velocity is the change in its
  object-audio view's position over the game frame; 127400 takes the camera's
  velocity. A jump of more than 10 units, such as a camera cut, reads as
  standing still.
- **Implementation.** `source_sound_doppler_cents_tu15`
  (`source_sound_doppler_tu15.hpp`), applied in `MultiplayerAudio`'s voice
  parameters through the original pitch conversion.
- **Comparison switch.** `HALO2_REFERENCE_NO_DOPPLER=1`.

### Environment reverb

TU1.5's 18E970 (retail 18BB80) already selects the first listener's two
sound environments through portals each game frame. It uses 190A80 (portal
paths), 18D960 (portal angles), 129170 (the fades, retail 126660) and
18CDD0 (each environment's enter and exit cue sounds). It runs natively, but
129170 fades by the sound-frame time (sound system +0x1E0), which only the
hardware sound frame (125B40) sets. That time stayed 0, so environments were
selected but never faded in. `publish_native_sound_elapsed` now publishes
it before each object-audio step.

`native_sound_environments` reads the two slots: fade, portal span and the
snde record from +8. The record holds room, room HF, rolloff, decay, HF
ratio, reflections, their delay, reverb, its delay, diffusion, density and
HF reference.

The mixer then follows the original:

- **Reverb parameters.** Each slot becomes one of two I3DL2 reverbs
  (the DSP's effects 9 and 8) with 21ECE0's conversion: room levels clamped
  to −64..0 dB, reflections to −64..+10, reverb to −64..+20, and diffusion
  and density as percent.
- **Portal panning (21EE80).** Each reverb feeds four speaker submixes
  (FL, FR, BL, BR). A speaker's gain is its half circle's share of the
  environment's portal span, mapped by the span's offset and scale and
  multiplied by the fade. Stereo output folds each back submix into its side.
- **Room send (227BF0).** Every 3D voice sends to both reverbs at its class
  reverb gain (+0x10), mapped over −64..0 dB. The room HF level repeats the
  send.

The reverb was first a software I3DL2 model. The sixth pass below replaces
it with the DSP's own reverb program and DirectSound's parameter
conversion. `HALO2_REFERENCE_NO_REVERB=1` keeps voices dry.

Probe, Cairo's start: native 18E970 selects `snde 863d2402` (room −12 dB,
decay 1.49 s, reverb +4.4 dB). With the frame time published, it fades in
over its authored 2 s transition. A battle-rifle burst sends at its class's
−9 dB, and the reverb lifts the sample's own decaying tail by 1 to 3 dB,
fitting Cairo's fairly dry room. The software model's measured decay was
38 dB per second, a decay time of 1.5 s against the authored 1.49 s.

### Obstruction and occlusion

`source_sound_obstruction_tu15` is a native port of the original's
obstruction pass. It reads the resident structure BSP (`[0x4ECC18]`) and the
loop-sound globals' door scales (`[0x4F9B58]+0x28`):

- **Door values (221A70).** Each cluster on the far side of a door from the
  listener takes the door's scale (18CAC0); the largest door wins.
- **Per voice (221E30).** Obstruction is the share of the listener-to-source
  segment inside the structure, from two line tests. If the class or sound
  asks for it, a portal-distance estimate (249D60) is used instead. A source
  in the listener's own leaf is clear.
- **Portal terms (222150).** Across clusters, the room gain falls with the
  listener's distance from the portal of the environment nearest the source
  (18CFD0, 14B240), unless both clusters share a sound region. Occlusion is
  the source cluster's door value.

`MultiplayerAudio::obstruction_frame` then applies 129790/220E00. Each
result is scaled by the class transmission multiplier (+0x50); obstruction
and occlusion ease at the class bends per second (+0x54, +0x58); and the
voice's I3DL2 values follow 227BF0:

- **Obstruction** takes the direct path's high frequencies (LF ratio 0).
- **Occlusion** takes direct and room, a quarter as much below the HF
  reference (LF ratio 0.25).
- **The room gain loss** lowers the room send.

Cost: the line tests are the whole cost of the pass. On Cairo's dense
interior, a structure raycast takes about 16 µs, so 64 voices (128 rays) took
2.0 ms per game frame; the door values, point location and portal terms took
0.015 ms together. Each game frame therefore retests at most 16 voices,
stalest first, and the others keep their last thickness. Obstruction already
eases at the class bend rate, so a voice retested every few frames sounds
the same, and with 16 or fewer located voices every voice is retested every
frame.

The line test uses the structure's own collision (`Simulation::structure_collision`),
not scenery or devices, matching 1DE630's structure BSP. Doors act through
the audibility door values instead. `HALO2_REFERENCE_NO_OBSTRUCTION=1`
leaves voices unobstructed.

### The global sound environment

`set_global_sound_environment` (HS 878) is called on Xbox only by Cairo's
`dck_mission` and `lvr_mission`: `1 1 0 0 1200 5` for the vacuum moments and
`1 1 1 1 8000 0` to restore (MCC changed 1200 to 1000; the vanilla profile
restores it). Outskirts' five calls in `chief_recovery_sequence` are MCC
additions that its vanilla profile removes. Its evaluator stores four gains, a
DLS2 filter cutoff (Hz) and a resonance (dB) in the loop-sound globals
(`+0x228..+0x23C`). The World runs it, and `native_sound_global_environment`
reads the result each game frame. 2279E0 clamps the gains to 0..1, and
227BF0 applies them to every 3D voice: the direct level (a), its HF level
(b) and the room send's base (c). The cutoff and resonance become the voice
processor's DLS2 filter (see the seventh pass). At the defaults
(1, 1, 1, 1, 8000, 0) nothing changes.

## The Xbox's own reverb, I3DL2 filters and crosstalk (2026-11-24, sixth pass)

The earlier passes approximated the reverb and the voice filters because the
original logic seemed unavailable. It is all in the executable:

- **The reverb program.** The TU1.5 `DSPImage` section holds the GP DSP's 15
  effect programs. Records 8 and 9 are DirectSound's I3DL2 reverb, and an
  earlier session had already ported them bit-exactly as
  `tu15_sound_ring_effect`, checked against original reference vectors.
  Fourteen history lines stream through 16-sample DMA blocks at 24 kHz:
  - five reflection taps feed four feedback lines (±0.4);
  - eight diffusion taps feed a four-line Householder network;
  - a four-line cascade feeds two damped long lines, with a cross-feed into
    the short network;
  - parameter changes glide the gains (0.971 per block for 336 blocks) and
    crossfade each moved tap out and back in over 16 blocks.
- **DirectSound's conversion.** `XAudioSetEffectData` (retail 37BA6F, type
  0xC) builds the record's parameter words 70..135 with 3838A4 and its
  helpers (383167, 383436, 3835E0, 3831B5, 3832D2, 38336C, 3837BA).
  `sound_i3dl2_reverb_words_tu15` is a native port:
  - delays at 24 kHz, rounded to four words and fitted into their lines;
  - the cascade scaled by a modal-density term from decay time and density;
  - one-pole damping filters giving 60 dB of loss over the decay time, with
    the HF ratio's extra loss at the HF reference;
  - diffusion's signed input gains;
  - output gains of 10^((reflections + room)/2000) and
    10^((reverb + room)/2000).
- **Per-voice I3DL2 filters.** DirectSound's 3D calculator (37AE57) turns
  each voice's DSI3DL2BUFFER into levels and filter poles with 38368B and
  3834F3 (`sound_i3dl2_voice_tu15`):
  - obstruction and occlusion split into a level loss (their LF ratio) and
    an HF loss;
  - each path's HF loss becomes a unity-DC one-pole low-pass,
    y = b·y + (1−b)·x, tuned to lose exactly that much at the listener's
    HF reference;
  - DirectSound's listener is its Generic default (3857B8: 5000 Hz, no room
    rolloff), which Halo 2 never replaces;
  - the room HF filter reads lRoom, never lRoomHF.
  These one-poles replace the earlier HF shelves.
- **Return path.** 21E4B0 creates eight FXIN voices on mixbins 15..22 (record
  9 writes 15..18, record 8 writes 19..22). Each plays into crosstalk mixbins
  6..9 at the volume 21EE80 sets. Effects 2 and 3 copy the I3DL2 send
  (mixbin 10) into the records' inputs (mixbins 11 and 12). A return
  therefore plays its reverb one 32-sample block late.
- **Crosstalk.** Halo's 3D buffers use mixbins 6, 8, 7, 9 and 10 (220A70).
  Effects 4..7 pass mixbins 6..9 through: their feedback gain is 0 unless a
  sound-effect preset (21F960) changes it, and 191550 restores it. Record 10
  then adds to each side a one-pole low-pass (pole 0x7645A1, gain 0xF88313)
  of the other side's output 11 samples earlier, and writes mixbins 0/1/4/5.
  Its flags keep processing on, and DirectSound never touches it. At low
  frequencies its feedback gain is −0.77, so a centred bass sound is about
  5 dB lower and a hard-panned one louder on its side. The stereo setup
  disables the rear (37BE6C sets the front/rear flag only for surround or
  AC3), matching the captured 37DFE1 call (front 0, rear −10000).
- **Stereo output.** The EP's encoder adds each back channel to its side at
  0x5A8279 (−3 dB), not at unity as the earlier fold did.

The mixer now runs all of this:

- Each environment slot owns record 9 or 8 as the DSPImage installs it.
- `SdlSoundPlatform::reverb` commits DirectSound's words when a slot's
  parameters change.
- Every 32-sample block runs both records with `process_admitted`, a new
  entry point that skips the per-block admission copy, so the audio thread
  never allocates.
- HRTF voices and the reverb returns sum into mixbins 6..9; record 10's
  recursion runs per sample, with no extra latency; the back pair folds in
  at −3 dB.
- `HALO2_REFERENCE_NO_CROSSTALK=1` (or `SdlSoundPlatformConfiguration::crosstalk`)
  leaves the 3D mixbins unprocessed.

### Sixth-pass results

| Check | Result |
|---|---|
| 3838A4 on the Generic listener vs the DSPImage's installed words for record 8 | 59 of 66 words identical. The image's tool designed the room HF filter at 24 kHz (DirectSound here uses 48 kHz), rounded the short delay one 4-word step lower (moving the last reflection tap with it) and rounded the cascade damping gain one unit higher. |
| Records 8/9 with converted words, impulse decay | T60 0.50 s, 1.51 s and 3.04 s for decay times 0.5, 1.49 and 3.0 s. Room −1000 mB lowers the tail by 10.0 dB. |
| Record cost | 3.9 µs per 32-sample block per record: about 1.2% of one core for both. |
| 38368B/3834F3 | The poles lose the designed HF level at 5000 Hz within 10 mB. Room HF follows lRoom. |
| Mixer crosstalk vs the record 10 port | Identical within 1e-5 over 512 samples of an HRTF voice. |
| Unit tests | `original_sound_hrtf_tu15`, `native_sdl_sound_platform`, the ring-effect reference vectors and guards, `multiplayer_authored_audio`, campaign audio, timeline and continuation tests all pass. |
| Armory → Cairo route | All 17 checks pass. |
| All 15 campaign levels, 1,200 ticks | The same 7 levels pass as before; no new faults and no loop gaps. |
| Lockout, Zanzibar, Coagulation | Clean. |

## The voice processor's own filter semantics (2026-11-24, seventh pass)

The sixth pass left three hardware questions open. DirectSound's code, the
game's own coefficient helpers and published retail-console measurements now
answer them.

### Filter modes on a 3D voice

381710 (SetFilter) writes the mode to CFG_MISC bits 16..17, the DLS2 words to
TAR_FCA and, on a voice with I3DL2 data, the I3DL2 poles (38368B's direct and
room poles) to TAR_FCB, whatever the mode. On a 3D voice:

- mode 1 is DLS2 plus I3DL2;
- mode 2 is ParaEQ plus I3DL2;
- mode 3 is I3DL2 alone.

The sources are xemu's register notes and the xboxdevwiki APU page. The mode
list explains why DirectSound promotes only a bypassed 3D voice to mode 3: mode
0 would drop the I3DL2 filters. Halo's mode 1 (2211E0) therefore keeps the
I3DL2 one-poles that the mixer already applies.

### The DLS2 filter

21E270 and 21E2D0 (TU1.5 220380/2203E0) are the coefficients of a Chamberlin
state-variable low-pass:

- cutoff word = 4096 log2(2 sin(pi fc / 48000)), that filter's f in log2;
- resonance word = 32768 x 10^(-dB/20), its damping q (32768 is q = 1).

The filter's gain at fc is 1/q, so "resonance" is the gain at the cutoff, as
in DLS2. 8 kHz is f = 1, the top of the range. The voice processor applies
these words as follows (retail-console impulse responses, xemu pull request
3105):

- update h = x - l - q·b, b += f·h, l += f·b, with no input scaling or
  waveshaping;
- round each step to 2^-19, halves up;
- take f to a 1024th of an octave, then to the nearest 1/65536, at most FFFFh
  (word D97Eh gave 3043h);
- use q = FC1/32768 unclamped.

The mixer now does the same (`sound_dls2_frequency_tu15`,
`sound_filter_round_tu15`) and filters the whole voice ahead of its I3DL2
direct and room paths. The native word helpers (`sound_dls2_cutoff_tu15`,
`sound_dls2_resonance_tu15`) equal the original helpers on all 8,992 captured
inputs.

| Setting | Result |
|---|---|
| Halo's default (8000 Hz, 0 dB) | Words 0 and 8000h: f = 65535/65536, q = 1. Flat within 0.0008 dB, so it is skipped. |
| Cairo's vacuum (1200 Hz, 5 dB) | Words D540h and 47FBh: +5.0 dB at 1200 Hz, then about 12 dB per octave (-14 dB at 3 kHz). The earlier Butterworth stand-in peaked at +2 dB. |

### The HRTF stage

3812E0 writes the kernel table's bytes to the voice processor unchanged:

- 15 HRIR words of left, right, left, right taps;
- then HRIR_X, which holds tap 30 and the delay.

The delay is one ear's integer delay byte shifted to s6.9: positive delays the
left ear (source right), negative the right. The retail-console measurements
give the coefficient as sign, 3-bit exponent and 4-bit mantissa over 2048:
DC through single taps gave 01h -> 1/2048, 40h -> 1/16 and 7Fh -> 31/32. That
is the format the fourth pass inferred from the table. The taps are applied
as written, and the HRTF headroom register had no effect. At that scale:

- the loudest of the 122 kernels peaks at -3.8 dB at any frequency, the
  headroom of a fixed-point FIR designed not to clip;
- a source straight ahead (the same bytes as DirectSound's no-3D kernel at
  3869C0) is flat at about -8.7 dB per ear, -11.9 dB in power.

Halo removes the headroom that could offset this: its sample voices are
streams with headroom 0 (2AE660), DirectSound gives 3D voices none (37CDCA),
and every mixbin's is 0 (21E4B0). A placed voice therefore plays about 9 dB
(mid-band) below the same sound unplaced, which is about 12 dB lower in power
than the fourth pass's unity-power scale. `HALO2_REFERENCE_HRTF_UNITY=1`
restores the old scale.

A changed HRTF takes over linearly across one 32-sample frame. Moving a tap
from 7Fh to 40h read 0.969, 0.742, 0.516, 0.289 and 0.091 of DC at samples
0, 8, 16, 24 and 31, exactly n/32 of the way. The mixer's 64-frame fade is
now that 32-frame ramp. How the delay moves was not measured.

### I3DL2 routing

The measured one-poles are y = (1-a)·x + a·y', with a = FCB 15:0/65536
(direct) and FCB 31:16/65536 (room), rounded like the DLS2 filter. That is
the mixer's form; it now rounds the same way. The measurement write-up says
the room path feeds sends 5 and 7, but states no probe for it. DirectSound's
own default 3D mixbins (38589C) are 6, 8, 7, 9, 10, the list Halo uses
(220A70), and 380AE6 keeps that order, so the I3DL2 bin is send 4. The mixer
keeps the room filter on that send.

### About the measurements

They come from raddad772's retail-console probes (xemu pull request 3105,
still open at the time of writing, its code largely AI-written). The
portare-ch/hakuX-lr pull request 166 ports it and compares 770 hardware rows.
The probe outputs are published only as an archive of photos and were not
checked here. The coefficient format agreed with this port's earlier
independent reading of the table, and the DLS2 word D97Eh reproduces 3043h
exactly.

### Centered ambience behind doors

Cairo's cluster ambience reaches the native lane as centered object-loop
requests with location flag 0x1000. A probe of 600 ticks found clusters 97,
101 and 102, all with BSP 0. The location is the sound entry's bytes from
+0x18: its scale at +4, its cluster at +0x2C (entry +0x44) and its structure
BSP at +0x2E (entry +0x46).

For such a sound in the current BSP (4686C4, which 11C1B0 sets on a BSP
switch), 129100 asks for the portal estimate from its own cluster. That
request uses the zero point and no leaf, and skips the line test. 129790 then
multiplies the sound's scale by 1 - clamp(occlusion x class transmission
(+0x50), 0, 1). The owner's callback (127F10) rewrites the location every
frame, so this is a per-frame loss, not a decay. A sound left in another BSP
with no estimate goes to scale 0.

The native lane now does the same:

- `obstruction_frame` computes each named cluster's door occlusion against
  the first local view;
- object loops and phased owners ease their tracks toward the reduced scale
  (`centered_scale`, through `SourceLoopPhasedOutputTu15::scale`).

Two differences remained after this pass, and the eighth pass settled both:

- the loss reached the tracks one game frame late, because the pass runs
  after `object_loop_frame`;
- the other-BSP branch was not applied.

`HALO2_REFERENCE_NO_OBSTRUCTION=1` turns it off with the rest of the pass.

### Measured but not applied

- **Route gains.** The voice processor's route gains follow whole-dB nodes,
  then fall linearly to nothing between -48 and -63 dB, and ramp across each
  32-sample frame. The eighth pass applies them.

### Seventh-pass results

| Check | Result |
|---|---|
| Native DLS2 word helpers vs the 8,992 original helper results | Identical. |
| DLS2 stage vs its closed-form response (300, 1200, 4000 Hz at Cairo's setting) | Within 1%; +5.0 dB at 1200 Hz. Open words flat within 0.001. |
| HRTF stage | Taps at 1/2048; straight ahead -11.94 dB power; loudest kernel -3.8 dB; a changed filter n/32 of the way at frames 0..31. |
| Centered cluster source | Its occlusion is its cluster's door value (unit test); on Cairo, clusters 97/101/102 are estimated each frame. |
| Unit tests | `native_sdl_sound_platform` (+device failure), `original_sound_hrtf_tu15`, `original_sound_driver_spatial`, `multiplayer_authored_audio`, both campaign audio tests, all `original_loop_*` tests built here, the synchronization tests: pass. |
| Armory -> Cairo route | All 17 checks pass. |
| All 15 campaign levels, 1,200 ticks | The same 7 levels pass; the other 8 stop on the same script builtins and owners as before, with no audio faults. |
| Lockout, Zanzibar, Coagulation (900 frames) | Exit 0, no audio faults. |

## Levels, resampling, delays, door loss and surround (2026-11-24, eighth pass)

The retail-console measurements behind the seventh pass (xemu pull request
3105) cover more of the voice processor than its filters. This pass applies
the rest, finishes the centered door loss and adds surround output.

### Route levels

DirectSound's 380B97 builds each voice route's attenuation register:

- the route's total level in millibels: voice volume, mixbin volume and the
  3D terms, each clamped to -10000..0;
- times 64 over 100, truncated, at most FFFh (1/64 dB units).

The voice processor turns the register into a level in 1/16384:

- straight lines between whole decibels, through round(4095 x 10^(-k/20)) x 4;
- from -48 dB, a straight line to nothing at -63 dB;
- across each 32-sample frame, a move from the last frame's level to this
  frame's, each sample's rounded down;
- a new voice fades in from silence over its first frame.

`sound_route_attenuation_tu15` and `sound_route_gain_tu15` hold the two
steps. The mixer converts each route once, from the sum in millibels of:

- the voice attenuation and the route gain;
- the direct I3DL2 level;
- for an HRTF voice, DirectSound's front or rear level.

The room and effect sends follow the same rule. 21EE80 sets the reverb
returns with `SetVolume` (millibels truncated, at least -6400), so they ramp
the same way; the earlier per-sample glide toward their gains was invented.

The difference from exact conversion is at most 0.014 dB above -48 dB. Below
it, the level ends up to 1 dB louder (about -54 dB for -55 dB), then falls
away to silence at -63 dB instead of -100 dB.

### Resampling and pitch

The voice processor resamples with 8-point Lagrange interpolation:

- each weight is rounded to 1/65536 at 256ths of a sample; at half a sample
  they are 1225, -245, 49 and -5 over 2048 on each side;
- between 256ths the weights are interpolated linearly, which the write-up
  marks as assumed;
- the cursor steps in 1/65536.

A pitch change moves across the frame in its log form (4096ths of an
octave): pitch 0 to -1000h advanced the cursor 23 samples in that frame.

The mixer now resamples this way (`sound_resampler_weights_tu15`). One
difference remains:

- the Xbox resamples a sound's native 22, 32 or 44 kHz data in one step;
- the imported MCC sounds arrive as 48 kHz Opus (`mcc_audio.cpp`), and
  retail ADPCM is converted to 48 kHz by libswresample;
- so only the pitch's relative step runs through the interpolator.

### HRTF delay and DLS2 frequency

The write-up's filter moves the signed delay with the taps, linearly across
the frame, reading between neighbouring samples. The mixer's crossfade of
two whole filters is replaced with that. On a linear input, the left ear
lags exactly n/4 samples at sample n of a move from 0 to 8.

A changed DLS2 frequency also moves linearly across the frame.

### Door loss, finished

In the original, the loss reaches the voice's gain in the same frame:

1. 21BF00, the callback of a loop's sounds, copies the owner's location,
   scale included, into each sound every frame.
2. 129790 lowers that copy.
3. 222E10 (retail 21C400) hands the copy's scale to the gain, 127CF0, in the
   same frame.

The track's slewed control (TU1.5 221C3C, inside retail 21B0E0) sets only the
pitch. The seventh pass slewed it toward the lowered scale instead.

The native lane, in both the object-loop lane and the phased owners, now:

- gives each loop voice its sound's scale as its gain control
  (`centered_scale`);
- slews the track's control toward the unlowered scale;
- re-applies the gain of a centered 0x1000 loop's voices once
  `obstruction_frame` has estimated its cluster, in the same frame;
- reads the loaded BSP from 0x474C24 (TU1.5's compare is at 12C595; retail
  uses 0x4686C4);
- estimates only sounds in the loaded BSP;
- silences a centered 0x1000 sound left in another BSP, as 129790 does.

### Surround output

Halo runs DirectSound's "light HRTF, 4 channel" mode: 37E126 installs 37DFE1
as the front/rear owner. 37BE6C enables the split only for surround or AC3.
With rear speakers:

- 3812E0 folds the rear kernels to their mirrored front ones;
- 37DFE1 splits each 3D voice by azimuth between the front (mixbins 6/7) and
  back (8/9) crosstalk pairs;
- close to the listener the split moves toward even. The threshold is half
  a metre: the listener distance factor (3.048, set by 21E4B0) times the
  buffer's (DirectSound's default 1.0, from its template at 385850) times
  the horizontal distance.

The mixer now keeps four speakers (mixbins 0, 1, 4 and 5):

- a stereo device gets the back pair folded into its sides at -3 dB (the
  EP's 0x5A8279);
- a device with a back pair plays it on its own speakers. Those are the
  quad, 5.1 and system-default choices of the existing speaker setting: SDL's
  quad back pair, 5.1's surrounds and 7.1's sides.

Combined loops (2216D0's four mixbin volumes) now keep their back bins. They
used to add them to the front at unity; on stereo they now fold at -3 dB, as
on the Xbox. Halo sends nothing to the centre or LFE on these paths.

`HALO2_REFERENCE_IDEAL_VOICES=1` restores exact levels that change at once
and linear interpolation. The reverb returns keep exact gains there too.

### Eighth-pass results

| Check | Result |
|---|---|
| Register and curve | DirectSound's conversion (0, -1, -600 and -10000 mB) and the curve at 0, 0.5, 1, 48, 55 and 63 dB match the measured rule. |
| Resampler weights | At half a sample they are -5, 49, -245, 1225 (each side) over 2048. |
| Mixer, voice processor on | A new voice fades in over its first frame, and a level change moves across one frame. |
| Pitch ramp | Rate 1 to 2 advances the cursor by the sum of floor(2^(n/32) x 65536)/65536. |
| Resampling | Three-quarter-speed output equals the Lagrange filter applied to the PCM. |
| HRTF delay | On a linear input the left ear lags n/4 at sample n of a move from 0 to 8. |
| Back pair | Stereo folds it at 0x5A8279. A quad device (SDL's dummy driver) plays it on its own speakers, and an HRTF voice with rear 0 and front -10000 plays only there. |
| Surround HRTF (unit test) | Rear kernels fold to the front ones, front and rear split by azimuth, and a source at the listener spreads to both. |
| Cairo, 600 ticks | Estimates still run for clusters 97, 101 and 102, so 0x474C24 matches the ambience's BSP field (0). |
| Unit tests | 23 audio CTests pass; 3 skip without MCC fixtures. `multiplayer_audio_continuation_tests` fails to link on the mono-effect parameter code that `multiplayer_audio.cpp` already used, as before. |
| Armory -> Cairo route | All 17 checks pass. |
| All 15 campaign levels, 1,200 ticks | The same 7 levels pass. The other 8 stop where they did before, on script builtins and missing native owners. |
| Lockout, Zanzibar, Coagulation (900 frames) | Exit 0, no audio faults. |
| Mixer cost (48 voices, half pitched HRTF voices) | 12.4% of one core on SDL's audio thread, against 10.1% with the voice-processor model off. |

## Native-rate playback and the last door-loss cases (2026-11-24, ninth pass)

### Native-rate playback

The voice processor reads a sound's own samples (the rate its definition
names) and steps its cursor by the pitch register, 2^(pitch/4096) samples per
output sample, through the 8-point filter. The imported PCM is 48 kHz and
most of the native lane counts in 48 kHz frames, so the mixer now rebuilds
the native samples instead of re-decoding:

- each native sample is a Blackman-windowed sinc low-pass at the native
  Nyquist over the 32 PCM frames around its position, normalised to unity
  DC. Positions repeat every 147 samples (22050 and 44100 Hz) or 2
  (32000 Hz), so each rate keeps one table;
- the native lane sends each voice's sample rate and pitch register
  (`SdlSoundNativeRate`): the 48 kHz rate in 4096ths of an octave plus
  221B90's base for the rate (`source_sound_pitch_base`);
- the mixer keeps the voice processor's own cursor in native samples and
  derives the 48 kHz cursor, which packet completion and playback positions
  still use, from it. An unpitched 22050 Hz sound therefore plays 0.01% fast
  and a 44100 Hz one 0.008% slow, as on the Xbox.

A probe of Cairo (600 ticks) and Delta Halo (900 ticks) found every voice at
rate index 1, 44100 Hz. Content above the native Nyquist (22.05 kHz there) is
removed, and the 8-point filter's droop and images appear as on the
hardware. Rate index 3 (48 kHz) has no pitch base of its own in 221B90 and
keeps the 48 kHz path.

### Door loss: the listener, one-shots

- The listener: for a sound that is not world-located, 127D00 picks the first
  active local player, which is the first view the estimate already used.
- One-shots: 128CA0 takes a one-shot's gain from its location's scale every
  frame, so a centered 0x1000 one-shot loses its door occlusion (or falls
  silent in another BSP) like a loop. Such a voice now keeps its location.
  Its cluster joins the frame's estimates, and its gain is refreshed with the
  loops'.

### Listener-relative sounds

A type-2 location holds its position in its listener's frame, Halo's
forward/left/up axes. 129790 hands it to the driver without 142700's world
transform, an obstruction estimate or Doppler. The native lane now plays such
World requests instead of rejecting them:

- the position becomes the SDK's head frame (right = -left, up, forward);
- its length is the distance;
- it gets the same HRTF and I3DL2 paths as any 3D voice.

### Surround and playback references

- 37E112, the centre/LFE owner of Halo's DirectSound mode, always returns 0
  and -10000, so 3D voices never reach the centre channel.
- Flag `0x100` makes `223B20` read the explicit playback interface at
  request `+0x98`. The scheduler also checks the sound's default playback
  data when that bit is clear. No World request in the probes or the
  15-level sweep carried `0x100` or a type-2 location.

### World request flags

The World observer reads the caller's `0xA0` bytes at `18CEFC -> 223B20`
before the scheduler changes them. The original caller initializes the
request flags at `18CE16..18CE1D` to `0x5` or `0x25`, according to its
callback pointer. It then sets or clears `0x8` for an object owner and
`0x100` for an explicit playback interface. `18CED7` adds `0x400` for a
valid forced selection. These are all the writes to that flag word on
the way to `18CEFC`, so this observer cannot deliver a request with
`0x10` set.

Inside `223B20`, the effect-binding call at `223BBA` reaches `224670`.
That owner can create a controller with `2204A0` and then write the
controller owner to request `+0x54` and set `0x10` at `2246F0..2246F6`.
This happens after the observation point and belongs to playback-effect
controller handling.

A Unicorn probe under `python3 tools/run_test.py -- .tools/python/bin/python -B -`
ran the original `18CDD0` caller and `228220` class lookup with and without
an object owner, callback, explicit interface and direct-route class flag.
It covered location types 0–2, a valid forced selection, either invalid
selector byte and a missing selector. At entry to `223B20`, the 192 cases
produced all 16 flag combinations described above and zero instances of
`0x10`. The owned XBE SHA-256 was
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

### Ninth-pass results

| Check | Result |
|---|---|
| Native playback (unit test) | A 22050 Hz voice at register -4597 advances 2^(-4597/4096) native samples per output sample. A 500 Hz tone keeps its shape within 0.002, and a 16 kHz tone is removed. |
| Unit tests | The same 23 audio CTests pass; 3 skip without MCC fixtures. |
| Mixer cost (48 voices, half pitched HRTF voices) | 11.0% of one core at 44.1 kHz native and 9.9% at 22.05 kHz, against 7.2% for the 48 kHz voice-processor path and 5.6% with it off (a quieter machine than the eighth pass's figures). |

## SDL mixer and live cue follow-up

The ninth-pass Armory replay consumed all seven tram lines. Their first
samples arrived 128–212 simulation ticks after the requests because the
SDL presentation clock had fallen behind the campaign clock. A live
script cue now keeps its mapped sample deadline within one simulation tick
of the current device clock. Explicitly scheduled cues keep exact sample
deadlines, and overdue cues play from their first sample.

Native-rate reconstruction uses the same double-precision taps and
accumulation order for contiguous PCM and across packet boundaries. The
platform regression compares mono and stereo at 22.05, 32 and 44.1 kHz with
different packet and callback sizes. A separate comparison against the
previous mixer matched rendered samples, playback clocks and completion
records byte for byte, including rate changes, filters and room sends.
In a seven-run 48-voice benchmark, the median mixer thread CPU cost fell
from 16.83% to 10.23% of one core at 44.1 kHz and from 14.99% to 9.13% at
22.05 kHz.

The six SDL tests passed in Release and with ASan/UBSan, with leak
detection disabled for the sandbox. The original Prophet fixture confirmed
337,600 presentation frames between cues. The prepared Lockout continuation
suite passed, including an explicit `0x100` World request that produced the
same radio distortion PCM as the existing explicit-effect API. The offscreen
Armory replay reached native player startup, where hardware-interface
enumeration was denied by the sandbox.

## Remaining parity work

- **Playback controllers.** Explicit World requests can select the MCC
  globals or scenario collection and use the existing mono-distortion
  send. Default custom playback and the controller's companion sounds
  remain open. The installed MCC sound-owned rows contain no DSP instances.
  The original companion phases use the admitted parent's updated
  location, per-voice distance bounds and retirement reason.
- **Rebuilt samples.** The Xbox read the definitions' own ADPCM samples. The
  native samples here are rebuilt from MCC's 48 kHz Opus, which is lossy and
  not the same data. Between 256ths of a sample the resampler's weights are
  the measurement write-up's assumption.
- Gameplay faults that stop levels outright (unsupported builtins such as
  `cinematic_clone_players_weapon`, missing native owners) are outside audio.
