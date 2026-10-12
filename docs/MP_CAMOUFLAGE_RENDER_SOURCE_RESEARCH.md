# Original camouflage cap callers and render inputs

Research snapshot: 2026-10-11. The original objective callbacks cap camouflage
only under their actual possession/variant guards. Shader scalar 37 resolves
the first biped in an object's parent chain, then clamps that biped's primary
amount to `[0,1]`. This scalar is a weapon-material input; the biped also has a
separate camouflage collector, override-shader and ordinary-draw suppression
path. This research changes no native implementation or gamemode admission.
The all-39-variant goal remains active.

## Primary sources and reproducibility

The unchanged images are [retail](../assets/disc/default.xbe), SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`;
[TU 1.1](../assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03),
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`;
and the locally owned u5English image at
`/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe`,
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The original [Lockout cache](../assets/disc/maps/lockout.map) is pinned to
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.

[oracle_camouflage_render_context.py](../tools/oracle_camouflage_render_context.py)
executes 144 whole direct cap calls, 2,304 whole registered Oddball callbacks
and 156 original scalar-dispatch calls across the three images. Backed player,
object/header, clock, selected-engine and context banks are explicit inputs;
the actual original inventory search, selected trait and entity-NONE dirty
wrapper execute. No instructions or game returns are replaced.
[Frozen fixture](../tests/fixtures/camouflage_render_context_original.json),
[report and complete instruction bodies](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/report.json),
[readable source index](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/source-index.json)

The normalized results agree across retail, TU 1.1 and u5. All 18 complete
retail/TU-to-u5 owner comparisons agree after image-address relocation,
including literal constant contents. The scalar dispatcher has other slots,
so only its actual slot-37 path is covered; the complete-shape comparisons
cover its three-instruction slot-37 shim and the whole parent resolver.
An independent generation reproduces the fixture and report byte for byte.
Fixture SHA-256 is
`604c6b7c3353a77e98588e4a13c944b52238b929164e557173112742b1e1a3f6`.
[Reproduction](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/reproduction.json)

```sh
.tools/python/bin/python tools/oracle_camouflage_render_context.py
.tools/python/bin/python tools/oracle_camouflage_objective_cap.py
.tools/python/bin/python tools/audit_camouflage_animation_masks.py
.tools/python/bin/python tools/inspect_camouflage_render_contract.py
```

The state/timer/fade/disruption owners and their separate limitations remain
in [MP_CAMOUFLAGE_STATE_SOURCE_RESEARCH.md](MP_CAMOUFLAGE_STATE_SOURCE_RESEARCH.md).
This report does not repeat or broaden that evidence.

## Objective cap ownership

| Owner | Retail | TU 1.1 | u5 |
| --- | --- | --- | --- |
| Cap | `15FE70` | `161510` | `1628F0` |
| CTF engine 1, slot 15 / `+3C` | `2403B0` | `2424C0` | `2496B0` |
| Oddball engine 3, slot 15 / `+3C` | `2BC920` | `2C2CA0` | `2C6790` |
| Oddball slot 35 / `+8C` trait | `2BCC90` | `2C3000` | `2C6AF0` |
| Inventory kind search | `19F3C0` | `1A2D20` | `1A8FD0` |

The cap accepts the full player datum in EAX. Player NONE and current-unit
NONE return. It then indexes the raw player/unit slots, requires unit `+134`
bit 3 and writes `min(primary +2B0, 0.5)` for finite inputs. The literal
comparison supplies an upper cap, with no zero floor: negative primary is
preserved. It calls the original dirty wrapper with the current unit in EAX
and stack mask `00800000`, even when the amount was already below the cap.
The wrapper naturally returns for the fixture's actual entity `+D4 = NONE`;
successful replicated-entity publication is outside this corpus.
[Whole TU cap](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/source/tu11-cap-00161510.asm),
[whole dirty wrapper](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/source/tu11-dirty_unit-000a87e0.asm)

Raw direct-call discovery finds TU calls at `2425D2` and `2C2CFE`, inside the
registered CTF and Oddball callbacks above. Both callback bodies were missing
from the earlier TU function-export index; absence from that index was not
absence of a caller. King slot 15 does not call this owner. Consequently this
cap is not a generic post-refresh King operation.
[Original xrefs, registrations and bodies](../tests/fixtures/camouflage_render_context_original.json)

The whole Oddball callback takes a stack player and returns `RET 4`. It requires
a current unit and a kind-2 item in one of its four inventory slots
`+218/+21C/+220/+224`. The original search resolves each item's actual `weap`
definition and reads signed short `+290`; it returns the first matching item.
Real Lockout flag has kind 1 and ball kind 2. With a ball present, the callback
queries the selected engine's actual trait slot 35, selector 1, returns when
that query is true, then excludes raw options mode byte `+C == 4` before calling
the cap. All four inventory positions, no item/SMG/flag/ball, unit presence,
mode 1/4 and activation flags are exercised.
[Whole callback](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/source/tu11-oddball_player-002c2ca0.asm),
[whole inventory search](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/source/tu11-inventory_kind_search-001a2d20.asm)

With a ball present, the actual Oddball selector-1 trait reads **options
`+22C`, bit 1**, the carrier camouflage flag. It does not use global camouflage
for this branch. Only a no-ball trait query delegates to `253F40`; the slot-15
callback already exits when no ball exists. The base corpus explicitly has
`+22C = 0`; its varying global flags do not constitute a carrier-trait-on case.
[Whole registered trait](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/source/tu11-oddball_trait-002c3000.asm)

[oracle_camouflage_objective_cap.py](../tools/oracle_camouflage_objective_cap.py)
therefore adds **576 whole callback calls** with actual `+22C` values 0/2,
ball in each inventory position, modes 1/4, unit presence and activation flags.
At active primary `.75`, mode 1 and a present unit, `+22C = 0` produces `.5`;
`+22C = 2` preserves `.75`. Global camouflage is deliberately on in both cases.
The three images agree and a second generation is byte-identical. Supplemental
fixture SHA-256 is
`2b61625b80d50d18857fe06aff3195352f1362ad28378b96e2c16b4613486fce`.
[Supplemental fixture](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/objective-cap/fixture.json),
[report](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/objective-cap/report.json),
[reproduction](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/objective-cap/reproduction.json)

CTF is **complete static body evidence**, not whole callback execution here.
Before its cap call, the body excludes mode 4, requires `15C930` to return true,
a current unit, seat index `+1FC = -1`, selected weapon from `CD1A0` other than
NONE/low16 `FFFF`, actual weapon kind 1, options game-over byte `+1128 = 0`,
weapon objective index `+17E != -1`, and false selected selector-1 trait and
`139FF0` queries. The callback then continues through separate objective logic.
The upstream `15C930` admission and objective services are not fabricated or
claimed executed by this research.
[Whole CTF body](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/source/tu11-ctf_player-002424c0.asm)

## Shader scalar 37: first biped, then clamp

Retail scalar dispatcher `336F0`, slot 37, enters `33850`, which obtains the
current object's full datum from the shader context and tail-calls `33670`.
TU/u5 use `336E0 → 33840 → 33660`. The resolver initially calls the full-datum
object lookup (`BADC0` retail / `BBCB0` TU / `BCDF0` u5), so the initial NONE,
salt and backed-header/body guards run unchanged. It starts with scalar zero.
[Whole retail resolver](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/source/retail-render_parent-00033670.asm),
[slot shim](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/source/retail-scalar37_shim-00033850.asm),
[initial lookup](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/source/retail-initial_object_lookup-000badc0.asm)

While object type byte `+AA` is nonzero, it follows full parent `+14`, returning
zero on parent NONE or a null body. These subsequent lookups use raw low16
header indexing. At the **first type-0 biped**, it stops, even if that biped
itself has a vehicle parent. It reads only that biped's primary `+2B0` and
clamps finite values to `[0,1]` with the original comparisons. It does not read
secondary `+2B4` or require activation bit 3. That distinction preserves the
remaining primary fade after the flag/timer changes.
[Resolver instructions](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/source/retail-render_parent-00033670.asm)

The 52 declared contexts per image cover primary `-.1/0/.25/.5/1/1.1`, an
unparented biped, a vehicle-parented biped, attached weapon, weapon→biped→vehicle,
weapon→vehicle, unparented weapon/vehicle and weapon→prop→biped. NONE, initial
salt mismatch/zero salt and null parent body are also exercised. A memory-read
observer records the actual primary slot read; original object/header bytes
remain unchanged. No cycle, invalid/unbacked raw parent, NaN or infinity is
admitted. Natural parent-link construction and shader-context publication are
separate owners, not inferred from these declared chains.
[Fixture input chains and observed reads](../tests/fixtures/camouflage_render_context_original.json)

The existing unattached-weapon helper correctly resolves zero for its restricted
unparented type-2 domain, but cannot supply held/FP owner camouflage. The
existing shadow root helper is different: retail `BAF80` follows parents to the
ultimate root. `3D7F0` separately computes model/death/camouflage shadow fade,
including current type/header distinctions. A weapon→biped→vehicle shader
context must therefore not use the shadow ultimate-root identity for scalar37.
[Unattached helper](../src/source_netgame_weapon.cpp),
[whole ultimate-root body](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/render-contract/ultimate_root-000baf80.asm),
[shadow source contract](source-shadow-submission.md),
[current native shadow owner](../src/source_shadow_submission.cpp)

## Authored masks and native material seam

Original `16B90` restores the authored program, applies `17170/174D0`, binds
`19BE0` then **`19CA0` pixel externs before `17420` animation**. Animated masks
win overlapping components. A general reversal of this order is invalid.
[Complete original binder](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/render-contract/draw_binder-00016b90.asm),
[complete pixel extern owner](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/render-contract/pixel_extern-00019ca0.asm),
[independently executed binding/order evidence](SOURCE_SHADER_COLOR_ANIMATION.md)

[audit_camouflage_animation_masks.py](../tools/audit_camouflage_animation_masks.py)
reads all 21 original Xbox multiplayer caches, matg multiplayer representations
2/3 bodies/hands, and every weap world/FP render-model material table. It retains
all authored shader LODs and available independent layers, with dynamic layer 3
implementation 2. The 786 NONE model/FP references are explicitly enumerated;
no replacement is inferred. The resulting **1,344 model roles, 2,142 shader-map
contexts and 22,155 pass contexts** are a conservative asset superset, not draws.
[Census, raw masks, roles and cache hashes](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/animation-masks.json)

Scalar 37 occurs in **273 pass contexts**, 13 weapon shader names across the
21 maps, each writing alpha via literal component 3. **168** of these contexts
also contain authored pixel animation, and **zero** have overlapping
register/channel masks. For example, Lockout carbine display writes scalar37
to register 20 alpha while its authored animations write register 10/19 alpha.
No body or hands material in this census writes scalar37. The mask report
reproduces byte for byte, SHA-256
`ce96aa8c79dd70aa00b8e453dc27ff506349cf50d0936685f3c1a7cb7bcee283`.
[Mask report](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/animation-masks.json),
[reproduction](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/animation-reproduction.json)

This proves that a scalar37-only remap commutes with the authored animated
pixel writes **within these exact asset/pass/default-switch contexts**. It
supports applying that remap to an already animated per-draw program copy in
the current renderer, without overwriting this census's animation channels.
It does not authorize the same sequencing for other externs, switches,
implementations, shaders or future assets. The original remapper's packing and
destination/fragment rules must still be used.
[Raw masks and declared limits](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/animation-masks.json)

At inspection, `world_material.cpp` already maps an explicitly supplied
camouflage value through scalar37 and retains that baseline in the selected
pass before ordinary animation. `animate_forward_material` restores the pass
bank and then applies animated masks. `SourceDynamicModelShaders` prepares the
resident LOD materials but explicitly does not choose owner/pose/attachments.
`renderer.cpp` creates a per-view material-uniform copy for existing owner
lighting. These are usable seams, not proof of live scalar37 owner binding.
[Material preparation](../src/world_material.cpp),
[animation](../src/shader_animation.cpp),
[resident bank](../src/source_dynamic_model_shaders.cpp),
[declared bank boundary](../include/halo2/source_dynamic_model_shaders.hpp),
[per-view draw material](../src/renderer.cpp)

The original cold unit primary is zero; unparented weapon resolution is zero.
Current body/hand template preparation uses an absent optional while held/FP
gun templates supply zero. A native binding should begin from an explicitly
resolved cold value of zero, then provide each current owner's resolved scalar
to every resident pass/LOD child it actually draws. It must preserve owner and
view isolation: two players using the same authored weapon shader can have
different primary amounts. A shared global material amount has no original
authority. The caller must still establish the actual object/parent context;
the first-biped rule is not permission to assign an arbitrary nearby player.
[Current runtime material construction](../src/multiplayer_runtime.cpp),
[cold state evidence](MP_CAMOUFLAGE_STATE_SOURCE_RESEARCH.md),
[actual scalar context evidence](../tests/fixtures/camouflage_render_context_original.json)

## Separate biped camouflage collector requirement

Scalar37 alone cannot establish biped camouflage rendering. Complete static
retail `4BCC0` has a separate normal branch at `4BED0..4BF23`: actual current
object type 0 or 1, primary `+2B0 > 0` (`COMISS/JBE`, unordered skips), then
geometry-context `+16C |= 2`, shader from global `[485A80]+D0` into `+174`, and
primary into `+184`. The comparison constant at `45DBD8` is exactly zero. This
branch does not test unit activation bit 3 or use scalar37 parent traversal.
It precedes the zero-caller-mode shield gate. An independent definition
`hlmt+9C` bit-0 branch forces flags 6 and literal primary bits `3F7D70A4`
(`0.9900000095`); the other definition/first-person bits alter flags 8/10.
[Complete selector](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/render-contract/geometry_selector-0004bcc0.asm),
[compiled constants and static-owner report](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/render-contract/report.json)

Whole collector offsets are shifted by four: flags `+170`, camouflage shader
`+178`, primary `+188`. With accepted model count `+166 > 0` and recursion guard
clear, `133AB0` tests camouflage mask 2 at `133BE0`. It copies all `1E4` bytes,
sets clone override shader `+1D0` from `+178`, discriminator `+1D4 = 1`, scalar
`+1D8` from `+188`, and computed depth scalar `+1E0`, then recurses with its guard
set. Its local ordinary-suppression byte is set at `133C45`, so the later
ordinary collector call at `133DD7..133DE2` is skipped for that camouflage
branch. Shield/overshield special clones remain separately ordered owners.
These are source field/collection contracts, not a rendered distortion proof.
[Complete collector](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/render-contract/collector-00133ab0.asm),
[existing collector/override research](SHIELD_OVERLAY_SOURCE_RESEARCH.md)

The minimal remaining visual work beyond scalar37 is therefore actual
camouflage collector admission, authored override shader resolution, cloned
geometry/material input transport and ordinary suppression, followed by the
real pass/queue/texture/blend consumers and shadow behavior. This report does
not infer their final framebuffer effect or replace them with generic alpha.
The seven complete static render-owner listings reproduce identically but
were not executed here as a successful camouflage draw. Complete native
gameplay, pickup producers, first-person/world presentation, GPU output,
replication and normal King admission remain separate requirements.
[Static report and limits](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/render-contract/report.json),
[static reproduction](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/render-contract/reproduction.json)
