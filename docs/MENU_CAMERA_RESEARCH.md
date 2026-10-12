# Retail mainmenu camera and carrier pose

Research date: 2026-10-07. This note records original cache data and executable instructions for the menu camera. Production camera, rendering and shader files were not edited by this research task.

The authored camera points are world coordinates. Their transitions preserve the observer's existing derivatives, include an immediate `camera_set` flush, and use a remaining-time quintic rebuilt on every observer update. The carrier is automatically given its named `idle` animation; that animation contains a root scale of **0.3937000036239624**, rather than the bind pose's scale of 1. These are separate source requirements affecting motion and apparent ship intersections. Sources: [camera_set instructions](../analysis/frontend/camera-research-16c2f0.txt), [observer adoption](../analysis/frontend/camera-research-16f570.txt), [quintic coefficients](../analysis/frontend/camera-research-16ff10.txt), [scenery idle initialization](../analysis/frontend/camera-research-10a3f0.txt), [original idle decoder result](../analysis/frontend/camera-research-carrier-idle-decoder.json).

## Authority and pins

| Primary/local source | Pin or location |
| --- | --- |
| Original executable `assets/disc/default.xbe` | SHA-256 `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d` |
| Original `assets/disc/maps/mainmenu.map` | SHA-256 `1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b` |
| Local CC0 reconstructed source `tools/reference/halo2-decompiled` | Commit `5f08630655e3ddc9ac45909090f11ae799fa0c27`; original instructions prevail over reconstructed C |
| Local Assembly tag schemas `tools/reference/assembly` | Commit `fd14ef3c46135e9422966c6a64cd926fb0bd23fb`; schema descriptions are secondary to actual cache bytes |
| Cache scenario | `scenarios\ui\mainmenu\mainmenu`, physical root `0x037D5A44`; Xbox version 8, build `02.09.27.09809` |

The evidence extracts and fixtures pin the executable/cache hashes in [cache metadata](../analysis/frontend/camera-research-cache.json), [retail trace report](../analysis/frontend/camera-research-retail-menu-trace.json), and [idle decoder report](../analysis/frontend/camera-research-carrier-idle-decoder.json). The script-source block is empty in this cache. The authored calls below were recovered from the original compiled syntax graph, not from a separately supplied `.hsc` text file.

## Authored points and calls

The `scnr+0x1E8` camera block has stride `0x40`: flags `u16+0`, type `i16+2`, name `char[32]+4`, an unknown four-byte field `+0x24`, position `float[3]+0x28`, Euler rotation `float[3]+0x34`. The unknown field is not an authored FOV. All flythrough points have flags 0 and type 0. The separate `anchor_flag_x01` exists, but the `camera_set` evaluator passes object datum `NONE`, so this flythrough does not apply that anchor transform. Sources: [original cache camera and flag records](../analysis/frontend/camera-research-cache.json), [evaluator `0x2A89B0`](../analysis/frontend/camera-research-2a89b0.txt), [camera_set `0x16C2F0`](../analysis/frontend/camera-research-16c2f0.txt), and [scnr schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2Xbox/scnr.xml).

The continuous `mainmenu_flythrough` script (`0x037D9434`) begins with `camera_control true`, immediately sets `ui_path_01`, sleeps 90 ticks, and follows this sequence. Duration and sleep are authored 30 Hz tick counts. Most next calls arrive halfway through the preceding requested transition. Sources: [compiled script expressions and raw nodes](../analysis/frontend/camera-research-cache.json), [script evaluator](../analysis/frontend/camera-research-2a89b0.txt).

| Point | Cache index | Duration | Following sleep | First-loop supplied call tick |
| --- | ---: | ---: | ---: | ---: |
| `ui_path_01` | 6 | 0 | 90 | 0 |
| `ui_path_02` | 7 | 500 | 250 | 90 |
| `ui_path_03` | 8 | 500 | 250 | 340 |
| `ui_path_04` | 9 | 500 | 250 | 590 |
| `ui_path_05` | 10 | 500 | 250 | 840 |
| `ui_path_06` | 11 | 500 | 250 | 1090 |
| `ui_path_07` | 12 | 500 | 250 | 1340 |
| `ui_path_08` | 13 | 500 | 250 | 1590 |
| `ui_path_09` | 14 | 500 | 250 | 1840 |
| `ui_path_10` | 15 | 400 | 200 | 2090 |
| `ui_path_11` | 16 | 400 | 200 | 2290 |
| `ui_path_12` | 17 | 400 | 200 | 2490 |
| `ui_path_01` | 6 | 300 | 300 | 2690 |

The table's call ticks are the fixture's supplied schedule obtained from the authored sleep counts. The fixture does not execute the full original HSC scheduler; it does not independently prove the exact continuous-script restart tick.

## FOV, orientation and clocks

The runtime initializer at `0x377670` loads `0x45E628 = 1.2217304706573486` radians into `0x54E854`, the default **70° horizontal FOV**. Adjacent initializers establish the 1°/110° limits. Director `0x16C840` reads this FOV, emits FOV channel flags 3 and zero transition duration. Horizontal-to-vertical conversion belongs to `0x171D90`; camera-point Euler records do not override FOV. Sources: [original FOV initializers](../analysis/frontend/camera-research-fov-initializers.txt), [director instructions](../analysis/frontend/camera-research-16c840.txt), [FOV conversion source](../tools/reference/halo2-decompiled/src/unknown_170d70.cpp).

Authored rotations are radians with identity forward `+X`, left `+Y`, up `+Z`. Original `0x11DF60` takes `EAX=Euler`, `EDX=forward`, `ECX=up`; its matrix builder `0x141CE0` takes `EAX=output`, stack Euler components, `RET 12`. With `ca=cos(a)`, `sa=sin(a)`, and equivalent `b/c` symbols, the basis is:

```text
forward = (ca*cb, sa*cc - sb*sc*ca, sb*cc*ca + sa*sc)
left    = (-sa*cb, sb*sc*sa + ca*cc, ca*sc - sb*cc*sa)
up      = (-sb, -cb*sc, cb*cc)
```

These conventions also govern authored object placement. Sources: [Euler conversion instructions](../analysis/frontend/camera-research-11df60.txt), [matrix builder instructions](../analysis/frontend/camera-research-141ce0.txt), [reconstructed Euler conversion](../tools/reference/halo2-decompiled/src/euler_vector_conversion.cpp).

`camera_set` converts authored ticks to seconds using the executable's float `1/30`, multiplies by runtime simulation ticks per second, and rounds to an integer with x87 `fistp`. Director `0x16C840` converts remaining simulation ticks back to seconds using simulation dt divided by game-time scale. Observer update dt is separately supplied by the frame update and multiplied by `0x468D28`, initially 1.0. The 30 Hz fixture explicitly supplies float dt bits `0x3D088889`; a 30 Hz trace alone is not evidence of original display frame pacing. Sources: [camera_set](../analysis/frontend/camera-research-16c2f0.txt), [director](../analysis/frontend/camera-research-16c840.txt), [observer service `0x16F280`](../analysis/frontend/camera-research-16f280.txt), [trace fixture](../analysis/frontend/camera-research-trace.py).

At `0x16C3E7..0x16C3F6`, every valid `camera_set` calls director update `0x1554B0(0.0f)` followed by observer update `0x16F280` with literal **`0x38D1B717`**, float **0.00009999999747378752** seconds. This flush occurs before the next normal tick-end decrement and preserves its small change to position/derivatives. At `0x1380AA` the original tick calls script processing; `0x1380B4..0x1380C8` then stores `max(remaining_ticks-1,0)`. Thus the first ordinary observer update after a 100-tick request sees 99 remaining ticks. Sources: [camera_set instructions](../analysis/frontend/camera-research-16c2f0.txt), [director service](../analysis/frontend/camera-research-1554b0.txt), [observer service](../analysis/frontend/camera-research-16f280.txt), [original tick instructions](../analysis/frontend/camera-research-137fe0.txt).

## Observer state and exact update

The observer is an absolute array at `0x4E9BD4`, stride `0x358`, rather than a pointer stored there. Script camera state is separately pointed to by `0x510C6C`: mode `+2`, remaining integer ticks `+0x14`, target position/forward/up `+0x18/+0x24/+0x30`, object `+0x3C`, point type `+0x42`. Sources: [camera_set](../analysis/frontend/camera-research-16c2f0.txt), [observer motion](../analysis/frontend/camera-research-16fe90.txt).

| Observer offset | Contents |
| --- | --- |
| `+4` | Incoming command pointer |
| `+8`, `+0x0C` | Copied command flags; 16 target floats |
| `+0x4C` | Three supplied world-velocity floats |
| `+0x94`, `+0x9C` | Six channel-flag bytes; six remaining-time floats |
| `+0xB6` | Suppress supplied world velocity |
| `+0xB8`, `+0x10C` | Final view; 16 current-state floats |
| `+0x180`, `+0x1B4` | 13 first/second derivatives |
| `+0x1E8/+0x21C/+0x250/+0x284/+0x2B8/+0x2EC` | 13 floats each for coefficients `a5/a4/a3/a2/a1/a0` |
| `+0x320` | 13 residual components |

The six groups contain `{3,3,2,1,1,6}` state components and `{3,3,2,1,1,3}` derivative components: position, positional offset, look offset, distance, FOV, orientation. The last group stores forward/up as six floats, with three rotational residual/derivative components. Sources: [adoption `0x16F570`](../analysis/frontend/camera-research-16f570.txt), [coefficient builder](../analysis/frontend/camera-research-16ff10.txt), [state evaluator](../analysis/frontend/camera-research-170630.txt).

`0x16C840` is a three-stack-argument function, `RET 12`; output command is the third argument. Its objectless point branch emits flags `1|8`, position/forward/up, zero world velocity and distance, duration at command `+0x88`, group flags at `+0x8C`, group durations at `+0x94`. `0x16F570(user)` is `RET 4`: it copies the `0xAC`-byte command into observer `+8`, adopts durations, and **retains existing derivative arrays**. With command flag 8, ordinary position/orientation duration is adopted directly from the director's remaining time. Other command/channel-flag combinations have a two-second interruption rule and must not be inferred from this menu case. Sources: [director](../analysis/frontend/camera-research-16c840.txt), [adoption](../analysis/frontend/camera-research-16f570.txt).

`0x16FE90` takes `EDI=user`, plain `RET`, and skips motion when incoming command flag `0x20` is set. Otherwise its order is residual construction (`0x170BB0`, including rotation residual `0x170C00`), coefficients (`0x16FF10`), acceleration (`0x1701F0`), velocity (`0x1703F0`), state advance (`0x170630`), then remaining times `max(T-dt,0)`. The three evaluators take `EAX=user`, plain `RET`; coefficient builder takes stack user, `RET 4`. Sources: [motion instructions](../analysis/frontend/camera-research-16fe90.txt), [residual](../analysis/frontend/camera-research-170bb0.txt), [rotation residual](../analysis/frontend/camera-research-170c00.txt).

For each component, `d=target-current`, `v/a` are retained observer derivatives, and `inv=1/T`. For `T>dt` with command flag 1, original SSE scalar float instructions form successive powers `inv2=inv*inv`, `inv3=inv2*inv`, `inv4=inv3*inv`, `inv5=inv4*inv`, then:

```text
a5 = (a*inv3)*0.5 - ((d*inv5)*6 + (v*inv4)*3)
a4 = (d*inv4)*15 + (v*inv3)*7 - a*inv2
a3 = (a*inv)*0.5 - ((d*inv3)*10 + (v*inv2)*4)
a2 = 0; a1 = 0; a0 = d
```

For the position group when world velocity `w` is enabled, additionally `a5 -= (w*inv4)*3`, `a4 += (w*inv3)*8`, `a3 -= (w*inv2)*6`, `a1 += w`. Mainmenu commands supply `w=0`. This polynomial is rebuilt from the current residual every update, including after interrupted `camera_set` calls. It is not a fixed initial-position interpolation. Source: [original coefficient instructions `0x16FF10`](../analysis/frontend/camera-research-16ff10.txt).

With `t=T-dt`, form powers by repeated float multiplication and evaluate left to right:

```text
acceleration = (a5*t3)*20 + (a4*t2)*12 + (a3*t)*6 + a2*2
velocity     = (a5*t4)*5  + (a4*t3)*4  + (a3*t2)*3 + (a2*t)*2 + a1
advance      = a5*t5 + a4*t4 + a3*t3 + a2*t2 + a1*t + a0
```

For scalar groups, add `advance` to current state; for orientation, rotate the current basis by the rotational advance vector using `0x170D70`. The derivative sign is the remaining-polynomial convention; a move in positive X can have negative stored velocity. Acceleration component limits are 1500 for the first three groups and 100000 for the last three. A violation clears that group's remaining time and other groups with exactly equal remaining time; the current component loop continues using its already computed `t`. Sources: [acceleration instructions](../analysis/frontend/camera-research-1701f0.txt), [velocity](../analysis/frontend/camera-research-1703f0.txt), [state advance](../analysis/frontend/camera-research-170630.txt).

When `t<=0` in the menu's flag-1/flag-8 case, acceleration/velocity become zero and the exact target is copied. Other flag cases have derivative continuation rules: flag 1 without the stop condition yields velocity `0-d/dt`; no flag 1 retains velocity. Original inverse dt uses x87 double `1.0 / float_dt`, then stores float. Preserve these branches and SSE operation order when claiming instruction differential parity. Original tick initialization calls `__control87(0x9001F,0xFFFFF)`; fixtures use FPCW `0x027F`, MXCSR `0x1F80`. Sources: [velocity](../analysis/frontend/camera-research-1703f0.txt), [advance](../analysis/frontend/camera-research-170630.txt), [tick initialization](../analysis/frontend/camera-research-137fe0.txt).

Orientation residual is computed from a relative basis/quaternion, not three independent Euler lerps. The original `0x1424F0` calls invert the current matrix and compose target against that inverse; reconstructed parameter order must be checked against the original registers. `0x170D70` ignores rotation-vector magnitudes below 0.0001, uses x87 sine/cosine with float stores, and performs the original signed cross-product rotation. The final basis check in `0x170630` uses 0.001 tolerances and cross/normalize repair. Sources: [rotation residual](../analysis/frontend/camera-research-170c00.txt), [rotation application](../analysis/frontend/camera-research-170d70.txt), [basis checks](../analysis/frontend/camera-research-170630.txt), [matrix helper source](../tools/reference/halo2-decompiled/src/unknown_141590.cpp).

## Original-instruction trace and boundaries

Run the research fixture with `.tools/python/bin/python analysis/frontend/camera-research-trace.py`. It executes unchanged original `camera_set`, director command creation, observer adoption, residual/quintic/evaluation helpers, tick decrement fragment, and final view `0x170FD0`. It supplies authored script calls, one local user, simulation rate 30, time scale 1, observer dt `1/30`, no object attachment and a no-BSP location service. The full director/observer service calls made by `camera_set` are trapped and supplied through the corresponding original command/adoption/motion/final-view routines after return, with the verified 0.0001 dt. Visibility/cluster services and the full game/HSC scheduler are outside this fixture. Sources: [fixture](../analysis/frontend/camera-research-trace.py), [trace report](../analysis/frontend/camera-research-retail-menu-trace.json), [2,991 sample CSV](../analysis/frontend/camera-research-retail-menu-trace.csv).

All 2,991 fixture samples have final-view position at observer `+0xB8` bit-identical to motion position at `+0x10C`. Original `0x170FD0` bypasses obstruction handling at `0x171674` when camera distance is zero, as it is for these menu commands; the fixture asserts obstruction routine `0x171830` is never called. This branch depends on distance, not FOV. Adding a ship-collision clamp would alter this original path. Sources: [final-view instructions](../analysis/frontend/camera-research-170fd0.txt), [trace fixture](../analysis/frontend/camera-research-trace.py).

The independent synthetic fixture starts at `(0,0,0)`, requests `(10,0,0)` over 100 ticks, and executes 30 original pre-decrement 30 Hz updates. With the original immediate flush, X is **1.671346664428711**; without flush, **1.6711196899414062**. The former post-decrement/no-flush sequence yields **1.6308174133300781**. This gives a discriminating expected value for the timing regression rather than a native implementation-derived constant. Sources: [synthetic fixture](../analysis/frontend/camera-research-synthetic.py), [synthetic results](../analysis/frontend/camera-research-synthetic.json).

## Carrier initialization and static codec

The scenario places scenery `ui_carrier` at `(-58.29459762573242,63.92333984375,250.6859130859375)`, Euler `(-2.612879991531372,0,0)`, placement scale 1. Its scenery/model/render-model/animation datums are `E1D50060/E1D60061/E1D70062/E1F3007E`. The render model has one root node with bind translation `(1.771570086479187,0,3.3486318588256836)`, quaternion `(0,0,0,-1)`, inverse scale 1 and inverse bind translation negating that root translation. Its three sections are rigid, with authored LOD indices `{0,0,0,1,2,2}`. Sources: [placement cache records](../analysis/frontend/camera-research-cache.json), [model/node records](../analysis/frontend/camera-research-carrier-model.json).

Original scenery creation `0x10A1B0` calls `0x10A3F0` at `0x10A214`. `0x10A3F0` receives object datum in EDX, checks the animation-state offset and graph, and invokes `0x10A660` with SID `0x0400000C` (**idle**), frame 0, attached object `NONE`, interpolate false, loop true. `0x10A660` takes graph datum in EAX and six stack arguments (object, SID, frame, attached object, interpolate, loop), `RET 0x18`; it starts the clip and forces pose update through `0xBD020`. Therefore the absence of a script animation call does not authorize using this scenery's bind pose. Sources: [creation instructions](../analysis/frontend/camera-research-10a1b0.txt), [idle initializer](../analysis/frontend/camera-research-10a3f0.txt), [animation service](../analysis/frontend/camera-research-10a660.txt), [scenery source](../tools/reference/halo2-decompiled/src/scenery.cpp).

The original graph at physical `0x03808494` has clip 1 `idle`: base type 0, one node, one frame, resource group 0, group offset `0x210`, static size `0x30`, animated size `0x20`, followed by two 12-byte channel-flag sets. Resource group 0 begins at map offset `0x00B34E00`, size `0x278`, so this clip's static codec begins at **`0x00B35010`**. Sources: [animation metadata/raw resource bytes](../analysis/frontend/camera-research-carrier-animation.json).

| Idle resource offset | Original value/layout |
| --- | --- |
| `+0` | Codec byte 1 |
| `+1/+2/+3` | Channel counts: rotation 0, translation 1, scale 1 |
| `+0x0C` | Translation data offset `0x20` |
| `+0x10` | Scale data offset `0x2C` |
| `+0x20` | Three float translations `(0,0,0)` |
| `+0x2C` | Float root scale bits **`0x3EC9930C`**, value **0.3937000036239624** |
| `+0x30` | Animated codec 3; rotation/translation/scale counts all 0 |
| `+0x50` | Static flags DWORDs `{0,1,1}` |
| `+0x5C` | Animated flags DWORDs `{0,0,0}` |

The unchanged original static translation decoder `0x28C4E0` reads codec pointer global `0x504480`, translation ordinal `0x5044B8`, output node pointer `0x5044C0`, and copies `codec + codec.u32[0x0C] + ordinal*12` to output `+0x10`. Scale decoder `0x28C510` uses ordinal `0x5044BC` and copies `codec + codec.u32[0x10] + ordinal*4` to output `+0x1C`. Both take no arguments, plain `RET`, and have no callback dependencies. Running these unchanged instructions on the actual static payload independently gives zero translation and the scale above. The final float is a scale channel, not translation Z. Sources: [translation decoder instructions](../analysis/frontend/camera-research-28c4e0.txt), [scale decoder instructions](../analysis/frontend/camera-research-28c510.txt), [decoder result](../analysis/frontend/camera-research-carrier-idle-decoder.json), [codec table/source](../tools/reference/halo2-decompiled/src/unknown_279d80.cpp).

The graph also contains a 20-frame `device:position` overlay mapped from `any:any:any` overlay `position`, but graph `+0x44` has zero ObjectOverlays entries. Original object-function update `0xBD970` iterates that separate block. The menu initializes `grav_lift_control`; this evidence does not establish automatic root motion from that device overlay. Source-required static idle scale is independently established by the creation and decoder paths above. Sources: [animation metadata](../analysis/frontend/camera-research-carrier-animation.json), [object-function overlay instructions](../analysis/frontend/camera-research-bd970.txt), [initializer script](../analysis/frontend/camera-research-cache.json).

## Integration checks

1. Match the native path against the original CSV with the same authored points, dt, flushes and pre-observer decrement; preserve derivatives when a target interrupts an unfinished transition.
2. Apply the named carrier idle pose through the normal animation/skin transform path, using the authored object basis and inverse bind transform. Bind scale 1 alone renders the carrier approximately `1/0.3937 = 2.54` times too large.
3. Recheck camera/ship crossings against material-classified solid hull geometry. The carrier render model also contains plasma/antigravity geometry; intersections with every render triangle are not automatically solid-hull penetration evidence.
4. Test render rates separately. Fractional-frame rendering must retain the original observer rules and simulation/script clocks; the fixed-dt fixture does not prove original display cadence or the whole animation/blending pipeline.

Brightness/shader behavior and matched retail framebuffer comparison remain outside this camera research. The evidence supports the original motion rules and static carrier idle channels; it does not establish whole-scene visual parity.
