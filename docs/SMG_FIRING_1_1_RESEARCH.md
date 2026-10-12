# Authored single-SMG firing and reload, disc update 1.1

This is a bounded transition specification for the unmodified update executable and the authored Lockout SMG. It covers single-SMG fixed-rate scheduling, the authored projectile constructor, forward velocity inheritance, and the later new-object motion pass under the recorded service boundaries. Complete multiplayer, actual world registration/impacts, all global RNG consumers, animation playback, and Xbox network compatibility remain unresolved.

## Authority and reproducibility

The instruction oracle is [`tools/oracle_smg_firing_1_1.py`](../tools/oracle_smg_firing_1_1.py). It maps the original executable sections and cache metadata, executes the original constructor, input setter, entire weapon update, actual animation timing lookups, and original projectile dispatch. No original instruction bytes are patched. Hooked engine services and their incoming registers, arguments, and supplied return values appear in each report.

| Input | SHA-256 |
| --- | --- |
| `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03` | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| Its `.text` section | `cc2f07253ca6d531ff068442df31cb91773a03ae2d6d25779f8be72ee95a5131` |
| `assets/disc/maps/lockout.map` | `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9` |

These are the map-pack disc's **1.1** instructions; they are not a claim about the final update ever released. The weapon is `objects\weapons\rifle\smg\smg`, datum `E57B0406`. The wielder definition is the real `objects\characters\masterchief\masterchief_mp`, datum `E9F2086D`; the player character selector is zero. Cache bytes are mapped using their original `7E42C800` magic. The clock is 30 Hz, `dt` bits `3D088889`. The initial shared RNG seed is `12345678`.

The fixture explicitly supplies a valid unit/weapon object relationship, positive ammo, a fixed aim, clear collision query results, absent live marker instances, no successful effect allocation, and a projectile allocator returning `NONE`. It records the actual projectile **request before that supplied allocation failure**. It does not fabricate a successfully created projectile. Animation notifications and the visual unit transition service are supplied; animation graph selection and timing lookup execute original instructions against original metadata.

Reports:

- [`smg-firing-1.1-oracle.json`](../analysis/multiplayer/map-pack/smg-firing-1.1-oracle.json): hold 45 ticks, release 15, mode 1.
- [`smg-firing-mode5-1.1-oracle.json`](../analysis/multiplayer/map-pack/smg-firing-mode5-1.1-oracle.json): the same inputs, mode 5/world state 4.
- [`smg-empty-reload-mode5-1.1-oracle.json`](../analysis/multiplayer/map-pack/smg-empty-reload-mode5-1.1-oracle.json): hold 135 ticks, release 90, natural empty reload.
- [`smg-full-reload-mode5-1.1-oracle.json`](../analysis/multiplayer/map-pack/smg-full-reload-mode5-1.1-oracle.json): hold 45, release 90, explicit reload immediately before tick 45's weapon update.

For example:

```sh
.tools/python/bin/python tools/oracle_smg_firing_1_1.py --mode 5
.tools/python/bin/python tools/oracle_smg_firing_1_1.py --mode 5 --hold-ticks 135 --release-ticks 90 --output analysis/multiplayer/map-pack/smg-empty-reload-mode5-1.1-oracle.json
.tools/python/bin/python tools/oracle_smg_firing_1_1.py --mode 5 --hold-ticks 45 --release-ticks 90 --reload-tick 45 --output analysis/multiplayer/map-pack/smg-full-reload-mode5-1.1-oracle.json
```

Every row retains state before update, after update, after dispatch, and all 52 barrel-state bytes. A second original dispatch pass is executed and asserted to leave state and events unchanged.

The reproducible expanded suite is [`smg-firing-suite/suite.json`](../analysis/multiplayer/map-pack/smg-firing-suite/suite.json): **22 cases, 2,220 ticks, 566 requests**, covering held/pulsed fire along each coordinate axis, mode 1 and mode 5, positive and initially empty ammo, no reserve ammo, both reload clips, and held fire resuming after natural reload. All eleven mode-1/mode-5 state-row pairs match; every second postpass is unchanged. Reports also retain tick-start, after-timer-decay, and after-input phases. Initial ammo overrides are explicitly labeled fixture state; the authored tag values are unchanged.

```sh
.tools/python/bin/python tools/oracle_smg_firing_1_1.py --suite analysis/multiplayer/map-pack/smg-firing-suite
.tools/python/bin/python tools/oracle_smg_firing_1_1.py --instruction-inventory analysis/multiplayer/map-pack/smg-original-instructions
```

[`smg-original-instructions/inventory.json`](../analysis/multiplayer/map-pack/smg-original-instructions/inventory.json) records hashes and direct calls for the original address listings. Static branch listings and dynamically executed fixture paths are distinguished: a listing alone does not prove every branch ran.

## Original tick order and event ownership

Original game tick `00139750` updates controls, then calls the object-update pass `000B7E00`, then `001C7660`, then postpasses `000B7F50` and `000B8040`, each of which can invoke `001019F0`. The clock's tick counter advances later. `000B7E00` reaches object type callbacks through `000BD360` / `0010CDF0`. Weapon callback slot `0046F1B0` points to `001021D0`. Unit update `000C6C50` decays its hand timers and supplies weapon control before child-weapon recursion.

The bounded oracle uses this order:

1. Execute actual hand-timer decay instructions `000C6CD8..000C6D05`.
2. Set original weapon input with `001045E0`: held is bit 1 (`0x2`); a rising edge also sets pressed bit 3 (`0x8`). These input values are fixture controls, not a guessed substitute for the original error marker.
3. If requested, execute reload command `00105640` before weapon update.
4. Execute entire `001021D0`; it runs the real trigger and barrel loops and marks pending shots.
5. Execute `001019F0`, which iterates weapon objects in ascending object-slot order and calls `00101A60` when weapon `+16C` bit 0 is set. `00101A60` invokes `00108300` for barrel `+04` bit 1, then clears pending flags.
6. Execute `001019F0` again and verify that no second projectile request or RNG advance occurs.

Game globals `+8 == 2` is the copied multiplayer options state (offset zero in the options structure). The mode-1 fixture therefore represents local multiplayer, not campaign. The mode-5 fixture additionally supplies simulation world state 4 and authority entity flags. The 60 positive-ammo rows are bit-identical between these fixtures. This validates the bounded state path under those supplied options, not a complete distributed session.

`000A38F0` is the `weapon-fire` event handler (vtable `00459740`, slot 11). It calls shot preparation, error advancement, and immediate dispatch. **It is not called by the normal SMG object-update path.** Its original base counterpart `000A2BA0` did not contain the new TU `00102090` call; see [`WEAPON_ERROR_1_1_RESEARCH.md`](WEAPON_ERROR_1_1_RESEARCH.md).

The authored SMG barrel `+34 == 1` bypasses `00108300`'s special mode-4/5 event branch. Both mode-1 and mode-5 original traces create direct allocator requests; they do not submit `weapon-fire` through `000A8B30` / `0008C0A0`. Calling `000A38F0` after the normal update would duplicate preparation/error advancement. The native host should consume the normal pending-shot path once; it must not infer that the replicated event observer is the local firing entry point.

## Authored fields needed by the bounded scheduling port

The original barrel record is stride `EC`, at cache file offset `31366192`. The trigger record is stride `40`, offset `31017112`. The magazine record is stride `5C`, offset `31366088`. Decimal offsets here are exact cache-file offsets, not runtime pointers. Semantic labels below are restricted to demonstrated reads; unestablished names are left as offsets.

| Record + offset | Type | Authored SMG value / role |
| --- | --- | --- |
| trigger `+00` | u32 flags | 0 |
| trigger `+04`, `+06` | i16 | Input 0, behavior 0 (Spew) |
| trigger `+08`, `+0A` | i16 | Primary barrel 0, secondary `-1` |
| barrel `+00` | u32 flags | 0 |
| barrel `+04`, `+08` | f32 | Rate bounds 15, 15, bits `41700000` |
| barrel `+18` | f32 | 0 |
| barrel `+1C`, `+1E` | i16 | Both zero; minimum/maximum firing gates in `001062B0` |
| barrel `+20`, `+24` | f32 | Both zero; recovery timing |
| barrel `+28` | i16 | Magazine index 0 |
| barrel `+2A`, `+2C` | i16 | Ammo used/check 1, additional loaded-ammo gate 0 |
| barrel `+2E` | i16 | 3; response/spacing gate, exact broader semantics unresolved |
| barrel `+34`, `+36` | i16 | 1, 2; `+34` selects direct dispatch for this SMG |
| barrel `+6A` | i16 | One projectile per fire |
| barrel `+D4`, `+D8` | f32 | Rate-fraction rise 1, fall 5 |
| barrel `+C4`, `+C8` | f32 | Angle-change rise `3F68BA2E`, fall `40555555` |
| barrel `+DC`, `+E0` | f32 | Error rise 1, fall 5 |
| barrel `+50`, `+54` | f32 | Alternate error rates `3FAAAAAB`, `40555555` |
| barrel `+58`, `+5C`, `+60` | f32 | Angle bits `3C0EFA35`, `3CD67750`, `3D567750` |
| barrel `+70`, `+74`, `+78` | f32 | Angle bits `3C8EFA35`, `3C8EFA35`, `3D449809` |
| barrel `+E4`, `+E8` | block | One firing-effect entry; its flags are zero |
| barrel `+9C`, `+A0` | f32 | Separate fraction-set gates 0, `3D4CCCCD` (0.05) |
| barrel `+CC`, `+D0` | f32 | Separate fraction-decay rates 20, 1000 |
| magazine | i16 ammo fields | Capacity 60, initial total 180, reserve capacity 180, reload amount 60 |
| magazine `+10`, `+18` | f32 | Authored reload seconds 2, chamber seconds 0; normal reload timing below uses animation lookup |

## State fields

Offsets are relative to the original weapon object except the barrel table, whose first entry begins at weapon `+1A4` (stride `34`). Preserve signedness and `-1` sentinels rather than treating every short as a positive duration.

| Location | Type | Demonstrated meaning |
| --- | --- | --- |
| barrel `+00` | i8 | Rate counter; constructor initializes 127 |
| barrel `+01` | u8 | Mode 0 idle, 1 firing, 2 transition, 3 recovery |
| barrel `+02` | i16 | Mode timer |
| barrel `+04` | u16 | Bit 0 request, bit 1 pending shot, bit 6 blocks repeated dry fire |
| barrel `+06` | u16 | Prepared-shot count in firing mode |
| barrel `+08`, `+0A`, `+0C` | i16 | Effect selection/remaining counters |
| barrel `+10` | f32 | Rate fraction |
| barrel `+14`, `+18` | f32 | Separate effect rates |
| barrel `+1C` | f32 | Spread-error fraction |
| barrel `+20` | f32 | Angle-change fraction; camera response is not established here |
| barrel `+24` | f32 | Subshot accumulator |
| barrel `+28` | f32 | Recovery fraction |
| barrel `+2E` | u8 | Per-marker counter |
| barrel `+30` | u32 | Effect handle |
| weapon `+16C`, `+16E` | u16 | Weapon flags, input flags |
| weapon `+170` | u8 | Analog input amount |
| weapon `+178`, `+17C` | u8, i16 | Weapon animation state, blocking/countdown state |
| weapon `+20C`, `+20E`, `+210` | u8, i16, u32 | Trigger state, timer, flags |
| weapon `+224`, `+226` | i16 | Magazine state, duration ticks |
| weapon `+22A`, `+22C` | i16 | Reserve ammo, loaded ammo |
| weapon `+230`, `+232` | i16 | Ammo-transfer key ticks, other timing |
| unit `+1FA`, `+1FB` | u8 | Primary/secondary hand timers |

**Correction to early research labels:** barrel `+24` is the subshot accumulator and `+28` is recovery fraction. The current report field names use these corrected offsets.

Additional 52-byte state transitions are independently visible in the original instructions and complete raw rows. At `00103267..00103282`, positive barrel `+14` decreases by authored barrel `+D0 * dt`, with single-precision multiplication/subtraction and a floor at zero. Successful preparation sets it to one at `00107775` only when authored `+9C > 0` and flag bit 4 is clear; authored SMG `+9C == 0`, so it remains zero. At `00103296..001032B1`, positive state `+18` similarly decreases by authored `+CC * dt`; successful preparation sets it to one at `00107799` because authored `+A0 > 0`. With `+CC == 20`, the next skipped-rate tick records `3EAAAAAA`; the next preparation's tick first floors it at zero and then resets it to one.

Successful preparation increments state `+2E` as an 8-bit counter. It persists through barrel mode reset. The one-entry firing-effect chooser sets selection `+0A == 0`, resets an exhausted `+08` selection mask and ORs bit zero, then initializes remaining count `+0C` from authored bounds **1/1** and immediately decrements it to zero. These equal bounds and zero random-choice flag consume no RNG. During dispatch, state `+0E` is read as signed 16-bit, incremented, and then reset to zero if the rate fraction is zero or its **old** value was at least authored `+2E == 3`. The resulting active-firing sequence is 1, 2, 3, 0, 1; resetting at the incremented value 3 would be incorrect. State `+2C` holds extra pending subshots and is zero in this authored fixed-rate trace; effect handle `+30` remains `NONE` under the supplied effect-allocation failure.

## Narrowed firing transition

The following describes the authored single SMG with positive ammo, no second weapon, and the recorded unblocked service fixture. It must not silently generalize to other trigger behaviors, dual wield, projectile counts, overheated weapons, or other firing-effect lists.

1. The Spew trigger branch (`001028C7..00102957`) sees held or pressed, checks primary barrel and calls `001066F0`. If eligible it sets trigger `+04` bit `0x20` and barrel request bit 0. Otherwise it clears the trigger bit. If released or newly pressed, `00107120` clears the dry gate only when barrel mode is not 1.
2. The barrel loop (`00103260`) clears its **stack-local marker**. Positive mode timer decrements at `001032BB`. Mode 0 enters mode 1 at `00103359` when the request is present and global blocking is false. Blocking is derived from original weapon input bit 5, positive weapon `+17C`, or weapon `+16C` bit 1. Mode 1 can be processed in this same update.
3. `001062B0` computes target rate from authored rate bounds and state rate fraction. For this SMG the target is exactly 15 and ticks per shot is exactly 2. If a shot was already prepared and `float(signed_counter)+1 < ticks_per_shot`, this tick cannot prepare another shot. Authored minimum/maximum counters are zero. A dry gate can also block preparation.
4. On a permitted preparation, clear request bit 0 at `001033D9`, set the **original marker** at `001033DF`, and call `00107160`. Successful preparation decrements loaded ammo by 1, increments shot count, sets own-hand timer to 9, sets pending-shot bit 1 and weapon pending bit 0, and resets the rate counter to zero. With the authored one-entry, zero-flag effect list and these services, preparation itself consumes no RNG.
5. On a rate-skipped tick with the held request still eligible, `00103415..00103428` increments the signed counter, caps it at 127, and **also sets the marker**. Thus marker true means an active firing update, including the between-shot tick; it does not mean a projectile was emitted.
6. On released input, `00108010` moves mode 1 to mode 2. On the next update, `00108110` enters mode 3 and, because authored recovery seconds are zero, immediately calls `00107F80`, resetting mode/counter/shot count to zero. The fixture therefore records mode 2 for one row and no persistent mode-3 row.
7. Marker true advances the separate rate fraction using authored `+D4` and calls `00102090` for spread/angle change. Marker false decreases rate fraction using `+D8`; error/angle decay follows the original positive-error gate. Use the existing bit-exact kernels documented in [`WEAPON_ERROR_1_1_RESEARCH.md`](WEAPON_ERROR_1_1_RESEARCH.md), preserving single-precision stores and operation order.
8. The postpass consumes a pending projectile only after that tick's error update. It uses `000D2940`'s opposite-hand timer blend and the original authored angle interpolation before original `00147A90` generates a direction.

Hold 45/release 15 emits 23 requests, on ticks 0, 2, ..., 44. Loaded ammo becomes 37; reserve remains 120. `00102090` runs on all 45 held ticks. First-shot error/angle-change bits are `3D088889` / `3CF83E10`; first minimum/maximum angle bits are `3C8EFA35` / `3C975156`. The first requested forward vector has bits `3F7FF510,3BBD4C91,3C8DF96C`. Tick 44 has error and angle change both 1. Tick 45 is mode 2; tick 46 is mode 0. Error reaches zero at tick 50, while angle change remains `3EAAAAAC` because the original idle decay branch is gated by positive error. Do not force it to zero after error reaches zero.

Alternating held/released input for 60 ticks emits 15 requests on ticks 0, 4, ..., 56. Its intervening barrel transition/recovery ticks prevent a request on every rising edge. Holding for 225 ticks then releasing 15 emits 86 requests and leaves 34 loaded / 60 reserve after the empty reload and resumed firing; use `reload_held_resume-mode5.json` for its exact restart phase.

## RNG and request ordering

`00147A90` uses the global seed pointer at `004EE9D8`. Original stores occur at `00147AC8` and `00147B9E`; each applies `seed = 1664525*seed + 1013904223` modulo 2^32. Tested axis directions are nondegenerate and consume two stores per request. A parallel-table-vector degeneracy can return after one store; prior complete-kernel tests cover that branch. The initial `12345678` seed becomes `CD305E6A` after the first request and `3B4CDA46` after 23 requests.

The ordering is object slot, then barrel/projectile dispatch order. A per-player or per-weapon RNG stream is not justified by these instructions. The firing oracle records every write to the supplied shared seed during its executed slice and supplies allocator failure. The separate successful upper-allocation/first-motion oracle below still observes only the two deviation stores for its bounded stock SMG cases. **Actual engine services, world startup, collisions, impacts and other objects may consume additional RNG; that broader global ordering is unresolved.**

The allocator descriptor's verified layout comes from `000B8A30`: `+1C` position → object `+64`, `+28` forward → object `+70`, `+34` up → object `+7C`, `+40` linear velocity → object `+88`, `+4C` angular velocity → object `+94`. The current report's vector labels follow these operands. Earlier labels used `+28/+34/+40` for position/forward/up and were corrected; the raw descriptor and random-output bytes did not change.

## Reload selection, start order, and actual timing

`00105640` is the magazine reload command (ECX magazine index, stack weapon index, RET 4). `00106C30` verifies ammo/state gates and starts it. Within the start it requests weapon animation state 5 through `00109A60`, then sets magazine state 1 through `00109C30`, then submits the reload effect. The command resets the trigger and barrel state through `00107F80` after the start. The clip lookup is not a supplied duration.

`00109C30` selects `00109D50` with animation state **7 when loaded ammo is zero**, **8 when ammo remains**. `001680B3` maps these to the original StringIDs below. `0016A06A` calls `00169F3B`, `0011CF00` selects the authored Spartan first-person definition, and `001CE8C0`/`001CEAC0` plus `001CF470` resolve the real first-person graph `E5AA0435`.

| Loaded ammo at reload start | Original state | Action StringID | Original clip index | Clip VA | Duration / transfer ticks |
| --- | --- | --- | --- | --- | --- |
| 0 | 7 | `0C000064` (`reload_empty`) | 30 | `8021DCAC` | 50 / 50 |
| Positive | 8 | `0B000065` (`reload_full`) | 31 | `8021DD18` | 50 / 50 |

Both clips have 50 frames and an empty frame-event block (`+4C` count zero). Original `001DE7A0` cannot find event kind 4; `001DE810` cannot find event kind 0, so duration and ammo-transfer key timing both use the 50-frame fallback. `0016A06A` converts frames at 30 fps to the 30 Hz clock through the original rounding routine. The authored magazine's nominal two-second field is not the timing source for these normal SMG reloads. The full and empty animation consumers must select their respective original clip, even though this fixture gets equal timing.

Natural empty trace: last successful request tick 118; tick 120 calls preparation with zero ammo, sets dry flags `C0`, and emits no request; tick 121 enters mode 2; tick 122 resets to mode 0. At tick 123, `00106580 -> 00101D50 -> 000E7EF0 -> 000E88B0 -> 000E8740 -> 00105640` invokes the actual unit action and starts the empty reload during the weapon update. Its post-update duration and transfer timers are 50. Tick 124 decrements to 49; tick 172 reaches 1; tick 173 executes `00106D40`, transferring 60 rounds (loaded 0→60, reserve 120→60) and enters magazine state 5 with timer `-1`. Tick 174 enters state 6, timer 0; tick 175 enters state 0, timer `-1`. Authored chamber seconds are zero.

The explicit positive-ammo trace starts reload before tick 45's update, so its **before** state contains timers 50 and its **after** state contains 49. Ammo transfers at tick 94: loaded 37→60, reserve 120→97, followed by state 6 at tick 95 and state 0 at tick 96. This one-tick difference follows command placement; the native caller must preserve its action-start phase rather than add an arbitrary timing adjustment.

The original timing selection and ammo state transitions are recovered here. Actual visual animation execution and interruption/blending, non-Spartan graph choice, dual-wield reload rules, alternate weapons, death/drop cancellation, unavailable-resource fallbacks, and complete projectile lifecycle remain outside this bounded result.

## Successful upper allocation, projectile velocity, and first motion

The separate oracle [`tools/oracle_smg_projectile_1_1.py`](../tools/oracle_smg_projectile_1_1.py) executes the untouched projectile callback `000FC2E0` for **69 recorded firing requests** and the original upper allocator `000B8A30` plus later `000B8040` for **32 first-shot cases**. The latter covers modes 1 and 5, aims X/Y/Z and `(0.6,0.8,0)`, and owner velocities `(0,0,0)`, `(5,-3,2)`, `(-5,3,-2)`, `(0,0,5)`. These velocities/aims and seed `12345678` are explicit service-fixture inputs; authored projectile metadata is unchanged.

Reports are [`smg-projectile-constructor-1.1-oracle.json`](../analysis/multiplayer/map-pack/smg-projectile-constructor-1.1-oracle.json) and [`smg-allocation-first-motion-1.1-oracle.json`](../analysis/multiplayer/map-pack/smg-allocation-first-motion-1.1-oracle.json). The 32-case report retains the full `C4`-byte request descriptor, full 428-byte object before/after motion, 12-byte object headers at each phase, original source projection operands, final direction, supplied service calls, and RNG stores. Fields can be extracted directly from the retained little-endian byte arrays:

| Raw report field | Offset / size | Demonstrated value |
| --- | --- | --- |
| `events[].request_descriptor_hex` | `+1C` / 12 | Requested position |
| Same descriptor | `+28` / 12 | Final requested forward direction |
| Same descriptor | `+34` / 12 | Requested up vector |
| Same descriptor | `+40` / 12 | Inherited linear velocity, before launch-speed addition |
| `after_creation_object_hex` | `+64` / 12 | Created position |
| Same object | `+70` / 12 | Created forward direction |
| Same object | `+88` / 12 | Complete created linear velocity |
| Same object | `+16C` / 4 | Constructor speed scale, `3F800000` (1) in these cases |
| Same object | `+190` / 4 | Constructor launch speed, `43C80000` (400) |
| `after_later_postpass_object_hex` | `+64` / 12 | Position after original first-motion update |
| Same object | `+168` / 4 | Accumulated travel distance |
| Every 12-byte header | `+02` / 1 | Flags: creation `0A`, activation `0B`, postpass `07` |

The real projectile is `objects\\weapons\\rifle\\smg\\projectiles\\smg_bullet`, datum `E59B0426`, root file offset **31385132**, size 1300. Object type is 5; original type descriptor `0046F3C8` gives 428-byte object storage and constructor `000FC2E0`. Relevant original fields are:

| Projectile definition + offset | Type | Authored bits / value |
| --- | --- | --- |
| `+BC` | u32 flags | `00000000`; immediate-motion bit 5 and speed-scale bit 10 are clear |
| `+D4`, `+D8` | f32 bounds | `00000000`, `00000000`; equal bounds skip timer randomization |
| `+E0` | f32 | `42200000` / 40; distance limit read by the motion path |
| `+164` | f32 | `00000000` / 0; air gravity scale |
| `+16C` | f32 | `42200000` / 40; copied into object `+170`, broader field semantics not established here |
| `+170` | f32 | `3E4CCCCD` / approximately 0.2; water gravity scale, water branch unexecuted here |
| `+17C`, `+180` | f32 | `43C80000`, `43C80000` / 400, 400; launch-speed bounds |
| `+184`, `+188` | f32 bounds | `00000000`, `00000000`; equal bounds skip speed randomization |

`000C95F0` calls `000BB0C0` to obtain owner linear velocity after traversing the object-parent chain. In the ordinary owner branch, `00108856..001088A9` computes the scalar from the **aim before deviation**, using this exact sequence of single-precision SSE operations:

```text
x = f32(original_aim.x * owner_velocity.x)
z = f32(original_aim.z * owner_velocity.z)
s = f32(x + z)
y = f32(original_aim.y * owner_velocity.y)
s = f32(s + y)
if s < 0: s = +0
```

After original `00147A90` deviation and subsequent direction preparation, `0010921B..00109269` multiplies each **final forward** component by this scalar and stores request `+40`. For the tested absent-platform, zero-speed-override case, no further inherited-velocity term is added. The operation is forward projection with negative motion clamped, rather than copying the owner's velocity vector.

`000FC2E0`, at `000FC55F..000FC5B2`, computes `launch = f32(object_speed_scale * definition[+17C])` and then, independently for X/Y/Z, computes `v = f32(final_forward_component * launch)`, **adds the already stored inherited velocity with a separate `ADDSS`**, and stores it to object `+88/+8C/+90`. Do not combine this into `(400+s)*forward`, use fused multiply-add, or add the raw owner vector; those expressions change behavior or rounding. It separately computes/stores `object[+190] = f32(object_speed_scale * definition[+17C])` at `000FC5BA..000FC5CA`.

Example, aim X and owner `(5,-3,2)`: original first-shot final forward is bits `3F7FF510,3BBD4C91,3C8DF96C`, scalar is 5, request inherited velocity is `(4.9991655349731445, 0.028884747996926308, 0.08665423840284348)`, and created velocity is `(404.9324035644531, 2.3396644592285156, 7.018993377685547)`. Use the report's bits as the differential authority, rather than converting these decimal examples back into expected floats.

The optional supporting-platform path is distinct: `0010DF90` can provide its velocity and `00109278..0010931E` adds the component perpendicular to final forward. The positive speed-override branch at `00109321..00109387` is also distinct. Neither is exercised by this absent-platform stock case, and this bounded specification must not silently substitute it for either branch.

**First motion can occur in the same tick as firing.** Original `000FE1E0` skips the constructor's immediate `000FCF90` path because authored `+BC` bit 5 is clear. That does not defer motion to the next game tick. Normal `00139750` calls `000B7E00`, then `000B7F50`, then `000B8040`. Creation during the pending-shot postpass leaves the new projectile dirty/update-enabled (`0A`). A valid location and original `000B8180` activation produce header `0B`. Later `000B8040` checks dirty bit 8, active bit 1, update bit 2, and absence of delete bit `10`; it clears dirty and calls `000BD360 -> 0010CDF0 -> 000FCEC0 -> 000FCF90`. The 32-case oracle executes this actual later pass and records exactly one motion call there. Existing active objects have already updated in the earlier pass; a native integration must preserve this phase distinction rather than update all objects twice.

Stationary aim-X first motion ends at `(13.331108093261719, 0.07702599465847015, 0.23107798397541046)` with distance bits `41555555`; these results use the actual source `dt` bits `3D088889`. Creation and this first motion add no shared RNG stores under the recorded stock-service boundary; the only two stores are the original direction-generation stores. This does not establish complete global RNG order.

The upper-allocation fixture supplies only lower storage/heap management (`000BD170`, `0018FD70`, `000C0270`), clear collision (`0016B240`), unavailable sound runtime (`0018B1F0`), absent live marker/effect services inherited from the firing fixture, and a valid location before original activation. It executes the original upper allocator and type callback rather than replacing their velocity math. **Actual Lockout location lookup, full active-world registration, impacts, material responses, damage, attachments, projectile expiry/deletion, sound and other-world-object order are not proven.** Header location 0 is explicitly supplied, not a result of real map traversal.

```sh
.tools/python/bin/python tools/oracle_smg_projectile_1_1.py
.tools/python/bin/python tools/oracle_smg_projectile_1_1.py --allocation-suite --output analysis/multiplayer/map-pack/smg-allocation-first-motion-1.1-oracle.json
.tools/python/bin/python tools/oracle_smg_projectile_1_1.py --instruction-inventory analysis/multiplayer/map-pack/smg-projectile-original-instructions
```

[`smg-projectile-original-instructions/inventory.json`](../analysis/multiplayer/map-pack/smg-projectile-original-instructions/inventory.json) pins 24 original projectile/frame/seed routine listings, including the demonstrated arithmetic and normal phase callers.

## Authored muzzle marker, eye projection, and supporting platform

The separate [`tools/oracle_smg_muzzle_1_1.py`](../tools/oracle_smg_muzzle_1_1.py) closes the earlier **absent marker** boundary for a supplied current animation pose. It executes original marker lookup and transform, eye-point calculation, aim-line projection, player aim/trace processing, supporting-platform velocity and upper allocation. [`smg-muzzle-platform-1.1-oracle.json`](../analysis/multiplayer/map-pack/smg-muzzle-platform-1.1-oracle.json) retains **49 cases**: modes 1 and 5 × X/Y/Z/`(0.6,0,0.8)` aim × crouch fractions 0/0.5/1 × absent/present platform, plus one supplied rotated-node case. All 24 mode pairs have bit-identical full descriptors and created objects. Every case still consumes exactly the two deviation RNG stores under these service boundaries.

`00105CC0` uses authored barrel marker StringID `+30` when nonzero/non-`NONE`; this SMG has zero and therefore uses primary-barrel default **`0F0000DB`, `primary_trigger`**. `00106070` selects the object queried by `000B9C20`; the stock weapon case selects the actual weapon. Real SMG model `E57C0407` refers to render model **`E57D0408`**. Its marker group is selected by original `001DC8F0`, then `001DC940` builds/transforms the marker using the object's current node matrices and remapping. The actual `primary_trigger` record is file offset **31368556**, stride `24`, node 0, with local translation bits `3DCE498E,00000000,3CD8BD6B` and quaternion `(0,0,0,-1)`. The tool preserves its complete original bytes.

The ordinary unattached Spartan eye routine `000CC320` first obtains actual object world position through `000BACC0`. For the unseated, ordinary-state branch, `000CC3AC..000CC3E5` computes these separate single-precision operations:

```text
a = f32(1 - unit[+2C4])
a = f32(a * biped_definition[+218])
b = f32(biped_definition[+21C] * unit[+2C4])
h = f32(a + b)
h = f32(h * unit[+A0])
eye.z = f32(h + world_position.z)
eye.x/y = world_position.x/y
```

Original Spartan definition `+218` is `3F1EB852` (approximately 0.62) and `+21C` is `3EE66666` (approximately 0.45). The native code must preserve this source blend and object scale when this branch applies; selecting either endpoint directly would miss intermediate crouch state. The separate seat/parent and view-offset branches are not established by these ordinary-state fixtures.

For the supplied identity node pose translated to `(10,20,30)`, original marker transformation yields `(10.100726127624512,20,30.026456832885742)`. Standing eye point is `(10,20,30.6200008392334)`. `000C95F0` projects that marker onto the **pre-deviation aim line through the eye point**, rather than using either the raw marker position or eye point alone. At `000C965D..000C96DA`, use separate `SUBSS/MULSS/ADDSS` operations:

```text
dx/dy/dz = f32(marker_component - eye_component)
s = f32(f32(aim.x * dx) + f32(aim.z * dz))
s = f32(s + f32(aim.y * dy))
projected_component = f32(f32(aim_component * s) + eye_component)
```

Unlike the owner-velocity scalar, this projection scalar is not clamped to positive. The authored extra-origin vector at barrel `+7C/+80/+84` is `(0,0,0)`; no extra offset changes these cases. The standing aim-X request position is bits **`41219C93,41A00000,41F4F5C3`**. The supplied rotated node pose changes the raw marker to `(10,20.100727081298828,30.026456832885742)`, while aim-X projection gives the eye point. This is an executed boundary test of pose consumption; it is not evidence that original animation playback produces that supplied pose.

`000C95F0` also invokes original `0016C240` to clip the projected point against the eye-to-muzzle segment, and the player path executes `001A8560` aim/trace processing before original `00147A90`. **The collision service must fill its output even on a clear result.** Original `0016B240` initializes result fraction to 1, and its tail always computes `result[+08/+0C/+10] = origin + fraction*delta`. `001A8560` reads that endpoint even when the query reports no hit. A return-only replacement leaves invalid/stale input and can produce a false apparent direction change at nonzero origins.

This oracle therefore executes untouched `0016B240` and `001E2020`; only lower BSP primitive `001E20C0` is supplied clear. `001E2020` initializes the zero hit-list count, and the original query wrapper computes the actual clear endpoint. The corrected nonzero-origin aim-X case preserves pre-deviation aim and produces the same original deviation bits `3F7FF510,3BBD4C91,3C8DF96C` as the earlier zero-origin fixture. No stale-query result is used as parity evidence.

For supporting-platform motion, original `0010DF90` traverses to the top owner object. Owner `+C0` must have bit 1 or bit 2; owner `+B8` selects the support object. It searches original physics-list globals `00517FE0`: count at `+1D80`, object indices at `+1D00`, records stride `E8`. In the firing caller's restricted branch, the support definition's **byte `+02` bit 7** must be set and record byte `+E5` must be nonzero. These are checked by untouched code, not overridden.

The fixture uses an **unchanged authored Turf scarab machinery definition**, datum `EAAD0938`, `scenarios\\objects\\covenant\\military\\scarab\\scarab`, with original definition flags at `+02 == 00C0`, rather than adding this flag to a Lockout definition. Its user-owned source cache `assets/multiplayer-map-pack/demos/dlc/00000002/turf.map` is pinned to SHA-256 `3000571be44bde1a62e2738b3a34b482c192772956631d555d8142e7a6d0e446`. Only that definition is copied into the bounded fixture's tag table; this is not a claim that a live scarab exists in Lockout.

At `0010E099..0010E12E`, the support record stores center at `+0C`, linear velocity at `+24` and angular velocity at `+3C`. Original `000BACC0` supplies the owner world point. Let `r` be that point minus support center; source single-precision operation order is:

```text
vx = f32(f32(omega.y * r.z) - f32(r.y * omega.z))
vy = f32(f32(r.x * omega.z) - f32(r.z * omega.x))
vz = f32(f32(r.y * omega.x) - f32(omega.y * r.x))
vx/vy/vz = f32(linear_component + corresponding_component)
```

Thus it produces `omega × r + linear`, with every source rounding step preserved. The supplied owner point `(10,20,30)`, support center `(1,2,3)`, linear `(4,5,6)` and angular `(0.1,-0.2,0.3)` produce platform velocity bits **`C0D9999A,40A00000,4119999A`**. It uses owner world position for this support calculation, not the final muzzle point.

When this helper returns true, `00109278..0010931E` adds the platform component perpendicular to the **final** direction. Its dot operation order is X, Z, then Y, with a separate store-rounded multiply for each projection component and a separate subtract/add:

```text
p = f32(f32(final.x * platform.x) + f32(final.z * platform.z))
p = f32(p + f32(final.y * platform.y))
request_velocity_component = f32(f32(platform_component - f32(p * final_component))
                                 + inherited_forward_component)
```

For aim X with owner `(5,-3,2)` and that supplied platform record, final inherited request velocity bits are **`4099A743,40A22524,411CD14F`**. The projectile constructor subsequently adds launch speed with the separate arithmetic already specified above. The platform term is not the full platform velocity and must not be added twice to the owner projection.

The suite preserves each original marker output and 112-byte marker record under `events[].original_muzzle_entry == "000b9c20"`, original eye outputs under `"000cc320"`, platform output under `"0010df90"`, and final inherited velocity at the `00109321` operand row. Its `supplied_current_node_matrices`, unit scale/position/crouch and `supplied_platform_physics_record` are explicit inputs. **Live animation-pose production, support detection and physics evolution, actual BSP/object collision outcomes, active-world registration and full-world RNG order are not established.** The new evidence justifies porting these consumers once their live inputs are source-backed; it does not justify fabricating those inputs.

```sh
.tools/python/bin/python tools/oracle_smg_muzzle_1_1.py
.tools/python/bin/python tools/oracle_smg_muzzle_1_1.py --instruction-inventory analysis/multiplayer/map-pack/smg-muzzle-original-instructions
```

[`smg-muzzle-original-instructions/inventory.json`](../analysis/multiplayer/map-pack/smg-muzzle-original-instructions/inventory.json) pins the 20 original marker/eye/platform/query listings used for this extension.

## Original shared-seed initialization and match options

[`tools/oracle_multiplayer_seed_1_1.py`](../tools/oracle_multiplayer_seed_1_1.py) executes unchanged original `001479B0` random-global initialization, `00139880` default-option creation and `00139540` option application. [`multiplayer-seed-1.1-oracle.json`](../analysis/multiplayer/map-pack/multiplayer-seed-1.1-oracle.json) records initialization, complete original options, supplied system services, and ten options application cases: states 1 and 2 with seeds `000078A8`, zero, one, `12345678`, and `FFFFFFFF`. State-2 multiplayer variant normalization `001A0EC0` executes original instructions.

`001479EC` initializes the deterministic word addressed by `004EE9D8` to **`000078A8`**. `00147A13` initializes a separate second word from system time, C-library random output, and uptime. Those host-dependent service values are supplied and labeled in this oracle. `001398DC` gives default match options `+10` the same deterministic value `78A8`. `00139540` copies `1118` option bytes into game globals `+8`, then `0013958B` writes option seed `+10` (now game globals `+18`) verbatim to the shared deterministic word. A native option default should therefore use source `78A8`; test seed `12345678` is not evidence for the default.

This establishes initialization and options application, **not the seed at the first shot** after actual map/player/world startup. Startup and other-world consumers can advance the shared stream. The first-shot seed remains supplied in these bounded weapon fixtures; do not claim full-world random-sequence parity from this result.

```sh
.tools/python/bin/python tools/oracle_multiplayer_seed_1_1.py
```
