# Native TU1.5 movement-link transition

`native_campaign_ai_movement_link_transition_tu15.cpp` supplies `200420`,
reached by the actual Armory actor update at `1FF6DE` on tick 3626. It selects
the authored jump parameters, evaluates the real ballistic trajectory and
publishes the resulting movement request. It does not move the unit or supply
a successful transition result.

The source ABI is EAX actor datum, ECX optional link-mask byte pointer,
stack destination XYZ pointer and force byte, returning AL with RET8.
Only the low byte of the second argument is a boolean: the observed false
argument was `41E90000`. The original caller passes link+0D and derives this
flag from the link's two frame identifiers. Its observed mask was `7F`.

Native `200310` and `278FA0` select the actor/character/scenario parameters.
The existing native character-property owner supplies `1ECD00`.
The native `FF1A0` adapter computes the actual arc, minimum-speed and optional
vertical/horizontal outputs used by both calls. The second call uses the
original x87 speed increment and its own output locations. Native `2B3E0`
performs the planar direction normalization, including the source orientation
and default-axis fallbacks. All these calls use actual inputs and native child
implementations.

The unit mode-5 path constructs request type `2E` and explicitly refuses at
`200669 -> E8E50`. The live diagnostic observed Johnson's unit mode as zero
at ticks 3601 and 3625; the parameter and ballistic calculations do not change
that field. The implementation does not substitute an action-dispatch result.

The unchanged English TU1.5 XBE is pinned to
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The complete original/native cohort has 570 cases: 564 whole returns and six
mode-5 named refusals. It reaches 271 of the 277 original instructions; only
the six return instructions after the unowned action call remain outside the
proof. All retained DATA, 324 ordered stores with exact PCs, defined AL results,
saved GP/ESP and retained FP states match, with 3,954 GP/FP assertions. Cases
cover the real mask/force-byte contract, signed WORD boundary counts, authored
force override, both ballistic calls, rejected trajectories, vertical arcs,
orientation fallbacks, NaNs and six FP profiles.

Eight semantic mutants are rejected. ASan/UBSan/LSan pass with the transition
owner and replay driver instrumented; reused native children and archives are
not reinstrumented by this parent check. The parameter and jump children have
their own original comparisons and sanitizer controls. Ordered reads,
unconsumed volatile GP/XMM, private child frames, arbitrary aliases and arbitrary
fault continuations are not claimed.

The canonical reference is
`tests/fixtures/native_campaign_ai_movement_link_transition_tu15.json.gz`
(160,632 bytes). Source hashes, child evidence, controls, retired intermediate
capture hashes and verified scratch removal are in
`build/test-results/armory-native-progress-20261113/movement-link-transition/qualification.json`.
The actual live result is recorded separately by the runtime probe.
