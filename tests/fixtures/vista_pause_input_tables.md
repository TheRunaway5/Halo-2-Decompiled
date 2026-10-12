`vista_pause_input_tables.json` contains literal results of unchanged Vista
45EE72, run for all 16 controller layout combinations and three English keyboard
presets. These are input ordinals and timings, with no artwork or executable bytes.

Reference `halo2.exe` SHA256:
`33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109`.

Reproduce from the project root (Python with Unicorn):

```sh
python tools/oracle_pause_input_settings.py --exe 'Halo 2 Vista installer/halo2.exe' --output /tmp/vista-input.json
build-multiplayer/controller_settings_tests /tmp/vista-input.json
```

The declared external boundaries are4381FD (English locale),437E51 (input
initialization service), and40FC03 (engine service). The actual45EE72 table
construction runs unchanged. Controller hardware uses original ordinals, before
SDL button translation. Tests compare every hardware input in every admitted
layout, including extended look and movement axes. Pickup held thresholds remain
owned by the existing native interaction hold and inventory consumers.

Auto-centering constants/admission were independently read from Xbox186E03 and
1878D0, with matg player-control+5C/+6E. Controller rates come from Vista6080DC and
461D0B. Native dead-zone and frame scheduling remain host policies; these tests do
not establish full original analog acceleration or physical-controller behavior.
