# Native LAN first-person registration identities

Native protocol8 now sends full source player/current-unit/current-weapon
identities for each assigned local client view. This supplies the previously
missing input to the already verified original registration/removal module.
It does not implement original Xbox network interoperability, a remote source
animation clock, observer startup, camera/aim or audio.

## Original behavior and native transport

[Source registration](SMG_FP_REGISTRATION_1_1_RESEARCH.md) establishes that
the current full unit controls rebuilding, selected full weapon identities
control hands, removal preserves channel/maps, and each view owns its state.
The prior differential verifies14 original steps,41 authored node pairs and
eight timer phases. The new adapter invokes that same module with actual
authoritative current identities. A corpse's retained unit/weapon is distinct
from the player's current unitNONE after synchronous detach.

The native wire format is our own transport. It appends the two-byte player
salt and two full four-byte object identities for each client's assigned view,
in transmitted slot order. The player's low16 index is its verified assigned
slot; its complete identity is reconstructed without discarding the salt.
Other participants' general gameplay state remains in the snapshot; their
binding identities are outside this per-client local-view extension.

This keeps the existing1400-byte packet bound:16 connected players,32-byte
variant name and four assigned local views consume exactly1400 bytes. The
protocol version changes because the layout changes; old clients reject it.
The decoder rejects inconsistent current unitNONE/weapon-present pairs. The
reliable observer input is the newest accepted authoritative snapshot, without
inferencing object generations from positions, shot ticks or rendering.

## Runtime scope and checks

The rendered LAN client imports the actual arms/weapon/graph definition and
observes each assigned view independently. It uses the binding's presence to
admit first-person rendering. Existing remote shot/reload elapsed-time sampling
remains a declared bridge; local matches use the recovered primary clock.
No remote channel frames or events are synthesized as source output.

[remote_fp_binding_tests.cpp](../tests/remote_fp_binding_tests.cpp) exercises
actual native UDP with four assigned views, full16-participant capacity,
maximum-length variant, retained independent channels, death ownership release,
respawn, leave and reconnect. Death begins at the earlier proven supplied
dead-bit boundary; source live contact/physics is not claimed by that test.
The test also checks retained corpse lookup and malformed current identities.
It returns77 when the user's original Lockout cache is absent.

All nine focused normal lifecycle/registration/clock/equipped-SMG and LAN checks
pass in `integration/remote-fp-focused-tests.log`. This includes the complete
maximum-size16-player/four-view packet, not only a small lobby. The transport
identity extension does not widen the original gameplay/physics claims.
