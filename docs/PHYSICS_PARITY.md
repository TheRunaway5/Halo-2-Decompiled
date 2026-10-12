# Physics and sandbox parity progress

The native runtime now uses recovered player movement, walking/air velocity,
jump admission, capsule dimensions, ordinary recharge, ballistic displacement,
armor lookup, shield overflow and body-impact kernels. The selected kernels match the
original Xbox executable in 53,018 instruction comparisons. **The entire
physics and sandbox are not yet 1:1.** Contact generation and the composed
engine state machines remain incomplete.

Shaders and lighting are being handled in another conversation and are outside
this work. Campaign work follows physics and sandbox completion, as requested.

## Runtime changes

* Player control is normalized before applying authored forward/backward/sideways
  speeds. Crouch blends speed and acceleration continuously. Ordinary ground and
  air movement use the recovered velocity kernel with authored slope constants
  and supplied support normals.
* Biped slope bounds come from the serialized cached sine/cosine fields. The
  apparent unreachable downhill interpolation branch is retained because it
  exists in the original executable.
* Gravity uses the original physics-global default, `4.1712594`. The former
  `matg` player-control `+68` multiplier was incorrect: it is zero in every one
  of the 30 original caches, and had disabled native gravity. The retail Lockout
  regression now checks both jumping and returning to ground.
* Jump admission follows the original signed-byte timers and held-input gate.
  A held jump cannot automatically repeat after landing; releasing it re-arms
  input. The impulse accounts for the supplied up and support velocity. The
  present runtime supplies stationary support and world up.
* Ballistic projectiles use trapezoidal displacement, range admission based on
  pre-gravity speed, rounded world endpoints and actual committed travel
  distance. A zero authored maximum range is unlimited.
* Ordinary recharge preserves distinct body/shield stun sentinel behavior,
  flag transitions, strict maximum comparisons and zero/negative shield-rate
  bookkeeping. Shield damage observes authored stun thresholds and preserves
  permanent stun.
* Original `matg` damage groups and material armor IDs now feed weapon impacts
  on shields. Both damage groups and both armor IDs contribute in original
  multiplication order. Overflow is converted back to unscaled body damage;
  a zero armor multiplier absorbs direct shield damage without passing it on.
  Shield depletion/damage flags come from the recovered scalar path.
* Body impacts use their separate authored material and armor IDs, model damage
  flags, minimum stun damage and default damage section. The recovered path
  retains negative lethal vitality, the protected `.0001f` minimum, section
  absorption and nearest-even stun ticks. Body stun is applied only above its
  authored threshold, and can overwrite the `32767` sentinel. A depleted body
  also marks its shield depleted without clamping the body fraction.
* Zero authored maximum body/shield vitality initializes a zero fraction for
  both player and actors. Native capsule hits use the original null-region
  fallback; they do not invent a head region. The scalar kernel supports
  supplied headshot eligibility, but runtime headshots await model hit selection.

## Instruction evidence

The harnesses pin `assets/disc/default.xbe` to:

```text
03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d
```

They execute the original instructions under Unicorn 2.1.4 and compare native
float bits, relevant timer/flag writes, return values and shield effect requests.
Retail tick startup selects x87 nearest-even rounding with 53-bit precision
(`FPCW=027F`); SSE uses `MXCSR=1F80`. Double intermediates reproduce the selected
x87 steps, with float stores at the original boundaries and contraction disabled.
The native application does not execute Xbox instructions.

| Kernel | Original entry | Cases | Boundary |
| --- | --- | ---: | --- |
| Player movement intent | `1E6120` | 5,000 | Player-control branch; mode/control selection supplied |
| Walking/air velocity | `1F0870` | 6,000 | Player definition; contacts/state supplied; climbing/carriers/history excluded |
| Capsule segment height | `DF5F0` | 3,000 | Origin callback supplied; clearance/contact excluded |
| Projectile gravity | `FA170` | 3,018 | Scalar acceleration |
| Jump admission | `E4680` | 5,000 | Support, landing and kick acceptance supplied |
| Ordinary recharge | `D5DE0` | 6,000 | Team/effect boundaries supplied; regions/recent damage/overshields excluded |
| Ballistic segment and distance commit | `F909A..F981A`, `F99B4..F9A2A` | 5,000 | Unaccelerated no-hit segment; air/water selection supplied |
| Armor multiplier | `D5BC0` | 6,000 | Original binary search/comparator run unchanged, including missing/duplicate/wrapping keys |
| Shield damage | `D9110` | 6,000 | Maximum vitality/material/effect boundaries supplied; actor override/parent services excluded |
| Body impact | `D8CB0` with `D9020`, `D5BC0`, `D5D20`, `DB090` unchanged | 8,000 | Team scale, section response, riders, depletion and effects supplied; actor stun override/model hit selection excluded |

All 53,018 cases match. The shield harness invalidates previously translated
`D5BC0` blocks before supplying its multiplier callback, so the second test
actually uses the intended boundary. Its independent armor-table test executes
that routine and its lookup/comparator unchanged.
The body harness restores and invalidates the original armor routine before
executing it. It compares body float bits, normalized requested damage, remaining
damage, stun, flags/reporting byte, section arguments and depletion/effect
requests. It supplies region/death services rather than claiming their parity.

The existing crouch, scalar-cast and fade harness also passes its 36,653 cases
with the same retail floating-point state. Those results establish their own
bounded behavior, rather than the full script VM or animation/presentation.

Run the current evidence against the built native kernel library:

```bash
cmake --build build-parity --target halo2_kernels
.tools/python/bin/python tools/differential_physics.py --library build-parity/libhalo2_kernels.so
.tools/python/bin/python tools/differential_damage.py --library build-parity/libhalo2_kernels.so
.tools/python/bin/python tools/differential_kernels.py --library build-parity/libhalo2_kernels.so
ctest --test-dir build-parity --output-on-failure
```

Evidence for the refreshed build is in `analysis/systems/parity-physics.json`,
`body-differential.json`, `parity-existing-kernels.json`, and the accompanying
logs. `damage-cache-fields.json` records all 30 maps' damage tables: 630 resident
damage groups and 5,098 materials, counting shared data per cache. Cairo has
21 groups and 182 materials; the battle rifle's damage group is `184551379`,
and Chief's shield material is index 147 with armor IDs `301991883` and zero.
Independent serialized fixtures are checked in `tests/systems_tests.cpp`.
Detailed source/instruction research is in
[PHYSICS_SANDBOX_RESEARCH.md](PHYSICS_SANDBOX_RESEARCH.md) and
[BODY_DAMAGE_RESEARCH.md](BODY_DAMAGE_RESEARCH.md). Cairo's Chief body material
is 56, with armor ID `251658404`; the marine body material is 9 with ID
`201328575`. Their default/head section fields are independently checked
against original bytes. The body report records 27 separate original-only
probes, including Miranda's cannot-die behavior and actual childless depletion;
these probes are not added to the native differential count.

The refreshed normal build passes all 18 CTest groups. All seven focused
vertex-program/menu-camera/frontend/native/retail-system groups pass with ASan/UBSan and leak detection;
the earlier full 11-group sanitizer run is also retained. These include the actual Lockout jump-and-landing
path, shield/body armor impacts, protected vitality and Cairo's serialized
damage groups/materials/sections. Final results are recorded in
`body-ctest.log`, `body-asan-ctest.log` and
`body-asan-recheck.log`; the latest camera build results are in
`analysis/frontend/camera-final-ctest.log` and `camera-asan-ctest.log`.
The earlier nine-group run is retained in
`physics-full-ctest.log`.

The rebuilt root `Halo2-Title-Launcher` and `Halo 2 Native (Title)`
desktop/app-menu shortcuts select
`build-parity/halo2_native`, before considering the older `build` directory.
Default launch explicitly selects retail title screen 9 and skips the boot
video, so it starts at the logo and **Press Start to Continue**. Direct movie
preview and other explicit runtime modes still pass through. A launcher run
from `/tmp` with only smoke-test flags renders 30 title frames on RTX 5080,
with synchronization validation and a screenshot in `title-startup.png`.
`title-startup.log` records that run. The launcher was copied with
`python3 tools/install_launcher.py --build build-parity --desktop --title-first`.
The separate standard launcher retains intro-first playback.
The fresh build also accepts the retail cache's duplicate dialog entry and
preserves high-bit tag IDs when opening variant parameter menus. Existing
retail menu tests cover loading the table and every parameter choice.

The reported title flythrough and brightness regressions are addressed in
[MENU_CAMERA_FIXES.md](MENU_CAMERA_FIXES.md): original carrier idle scale,
observer/director timing, fractional rendering, 70-degree horizontal FOV and
deterministic display transfer. They do not establish whole-scene visual parity.

An earlier Cairo run through the launcher from `/tmp` rendered 90 frames on RTX 5080,
resized to 960x600, retained three placed marines and reported no Vulkan
validation/synchronization errors. `physics-cairo.log` and `physics-cairo.ppm`
record runtime execution; they are not original/native visual or gameplay
comparisons.

## Remaining acceptance work

| Area | Required before full parity |
| --- | --- |
| Collision and physics | Retail collision masks, Havok contacts/solver, steps, ledges/ceilings, material friction/restitution, moving supports, actor contacts, dynamic objects/devices, breakables and ragdolls |
| Biped state | Original mode dispatch/update ordering, unsupported grace, hard landing, jump fatigue, climbing/flying/swarm modes, carriers, animation/root-motion history and authoritative object state |
| Weapons and inventory | Every trigger/barrel mode, original RNG/spread, burst/reload timing, heat/charge, dual wielding, pickups/switching, melee and grenades |
| Projectiles | Guidance, water transitions, speed curves, collision iteration, bounce/penetration/material responses, attachments, arming/detonation and explosions |
| Damage | Actual model hit regions/headshots, section state/responses/permutations, radius/obstruction, owner/friend rules, difficulty/team scaling, overshields, recent damage, impulses, full death side effects and actual effect/audio dispatch |
| AI | Original perception/brain/tactical behavior, path costs/orders, animation synchronization, difficulty/placement upgrades and authored encounters |
| Campaign after sandbox | Complete HS/host services, encounters, dialogue/music, cinematic actors/devices, saves/reverts, BSP transitions and authored mission playthroughs |

Native floor/wall/occlusion tests and GPU smoke checks cannot establish those
retail behaviors. Full acceptance requires composed original/native traces and
authored scenarios in addition to the selected kernel comparisons.
