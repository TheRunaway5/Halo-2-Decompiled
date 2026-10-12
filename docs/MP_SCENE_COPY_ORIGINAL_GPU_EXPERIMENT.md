# Private original scene-copy experiment: live commands and capture boundary

The private xemu experiment executed unchanged retail `352E0(NONE, 1)` with the
restored Lockout resources. Its **1,672 bytes / 119 method packets match the
immutable CPU oracle byte for byte**. The original frame subsequently reached
the real SDK PUT store, and GET advanced beyond the copy span. A RenderDoc
capture was saved, but **same-build replay failed before draw or resource
inventory** with an end-of-stream error in `MakeContextCurrent`. No sampled GPU
source, destination before/after pixels, framebuffer center, or physical NV2A
parity is established.
[Live evidence report](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T010704.765058Z/evidence/report.json),
[failed replay result](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T010704.765058Z/replay/report.json),
[immutable CPU proof](MP_CAMOUFLAGE_SCENE_COPY_SOURCE_RESEARCH.md)

## Private preparation and preserved failures

Three separately owned attempt directories remain. Attempt1 stopped before a VM
launch because `qemu-img` failed during `io_uring` initialization even for
`--version`. Setting `RLIMIT_MEMLOCK=(0,0)` only in that image utility's child
process recovered operation; parent and global limits were untouched. Attempt2
rebased successfully but xemu rejected its JSON backing filename before guest
boot. Attempt3 used an ordinary absolute private base filename, and
`qemu-img info --backing-chain` verified that both chain members were inside
that new directory. No QCOW metadata was hand-edited.
[Attempt1 report](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T005710.847109Z/report.json),
[attempt2 failure and cleanup](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T010234.790842Z/cleanup.json),
[attempt3 preparation](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T010704.765058Z/preparation.json),
[validated backing chain](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T010704.765058Z/backing-chain.json),
[reproducible private preparer](../tools/prepare_scene_copy_original_gpu.py)

The retained source base disk, overlay and EEPROM were read and copied with
`cp --reflink=auto --sparse=always`. Their full hashes matched before copying,
after copying, and after the live experiment. The successful VM used private
EEPROM/config/data, Xvfb, QMP and guest GDB endpoints. Process ownership was
recorded from its unique config pathname before operation. Only the new owned
VM, debuggers, replay processes and display were operated or cleaned up.
[Source preservation checks](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T010704.765058Z/evidence/report.json),
[owned process identity](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T010704.765058Z/ownership.json),
[cleanup](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T010704.765058Z/cleanup.json)

## Unchanged original call and resource state

Original xemu 0.8.136, commit `fc24584ce88f0915ad7f04775bb7712c2e3f49ee`,
restored `lockout-weather-outdoor` with its authentic configured controller
before reaching original `391F0` through a hardware breakpoint. At this point
current target0, the 640×480 source header at `7FF20000`, target19's 320×240
record at `4B56A0`, camera/raster rectangles `[0,0,480,640]`, graphics resources,
command buffer and capacity were retained live inputs. The new command range
was `83EF7EFC..83EF8584`, within the original limit `83EFDF1C`.
[Snapshot restore](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T010704.765058Z/snapshot-load.json),
[live resources and CPU backing exports](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T010704.765058Z/evidence/report.json)

The declared external call used backed original stack space at `D00B3000`,
return address `391F0`, and arguments `FFFFFFFF,1`. It reached that return
hardware breakpoint with ESP=`D00B300C`. No instruction, readiness flag, service
return, barrier or GPU register value was replaced. Before resuming the
original frame, the saved stack bytes, GPRs, EFLAGS and MXCSR were restored.
Volatile XMM results and empty x87 register residues retained the call's
results. This is an explicit external-call composition, not evidence of the
natural camouflage caller's scheduling or a completely restored frame context.
[Exact debugger transcript](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T010704.765058Z/guest-gdb.log),
[before and return registers](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T010704.765058Z/copy-return/qmp.json)

All 27 named copy/SDK owner ranges match the pinned retail XBE in all three
exported RAM states. The live seven-instruction screen program also matches the
immutable source program. The emitted copy bytes have SHA256
`287464d4e06e091e1e7cee9c5e2e99bb13a1f8d227488a9c10b61b420e0142ad`.
CPU RAM backing exports are preserved explicitly as CPU observations; they are
not substitutes for GPU-resident source or destination surfaces. The live VM
was not instruction-traced in full; the earlier independent CPU oracle provides
that separate bounded instruction-execution evidence.
[Recomputed original-byte comparisons](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T010704.765058Z/evidence/report.json),
[exact command bytes](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T010704.765058Z/evidence/copy-commands.bin),
[read-only evidence recomputation tool](../tools/summarize_scene_copy_original_gpu.py)

## Real publication and the completion limit

The copy itself returned without reaching `3FAC30`. After the original frame
resumed, it reached that unchanged SDK owner with device=`404FE0`. The actual
`3FAC58` instruction wrote to `[FD800000+40]`: PUT changed from `03EF61E4` to
`03EFDF20`, publishing the contiguous range containing the copy. Immediately
after the single step GET was `03EF64DC`; later PUT and GET were both
`03F120D0`, beyond the copy end `03EF8584`. The original `3FA970` wait owner
also returned naturally to `3F9DA0`.
[Observed original PUT/GET and wait transcript](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T010704.765058Z/guest-gdb.log),
[bounded publication record](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T010704.765058Z/evidence/report.json)

That wait requested reference `20708F`, already older than the copy-context
latest reference `2070AB`; completed reference was `2070B1` at the observed
wait. It is not an identified fence placed after this particular draw. GET
progress proves original queue consumption, not by itself raster completion
or exported copy pixels. No SDK flush, CPU return, GET value, or RenderDoc API
success is promoted to a framebuffer result.
[Exact queue boundary](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T010704.765058Z/evidence/report.json)

Complete indexed source listings remain for actual publication `3FAC30`,
visibility flush `3FADE0`, GET/cache `3FA880`, queue/reference query `3FA900`,
reference insertion `3FA680` and reference wait `3FA970`. `3FADE0` performs
`SFENCE`, sets bit10000 at device-register-base`+100410`, and waits for its
clearance before the PUT store. The reference wait has real memory/kernel
branches. These contracts explain the observed path without supplying successful
service or completion returns. `400FA0` is state packet emission, not submit.
[Submit](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T005710.847109Z/submit-003fac30.asm),
[visibility](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T005710.847109Z/visibility_flush-003fade0.asm),
[GET](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T005710.847109Z/get_query-003fa880.asm),
[query](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T005710.847109Z/queue_reference_query-003fa900.asm),
[insert](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T005710.847109Z/reference_insert-003fa680.asm),
[wait](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T005710.847109Z/reference_wait-003fa970.asm)

## Same-build capture failure

Manual standard RenderDoc capture began and ended with return 1 on the observed
PGRAPH OpenGL thread 37, context `555557D1E1B0`. Both xemu and replay used Mesa
llvmpipe. The retained capture is 14,239,513,299bytes, SHA256
`620c2fcbdffab73b688124fbdad55f46b5121e1bf204e453c124f1473c498cac`.
Offline replay was bounded to 48 GiB address space in its own process and exited
after recording the failure; no global memory limit changed.
[Host capture transcript](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T010704.765058Z/host-gdb.log),
[capture and replay identity](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T010704.765058Z/evidence/report.json),
[owned replay memory limit](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T010704.765058Z/replay-memory-bound.json)

Injection and replay used the **same RenderDoc build and library**. Injection
preloaded `.tools/renderdoc/usr/lib/librenderdoc.so`, SHA256
`37c7a711cd81d848934a757222300d70f49365da87ef56d80f013f8593e37666`.
The replay executable `.tools/renderdoc/usr/bin/qrenderdoc` has SHA256
`48db4bbec6b58cf7793befb6d06ca512da70fb510d3ba1d37de2b35cac17c3cb`;
its `$ORIGIN/../lib` RUNPATH and `ldd` resolve that identical library. Capture
header says `1.45 2fc0bc`, and replay log / `renderdoccmd version` identify
commit `2fc0bc04cb95499635f63986a55bc6f67849dd9f`. The control source requests
standard API 1.6.0. There is no demonstrated injection/replay version mismatch.
[Paths, full hashes, version, loader and header evidence](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T010704.765058Z/evidence/report.json),
[unchanged capture control source](../tools/xemu_gl_capture_control.c)

`OpenFile` succeeded, but `OpenCapture` returned
`Replaying the capture encountered invalid/corrupted data: Reading off the end
of data stream`. The saved log pinpoints `MakeContextCurrent` serialization.
It never returned a replay controller, so no target19 draw, sampled resource,
program interpolation, before/after GPU destination or pixels were identified.
The capture remains unaltered. Host startup failures, client reset on QMP quit,
logs, private disks, CPU exports and the failed replay are preserved. A valid
source capture/replay of this actual copy draw remains the concrete prerequisite
for the GPU oracle; native code, renderer integration and variant admission are
unchanged.
[Failed same-build replay](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T010704.765058Z/replay/report.json),
[exact replay log](../analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012/attempt-20261012T010704.765058Z/replay/RenderDoc-replay.log),
[offline exporter](../tools/replay_scene_copy_original_gpu.py)
