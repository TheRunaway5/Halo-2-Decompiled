# Registered SMG first-person clock: original disc 1.1 evidence

This slice executes the original registered first-person update, activity selection, animation-event traversal, resource lookup boundary, decoder and pose instructions. It does not establish Xbox matrix bit parity or complete camera, audio, world-effect or projectile lifecycle parity. Initial `CE510` loadout remains instant; the timed ready case is a separately supplied current-index admission control.

## Authority and preservation

The executable is the user's map-pack disc `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`. Lockout SHA-256 is `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`. This is disc 1.1 evidence, not a claim about the final later title update.

The [clock oracle](../tools/oracle_smg_fp_clock_1_1.py) is frozen at `7d5548e26d3503ea1057aef3202a9285d7e9fa431d0a943c372788b64282b5d6`. It imports the unchanged [registered-view producer](../tools/oracle_smg_fp_registration_1_1.py), SHA-256 `06c0ddfece364a4361b60219b4f2ab26e98acc693aacccbe16eb89f8d0535e92`, and all its original allocated/equipped dependencies. The previous registration, hand feedback, constructor, attachment, animation, firing and collision suites are unchanged.

The [six-case freeze](../analysis/multiplayer/map-pack/smg-fp-clock/frozen/suite.json), SHA-256 `d831472491423ff7a2fe9db7a20daabc2727e29a1a340a70739efed07e4ccebb`, contains **541 ticks**, each independently reexecuted byte-identically:

| Case | Ticks | Original report SHA-256 |
| --- | ---: | --- |
| idle | 95 | `e06d5ab33a831373cecb633be6e41652fd125707cb445a1e6c907d96b41665d1` |
| release | 25 | `c6a4e8cc726fa65d41f32deb3f59de3cd94fe1e0d2d7276c2efe2cbae9a2ba11` |
| pulse | 36 | `1c307daf0c47bac994f0712c21451b0bfc7f522f305e64d4f559ff31b79bc071` |
| ready admission probe | 25 | `444719b0992b4648d50422dc75445a5d33f72d991d78cd0c1b8e684858f0a2a4` |
| manual full reload | 135 | `1617c3b1fa160b0a9836fe759cfea2f46ebe437bf5cdb96cf87fbcae6c740733` |
| held fire, empty reload | 225 | `2f604ce2897bf04d7da96654cb0b7961e1363e588ae0e5064b2b0f79c54523d7` |

The frozen ready admission probe first invokes `E9D50` with request0, which falls back without selecting ready. Its actual ready producer is the **subsequent normal unit wrapper**, not that direct probe. The [separate ready producer](../tools/oracle_smg_fp_ready_producer_1_1.py) removes that unnecessary direct invocation while preserving the first freeze. Only `UNIT+212=FF` is supplied; `C7E90` itself produces `E9D50(UI,07000101,0,0)`. Use that separate report for the ordinary request-producer contract.

The separate ready tool is frozen at SHA-256 `634e30e563558ffeeb42669ecf8c9fbeab72a3d64db67a6f87a3f62f7e259797`; its [report](../analysis/multiplayer/map-pack/smg-fp-clock/ready-producer/report.json) and [independent repeat](../analysis/multiplayer/map-pack/smg-fp-clock/ready-producer/report-reexecution.json) both have SHA-256 `578faf3a028b8a5843b39491f6fd57539d5e317915b87e2e3d2321a3febe5a1d`.

The canonical [source fixture](../analysis/multiplayer/map-pack/smg-fp-clock/source-fixture.json), SHA-256 `4f87bca8cda0b5b5652a0dadf9c48a5e957ae1c8e99caadd7db524624e48c2df`, replaces the first freeze's ready admission probe with this clean wrapper case: six cases, **541 ticks and2705 phase snapshots**. It retains the exact initial/source phase state, original activity requests, original clock-call channel bytes, event records, view writes and explicit inputs. The [audit](../analysis/multiplayer/map-pack/smg-fp-clock/audit.json), SHA-256 `f403d5d4c62d38897f39bcab4582ead272aca2f333e917fc9f5676ec8676d1f5`, independently checks all541 captured primary FP clock calls' separated float operations, stored frame/rate bits, end/wrap/predictive flags and timing/future-event masks against the authored source fields. It also validates original clip/event/reference bytes, all resource groups, raw manager/phase offsets and **8928 instruction rows** across the two [source inventories](../analysis/multiplayer/map-pack/smg-fp-clock/frozen/instructions/inventory.json) and [ready inventory](../analysis/multiplayer/map-pack/smg-fp-clock/ready-producer/instructions/inventory.json).

Reproduction uses the frozen oracle separately for `(scenario,ticks)` idle95, release25, pulse36, ready25, reload135 and empty_reload225, then the separate ready producer for25 ticks. Save both independent executions to separate files and run `tools/audit_smg_fp_clock_1_1.py`. No native implementation is used to generate expected outputs.

## Executed setup and phase order

Original starting-SMG allocation and instant equip produce weapon10002, parent/owner10001 and real right-hand attachment. The source profile sanitizer `14D600` produces player selector2, then `1677C3/1677E5/1678C1` allocate and initialize the four-view storage and bind the actual source weapon, Spartan arms and FP graph. Both node-name maps are the original registration result. No FP slot, graph mapping or selected animation is directly supplied.

All nine FP graph `E5AA0435` resource descriptors load exact bytes from the user's `shared.map`; resource records retain descriptor bytes, file offset, length, residency address and byte hash. `124F30` is an explicit residency service. `1E13C0`, `27FB10` and their original decode instructions execute. `156AD0` and `188D00(view0,unit10001)` initialize the original camera controller/storage using explicitly allocated backing storage; a full gameplay camera/world update is not executed.

Each tick runs original input edges/action generation/conversion, unit manager blend, `C7E90`, actual allocated weapon `1021D0`, unit action callbacks, unit animation clock/local decode, projectile postpass `1019F0`, then **`1678C1 ->168866`**. The static normal main-frame listing `139750` independently places first-person observation after its object postpasses. The fixture executes those bounded phases, not the entire main frame. Row fields retain `before`, `after_unit`, `after_weapon`, `after_postpass`, and `after_fp` separately.

The timer record and both initial random seeds are recorded explicit inputs; shared seed12345678 and local seed0 are fixture boundaries, not asserted original multiplayer initialization. Yaw, pitch and movement inputs are zero here. Actual control conversion and ordinary unit wrapper execute.

## State layout and exact clock operations

For view `v`, hand `h`, let `H = [4F1198] + v*20CC + h*1010 + C`, and manager `M=H+8`. Three 32-byte channels are at `M+0`, `M+20`, `M+40`. The graph, packed animation identifier, delay, byte flags, ushort options/event masks, rate and frame fields use offsets `0`, `4`, `10`, `11`, `12`, `14`, `16`, `18`, `1C`. Full source manager144 bytes, overlay64 bytes, hand scalar bytes, full first-person view bytes and primary decode scratch are retained.

`168866` decrements signed hand counter `H+F2` only when positive, then computes activation using original camera-mode `156EB0` and valid weapon manager `B7670`, calling `169DDC`. It calls `1CF100` with the real manager, hand callback `168841` or `168853`, view index and source scratch buffers. The channel update executes only with a valid graph/animation, valid manager master graph and manager disabled bit0 clear. The existing three-channel loop/blend rules remain original; do not replace them with a single elapsed-time counter for arbitrary graphs.

`1CA2B0` clears ushort event masks `channel+14/+16`. A positive unsigned delay byte `+10` decrements and returns. Otherwise update requires option bit0 and `(flags11 & 09)==0`. It computes **separate single-precision operations in this order**:

1. `step = round_float(TIME+4 * rate18)` (`MULSS` at1CA2EE).
2. `step = round_float(step * 30.0)` (`MULSS` at1CA2F3).
3. `new_frame = round_float(step + old_frame1C)` (`ADDSS` at1CA2FB).

`1CA090` clears flags11 bits1..3 with `&F1` and clamps a negative new frame to0. Its end threshold is `round_float(float(signed frame_count14) - epsilon)`, where original constant `46504C` has bits **38D1B717** (`9.999999747378752e-05`). It traverses events before altering frame. At or above threshold it traverses remaining events to largest finite float. If option bit1 is clear it sets flags11 bit3 and clamps to threshold. Otherwise, option bit5 with authored loop frame0 invokes original variant selection `1CEDA0`; option bit3 resets rate to1 in that branch. For the observed stock clips the identifier remains the authored selected clip. It computes `wrapped = round_float(round_float(new_frame-threshold) + float(signed loop_frame2E))`, traverses loop events and sets flags11 bit2.

`1C9F50` clamps/stores frame against0 and the authored threshold. If option bit0 and flags11 bits0/3 are clear, it computes the same separate rate/delta/30/add operations using the **stored frame** and sets flags11 bit1 when that look-ahead reaches the threshold, clearing it otherwise. `1CA3A0` then recomputes future timing-event mask16. This predictive end flag is why idle is reselected one clock update before a naive modulo-frame implementation would wrap. The full source instruction listings preserve the branches, comparison directions and rounding order.

After `1CF100`, if it succeeded and primary `flags11 &0A` is nonzero, `168866` invokes `16823F`. The stock idle activity0400000C requests idle again. The firing activity06000006 requests idle only when authored weapon flag17 is clear **or** signed `H+F2<1`. Original SMG flag17 is set. Ready05000024 and both stock reload activities select idle on that same end handling path. Selection uses original `168483 ->1D0A60` graph lookup; it resets the channel through that original request.

## Activity selection and observed timelines

`16A356` maps the original action through `1680B3`. For a fire1 action it adds original constant bits3D4CCCCD (`0.05000000074505806`) to `H+E8` and sets signed `H+F2=4`. `168483` returns immediately when the requested and current activity are06000006 and authored weapon flag17 is set. Thus a repeated shot refreshes feedback but **does not restart the firing animation**. The per-view clock later decrements `F2` and processes/decays the response using the original instructions.

| Source action | Authored clip | Frames | Activity |
| --- | --- | ---: | --- |
| initial/register idle | 21 `first_person:idle` | 89 | 0400000C |
| fire1 | 20 `first_person:fire_1` | 5 | 06000006 |
| timed ready | 29 `first_person:ready` | 19 | 05000024 |
| empty reload | 30 `first_person:reload_empty` | 50 | 0C000064 |
| nonempty reload | 31 `first_person:reload_full` | 50 | 0B000065 |

At initial held-fire tick0, weapon phase selects fire20/frame0; FP phase advances frame1. The first five post-FP frames have bits `3F800000,40000000,40400000,40800000,38D20000`; the last is the original epsilon-bearing wrapped result, not exact0. In the release case, held input ends after11 but original shots occur at0,2,4,6,8,10,**12**: the first released input tick still prepares the last source shot and refreshes FP feedback. The real FP phase selects idle21/frame0 at tick18. Pure idle reselects idle/frame0 at tick87.

Manual nonempty reload begins in unit tick46 and selects clip31/frame0. The same FP phase advances frame1; sound crosses at tick47. End handling selects idle/frame0 at FP tick94. Magazine transfer is separately weapon tick95, followed by weapon-own post-reload state3 at96 and normal mode0 at97. A visible animation end therefore does not authorize transferring ammo early.

Held-empty case fires projectile60 at118. Automatic reload starts in weapon tick123 and selects clip30; FP advances frame1 that tick. The sound crosses at124. FP returns idle at171. Magazine transfer is173, post-reload own request174, and the next real shot/FP fire selection175.

In the clean normal-wrapper ready control, unit tick0 produces timed equip with blocking15 and selects ready29/frame0. Weapon tick0 decreases blocking14; FP advances frame1. Weapon tick14 reaches blocking0. Ready's timing event frame15 crosses at FP tick15, and FP selects idle/frame0 at17. This control is deliberately separate from instant starting equipment and must not introduce unconditional startup blocking.

## Original event contract and audio boundary

`1CAE40` traverses original timing, sound and effect blocks at clip offsets4C/54/5C. Ordinary event membership is the half-open interval `[min(old,new),max(old,new))`; event frame is a signed short. Timing traversal requires signed option-low-byte nonnegative, ORs `1<<(event_id&31)`, and invokes callback type0. Sound traversal additionally requires option bit8 clear; effect traversal option bit9 clear. Reference flags2/4 require option2000/1000 respectively. Reference flag10 suppresses backward crossings; flag20 suppresses forward crossings.

The actual callback record is20 bytes: ushort type at0, signed-short authored frame at2, referenced tag datum at4, timing id/dword at8 (`FFFFFFFF` for sound/effect), reference flags atC, marker StringID at10. The report captures the original bytes rather than supplying this record. `168841`/`168853` supply hand0/1 and forward it to `1687C2`. That consumer acts only on type1 with non-NONE sound and the original camera/flag8 admission.

For stock ready the original graph has timing record **id4/frame15** (`04000F00`) and sound index1/frame1. Both reload clips have no timing-event records and one sound index2/frame1; their selected sound tag isE5AD0438. Ready's tag isE5AC0437. Fire and idle contain no event records. The graph reference blocks and all event bytes are copied into authored metadata.

Original `1687C2 ->18AA90 ->18ACD0/18AA40` executes with actual SMG datum10002, sound tag inEBX, source owner argumentNONE and volume1. Only `18AD70` resource/playback is supplied unavailable; original `RET18` cleanup is verified. The source writes returned `FFFFFFFF` to `H+1008` and the current activity's low ushort to `H+100C` even after unavailable playback. No sound output or audio completion clock is claimed.

## Decode, pose and remaining boundaries

Sampler rows retain actual codec headers, mode, node count, frame indices, interpolation bits, dispatch targets and mask pointers. Per-tick source scratch and full view capture original local channels and original Unicorn pose outputs. Exact original model/node mapping comes from the frozen registration producer.

Xbox `RCPSS` approximation remains unvalidated against original hardware. Channel choice, source clock/event traversal, source resource decoding and measured Unicorn matrix output are distinct claims; passing these source traces cannot establish Xbox matrix bit parity. Full camera world producer, live aim/movement response, successful projectile/world/effect lifecycle, sound scheduler/playback and fidget's actual multiplayer local-seed initializer remain outside this fixture. The reports retain their supplied inputs/leaves explicitly. `native_gameplay_update_applied` remains false for this research artifact.
