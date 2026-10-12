# King polygon helper over captured renderer state

The unchanged retail polygon helper now executes over the genuine captured Lockout renderer state. Its actual shader-record filler returns AL1 in all three probes. The captured camera and an explicitly distant camera take complete original no-draw returns. A camera placed at the authored hill center reaches the real bitmap-request owner because the hill texture is absent from this retained Slayer/weather frame. This establishes the actual resource boundary; it does not establish emitted hill vertices or GPU output. [Tool](../tools/research_king_draw_snapshot.py), [complete report](../analysis/multiplayer/game-variants/king-draw-snapshot-source-20261012/complete/report.json).

The tool reads the original 64MiB RAM through its captured CPU page tables, preserving the captured device pointer, command cursor/capacity, shader and bitmap banks, readiness and program objects. The original retail executable has SHA256 `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`; captured RAM has `f76d0569dedcd2e94d3f2ee4c67a97db47cefea848105a8b24fe08342daf24cd`. The exporter, function index, actual imported tool dependencies and every executed instruction are pinned in the report. [Captured renderer export](../analysis/multiplayer-shading/original-weather-20261009/session1/lockout-weather-particle-entry/export.json), [original-only renderer harness](../tools/oracle_camouflage_pass_services.py).

Declared inputs are the 32-point Lockout hill state produced by the original initialization/build chain, original literal RGB at `468C84`, actual settings-selected shader `F2C81143`, caller record/stack storage and, in two probes, camera XYZ. These are an explicit API composition. The actual filler `D4BC0(shader,0,0,15,0,NONE,record)` supplies its record to the unchanged `19F680` helper. Its register arguments are EAX=implementation0, ECX=LOD0 and EDX=shader; its nine stack arguments are layer15, record, points, count32, center, radius, perimeter, RGB and height0.8. [Original draw ABI and authored tags](MP_KING_VISUAL_CONSUMERS_SOURCE_RESEARCH.md), [all supplied inputs and record output](../analysis/multiplayer/game-variants/king-draw-snapshot-source-20261012/complete/report.json).

| Camera input | Actual filler | Actual polygon owner |
| --- | --- | --- |
| Unchanged captured camera | Complete, AL1 | Complete far-view return; zero new command bytes |
| Authored center XYZ | Complete, AL1 | Stops before real `12CE00`; 304 pass-preparation bytes |
| Center X + ten radii | Complete, AL1 | Complete far-view return; zero new command bytes |

The center call reaches 2,500 distinct original instructions. Its real requested bitmap record is `804E2234`, contains datum `F2CB1146`, and reports request bits0. No bitmap allocation/I/O/fallback continuation, successful binder return, device progress or texture residency is supplied. The 304 bytes belong to preparation before that boundary and contain no claimed boundary-geometry stream. The earlier [diagnostic output](../analysis/multiplayer/game-variants/king-draw-snapshot-source-20261012/diagnostic-0/report.json) remains preserved. [Complete stop and packet records](../analysis/multiplayer/game-variants/king-draw-snapshot-source-20261012/complete/report.json).

Two separate processes reproduced the final report byte-for-byte. Report SHA256 is `c8d1a1ae54bdcc3ac399d4de7cf0ee643e2fdaeb1d8c005c1964c515fb78fa19`; tool `88738e7f7c38ddd134f8c5609f556423d472adcc66284f752c2d6109dfd54932`; reproduction manifest `80f1275df624ed3adef5df80c0408c09fb91a63184ec53cf39b8162adb811593`. Future runs must use fresh directories. [Reproduction manifest](../analysis/multiplayer/game-variants/king-draw-snapshot-source-20261012/reproduction.json).

```sh
.tools/python/bin/python tools/research_king_draw_snapshot.py --output analysis/multiplayer/game-variants/king-draw-snapshot-source-20261012/fresh/report.json
```

Native material/geometry binding, marker projection and full world/match delivery remain required. This research does not change normal King admission or complete the all39-variant goal.
