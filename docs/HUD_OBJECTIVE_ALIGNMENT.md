# Objective meters and shield/radar alignment

The missing `territory_meter_hahaha` special7 path now draws up to eight
territory meters. Each local view and each territory owns independent shader
state. The original `bomb_defusal` widget now receives visibility bit0x400 and
input67; this meter serves both Assault bomb arming and the local Territories
capture, despite its asset name.

The retained TU1.5 options, player/object pools, engine state and team palette
produce the presentation. `source_hud_objectives_tu15` reads those owners;
`source_hud_objectives` implements the defined portion of disc15F3A0/TU1.5
161E20 and bomb fraction24B430. Territory capture uses the original1/63 factor,
active-player filtering, strict first-player tie ownership for neutral meters,
the maximum active capture for occupied meters, and original gray/pink
fallback colors. Progress reaches the authored shader without an added clamp.
Native snapshot protocol91 carries this per-player state.

## Placement

Both original executables place these objective widgets at the lower center.
The full-screen charge quad is `(250,344)`–`(390,357)` in the640x480 HUD. Its
authored split image is86x9. The territory bitmap is14x11 with one pixel between
meters. The original code centers the row **after** rounding its base quad;
even meter counts consequently retain half-pixel X coordinates. For example,
three full-screen territories start at X298,313 and328, all at Y365.

Shield fills and outlines retain their independent authored registrations.
The full-screen Spartan fill quad is `(47,346)`–`(175,378)` and its outline is
`(47,345)`–`(175,377)`. The Elite pair both use `(48,344)`–`(176,376)`.
The Spartan one-pixel offset is original artwork placement. Selected Vista
fill sampling still uses its extracted artwork correction; it does not move
the source outline or radar. See [the source artwork records](MP_HUD_FUNCTIONALITY.md#vista-shield-alignment-and-full-scoreboard-artwork-2026-10-11).

Native MCC shield pages now receive that authored sampling correction too:
three physical texels upward for the full-screen Spartan fill, and one left
and two upward for the Elite fill. The complete known bitmap tuple must match;
original artwork, split pages, outlines and unknown banks receive no offset.
Explicit artwork packs continue to use their own alignment records. This closes
the dark strip between the full shield and its top outline without moving the
original quads. Fresh source records and red/green checks are retained under
`analysis/shield-gap-20261021/`; the GPU regression checks both species and
overshields at640x480,1280x720 and3840x2160.

The motion-sensor composite remains84x84 for one player and64x64 for split
screen, with its original center relative to the separately authored radar
background. Native widescreen output retains the existing uniform artwork
scale and safe-area adaptation. It does not stretch meters to fill a viewport.

## Evidence and repeatable checks

[The unchanged executable oracle](../tools/oracle_hud_objectives.py) compares
disc and TU1.5 outputs for320 bitmap dispatches,860 objective/shield/radar
background quads and160 objective queries. Query cases include missing
palettes, neutral/owned territory capture, ties, empty entries, absent units,
and bomb arming timing. The C++ regression compares both the pure query and
the retained-memory reader with the original output bits.

The shader/geometry regressions run with original, selected UI and prepared
MCC assets. Vulkan checks exercise visible charge and territory pixels,
individual red/blue territory colors, changing progress and private local-view
materials. The shield/radar framebuffer regression also runs with all three
artwork sources, both species,1–4 local players and640x480/1280x720 surfaces.
It covers shield damage/recharge/overshields and transparent padding, radar
contacts and sweep, and unchanged transparent square corners.

The existing radar viewport oracle was rerun: its80 original source quads
reproduce the stored fixture byte-for-byte. The native regression compares all40
current safe-area cases; the additional40 cases record the alternate overscan
profile. Small source hashes and validation receipts live under
`analysis/objective-hud-20261020/`. Generated binaries and framebuffers are
temporary and removed with the enclosing test scratch.

On2026-10-20 the native build and22 focused CTests passed. The final GPU checks
ran serially (`--test-jobs 1`) to avoid concurrent `xvfb-run` display allocation
and cleanup races. No Vulkan validation errors were reported.

Regenerate the local fixtures from owned original inputs:

```bash
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  .tools/python/bin/python tools/oracle_hud_objectives.py \
  --tu15 /path/to/TU1.5/default.xbe \
  --output analysis/objective-hud-20261020/original.json \
  --geometry tests/fixtures/hud_objective_geometry_original.tsv \
  --queries tests/fixtures/hud_objective_query_original.tsv
python3 tools/run_tests.py --target hud_objective_tests \
  --filter '^original_hud_objectives$'
```

The native match and snapshot admission gates still support Slayer and King.
This change completes the original objective **HUD** consumers and source
publication; it does not implement or certify complete Assault/Territories
gameplay. Original-instruction geometry and Vulkan evidence also do not
certify synchronized physical-Xbox framebuffer equality or a campaign playthrough.
