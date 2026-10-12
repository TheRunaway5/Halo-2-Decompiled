# Resident BSP runtime checkpoint

The native multiplayer renderer now keeps eligible source-selected BSP parts in its existing static vertex buffer. The recorded same-frame reference and resident paths produce identical 1280×720 PPM files in all eight confirmed X11 cases: one and four local views on Lockout, Ascension, Zanzibar and Backwash. This establishes equivalence to the prior native copy path in these cases; it does not establish complete Xbox graphics parity or full-refresh split-screen performance.

The immutable checkpoint is [resident-bsp-runtime-20261008](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/integration/resident-bsp-runtime-20261008). Its new `manifest.json` appends final evidence pins without changing `initial-pins.json`, earlier reports, copied binaries or prior freezes. The read-only audit independently parses every final selection capture, compares complete PPM files, recomputes every accepted timing summary and checks all 130 initial pins. It also checks the unchanged 237 resident-plan, 98 prepared-effect and 123 primary-light-option freeze pins. No build or GPU experiment was run by this documentation/audit step.

## Integration and exact fallback

`SourceBspResidentPlan` replaces the ordinary BSP scene constructor for the same mesh and appends the same material/LOD/lightmap bank. It retains the raw BSP vertices and an encounter-order atlas. The renderer independently groups the input mesh and rejects any mismatch in its complete atlas bytes, catalogue identities or physical first/count ranges. The static GPU buffer is uploaded once and lasts for the renderer lifetime; the owning plan is constructed before and outlives that renderer. Per-view binding vectors and vertex spans remain alive through the draw call, and a view cannot submit a foreign atlas owner.

The host `Vertex` record is 132 bytes. It preserves position/color/UV/lightmap UV, normal/tangent/binormal/incident, lightmap index, BSP geometry identity, PRT cluster/weights, section, part and the stipple word at byte 128. Material at byte 32 is a CPU draw binding, not a world shader vertex attribute. The token binds the actual selected material while an eligible resident vertex can retain its raw CPU material word. Stipple is shader input location 11 and must remain exact.

Selection still consumes the actual per-view source projection bank, portal/fog admission, view rectangle, player count and modifier. Accepted, suppressed and untouched catalogue parts remain distinct: absent material suppresses a part, absent stipple override preserves its raw attribute, and the ordered accepted record stream retains duplicate admission requests. The earlier isolated proof remains [MP_RESIDENT_BSP_PLAN_RESEARCH.md](/home/eric/Developer/Halo-2-Decompiled/docs/MP_RESIDENT_BSP_PLAN_RESEARCH.md): 80 actual admission cases are separate from the 870 captured-output part submissions. The runtime captures do not convert those output-only records into invented admission evidence.

For each view, `merge` emits one lexicographically sorted `(material, lightmap, geometry, section, part)` stream containing both BSP and dynamic objects. It preserves the prior draw key, count, stable vertex order and ordered material/lightmap descriptor requests. `logical_first` retains the old packed offset for comparisons; the draw uses its new physical `first` in the selected vertex buffer. All ordinary/late world, overlay, environment, base, planar-fog, water and bloom consumers select that buffer per token. Foreground/HUD paths restore their dynamic buffer; shadow callbacks and particles retain their own bindings.

Residency requires one complete catalogue part, no same-key object collision, owned BSP geometry and an unchanged exact stipple stream. A changed or unknown/nonuniform stipple, collapsed BSP key, same-key dynamic object, or non-BSP geometry keeps the complete prior dynamic stream. Collisions remain one sorted draw with BSP vertices before appended objects. Dynamic staging offsets still include background, debug, foreground, UI and shadow spans, and buffer growth occurs after the frame fence retires its previous use. This optimization changes no shader program, LOD choice, material/pass order, effects or quality settings.

In the complete Lockout timing runs, four-view dynamic staging falls from 371,412 to 198,960 vertices and one-view staging from 59,055 to 15,726. Those totals include dynamic objects and other staging consumers; they are not a claim that every remaining vertex is a stipple fallback. In the preceding isolated capture, only two partial-stipple BSP tokens / 144 vertices required that fallback.

## Same-frame Vulkan evidence

[x11-paired/commands.json](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/integration/resident-bsp-runtime-20261008/x11-paired/commands.json) records the exact arguments, validation environment and results. Every command explicitly sets `SDL_VIDEODRIVER=x11` and runs under Xvfb; every log observes `SDL video backend: x11`. The probe uses the ordinary initial multiplayer frame with tick zero and rejects admitted controllers or external movement/jump/crouch/fire/reload input. It supplies no replacement camera, pose, admission or gameplay result.

Before either draw, the runtime expands the resident selection and compares all 132 bytes of each vertex against `SourceBspShaderScene`, plus all seven fields of each ordered selected-part record. The reference draw copies the same actual view packets and appends the same dynamic object vertices. It clears only the resident-plan routing. Both draws use the same renderer, camera/time/material/lighting banks, foreground, background and HUD. The source BSP postprocess state is saved/restored because patchy-wind preparation advances even at unchanged elapsed time; the final state advances once.

| Map | Views | Ordered accepted part records | PPM/pixel differences | Vulkan validation lines |
| --- | ---: | ---: | ---: | ---: |
| Lockout | 1 / 4 | 33 / 166 | 0 / 0 | 0 / 0 |
| Ascension | 1 / 4 | 61 / 156 | 0 / 0 | 0 / 0 |
| Zanzibar | 1 / 4 | 95 / 320 | 0 / 0 | 0 / 0 |
| Backwash | 1 / 4 | 363 / 1,346 | 0 / 0 | 0 / 0 |

The 26 paired SPV hashes are unchanged before/after and match their frozen copies. The profiling pins also retain three generated particle-program files. Logs identify an NVIDIA GeForce RTX 5080 and contain Xvfb's “No DRI3 support detected” informational messages, which are retained separately from Vulkan validation errors. These are ordinary initial-frame comparisons, not animated gameplay, every possible camera/admission branch, or physical-display scanout validation.

## Backend-specific performance

[x11-profiles/report.json](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/integration/resident-bsp-runtime-20261008/x11-profiles/report.json) contains six complete 180-frame Lockout runs at 1280×720, with 60 warmup frames and 120 measured frames each. All samples submitted; logs observe X11. FPS below is `1000 / mean frame-wall milliseconds`. Geometry packing/upload and present are host durations, not GPU execution timestamps. The renderer retains FIFO presentation.

| Confirmed X11 run | Reference FPS → resident FPS | Geometry ms | Upload ms | Present ms |
| --- | ---: | ---: | ---: | ---: |
| Four views, repeat 1 | 18.60 → 19.81 | 9.54 → 7.96 | 3.08 → 1.73 | 21.90 → 23.11 |
| Four views, repeat 2 | 18.88 → 20.91 | 9.39 → 6.45 | 3.02 → 1.68 | 21.89 → 22.15 |
| One view | 39.02 → 42.14 | 1.15 → 0.37 | 0.33 → 0.09 | 18.82 → 18.61 |

The earlier inherited-desktop-environment runs had an Xvfb `DISPLAY` but did not select SDL X11. [backend-observation.log](/home/eric/Developer/Halo-2-Decompiled/analysis/multiplayer/integration/resident-bsp-runtime-20261008/backend-observation.log) explicitly observes **Wayland** under that environment. The old profiler did not print its backend, so the correction retains that qualification instead of claiming a missing per-run observation. Its first complete four-view pair measured 32.28 → 39.37 FPS, geometry 9.25 → 6.29 ms and upload 2.96 → 1.66 ms, with present near 0.06 ms. These results must remain separate from the confirmed X11 runs.

The earlier four-view resident repeat 2 and one-view resident run stopped on external gameplay input and are excluded. Their unpaired successful references are also excluded from paired performance claims. Their original logs/reports remain unchanged, including the earlier incorrect “controlled Xvfb” description; the new audit records the correction. Neither backend result establishes full-refresh split-screen operation or a guaranteed FPS on the user's physical display.

## Frozen source, binary and capture schema

`observed-source/` retains the scoped runtime, renderer, resident-plan, comparison/profiler source and interfaces observed with this link. `linked-inputs/` retains the exact runtime/tool objects, five archives and binaries; `observed-link.json`, `profile-relink.json` and `earlier-profile-relink.json` retain their recorded commands. The archive members may predate these observed source files. This checkpoint does not claim a complete fresh source-tree build or hermetic rebuild; its exact linked inputs are preserved independently of the current shared checkout and host libraries.

`schema-source/` copies the earlier immutable plan's relevant type headers and records their relationship to checkpoint-observed headers. The `.selection` stream is the frozen tool's little-endian host ABI, not a portable versioned format. Each view contains tick/view u32; the raw **0xC0-byte `SourceViewProjectionBank`**; 512 cluster-entry bytes; 16 marked-cluster u32s; counted cluster and instance records `(u32 id,u16 flags,u16 association,u32 other)`; counted four-u32 association groups; capacity u8; outer top/bottom i16; local players u32; modifier u8; and counted 22-byte selected-part records `(i32 geometry,u32 section,u32 part,u32 LOD,u32 material,u8 coverage,u8 alpha)`. The audit requires exactly one or four ordered tick-zero views and consumes every byte.

Recheck recorded evidence without starting a GPU job:

```sh
python analysis/multiplayer/integration/resident-bsp-runtime-20261008/audit-checkpoint/audit.py \
  --root /home/eric/Developer/Halo-2-Decompiled --output /tmp/halo2-resident-bsp-audit.json
```

`audit-checkpoint/reproduction.json`, independent audit outputs and `source-freeze-map.json` record the exact documentation/audit inputs and boundaries. `manifest.json` pins the complete freeze and the saved document. It is an appended checkpoint; every earlier evidence file and prior freeze stays byte-identical.
