# Shared original round-reset owner and owned-world probe

The shared native reset owner now preserves the complete original round-reset
control flow and player/object iteration. Its228 CPU comparisons cover retail,
TU1.1 and TU1.5, including33 stopped service calls. A separate unchanged TU1.1
probe executes the authored Elimination full reset with a source-joined player,
actual constructed biped and attached SMG. Both objects are really deleted
before the reset reaches its remaining sound/scenario dependencies.

This is progress toward the vanilla data-driven game-mode goal. The new owner
is compiled into the native systems target but remains unbound to the complete
world/scenario services. Neither source probe completes a full world reset.
No additional variant, round setting or weapon path gains runtime admission.

## General reset policy

| Original owner | Retail | TU1.1 | TU1.5 |
|---|---|---|---|
|Full round-reset control |`159E20`|`15B460`|`15C840`|

[The native owner](../src/source_round_reset.cpp) receives the actual mutable
global slots and raw-memory service. It preserves the original variant flag
at game globals+184 bit3, raw game-mode byte, pool bitmap/high-water iteration,
signed16 object cursor, full salted handles and original field widths.

For ordinary modes, it first deactivates and deletes every occupied player's
current unit. The subsequent object pass uses the original type mask, walks
parent ancestry and checks both the object and root placement index. Weapons
resolve their definition through the real tag table and read the original
word at definition+290; category0 is removed, while other categories survive
that branch even during a full reset. Type11 also consumes the original object
flag at bit14. Mode4 has its own source-backed full-reset branch for unparented
objects without a replication entity. No named mode, map or weapon lookup
implements these decisions.

For a full reset, the owner then calls decal cleanup, reloads and fills the
4201-byte mask in the original DWORD/byte order, and calls contrail, effect,
registry and light cleanup. A present selected engine requires sound cleanup.
Scenario reset receives0, the scenario callback receives the reloaded raw game
type, and scenario placement runs last. Missing reached services or backing
throw; there are no successful fallback callbacks.

[The independent CPU oracle](../tools/oracle_round_reset.py) executes the whole
original reset and its actual player/object iterators. Its world-service leaves
are declared API inputs: deletion recursively retires declared test headers
and clears matching current-unit fields. One decal callback redirects the mask
global to expose the caller's reload. These leaves test caller policy and
feedback; they do not implement original deep object teardown and are never
installed in the production match.

The native comparison checks all retained banks,19,054,416 bytes,523,530
ordered writes and4,383 reached service calls. It covers sparse headers,
object types including the original low-five-bit shift behavior, ancestry,
weapon category words0,1,FFFF and8000, both full-reset settings, engine presence
and mode1/4 branches.33 additional cases remove each reached callback entirely
and reproduce the original partial state before the missing owner. Missing
raw memory also stops. Six complete instruction comparisons establish source
correspondence for the reset and both iterators across the three executables.

## Actual joined-player reset

[The owned-world probe](../tools/oracle_owned_round_reset.py) retains the earlier
actual joined-player and constructed-body preparation. It adds original mask
and complete decal storage initialization in their selected callback order.
The original decal VirtualAlloc request receives actual mmap-backed raw pages;
its original constructor then owns the cache fields. The earlier separately
bound vertex-cache API fixture is not used by this probe.

Original initializers19/20/22/25/27/28/30/34/37/39/40 and full AI58/physics59,
their selected resets, and the retained mask/activation/world APIs execute.
These selected APIs retain table order but are not whole title/session startup.
The live compatibility KPCR/ETHREAD, CRT/process heap, physical backing,
cache/resource delivery and optional creation boundaries retain their earlier
documented limits. The worker remains alive while its original context is used.
No Xbox hardware or complete platform allocation-order claim follows.

The unchanged authored row7, including its original private controller glyph,
comes from `mainmenu.map` and the original variant initializer/setter contract.
Its304 bytes are delivered as the reset's configuration input. The full-reset
bit stays enabled, and the buffer is unchanged before/after the reset. Actual
14D350 joins playerEC700000; complete1509C0 selects an authored Lockout start,
constructs its biped and SMG, and binds the current unit through the original
ownership path. Sign-in/session transport remains an explicit API boundary.

Two fresh probes each enter complete15B460 with those live owned objects:

- With sound uninitialized, the source deletes both objects, resets decals,
  masks, contrails, effects, registry and lights, then stops at127B55 reading
  NULL+78. Object/component/physical counts have returned to zero.
- A separate original sound-initialization prefix creates its real cold
  system context. An independent reset on that prefix deletes both objects,
  passes127B50 through the original unavailable-sound gate and enters D6B60
  scenario reconstruction. It stops at16D2C3 reading NULL+34 when18A9C0 needs
  the missing object-looping-sound pool. One scenario object has already been
  partially allocated; it is not a completed reconstruction.

There are four actual object deletions across the two probes. Player current,
previous and watched references are NONE afterward, while the joined player
record survives. During reset every inherited provided gameplay/resource leaf
is guarded before its hook could return. No reset gameplay/resource service is
supplied. Original object/world/physics callbacks, recursive weapon retirement,
body-heap release and datum deletion execute. Whole guest `.text` is unchanged.
The retained instruction listing records observed original addresses, including
stopped instructions and host/platform boundaries, rather than Xbox hardware
instruction retirement.

## Sound prefix and next owners

The sound probe uses only the exact original12008..12025 static-arena
reservation fragment, with EBX0 from the preceding original12001 instruction.
It excludes the preceding disk/partition call and the later whole12000 startup.
Actual host mmap backing satisfies the fragment's raw VirtualAlloc request;
original instructions set the static pointer/cursor and construct the sound
system. New cache VirtualAlloc requests also receive only owned raw backing.
No sound readiness, voice, cache entry or success field is supplied.

Whole126B20 stops at the actual kernel address-protection request through
417BA8, returning to21A7A3 with arguments82A47000,300000,4. No platform return
is supplied there. Its earlier original clear leaves system+78 at0. The later
reset is an independent source API experiment on this cold prefix, not a
resumed initializer or proof of successful audio startup. Device creation and
the later availability-setting instructions are not executed.

The next scenario stop has an exact original owner: table row46 initializes
the1024x18 pool through18A200 and resets it through18A2A0. Its original source
name is `object looping sounds`, and18A9C0 loads the missing global4F485C
before allocating. The initializer/reset are source-listed next dependencies,
not executed or supplied in this probe. Their real downstream audio contexts
must be recovered before treating scenario reconstruction as complete. C1870
and D6C40 remain unreached here.

## Reproduction and validation boundary

```sh
.tools/python/bin/python tools/oracle_round_reset.py
.tools/python/bin/python tools/oracle_round_reset.py \
  --output analysis/multiplayer/game-variants/round-reset-native/reproduced/report.json \
  --fixture analysis/multiplayer/game-variants/round-reset-native/reproduced/fixture.json \
  --verify-fixture tests/fixtures/round_reset_original.json
.tools/python/bin/python tools/oracle_owned_round_reset.py
.tools/python/bin/python tools/oracle_owned_round_reset.py \
  --output analysis/multiplayer/game-variants/owned-round-reset/reproduced/report.json
cmake --build build --target round_reset_tests \
  CMakeFiles/halo2_systems.dir/src/source_round_reset.cpp.o -j2 \
  > analysis/multiplayer/game-variants/round-reset-native/build.log 2>&1
ctest --test-dir build -R '^original_round_reset$' --output-on-failure \
  > analysis/multiplayer/game-variants/round-reset-native/ctest.log 2>&1
c++ -std=c++20 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Iinclude tests/round_reset_tests.cpp src/source_round_reset.cpp \
  -o analysis/multiplayer/game-variants/round-reset-native/round_reset_tests_asan \
  > analysis/multiplayer/game-variants/round-reset-native/sanitizer-build.log 2>&1
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
  analysis/multiplayer/game-variants/round-reset-native/round_reset_tests_asan \
  tests/fixtures/round_reset_original.json \
  > analysis/multiplayer/game-variants/round-reset-native/sanitizer.log 2>&1
.tools/python/bin/python tools/verify_round_reset.py
```

The CPU report/fixture and owned-world report reproduce byte-for-byte in
independent runs. The configured focused native check and direct ASan/UBSan
build with leak detection pass228 records and33 absent-service checks. The new
systems translation unit compiles with the configured production flags.
The verifier checks original bytes/disassembly, full source correspondence,
raw hashes, exact repeats, source-owned deletion/counts, exact dependency stops,
authored configuration preservation, native results, source freshness and
artifact pins. No fresh whole production link/startup is claimed for this slice.

Complete native world/physics/scenario binding, normal sound initialization,
round preparation/replication and full edited-setting local/network matches
remain required. The goal stays active: ordinary Slayer/Team Slayer startup
admission remains the retained boundary, with37 other built-in rows gated.
