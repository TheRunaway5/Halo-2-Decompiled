# Original mode3 weather plate pixel-program production

`source_weather_plate_program` reproduces the parameterized mode3 producer in
retail `0003A5C0`. It returns the original 60 pixel-definition words and the
optional selected depth. It accepts the actual 52-byte compositor record,
explicit start/end and near/far values, original low-byte flags, and arithmetic
controls. It does not substitute the one captured Lockout definition for other
inputs. The unchanged-original differential produced 1,651 distinct successful
definitions.

Authority is `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The full routine is retained in
`analysis/multiplayer-shading/weather-draw-lockout-state1-inputs/source-0003A5C0-disassembly.txt`.
The CPU reference is the immutable `lockout-weather-particle-entry` snapshot,
RAM SHA-256 `f76d0569dedcd2e94d3f2ee4c67a97db47cefea848105a8b24fe08342daf24cd`.
No shader semantics were inferred from generated GLSL or a map name.

## Literal source behavior

The record comprises one consumed flag byte, three ignored padding bytes and
four float3 triples at offsets `4,10h,1Ch,28h`. The record flag selects alpha
versus RGB texture mappings. All four caller flags likewise inspect only their
low byte. In particular, `0x100` is false.

`3A5DA..610` first rounds `1/far` to binary32 and then separately multiplies the
start and end. `3A67B..6CB` floors each endpoint at precision 65,536, converts
through signed32 and rounds the result back to binary32. A nonpositive
quantized range returns false before changing the old definition or optional
depth. The reciprocal span is rounded separately and mode3 caps it at the
literal 128. If capped, the original precedence is center, anchor end, retain
start; no selected anchor returns false. A negative selected start requires
both end and start flags before it can become zero.

Successful mode3 selects reciprocal class `0..7`. The implementation preserves
the original class-dependent combiner words, x87 binary64 intermediate
operations, binary32 stores, and floor/clamp of the slope bytes. It applies the
second start-depth packing floor separately. Source `131ED0` multiplies raw
color components by 255, rounds signed integers nearest-even, and masks each
with `FF`. It does **not** clamp the four input triples; `37A60`'s earlier
caller clamp is a separate producer. This module preserves raw byte wrapping.

Optional depth uses the selected floating start before that second packing
floor, multiplied by actual far and clamped upward to actual near. For example,
the centered `10..10.046875` range at far1024 returns `6.0234375`
(`40C0C000`), whereas reconstructing its packed endpoint would give
`6.015625`. A null output request creates no depth output.

## Admission and ownership

This pure producer admits mode3 with the `skip_state` low byte nonzero. Modes
0,1,2, unproved modes and raster-state execution are rejected explicitly.
Successful original execution still installs the definition through
`3B81C → 1CCF0 → 3FA3D0`; SDK installation is a separate integration boundary
and is not a side effect of this functional C++ API.

Source and host arithmetic require nearest-even, source x87 PC53 and gradual
SSE handling. Compilation enforces `FLT_EVAL_METHOD==0`, IEEE binary32/64 and
no fast-math, with separate binary32 stores and disabled contraction. Sticky
status bits and the observed reserved x87 bit difference
are accepted; different rounding, precision, exception masks, flush/DAZ or
reserved MXCSR bits are rejected.
Host gradual-control inspection is implemented for x86 SSE, including the
MSVC x86/x64 preprocessor branches; other hosts are explicitly rejected until
their control inspection is independently proved. Linux x86-64 is the measured
host here; Windows/MSVC and other hosts were not executed in this task.
The module requires finite positive far,
finite supplied floats and finite derived values. Depth floor conversions and
rounded color conversions must fit signed32; overflow/NaN/Inf classification
is explicitly unproved. Near can exceed far and raw colors can exceed `0..1`;
no invented clip/tint/alpha limit is applied.

The result contains an optional `PixelProgram` and optional depth. An original
false return yields an empty result, allowing the caller to preserve the prior
banks. Unsupported classifications throw `std::invalid_argument` before any
caller-owned output can be assigned. No simulation, resource readiness,
texture ownership, view history or runtime activation is supplied here.

## Complete unchanged-original differential

`tools/differential_source_weather_plate_program.py` runs whole original
`3A5C0`, including its real SDK installation callees, in an owned CPU clone.
Every executed instruction byte is checked against the immutable executable;
there are no patches, replaced services or manufactured callee returns. All
nine captured SDK/program/global/command pages are restored for each
independent call. Write tracing confirms that all six externally written pages
are in that restored set. The original emulator and GPU are never controlled.

The 2,440 cases compare success, depth presence, the depth bit pattern and all
60 words: **153,720 equal output words**. There are 1,780 source successes,
660 source false returns preserving `C0DE0000+i` program sentinels and
`DEADBEEF` optional depth, 1,240 optional depth outputs and 764 explicit null
pointer cases. All eight reciprocal classes execute. The original calls
`131ED0` 7,120 times and each installation callee 1,780 times, verifying 642
distinct executable instructions.

Inputs include the actual prior391F0 Lockout compositor and actual
near/far words `3C800000/44800000`; the resulting full definition equals the
retained original bank and optional depth is `41200000`. Lockout and Warlock
weather plates are independently decoded from their authored caches: depth
ranges `10..30` and `18..30`, first tint `1` and `0.6509799957`, and opacities
`0.5/0.8/0.9` and `0.3/0.4/0.5`. Explicit diagnostic intensity factors
`0,.25,.5,1` shape the source caller's record; these factors are not claimed as
historical runtime weather intensity. Additional controls cover quantization,
class boundaries, all anchor combinations, half steps, null output, low-byte
flags, nearest-even wrapping and 2,048 seeded bounded finite inputs.

The retained original single-baseline proof was also corrected to restore
actual near485658. All60words, optional depth and 17 captured constants stayed
identical. Its prior files and correction ledger are preserved at
`weather-plate-original-reference/history-before-near-restoration/` and
`near-restoration-correction.json`.

## Reproduction and checks

Build the CPU-only module/test with:

```sh
bash tools/build_source_weather_plate_program.sh analysis/multiplayer-shading/weather-plate-program/build
analysis/multiplayer-shading/weather-plate-program/build/source_weather_plate_program_tests tests/fixtures/retail_source_weather_plate_program.tsv
analysis/multiplayer-shading/weather-plate-program/build/source_weather_plate_program_tests --admission
```

The build needs no Vulkan, SDL, cache archive or graphics context. The same
helper accepts `asan` as its second argument. Normal and Address/Undefined/
LeakSanitizer runs pass the full frozen fixture and 30 unsupported-admission
controls, including active host DAZ/FTZ rejection. Empty, malformed, truncated,
trailing and missing fixture inputs
are required failures. `--probe` accepts exactly 25 hex input words per row and
prints 63 output words; an empty probe also fails.

The fixture header documents word order. Source failures retain explicit prior
sentinels; they are not replaced with zero-filled definitions. The frozen
original fixture, differential JSON, test logs and file hashes are in
`tests/fixtures/retail_source_weather_plate_program.tsv` and
`analysis/multiplayer-shading/weather-plate-program/`.

The complete original replay can be regenerated with the bundled Python and
`tools/differential_source_weather_plate_program.py --binary <test-binary>
--output <report.json> --write-fixture <fixture.tsv>`. The private
`analysis/multiplayer-shading/weather-plate-program/verify-controls.py`
reproduces positive, malformed-input and compiler/control checks: 36 expected
outcomes, including 29 required failures. Forced GNU preprocessor checks cover
the unproved-host rejection and both MSVC-named x86 branches; those checks do
not establish Windows/MSVC or physical32bit/nonx86 execution.

Parent integration registers this helper in `halo2_weather_simulation` through
`cmake/SourceWeatherSimulation.cmake`. Its x86 test path includes the plate
fixture and admission checks. The parent private CMake support project at
`weather-cpu-cmake-integration/` passes all four registered particle/plate CPU
checks at `-O3`, including these 2,440 plate cases and 30 plate controls. That
support project checks registration and the owned CPU producers; it is not a
live weather frame or a full application rebuild.

This is CPU word/depth parity for the admitted mode3 domain. It does not extend
the frozen observed-packet Vulkan renderer's admission, establish original
NV2A raster arithmetic, prove live weather producer chronology, or establish a
native Warlock frame. The separately measured plate scene remains documented
in `SOURCE_WEATHER_PLATE_GPU_PARITY.md`.
