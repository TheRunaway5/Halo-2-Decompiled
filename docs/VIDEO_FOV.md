# Video field of view

Video Settings now includes **Field of View** and **Vehicle Field of View**.
Both accept 70 through 120 degrees. On foot defaults to 78; vehicles default
to the original TU1.5 setting of 70. They persist independently in
`menu.cfg` (`pc_fov`), and reset with the other video defaults. The imported
numeric choice pane supplies keyboard, controller, wheel, and mouse navigation.
The pause Video Settings menu exposes the same controls.

The on-foot presentation conversion follows the installed MCC Halo 2 binary's
modern camera branch:

- `halo2.dll` SHA-256: `de65b4f4fdbf3f0a5eab7431fe530da17dd815599182dfd6ae9b7e21cf171946`.
- RVAs `3C230` / `3C360`: independent ordinary/vehicle angles, clamped to 70–120,
  converted from degrees to radians.
- RVAs `6F4490` / `7DFF10`: horizontal observer angle becomes a vertical render
  angle using a reference aspect capped at 16:9, with scale `0.8949999809265137`.
  Wider physical viewports reveal more horizontal scenery.
- Routines `6F1700` / `7DFF10` also apply this scale to screen-offset aiming.
- World, culling, and world effect cameras receive the resulting projection
  through the existing source safe-frustum. The HUD retains its layout.
- The first-person pass at `7E0C60` saves the world camera and overwrites its
  vertical angle at `7E0F35` / `7E0F3D` with the fixed float
  `0.8655884265899658`. Native single-player hands and weapon geometry retain their framing
  at 120 degrees. Native ordinary gameplay supplies its independent lens to
  both opaque/deferred geometry and first-person muzzle particles. Native NV2A
  W-depth scaling remains in its original units.
- Local split-screen uses the authored unzoomed biped lens and the original
  grid/span factors for hands, weapons and first-person effects. The single-view
  MCC lens exposed the rocket launcher's rear in wide horizontal panes. The
  source grid ratio also had its axes reversed: TU1.5 `2F7E0` consumes span in
  EDX and grid on the stack, and computes `grid.y * span.x / (span.y * grid.x)`.
  Its factors are 0.5 for horizontal halves, 0.8 for vertical halves and 0.785
  for square cells. The PC world lens remains controlled by the slider.
- The 33 weapon definitions shared by MCC's `shared.map` and the Xbox Lockout
  cache have identical first-person offsets, so no replacement model offsets
  are introduced for the wider world view.

Original observer and replicated vehicle packets remain in source units. The
presentation rebases source zoom through its tangent ratio, preserving scope
magnification and the existing zoom transition owner. The vehicle preference
applies only to an actual seated camera. At 70 it preserves the complete
TU1.5 observer lens and original projection/split-grid factors. Higher settings
scale that lens in tangent space and still use the original vehicle projection;
they are explicit PC customization. Existing saved values are preserved.
Death and scripted cinematic cameras,
and explicit reference capture projections, keep their source lenses. The
ordinary campaign player camera also consumes the on-foot preference; campaign
vehicle interaction remains subject to that runtime's existing support.

`tools/oracle_mcc_gameplay_fov.py` executes unchanged MCC `7DFF10` and `6F4490`
instructions against declared viewport inputs. It supplies the modern-camera
flag as true and host double `tan` / `atan2` services. Its 315 outputs cover the
entire slider range at six viewport aspects and nine zoomed inputs, and are
retained in `tests/fixtures/mcc_gameplay_fov.tsv`. `gameplay_fov_tests` compares
native conversion against those outputs and checks zoom and ultrawide behavior.
This establishes the selected camera math, not full live MCC framebuffer or
camera-transition parity. Native UI and runtime validation is recorded under
`analysis/camera/mcc-fov-20261014/`.

`tools/oracle_mcc_first_person_fov.py` executes the first-person camera override
without hooks for 20 incoming lens/aspect combinations. `gameplay_fov_tests`
also checks its generated `mcc_first_person_fov.tsv` outputs.
`first_person_fov_render_tests` renders the actual posed Lockout starting weapon
and hands with controlled colors at 4:3, 16:9, and 32:9. It requires identical
foreground pixels at 78 and 120, visible geometry, and a different result when
the world lens is deliberately substituted. The gameplay runtime probe also
renders the real textured scene at both settings. Evidence is recorded under
`analysis/camera/mcc-first-person-fov-20261014/`; these checks do not establish
live MCC framebuffer, depth-buffer, or full animation parity.

`tools/oracle_split_first_person_fov_tu15.py` authenticates the English TU1.5
XBE, uniquely locates the matching instructions and executes unchanged
`174BF0` and `2F7E0` without hooked math or game services. Twelve authored
angle/grid/span cases are retained in `tu15_split_first_person_fov.tsv`.
The FOV unit test and source-shaded weapon framebuffer checks compare against
those independent outputs. Actual Coagulation split-screen captures at
2560x1380 and 78/120 degrees are recorded under
`analysis/rocket-clipping-mcc-20261022/`. These establish the clipping fix and
the numeric split lens; they do not certify complete OG framebuffer parity.
