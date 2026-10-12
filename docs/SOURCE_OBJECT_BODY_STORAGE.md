# Persistent source object body storage

The simulation now owns [SourceObjectBodyStorage](../include/halo2/source_object_body_storage.hpp)
beside its existing shared [NativeObjectLifetimes](../include/halo2/native_object_lifetimes.hpp).
This supplies retained mutable storage for recovered constructor kernels. It
does not construct or register a game object by itself. The ordinary native
unit, scene, weapon and projectile owners still need their actual constructor,
pose, activation and destruction services before they can publish source bodies.

## Original authority

The [storage research](SHIELD_OBJECT_BODY_STORAGE_RESEARCH.md) pins complete
TU1.1/u5 allocation, append, retirement, pool and heap owners, all thirteen
selected type extents, and startup operands. TU1.1 `B76E5` supplies heap capacity
`100000h`; `125520` obtains `(capacity+53h)&~3` bytes and initializes it with name
`objects`, context0. This is distinct from the shared `object` header pool with
2048 twelve-byte records. Retail startup supplies the same capacity/name.

The native adapter composes existing complete [BD170](../src/unit_action_dispatch.cpp),
[BD270](../src/unit_constructors.cpp) and [finite heap](../src/object_body_heap.cpp)
kernels. Allocation success and relocation come from current capacity, links
and bytes. No captured body or supplied success result drives the adapter.
`OriginalData` separately imports each authenticated TU1.1/u5 type record's
literal `+8` field through [base_extent](../include/halo2/object_construction_tables.hpp).
The older bounded construction-table `read` ranges remain unchanged; adjacent
unowned fields remain unavailable. Base-image type metadata remains unimported.

## Ownership contract

`reserve_storage()` obtains a full datum from the same pool used by native
categories and leaves the cleared header type byte untouched. Its native
sidecar type `FF` means unresolved; it is never written to source header`+3`.
Original generic construction owns that later type store. An unresolved raw
reservation cannot be published through `publish_native`.

BD170 reserves the header, tries real body allocation with metadata0/0, writes
header extent`+6` and clears precisely the signed base extent. Physical padding
survives. Failed body allocation retires its newly reserved header once; failed
header allocation performs no heap allocation. A live body pointer prevents
direct header-only retirement through `NativeObjectLifetimes::release`.

BD270 uses the actual current header extent and body pointer. Growth exposes
the full enlarged physical payload before BD270 commits logical extent. A
relocation preserves the entire old physical payload, including padding;
callers reacquire body views afterward. Zero append writes offset`FFFF` before
size0 without a grow. Failed grow retains the old body and descriptor state.
The adapter preserves existing kernels' bounds stops for unsupported malformed
signed/wrapped ranges; those native exceptions are not an original exception ABI.

Storage retirement clears header flags, frees/splices the physical block,
clears the body pointer and then retires the shared header once. It composes
the raw heap free owner with the shared pool retirement, rather than running
the complete raw BD1F0 retirement and freeing the datum a second time. This
matches BD1F0's valid storage state; raw heap observer PCs identify `18FE60`,
not BD1F0's inline free instructions. No full observer-trace equivalence is
asserted for this composition. Preceding detach, unregister, physics and other
destruction services remain the caller's responsibility.

The address tokens `02000000/02200000/02300000` map the owned arena, the **same**
shared header bank and private resize scratch. They are native opaque tokens,
not captured Xbox globals or host pointers. Default initial arena byte`CD` is
an explicit native allocator input. Original initialization clears only its
64-byte heap header; the adapter makes no claim about original platform arena
contents. Smaller capacities and other initial bytes used below are controls.

The value-owned bank retains arena bytes, resize scratch and full-handle
associations; it stores no callbacks or references to another owner. The
[campaign save/restore](../src/simulation.cpp) copies it beside the shared pool,
then restores both before scene consumers synchronize. Explicit simulation
storage methods do not infer scene membership or completed constructor state.

## Validation

The [original driver](../tools/measure_source_object_body_storage.py) executes
unchanged TU1.1 `16D070/16D210/16D220`, `18FC80`, `BD170`, `BD270` and `BD1F0`
with their real nested pool/heap helpers. Code bytes are checked on every
executed instruction. It supplies declared raw backing, finite capacities,
name/ABI inputs and explicit payload/header controls, with no instruction
patches or allocation-result stubs. Both selected immutable type banks are
read directly. This increment executes TU1.1 dynamically; the u5 evidence is
the separately retained complete static pairing and literal metadata.

The [capture](../analysis/damage-shields-goal-20261011/body-storage/measured/report.json)
and [independent repeat](../analysis/damage-shields-goal-20261011/body-storage/repeated/report.json)
contain 83 sequences and 1,996 complete storage snapshots. All 1,795 files
repeat byte-identically. Fixture SHA-256 is
`4372a3174a20bfd26cb1912730b04cb4295ad267d42775cee50fdd0e25dae3aa`.
The sequence includes all thirteen actual base extents, base/header pressure,
failed and relocating append, padding, zero append, alignment shift masking,
flags/type retention on release, shared native-category controls, free/reuse,
the actual startup capacity, and persistent bounded randomized streams.
[Repeat comparison](../analysis/damage-shields-goal-20261011/body-storage/repeat-comparison.json).

The [native test](../tests/source_object_body_storage_tests.cpp) compares every
byte of pool cursor/high-water/count/salt, bitmap, all headers and full arena
after every original operation: 9,589,036 compared bytes. It also checks stale
ownership, unresolved publication, physical-padding copy, independent value
copies and private scratch restoration. The [campaign test](../tests/campaign_tests.cpp)
checks save/revert with nonempty, relocated source storage and a discarded
replacement lifetime. All nine focused checks pass, with none skipped: storage,
shared lifetimes, the original datum pool and construction-table providers,
allocation/action dispatch, three constructor-tail configurations and campaign
checkpoints. [Test log](../analysis/damage-shields-goal-20261011/body-storage-tests.log).
The full native executable rebuild and `--help` entry also pass; all nine
checks pass again against the final linked binaries. The first full build
encountered a concurrent vehicle-motion generator syntax error, already
corrected by that work before this rebuild; this increment did not edit those
files. [Scoped source, binary and validation manifest](../analysis/damage-shields-goal-20261011/body-storage-validation.json).

This closes the persistent allocation/storage and selected extent-provider
boundaries. Complete generic/type constructors, body/header flag producers,
pose/physics and activation services, registration packets and the live shield
collector remain open. No original shield pixel, natural gameplay or broader
damage-source completion claim follows from these storage comparisons.

The [next constructor handoff](SHIELD_COLD_POWER_MODULE_CONSTRUCTOR_HANDOFF.md)
pins an actual power-module row and its real scenario request producer.
Mandatory `148360` pressure ownership precedes body allocation. Its main and
overflow heaps are distinct from this body arena; an empty body arena cannot
stand in for the missing pressure environment. The complete generic caller
must retain that boundary until its actual resource owner is bound.
