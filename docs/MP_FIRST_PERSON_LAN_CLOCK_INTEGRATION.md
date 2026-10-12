# Native LAN first-person animation clocks

The native LAN client now uses the SMG activity and channel state produced by
the real authoritative weapon callback, rather than choosing a fire or reload
clip from `shot_tick` and `reload_tick`. The original single-SMG clock and
registration authority is recorded in
[the source clock audit](SMG_FP_CLOCK_1_1_RESEARCH.md). Higher-refresh sampling is
the native presentation policy documented in
[the animation integration](MP_ANIMATION_PRESENTATION_INTEGRATION.md).

`MultiplayerMatch::configure_replicated_first_person` installs a separate native
presentation replica for each connected participant. Original full player,
current unit and current weapon identities select the recipient of an actual
weapon activity callback. Joined units receive their authored registration;
ordinary later unit replacement remains registered after the world/postpass.
The replica copies the post-action channel input, then calls the existing source
clock once per simulation tick. It never installs a local observer and never
sets `SmgState::hand_timer_registered`. This is native presentation replication,
not an original Xbox dedicated-server first-person owner.

Native protocol11 sends both endpoints of that completed clock step, including
graph, full animation identifier, delay/flags/options, event/future masks,
bit-preserved rate/frame, activity and fire feedback ticks. Original node maps
are rebuilt from the client's imported authored models and graph. The client
checks the received clip against those assets and reproduces the complete clock
transition on a callback-free copy before applying either endpoint. Invalid
pairs cannot partially update the retained view. No received animation state
changes gameplay, dispatches a shot or replays sound/effect callbacks.

The entire roster is still published atomically through bounded1400-byte UDP
datagrams. A lost packet does not publish half a clock. A newer complete
snapshot carries its own clock input/output, allowing recovery without an
invented activity history. An older missed snapshot is not blended toward the
new snapshot: the new pair contains its own validated consecutive source-clock
interval and can be sampled directly. Salted object replacement and actual
clock cuts retain the existing snap rule; body/position interpolation still
snaps when its previous snapshot is missing. Initial registration before the
first completed tick has no clock pair and retains its authored initial pose.
Older native clients must be updated with the host; this is not Xbox System Link.

The native high-refresh animation fraction uses the local fixed-step remainder,
independently of packet arrival. The four-view rendered probe exposed that the
position interpolation window restarted on every snapshot: below30FPS, every
draw received a snapshot and therefore sampled animation fraction0. That packet
window still owns position smoothing. A validated contained FP clock pair uses
the local render fraction, with the same identity/clock cuts and no extra event
dispatch. Missing third-person snapshot intervals still cut to the current pose.

The actual four-view SDL test exposed a separate native input-delivery problem:
render catch-up could send a short R press and its release before the next host
poll, which kept only the release. Native protocol11 repeats per-view rising-edge
counters for jump, fire and reload in each newer input message. The host delivers
each pending press once through the ordinary Controls input while retaining the
newest continuous axis/button state. A dropped press packet can be recovered
from the following release packet; duplicate or older messages cannot create a
new press. This does not supply source action admission or promise an accepted
reload/shot while the real weapon owner blocks it. Existing stale-input handling
also clears pending presses when the peer's input expires.

The original fixture still matches541 ticks,2,705 phase states and four
animation callbacks. Its516 naturally retained Match ticks additionally compare
the replica's two channel endpoints to the independent original postpass and
completed-clock states. The separate actual UDP test uses unmodified Lockout
world collision/starts and imported1.5 data,16 participants and four independent
real weapon inputs. It compares the dedicated presentation replica to a twin
Match with actual registered local source clocks, including idle, fire, manual
reload and empty reload. It verifies the differing hand-feedback registration
owners and unchanged ammo/vitality,21,304 samples at30/60/120/144/240Hz,16,534
fractional samples, three bounded fragments, reordering, duplication, complete
snapshot loss, late delivery and recovery. Those sampling rates are arithmetic
and pose-cadence checks; they do not claim measured display performance.

The SDL rendered animation probe also has a native UDP mode. Its server consumes
the real client's delivered commands; the probe supplies only SDL input events
after actual keyboard focus and observes the actual submitted posed vertices.
Full body locomotion/action-manager ownership, dual hands, second-machine LAN,
Xbox wire interoperability and complete animation-triggered audio/effects remain
separate integration requirements. Split-screen frame pacing is measured
separately from channel interpolation.
