# Original camouflage vertex externs and bounded pass preparation

Whole original `19BE0` copies scalar externs directly and copies RGB externs
without disturbing alpha. Its destination high bits select a texture input;
they do **not** request scalar inversion. The new oracle preserves 9,124
complete-owner cases, including all 420 earlier compiled camouflage cases.
Separate evidence executes the color40 state consumer and starts `16B90` at
entry through each reachable genuine resource/device boundary. This admits
no framebuffer effect, full camouflage pass binder, King normal flow, or
remaining gamemode.
[Vertex fixture](../tests/fixtures/camouflage_vertex_extern_original.json),
[vertex provenance](../analysis/multiplayer/game-variants/camouflage-binder-20261011/vertex-report.json),
[state and pass-prefix evidence](../analysis/multiplayer/game-variants/camouflage-binder-20261011/binder-report.json)

## Authority and execution boundaries

The authority remains retail `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`,
and unchanged Lockout metadata, SHA-256
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
The earlier pass report remains byte-identical at
`152fca115d6c5e0f5a528873b2f95938446aa1b29b6607fc61ec07ff5d0887c2`.
New tools assert every executed instruction matches the original image,
preserve complete indexed owner listings, and supply no patched instruction
or function return. Declared bank, object, fog, state and command-buffer
inputs are identified separately from authored metadata.
[Immutable earlier pass evidence](../analysis/multiplayer/game-variants/camouflage-pass-20261011/report.json),
[vertex oracle](../tools/oracle_camouflage_vertex_extern.py),
[state/boundary oracle](../tools/oracle_camouflage_binder_boundaries.py)

## Exact vertex argument contract

`19BE0` reads four-byte extern rows from postprocess block `+3C`. The packed
range contains a nine-bit first index and seven-bit count. Bytes0..1 are
the destination, byte3 is the source index, and byte2 is ignored. Destination
bits0..3 choose register0..15, bit4 chooses bank0/1, and bits5..9 choose the
component. Bits10..12 do not affect this consumer. Bits13..15 are a texture
selector passed as the **first** stack argument to `1B2C0`; the **second**
argument is literal0, the scalar mode. Specifically, `19BE0` pushes0 at
`19C40` then the high destination bits at `19C42`. After saving ESI,
`1B2C0` reads first argument `[ESP+8]` into ESI, and second argument
`[ESP+C]` into ECX on the scalar branch. The fixture now records these
actual callee arguments for every reached call.
[Complete19BE0](../analysis/multiplayer/game-variants/camouflage-binder-20261011/vertex_extern-00019be0.asm),
[complete1B2C0](../analysis/multiplayer/game-variants/camouflage-binder-20261011/extern_value-0001b2c0.asm),
[observed callee inputs](../tests/fixtures/camouflage_vertex_extern_original.json)

For components0..3, the reached mode0 branch copies source bits from
`bank+5C+source*4` to the selected axis. It performs no clamp or inverse.
For component4, it copies the three words at `bank+108+source*12` to RGB,
preserving W; the texture selector and scalar mode are unused. Each row
ORs `1<<register` into the appropriate dirty word at `bank+1730/1734`,
including writes whose value was already equal. Bank0 constants begin
at `+1530`, bank1 at `+1630`. Whole `18E80` later uploads bank1 registerN
to shader constant `18+N` and clears only that register's dirty bit.
[Read/write and dirty-mask instructions](../analysis/multiplayer/game-variants/camouflage-binder-20261011/vertex_extern-00019be0.asm),
[scalar/RGB branch instructions](../analysis/multiplayer/game-variants/camouflage-binder-20261011/extern_value-0001b2c0.asm),
[source bank1 upload owner](../analysis/multiplayer/game-variants/camouflage-binder-20261011/vertex_upload-00018e80.asm)

The callee has an inverse scalar branch, cosine/sine branches and texture
dimension branches, but `19BE0` does not request those scalar modes. This
research does not extend dimension admission. The first version of the new
fixture mislabeled destination bits13..15 as `mode`; the original outputs
were already direct copies. The corrected fixture calls them
`texture_selector`, records `scalar_mode=0`, and adds observed arguments.
Every output bank remained unchanged. Case4320, raw mapping `00200000`
with scalar-2, returns-2 and demonstrates why naming the high bits an inverse
mode was incorrect. The final fixture SHA-256 is
`3fdd71f1d21c78ec0a679a3b0d214e8fdc9239e03c208d67429b513d4948174d`.
[Correction and final fixture provenance](../analysis/multiplayer/game-variants/camouflage-binder-20261011/vertex-report.json)

The 8,640 finite single-row cases cover components0..4, both banks, all16
registers, texture selectors0/1, ignored byte2 values0/A5/FF, nonzero
ignored destination bits, nonzero untouched axes, existing dirty masks,
negative/positive finite values, both signed zeros and fade residue
`31C00000`. Another64 cases check overlapping-row order. The remaining420
import immutable original `1AE70` source-bank outputs and reproduce every
earlier vertex bank and mask exactly. An independent audit of all9,124
results found 7,504 direct scalar writes and 1,876 RGB writes, with exact
untouched words, ordered writes and mask OR behavior. Nonfinite input
policy and dimension components are outside these cases.
[Fixture inputs and outputs](../tests/fixtures/camouflage_vertex_extern_original.json),
[derived interpretation audit](../analysis/multiplayer/game-variants/camouflage-binder-20261011/vertex-contract-audit.json),
[fresh reproduction manifest](../analysis/multiplayer/game-variants/camouflage-binder-20261011/vertex-reproduction.json)

The actual authored rows are narrower:

| Layer/implementation | Raw row | Destination | Source |
| --- | --- | --- | --- |
|18, implementation0/1/2|`591C0025`|bank1/register9/Z|scalar37|
|18, implementation3|none|none|none|
|19, implementation0|`981C0029`|bank1/register8/RGB|color41|
|19, implementation0, next row|`781C0027`|bank1/register8/W|scalar39|

All actual rows use texture selector0. Source `1AE70` separately resolves
the first biped's clamped camouflage scalar37. Color41 and scalar39 come
from the declared original camera/fog context; they are not interchangeable
with scalar37. The420 prior cases preserve their exact produced bits.
[Authored mappings and original banks](../analysis/multiplayer/game-variants/camouflage-pass-20261011/report.json),
[earlier scalar/object source contract](MP_CAMOUFLAGE_RENDER_SOURCE_RESEARCH.md)

## State color40 and order

Layer18 implementation0/2 have raw state-extern row `00020028`; the other
three implementations have no state extern. This row is decoded by
`1A170`, not by the vertex destination decoder. Byte0 selects literal
state-table slot0 at `467020`, whose bytes `4B000000` name render state75;
byte1=2 selects the color branch and byte3 selects **color40**. Color40 is
the clamped camouflage amount repeated in RGB. It is distinct from the
unrelated scalar40 local-user value.
[Literal table and actual ranges](../analysis/multiplayer/game-variants/camouflage-binder-20261011/binder-report.json),
[complete state consumer](../analysis/multiplayer/game-variants/camouflage-binder-20261011/state_extern-0001a170.asm),
[original color producer evidence](../analysis/multiplayer/game-variants/camouflage-pass-20261011/report.json)

Whole `131ED0` multiplies RGB by binary32 255 using x87, converts with
`FISTP` under the actual control word, masks each integer with FF, and
packs `R<<16 | G<<8 | B`. It contains no clamp. `1A170` replaces state75's
low24 bits and preserves its high alpha byte. For state75, it calls whole
`3F5CF0` using immediate method `4034C`, then stores the resulting word
in the source state bank. The source has no early equality suppression in
this color path. These are actual original CPU command words, with no GPU
acceptance claimed.
[Complete RGB pack owner](../analysis/multiplayer/game-variants/camouflage-binder-20261011/rgb_pack-00131ed0.asm),
[state consumer at1A89C and1A847](../analysis/multiplayer/game-variants/camouflage-binder-20261011/state_extern-0001a170.asm),
[whole immediate-state owner](../analysis/multiplayer/game-variants/camouflage-binder-20261011/immediate_state-003f5cf0.asm)

The 5,040 state compositions independently execute whole `1AE70` with the
420 prior primary/fog/view inputs, then whole `1A170` under three declared
alpha baselines and four rounding controls. They record 6,048 actual
FISTPs; each observes the requested control word at the original instruction,
after the harness preparation reset. Empty authored state ranges return
without writes. For camouflage.25 and alpha7F, nearest rounding emits
`4034C,7F404040`; round-down/chop produce `7F3F3F3F`. All alpha baselines
survive unchanged. This is a complete consumer composition, not proof of
original frame-default state or of the graphics device consuming the packet.
[All state inputs, observed FPCW and outputs](../analysis/multiplayer/game-variants/camouflage-binder-20261011/binder-report.json)

## Entry-to-boundary16B90 evidence

Static `4D0B0` first writes the original draw shader to `485B50`, namely
global bank `485B48+8`. On eligible layers18/19 it replaces the shader
**argument** with the matg rasterizer camouflage override and LOD0. These
are distinct identities: texture extern26 reads the original shader's bump
input, while `16B90` selects the override template/pass. `1BBF0` and
`16B10` govern state reset; `16B10` does not establish all scalar/color
dirty masks. Consequently the new prefixes declare their initial masks;
they do not claim a natural cold draw context.
[Original draw identity and override](../analysis/multiplayer/game-variants/camouflage-binder-20261011/draw_callback-0004d0b0.asm),
[original dispatch](../analysis/multiplayer/game-variants/camouflage-binder-20261011/shader_dispatch-0001bbf0.asm),
[bank reset](../analysis/multiplayer/game-variants/camouflage-binder-20261011/bank_reset-00016b10.asm)

Thirty fresh executions start at `16B90`, with all five actual compiled
implementations, primary0/.25/fade residue, and fog NONE/present. They use
actual original-body shader `E9080783` at bank+8, override `E7E90664` as the
argument, original relocated metadata, a backed current biped, declared
vertex baselines/dirty masks, and backed command-buffer storage. The original
device identity `407488→404FE0` supplies memory for unchanged command
writers; no resource/cache/device function return is supplied.
[Complete declared inputs and prefixes](../analysis/multiplayer/game-variants/camouflage-binder-20261011/binder-report.json)

All24 layer18 prefixes reach real bitmap-cache binder `143C0` before executing
its first instruction. Implementations0/1/2 request the original-body bump
at stage0; implementation3 reaches the override bitmap at stage1 after
its authored parameter binding. They have already executed ordinary states,
texture states, `1AE70`, pixel parameters and overlay preparation, but have
**not** yet executed vertex/pixel/state extern remapping. A successful
resource result is not invented to cross this boundary.
[Reached bitmap arguments and ordered owner entries](../analysis/multiplayer/game-variants/camouflage-binder-20261011/binder-report.json),
[actual texture resolution owner](../analysis/multiplayer/game-variants/camouflage-binder-20261011/texture_extern-00019530.asm)

All six texture-free layer19 prefixes execute vertex/pixel externs, the empty
state/texture-state extern ranges, whole `17420`, constant uploads, and
texture-binding owners, then stop at pixel-program device owner `3FA3D0`
before its first instruction. The report preserves all emitted commands
and 1,376 distinct executed original instructions across the new state and
prefix evidence. These are bounded **prefixes of16B90**; none returns from
the whole pass binder or proves a submitted draw.
[Layer19 prefix order and commands](../analysis/multiplayer/game-variants/camouflage-binder-20261011/binder-report.json),
[program/device boundary](../analysis/multiplayer/game-variants/camouflage-binder-20261011/program_bind-0001b050.asm)

The full static ordering after extern resolution is pixel definition/parameters,
overlay preparation, `18A90` texture resources, `19BE0` vertex externs,
`19CA0` pixel externs, `1A170` state externs, `1AB50` texture-state externs,
then `17420` authored animation, followed by constant/resource/program
publication. Extern binding therefore supplies the baseline **before**
authored animation. A late per-draw overwrite after animation has no support
in this owner. Earlier synthetic whole19CA0→17420 evidence separately shows
why an authored animation may overwrite the same destination.
[Complete original pass binder](../analysis/multiplayer/game-variants/camouflage-binder-20261011/pass_binder-00016b90.asm),
[whole animation selector](../analysis/multiplayer/game-variants/camouflage-binder-20261011/animation-00017420.asm),
[separate composition fixture](../tests/fixtures/camouflage_material_animation_original.json)

## Smallest supported native seam and remaining work

The existing `remap_vertex_constants` is the separate `17000`
parameter-vector consumer. The parent's new `remap_source_vertex_externs`
is the corresponding scalar/RGB seam for `19BE0`; its inputs should remain
separate scalar and color banks, preserve unselected axes, and return dirty
bits for OR into the caller's bank. The new fixture gives independent
expected outputs for that seam. Existing world change-color consumers can
use it without admitting missing camouflage resources. This research does
not perform native comparisons or change native sources.
[Parameter-vector helper](../src/render_program.cpp),
[separate vertex-extern interface](../include/halo2/source_vertex_extern.hpp),
[separate implementation](../src/source_vertex_extern.cpp),
[world bank consumer](../src/world_material.cpp)

The loader currently retains pixel/vertex extern ranges and ordinary state
records; a dynamic state75 consumer additionally needs the authored
state-extern range, the source color bank, and the state word resulting from
ordinary pass state application. RGB packing must preserve that word's
alpha and occur at the source position before animation. These bounded
contracts do not establish texture extern17/26 readiness, compiled vertex
resource acceptance, framebuffer-copy sampling, layer scheduling, depth/color
alias writes or final biped pixels. The separate alias research also records
the pinned xemu backend's unsupported concurrent color/zeta alias handling;
it must not be promoted into a faithful framebuffer oracle.
[Current pass loader](../src/material.cpp),
[current native state application](../src/world_material.cpp),
[original resource/scheduling contract](MP_CAMOUFLAGE_PASS_SOURCE_RESEARCH.md),
[separate alias evidence and limits](MP_CAMOUFLAGE_DEPTH_ALIAS_SOURCE_RESEARCH.md)

Reproduction commands and byte-equality manifests are saved alongside both
reports. The corrected vertex report SHA-256 is
`94d52494ae4c1e55e23adfc5bdf6238d05bede4f35368a444db4a34317ed3694`.
No original source cache, earlier immutable proof, native implementation,
CMake configuration, progress status or gamemode admission was modified.
These graphics-owner executions use the pinned retail image; this note does
not establish title-update graphics-owner equivalence.
[Vertex reproduction](../analysis/multiplayer/game-variants/camouflage-binder-20261011/vertex-reproduction.json),
[state/binder reproduction](../analysis/multiplayer/game-variants/camouflage-binder-20261011/binder-reproduction.json)
