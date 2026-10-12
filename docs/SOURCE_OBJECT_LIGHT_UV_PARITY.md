# CPU object-light texture coordinates

`source_object_light_uv` implements complete original `D2DC0` using a literal
resident geometry index triple and the second descriptor (zero-based index1,
byte offset `20` hex from the descriptor bank). It includes descriptor `+8`
in the data address; the radiance/incident services independently ignore that
offset. Declaration24 reads float2. Every other source declaration follows
the signed SHORT2 midpoint branch `(short*2+1)/65535`, including its literal
reciprocal word `37800080`. GPU SNORM conversion is not substituted.

Both X branches interpolate C before B. Float2 Y interpolates B before C;
SHORT2 Y interpolates C before B. Each scalar operation rounds independently.
The final source bounds are exactly[-1000,1000]. These are texture query
coordinates and are separate from GPU texture filtering or authored model
compression bounds.

`tools/differential_source_object_light_uv.py` executes unchanged original
`D2DC0` with the independently frozen resident/authored index/data banks from
the object-light stream audit. It also adds explicitly controlled declaration,
unaligned byte-offset, midpoint, extrapolated-weight and clamp-boundary cases.
The corrected finite-input corpus contains22,692 cases across88 raw geometry
banks; all45,384 output words match. All115 executed instruction sites match the
supplied original XBE. No original service is patched or replaced; no GPU or
guest I/O occurs. The authored/resident byte banks are relocated inputs to
this pure routine, rather than a native live residency/pose producer.

Complete `D1E10` object-branch replay exposed the earlier native fixture's
synthetic third-slot placement, which had hidden a wrong descriptor index.
The implementation and frozen raw-bank corpus now select the actual second
descriptor. The whole-call check additionally verifies1,728 original UV pairs
in their actual caller and original resident geometry layout.

Normal CTest `retail_source_object_light_uv` and address/undefined-behavior
checks with leak detection pass. Source, fixture, binaries and logs are pinned
in `analysis/multiplayer-shading/object-light-uv-native-validation.json`.
The original query and geometry provenance is retained in
`tests/fixtures/retail_source_object_light_uv.provenance.json`.
