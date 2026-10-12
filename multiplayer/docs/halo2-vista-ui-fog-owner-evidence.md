# Vista UI scene fog ownership and material binding order

Research recorded 2026-10-12. This is an evidence-only follow-up to the finalized [Vista roster/ring corpus](/home/eric/Developer/Halo-2-Decompiled/multiplayer/docs/halo2-vista-matchmaking-roster-evidence.md). It changes no product source, builds, launchers, deployments, existing research or user identity data. Its separate artifacts are under [.local/vista-ui-fog-owner-20261012](/home/eric/Developer/Halo-2-Decompiled/.local/vista-ui-fog-owner-20261012).

The extended original-byte controls do **not** establish a deterministic live UI fog-enable value. On the admitted UI initializer path, the 288-byte fog record is cleared but the following enable byte retains the supplied initial stack pattern through real scene services and the full renderer-frame copy. A false gate preserves prior c12–17 values; an explicitly supplied true gate produces those six registers through the original fog routine. Neither observation authorizes a fabricated false flag or a zero constant bank as original Vista behavior.

The ordinary changed-pass material path selects/binds the pixel shader before binding the actual staged ring textures. Earlier first-material texture clears and subsequent shader-cache skips are separate cases. This is static original owner evidence; it does not settle the original D3D9 2D-sampler/cube-resource compatibility or establish Windows framebuffer parity.

## Primary source, reproduction and control scope

All PC instructions come from the supplied `Halo 2 Vista installer/halo2.exe`, SHA-256 **33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109**, manually PE-section-mapped at preferred base `00400000`. The actual cache/compiled ring shaders remain pinned in the earlier corpus; they are not changed or re-exported here. API contracts are checked against Microsoft's [SetVertexShaderConstantF](https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3ddevice9-setvertexshaderconstantf), [SetPixelShader](https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3ddevice9-setpixelshader) and [SetTexture](https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3ddevice9-settexture) documentation. Those documents describe the API calls, not Halo's live state or a resource-type workaround.

Reproduce from the repository root:

```sh
.tools/python/bin/python .local/vista-ui-fog-owner-20261012/export_source_blocks.py
.tools/python/bin/python .local/vista-ui-fog-owner-20261012/oracle_ui_fog.py
```

[oracle_ui_fog.py](/home/eric/Developer/Halo-2-Decompiled/.local/vista-ui-fog-owner-20261012/oracle_ui_fog.py) executes **52 controls covering 1,072 distinct original instructions**, verifying every reached instruction against the pinned executable bytes. It imports no native implementation and supplies no replacement instruction, service callee, D3D device or mocked vtable. Explicit resident input states admit natural original fast/cache-hit paths. Stops and separately entered prefixes are recorded as such; they are not claimed to be continuous full D3D execution.

| Control | Cases | Boundary |
| --- | ---: | --- |
| UI initializer through real scene services | 4 | Stops before first backbuffer/device operation |
| Frame argument and complete frame-stack copy | 12 | Separate post-backbuffer prefix; stops before frame application |
| Constructed frame's conditional fog gate | 4 | Separate gate entry; stops at return or real fog producer |
| Apply-frame zero-mode service preserves supplied bank | 4 | Already-current render state; original callees execute |
| Exact binary register-cache range | 16 | Full original cache routine; no GPU submission |
| Material reset preserves supplied bank | 4 | No previous pass; existing fallback shader; current bias states |
| Fog gate and original producer/cache lifetime | 8 | Explicit flag 0/1; enabled path stops before GPU upload decision |

[ui-fog-controls-fixture.json](/home/eric/Developer/Halo-2-Decompiled/.local/vista-ui-fog-owner-20261012/ui-fog-controls-fixture.json) contains full constructed frames, frame writes, source/copy hashes, explicit inputs, reached boundaries and final fog words. [executed-original-instructions.txt](/home/eric/Developer/Halo-2-Decompiled/.local/vista-ui-fog-owner-20261012/executed-original-instructions.txt) records the instruction union. [source-blocks.json](/home/eric/Developer/Halo-2-Decompiled/.local/vista-ui-fog-owner-20261012/source-blocks.json) pins 24 bounded original disassembly blocks separately from executable controls.

## UI initializer leaves the conditional byte unresolved

At `591CA8`, the actual stack frame supplied later to the renderer is `0x298` bytes. Its fog record starts at `frame+0x14C`, spans `0x120` bytes, and ends immediately before the enable byte at **frame+0x26C**. The original CRT clear called at `591DEC` zeros that record; it does not include the enable byte. Primary: [591CA8](/home/eric/Developer/Halo-2-Decompiled/.local/vista-ui-fog-owner-20261012/vista-00591ca8.asm).

The extended constructor controls execute through the real services at `591EBD→595E72`, `591EC2→59A099`, `591EC7→54CB17` and `591ED0→6736A4`. Their explicitly declared resident state is an inactive observer, an effect data array with zero valid indices, optional scene datum `FFFFFFFF`, and the ordinary backbuffer path. Source `6788A0` initializes that optional datum to `FFFFFFFF`; this is not a replacement return value. Camera/forward/up and rectangle are explicit test inputs. FOV `1.693` is also an explicit controlled value, **not** a recovered conversion of the cache's FOV97.

All four initial stack fills (`00`, `01`, `A5`, `FF`) produce an entirely zero fog record while preserving the corresponding enable byte. A write hook observes every reached write overlapping the renderer frame and confirms none overlaps `+0x26C`. This extends the earlier stop-before-services result. It proves the admitted path, not every possible observer/effect state or actual live stack contents.

The controls stop at `591EEC`, before the ordinary GetBackBuffer operation. The alternate target path would first use AddRef at `591EE8`; neither device path is supplied or executed. A **separate** control starts at `591F01`, with the measured constructor stack position, and executes `6620CF`. That routine increments stack index `868158`, selects `E3DA40 + index*0x298`, and copies all `0xA6` DWORDs. Starting indices `-1`, `0` and `1` all preserve the entire frame and byte at `+0x26C`. It stops before `661F3D`, without normalizing the byte. Primary: [6620CF](/home/eric/Developer/Halo-2-Decompiled/.local/vista-ui-fog-owner-20261012/vista-006620cf.asm).

## Conditional producer and prior register lifetime

`661F3D` first copies the full frame to `E3DF70`. At **662099** it tests `byte[frame+0x26C]`. Zero returns without copying/applying fog. Nonzero copies 288 bytes from `frame+0x14C` to `E3E0BC`, then calls **677EAC** with that record and the current camera/forward globals. Independent gate controls show zero returns and each nonzero test pattern reaches the real producer. Primary: [661F3D](/home/eric/Developer/Halo-2-Decompiled/.local/vista-ui-fog-owner-20261012/vista-00661f3d.asm), [677EAC](/home/eric/Developer/Halo-2-Decompiled/.local/vista-ui-fog-owner-20261012/vista-00677eac.asm).

The producer calls the exact register cache `44FF5B` with first register **12**, count **6**, bank base **E3C7B0**. Changed data is subsequently submitted through device vtable `+0x178`, with the same first/count arguments. The API's count is in float4 registers, not scalar floats. The new enabled controls execute real `677EAC` and real `44FF5B`, stopping at `678151` before the upload decision; disabled controls execute to return. With four explicit prior binary banks, disabled retains every prior word and enabled replaces only c12–17. The values are not chosen to represent a live bank: DWORD `i` is `(seed + i*0x101) mod 2^32`, for seeds `3F000000`, `80000000`, `7FC00000`, `FFFFFFFF`.

`44FF5B` compares binary words using XOR/OR. On a full match it returns AL0; at a mismatch it copies the remaining requested register range and returns AL1. It does not normalize signed zero or NaN words and does not clear registers outside the requested range. Sixteen full-routine controls cover c0–11 and c12–17, both cache hits and one-bit changes. Primary: [44FF5B](/home/eric/Developer/Halo-2-Decompiled/.local/vista-ui-fog-owner-20261012/vista-0044ff5b.asm).

The adjacent camera/projection producer `661B53` requests first0/count12 and copies 48 DWORDs to `E3C6C0`. Its source therefore supplies **c0–11**, not a neutral c12–17 bank. This is static source ownership: the routine has a real SetViewport call at `661CC1`, so this corpus does not pretend to execute the entire camera producer without a device. The original zero-mode service called earlier at `661FDA→6603D2(0)` executes fully in four already-current-state controls and preserves the supplied bank. Primary: [661B53](/home/eric/Developer/Halo-2-Decompiled/.local/vista-ui-fog-owner-20261012/vista-00661b53.asm), [6603D2](/home/eric/Developer/Halo-2-Decompiled/.local/vista-ui-fog-owner-20261012/vista-006603d2.asm).

The material reset at `662094→669835→66882C` is also not an observed fog-bank reset. Four full original controls admit the no-previous-pass branch, an already-created fallback PS, and already-current zero depth/slope-bias states (`195`, `175`); the supplied 256-register CPU bank remains byte-identical. The previous-pass default-state branch and fallback creation are not exercised, so this is not a whole-program proof that no other owner can ever modify the bank. A separate original resubmission owner `660DA7` uploads all 256 cached VS registers and 32 cached PS registers; its source resubmits the current banks, not zeros. Primary: [669835](/home/eric/Developer/Halo-2-Decompiled/.local/vista-ui-fog-owner-20261012/vista-00669835.asm), [66882C](/home/eric/Developer/Halo-2-Decompiled/.local/vista-ui-fog-owner-20261012/vista-0066882c.asm), [660DA7](/home/eric/Developer/Halo-2-Decompiled/.local/vista-ui-fog-owner-20261012/vista-00660da7.asm).

Popping a renderer frame at `66268D` decrements the frame-stack index and, when another frame remains, calls `661F3D` on that previous frame. It then restores the prior fog/camera records. That restoration again passes through the previous frame's conditional gate; a restored fog struct alone does not prove an upload or zero bank. This pop/apply path is static ownership evidence, not executed full device restoration. Primary: [66268D](/home/eric/Developer/Halo-2-Decompiled/.local/vista-ui-fog-owner-20261012/vista-0066268d.asm).

## Layer15 pixel shader precedes actual texture binding

The earlier pinned driver identifies the ring's ordinary transparent layer15 and compiled variant0. The complete material driver **669AAD** supplies the following order on the changed-pass or relevant shader-dirty path:

| Call site | Original owner | Effect |
| --- | --- | --- |
| `669DC9` | `66862C` | Select/create compiled pixel shader and store selected pointer |
| `669DD0` | `66632E` | Device vtable `+0x1AC` binds selected or explicit fallback pixel shader |
| `669DFC` | `668FBE` | Stage actual static bitmap slots for this pass |
| `669E63` | `6679D9` | Visit dirty stages0–7; actual bitmap bind at `667AF2→65F600` |
| `65F6A9` | `66EBC7` | Device vtable `+0x104` binds resolved resource when it differs from cached binding |

Primary: [669AAD](/home/eric/Developer/Halo-2-Decompiled/.local/vista-ui-fog-owner-20261012/vista-00669aad.asm), [66632E](/home/eric/Developer/Halo-2-Decompiled/.local/vista-ui-fog-owner-20261012/vista-0066632e.asm), [668FBE](/home/eric/Developer/Halo-2-Decompiled/.local/vista-ui-fog-owner-20261012/vista-00668fbe.asm), [6679D9](/home/eric/Developer/Halo-2-Decompiled/.local/vista-ui-fog-owner-20261012/vista-006679d9.asm), [65F600](/home/eric/Developer/Halo-2-Decompiled/.local/vista-ui-fog-owner-20261012/vista-0065f600.asm), [66EBC7](/home/eric/Developer/Halo-2-Decompiled/.local/vista-ui-fog-owner-20261012/vista-0066ebc7.asm).

The branch at `669DB1..669DBE` can skip the selector/SetPixelShader when the shader state is already current. `66EBC7` can likewise skip a redundant resource binding. An earlier first-material reset in `669E9A`, controlled by `8684AA`, calls `66EBC7(stage,NULL)` at `669F2D` for stages0–7 **before** normal pass selection; these are clears, not actual ring resource bindings. Primary: [669E9A](/home/eric/Developer/Halo-2-Decompiled/.local/vista-ui-fog-owner-20261012/vista-00669e9a.asm).

This order is recovered from unchanged original call sites, not a fully executed draw or evidence that either D3D9/Wine binding order handles the cube/2D mismatch correctly. Diagnostic probes may test both orders, but their driver/runtime results must remain separate from original source evidence.

The separately recorded [D3D9 diagnostic probe](/home/eric/Developer/Halo-2-Decompiled/multiplayer/docs/halo2-vista-ring-d3d9-probe.md) documents clean binding-order controls under DXVK2.5.3 and wined3d. It supplies compatibility evidence for those Wine implementations and explicitly leaves original Windows interpretation unresolved; it does not supply the missing live UI flag or fog bank.

## Supported integration boundary and remaining evidence

The native bridge can preserve the actual conditional producer, exact c0–11 versus c12–17 ranges, binary register-cache semantics, nested frame contract and recovered ordinary material binding order. The actual zero fog record's conditional constants remain the earlier fixture's supported producer output. Those constants are available **if** the producer is legitimately invoked; the UI initializer does not establish that invocation.

The missing proof is a live original UI frame's `+0x26C` value and inherited CPU/GPU c12–17 state, including any earlier owners/device-reset transitions. This corpus does not recover those from an actual original Windows process or framebuffer. Clearing the whole native frame, forcing either flag value, treating disabled as neutral fog, or inventing a zero prior bank would each be a declared accommodation requiring its own policy and rendered validation, not recovered Vista parity. The existing cube-resource/2D-declaration boundary also remains unresolved here.

## Pinned validation

The final fixture SHA-256 is **e0b3d5bfd7cdb3cddc49eb94d2f59a10ad670a033c8fe9f43d293e3191e59645**. The source blocks, runner, fixture and instruction-trace hashes are frozen separately in `frozen-evidence.json` in this new corpus. The earlier finalized ring corpus and its manifest remain untouched.
