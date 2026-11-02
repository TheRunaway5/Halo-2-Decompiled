# Native campaign frame geometry

The native component implements direction transform `145140`, inverse affine
point `145200`, inverse permutation `218FE0`, frame direction `218880`, inverse
frame point `218960` and framed difference `218EB0`. Retained matrix, object,
node, frame and authored permutation records remain the data owners. Source
read order, signed selectors, aliases, per-coordinate stores and hardware SIMD
rounding/status are compared with unchanged TU1.5 execution. No path, collision
decision or source executable is used as a gameplay substitute.

The [current proof](../analysis/campaign-native-ai-frame-geometry-20261028/proof-20261030.json)
records1,143 original cases:780 completed calls,357 read/write prefixes and six
real missing-source faults. All328 new-owner instruction sites are exercised.
GCC, Clang, actual-core ASan/UBSan, nine deliberately incorrect variants and an
identical independent original repeat pass. The semantic core is unchanged
from the initial draft. The canonical v2 observation fixes the initial alias
input's incomplete mutable output extent; the initial reference remains as
diagnostic evidence.

The private inverse API additionally matches136 completed disjoint original
output profiles. It returns XYZ/status as native values, consumes the same
retained reads, preserves host FP state, publishes the original source FP state
and writes no engine memory. It mounts no synthetic source stack or output
bank. Interrupted private results and arbitrary private/output aliases remain
outside that profile.

All completed source/build/original-execution scratch owners are removed. The
234,809-byte canonical fixture is required reference input; no binary, raw
bank or capture is retained. Regeneration and validation use:

```sh
python3 tools/run_test.py -- .tools/python/bin/python3 -B tools/oracle_native_campaign_ai_frame_geometry_tu15.py --output @HALO2_TEST_SCRATCH@/repeat.json.gz --report analysis/frame-repeat.json --verify tests/fixtures/native_campaign_ai_frame_geometry_tu15_v2.json.gz
python3 tools/run_test.py -- python3 tools/validate_native_campaign_ai_frame_geometry_tu15.py --output analysis/frame-validation
```

The main CMake module compiles the native component. This does not qualify a
numeric World call boundary, actual `1C9F10`/`27B340` search, authored navigation,
complete missions, co-op, native-only closure or launcher delivery.
