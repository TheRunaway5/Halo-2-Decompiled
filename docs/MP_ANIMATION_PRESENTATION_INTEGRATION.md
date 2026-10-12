# Higher-refresh multiplayer animation presentation

The native renderer now samples the existing player idle presentation and the
registered single-SMG first-person skeleton between simulation updates. Gameplay,
source animation clocks, shot/reload events and callbacks remain at their existing
30Hz schedule. This is the user's requested native presentation enhancement;
it is not a claim that the Xbox renderer has the same higher-refresh owner.

The source boundary is recorded in
[the original animation audit](MP_ANIMATION_RENDER_INTERPOLATION_RESEARCH.md).
Original fractional animation codecs, tick-owned first-person manager updates,
and simulation-owned pose-transition buffers are distinct. The source's saved
transition bank is not substituted for an adjacent render-tick bank.

`MultiplayerMatch::step` copies the actual retained view after action selection
and registration, immediately before `first_person_animation_clock`. Rendering
consumes this exact input and its completed output. A callback-free copy of the
source clock must reproduce every represented hand field before fractional
sampling is accepted. This permits a newly selected reload/fire clip to start at
its actual frame0 without blending the preceding activity into it. The stored
hand, event masks and feedback timers are never advanced by rendering.

Changed full unit/weapon/graph/channel identities, snapshot gaps and clock rewind
snap. The same-idle completion reselection also snaps; its decreasing frame has
no retained source loop flag. A genuine retained loop uses the source threshold
`frame_count - float(38D1B717h)` before wrapping. The decoded sampler retains the
last authored frame rather than blending that key toward the first loop key.
The shared arms/weapon skeleton is sampled before camera-control inversion,
camera composition, marker IK, inverse binds and skinning. No world-space vertex
or matrix blending is introduced.

Third-person body idle sampling and the attached weapon's hand marker consume
the same fractional render time. Full player/unit identities guard position and
body interpolation. Authoritative object/query poses remain on simulation time.
The existing source-selected locomotion manager, secondary layers, full feedback
are still missing live producers. The later native LAN source-clock integration
is documented in [the LAN clock integration](MP_FIRST_PERSON_LAN_CLOCK_INTEGRATION.md). A decoded
idle presentation is not a replacement for those owners. Physical Xbox
reciprocal/packed-codec and complete raster equality remain separate limits.

The original first-person fixture is replayed at exact rational render cadences
30/60/120/144/240Hz. It retains541 original ticks,2,705 full phase states,four
callbacks and516 actual Match ticks. Both fractional endpoints are checked
exactly, salted replacement and snapshot-gap cases snap, and actual decoded
arms/weapon skins change at fractional samples under declared fixed preview
camera/aim inputs. Normal and address/undefined sanitizer runs pass. The final
native rendered SDL probe observes the actual submitted posed vertices and
independent split views; its logs are retained with the isolated test build.

The checkpoint is `build-kbm-20261008-184611`. Its source manifest explicitly
records the prior verified `frontend.cpp` used for title startup: the separate
current menu work stops at an unresolved plasma-rifle gauge shader input. The
working-tree menu implementation is preserved. Original maps/assets/update data
remain external to the code snapshot; this Linux checkpoint reads the supplied
1.5 update data and does not execute or redistribute that executable.
