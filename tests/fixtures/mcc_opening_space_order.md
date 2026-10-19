# MCC opening transparent model order

The queue and main-unit TSV files are one complete captured main handoff from
the selected MCC `00a_introduction.map` (SHA256 is in both fixture headers).
The queue contains twelve unmodified 112-byte requests; the main-unit fixture
retains ordinal, full object/definition IDs and all eight native unit words.
Capture receipt: `analysis/opening-cinematic-20261015/mcc-space-binding-v786`.
The debugger only reads the completed source banks.

`mcc-space-gpu-order-v785` independently observes the current Vulkan consumer
drawing High Charity in object slot14, then Threshold in slot15. Both use the
same premultiplied blend pipeline, with depth test LEQUAL and no depth writes.
That sequence explains the reported occlusion.

The complete original TU1.5 sort prefix at1E35B..1E397, including whole140A60,
140C20 and1E6E0 bodies, sorts the fixture as
`6,7,5,8,4,9,3,10,0,1,2,11`. Threshold is command7, before High Charity's8.
`mcc-space-tu15-sort-v789` executes unchanged instructions from XBE SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`,
with no instruction patches or supplied callee returns, under60MiB hard/
64MiB measured/zero swap and the global validation lock. All plane-present
low bytes are zero, so this sort does not consume camera coordinates.
The native ordinal sorter produces exactly the same order.

The regression drives the single-part forward-model consumer with these real
queue/unit bytes and the observed slot14/15 order. Sorting only those two
commands first changes their equal-key ordering; the complete bank must be
sorted before projecting its model owners. The consumer moves each existing
draw with its geometry, colors, lighting and shader-function owner. It changes
no asset, depth, blend, camera or animation values.

This seam handles ordinary41010 requests which uniquely match a complete
forward-only single-part draw. Multi-part/group callbacks and ordering relative
to other effect/BSP owners are separate integration work. This fixture is not
full cutscene or Xbox framebuffer parity.
