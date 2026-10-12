# Current object PRT contact sampling

The native `source_prt_contact_sample` reproduces complete retail `241C0`
from an explicitly supplied current PRT cache. This is the object branch of
`D1E10`, after collision-to-render attribution. Its cache differs from the
authored PRT definition and the shader constant bank used to draw an object.

Authority is the unchanged `assets/disc/default.xbe`, SHA256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The implementation is in `include/halo2/source_prt_contact.hpp` and
`src/source_prt_contact.cpp`; no cache allocation or query scheduling is
invented by this helper.

## Source selection and arithmetic

`241C0` scans the signed active prefix at `5093E8` over literal 104-byte
records at `4B89B0`. It selects the first **full owner key** match, including
salt. A missing owner leaves both caller outputs untouched. In contrast to
other owner-slot helpers, low16 equality alone does not select this cache.

A matched row supplies incident at `+5C`, multiplied by the literal `.8f`
constant. Its arena handle at `+8` supplies an offset from arena `487B1C`.
The cache-slot argument selects a coefficient offset from the sixteen words at
`+C`, and
a palette count from the sixteen bytes at `+4C`. A palette greater than
`count-1` becomes zero; the source does not clamp negative selectors.
Each palette advances 160 floats. Vertex cluster floats use truncation
toward zero and advance sixteen floats each.

The complete `D1E10` caller passes contact `+1C` (render region) as this slot,
independently of the resident geometry section at contact `+14`. Whole-call
composition checks unequal indices against distinct coefficient banks; see
[the object-light sampler](SOURCE_OBJECT_LIGHT_SAMPLE_PARITY.md).

Three vertex colors evaluate the original ordered PCA products and ambient
addition. The two attributed triangle fractions interpolate those colors,
then each channel clamps to `[0,1]`. The helper preserves SSE operation order
with contraction disabled. It rejects consumed addresses outside the
explicit arena, missing cache prefixes and unsupported cache-slot indexes.
Skipped/missing owners do not demand unconsumed banks.

`source_prt_contact_short_point` also reproduces `D1E10`'s CPU declaration57
conversion: the signed cluster is direct, while each signed coefficient
uses `(SHORT*2+1)/65535`, with the literal float reciprocal and source
rounding order. GPU SHORT input conversion must not replace this CPU
midpoint conversion.

## Independent retained proof

`tools/audit_source_object_prt_snapshot.py` runs complete unchanged `D1E10`
on eight successful diagnostic `14A8E0` attributions from the immutable
tick12474 snapshot. All eight finish without replacing services; each
calls the original `24490`, residency service, `241C0`, geometry color
service and diffuse sampler. The six current cache rows and entire arena
remain unchanged. These are explicit diagnostic queries over actual current
state, rather than captured game query scheduling.

`tools/differential_source_prt_contact.py` then executes complete unchanged
`241C0` over the same eight actual inputs and controlled cache inputs.
The fixture records **1,227 cases / 8,589 exact output words**, including
full salt mismatches, distinct duplicate-owner rows, palette fallback,
negative selectors, cluster truncation, all sixteen cache slots, triangle
fractions outside the unit interval and untouched miss outputs. A separate
fixture preserves all **24 actual CPU SHORT points / 120 decoded words**
entering `241C0` from the original `D1E10` calls.

Reports and binary bank provenance are retained in
`analysis/multiplayer-shading/object-prt-snapshot-original.json`,
`prt-contact-original.json` and `tests/fixtures/retail_source_prt_contact.*`.
The CMake target is `source_prt_contact_tests`; the CTest name is
`retail_source_prt_contact`.

The coherent normal focused checks pass together with the object-vector and
draw-unit-culling helpers (3/3). A separate strict-floating build of the
production helper and cache byte readers passes address/undefined-behavior
sanitizers with leak detection enabled. Logs are `prt-contact-focused-tests.log`
and `prt-contact-asan-tests.log`; the validation manifest preserves their
source, fixture and binary hashes.

## Runtime boundary

This helper consumes the actual current cache and resident PRT points. It
does not create the cache's owner/LOD/region records, refresh its node
coefficients, allocate the arena or acquire resident resources. The complete
`D1E10` native object branch is now composed and verified separately over
explicit geometry, cache and bitmap-service inputs. Its live producers remain
required for activation.
Existing native shader PRT banks and expanded GPU vertices alone do not
establish those inputs or complete gameplay object-light queries.
