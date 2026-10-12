# Original weather plate graphics packet

The isolated native pass is `SourceWeatherPlateRender` in
`include/halo2/source_weather_plate_render.hpp` and
`src/source_weather_plate_render.cpp`. Its two shaders preserve the observed
VS21 class 0 operations and complete original `3A5C0` mode 3 combiner definition.
This module does not provide live visibility, plate simulation, clocks, RNG,
bitmap readiness, residency or a depth-alias producer.

## Original authority and packet admission

The original executable is the unchanged supplied `assets/disc/default.xbe`,
SHA-256 `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The real original Lockout draw is event 215778 in the private xemu OpenGL
llvmpipe capture `original-lockout-gl-source_capture.rdc`. Its five VS token
comments equal every word of authored global VS21 class 0, SHA-256
`c57b3f2509d664bba750f1b99fd09351efd0ddc4b9c5d2288e7fd2154896799a`.
Its four-vertex quad strip immediately precedes the separately identified
VS22 snow event 215873. The independently inspected event 215234 has another
vertex program and one sampler and is rejected as the plate reference.

`analysis/multiplayer-shading/weather-plate-original-reference/` retains the
actual GLSL, state, constants, five four-vertex immediate streams, before/after
scene color and D24/stencil, packed depth alias and seven levels of all three
256-square layer textures. Every one of the 307,200 RGBA alias pixels equals
the current D24S8 raw bytes reordered `[2,1,0,3]`, with the same RenderDoc GL
row order and zero differing bytes. A scalar float depth sampler is not this
source interface.

`freeze-plate-packet.py` executes complete unchanged original `3A5C0` mode 3
with the actual prior-391F0 snapshot plate inputs, restored actual SDK banks
and pinned instruction verification. It verifies 568 distinct instructions,
replaces no title/SDK callee, and produces all sixty words exactly equal to the
retained actual prior plate definition. All seventeen actually reflected
capture constants equal its packed byte expansion exactly. The immutable CPU
snapshot and the GPU capture have separate frame histories; their equal pixel
definition does not establish equal simulation or loading history.

## Native graphics contract

The caller supplies the exact program, four immediate v0..v4 records, c21..23
scroll, clip range, eight inclusive window-clip regions and four actual images.
Stage 0 is a full-surface RGBA8 alias with nearest/clamp sampling. Stages 1 through 3 use
repeat and linear min/mag/mip filtering. Sampler handles are opaque; their
documented source state is a caller precondition. The matched fixture creates
these states explicitly and uploads actual captured RGBA mip bytes.

The pass uses the original masked MOV/ADD outputs, screen coordinates truncated
to sixteenth-pixels and constant source W. Pinned xemu's quad strip emits
triangles 0/1/2 and 2/1/3, which the native four-vertex triangle strip preserves.
It uses ONE/INV_SRC_ALPHA, rounded-byte alpha GREATER0, RGB-only writes,
LEQUAL W depth and no depth/stencil writes. The observed plate supplies equal
projected Z/W at all four vertices and zero depth offset/factor. Other pixel
definitions are rejected pending their own original admission.

Finite inputs and derived VS/projection outputs are checked before changing
bindings. The context verifies physical depthClamp support when requested;
the caller must enable it on the logical device. Recording requires the
prepared full-surface viewport and a scissor within that surface. No extra
screen-size, UV, opacity, color or depth multiplier is fitted.

## Measured validation and limits

`tests/source_weather_plate_scene_tests.cpp` checks the literal packet and
alias on the CPU. Its `--gpu` fixture uses the actual original before-scene
color and D24/stencil at 640×480, then compares the isolated native pass with
the original after-scene. `--gpu-baseline` creates and destroys only the
graphics context for the external sanitizer control. The standalone build
helper is `tools/build_source_weather_plate_scene_prototype.sh`.

On the RTX 5080, the original changes 181,676 pixels and the native changes
181,343. There are 14,477 differing pixels, maximum RGB deviation 1 byte, total
RGB absolute difference32,098. The test explicitly bounds the measured
deviation to one byte. That residual is unresolved; no compensation is added.
All 307,200 original D24 integers and stencil bytes are preserved before and
after, every alpha byte is unchanged, and ordinary reuse is byte-identical.
Seven invalid packet controls and five invalid viewport/scissor controls are
rejected. Direct recording after rejected prepare calls, without preparing a
valid packet again, preserves the earlier exact output.

Normal and ASAN/address/UB GPU checks pass with identical readbacks and zero
validation/synchronization errors. CPU ASAN with leak detection passes.
Graphics-enabled LeakSanitizer reports 3,156 bytes in 29 allocations in both
the full render and empty-context control, with libdbus and unloaded graphics
module stacks. That external teardown limitation is retained in the logs;
the address/UB GPU run disables leak detection only. This is not a clean
graphics LeakSanitizer claim.

Final inputs, binaries, shaders, logs and checksums are recorded in
`analysis/multiplayer-shading/weather-plate-scene-prototype/validation.json`
and `source-input-manifest.json`. `original-native-plate-phone.png` shows the
actual original and native plate over the same original scene with nearest
crops and the residual label. This is one admitted emulator-host graphics
pass, not full native scene parity, physical Xbox equality, or all-map weather
history/resolution proof.
