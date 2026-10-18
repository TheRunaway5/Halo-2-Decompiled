# Native pages for the original TU1.5 GP allocation caller

The later native scratch resource composition now runs through the actual
table publication, source growth and context destructor; see
`sound_gp_scratch_resources_tu15.md`. The v404–v413 prefix experiment below is
retained as its earlier bounded checkpoint.

`native_tu15_sound_gp_pool_tests` composes the actual recovered38DE81 caller
with38796C and the native `SourceRuntimeSoundDmaResources` page owner. The
owner handles only the authenticated GP caller's allocation, capacity, page
lock/unlock, physical-address and release requests. Their imports, return
frames, parent frames, context, resource bank and creating thread are checked.
Other SDK allocation callers retain their existing owner.

The native pages share the existing descriptor allocator's namespace. Their
addresses are logical resource tokens backed by real retained native64 pages;
they are not host PCI addresses or recovered Xbox physical addresses. Physical
reads/writes require a single retained locked allocation and use its actual
source-bank storage. This synchronous API does not establish asynchronous DMA,
kernel pinning, processor scheduling or a device checkpoint.

The composed case uses the actual image's774144-byte request:189 pages. Its
context and table binding are declared resource inputs. Original387BAA writes
the pool-statistics pointer; original38796C performs193536 zero stores; original
38DE81 writes all189 data SGEs against the native pages. The test retains the
seven deliberately unconfigured prefix entries unchanged. It stops before the
original38DF31 GP table-limit MMIO write without supplying a result. These
checks execute89 original instruction addresses. They do not establish the
prefix factory, table publication, GP startup or audible parity.

Run the registered test under the mandatory source-test budget after building
its target serially within1GiB/zero swap:

```sh
python3 tools/tag_match_memory_budget.py --output-root NEW_GUARD --timeout 60 -- \
  ctest --test-dir ABSOLUTE_BUILD -R '^native_tu15_sound_gp_pool_pages$' --output-on-failure
```

A real native audio backend is required; dummy/disk backends are rejected.
Clang/current-root CMake v404 and guard v405 pass. Frozen GCC v406/v407 also
passes. The latter measured5165056 bytes peak with zero swap. Private frozen
controls v408–v411 reject, respectively, the missing owner, a one-page physical
address error, a discarded lock and an incorrectly shortened context check.
The existing descriptor DMA regression still passes with the changed owner
(v412/v413): four original cases retain their exact instruction/store order,
descriptor bytes and real native resources on PipeWire.

The fixture also checks partial pages, real resource aliasing, growth/release,
foreign frames/threads, overlapping allocation namespaces, invalid ranges,
lost backing and failure lifetime. A partial resource transaction faults the
owner while retaining diagnostic pages until teardown; no source rollback or
success is invented.

The required context extent is36 bytes. Original38DEBC/BD/BE/C2 execute four
MOVSD reads from context+14 through context+23, including the unnamed final
word. Original fixture v381 retains that exact footprint. Intermediate
v399/v401 and v402/v403 used an erroneous32-byte fixture assumption: their
failure/pass pair is superseded and is not evidence of a production bug/fix.
The final fixture and provider both retain36 bytes, and v411 rejects the
deliberate32-byte provider control.

Source-only v394/v396 authenticate import ordinals, the release wrapper and
the image request against the owned executable. v414/v415 trace the next
original table constructors and publication callers. None of these receipts
certifies physical MCPX behavior, final volume/sample timing, the complete
cutscene engine, a current installed game or the requested finished video.
