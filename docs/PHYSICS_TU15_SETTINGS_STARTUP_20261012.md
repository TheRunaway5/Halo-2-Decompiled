# TU1.5 startup EEPROM query ownership

The actual settings wrapper `2E0FE2` and import thunk `2E101C` compile to native
code and are connected to their original fresh startup call at `2E00FC`.
`source_run_tu15_process_startup_with_settings` composes the existing original
startup, record, shutdown and hardware-data owners with the new wrapper and
EEPROM resource query. Status conversion301 remains required with its exact
pending frame. No mapping return, timer result, heap or physics result is
supplied. This is another prerequisite for live world creation, not a completed
multiplayer physics system.

English TU1.5 is the sole execution/generation target. The owned executable is
SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The two complete source bodies contain13 instructions. The current comparisons
reach11; the final pop/return await the genuine status-conversion owner.
Production contains explicit compiled arithmetic and calls, with no decoder,
original instruction execution or recorded result replay.

## Actual resource and native boundary

The resource loader reads the user's actual256-byte EEPROM file and retains
only its immutable96-byte UserSettings section. It imports no encrypted or
factory key section. Pinned primary structure declarations independently
establish section offset96, size96, and MiscFlags offset88. The original caller
queries index17 (`XC_MISC`) with a four-byte output and actual type/length
pointer arguments. The new owner authenticates that complete source wrapper
frame before accessing output storage.

The checksum uses the complete pinned primary carry-folding expression over
the actual UserSettings bytes. A valid record returns its actual MiscFlags;
an invalid record returns `STATUS_DEVICE_DATA_ERROR` and retains the untouched
type/value fields. ResultLength is written first in both cases. The invalid
checksum path is exercised explicitly; no settings default or startup-success
value is chosen. Missing writable output ownership throws before publication.

This is a declared native snapshot/query ABI. It preserves other live GP/XMM,
flags and FP state while returning actual query status in EAX and consuming its
complete stdcall frame. It is not original Xbox kernel-clobber, mutable EEPROM
or kernel synchronization proof. The eight-byte hardware input remains the
previous observed modified Complex4627/xemu resource, with its existing scope
and physical-Xbox limitations.

## Independent execution and comparison

`compile_tu15_settings_primary_20261012.py` extracts the complete primary
ExQueryNonVolatileSetting body, lookup table, type declarations, lookup helper
and checksum helpers from pinned Cxbx commit
`585c49a50af1255ab155099e06f24505f9c5a800`. It compiles them independently to
i386 code. The oracle uses an actual i386 atomic lock over its immutable
single-thread snapshot and declared independently compiled byte memcpy/memset
primitives. Their output-write granularity is an explicit native contract,
not recovered Xbox kernel store instructions.

The original-only oracle executes unchanged TU1.5 startup/setup/wrapper/thunk
bytes over fresh literal header/section data, loader-zeroed storage and a
patterned stack. It authenticates182 source instruction sites. Its independent
primary query executes over both the actual owned UserSettings bytes and an
explicit checksum-damaged copy. This damaged record is a declared negative
procedure input, not a live-game settings change. The original EEPROM is not
written.

The48 fresh prefixes cover two entry points, two flag inputs, three stack
alignments, empty x87 TOP0/3 and both resource conditions. They retain2,016
original source instructions,1,416 source writes and1,080 primary platform
writes, including264 EEPROM query writes. Twenty-four query results are actual
success and24 are actual checksum failure. Each reaches original status
conversion call `2E0FFA` with encoded ordinal301 and its full8-byte frame.
The mapper has not executed. The original oracle repeats byte-identically.

Native comparisons check exact source instruction order, ordered source and
primary output writes, all retained banks, pending status frame, GP/XMM,
applicable flags, FS, FP control/status and MXCSR. Additional negative checks
reject unnamed resources, invalid caller frames, unsupported callers and
read-only outputs without changing query outputs or CPU state.

Release and sanitizer cohorts compile the new runtime and actual production
CMake modules. The unchanged shared world archive is copied from the preceding
kernel-data component's freshly compiled Release/sanitizer builds, with its
complete native source/header pins checked. No unchanged heap or world body
is copied into the new generated runtime. Production artifacts are copied into
an immutable snapshot before sealing so later builds do not replace evidence.

Evidence is under
`analysis/multiplayer/sandbox-source/tu15-settings-startup-20261012/`.
The validation report records the exact sources, generator, primary compiler,
repeated original fixture, reused archive inputs, fresh builds and production
snapshot. Earlier evidence directories remain historical.

## Remaining integration

Status conversion301, timer149 and the complete process startup caller remain
required. The source heap global is not installed by these cases. Source world,
BSP, body/contact and controller creation, original tick ordering and solved
readback still need integration into `Simulation::step`. The authored Lockout
lift and Coagulation vehicle acceptance failures remain unresolved. A green
startup comparison supplies no new live physics or full1:1 parity claim.
