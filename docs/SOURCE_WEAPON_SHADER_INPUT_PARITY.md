# Source weapon shader inputs and energy sword material

The placed sword identified by original20 event8787 already has an admitted
native material path. Its top and bottom select authored layer15, pass0,
implementation0 of `transparent\plasma_mask_offset`, with the original
`transparent_generic_default` vertex code, four authored texture bindings,
additive ONE/ONE blending, no culling and no depth writes. Both have24 authored
animation overlays. The initialized object idle lookup returnsNONE; a named
animation is not substituted for that controller result.

Overlay23 reads the `turned_on` range selector `09000568`. The earlier native
multiplayer callback supplied magazine digits and returned0 for this selector.
The new [typed helper](../include/halo2/source_weapon_shader_input.hpp) consumes
actual bound weapon fields, independently of material names:

- Item flags at object+12C: original10CF50 is true when bit0 is set and bit1 is
  clear.
- Weapon flags at object+16C: FF5F0 tests low-byte bit5.
- The float at object+184: FF5F0 requires it to be ordered below1. Exactly1,
  NaN and positive infinity produce0; negative values remain source inputs.

FF5F0 returns1 and enabled=true exactly when the item predicate is false or
weapon bit5 is set, and the +184 comparison succeeds. The helper admits only
the fully recovered `turned_on` selector. Magazine, firing, heat, generic
object functions and other weapon selectors retain their separate producers.
No whole weapon state is inferred from an authored tag or netgame row.

The untouched [oracle](../tools/differential_source_weapon_shader_input.py)
executes complete FF5F0→10CF50 and then the actual top/bottom cache banks through
17550→17420→176A0/17960. Its external shader callback supplies the independently
executed original weapon-function result. Explicit clocks exercise the actual
periodic tables and all24 overlays, with no inverted captured tint or patched
original callee. All580 cases and110,200 checked words are bit-exact; the76-row
frozen regression passes regular and ASAN/UBSAN builds. The animation test shares
the prepared authored baseline between source and native to isolate animation
arithmetic; it does not claim an independent material-assembly proof.

The independently retained earlier object `E2F70088` has item flags0, weapon
flags20h and +184float0. The unchanged constructor probe has the same item and
float values with weapon flags0. Both yield `turned_on=1`. The retained object
snapshot precedes original20; these observations are not relabelled as one
complete same-frame state.

The [texture audit](../tools/audit_energy_blade_texture_mips.py) independently
compares all20 unchanged original8787 bound mip images with the literal authored
BC chains decoded by the pinned original replay decoder. All13,640 RGBA8 pixels
match byte for byte. The four source texture views begin at authored mip offsets
0,1,2,2; native `load_bitmap` keeps the highest authored resident chain. Original
1B0A0→143C0 additionally chooses a coverage-driven resident mip from the shader
context and view modifier. Bound-view differences alone do not establish a
pixel error under derivative sampling, and this audit does not force a lower
native texture or fit a residency policy.

Reproduce the source input and animation proof with:

```sh
.tools/python/bin/python tools/differential_source_weapon_shader_input.py \
  --binary <build>/source_weapon_shader_input_tests
.tools/python/bin/python tools/audit_energy_blade_texture_mips.py
```

Actual world-item allocation, generation/lifetime, levitated update and per-view
submission are now connected through `SourceNetgameWeaponScene` and the native
Match object pool. The independently proved constructor fields supply this
helper's input; see [the producer and live boundaries](SOURCE_NETGAME_WEAPON_PARITY.md).
The retained8787 animation clock is still an explicit missing input. Its
captured PS10/11 colors remain evidence, not values to overwrite onto native
materials. The fresh tick6 source-camera diagnostic retains that boundary.
