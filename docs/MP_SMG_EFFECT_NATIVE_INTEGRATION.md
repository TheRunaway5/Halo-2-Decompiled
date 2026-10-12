# Native muzzle-effect consumer registration

The [frozen effect audit](MP_SMG_EFFECT_OWNER_RESEARCH.md) supplies the original
stock-SMG effect request and visibility consumers. `src/weapon_effect.cpp` is now
registered in `halo2_systems`, the library used by the native executable. The
fixture executable is `weapon_effect_tests`, linked to that library; CTest names
it `original_weapon_effect_request_visibility`. This corrects the frozen audit's
suggested executable-library linkage without changing its source evidence.

Packet sizes in that audit use source hexadecimal offsets: request0x60 bytes
(96), two direction entries totaling0x38 bytes(56), and effect state0x190
bytes(400). The comparisons preserve every byte and the ordered retirement
callback. Allocation, attachment/current matrices, refresh, event/particle and
sound services remain explicit required owners. This registration does not
add a live rendered flash or infer one from `shot_tick`.

The separate continuing local-view source execution has reached genuine world
and FP muzzle markers and particle creation after an explicitly unavailable
audio return. Its later measured owner/lifetime/render composition must be
integrated before declaring visible muzzle parity. The completed201923 Linux
checkpoint predates this module's build registration and remains unchanged.
