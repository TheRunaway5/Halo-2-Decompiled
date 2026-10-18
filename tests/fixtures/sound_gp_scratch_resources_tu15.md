# Original GP table publication with actual native backing

`native_tu15_sound_gp_scratch_tests` runs the recovered CPU table constructors,
page cursor, copies, pool allocation/growth and context destructor against the
actual `SourceRuntimeSoundDmaResources` allocations. The source bank, native
pages and stereo output are retained together on the creating executor.

Original38E996's two GP requests are C000 bytes of prefix data and1060 bytes
of table storage, both aligned4000. Original388294 reserves204 entries;
38DD54 derives524 entries from the requested table bytes, constructs eight
initial SGEs and publishes that actual base/count. Original38F5F3 now derives
three appended descriptors from the actual19-page EP allocation's last three
pages. The fourth page is the parent's separate391410 descriptor. These execute
the original append path, taking reservation516 to512 and the published prefix
from8 to12. EP/GP software contexts remain declared; complete device/PIO factory
startup is not established by this test. See
[`sound_gp_ep_append_bridge_tu15.md`](sound_gp_ep_append_bridge_tu15.md).

The native translation service admits only the authenticated38DD54 ->38E6DF
->38E668 kernel frame and retained prefix allocation. Original38DE81 then
locks189 real data pages and publishes201 actual table entries. Reads/writes
walk the source-produced SGE bytes and resolve their retained native pages;
there is no parallel scratch buffer. Every operation checks resource identity,
descriptor publication, table bounds, high words, context and pool locks.
Writes preflight the whole transfer before changing any page.

Prepared immutable image data and original control bytes are copied by the
original38DE59 instructions. All823296 resulting bytes match the independently
prepared executable-derived boot image. Its request word remains3. This is
composition with the previously verified immutable loader, not execution of
the complete388309 decode/descriptor-relocation/GP-acknowledgement caller.

The original growth path publishes190 new pages, unlocks/releases the old189
pages through the newly recovered3879B3 wrapper, and updates source statistics.
Original38DD26 then unlocks/frees the current pool and clears its context pointer.
Subsequent access to retired data is rejected. Prefix/table descriptor resources
remain retained by their owner; this test does not supply their full SDK/APU
teardown or asynchronous GP retirement.

The generic scalar-store helper reads a preimage even for an unobserved store.
Native probe v435/v436 reaches the original first GP table store and rejects
that extra MMIO read. The DSP module now offers a typed native store at exactly
38DDE3,38DDED,38DE4D and38DF31. The owner records the actual resource transition;
it supplies no unknown MMIO preimage. The generic before/after byte observer
continues to cover ordinary memory. Declared offline MMIO byte sinks retain
their existing full store-digest comparison. No shared physics helper changes.

Build `native_tu15_sound_gp_scratch_tests` serially within1GiB/zero swap, then:

```sh
python3 tools/tag_match_memory_budget.py --output-root NEW_GUARD --timeout 60 -- \
  ctest --test-dir ABSOLUTE_BUILD \
  -R '^native_tu15_sound_gp_scratch_publication$' --output-on-failure
```

The target requires the authenticated owned TU1.5 executable and a real native
audio backend; absent original data skips the registered test. Clang v439/v441
verifies the full growth path; GCC v444/v446 additionally verifies the context
destructor. That earlier composed case executed356 original instruction addresses.
The current403-address case includes the EP bridge and its three-page producer;
Clang/GCC v528 and all nine source/resource checks per compiler v529/v530 pass.
Current-root final Clang/CMake v450 and all five registered source/resource
tests v451 pass with the same destructor path.
The existing descriptor-DMA regression also passes after recompilation against
the enlarged current owner (v452/v453); its original instruction/store traces,
descriptor bytes, failed-scope lifetime and actual PipeWire backing remain equal.
Private controls fail for a wrong prefix physical page, discarded count
publication and ignored SGE high word (v447–v449). The62-case original CPU suite
also passes124 comparisons/189731 ordered stores/34 external stops (v440/v442).
Strict regeneration v445 matches all eleven selected modules:180 complete
bodies,17150 instructions and62312 original bytes.

This native resource binding does not implement MCPX MMIO reads/status, the
complete upstream factory, actual image upload, GP peripheral DMA execution,
timer/IRQ, GP/EP/output clocks, final routing/mix or audible/video parity. The
ordinary live world is not connected to this scratch view and still stops
before38846E's unowned request publication. No source acknowledgement, readiness,
gain correction, authored sleep change or clock rebase is supplied.
