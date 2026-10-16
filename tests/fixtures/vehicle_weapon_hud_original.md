# Vehicle weapon HUD ownership reference

Generate the local comparison payload from authenticated, unchanged TU1.1 and
TU1.5 executables:

```
uv run --with capstone --with unicorn python tools/oracle_vehicle_weapon_hud.py \
  --output tests/fixtures/vehicle_weapon_hud_original.json
```

The 648 controls cover parent/seat absence, seat flags8/20, parent inventory,
child object eligibility, definition flags, signed type byte shifts and sibling
selection order. The original status producer's complete selection block and
equipped-weapon and parent HUD tag routines execute. Downstream numeric status
projection is a declared service boundary. It cannot return a selected weapon
or HUD. Campaign and multiplayer parent interfaces, including absent, single
and paired HUD references, are covered. The native test checks both image layouts against the recorded decisions
and draws the authored vehicle/turret crosshairs through HUD owner type4.

`vehicle_weapon_hud_world_tests` separately checks actual retained source vehicle
entry, original13DA89/1055F0 numeric publication and authored HUD submission.
