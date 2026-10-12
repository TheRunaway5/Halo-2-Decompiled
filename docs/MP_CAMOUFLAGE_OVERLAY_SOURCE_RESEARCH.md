# Original camouflage collector, override assets and FP owner

Research snapshot: 2026-10-11. The original biped visual path selects an actual
global shader, creates a camouflage collector clone and suppresses ordinary
collection. The authored shader contains separate layer-18/layer-19 passes.
Weapon scalar 37 uses the actual full weapon identity in both world and FP
material contexts, resolving its first biped parent. This is bounded original
source/asset evidence; resident geometry, final refraction/stencil/framebuffer
and native visual parity are not established. No native code, mode admission
or all-39-variant goal status changes in this research.

## Sources and reproduction

The [original retail executable](../assets/disc/default.xbe) is pinned to
SHA-256 `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The [original Lockout cache](../assets/disc/maps/lockout.map) is pinned to
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
The asset census records the hash of each of the 21 owned Xbox multiplayer
caches. No MCC/Vista assets or assumed update-address deltas supply these
render contracts. TU/u5 correspondence for the complete selector/collector
owners remains unestablished here.
[Asset report](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/assets.json)

Three independent tools preserve the evidence:

| Tool | Supported result | Output SHA-256 |
| --- | --- | --- |
| [oracle_camouflage_overlay.py](../tools/oracle_camouflage_overlay.py) | 224 whole selector calls, declared composition into whole collectors with empty drawable sections | `9141bdf38c30a14fb60253e14e02836e17cc317d69582021d01ad1b89f6a7e17` |
| [audit_camouflage_override_assets.py](../tools/audit_camouflage_override_assets.py) | Literal matg/shad/stem/spas/vrtx/bitm inventory across 21 caches | `811e9258da1fe7d71119afa238ffd2badb1cc68f740d78c4e070ee1475c848c8` |
| [inspect_camouflage_overlay_contract.py](../tools/inspect_camouflage_overlay_contract.py) | 18 complete static owners and 12 whole FP identity/context helper compositions | `5c8494c541fcb2758118878f1649d203aebfa44e78bf0ff659e48bad09aca962` |

Each report and the complete static listings reproduce byte for byte in a
fresh generation. Executed instruction bytes are checked against the pinned
image. No instruction patches or supplied game returns are used. The tools
declare their backed records and their composition boundaries explicitly.
[Reproduction record](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/reproduction.json)

```sh
.tools/python/bin/python tools/oracle_camouflage_overlay.py
.tools/python/bin/python tools/audit_camouflage_override_assets.py
.tools/python/bin/python tools/inspect_camouflage_overlay_contract.py
```

## Actual selector and clone behavior

Retail `4BCC0` uses `EDI = geometry context`, stack object/distance/caller-mode/
FP-output/force-world/LOD-output and `RET 18h`. The full selector executes
unchanged together with `B9CE0`, `3D3E0`, `4C450` when applicable and `4C0D0`.
The fixture uses real Lockout Spartan `E9F2086D` and Elite `EA1B0896`
definitions/model records; current model-variant byte `+B1 = FF`, distance 10,
LOD cap 5, previous-frame history `4BA050 = -1`, modifier `4BA019 = 0`, and FP
count 1 are declared inputs. All selected models return through the actual
range finalizer, rather than stopping before `4C0D0`. That finalizer zeros the
range bank for this explicit variant-byte case; other variant/range histories
are outside this corpus.
[Whole execution report](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/whole-owners.json),
[complete selector](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/contract/geometry_selector-0004bcc0.asm)

In the ordinary definition branch, `4BED0..4BF23` requires current object type
0 or 1 and primary `object+2B0 > 0`. Its `COMISS/JBE` also skips unordered
inputs statically; NaN/infinity are not executed in this corpus. On admission
it ORs geometry flags `+16C` with 2, loads shader datum `[485A80]+D0` to `+174`,
and copies primary to `+184`. It does not test activation bit 3 or perform
scalar37's parent walk. The later caller-mode gate controls shield selection,
so nonzero caller mode does not itself disable this camouflage branch.
[Original instructions](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/contract/geometry_selector-0004bcc0.asm)

The independent `hlmt+9C` bit-0 branch instead forces flags 6 and literal
primary `3F7D70A4` (`0.9900000095`); bits 1/2 and the FP discriminator also
control flags 8/10. That special definition branch is static evidence here.
The real Spartan/Elite definitions in the 224-case run use their unchanged
ordinary definition flags. The source's FP discriminator adds flag 8 to its
positive-camouflage selection cases.
[Complete branch](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/contract/geometry_selector-0004bcc0.asm),
[compiled constants](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/render-contract/report.json)

The next phase is a **declared composition**, not execution of the complete
`4B5D0` geometry builder. It copies the actual returned flags/shader/primary
to a zero-initialized `1E4h`-byte whole collector: offsets `+170/+178/+188`,
accepted-model count `+166 = 1`, backed owner word at model-context `+48`,
and **drawable-section count zero**. Root render context, camera/depth inputs
and the counter bank are backed and explicitly cold. Complete `133AB0` and
`133F70` execute to return, including the actual clone recursion and biped
kind check. No ready geometry or successful rendering callback is supplied.
[Composition and raw collector snapshots](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/whole-owners.json),
[oracle](../tools/oracle_camouflage_overlay.py)

`133AB0` requires accepted models and a clear recursion guard. Its camouflage
flag-2 branch copies all `1E4h` bytes, installs the override shader at `+1D0`,
discriminator byte `+1D4 = 1`, primary scalar at `+1D8`, and computed depth
scalar at `+1E0`, then recursively collects with guard 1. It copies alternate
draw flags `+1B0` to clone `+1AC`; the semantics of those raw draw flags are not
inferred here. The root's `+1CC` byte prevents duplicate counter allocation;
the cold bank increments once and records counter kind `+1C4 = 1`. Setting
the local suppression byte at `133C45` skips the root's ordinary `133F70` call
at `133DD7..133DE2`. Shield/overshield branches remain distinct later owners.
[Complete collector](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/contract/collector-00133ab0.asm)

The executed finite results are:

| Primary input | Cases | Whole collector calls | `133F70` recipient | Counter after |
| --- | --- | --- | --- | --- |
| `-.1`, `-0`, `+0` | 96 | 96 root calls | Root ordinary collector | 0 |
| `.25`, `.75`, `1`, actual fade residue `31C00000` | 128 | 128 roots + 128 clones | Camouflage clone only | 1 |

Each row varies species, activation flags 0/8, FP identity equality,
force-world 0/1 and caller mode 0/1. There are 352 whole `133AB0` entries and
224 whole `133F70` entries. All source unit bytes remain unchanged. The small
positive residue still admits camouflage after activation clears; replacing
this admission with the activation flag would change the original behavior.
The residue is an original fade output documented by the separate state
corpus, not a guessed epsilon cutoff.
[Whole result/input records](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/whole-owners.json),
[fade timeline](MP_CAMOUFLAGE_STATE_SOURCE_RESEARCH.md)

## Literal global shader and authored passes

The pointer is owned by original `22590`: if active matg `+108` count is zero
it clears global `485A80`; otherwise it assigns matg `+10C`. The selector
then reads rasterizer-row `+D0`, the datum word of the tag reference starting
at `+CC`. This complete owner is static evidence; its later resource/device
initialization is not executed or replaced. The execution oracle supplies
the actual backed rasterizer pointer read from Lockout, not a substitute
shader chosen by name.
[Complete pointer owner](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/contract/rasterizer_pointer-00022590.asm),
[schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/matg.xml)

Every one of the 21 owned caches has one `108h`-byte rasterizer row and the
same shader/template/pass family. Lockout's exact datums are:

| Record | Lockout datum | Authored identity |
| --- | --- | --- |
| Global shader | `E7E90664` | `rasterizer\render_layer_disabled` |
| Template | `E7EA0665` | `shaders\shader_templates\opaque\render_layer_disabled` |
| Layer 1 pass, 1 implementation | `E7EB0666` | `shaders\shader_passes\texture_mapping\texaccum_disabled` |
| Layer 18 pass, 4 implementations | `E7EC0667` | `shaders\shader_passes\active_camo\active_camo_opaque` |
| Layer 19 pass, 1 implementation | `E7EF066A` | `shaders\shader_passes\active_camo\active_camo_opaque_stencil_modulate` |

The sole shader LOD has available-layer mask `000C0002`. The apparently
disabled shader name does not mean it lacks camouflage passes. The report
retains each cache's actual datum, raw records, selected packed layer/pass/
implementation ranges, texture bindings, packed extern records, render/
texture states and vertex payload hashes; names alone are not the authority.
[Literal all-map inventory](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/assets.json)

For layer 18, implementations 0/1/2 have numeric texture-extern bindings
26/0/17/1 at stages 0/1/2/3. Implementation 3 uses authored bitmap parameter 0
at stage 0 and numeric extern 1 at stages 1/2/3. The shader bitmap table is
`default_vector`, `default_vector`, `color_white`; Lockout datums are
`E1970022/E1970022/E1CA0055`. Layer 19 has no texture bindings. The selected
passes reference actual Xbox vertex shader families `stencil_texture`,
`transparent_active_camo_viewer_normal`, `transparent_active_camo_dxdy` and
`fog_patchy_stencil_modulate` (see report for classifications/payloads).
These are resource declarations,
not successful texture-cache acquisition or generated refraction targets.
The numeric texture externs' complete target producers remain unresolved
here; assigning a scene/depth/reflection meaning from their numbers or pass
names would be guesswork.
[Packed texture records and resources](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/assets.json)

All 21 rasterizer rows share the same parameter bits: D4 flags 0, D8 11.0,
DC 22.0, E0/E4/E8 `3F57D7D4/3F56D6DC/3F69E9F2`, EC/F0 9.0, and F4/F8/FC
1.0. The schema labels these refraction/falloff/tint and hyperstealth fields.
Their actual runtime consumption and final visual arithmetic are not
executed by this asset census.
[Raw rows](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/assets.json),
[supplemental schema names](../tools/reference/assembly/src/Assembly/Plugins/Halo2/matg.xml)

## FP gun material context is the full weapon owner

Original `3D2C0` fills the FP bank through `165F16(user, selected_unit, 4,
4C1BD8)`. The producer reads two hand states separated by `1010h`. Its gun
record call at `166162..16617A` passes the actual registered hand weapon
`[ESI]` at `166178` to `165E9F`. That complete builder stores the full owner
at record `+4`, authored render model at `+0`, flags at `+8`, and matrices at
`+C`. Its arms call at `1660D8` and optional body call at `16621F` instead pass
the selected unit. These are explicit distinct owner inputs, not an unknown
FP camouflage owner or an implicit unit substitution for gun records.
[Complete FP bank initializer](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/contract/fp_bank_init-0003d2c0.asm),
[complete producer](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/contract/fp_producer-00165f16.asm),
[complete record builder](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/contract/fp_record_builder-00165e9f.asm)

`3D4F0` with AL true reads the selected `D0Ch`-stride FP record, returning
record `+4` as geometry owner. `4B5D0` stores it at geometry `+30` and calls
`3DDD0(owner)`; the selected cache entry's bank at `+44` begins with the full
owner written by `3DF35` on initialization. The builder stores that bank
pointer at geometry `+44`, thus whole collector `+48`. `133F70` passes it as
`2CBF0` argument 5; `2CC84` stores it at draw unit `+10`. Finally `4D9C0` at
`4DA6D..4DA70` loads that pointer and calls `1BD50`, which publishes it to
global `485A58` before shader dispatch `1BBF0 → 16B90`. Scalar37's shim reads
the first word of that context and invokes the original first-biped resolver.
[Identity outputs](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/contract/model_identity-0003d4f0.asm),
[geometry builder](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/contract/geometry_builder-0004b5d0.asm),
[cache owner](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/contract/object_cache-0003ddd0.asm),
[subpart collector](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/contract/subpart_collector-00133f70.asm),
[draw allocator](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/contract/draw_allocator-0002cbf0.asm),
[draw binder](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/contract/draw_binder-0004d9c0.asm),
[context installer](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/contract/context_installer-0001bd50.asm)

The 12 helper compositions independently execute whole `165E9F`, `3D4F0`,
the retained branch of whole `3DDD0`, `1BD50`, and original scalar37 dispatch/
shim/resolver/object lookup. Inputs use real carbine FP mode `EC6E0AE9`, its
actual node count, explicitly declared identity matrices, backed weapon/biped
records and retained cache banks. Weapon and unit identities survive unchanged;
unparented weapon gives zero, attached weapon gives its biped's primary,
and unit-owner records give their own primary (0/.25/`31C00000`). This is a
declared helper composition, not whole `165F16` production, cache allocation,
geometry admission, draw-unit transport or shader/device success. Those
upstream/downstream full bodies above are static source evidence only.
[Inputs, outputs, executed bytes and complete bodies](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/contract/report.json)

Applying each owner bank's resolved scalar before ordinary animation preserves
the source's `16B90` extern-before-animation order. The earlier asset census
separately establishes no scalar37/animation mask overlap within its exact
weapon contexts, and no scalar37 body/hands usage in those caches. That does
not replace the biped clone/override path established here. Different owners
and local-view/hand banks must retain their own contexts and resolved values.
[Earlier scalar/mask/order evidence](MP_CAMOUFLAGE_RENDER_SOURCE_RESEARCH.md)

## Remaining visual requirements

This research establishes actual selection, clone/suppression and authored
resources. Its empty-section collectors make zero `2CBF0` calls, so they do
not prove actual resident model-section acceptance, override table admission,
scalar quantization/transport, `41040` override-mask compatibility, pass
implementation choice, texture/vertex resource readiness or world/FP queue
execution. Static owners for these seams are preserved; prior shield research
describes their ABI without implying camouflage draw execution.
[Explicit whole-owner boundary](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/whole-owners.json),
[shield transport research](SHIELD_OVERLAY_SOURCE_RESEARCH.md)

The complete runtime producers/consumers of texture extern 26/0/17/1,
framebuffer copy/target lifetimes, layer-18 refraction and layer-19 stencil/
blend behavior still require original execution and GPU comparison. Shadow
camouflage uses a separate root/fade owner. Generic alpha transparency or
weapon scalar37 alone cannot stand in for those owners. Normal King admission,
the wider gamemode goal and complete live visual/network proof remain gated
by their existing requirements.
[Prior shadow/scalar boundary](MP_CAMOUFLAGE_RENDER_SOURCE_RESEARCH.md),
[asset execution limits](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/assets.json)
