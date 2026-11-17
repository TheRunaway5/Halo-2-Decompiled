# Track `core` (coordinator)

Native in `src/gameplay/objects/native_object_factory_core_tu15.cpp`; oracle
cases in `tools/native_object_factory_cases_core.py`.

| Owner | Native | Cases | Coverage | Mutations |
|---|---|---|---|---|
| 0x474B90 type table | `object_type_callbacks`, `object_type_chain` | static: rechecked against the XBE on every oracle load | all 13 types, slots +10..+80 | — |
| B9960 | `placement_data_new` | 300 | 87/87 | 3/3 rejected |
| BBE00 | `object_world_position` | 400 | 76/76 | 1/2 rejected; the survivor is equivalent (DAZ cannot make a scale equal 1.0) |
| BABA0 (+ native BA910/16E290/1D30A0/29FFF0) | `object_refresh_spatial` | 400 | full, including 16E290 | 1/1 rejected |
| C0200 (+10E1F0) | `object_notify_nodes` | 200 | full except 10E1F0's callback call (units only; refused as 1157D0) | no observable stores for non-unit types |

Together these give 1,300 cases with 24,194 identical ordered stores.

Refused dependencies: 1157D0 (unit node notification).
