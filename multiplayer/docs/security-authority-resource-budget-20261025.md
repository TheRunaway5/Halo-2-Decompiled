# Network authority resource admission

This addresses Security Cloud finding `csf_939cd12566dbf069184fa29e` from
scan `wfr_8c89af4d74f2e8f7327c653385f32f182e2d83dc62de98f20707ae74d3d52047`.
The network receiver does not use the larger offline codec defaults.

| Reservation | Maximum |
| --- | ---: |
| Two authenticated encoded transfer slots | 64 MiB total |
| Retained canonical base plus expanded successor | 192 MiB total |
| Weighted native input and staging storage | 768 MiB total |
| Current, workspace and incoming decoded immutable grenade resource banks | 96 MiB total |
| Fixed native schema, bounded host sidecars and transfer metadata allowance | 64 MiB |
| Composite input reservation | 1,184 MiB |

`authority_network_limits.hpp` checks this sum at compile time. This is a
conservative reservation for the admitted input owners. Previously imported
dynamic values in the current and workspace worlds are included. Independently
loaded map/cache/image data and fixed local runtime resources,
allocator bookkeeping, thread stacks and process RSS are separate. The bound
must not be represented as a whole-process RSS limit.

Each archive and delta output is limited to 96 MiB decoded, each encoded cut to
32 MiB, and each decoded patch to 96 MiB. A tiny forged stream cannot declare
an arbitrarily large output: the preallocation check allows at most
`2048 * compressed_bytes + 65536`, which preserves highly compressible zero
banks. The decoded ceiling remains the primary admission limit. Delta
reconstruction accounts for encoded bytes, retained base, patch, successor and
decompressor allowance together before allocating the patch or successor.

The Rust grammar parser counts caller backing, its owned decoded backing,
flat node metadata and native materialization claims in one working budget.
Network decoding passes a 448 MiB working limit and a 96 MiB native claim
limit. The canonical delta image enters the same parser through its raw API;
the receiver no longer recompresses it and then decompresses it again.

Parser and native staging peaks are successive phases. The parsed tree and
its owned raw copy retire when typed materialization returns, before native
preparation starts. Even reserving 448 MiB for parsing plus 96 MiB for the retained
base, 192 MiB for both earlier native worlds, 64 MiB for both wire slots, 96 MiB for
immutable resource banks and 64 MiB of fixed scaffolding gives 960 MiB, below the
1,184 MiB composite reservation used by the later native preparation phase.

Native staging receives a second cumulative admission check in the typed
materializer, before each dynamic allocation. Bank storage claims use a
coefficient of six. Before the workspace commits, at most three new bank
representations coexist: input, replacement and optional aligned platform
copy. After that commit, at most five coexist: input, committed workspace,
rebound snapshot, target replacement and optional aligned platform copy.
The old target and workspace banks have separate retained allowances below.
The complete bank remains an immutable owning value between those stages.
Shared or mapped backing does not earn a discount in the reservation.

Other dynamic values use a coefficient of twenty-four. The deepest copied leaf is
the source lifetime bank. At the return from scene preparation, its logical
owners can be the incoming checkpoint, target, workspace, rebound checkpoint,
prepared Match values, prepared Simulation values, Simulation lifetime owner,
the temporary admitted Scene lifetime owner and Scene's retained lifetime
owner: nine logical copies, including the old target and newly committed
workspace. Other Simulation subowners have at most seven logical copies.
For newly allocated ordinary values, reserve eight logical copies, two copies
of vector-growth slack, five copies for one canonical binding buffer and its
growth slack, and one spare copy: sixteen in total. The triangle binding,
for example, serializes a native triangle into about twice its native size.
Native vectors materialized from the parser reserve their admitted extent
directly. Map and set claims already include node overhead.

Let `B` be new normalized bank storage and `N` all other new normalized native
storage. The materializer enforces `6*B + 24*N <= 768 MiB`. Before workspace
commit, new staging costs at most `3*B + 16*N <= 512 MiB`; both old worlds may
retain 96 MiB each, so the sum is at most 704 MiB. After workspace commit its old
storage has retired: new staging costs at most `5*B + 16*N <= 640 MiB`, and the
one remaining old target contributes at most 96 MiB, at most 736 MiB in total. Both
phases fit the 768 MiB reservation. A small
replacement cut does not relabel the larger previous worlds as small. The sum
is cumulative across fields and owners, rather than granting a new allowance
for each subtree.

The owner paths supporting that accounting are
`SourceMatchAuthority::prepare_with_local_views`/`prepare`,
`MultiplayerMatch::prepare_authority`, `Simulation::prepare_authority`, and
the nested scene, device, prop, scenery, presentation, script, grenade and
collision restore methods. A future change adding a retained copy or expanding
a compact input into a new owner must update the reservation and regression.

Compact records that expand during restoration have explicit charges:

* A synchronization record reserves 512 bytes for its real mutex/semaphore,
  registry node, shared control node and source record. Static assertions in
  the native synchronization implementation check the real object layouts.
* `SourceRuntimeMemory::restore_storage_overhead` computes sparse full-page,
  direct-page and partial-page index blocks from the actual incoming addresses,
  plus region/control nodes and mapped-storage alignment. The materializer
  charges that amount before any bank restoration; a sparse topology does not
  obtain uncharged index blocks from a small payload.
* Device and scenery collision validation counts the exact triangle output
  against the already admitted saved triangle bank before generating any
  triangle vector. Polygon fan enumeration uses constant temporary storage.
* Grenade resource admission bounds the complete requested immutable bank
  before loading, including decoded animation buffers and load scratch. Unit
  hierarchy loading avoids unused mesh buffers. Each bank is limited to 32 MiB;
  the composite reserves current, workspace and incoming banks separately,
  even when immutable sharing makes their physical storage smaller.

A complete-world reservation survives the decoder/result boundary until
native preparation has finished or the cut is rejected. Complete-world decode
and preparation are serialized across receiver instances. A full replacement
waits for an active replay to retire its dynamic phase storage; replay-to-replay
prefetch retains its existing bounded pipeline. Cancellation and rejected cuts
release their owned images, parsed trees and reservation.

Both standalone world archives and nested host-continuation archives receive
the explicit network codec limits. Offline diagnostic APIs retain configurable
larger limits. This resource admission grants no world validity or authority:
independent roster, context, schema, digest and native preparation checks still
run before acknowledgment or adoption.

The 96 MiB decoded ceiling was selected after measuring an actual four-player
TU1.5 Lockout checkpoint: 9,140,260 encoded bytes and 87,674,470 canonical bytes.
Normalized native claims were 76,023,135 bytes, including 61,980,903 bank bytes
and 14,042,232 other bytes. The flat tree reserved 134,217,728 metadata bytes;
the observed parser/materialization peak was 385,726,059 bytes. These values
show why a 64 MiB ceiling or a 192 MiB parser reservation would reject a real cut.
The policy remains finite and rejects a 1 GiB declaration before allocation.
The final fixture admitted 16 actual source bodies after 20 genuine source ticks.
The finite lifecycle budget was 630 ticks, derived from the configured respawn,
suicide and betrayal intervals plus one source second. Every slot had an
allocated player, current unit, matching full player/unit identities and actual
source object-body backing before capture. No test-supplied birth or respawn
state established that admission. The resulting archive measured 9,183,721
encoded bytes, 87,707,746 canonical bytes, 76,031,301 normalized native bytes and
709,094,970 weighted staged bytes. Both the four-player and sixteen-body values
pass the explicit 96/448/768 MiB decoded/parser/staging limits. These are source
continuation fixture measurements, not a whole-process memory benchmark or
qualification of every imported map/runtime configuration.

Validation measurements and frozen source hashes are recorded in
`build/test-results/security-scan-20261025-*`. The safe pre-fix allocator probe
failed when the original reader requested a gigabyte allocation; the test
intercepted that request instead of allocating the gigabyte. Regression cases
also cover the 96 MiB full/delta boundary, repeated over-limit rejection, a
sparse-address bank, exact staged-budget rejection, the nested host archive,
and a real source checkpoint passing the explicit network limits.
