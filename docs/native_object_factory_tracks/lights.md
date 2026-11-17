# Track `lights` (the light tick)

The per-tick update of every live light: the frame's `call_actual(0xc3430u)`
in `src/physics/physics_live_world_tu15.cpp` (original 13AB4E).

- Source: `src/gameplay/objects/native_object_factory_lights_tu15.cpp`
- API: `include/halo2/native_object_factory_lights_tu15.hpp`
- Kernels: `tests/native_object_factory_lights_kernels.cpp`
- Cases: `tools/native_object_factory_cases_lights.py`

The track owns C3430 and C5290. It composes other tracks' natives:

| Owner | Native | Track |
|---|---|---|
| C2CB0 | `light_update` | attachments |
| C4D30 (with C2870, C39B0, C48E0, 16D740, ...) | `light_reconnect` | placement |
| 1D30F0 | `native_tu15_remove_spatial_chain` | shared native |
| 16EA60 / 16E8A0 | `native_tu15_data_array_next` / `native_tu15_data_array_delete` | shared natives |

The code follows the native code standard
([REMAINING_TRANSLATED_20261115.md](../native_replacement/REMAINING_TRANSLATED_20261115.md#code-standard-user-2026-11-15)):
named views (`DataArray`, `GameTime`, `Light`) replace raw offsets, and the
live world's adapter (`light_tick`) holds the call's FP controls, the MCC
object-function namespace and the switches.

## Owners

| Owner | Native | What |
|---|---|---|
| C3430 | `lights_update` | For every live light of [0x4ECBDC], in index order: C2CB0 recomputes its intensity and colours; a light linked into the light partition (+2 bit 3) whose owner (the +4C marker owner, else the +58 parent) moved this tick (object header +2 bit 2) is unlinked when lit (+2 bit 1: 1D30F0 over 0x4ECBE0, then +2 bit 3 cleared) and reconnected (C4D30); a timed light (+2 bit 4) is deleted (C5290) once its age in ticks (tick at entry minus +14, CVTSI2SS) exceeds ticks per second (int16 [0x51DF54] +2, CVTSI2SS) times the definition's +B0 lifetime (MULSS, COMISS). The definition is read before C2CB0. |
| C5290 | `light_delete` | A linked and lit light is unlinked (1D30F0, +2 bit 3 cleared). With the second argument set and a +58 parent, the parent's body +4 bit 6 is set when another live light (a different datum) names the same parent, else cleared (a dword read-modify-write after the scan). The datum is freed (16E8A0 on the light data array). |

## Proof

Physical-original cases with `HALO2_ORACLE_STRICT_WRITES=1` (seed 0x5C41),
`HALO2_FACTORY_TRACKS=lights`, the all-tracks harness:

| Kernel | Entry | World | Cases |
|---|---|---|---|
| `light_delete` | C5290 | placement's light world (pool, light partition links, shared and unique parents) | 600 |
| `lights_update` | C3430 | placement's light world: lit, linked and timed lights, owners moved or not, C2CB0 on literal function names and definitions without functions, [[0x4EABC4]] mostly set (C2CB0 reaches C4D30) | 600 |
| `lights_update_functions` | C3430 | attachments' light-update world (function-driven intensities, fades, owners; never linked) | 600 |

Results (final fixtures): `PASS 1800 physical original evaluator tails,
3600 native comparisons, 290642 ordered original stores`. Coverage
(executed original instructions): C3430 132/134, C5290 79/79; through the
tick, C2CB0 405/407, C4D30 251/262 and 16D740 602/627 (placement's own
kernels cover the rest). C355B/C355D are unreachable: they handle a NONE
datum inside the loop, which the loop condition already excludes.

The fixtures (lifetimes around ticks per second times the definition's
lifetime, denormal and signed-zero lifetimes with age-0 marker lights,
parents with and without body +4 bit 6, a pool row size other than 0x110,
moved and still owners, linked, lit and timed state mixes) were grown by the
mutation screen below.

ASan/UBSan (`-DHALO2_SANITIZERS=ON` harness, `halt_on_error=1`, the final
sources: these kernels with placement's light and fit kernels and
attachments' kernels, C3910 and 13E6E0 included): `PASS 7400 physical
original evaluator tails, 14800 native comparisons, 374557 ordered original
stores`, no report.

## Mutation screen

Out of tree (each mutant a scratch copy compiled with the kept harness
flags and linked into a scratch harness; the in-tree SHA-256 checked after
each round). 15 mutants over C3430 and C5290; 12 rejected in round 1 at
the kernels' counts. Two survivors were rejected after the fixtures gained
parents whose body +4 bit 6 is set and a pool row size other than 0x110.
The last is equivalent: the expiry's COMISS without DAZ, because under DAZ
the lifetime (integer ticks per second times a float the MULSS already
zeroes when denormal) is never denormal.

## Production

`PhysicsLiveWorldTu15::Impl::light_tick` replaces `call_actual(0xc3430u)`:

- `HALO2_REFERENCE_LIGHT_UPDATE_TU15=1` keeps the translated C3430.
- `HALO2_LIGHT_UPDATE_DIFFERENTIAL_TU15=1` runs the native under a write
  journal, rolls it back, runs the translated C3430 and compares every byte
  either wrote outside C3430's dead stack. The original's effects are kept.
  `[light-update ...]` lines count refusals and differences.
- Otherwise the native runs (the default since 2026-11-16;
  `HALO2_NATIVE_LIGHT_UPDATE_TU15=1` is accepted for compatibility). A
  refusal in the native mode is a hard error.

The native call opens the cache's object-function namespace
(`ObjectFunctionNamespace`, as the node rebuild binding does): C2CB0's BCB70
and 10DE40 callbacks compare MCC StringIDs. A refusal is caught inside the
call's execution scope, so the differential's refusals do not mark the
world's continuation faulted (the first Coagulation run stopped two ticks
after a refusal before this).

### Differential sweeps (scripted input)

Every MP map of the selected cache at 1 and 4 players (600 ticks, the node
rebuild sweep's scripted input: fire, grenades, jumps, crouch, use, turning
and moving) and campaign 01a, 01b, 03a, 03b, 05a and 08b (600 frames of the
deterministic `--campaign-reference-capture` recipe), each with
`HALO2_LIGHT_UPDATE_DIFFERENTIAL_TU15=1`:

| Sweep | Binary | Result |
|---|---|---|
| 1 | C3910 native, 13E6E0 refused | 56 runs, 28,474 C3430 calls compared, 0 differed, 99 refused: 13E6E0 on Highplains 4p (52) and Waterworks 4p (47). The very first Coagulation 4p run had refused C3910 at tick 19. |
| 2 | 13E6E0 native too | **56 runs, 28,573 calls compared, 0 differed, 0 refused.** |

The runs that stop early do so for the known reasons outside the light
update, after every compared call matched: Needle and Street Sweeper
(`active_camouflage` shader inputs), 01b (the 1CD130 trim owner, before the
first light update), 03a, 03b and 05a (scene-copy resources on another
renderer thread), 08b (B9180's runtime backing, before the first light
update). The first campaign room (01a) compares 900 calls. The binary was
rebuilt with the native default near the end of sweep 2; the differential
mode is selected by its variable in both builds and the native code is the
same.

### Default and parity

The native is the default (`light_update_mode`). Evidence:

- Armory frame parity (deterministic `--campaign-reference-capture`,
  Original, 300 frames, 1280x720, Vulkan validation on): the translated
  route, the native (opt-in) and the default binary twice all give frame
  `5627a51badf47762` and trace `8a9b58b38bef`, no validation messages.
- MP frame parity (`tools/perf_4k60/parity.py`, 3840x2160, the translated
  route as the reference): Coagulation's default cases (native opt-in)
  PASSED with hashes 9c14a330..., 13fa114e..., 05f31264... (the node
  rebuild receipts' references); Highplains (13E6E0) with `--motion`, cases
  4:300:10:200 and 1:300:10:200, PASSED (dbb99fa6..., a9bb4be7...);
  the default binary on Coagulation's default cases PASSED with the same
  hashes.
- Census: one binary (`ff04359c...`), `HALO2_CENSUS_NATIVE_BRIDGES=1`, the
  translated route (`HALO2_REFERENCE_LIGHT_UPDATE_TU15=1`) against the
  default. Coagulation 4p, 450 ticks of the sweep's scripted input:
  279,434,088 translated instructions -> 264,596,131 (-14.84M, -5.3%); C3430
  450 entries / 1.21M instructions -> 0; C2CB0 21,770 / 5.28M -> 64 / 15k
  (light attachment births, C2260); C2B30 21,770 -> 64; 134870 43,108 ->
  112; BEC50 44,112 / 2.02M -> 16,927 / 0.79M; C9070 20,803 -> 11,200; BC8F0
  8,367 -> 4,903; C3910 3,107 -> 532; C4D30 25,430 -> 25,173 and 16D740 8 ->
  8 (their other callers, BA630 and C0F60). Armory 01a, 300 frames:
  80,887,608 -> 79,701,781 (-1.19M, -1.5%); C3430 450 / 165k -> 0, C2CB0
  2,706 / 528k -> 6 / 1.2k. BCB70 and 10DE40 have no translated entries in
  either role on this binary.

Receipts (small, kept): `build/test-results/light-update-20261116/`: both
sweeps' per-run JSON and `[light-update]` lines, the parity logs and JSON,
the census comparisons, the mutation logs and lists, the oracle reports
(`r-lights6.json`, `r-color.json`) and the scripts that produced them.

## Other translated callers (2026-11-16, continuation)

The light natives are bound where translated code still called the
original owners (`PhysicsLiveWorldTu15::Impl::light_caller`):

| Site | Caller | Callee | Native |
|---|---|---|---|
| C100F | C0F60 (object tree detach/reconnect: an attached light) | C4D30 (with 16D740) | placement's `light_reconnect` |
| C3810 | C3750 (owned lights; C0F60's C1034, BA630) | C4D30 | `light_reconnect` |
| C2135 | C20D0 (every light without a marker owner or parent) | C4D30 | `light_reconnect` |
| C2353 | C2260 (light attachment creation, C0CD0's C0DE9) | C2CB0 | attachments' `light_update` |
| C0756 | C0700 (the render's attachment scale, 41970's 41A31) | BCB70 (with BC8F0, 10DE40) | attachments' `object_function_value` |
| C09EC, C0A57 | C0910 (the node lights' weights and scales; continuation) | BCB70 | attachments' `object_function_value` |

C100F, C3810, C2135 and C2353 were direct calls inside the live support
unit: `tools/regenerate_light_caller_boundaries_tu15.py` re-emits them as
required boundaries (it checks each CALL against the authenticated XBE
and changes only those four emissions), and
`tools/generate_physics_live_support_tu15.py` lists them with its other
required boundaries, so a full regeneration emits the same calls. C0700
lives in the shadow frame unit, whose call to BCB70 already leaves the
unit through `dispatch` and the World's `required`.

The adapter reads the stack arguments (C4D30 and C2CB0: the light, RET 4;
BCB70: object, name and C0700's local value, RET 0xC), takes the call's
FP controls and opens the cache's object-function namespace (the only
MCC join in the three closures: C4D30's has none, C2CB0's and BCB70's are
the object-function names of BCB70, BEC50, C9070, E0900, F9270 and 1046C0).
The callers read no register the originals return: each next instruction
writes EAX, ECX, EDX and the flags first; C0700 reloads its local.

Switches: `HALO2_REFERENCE_LIGHT_CALLERS_TU15=1` keeps the translated
callees (the call continues on its ordinary route),
`HALO2_LIGHT_CALLERS_DIFFERENTIAL_TU15=1` runs the native under a write
journal, rolls it back, runs the translated callee and compares every byte
either wrote outside the callee's dead stack (below the return ESP: the
callee's locals and its popped arguments, where BCB70's 10FB70 stores its
slot), and the post-call ESP
(`[light-callers ...]` lines, per-site counts),
`HALO2_NATIVE_LIGHT_CALLERS_TU15=1` runs the native.

The natives the callers reach are the oracle-proven owners of placement
and attachments (their kernels: `light_reconnect`, `object_reconnect_lights`,
`collision_sphere_fit`, `light_update`, `object_function_value`,
`object_function_evaluate`, `object_type_function_value`). On the
continuation's sources (the overrun plane stack, C3AB0's type 1, BEC50's
callees): the lights kernels `PASS 1800 ..., 297687 ordered original
stores` (C3430 132/134, C5290 79/79, C2CB0 405/407, C4D30 249/262);
ASan/UBSan over the placement, lights and attachments kernels touched
(15 kernels): `PASS 10800 physical original evaluator tails, 21600 native
comparisons, 479531 ordered original stores`, no report.

### Differential sweeps of the callers

The light update's sweep (every MP map at 1 and 4 players, 600 ticks of
scripted input; campaign 01a, 01b, 03a, 03b, 05a and 08b, 600 frames)
with `HALO2_LIGHT_CALLERS_DIFFERENTIAL_TU15=1` and
`HALO2_LIGHT_UPDATE_DIFFERENTIAL_TU15=1`:

| Sweep | Binary | Result |
|---|---|---|
| 1 | dead stack below the entry ESP only | 56 runs; light callers 1,209,362 calls compared (C100F 1,094,354, C0756 112,342, C2353 2,387, C3810 266, C2135 13), 14,832 differed, 0 refused; light update 28,573 compared, 0 differed, 0 refused. Every difference was C0756 -> BCB70 writing one dword at the call's entry ESP + 12: BCB70 passes its own value argument's slot to 10FB70 as the override slot (BCBB7..BCBBE), which the original fills and RET 0xC pops. The adapter keeps it local, as the oracle does for every stack output |
| 2 | the dead stack below the return ESP | **56 runs; light callers 1,209,362 calls compared (the same per-site counts), 0 differed, 0 refused; light update 28,573 compared, 0 differed, 0 refused.** |

Runs that stop early do so for the known reasons outside these owners,
after every compared call matched (Needle and Street Sweeper, 01b, 03a,
03b, 05a, 08b; see the light update's sweeps). The binary (`641513...`)
was rebuilt with the native default near the end of sweep 2 (`e465b2...`);
the differential is selected by its variable in both builds and the
native code is the same. The light update's differential ran in the same
runs on the item-1 natives (the overrun plane stack, the orthogonal
frustum, BEC50's callees) without a difference.

### Default, parity and census of the callers

The native is the default at the five sites (`light_caller_mode`;
`HALO2_NATIVE_LIGHT_CALLERS_TU15=1` is accepted for compatibility).

- Armory frame parity (deterministic `--campaign-reference-capture`,
  Original, 300 frames, 1280x720, Vulkan validation on): the translated
  callers (`HALO2_REFERENCE_LIGHT_CALLERS_TU15=1`), the native opt-in and
  the default binary (`e465b2...`) all give frame `5627a51badf47762` and
  trace `8a9b58b38bef`, no validation messages.
- MP frame parity (`tools/perf_4k60/parity.py`, 3840x2160, the translated
  callers as the reference, the default binary as the candidate):
  Coagulation's default cases PASSED (9c14a330..., 13fa114e...,
  05f31264..., the earlier receipts' references); Backwash (the most C0756
  calls) with `--motion`, cases 4:300:10:200 and 1:300:10:200, PASSED
  (65dead3c..., 337820f5...).
- Census, one binary (`e465b2...`), `HALO2_CENSUS_NATIVE_BRIDGES=1`, the
  translated callers against the default. Coagulation 4p, 450 ticks of the
  sweep's scripted input: 268,421,408 translated instructions ->
  266,529,845 (-1.89M, -0.7%); C4D30 25,173 entries / 762k instructions ->
  8 / 504; C48E0 1,595 -> 8; C2CB0 64 -> 8; BEC50 16,927 -> 13,153; C9070
  11,200 -> 8,650; BC8F0 4,903 -> 4,369; 16D740 8 -> 8. Armory 01a, 300
  frames: 79,890,873 -> 79,751,799 (-139k); C4D30 948 -> 3, C48E0 48 -> 3.
  The residue comes from callers not bound here: BCB70's other translated
  sites (C0910's C09EC/C0A57, the effects' 17C47C/17C497/17EA30), the
  effects' C2CB0 and 16D740 calls (the paused group E) and C2CB0's own
  C4D30 call when the effects reach it; C0F60, C2260 and C0700 themselves
  stay translated.

Receipts (small, kept): `build/test-results/light-callers-20261116/`.

## C0910's BCB70 sites (2026-11-16, continuation)

C0910 (an object's node lights; the object tick's BE55F and BA265) is the
native, machine-shaped `NodeLights` bridge of the object subtree
(`native_tu15_object_subtree.hpp`), whose BCB70 child was the unit's own
`run_bcb70`. The binder (`tools/native_object_subtree_replacements.py`,
`REQUIRED_CHILDREN`) now routes that child through the World's required
owner at its original call site (C09EC for a light block entry's weight,
C0A57 for its scale; the site is the CALL before the return address the
closure pushed), where `light_caller` runs the attachments' native
`object_function_value` with the object-function namespace. C0910 reads
BCB70's AL (the active byte) and the value it stored into C0910's own
frame, so the adapter now returns the active byte in AL (for every BCB70
site) and the differential compares AL too. The modelstate track's native
C0910 already calls the same native with this call shape and is proven by
its composed cases; `object_function_value` is the attachments' kernel
(1,500 cases, BCB70 138/138).

**Sweep** (the light callers' and light update differentials, every MP
map at 1 and 4 players with the scripted input, campaign 01a..08b as far
as they run; binary `a78b8df8...`, unchanged during the sweep): **56 runs;
light callers 1,211,181 calls compared, 0 differed, 0 refused** (C100F
1,094,354, C0756 112,342, C2353 2,387, C09EC 1,819, C3810 266, C2135 13);
light update 28,573 compared, 0 differed, 0 refused. C09EC ran on
Cyclotron (1,340) and Colossus (479) at 4 players; C0A57 (a light block
entry's scale) ran nowhere. The early stops are the known ones (Needle and
Street Sweeper, 01b, 03a, 03b, 05a, 08b). The same sweep is the live
check of the instanced-geometry continuation: C4D30's 16D740 calls (bit 0
only) through the reworked `collision_features` match the original.

A census without the native bridges (Waterworks, Zanzibar and Containment
at 4 players, Armory, 300 ticks or frames) counted 6,048..11,450 C0910
entries and none past its definition +1C test (C0954): the rows run only
for the few objects whose definition has that bit (on Cyclotron and
Colossus).

**Default:** native at both sites (with the other light callers;
`HALO2_REFERENCE_LIGHT_CALLERS_TU15=1` keeps the translated BCB70, now
reached through the required owner). Armory frame parity (deterministic
`--campaign-reference-capture`, Original, 300 frames, 1280x720, Vulkan
validation on), binary `02dba867...`: the default and the translated
callers both give frame `5627a51badf47762` and trace `8a9b58b38bef`, no
validation messages. MP frame parity (`tools/perf_4k60/parity.py`,
3840x2160, the translated callers as the reference, the default as the
candidate): Cyclotron with `--motion`, cases 4:600:10:200 and
4:300:10:200, PASSED (c12a5ca6..., 3588657a...); Coagulation's default
cases with every light caller native PASSED with the earlier receipts'
hashes (9c14a330..., 13fa114e..., 05f31264...).

Census (`HALO2_CENSUS_NATIVE_BRIDGES=1`, 600 ticks of the sweep's input,
one binary, all light callers translated against the default): Cyclotron
4p 486,182,725 -> 480,776,844 translated instructions (-5.41M), BEC50
59,228 -> 56,069 entries; Colossus 4p 307,083,992 -> 305,202,036 (-1.88M),
BEC50 10,028 -> 7,132. The switch moves every light caller, so these
deltas include C100F, C0756 and C2353 as well as C09EC. Receipts:
`build/test-results/instanced-geometry-20261116/`.

## C0F60 native again (2026-11-16, object subtree)

The C100F re-emission changed C0F60's canonical body, so the object subtree
binder lost its pin and C0F60 ran translated (its C100F call already reached
this binding). The binder now pins the new body, requires the C100F
required-boundary route, and sends the native C0F60 bridge's C4D30 child
through the World's required owner at C100F
(`tools/native_object_subtree_replacements.py`, `REQUIRED_CHILDREN`). So C100F
reaches `light_caller_site` from the native C0F60 too, and runs placement's
`light_reconnect` by default (the unit's C4D30 under
`HALO2_REFERENCE_LIGHT_CALLERS_TU15=1`). Details:
[BE850_object_subtree.md](../native_replacement/BE850_object_subtree.md#c0f60s-c100f-boundary-2026-11-16).
