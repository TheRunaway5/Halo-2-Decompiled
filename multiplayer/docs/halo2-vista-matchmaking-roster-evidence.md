# Vista matchmaking roster and active progress circle

Research recorded 2026-10-12. This investigation changes only this evidence note and its private research artifacts. It does not modify product source, builds, deployments, existing evidence or user identities. All executable controls use unchanged original instructions; stopped service boundaries are explicitly identified below.

The original Vista screen210 has an authored roster definition but its recovered constructor/setup does **not** supply a runtime list owner. There is no recovered special row/font override on that path. Rendering a chosen-name roster is therefore a declared native accommodation. The active step's animated circle is the actual `halo_ring2` scenery model with a looping `idle` clip. Its Vista scene viewport uses the original height/1200 coordinate multiplier, including per-boundary integer truncation; it is not exempt because screen182 happens to share Xbox authored bounds. Animation-payload equality does not establish Xbox/Vista model, shader or texture equality.

## Primary sources and artifacts

| Source | SHA-256 |
| --- | --- |
| Supplied Vista `Halo 2 Vista installer/halo2.exe` | `33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109` |
| Original `assets/disc/default.xbe` | `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d` |
| Original Xbox `assets/disc/maps/mainmenu.map` | `1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b` |
| Actual Vista `assets/imported/a13438f3719c48bcb914624ed35022c7/vista/maps/mainmenu.map` | `eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44` |

The complete artifact directory is [vista-matchmaking-roster-research-20261012](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012). The inputs are the owned original executable/cache/font bytes, not screenshots or recreated layouts. Original PC code is manually PE-section-mapped; Xbox code is loaded from its actual XBE sections.

- [extract_roster_assets.py](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/extract_roster_assets.py) preserves skin20, font headers, every progress bitmap and all shared-animation records in `roster-assets.json`.
- [oracle_roster_controls.py](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/oracle_roster_controls.py) executes11 bounded controls covering117 distinct original instructions, producing `vista-roster-fixture.json` and an original-instruction trace.
- [extract_ring_assets.py](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/extract_ring_assets.py) preserves the actual scenario placement, scen/hlmt/mode/jmad owner records, geometry resources and screen182/210 model scene records in `ring-assets.json`.
- [oracle_ring_controls.py](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/oracle_ring_controls.py) executes6 actual scale-producer controls,48 scene viewport controls,768 ring clock controls and one scenery named-idle argument control. It verifies every reached instruction against the original image and writes `ring-controls-fixture.json`. It does not import the native implementation.
- [compare_ring_materials.py](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/compare_ring_materials.py) independently compares the two caches' declared vertex input streams, actual shader constant banks, bitmap headers and stored top-mip payloads. Its result is `ring-geometry-material-comparison.json`; it is not a completed shader/framebuffer comparison.
- [extract_vista_ring_pass.py](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/extract_vista_ring_pass.py) preserves the actual PC pass, template remaps, ten compiled shader records, and five textures with all 71 stored face/mip blocks in `vista-ring-pass.json`, `ring-shaders/` and `ring-textures/`.
- [oracle_vista_ring_material.py](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/oracle_vista_ring_material.py) executes 12 bounded original-byte controls covering 1,391 distinct original instructions. It produces `vista-ring-material-fixture.json`, including original static material staging, final RGBA constant reordering, flags 0–3 decompression, the actual class-2 caller gate, conditional fog production and all three compiled PS selectors.

Reproduce the seven scripts above using `.tools/python/bin/python` from the repository root, in the listed order. They write only this artifact directory. The inspection module `inspect_original_roster.py` is imported read-only by the controls; its exploratory disassembly entrypoint is not required. Artifact hashes are frozen in `frozen-evidence.json` and the concluding provenance section below.

## Vista210 supplies no custom runtime roster owner

Vista screen210 is datum `e62d040b`, `ui\screens\game_shell\xbox_live\matchmaking_progress_dialog\matchmaking_progress_screen`; it has flags18 and three panes. The functions around `6463AB..6466` instead construct **screen14**, the PC pregame lobby, with its custom list objects. They are not a source-backed roster/font policy for screen210. The actual210 constructor is **646AD0**, factory646CEF, vtable7D4074, setup virtual+58=**6468AF**, update virtual+0C=646BA6. Primary disassemblies: [646AD0](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-00646ad0.asm), [6463AB](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-006463ab.asm), [6468AF](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-006468af.asm).

The executed setup block begins at6468BF, after the real screen-to-tag resolver's return. EAX supplies the actual screen datum; the original real CRT `memset687BA9` clears the descriptor. The resulting26DWORDs are exactly `[0,3] +24zeros`. Setup then passes the descriptor and tag to610A44. This producer does not construct a list, row object, font object or text-bound override.

The original initializer gate610A82, with the actual flags18/pane-count3, passes that descriptor to6107DF with its false low-byte flag and selects pane0. Loader61095D uses the signed current-pane WORD at `this+9F8`, indexes a16-byte descriptor row, reads its list pointer at `descriptor+10+pane*16`, and **skips** both61208E attach and614990 list initialization when that pointer is null. Controls cover all three null rows plus positive-pointer controls that reach the actual attach boundary. They stop before attachment/refresh services instead of fabricating those services. Primary disassemblies: [610A44](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-00610a44.asm), [6107DF](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-006107df.asm), [61208E](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-0061208e.asm).

The complete derived constructor suffix646AEE through return writes its vtable; six UTF16 string terminators; two object flags; and shared flagD78DFD. All ten writes are retained in the fixture. No list/font/bounds allocation or modification occurs there. The common refresh60FB1B separately creates pane text, bitmap, HUD/grid/model widgets; it does not manufacture the missing descriptor-owned roster list. See [60FB1B](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-0060fb1b.asm).

This narrows the earlier [progress note](/home/eric/Developer/Halo-2-Decompiled/multiplayer/docs/halo2-matchmaking-progress-evidence.md): its statements about one16-slot roster **definition** per pane remain raw-cache facts. They do not prove that original Vista210 actually instantiated/displayed that roster. No original screenshot/full Windows process execution is claimed by this constructor/loader corpus.

## Authored row and font density boundary

Each actual Vista/Xbox skin20 has the same one44-byte text record: flags1, font selector1, bounds `(top,left,bottom,right)=[27,36,7,220]`, a20-unit-high box. Vista text record file offset60302224 is preserved verbatim in `roster-assets.json`. No investigated210 custom callback changes it.

The font table entry read by the native importer is `conduit-12`. Actual font headers at+204 contain ascent/descent/leading **Xbox15/4/0**, **Vista29/8/0**. Vista font SHA-256 is `70b569cb17ad5a7dcedc2d134e0c387038e8a78ff263b57e0772d9788f4a6675`; Xbox font SHA-256 is `337dfeb3b9dd787bcd9309d2efd8d8b3d3bcac2a0169000d0249abc131dfdab8`. These are asset/header facts, not proof of the complete original logical-font-provider mapping, physical glyph upload or rasterizer behavior. The original logical-font service has its own mapping/readiness ownership, documented separately in [font-measure research](/home/eric/Developer/Halo-2-Decompiled/analysis/frontend/vista-session-flow-audit/widget-font-measure-original-20261012/README.md).

A native chosen-name roster can retain the actual PC pane/list/skin geometry while applying explicitly documented density normalization to **this skin only**, such as original logical Xbox metrics with actual high-resolution Vista glyph samples. That is a native accommodation for a newly bound roster, not a recovered original Vista row callback. Do not enlarge all fonts/rows or assert that original Vista used the Xbox font file. The exact native draw/hit bounds and readable names still require rendered native validation.

## The active step circle is a model animation

All three actual Vista210 model-scene records name the single scene object **halo_ring2** and light **ui_basic1**. Their camera is the same `[-5.75,-19,-22.760000228881836]`, FOV97; their viewports move across the three numbered steps:

| Pane | `(top,left,bottom,right)` | Scene file offset |
| --- | --- | --- |
| 0 | `[308,-440,146,-282]` | 60442324 |
| 1 | `[308,-260,146,-102]` | 60443084 |
| 2 | `[308,-78,146,80]` | 60444008 |

`halo_ring2` resolves through scenario scenery palette→scen→hlmt→mode/jmad to `scenarios\ui\mainmenu\ui_polygons\progress_symbol\progress_symbol` in both caches. The scenario placements have identical position/rotation/scale words, including scale `.004999999888241291`. The exact records and offsets are retained in `ring-assets.json`.

Both actual graphs have one named **idle StringID0400000C**,121frames, **loop_frame1**, siblings `[-1,-1]`, and all three event counts0. Xbox animation record starts file58601392, stride108; Vista starts55925956, stride96. The actual codec payload is408bytes (40static+344animated+24masks), **byte-identical** in both caches, SHA-256 `aae46b1c5d4cee32c2d5a0afd62a46931fe82ecb28bbb9a38dd85e492049989f`.

The separate Vista `done` bitmap is one128×128 frame with no sequence; every210 bitmap's FPS is0. Shared animation31 has250ms intro/outro keys affecting alpha/XYZ, loop0 and **no ambient keys**. Those bitmap records do not supply a looping spin. The source-backed active circle is the scenery model, not an invented rotated bitmap or guessed angular velocity.

Original SCEN constructor10A1B0 invokes **10A3F0**, which clears active flag0 then, if an animation-state offset and graph exist, requests **10A660(graph,object,idle,frame0,NONE,false,true)**. On success it sets active flag0. The new unchanged-byte producer control verifies the complete arguments and graph at the actual10A660 call boundary; it does not replace or pretend to execute that action-selection service. Primary original disassemblies are [10A1B0](/home/eric/Developer/Halo-2-Decompiled/analysis/frontend/camera-research-10a1b0.txt), [10A3F0](/home/eric/Developer/Halo-2-Decompiled/analysis/frontend/camera-research-10a3f0.txt), [10A660](/home/eric/Developer/Halo-2-Decompiled/analysis/frontend/camera-research-10a660.txt); decompiled owner [scenery.cpp:290](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/scenery.cpp:290).

For the actual ring metadata, the new original1C68C0 controls execute its real callees with the actual graph/clip bytes. They cover600 successive explicitly supplied1/30-second ticks and168 endpoint/pause/delay probes at six supplied clock deltas. All768 cases match the separately calculated binary32 frame/rate/status/delay formula. There are no event callbacks or RNG writes. The loop endpoint is `float(121)-floatword38D1B717`, the increment is separately rounded `(dt*rate)*30`, and loop1 wraps by `(advanced-end)+1`, preserving fractional excess. The first continuous test wrap occurs at tick120, followed by tick240; this is a result of the declared1/30 input, not proof of the original live scheduler or an independently guessed four-second wall clock.

The already existing `frontend_model_clock` admits loop1 and expresses this formula. `source_scenery_idle_phase` currently admits only loop0 and must not be presented as a ring clock owner without changing its admission and independently validating the change. `Cache::load_initialized_object_idle` expresses the source named-idle selector, but its animation metadata traversal has Xbox strides; codec-payload equality alone does not make its metadata walker Vista-safe. The concrete native preparation gap observed at investigation time was that non-bipd requested models were not initialized with a clip; the parent implementation owns the progress-ring-only fix and its rendered validation. No product change is part of this research.

## Exact Vista model viewport owner

Common pane refresh60FB1B creates a model-scene widget through **60E94D→6404CA**. The ctor retains the definition pointer at+70, copies the viewport at definition+38 into widget bounds through6116D2, and sets vtable7D3AB4. Its render virtual+10 is **6407BD**. The first render argument, retained in EBP at6407F8, is a signed16-bit `(top,left,bottom,right)` drawable rectangle. Render checks the original render-mode/object service gates before reaching the bounded arithmetic prefix. This corpus enters that prefix with explicit resident inputs; it does not fabricate those prior services. Primary: [60E94D](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-0060e94d.asm), [6404CA](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-006404ca.asm), [6407BD prefix](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-006407bd.asm), [64090D viewport arithmetic](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-0064090d.asm).

**65E191→664979** produces E3E424 from actual integer drawable height multiplied by original binary32 word **3A5A740E** at7DBD6C (reciprocal1200). The six producer controls run its real664979 callee and stop65E1EA before the next unrelated service. No substitute scale provider is used. Primary: [65E191](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-0065e191.asm), [664979](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-00664979.asm).

The unchanged render block **64090D..6409A2** computes, in this order:

1. Signed WORD differences `width=right-left`, `height=bottom-top`; arithmetic integer shifts `half_width=width>>1`, `half_height=height>>1`, then CVTSI2SS conversions. The input rectangle's origin is not added back by this prefix.
2. `left=CVTTSS2SI(float(half_width)+MULSS(float(authored_left),scale))`.
3. `top=CVTTSS2SI(float(half_height)-MULSS(float(authored_top),scale))`.
4. `right=CVTTSS2SI(float(half_width)+MULSS(float(authored_right),scale))`.
5. `bottom=CVTTSS2SI(float(half_height)-MULSS(float(authored_bottom),scale))`.

Each MULSS and subsequent ADDSS/SUBSS rounds separately; each boundary independently truncates through CVTTSS2SI. Source physical width/height are differences of these **already truncated** boundaries. A single scaled span or floating half-center is not equivalent. At1001×601 the integer center is `(500,300)`, not `(500.5,300.5)`.

| Drawable | Pane0 physical `(top,left,bottom,right)` | Pane1 | Pane2 |
| --- | --- | --- | --- |
| 640×480 | `[116,144,181,207]` | `[116,216,181,279]` | `[116,288,181,352]` |
| 1280×720 | `[175,376,272,470]` | `[175,484,272,578]` | `[175,593,272,688]` |
| 2560×1080 | `[262,884,408,1026]` | `[262,1046,408,1188]` | `[262,1209,408,1352]` |
| 1001×601 | `[145,279,226,358]` | `[145,369,226,448]` | `[145,460,226,540]` |

All48 controls retain input WORDs, exact scale/output words and expected outputs: four actual PC scene records (three210 plus182), six dimensions (also1920×1080/3440×1440), and two rectangle origins. Screen182's actual `[39,-55,-87,55]` also scales; at1280×720 it becomes `[336,607,412,673]`. No screen-id182 exception exists in this prefix. Equal authored Xbox/PC182 bounds do not justify passing all PC scene bounds through a640×480 native canvas unchanged.

The native renderer may carry the exact original physical rectangle separately from logical canvas coordinates, using it directly for viewport/scissor/frustum construction. An inverse logical conversion is an adapter boundary and must not change the source rectangle through a second round of truncation. This note does not establish Vulkan/D3D pixel-center equivalence, completed source frustum setup or full-frame pixel parity.

## Vista artwork and native decoder limits

Both ring modes have1,622 class2 vertices and identical4,038-byte index streams (SHA-256 `5a3940aead379214549bed827f611d3e38ff58333eac1e965328b01e61eda234`), matching bone-index bytes and matching source compression-bound records. **The vertex streams are not identical**: Xbox uses position/UV/basis strides8/4/12; Vista16/8/36. Comparing declared normalized SHORT inputs with actual PC FLOAT inputs gives only599/4,866 position component words and1,112/3,244 UV words identical. Maximum position input deltas per axis are approximately `4.54187e-5,4.25577e-5,2.38419e-7`; UV deltas approximately `4.45247e-5,4.44651e-5`. The comparison preserves basis lanes by physical order, avoiding an inferred cross-platform tangent/binormal mapping. These are declared vertex-input comparisons, not final world-space geometry or a pixel-error bound.

The actual shaders are `scenarios\ui\mainmenu\ui_polygons\shaders\polygons` and `...\progress_illuminated`, with templates `transparent\one_alpha_env` and `transparent\one_alpha_env_illum`. Their vertex-constant bytes and bitmap paths match, but pixel constant DWORD order differs:

| Shader | Xbox DWORDs | Vista DWORDs |
| --- | --- | --- |
| polygons | `00ffffff,03000000,00000000,00ffffff` | `00ffffff,00000000,03000000,00ffffff` |
| progress_illuminated | `00ffffff,03000000,00ffffff,00808080` | `00ffffff,00ffffff,03000000,00808080` |

These are numeric little-endian DWORD values, with no guessed color/switch semantics. Differing shader/pass mappings can account for order changes; raw constant comparison does not prove final rendering equality or inequality.

Both reflection cubemaps are64×64 DXT1 in both caches. After removing only PC RFC1950 wrapping, their top-level stored blocks differ across six faces: **9,827 bytes for ui_reflection**, **8,885 for ui_reflection2**. The polygon_blue128×128, color_white8×8 and default_additive4×4 top-level payload bytes match. PC compressed bitmaps declare shorter tails ending at4×4; Xbox headers retain smaller declared tail levels and padded resource extent. The comparison does not claim differing compressed blocks necessarily produce different final sampled pixels.

At inspection time, native [Cache::load_bitmap](/home/eric/Developer/Halo-2-Decompiled/src/material.cpp:56) redirects every Vista bitmap through `vista_texture_donor` and the **Xbox** decoder. Native [Vista material branch](/home/eric/Developer/Halo-2-Decompiled/src/material.cpp:214) returns retained PC template/constant/LOD metadata before generating material passes, with the explicit unresolved rendering gap. Therefore merely loading the PC mode cannot deliver actual PC ring texture/material rendering. Native generic model decoding supports its declared FLOAT streams, but original shader input/constant/pass ownership and actual PC texture decoding remain independent integration requirements. The source named-idle metadata reader's Xbox stride is a separate blocker from the byte-identical clip payload.

A strict Vista-artwork implementation needs actual PC geometry, actual PC cubemap resources and source-supported PC material mapping, or an explicitly identified accommodation. The shared model path/name and shared animation cannot be used as evidence that the existing Xbox model/material is Vista artwork. A static ring bitmap, guessed spin angle, guessed geometry scale or copied Xbox constant order would not close these boundaries.

## Actual Vista ring shader and texture contract

Both actual templates expose only layer 15 (`0x8000` in their one LOD record), resolve to pass datum `e1a70031`, `shaders\shader_passes\transparent\default_transparent_one_sided`, and use its one implementation. The root pass is at cache file 55921240; its postprocess record is at 55921276. The template implementation range words use low nine bits for first index and upper seven bits for count, as consumed by the original methods; treating them as a ten/six split from a third-party header would misread this cache. The complete raw records and locators are retained in [vista-ring-pass.json](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-ring-pass.json).

The original PC model widget calls `591CA8` at `640B84`; that scene driver calls `591566` with first argument 2 at `591F92`. In `591566`, temporary variant 2 is used for earlier layers 3/5 and reset to EDI (which was initialized to zero) at `5916FC`. The later depth variant 1 also draws layers 3/5 and resets to zero at `59199D`. The actual transparent layer 15 is then selected at `591A77` through `5A155C(15)`, followed by `5A176F` and `5A17D9`. There is no later direct write to `E49494` within that driver before this dispatch. Thus the driver's source-supported transparent selection is **variant 0**, the 792-byte `ps_2_0`; variant 1 is a separate 652-byte depth program and variant 2 a 948-byte `ps_3_0` MRT program. This is static owner/control-flow evidence, not a complete original D3D scene execution or proof that every intervening service preserves every global. Primary blocks: [scene driver](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-00591ca8.asm), [complete layer driver](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-00591566.asm), [actual compiled selector 66862C](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-0066862c.asm). Three unchanged-byte controls independently supply variants 0/1/2 and verify each exact code pointer, stopping before D3D creation.

The actual vertex program is datum `e19d0027`, `rasterizer\vertex_shaders_dx9\transparent_generic_reflection`; class 2 uses its 2,308-byte record 2. Both this `vs_2_0` and the selected pixel program were disassembled with the real Microsoft `D3DXDisassembleShader` from the installed 32-bit `d3dx9_31.dll` under Wine. No D3D device was fabricated. All ten actual shader records were disassembled successfully; the probe, DLL, runner, shader input and output hashes are retained in [shader-disassembly-provenance.json](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/shader-disassembly-provenance.json). The authoritative instruction texts are [class-2 VS](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/ring-shaders/vertex-e19d0027-record2.asm) and [selected PS](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/ring-shaders/pass-e1a70031-impl0-pixel0-variant0.asm). Disassembly is not GPU execution or framebuffer proof.

### Decompression and static constants

The VS explicitly computes position `v0*c170+c171` before the bone rows at `c50..52`, and computes UV from `v2*c172.xy+c172.zw` before the material UV transforms at `c20..25`. Therefore the PC FLOAT streams are normalized shader inputs, not final model positions or UVs. Original `66F551` initializes `(c170,c171,c172)` to `((1,1,1,1),(0,0,0,0),(1,1,0,0))`. With flag 1 it replaces XYZ with separately rounded `(max-min)*0.5` and `(min+max)*0.5`; with flag 2 it applies the same half-range/midpoint mapping to the U/V bounds. It submits three registers beginning at 170. No guessed model scale is needed. Primary: [66F551](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-0066f551.asm).

The actual ring bounds record begins at file **55919396**, referenced by mode `+0x14/+0x18`. Actual section file **55919484** has geometry class **2** and compression flags WORD `section+0x1A = 3`. Original `6809C4` resolves the actual mode/section, passes the class to the real `77D660`, conditionally ORs flag 1 on a true result, then invokes `66F551`. The unchanged-byte control captures **argument 2, returned AL=0, passed flags 3, and the actual first bounds pointer**. The conditional OR is skipped for this ring; section flags alone are sufficient only for this established class-2 case. This does not generalize the decoder policy to other geometry classes. Primary: [6809C4 caller](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-006809c4.asm), [77D660 class gate](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-0077d660.asm), [12-control fixture](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-ring-material-fixture.json).

Original static producers `6666E3` and `668919` map the actual template constants. Original `667061` additionally reorders staged ARGB floats into final D3D RGBA. The two unchanged material controls run their real callees and retain both storage forms; the table below is the **submitted RGBA form**, not the earlier staging order. `w3c40c0c2` denotes the exact binary32 word, avoiding a decimal replacement.

| Material | VS c202 | VS c203 | PS c1 | PS c2 | PS c3 |
| --- | --- | --- | --- | --- | --- |
| polygons | `(.16,.16,.16,1)` | `(.33,.33,.33,1)` | `(1,1,1,0)` | `(0,0,0,w3c40c0c2)` | `(1,1,1,0)` |
| progress_illuminated | `(1,1,1,1)` | `(1,1,1,1)` | `(1,1,1,0)` | `(1,1,1,w3c40c0c2)` | `(w3f008081,w3f008081,w3f008081,0)` |

Exact `.16` and `.33` source words are `3e23d70a` and `3ea8f5c3`; all 16-component source words are in the fixture. The selected VS interpolates its output color from c202/203 using a transformed-normal term, produces a reflection direction from transformed normal and eye inputs, and generates fog-related oD1. The selected PS samples cube s0 plus projected 2D s1/s2/s3, applies its explicit saturation/color/alpha operations, and writes oC0. A Vulkan equivalent must preserve the actual instruction/constant/sampler contract; replacing it with a generic diffuse shader would not establish Vista material parity.

### Pass states, sampler remaps and actual resources

The seven packed five-byte pass states decode as follows. These names are checked against the primary [Microsoft D3D9 render-state enum](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3drenderstatetype), [blend factors](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dblend) and [blend operations](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dblendop); their values come from the actual PC pass record.

| State | Value |
| --- | --- |
| 168 COLORWRITEENABLE | 7, RGB |
| 27 ALPHABLENDENABLE | 1 |
| 19 SRCBLEND | 2, ONE |
| 20 DESTBLEND | 6, INVSRCALPHA |
| 171 BLENDOP | 1, ADD |
| 14 ZWRITEENABLE | 0 |
| 175 SLOPESCALEDEPTHBIAS | float word 0 |

State 171 is **not** separate-alpha enable; that enum is 206. These seven states do not establish the inherited cull, Z-enable, depth comparison, alpha-test or separate-alpha context of the complete UI frame. Primary consumer: [666D7A](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-00666d7a.asm).

All four actual 24-byte sampler records are `202201010101000000000000000000000000000000000000`. Original `666E09` maps bytes 3/4/5 to address U/V/W state 1; byte 1 low nibble, byte 0 high nibble and byte 1 high nibble to MAG/MIN/MIP filter 2; DWORD at `+0x0B` to LOD bias 0; byte 2 high nibble to max-mip 0; byte 2 low nibble to anisotropy 1; and DWORD at `+7` to border color 0. Thus the stored policy is wrap U/V/W with linear MAG/MIN/MIP and the listed zero/default scalar values. The unused bytes are retained, not assigned invented semantics. Primary: [666E09](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-00666e09.asm), [Microsoft sampler-state enum](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dsamplerstatetype), [filter enum](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dtexturefiltertype).

The unchanged `66661B→668FBE` staging controls prove these texture parameter/stage bindings; raw shader bitmap order alone would give a different answer:

| Material | s0 | s1 | s2 | s3 |
| --- | --- | --- | --- | --- |
| polygons | ui_reflection, cube | polygon_blue, 2D | default_additive, 2D | polygon_blue, 2D |
| progress_illuminated | ui_reflection, cube | **ui_reflection2, cube** | polygon_blue, 2D | polygon_blue, 2D |

The selected PS declares **s1 as 2D**, while the actual illuminated bitmap header for staged `ui_reflection2` is type 2, cube. A bounded binding-owner trace confirms that the ordinary original path does not convert it to a face: `65F600` resolves the bitmap frame with `77D482`, obtains its resource through `665C37→665986`, and passes that resource to `66EBC7`, which calls device vtable `+0x104` ([SetTexture](https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3ddevice9-settexture)). The upload path `665986→675010→67B8F0` selects `660930` for type WORD `+0x0A = 2`; `660930` uses device vtable `+0x64` ([CreateCubeTexture](https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3ddevice9-createcubetexture)), including its fallback. No cube-face extraction or 2D reinterpretation appears in this recovered path. This is static source evidence, not an executed D3D upload, shader-validation result or rendered original illuminated surface. The original-cache/runtime compatibility remains unresolved: do not silently choose a cube face, relabel the source resource or change the PS declaration and then claim original parity. Both actual resource bytes and the exact staged datum `e1ac0036` are preserved. Primary: [65F600](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-0065f600.asm), [665986](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-00665986.asm), [67B8F0](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-0067b8f0.asm), [660930](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-00660930.asm), [66EBC7](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-0066ebc7.asm), staging fixture, selected PS and bitmap records in the manifest.

The source bitmap export is lossless and donor-free: strict RFC1950 inflation, explicit PC formats 14 DXT1 / 10 XRGB, tight stored face-major mip spans. All 15 stored faces across the five resources are exported as individual DDS files, with **71 `*-mip*.bin` blocks** including every stored tail level. Faces are numbered in stored order without guessed cube directions. The two 64×64 cubemaps have five stored levels ending at 4×4; polygon_blue has six (128×128 through 4×4), color_white has two (8×8 and 4×4), default_additive has three (4×4 through 1×1). Exact cache offsets, compressed/expanded hashes, per-face stride and every mip hash are retained in the manifest. This supplies reproducible input data for the parent's actual-PC decoder tests; it does not prove original GPU upload layout or final sampled pixels.

### Conditional fog owner remains a live-state boundary

The actual VS reads c12–15. Original `661F3D` gates the fog producer on frame byte `+0x26C`; only when enabled does it copy the 288-byte record at `frame+0x14C` and call `677EAC` with the current camera and forward vector. `677EAC` produces six registers c12–17 and reaches the real constant-cache/device boundary at `67814C`. Its denominator floor is original binary32 word `38d1b717` at `7BA8C8`. The producer consumes distance/plane/density fields, rather than a guessed single fog color. Primary: [661F3D gate](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-00661f3d.asm), [677EAC producer](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/vista-00677eac.asm).

The unchanged `591CA8` prefix control proves that its 288-byte fog region is cleared before the first scene services. It also proves that the enable byte is **not assigned by that prefix**: it retains the supplied diagnostic stack pattern A5. The separate zero-record producer control retains all resulting c12–17 words, stopping before device submission. It establishes only what that explicitly supplied conditional invocation would produce. It does **not** establish whether the live UI enables it, later changes the record/enable byte, or inherits an earlier constant state. No additional original UI render-frame lifetime evidence is claimed. The explicit controlled FOV 1.693 used for this partial initializer test is also not claimed as a recovered FOV conversion.

Supported integration can therefore use actual PC streams/resources, the proved class-2 flags, exact static RGBA constants, source pass/sampler records, selected variant 0 and the conditional fog producer contract. Complete Vista D3D/Vulkan material equivalence still requires the unresolved resource binding, live frame/constant ownership and framebuffer validation.

## Provenance and validation limits

All seven artifact runners were rerun successfully after the final material follow-up. The roster control fixture retains 11 controls / 117 distinct original instructions. Ring controls retain 6 scale, 48 viewport, 768 clock and 1 named-idle producer controls: 94 distinct original Vista and 365 distinct original Xbox instructions, with 2,118 Vista and 224,621 Xbox dynamic instructions. The material fixture retains 12 controls / 1,391 distinct original instructions; these controls enter explicitly stated original prefixes or pure methods and never replace instructions/callees. Ten shader disassemblies and all 71 source mip exports have separately pinned provenance. Passing these bounded controls does not assert full original service, D3D or native framebuffer execution.

| Current artifact | SHA-256 |
| --- | --- |
| vista-roster-fixture.json | `c7e2937a48aaf6da13e9af7eae2df8fe07b2863dda239c44acb9b3abfafa9d1c` |
| executed-original-instructions.txt | `901a377bf54051212cdf8b3c2d91001e20085fcd8a8baa2ff84238bada21e7d3` |
| ring-assets.json | `dae9f1f773d21e30fbd7391127c8aaf06c6ef8e652b2b455e0474a88e0da9287` |
| ring-controls-fixture.json | `c493aca833f4a37ea5ee9d680fe5370569e960dcec35c07f8145d90283a9bf1a` |
| ring-geometry-material-comparison.json | `38cf5ca2eedf4b2760c63397d217f531ea2ecda1b213ed82cfbad9a8964de8d9` |
| vista-ring-pass.json | `ca7d38c6c182baab4ad11e690b120501cdf017e2c7a6b5a6b4fb54bb2cf6f6fc` |
| vista-ring-material-fixture.json | `1ba6a463e530651299198ebf1b4f48acce967e1450234aedc2e3c093572f4aeb` |
| material-executed-original-instructions.txt | `627eab5e4ce8ba4280a5361bec1b485bdf9fd0eb26a708edede894b166e3cf28` |
| shader-disassembly-provenance.json | `88408d7c5343970e949847189a127f685f2947d345665b0fcf1e100642ab4abc` |

The final [frozen-evidence.json](/home/eric/Developer/Halo-2-Decompiled/.local/vista-matchmaking-roster-research-20261012/frozen-evidence.json) additionally pins this note, the runners, original disassembly text, shader binaries/disassemblies and every exported face/mip. It excludes itself to avoid a self-hash cycle.

Unresolved: original Vista full Windows/provider/runtime list lifetime; full original logical-font/raster output; ring wall-clock scheduling and render interpolation; the original illuminated cube/2D binding; live UI fog enable/constant lifetime; complete PC material/texture execution; original D3D versus native framebuffer equality; live authenticated rendered progress during actual matchmaking. No Discord/account IDs, credentials, hidden ratings or session secrets are required or stored in this corpus. Chosen Halo names may populate a declared accepted-participant roster without exposing private identity/rating fields.
