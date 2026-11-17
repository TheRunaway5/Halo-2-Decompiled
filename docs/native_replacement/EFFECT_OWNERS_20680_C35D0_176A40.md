# Effect owners `20680`, `C35D0` and `176A40` (native 2026-11-15, rewritten 2026-11-16)

Group E of [the remaining translated census](REMAINING_TRANSLATED_20261115.md).
Three translated effect owners are native and bound. Each closure also
covers the only callees of its owner (run inline), so seven translated
routines leave the census:

| Owner | Unit | Inline callees | Role | Census before (instr/frame) |
|---|---|---|---|---|
| 20680 | lens flare | | lens query frame reset: when the lens query stamp ([4922C0]) moved on, or stayed in view modes 4..7, clear bit 1 of the flags of the 511 lens query records (32 bytes from 52DFDC), remember the stamp, and set the "reset" byte [52DFD8] and the "queries issued" byte [52DFD9]; otherwise clear [52DFD8]; [532094] = -1 | 2,065 |
| C35D0 | effect light view | C3300, C2A80 | per-view light fades: every active light ([4ECBDC], flag bit 1) gets its overall, diffuse, specular, secondary and volume distance fades (+100, +F8, +FC, +104, +108), then diffuse = min(diffuse, overall) and secondary = min(secondary, diffuse) | 500 + 536 + 1,628 |
| 176A40 | death cycle, combat action, particle frame | 252100 | particle property inputs: for every set bit of the mask, the input's value (17 inputs: particle age, emit time and randoms, emitter age and randoms, system LOD, game time, effect A/B scale, rotation, the two explosion camera projections, location random) to the output array | 1,059 + 699 |

The census figures are the 2026-11-15 Armory census
(`remaining-by-unit.json`), about 6.5k translated instructions per frame in
all.

## Native route

The 2026-11-15 closures were machine-shaped: registers, flags and the stack
were tracked in the logic. They are rewritten to the code standard of
[REMAINING_TRANSLATED_20261115.md](REMAINING_TRANSLATED_20261115.md), as the
object update owners were.

**Logic**: `src/rendering/effects/native_tu15_effect_owners_systems.hpp`
(`halo2::native_effects`). It is game logic with no registers, flags, stack
slots or coverage marks:

- `lens_query_reset::run` (20680), with `LensQueryRecord` and its flags;
- `light_fades::run` (C35D0), `fade_light` (C3300) and `distance_fade`
  (C2A80), with `LightDatum`, `LightDefinition` (the three distance classes),
  `TagInstance`, the five fades and their class distance tables. The names
  follow `EffectLightViewFades` in `effect_light.hpp`, whose native
  presentation computes the same fades;
- `particle_inputs::run` (176A40) and `input_value` (252100), with
  `Particle`, `Emitter`, `Location`, `EffectDatum`, `GameTime`, `Camera` and
  the 17 inputs. The input names follow the particle tags' property input
  list; each agrees with the field its entry reads;
- the data array layout, its in-use search and the datum handles are the
  object update systems' (`native_tu15_object_tick_systems.hpp`).

**Adapters**: `src/rendering/effects/native_tu15_effect_owners.hpp`
(`native_effect_owner_detail`). They use the shared boundary framework
(`boundary::Port`, `boundary::Memory`) and the verification machinery
(`admitted`, `execute`, `Trace`, `Replay`) in `native_tu15_object_tick.hpp`.
Group E no longer uses `Machine`. Per owner, the adapter holds:

- the access role -> instruction table;
- the register effects that a store, a child or the exit can observe. For
  example, within the light walk EBX, and C3300's and C2A80's scratch GP
  registers, are never observed, so they are not mirrored;
- the inline callees' frames: argument pushes, return addresses, register
  saves and restores, C3300's local (the secondary fade), C2A80's distance
  spill, and 252100's 1C-byte frame;
- the child calls;
- the exit registers and flags;
- the SSE and x87 residue;
- 252100's jump table, read from the bank at 252430. A target outside its
  14 entries throws the unit's `Unbound TU1.5 contact dispatch target`;
- coverage.

What stays exact:

- Every original memory access happens in program order, at the same
  address and width, including every re-read the original makes. For
  example, C3300 reads the light's radius six times, and 252100 reads the
  rotation twice.
- Every store goes through the unit's store helper.
- SSE scalar operations are the original instructions in the original
  two-address operand order. They run as inline asm with compiler barriers,
  each between the same two memory accesses as in the original. So NaN
  payloads, rounding, DAZ/FTZ and the MXCSR sticky flags match, at a faulting
  access too.
- C2A80's distance is the original x87 sequence on the host x87 under the
  live control word (FLD m32 operands; one FSTP m32 spill whose C1 the
  adapter publishes).
- C2A80 repeats the range comparison and the division before each later
  decision. The repeats have the same operands, so they give the same values
  and raise no new MXCSR flags. The logic does each once.
- Values kept in callee-saved registers across a call are handed back:
  - C3300 saves the walk's handle and index in its frame, and the walk
    continues with what it restored;
  - 176A40's pending mask, outputs, input and particle are loaded into their
    registers from the logic before 4EFA0 and read back after it, so a
    4EFA0 that does not preserve them is followed exactly.
- EFLAGS are computed where a child or the caller observes them: the 16EA60
  entry (`XOR EDX,EDX`), the 4EFA0 entry (`TEST EDX,EDX`) and the exits
  (`TEST`/`CMP`).

Children stay genuine and are entered with the exact register, flag, XMM,
x87 and stack ABI:

- C35D0's 16EA60, through the unit's body (the native integer owner);
- 252100's 4EFA0, through the direct body (combat action, particle frame) or
  the unit's dispatch (death cycle: the required owner round trip).

Admission:

- TU1.5, no active trace, observation-only stores, a literal cursor, no
  instruction callbacks, or the `HALO2_CENSUS_NATIVE_BRIDGES` diagnostic;
- no live service PC in the closure;
- C35D0: masked x87 and MXCSR exceptions, no active MMX, x87 depth at most 3
  (C2A80 needs five slots; a deeper stack keeps the translated stack-fault
  route);
- 176A40: masked MXCSR exceptions.

### Switches

- `HALO2_REFERENCE_EFFECT_OWNERS=1` restores all three translated owners;
  `HALO2_REFERENCE_EFFECT_OWNERS=20680,c35d0,176a40` (any subset) restores
  the listed ones.
- `HALO2_VERIFY_EFFECT_OWNERS=1` records every admitted native call (children
  genuine, run once) and replays the unmodified translated bodies against the
  record. Reads must match in order, address and width; stores in pc, address
  and bytes; each child call in target, position and complete entry ABI; and
  the final registers, flags, XMM, x87 and MXCSR must be identical. A summary
  prints at exit: `Effect owners verify: ...`.

### Binding

`tools/native_effect_owner_replacements.py`, called from
`spatial_owner_bindings` in `tools/specialize_native_operands.py` (registered
2026-11-16; the helper finds each pinned closure in its unit itself; receipt
key `native_spatial_owners`), installs the prefix only in the build-local
specialized copy. It is a build dependency in
`cmake/NativeOperandSpecialization.cmake`.

An owner is bound only when:

- every body of its closure matches its pin (state type normalized; 252100
  has two pins, direct and dispatched 4EFA0);
- every `selected()` operand of those bodies is the identity under TU1.5;
- each genuine child keeps its direct body, or (4EFA0) its dispatch.

The verification reference is the canonical bodies with only the genuine
child calls routed to the replay. Inline callees keep translated reference
copies.

Bound units (2026-11-16, after the rewrite and re-proof): `BOUND = ('20680',
'c35d0', '176a40')`. One bridge in each of five units: lens flare (20680),
effect light view (C35D0), and particle frame, combat action and death cycle
(176A40). The specializer's receipts list them under
`native_spatial_owners`.

## Proof

`tools/validate_native_effect_owners_tu15.py` (CTest
`native_tu15_effect_owners`, `cmake/NativeTu15EffectOwners.cmake`, included
from `CMakeLists.txt` since 2026-11-16) extracts the pinned bodies and the
generated references and compiles `tests/native_tu15_effect_owners_tests.cpp`.
The harness is unchanged by the rewrite.

The stand-ins:

- 16EA60 returns -1, valid indices, or an index whose handle is -1;
- 4EFA0 writes the vector, clobbers XMM/GP/flags and occasionally the
  callee-saved registers and MXCSR sticky bits.

Each result is a hash of the child's complete entry ABI (GP, flags, XMM, x87
depth/status, MXCSR) and the memory it sees, so any entry difference
diverges.

Fixtures:

- light arrays: bit vectors, salt words (including -1), header bits, tag
  rows, definitions with out-of-table indices, negative counts and a
  65,536-entry array whose next handle is -1;
- positions, radii, offsets and constants with NaN payloads, infinities,
  denormals and -0;
- the lens flare camera, mode and previous-index words at their boundaries;
- masks with bits beyond the table, a corrupted jump-table entry, null
  records, owner handles of -1, game-time integers, and dual-NaN projection
  terms with distinct payloads;
- every x87 precision and rounding control, x87 depths 0..4 with live
  condition bits and sticky flags, every MXCSR rounding, DAZ and FTZ mode
  (and pre-set sticky bits);
- fault injection (one fixture page removed).

Each case compares:

- ordered stores (pc, address, preimage, bytes);
- the child call log;
- the whole bank;
- GP, EFLAGS, XMM, x87 lanes and status (C1, TOP and sticky flags), MXCSR
  and the cursor.

A fault case must produce the same exception, cursor and store prefix.

The mutation screen names the file of each defect: the logic
(`native_tu15_effect_owners_systems.hpp`, 73 mutants) or the adapter
(`native_tu15_effect_owners.hpp`, 53). They include access-order defects
caught in verify mode (6), register-mirror and hand-back defects, and the x87
and SSE operand-order defects of the earlier screen. One earlier defect
class has no counterpart: swapping the operands of the volume range's
MULSS changes only a dual-NaN payload that then has to survive into the
XMM2 residue, which the fixtures almost never produce. It is replaced by
dropping the radius factor.

Results (2026-11-16, the rewrite; report
`build/test-results/effect-owners-legibility-20261116/validator.json`):

| Mode | Cases | Comparisons | Ordered stores | Children | Faults |
|---|---|---|---|---|---|
| Release | 8,000 | 7,240 (1,334 20680, 3,239 C35D0, 2,667 176A40) | 637,587 | 4,670 | 650 |
| Census | 2,000 | 1,805 | 159,485 | 1,127 | 162 |
| Verify | 2,000 | 1,785 | 160,953 | 1,144 | 166 |
| Reference switch (C35D0) | 1,000 | 501 | 57,361 | 192 | 53 |
| ASan/UBSan | 1,600 | 1,459 | 126,630 | 947 | 129 |
| ASan/UBSan verify | 400 | 354 | 34,725 | 225 | 33 |

- 59/59 reachable branches (3 declared unreachable, not reached).
- 126/126 mutants rejected.
- Binding: installs in all 5 units, idempotent; refuses 5 changed bodies, 2
  missing children and a non-identity operand.
- Source hashes rechecked after the screen.

Every equivalence figure, and the coverage bitmap of every mode, equals the
machine-shaped closures' on the same harness (run before the rewrite with
the binding checks forced): the reference bodies decide them.

## Production results

**Build** (2026-11-16 13:22, `build-perf-4k60` through `scratchpad/build_perf.sh`):
one bridge in each of the five specialized units. Binary sha256 `4bb86eb1…`;
a pinned copy (`halo2_native.E1`) was used for the runs and then deleted.

**Not run: the Armory parity and the MP verify sweep.** The launcher-selected
MCC cache `assets/imported/mcc-82114d7d2e034d48ba762109935f9ca3` (in
`gameplay-runtime-selection.json`) no longer exists. `assets/` was last
changed at 10:15 on 2026-11-16, and the trash was emptied at 11:46. All
three Armory roles stop with `cannot open .../01a_tutorial.map`.

No other cache on the machine is a substitute:

- the surviving imports pin different maps (coagulation `a94c4fda…`, against
  the selection's `2bb23227…`);
- the newest of them (`/mnt/SSD4TB/Halo-2-Decompiled/assets/imported/mcc-38f68db0…`)
  is refused by the current build: `Sky material/texture semantics differ
  from the authenticated projection input`.

Once the selected cache is back, run (receipt scripts in
`build/test-results/effect-owners-legibility-20261116/scripts/`):

- Armory parity: `python3 tools/run_test.py --scratch-root .local/test-scratch
  -- zsh build/test-results/effect-owners-legibility-20261116/scripts/armory.sh
  BINARY OUTFILE`. It runs `scratchpad/parity.sh` (300 frames, 1280x720) as
  native, reference (`HALO2_REFERENCE_EFFECT_OWNERS=1`) and verify
  (`HALO2_VERIFY_EFFECT_OWNERS=1`), and keeps each role's line and the
  `Effect owners verify:` summary. Expect frame `5627a51badf47762` and trace
  `8a9b58b38bef` in all three.
- MP sweep: `scripts/mp_sweep.sh BINARY OUTDIR verify --env
  HALO2_VERIFY_EFFECT_OWNERS=1`, then `scripts/mp_sweep.sh BINARY OUTDIR
  native`. These run group L's `scratchpad/groupL/tools/l_sweep.py` over every
  MP map at 1p and 4p with scripted input, 3 workers, one role at a time.
  Compare the frames of the two roles. The known stops and nondeterministic
  cases are those of group L's sweeps.

## Live checks (2026-11-16, evening): bound

The selected cache is now `assets/imported/mcc-7fde0eefc11d4530beed3cb109781361`,
and all the live checks have passed. `BOUND = ('20680', 'c35d0', '176a40')`
stays filled. These runs were made together with group F's five owners (see
"Groups E and F: live checks and binding" in
[REMAINING_TRANSLATED_20261115.md](REMAINING_TRANSLATED_20261115.md)).
Receipts and scripts are in
`build/test-results/effect-frame-owners-live-20261116/`.

**Build.** `scratchpad/build_perf.sh` reconfigured cleanly and re-specialized
all units, with one group E bridge in each of the five units. The linked
binary is byte-identical to the 13:22 build (sha256 `4bb86eb1…`). Every run
used the pinned copy `halo2_native.EF`, which was deleted afterwards.

**Armory parity** (300 frames, 1280x720, deterministic capture). Every run
gives frame `5627a51badf47762` and trace `8a9b58b38bef`:

- default;
- `HALO2_REFERENCE_EFFECT_OWNERS=1`;
- group F's reference switches;
- both groups' reference switches;
- both groups' verify switches;
- a default pass with Vulkan validation, which reports 0 messages.

Verify summary: `Effect owners verify: 20680 300, C35D0 600, 176A40 1312
calls; 866540 ordered accesses (307866 stores) and 870 child boundaries
identical`.

**MP and campaign sweep.** This is group L's `l_sweep.py` with scripted fire,
grenade, jump, use, crouch, move and look over 600 ticks. It covers every MP
map at 1p and 4p, plus 01a, 01b, 03a, 03b, 05a and 08b at 600 frames: 56
cases in two roles, verify (both groups' verify switches) and native.

- 0 verify differences.
- 47 cases exit 0. The other 9 are the known stops, with the same errors as
  the object-update sweep:
  - needle and street_sweeper at 1p and 4p (active_camouflage);
  - 01b (1CD130 trim owner);
  - 03a, 03b and 05a (scene-copy renderer thread);
  - 08b (B9180 backing at 0x48C33C).
- Native and verify frames are identical in 44 of the 47 cases. The three
  that differ are the known nondeterministic ascension, elongation and
  lockout at 4p.
- The same 44 frames also equal the object-update sweep's native frames.
- Effect owner verify totals: 20680 28,217, C35D0 70,230 and 176A40
  3,638,726 calls; 249,305,234 ordered accesses (79,902,193 stores) and
  563,284 child boundaries identical, over 54 cases.

**Census** (Armory, 300 frames, `HALO2_CENSUS_NATIVE_BRIDGES=1`, on the same
binary). One run has every reference switch of both groups on, the other has
them off. Both runs give the parity hashes. Every group E routine goes to 0:

| Routine | Entries | Instructions | Per frame |
|---|---|---|---|
| 20680 | 300 | 619,500 | 2,065 |
| C35D0 | 600 | 150,000 | 500 |
| C3300 | 2,400 | 160,800 | 536 |
| C2A80 | 12,000 | 488,400 | 1,628 |
| 176A40 | 1,312 | 317,572 | 1,059 |
| 252100 | 17,586 | 209,565 | 699 |
| Group E | | 1,945,837 | 6,486 |

The run total goes from 79,082,471 to 74,881,346 for groups E and F
together, which is 263,608 to 249,604 per frame.

## Remaining group E work

Still translated, from the group E claim in
[REMAINING_TRANSLATED_20261115.md](REMAINING_TRANSLATED_20261115.md). The
census was not re-run here, so there are no new figures:

- `death_cycle_runtime` and its copies: BA9E0, 17C140, 178070, C3A20, 17C370,
  17C5C0, 178110, 22B790, 251C80, 250780, 17BEB0, 30BD0, and others;
- `source_effect_particle_frame` and `source_effect_particle_draw`, apart from
  176A40;
- `source_lens_flare`: 17C710 and 20700;
- `native_light_shape`: C3AB0.

Group L's 179CD0 calls 17C370, 17C5C0 and 17C140 as genuine children. Port
new owners straight to the code standard, with the logic/adapter split used
here.
