# Retained unit animation clocks and local-pose delivery

The new native modules reproduce the original retained animation manager/channel owners and local sampler delivery. They provide real owner segments for integration; they do not yet make live characters walk, apply the unit pose callback, or produce collision matrices.

`include/halo2/unit_animation.hpp`, `src/unit_animation.cpp`, and `tests/unit_animation_tests.cpp` reproduce `1CF100`, `1CA2B0`, `1CA090`, `1C9F50`, `1CA3A0`, `1CAE40`, `1CF070`, `1D0ED0`, and `1DCD10`. The independent comparison matches **3,486 complete original root calls**, **29 original variant/event callback deliveries**, and every complete manager/channel entry/return buffer over the prior **519 ticks**. Two additional comparisons use the genuinely allocated Spartan's literal manager states and actual node count. Normal and ASan/UBSan runs pass.

`include/halo2/unit_animation_pose.hpp`, `src/unit_animation_pose.cpp`, and `tests/unit_animation_pose_tests.cpp` reproduce `1D0F80` retained-channel scheduling and `1CA950` sampling admission/delivery. **23 owner calls** from the genuine allocated unit composition match all original node masks, packed IDs, counts, frame/weight words, interpolation admission and complete local banks at the decoder boundary. `280030` remains a required decoder callback; its actual original output is delivered only in the differential. Normal and ASan/UBSan runs pass. An absent decoder fails before any local-bank change.

## Original authority and repeat

The untouched TU1.1 executable is `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`. Actual graph definitions and event/reference arrays come from the original resident Lockout/cache state. The complete instruction listings and unique TU1.5 body matches are recorded beneath `analysis/multiplayer/integration/inventory-source/unit-animation-clock/{u5-correspondence,pose/u5-correspondence}`. The authenticated owned English u5 image is SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

`tools/oracle_unit_animation_manager_1_1.py` adds read-only observation to the immutable equipped-unit-phase fixture. It assigns no new guest input, pose, admission, timing or successful service. Every field of each original six scenario report remains equal to its frozen predecessor, excluding only that predecessor's tool identity. The six new reports and twelve instruction listings repeat byte-identically in a separate process: **19 files** including the index. Its inherited explicitly preallocated inventory, processed commands, resource residency, external effects/audio/attachment services and finite `dt=3D088889` remain declared fixture boundaries.

`tools/oracle_unit_animation_pose_1_1.py` instead observes the unchanged genuinely allocated owning-player/item composition from [MP_SPARTAN_OWNED_POSE_RESEARCH.md](MP_SPARTAN_OWNED_POSE_RESEARCH.md). All prior **calls, states, reads, stores and points** reproduce exactly. Its new **212 sampler/resource calls** and raw outputs independently repeat byte-identically. It does not choose a clip, mask, pose, physical mode, root, contact or PVS bit. The separately invoked root schedule and inherited finite clock interval remain explicit; this is not a complete admitted source frame.

The exported expectations are original recorded buffers and cache bytes. There is no native-derived expected geometry. For `280030`'s interpolation stack word, only the actually defined low byte is represented; its inherited uninitialized upper stack bits are not invented. The two allocated-manager cases are selected by the actual ESI manager pointer belonging to datum `E26F0000`; other world-item manager calls are excluded, rather than being mislabeled as player calls.

## Exact ownership and integration order

The allocated Spartan stores the manager size/offset at unit `+128/+12A` and the extension size/offset at `+33C/+33E`. Their literal retained extents are `90` and `124` bytes. The manager owns channels at `+00`, `+20`, `+40`. The equipped weapon overlay lives at **extension+9C**, with further extension channels at `+BC/+DC`; it is not manager+20.

Each 32-byte channel retains graph datum `+00`, packed inherited-graph/clip shorts `+04/+06`, opaque lookup fields `+08..+0F`, delay byte `+10`, status byte `+11`, options `+12`, crossed/future event masks `+14/+16`, rate float `+18`, and frame float `+1C`. The manager also retains transition counters `+60/+64`, master graph `+68`, flags `+6C/+6E`, mode/class/weapon/activity StringIDs `+70/+74/+78/+7C`, and layer weight `+80`. These are preserved source packets, not a parallel collection of approximate gameplay states.

The original allocated-unit ordinary/new-object path establishes this order:

1. `BD360 → 1D0ED0` advances the two manager counters. The `+64` counter advances first; its completed state clears bytes `+65,+64,+67` in that order. Then `+60` advances. This is a separate phase from the channel clock.
2. `10CDF0 → C7E90` consumes control/action state, followed by `DE960 → E1800 → ... → E44C0` producing the actual activity request. `C8140` owns the earlier command/control packet stores, including the motion request at unit `+1B0`.
3. `112E10` executes request/selection owners and then `1CF100`; the latter updates valid manager channels in ascending order, performs third-channel completion, updates layer event flags and optionally synchronizes manager+20 to the primary fraction. Actual secondary extension-channel clocks occur later in `112E10`; the new generic channel function can update those literal packets when its real caller admits them.
4. Dirty local processing `BDF80 → BEA50 → 1D0F80 → 1CA950 → 280030` uses actual retained channels, mask and node count. The genuine player case delivers **21 local nodes** during this callback stage; the complete allocated bank has **33 nodes**. Do not pass a guessed count of 33 everywhere.
5. `113830` applies the unit callback, including its current aiming/IK/action overlay producers. `BA1C0` then composes current matrices with actual roots/parents. Those consumers cannot be replaced by declaring the sampled bank a completed current collision pose.

The allocated fixture is player `EC700000`, biped `E26F0000`, actual starting SMG `E2700001`, authored start37/cluster4 and source PVS `FFDF0000`. The source initial equip has primary clip185 idle59 and secondary NONE. The older explicitly preallocated fixture additionally exercised ready207, firing1726, full reload215 and empty reload217; its initial ready overlay is not substituted for the allocated initial state. Neither suite supplies a second carried weapon.

## Clock, events and arithmetic

`1CA2B0` first clears both event masks. Positive delay decrements and returns. With option bit0 and status bits0/3 clear, it delivers `((dt * rate) * 30) + frame`, preserving three binary32 rounding points. `1CA090` clears status bits1..3, traverses original timing/sound/effect records, rechecks the complete packed ID after each whole callback traversal, and performs original completion/loop/variant ownership. The end threshold is `frame_count - 38D1B717`; loop completion sets status bit2, while non-loop completion sets bit3. `1C9F50` clamps and marks next-step completion; `1CA3A0` independently produces the future timing mask.

`1CAE40` consumes the recorded authored order, traversing timing, then sound, then effect arrays. It retains exact interval endpoints, suppression/direction/reference flag gates and 20-byte callback packets. The caller supplies the real callback identity/context. A nonzero original callback identity with no consumer throws; a missing variant/shared-RNG producer throws before selecting a variant. There is no successful default or default repeated clip. Definitions must remain valid across callback execution, and callers must preserve packet aliasing.

`1CF070` uses the original `3EE66666/3F0CCCCD` layer thresholds and `0380/FC7F` event-option masks. The third completion transition requests duration bits `3E88B439` from the actual `1CEFE0` owner, rather than installing a fixed native tick count. Its positive completion branch, valid manager+20 synchronization and absent-master failure branch are source-listed-only in this measured clock suite. The interface retains the required producers; the report does not claim their positive dynamic coverage.

`1CA950` uses the actual channel delay and definition+10, option bit6, channel rate and source frequency to decide fractional sampling. It selects root graph+5C or+7C masks from option bits11/10, preserving their priority, intersections and aliasing. Inherited-node mapping is delegated only when the packed low short requires it. The original `1D0F80` layer helper `1D18F0` and override-root producer `1D1B00` are required callbacks on their admitted branches. Neither branch, graph-mask intersection, delayed-type early return nor inherited-map delivery is positively exercised by the 23 genuine neutral cases. Their complete source bodies are preserved and uniquely matched to u5; absence is not treated as a successful pose.

The native numeric contract is finite inputs under the measured masked, nearest scalar-SSE environment. It preserves binary32 operands/rounding, signed-byte counter comparisons and exact stored bits. CPU exception/status flags and nonfinite input branches are not represented; unmeasured nonfinite inputs stop explicitly. No Xbox RCP/RSQRT seed or full packed-codec parity is inferred from these owner comparisons.

## Verified 1.5 correspondence

| TU1.1 | Authenticated u5 | Owner |
|---|---|---|
|1CF100|1D39C0|Manager clock|
|1CA2B0|1CEB70|Channel clock|
|1CA090|1CE950|Set/advance channel|
|1C9F50|1CE810|Frame clamp/status|
|1CA3A0|1CEC60|Future timing mask|
|1CAE40|1CF700|Event traversal|
|1CF070|1D3930|Layer options|
|1D0ED0|1D5790|Counter phase|
|1DCD10|1E15D0|Counter advance|
|1CEDA0|1D3660|Variant boundary|
|1CEFE0|1D38A0|Save/start transition boundary|
|C04F0|C1630|Unit event boundary|
|1CA950|1CF210|Local sampler delivery|
|1D0F80|1D5840|Manager local scheduling|
|280630|284090|Mask intersection|
|1D18F0|1D61B0|Blend channel boundary|
|1D1B00|1D63C0|Override root boundary|
|1DE9E0|1E32A0|Inherited mapping boundary|
|1E13C0|1E5C80|Resource notification boundary|

All are unique full normalized-body/internal-branch matches with original encoded bytes, paired direct calls and memory operands. This is static correspondence, not a constructed u5 player/pose execution. External callees remain separately owned boundaries.

## Live blockers and concrete registration

Current native search finds `SourcePlayerControl`/`produce_source_player_control` only in the producer module and tests; it has no live caller. Native `Unit` does not retain these manager/channel/local packets. `Body.grounded` cannot stand in for unit+3DC, and world velocity cannot stand in for the source command request/classifier. The minimum live adapter must retain constructor/equip-owned packets, deliver real `Controls` through the proven command and C8140 consumers, run the actual physical/request/selector phase, retain variant RNG and clock ownership, and supply actual graph/event/decoder/current-unit callback services. `110BF0/111500/1D0A60/1CFDE0/115650`, `1D14F0` layer-motion production, `113830/IK`, and current collision-matrix ownership are still outside these modules. Existing Cache named-action decoding is useful lower data, but cannot silently replace the original selection/admission owners.

Root can register the isolated modules now; no shared production file was edited:

```cmake
target_sources(halo2_systems PRIVATE src/unit_animation.cpp src/unit_animation_pose.cpp)
add_executable(unit_animation_tests tests/unit_animation_tests.cpp)
target_link_libraries(unit_animation_tests PRIVATE halo2_systems)
add_test(NAME original_unit_animation_manager COMMAND unit_animation_tests
  ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/inventory-source/unit-animation-clock/native-fixtures/fixtures.txt)
add_test(NAME original_owned_unit_animation_manager COMMAND unit_animation_tests
  ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/inventory-source/unit-animation-clock/owned-fixtures/fixtures.txt)
add_executable(unit_animation_pose_tests tests/unit_animation_pose_tests.cpp)
target_link_libraries(unit_animation_pose_tests PRIVATE halo2_systems)
add_test(NAME original_unit_animation_pose_delivery COMMAND unit_animation_pose_tests
  ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/inventory-source/unit-animation-clock/pose/native-fixtures/fixtures.txt)
set_tests_properties(original_unit_animation_manager original_owned_unit_animation_manager
  original_unit_animation_pose_delivery PROPERTIES SKIP_RETURN_CODE 77)
```

The two modules are dependency-light C++20. `tools/validate_unit_animation_native.py` compiles both with `-ffp-contract=off -Wall -Wextra -Werror`, compares normal and ASan/UBSan results, and records source/fixture pins. These are owner/boundary comparisons, not a live-game or Xbox-hardware validation claim.

Principal frozen pins, beneath `analysis/multiplayer/integration/inventory-source/unit-animation-clock`:

| Artifact | SHA-256 |
|---|---|
|measured/report.json|40b9954c42c3d2469e472535ba0d1a240cdb44983e7d08944a9b5935d01333b5|
|native-fixtures/fixtures.txt|41117ae68cdba0c434eef35d389bf4df9f0e5476a70a2ae42793ce4777f3e5bd|
|owned-fixtures/fixtures.txt|6d98f6584f7a94405ea159f32c06fd4bb68038111d56775193a13b549efda055|
|pose/measured.json|092ba9cdb4413d472692683022799bd1f403f56b96118c315e6d4e2de664246c|
|pose/native-fixtures/fixtures.txt|84e54eab8f961ff406d7ba2abde064347bee0e483100ac1bde40edd9b9d4ff07|
|u5-correspondence/report.json|2fea1c4c55eeb003ae40ba5f7e90e14ba5eeb7f67d99fb4c1446c2c15d4bf807|
|pose/u5-correspondence/report.json|e303908247990c266cce844dc832378aad19e2324abcf0c24534208ca6d5a701|
|validation/final/report.json|67571577d4287525ae42db9ed0b68c54f03502070428e5fb7286afab9db4d81d|

The companion `manifest.json` pins new module/tool/doc bytes, every measured/repeated source report and listing, exports, validations and all preserved prior dependencies.
