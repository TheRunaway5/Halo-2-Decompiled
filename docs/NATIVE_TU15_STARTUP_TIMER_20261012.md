# Native TU1.5 startup timer and process heap

The BIOS is a local reference only. Distributed launchers and the native game
must use native platform services and must not require or package a BIOS,
EEPROM image, captured kernel text or kernel tables. This continuation of
the [native desktop startup adapter](NATIVE_TU15_STARTUP_PLATFORM_20261012.md)
keeps that boundary: the only original image input is the locally imported,
user-owned English TU1.5 title executable.

## Implemented path

`NativeTu15StartupTimer` owns a real Linux nonblocking, close-on-exec timerfd.
The original TU1.5 caller initializes its timer and DPC, queries the native
application preference, and submits its actual signed six-hour relative
request. The adapter retains the source timer record and its owned insertion
links. Rearm returns the previous insertion state; cancel disarms the actual
resource and removes those links. Expiration is consumed once, signals the
source record and passes the retained callback and real system-time arguments
to the application's serialized source executor. Destruction closes the file
descriptor and releases the owned head without executing the callback.

The callback is the complete four-instruction original TU1.5 power DPC,
compiled to native C++. It consumes the power preference produced by that
same startup. Its false branch returns; its true branch still requires the
application lifecycle owner for import360. No host shutdown is performed by
this component. Timer waiters, foreign DPCs, absolute/periodic requests and
other platforms remain outside this adapter's supported domain.

The composed full startup now executes the original heap constructor over
actual native reserved and committed pages. The original caller publishes
the resulting process heap at56EE28. The comparison does not install that
global manually. Execution next reaches original volume validator2DBA2D,
called at2DBAA3 with the complete pending frame
`a8ba2d0038de4600`. The required volume/filesystem owner is still explicit;
no successful volume result is supplied.

The native ABI preserves CPU state outside the documented return/stack
effects of each adapted platform call. It does not claim original Xbox
kernel register clobbers, list scheduling or kernel write granularity.
Complete process entry requires a clear direction flag. Earlier original-only
DF-set attempts corrupted the original copy/return path; those failed cohorts
are retained. No CLD patch or forced successful return was added.

## Evidence

The unchanged original TU1.5 procedure starts from a fresh loaded image,
patterned stack, BSS, source FS chain and actual reserved/committed host pages.
Independent pinned primary record, registry and critical-section bodies supply
those compatibility operations. The application configuration, clocks and
native timer ABI are declared inputs rather than Xbox kernel observations.
The original-only run does not read native expected outputs or captured heaps.

Forty-eight fresh cases cover full process entry and timer setup entry,
both native power preferences, two DF-clear flag states, three stack alignments
and empty x87 TOP0/3. Comparisons cover31,656 original instructions,
13,008 ordered source stores,1,008 primary-owner stores,672 native
configuration/timer stores,48 virtual-resource requests, complete pending or
returned ABI, and all121,536 final bank bytes. Twenty-four setup calls return;
the24 full entries create and publish their heap before the pending volume
call. The original DPC adds144 instruction observations and24 stack stores.
Independent original and generator repeats are identical.

Separate real-resource tests cover six-hour arm/rearm, previous-state returns,
cancel, invalid-frame rejection before mutation, a real five-millisecond
expiry, required callback ownership before consuming expiry, both compiled
original callback branches, no expiry replay and descriptor/list retirement.

The frozen production CMake cohort passes all three selected tests in each
Release and ASAN/UBSAN/leak build, without skips. It compiles the actual native
modules with strict FP options and reuses only its pinned, previously validated
world archive after verifying all compiler-discovered archive inputs. A real
file/syscall trace records timerfd creation, scheduling, querying and polling,
and confirms no BIOS, EEPROM or captured kernel reference file is opened.

Evidence directory:
`analysis/multiplayer/sandbox-source/native-tu15-startup-timer-20261012/`.
Frozen audit: `validation-frozen-v1.json`, SHA-256
`da346f935ff8a9b22990eee5efac58d1160831c8c010c7ebe972fdf4b1eda782`.
Source manifest SHA-256:
`a6514bb836c0869c3f70ca7d20cee775e6f0058c33e8f690e28e12177e533bdd`.

The initial live-checkout audit rejected four concurrent shared-file changes:
`cache.hpp`, `tags/binary.hpp`, `tags/types.hpp` and the image-loader test.
They are preserved. The separate frozen audit validates the actual compiled
snapshot and records both hashes for each difference. It verifies that the
new timer implementation, callback, tests, CMake module and original procedure
still match that snapshot. Its result does not validate those later shared
changes or claim a complete current game build.

## Remaining integration

This is a tested startup component, not an updated playable executable.
The volume/filesystem service, full startup return, ordinary allocator and
renderer completion, populated world/body/controller construction and live
tick/readback integration remain required. The Lockout lift and Coagulation
vehicle acceptance failures remain open. The complete multiplayer and
physics-engine work is unfinished.
