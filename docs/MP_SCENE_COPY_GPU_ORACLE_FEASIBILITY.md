# Scene-copy GPU reference feasibility from retained original artifacts

The existing owned artifacts support a concrete private-session experiment, but
they do **not yet identify a framebuffer oracle for `352E0(NONE,1)`**. Copied
Lockout VM snapshots, the original disc/runtime dependencies, genuine original
xemu graphics captures and working capture/readback scripts are retained. The
completed CPU oracle's RAM/backing hashes do not establish the same call's
GPU-resident source, command submission or completed target19 output. No VM,
process, debugger, socket or RenderDoc replay was operated for this inventory;
no disk, EEPROM, configuration, native source or frozen proof was changed.
[Read-only inspector](../tools/inspect_scene_copy_gpu_artifacts.py),
[artifact identities and observations](../analysis/multiplayer/game-variants/scene-copy-gpu-artifacts-20261012/report.json)

## Existing state

The inspector reads QCOW headers and snapshot tables directly through read-only
file handles. Ten files in five relevant copied sessions were inspected. Each
session overlay names its own adjacent copied base, rather than requiring a
future experiment to write a retained source disk. Snapshot names and declared
VM-state sizes below are metadata observations; loading them was not tested.
The disks were not fully hashed or treated as immutable captures.
[Raw QCOW metadata inventory](../analysis/multiplayer/game-variants/scene-copy-gpu-artifacts-20261012/report.json)

| Copied session | Snapshot inventory |
| --- | --- |
| `original-weather-20261009/session1` | Ivory Tower, Zanzibar, `lockout-weather-baseline` (62,187,361 VM-state bytes), `lockout-weather-outdoor` (62,702,881) |
| `original-clear-scheduling/original-session` | Same four names and VM-state sizes |
| `world-effects-goal-20261011/original-session` | Same four, plus `frag-light-draw-20261011` |
| `registration-snapshot-restore-20261008` | Ivory Tower and Zanzibar |
| `original-xemu` | Ivory Tower and Zanzibar |

Paths in this table are under `analysis/multiplayer-shading/`. The first session's
retained launch/config records specify private X11, original xemu0.8.136,
OpenGL llvmpipe, paused boot, separate paused snapshot loading and a bound
keyboard-backed Xbox USB controller. Its saved load transcript demonstrates
the required controller topology. The runtime, RenderDoc library, copied
EEPROM, firmware and owned disc paths still exist; the inspector records only
existence/size for those runtime dependencies, not a successful launch or their
full content identity.
[Original launch record](../analysis/multiplayer-shading/original-weather-20261009/session1/launch.json),
[configuration](../analysis/multiplayer-shading/original-weather-20261009/session1/xemu.toml),
[previous paused load/topology transcript](../analysis/multiplayer-shading/original-weather-20261009/session1/gl-manual-load-and-run.json),
[current file inventory](../analysis/multiplayer/game-variants/scene-copy-gpu-artifacts-20261012/report.json)

Three actual captures and their saved summaries were inspected, without replay:

| Capture | Saved summary |
| --- | --- |
| Lockout `original-lockout-gl-source_capture.rdc` | SHA256 `53a7271c…720a5`; 141,671,532 bytes; 11,116 actions, 1,792 four-index actions, 205 textures |
| Lockout `original-lockout-gl_frame13537.rdc` | SHA256 `6e69de5c…514dc`; 7 actions, 1 four-index action, 15 textures |
| Zanzibar `original_capture_20.rdc` | SHA256 `d99a5c97…642ee`; 1,650 actions, 204 four-index actions, 184 textures |

The full Lockout recording is a real multi-frame original-title OpenGL capture;
the separately exported event215873 is a weather point draw with actual
before/after color and depth resources. That event is not the missing copy.
None of these three saved texture summaries contains a320×240 texture row.
This does not prove that all captures lack a copy, nor does a four-index draw
identify one. Pipeline state, exact programs, resource identities, surface
scaling and event-to-original-call correlation have not been audited for a
target19 copy.
[Capture provenance](../analysis/multiplayer-shading/original-weather-20261009/session1/source-gl-capture-provenance.json),
[existing weather resource export](../analysis/multiplayer-shading/original-weather-20261009/session1/source-snow-event215873/surfaces.json),
[exact three-capture inventory](../analysis/multiplayer/game-variants/scene-copy-gpu-artifacts-20261012/report.json)

## Why RAM and a CPU return are insufficient

The original weather-entry export retains64MiB of physical RAM and unchanged
CPU registers at391F0 with debug/nonrunning status. It is not a same-call GPU
surface/queue capture. The scene-copy oracle subsequently executes a fresh
external original call over those bytes, emits1,672 command bytes and returns;
its source/target backing hashes identify the input RAM. They do not show that
the current GPU scene was downloaded into that RAM, that newly generated
commands were submitted, or that target19 was rendered.
[Original CPU export](../analysis/multiplayer-shading/original-weather-20261009/session1/lockout-weather-particle-entry/export.json),
[CPU oracle and exact limits](MP_CAMOUFLAGE_SCENE_COPY_SOURCE_RESEARCH.md),
[original SDK state-packet owner, not a GPU completion marker](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/dirty_flush-00400fa0.asm)

Pinned xemu OpenGL texture binding first looks for a resident render surface at
the texture's VRAM address. A compatible surface can be used directly; another
path downloads dirty overlapping surfaces before treating backing bytes as
texture data. Thus a paused CPU memory export cannot be silently substituted
for the actual resident source. OpenGL surface download explicitly reads the
surface into VRAM and clears its dirty state. The proposed oracle must capture
the real sampled resource and completed destination at the draw boundary.
[Resident surface-to-texture selection](../tools/reference/xemu-0.8.136/hw/xbox/nv2a/pgraph/gl/texture.c:255),
[actual OpenGL download owner](../tools/reference/xemu-0.8.136/hw/xbox/nv2a/pgraph/gl/surface.c:765)

PGRAPH save hooks delegate to the selected renderer. The locally retained
Vulkan implementation requests and waits for dirty-surface downloads before
save. The exact OpenGL save-hook implementation and complete NV2A VMState
serialization owner are not in this selected local source set. Snapshot
metadata alone therefore does not prove the exact current source, queued
commands and renderer state for the earlier391F0 RAM export. A new private
restore must measure its own state and input resources.
[PGRAPH save delegation](../tools/reference/xemu-0.8.136/hw/xbox/nv2a/pgraph/pgraph.c:3231),
[Vulkan download/wait save hooks](../tools/reference/xemu-0.8.136/hw/xbox/nv2a/pgraph/vk/renderer.c:150),
[xemu snapshot loading](../tools/reference/xemu-0.8.136/ui/xemu-snapshots.c:241),
[verified pinned-source inventory](../analysis/multiplayer/game-variants/scene-copy-gpu-artifacts-20261012/report.json)

## Bounded experiment supported by these artifacts

First, an offline replay audit could search the retained original captures for
an actual matching draw. It must establish the original screen program, the
mode1 sixty-word pixel program, four source bindings, actual sampler/state
values, position/tap attributes and target19's identity. A texture size or pass
name alone is insufficient. If a matching event exists, export its sampled
source, destination immediately before/after, post-VS output, viewport and
shader inputs. The existing event export script shows the supported
`SetFrameEvent`/`GetTextureData` mechanism. Such an audit would replay a retained
original graphics recording; it would not rerun the original CPU caller.
[Retained resource-export mechanism](../analysis/multiplayer-shading/original-weather-20261009/session1/export-source-snow-surfaces.py),
[retained draw-export mechanism](../analysis/multiplayer-shading/original-weather-20261009/session1/export-source-snow-draw.py),
[actual CPU program/geometry identity](MP_SCENE_COPY_RECTANGLES_SOURCE_RESEARCH.md)

If no existing event matches, the concrete next experiment is an isolated
copy of the Lockout base/overlay/EEPROM, with only the new overlay's backing
path/configuration redirected to its new private copies. Use distinct QMP,
guest-debugger and private display endpoints; recreate the observed controller
topology before paused `lockout-weather-outdoor` loading. Retained sessions and
their sockets remain untouched. This is a proposed future experiment, not an
operation performed here.
[Existing disposable-session preparation](../analysis/multiplayer-shading/original-weather-20261009/session1/preparation.json),
[previous isolated launch/setup](../analysis/multiplayer-shading/original-weather-20261009/session1/launch.json)

In that new session, first record the genuine current target, actual source
header/backing/dimensions, target19 readiness/dimensions, programs, camera,
command pointer/limit, resident input surfaces and PFIFO/PGRAPH state. Refuse
the case if the original target is unavailable or a real service cannot
complete. Intercept a natural `352E0(NONE,1)` call where available. A separately
declared external call to unchanged352E0 over that measured environment is an
alternative narrow composition experiment; it must be labelled as such and
does not prove the natural camouflage caller/schedule. No successful service
return, readiness word, resource header or guest instruction may be supplied.
[Original full-call contract](MP_CAMOUFLAGE_SCENE_COPY_SOURCE_RESEARCH.md),
[actual authored camouflage caller/schedule, separately required](MP_CAMOUFLAGE_PASS_SOURCE_RESEARCH.md)

Start standard RenderDoc capture on the observed original rendering thread's
real GL context using the retained host capture-control mechanism. Record the
original call's produced command span, then let the unchanged title/SDK perform
its real submission and renderer consumption. A breakpoint at CPU RET alone
is insufficient. The target draw must appear in the recording with the exact
program/resources/geometry; end capture on the observed context after that
work. If no draw is consumed or submission waits at a real queue service, save
that boundary rather than replacing it. The exact original queue submission
and completion point for this proposed call remains to be observed.
[Existing non-replacing GL capture control](../tools/xemu_gl_capture_control.c),
[previous host-debugger capture setup](../analysis/multiplayer-shading/original-weather-20261009/session1/host-gl-start.gdb),
[original captured execution provenance](../analysis/multiplayer-shading/original-weather-20261009/session1/source-gl-capture-provenance.json)

Finally export all four actual sampled bindings, actual target19 before/after
that event, dimensions/formats/row order, sampler state, programs, post-VS
positions and viewport. RenderDoc replay readback supplies the completed
translated draw's output; it must be kept separate from CPU RAM bytes and from
later presented images. This yields a bounded xemu graphics reference against
which native sampling can be compared without assuming a raster-center bias.
The source/target19 color allocations in the existing CPU context are distinct,
and target19 uses NULL depth. The experiment can stop after the copy, without
executing the separately unsupported target3 simultaneous color/depth alias.
[Existing before/after readback script](../analysis/multiplayer-shading/original-weather-20261009/session1/export-source-snow-surfaces.py),
[actual target19 color/depth contract](MP_CAMOUFLAGE_SCENE_COPY_SOURCE_RESEARCH.md),
[separate alias limitation](MP_CAMOUFLAGE_DEPTH_ALIAS_SOURCE_RESEARCH.md)

This would be original title command execution through xemu's translated
OpenGL/Vulkan backend. The retained Lockout reference uses software OpenGL
llvmpipe, which must remain explicit; a new hardware/backend run would be a
separate environment. Neither establishes physical NV2A precision or Xbox
framebuffer parity. The existing CPU geometry proof, emulator raster-center
implementation, native GPU result and complete camouflage effect remain
separate claims. The inventory report has no VM launches, process operations,
disk/EEPROM writes, instruction patches or supplied returns.
[Inventory boundaries](../analysis/multiplayer/game-variants/scene-copy-gpu-artifacts-20261012/report.json),
[separate screen-program evidence](MP_SCENE_COPY_RECTANGLES_SOURCE_RESEARCH.md)
