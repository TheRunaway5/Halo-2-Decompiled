# Source CPU sound startup and native platform gate

The new `source_sound_startup` owner preserves the original126B20 CPU constructor order, raw pool/storage layout, checksum, controller clock/RNG requests and final availability commit. A real native platform constructor is required at the2205C0 seam; no guest DirectSound result, capability, saved setting, listener, permutation or availability flag is supplied. This module does not yet connect live multiplayer requests to playback.

Authority is the owned disc1.1 image SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` and authenticated1.5 image `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`. Seventeen complete bodies and their internal branches match uniquely, including126B20→127EE0,21BC50→220550,21A6E0→21EFE0 and2205C0→224EC0. Executable bytes are authority; annotations and remembered defaults are unused. [Whole-body correspondence](../analysis/multiplayer/integration/sound-startup-native/paired/report.json), [source listings](../analysis/multiplayer/integration/sound-startup-native/paired/instructions/).

## Required retained inputs

`SoundStartupState` retains the original reference count, static arena base/cursor/checksum, allocator identity, sound-system pointer and pool/backing pointers. Its fields have no invented startup defaults. The required `memory(address,extent)` provider resolves the actual32-bit addresses to real owned backing. Bounds errors stay host-unavailable exceptions. Existing caller-owned records are not relocated or filled to force source admission. [Interface](../include/halo2/sound_startup.hpp), [CPU owner](../src/sound_startup.cpp).

`SoundStartupConstants` contains only `ambient_transition_bits` and the sixteen literal `voice_policy` bytes. The first is read from4457BC→44923C (`db0fc940`); policy451220→455B20 is `28002900010024002400010033000000`. Its signed-short fields are driver40/41/1, static36/36/1, shared51, trailing0. These are source policy, not queried host caps or an SDL steal limit. [Verified operands](../analysis/multiplayer/integration/sound-startup-native/paired/providers.json).

The actual owning services remain mandatory:

- `cache_initialize(state)` is complete21A6E0, including real raw allocation, page/pool fields, physical reservation/protection and its owning dependencies. It is void in source; a missing owner must throw rather than return fake success. The prior original cache freeze owns this behavior; it is not ported by this new module.
- `utc_seconds()`, `crt_random()`, `tick_milliseconds()` execute in that order for21BC50. The CRT callback must consume the real retained CRT stream. The module XORs those actual results into controller+8 and preserves controller+2's lower7 raw backing bits. No seed is assigned.
- `native_platform_initialize(policy)` is the explicitly named host adaptation of2205C0. True must follow actual completed host-owned device/stream setup, false an actual failure. It never substitutes a successful guest Xbox constructor or creates a source driver object.

## Preserved producer order

For references0,126B20 allocates/clears20C and writes policy+79=1/+7D=0; invokes cache; writes the original ambient sentinels/operands; constructs `sound sources`384×BC and clears1200 static backing; constructs `sound playback controllers`64×1C plus raw1C root and consumes the three ordered clock/CRT requests; constructs `looping sounds`128×D4, `sounds effects`16×14 and `sound voices`51×6C; requests actual native platform setup; then initializes the original static voice rows/counts and writes system+78=1. Reference count increments only on a completed source return, including an ordinary false platform outcome. Missing/unavailable services stop before that return. References already nonzero skip constructors and increment only the retained counter.

The static pool helper preserves11E340/16D070/16D0D0/16D220 layout: header4C, actual data/bitmap pointers, allocator word, cookie, name, valid byte, salt, counters and only the original cleared element words. Source static pools have flags0 after their constructor; the general `DatumPool` default flag4 is not silently substituted. Allocation aligns4, advances by the exact padding-plus-size and updates the original CRC over those four little-endian size bytes. String-copy output is exact, while its native byte writes are labeled instruction0 rather than fabricated C-helper instruction identities.

The full caller's +78 commit is behind every prior constructor/platform gate. +7A remains0: it belongs to the later126DE0 active-map owner. Source startup alone cannot admit128380. The native platform queue and numeric parameter modules are separate source/platform evidence; a real device alone does not produce active-map metadata, local listener/RNG, source cache admission, factory, arbitration or completion ownership. [Platform contract](MP_SOUND_PLATFORM_VOICE_CONTRACT.md).

## Original execution and limits

The new oracle independently reexecutes the earlier unchanged heap/CRT/game RNG prerequisites, then the full original126B20 with complete recorded compatibility context and recorded real clocks. Original cache and all CPU constructors execute, and the caller stops at the genuine2205C0 request with source+78=0 and references0. There is no supplied return. This is replay of recorded host context/clocks, not a second live Wine worker or ordinary Xbox thread. The earlier raw static arena protocol remains an explicit input; complete title reservation/allocation order is unproved. [Original report](../analysis/multiplayer/integration/sound-startup-native/measured/report.json), [earlier RNG ownership](SMG_AUDIO_RNG_STARTUP_1_1_RESEARCH.md).

A separate unchanged126C9D..126D39 commit fragment consumes the actual produced prefix and writes its source CPU outputs. Its +78 writer is **not** an accepted whole-device initialization. The native `source_sound_startup_commit_fragment` is similarly explicit and intended for this bounded check; ordinary startup must use the whole owner and real platform dependency.

The native prefix matches the complete23000-byte arena, all retained globals and2,188 ordered represented stores. Native string-copy bytes match complete source outputs; individual C-helper stores are not claimed equal. The separate commit matches all741 original stores and complete arena. Normal and GCC ASan/UBSan checks pass; independent source/export/body comparisons are byte-identical. No successful platform reply is used by the prefix test. Its cache callback retains the independently measured original constructor's zero effects on this module's arena/globals, an explicit fixture dependency rather than a complete native cache implementation. [Tests](../tests/sound_startup_tests.cpp), [audit](../analysis/multiplayer/integration/sound-startup-native/audit.json).

Prior freezes remain unchanged. Preliminary pre-pinning records are retained separately and do not replace canonical evidence. The final manifest pins observed standalone source/binaries and original dependencies, with no claim of a rebuilt complete release or fresh shared archive.

## Reproduction

Use fresh output directories:

```sh
.tools/python/bin/python tools/research_sound_startup_u5.py --output <new-paired-directory>
.tools/python/bin/python tools/oracle_sound_startup_native.py --output <new-original-directory>
.tools/python/bin/python tools/export_sound_startup_fixtures.py --report <new-original-directory>/report.json --providers <new-paired-directory>/providers.json --output <new-fixtures.tsv>
sound_startup_tests <new-fixtures.tsv>
```

The next live owner remains actual cache/platform startup composition and full126DE0 activation, then128380/129450 admission with real local-player bindings and local RNG.127540 resolves the current globals+14 tag, calls2217E0(tag+28), and copies tag+48/+4C/+50 into system+1EC/+1F0/+1F4; it is not a cache reset.192CA0 issues four effect-data requests at indices4..7 from actual environment buffers, then clears its22/24 words. Those owners must remain required rather than being replaced with empty resets or inferred map readiness.
