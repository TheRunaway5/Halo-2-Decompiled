# Opening cinematic panel environment admission

The original opening cache uses opaque/tex_bump_env_fast_masked for
panel_mech_ext_window and panel_hall_door_frame. Each authored LOD selects
layer8/implementation0 with the compiled stencil_env world program. The dynamic
preparer admitted bumped and super-fast variants but rejected this existing
variant, preventing the human introduction from rendering its cinematic panels.

The correction admits the existing original program in the independent layer8
pass. It preserves layer3's dynamic lighting choice and layer8-before-layer7
order. No replacement vertex arithmetic, pixel program or lighting value is
supplied. The existing static stencil_env support/evidence is documented in
docs/TU15_CAMPAIGN_ENVIRONMENT_20261013.md; dynamic admission is checked separately.

campaign_cinematic_environment_material_tests checks both materials and both
LODs: selected original program/fragment offsets, vertex code/declaration and
sampler states remain intact. Four bindings pass under the unchanged source-match
guard in intro-panel-material-integration-v3. Native panel pixels and full
opening playback remain separate gates. The earlier diagnostic and failed
binding check are retained in the preceding cohorts.
