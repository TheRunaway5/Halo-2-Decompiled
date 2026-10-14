# TU1.5 runtime prop birth and conveyor integration

The live Colossus authored spawner test exposed a real source integration defect:
all scenario births used the automatic placement caller's `C1120(...,1,1)` mode.
For type11 this forces creation request bit8 and object+C1 bit0, freezing
script-created gas containers at their authored spawns. Original runtime caller
B15B0 instead uses `C1120(...,0,0)`. D7AA0/D7B80 use1,1 for automatic placement;
D75A0 and B9B70 carry the actual original flags into the body.

`SourceScenarioBirth` now preserves this caller distinction through SceneObjects,
Match, MultiplayerObjectRuntime and PhysicsLiveWorldTu15. Initial/BSP placement
uses automatic mode; explicit/script recreation uses runtime mode. No host
unfreeze, velocity, force, contact or solved pose is supplied. The original
creation owners perform every body/physics operation.

Evidence is retained under `analysis/multiplayer/physics-live-tu15-20261012/`:

- `original-source-birth-context-v357.json`: raw TU1.5 original caller/flag contracts.
- `script-prop-red-v355.log`: real authored-script regression before the repair.
- `script-prop-runtime-v357.log` and `script-prop-runtime-v358.log`: subsequent
  actual dynamic paths exposed missing original surface/query owners.
- `conveyor-source-v360/{root,frozen}`: complete authenticated355D0 and1E1170,
  146 instruction sites. Original13DE70 function evaluation,315C90/315CB0
  activation helpers and the original virtual query call remain responsible.
- `script-prop-runtime-v360.log`: PASS, three moving script-created generations,
  650 real host/client UDP device-bank comparisons with no script faults.
- `prop-actual-entry-capture-v360`: actual live entry/return ABI and all committed
  source bytes, exported read-only while running the real world.
- `conveyor-actual-original-hardware-v360/report.json`: PASS396 original instructions.
- `prop-query-actual-original-hardware-v360/report.json`: PASS102 original instructions.

Each original replay executes unchanged TU1.5 text and real callees, compares
60,084,310 accessible final source bytes and all captured GP/SSE/x87/FP state.
The host user-mode interrupt flag already differs at entry and is explicitly
excluded (mask0xFFFFFDFF); all raw flags are retained. This is host i386 evidence,
not Xbox hardware proof, and one real call per entry is bounded coverage.

Visible v361 validation now passes on one stable acceptance ELF (SHA256
`99cf97bf0501c02eef1cd22c5542ffd4d289bbcd92bc265d3060cce98be96ad8`). Eight cases
retain continuously verified live desktop mirrors and5,220 exact UDP states:
Colossus scripted props; movement; object recreation/blast/debris; actual
Spectre driver/passenger cameras; wheel support; eight device-script stages;
and both explicitly created Backwash HVAC placements. The conveyor fixture
renders three moving prop generations, two with observed falling and horizontal
travel, with1,441 aimed camera observations. Its actual surface shader is
`conveyor_belt_pro`; the captured original function rates are U=-0.5,V=0.
This observation does not itself measure complete retail transport trajectories.

The Colossus field trace observes83 actual velocity-changing original calls in
one of its four authored fields. Its captured member is a type11 gas-container
CBASE garbage fragment. Untouched original i386 execution matches that live
force call across1,719 instructions and60,084,310 source bytes. The other three
fields have no observed members in this run and remain open. See
`colossus-nonplayer-fields-observed-v361.json` and
`colossus-can-force-original-hardware-v361/report.json`.

`runtime-birth-verified-live-v361.json` aggregates the eight live reports/videos.
The refreshed selected device/body/geometry/departure/continuation checks pass;
the first broad selection also listed three not-built test executables and is
retained as such. The two original world/qword checks subsequently build and
pass in `prop-world-oracle-regressions-v361.log`. The legacy mixed-version
contact fixture is outside this TU1.5 validation selection.

Elongation's unchanged authored giant-crate spawners also pass650 real UDP
steps in `elongation-prop-runtime-v362.log`. Birth freeze assertions apply at
first observation; subsequent original sleep/freeze transitions remain owned
by the original world. Complete world/controller/object-update original-call
comparisons and visible Elongation validation are being run. Full player, vehicle, object and all non-player field parity
remain open; this checkpoint does not close the overall goal.
