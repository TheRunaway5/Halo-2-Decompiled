# Original projectile vector angle

`projectile_impact_angle_original.tsv` contains 520 independently executed
original results: six input binary32 words and one output binary32 word per row.
Its `.provenance.json` identifies the original image, complete function entry,
real static initializer and immutable pi operand. Both generated files remain
local under the repository's existing oracle-payload ignore policy.

Regenerate from the user-supplied retail image in the main checkout:

```sh
.tools/python/bin/python tools/oracle_projectile_impact_angle.py \
  --xbe assets/disc/default.xbe \
  --output tests/fixtures/projectile_impact_angle_original.tsv
cmake --build build --target projectile_impact_angle_tests
ctest --test-dir build -R '^original_projectile_impact_angles$' --output-on-failure
```

The generator requires Python, Capstone and Unicorn. It executes the complete
original routine and CRT calls without service replacements. The source pi
initializer is executed before testing, rather than filling the runtime global
with an assumed value.

The retail image used has SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The same generator was also run against TU1.1 and TU1.5, and their result files
compare byte-for-byte with retail. The native test requires exact output bits.
This covers finite unit normals/directions, grazing contacts and response-angle
thresholds. It does not establish collision, noise, bounce, attachment or GPU
rendering behavior.
