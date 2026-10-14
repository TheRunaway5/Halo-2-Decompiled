# Original Backwash swarm particles in the native world renderer

`SourceWorldParticleDrawsTu15` selects placed effect systems that actually use
the authored type2 movement controller. The 21-map resource audit activates
this consumer only for Backwash. It reads source-created particles and submits
their authored sprite/material data to the existing native GPU renderer. It
does not implement another swarm simulation or change particle size, color,
placement, contacts or visibility to make the capture pass.

The original frame order matters. `13AA28 ->179DF0` precedes game ticks at
`13AAFF` and the later `2B630` render-visibility reset. `1778D0` reads the
preceding render's `4D1EE0` cluster union and retires invisible locations through
`178290`. Advancing after `begin_model_frame()` cleared that union and repeatedly
destroyed the match's particle locations. The corrected native loop advances
before game ticks, then starts the new render frame. Its particle world owner
also remains independent of vehicle resources; Backwash has no vehicle models.

The graphics consumer uses the actual `1E0F0` transparent-storage constructor,
native world-pass count boundary corresponding to `40EC2/40EC8`, complete
`1373F0/136EA0/1770C0` view/admission owners, and their queued system/location
pointers, sort depth and bounds. It resolves full salted source identities and
feeds the [original pool/render-input bridge](SOURCE_PARTICLE_SPRITE_CONTEXT_TU15.md).
The native backend supplies the world pass and camera inputs. This is not a
claim that the complete original Xbox graphics context or outer frame owner
has been recovered.

The original controller's five bodies/668 instructions remain unchanged.
Untouched host x86 execution agrees with its captured source-created call over
58,970,198 non-private-stack bytes. The whole original1784B0 producer also agrees
with14 callback inputs from actual source pools. The new match-world test runs
240 frames and240 repeated views, retains14 final draws, keeps the queue at
two commands, and verifies that repeated views neither advance particle records
nor consume either retained source RNG word.

Ordinary source startup still obtains its auxiliary seed through the original
startup algorithm and real native clock inputs. Finite local script captures
now deliver the existing `--mp-reference-auxiliary-seed` input to the constructed
source auxiliary word before scenario factories consume it. The native
application's legacy visual channel receives the same explicit replay input.
Both final source words are recorded. This makes the capture's inputs explicit;
the application/source stream handoff across all native consumers remains open.

Accepted pixel counts and the installed runtime are recorded in
[the launcher delivery receipt](SOURCE_WORLD_LAUNCHER_DELIVERY.md). A camera-only
one-view comparison and a four-view comparison using declared native look
controls establish visible particle submissions. Ordinary four-view initial
cameras have no visible glowbugs and are retained as a zero-pixel case. None of
these captures relocates an object/effect or bypasses the one-player camera
override restriction. Exact enabled repeats and source/native RNG agreement
are required, with zero Vulkan validation errors.

The legacy NativeWorldEffects path continues to reject controller2; this new
consumer handles the original particles separately. Full glowbug effect graph
light/lens ownership is not complete, so whole-graph admission remains93/96.
Independent network-client particle/PVS history, complete application RNG
handoff, resets, all map interactions, Vista-only maps, native MCC gameplay and
Xbox framebuffer parity remain open. Host rendering evidence does not close
those gates.

Evidence: `analysis/map-world-features/swarm-lifecycle-20261014/live-swarm/`.
The vehicle-gated and premature-visibility-reset candidates, uncontrolled
source-RNG repeat mismatch and invalid terminal input row remain retained.
