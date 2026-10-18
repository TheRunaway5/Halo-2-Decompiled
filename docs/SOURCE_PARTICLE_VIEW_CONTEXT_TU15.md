# Original particle view-context kernel

The particle kernel module now owns complete TU1.5 bodies `132B0`, `15350` and
`3FF670`: 73 bodies and 6,310 original instruction sites in total. The real
`2B9F0` call at `2BBFB` authenticates `132B0`; its call at `132E8` authenticates
`15350`, whose call at `1536E` authenticates `3FF670`. The generator checks
these calls inside their complete containing bodies against the owned English
TU1.5 executable, SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

`132B0` copies the caller's entire 0x298-byte packet into the original view
bank. It selects the target mode, resets stencil selection, and calls the
existing complete `12F80` camera publisher with the actual copied camera,
projection, mode and scaling inputs. Its remaining target/clear and shader
state calls stay explicit required owners. This replaces no live runtime
publication yet; the complete context cannot be accepted until its downstream
owners and native GPU consumers are available.

`15350` retains its original seven-way selector and out-of-range behavior.
Its original 28-byte dispatch table is mounted read-only from the executable.
`3FF670` retains the source stencil caches and command arithmetic, including
all combinations of enable, dependent state, two-sided state, reference mode
and depth presence. The existing `2BCB0` body now calls the newly owned selector
at seven previously unresolved call sites. Its other generated code is exact;
all 69 other previous function definitions are also exact.

The permanent `native_tu15_particle_view_context` check has 47 cases:

- Six complete context prefixes copy and publish the camera for modes 0, 1 and
  2, with scaled and unscaled input. They stop before the original `14BA0`
  target owner, leaving the pre-existing queue counts 17 and 19 untouched.
- Nine selector cases cover all seven choices plus positive and negative
  out-of-range input. Six require the original `3FF720` or `222B0` owner;
  three return completely with the original cache and stack behavior.
- Thirty-two complete stencil calls check both methods and cache writes across
  all five input choices, including depth absent/present and matching/different
  reference modes.

All 47 cases match unchanged TU1.5 execution in general/SSE registers, defined
flags, x87/MXCSR state, instruction path, and every retained source byte
(143,213 bytes per case). There are 35 complete calls and 12 required-owner
prefixes. No instruction patch or supplied game/GPU return is used. These are
declared CPU inputs and command storage; they establish neither original SDK
startup nor actual native render-target, stencil or queue-retirement history.

The existing camera-before-reset resource test also passes against the expanded
kernel. Seven retained source draws and the separate 77-packet allocation-growth
check pass with no Vulkan validation errors. All six images, including the
empty control, remain exact to the accepted camera-order stage; the five
positive pixels remain a cleared-depth check. Retained RNG/pool bytes are
unchanged. This does not establish ordinary scene-depth visibility or a complete
original GPU parent.

The generated kernel reproduces byte-for-byte, and the kernel and new test
compile with normal CMake flags. CMake registers the permanent test; its GPU/CPU
execution receipts use the recorded coherent private backend. No combined
current application link or installation is claimed. Both normal launchers
still select runtime `1fe89481` and report native MCC gameplay unavailable.
The accepted `7501144f` world binary and 59 shaders are preserved.

Evidence is in `analysis/map-world-features/particle-view-context-20261018/`.
The next integration boundary is original target selection, then completing
the prepared view before collection and proving queue consumption/GPU
completion before the next reset. Full original parent and shader/matrix/
vertex-stream ownership, live world-depth visibility, all-map interactions,
network/late-join/reset behavior, normal-launcher delivery and Xbox visual
acceptance remain open. The all-map goal remains active.
