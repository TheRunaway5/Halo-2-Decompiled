# Halo 2 voice behavior and native integration evidence

Research date: 2026-10-10. This is an implementation evidence map, not a claim
that native voice chat is implemented or that the retired Xbox voice protocol
has been reproduced. The recovered Xbox routing, Vista presentation assets,
and replacement native service choices are separate authorities.

## Primary sources and platform difference

- Microsoft original Xbox Halo 2 manual, printed pp. 22–23:
  [official PDF](https://download.microsoft.com/download/f/9/9/f99ab8f0-5191-4edd-b312-7a9b9e4784fa/halo2_covenant_mnl_us-en.pdf).
  Downloaded directly from Microsoft and read with `pdftotext`; the browser
  tool rejects its octet-stream MIME type.
- Original Halo 2 Vista manual, printed pp. 24–27 and keyboard reference:
  [archived original PDF](http://files4.bungie.org/Halo2GameManual_en.pdf).
  This is the authored manual, hosted by HBO. The HTTPS endpoint has a
  hostname/certificate mismatch; the public HTTP PDF downloaded successfully.
- Bungie's September 17, 2004 Q&A, Drox question:
  [archived first-party text](https://halo.bungie.org/halo2updates/search.html?date=20040917).
- Bungie's December 3, 2004 Q&A, Fock question:
  [archived first-party text](https://wiki.halo.fr/Chronique:BWWU_03/12/2004).
  This is a mirror of Bungie's update, not an independently sourced wiki claim.
- Recovered client source:
  [network_voice.cpp](../../tools/reference/halo2-decompiled/src/network_voice.cpp),
  [network_voice.h](../../tools/reference/halo2-decompiled/include/network_voice.h),
  [unknown_0574e0.cpp](../../tools/reference/halo2-decompiled/src/unknown_0574e0.cpp),
  [unknown_0662e0.cpp](../../tools/reference/halo2-decompiled/src/unknown_0662e0.cpp),
  [unknown_2c4e9c.cpp](../../tools/reference/halo2-decompiled/src/unknown_2c4e9c.cpp),
  and [unknown_2b116a.cpp](../../tools/reference/halo2-decompiled/src/unknown_2b116a.cpp).

**Xbox and Vista voice control semantics differ.** The Xbox manual describes
ordinary speech as proximity voice, including nearby enemies, with White
arming team radio to teammates anywhere on the map. Radio remains open until
speech stops. Bungie's September Q&A confirms this is a single press rather
than a requirement to hold the button, and is not a toggle that replaces
proximity voice. The December Q&A says nearby players can hear speech even
while team radio is used; the approximate radius is spherical, around 30 feet,
and walls/floors do not obstruct it. Volume conveys distance. These claims
come from the Xbox manual and the two dated Bungie answers linked above.

The Vista manual instead documents default speech to teammates anywhere on
the map, with N or the front left bumper opening speech to everyone until
speech stops. Its keyboard reference explicitly labels N as global voice
chat. It still describes nearby enemy eavesdropping. This is original Vista
behavior, not a reason to silently change the Xbox radio policy. The project's
current UI uses Xbox menu semantics with imported Vista HD artwork; voice
behavior needs the same explicit authority choice. The findings below describe
the recovered Xbox source. Vista artwork availability does not establish
Vista's runtime voice implementation.

Do not use [Bungie Weekly Update item 117](https://halo.bungie.org/bwu/index.html?item=117)
as Halo 2 evidence: it is March 9, 2007 and discusses Halo 3. In particular,
the commonly surfaced Back/right-stick/X mute procedure belongs to that
different game's update.

## Recovered Xbox radio, proximity and mute rules

The following function addresses refer to the recovered source, with unknown
field names retained where their complete meaning has not been established.
They are evidence of client policy, not proof of native runtime integration.

| Concern | Concrete recovered behavior | Authority |
| --- | --- | --- |
| Radio activation | `function_57620` arms from input `values[5]` or `values[8]` when the player is eligible. It starts radio effect 0. Input-to-physical-button ordinals still need the input decoder; Xbox White is independently established by the manual. | `unknown_0574e0.cpp`, `0x57620` |
| Radio speech hold | While armed, pressed input or `function_53750` talking status refreshes `last_talk_times`. After silence it plays radio effect 1 and clears the armed state. | `0x57620`; talking getter `network_voice.cpp`, `0x53750` |
| Silence and maximum | Default silence interval is **0.75 seconds** (`real16f4`); maximum arm duration is **10 seconds** (`real16f8`). At maximum it stops the radio effect and requires release before clearing/rearming. These are recovered defaults, not measured physical microphone thresholds. | `unknown_0662e0.cpp:830–831`; `0x57620`, `0x57790` |
| Team radio audience | Normal team games require equal, non-NONE teams; free-for-all allows the radio audience; Juggernaut has a specific both-non-Juggernaut rule. Preserve the game-type exception rather than reducing all routing to team equality. | `network_voice.cpp`, `0x57270`, `0x57c20` |
| Proximity candidate radius | Three-dimensional Euclidean distance must be at most `R + 1.5`, where `R` is runtime `motion_sensor_range`, with **8.0** fallback. There is no wall/occlusion test in this candidate computation. Current unit is used, with previous-unit fallback. | `0x57080`, `voice_proximity_object` |
| Speaker routing | Route 1 is headset, route 2 is nonspatial speaker playback, route 3 is positional speaker playback. When multiple local listeners share speaker output, proximity selects the nearest eligible listener rather than stacking the talker once for each local player. | `0x583c0`, `0x57eb0`, `0x58590` |
| Alive/dead handling | `function_589e0` treats the current unit, missing previous unit, or the first **1.0 second** after previous-unit death as eligible. `0x57c20` and `0x577d0` separately handle alive/dead categories. Do not replace this with a simple dead-player voice prohibition. | `0x589e0`, `0x57c20`, `0x577d0` |
| Muting and capability | Outgoing allowed masks remove local mutes, reverse mutes, and recipients with port flags 1/2. Playback also checks `unknown110` both ways. Shared speaker routes are suppressed if another local player blocks the talker, protecting split-screen mute choices. Voice account capability is tested separately. | `0x55990`, `0x57d60`, `0x541a0` |
| Talking indicators | Local status uses chat-data-ready state; remote status calls XHV `IsTalking`. Talking appearance must be driven by actual voice state, not clan membership or an unconditional icon. | `0x53750`, `voice_xhv_is_talking` |

Proximity attenuation is more specific than a guessed linear volume ramp.
`function_586f0` transforms the talker position through the listener camera,
calculates distance `d`, obtains directional gains, omits the smallest of five
speaker gains, and writes four mix-bin volumes. The recovered initial values
are distance volume `D = -80`, directional volume `A = -1000`, and linear
directional gains enabled (`voice_player_settings_initialize`, `0x56ae0`). Its
distance contribution is:

```text
distance_contribution = D*d/(R + 1.5)
if d > R:
    distance_contribution += (D/R)*(d - R)*150
mix_bin_volume = clamp((1 - directional_gain)*A + distance_contribution,
                       -10000, 0)
```

The receiver submits these DirectSound mix-bin volumes, not an ordinary
0-to-1 distance multiplier. The original directional helper is
`function_192d40`; a faithful native stereo output requires an explicit
mapping from those source speaker gains. Do not assert that an invented
left/right pan or rolloff curve reproduces this route.

## Session modes, XHV and limits of recovery

`voice_update_mode` (`0x544e0`) chooses mode 1 for an active playable world,
mode 2 in several other session-manager states, and mode 3 for states 7/8
when session data byte `0x48` bit 0 is set. `function_56c60` gives mode 2 the
session player mask; mode 3 gives only other players on the same valid team.
`function_56b70` computes world radio/proximity/death masks in mode 1 and
passive session routes in modes 2/3. This supports a distinct lobby/session
voice audience, but the full mapping of numeric session states to named
pregame, search, intermission and postgame stages has not been established in
this research. Do not choose a lobby restriction or all-party override by
assuming that state mapping.

The XHV engine's runtime parameters (`0x55780`) specify four local talkers;
engine mode 2 enables fifteen remote talkers and five compressed buffers.
Custom VAD is disabled, and `MicrophoneRawDataReady` returns `E_NOTIMPL`.
Therefore the recovered source relies on the platform speech detector; it
does not supply a native RMS threshold that can simply be copied. The
0.75-second radio silence timeout is recovered independently of that VAD.

The incoming channel reader works with ten-byte compressed voice units and
adds relay/member metadata, but XHV's encoding/decoding internals are opaque
here. This does **not** establish the codec name, bitrate, sample rate, frame
duration, encryption, NAT behavior or a working retail-compatible codec.
Do not claim a modern Opus stream is the original Xbox wire format.

The source contains a material recovery limitation: route selector
`function_565c0` is disabled under `#if 0`, with a comment explaining that its
source shape disrupts a caller-local initialization at matched `0x54890`.
[stubs/lane_d.cpp](../../tools/reference/halo2-decompiled/src/stubs/lane_d.cpp)
provides an empty stub at that address. Thus the recovered voice module is
not a complete, executable behavioral oracle merely because much of its
source is present.

## Authored voice UI already available

The existing [Xbox/Vista definition inventory](../../analysis/frontend/hd-ui-followup-20261009/vista-menu-extension-inventory.json)
contains these screens in both caches. The inventory documents definitions,
not live voice settings support.

| Screen ID | Authored tag | Header / list |
| --- | --- | --- |
| 53 / `0x35` | `ui\\screens\\game_shell\\settings_screen\\player_profile\\voice_settings_menu` | VOICE; two-row settings skin 2, positioned `(-254, 130)` |
| 54 / `0x36` | `...\\voice_mask` | VOICE MASKING; platform XHV feature, not implemented by an arbitrary Opus encoder |
| 55 / `0x37` | `...\\voice_through_tv` | VOICE OUTPUT; four-row default skin 0 at `(-106, -20)` |

Screen 55 pane 0 text index 0 has authored bounds `[top=72, left=-122,
bottom=-19, right=122]`, string `0x1a0017bd`. It explains the default split
between team headset output and proximity speaker output. Its settings list
maps rows directly to profile values 0–3 in `c_voice_through_tv_list`:

| Row/value | String ID | Original English |
| --- | --- | --- |
| 0 | `0x0d000304` | Mixed |
| 1 | `0x25000305` | Speakers Only |
| 2 | `0x21000306` | Xbox Communicator Only |
| 3 | `0x0e000307` | Disable Voice |

The labels were read from the actual local Xbox and Vista `matg` English
locale tables with the bounded inventory reader and match in both caches.
`function_57d60` preserves spatial speaker routing in value 1, forces headset
route 1 in value 2, and disables routing in value 3. A PC device chooser or a
replacement name for Xbox Communicator is a native extension; retain the
imported Vista high-resolution artwork and authored text style while making
any replacement semantics explicit. Do not expose voice masking as functional
unless its actual speech effect is implemented.

## Current native seams and missing parts

At this research snapshot, the native port has no microphone capture or live
network voice implementation. None of the existing SDL device opens uses
capture mode. This is a code inspection result, not a runtime audio test.

| Existing seam | What it currently does | Work needed for live voice |
| --- | --- | --- |
| [audio.cpp](../../src/audio.cpp), [audio.hpp](../../include/halo2/audio.hpp) | Playback-only SDL device; 48 kHz float stereo, 512 requested callback frames. Finite PCM sounds and sequence continuations. | Dedicated capture owner and bounded microphone PCM queue; codec/VAD work outside the mixer callback. Existing loop continuations explicitly must not block or decode. |
| [sdl_sound_platform.cpp](../../src/sdl_sound_platform.cpp), [header](../../include/halo2/sdl_sound_platform.hpp) | Playback-only 48 kHz stereo float packets, exactly two retained packet slots per voice, completion accounting, explicit volume and stereo route matrix. | A chat PCM feeder could use this native backend, while retaining a separate jitter/decoder owner. Do not graft chat onto authored sound cache datum/completion ownership. |
| [multiplayer_audio.cpp](../../src/multiplayer_audio.cpp) | Authored game sound orchestration through native playback voices. | The word “voice” here means a sound playback instance, not microphone chat. |
| [multiplayer_network.cpp](../../src/multiplayer_network.cpp), [header](../../include/halo2/multiplayer_network.hpp) | Native UDP protocol 25, 1400-byte packet ceiling; messages hello/input/snapshot/leave/reject. Explicitly not the retail System Link wire protocol. | No voice packet, room admission, media sequence, jitter buffer, encryption or speaker routing. Live voice needs its own bounded media transport and authoritative room membership. |
| [multiplayer_hud.cpp](../../src/multiplayer_hud.cpp) | `text_value` returns empty for voice/talking text (`flags & 8`), with an explicit missing-service comment. | Supply actual audible/talking player state and local screen name before enabling authored talking text/indicators. |
| [pc_settings.hpp](../../include/halo2/pc_settings.hpp) | Current PcAction contains movement/combat/scoreboard only. | No radio/voice action binding. Do not assume Vista N is an existing native binding. |
| [online_account.hpp](../../include/halo2/online_account.hpp) | Discord-authorized account and clan display snapshots; credentials remain in worker. | Clan membership is not a live party, game room or proximity audience. A clan-wide voice room would be a separate native feature, not recovered original match voice policy. |

The user's chosen screen name can remain the visible speaker label. Stable
service player IDs are appropriate internally for identity, mute entries and
room authorization, but should not be rendered; account login does not itself
establish a Discord voice connection. Opaque native session/media identities
and an authenticated room-ticket flow are replacement-service design choices,
not claims about the original Halo 2 server API.

## Libraries available and implementation boundary

[CMakeLists.txt](../../CMakeLists.txt) already discovers SDL2, Threads, CURL,
OpenSSL Crypto and FFmpeg `libavcodec/libavformat/libavutil/libswresample/
libswscale`. SDL's official
[SDL_OpenAudioDevice documentation](https://wiki.libsdl.org/SDL2/SDL_OpenAudioDevice)
supports recording by setting `iscapture` nonzero, either callback delivery or
queued capture/dequeue, and an obtained format. Native capture can therefore
use the existing SDL dependency, but must negotiate the real capture format
and handle device removal separately from playback.

The current host reports these `pkg-config` versions: SDL2 **2.32.74**,
libavcodec **63.1.102**, libavformat **63.1.102**, libavutil **61.1.102**,
libswresample **7.1.102**, and Opus **1.6.1**. `libopus.so` is installed, but
CMake does **not** currently discover or link Opus. Host installation is not
portable build/package coverage.

Opus is a candidate modern replacement codec, explicitly a native transport
choice. The official [1.6 encoder API](https://opus-codec.org/docs/opus_api-1.6/group__opus__encoder.html)
supports mono/stereo, 48 kHz float input and VOIP mode; valid 48 kHz frames
include 960 samples (20 ms). Its
[decoder API](https://opus-codec.org/docs/opus_api-1.6/group__opus__decoder.html)
supports float output, missing-packet concealment and optional forward-error
recovery. These facilities do not provide microphone capture, device choice,
echo cancellation, room admission, packet pacing, jitter scheduling or the
original XHV speech detector automatically.

Before reporting usable voice chat, meaningful validation must include two
real clients with capture-to-network-to-speaker audio, authenticated room
admission/removal, mute and disabled-voice behavior, speaker indicators, packet
loss/reordering, device removal, leaving/rejoining and room changes. Gameplay
parity additionally needs teammate/enemy distance boundaries, simultaneous
radio plus proximity, walls/floors, Juggernaut/FFA, death grace, and split-screen
speaker/mute rules. A codec unit test, generated PCM, one-process loopback or
passing frontend screenshots cannot establish physical microphone operation,
WAN/NAT traversal, subjective latency/clarity or original voice behavior.

Unknowns remaining: complete session-stage routing map, exact XHV VAD and
voice masking DSP, retail codec format, transport protection/NAT protocol,
stereo mapping of source surround gains, measured acoustic behavior, and the
preferred Xbox-versus-Vista voice control authority. Preserve these limits
until source or runtime evidence resolves them.
