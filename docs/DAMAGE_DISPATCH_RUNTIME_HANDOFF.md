# Shared direct damage dispatcher: runtime handoff

The next shared owner is the complete original dispatcher, with its amount producer and genuine lower call boundaries. Replacing its first call with a supplied amount, or routing it through `apply_unit_damage`, would omit original admission, hierarchy, request mutation, region and result behavior. The strongest retained runtime reference is the constructor-owned TU1.1 kill-trigger run. The falling diagnostic requests reach the dispatcher but lack the damage-tag table row required by its first lookup. No continuation, tag row, callback return or game field was supplied in this research.

## Authority and reproduced correspondence

Fresh complete bodies authenticate retail `D7B80` → TU1.1 `D9180` → owned TU1.5 `DA0D0`: **532 instructions**, equal normalized instructions and internal branch topology. The retail comparison retains its actual direct edges; the TU1.1/TU1.5 comparison retains every paired direct call and image operand. The immediate 17 callee targets are separately paired, including the repeated designator query's two callsites. Six amount/lookup owners plus fifteen other immediate callees total **21 complete bodies / 3,061 instructions**. This is static correspondence, not a new TU1.5 execution or hardware result. [Retail evidence](../analysis/damage-shields-goal-20261011/dispatcher-boundary/retail-correspondence.json), [dispatcher/amount pairing](../analysis/damage-shields-goal-20261011/dispatcher-boundary/pairing/report.json), [immediate callee pairing](../analysis/damage-shields-goal-20261011/dispatcher-boundary/direct-callees/report.json).

Reproduce with the repository's `.tools/python/bin/python`, running `analysis/damage-shields-goal-20261011/dispatcher-boundary/{pair.py,pair-direct.py,incoming.py,retain.py}` in that order. The pairing wrapper reuses the pinned complete-body matcher with an explicitly replaced entry list; its inherited report wording mentions pose requests, but the retained entries and bytes are these damage owners. Original image/cache/helper/current-source hashes and the extracted observation hashes are in the [manifest](../analysis/damage-shields-goal-20261011/dispatcher-boundary/manifest.json). No production, CMake, tests or earlier evidence were edited.

## Complete dispatcher ABI and retained frame

At original entry ESP=`S`, the six DWORD stack cells at `S+04..18` are:

| Cell | Source consumption |
| --- | --- |
| `+04` | Writable `0x88`-byte damage request pointer. |
| `+08` | Full target object datum. |
| `+0C` | Signed low16 index: bounds-checked against damage-info `+C4`; its stride16 entry at `+C8` supplies accumulator `+08`. Original target also receives it at accumulator `+10`. |
| `+10` | Signed low16 value retained at accumulator `+0C` for original target. Keep its unidentified meaning explicit. |
| `+14` | Signed low16 model entry index, NONE=`FFFF`; when present, model `+5C + index*14` supplies damage section `+06` and material `+10`. This branch has no local count check. |
| `+18` | Original target's opaque pointer/value retained at accumulator `+14`, subsequently consumed by lower region/response owners. |

`SUB ESP,F4`, the three unconditional saved registers and the conditional saved ESI put steady per-target ESP at `S−104`. The accumulator occupies steady `+34..67` (`0x34` bytes); ancestry begins at `+70`, and the result begins at `+B0`. There are **sixteen ancestry cells before the result**, with no original bounds/cycle check. The result's source footprint is **`0x54` bytes**, ending at `S−1`; `DAA90`'s last DWORD write is result `+50`. Earlier SMG research's `0x58` allocation is an oversized isolated buffer, not this source frame extent. Original code initializes the whole accumulator with 13 `STOSD`s, but does not clear the whole result or its padding. DF must follow the original forward-string convention. [Complete dispatcher instructions, `D9180..D98D5`](../analysis/damage-shields-goal-20261011/dispatcher-boundary/pairing/instructions/000d9180-old.asm), [result producer](../analysis/damage-shields-goal-20261011/dispatcher-boundary/direct-callees/instructions/000daa90-old.asm).

Normal completion uses `RET 18`, so ESP becomes `S+1C`; saved EBX/EBP/ESI/EDI are restored and EAX is `FFFFFFFF` from the exhausted loop index. This is not a damage amount or success result. GP/flags/SSE/x87 state must pass through original callees on the shared memory/stack owner. Callee frames must retain actual return addresses and mutations on a missing-owner stop. Scalar adapters do not establish this ABI. [Dispatcher epilogue `D98B0..D98D5`](../analysis/damage-shields-goal-20261011/dispatcher-boundary/pairing/instructions/000d9180-old.asm), [current full-state falling bridge](../src/biped_falling_damage.cpp).

## Operation and immediate call ownership

The mandatory first call is `DB160` before target ancestry or target damage-info lookup. Event flags `5` or `200` select just the target; otherwise object `+14` is followed until NONE and processed from outer ancestor toward the initial victim. The victim's damage-info flag bits4/6 establish child shield/body propagation permissions. Each iteration recreates the accumulator; remaining damage carries to the next iteration. Request material `+7C`, feedback vitality `+78`, reporting byte `+84`, counter sentinel `+7E`, and object saved owner `+C2/+C4/+C8` can change. Missing model damage-info skips vitality processing but still reaches result construction and delivery. [Dispatcher `D91CE..D925F`, `D9270..D951B`, `D9827..D98C5`](../analysis/damage-shields-goal-20261011/dispatcher-boundary/pairing/instructions/000d9180-old.asm).

The following are **callsite ABIs**, not replacements for the complete callee. All TU1.5 addresses are independently paired complete bodies; the paired reports retain source operands and internal branches.

| TU1.1 → TU1.5 | Actual dispatcher call inputs / result | Native boundary at research snapshot |
| --- | --- | --- |
| `DB160 → DC0B0` | EAX=request, ECX=`jpt+10`, EBX=full target; stack1=byte-output pointer; `RET4`, amount in XMM0. | Complete raw owner missing; scalar interpolation/player leaves exist. |
| `D7160 → D80A0` | ECX=target; EAX=damage-info pointer or0. | Complete TU1.1 object graph and new paired vitality module exist. |
| `15E3F0 → 15F7D0` | EAX=victim player; stack=attacker player, request `+7E` container. | Raw counter owner missing; do not infer attribution from the native post-hit hook. |
| `1E2F50 → 1E7810` | CX/SI=the two signed designators; AL predicate. | Raw query missing; campaign and selected multiplayer engine routes differ. |
| `CD460 → CE4F0` | Stack=target, request `+3C` direction pointer; AL predicate. | Raw target/animation-direction admission owner missing. |
| `DD830 → DE780` | EAX=current child, ECX=its parent, EDX=request; XMM0=seat transfer scalar. | Missing complete seat/damage-info owner. |
| `D8070 → D8FC0` | EDI=current target; no explicit stack argument. | Scalar depletion/effect-attempt adapter exists; full original effect/variant calls are not joined. |
| `D7E00 → D8D50` | Stack=target, request owner pointer `+08`, original flag-derived DWORD,0. | Complete raw death/section/child propagation owner missing. |
| `DA710 → DB660` | Stack=target,`jpt+10`,request,accumulator. | Shield arithmetic/effect attempts exist; full source bank/callees are separate. |
| `DC810 → DD760` | Stack=request,target. | Missing vehicle/rider owner; it can call `D9180` recursively. |
| `D98E0 → DA830` | Stack=request,current amount DWORD,target,accumulator. | Missing complete section/radius owner. |
| `DA2B0 → DB200` | EDI=accumulator; stack=target,`jpt+10`,request. | Body arithmetic exists; full section/material/death call graph is separate. |
| `B8250 → B9390` | EAX=target. | Existing complete animation/vehicle/weapon graphs. |
| `DD280 → DE1D0` | Stack=**initial** target, DWORD at accumulator`+2C`, overlapping DWORD at`+2E`. Preserve widths and flag-adjacent bytes. | Complete physics/region aftermath owner missing. |
| `DAA90 → DB9E0` | EAX=result, ESI=request, EDI=accumulator; stack=current target. | Complete raw result owner missing; always retain its type callback. |
| `DAC40 → DBB90` | EDI=result; stack=current target. | Recent-damage leaf exists; complete original consumer/reaction/impulse/credit graph missing. |
| `B9430 → BA570` | Stack=current target. | Existing complete death-startup graph; real destruction services still required. |

Authority: [all actual dispatcher callsites](../analysis/damage-shields-goal-20261011/dispatcher-boundary/pairing/report.json), [all immediate complete bodies](../analysis/damage-shields-goal-20261011/dispatcher-boundary/direct-callees/report.json), [exact native function-definition snapshot](../analysis/damage-shields-goal-20261011/dispatcher-boundary/native-entry-snapshot.json), [native adapters](../src/unit_damage.cpp), [recent-damage leaf](../src/damage_feedback.cpp).

`D8070` calls `1781F0` with authored damage-info `+B0`, default-owner pointer `*46E8C0`, target in EDX and three zero arguments, then clears object`+F4`, sets depleted bit3 and calls `BB6E0(target,−1,2,−1)`. No result is inferred for these external calls. `DAA90` copies the request owner as **three DWORDs**, preserving its container padding, builds normalized damage fields and calls `CB1D0` for unit types0/1 or `11A800` for type12. `DAC40` includes the recent-damage stores, typed impulses, credit/history and final reaction callbacks; those require their real owners. [Depletion body](../analysis/damage-shields-goal-20261011/dispatcher-boundary/direct-callees/instructions/000d8070-old.asm), [result body](../analysis/damage-shields-goal-20261011/dispatcher-boundary/direct-callees/instructions/000daa90-old.asm), [delivery body](../analysis/damage-shields-goal-20261011/dispatcher-boundary/direct-callees/instructions/000dac40-old.asm).

## First missing owner: amount production

`DB160` consumes `jpt+10` fields `+0C/+10/+14` (tag `+1C/+20/+24`) and request fraction`+54`, event scale`+5C`. Equal ordered upper bounds skip RNG. Unequal or unordered bounds execute original x87 sampling through `*4EE9D8`: LCG`19660D/3C6EF35F`, high16 sample, literal scale`4650C0`, then lower-plus-difference interpolation. It uses ordered SSE interpolation and event scaling afterward. Preserve x87 stack/control/status and SSE exceptions/rounding rather than replacing the random branch with a C++ expression. Equal authored bounds do not prove that branch or its RNG state provider. [Complete `DB160`, `DB160..DB1E9`](../analysis/damage-shields-goal-20261011/dispatcher-boundary/pairing/instructions/000db160-old.asm).

The remaining exact state dependencies are:

- Request source object `+0C` → complete `BBCB0(mask3)`; possible controlling-unit `+24C`, actor`+12C` and actor-pool stride`888`. `1CBE30` conditionally multiplies the amount through actor`+7C0` and request bit3. Campaign options `*4EDF18+08`, victim type`+AA`, victim`+134` high bit and source designator`+138` can suppress the amount. [Amount branch](../analysis/damage-shields-goal-20261011/dispatcher-boundary/pairing/instructions/000db160-old.asm), [actor leaf](../analysis/damage-shields-goal-20261011/dispatcher-boundary/pairing/instructions/001cbe30-old.asm).
- Selected engine is read through `*4F10B8+C14` and `566084[index]`. With an engine, `D90E0` resolves target player through validated headers/type0/1 or parent traversal, then `15F520` consumes attacker/player/variant/trait fields. An exact zero multiplier clears **one byte** at request`+68`; it does not replace the full DWORD. Existing `source_player_damage_scale` covers its separately verified semantic consumer and native Match fields; it does not supply complete target resolution, raw guest banks or arbitrary volatile-register results. [Target resolver](../analysis/damage-shields-goal-20261011/dispatcher-boundary/pairing/instructions/000d90e0-old.asm), [original player scale](../analysis/damage-shields-goal-20261011/dispatcher-boundary/pairing/instructions/0015f520-old.asm), [native binding](MP_PLAYER_DAMAGE_NATIVE_BINDING.md), [native implementation](../src/source_player_damage.cpp).
- Without an engine, campaign designator range checks and relationship bits use `12C740`; the admitted path invokes `1ED090(0)`, consumes XMM0 with the live XMM1 amount and writes the output byte1. Other paths leave the byte untouched; `D9180` initializes it0 before the call. These lower calls and their exact preserved floating-point registers cannot be replaced by a quiet campaign scale. [Amount epilogue `DB2DE..DB356`](../analysis/damage-shields-goal-20261011/dispatcher-boundary/pairing/instructions/000db160-old.asm).

The narrow useful implementation increment is complete paired `D9180 + DB160 + D90E0 + 1CBE30`, linked to existing real `BBCB0/D7160` owners and explicit required-owner boundaries for the remaining calls. It can retain legitimate prefixes and pending source frames before the full effect/section/result graph is joined. It cannot by itself enable live falling or claim all direct damage complete. [Current paired vitality interface](../include/halo2/source_vitality_runtime.hpp), [current object graph routing](../src/unit_object_runtime.cpp), [current falling bridge](../src/biped_falling_damage.cpp).

Minimum selected-image state references are independently authenticated as follows. Mutable pointer cells contain zero in the executable; those literal bytes are **not initialized game state**. The RNG scale's exact binary32 word is `37800080`, retained as raw little-endian bytes`80008037`, rather than an inferred reciprocal. [Paired image operands](../analysis/damage-shields-goal-20261011/dispatcher-boundary/pairing/report.json).

| Owning input | TU1.1 address → TU1.5 address |
| --- | --- |
| Current tag table pointer | `4EB114 → 4F0414` |
| Shared object header pool pointer | `4E78D0 → 4ECBD0` |
| Options pointer | `4EDF18 → 4F3218` |
| Shared random-state pointer | `4EE9D8 → 4F3CD8` |
| Engine globals pointer / engine implementation array | `4F10B8 → 4F63B8` / `566084 → 56DFA0` |
| Player pool pointer | `4F01F4 → 4F54F4` |
| Campaign relationship / actor pool pointers | `4FCDC4 → 5020C4` / `4FCDC8 → 5020C8` |
| One / zero / epsilon literals | `465030 → 46A0D0` / `465048 → 46A0E8` / `46504C → 46A0EC` |
| Random sampling scale / unsigned-conversion fallback literals | `4650C0 → 46A160` / `465028 → 46A0C8` |

## Existing actual frames and explicit limits

The [extracted kill-trigger evidence](../analysis/damage-shields-goal-20261011/dispatcher-boundary/kill-trigger-dispatcher.json) pins the unchanged earlier report and retains call401's constructor-owned banks, full damage request, call402's amount subtree reads/stores/frames, dispatcher-owned reads/stores, the exact initialized accumulator before `DAA90`, and the generated84-byte `DAC40` result packet. Actual inputs are request`021FCF78`, target`E26F0000`, damage tag`EAE90964` (`globals\\distance`), request flags4 and NONE signed indices. `DB160` receives ECX`80339530`, returns normally through real `BBCB0/D90E0/15F520`, and the dispatcher then reaches depletion, body, result and delivery owners. The selected player-scale call has attacker=victim=`EC700000`, so it takes the original identity early return; this case does not exercise normal enemy traits or randomized damage.

That experiment starts from a declared authored-volume interior point delivered through an original position owner. It preserves constructor/physics/control/animation setup and records its prior OS backing, optional-effect, sound/audio and firing-feedback boundaries. The complete `D9180/DAC40` calls have **no recorded return**: the experiment eventually stops inside feedback resource code at `20EFC2`, reading `00000024`. Source feedback/death writes before that stop are observations, not a completed aftermath. The legacy call recorder did not capture full flags/SSE/x87 state; absent CPU fields remain absent in this extraction. [Original immutable report](../analysis/multiplayer/integration/spartan-kill-trigger-20261010/probe/report.json), [retained limits and terminal frame](../analysis/damage-shields-goal-20261011/dispatcher-boundary/kill-trigger-dispatcher.json).

The [falling extraction](../analysis/damage-shields-goal-20261011/dispatcher-boundary/falling-pending.json) retains20 TU1.1 and20 TU1.5 pending dispatcher frames with the actual complete measured CPU ABI and source-built requests. Their diagnostic tag bank lacks the row read by `D91B1` for `EAE90964`. This is not permission to repair it with a fabricated pointer. A real cache image/binding service already exists, but joining its literal table to current fully constructed source objects remains a distinct owner; the diagnostic object's relocated tag identity cannot simply be treated as that live cache binding. [Original falling experiment](BIPED_FALLING_DAMAGE_RUNTIME.md), [native cache tag delivery](../src/source_runtime_cache_tags.cpp).

An independent raw direct-call scan retains20 incoming candidate edges in each image. It authenticates caller membership where the original reachable function body is recovered and explicitly leaves missing navigation boundaries unresolved. Concrete original producers include the falling call, kill-volume postphysics call, projectile impact and vehicle/rider transfer; other retained callers remain identified by their actual addresses. This is not a live producer census or proof of indirect callers. The vehicle owner calls the dispatcher recursively, so a shared implementation must support nested original frames rather than a singleton scratch result. [Incoming edge bytes and caller contexts](../analysis/damage-shields-goal-20261011/dispatcher-boundary/incoming-calls.json), [complete `DC810`](../analysis/damage-shields-goal-20261011/dispatcher-boundary/direct-callees/instructions/000dc810-old.asm).

## Production handoff gates

`Simulation::damage` currently supplies admission and amount, calls scalar shield/body processing, reports recent damage, changes native death state and invokes a post-hit hook. Its direct and area routes use native collision/candidate inputs. Complete source delivery needs the caller's one persistent header/body/tag/clock/options/player/actor/physical bank and the actual request/hit-region producers, with every reached effect, section, physics, type callback and credit/history owner retained. Storage allocation alone proves none of those fields. [Native path](../src/simulation.cpp), [adapter scope](../include/halo2/unit_damage.hpp), [persistent body storage contract](../include/halo2/source_object_body_storage.hpp).

The next evidence gates are: compare unchanged complete dispatcher prefixes/full returns using real input banks; preserve full pending ABI at each missing owner; join the genuine amount/player/campaign state providers; join full vitality/region/effect/result/aftermath owners; then connect source-authoritative producer requests and gameplay state. Keep live source postphysics/support motion, impact/collision admission, distance/kill-volume admission, and visual effect attachment as separately tested dependencies. Current scalar/original observation evidence establishes useful foundations without satisfying the active full damage/health/shield goal. [Goal scope](DAMAGE_SHIELD_COVERAGE_GOAL.md), [falling runtime limits](BIPED_FALLING_DAMAGE_RUNTIME.md).
