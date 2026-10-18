# Projected 2003 light response on current campaign assets

`projected_constants_2003.cpp` implements the projective/pyramid branch of
unchanged May2003 `A3CF0`, including its explicit intermediate float stores.
The input adapter uses the current native completed shape and actual light
frame; receiver queries, native light/part order and stencil casters stay with
their existing source owners. Current cinematic frustums remain frustums.

The original's `C15..20` are adapted into the private response bank's
`C31..36`. The old near-face offset is **not** retail `C36`: original `C20.z`
contains `(shape[10] + 1/128) * (-1/(far-near))`. Its unwritten `C16.w` and
`C20.xy` retain caller values. Current completed light colors remain authored
current-asset inputs to the recovered RGB packer and dependent program.

The unchanged original CPU oracle executes 384 authenticated instructions on
five actual current cinematic shapes plus 512 declared projective/pyramid
fixtures. The native calculation matches all 12,408 binary32 output words
exactly. Another 1,551 cases execute original utility vertex `2F`, dependent
vertex `25` and the actual current Chief sphere-specular vertex programs at
three LODs. Their adapted bump/light/eye/falloff/gel inputs match exactly.

Original `35E81..35E94` supplies `V15=(0,.5,1,42)` through original `33D480`.
Original `18087F..180885` supplies static gel `V9.xy=(0,0)`; the scrolling
branch has a separate time-dependent owner and remains outside admission.
Consequently the admitted projected gel uses original25's `S/Q,T/Q`, with
`Q=dot(C16-position,C19.xyz)*C15.w+(1-C15.w)`. The current actual 2D gel frame
and authored sampler are retained. Original96200's no-gel fallback selects
default2D frame1; all 84 RGB/alpha mip-payload bytes are white.

The current opening uses two actual authored gel bitmaps across Chief,
Master Guns, Arbiter and Prophets; Cairo Fill has no authored gel. All five
static definitions are admitted by this response adapter. Moving, runtime,
animated, cube and volume gels are not silently converted. A native combined
implementation's later gel packet is skipped only when an earlier recovered
dependent pass already includes that gel; it is not applied twice. A null
companion bank retains the ordinary native pass; an explicitly empty bank
marks that already-included later packet. Original ignores this optional bank.

Program/geometry residency is shared across contrast and mask variants;
complete program words and source constants are uploaded per draw. Source
completion stores metadata without bitmap payloads and invokes the existing
binder with image loading disabled. 63 full/image-free current Chief, Arbiter
and Elite armor completions match programs, raster state and every constant
exactly, with no image or animated-frame loads in the image-free result.

The retained first projected integration, `light-response-native-v3`, completed
the opening in 225.25 seconds with 1,041 native frames, no script faults or
Vulkan validation errors, 20 exact Original restorations and 14 positive
light/shadow capture pairs. It admitted Cairo Fill only; its authored-gel
lights stayed on the previous route. The next `light-response-native-v4`
adds their actual gel bindings and the image-free completion. It finishes the
full opening in 229.62 seconds with 968 native frames, no script faults or
Vulkan validation messages, 20 exact Original restorations and 14 positive
light/shadow pairs with no shadow brightening. Actual captured frames submit
up to 204 companion draws. Its sphere group/visor images and Original remain
byte-identical to the earlier sphere integration. Campaign-v4 is promoted
through the separate campaign launcher; recording verification is independent.

Evidence lives in `analysis/pstencil/projected-response-v1/`: unchanged CPU
oracle, actual native shape/frame inputs, exact scalar/vertex test, original
default-attribute instruction bytes, actual gel eligibility and default2D
payload audit. Runtime candidates retain their own sources, commands, shaders
and native capture controls in separate folders. This is current-asset
adaptation evidence; no matching May2003 GPU frame or complete presentation
parity has been established. Accumulated normals, animated families and full
asset/face coverage remain separate acceptance work.
