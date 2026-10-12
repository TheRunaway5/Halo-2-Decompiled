# King hill declaration and preparation corridor

The original hill-specific tail installs a stream declaration, clears stream
input bindings, selects the pending authored vertex program, publishes texture
changes, and flushes SDK dirty state. **It supplies no hill-specific cull override
or vertex-constant values in `19F844..19F8AC`.** The helper saves the
current cull value after authored material preparation and restores that value
after its immediate draw. Position0, UV1 and RGBA11 remain the immediate
geometry inputs recovered in the separate emission proof. These conclusions
follow from the complete [original helper listing](../analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/complete/instructions/retail-hill_helper-0019f680.asm),
the [bounded runtime report](../analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/complete/report.json),
and the earlier [emission research](MP_KING_HILL_EMISSION_SOURCE_RESEARCH.md).

This is an explicit CPU composition over genuine captured resources. The
whole center-camera material preparation still stops at its real bitmap request
`12CE00`; its unchanged report is [preserved separately](../analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/complete/preserved-whole-center.json).
Neither this composition nor its CPU packets establish a whole successful hill
material call, GPU command consumption, or pixels.

## Authorities and correspondence

The report pins retail `default.xbe` (`03215919…7935d`), TU1.1
(`aaea4c9c…5377`), and owned English TU1.5 (`21b207e7…c4fd`), along with the
original Lockout snapshot RAM (`f76d0569…24cd`), export, tooling and earlier
King geometry report. Its `static` member contains **27 complete owner listings
per image and 54 complete instruction-shape comparisons**, including internal
branches and switch-table topology. Image-address relocations and referenced
literal bytes are retained, rather than assuming constant address deltas.
The hill descriptor is byte-equal in all three images. Retail alone is executed
against the genuine captured renderer state; the TU evidence here is complete
static correspondence. [Primary bytes and listings](../analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/complete/report.json)

| Owner | Retail | TU1.1 | TU1.5 |
| --- | --- | --- | --- |
| Hill helper | `19F680` | `1A2FE0` | `1A9290` |
| Material/program caller | `4B2D0` | `4B2C0` | `4B2C0` |
| Vertex-context reset | `1C6B0` | `1C690` | `1C690` |
| Vertex-context prepare | `1C710` | `1C6F0` | `1C6F0` |
| Declaration builder | `1C330` | `1C310` | `1C310` |
| Program load | `1C590` | `1C570` | `1C570` |
| SDK program selection | `3F6CF0` | `3FD330` | `400DD0` |
| Texture publication | `1CF50` | `1CF30` | `1CF30` |
| SDK dirty flush | `400FA0` | `4075E0` | `40B080` |
| SDK cull setter | `3F53D0` | `3FBA10` | `3FF4B0` |

The actual absolute operands map as follows. These are recorded instruction
operands, with complete source backing in `static.constants`.

| Meaning | Retail | TU1.1 | TU1.5 |
| --- | --- | --- | --- |
| Vertex context | `51F0F0` | `526980` | `52DCC0` |
| Current cull | `4076E4` | `40DD24` | `4117C4` |
| Stream0 wanted descriptor | `51F17C` | `526A0C` | `52DD4C` |
| Stream0 cached descriptor | `51F1BC` | `526A4C` | `52DD8C` |
| Descriptor dirty byte | `51F2FC` | `526B8C` | `52DECC` |
| Literal descriptor | `43F8DF` | `445F1F` | `44999F` |

## Exact source order and ABI

After the real `4B2D0` material call, `19F75F..19F844` walks the selected
shader/pass/implementation vertex tag and stores the register whose semantic
ID is3 in frame`+44`. Actual hill metadata has semantics `[0,3]`, so this
register is1. `19F844` reads `[4076E4]` into frame`+24`. It then calls
`1C6B0` with **EDX=context**; this is a register ABI with no stack argument.
The reset preserves all eight program-binding records, active selector`+80`,
program-address word`+84` and pending-selection byte`+88`. It sets stream
count`+2D0=0`, streams-dirty`+2D4=1`, descriptors-dirty`+20C=1`, clears sixteen
12-byte stream records`+210..+2CF`, and clears sixteen wanted pointers`+8C`
and cached pointers`+CC`. [Reset owner](../analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/complete/instructions/retail-vertex_reset-0001c6b0.asm)

The helper compares cached stream0 pointer`[51F1BC]` with literal`43F8DF`,
writes the literal to wanted stream0`[51F17C]`, and sets dirty byte`[51F2FC]=1`
on difference. Because the preceding reset cleared cached pointers, this
comparison differs in these calls. The20 literal bytes are
`3b00020402060205020301ff0000000000000000`. The builder skips the first byte,
then consumes signed semantic/type pairs, stopping at semantic`FF`:

| Semantic ID | Type selector | Byte offset before element | Original converted format |
| --- | --- | --- | --- |
| 0 | 2 | `00` | `32` |
| 4 | 2 | `0C` | `32` |
| 6 | 2 | `18` | `32` |
| 5 | 2 | `24` | `32` |
| 3 | 1 | `30` | `22` |

Offsets and converted format words in this table are hexadecimal. Whole
`1C330(context, output)` has two stack arguments and `RET8`. It obtains
semantic-to-register mapping from the context's active vertex-tag
classification; every output row initially has stream0, offset0, format2.
It advances the source offset even for semantics absent from the program.
For the actual hill `[0,3]` classification, only register0 becomes
`{stream0,offset0,format0x32}` and register1 becomes
`{stream0,offset0x30,format0x22}`. All other rows, including register11, retain
format2. It transfers wanted pointers into cached pointers, clears wanted
pointers, and clears descriptor-dirty while consuming them. The first descriptor
byte's semantic meaning is not inferred; this owner does not read it.
[Builder](../analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/complete/instructions/retail-declaration_build-0001c330.asm),
[format conversion](../analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/complete/instructions/retail-format_enum-00035790.asm),
[byte sizes](../analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/complete/instructions/retail-format_bytes-00035850.asm)

The helper calls **`1C710(context)` twice**, each with one stack argument and
`RET4`. First it rebuilds the declaration when dirty. With stream count0 and
streams-dirty1, it calls the unchanged SDK stream-input owner
`3F7440(0,0,0)` and clears streams-dirty. The owner's conditional reference-release
paths are retained in the static listing; the captured SDK stream count is0,
so these cases do not exercise a positive reference release. It selects the generated declaration and
program address only when pending-selection`+88` or remaining descriptor-dirty
requires it. The selection ABI is **ECX=declaration, EAX=context`+84`** into
`3F6CF0`, not a stack argument list. It clears pending-selection afterwards.
The immediately repeated `1C710` has no remaining work in all measured cases.
[Prepare owner](../analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/complete/instructions/retail-vertex_prepare-0001c710.asm),
[SDK stream reset](../analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/complete/instructions/retail-sdk_stream_inputs-003f7440.asm),
[SDK selection](../analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/complete/instructions/retail-sdk_program_select-003f6cf0.asm)

Next `1CF50()` compares each of four wanted/cached texture-header pointers;
it calls the real SDK setter only for changed headers. `400FA0()` flushes
already dirty SDK state, retaining real texture/program/header/device inputs.
It is neither a submit/completion barrier nor a hill-specific state recipe.
Finally the helper loads the genuine device/cursor and either proceeds when
cursor<limit or calls actual capacity owner`3FAAE0`. No capacity response is
supplied by this research. [Texture publication](../analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/complete/instructions/retail-texture_publish-0001cf50.asm),
[dirty flush](../analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/complete/instructions/retail-dirty_flush-00400fa0.asm)

## Program, constants and raster distinctions

The hill tail does not load a different vertex program. Its actual caller
`4B2D0` resolves the settings-selected shader's LOD0/layer15/implementation0,
calls authored material owner`1BBF0`, then passes the implementation's vertex
datum to **whole`1C590(context, datum, index0)`**. This ABI uses ESI=context,
EAX=vertex datum and one stack index. For Lockout the datum is`F2CC1147` and its
genuine captured program is132bytes, containing an8-instruction header/payload.
On a changed binding, `1C590` calls SDK program load at slot0, records the
program identity, sets address`+84=0` and pending-selection`+88=1`, and dirties
declarations/streams. Its subsequently pending selection is consumed by the
hill tail as described above. [Owning caller](../analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/complete/instructions/retail-material_and_program_caller-0004b2d0.asm),
[program loader](../analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/complete/instructions/retail-program_load-0001c590.asm),
[SDK program bytes](../analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/complete/instructions/retail-sdk_program_load-003f6c60.asm)

There is no call to the cull setter in`19F844..19F8AC`. After the draw,
`19FC5A..19FC5F` restores the saved frame`+24` through `3F53D0(value)`. That
setter publishes a disable command for0, or enables and sets winding from the
actual nonzero value and current front-winding cache. This is restoration of
the post-material cull value; it does not justify imposing an extra native
hill-only cull constant. [Cull owner](../analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/complete/instructions/retail-sdk_cull-003f53d0.asm)

An adjacent **earlier** operation must not be confused with this tail:
`19F741..19F745` calls `1CDD0` with EAX0 and stack flags0, before authored
material preparation. Static instructions initialize12 words to
`(1,1,1,1;0,0,0,0;1,1,0,0)`. If previous range-transform cache byte`485AF1`
is set, that owner writes those12 words beginning at constant170 (`AA`) and
clears the cache flag. The snapshot has that flag1. This separate conditional
constant reset is statically recovered here; these cases deliberately omit
that earlier corridor and do not claim to execute it.
[Range-reset owner](../analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/complete/instructions/retail-range_constants-0001cdd0.asm),
[constant writer](../analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/complete/instructions/retail-sdk_constant_write-003f7330.asm)

`400FA0` has generic branches capable of additional state/program/constant
publication depending on retained SDK inputs. In this genuine context,
the static SDK header control word is`0x10`, its fog-program branch returns
without writing a constant/program payload, and no `3F7330` call occurs in
the tail. The report preserves those real controls, all emitted packets and
source writes; it does not generalize the208-byte dirty flush to other states.
[Downstream conditional owner](../analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/complete/instructions/retail-dirty_fog_constants-003f6f40.asm)

## Bounded original execution and reproduction

The tool runs whole original`D4BC0`, the hill prefix through its real distance
locals, and its genuine metadata walk, establishing the original frame at
`200FDFC0` with UV register1. Material preparation is explicitly omitted and
its genuine stop preserved. Cases with hill program selection use a separate
declared call stack`20080000` for whole`1C590`, then resume the untouched
observed hill frame. Every executed instruction is checked against original
retail bytes. Genuine device`404FE0`, cursor`83F9EC64`, limit`83FA0FB0`, bitmap
record`804E2434`, shader/program/texture backing and readiness remain inputs;
no instruction, game/SDK return, resource or progress value is substituted.
[Tool](../tools/research_king_hill_preparation.py),
[1038 observed original instructions](../analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/complete/observed-original.asm)

| Case | Whole program load | Tail command bytes | Repeated tail |
| --- | --- | --- | --- |
| Captured weather program | None | 208 | Not requested |
| Explicit authored hill program composition | 140bytes | 216 | Not requested |
| Same hill composition plus repetition | 140bytes | 216 | 0bytes |

All4 partial preparation corridors reached the explicit stop before`19F8AD`;
all36 observed service entries returned through unchanged original code.
The captured weather context has no matching hill declaration semantics and
no pending program selection; it is retained separately from the actual hill
program composition. The hill composition adds the real8-byte start-address
selection packet (`method1EA0`, value0); both contexts flush208bytes from
captured dirty state. Repeated preparation still performs CPU bookkeeping
writes, despite emitting no commands. Cull remains0, the authored material
vertex-vector bank remains byte-equal, the loaded program shadow is unchanged
by the tail, and the hill bitmap record remains byte-equal in all cases.
[Raw cases, events, before/after banks and packets](../analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/complete/report.json)

The [fresh independent-process reproduction](../analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/reproduction.json)
compares every output file byte-for-byte. Early incomplete static decodes and
report-metadata failures remain under`diagnostic-0..4` with explicit failure
records; later successful diagnostics are also retained. The apparent SDK
TU difference in one failed attempt was caused by an inventory labelling an
internal basic block as a function start. Complete SDK CFG traversal with the
actual D3D section recovered equal full owners and switch topology in all
54 final comparisons.

```sh
.tools/python/bin/python tools/research_king_hill_preparation.py \
  --output analysis/multiplayer/game-variants/king-hill-preparation-source-20261012/NEW/report.json
```

The finite native binding opportunity is the real settings-selected material,
its authored vertex program, explicit position0/UV1/RGBA11 immediate values,
and this exact declaration/stream preparation without an invented cull or
post-pass constant override. Actual bitmap readiness, complete authored pass
preparation, GPU submission/consumption and final framebuffer rendering remain
separate requirements. No production, CMake, test, progress or admission file
is changed by this research.
