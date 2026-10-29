# Native campaign cinematic reset and capture acknowledgement

The semantic TU1.5 `1FCB0` reset and `1FCE0` capture acknowledgement are implemented in [the native screen lifecycle module](../src/campaign/native_campaign_screen_lifecycle_tu15.cpp), behind [a memory interface](../include/halo2/native_campaign_screen_lifecycle_tu15.hpp). There is no virtual CPU state, executable image, instruction decoder or lifted fallback in either algorithm.

Reset reads the actual cinematic interface, clears its 255 words in order, then reads the actual texture-target binding and invalidation byte. A missing interface leaves target state untouched. A present target with a clear invalidation byte gets one byte set; an already invalidated target is unchanged. Target lookup happens after the reset, so overlapping banks retain original behavior. Acknowledgement clears only the interface's capture byte at `+A0` and is a no-op for an absent interface.

The direct campaign wrappers call guarded `PhysicsLiveWorldTu15` methods. They retain executor, publication, mutation and continuation-fault checks instead of borrowing writable memory outside the world call. Native void journal IDs 33 and 34 record/replay these mutations without inventing old volatile CPU return values; IDs 1–32 remain unchanged. The shared decoder checks complete empty-argument/empty-result schemas before invoking either world mutation.

The world's original-address dispatch now also selects [the native entry transport](../src/campaign/native_campaign_screen_lifecycle_tu15_binding.cpp) for `1FCB0`/`1FCE0`. It calls the semantic kernels, reads the actual return word and consumes four caller bytes; it does not reconstruct volatile registers, flags or retired callee stack scratch. [Authenticated original caller continuations](../analysis/campaign-native-screen-binding-20261028/callers.json) establish the void contract, including downstream consumers.

## Independent original and native proof

The [physical reference generator](../tools/oracle_native_campaign_screen_lifecycle_tu15.py) executes unchanged English TU1.5 i386 instructions offline on the host CPU. It authenticates image SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`; it neither patches instructions nor supplies callee outcomes. The two routines are call-free.

- [Original proof](../analysis/campaign-native-kernels-20261028/screen-lifecycle/original.json): 42 complete calls and 16 actual read/write prefixes, covering all 21 original instruction sites. Cases include absent/present interfaces/targets, dirty bytes 0/1/2/255, unaligned banks and aliasing the target/binding globals.
- [Exact original repeat and cleanup aggregate](../analysis/campaign-native-kernels-20261028/screen-lifecycle/proof.json): the compressed original reference regenerates byte-for-byte and all eight recorded private scratch/build paths are removed.
- [Root CMake regression](../build/test-results/campaign-native-20261028-screen-lifecycle-v2/report.json): ordered reads/stores, complete retained banks, fault prefixes and unchanged native FP state.
- [Clang ASan/UBSan and mutation proof](../analysis/campaign-native-kernels-20261028/screen-lifecycle/clang-sanitizers/report.json): the original fixture rejects private source mutations that omit the final reset word or clear the wrong acknowledgement byte.
- [GCC and caller compilation](../analysis/campaign-native-kernels-20261028/screen-lifecycle/gcc-callers-v2/report.json): the same native/mutation cases pass; world, screen-wrapper and journal-decoder translation units compile. This is an object/caller check, not a newly linked full game.
- [Native journal proof](../analysis/campaign-native-screen-journal-20261028/proof.json): actual native reset/ack mutations replay identically. Normal and sanitizer checks reject unknown IDs, trailing arguments, forged results, reordered or unconsumed calls and untracked writes before accepting publication.
- [Original-address transport proof](../analysis/campaign-native-screen-binding-20261028/proof.json): the 58 original semantic cases pass through the actual binding, along with two missing-RET controls; normal/sanitizer and Clang controls reject incorrect SP consumption or an invented void EAX return.
- [Final current caller compilation](../analysis/campaign-native-kernels-20261028/screen-lifecycle/final-caller-compile/report.json) compiles the world, wrapper and decoder after native entry routing. It is explicitly a build-only object check, not repeated runtime acceptance.

The required compressed original data fixture is [30,212 bytes](../tests/fixtures/native_campaign_screen_lifecycle_tu15.json.gz). It contains original input/output data, not executable instructions. Compiler products, physical reference ELFs, raw banks and decompressed fixtures are owned by the official disposable runners.

## Scope and continuation

The ordinary compiler ABI has a clear direction flag; this typed reset is not a general x86 `REP STOSD` emulator. [Full timed-screen update and depth-of-field setup](CAMPAIGN_NATIVE_SCREEN_TIMED_TU15.md) are now semantic native kernels too. The production graph replaces the old four-body generated screen module with a thin native compatibility entry. Other cinematic controllers, composed campaign replay and matched original presentation retain separate acceptance gates.

Run the retained original regression with:

```sh
python3 tools/run_tests.py --target native_campaign_screen_lifecycle_tu15_tests \
  --filter '^native_campaign_screen_lifecycle_tu15$' \
  --output build/test-results/campaign-screen-lifecycle
```

Run the isolated original/mutation proof using the required scratch owner:

```sh
python3 tools/run_test.py -- python3 tools/validate_native_campaign_screen_lifecycle_tu15.py \
  --compiler clang++ --sanitizers \
  --output analysis/campaign-screen-lifecycle-recheck
```
