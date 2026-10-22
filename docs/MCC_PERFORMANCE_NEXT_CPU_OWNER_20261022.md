# Next CPU owner after vector-read specialization

Read-only investigation, 2026-10-22. Target remains ordinary-runtime 3840×2160
at 120 FPS with one player and 60 FPS with four players, preserving TU1.5
behavior and graphics. This task changed no production code and did not touch
the parent's active native build.

## Recommendation

Extend the existing **build-local operand specializer** to remove a redundant
pure branch-membership check at eligible generated switch sites. The first
concrete site is `2E62C7`, inside the historically hot ordinary-main chain
`44710 → 44E80 → 2E79A0 → 2E6290`. Keep canonical generated sources unchanged,
all original source-memory reads/writes and instruction callbacks intact, and
retain the exact default-path errors. This is a measured, state-free component
opportunity; its integrated FPS benefit remains unmeasured.

Do **not** introduce a topology epoch/cache just to remove the second region
lookup in scalar writes. A current-source microprobe measured that lookup at
about **1.19 ns**, against **26.17 ns** for a complete scalar write with one
guard and two real atomic mutation observers. Avoiding one lookup has at most
about 4.6% upside in that particular write microcase before added proof/check
cost. The lookup is already a cheap full-page index hit, and retaining an
iterator across arbitrary callbacks creates a real lifetime hazard.

## Evidence provenance and source drift

The historical [two-player profile](../build/test-results/splitscreen-optimization-20261022/optimized-profile-2p.json)
belongs to runtime SHA-256
`7db35ababe9ba5e2b351ae5a9ccb96fb5800f66d9e2b3bc14b1dbc37470e7c79`,
1280×720 Coagulation, 930 samples after warmup. It recorded 57.42% inclusive
translated execution, 26.77% inclusive shadow module work, and 21.08% inclusive
each in `run_44710` and `run_44e80`; those are nested owners, not additive costs.
`run_2e79a0` and `run_2e6290` account for 19.14% and 18.39% inclusive respectively.

Exclusive historical samples include write dispatch 6.34%, instruction cursor
5.59%, memory write 5.48%, memory read 4.84%, region lookup 3.66%, validation
2.80%, continuation observer 2.80%, world observer 2.47%, and physical guard
2.26%. The profile ranks owners; it does not measure the current source or the
parent's new vector-read path.

Seven inspected current production files differ byte-for-byte from the
[profiled delivery inventory](../build/test-results/splitscreen-optimization-20261022/delivery/source-hashes.json),
including moved include paths and later changes. The local
[source/provenance receipt](../build/test-results/mcc-next-cpu-owner-20261022/source-provenance.json)
records both hashes. No claim of source-version profile matching is made here.

## Exact branch specialization

[The current shadow module](../src/rendering/shadows/source_shadow_frame_tu15_generated.cpp)
defines `canonical_branch` at line 346 as a pure switch with 110 mappings. Every
mapping returns its input unchanged; MapPack11 returns the input directly, and
other image values use that checked table. Its only observable failure is
`Unbound TU1.5 contact dispatch target`.

At line 17368, original `2E62C7` reads a target from a source jump table, calls
that helper, then dispatches through 23 local cases. A target already present
in those local cases therefore undergoes two membership checks. The adjacent
byte-table read at line 17367 and the target-table read must both remain.

For an eligible site, evaluate the original target expression once at the same
position and use this structure:

```cpp
const auto target = original_target_expression;
switch (target) {
case original_local_target: goto original_label;
// All other original local cases, unchanged.
default:
    (void)canonical_branch(target, original_image_expression);
    throw std::runtime_error(original_local_default_message);
}
```

This preserves the important distinction: a TU1.5 target outside the global
table raises the original **global** error; a globally recognized target absent
from this local switch raises the original **local** error. MapPack11 and
invalid enum values keep their existing treatment. No source memory is replaced
with host literals, and no mutable state, observer or resource permission is
cached or bypassed.

Eligibility must require the exact recognized pure-helper grammar, unique
mapping keys, **all helper mappings identity**, every local case present in the
helper map, and an exact recognized outer case/default grammar. Preserve
comments/strings, reject ambiguous syntax, retain the helper, make the transform
idempotent, and include source/mapping/output hashes plus site counts in the
existing specialization receipt. A nonidentity helper cannot use this transform:
an incoming raw target might map to a different local target.

The [source inventory](../build/test-results/mcc-next-cpu-owner-20261022/dispatch-inventory.json)
found 330 such source switch expressions in 39 generated modules. Thirty-one
modules have entirely identity helper maps, containing **285 potential sites**;
the other eight modules must remain unchanged by this proposal. This inventory
checks mappings and local targets, not the complete outer grammar. The
specializer must still qualify each site. Examples include ordinary shadow,
campaign, multiplayer-engine, vehicle and portal-light modules; no broad manual
replacement is justified.

## Component experiment performed

The [isolated dispatch probe](../build/test-results/mcc-next-cpu-owner-20261022/dispatch-cost.json)
compiles the exact current canonical helper and the hot site's 23 local targets
with `-O3 -DNDEBUG`. Baseline and specialized functions are kept independently
callable, with randomized valid-target timing and alternating execution order.
The retained disassembly shows a real `call canonical_branch` before the
baseline's local comparisons; the specialized valid path begins local
comparisons and leaves that helper call in the default path.

The final run passed **3,330 return-value / exact-error comparisons**, including
all 110 globally known targets, known-but-not-local targets, random unknowns,
both image values and an invalid enum. Five eight-million-call timing pairs
measured median CPU cost **13.530 ns → 12.311 ns**. An earlier layout measured
12.452 ns → 11.640 ns. Both showed a small benefit, with layout/host variation.
These are approximately 6–9% reductions in this isolated dispatch cost, **not**
in the full routine or frame. The probe has no source-memory traffic, geometry,
register-file updates or actual source-call integration.

The [lookup probe](../build/test-results/mcc-next-cpu-owner-20261022/lookup-cost.json)
compiled current memory code with a private header copy exposing `locate` only;
no layout or implementation changed. Five four-million-iteration passes checked
20 million exact deliveries to each mutation observer. Its numbers above are
component costs, not an integrated saving or a reason to remove observers.

## Already present; preserve these contracts

* [Write protection](../src/runtime/source_runtime_write_protection.inc), lines
  135–172, already uses per-thread callback arrays, retained write-epoch identity,
  a recursive fallback, and a fallback for banks exceeding 16 entries. A shared
  lock spans every callback so concurrent retirement cannot free a borrowed
  function. Replacing it with an unlocked cached pointer is unsafe.
* [Physical protection](../src/runtime/source_runtime_host_physical_memory.cpp),
  lines 57–69, already returns before locking for ordinary lower-half writes;
  spanning and physical writes retain real protection checks.
* [Memory transfer](../src/runtime/source_runtime_memory.cpp), lines 40–57,
  already specializes 1/2/4/8-byte memmoves. The generated
  [write helper](../src/physics/runtime/physics_step_internal.hpp), lines 23–39,
  already avoids preimage reads when no store trace exists.
* Scalar writes still validate full extents before mutation. Backing guards
  precede global guards/observers; the post-callback lookup is at current line
  443. Do not remove it without a complete lifetime proof. Cross-owner staging,
  overlap semantics, empty writes, wrap failures and commitment failures remain
  required.
* [World](../src/physics/physics_live_world_tu15.cpp), lines 225–232, and
  [continuation](../src/gameplay/multiplayer/multiplayer_object_runtime.cpp),
  lines 676–684, use real per-write atomic mutation counters. Batching them per
  routine or tick changes observable invalidation behavior.
* `SourceRuntimePolicyHold` is a publication/import exclusion mechanism. Its
  use in `NativePublicationHold::Impl`, at physics-world lines 3456–3498, installs
  whole-bank **write-denying** guards. It is not a lease for ordinary unlocked
  source execution.

## Next larger host-preparation experiment

There is a safe immutable-data seam, but its cost must first be measured on the
new runtime. [SourceCampaignModel::vertices](../src/campaign/source_campaign_model.cpp),
lines 144–163, still scans the complete immutable LOD vertex union for every
selected section, copies bones/compound definitions into a temporary model,
and then skins. Preindex **ordered source vertex indices** by section when each
immutable LOD resource is admitted, and gather those exact indices on the draw
path. Keep ordered duplicate section requests, vertex origins, skin bindings,
LOD-specific compound palettes and per-instance baked lighting unchanged.

This is distinct from skinning reuse already present: ordinary campaign draw
memoizes posed `(section, baked-instance)` results within one call, and ordinary
unit/vehicle paths already prepare one selected LOD pose through frame-owned
providers. Do not count those existing optimizations again. Likewise,
[SourceDynamicModelShaders](../src/rendering/models/source_dynamic_model_shaders.cpp),
lines 75–105, rebuilds a map across all authored parts for each selection;
sharing a selection requires identical complete collector/view/coverage inputs.
Neither case permits skipping native `44710`, copying another view's results,
or ignoring its real source writes and camera-dependent inputs.

## Falsifiable acceptance plan

1. Add the pure switch transform to `tools/specialize_native_operands.py` with
   differential tests in `tests/test_native_operand_specialization.py`. Include
   nonidentity helpers, duplicate maps, malformed syntax, comments/strings,
   expression evaluation counts, exact global/local errors, both images,
   invalid enum, idempotence and unchanged canonical source hashes.
2. Keep a build/runtime reference switch so one executable can compare the new
   branch path with the old one. The source entry, source reads/writes,
   instruction trace, registers/flags/SSE/x87 and callback order must agree on
   actual reached `44710 → 44E80 → 2E6290` cases, including injected faults.
3. Do not treat `original_tu15_shadow_entry` as coverage of this chain: its
   current fixture has 146 cases at entries `226B0`, `335B0`, `335D0` and **zero**
   instruction hits for `44710`, `44E80`, `2E79A0` or `2E6290`. The existing
   `tools/audit_live_shadow_main_cull_tu15.py` provides an unchanged-original
   comparison route for a genuine collected main bank; it requires retained
   real inputs and does not itself certify native or GPU parity.
4. Reprofile the parent's pinned current runtime after vector reads. If this
   dispatch owner is no longer material, do not expand this optimization merely
   because the microprobe improved. Otherwise run alternating unsampled
   ordinary 4K one/four-player cases with identical executable, scene, camera,
   settings and warmup. Require meaningful frame-time/percentile improvement;
   a faster isolated dispatch does not establish 8.33/16.67 ms complete frames.

Relevant memory regressions remain `source_runtime_write_dispatch` and its
reference variant, `source_runtime_policy_publication`,
`source_runtime_transfer_boundaries`, shared-backing/commitment tests and the
source-execution observer/call tests. Immutable section gathering needs complete
ordered vertex/binding equality for duplicates, all selected LODs and baked
instances; existing palette/material/vehicle-skin regressions are supporting
coverage, not a substitute for that direct check.

All small probes ran through `tools/run_test.py`; code, compiler outputs and
executables lived in `HALO2_TEST_SCRATCH` and were removed. Only this document,
bounded JSON evidence and hashes remain. No full build, game launch, new asset
copy or large retained artifact was created. The cleanup/provenance receipt
records post-exit checks.
