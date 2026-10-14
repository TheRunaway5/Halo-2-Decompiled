# MCC gameplay FOV reference outputs

`mcc_gameplay_fov.tsv` and `mcc_gameplay_fov.provenance.json` are generated local
reference artifacts, excluded by the repository's existing fixture policy.
Generate them from the user's installed MCC `halo2.dll` with:

```sh
python tools/oracle_mcc_gameplay_fov.py \
  --dll '/home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll' \
  --fixture tests/fixtures/mcc_gameplay_fov.tsv \
  --report tests/fixtures/mcc_gameplay_fov.provenance.json
```

The generator requires `pefile` and `unicorn`, authenticates the DLL's SHA-256,
and runs original RVAs `7DFF10` and `6F4490`. It supplies the modern-camera flag
and host double tangent/atan2 services. Its 315 cases cover every integer slider
value from 70 to 120 at six viewport aspects, plus nine zoomed angles. Native
`gameplay_fov_tests` tolerates at most `2e-7` radians. See `docs/VIDEO_FOV.md`
for implementation scope and validation boundaries.
