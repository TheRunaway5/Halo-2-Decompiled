# Local affine pose writes

The internal `matrix(BonePose)` in `model_pose.cpp` now writes rotation×scale
and translation directly. Original `1421F0` produces these banks directly;
there are no intervening identity translation or scale matrix products.
Native identity products added zero terms and changed signed-zero rotation
components before original `142A60` root composition.

Untouched cold factory traces for Triplicate type7 placements0 and5 observe
the local quaternion, original norm and reciprocal, local matrix and root
composition. Native normalized rotation already matches all nine original
rotation words. The first difference was the subsequent identity TRS product:
it erased both local negative-zero words, one of which remains negative zero
after original root composition. The correction changes that operation only.
Quaternion sampling, normalization, hierarchy and transform composition are
unchanged. There are no placement-specific values or signed-zero patches.

The independent untouched `1421F0` differential executes 4,118 cases and
53,534 output words: both actual constructor locals, 4,096 variations in
their observed unit-norm quaternion family and 20 signed-zero translation
cases. The original norm and reciprocal are observed as1 on every case;
neither is hooked or substituted. Power-of-two scales cover positive and
negative scaling without introducing an unrelated matrix-to-basis rounding
boundary. The frozen production fixture contains265 rows and calls actual
`pose_model_node` through its one-root-bone bind path.

Coherent production comparisons require all 49 complete MACH constructor
node0 banks and cached spheres to match the original. All 278 SCEN node0 banks
and cached spheres, and all 415 BLOC cached spheres, remain exact. These are
constructor-state comparisons; later controllers and physics updates retain
their separate admission and clock boundaries.

The differential establishes this recorded unit-norm domain, including both
authored Triplicate locals. It does not establish general quaternion
normalization, hierarchy arithmetic, arbitrary scale roundtrips or hardware
`RCPSS` parity. The current GLM quaternion normalization remains unchanged;
no approximate reciprocal was added or replaced.

Evidence:

- `analysis/multiplayer-shading/machine-initial-matrix-trace-original.json`
- `analysis/multiplayer-shading/model-affine-zero-probe.cpp`
- `analysis/multiplayer-shading/source-affine-pose-original.json`
- `tests/fixtures/retail_source_affine_pose.tsv` and provenance
- `tests/fixtures/retail_machine_initial_bounds.tsv` and provenance
- `analysis/multiplayer-shading/model-affine-ctest.log` (five regular tests)
- `analysis/multiplayer-shading/model-affine-asan-ctest.log` (five ASAN/UBSAN tests)
- `analysis/multiplayer-shading/model-affine-fp-ctest.log` (five shared FP checks)
