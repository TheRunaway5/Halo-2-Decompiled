# Original render-state externs and reset limits

Whole `1A170` consumes a seven-row literal descriptor table, scalar or RGB
source banks, and the current state cache. The new fixture preserves 14,792
complete original calls, including same-value re-emission and ordered writes.
State138/157 retain the computed **raw state word** in the cache even though
their original device owners transform the emitted payload. Whole `16B10`
and bounded `1BBF0` prefixes separately establish the supported reset behavior.
This research does not admit a native state importer, graphics publication,
camouflage framebuffer effect or gamemode.
[Source fixture](../tests/fixtures/render_state_extern_original.json),
[provenance, census and instruction evidence](../analysis/multiplayer/game-variants/render-state-extern-20261011/report.json),
[whole-state oracle](../tools/oracle_render_state_extern.py)

The authority remains retail XBE SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`
and unchanged Lockout SHA-256
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
Each executed instruction is checked against the original image. No original
instruction or function return is substituted. Command storage and line-width
device scale1 are declared, backed inputs; successful cache/device/rendering
services are not supplied.
[Pinned inputs and boundaries](../analysis/multiplayer/game-variants/render-state-extern-20261011/report.json)

## Literal rows and record decoding

The render table is exactly the seven relevant descriptors at
`467020..467038`. The immediately adjacent `46703C` starts the **separate**
ten-row texture-state table used by `1AB50`; it must not be admitted as
additional render-state descriptors. Assembly's authored Constants enum
corroborates the seven render choices. That schema supplements the original
instructions and literal bytes rather than replacing their authority.
[Complete1A170](../analysis/multiplayer/game-variants/render-state-extern-20261011/state_extern-0001a170.asm),
[separate1AB50 base/indexing](../analysis/multiplayer/game-variants/render-state-extern-20261011/texture_state_extern-0001ab50.asm),
[authored Constants enum](../tools/reference/assembly/src/Assembly/Plugins/Halo2/spas.xml)

| Index | Literal bytes | State | Scalar conversion byte | Authored label |
| --- | --- | --- | --- | --- |
|0|`4B000000`|75|0|Constant Blend Color|
|1|`4B000100`|75|0|Constant Blend Alpha|
|2|`3D000202`|61|2|Alpha-Test Ref|
|3|`4D000301`|77|1|Depth Bias Slope Scale|
|4|`4E000401`|78|1|Depth Bias|
|5|`9D000501`|157|1|Line Width|
|6|`8A000600`|138|0|Fog Color|

Each four-byte extern row has descriptor index at byte0, provider selector
at byte1, ignored byte2, and provider index at byte3. Provider byte1 exactly1
uses scalar `bank+5C+source*4`; every other value takes RGB at
`bank+108+source*12`. The source range uses nine first-index bits and seven
count bits, preserving row order. `1A170` itself does not bounds-check the
descriptor or provider index. The seven supported descriptors and declared
provider spans are the evidence boundary; reading adjacent data for malformed
indices is not a supported tag contract.
[Literal table and observed cases](../analysis/multiplayer/game-variants/render-state-extern-20261011/report.json),
[complete decoding and iteration](../analysis/multiplayer/game-variants/render-state-extern-20261011/state_extern-0001a170.asm)

For scalar conversion byte0, the source rounds `x*255` with x87 `FISTP`,
shifts the resulting integer left24, and replaces the current state's alpha
byte while preserving its low RGB24. There is no clamp. Byte1 copies the
binary32 bits into the whole state word. Byte2 uses `FISTP(x)` as a signed32
whole-word value. The RGB provider branch ignores that scalar conversion
byte: whole `131ED0` packs RGB and `1A170` replaces low24 bits while
preserving existing alpha. The actual FPCW and produced integers are recorded
at each reached conversion.
[Scalar branches at1A1CD,1A509 and1A801; color branch at1A89C](../analysis/multiplayer/game-variants/render-state-extern-20261011/state_extern-0001a170.asm),
[separate generic RGB conversion corpus](MP_STATE_RGB_PACK_SOURCE_RESEARCH.md)

## State words, command payloads and equal writes

For target states below92, `1A170` obtains the literal method from
`43E6F0+state*4`, calls whole `3F5CF0`, then writes the computed word to
`407498+state*4`. The fixture records `ordered_writes` at the genuine
writer entry, before command emission. Neither this source branch nor the
special owners below suppresses a same-value write. Repeating all7,392
single-row cases with the already-produced cache value emits the same8-byte
command again. Eight additional multirow cases record alpha/RGB carry and
write order within one whole invocation, including equal writes.
[Observed first/repeated/ordered writes](../tests/fixtures/render_state_extern_original.json),
[immediate command owner](../analysis/multiplayer/game-variants/render-state-extern-20261011/immediate_state-003f5cf0.asm)

State138 calls whole `3F5370`. This emits method `402A8`, swapping the
R/B payload bytes, while preserving alpha and green. It stores the original
**unswizzled** input word at `4076C0`, the state138 cache. For example,
scalar.25 with prior `00123456` under nearest rounding produces cached
`40123456`, while the packet payload is `40563412`. A CPU planner's output
must therefore be `40123456`; the packet transform belongs to publication.
[Complete fog-color owner](../analysis/multiplayer/game-variants/render-state-extern-20261011/fog_color-003f5370.asm),
[actual state138 cases](../tests/fixtures/render_state_extern_original.json)

State157 calls whole `3F5730`. It interprets the planned raw word as a float,
multiplies by device `+964` and source constant8, adds.5, stores binary32,
then whole `3FCF00` uses `CVTTSS2SI`. The resulting unsigned comparison
clamps values above511 to511 before emitting method `40380`. The cache
at `40770C` nevertheless receives the **original raw word**. Under the
declared scale1, scalar.25 emits integer2 while cached state157 remains
binary32 `3E800000`. RGB-provider probes also preserve their planned raw
word even when the device owner interprets that word as a nonfinite float.
This is a declared generic consumer probe, not an authored line-width setting.
[Complete line-width owner](../analysis/multiplayer/game-variants/render-state-extern-20261011/line_width-003f5730.asm),
[original truncating conversion](../analysis/multiplayer/game-variants/render-state-extern-20261011/trunc_float-003fcf00.asm),
[actual cache and packet outputs](../tests/fixtures/render_state_extern_original.json)

The 7,392 single-row cases cover all seven descriptors, provider1 and
non1 controls0/2/255, ignored byte2 values0/A5/FF, source indices0/40,
nonzero range first indices, four observed rounding controls, three
declared initial-state patterns, and11 finite scalar/color controls.
Inputs include negative/over1 values, signed zeros, fractional ties, fade
residue and signed-conversion overflow. There are14,784 first/repeated calls
plus eight ordered calls. The known Unicorn FPSW limitation from the RGB
research remains explicit: observed integer/mask outputs do not establish
architectural exception-status delivery or unmasked exceptions. Other
precision controls, nonfinite provider inputs, title-update equivalence and
hardware execution are outside this corpus.
[Fixture coverage and limits](../analysis/multiplayer/game-variants/render-state-extern-20261011/report.json),
[separate FPSW limitation](MP_STATE_RGB_PACK_SOURCE_RESEARCH.md)

## Reset source and authored baseline

The complete Lockout compiled-spas census covers79 pass tags and135
implementations. It finds only two authored render-state extern rows, both
actual camouflage descriptor0/provider2 RGB rows; it finds **no** state75
record in any implementation's `+11C` reset range. Consequently scalar and
other-color cases above are declared whole-consumer probes, distinct from
the authored cache's natural contexts. This census does not cover every
owned map or establish a compiler rule rejecting other combinations.
[Full compiled-spas census](../analysis/multiplayer/game-variants/render-state-extern-20261011/report.json)

Whole `16B10` first applies the previous implementation's `+11C` reset
states through whole `18900`, then clears its selected pass/implementation
and several bank fields. For the five actual camouflage implementations,
layer18/0 and19/0 reset only state57 to`203`; the remaining reset ranges
are empty. All five complete reset calls preserve declared state75
`7F123456`. There is no source basis here for inventing a universal
state75 reset value. For camouflage implementation18/0 and18/2, the ordinary
**selected pass state list** separately supplies baseline `FF000000`
before its color40 extern replaces RGB.
[Whole16B10 and exact reset rows](../tests/fixtures/render_state_extern_original.json),
[complete reset owner](../analysis/multiplayer/game-variants/render-state-extern-20261011/bank_reset-00016b10.asm),
[authored selected camouflage pass states](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/assets.json)

`1BBF0` calls `16B10` only when `4670BC` is nonzero, then applies several
other original state defaults. Its own complete instructions contain no
direct state75 assignment. Five reset-disabled prefixes reach the genuine
`16B90` entry without changing state75. Five reset-enabled prefixes execute
the previous reset range and reach `3F54F0→3F6F40`. The latter requires the
live program pointer at device `+794` and dereferences its flags at+4;
that program is not backed in this declared context. The saved prefixes
stop **before** `3F6F40`, without inventing a valid program or its return.
State75 is unchanged at that boundary. Completion of reset-enabled whole
`1BBF0` and its downstream pass execution remains unverified.
[Complete dispatcher](../analysis/multiplayer/game-variants/render-state-extern-20261011/shader_dispatch-0001bbf0.asm),
[actual reset prefixes](../tests/fixtures/render_state_extern_original.json),
[missing live-program dependency](../analysis/multiplayer/game-variants/render-state-extern-20261011/fog_program_dependency-003f6f40.asm)

The native CPU seam can preserve one ordered `{state,value}` write per
source row, using the correct scalar/color bank and the incoming state cache.
The authored `impl+11E` range belongs separately in pass metadata; it is not
the `+11C` reset range or `+120` texture-state extern range. State75 for the
two actual blending camouflage implementations starts with their ordinary
authored state, then receives color40 before authored animation. The generic
CPU primitive, metadata import, graphics publication and framebuffer proof
remain separate validation claims.
[Source ordering in16B90](../analysis/multiplayer/game-variants/camouflage-binder-20261011/pass_binder-00016b90.asm),
[separate binder evidence](MP_CAMOUFLAGE_BINDER_SOURCE_RESEARCH.md)

The final fixture SHA-256 is
`238cf256236c7d411f8aa5fbd6793df302ec49194a6e069c008130461d985087`;
report SHA-256 is
`48db152c1285a2cba0dff5c413882329ca9b602ad9ce2a6799ef3658b88a8170`.
The reproduction manifest compares a fresh fixture, report and all11 owner
listings byte-for-byte. Existing RGB, vertex and binder evidence remains
immutable. This research changes no native sources, CMake, progress or
gamemode admission.
[Fresh reproduction manifest](../analysis/multiplayer/game-variants/render-state-extern-20261011/reproduction.json)
