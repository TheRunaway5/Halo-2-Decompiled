# TU1.5 animation resource read —1260C0

The sole generated1260C0 owner in `src/physics/physics_live_support_tu15_generated.cpp`
now defaults to a structured native controller. `HALO2_REFERENCE_ANIMATION_RESOURCE_READ`
retains its64 original observation sites. The common generator installs the same exact
full-body/dispatcher binding. The original body SHA remains
`88d97d845f1572c8181d0a3c60503cb9465fae23739e9d02c53797902ab5f6c8`.

The controller lazily acquires its mode, tracking/resource descriptors, allocated page rows,
byte extent, priority and file identity. It computes the real page destination, rounds the
transfer to sectors, makes the genuine1A3B70 I/O handoff and optionally calls1262B0. After
actual child continuations, it rereads the current tracking/cache/rows and stores the authored
age-adjusted timestamp between the original saved-register pops. Child GP state and frame
aliases remain authoritative.

The existing World126139 I/O policy was extracted into
`include/halo2/source_animation_cache_io_tu15.hpp`. World still supplies the same
`resources.animation`, descriptor view, `resources.load` and
`retain_resource_read(...,true)` owners. The helper publishes sector bytes, retains the real
request identity, marks completion, then updates EAX/ESP. The separate first-person-context
provider retains its additional Cache validation. This change does not assert Xbox asynchronous
queue scheduling or new real-asset acceptance.

Two retained integer shifts previously cleared AF through the generic reference helper.
Unchanged hardware exposed that mismatch. Their exact expressions now use seeded native
SHL primitives (variable CL at1260F5; immediate2 at126115); the binder accepts only those
exact corrected forms or the original expressions before checking the unchanged full-body SHA.
No flag bits are masked and no original hardware instructions are patched.

Implementation and regression owners:

- `include/halo2/native_tu15_animation_resource_read_kernel.hpp`: read/dispatch/publication DATA.
- `src/physics/runtime/native_tu15_animation_resource_read.hpp`: caller ABI, flags and frame phases.
- `tools/native_animation_resource_read_replacements.py`: whole-body, exact primitive and dispatcher pins.
- `tools/validate_native_tu15_animation_resource_read.py`: original CPU/semantic-I/O qualification.
- `tests/source_animation_cache_io_tu15_tests.cpp`: previous-versus-shared World policy regression.

Official GCC sanitizer and Release gates both passed208 native transfers and208 forced
reference transfers:24 complete backed-service returns,24 genuine I/O entry stops and160
before-parent-read prefixes. Six complete returns overlap sector writes with saved frame
words; four cases enter1260C0 with DF1 after genuine DF0 page allocation. Each route compares
188 full I/O/wait entry ABI packets, complete GP/EFLAGS/all XMM, MXCSR/x87 control/status,
live80-bit caller values, full banks, ordered reads/stores and the final source cursor.
All64 parent sites execute; ten authentication mutations and seven admission refusals are
checked; proof-source drift is zero.

The oracle first runs unchanged126060/page/datum CPU owners to obtain actual allocated
banks. It runs unchanged1260C0 up to1A3B70, executes the exact production synchronous I/O
helper with declared backed sector/descriptor and retained-request owners, then runs unchanged
CPU from12613E. Bytes and job identities are computed by those owners. No child-result list
is supplied. This segmented proof qualifies CPU framing/continuation under that native I/O
policy. Pending promotion/yield behavior inside1262B0 remains an explicit dependency; whole
native child closure, physical page-fault atomicity and arbitrary callback exceptions remain open.

Evidence:

- `build/test-results/native-animation-resource-read-official-20261109/report.json`: paired result.
- Sanitizer evidence `native-tu15-animation-resource-read.json`:12,017 bytes, SHA256
  `c428840e5ee4b01c495645190c254f3a3de8adc553fc463109d4d38e8d1c293d`.
- Release evidence:11,941 bytes, SHA256
  `650b66117f46fbc4ab4a9959ba5ab9609721707ce60fb0ce330a8682258d6015`.
- `build/test-results/native-animation-resource-read-primary-generation-20261109/report.json`:
  fresh complete primary generation, exact prefix/idempotency, raw and specialized full-CPP
  syntax checks passed.
- `build/test-results/native-animation-cache-io-official-20261109/report.json`: provider factoring
  sanitizer/Release gates each passed3,680 policy comparisons, including failure ordering and
 160 unrelated-owner refusals. Current World full-CPP syntax passed separately.

This port is later than frozen engine/accounting r7. All private build trees, generated
translation units, ELF probes, payloads and raw banks were removed. No large artifacts remain.
