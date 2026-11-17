# 134D30 cluster contents

`134D30` is the step of the portal visibility pass that turns the clusters a
view can see into render work. `134C00` calls it with the view as its one
stack argument (stdcall, `RET 4`). It is native by default in the generated
Shadow owner (`src/rendering/shadows/source_shadow_frame_tu15_generated.cpp`);
`HALO2_REFERENCE_CLUSTER_CONTENTS=1` restores the translated body.

## What it does

For each cluster on the view's visible cluster list (descriptor `+A6C`
count, `+A6E` entries of 26 bytes), in list order:

1. whether the cluster lies in the current fog (`171070`, fog `[4C67AC]`);
   the objects of this cluster carry that byte into their admission;
2. unless the view skips objects (view flag `0x40`), every object the
   cluster references is offered to the object admission (`135B60`) with
   the cluster's list position: first the collideable partition
   (`[4EABB0]` heads, `[4EABB4]` entries), then the non-collideable one
   (`[4EABA4]`, `[4EABA8]`). With the "machines only" flag (`0x400`) only
   machines (object type 7) are taken from the collideable partition;
3. the main view (kind `+2A78` is 0), unless it skips lights (`0x10`),
   offers every light the cluster references (`[4ECBE0]`, `[4ECBE4]`) to
   the light admission (`135540`);
4. the main view, while it has no sky cluster yet, takes the owner cluster
   of the current sky (`[4C670C]`) from the structure BSP's sky owner table
   (`+AC` count, `+B0` int16 table), when the sky is drawn (`[4C6709]`) and
   `[474C24]` is not -1.

Then:

5. the main view, unless it skips lights or its `+8` word is -1, asks
   `C5980` for up to four lights. Each one whose `+48` word is -1, that is
   active (`C5170`) and not yet queued this light frame (`+C` against
   `[4ECBD8]`) is appended to the view's light queue (`+10`; the queue
   append `166B60` runs inline) and marked queued (`C5140`);
6. unless the view skips objects, the objects of the sky cluster are
   admitted with no cluster position (-1) as sky objects (fourth argument 1).

Each cluster list is a chain of `ClusterEntry` datums (`+4` owner, `+8`
next). The original reads `next` before `owner` at every step, reloads the
entries pool after each admission (the non-collideable walk before testing
for the end of the list) and keeps the next entry, the fog word and the
view in callee-saved registers across the calls.

## What is native

- Logic: `src/rendering/native_tu15_cluster_contents_systems.hpp`
  (`cluster_contents::collect` and its steps `admit_cluster_objects`,
  `admit_cluster_lights`, `find_sky_cluster`, `queue_listed_lights`,
  `admit_sky_objects`, the list `step` and the queue `append`). Named
  layouts: `ClusterPartition`, `ClusterEntry`, `VisibleCluster`,
  `VisibilityDescriptor`, `View` (+ `view_flags`), `RenderQueue`, `Light`,
  `StructureBsp`, and the shared `DataArray`/`ObjectHeader`/`ObjectData` of
  `native_tu15_object_tick_systems.hpp`. Every access names its role
  (`Access`); the ten list walks share one five-access step
  (`walk_access(walk, field)`). The values kept across calls are `Kept`.
- Adapter: `src/rendering/native_tu15_cluster_contents.hpp`. It owns the
  ABI only: the 0x24-byte frame and the four saved registers, every slot,
  push, pop and return address, every register at each access (role table
  `before`/`loaded`), the walks' IMUL/ADD address arithmetic, the
  partial-register writes (AL, CL, AX, SI) and the flags. Two instructions
  need hardware flags because their undefined flags reach the next access:
  `IMUL ECX,ECX,110` at `1350D4` (`light_datum_offset`) and `SHL ESI,2` at
  `135149` (`sky_head_offset`). Both run on the host with the source flags
  seeded in the same assembly block. On the test host (AMD Zen 5) IMUL
  keeps SF/ZF/AF/PF from the preceding AND.
- Children stay genuine and are entered through the unit's own route (the
  binder's lambda calls the unit's `dispatch`): `171070` and `135B60` run
  their own native bridges; `135540`, `C5980`, `C5170` and `C5140` have no
  body in the unit and reach the State's required boundary. `166B60` (a
  pure leaf) runs inline with its own frame, arguments and `RET 10`.
  `PREFETCHT0` performs no architectural access and is not replayed.
- Binding: `tools/native_cluster_contents_replacements.py` (complete body,
  resolver and dispatcher pins), registered in `tools/native_source_prefixes.py`.

Admission: TU1.5 image, no MMX alias, the exact live instruction-cursor
contract and a certified required owner
(`native_required_owner_policy_detail::preserves_instruction_policy`): the
native parent skips the remaining instruction callbacks after a required
owner returns. `HALO2_CENSUS_NATIVE_BRIDGES=1` also admits the uncursored
census callbacks (the closure `134D30`..`166BAF` holds no service PC). The
parent executes no FP instruction, so there is no raw x87 scope; the
children open their own.

## Proof

`tools/validate_native_cluster_contents_tu15.py` runs the unchanged TU1.5
instructions of `134D30`, `166B60` and the genuine `171070` and `135B60`
trees single-stepped on physical i386. Each run stops before the four
externals; a deterministic stub (`stub()` in the validator, `Stub` in
`tests/native_tu15_cluster_contents_tests.cpp`, also the harness's
certified required owner) supplies that callee's ABI and storage effects
and the original resumes at its return address:

- all four pop their return address (`135540` and `C5980` also 8 argument
  bytes) and clobber EAX/ECX/EDX/EFLAGS/XMM0-7; per mode they set an MXCSR
  flag, end the light walk through EDI (`135540`, a callee-saved register),
  or add 1 to the caller's list position slot;
- `C5980` writes the case's listed lights into the caller's buffer and
  returns the case's count (also 0 and negative);
- `C5170` returns active unless the handle's bit 16 is set (AL 1 or 0x80);
- `C5140` stamps the light's `+C` with `[4ECBD8]`.

The native route is compared at every ordered read and store (complete
GP/EFLAGS/XMM/MXCSR/x87 packet), at every required entry (complete ABI), at
the final ABI and over every backed byte. The corpus has 20 families of 8
variants: list shapes and lengths (empty, 1..4 entries, odd datum sizes),
the skip/kind/machines-only flags, sky gates one by one, the direct light
pass (links, activity, stamps, full and zero-capacity queues, zero and
negative counts, duplicate lights), the stub modes, unstamped objects
(the genuine `135970` geometry path), fog membership paths with cluster -1,
and a light queue whose value stream aliases the caller's listed-light
buffer (the original reads each listed light back from the frame).

Results: see "Runs" below.

## Runs

(filled in below by the run)
