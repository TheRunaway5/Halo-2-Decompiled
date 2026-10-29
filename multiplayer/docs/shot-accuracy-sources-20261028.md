# Shot accuracy: original owners and current native boundary

Research date: 2026-10-28. This note records source evidence; it does not claim
that the website, accepted reports or launcher delivery have been verified.
No production source was changed for this investigation.

## Exact numeric source

The unchanged English TU1.5 executable is
`/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe`,
SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Its weapon-stat add owner **19B860** reads/writes a little-endian word at:

```text
51EEF4 + 2 * (player_slot * 437 + 78 + reporting_index * 8 + category)
```

Category **4** is shots fired; category **5** is successful shot marks. Its
actual instruction at **19B898** reads from base **51EF90**, which is
**51EEF4 + 156 bytes**. The matching PCR source offset is **0xECC**, exactly
**0xE30 + 156**. A fixture using player word 77, or adding 154 bytes to the
general-stat base, is one word too early. These are numeric source offsets,
not alternate encodings. See [the native layout](../../include/halo2/source_result_statistics.hpp),
[the reproduced writer](../../src/gameplay/multiplayer/source_result_capture.cpp)
and [PCR's consumer](../../src/ui/vista/vista_pcr_results.cpp).

The original writer requires recording byte **51DFA0 != 0** and frozen byte
**51DFA1 == 0**. It masks the old value with `0x7FFF`, adds with signed-32-bit
wrap, clamps to that category's imported descriptor and preserves bit 15.
Both categories 4 and 5 have descriptor bounds **0..32767**; category 6 also
has those bounds. Kills/deaths/betrayals/suicides (0..3) instead cap at 1023.
The seven 16-byte descriptors are at **47A658**. The low 15 bits are the
numeric value, not a signed 16-bit counter. The original call results and
descriptor values are retained in [source-mark-probe.json](evidence/shot-accuracy-tu15-20261028/source-mark-probe.json).

The actual PCR result consumer sums reporting indices **5 through 38,
inclusive**, separately for categories 4, 5 and 6. Thus the stable aggregate
definition for this existing PCR boundary is:

```text
shots = sum(player[78 + w*8 + 4] & 0x7FFF for w in 5..38)
hits  = sum(player[78 + w*8 + 5] & 0x7FFF for w in 5..38)
```

Preserve source counts. Do not substitute an input-button count, raw damage
event count, global simulation diagnostic, or a guessed general-stat field.
For career totals, add accepted match shot and hit counts before dividing;
averaging per-match percentages changes the denominator. A match without
verified shot counters is unknown, not proof that zero shots were fired.
PCR's [existing display routine](../../src/ui/vista/vista_pcr_screen.cpp)
returns zero for `shots <= 0`; otherwise it truncates its binary32 calculation
of `hits * 100 / shots`. It does not impose an upper percentage clamp.

## Reporting indices are not weapon IDs

Original **19B8F0** masks the projectile/damage reporting byte with `0x3F`
and maps it through its original 41-case dispatch table at **19B9F8**. Inputs
41..63 take original default index 0. The mapping is not identity: for
example raw cause 19 maps to reporting index 15, cause 15 to 16, cause 25 to
40, cause 39 to 20, and cause 40 to 21. Use this original mapping when
interpreting the source bank, not weapon tag ordinals or frontend choices.
[OriginalData imports this table](../../src/assets/original_data.cpp);
[the probe receipt](evidence/shot-accuracy-tu15-20261028/source-mark-probe.json)
contains every low-six-bit mapping.

For named reference corroboration, [authored-reporting-indices.json](evidence/shot-accuracy-tu15-20261028/authored-reporting-indices.json)
records original Lockout's weapon barrel and projectile metadata, with the
map hash and file offsets. Examples are Magnum 5, plasma pistol 6, Needler 7,
SMG 8, plasma rifle 9, battle rifle 10, carbine 11, shotgun 12, sniper 13 and
beam rifle 14. This is offline original-data evidence; it does not establish
that every currently imported MCC tag has identical metadata. Runtime
acceptance must use prepared MCC content.

## What is counted

The original owners are **15F6E0** (create shot mark) and **15F7D0** (consume
successful mark). The independently recovered retail equivalents are
**15CCA0/15CD90**, readable in
[unknown_157450.cpp](../../tools/reference/halo2-decompiled/src/unknown_157450.cpp).
This investigation executed the TU1.5 instructions, rather than assuming that
the retail decompilation was sufficient.

**Creation.** Original **109220**, reached by the weapon's pending-projectile
postpass **102980**, calls **15F6E0 at 10951B**, before entering its projectile
loop at **109550**. It resolves the actual owning player and the actual
projectile's reporting byte. The mark owner chooses one of eight records at
player **+1D8** (stride 8: signed-short identifier, reporting byte, tick).
It prefers an unused/older-than-60-seconds record, otherwise replaces the
oldest. It assigns the incrementing global short at **48C7A4**, stores the
reporting byte/time and adds one category-4 count. Creation itself is not
gated on the recording bytes; the counter writer is.

**Pellets and bursts.** All projectiles in that call share the same mark;
**10A303/10A32C** copy it into each new projectile's **+1A8** word. Original
Lockout's shotgun barrel has projectile count **10** at barrel **+6A**, so
ten pellets from that discharge do not mean ten shots or ten successful hits.
The battle rifle has burst bounds **3/3** at **+1C/+1E** and one projectile
at **+6A**: its separate admitted barrel rounds each go through the
pending-projectile postpass. The counted event is a discharged barrel shot,
not a fresh trigger press. This burst conclusion follows the original
postpass and authored barrel data; the retained probe does not execute an
entire three-round BR firing timeline.

**Successful hit.** Damage owner **DA0D0** calls **15F7D0 at DA2A4** when the
damage request has a non-NONE mark, an owning player and a player-owned
biped/vehicle target. This happens before the subsequent shield/body damage
application. The hit owner verifies the shooter's live pool salt, requires a
selected engine, invokes that engine's actual relation method **27**, then
finds the matching mark. For Slayer, the actually registered method is
**25BC70**, which compares the two membership values. An accepted relation
adds category 5 using the **stored mark's** reporting byte and immediately
sets the mark identifier to NONE. See the original damage decompilation at
[damage.cpp](../../tools/reference/halo2-decompiled/src/damage.cpp).

Repeated pellets, penetration contacts or repeated damage notifications with
that same consumed identifier cannot add more successful counts. A friendly
relation leaves the mark unconsumed, so a later accepted relation may count
it. The hit owner does not independently test the mark age; replacement is
owned by creation. A wrapped global mark identifier of `0xFFFF` is NONE and
cannot be consumed, though its firing count can still be recorded.

For a clean, uninterrupted recording, these semantics prevent a single shot
from generating multiple hits. However **`hits <= shots` is not an unconditional
source-bank invariant**. The original-call probe deliberately disables
recording during creation, enables it before consumption, and obtains
**0 recorded shots, 1 recorded hit**. The mark survives because creation and
consumption are separate from the writer's recording gate. Freezing before
consumption similarly clears the mark without incrementing the frozen count.
This counterexample is a declared partial recording window, not evidence that
ordinary shotgun or splash damage legitimately yields multiple successful
shot marks. Preserve authenticated source values; do not silently clamp or
reconstruct them to satisfy an assumed inequality.

## Current native integration and verification seam

The production source world already has both original producers:

| Boundary | Current source |
| --- | --- |
| 10951B -> full 15F6E0 -> 19B860 kind 4 | [source_sandbox_special_tu15_generated.cpp](../../src/gameplay/objects/source_sandbox_special_tu15_generated.cpp), call near line 4466 and full owner near 6567 |
| DA2A4 -> full 15F7D0 -> 19B860 kind 5 | [source_damage_delivery_runtime_generated.cpp](../../src/gameplay/units/source_damage_delivery_runtime_generated.cpp), call near 3679 and full consumer near 8660 |
| Compose real sandbox / damage owners | [physics_live_world_tu15.cpp](../../src/physics/physics_live_world_tu15.cpp), `required`, sandbox dispatch and `Module::Damage` branch |
| Copy full source bank to match result storage | [multiplayer.cpp](../../src/gameplay/multiplayer/multiplayer.cpp), `synchronize_source_engine`, original 51EEF4 read |

Those paths do not return synthetic shot/hit counts. The generic C++
`source_result_capture.cpp` helper's explicit kill/death callers produce
categories 0, 1, 2, 3 and 6, not 4/5. That observation is **not** a missing
producer in the actual source world: the original lowered weapon/damage
owners write the shared original bank directly. A second counter there
would risk double counting. `MultiplayerMatch::step` synchronizes the
source-engine result bank at the end of its source-owned tick, and
`recorded_statistics()` exposes that copy.

The existing focused native seam is
[tests/multiplayer_engine_combat_tu15_tests.cpp](../../tests/multiplayer_engine_combat_tu15_tests.cpp),
registered as **native_mcc_multiplayer_engine_combat_slayer_tu15** in
[MultiplayerEnginesTu15.cmake](../../cmake/MultiplayerEnginesTu15.cmake).
It creates `MultiplayerPreparedWorld` with actual source rosters/units/starting
weapons, positions two players using the original position owner and gives
them ordinary aim/fire controls until the real fatal hit. The test already
checks score/stat synchronization. At research time it reports global
`simulation().shots()/hits()` diagnostics, and does **not** assert original
category-4/5 values. Those diagnostics are not the profile accuracy source.

Its required inputs are:

- A prepared **MCC Lockout map**, configured through
  `HALO2_MCC_MULTIPLAYER_ENGINE_MAP`; the native input is not the original
  offline Xbox map. An available prepared reference in this workspace is
  `assets/imported/mcc-sky-water-tu15-20261025-v7/halo2/h2_maps_win64_dx11/lockout.map`.
- `tests/fixtures/multiplayer_engines_original_tu15.json`.
- A report path inside the build's owned scratch and optional preset `Slayer`.

The precise additional native assertions are: nonzero original shots/hits
after ordinary firing, every relevant source word equal to its counterpart
in `recorded_statistics()`, and the exact bank preserved through finalization
and any terminal report publication. This investigation identifies the seam;
it did not rebuild/run that native test or prove terminal UDP/API/website
delivery. An example focused runner invocation is:

```sh
python3 tools/run_tests.py \
  --target multiplayer_engine_combat_tu15_tests \
  --filter '^native_mcc_multiplayer_engine_combat_slayer_tu15$' \
  --cmake-arg=-DHALO2_MCC_MULTIPLAYER_ENGINE_MAP=/absolute/prepared/MCC/lockout.map \
  --retain-report multiplayer-engine-combat-slayer.json
```

## Retained bounded evidence

[source-mark-probe.json](evidence/shot-accuracy-tu15-20261028/source-mark-probe.json)
contains 11 successful original whole-call cases, exact image hash, declared
raw pool/engine/clock/recording inputs, reached owners, original instruction
listing hashes, descriptor bounds and the complete cause mapping. Original
engine registration **3825D0** ran; its actual Slayer relation callback ran;
no original instruction or service return was patched. This proves the
specified mark/stat component, not complete weapon/world/platform execution.

Probes ran as inline Python commands under `tools/run_test.py`, using the
existing XBE reader and `Source` emulator adapter. No new permanent test
helper or binary was created. The explicitly recorded scratch directory
`.local/test-scratch/halo2-test-ocblvqhl` was verified absent afterward.
Only the two small JSON receipts (about 20 KiB combined) and this note are
retained; original assets, existing references and active builds were preserved.

Read-boundary source hashes (research snapshot; concurrent later edits require
a refresh before a current-code claim):

```text
905c67a05f240ca3699fa49ff72156384aab1c85cfe13e405f2b20807ee0dcf5  src/gameplay/multiplayer/source_result_capture.cpp
cebf3de2549ec9addfec3310aac7c4884b958935052630aa71f9d912ed41393c  src/gameplay/objects/source_sandbox_special_tu15_generated.cpp
7e884856f0ab8bd7f9bdbeaad8bb9dcc62870d004a17795f592176cbc775f5a5  src/gameplay/units/source_damage_delivery_runtime_generated.cpp
ea5e0a4588efb2d09641b12c1ff5a7f933edd754ab158685e1f7eeab02401465  src/gameplay/multiplayer/multiplayer.cpp
49385d7077c15346c2e517d41b5c719588c7a534ae2d9bd6333d0fb363ec1e97  src/ui/vista/vista_pcr_results.cpp
88bc5a5556c6769fecc93a6df46bc7e4ac3189b286235de6e110b1854d1bd346  tools/reference/halo2-decompiled/src/unknown_157450.cpp
055372b531c925eb2f843ee4176b0725afce176a142cd5456665368f71766973  tools/reference/halo2-decompiled/src/damage.cpp
a5c8122dd615a622b3551f4bc4f6a6a7c62f2f30e123f5b04ab40240db30cae9  tests/multiplayer_engine_combat_tu15_tests.cpp
```
