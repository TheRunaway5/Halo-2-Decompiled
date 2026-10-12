# Shared sound cleanup for round resets

The native cache library now includes the original sound-source stop and round
cleanup owners. Cleanup consumes the retained sound/controller/effect/cache
pools and their actual data fields. It has no variant-name or weapon-name
exceptions. Its reached source callback and native audio services are required;
an absent service stops the owner instead of reporting success.

This is a prerequisite for the complete world reset. It does not supply normal
sound startup, map activation or the other reset services, and is not yet bound
to later native rounds. Elimination and additional variant admission remain
gated. The earlier unchanged original reset probe still retains its missing
sound-context stop.

## Original authority and behavior

| Owner | Retail | TU1.1 | TU1.5 |
|---|---|---|---|
|Round sound reset|`126400`|`127B50`|`128F10`|
|Release effects|`21D4D0`|`21F5E0`|`223EE0`|
|Stop source|`127320`|`128A70`|`129E30`|
|Release static voice|`12A5D0`|`12BD20`|`12D0E0`|
|Unpin source cache reference|`126960`|`1280B0`|`129470`|
|Remove source|`127390`|`128AE0`|`129EA0`|
|Detach controller|`219C80`|`21BD90`|`220690`|
|Release shared voice|`21E050`|`220160`|`224A60`|
|Stop driver stream|`2AE4B0`|`2B47C0`|`2B81C0`|
|Free datum|`16BA40`|`16D4C0`|`16E8A0`|

These ten complete owners have20 cross-image shape comparisons, including
branch topology and relocated operands. The native implementation is
[sound_source_lifecycle.cpp](../src/sound_source_lifecycle.cpp); its raw owner
contract is [sound_source_lifecycle.hpp](../include/halo2/sound_source_lifecycle.hpp).

The round owner reads system+78. When enabled, it releases each occupied effect
and its controller reference, then iterates the original source pool. Sources
whose byte2 is zero stop with reason10; nonzero byte2 remains persistent. Source
callbacks may change the pool global, which the owner reloads after each stop.
Pool traversal uses the actual bitmap and signed high-water field, rather than
assuming a contiguous occupied array.

Stopping a source releases its signed16 static voice index when present,
decrements original cache references and clears the cache-pin bit. Cache lookup
retains the original signed16 tag/range/permutation/chunk offsets and signed8
source selectors. An actual nonempty callback receives its original context,
source datum and reason before source removal. Source removal updates controller
reference and byte3 fields and frees the salted datum in the original order.

Static voice cleanup releases both retained cache-definition references. It
stops an existing driver stream, clears the original stream bit and releases
the shared voice reference. Shared reference counts retain16-bit width and preserve
wrap; controller references preserve their upper flag bit and wrap only the
lower seven bits. Datum deletion preserves bitmap/count/cursor/high-water
updates and the original optional BA poison pattern.

The complete represented stream-stop caller needs actual retained buffer
status, conditional Stop, StopEx at time0 with flags3, and the retained stream
flush method. This module does not produce those identities or statuses. No
positive platform/callback execution is claimed by the differential suite.

## Startup and probe boundaries

Original TU1.5 process startup `12000` requests its23000-byte static arena with
VirtualAlloc `(0,23000,1000,4)` before later initialization, including sound.
Whole process startup and that platform allocation are not executed here.
The existing [native sound storage/startup work](MP_SOUND_CACHE_STARTUP_NATIVE_INTEGRATION.md)
already represents real OS backing and the portable CPU startup/cache owners.
Their ordinary match binding remains required.

[The oracle](../tools/oracle_sound_source_lifecycle.py) retains the independently
captured complete TU1.1 startup-prefix arena, SHA-256
`b2a1175e7dd524fc7d9587333e8055abd8cb4d30f6417e0ff293e9d1e4444287`.
That original caller stopped before its device constructor. Reusing its layout
as raw API backing in retail/TU1.5 is an explicit composition, not complete
startup in those images. Availability and occupied record fields are declared
controls. Pool slots use the original indexed allocator and genuine salts.
Additional metadata/cache/driver records are raw API inputs; they do not
establish authored audio selection, a resident chunk or cache readiness.

The suite executes225 calls:216 complete original returns and nine retained
dependency stops. It covers sparse slots through383, persistent byte values,
controller low-seven-bit edges, debug poison, signed metadata selectors,
zero/one/two shared-voice references and direct stop reasons. Original nonempty
callbacks stop before the indirect call. An unavailable hardware buffer stops
at its original NULL read. No instruction or service return is replaced.

The native comparison checks all225 outcomes, complete retained regions
(61,756,200 compared bytes) and all51,633 ordered writes, including the exact
partial state before those stops. Portable observer writes have instruction
identity0; they are not assigned invented x86 PCs. The fixture is
[sound_source_lifecycle_original.json](../tests/fixtures/sound_source_lifecycle_original.json),
SHA-256 `52564fbbed3de8e5915bb82d01d25fc4cbea1d3be52eb6bb9601ef4176e7270a`.
An independent original execution reproduces the complete fixture byte for byte.

```sh
.tools/python/bin/python tools/oracle_sound_source_lifecycle.py
.tools/python/bin/python tools/oracle_sound_source_lifecycle.py \
  --fixture analysis/multiplayer/game-variants/sound-source-lifecycle/reproduced/fixture.json \
  --output analysis/multiplayer/game-variants/sound-source-lifecycle/reproduced/report.json \
  --verify-fixture tests/fixtures/sound_source_lifecycle_original.json
cmake --build build --target sound_source_lifecycle_tests -j2
ctest --test-dir build -R '^original_sound_source_lifecycle$' --output-on-failure
```

Both the implementation and test translation units also pass ASan/UBSan with
the complete original fixture. Eight focused checks pass: the new owner,
existing startup/cache/looping/map-activation/submit owners, shared round phases
and native game ending. The rebuilt production executable advances90
four-player Lockout ticks. That startup check does not exercise this unbound
cleanup owner or establish a complete match.

Normal sound owner production/binding, actual accepted callback and platform
cleanup, constructed live object deletion, the scenario/placement reset tail
and complete local/network match flows remain required by the operational goal.
