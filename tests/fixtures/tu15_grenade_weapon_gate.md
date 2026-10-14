# TU1.5 grenade weapon gate oracle

```sh
.tools/python/bin/python tools/oracle_tu15_grenade_weapon_gate.py \
  --xbe '/path/to/owned/u5English/default.xbe' \
  --fixture tests/fixtures/tu15_grenade_weapon_gate_original.json
cmake --build build --target tu15_grenade_weapon_gate_tests
build/tu15_grenade_weapon_gate_tests tests/fixtures/tu15_grenade_weapon_gate_original.json
```

The English TU1.5 image is pinned to SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The oracle executes the complete unchanged106510 leaf with declared
weapon/header/tag inputs. Its 120 combinations cover absent/present weapons,
authored flag6, unrelated flag bits and signed states across the gate's5..10
interval, including negative and overflow boundaries. No lower return is
supplied. The native comparison checks all GP outputs, defined flags, exact
instruction paths and unchanged retained memory.

This proves the leaf gate, not that a weapon has reached idle or that a grenade
was admitted, released or detonated. Separate live acceptance must establish
those owning transitions. Generated proprietary fixture bytes remain ignored.

