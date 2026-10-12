# Native SDL packet playback service

The new `SdlSoundPlatform` supplies an actual host device, independent streams,
explicit parameters and ordered PCM packet lifetime for the original CPU sound
owners. It is registered as `halo2_sound_platform` and publicly linked by
`halo2_systems`. No live multiplayer consumer or source availability flag is
installed by this checkpoint. The generic menu `AudioEngine` is unchanged.

The source authority is the frozen [platform voice contract](MP_SOUND_PLATFORM_VOICE_CONTRACT.md)
and its authenticated paired1.1/u5 bodies.2215F0 declares two attached packets;
2B4B30 requires actual input acceptance;222840 accounts a selected chunk for
every completion status and only pumps another packet after success. The service
owns exactly two retained slots per stream. It does not choose sound ranges,
permutations, listeners, class policies, cache pins or a source voice limit.
Private host voice identities remain distinct from source datums.

## Actual host adaptation

Construction calls `SDL_InitSubSystem` and `SDL_OpenAudioDevice`, requiring an
actual nonzero device with a48kHz stereo float callback format. The caller
explicitly supplies callback buffer size, native delivery mode and optional
device name. SDL provides any hardware format conversion when allowed changes
are zero; this is documented [SDL device behavior](https://wiki.libsdl.org/SDL2/SDL_OpenAudioDevice).
The real device starts paused and callback delivery is enabled only for the
explicit device-callback mode. Manual delivery still creates a real paused
device and uses the same mixer.

Every voice requires the original computed positive binary32 relative rate,
signed attenuation in hundredths of a dB, explicit pause state and supported
native stereo routes. No unity rate/gain, implicit pan, guessed Xbox mix-bin
translation or unsupported effect substitute is supplied. The range−10000..0
and attenuation units follow [the driver volume contract](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee418150(v=vs.85)).
Native amplitude uses `10^(attenuation/2000)`, then the provided channel matrix.
Linear PCM interpolation, ordered double-precision accumulation and final
float output saturation are explicit native mixer services. They are not Xbox
DSP bit, resampling, latency or physical speaker parity claims.

The decoder supplies owned stereo48kHz PCM, an explicit frame interval and a
nonzero caller packet token. The backend does not infer a compressed byte/frame
offset. Submitted PCM remains owned after consumption until the source caller
acknowledges the oldest completion. Pending and unaccounted completed packets
both retain a slot, so input acceptance expresses actual remaining capacity.
Partial consumption does not create success. Pause is independent per stream.
Flush completes pending packets with E_ABORT; a matching device-removal event
completes pending packets with E_FAIL. Already completed packets keep their
actual status and order. A stream cannot be destroyed with unaccounted packets.
Final platform destruction joins the device callback before releasing abandoned
native storage; normal source teardown must flush/account its streams first.

The callback allocates nothing, releases no PCM backing and invokes no source
callback. Client changes and manual mixing use the device lock; SDL already
locks its real callback, as documented in [SDL callback locking](https://wiki.libsdl.org/SDL2/SDL_LockAudioDevice).
Completion and acknowledgement run through the source owner on its own thread.
The class neither advances source sample clocks nor signals source scheduler
admission. No32-voice cap, stealing, unsolicited looping or permutation choice
is introduced.

## Validation and limits

Production GCC normal and ASan/UBSan checks pass `native_sdl_sound_platform`,
`native_sdl_sound_platform_device_failure` and `owned_sdl_sound_reload_packets`.
The native controls cover two-slot backpressure, partial consumption, retained
PCM through ordered accounting, flush after a real completed packet, independent
pause, explicit swapped/multiple routes, eight relative rates with cross-packet
interpolation,80 simultaneous private streams, absent/invalid providers, and
device errors. The dummy driver's actual callback consumes both queued packets
after explicit resume. It retains PCM until both completions are acknowledged.
An unavailable SDL driver causes actual construction failure and no handle.
These are native service controls, not fabricated accepted original voices.

The actual owned Lockout reload datumE5AD0438 with explicitly provided decoder
indices0/0 yields73422 stereo48k frames. The production Cache/decoder feeds two
explicit native packet intervals. Every one of146844 unity-matrix output samples
matches decoded PCM after documented output saturation; the output is nonzero.
Both packet tokens complete in order and backing survives until accounting.
This decoder control does not implement live source permutation selection,
admission, listener gain or audible physical output.

Isolated Clang normal and ASan/UBSan checks exercise the same native callback
and error controls. They are not full-project Clang validation. An initial test
used a fixed30ms delay and failed under compilation load; the retained preliminary
log records it. The corrected test observes a callback after the queued paused
state with a bounded timeout, then checks pause and consumption independently.

The source constructor/startup, map activation, local player/controller/RNG,
cache initialization and pinning,128380 admission, arbitration/factory, complete
221B90 routing/effect owner and live event consumer remain separate required
owners. Missing services still remain unavailable. No launcher, native game
screenshot, GPU output, LAN session or all-map audible SFX claim is made here.
