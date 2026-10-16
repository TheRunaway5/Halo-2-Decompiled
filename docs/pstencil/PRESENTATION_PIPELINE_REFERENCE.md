# 2003 presentation pipeline: primary reference

The recovered implementable stage is the **resident six-combiner environment response selected by the default layered submission path**. The older five-combiner callback is a separate historical fallback. The current independent environment adapter preserves current authored inputs; this does not establish complete 2003 accumulation, asset-parameter or framebuffer parity.

Primary inputs: supplied `halo2debug.xbe`, SHA-256 `5a9edb4c874a782594c86020db49b0086f44ae38f53ff2276b3192a2c2383d18`; matching Windows `tool.exe` schemas; and `game/maps/ec_interior_test.map`, build **02.05.03.1968**. All **62 game / 13 tool function spans** were compared byte-for-byte with their primary images. [Export verification](../../analysis/pstencil/2003-presentation/build.json), [cache/schema provenance](../../analysis/pstencil/2003-presentation/resident-environment.json).

## Actual default submission path

Switch **0074455C**, named `render_old_rasterizer`, occupies zero-initialized storage. Zero selects the layered renderer; nonzero selects the old rasterizer. Runtime console mutation remains unobserved. For environment layer **C**, `render_layer_draw` **000E8610** tests mask **017FDE3E** and dispatches primitive list **000A62D0**. Its callbacks **001CD8A0/rasterize_model_part** and **001D2D00/rasterize_environment_strip** build setup through **001D5210**, then submit through **00097C50**. This route bypasses the old generic drawing bridge. [Switch evidence](../../analysis/pstencil/caster-parity/2003/legacy-switch-references.json), [layer dispatch](../../analysis/pstencil/2003/disassembly/000e8610.asm), [primitive dispatch](../../analysis/pstencil/2003-presentation/game/functions/000a62d0.c), [model](../../analysis/pstencil/2003-presentation/game/disassembly/001cd8a0.asm), [BSP](../../analysis/pstencil/2003-presentation/game/disassembly/001d2d00.asm).

The resident cache records concrete selection: Chief armor, visor and first-person arms reference `default_environment_map` in lighting model **0/legacy**, and `default_environment_map_tangent_space` in **1/bump lightmap, separate envmap**. BSP materials including `matte_grey`, `int_floor` and `int_fibgls_panel` select the default pass. This is code plus compiled selection data; actual visibility and an unmodified original frame remain separate evidence. [Decoded layer/pass records](../../analysis/pstencil/2003-presentation/resident-environment.json).

The cache ABI is primary-proved: **00037CA0** addresses `index*20h`, checks full datum **+0C**, and bounds against header **+0C**; **00038120** checks groups **+0/+4/+8** and returns payload **+14**. The extractor validates all **365** datum ordinals, names and nonnull payload bounds. [Instance getter](../../analysis/pstencil/2003-presentation/game/functions/00037ca0.c), [tag getter](../../analysis/pstencil/2003-presentation/game/functions/00038120.c), [read-only extractor](../../analysis/pstencil/2003-presentation/extract_resident_environment.py).

## Exact resident program and fields

Both resident environment passes contain the identical **240-byte program**, SHA-256 **40c792b9a3b656e2c1e83bc5ca27fe3e319c7404dc95b5d1dd4cdd5732cd4cac**. Default datum **E1890015** has payload file offset **B7477C**, implementation **B74C98**; tangent-space datum **E192001E** has payload **B7787C**, implementation **B77DD4**. [Default binary](../../analysis/pstencil/2003-presentation/default_environment_map-resident-0.bin), [tangent-space binary](../../analysis/pstencil/2003-presentation/default_environment_map_tangent_space-resident-0.bin), [60 words and offset-bearing records](../../analysis/pstencil/2003-presentation/resident-environment.json).

**001D5210** reads `spas+30` as **13C-byte** implementation records. The matching primary tool schema and actual consumers agree:

| Offset | Ownership |
| --- | --- |
| `+10` | hack pixel shader index **13**, custom compiled program |
| `+14..+103` | 60-DWORD program |
| `+104` | VS V[n] register/data-index pairs **10→0, 11→1** |
| `+110` | VS C[n] remaps; empty here |
| `+11C` | default constant data: two RGBA vectors of ones |
| `+128..+12B` | bitmap types |
| `+12C` | texture count **4** |
| `+130` | compiled render/texture states |

Per-material overrides supply values, textures and animated/global overlays. **00097C50** invokes **000964B0** with custom index 13 and the program pointer, applies states/textures, and writes constant attributes through **0033D480**. Index 13 uses the supplied words. [Setup](../../analysis/pstencil/2003-presentation/game/functions/001d5210.c), [submission](../../analysis/pstencil/2003-presentation/game/functions/00097c50.c), [custom-index consumer](../../analysis/pstencil/2003-presentation/game/functions/000964b0.c).

| Input | Proven role |
| --- | --- |
| `V10 → oD0 → combiner V0` | normal tint RGB / brightness alpha |
| `V11 → oD1 → combiner V1` | glancing tint RGB / brightness alpha |
| stage 0 / `T0` | default: bump accumulation override **2**; tangent-space: authored local bump bitmap |
| stages 1/2 | dot-product intermediates |
| stage 3 / `T3` | reflected authored environment cube |
| combiner 0 C0 RGB | encoded **positive engine world camera forward** |
| combiner 3 C0 alpha | midrange tint factor |
| combiner 4 C0 RGB | midrange tint color |

Words **53=00011106**, **54=00064621**, **55=00000111** mean six combiners, texture modes **[1,17,17,12]**, signed D3D dot mapping. The first RGB instruction **68410B0B** reads **EXPAND_NEGATE(T0)**. The tangent-space variant literally uses its local bump T0 for this calculation; substituting normalized T2 changes the recovered response. [Resident bindings](../../analysis/pstencil/2003-presentation/resident-environment.json), [texture interpreter](../../shaders/texture_program.glsl).

Loose source names/defaults corroborate roles: normal/glancing RGB **(1,1,1)**, brightness **1**, midrange factor **0**, midrange color **(0,0,0)**. These are pass defaults, not replacement presets. Resident original Chief armor instead supplies V10 **(.85,.85,.69,.2)**, V11 **(.43,.57,.62,.05)**, factor byte **1A**, midrange RGB **006E919E**. Current Chief's brightness and midrange values differ and remain current authored values in the adaptation. [Primary-schema loose decode](../../analysis/pstencil/2003-presentation/passes.json), [exact source](../../analysis/pstencil/2003-presentation/environment.psc), [resident overrides](../../analysis/pstencil/2003-presentation/resident-environment.json), [current compiled material](../../analysis/pstencil/2003-presentation/current-chief-environment.json).

## Arithmetic and camera sign

For ordinary `[0,1]` colors/brightness, the compiled program reduces to this expression. `expand(x)=2*x-1`; exact words retain byte quantization, unsigned reads and intermediate saturation.

```text
d = clamp(dot(-expand(T0.rgb), expand(C0[0].rgb)), 0, 1)
f = 1 - (1-d)^4
m = 2 * min(f, 1-f) * C0[3].a
tint = f*V0.rgb + (1-f)*V1.rgb
tint = (1-m)*tint + m*C0[4].rgb
brightness = f*V0.a + (1-f)*V1.a
rgb = brightness * (tint*T3.rgb + (1-tint)*T3.rgb^8)
alpha = 0
```

The first dot is signed; subsequent inverse/unsigned operations constrain its facing contribution. The midrange mux tests previous R0 alpha's MSB (`word53 & 100h`). Literal words govern out-of-range inputs and exact saturation. [Compiled program](../../analysis/pstencil/2003-presentation/resident-environment.json), [combiner decoder](../../shaders/combiner.glsl).

**0023A300**, vector extern **1**, returns **007436EC/F0/F4**, encoded `(forward+1)*.5` for pixel constants. **001D5000/001D45A0** consume the overlay; **001D5210** clamps/packs it. **00050DF0** copies the render camera to window **+18**, **000359E0** copies the window to **007436C8**, and **000EC020** negates camera **+0C** for view-camera Z. Thus resident C0 is positive engine forward; the first instruction negates the sampled normal. [Vector producer](../../analysis/pstencil/2003-presentation/game/functions/0023a300.c), [overlay](../../analysis/pstencil/2003-presentation/game/functions/001d45a0.c), [camera copy](../../analysis/pstencil/2003-presentation/game/functions/00050df0.c), [window copy](../../analysis/pstencil/2003-presentation/game/functions/000359e0.c), [view construction](../../analysis/pstencil/2003-presentation/game/functions/000ec020.c).

## Vertex contract and bounded current adapter

**000981C0** selects environment programs **48..4E** for its nonzero lighting-model branch and **4F..51** for zero, with vertex-class variants. Exported programs retain descriptor/code addresses and hashes. They move **V10 directly to oD0** and **V11 directly to oD1**. World program **4E** emits V3 UV to oT0, V6/V5/V4 to oT1/2/3.xyz, and `C4.xyz−V0.xyz` to their w components; **51** emits projected screen coordinates and identity basis components. Complete source declaration/palette producers remain broader vertex-pipeline work. [Selector](../../analysis/pstencil/2003-presentation/game/disassembly/000981c0.asm), [program export](../../analysis/pstencil/2003-presentation/original-environment-vertex.json), [4E instructions](../../analysis/pstencil/2003-presentation/original-vs-4e.txt), [51 instructions](../../analysis/pstencil/2003-presentation/original-vs-51.txt), [reproducer](../../analysis/pstencil/2003-presentation/extract_environment_vertex.py).

Current independent `default_environment_map_tangent_space` supplies local bump T0, reflected environment T3, V10/V11 RGBA and positive-view C0. Its four-combiner consumer uses stage **2 C0 alpha** in the same midrange-factor role and stage **3 C0 RGB** in the same midrange-color role. This is a compiled-consumer crosswalk; stripped current parameter-name tables are not assumed to preserve names. [Current words/remaps/vertex program](../../analysis/pstencil/2003-presentation/current-chief-environment.json), [current binder](../../src/world_material.cpp).

The adapter now uses the exact resident six-combiner control words, including **64621**, preserves completed current V10/V11, maps, vertex program and destination alpha, and maps current **word10 unchanged**, `word12 & FF000000` → original **word13**, `word13 & 00FFFFFF` → original **word14**. Current texture interpretation forms the same T3 reflection products with stage 2 DOTPRODUCT; the discarded T2 normalization-cube sample is unnecessary. Admission is restricted to the recognized independent tangent-space pass/control-word family. This preserves current authored vehicle/weapon/prop values without imposing 2003 Chief presets. [Adapter/admission predicate](../../src/pstencil/environment.cpp), [interface](../../include/halo2/pstencil/environment.hpp), [texture stages](../../shaders/texture_program.glsl).

**Covered:** recovered environment response within that current interface. **Still open:** combined lightmap/dynamic/environment families (including combined visor passes), original asset values, full original vertex/NV2A precision, intermediate targets and complete scheduling. Renderer integration and live capture validation are tracked by the main task separately.

## Target, blend and alpha ownership

The loose pass declares target **3/light accumulation**, blend **3/add source times destination alpha**. Resident states independently contain RGBA writes, blend enable, source **304/DST_ALPHA**, destination **1/ONE**, operation **8006/add**, depth equal **202**, depth-write zero and alpha-test zero. **00097C50** normalizes depth-enable state 8F values 1/2 to 2. Zero shader alpha preserves destination alpha while that existing alpha modulates added RGB. Its preceding producer belongs to the broader accumulation pipeline. [Loose declaration](../../analysis/pstencil/2003-presentation/passes.json), [resident states](../../analysis/pstencil/2003-presentation/resident-environment.json), [state submit](../../analysis/pstencil/2003-presentation/game/functions/00097c50.c).

**00033000** allocates 640×480 secondary targets via **00031090**, rasterizer format **0B**, linear flag **1**. **0009FEA0** resolves table cell **003F4870** to literal hardware format **12**, Xbox linear A8R8G8B8. Target **9** owns bump accumulation, **10** utility response, **11** texture accumulation; they share primary depth. Faithful reconstruction must retain intermediate encoding/quantization and alpha ownership. [Allocation](../../analysis/pstencil/2003-presentation/game/functions/00033000.c), [format conversion](../../analysis/pstencil/2003-presentation/game/functions/0009fea0.c), [literal values](../../analysis/pstencil/2003-presentation/build.json).

Next larger candidates: bump **000E8040** plus hacked pixel index 1 in **000964B0**; texture **000E80B0 / 001D0A90 / 001D0AF0**; utility/dependent response **000E88D0 / 001D0F90 / 001D3090**. Their compiled selection, channel encodings and current input mappings remain incomplete. [Bump](../../analysis/pstencil/2003-presentation/game/functions/000e8040.c), [texture](../../analysis/pstencil/2003-presentation/game/functions/001d0af0.c), [dependent setup](../../analysis/pstencil/2003-presentation/game/functions/001d0f90.c), [dependent light](../../analysis/pstencil/2003-presentation/game/functions/001d3090.c).

## Historical fallback and validation limits

**0023A1B0** is a false-returning stub belonging to the old callback bridge; it does not disable the default primitive pipeline. Model callback **001CF010** constructs a five-combiner response, but sole recovered direct caller **000E9B40** admits only layers D/6 into its callback switch and skips C. Old BSP callback **001D1680** constructs the same response with white RGB. Neither establishes default environment presentation. [Stub](../../analysis/pstencil/2003-presentation/game/disassembly/0023a1b0.asm), [caller gate](../../analysis/pstencil/2003-shadow-lighting/disassembly/000e9b40.asm), [references](../../analysis/pstencil/2003-presentation/references.tsv), [old BSP](../../analysis/pstencil/2003-presentation/game/functions/001d1680.c).

The fallback remains reference-only: unchanged **001CF17A..001CF309**, memset and RGB packer execute with FPCW **023F**, stopping before GPU submission. Across **4,105 fixtures**, only word 10 varies. It packs negative forward and uses a squared, clamped T2 dot, unlike the resident response. No instructions/services are replaced. [Oracle JSON](../../analysis/pstencil/2003-presentation/fallback-environment.json), [program](../../analysis/pstencil/2003-presentation/fallback-environment.bin), [reproducer](../../analysis/pstencil/2003-presentation/extract_fallback_environment.py).

Seven loose passes decode to complete file boundaries using primary tool schemas. An attempted original Windows source compilation reached an unresolved static-CRT Windows-import boundary; resident extraction supplies the actual compiled program directly. Original Xbox framebuffer/texture precision, complete MRT alpha production and per-frame original scenario admission remain unverified. [Loose decoder](../../analysis/pstencil/2003-presentation/decode_passes.py), [resident extraction](../../analysis/pstencil/2003-presentation/extract_resident_environment.py).
