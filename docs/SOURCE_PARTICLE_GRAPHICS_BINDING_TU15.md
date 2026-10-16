# Renderer-backed original particle graphics binding

The subsequent [depth binding and original layout work](SOURCE_PARTICLE_DEPTH_BINDING_TU15.md)
now owns actual D24S8 presence and format metadata. This document retains its
34-owner renderer-binding checkpoint below.

`SourceParticleGraphicsBindingTu15` now connects the original CPU command and
constant storage to a genuine live renderer. Its device lease, constant-mode
control and attachment use must name the same logical device. The issuing
thread, attachment generation and renderer retirement remain enforced. An
unissued control, unknown device field, unknown flag reader, foreign world,
recursive memory owner or stale/retired renderer cannot supply source state.

The source ABI prefix uses the authenticated `411568 =40F0C0` alias. It owns
the actual command cursor/end and the admitted mode fields; it does not claim
the complete original `405CF0` device constructor. The native interface owns
CPU constant shadows and therefore supplies non-pure shadow behavior. Other
original flags are not silently treated as zero: their readers stop. Only
the authenticated `400EF9/247E32` flag consumers and the `400F0F` store
observer's before-image are admitted.

Whole original `400EF0` realizes the genuine renderer's selected `0x11` mode,
writing mode1 and flag `0x200`. Its thirteen executed instructions, GP/SSE,
defined flags, x87/MXCSR and all60,371,030 retained bytes match unchanged host
i386. No SDK return is supplied. Four newly authenticated bodies bring the
particle-draw module to34 owners and2,695 instructions; all previous thirty
body-site sets remain intact.

The actual Backwash view1 callback now executes519 instructions and emits104
bytes before the required `3FF5D0` depth owner. That whole prefix also matches
unchanged host-i386 across registers, defined flags, x87/MXCSR, ordered path
and all60,371,030 retained bytes. The renderer-backed fields and callback ABI
are declared native platform inputs, not proof of original boot/resource
history. The complete parent still does not submit its primitives.

Seven unchanged actual source particle packets, with their original material,
also pass through whole `401410` and the native constant consumer into the
production Renderer. On cleared scene depth they change12 RGB pixels. All192
constant registers remain bit-exact, the packet/direct images are identical,
and the enabled repeat is exact. The green particle pixels and difference
were inspected. Positions, size, color, material, clipping and source particle
state are not adjusted. This isolated positive GPU test does not establish
ordinary map visibility or original Xbox framebuffer parity. The original
parent's other control packets remain separate from this constant-only test.

The GPU contract test also exercises actual attachment recreation, renderer
retirement, cross-thread construction/use and unknown state. Both GPU checks
report zero Vulkan validation errors. The first environment-only attempt
lacked the bundled validation layer; subsequent runs explicitly use the same
layer/Xvfb setup as the earlier accepted captures. Intermediate failed builds
and the rejected pre-store-reader guard are retained separately.

The remaining depth owner needs the real depth surface, viewport/control
state and its original dependencies. In particular, the unconstructed cache
at `4116C0` must not be accepted merely because it contains zero. Shader/pass,
bitmap and sprite command production/consumption must then complete before
the live queue decoder can be replaced and a combined launcher delivery made.
The observed `17BC` command is logic-op enable in the
[xemu NV2A register definitions](https://github.com/xemu-project/xemu/blob/master/hw/xbox/nv2a/nv2a_regs.h#L1112);
the original TU1.5 bytes remain the game authority, and this register reference
does not establish a complete native consumer for the control stream.

Evidence and current continuation are under
`analysis/map-world-features/particle-graphics-binding-20261015/`.
The accepted renderer probe is `actual-renderer-parent-v7/`, with unchanged
CPU comparisons in `original-hardware-v1/comparison.json` and GPU ownership
checks in `binding-contract-v1.json`. `acceptance.json` pins the stage.
This source component has not installed another normal runtime. The full
all-map, network/latejoin/reset, original-history, Vista/MCC and Xbox visual
requirements remain active.
