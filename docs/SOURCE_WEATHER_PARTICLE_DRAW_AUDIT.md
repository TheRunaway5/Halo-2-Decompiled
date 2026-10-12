The original particle draw caller is recovered as an ordered source interface,
but complete `391F0` execution and native rendering remain unproved. The new
audit inspects literal XBE bytes, reads two immutable Lockout phases, and runs
only the complete original resource query and the actual weather-entry
admission caller on owned clones. It supplies no renderer, readiness, wait,
I/O, random or timing result.

The durable evidence is the
[retained-bank report](../analysis/multiplayer-shading/weather-particle-draw-inspection.json),
[actual-entry report and phase comparison](../analysis/multiplayer-shading/weather-particle-update-entry-inspection.json),
[literal disassembly](../analysis/multiplayer-shading/weather-particle-draw-inspection-disasm.txt),
and [read-only extractor](../tools/audit_source_weather_particle_submission.py).
Code extent hashes and static instruction counts are inspection evidence.
Executed instruction counts occur only in the separately labeled original
query/admission sections. The pinned original XBE is
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.

`36B50` consumes the actual clipped cluster list, not a PVS mask. The signed
current cluster is visibility bank `547F88+8` (`547F90`); its clipped-list
header pointer is `+C` (`547F94`), with signed count at header `+4` and DWORD
cluster list at header `+C`. BSP `4E0348` supplies weather palette `+84/+88`,
cluster rows `+9C/+A0`, current-cluster portal indices `cluster+8C/+90`, portal
rows `BSP+60`, and plane data through `BSP+18 -> +C`. Camera XYZ is
`485618..485620`. The original palette flags and distances occupy 16 slots;
distance starts at 32767, the containing cluster uses -1, and other visible
clusters use the original nearest matching current-cluster portal metric.

View zero first derives elapsed `509418` from the admitted absolute double
`4858A0` and previous float `50941C`, then calls complete `398D0`. Weather
disable is `509420`. For each admitted palette, the source resolves its datum
through tag table `4E3B44`, updates wind on view zero, advances plates with
`37260`, advances particles with `38450`, draws plates with `37A60`, then
draws particles with `391F0`. Update and draw loops have separate chained
AL/CL admission; an update chain is not an interchangeable draw-ready result.

`1320F0` owns visibility state `547F88` for the normal path, or `54AA68` for
the alternate path. It calls `132B80`, `1348E0`, a mode-dependent
`173130`/`132D60`/`1733E0`, then `134390` and `132220`. `131FF0` obtains the
actual shared storage through `VirtualAlloc`, then `134980` initializes both
visibility banks. Camera/view publication is separate: `132D0` copies the
caller’s 0x298-byte bank to `485600`, including signed view slot, camera at
`+18` and projection at `+8C`. The copied bank and its real caller chronology
are necessary inputs; a retained post-view slot is not promoted to an active
slot.

| Immutable phase | View / containing cluster | Actual clipped clusters | Original result |
| --- | --- | --- | --- |
| `lockout-state1`, paused at `12B340` | -1 / 15 | 15, 9, 2 | Admission caller is not replayed; no view 0 is invented. |
| `lockout-weather-update-entry`, paused at `36B50` | 0 / 13 | 13, 15 | Complete `36B50` and `398D0` return; all palette flags are zero. |

Lockout palette 0 is authored on clusters 0, 2, 7, 8 and 9. Thus the first
retained list contains weather clusters while the second actual entry list
does not. The complete entry replay compares all 16 flags and distance words:
zero flags and `46FFFE00` (32767) in every distance. It verifies 144 original
instructions, restores and reads back 14 pages, and reaches no wind,
simulation or draw call. Source clock/command writes occur only on the owned
clone and are retained explicitly. The auxiliary stream is not advanced.
Supplied FPCW is `023F`, MXCSR is `1FA0`, with an explicit fresh empty x87
stack and synthetic callee stack/return; captured general registers remain
unchanged as incoming values. These are diagnostic execution controls, not
a reconstructed full CPU continuation.

Both phases contain palette datum `E77D0608`, weat root `8026E87C` and the
type-1 particle system `8026E92C`. Its published group `819C6008` has 1,024
render rows at `819C603C` (20 bytes each) and state rows at `819CB040` (32
bytes each). The actual descriptor at system `+40` is runtime-linked; cache
handle is `E5FA000B`, cache done is 1, and cache row owns descriptor
`8026E96C`. The authored sprite is `E77E0609` (`snow_flake`). Each complete
unchanged `12DE70` mode-3 query returns AL=1 in 19 verified instructions, with
10 pages restored and no service or nested callee. It only touches physical
row timestamp `7FE601D8` with the geometry manager’s clock. In the retained
phase that word is already `0001EBA4`; the actual-entry clone changes
`0001FA0D` to `000207E5`. No ready or completion result is supplied, and the
cloned query does not reconstruct the earlier read/initialization history.

The two runtime arrays differ from authored decompressed resources. The
retained phase has 637 positive, 189 zero and 198 negative particle ages;
the actual entry has 771 positive, 37 zero and 216 negative ages. Array
hashes, full raw banks and authored payloads are preserved independently.
Those differences establish that raw assets are not current particle state;
they do not establish how many updates or initialization calls occurred.
Elapsed, intensity and auxiliary stream words are retained separately without
advancing the live original or assigning a same-draw history.
The raw system also retains group count `+64`, word `+84`, byte `+86` and
float `+88`. Neither phase's `+86` byte is promoted to an initialization or
readiness claim; the complete resource query supplies the admitted result.

`391F0` takes a 140-byte system pointer on the stack, returns with `RET 4`,
and exposes an AL result. It first calls actual `12DE70` with ECX=`system+40`
and EAX=3. A complete false resource query returns AL=1 immediately and emits
no draw setup. A pending resource service is an unresolved boundary, not a
false or true query. After readiness, `system+68` supplies the published
group: count at `+0`, render pointer at `+4`, state pointer at `+C`.

| Consumed draw input | Original source |
| --- | --- |
| Particle type | WORD system `+6C`; exactly 2 selects lines, other values select points. |
| View partition | Count divided by signed player count at `4E8C20 -> +8`, clamped 1..4; signed view at `485600` selects one equal contiguous range; remainder is discarded. |
| Draw admission | State `+1C` age must be ordered greater than zero; zero, negative and NaN are skipped. |
| Point position / size / color | Render `+0/+4/+8`, render `+C * 16777215`, packed render `+10`. |
| Rain endpoints | Start=`position-(state velocity*509418)*system+78`; end=current position; v0.w=1. |
| Rain alpha | System `+70/+74 * 256`, original x87 `__ftol2`, shifted 24; render RGB retained without a replacement clamp. |
| Rain line width | System `+7C`, passed unchanged to complete SDK `3F5730`. |

The ready path marks `4670BC=1` and calls `16B10` with `485B48`. It selects
VS22 from `485A80 -> +5C -> +B4`, index 0, through shader cache `51F0F0` and
`1C590`. It calls `12FA0` with projection `48568C`, camera `485618`, scaled
byte 0 and mode `4858B8`, then constructs the 60-word original `39780` pixel
program. Non-rain invokes `143C0` with sprite datum in EAX, index ECX=0,
XMM2=0 and stage 3 on the stack. Rain omits that call. `1CF80` subsequently
applies the desired textures on all four stages. The parent audit owns this
complete stage/cache/camera bridge; a raw sprite asset is not a proved bind.

The caller writes the following ordered raw setup methods/values through its
original SDK calls: `40304=1`, `40344=302`, `40348=303`, `40300=1`,
`40340=0`, `4033C=206`, `4035C=0`, `3F54F0(2)`, `40354=203`.
Non-rain also sets `407670/407674=1`, `407678/40767C=0`, `407680=1` and ORs
dirty bank `407490` with `900`. `400FA0` consumes the actual dirty/device
banks before Begin `417FC=1` for points or `417FC=2` for lines. Each vertex
is immediate method `101518` followed by XYZ/W. The real `3FB3E0` color call
uses attribute 1, method `41944`, and swaps the red/blue byte lanes of the
packed word. Push-buffer capacity branches call `3FAAE0`; actual dirty-state,
allocation, pump and completion effects remain separate source services.

Cleanup emits End `417FC=0`, masks device `+8` with `FFFFE7FF`, conditionally
calls `3FA680` for prior bit `1000`, sets `407670/407674=0`, emits
`40304/40300=0` with `407584/407588=0`, and calls `16B10` again with
`4670BC=1`. It does not restore a saved prior state bank, call `3F54F0(0)`,
or reset line width through `3F5730`.

The [projection/VS inspection](../analysis/multiplayer-shading/weather-12fa0-producer-inspection.md)
recovers all `12FA0/12D50` inputs and constant fields. VS22 uses RCP for point
diameter and distinct RCC for screen position. There is no viewport-width
multiplier in point diameter. SDK line width uses x87
`(width * device+964 * 8) + 0.5`, stored float32, CVTTSS2SI and unsigned cap
511. The finite resolution scale producer selects the smaller horizontal and
vertical scale; retained scale is 1. Exact NV2A reciprocal and point/line
raster coverage remain hardware boundaries.

The complete source-inspected SDK `3FF920` adds a separate point-state
dependency. It consumes size/minimum/maximum at `407668/40766C/407684`, the
scale/sprite enables, coefficients `407678/40767C/407680`, device resolution
scale `+964` and unsigned raster height `+EEC`. The scale-enabled branch emits
eight point parameters with method `200A30`; fixed POINTSIZE `4043C` is
separately encoded from retained point size through the SDK conversion. The
complete actual entry/return replay below corrects the earlier minimum
attribution by following the full x87 stack. `391F0`
retains size/minimum/maximum while setting enables and coefficients. Shader
oPts therefore does not close the hardware point-size contract. The report
retains these captured words without assigning them to a historical draw.
SDK `403400` begins stage processing at 3 when sprite enable is nonzero,
otherwise 0, unless device `+784` causes its earlier return. The parent owns
the complete stage binding bridge.

The existing 754-case geometry oracle is retained unchanged as bounded
immediate-input, pixel, clipped-palette and c10/c11 arithmetic authority. It
replaces resource, camera, shader, state flush, line width and SDK command
services. Its color hook emits synthetic `C010C010` and copies packed color;
it does not execute the real `3FB3E0` packing. Its 112 viewport cases compare
only c10/c11. The 448 analytic shader cases use synthetic moderate finite W;
the historical GPU manifest also predates the current shared GLSL hash.
These limits prevent promoting those earlier bounded fixtures to a complete
original draw, projection producer or current renderer proof.

The later actual `391F0` entry and its `36F00` return now have a complete
unchanged replay in `tools/audit_source_weather_particle_entry_original.py`.
The replay uses the incoming registers/stack argument and executes every
reached title/SDK callee without service hooks: 1,728 distinct instructions,
34,021 steps, 67 restored pages, AL=1, 238 point/color pairs. All 8,872
distinct non-stack bytes written by source, all returned GPR/control/XMM values
and the full 7,680-byte command span equal the actual return. Thirteen
surrounding bytes outside the source-written set differ and remain explicit.
The SDK packet contains eight parameters
`{0,0,3C91A2B4,42800000,42800000,42800000,80000000,0}`, modes 1/1 and
fixed POINTSIZE 8 from retained size 1. This is command-staging authority;
GPU consumption, raster coverage and presentation remain separate.

`tools/freeze_source_weather_particle_packet.py` verifies and freezes the
actual program, 48 camera words, 60 pixel words, 1,024 initialized render/state
rows, 238 emitted inputs, point words and full command span in
`analysis/multiplayer-shading/weather-particle-packet/packet.json` plus
`manifest.json`. The parent draw audit documents the independent complete
sprite binding and its actual four-mip, 1,408-byte resource. No input was
fit to a screenshot and no native live clock/readiness policy is supplied.

The smallest complete native seam would take actual active view/camera and
projection banks, actual clipped visibility, exclusively owned initialized
particle/group arrays, the real resource admission result or unresolved
boundary, source elapsed/view count, authored sprite/shader/pixel metadata,
and actual stage/device/command state. It would return ordered state/vertex
events plus every source bank mutation, preserving resource skip and command
service boundaries separately. A pure geometry vector or an assumed ready
sprite omits those contracts. No such helper or renderer is implemented here,
and no production, CMake, GLSL, GPU, guest or session files are modified.
