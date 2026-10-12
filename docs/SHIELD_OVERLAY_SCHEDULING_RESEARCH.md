# Shield override part banks and original scheduling

Date: 2026-10-11. This continues
[the plasma pass research](SHIELD_PLASMA_PASS_RESEARCH.md) for the active
damage/shield goal. It identifies a bounded part-bank seam and the scheduling
owners still needed for live draws. No implementation was changed for this
research, and no live or original-framebuffer parity is claimed.

## Evidence

Authority is the original disc [`default.xbe`](../assets/disc/default.xbe),
SHA-256 `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`,
and [`lockout.map`](../assets/disc/maps/lockout.map), SHA-256
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
Instruction listings and a hash-pinned snapshot of the inspected native files
are retained in
[`overlay-pass/scheduling/`](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/).
The manifest includes exact function byte sizes/hashes, all four actual
shield priority/command fields, and native snapshot hashes:
[`source-evidence.json`](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/source-evidence.json).
Ghidra/decompiled text was navigation only. No TU1.1/TU1.5 correspondence for
these owners was established here.

The retained extractor can refresh the original instructions/fields and take
a new native snapshot. Preserve the current manifest/snapshot first if the
historical inspected native state is needed:

```sh
.tools/python/bin/python analysis/damage-shields-goal-20261011/overlay-pass/scheduling/extract.py
```

## Minimum independent override part-bank service

`41040` receives ECX = original part ordinal, DX = signed 16-bit render-cache
index, and seven stack arguments: view mode, coverage byte, original material
geometry context, override shader, force-regular byte, and two callback
arguments. It returns with `RET 1C`. The original material geometry context
contains shader `+00`, resolved center `+04..0C`, depth bias `+10`,
plane-present byte `+14`, plane `+18..24`, and resolved part LOD scale `+28`.
The actual part is a 72-byte resident record; its type word is `+00`.
[41040 instructions](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/00041040-retail.asm),
[45CE0 instructions](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/00045ce0-retail.asm)

The service can produce admitted original part identities and chosen material
bindings without deciding their eventual renderer queue. Its required sequence
is:

| Step | Exact source behavior | Required retained metadata/input |
| --- | --- | --- |
| Original part gate | `41120..41134` rejects type 0/5 and original shader `NONE`. | Original section/part, type, original material/shader. |
| Choose shader | `4113A..41145` uses the override unless `NONE`. | Original shader and explicitly selected shield datum. |
| Chosen shader LOD | `410F8..4111A` forms metric; `4115B..41162` passes chosen shader `+3C` to `1BF50`. | Original collector coverage byte; resolved part scale; actual view multipliers/modifier; chosen shader thresholds and `+3C`. |
| Selected mask | `411AC..411BE` reads chosen template LOD mask at the selected ordinal. | Chosen template's complete LOD mask bank. |
| Original mask indexing | `411D1..41226` reads original template mask at the **chosen shader's selected LOD ordinal**. | Original template's complete mask bank, including that ordinal. |
| Override compatibility | `41275..4129D`: if original mask has `0x8080` and lacks `0xA`, reject when **override LOD 0** mask has `0x200000`. | Original selected-ordinal mask and override LOD 0 mask. |
| Result | Preserve chosen shader, chosen LOD/mask, original identity, original posed geometry and callback context. | Original input class/local positions and current pose ownership, independent of ordinary material slots. |

[41040](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/00041040-retail.asm)

The LOD metric is computed in source instruction order:
`float(coverage_byte) * resolved_part_scale * 4670E4 * 4`.
`1BF50` receives the chosen shader's command/bias word in hidden EAX, keeps it
in ESI, optionally multiplies the metric by `45DC0C` when `4BA019` is set,
and uses `485B28[bias]` for the threshold scan. Bias 5 bypasses the scan and
selects ordinal 0. Thresholds are the chosen template's 10-byte LOD records
at `+6`; the resulting ordinal is clamped to the final valid ordinal.
The source empty-bank result is `-1/true`; it is not evidence for a draw.
`41040` does not inspect the `1BF50` boolean before dereferencing the ordinal.
A bounded native service should require a real nonempty chosen bank and an
in-range original mask at that ordinal, and report an unresolved corpus
boundary if either is absent. It must not reselect the original LOD or clamp
the original mask to LOD 0.
[1BF50](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/0001bf50-retail.asm),
[existing source metric/LOD helper](../src/source_model_shader.cpp),
[LOD interface](../include/halo2/source_shader_lod.hpp)

All four actual Lockout shield roots have priority flags `shad +16 = 0`,
command/bias flags `shad +3C = 0`, one LOD, and mask `0x8000` at LOD 0.
Therefore their selected ordinal is 0 and the compatibility test never adds
a rejection, even when the original material meets the `0x8080/0xA`
predicate. The mandatory original type/material gate still applies.
[Literal cache fields](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/source-evidence.json),
[full authored pass metadata](../analysis/damage-shields-goal-20261011/overlay-pass/lockout-plasma-pass.json)

Override also changes priority and ordinary command inputs. `41157` retains
the chosen shader pointer in EBX; `4130D/4133E` read its `+3C` for regular
command generation, and `413CD..413E1` use its `+16` bits 0/1 for `40E10`.
Priority mapping is `(bit0,bit1): (0,0)->3, (1,0)->1, (0,1)->2, (1,1)->0`.
The actual shields therefore use priority 3. Retain original part type,
render-cache flags and callback arguments; override does not replace these.
[41040](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/00041040-retail.asm),
[40E10](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/00040e10-retail.asm),
[40E30](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/00040e30-retail.asm)

The existing `Cache::load_source_transparent_model_parts` already preserves
the original 72-byte words/type/material, original shader, `+16/+3C`, template
LOD masks, section rigid node and resident node map. The separate
`load_source_transparent_shader` loads corresponding override metadata, while
`load_source_shader_lod_definition` and `source_model_shader_part` own the
ordinary shader LOD arithmetic. These are sufficient metadata seams for a
bounded ordinary-`shad` override bank. Dynamic `MTPR/prt3` template virtual
services remain outside that seam.
[Part metadata interface](../include/halo2/source_transparent_parts.hpp),
[cache decoding](../src/source_transparent_parts.cpp),
[model shader interface](../include/halo2/source_model_shader.hpp)

`SourceDynamicModelShaders` currently owns original-shader LOD slots and
`select` erases ordinary rejected parts. It has no override API. An override
bank must enumerate the original current geometry **before** that erase and
select the override's LOD independently of the ordinary material slot. It
must not infer original part eligibility from whichever vertices survived
ordinary shader selection. This is a source-derived seam proposal, not an
implemented service.
[Inspected implementation](../src/source_dynamic_model_shaders.cpp),
[interface](../include/halo2/source_dynamic_model_shaders.hpp),
[retained native snapshot](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/native-snapshot/src/source_dynamic_model_shaders.cpp)

Subsequently, the parent implemented the separate `SourceShieldModelShaders`
resident bank and reported its full original-`41040` prefix/`1BF50` comparison
passing 1,533 cases, including 93 actual part contexts. Its four-model native
test covers 48 view selections, 288 part checks and 329,940 posed vertices.
This supersedes the unimplemented-bank status above while leaving the
historical snapshot intact. These are part/material/geometry checks, not live
queue or pixel evidence.
[New bank](../src/source_shield_model_shaders.cpp),
[bank interface](../include/halo2/source_shield_model_shaders.hpp),
[original instruction oracle](../tools/oracle_shield_override_parts.py),
[native test](../tests/source_shield_model_shaders_tests.cpp)

## Upstream part and current geometry admission

The bank alone is not full part visibility. Ordinary `44AC0` first requires
render-cache `flags & requested_mask & 0x1FFFFF` nonzero, packed code group
bits 4..8 equal the requested group, flags bit 0 set, and cache byte `+1A`
nonzero. It resolves the resident part bank with `449E0`, geometry metadata
with `45CE0`, and per-part visibility with `460D0`. The override datum is
looked up through per-cache byte `4C0ED6` into `4C1228`; `FF` means none.
For the ordinary group 31 it calls `41040` with force byte 0. The main
deferred-FP producer uses the same part/geometry/visibility services and
requires packed code `& 0x1F0 == 0x1F0`.
[44AC0](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/00044ac0-retail.asm),
[3FDF0](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/0003fdf0-retail.asm),
[449E0](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/000449e0-retail.asm)

`460D0` returns true without the conditional subpart test when render-cache
flag `0x40` is absent. When present, it checks the actual visibility mask for
the part's authored subpart range (`part +0A` first, `+0C` count). A native
full-admission claim needs that upstream mask producer, not only a loaded
section. `45CE0` uses the current posed rigid-node translation as model
center; rigid node `NONE` uses the object matrix translation. Model flag 4
uses the actual geometry-LOD context remap before accessing that node.
Depth bias uses cache `+1B`'s low nibble: zero gives zero; otherwise
`(nibble - 5) * floatbits(0x3A83126F)`. The part scale is `part +2C` when
positive, else 1. Model parts have no plane output. Current native helpers
already expose this explicit geometry contract.
[460D0](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/000460d0-retail.asm),
[45CE0](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/00045ce0-retail.asm),
[native geometry helpers](../src/source_transparent_parts.cpp)

## World layer 15 is an ordinal queue

At `412A3`, mask `0x8000` with force byte zero skips regular command creation.
At `41361..41381`, layer-15 bit set, force zero and view mode zero reaches
`1E2D0`. The latter allocates at most 1,024 records of stride `0x70` and
increments `4B6280`; it clears priority and only the low plane-present byte,
not the entire record. `41040` then fills:

| Offset | Value/owner |
| --- | --- |
| `+00` | Chosen shader priority from `40E10`. |
| `+04` | `0 - ((deltaZ*forwardZ + deltaY*forwardY) + deltaX*forwardX) - depth_bias`, separate source SSE operations. |
| `+08` | Zero secondary key. |
| `+0C/+10` | Geometry context's plane-present byte and optional plane. |
| `+20` | Callback token `41020`. |
| `+24` | Chosen override/original shader datum. |
| `+28` | Original packed draw token: coverage byte, render-cache identity and original part ordinal. |
| `+64` | Resolved geometry center. |

[41040](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/00041040-retail.asm),
[allocator 1E2D0](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/0001e2d0-retail.asm)

`1E370` initializes 16-bit original ordinals in `[4B6284,4B6280)`, runs the
actual `13DF80` sort with `1E700`, and calls each nonnull record callback with
payload `+24`. Priority subtraction wraps as signed 32-bit; plane comparisons
take precedence over depth and secondary ties, and may not form a strict weak
ordering. The original ordinal sort's equal-key swaps matter. Existing
`source_transparent_sort` represents that algorithm; `std::sort`, stable
sorting or sorting material IDs is not equivalent.
[1E370](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/0001e370-retail.asm),
[1E700](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/0001e700-retail.asm),
[existing implementation](../src/source_transparent_sort.cpp),
[existing differential scope](SOURCE_TRANSPARENT_SORT_PARITY.md)

`41020` invokes `40F60` with admission callback `4CBF0` and draw callback
`4D0B0`. `40F60` obtains the chosen shader template's **LOD 0** layer-15 pass
count, then invokes those callbacks with LOD 0, layer 15, implementation 0
and each pass ordinal. The transparent payload does not retain the earlier
selected LOD. This distinction is harmless for these one-LOD shields but must
not be generalized into selected-LOD transparent drawing.
[41020](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/00041020-retail.asm),
[40F60](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/00040f60-retail.asm),
[callback binding/draw owners](SHIELD_OVERLAY_SOURCE_RESEARCH.md)

When context `+2A78 == 0`, `40E90` clears regular command globals and both
transparent count/first globals, calls `41C80`, then `44AC0` with group 31.
It subsequently calls other producers: `176BB0` or `176CB0`, `174220(0)` in
the former branch, `420A0`, `42760`, and `41640`. Therefore ordering is across
the shared command bank, not solely across shield models. Their complete
live integration is an external boundary here. The current sorter also has
an explicit grouped-section producer, so its data structures are useful, but
that does not prove these live callers are wired.
[40E90](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/00040e90-retail.asm),
[existing queue API](../include/halo2/source_transparent_sort.hpp)

## First-person shader choice and first-person scheduling are distinct

The selector's FP shader datum does not identify a regular-command draw path.
`133F70` maps collector byte `+16C` into cache flag `0x400`, separately from
per-model projection byte `+70+ordinal` mapped into flag `0x800`.
`44AC0` defers a cache with `0x400`: for its view argument zero, it appends
the cache index to `4C8740`, limited to 32; for nonzero view it copies eight
dwords into the corresponding `4C62F8` bank, also limited to 32.
[133F70](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/00133f70-retail.asm),
[44AC0](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/00044ac0-retail.asm)

`3FDF0` stages the main deferred bank from `4C8740`. It calls `41040` with
view mode 0 and force byte 0, so an admitted pure-mask-8000 shield appends a
transparent command. At entry it sets `4B6284 = 4B6280`, saves the current
regular bank and clears its aggregate flags, then calls `174220(1)` before
enumerating the deferred parts. Its stack selector zero executes regular
layers 1, 3, 8, 20 and 7 (subject to their guards); nonzero executes layer 16.
It does not execute layer 15 as a regular command list.
[3FDF0](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/0003fdf0-retail.asm),
[174220 retained body](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/00174220-retail.asm)

The specialized `402B0` stages the copied bank and calls `41040` with view
mode 1 and force byte 1. This forces regular command construction and excludes
transparent enqueue. Its caller `404B0`, however, explicitly executes layers
11, 14, 12 and conditional 13. It has no layer-15 execution. Thus the facts
that `402B0` accepts a part and `41040` can emit a regular layer-15 command
do **not** establish a shield draw from that specialized path.
[402B0](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/000402b0-retail.asm),
[404B0](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/000404b0-retail.asm)

The visible late `2BCD0` order is: `36B50`, layer-15 `44370`/flush, `1595D0`,
`42B20`, conditionally `1E370` for the world queue; then, when its outer mode
allows FP, set `485A76 = 1`, call `3FDF0` with stack selector 0, call `404B0`,
call `3FDF0` with stack selector 1, conditionally call `40870`, clear
`485A76`, and continue toward postprocess/HUD. `40870` drains with `1E370`
and restores `4B6280 = 4B6284`.
[2BCD0](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/0002bcd0-retail.asm),
[40870](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/00040870-retail.asm)

Each `3FDF0` entry writes a new suffix start. The retained trace below resolves
the shield-command lifetime for explicit resident and external-service inputs:
the first call's suffix is superseded by the second call's start write; there
is no intervening shield draw from that suffix. The final drain uses only the
second suffix and restores count to its start. It does not free the first
suffix. Particle production and GPU/material binding remain explicit service
boundaries, not inferred live behavior.

## Retained first-person suffix execution trace

The new isolated instruction trace runs unchanged `2C2AB..2C2D0` caller
instructions and complete `3FDF0`, `404B0/402B0`, `40870`, `41040`, `1BF50`,
`449E0`, `45CE0`, `460D0`, `40DE0/40E30/3B9D0`, `1E370`, the actual ordinal
sort, and `41020/40F60`. Synthetic resident/tag/pose inputs describe one
ordinary `shad` part and an override with the actual shield's one-LOD
`0x8000` topology. Original executable bytes are not patched.
[Trace script](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/trace_fp_suffix.py),
[111-case trace report](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/fp-suffix-trace.json),
[representative retained output](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/fp-suffix-trace.txt)

Its hooks explicitly supply an empty `174220` particle service, a resolved
current-pose context to `1D6B0`, current-object/view setup (`C17F0/4A1E0`)
for the populated specialized bank, and GPU state/cleanup services
(`15370/35AA0/3F5CF0/15680/3F5590/3F8760/3F6190/14BC0`). Material admission
`4CBF0` returns an explicit tested false/true value; the draw-service `4D0B0`
logs the actual invocation. No hook changes the scheduling, LOD, part gate,
allocator, ordinal sorter, queue range, or `40F60` arguments. This is bounded
instruction execution with explicit external callbacks, not original GPU
execution or a real frame capture.

All 111 cases satisfy the expected source range/callback behavior. They vary
existing prefix count 0/2/1022/1023/1024, accepted/rejected original part type,
caller drain off/on, pass admission false/true, view-type mask, and empty or
populated specialized bank. For an accepted part with prefix 2 and final drain
enabled:

| Original event | Count | First | Consequence |
| --- | --- | --- | --- |
| First `3FDF0` entry / `3FE00` | 2 | 2 | Establish first suffix. |
| First `41040 -> 1E2D0` | 3 | 2 | Append shield at ordinal 2. |
| `404B0`, empty or populated | 3 | 2 | No transparent range change or layer-15 draw. |
| Second `3FDF0` entry / `3FE00` | 3 | 3 | Supersede first suffix. |
| Second `41040 -> 1E2D0` | 4 | 3 | Append shield at ordinal 3. |
| `40870 -> 1E370` | 4 | 3 | Sort/call ordinal 3 only. |
| `41020 -> 40F60` | 4 | 3 | Call admission with LOD 0/layer 15/implementation 0/pass 0; call draw only when admitted. |
| `4087A` | 3 | 3 | Restore count to the second suffix start; ordinal 2 remains allocated before it. |

The populated `404B0/402B0` case uses both regular-context flags and executes
actual `40E30/3B9D0`, producing layer-15 aggregate flags `0x8000` in both
regular contexts. `404B0` still draws none of its 11/14/12/13 layers for that
mask and does not consume the transparent suffix. This directly tests the
distinction between constructing a regular layer-15 command and executing it.
[40E30](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/00040e30-retail.asm),
[3B9D0](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/0003b9d0-retail.asm),
[404B0](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/000404b0-retail.asm)

Capacity is observable: with prefix 1,023 the first call consumes the final
slot, the second call sets first to 1,024 and its allocation fails, so the final
drain is empty. Do not reclaim the first allocation or collapse the two calls
when reproducing that behavior. With drain disabled the caller invokes neither
`40870` nor `40F60`, leaving both allocations. Rejected type 0/5 allocates no
shield in either call. Admission false still executes the selected callback
and suppresses its draw-service invocation.
[Retained traces](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/fp-suffix-trace.json)

A complementary linear instruction scan of all 19,128 positive-size original
inventoried bodies found direct `1E370` references only at `2C19A` and
`40870`; the only direct `3FDF0` calls are `2C2B3/2C2C2`, and the only direct
`40870` caller is `2C2CB`. No literal absolute function-pointer pattern for
those three targets occurs in any original section. Absolute writes of suffix
start `4B6284` occur in initialization (`13450/1E110`), world reset `40E90`,
and `3FDF0`; count writes also occur in the known allocators/group producers
and `40870`. The retained scan states its limit: it is not a universal proof
against arbitrary computed pointers/aliases or unknown external side effects.
Together with the executed populated intermediate route, it closes the stated
shield suffix question without inventing an early drain.
[Original reference scan](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/queue-direct-references.json)

```sh
.tools/python/bin/python analysis/damage-shields-goal-20261011/overlay-pass/scheduling/trace_fp_suffix.py
```

## Current native routes do not yet preserve this queue

The inspected native files are hash-pinned in the evidence snapshot, since
parent implementation work continues concurrently. `Renderer::group` groups
dynamic vertices in a `map<DrawKey,Pack>`, merging repeated runs and ordering
by `(material,lightmap,cluster,source_section,source_part)`. Its preserve-order
mode is currently used for sky runs. Dynamic world and foreground grouping
has no object-owner/cache ordinal, transparent priority/depth/plane, callback
payload or suffix start. `ModelView` has world/foreground spans, particle
banks, source view constants and other explicit render policies, but no
`SourceTransparentCommand` bank/ordinal stream.
[Renderer grouping](../src/renderer.cpp),
[ModelView](../include/halo2/renderer.hpp),
[retained Renderer snapshot](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/native-snapshot/src/renderer.cpp)

Per view, the world late route submits after-fog dynamic draws after water,
then world particles and patchy postprocess. Foreground uses its separate
source camera bank/projection, retains world depth in the gameplay atmosphere
path, and draws accumulation/base, dynamic/environment and foreground
particles. `draw_overlays` iterates material children per grouped draw;
`draw_environment` invokes these children after its environment draws. Those
are material composition helpers, not `1E370` global ordinal scheduling.
The existing source transparent sorter has no call from the inspected
`Renderer` or `multiplayer_runtime`.
[Renderer source](../src/renderer.cpp),
[existing sorter](../src/source_transparent_sort.cpp),
[retained native snapshot](../analysis/damage-shields-goal-20261011/overlay-pass/scheduling/native-snapshot/)

`multiplayer_runtime` retains original section/part identity while posing
world units, hands and weapons, then applies ordinary shader selection and
appends geometry to world/foreground spans. The new actual plasma binder,
animated constant-bank proof and local-position lowering provide material
and pose foundations, but none creates the missing source command stream.
Existing projection/depth machinery can be reused only after its source
context is explicit for each admitted command.
[Runtime](../src/multiplayer_runtime.cpp),
[material/pose findings](SHIELD_PLASMA_PASS_RESEARCH.md)

The smallest next proof-backed work is therefore: independently admit an
override part bank from original current geometry; retain owner, original
part/class, chosen shader, source center/visibility and view context; produce
the literal shared transparent commands; retain the original world/FP suffix
ranges and original sort ordinals; map command payloads to bounded native
vertex ranges without merging different owners; and execute them at the
proved caller stage with matching camera, target and inherited depth state.
Sorting only shields or drawing immediately after each model cannot establish
the shared original order. The bounded FP shield suffix lifecycle is now
resolved above; full particle-producer ordering, target/state ownership and
live renderer integration still need proof. No generic after-model or
after-foreground placement is justified by this evidence.

The subsequent [live producer research](SHIELD_LIVE_PRODUCER_RESEARCH.md)
retains original `2CBC0/133720/133AB0/133F70/2CBF0/44AC0` assembly order,
the `134240/163D50` admission-list swaps, actual retail
`174220/174420/1E4E0/173C90` particle-location callbacks, and the concrete
current scene/pose/registration handoffs needed for one shared bank. Its
67-case unchanged-instruction probe establishes the local reorder and
particle-allocation behavior; it does not establish a joined live collector.
The persistent queue owner and ordered renderer consumer are being
implemented separately by the parent agent after this report's historical
native snapshot. Their implementation does not supply the missing producer
registration history or current collector inputs by itself.
