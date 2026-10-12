# Object light PRT and baked-light combination

`source_object_light_mix` reproduces the complete object-light combination
region `D247D..D292E` inside original `D1E10`, including its calls to
`24590`, `30BF0` and `131ED0`. It consumes explicit PRT and baked samples;
it does not obtain the current object cache or resident geometry for them.

The authority is unchanged `assets/disc/default.xbe`, SHA256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
Implementation and public inputs are in `src/source_object_light_mix.cpp`
and `include/halo2/source_object_light_mix.hpp`.

Baked input exists only if both original `D33A0` radiance and `D31C0`
incident services succeed. If only one complete sample exists, the source
uses its incident and clamped color directly. If neither exists, it keeps
the supplied original default incident and writes zero lightmap color.

For two samples, each color's source luminance is
`(blue*.11 + green*.59) + red*.3`. If either luminance is at most `.0001`,
the source uses the other sample. Otherwise it adds the colors and combines
their separately normalized incident directions with these luminance
weights. The combined direction uses the original normalization threshold
and scales by the greater of the two original incident magnitudes. Packed
color uses the existing independently checked `131ED0` byte conversion.
Operation order, double-precision x87 magnitude evaluation and intermediate
float spills are preserved; floating contraction is disabled.

`tools/differential_source_object_light_mix.py` executes the unchanged
combination region and all three unchanged callees over **2,128 cases /
8,512 exact words**. The first eight cases also reproduce the combination
inputs and outputs independently observed during complete unchanged
`D1E10` execution over retained tick12474 diagnostic contacts. Controlled
cases cover every input-presence branch, out-of-range color channels,
adjacent luminance and normalization thresholds, zero and opposed incident
directions and incident magnitude retention. No source instruction or
callee result is replaced.

This arithmetic-region proof deliberately supplies the prior services'
outputs. It does not establish native owner registration, current PRT cache
production, geometry residency, `D33A0/D31C0` data production, later material
diffuse sampling or the complete query's status/lifetime behavior. The
CTest is `retail_source_object_light_mix`, and evidence is retained in
`analysis/multiplayer-shading/object-light-mix-original.json` and
`tests/fixtures/retail_source_object_light_mix.*`.
