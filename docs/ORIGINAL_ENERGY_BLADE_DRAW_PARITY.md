# Original Zanzibar energy sword draw

Original20 event8787 is the third-person energy sword's upper blade,
`objects\weapons\melee\energy_blade\shaders\energy_blade_top`.
The native multiplayer screenshot lacks this world item; it is not a measured
pixel-combiner failure of a submitted native sword draw.

The independent [read-only audit](../tools/audit_original_energy_blade_draw.py)
compares the immutable original GPU exports with literal Zanzibar cache
metadata. Its [hashed report](../analysis/multiplayer-shading/original-energy-blade-draw8787.json)
records the following exact evidence:

- The32 captured strip indices match render model`E2D90164`, section0, part0,
  shader`E2DA0165`. All nine other authored parts reject that strip identity.
- All12 c170..172 decompression words match the third-person model's literal
  position/UV bounds. The FP model's different compression record is an
  independent negative control.
- The earlier retained object`E2F70088` has definition`E2D70162` and follows
  the exact definition→hlmt`E2D80163`→mode`E2D90164` chain. Its complete12-word
  consumed c50..52 root palette matches event8787 bit for bit after the original
  separate scale multiplication and matrix transpose. No proximity search is
  used to select the owner.
- That actual object's item owner isNONE, parent isNONE, netgame row is5 and
  birth tick is5. Literal scenario row5's collection`E2D60161` contains the
  same weapon definition. Its authored flag1 levitation request is separately
  recorded; physical/current-state production is not inferred from that flag.

The object export is from the saved Zanzibar baseline at tick12474, earlier
than the1500ms original20 capture interval. Exact consumed palette and tag/row
identity corroborate this draw; they do not establish equality of every item
field or phase across those two observations. Raw preVS vertex bytes were not
exported for this draw. The captured animated purple tint and its four resident
textures remain separate inputs from the authored default color/bitmap list.

At pixel206,168, the original pixel history records two additive blade fragments:
RGB43/37/31 becomes108/83/124 and then173/129/217. Each measured fragment outputs
approximately(.253503561,.179707721,.365865976,0). The retained native pixel has
only its opaque base RGB44/37/31. This establishes a missing draw contribution,
not complete original hardware texture-decoder equivalence.

`multiplayer_runtime.cpp` currently submits BSP/water, source scenery/machines/
props, player bodies, held starting weapons and FP geometry. It does not create
or submit the authored netgame world-weapon rows. `LevelContent` imports those
rows and the bounded inventory timer preserves their source cadence, but the
live match has no world-item allocator/ownership registry, original placement/
settling update or renderer scene for these lifetimes. A future native world
weapon path must consume actual source birth descriptors, salted lifetime,
node/physics updates, row binding, current shader functions and per-view
admission. Captured palette or static authored pose is not a substitute for
those production services.

Reproduce the identity audit without guest or GPU access:

```sh
.tools/python/bin/python tools/audit_original_energy_blade_draw.py
```

The report pins original20 capture SHA`d99a5c97c85916c7b07e30dc1f181d6db81b3a5246686bcbeebebbc5445642ee`,
cache SHA`2000bfc26fa38929e3ae8792618465ca5a60cc6bbeb753426e9d3a99954bdafb`
and every consumed retained export.
