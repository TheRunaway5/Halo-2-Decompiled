# Measured multiplayer CPU improvement

Same actual native runtime, Lockout/stock SMG/u5 English data, 1280×720, forced isolated X11, NVIDIA RTX 5080 and Ryzen 7 9800X3D. Each run submits 210 frames; statistics use 180 after discarding 30 warmup frames. No screenshots, vertex hashing or action inputs run during measurement.

| Median ms | One before→after | Four before→after |
|---|---:|---:|
| CPU preparation | 14.93→4.81 | 58.14→17.46 |
| Static world | 10.81→1.07 | 42.62→4.10 |
| Body/held weapon | 0→0 | 5.83→3.67 |
| Renderer including waits | 23.79→22.19 | 45.61→42.84 |
| Active frame | 38.70→27.00 | 103.82→60.38 |

Full refresh is **not established**. Inverse mean active-frame indications improve 25.74→36.40/s and 9.51→16.47/s; these exclude the timing observer/final container destruction and are not physical presentation timestamps. The renderer still uses FIFO and one prior-frame fence. The combined renderer block must be separated into CPU preparation/encoding and actual wait/present phases before attributing the remaining cost.

Verified transformations:

- Existing source stipple level cached once per selected authored part: all 256 full levels/32-row patterns and 80 complete selected BSP vertex/ordered-part outputs match. Isolated median selection 9.676→0.355 ms. Only agent-owned production edit is `src/source_bsp_shader_scene.cpp`, SHA `0ddba509b539a8c416b4463b63235040d78187aea7ae297c0ccba8f4c828c0bc`.
- Root's frame-local body cache: 2,520 full body/node/attached-SMG comparisons match. Actual four-view frames retain 12 body draws while builds fall 12→4; view-specific selection and lighting remain independent.
- Root's consecutive draw-key iterator cache: **258** complete stable vertex/ordered draw-request comparisons pass normally and under ASan/UBSan, with empty/reordered/repeated/bounds controls. Actual selected 43,329-vertex world: isolated grouping 1.350→0.658 ms; four repeated groups 7.312→3.885 ms. CPU comparisons retain descriptor request arguments/order; no GPU handle is manufactured.

Evidence: `analysis/multiplayer/integration/render-cpu-profile-20261008/runtime-summary.json`, four runtime TSV/log pairs, `baseline-pins.json`, `manifest.json`, body/BSP/group comparison TSVs and retained pre-change source/executable. Exact baseline profiler binary SHA: `3176e6003bca69be624bae052e1efdf23140ca88b152db75054a25e7bc336880`. [Full source/key boundaries and reproduction](MP_RENDER_CPU_PROFILE_RESEARCH.md).

Next read-only renderer phases: fence; global constants/resources; per-view constants/postprocess; geometry/scatter/upload; acquire; command encoding; submit; present. Static **unmeasured** candidates are repeated resident pass lookups, rebuilding immutable extern mapping lists and repeated ordered lighting-range searches for overlay materials. None justify dropping shaders, lighting, effects, visibility, HUD or resolution.
