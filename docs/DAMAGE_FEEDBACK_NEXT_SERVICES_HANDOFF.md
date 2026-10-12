# Next reached damage-feedback services

Research snapshot: 2026-10-12. All four requested TU1.1 owners have unique complete TU1.5 counterparts: **495 instructions**, including every retained branch and original instruction byte. Existing authenticated reports were searched first; these four previously appeared only as incoming CALL targets. The [new complete-body report](../analysis/damage-shields-goal-20261011/feedback-next-services/paired/report.json), SHA-256 `1e87fea980174b232db5816523585cf37d80a626ab707dd4dd5fdea6fcd8e922`, preserves both listings, exact direct-call sites and global operands. None has a jump table. The [ABI/state audit](../analysis/damage-shields-goal-20261011/feedback-next-services/abi-state-audit.json), SHA-256 `6f20f48c4839075d5232c2d6b8b095d2e4af945cd1b23ff63b5d8c4ee8523b92`, re-reads all listed bytes from the unchanged executables and verifies the four actual paired incoming CALL edges. No production, CMake, tests or generator were edited by this research.

| TU1.1 → TU1.5 | Instructions | Exact incoming ABI and return |
| --- | ---: | --- |
| `DFC50 → E0B70` | 247 | Stack `{unit, vec3*}`, `RET 8`; no semantic return asserted. Actual `DAC40:DB067→DBFB7`. |
| `A8ED0 → B2320` | 170 | Hidden **ESI=damage report**, stack `{unit}`, `RET 4`. Actual `DAC40:DB123→DC073` moves the report into ESI just before CALL. |
| `CA320 → CB4A0` | 38 | **ECX=unit**, plain `RET`, EAX=parent seat-definition address or zero. Actual `111700:1117F2→112712`. No callees. |
| `115C60 → 116B80` | 40 | **EAX=unit, signed DX=index, EDI=valid writable output-DWORD address**, plain `RET`, AL=`!20EF90(...)` for a found datum, otherwise zero. Actual `115CE0:115D2C→116C4C`. |

## State and cold branches that must remain literal

`DFC50` resolves object pool `4E78D0→4ECBD0` and tag table `4EB114→4F0414`; object definition `+BC` bit 20 returns early. It copies the input vector and tests its squared length against the original binary32 `3FFAE147` (`1.959999918937683`). An object with `+10A` bit 2 clear scales the copied vector by `0.5`. It ORs bit 1 into the WORD at object `+348`. The optional angular branch uses definition `+264` bit 3 and state byte `+3DC`, normalizes the cross product through `30BD0`, advances the **current mutable seed behind `4EE9D8→4F3CD8`**, and preserves the x87 length/random-factor sequence. Depending on state and vector magnitude it calls the wake/state/impulse/orientation owners below. State `+3DC==3` also clears object bytes `+3F8/+3F9` before `1F0D30` and `E2F00`. Parent `+14`, player/owner `+13C`, and state byte `+34B` gate the final horizontal-vector branch. The definition early exit does not exercise these lower services. [Complete DFC50 instructions](../analysis/damage-shields-goal-20261011/feedback-next-services/paired/instructions/000dfc50-old.asm).

Two actually dereferenced pointer constants were checked beyond the immediate global operand: TU1.1 `46FC44→447144` / TU1.5 `474D04→44ABEC` point to the same zero vec3; TU1.1 `46FC50→447198` / TU1.5 `474D10→44AC40` point to the same `(0,0,1)` vec3. Their complete 12-byte pointees match. The [separate pointer evidence](../analysis/damage-shields-goal-20261011/feedback-next-services/pointer-constants.json), SHA-256 `20a38f9cc3f19ee65382a87fc66b011fc9dcbf2278e9b946cb14149e9ba8dc07`, retains both slot and pointee raw reads, individual hashes, original executable identities and the paired instruction references. Actual TU1.5 physical-bank execution dynamically reads its own pointer values through those mapped slots; a static pointee-address-map entry is not required merely because these data exist. The mutable RNG pointer is not such an immutable supplied seed.

`A8ED0` reads the **signed session byte at `*[4EDF18]+0C`** (`4F3218` in TU1.5). Its literal comparisons admit **only mode 5**. Mode 4 and ordinary cold modes return before reading the hidden ESI report, object or tag banks. Passing those branches does not validate the report/event path. In mode 5, header type bits 0/1 or 11 gate the object; unit types obtain object `+13C`. The admission checks use report damage-definition `+40 × object-definition +14`, recent report amounts `+44/+48`, alive state `object+10A` bit 2, and responsible-player identity at report `+0C`. `A66E0` must then return a non-NONE current simulation entity; `BBCB0(report+10, -1)` validates the second owner. [Complete A8ED0 instructions, `A8ED0..A9008`](../analysis/damage-shields-goal-20261011/feedback-next-services/paired/instructions/000a8ed0-old.asm).

The mode-5 path constructs two owner DWORDs `{unit, valid report+10 or FFFFFFFF}` and a zero-initialized **56-byte** payload, then calls `B6960` with EAX=`FFFFFFFF`, stack `{0, 2, owners*, 0x38, payload*, *[4D62F4]}`. The payload fields below follow actual stack addresses; the input report is not normalized in place. Clamps mean the original ordered SSE branches, including their NaN behavior.

| Payload offset | Source/value |
| --- | --- |
| `00` | report `+08` DWORD |
| `08` | report `+0C` WORD |
| `0A` | byte indicating `30BD0`'s returned length-or-zero is above the original epsilon |
| `0C..14` | copied/conditionally normalized vec3 from report `+18..20`, through `30BD0` |
| `18`, `1C` | report `+34`, `+38`, clamped to `[0,2]` |
| `20` | report `+04` DWORD |
| `24` | report `+48`, clamped to `[0,3]` |
| `28` | report `+44`, clamped to `[0,9]` |
| `2C`, `2E` | report `+3C`, `+40` WORDs |
| `30` | report `+50` DWORD |
| `34` | report `+00` BYTE |

Other payload bytes remain zero from the original fourteen-DWORD clear. The current session/network owners `4D6D44` and callback/global `4D62F4` are not manufactured by this body. [A8ED0 packing and dispatch, `A9008..A9130`](../analysis/damage-shields-goal-20261011/feedback-next-services/paired/instructions/000a8ed0-old.asm).

`CA320` reads the unit's parent handle `+14` and signed seat WORD `+1FC`. Parent NONE or seat `-1` returns zero. A parent's type byte `+AA` must produce mask bits 0/1; success returns `parent_definition[+1CC] + signed_seat*0xB0`. The routine does **not** validate a full salted handle or clamp the seat against an authored count; adding either would change the recovered behavior. It does not allocate or mutate a seat. [Complete CA320 instructions](../analysis/damage-shields-goal-20261011/feedback-next-services/paired/instructions/000ca320-old.asm).

`115C60` resolves a signed component offset at object `+342`, reads that component's first DWORD as a tag datum, validates signed DX against the tag's count `+0C`, and selects `tag[+10] + index*16 +0C`. Found non-NONE datums pass to `20EF90` in EAX; the local selected datum remains the output even if `20EF90` reports busy. **The final gate is `*[EDI] != 0`, not `EDI != nullptr`.** EDI must always be readable; zero suppresses the store, while a nonzero initial value permits writing the selected datum or `FFFFFFFF`. The actual `115CE0` caller passes a writable local initialized to `FFFFFFFF`. A NONE component tag or invalid index avoids `20EF90`; those cold branches do not validate its animation/resource state. [Complete 115C60 instructions](../analysis/damage-shields-goal-20261011/feedback-next-services/paired/instructions/00115c60-old.asm), [actual caller context](../analysis/damage-shields-goal-20261011/feedback-next-services/abi-state-audit.json).

## Lower owners and remaining limits

Three immediate lower bodies already have authenticated complete pairs and were reused instead of regenerated. Their retained listings were rechecked against both original images: `30BD0→30BD0` (38 instructions, EAX=writable vec3, XMM0=length-or-zero), `BACC0→BBE00` (76, EAX=unit/ESI=writable point), and `BBCB0→BCDF0` (35, EAX=full object datum/stack type mask). [Reuse audit with report/listing hashes](../analysis/damage-shields-goal-20261011/feedback-next-services/abi-state-audit.json).

The following targets are established by the paired CALL instructions, **not complete lower-body proofs in this handoff**. Their current banks and reached behavior remain required when the composed path reaches them.

| Calling owner | Lower TU1.1 → TU1.5 | ABI at this actual CALL site |
| --- | --- | --- |
| `DFC50` | `E6E10→E7D30` | ECX=unit |
| `DFC50` | `1F0D30→1F5620` | ECX=object`+3EC`, stack `{unit}` |
| `DFC50` | `E2F00→E3E20` | EAX=unit, stack `{1}`; raw pushed DWORD `00000001`, distinct from float `1.0` |
| `DFC50` | `B8770→B98B0` | Stack `{unit, FFFFFFFF, point*, admitted_vector* or 0, angular_vector* or 0}` |
| `DFC50` | `B8490→B95D0` | EAX=0, ECX=normalized horizontal-vector local, EDX=actual up-vector pointer; stack `{unit,0,0}` |
| `A8ED0` | `A66E0→AFB30` | ECX=unit, requires current simulation entity/world bank |
| `A8ED0` | `B6960→B8050` | EAX=FFFFFFFF and the six stack arguments above; actual session/event owner |
| `115C60` | `20EF90→213890` | EAX=selected datum, AL consumed as busy/status |

The parent reports these four new bodies are now compiled and routed through the shared native owner. Its current executed-instruction coverage is below; these measurements were supplied by the parent's unchanged-executable/native comparison follow-up, not rerun by this research agent. The raw corpus covers both original images, while the deeper actual native composition is TU1.1 only.

| Body | Raw both-image corpus | Actual TU1.1 composition |
| --- | ---: | ---: |
| `A8ED0` | `7/170` (mode-0 early return) | `7/170` |
| `DFC50` | `0/247` | `108/247` |
| `CA320` | `0/38` | `38/38` |
| `115C60` | `30/40` | `30/40` |

These are **executed distinct instruction counts**, not branch/path completeness. In particular, the full both-image body pair does not imply that either raw execution exercised the DFC50 vector/RNG/physics path, CA320 parent-seat path, A8ED0 mode-5 event path, or 115C60's successful `20EF90` path. The parent retains all pending frames. Its next composed first entries are `179B90`, `B8770`, `329168`, `E6E10`, `E86E0`, and `1C9D80`; this research does not replace them with supplied results. `329168→32CCD8` already has a separate complete proof via actual incoming paired edges in the [preserved conversion-v2 report](../analysis/damage-shields-goal-20261011/delivery-next-boundary/conversion-v2/report.json); the duplicate body at `37D0B4` remains excluded. A complete static pair proves the bytes/control flow of these owners, not the missing mutable session, physics, animation, RNG, contact, report or render state, and not a live full damage/shield/render completion.
