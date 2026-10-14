# Triangle flashes and third-person SMG stance, 2026-10-13

All edits, builds and diagnostic output are in
`/home/eric/Developer/Halo-2-Decompiled`. The SSD checkout was read only to
identify the running executable and compare relevant renderer source.

## Confirmed animation defects

The previous body composer treated `aim_still_up`/`aim_still_down` (and their
moving counterparts) as separate skeleton layers and applied both at weight1.
They are raised/lowered weapon alternatives. Original TU1.1 `113830` at
`114028..114056` computes complementary weights. Doubling the overlays rotated
the torso excessively and displaced the weapon. The normal native combat pose
now samples raised aim; an explicit lowering weight samples both alternatives
with the source weighting rule. Publication of the full original extension+88
lowering state into native snapshots remains separate work.

The regression failed before the correction with
`Raised SMG stance compounded aim_still_up with aim_still_down`.
The original raised asset samples restore neutral held-weapon forward vectors
from approximately `(0.877,0.281,-0.389)` to `(1.000,0.001,-0.013)` for Spartan,
and `(0.779,0.115,-0.616)` to `(0.997,0.020,-0.069)` for Elite.
Tests cover the source bindings, yaw endpoints, five lowering weights and
neutral held-weapon direction. The existing rifle/SMG class selection was
correct; these corrections change composition and hand alignment.

Third-person rendering also omitted the original world support-hand IK
callback (`113BA0/113980`). The shared cache loader now accepts the actual
held weapon, retains species class aliases and stance-specific marker pairs,
and uses the existing original two-bone solver on the actual weapon nodes.
Skinning follows this callback. Render observations retain those same nodes.
The Spartan SMG previously missed its support marker by about 0.036 units;
226 actual two-view pose observations now satisfy a 0.002-unit marker bound.
The Elite SMG has no authored support-hand constraints; none are invented.

Both species' real SDL/Vulkan action probes pass pitch/yaw aim, advancing
left/right turns, sword pickup, ground/air/crouched special strikes, three
ordinary strikes, full-body primary selection and bounded submitted geometry.
Six focused animation/upload/frontend IK CTests pass. Explicit locomotion
runs also pass with Base, TU1.1 and TU1.5 executable operands.
The main `halo2_native` target builds successfully.

## Triangle-flash evidence and limits

The flash remains unconfirmed and **has no claimed fix**. No renderer guard,
triangle removal, clipping change or allocation policy was substituted without
reproducing the reported defect.

Artifacts and diagnostic prototypes are under
`analysis/triangle-flashes-20261012/`:

- Solo Lockout roaming visited 12 authored spawn positions, traveled 188.413
  units (including diagnostic placement changes), and submitted 4,144 frames.
  A second two-view 4:3 run submitted 2,740 frames and 5,480 bounded FP draws.
  A completed two-view 1280x720 run submitted 2,472 frames and 4,944 bounded
  FP draws. The full recording was scanned for frame differences, and selected
  candidate frames were visually inspected.
  Both enabled finite-triangle/range checks and independent owned-versus-borrowed
  upload-byte comparison; neither assertion fired.
- Ascension exercised several regions; the initial probe stopped when its
  diagnostic teleport encountered a dead player. Later roaming skips that
  invalid teleport. Its shorter recording is not a completed acceptance run.
- Frame-difference candidate sheets and contact sheets were inspected. The
  reviewed candidates showed map boundaries, surface detail and camera motion;
  no isolated narrow triangle was identified in those samples. This is not
  proof that every recorded frame is artifact free.
- BSP topology expansion matched the source list indices for Lockout and ten
  additional disc multiplayer maps. An independent resource audit covered
  3,257 model parts across the existing 21-map model fixture. Its 16 flag/count
  disagreements were all single triangles, whose list/strip output is identical.
- A live `nvidia-smi pmon` snapshot reported 605 MiB for the existing game process,
  with 2,570 MiB total GPU use out of 16,303 MiB. One snapshot cannot establish
  or exclude a leak. The earlier short-run measurements remain separately
  documented in the2026-10-12 notes.

The running game process observed during this work used
`/mnt/SSD4TB/Halo-2-Decompiled/build/halo2_native`, SHA256
`4d1bed3813fc345c282bd383d9cea2199283f80a44e549e3abc5deaa846202b7`.
It was not stopped or replaced. Main-folder changes require restarting with
the main-folder launcher. Renderer/cache source compared identically between
the checkouts at that observation; the binary difference alone does not
explain the triangle flash.

Roaming diagnostics accept `--roam`, `HALO2_GEOMETRY_VIEWS=1..4` and an optional
`HALO2_GEOMETRY_WIDTH` (16:9). Widescreen uses different source split rectangles
from 4:3. The completed widescreen recording uses an explicitly paced,
fragmented MP4 and is included above. An earlier recording that did not
finalize is excluded from the successful evidence.
