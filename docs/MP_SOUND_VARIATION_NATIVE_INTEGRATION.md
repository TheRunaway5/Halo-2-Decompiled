# Native sound variation integration boundary

The native resident-metadata importer and original gain, pitch and permutation
kernels match the frozen disc 1.1 sound adapter observations. This is a bounded
code integration into `halo2_cache`, not audible playback or accepted ordinary
scheduler admission. The primary authority remains the unchanged
[sound adapter study](SMG_AUDIO_ADAPTER_1_1_RESEARCH.md).

`Cache::load_sound_variation` imports the actual `snd!`, `ugh!` and `sncl`
records, including the runtime last/forced bytes and used bank. It resolves
metadata addresses through the cache metadata translator; resource-file encoded
addresses are a different namespace. No map, sound, original executable, chunk
or copied metadata blob is embedded in the native code.

`source_sound_gain` follows original `1269B0`: always advance the local LCG;
multiply the high16 result by the original `4650C0` scalar; spill that draw to
float32; preserve the separate variance subtraction, multiply, zero addition
and authored base addition. `source_sound_pitch` follows `126A40`: always
advance, read signed-short pitch bounds, retain the source 53-bit calculation
until its caller's float32 store. These helpers require an explicitly supplied
local seed; they do not initialize or replace the global audio RNG.

`source_sound_permutation` follows `21B180` and its measured helper path. It
consumes a forced index without a random draw, otherwise draws candidate
indices, rejects already used indices, preserves the last selected index on an
all-used reset, performs the authored skip test with its own random draw, and
uses the original `count*2` skip retry limit. Class bit5 suppresses persistent
forced/last/used state. Single-permutation ranges do not commit a used bank.
The native store trace retains the source instruction, before seed and after
seed for every actual draw. The wrapper uses the original zero invert/optional
output inputs; other helper callers remain separate.

Validation:

- 17 authored sound definitions, playback/class/range records and 23
  permutation records compare byte-for-byte with independent original captures.
- 32 successive ready/reload selector calls compare selected index, runtime
  last/forced state, used bank, before/after seed and every ordered LCG store.
- Three direct original creation slices compare gain at `source+94`, caller
  pitch store at `+90`, seed snapshot at `+98`, selected permutation at `+9D`
  and final local seed. These source call sites explicitly bypass unavailable
  outer admission; they are not successful playback fixtures.
- All 83 random-state stores match in the ordinary and address/undefined/leak
  sanitizer comparisons. The normal and sanitizer CTest checks each pass1/1.
- A fresh disposable source copy builds without original assets, executables,
  analysis exports or downloaded reference tools. Its55 tests contain18 passes
  and37 expected original-data skips. This snapshot includes the new sound
  module; later shared rendering work has its own checks.

Reproduction uses the locally extracted Lockout map and update executable:

```bash
cmake -S . -B build-multiplayer -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-multiplayer --target sound_variation_tests -j 4
python3 tools/differential_sound_variation.py
ctest --test-dir build-multiplayer -R '^retail_sound_variation$' --output-on-failure
```

The exporter pins the independent original oracle, suite, fixture, audit,
document, prior authority and instruction listings. Its expected values are
literal copies of original observations, not values computed by the native
implementation. Reports and the generated TSV remain under ignored `analysis`.
The CTest check skips with77 when the source map/fixture is absent.

Measured pitch bounds and permutation skip fractions are zero in these stock
cases; class bit5, nonzero pitch interpolation and skip retries beyond used-bit
rejections are inspected instruction branches without executed coverage here.
The bounded importer accepts counts1..31; invalid/larger counts are explicitly
unrecovered. The original configured FPCW027F/MXCSR1F80 context is a supplied
fixture, not recovered Xbox hardware initialization. Only Linux x86_64 native
and sanitizer comparisons were executed.

Production FP callbacks, complete audio system/map activation, seed
initialization and global draw ordering, real cache/voice arbitration, listener
and world producers, decoding, host stream completion and audible parity remain
integration gates. Native gain/permutation helpers cannot turn availability on
or manufacture a successful sound handle. The separate original storage/cache
provider study is progressing against real allocation and resource reads.
