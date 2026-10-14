# TU1.5 local model light vertex inputs

Authority: the unchanged owned OG Xbox TU1.5 executable, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`,
and original Xbox cache assets. Vista is not used. These results establish
bounded resource/declaration and vertex-output behavior, not complete rendered
gameplay frames or physical Xbox GPU precision.

## Native implementation

The raw model geometry owner now retains rigid-boned and skinned sections,
including original interleaved position, bone-index and stored-weight bytes,
alignment gaps, node maps and index order. TU1.5 uses byte UNORM input for both
indices and weights. The adapter neither normalizes the stored weights nor
completes their first component on the CPU: the original compiled vertex
program owns its weight arithmetic.

The raw binding admits original compiled classes2..6 with the actual full
object identity and caller-owned current palette. Geometry class, available
influences and palette extents are checked before submission. Static vertex
generation adds the original local input registers, ARL and relative c50..169
palette reads while preserving the existing class0/1 emitter's restrictions.
Generated catalog ABI2 identifies this extended contract; all78 admitted
modules are compiled and their compact UBO/input/output ABI is checked.

An original compression flag was also incorrectly rejected. Cairo character
sections author flags7; unchanged TU1.5 `1CDB0` tests only the low position/UV
bits. The native compression bank now follows those guards. The independent
original prefix comparison stops immediately before device submission, without
replacing any arithmetic instruction or result.

The focused regression sweep also found a pre-existing contract inconsistency:
raw model bytes were exposed through the BSP-only `raw_header()` interface.
That interface again returns an empty model header. Original model indexed
ranges retain their independent model resource loader and its own evidence.

## Evidence

Evidence root: `analysis/shader-parity-20261013`.

| Check | Scope | Result |
| --- | --- | --- |
| Whole original TU1.5 `1C310` | 12 declaration cases; 143 distinct unchanged instructions; no intercepted instructions | Original register stream, offset and format words retained in `tests/fixtures/tu15_skinned_vertex_declarations.original.json`. |
| Authored Lockout geometry | 400 model sections, 2,427 original register bindings, three stored position layouts | Pass. |
| Authored Cairo geometry/compression | 419 sections, 2,611 original register bindings, four stored position layouts, 5,028 compression words | Pass, bit-exact compression bank. |
| Original Cairo compression prefix | 917 authored model sections; 71 distinct unchanged arithmetic instructions | Retained in `tests/fixtures/tu15_cairo_model_compression.original.json`; device submission/lifetime is outside this fixture. |
| Lockout Vulkan vertex comparison | Four authored local classes; seven resolution/split views; 91,833 GS records | 4,683,483 exact words; 28 palette negative controls; zero validation errors. |
| Cairo Vulkan vertex comparison | All five local classes, including actual Marine four-influence section32; seven views; 32,739 GS records | 1,669,689 exact words; 35 palette negative controls; zero validation errors. |

The GPU comparison runs the same unchanged raw input streams and explicitly
controlled, distinct per-node matrices through native shaders and independently
retained original-emitter arithmetic. It covers640x480,1280x720,1920x1080 and
all four1280x720 split views. These matrices are test inputs, not a claim of
native production of an actor's current pose or render cache. Original GPL
verification shader text remains in analysis artifacts and is not production
source. The four-influence read-only census separately finds79 authored sections
across the supplied maps; that count is inventory, not live rendering coverage.

Registered checks:

```sh
ctest --test-dir build-shader-parity-20261013-v1 --output-on-failure \
  -R '^original_tu15_(campaign_)?skinned_(geometry|light_vertex_gpu)$'
```

The coherent source is `.local/shader-parity-source-20261013-v1`; it extends the
retained vehicle source checkpoint with explicitly hashed shader changes. It
is separate from the current combined launcher delivery. The broad goal remains
in `SHADER_PARITY_COMPLETION_GOAL.md`.

## Remaining live gate

`SourceModelEffectLights` can consume these classes, but genuine model receiver
collection, current palette/cache production, lifetimes and shared ordered
light-layer submission still need their live owners. Obtain a visibly active
original model illumination case and compare its native pixels. Invisible
reference draws must continue to fail acceptance. The complete28-draw light
segment still fails strict framebuffer equality; vertex and source-bank matches
do not close that gate. Full gameplay frames and installed-runtime delivery
follow the live integration and pixel fixes.
