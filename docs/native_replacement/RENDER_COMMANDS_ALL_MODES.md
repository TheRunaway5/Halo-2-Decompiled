# Render-command production in every view mode (2026-11-14)

Scope (coordinator): `40E20`/`40E80`, `41030`, `458C0`, `45CD0`, `460C0`,
`44AB0`, `45550`, `44680`, `4B5C0`, `4BCB0`, `4C440` and their executed
callees, in the main, shadow, projected-light and texture-camera views. Other
agents own the projection executor (`44E80`, `2E79A0`, `2E6290`, `2E6ED0`),
portal visibility (`134C00`..`136230`, `1362A0`, `1365C0`, `31570`) and
PVS/MOPP/matrix leaves (`170C80`, `309940`, `BD080`, `14xxxx`).

## Where the translated work really ran

The instruction census needs `HALO2_REFERENCE_INSTRUCTION_CALLBACKS=1`, which
makes every cursor-gated native bridge refuse, so it overstates translated
owners that are native in production. A sampled production profile (Original
mode, Armory, 1920x1018, `armory_ab.py --sample`, frames 360..480) attributes
every translated sample of these owners to one caller:

| Translated owner | Samples | Production caller |
|---|---|---|
| `40E80` → `44AB0` → `45CD0`/`41030`/`40E20`/`460C0` | 2.5% | `pstencil::native_light_geometry_tu15` (projected light, `run_actual(0x40e80)`) |
| `44710` → `45550` → `2E7C40`/`2E6ED0`, `458C0` | 1.9% | the same function (`run_actual(0x44710)`) |
| `136230` → `1362A0`/`1365C0` → `4B5C0` → `4BCB0`/`4C440`/... | 4.1% (4B5C0 3.2%) | the same function (`run_actual(0x136230)`) |
| `44E80` → `458C0` | 0.1% | main-view native `44710` walk (projection executor's `44E80`) |
| `44680` | 0.4% | `170C80` PVS in the physics live-support unit (PVS owner) |

The main view already ran native `40E80` (`run_native_command_frame_tu15`);
the texture-camera scene reuses that ordinary native view collector. The MP
shadow roots and the projected-light path were the remaining translated
command producers.

## Done

### Projected-light and shadow-root `40E80` → existing native kernel

`source_command_frame_site_tu15` (`include/halo2/native_tu15_command_walk.hpp`)
binds the existing native `native_tu15_command_frame_40e80` (with native
`41C70`, `44AB0`, `449D0`, `45CD0`, `460C0`, `41030`/`40E20`/`1BF30`/`3B9C0`)
at both secondary-view callers:

- `pstencil::native_light_geometry_tu15` (campaign and MP projected lights);
- `source_live_shadow_root_frames_tu15` (MP shadow roots; the former
  `HALO2_NATIVE_SHADOW_COMMANDS=1` gate is now the default route).

`HALO2_REFERENCE_LIGHT_COMMANDS=1` / `HALO2_REFERENCE_SHADOW_COMMANDS=1`
restore the original at that site. `HALO2_SECONDARY_COMMANDS_ORACLE=1` runs
the native kernel inside an undo journal, then the original, and compares the
final value of every byte either stored outside the dead emulated call stack,
plus the x87 control/status and MXCSR words
(`include/halo2/source_native_store_oracle_tu15.hpp`). Counts:
`HALO2_NATIVE_COMMAND_SITE_STATS=1`.

### Projected-light and shadow-root `44710` → native walk with native `45550`

`source_cull_walk_site_tu15` (`include/halo2/native_tu15_cull_walk_44710.hpp`)
binds the existing native `44710`/`44680` walk at the same two callers.
Its sphere entries now run the new native `45550`
(`native_tu15_sphere_cull_45550`) and `458C0`
(`native_tu15_sphere_mask_458c0`), with `145560` composition, `145200`
inverse point and `BAC30` node lookup written in the original SSE operand
order. The query-code branch keeps `2E7C40`/`2E6ED0` (projection executor)
behind a service in `45550`'s own frame shape; a grown result array is
released through the native `149E00`→`233420` owners. Refusals (unmasked FP
exceptions, a non-default allocator) are decided before any effect.

Switches: `HALO2_REFERENCE_SECONDARY_CULL=1` (original `44710` at the site),
`HALO2_REFERENCE_SPHERE_CULL=1` (original `45550` inside the native walk),
`HALO2_SECONDARY_CULL_ORACLE=1` and `HALO2_SPHERE_CULL_ORACLE=1` (store/FP
oracles), counts `HALO2_NATIVE_CULL_WALK_STATS=1`.

### Projected-light and shadow-root `4B5C0` → existing native builder (default, byte-record proven)

`1362A0` (`136501`) and `1365C0` (`136A27`) now reach `4B5C0` through a
required boundary (`tools/generate_shadow_frame_tu15.py`
`NATIVE_MODEL_BUILD_CALLS`; the generated unit carries the same two sites).
`PhysicsLiveWorldTu15::Impl::required` runs the ordinary views' native
`source_model_build` over the same banks, after checking that the builder's
published view inputs (`4C6870`, `4C683C`, `4C6839`) already hold exactly the
values it would write. It uses `model_build_world(..., light_view=true)`: the
original body reaches `3DAF0` (`4BA11`) and `13A2E0` (`4B96E`) only through the
required boundary, so the light-view world has neither of the ordinary views'
host services, the campaign `model_lighting_correction` after `3DAF0` and the
`observed_world_nodes` rendered-WORLD publication. `HALO2_REFERENCE_LIGHT_MODEL_BUILD=1`
restores the original. `HALO2_LIGHT_MODEL_BUILD_TRACE=<file>` records, per
build, every touched non-stack byte with its value before the first store and
after the call. Counts: `HALO2_LIGHT_MODEL_BUILD_STATS=1`. The builder's own
translated services (`3DAF0` lighting, `BCB70`, `BF640`, `23670`) are unchanged.

## Proof

All on one binary (shared `build-perf-4k60` build of 18:40, which carries
every agent's in-progress edits), switching only this work's `HALO2_REFERENCE_*`
routes. Armory (01a) uses `--campaign-reference-capture .05 0 305419896` at
1280x720 in Original mode, with 300 frames unless stated.

Physical original (`tools/validate_native_tu15_sphere_cull_45550.py`, CTest
`original_tu15_native_sphere_cull`). Whole unchanged `45550`/`458C0`, with
genuine `145560`/`145200`/`BAC30`/`32A660`, runs on physical i386 with no
patch and no supplied outcome.
- 328 cases (208 `45550`, 120 `458C0`); 280 change persistent bytes.
- Every non-stack byte and the x87 CW/SW and MXCSR words match.
- Coverage: 6,038 leaves, of which 783 preselected, 522 unbounded, 2,088 BSP
  (cluster/instance), 357 without an object, 1,118 repeated node, 451 without
  BAC30 nodes, 719 node transforms, 502 hits and 4,231 misses. Inputs include
  special floats (NaN payloads, infinities, denormals, signed zero), exact
  boundary equality ±1 ulp, all four rounding modes and DAZ/FTZ. `458C0` cases
  cover bit and range lists, odd and negative counts, and the byte-only flag.
- ASan/UBSan replay of the same 328 cases passes.
- Out-of-tree mutation screen: 11 of 12 mutants killed. The survivor drops
  the node-transform cache, which is an equivalent mutant: re-deriving the same
  node transform yields identical bytes and MXCSR flags. Source SHA-256
  `8e22b5c7831abd89…` was unchanged afterwards.
- Disposable sanitizer CTest cohort: `original_tu15_native_sphere_cull`,
  `original_tu15_native_cull_walk`, `native_tu15_cull_walk_fixture`,
  `native_tu15_cull_walk_controls`, `native_tu15_command_walk_controls` and
  `original_tu15_native_command_walk` all pass (6/6).

Live store oracles. Each compares the final value of every byte either side
stored outside the dead call stack, plus the FP words:

| Site | Workload | Calls | Bytes compared | Mismatches |
|---|---|---|---|---|
| light `40E80` | Armory, 120 frames | 240 | 637,864 | 0 |
| light `44710` (whole site, native `45550` inside) | Armory | 600 | 69,600 | 0 |
| `45550` per call | Armory, 120 frames | 4,920 | — | 0 |
| shadow-root `40E80` | MP Coagulation 1p firing, 150 ticks | 149 | 33,078 | 0 |
| shadow-root `44710` | same | 149 | 2,384 | 0 |
| shadow-root `45550` | same | 149 | — | 0 |

No `45CD0`/`41030` residual ran in the light view: 114,824 native parts and
99,524 native emits, with 0 residual.

`4B5C0` cannot be journaled, because its services commit and re-protect
backing (`Original runtime write to read-only physical page` on rollback). It
is proven instead by a cross-run byte record (`HALO2_LIGHT_MODEL_BUILD_TRACE`,
see "4B5C0 byte-level record result" below). Per call it writes the site, the
owner and every touched non-stack byte with its value before the first store
and after the call. The comparison checks, per call, every byte either route
touched; a byte only one side touched must keep its before value on that side.
The native builder's caller scratch (`60400000..60404000`, as dead as
`4B5C0`'s own stack frame) is excluded.

Frame parity, reference versus native:
- Armory, two runs each: frame `5627a51badf47762`, trace `8a9b58b38bef450d`,
  identical.
- MP Coagulation (`tools/perf_4k60/parity.py`): 4p tick 90 `9c14a330…`, and 1p
  tick 150 firing `05f31264…`, are identical, including the repeat.
- In that 1p case the shadow-root path now runs natively: 149 shadow `40E80`,
  149 `44710`/`45550` and 298 `4B5C0` builds.
- `4B5C0` also has the byte record and more workloads (Coagulation `--motion`,
  Zanzibar, Lockout); see the record result below.

Projection-executor join. With native `45550`, the projection agent's native
`2E7C40` bridge admits the native root frame. That route was added at 18:24
for this owner. Measured: 11,400/11,400 sphere queries native, 0 declined.

Instruction census, same binary, reference switches on → off (Armory, 300
frames):
- Total translated instructions: 945,486 → 736,601 per frame (−22.1%).
- This scope's owners: 65.4 M → 9.4 M per 300 frames. `40E80`, `40E20`,
  `41030`, `45CD0`, `460C0`, `44AB0`, `1BF30`, `3B9C0`, `40DD0`, `41C70`,
  `41C10`, `44710`, `45550`, `4B5C0`, `4BCB0`, `4C440`, `4C2A0`, `4C0C0`,
  `4C180`, `13A2E0` and `3D4E0` all reach 0.
- Left: `458C0` at 4.59 M, from the main view's translated `44E80`, and
  `44680` at 4.69 M, from `170C80`.

Armory A/B (`armory_ab.py`, Original, 1920x1018, same binary with the five
reference switches on vs off, interleaved). Host load was 17–32 throughout,
from other agents' builds, so magnitudes are unreliable. All five pairs favour
native. Steady interval, reference → native:
- pair 1: 118.1 → 61.3 ms;
- pair 2: 52.2 → 46.6 ms;
- pair 3: 154.6 → 95.1 ms;
- pair 4: 89.3 → 64.6 ms;
- pair 5: 51.6 → 49.7 ms.
The two least-loaded pairs (2 and 5) give −5.5 and −1.9 ms. Use the census as
the load-independent measure, and repeat the A/B on a quiet host.

## Remaining, ranked

1. `44E80` → `458C0` (4.59 M/300 frames). The main-view frustum entry is the
   projection executor's. Its native port should call
   `native_tu15_sphere_mask_458c0` (mode 0 bit lists, or ranges) with host rows,
   as native `45550` does. No other caller is left.
2. `170C80` → `44680` (4.69 M). The PVS owner's walk calls the four-word bit
   iterator. Native `44710` already inlines `44680` (`std::countr_zero` over
   the pinned word); reuse that loop when `170C80` is ported.
3. `4B5C0`'s translated services, called from the native builder in every
   view (ordinary and light). Armory 300 frames, census with native bridges:
   - `BF640` deferred nodes, 0.68 M, via `144CF0`/`145560`. Nothing native
     exists yet. Coordinate with the node-rebuild go-live
     (`BF050`/`BF0C0`/`BFF20`/`BFB90`), which owns its callees.
   - `3DAF0` lighting: native by default since 2026-11-15, producing the
     uncorrected original bank; the correction stays an ordinary-view-only
     service. See [object lighting](OBJECT_LIGHTING_3DAF0.md). Its `D3D90`
     query (`D4F90`, the `14DA70` ray/surface-UV tree, `D4350` sample) is
     still translated, about 1.6 M per 300 Armory frames.
   - `BCB70` scalars, 0.26 M. The attachments track's native
     `object_function_value` (physical oracle proven) still refuses the
     vehicle, projectile, device and machine/crate callbacks and several
     `BEC50` names. It stores nothing but its output before a refusal, so the
     admission `scalar` service could try it first with a pre-effect decline.
     That service is shared with the ordinary views, so it needs its own
     record.
   - `23670` PRT does not run in Armory.
4. `41C70` services: `D7230` (per-object update) and `41970` (attachment
   scheduling, `C0700`/`BAD60`) still run translated from native `41C70`, in
   all modes.
5. `2E7C40`'s array layout. The native `45550` service still builds `45550`'s
   original hkArray frame in emulated stack memory for the bridge. Once the
   projection executor exposes a typed sphere query, hand it host rows instead.
6. Remove the redundant `HALO2_NATIVE_SHADOW_COMMANDS` hook in
   `source_bsp_effect_lights.cpp`. The default route is now native without it.


## Files

- New: `include/halo2/source_native_store_oracle_tu15.hpp`,
  `include/halo2/native_tu15_sphere_cull_45550.hpp`,
  `src/rendering/world/native_tu15_sphere_cull_45550.cpp`,
  `tests/native_tu15_sphere_cull_45550_tests.cpp`,
  `tools/validate_native_tu15_sphere_cull_45550.py`, this document.
- Changed: `include/halo2/native_tu15_command_walk.hpp`,
  `src/rendering/world/source_native_command_walk_tu15.cpp` (40E80 site),
  `include/halo2/native_tu15_cull_walk_44710.hpp`,
  `src/rendering/world/source_native_cull_walk_tu15.cpp` (44710 site, native
  45550 service), `src/pstencil/native_light_receiver.cpp`,
  `src/rendering/shadows/source_live_shadow_roots_tu15.cpp`,
  `src/physics/physics_live_world_tu15.cpp` (4B5C0 required boundary,
  native by default, light-view `model_build_world`),
  `tools/generate_shadow_frame_tu15.py`, and
  `src/rendering/shadows/source_shadow_frame_tu15_generated.cpp`. The last is
  a surgical change: the two 4B5C0 call sites become `s.required`, matching
  the generator. Also changed: `cmake/NativeTu15CullWalk.cmake` and a pointer
  in `docs/NATIVE_4K60_SPLITSCREEN_GOAL.md`.

## 4B5C0 byte-level record result

### First run (22:15 shared binary): 344,814 mismatches

`trace4b5c0.sh` ran 13,193 builds per route. It compared 3,337,525 bytes and
found 344,814 mismatches in 5,100 calls, all in the 64 KiB region at 82F30000.
The first was in call 0 (owner E2880019) at 82F325F4, with 90-114 diverging
bytes per call. The native route was made opt-in until the cause was found.

### Root cause: the campaign lighting correction ran in light views

The diverging region is the render-cache pool (`[515F14]+44`, 256 records of
256 bytes). The bytes are `3DAF0`'s lighting bank (record `+54`, 80 bytes)
and its current/target samples (`+1C`, `+30`, 20 bytes each, with the unit
incident vector the record showed). The node matrices were never involved.

`model_build_world`'s lighting service runs `3DAF0` and then, in campaign,
`main.cpp`'s `correct_source_model_lighting`. That callback rewrites those
120 bytes from the native D1850/D1E10 lightmap query, and advances its host
interpolation cache for root objects. The original light-view `4B5C0`
reaches `3DAF0` at `4BA11` through the required boundary, and that boundary
applies no correction. So the native light builds wrote corrected lighting
where the original keeps `3DAF0`'s own bank. The frames stayed identical,
probably because the next ordinary-view build rewrites the record before
anything shows it.

The light-view world also published `observed_world_nodes` into
`rendered_model_nodes` (host only, so absent from the byte record). The
original `13A2E0` at `4B96E` never does that. It could have handed a WORLD
pose to `rendered_world_nodes()` consumers for an object built only in a light
view.

**Fix.** `model_build_world(view, players, modifier, light_view)`: the
`4B5C0` boundary passes `light_view=true`, which omits both host services.
Nothing else in the builder differs. Ordinary views are unchanged.

### Result (22:53 shared `build-perf-4k60` binary; its validation copy has since been removed)

The native route is the default again; `HALO2_REFERENCE_LIGHT_MODEL_BUILD=1`
runs the original. Byte record (`trace4b5c0_v2.sh` / `mp_trace4b5c0.sh`), same
binary, reference versus native:

| Workload | Builds | Bytes compared | Mismatches | Frame (both routes) |
|---|---|---|---|---|
| Armory 01a, Original, 300 frames | 13,193 | 3,124,805 | 0 | `5627a51badf47762`, trace `8a9b58b38bef450d` |
| MP Coagulation 1p, firing, tick 150 | 298 | 956,602 | 0 | `05f31264…` |
| MP Coagulation 1p, `--motion`, tick 150 | 315 | 1,000,888 | 0 | `95a8d7c0…` |
| MP Zanzibar 1p, firing, tick 150 | 966 | 37,024 | 0 | `e8053beb…` |
| MP Lockout 1p, `--motion`, tick 150 | 123 | 555,361 | 0 | `6f565c44…` |
| MP Coagulation 4p, firing, tick 150 | 0 (no light build) | — | — | `13fa114e…` |
| MP Coagulation 4p, tick 90 | 0 | — | — | `9c14a330…` |

The native route also touches bytes the original does not (1,228,973 in
Armory), and every one of them ends with its before value. Its caller scratch
(`60400000..60404000`) is excluded, like the original's stack frame. Armory
frame parity under the Khronos validation layer gives `5627a51badf47762` on
both routes with 0 validation messages. The native route repeats that
without the layer.

Not covered: the continuations at `136506` and `136A2C` reload
EAX/ECX/EDX/EFLAGS, so the native return leaves them as they were. The MXCSR
and x87 sticky status words are not carried back. No STMXCSR exists in these
units, and the `fnstsw` sites test only the condition codes of a fresh
compare. pStencil mode rejects the deterministic campaign capture, so it has
no byte record. 4p MP does not reach these builds.

Instruction census, same binary, Armory 300 frames, `HALO2_CENSUS_NATIVE_BRIDGES=1`,
reference to native:
- Translated instructions fall from 636,723 to 587,136 per frame (−7.8%).
- `4B5C0`, `4BCB0`, `4C440`, `4C2A0`, `4C0C0`, `4C180`, `4C630`, `3D4E0`,
  `3D420`, `3D3D0`, `13A2E0`, `13A1A0` and `13A130` all reach 0.
- `BF640` falls from 800,670 to 681,958, because the native builder applies
  its entry gates (`+112 == -1`, `+B3 > 0`) itself.
- `3DAF0` (465,906) and `BCB70` (255,364) are unchanged. `23670` does not
  run in Armory.

Timing. The windowed `armory_ab.py` A/B could not run because the desktop
session was locked: presentation was clamped ("Window dimensions were
clamped"). A substitute measured process CPU time (user+sys) for three
interleaved pairs of the 300-frame Armory capture under xvfb, same binary.
Host load was 10.6–23.8 from other agents.
- Reference: 51.7, 46.6 and 38.9 s.
- Native: 48.9, 41.0 and 48.1 s.
- Pairs 1 and 2 favour native (−2.8 and −5.6 s); pair 3 favours the
  reference (+9.2 s).

That is inconclusive at this load. Use the census as the load-independent
measure, and repeat `armory_ab.py` unlocked on a quiet host. All six runs
reproduced frame `5627a51badf47762` and trace `8a9b58b38bef450d`.
